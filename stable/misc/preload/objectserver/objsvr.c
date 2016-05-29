#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE 

#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <signal.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <strings.h>
#include <malloc.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <sys/epoll.h>

#define VERSION		"2008.7.8"
#define POLL_MAX_FD_SIZE 200
#define sys_error (strerror(errno))
#define FILE_HDR_SUFFIX	"^header"
#define BUF_SIZE	4096

struct conn_state_st {
	int fd;	//socket
	FILE * filefp;
	char filename[BUF_SIZE];
	time_t start_ts;
	char buf[BUF_SIZE];
	int header_ok;
	unsigned int send_header_bytes;	//统计值
	unsigned int send_body_bytes;
};

struct conn_state_st conn_states[POLL_MAX_FD_SIZE];
int g_listen;
static char *ip;
static int port;
int g_conn_count;


static struct pollfd poll_fds[POLL_MAX_FD_SIZE];
static struct pollfd poll_fds_bak[POLL_MAX_FD_SIZE];


static void commSetReuseAddr(int fd)
{
	int on = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on)) < 0)
		fprintf(stderr, "commSetReuseAddr: FD %d: %s\n", fd, strerror(errno));
}
//-------------------------------------------------------------------
//-------------------------------------------------------------------
void safe_free_p(void ** ptr)
{
	if (ptr) {
		if (*ptr)
			free(*ptr);
		*ptr = NULL;
	}
}


/*
 *  * flag:
 *   * 		begin is 0
 *    * 		end is 1
 *     */
void timestamp(int flag)
{
	struct timeval tv;
	struct timezone tz;
	static long start, end;

	if (flag == 0) {
		gettimeofday(&tv, &tz);
		start = tv.tv_sec * 1000 + tv.tv_usec / 1000;
		return;
	} else if (flag == 1) {
		gettimeofday(&tv, &tz);
		end = tv.tv_sec * 1000 + tv.tv_usec / 1000;
		printf("__________set time stamp_____cost %lu milli-second\n",
				end - start);
		return;
	}
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void * safe_calloc(int size)
{
	void * buf = NULL;
	while (buf == NULL) {
		buf = calloc(1, size);
		if (buf == NULL)
			usleep(1000);
	}
	return buf;
}


//-------------------------------------------------------------------
//-------------------------------------------------------------------
int safe_write(int fd, void *buffer,int length)
{
	int bytes_left = 0;
	int written_bytes = 0;
	char *ptr=NULL;

	ptr=buffer;
	bytes_left = length;

	if (length == 0)
		return 0;	

	int retry_times = 0;
	
	while (bytes_left > 0) {
		written_bytes = write(fd, ptr, bytes_left);
		if (written_bytes == 0) {
			fprintf(stderr, "connection has gone!\n");
			return length - bytes_left;
		}
		if(written_bytes < 0) {       
			if(errno == EINTR) {
				usleep(100000);
				continue;
			} else {
				//fprintf(stderr, "fwrite error(%d)\n", errno);
				retry_times++;
				if (retry_times < 100) {
					usleep(100000);
					continue;
				} else {
					return -1;
				}
			}
		}       
		bytes_left -= written_bytes;
		ptr += written_bytes;
	}
	return length;
}



//-------------------------------------------------------------------
//-------------------------------------------------------------------
void update_event(int fd, int event)
{
	if (0 == event) {
		poll_fds_bak[fd].fd = -1;
		poll_fds_bak[fd].events = 0;
		poll_fds_bak[fd].revents = 0;
	} else {
		poll_fds_bak[fd].fd = fd;
		poll_fds_bak[fd].events = event;
		poll_fds_bak[fd].revents = 0;
	}
}


//-------------------------------------------------------------------
//-------------------------------------------------------------------
void close_sock(int fd)
{
	/*	printf("Send (fd=%d)header %d bytes, body %d bytes! close it\n", fd, 
			conn_states[fd].send_header_bytes, conn_states[fd].send_body_bytes);
	*/
	if (fd == -1)
		return;
	close(fd);
	conn_states[fd].fd = -1;

	if (conn_states[fd].filefp)
		fclose(conn_states[fd].filefp);
	conn_states[fd].filefp = NULL;

	memset(conn_states[fd].buf, 0, BUF_SIZE);
	memset(conn_states[fd].filename, 0, BUF_SIZE);


	conn_states[fd].send_header_bytes = 0;
	conn_states[fd].send_body_bytes = 0;
	conn_states[fd].header_ok = 0;
	conn_states[fd].start_ts = 0;
	update_event(fd, 0);	//在poll中update会影响poll数组
	g_conn_count--;
}
//-------------------------------------------------------------------
//-------------------------------------------------------------------
char * get_filename(char * mybuf, char * mypath)
{
	char *mystr = NULL; 
	char *temp = NULL;
	int len=0;

	//FIXME:浣跨敤TrimLeft TrimRight
	mystr = strstr(mybuf, "X-Local-Object: ");
	if (NULL == mystr) {
		fprintf(stderr, "local-object path has error,please check\n");
		return NULL;
	}
	temp = mystr;
	while (*temp != '\r') 
		temp++;
	*temp = 0;
	len = temp - mystr - strlen("X-Local-Object: ");
	strncpy(mypath, mystr+strlen("X-Local-Object: "), len);
	//fprintf(stderr, "Xcell Path=%s\n", mypath);
	return mypath;
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
static void read_request(int fd)
{
	char buf[4096];
	int read_len = 0;
	int offset = 0;

	offset = strlen(conn_states[fd].buf);

	assert(fd > 0);
	
	//timestamp(0);	
	//fprintf(stderr, "test read from squid time cost\n");
	read_len = read(fd, buf, 4096);
	//fprintf(stderr, "read request:\n%s\n", buf);
	//timestamp(1);

	if (read_len <= 0) {
		close_sock(fd);
	} else {
		if (strlen(conn_states[fd].filename) == 0) {
			//append buf
			strcat(conn_states[fd].buf, buf);

			//get file name
			memset(conn_states[fd].filename , 0, BUF_SIZE);
			get_filename(buf, conn_states[fd].filename);
			if (strlen(conn_states[fd].filename) != 0) {
				char hdr_file[2048];
				strcpy(hdr_file, conn_states[fd].filename);
				strcat(hdr_file, FILE_HDR_SUFFIX);
				conn_states[fd].filefp = fopen(hdr_file, "r");
				if (conn_states[fd].filefp  == NULL) {
					fprintf(stderr, "Open local object file %s(%lu) error\n", 
							hdr_file, (unsigned long)strlen(hdr_file));
					close_sock(fd);
					return;
				} else {
					/* printf("open local file fd=%p for %s\n", 
							conn_states[fd].filefp, hdr_file); */
				}
			}
		} else {
			//do nothing
		}
	}
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
static void write_reply(int fd)
{
	char buf[4096];
	int len = 0;
	int rtn = 0;

	if (fd == -1)
		return;

	//timestamp(0);
	//fprintf(stderr, "test fread from large file time cost\n");
	//add by sxlxb 1224
	if (conn_states[fd].filefp == NULL) {
		close_sock(fd);
		return;
	}
	assert(conn_states[fd].filefp); //add by sxlxb 1224
	len = fread(buf, 1, 4096, conn_states[fd].filefp);
	//timestamp(1);
	
	if (len <= 0) {
		//read over
		if (conn_states[fd].header_ok == 0) {
			conn_states[fd].header_ok = 1;
			assert (conn_states[fd].filefp != NULL);
			fclose(conn_states[fd].filefp);
			conn_states[fd].filefp = fopen(conn_states[fd].filename, "r");
			if (conn_states[fd].filefp == NULL) {
				goto out;
			} else {
				return;
			}
		} else {
			goto out;
		}
	}

	//timestamp(0);	
	//fprintf(stderr, "test safe_write to squid time cost\n");
	rtn = safe_write(fd, buf, len);
	if (rtn < 0 || rtn != len) {
		fprintf(stderr, "\nLINE=%d,(fd=%d) write failed, errno=%s\n", __LINE__, fd, sys_error);
		goto out;
	}
	//timestamp(1);	
	if (conn_states[fd].header_ok == 0) {
		conn_states[fd].send_header_bytes += rtn;
	} else {
		conn_states[fd].send_body_bytes += rtn;
	}
	return;
out:
	close_sock(fd);
	conn_states[fd].filefp = NULL;
}



//-------------------------------------------------------------------
//-------------------------------------------------------------------
int core_loop()
{
	int num;
	int i;

	//timestamp(0);	
	//fprintf(stderr, "test poll time cost\n");
	memcpy(poll_fds, poll_fds_bak, sizeof(poll_fds));
	num = poll(poll_fds, POLL_MAX_FD_SIZE, 5000);
	//timestamp(1);

	if (num == -1) {
		return -1;
	} else if(num == 0) {
		return 0;
	} else {
		for(i=0; i< POLL_MAX_FD_SIZE; i++) {
			if(poll_fds[i].fd < 0) {
				if (poll_fds[i].revents) {
					//printf("Bug:event=%x\n", poll_fds[i].revents);
				}
				continue;
			}
			int read_event = poll_fds[i].revents & POLLIN;
			int write_event = poll_fds[i].revents & POLLOUT;
			int error_event = poll_fds[i].revents & POLLERR;
			int hup_event = poll_fds[i].revents & POLLHUP;
			if( (error_event != 0) || (hup_event != 0) ) {
				close_sock(i);
				continue;
			}
			if(read_event) {
				if (poll_fds[i].fd == g_listen) {
					//handle listen fd
					int fd = accept(g_listen, NULL, 0);
					//printf("Accept fd=%d\n", fd);
					g_conn_count++;
					if (g_conn_count > POLL_MAX_FD_SIZE-10) {
						fprintf(stderr, "preload task too many!, try later, close fd=%d\n", fd);
						close_sock(fd);
						continue;
					}
					conn_states[fd].fd = fd;
					conn_states[fd].filefp = NULL;

					memset(conn_states[fd].buf, 0, BUF_SIZE);
					memset(conn_states[fd].filename, 0, BUF_SIZE);

					update_event(fd, POLLIN|POLLOUT|POLLHUP|POLLERR);

					//handle request
					read_request(fd);
				} else {
					//handle squid tcp
					read_request(poll_fds[i].fd);
				}
			}
			if (write_event)  {
				if (poll_fds[i].fd > 0)
					write_reply(poll_fds[i].fd);
			} 
		}
		return num;
	}	
}


//-------------------------------------------------------------------
//-------------------------------------------------------------------
static void usage(void)
{
	fprintf(stderr, "Usage: ihttpd ip port, version %s\n", VERSION);
	exit(1);
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
static void parse_options(int argc, char **argv)
{
	if(argc != 3)
		usage();
	ip = strdup(argv[1]);
	port = atoi(argv[2]);
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
int listen_init()
{
	struct sockaddr_in serv;
	int listen_fd = -1;
	
	if(-1 == (listen_fd = socket(AF_INET, SOCK_STREAM, 0))) {
		return -1;
	}
	g_listen = listen_fd;

	assert(port > 0);
	assert(NULL != ip);
	serv.sin_port = htons(port);
	serv.sin_family = AF_INET;
	if(0 == inet_pton(AF_INET, ip, &serv.sin_addr)) {
		return -1;
	}
	
	commSetReuseAddr(listen_fd);
	
	if(-1 == bind(listen_fd, (struct sockaddr *)&serv, sizeof(serv))) {
		return -1;
	}
	if(-1 == listen(listen_fd, 5)) {
		return -1;
	}

	//setnonblocking(listen_fd);
	update_event(listen_fd, POLLIN | POLLERR | POLLHUP);
	return 0;
}


//-------------------------------------------------------------------
//-------------------------------------------------------------------
int init(void)
{
	int rtn = 0;
	rtn = listen_init();
	if (rtn == -1) {
		fprintf(stderr, "object server:create listen socket error\n");
		return -1;
	}
	signal(SIGPIPE, SIG_IGN); 
	return 0;
}


//-------------------------------------------------------------------
// only for test large file support
//-------------------------------------------------------------------
void test_large_file()
{
	FILE * fp = fopen("./test.3g", "rb");
	if (fp == NULL) {
		printf("Open error\n");
		return;
	}
	unsigned int total_len = 0;
	int len = 0;
	int rtn = 0;
	while (1) {
		char buf[4096000];
		rtn = fseeko(fp, total_len, SEEK_SET);
		printf("fseeko: %d (%u)\n", rtn, total_len);
		len = fread(buf, 1, 4096000, fp);
		if (len < 0) {
			printf("read len=%d\n", len);
		} else {
			total_len += len;
			printf("total_len=%u\n", total_len);
		}
	}
}
//-------------------------------------------------------------------
//-------------------------------------------------------------------
int struct_init()
{
	int i=0;
	for (i=0; i<POLL_MAX_FD_SIZE ; i++) {
		update_event(i, 0);
	}
	return 0;
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
int main(int argc, char ** argv)
{
	struct_init();

	parse_options(argc, argv);

	if (init() == -1) 
		exit(-1);

	pid_t pid = fork();
	if (pid > 0)
		exit(0);

	while (1) {
		core_loop();
	}
}
//-------------------------------------------------------------------
//-------------------------------------------------------------------
