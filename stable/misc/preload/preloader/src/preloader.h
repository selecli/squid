#ifndef PRELOADER_H
#define PRELOADER_H

#include <time.h>
#include <glob.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>
#include <assert.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>

#define  CFG_DIRECTIVE_NONE                  -1
#define  CFG_DIRECTIVE_ATOMIC                -2
#define  CFG_DIRECTIVE_COMPOSITE             -3

#define  IO_EVENT_READ                       -1
#define  IO_EVENT_WRITE                      -2
#define  IO_EVENT_ERROR                      -3

#define  PROTOCOL_HTTP                        0
#define  PROTOCOL_HTTPS                       1

#define  SSL_V2                              -1
#define  SSL_V3                              -2
#define  SSL_V23                             -3

#define  HTTP_METHOD_GET                     -1 
#define  HTTP_METHOD_HEAD                    -2
#define  HTTP_METHOD_PUT                     -3
#define  HTTP_METHOD_POST                    -4
#define  HTTP_METHOD_NOT_SUPPORT             -5
#define  HTTP_STATUS_OK                       200
#define  HTTP_STATUS_BAD_REQUEST              400
#define  HTTP_STATUS_FORBIDDEN                403
#define  HTTP_STATUS_NOT_ALLOWED              405
#define  HTTP_STATUS_PROCESS_TIMEOUT          408
#define  HTTP_STATUS_INTERNAL_SERVER_ERROR    500
#define  HTTP_ERROR_PARSE_LINE               -1
#define  HTTP_ERROR_PARSE_HEADER             -2
#define  HTTP_ERROR_PARSE_BODY               -3

#define  TASK_STATE_READY                    -1
#define  TASK_STATE_REFRESH                  -2
#define  TASK_STATE_PRELOAD                  -3
#define  TASK_STATE_REPORT                   -4
#define  TASK_STATE_REMOVE                   -5
#define  TASK_STATE_DONE                     -6
#define  TASK_STATE_ERROR                    -7

#define  PRELOAD_STATUS_OK                    200
#define  PRELOAD_STATUS_REMOVED               205
#define  PRELOAD_STATUS_ERROR                 500

#define  GRAB_STATE_READY                    -1
#define  GRAB_STATE_IP                       -2
#define  GRAB_STATE_DNS                      -3

/* rfc1035 - DNS */
#define  RFC1035_TYPE_A                       1
#define  RFC1035_TYPE_CNAME                   5
#define  RFC1035_TYPE_PTR                     12
#define  RFC1035_CLASS_IN                     1
#define  RFC1035_MAXHOSTNAMESZ                256

#define  WAITING_QUEUE                       -1
#define  RUNNING_QUEUE                       -2

#define  SOCKADDR_IP                         -1
#define  SOCKADDR_DNS                        -2

#define  RUN_FOREGROUND                      -1
#define  RUN_BACKGROUND                      -2

#define  SCHED_POLICY_FIFO                   -1
#define  SCHED_POLICY_HPFP                   -2

#define  LIST_HEAD                           -1
#define  LIST_TAIL                           -2

#define  RBTREE_RED                           0
#define  RBTREE_BLACK                         1

#define  MKDIR_NONE                          -1
#define  MKDIR_PARSE                         -2
#define  MKDIR_DIRECT                        -3

#define  RET_OK                              -1
#define  RET_ERROR                           -2

#define  ACTION_NONE                         -1
#define  ACTION_REFRESH                      -2
#define  ACTION_PRELOAD                      -3
#define  ACTION_REFRESH_PRELOAD              -4
#define  ACTION_REMOVE                       -5

#define  CHECK_TYPE_BASIC                    -1
#define  CHECK_TYPE_MD5                      -2
#define  CHECK_TYPE_SHA1                     -3

#define  PORT_DEFAULT_LISTEN                  31108
#define  PORT_DEFAULT_REFRESH                 21108
#define  PORT_DEFAULT_PRELOAD                 80
#define  PORT_DEFAULT_REPORT                  80

#define  SIZE_16                              16
#define  SIZE_32                              32
#define  SIZE_64                              64
#define  SIZE_128                             128
#define  SIZE_256                             256
#define  SIZE_512                             512
#define  SIZE_1KB                             1024
#define  SIZE_2KB                             2048
#define  SIZE_4KB                             4096
#define  SIZE_8KB                             8192
#define  SIZE_16KB                            16384
#define  SIZE_32KB                            32768
#define  SIZE_64KB                            65535

#define  MAX_CONCURRENCY                      32
#define  MAX_PRELOAD_WKR                      32
#define  MAX_TASK_LIMIT                       1024
#define  MAX_OPEN_FD                          1024

#define  APP_NAME                             "preloader"
#define  APP_VERSION                          "1.0.6"
#define  APP_PID_FILE                         "/var/run/preloader.pid"
#define  APP_CFG_FILE                         "/usr/local/preloader/conf/preloader.conf"

#define  LOG_WRITE                           -1
#define  LOG_ABORT                           -2
#define  LOG_ROTATE_NUMBER                    5
#define  LOG_ROTATE_SIZE                      40960000
#define  LOG_ACCESS_FILE                      "/var/log/chinacache/preloader/access.log"
#define  LOG_DEBUG_FILE                       "/var/log/chinacache/preloader/debug.log"
#define  LOG_ERROR_FILE                       "/var/log/chinacache/preloader/error.log"


#define LogAccess(fmt, args...) \
        log_write(NULL, 0, LOG_WRITE, g_log.access, 0, (fmt), ##args)

#define LogDebug(level, fmt, args...) \
        log_write(__FILE__, __LINE__, LOG_WRITE, g_log.debug, level, (fmt), ##args)

#define LogError(level, fmt, args...) \
        log_write(__FILE__, __LINE__, LOG_WRITE, g_log.error, level, (fmt), ##args)

#define LogAbort(fmt, args...) \
        log_write(__FILE__, __LINE__, LOG_ABORT, g_log.error, 0, (fmt), ##args)

#define xmemzero(s, n) \
        (void)memset((s), 0, (n))

#define xerror() \
        strerror(errno)

#define safe_process(handler, ptr) \
        if (NULL != (handler) && NULL != (ptr)) { (handler)((ptr)); (ptr) = NULL; }

#define safe_process2(handler, ptr) \
        if (NULL != (handler) && NULL != (ptr)) { (handler)((ptr)); }

#define xassert(asn) \
        ((asn) ? ((void)0) : LogAbort("assert(%s) abort.\n", #asn))

#define ARRAY_ITEM_DATA(a, i) \
        (((a)->items[(i)])->data)

#define rbtree_color_is_red(node) \
        (0 == (node)->color)

#define rbtree_color_is_black(node) \
        (1 == (node)->color)


typedef  void SIG_HDL(int);
typedef  void CB_HDL(void *);
typedef  void FREE(void *);
typedef  void CFG_HDL(void *, void *);
typedef  void IST_HDL(void *, void *);
typedef  int LIST_CMP(void *, void *);
typedef  int LIST_MOD(void *, void *);
typedef  int LIST_TRL(void *);
typedef  int COMPARE(const void *, const void *);
typedef  int HASH_HDL(void *);
typedef  LIST_CMP CMP_HDL;
typedef  unsigned int HASH(const void *, const unsigned int);
typedef  ssize_t SND_MTD(int, char *, size_t);
typedef  ssize_t RCV_MTD(int, char *, size_t);



extern  struct server_log_st     g_log;
extern  struct type_size_st      g_tsize;
extern  struct comm_epoll_st     g_epoll;
extern  struct file_st           g_cfg_file;
extern  struct comm_st *         g_commtable;
extern  struct wkr_listen_st     g_listen;
extern  struct wkr_preload_st    g_preload;
extern  struct wkr_grab_st       g_grab;
extern  struct server_global_st  g_global;
extern  struct rbtree_st *       g_rbtree;
extern  int                      g_run_mode;
extern  int                      rfc1035_errno;
extern  const char *             rfc1035_error_message;


struct array_item_st {
    void * data;
    FREE * data_free;
};

struct array_st {
    unsigned int count;
    unsigned int capacity;
    struct array_item_st ** items;
    pthread_mutex_t mutex;
};

struct list_node_st {
    void * data;
    struct list_node_st * prev;
    struct list_node_st * next;
};

struct list_st {
    uint32_t number;
    struct list_node_st * head;
    struct list_node_st * tail;
    pthread_mutex_t mutex;
};

struct rbtree_node_st {
    unsigned long key;
    unsigned int color : 1; /* red or black */
    struct rbtree_node_st *left;
    struct rbtree_node_st *right;
    struct rbtree_node_st *parent;
};

struct rbtree_st {
    unsigned int number;
    struct rbtree_node_st *root;
    struct rbtree_node_st *sentinel;
    IST_HDL *insert;
    pthread_mutex_t mutex;
};

struct hash_link_st {
    void *key;
    struct hash_link_st *next;
};

struct hash_table_st {
    char *name;              /* hash table name */
    unsigned int size;       /* buckets number */
    unsigned int number;     /* current objetcs number */
    HASH *hash;              /* hash key function */
    COMPARE *cmp;            /* compare function */
    struct hash_link_st **buckets;    /* hash buckets array */
    pthread_mutex_t mutex;   /* mutex for thread safe */
};

struct md5_context_st {
    unsigned int  state[4];     /* state (ABCD) */
    unsigned int  count[2];     /* number of bits, modulo 2^64 (lsb first) */
    unsigned char buffer[64];   /* input buffer */
};

struct data_buf_st {
    char *buf;
    size_t size;
    size_t offset;
};

struct comm_data_st {
    int type;
    void *data;
};

struct comm_epoll_st {
    int fd;        
    int timeout; 
    int maxevents;
    struct epoll_event *events;   
    pthread_mutex_t mutex;
};

struct io_event_st {
    unsigned int active          : 1;
    unsigned int ready           : 1;
    unsigned int eof             : 1;
    unsigned int timedout        : 1;
    unsigned int error           : 1;
    unsigned int posted_ready    : 1;
    unsigned int posted_timedout : 1;
    unsigned int posted_error    : 1;
    unsigned int timer_set       : 1;
    CB_HDL *handler;
    void *handler_data;
    struct data_buf_st dbuf;
    struct rbtree_node_st timer;
    pthread_mutex_t mutex;
};

struct comm_accept_st {
    struct comm_listen_st *ls; /* owner */
    struct comm_connect_st *cn;
    struct http_st *http;
    unsigned int done : 1;
};

struct comm_ssl_st {
    int version; /* SSL/TLS: default is SSLv23. */
    SSL *ssl;
    SSL_CTX *ctx;
};

struct comm_connect_st {
    int fd;
    struct comm_ssl_st *ssl;
    struct io_event_st *rev;
    struct io_event_st *wev;
    unsigned int ssl_checked : 1;
    unsigned int link        : 16;
};

struct comm_preload_st {
    struct task_st *task;
    struct http_st *http;
    struct comm_connect_st *cn;
};

struct task_url_st {
    char *id;
    char *url;
    char *uri;
    char *host;
    unsigned int protocol : 1;
    unsigned int port     : 16;
};

struct task_refresh_st {
    int status;
    int max_reply_body_size;
    unsigned int start : 1;
    unsigned int done  : 1;
    unsigned int error : 1;
};

struct m3u8_st {
    char *url_prefix;
    unsigned int id;
    unsigned int enable : 1;
    struct data_buf_st dbuf;
    struct task_st *task;
};

struct digest_st {
    int type;
    void *context;
    unsigned char *digest;
};

struct limit_rate_st {
    size_t rate;
    time_t start;
    size_t total_len;
    double mean_rate; /* kbps */
};

struct task_preload_st {
    int status;
    int start_time;
    int finish_time;
    char *date;
    char *http_status;
    char *last_modified;
    char *content_length;
    char *cache_status;
    double download_mean_rate;
    struct m3u8_st m3u8;
    struct digest_st digest;
    struct limit_rate_st limit_rate;
    unsigned int line_done : 1;
    unsigned int hdr_done  : 1;
    unsigned int start     : 1;
    unsigned int done      : 1;
    unsigned int error     : 1;
};

struct task_report_st {
    int status;
    unsigned int retry : 8;
    unsigned int start : 1;
    unsigned int done  : 1;
    unsigned int error : 1;
};

struct task_st {
    int state;
    int method;
    int action;
    int priority;
    int check_type;
    int need_report;
    int is_override;
    char *sessionid;
    unsigned int nest_track_level;
    struct sockaddr_in preload_addr;
    struct sockaddr_in report_addr;
    struct task_url_st *task_url;
    struct task_refresh_st refresh;
    struct task_preload_st preload;
    struct task_report_st  report;
    struct comm_connect_st *cn;
    pthread_mutex_t mutex;
    unsigned int done    : 1;
    unsigned int removed : 1;
};

struct http_header_st {
    char *name;
    char *value;
};

struct http_body_st {
    size_t length;
    struct data_buf_st *dbuf;
};

struct http_parsed_st {
    unsigned int line       : 1;
    unsigned int header     : 1;
    unsigned int body       : 1;
    unsigned int done       : 1;
    unsigned int error      : 1;
    unsigned int error_type : 3;
};

struct http_request_st {
    char *uri;
    char *proto;
    int method;
    unsigned int major : 16;
    unsigned int minor : 16;
    ssize_t content_length;
    struct array_st header;
    struct http_body_st body;
    struct http_parsed_st parsed;
    unsigned int built  : 1;
    unsigned int packed : 1;
};

struct http_reply_st {
    int status;
    int major  : 16;
    int minor  : 16;
    int packed : 1;
    int built  : 1;
    ssize_t content_length;
    struct array_st header;
    struct http_body_st body;
    struct http_parsed_st parsed;
};

struct http_st {
    struct http_request_st *request;
    struct http_reply_st *reply;
};

struct close_handler_st {
    void *data;
    CB_HDL *handler;
    struct close_handler_st *next;
};

struct comm_st {
    unsigned int inuse : 1;
    struct comm_connect_st *cn;
    struct close_handler_st *close_handler;
    pthread_mutex_t mutex;
};

struct comm_listen_st {
    unsigned int n_accept_once;
    struct comm_connect_st *cn;
    struct list_st *accept_list;
};

struct comm_grab_st {
    int state;
    time_t last_time;
    struct sockaddr_in addr;
    struct comm_connect_st *cn;
    unsigned int start        : 1;
    unsigned int done         : 1;
    unsigned int dns_query_ok : 1;
    unsigned int error        : 1;
};

struct rfc1035_rr_st {
    char name[RFC1035_MAXHOSTNAMESZ];
    unsigned short type;
    unsigned short class;
    unsigned int ttl;
    unsigned short rdlength;
    char *rdata;
};

struct rfc1035_query_st {
    char name[RFC1035_MAXHOSTNAMESZ];
    unsigned short qtype;
    unsigned short qclass;
};

struct rfc1035_message_st {
    unsigned short id;
    unsigned int qr:1;
    unsigned int opcode:4;
    unsigned int aa:1;
    unsigned int tc:1;
    unsigned int rd:1;
    unsigned int ra:1;
    unsigned int rcode:4;
    unsigned short qdcount;
    unsigned short ancount;
    unsigned short nscount;
    unsigned short arcount;
    struct rfc1035_query_st *query;
    struct rfc1035_rr_st *answer;
};

struct dns_query_st {
    int rcode;
    int nsends;
    int attempt;
    time_t start_time;
    time_t sent_time;
    ssize_t len;
    unsigned short id;
    char * name;
    char buf[SIZE_4KB];
    const char *error;
    struct rfc1035_query_st query;
};

struct comm_dns_st {
    struct comm_connect_st *cn;
    struct comm_grab_st *grab;
};

struct comm_ip_st {
    struct comm_connect_st *cn;
    struct comm_grab_st *grab;
    struct http_st *http;
};

struct config_dns_st {
    char *host;
    unsigned int port : 16;
};

struct config_addr_st {
    int type;
    union {
        struct sockaddr_in ip;
        struct config_dns_st dns;
    } sockaddr;
};

struct http_option_st {
    unsigned int send_timeout;
    unsigned int recv_timeout;
    unsigned int connect_timeout;
    unsigned int keepalive_timeout;
    unsigned int max_request_body_size;
    unsigned int max_request_header_size;
};

struct directive_st {
    int type;
    char * directive;
    struct array_st * params;
    CFG_HDL * handler;
};

struct config_directive_st {
    const char * directive;
    int type;
    unsigned int param_number;
    CFG_HDL * handler;
};

struct wkr_listen_st {
    struct {
        struct sockaddr_in addr;
    } cfg;
};

struct wkr_preload_st {
    struct {
        struct sockaddr_in addr;
        unsigned int worker_number;
        unsigned int m3u8_engine_enabled;
    } cfg;
};

struct wkr_grab_st {
    struct {
        struct config_addr_st addr;
        unsigned int interval_time;
    } cfg;
    struct comm_grab_st grab;
};

struct type_size_st {
    size_t log_st;
    size_t pointer;
    size_t array_st;
    size_t array_item_st;
    size_t list_st;
    size_t list_node_st;
    size_t rbtree_st;
    size_t rbtree_node_st;
    size_t hash_link_st;
    size_t hash_table_st;
    size_t in_addr;
    size_t sockaddr_in;
    size_t epoll_event;
    size_t io_event_st;
    size_t directive_st;
    size_t config_addr_st;
    size_t session_st;
    size_t task_st;
    size_t task_url_st;
    size_t data_buf_st;
    size_t comm_st;
    size_t comm_data_st;
    size_t comm_ssl_st;
    size_t comm_connect_st;
    size_t comm_accept_st;
    size_t comm_listen_st;
    size_t comm_preload_st;
    size_t comm_dns_st;
    size_t comm_ip_st;
    size_t http_st;
    size_t http_header_st;
    size_t http_request_st;
    size_t http_reply_st;
    size_t close_handler_st;
    size_t md5_context_st;
    size_t dns_query_st;
};

struct file_st {
    char *   path;
    off_t    size;
    time_t   mtime;
    uint8_t  alive;
    uint32_t rotate_size;
    uint32_t rotate_number;
    uint32_t rotate_count;
    struct {
        int fd;
        int flags;
        mode_t mode;
    } fd;
    struct {
        FILE * fp;
        char * mode;
    } fp;
};

struct log_st {
    struct file_st fmsg;
    uint8_t print_level;
    uint8_t rotate_number;
    uint32_t rotate_size;
    pthread_mutex_t mutex;
};

struct server_log_st {
    struct log_st *access;
    struct log_st *debug;
    struct log_st *error;
};

struct server_global_st {
    struct {
        int sched_policy;
        unsigned int task_limit_number;
        unsigned int task_concurrency;
        struct sockaddr_in dns_addr;
        struct log_st *access_log;
        struct log_st *debug_log;
        struct log_st *error_log;
        struct http_option_st http;
    } cfg;
    struct list_st *waiting_list;
    struct list_st *running_list;
};

struct session_st {
    char * id;
    int method;
    int action;
    int priority;
    int encoding;
    int check_type;
    int need_report;
    int is_override;
    time_t start_time;
    size_t limit_rate;
    unsigned int nest_track_level;
    struct list_st *taskurl_list;
    struct sockaddr_in preload_addr;
    struct sockaddr_in report_addr;
    pthread_mutex_t mutex;
};

extern  struct log_st *log_create(const char *, const uint8_t, const uint32_t, const uint32_t);
extern  void log_open(struct log_st *);
extern  void log_reopen(struct log_st *);
extern  void log_init(struct log_st *, const char *, const uint8_t, const uint32_t, const uint32_t);
extern  struct log_st *log_clone(struct log_st *);
extern  void log_write(const char *, const int, const int, struct log_st *, const uint8_t, const char *, ...);
extern  void log_rotate(struct log_st *);
extern  void log_close(struct log_st *);
extern  void log_destroy(struct log_st *);

extern  int comm_socket(int, int, int);
extern  int comm_listen(int, int);
extern  int comm_accept(int);
extern  int comm_connect(int, struct sockaddr_in *);
extern  struct comm_listen_st *comm_listen_create(int);
extern  struct comm_accept_st *comm_accept_create(int, struct comm_listen_st *);
extern  struct comm_connect_st *comm_connect_create(int);
extern  struct comm_preload_st *comm_preload_create(int, struct task_st *);
extern  struct comm_dns_st *comm_dns_create(int, struct comm_grab_st *);
extern  struct comm_ip_st *comm_ip_create(int, struct comm_grab_st *);
extern  struct io_event_st *comm_io_event_create(void);
extern  void comm_set_connection(int, struct comm_connect_st *);
extern  void comm_set_close_handler(int, CB_HDL *, void *);
extern  void comm_process_posted_event(struct io_event_st *);
extern  void comm_set_io_event(int, int, CB_HDL *, void *);
extern  void comm_select_io_event(void);
extern  void comm_set_timeout(int, int, time_t);
extern  void comm_process_io_event(int);
extern  void comm_event_init(struct io_event_st *);
extern  ssize_t comm_send(int, char *, size_t);
extern  ssize_t comm_recv(int, char *, size_t);
extern  int comm_sendto(int, char *, size_t, int, const struct sockaddr *, socklen_t);
extern  ssize_t comm_recvfrom(int, void *, size_t, int, struct sockaddr *, socklen_t *);
extern  int comm_set_close_on_exec(int);
extern  int comm_set_block(int);
extern  int comm_set_nonblock(int);
extern  int comm_set_reuse_addr(int);
extern  int comm_bind(int, struct sockaddr_in *);
extern  int comm_open(int);
extern  void comm_close(int);
extern  void comm_accept_close(void *);
extern  void comm_listen_destroy(void *);
extern  void comm_accept_destroy(void *);
extern  void comm_connect_destroy(void *);
extern  void comm_preload_destroy(void *);
extern  void comm_dns_destroy(void *);
extern  void comm_ip_destroy(void *);
extern  void comm_io_event_destroy(struct io_event_st *);
extern  int comm_ssl_connect(struct comm_connect_st *);
extern  ssize_t comm_ssl_recv(int, char *, size_t);
extern  ssize_t comm_ssl_send(int, char *, size_t);
extern  void comm_ssl_close(struct comm_ssl_st *);

extern  void http_parse_request(struct http_request_st *, struct data_buf_st *);
extern  void http_build_request_line(struct http_request_st *, int, char *, char *, int, int);
extern  void http_build_body(struct http_body_st *body, struct data_buf_st *dbuf);
extern  struct http_header_st *http_build_header(const char *, const char *);
extern  void http_add_header(struct array_st *, struct http_header_st *);
extern  struct http_header_st *http_find_header(struct array_st *, const char *);
extern  void http_parse_reply(struct http_reply_st *, struct data_buf_st *);
extern  void http_build_reply(struct http_reply_st *, int, int, int, struct data_buf_st *);
extern  void http_pack_request(struct data_buf_st *, struct http_request_st *);
extern  void http_pack_reply(struct data_buf_st *, struct http_reply_st *);
extern  struct http_st *http_create(void);
extern  void http_destroy(struct http_st *);

extern  int task_queue_number(int);
extern  struct task_st *task_queue_fetch(int, void *, CMP_HDL *);
extern  struct task_st *task_queue_find(int, void *, CMP_HDL *);
extern  void task_queue_remove(int, void *, CMP_HDL *);
extern  void task_queue_insert(int, struct task_st *);
extern  struct task_url_st *task_url_create(char *, char *);
extern  void task_url_free(struct task_url_st *);
extern  void task_free(struct task_st *);
extern  void task_submit(struct session_st *);
extern  void task_submit_subtask(struct task_st *task, char *url);

extern  struct array_st * array_create(void);
extern  void array_init(struct array_st *);
extern  void array_grow(struct array_st *, uint32_t);
extern  void array_append(struct array_st *, void *, FREE *);
extern  void array_insert(struct array_st *, int, void *, FREE *);
extern  void array_remove(struct array_st *, void *);
extern  void array_delete(struct array_st *, void *);
extern  void array_clean(struct array_st *);
extern  void array_destroy(struct array_st *);

extern  struct list_st * list_create(void);
extern  uint32_t list_member_number(struct list_st *);
extern  int list_remove(struct list_st *, void *, LIST_CMP *);
extern  int list_find_2(struct list_st *, void *, LIST_CMP *);
extern  void * list_find(struct list_st *, void *, LIST_CMP *);
extern  void list_insert_head(struct list_st *, void *);
extern  void list_insert_tail(struct list_st *, void *);
extern  void *list_fetch(struct list_st *, void *, LIST_CMP *);
extern  void list_travel(struct list_st *, LIST_TRL *);
extern  void list_travel_2(struct list_st *, LIST_TRL *);
extern  void list_delete(struct list_st *, void *, LIST_CMP *);
extern  void list_modify(struct list_st *, void *, LIST_MOD *);
extern  void list_destroy(struct list_st *);

extern  struct rbtree_st *rbtree_create(IST_HDL *);
extern  void rbtree_insert(struct rbtree_st *, struct rbtree_node_st *);
extern  void rbtree_remove(struct rbtree_st *, struct rbtree_node_st *);
extern  void rbtree_insert_event_timer(struct rbtree_st *, struct rbtree_node_st *);
extern  void rbtree_process_event_timeout(struct rbtree_st *);

extern  struct hash_link_st * hash_link_create(void *, const uint32_t);
extern  struct hash_link_st * hash_link_create_2(void *);
extern  struct hash_table_st * hash_table_create(COMPARE *, HASH *, uint32_t);
extern  unsigned int hash_table_number(struct hash_table_st *);
extern  unsigned int hash_key_calculate(const void *, unsigned int);
extern  void * hash_table_lookup(struct hash_table_st *, const void *);
extern  int hash_table_lookup_2(struct hash_table_st *, const void *, HASH_HDL *);
extern  int hash_table_lookup_3(struct hash_table_st *, const void *, COMPARE *);
extern  void hash_table_join(struct hash_table_st *, struct hash_link_st *);
extern  void hash_table_remove(struct hash_table_st *, struct hash_link_st *);
extern  void hash_table_remove_2(struct hash_table_st *, void *);
extern  void hash_table_remove_3(struct hash_table_st *, void *, HASH_HDL *);
extern  void hash_table_destroy(struct hash_table_st *);

extern  void md5_init(struct md5_context_st *);
extern  void md5_update (struct md5_context_st*, unsigned char *, unsigned int);
extern  void md5_final (unsigned char *, struct md5_context_st *);

extern  ssize_t rfc1035BuildAQuery(const char *, char *, size_t, unsigned short, struct rfc1035_query_st *);
extern  ssize_t rfc1035BuildPTRQuery(const struct in_addr, char *, size_t, unsigned short, struct rfc1035_query_st *);
extern  void rfc1035SetQueryID(char *, unsigned short);
extern  int rfc1035MessageUnpack(const char *, size_t, struct rfc1035_message_st **);
extern  int rfc1035QueryCompare(const struct rfc1035_query_st *, const struct rfc1035_query_st *);
extern  void rfc1035MessageDestroy(struct rfc1035_message_st *);

extern  void * xmalloc(size_t);
extern  void * xzalloc(size_t);
extern  void * xcalloc(size_t, size_t);
extern  void * xrealloc(void *, size_t);
extern  int xisdir(const char *);
extern  int xmkdir(const char *, mode_t, int);
extern  void xrmdir(const char *);
extern  int xopen(const char *, int, mode_t);
extern  int xopen2(const char *, int, mode_t);
extern  FILE * xfopen(const char *, const char *);
extern  FILE * xfopen2(const char *, const char *);
extern  off_t xfsize(const char *);
extern  void xfclose(FILE *);
extern  void xfflush(FILE *);
extern  char * xstrncpy(char *, const char *, size_t);
extern  char * xstrdup(const char *);
extern  char * xstrndup(const char *, size_t);
extern  void xunlink(const char *);
extern  int xisspace(const char);
extern  int xisCRLF(const char);
extern  int is_matched(void *, void *);
extern  int always_matched(void *, void *);
extern  void parse_conf(void);
extern  void * xml_parse(const char *, const size_t);
extern  void session_free(struct session_st *session);

extern  struct data_buf_st *data_buf_create(size_t);
extern  struct data_buf_st *data_buf_clone(struct data_buf_st *);
extern  void data_buf_init(struct data_buf_st *, size_t);
extern  void data_buf_append(struct data_buf_st *, const char *, size_t);
extern  void data_buf_reset(struct data_buf_st *);
extern  void data_buf_resize(struct data_buf_st *dbuf, size_t size);
extern  void data_buf_clean(struct data_buf_st *);
extern  void data_buf_destroy(struct data_buf_st *);

extern  void current_gmttime(char *, size_t);
extern  void server_name(char *, size_t);
extern  void server_exit(int);
extern  int dns_send_query(char *, struct sockaddr_in *, struct comm_grab_st *);

extern  void listen_worker_start(void);
extern  void preload_worker_start(void);
extern  void grab_worker_start(void);
extern  void help_worker_start(void);

#endif
