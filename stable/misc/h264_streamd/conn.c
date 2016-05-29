#include "h264_streamd.h"
#include "conn.h"
#include "proc.h"
#include "moov.h"
#include "misc.h"
#include "log.h"
#include "mp4_io.h"
#include "mp4_process.h"
#include "moov.h"
#include "output_bucket.h"
#ifdef BUILDING_H264_STREAMING
#include "output_mp4.h"
#define X_MOD_STREAMING_KEY X_MOD_H264_STREAMING_KEY
#define X_MOD_STREAMING_VERSION X_MOD_H264_STREAMING_VERSION
#endif
#ifdef BUILDING_FLV_STREAMING
#include "output_flv.h"
#endif
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
		do_log(LOG_ERR, "server_init: can't create new sock!\n");
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
		printf("moov_generator already running...\n");
		do_log(LOG_ERR, "server_init: can't bind server socket!\n");
		goto err;
	}
	
	printf("Front-end running...\n");
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

	do_log(2,"send to %d success len = %d!\n",fd,length);
	return length;
}


/*
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
*/

int moov_data_handler(int fd, struct mp4_req  *req)
{
	FILE *infile;
	uint64_t filesize = 0;
	infile = fopen(req->obj_path, "rb");   //¶ÁÈëmp4ÎÄ¼þ
	if (!infile){
		fclose(infile);
     		goto err;
	}
	
	fseek(infile, 0, SEEK_END);
	filesize = ftell(infile);
	fclose(infile);

	{
		//unsigned int mdat_start = req->mp4_offset;
		mp4_split_options_t* options = mp4_split_options_init();
		options->file_offsets = req->mp4_offset;
#if 1
		/*QQVIDEO_ON_DEMAND */
		if(req->start < 0)
			req->start = 0;

		if(req->end < req->start)
			req->end = 0;
#endif	
		options->start = req->start;
		options->start_integer = req->start;
		options->end = req->end;

		if (strstr(req->url,"dezhi.com") != NULL)
			options->get_keyframe_flag = 1;

		//mp4_split_options_set(options,"start=448.8",11);

		//print if debug level >=2 
		debug("mp4 req->url [%s] \n",req->url);
		debug("mp4 options->get_keyframe_flag [%s]\n",options->get_keyframe_flag==0?"get the frame Before keyframe":"get the frame After keyframe");
		debug("mp4 req->start %d \n",req->start);
		debug("mp4 req->mp4_offset %d \n",req->mp4_offset);
		debug("mp4 start_integer %lld \n",options->start_integer);
		debug("mp4 start_float %f \n",options->start);
		debug("mp4 end %f \n",options->end);

		struct bucket_t* buckets = 0;
		int verbose = 0; //debug_level

		int http_status = mp4_process(req->obj_path, filesize, verbose, &buckets, options);

		mp4_split_options_exit(options);

		if(http_status != 200)
		{
			if(buckets)
			{
				buckets_exit(buckets);
			}	

			debug("mp4 status %d \n",http_status);

			goto err;
		}

		if(buckets != 0){
			debug("mp4 process is success\n");
		}

		struct bucket_t* bucket = buckets;

		struct mp4_rpl rpl;
		long len = 0;

		memset(&rpl, 0, sizeof(rpl));

		if(bucket)
		{
			do
			{

				switch(bucket->type_)
				{
					case BUCKET_TYPE_MEMORY:
						debug("memory size %lld\n", bucket->size_);
						rpl.len += bucket->size_;
						break;
					case BUCKET_TYPE_FILE:
						rpl.mdat_size =  bucket->size_   - ATOM_PREAMBLE_SIZE;
						rpl.mdat_start = bucket->offset_ + ATOM_PREAMBLE_SIZE;
						rpl.mdat_size =  bucket->size_ ;
						rpl.mdat_start = bucket->offset_; 

						debug("memory offset %lld\n", rpl.mdat_start);
						debug("memory size   %lld\n", rpl.mdat_size);
						rpl.len += bucket->size_;
						break;
				}
				bucket = bucket->next_;

			} while(bucket != buckets);
			//} while(bucket != 0);
			//buckets_exit(buckets);
	}

   		debug("content length %lld\n", rpl.len);
		
		send_data(fd, &rpl, sizeof(rpl));
		{	
			
			do
        		{

          			switch(bucket->type_)
         			{
          			case BUCKET_TYPE_MEMORY:
					len += send_data(fd, bucket->buf_, bucket->size_);

            	 			break;
          			case BUCKET_TYPE_FILE:
					//len += send_data(fd, moov_data, moov_atom.size_);
            				break;
         			}
		  		bucket = bucket->next_;

       		 	} while(bucket != buckets);
        		//} while(bucket != 0);
        		buckets_exit(buckets);

			free(req);
			req = NULL;
			
		}
	}
	return 1;
err:
	do_log(LOG_DEBUG, "moov_data_handler: moov_seek erro!\n");
	if (req != NULL) {
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
			//errlog("recv_handler:close %d\n\n", handle_ptr->fd);

			free(handle_ptr);
			handle_ptr = NULL;
		}

		usleep(100000);
	}
	assert(0);
}


