#ifndef DEFINE_H
#define DEFINE_H 

#define MAX_PATH_LEN	256
#define MAX_URL_LEN	256
#define BUF_SIZE	1024

typedef void PF(int, void *);

enum {
	REQ_OFFSET,
	REQ_PERCENT
};

struct mp4_req {
	char obj_path[MAX_PATH_LEN];
	int mp4_offset;
	char url[MAX_URL_LEN];
	int start;
	int end; 
	int type;//1 start seconds 2 percent 3 offset
};

struct mp4_rpl {
	uint64_t len;
	uint64_t mdat_start;
	uint64_t mdat_size;
};

struct config {
	int max_proc_num;
	int max_wait_num;
	int listen_port;
	char pid_file[MAX_PATH_LEN];
	char listen_ip[32];
	int debug;
	int log_level;
	int udta_start;
};

struct glb_val {
	int cur_proc_num;
	int clean;	/* clean flag before exit */	
	int server_fd;
	int child;
};


#endif
