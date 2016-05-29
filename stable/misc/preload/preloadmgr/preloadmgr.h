#ifndef PRELOADMGR_MACRO_H
#define PRELOADMGR_MACRO_H

#define HDR_VALUE_LEN     1024
#define MAX_URL_BUF_SIZE  4096
#define MAX_COMM_BUF_SZIE 512

struct config_st {
	int number;
	char *listname;
	char *command;
	int version;
	int help;
	int retry;
	int verbose;
	int subthreads;
	int count;
	int gettaskCounts;
};

struct thread_task_st {
	int myvalue;
	int myhandlenumber;
	int subthreads;
	pthread_t pid;
};

/* add by xin.yao:2011-11-19 */
enum {
	TASKLIST_ERR = -1,   //error in handling
	TASKLIST_EMPTY = 0,  //no task remained, exit now
	TASKLIST_MORE,		 //have more task
	TASKLIST_OK,         //can do task now
	TASKLIST_HANDLING,     //waiting for obtain tasklist; 
	TASKLIST_PREPARING,
	TASKLIST_NULL
};

typedef struct http_header_st {
	int  exist;
	char name[MAX_COMM_BUF_SZIE];
	char value[HDR_VALUE_LEN];
} hdr_t;

typedef struct http_res_hdrs_st {
	hdr_t res_code;
	hdr_t cont_len;
	hdr_t last_modified;
	hdr_t date;
	hdr_t etag;
	hdr_t powered_by;
} http_res_hdrs_t;

struct handle_res_st {
	int  task_type;
	int  detail;
	char taskid[MAX_COMM_BUF_SZIE];
	char devid[MAX_COMM_BUF_SZIE];
	char status[MAX_COMM_BUF_SZIE];
	char local_md5[MAX_COMM_BUF_SZIE];
	char taskurl_md5[MAX_COMM_BUF_SZIE];
	char task_url[MAX_URL_BUF_SIZE];
	http_res_hdrs_t res_hdrs;
};

struct sync_cond_st {
	pthread_t thread_id;
	pthread_cond_t cond;
	pthread_mutex_t mutex;
};

struct tasklist_sync_st {
	FILE *fp;
	int status;   
	pthread_cond_t cond;
	pthread_mutex_t mutex;
	struct sync_cond_st prepare;
	struct sync_cond_st handle;
};

struct preload_handle_info_st {
	time_t start_time;
	time_t end_time;
	off_t used_time;
	off_t total_tasks;
	off_t failed_times;
	off_t succeed_times;       
	off_t total_download_size; /* bytes */
};

void writeLogInfo(const int, char *, ...);
/* end by xin.yao:2011-11-19 */

#endif

