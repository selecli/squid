#ifndef __REFRESH_HEADER_H
#define __REFRESH_HEADER_H

#include <stdint.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <net/if_arp.h>
#include <stdbool.h>
#include <sys/epoll.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <assert.h>
#include <ctype.h>
#include <db.h> 
#include <fnmatch.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/wait.h>
#include "mempool.h"
#include "log.h"
#include "list.h"
#include "squid_enums.h"
#include "rf_err_def.h"

#define FD_MAX	2560
#define MAX_EVENTS 2560
#define EPOLL_TIMEOUT 100	//milliseconds
#define PIDFILE			"/var/run/refreshd.pid"
#define BUFFER_SIZE	409600
#define REFRESH_DB_PATH "/data/refresh_db/"
#define REFRESH_HASH_DB_PATH "/usr/local/squid/bin/hash_db"
#define REFRESH_DB_SUB_FILE_NUMS 5
#define RF_XML_HEADER "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
#define REFRESH_CLI_PATH "/usr/local/squid/bin/refresh_cli"
#define SESS_STACK_SIZE 50

#define RF_CLI_FLAG_KEEPALIVE 0x0001

#define DB FILE

#define DB_NUM 5

#ifdef CC_X86_64
#define PRINTF_UINT64_T "lu"
#else
#define PRINTF_UINT64_T "llu"
#endif

/* MD5 cache keys */
typedef enum
{
        RF_FC_ACTION_UNKNOWN,
        RF_FC_ACTION_INFO,
        RF_FC_ACTION_ADD,
        RF_FC_ACTION_VERIFY_START,
        RF_FC_ACTION_VERIFY_FINISH,
        RF_FC_ACTION_URL_PURGE,
        RF_FC_ACTION_URL_EXPIRE,
        RF_FC_ACTION_DIR_PURGE,
        RF_FC_ACTION_DIR_EXPIRE,
        RF_FC_ACTION_URL_PURGE_RET,
        RF_FC_ACTION_URL_EXPIRE_RET,
	RF_FC_ACTION_DIR_PURGE_RET,
        RF_FC_ACTION_DIR_EXPIRE_RET,
        RF_FC_ACTION_VERIFY_RET,
	RF_FC_ACTION_DIR_EXPIRE_REFRESH_PREFIX_START,
        RF_FC_ACTION_DIR_EXPIRE_REFRESH_WILDCARD_START,
        RF_FC_ACTION_DIR_PURGE_REFRESH_PREFIX_START,
        RF_FC_ACTION_DIR_PURGE_REFRESH_WILDCARD_START,
	RF_FC_ACTION_DIR_REFRESH_FINISH,
	RF_FC_ACTION_DIR_REFRESH_RESULT
} rf_fc_action;

typedef enum{
	RF_CLIENT_STATUS_UNKNOWN,
	RF_CLIENT_VERIFY_START,
	RF_CLIENT_VERIFY_FINISH
} rf_enum_client_status;

typedef struct {
	char *squid_id;
	rf_fc_action action;
	char *url;
	char *others; //sessid and retcode
} rf_fc_input;

typedef enum {
	RF_CLIENT_UNKNOW,
	RF_CLIENT_FC,
	RF_CLIENT_CLI,
	RF_CLIENT_REPORT
} rf_enum_client_type;

enum {
	RF_WL_ACTION_ADD_TAIL,
	RF_WL_ACTION_ADD_FIRST,
	RF_WL_ACTION_DEL
};

typedef struct _rf_url_list{
	char *buffer;
	uint64_t len;
	uint64_t id;
	uint64_t ret;
	struct _rf_url_list *next;
} rf_url_list;

typedef struct _rf_cli_dir_action{
	int action;
	char *report_address;
} rf_cli_dir_params;

typedef enum{
	RF_SESSION_NEW,
	RF_SESSION_SETUP,	//for dir refresh
	RF_SESSION_DISPATCH,	//for dir refresh
	RF_SESSION_PENDDING,
	RF_SESSION_PENDDING2,	//for dir refresh
	RF_SESSION_DONE,
	RF_SESSION_SYNC,	//for dir refresh
	RF_SESSION_FREE
} rf_session_status;

typedef enum{
	RF_SESSION_INIT,
	RF_SESSION_PACKED,
	RF_SESSION_FAILED,
	RF_SESSION_SUCCESS,
	RF_SESSION_CLOSE
} rf_session_report_status;

typedef struct _rf_cli_session{
	struct list_head i_list;

	rf_session_report_status report_status;		//for report dir refresh result
	int log;					//only log report info once
	int report_times;				//report times

	rf_fc_action method;
	char sessionid[128];
	rf_session_status status;

	uint64_t url_number;
	uint64_t url_all_number;		//squid count *(sess->url_number)*(config.compress_formats_count + 2)
	uint64_t success;
	uint64_t fail;
	time_t	begin;
	time_t  end;
	time_t last_active;

	void *params;
	rf_url_list *url_list;

	int fd;
	char *peer_ip;
	int async_fd;
} rf_cli_session;

typedef struct _rf_write_buffer{
	char *buffer;
	int len;
	int send_len;

	struct _rf_write_buffer *next;
} rf_write_buffer;

typedef struct _rf_write_list{
	struct _rf_write_buffer *first;
	struct _rf_write_buffer *last;
	uint64_t count;
	pthread_mutex_t mutex;
} rf_write_list;


typedef struct _rf_client{
	rf_enum_client_type type;	//client type
	rf_enum_client_status status;	//client status

	int inuse;	
	uint32_t fd;
	int epoll_flag;
	char *ident;		//client identity
	uint32_t flag;		//http flag
	uint32_t content_length; //content length

	void *data;		//client data

	char *read_buff;
	uint32_t read_buff_len;
	uint32_t read_buff_size;

	char *xml_buff;
	uint32_t xml_buff_len;

	rf_write_list *wl;	//write list

	char *db_files[DB_NUM];
	char *db_bak_files[DB_NUM];
	void *db_handlers[DB_NUM];
	void *db_bak_handlers[DB_NUM];

	char *db_file;
	void *db_handler,*db_bak_handler;

	time_t setup_time;	//connect setup time
}rf_client;


typedef struct _rf_config{
	uint32_t log_level;
	unsigned short port;

	uint32_t db_commit_interval;
	uint32_t db_commit_count;
	uint32_t url_wait_timeout;
	uint32_t dir_wait_timeout;
	uint32_t dir_wait_timeout2;
	uint32_t if_hash_db;
	uint32_t enable_dir_purge;

	char report_ip[16];
	char test_url[128];
	char compress_delimiter[16];
	char purge_success_del[16];
	char bdb_cache_size[128];
	char bdb_page_size[128];
	bool is_dir_refresh;
	char* compress_formats[128];
	int compress_formats_count;
	char **url_remove_host;
	int url_remove_host_count;
} rf_config;

typedef struct _rf_thread_stack{
	rf_cli_session *sess_stack[SESS_STACK_SIZE];
	uint32_t sess_count;

	time_t thread_begin;
	time_t thread_end;
	pthread_t ptid;
} rf_thread_stack;

typedef struct _hash_dir_sess_stack
{
	rf_cli_session *sess_stack[SESS_STACK_SIZE];
	uint32_t sess_count;
} hash_dir_sess_stack;

// globla
extern rf_config config;
extern rf_client *fd_table;
extern rf_client *rfc_current;
extern time_t now;
extern int listen_fd;
extern bool rf_shutdown;
extern int async_report_fd;
extern bool async_report_fd_flag;
extern rf_thread_stack thread_stack;
extern bool rf_db_is_error;
extern hash_dir_sess_stack hash_dir_sess_stacks[DB_NUM];
extern int db_num;

typedef int rf_url_compare(char *token,char *url);
typedef int rf_match_callback(rf_client *rfc,rf_cli_session *sess,char *url);
typedef void epoll_handle_accept(int listen_fd);
typedef int epoll_handle_read_event(int fd);
typedef int epoll_handle_write_event(int fd);

void disconnect_fd(int fd);
int rf_fd_init(int fd);

void mem_check();
//rf_config.c
int config_parse(const char *confile);
//rf_db.c
int rf_store_init();
void rf_store_dest();
DB* get_db_handler(const char *db_file);
int rf_add_url(DB *db,const char *u_data);
int rf_add_url_real(DB *db,const char *u_data,int u_data_len);

int rf_db_close(DB *db);
int rf_flush(DB *db);
int rf_match_url(rf_client *rfc,rf_match_callback *url_match_callback);

void rf_db_check();
//common.c
char* get_peer_ip(int socket_fd);
char *signal_itoa(int sig);
bool is_local_socket(int socket_fd);
bool is_local_ip(uint32_t ip);
int is_socket_alive(int sock_fd);
void daemonize(void);
pid_t get_running_pid(void);
int check_running_pid(void);
int action_atoi(char *action);
char *action_itoa(int action);
int *strto_mi(char *buffer,char token);
int ignore_errno(int ierrno);
int get_url_filenum(char *url);
char *rf_get_db_bak_file(char *squid_id, int index);
void check_db_handlers(rf_client *rfc, char* squid_id);
void check_db_bak_handlers(rf_client *rfc, char* squid_id);

int rf_fc_gen_buffer(char *buffer,int action,char *url,char *inte0,int *inte1);

int rf_url_match_prefix(char *token,char *url);
int rf_url_match_regex(char *token,char *url);

char *rf_http_header(int len);
char *rf_http_header_post(int len);

char *rf_get_db_file(char *squid_id, int index);
char *rf_get_db_filebak(char *squid_id, int index);

int fd_setnonblocking(int fd);
int string_split(const char *string,const char *delimiter,char *prefix,char *suffix);
int fd_close_onexec(int fd);
char *get_ip_by_ethname(const char *ethname);
int convert_G_B(char *s,uint32_t *gb,uint32_t *byte);
void backtrace_info();
//rf_epoll.c
extern struct epoll_event events[MAX_EVENTS];
int rf_epoll_add(int fd,int flag);
int rf_epoll_mod(int fd,int flag);
int rf_epoll_remove(int fd);
void rf_epoll_init(void) ;
void rf_epoll_dest(void) ;

int rf_epoll_wait(epoll_handle_accept *accept_handler,epoll_handle_read_event *read_handler,epoll_handle_write_event *write_handler);
//rf_session.c
void rf_session_init();
void rf_session_dest();
void rf_session_add(rf_cli_session *sess);
void rf_session_del(rf_cli_session *sess);
int rf_set_session_report_status(int fd, int status);

int rf_session_check();
rf_cli_session *rf_find_session(char *sessionid);
int rf_set_session_url_ret(char *sessionid,char *url,int ret);
int rf_set_session_dir_ret(char *sessionid,int ret);
char * rf_url_hostname(const char *url,char *hostname);
int rf_get_fc_alive(int *array,int *count);

//rf_fc_process.c
int rf_fc_process(int fd);
void rf_notify_squid_verify();

//rf_cli_process.c
int  rf_cli_process(int fd);

//rf_xml.c
int rf_xml_parse(rf_cli_session *sess,const char *buffer,int size);

//rf_write_list.c
void rf_wb_free(rf_write_buffer *wb);
void rf_wl_free(rf_write_list *wl);
void rf_wl_init(rf_write_list *wl);
void rf_wl_push(rf_write_buffer *wb,rf_write_list *wl,int action);
rf_write_buffer *rf_wl_pop(rf_write_list *wl);
int rf_wl_xsend(int fd);
int rf_add_write_list(int fd,char *buffer,int len,int action);

bool rf_wl_check(int fd);
#endif
