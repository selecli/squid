#include "h264_streamd.h"
#include "misc.h"
#include "proc.h"
#include "conn.h"
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/select.h>
#include <assert.h>
#include <stdbool.h>

struct glb_val g_val;
struct config g_cfg;
pthread_mutex_t mutex;
bool version = false;
static bool shutdown_flag = 0;

void main_parse_options(int argc, char **argv)
{
	int i = 0;
	while( ( i = getopt(argc, argv, "Dhvt:l:u")) != EOF) {
		switch(i) {
			case 't':
				g_cfg.max_proc_num = atoi(optarg);
				break;
			case 'l':
				g_cfg.log_level = atoi(optarg);
				break;
			case 'v':
				version = true;
				break;  
			case 'D':
				g_cfg.debug = 0; 
				break;
			case 'h':
				version = true;
				break;
			case 'u':
				g_cfg.udta_start = 1;
				break;
		}
	}

}

struct mp4_req *readRequest(int accept_fd)
{
	int len = 0;
	struct mp4_req req; 
	memset(&req, 0, sizeof(struct mp4_req));
	
//	len = recv(accept_fd, &req, sizeof(struct mp4_req), 0);
	len = safe_read(accept_fd, &req, sizeof(struct mp4_req));

	//debug("%d is recv -----------------------------", len);

	if (len != sizeof(struct mp4_req)) 
		return NULL;

	struct mp4_req *alloc_mp4_req = NULL;
	alloc_mp4_req = (struct mp4_req *)malloc(len);
	memcpy(alloc_mp4_req, &req, len);
	return alloc_mp4_req;
}

void * main_loop(void * args)
{
	fatallog("Enter main loop... \n");

	int req_fd; 
	struct sockaddr_in req_addr;    
	socklen_t sock_size;
	struct mp4_req *mp4_ptr;
	struct req_queue_con *add_req = NULL; 

	while(1) {
		if(shutdown_flag){
                        break;
                }
		if ((req_fd = accept(g_val.server_fd, (struct sockaddr *)&req_addr, &sock_size)) > 0) {
			errlog("accept fd : %d\n",req_fd);

			mp4_ptr = readRequest(req_fd); 
			if (mp4_ptr) {
				int ret = set_no_blocking(req_fd);
				if(ret == -1){
					close(req_fd);
					continue;
				}

				add_req = (struct req_queue_con *)malloc(sizeof(struct req_queue_con));
				if(add_req){
					add_req->mp4_ptr = mp4_ptr;
					add_req->fd = req_fd; 
				}       
				pthread_mutex_lock(&mutex);
				queue_flag test = req_queue_test();
				if (test != QUEUE_FULL)
					req_queue_add(add_req);
				else{
                                       errlog("queue is full and close connection : %d",req_fd);
                                       close(req_fd);
                                       free(mp4_ptr);
                                       free(add_req);
                                       add_req = NULL;
                               }

				pthread_mutex_unlock(&mutex);
			}       
			else{
				close(req_fd);
			}
		} 
		
	}
	return NULL;
}

void shut_down()
{
	fatallog("moov_generator stop now ...\n");

	int ret;
	if ((ret=unlink(g_cfg.pid_file)!=0)) 
		do_log(LOG_WARN, "ERR: Unable to remove %s", g_cfg.pid_file);

	close(g_val.server_fd);
	close_log();
}

void set_default()
{
	g_cfg.max_proc_num = 100;
	g_cfg.max_wait_num = 32;
	g_cfg.listen_port = 5001;
	g_cfg.debug = 1;
	g_cfg.log_level = 1;
	g_cfg.udta_start = 0;
	strcpy(g_cfg.pid_file, "/var/run/moov_generator.pid");
	strcpy(g_cfg.listen_ip, "127.0.0.1");
}

void signals_handle(int sig)
{
	fatallog("receive signal : %d\n",sig);

	switch(sig) {
		case SIGSEGV:
		case SIGABRT:
			shutdown_flag = true;
			exit(-1);
			break;
		case SIGINT:
		case SIGTERM:
		case SIGHUP:
			shutdown_flag = true;
			break;
		case SIGUSR1:
			break;
		case SIGUSR2:
			break;
		case SIGCHLD:
			break;
		case SIGQUIT:
			break;
		default:
			break;
	}
}

static void signal_init(void)
{                       
	if( (signal(SIGPIPE, signals_handle) == SIG_ERR) ||
			(signal(SIGINT, signals_handle) == SIG_ERR) ||
			(signal(SIGTERM, signals_handle) == SIG_ERR) ||
			(signal(SIGUSR1, signals_handle) == SIG_ERR) ||
			(signal(SIGUSR2, signals_handle) == SIG_ERR) ||
			(signal(SIGCHLD, signals_handle) == SIG_ERR) ||
			(signal(SIGQUIT, signals_handle) == SIG_ERR) ||
			(signal(SIGSEGV, signals_handle) == SIG_ERR) ||
			(signal(SIGABRT, signals_handle) == SIG_ERR) ||
			(signal(SIGHUP, signals_handle) == SIG_ERR) ) {
		errlog("set signal handler failed!");
	}
}       

void printf_usage(void) 
{
	fprintf(stderr, "version 2.0.1\n");
	fprintf(stderr, "usage:\n");
	fprintf(stderr, "-D daemon mode\n");
	fprintf(stderr, "-v version message\n");
	fprintf(stderr, "-t thread pool size\n");
	fprintf(stderr, "-l log level [1..3] default 1\n");
	fprintf(stderr, "-u 1 start udta start write \n");
	fprintf(stderr, "./moov_generator -t 200 -D \n");
	exit(0);
}

void writepidFile(void)
{
	FILE *fd;
	char pid[128];
	char buff[128];
	FILE  *procfd;
	char *pos = NULL; 
	int pidnum; 

	if ((fd = fopen(g_cfg.pid_file, "r")) != NULL) { 
		fgets(pid, 128, fd);
		pidnum = atoi(pid);
		snprintf(buff, 128, "/proc/%s/status", pid);
		if ((procfd = fopen(buff,"r")) != NULL) { 
			memset(pid, 0, 128);
			fgets(pid, 128, procfd);
			pid[strlen(pid)] = '\0'; 
			if ((pos = strstr(pid, "moov_generator")) != NULL) { 
				printf("%d has run\n",pidnum);
				exit(0);
			}       

			fclose(procfd);
		}       

		fclose(fd);
	}

	if ((fd = fopen(g_cfg.pid_file, "w+")) ==NULL) 
	{
		printf("create moov_generator error, %s\n", strerror(errno));
		exit(-1);
	}

	pid_t mypid;
	mypid = getpid();
	char trans[128];
	sprintf(trans, "%d", mypid);
	fputs(trans, fd);
	fclose(fd);
}

int main (int argc, char **argv)
{
	set_default();

	main_parse_options(argc, argv);	/* parse command options */
	
	if(version)
		printf_usage();
	
	if (!g_cfg.debug)
		daemon_handler();

	pthread_mutex_init(&mutex, NULL);
	if ((g_val.server_fd = server_init(g_cfg.listen_port, g_cfg.listen_ip)) < 0) {
		errlog("main: can't init h264_streamd!\n");
		return -1;
	}

	signal_init();
	queue_init();	
	int ret = 0;
	pthread_t p;

	writepidFile();	

	if ((ret = pthread_create(&p, NULL, main_loop, &g_val.server_fd)) < 0){
		debug("create thread error. main_loop\n");
		return -1;
	}
	
	int i = 0; 
	int n = 0;
	if(g_cfg.max_proc_num)
		n = g_cfg.max_proc_num;
	else 
		n = 100;
	for(i = 0; i < n; i ++) {
		if ((ret = pthread_create(&p, NULL, recv_handler, NULL)) < 0)
			debug("create thread error. recv_handler \n");
	}

	while (1) {
		if(shutdown_flag){
			break;
		}

		sleep(1);
	}

	shut_down();
	return 1;
}

