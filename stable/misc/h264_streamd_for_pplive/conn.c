#include "h264_streamd.h"
#include "conn.h"
#include "proc.h"
#include "moov.h"
#include "misc.h"
#include "log.h"
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
int set_no_blocking(int fd)
{
	int flags;
	int dummy = 0;

	if ((flags = fcntl(fd, F_GETFL, dummy)) < 0) {
		do_log(LOG_ERR, "set_no_blocking: %d fcntl F_GETFL: %s\n", fd, xstrerror());
		return -1;
	}

	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
		do_log(LOG_ERR, "set_no_blocking: FD %d: %s\n", fd, xstrerror());
		return -1;
	}
	return 1;
}


int server_init(short server_port, char *ip)
{
	int server_fd;
	int ret;
	struct sockaddr_in s;

	if ((server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
		do_log(LOG_ERR, "server_init: can't create new sock!");
		goto err;
	}

	memset(&s, 0, sizeof(struct sockaddr_in));
	s.sin_addr.s_addr = inet_addr(ip);
	s.sin_port = htons((int)g_cfg.listen_port);
	s.sin_family = AF_INET;
	int option = 1;
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
	if ((ret = bind(server_fd, (struct sockaddr *) &s, sizeof(s))) != 0)
	{
		do_log(LOG_ERR, "server_init: can't bind server socket!");
		goto err;
	}
	
	/*
	if (set_no_blocking(server_fd))
	{
		do_log(LOG_ERR, "server_init: can't setoptions no_blocking!");
		goto err;
	}			
	*/

	if (listen(server_fd, 15)) 
	{
		do_log(LOG_ERR, "server_init: Can't listen on socket");
		goto err;
	}	
	
	return server_fd;
err:
	if (server_fd)
		close(server_fd);
	return COMM_ERR;
}

int safe_read(int fd,void *buffer,int length)
{
	int bytes_left = length;
	int bytes_read = 0;
	char * ptr = buffer;
	int retry_times = 0;

	while (bytes_left > 0) {
		bytes_read = recv(fd, ptr, bytes_left, 0);
//		bytes_read = read(fd, ptr, bytes_left);
		if (bytes_read < 0) {
			if((errno == EINTR) || (errno == EAGAIN)) {
				usleep(50000);
				retry_times++;

				//timeout 5s
				if(retry_times > 100)
					return -1;
			} else 
				return -1;
		} else if (bytes_read == 0){
			//peer close the socket
			return 0;
		}
		else{
			bytes_left -= bytes_read;
			ptr += bytes_read;
		}
	}

	return length - bytes_left;
}


int send_data(int fd, void *buffer,int length) 
{
	int bytes_left = 0;
	int written_bytes = 0;
	char *ptr=NULL;
	int retry_times = 0;
	int buf_size = 8192;
	ptr=buffer;
	bytes_left = length; 

	if (length == 0)
		return 0;   

	while (bytes_left > 0) {

		if(bytes_left < 8192){
			buf_size = bytes_left;
		}

		written_bytes = write(fd, ptr, buf_size);

		if (written_bytes == 0)
			return length - bytes_left;

		if(written_bytes < 0) {       
			if((errno == EINTR) || (errno == EAGAIN)) {
				usleep(500000);
				retry_times++;

				//timeout 5s
				if(retry_times > 100){
					errlog("retry 100 ,write timout !! %d ",length);
					return -1;
				}
			} else {
				errlog("write to %d error %d,%s\n",fd,errno,strerror(errno));
				return -1;
			}
		}
		else{
			bytes_left -= written_bytes;
			ptr += written_bytes;
		}
	}

//	do_log(2,"send to %d success len = %d!",fd,length);
	return length;
}

void write_char(unsigned char* outbuffer, int value)
{
  outbuffer[0] = (unsigned char)(value);
}

void write_int32(unsigned char* outbuffer, long value)
{
  outbuffer[0] = (unsigned char)((value >> 24) & 0xff);
  outbuffer[1] = (unsigned char)((value >> 16) & 0xff);
  outbuffer[2] = (unsigned char)((value >> 8) & 0xff);
  outbuffer[3] = (unsigned char)((value >> 0) & 0xff);
}

struct atom_t
{
  unsigned char type_[4];
  uint64_t size_;
  uint64_t start_;
  uint64_t end_;
};

#define ATOM_PREAMBLE_SIZE 8

unsigned int atom_header_size(unsigned char* atom_bytes)
{
  return (atom_bytes[0] << 24) +
         (atom_bytes[1] << 16) +
         (atom_bytes[2] << 8) +
         (atom_bytes[3]);
}

int atom_read_header(FILE* infile, struct atom_t* atom)
{
  unsigned char atom_bytes[ATOM_PREAMBLE_SIZE];

  atom->start_ = ftell(infile);

  fread(atom_bytes, ATOM_PREAMBLE_SIZE, 1, infile);
  memcpy(&atom->type_[0], &atom_bytes[4], 4);
  atom->size_ = atom_header_size(atom_bytes);
  if(atom->size_ <= 0 ){
	errlog("atom size<=0 and return 0 %d",atom->size_);	
	return 0;
  }
  atom->end_ = atom->start_ + atom->size_;

  return 1;
}

void atom_write_header(unsigned char* outbuffer, struct atom_t* atom)
{
  int i;
  write_int32(outbuffer, atom->size_);
  for(i = 0; i != 4; ++i)
    write_char(outbuffer + 4 + i, atom->type_[i]);
}

int atom_is(struct atom_t const* atom, const char* type)
{
  return (atom->type_[0] == type[0] &&
          atom->type_[1] == type[1] &&
          atom->type_[2] == type[2] &&
          atom->type_[3] == type[3])
         ;
}

void atom_skip(FILE* infile, struct atom_t const* atom)
{
  fseek(infile, atom->end_, SEEK_SET);
}

void atom_print(struct atom_t const* atom)
{
  //printf("Atom(%c%c%c%c,%lld)\n", atom->type_[0], atom->type_[1],
  //        atom->type_[2], atom->type_[3], atom->size_);
}


int moov_data_handler(int fd, struct mp4_req  *req)
{
	FILE *infile;
	struct atom_t ftyp_atom = {"",0,0,0};
	struct atom_t moov_atom = {"",0,0,0};
	struct atom_t mdat_atom = {"",0,0,0};
	unsigned char *moov_data = NULL;
	unsigned char *ftyp_data = NULL;

	unsigned char* new_moov_data = NULL;

	uint64_t filesize = 0;
	uint64_t moov_datasize = 0;

	infile = fopen(req->obj_path, "rb");   //¶ÁÈëmp4ÎÄ¼þ
	if (!infile){
		fclose(infile);
     		goto err;
	}

	{
		fseek(infile, 0, SEEK_END);
		filesize = ftell(infile);
		fseek(infile, req->mp4_offset, SEEK_SET);

		struct atom_t leaf_atom;

		while(ftell(infile) < filesize)//¶ÁÈ¡ftyp²¿·Ö£¬moov²¿·Ö£¬ºÍmdat²¿·ÖµÄÍ·ºÍÊý¾Ý
		{
			//debug("%s position : %ld,len %lld",req->obj_path,ftell(infile),filesize);
			//errlog("%s position : %ld,len %ld",req->obj_path,ftell(infile),filesize);

			if(!atom_read_header(infile, &leaf_atom))
				break;

			atom_print(&leaf_atom);

			if(atom_is(&leaf_atom, "ftyp"))
			{
				ftyp_atom = leaf_atom;
				ftyp_data = malloc(ftyp_atom.size_);
				fseek(infile, ftyp_atom.start_, SEEK_SET);
				fread(ftyp_data, ftyp_atom.size_, 1, infile);
			}
			else if(atom_is(&leaf_atom, "moov"))
			{
				moov_atom = leaf_atom;
				moov_data = malloc(moov_atom.size_);
				fseek(infile, moov_atom.start_, SEEK_SET);

				moov_datasize = fread(moov_data, moov_atom.size_, 1, infile);
				if(moov_datasize != 1){
					errlog("fread moov_data error ! (%d), %s",moov_datasize,strerror(errno));
					fclose(infile);
					goto err;
				}
			}
			else if(atom_is(&leaf_atom, "mdat"))
			{
				mdat_atom = leaf_atom;
			}

			atom_skip(infile, &leaf_atom);
		}
	}
	fclose(infile);

	if (!moov_data || moov_data[0]=='0'){
		errlog("moov data is null !!");
		goto err;
	}

	{
		unsigned int mdat_start = (ftyp_data ? ftyp_atom.size_ : 0) + moov_atom.size_;
		mdat_start += req->mp4_offset;
	
  		//do_log(2,"req->mp4_offset is  %d ",req->mp4_offset);
  		//do_log(2,"req->mp4_offset1 is  %ld ",mdat_start-mdat_atom.start_);
  		//do_log(2,"req->mp4_offset2 is  %ld ",mdat_start);
  		//do_log(2,"req->mp4_offset3 is  %ld ",mdat_atom.start_);

		if (moov_seek(moov_data,
			moov_atom.size_,
			req, 0, &mdat_atom.start_, &mdat_atom.size_, mdat_start-mdat_atom.start_)< 0 ) {
			goto err;
		}
		
		{	
			{
				//remove the free atom of moov
				uint64_t new_moov_datasize = 0;
				new_moov_data = rm_freeatom(moov_data,moov_atom.size_,&new_moov_datasize);

				struct mp4_rpl rpl;
				unsigned char mdat_bytes[ATOM_PREAMBLE_SIZE];
				long len = 0;
			
				memset(&rpl, 0, sizeof(rpl));
				atom_write_header(mdat_bytes, &mdat_atom);

				//rpl.len = ftyp_atom.size_ + moov_atom.size_ + mdat_atom.size_;			
				rpl.len = ftyp_atom.size_ + new_moov_datasize + mdat_atom.size_;
				
  				//do_log(2,"content length is  %ld ",rpl.len);

				rpl.mdat_size = mdat_atom.size_ - ATOM_PREAMBLE_SIZE;
				rpl.mdat_start = mdat_atom.start_ + ATOM_PREAMBLE_SIZE; 

  				//do_log(2,"new moov size is  %ld ",new_moov_datasize);
  				//do_log(2,"old moov size is  %ld ",moov_atom.size_);

				len = send_data(fd, &rpl, sizeof(rpl));
				if(len != sizeof(rpl)) goto err;

				len = send_data(fd, ftyp_data, ftyp_atom.size_);
				if(len != ftyp_atom.size_) goto err;

				//len = send_data(fd, moov_data, moov_atom.size_);
				//if(len != moov_atom.size_) goto err;
				
				len = send_data(fd, new_moov_data, new_moov_datasize);
				if(len != new_moov_datasize) goto err;

				len = send_data(fd, mdat_bytes, ATOM_PREAMBLE_SIZE);
				if(len != ATOM_PREAMBLE_SIZE) goto err;
			}
		}
	}

err:
	if (ftyp_data) {
		free(ftyp_data);
		ftyp_data = NULL;
	}
	if (new_moov_data) {
		free(new_moov_data);
		new_moov_data = NULL;
	}
	if (moov_data) {
		free(moov_data);
		moov_data = NULL;
	}
	if(req){
		free(req);
		req = NULL;
	}

	return -1;
}

void * recv_handler(void *args)
{
	for(; ;) {
		struct req_queue_con *handle_ptr = NULL; //add by sxlxb
		pthread_mutex_lock(&mutex);
		queue_flag test = req_queue_test();
		if (test != QUEUE_EMPTY) {
			handle_ptr = req_queue_del();
		}
		pthread_mutex_unlock(&mutex);

		if (handle_ptr) {
			//debug("handle_ptr->fd: %d\n", handle_ptr->fd);

			moov_data_handler(handle_ptr->fd, handle_ptr->mp4_ptr);

			close(handle_ptr->fd);
			//debug("recv_handler:close %d\n\n", handle_ptr->fd);

			free(handle_ptr);
			handle_ptr = NULL;
		}

		usleep(100000);
	}
	assert(0);
}


