#ifndef __SERVER_HEADER_H
#define __SERVER_HEADER_H
#define _GNU_SOURCE

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
#include <netdb.h>
#include "log.h"
#include "list.h"
#include "mempool.h"
#include "server_array.h"
//#include "server_string.h"

#define MAX_EVENTS 1024
#define EPOLL_TIMEOUT 100       		//milliseconds
#define PIDFILE                 "/var/run/ca_serverd.pid"
#define BUFFER_SIZE     (65535*2)		//read buff size in fd_struct
#define MAX_BUFF_SIZE     (65535*2)


#define RECV_BUFF_SIZE 65535			//max bytes per read event

#define MAX_HEADER_SIZE 65535
#define MAX_URL 4096

#define MAX_HEADER_NUM 30
#define MAX_HEADER_LEN 256

#ifdef CC_X86_64
#define PRINTF_UINT64_T "lu"
#else
#define PRINTF_UINT64_T "llu"
#endif

#define EBIT_SET(flag, bit)     ((void)((flag) |= ((1L<<(bit)))))
#define EBIT_CLR(flag, bit)     ((void)((flag) &= ~((1L<<(bit)))))
#define EBIT_TEST(flag, bit)    ((flag) & ((1L<<(bit))))

/* bit opearations on a char[] mask of unlimited length */
#define CBIT_BIT(bit)           (1<<((bit)%8))
#define CBIT_BIN(mask, bit)     (mask)[(bit)>>3]
#define CBIT_SET(mask, bit)     ((void)(CBIT_BIN(mask, bit) |= CBIT_BIT(bit)))
#define CBIT_CLR(mask, bit)     ((void)(CBIT_BIN(mask, bit) &= ~CBIT_BIT(bit)))
#define CBIT_TEST(mask, bit)    ((CBIT_BIN(mask, bit) & CBIT_BIT(bit)) != 0)

#define xmemcpy(d,s,n) bcopy((s),(d),(n))
#define xmemmove(d,s,n) bcopy((s),(d),(n))
                        
#define xisspace(x) isspace((unsigned char)x)
#define xtoupper(x) toupper((unsigned char)x)
#define xtolower(x) tolower((unsigned char)x)
#define xisdigit(x) isdigit((unsigned char)x)
#define xisascii(x) isascii((unsigned char)x)
#define xislower(x) islower((unsigned char)x)
#define xisupper(x) isupper((unsigned char)x)
#define xisalpha(x) isalpha((unsigned char)x)
#define xisalnum(x) isalnum((unsigned char)x)
#define xisgraph(x) isgraph((unsigned char)x)

/* String */
#define strLen(s)     ((/* const */ int)(s).len)
#define strBuf(s)     ((const char*)(s).buf)
#define strChr(s,ch)  ((const char*)strchr(strBuf(s), (ch)))
#define strRChr(s,ch) ((const char*)strrchr(strBuf(s), (ch)))
#define strStr(s,str) ((const char*)strstr(strBuf(s), (str)))
#define strCmp(s,str)     strcmp(strBuf(s), (str))
#define strNCmp(s,str,n)     strncmp(strBuf(s), (str), (n))
#define strCaseCmp(s,str) strcasecmp(strBuf(s), (str))
#define strNCaseCmp(s,str,n) strncasecmp(strBuf(s), (str), (n))
#define strSet(s,ptr,ch) (s).buf[ptr-(s).buf] = (ch)
#define strCut(s,pos) (((s).len = pos) , ((s).buf[pos] = '\0'))
#define strCutPtr(s,ptr) (((s).len = (ptr)-(s).buf) , ((s).buf[(s).len] = '\0'))
#define strCat(s,str)  stringAppend(&(s), (str), strlen(str))

#define MAX_CA_IP 10

typedef struct _in_addrs
{
        struct in_addr m_addr;
        char good_ip;
} in_addrs;

//read write return value
enum 
{
        OK,
	CLOSE,			//session finish success
        ERR,
        ERR_CONFIG_FILE,
	ERR_EPOLL,
	ERR_PEER_CLOSE,
};

//fd_type
enum
{
	UNKNOW,
	FC_2_S,
	S_2_WEB,
	S_2_CA,
};

//ca host type: host like: www.ca.com or host ip table in file;
enum
{
	UNKNOW_IP,
	HOST_IP,
	FILE_IP,
};

#define white_space " \t\r\n"
#define null_string "NULL"

typedef struct _server_config
{
	uint32_t log_level;
        unsigned short port;

	char ca_host[128];
	char ca_ip_file[128];
	unsigned short ca_port;

	char req_list_file[128];
	char rep_list_file[128];
	char content_type_list_file[128];
	char rep_status_list_file[128];

	bool hash_request_url;

} server_config;


typedef struct _session_fds
{
	uint32_t fc_2_s_fd;	//fd: FC 	<--->	server
	uint32_t s_2_w_fd;	//fd: server	<--->	web server
	uint32_t s_2_ca_fd;	//fd: server	<--->	CA

	void *data;		//used to save some header data;
} session_fds;

enum
{
	CONN_DEF,
	CONNING,
	CONN_OK,
};

enum
{
	POST_DEF,
	NOT_POST,
};
////////////////////////////////////////////////////////
struct _String {
    /* never reference these directly! */
    unsigned short int size;    /* buffer size; 64K limit */
    unsigned short int len;     /* current length  */
    char *buf; 
};
typedef struct _String String;
	
/////////////////////////////////////////////////////////
enum {
    METHOD_NONE,                /* 000 */
    METHOD_GET,                 /* 001 */
    METHOD_POST,                /* 010 */
    METHOD_PUT,                 /* 011 */
    METHOD_HEAD,                /* 100 */
    METHOD_CONNECT,             /* 101 */
    METHOD_TRACE,               /* 110 */
    METHOD_PURGE,               /* 111 */
    METHOD_OPTIONS,
    METHOD_DELETE,              /* RFC2616 section 9.7 */
    METHOD_PROPFIND,
    METHOD_PROPPATCH,
    METHOD_MKCOL,
    METHOD_COPY,
    METHOD_MOVE,
    METHOD_LOCK,
    METHOD_UNLOCK,
    METHOD_BMOVE,
    METHOD_BDELETE,
    METHOD_BPROPFIND,
    METHOD_BPROPPATCH,
    METHOD_BCOPY,
    METHOD_SEARCH,
    METHOD_SUBSCRIBE,
    METHOD_UNSUBSCRIBE,
    METHOD_POLL,
    METHOD_REPORT,
    METHOD_MKACTIVITY,
    METHOD_CHECKOUT,
    METHOD_MERGE,
    /* Extension methods must be last, Add any new methods before this line */
    METHOD_EXT00,
    METHOD_EXT01,
    METHOD_EXT02,
    METHOD_EXT03,
    METHOD_EXT04,
    METHOD_EXT05,
    METHOD_EXT06,
    METHOD_EXT07,
    METHOD_EXT08,
    METHOD_EXT09,
    METHOD_EXT10,
    METHOD_EXT11,
    METHOD_EXT12,
    METHOD_EXT13,
    METHOD_EXT14,
    METHOD_EXT15,
    METHOD_EXT16,
    METHOD_EXT17,
    METHOD_EXT18,
    METHOD_EXT19,
    METHOD_ENUM_END
};

typedef unsigned int method_t;

/* request method str stuff; should probably be a String type.. */
struct rms { 
    char *str;
    int len; 
};
typedef struct rms rms_t;

/* recognized or "known" header fields; @?@ add more! */
typedef enum {
    HDR_UNKNOWN = -1,
    HDR_ACCEPT = 0,
    HDR_ACCEPT_CHARSET,
    HDR_ACCEPT_ENCODING,
    HDR_ACCEPT_LANGUAGE,
    HDR_ACCEPT_RANGES,
    HDR_AGE,
    HDR_ALLOW,
    HDR_AUTHORIZATION,
    HDR_CACHE_CONTROL,
    HDR_CONNECTION,
    HDR_CONTENT_BASE,
    HDR_CONTENT_DISPOSITION,
    HDR_CONTENT_ENCODING,
    HDR_CONTENT_LANGUAGE,
    HDR_CONTENT_LENGTH,
    HDR_CONTENT_LOCATION,
    HDR_CONTENT_MD5,
    HDR_CONTENT_RANGE,
    HDR_CONTENT_TYPE,
    HDR_TE,
    HDR_TRANSFER_ENCODING,
    HDR_TRAILER,
    HDR_COOKIE,
    HDR_DATE,
    HDR_ETAG,
    HDR_EXPIRES,
    HDR_FROM,
    HDR_HOST,
    HDR_IF_MATCH,
    HDR_IF_MODIFIED_SINCE,
    HDR_IF_NONE_MATCH,
    HDR_IF_RANGE,
    HDR_LAST_MODIFIED,
    HDR_LINK,
    HDR_LOCATION,
    HDR_MAX_FORWARDS,
    HDR_MIME_VERSION,
    HDR_PRAGMA,
    HDR_PROXY_AUTHENTICATE,
    HDR_PROXY_AUTHENTICATION_INFO,
    HDR_PROXY_AUTHORIZATION,
    HDR_PROXY_CONNECTION,
    HDR_PUBLIC,
    HDR_RANGE,
    HDR_REQUEST_RANGE,          /* some clients use this, sigh */
    HDR_REFERER,
    HDR_RETRY_AFTER,
    HDR_SERVER,
    HDR_SET_COOKIE,
    HDR_UPGRADE,
    HDR_USER_AGENT,
    HDR_VARY,
    HDR_VIA,
    HDR_EXPECT,
    HDR_WARNING,
    HDR_WWW_AUTHENTICATE,
    HDR_AUTHENTICATION_INFO,
    HDR_X_CACHE,
    HDR_X_CACHE_LOOKUP,         /* tmp hack, remove later */
    HDR_X_FORWARDED_FOR,
    HDR_X_REQUEST_URI,          /* appended if ADD_X_REQUEST_URI is #defined */
    HDR_X_SQUID_ERROR,
    HDR_NEGOTIATE,
#if X_ACCELERATOR_VARY
    HDR_X_ACCELERATOR_VARY,
#endif
    HDR_X_ERROR_URL,            /* errormap, requested URL */
    HDR_X_ERROR_STATUS,         /* errormap, received HTTP status line */
    HDR_X_HTTP09_FIRST_LINE,    /* internal, first line of HTTP/0.9 response */
    HDR_FRONT_END_HTTPS,
    HDR_PROXY_SUPPORT,
    HDR_KEEP_ALIVE,
    HDR_OTHER,
#ifdef CC_FRAMEWORK
        HDR_LOCAL_OBJECT,
#endif
    HDR_ENUM_END
} http_hdr_type;

/* big mask for http headers */
typedef char HttpHeaderMask[(HDR_ENUM_END + 7) / 8];

struct _http_version_t {
    unsigned int major;
    unsigned int minor;
};
typedef struct _http_version_t http_version_t;

typedef enum {
    HTTP_STATUS_NONE = 0,
    HTTP_CONTINUE = 100,
    HTTP_SWITCHING_PROTOCOLS = 101,
    HTTP_PROCESSING = 102,      /* RFC2518 section 10.1 */
    HTTP_OK = 200,
    HTTP_CREATED = 201,
    HTTP_ACCEPTED = 202,
    HTTP_NON_AUTHORITATIVE_INFORMATION = 203,
    HTTP_NO_CONTENT = 204,
    HTTP_RESET_CONTENT = 205,
    HTTP_PARTIAL_CONTENT = 206,
    HTTP_MULTI_STATUS = 207,    /* RFC2518 section 10.2 */
    HTTP_MULTIPLE_CHOICES = 300,
    HTTP_MOVED_PERMANENTLY = 301,
    HTTP_MOVED_TEMPORARILY = 302,
    HTTP_SEE_OTHER = 303,
    HTTP_NOT_MODIFIED = 304,
    HTTP_USE_PROXY = 305,
    HTTP_TEMPORARY_REDIRECT = 307,
    HTTP_BAD_REQUEST = 400,
    HTTP_UNAUTHORIZED = 401,
    HTTP_PAYMENT_REQUIRED = 402,
    HTTP_FORBIDDEN = 403,
    HTTP_NOT_FOUND = 404,
    HTTP_METHOD_NOT_ALLOWED = 405,
    HTTP_NOT_ACCEPTABLE = 406,
    HTTP_PROXY_AUTHENTICATION_REQUIRED = 407,
    HTTP_REQUEST_TIMEOUT = 408,
    HTTP_CONFLICT = 409,
    HTTP_GONE = 410,
    HTTP_LENGTH_REQUIRED = 411,
    HTTP_PRECONDITION_FAILED = 412,
    HTTP_REQUEST_ENTITY_TOO_LARGE = 413,
    HTTP_REQUEST_URI_TOO_LONG = 414,
    HTTP_UNSUPPORTED_MEDIA_TYPE = 415,
    HTTP_EXPECTATION_FAILED = 417,
    HTTP_UNPROCESSABLE_ENTITY = 422,    /* RFC2518 section 10.3 */
    HTTP_LOCKED = 423,          /* RFC2518 section 10.4 */
    HTTP_FAILED_DEPENDENCY = 424,       /* RFC2518 section 10.5 */
    HTTP_SPECIAL=499,       /*chinacache special for customized_server_side_error_page added by jiangbo.tian */
    HTTP_INTERNAL_SERVER_ERROR = 500,
    HTTP_NOT_IMPLEMENTED = 501,
    HTTP_BAD_GATEWAY = 502,
    HTTP_SERVICE_UNAVAILABLE = 503,
    HTTP_GATEWAY_TIMEOUT = 504,
    HTTP_HTTP_VERSION_NOT_SUPPORTED = 505,
    HTTP_INSUFFICIENT_STORAGE = 507,    /* RFC2518 section 10.6 */
    HTTP_INVALID_HEADER = 600,  /* Squid header parsing error */
    HTTP_HEADER_TOO_LARGE = 601 /* Header too large to process */
} http_status;
/* http status line */
struct _HttpStatusLine {
    /* public, read only */
    http_version_t version;
    const char *reason;         /* points to a _constant_ string (default or supplied), never free()d */
    http_status status;
};
typedef struct _HttpStatusLine HttpStatusLine;

struct _HttpMsgBuf {
    const char *buf;
    size_t size;
    /* offset of first/last byte of headers */
    int h_start, h_end, h_len;
    /* offset of first/last byte of request, including any padding */
    int req_start, req_end, r_len;
    int m_start, m_end, m_len;
    int u_start, u_end, u_len;
    int v_start, v_end, v_len; 
    int v_maj, v_min;
};
typedef struct _HttpMsgBuf HttpMsgBuf;


struct _HttpHeaderEntry {
//    http_hdr_type id;
    int active;		// 1: will be send to CA
    String name;
    String value;
};
typedef struct _HttpHeaderEntry HttpHeaderEntry;

/* possible owners of http header */
typedef enum {
    hoNone,
#if USE_HTCP
    hoHtcpReply,
#endif
    hoRequest,
    hoReply
} http_hdr_owner_type;

struct _HttpHeader {
    /* protected, do not use these, use interface functions instead */
    Array entries;              /* parsed entries in raw format */
//  HttpHeaderMask mask;        /* bit set <=> entry present */
    http_hdr_owner_type owner;  /* request or reply */
    int len;                    /* length when packed, not counting terminating '\0' */

    method_t method;
    String uri;			// request uri 
    String url;			// request url -- http://host/uri
    http_version_t http_ver;	// http version

    String ip;			// web server ip, get from FC 
    int size;			// ori header size \r\n\r\n|

    HttpStatusLine sline;

    char header_ok;
    void *data;			// point to its own fd_struct
};
typedef struct _HttpHeader HttpHeader;

typedef struct _fd_struct
{
	int inuse;
        uint32_t fd;
	char epoll_flag;
	char type;

	time_t setup_time;

	char *read_buff;
	char *write_buff;
        uint32_t read_buff_size;
	uint32_t write_buff_size;

	uint32_t read_count;
	uint32_t write_count;

	uint32_t read_buff_pos;
	uint32_t write_buff_pos;

	char close_delay;
	char read_delay;
	char write_delay;
//	char header_ok;
	char conn_status;
	char need_post;

	HttpHeader header;
	struct in_addr ca_addr;

	void *data;
} fd_struct;


typedef struct _Packer
{
	char *buf;		//point to the begin of the buffer to be append
	uint32_t offset;	//current offset, must < size -1
	uint32_t size;		//buffer size
	uint32_t full;		//full?
} Packer;


typedef void epoll_handle_accept(int listen_fd);
typedef int epoll_handle_read_event(int fd);
typedef int epoll_handle_write_event(int fd);

extern server_config config;
extern fd_struct *fd_table;

extern char req_list[MAX_HEADER_NUM][MAX_HEADER_LEN] ;
extern char rep_list[MAX_HEADER_NUM][MAX_HEADER_LEN] ;
extern char content_type_list[MAX_HEADER_NUM][MAX_HEADER_LEN] ;

extern unsigned int rep_status_list[128];
extern int rep_status_num;

extern const char* HOST_IP_HEADER;

//////////////////////////////////////////
extern int server_epoll_wait(epoll_handle_accept *accept_handler,epoll_handle_read_event *read_handler,epoll_handle_write_event *write_handler);
extern void server_epoll_init(void);
extern int server_epoll_add(int fd,int flag);
extern int server_epoll_mod(int fd,int flag);
extern int server_epoll_remove(int fd);
extern void server_epoll_dest(void);
extern int fd_close_onexec(int fd);
extern int fd_setnonblocking(int fd);
extern void disconnect_fd(int fd, int close_type);
extern int ignore_errno(int ierrno);
extern int is_socket_alive(int sock_fd);
extern char* get_peer_ip(int socket_fd);
extern unsigned long ELFhash(const char *key);

extern int config_parse(const char *confile);
extern void daemonize(void);
extern pid_t get_running_pid(void);
extern int check_running_pid(void);

extern char *
xstrncpy(char *dst, const char *src, size_t n);

extern void
stringLimitInit(String * s, const char *str, int len);
extern void
stringClean(String * s);
extern void
stringInit(String * s, const char *str);
extern String
stringDup(const String * s);

extern int get_ca_ip(char *host, int type, const char *uri, struct in_addr * addr);
extern int mark_bad_ip(struct in_addr addr);
extern int hash_request_uri;
extern int ca_host_type;

extern void dump_ca_host_ip();
extern int init_ca_host_ip(char * host, int type);

extern int process_header_real(HttpHeader * header, char *buf, int len);

extern void free_http_header(HttpHeader * header);

#endif
