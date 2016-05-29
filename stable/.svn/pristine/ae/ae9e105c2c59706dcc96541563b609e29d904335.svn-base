#ifndef DETECT_ORIG_H
#define DETECT_ORIG_H

#include <sys/epoll.h>
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
#include <fnmatch.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <math.h>
#include "log.h"
#include "mempool.h"
#include "framework.h"
#include "dbg.h"
#include "shortest_path.h"
#include "hashtable.h"

#define MIN(a, b)	(a) > (b)? (b):(a)
#define MAX(a, b)	(a) > (b)? (a):(b)

#define DETECT_VERSION "V1.26"
//#define FD_MAX  25600
//#define MAX_EVENTS 25600
#define FD_MAX  344800
#define MAX_EVENTS 344800 

#define PROCESS_TIMEOUT_DEFAULT (10 * 60)

#define EPOLL_TIMEOUT 100   //milliseconds
//#define EPOLL_TIMEOUT 1000   //milliseconds
#define CBUFFSIZE (2048)
#define MAX_BUF_SIZE 1024

#define INITIAL_VALUE 3000
#define INCREASE_STEP 500
#define MAX_ERROR_INFO_SIZE 200
#define MAX_IP_ONE_LINE 64
#define MAX_HOST_LEN 200
#define MAX_ONE_LINE_LENGTH 2048
#define OFFSET_OF(type, member) ((size_t)(char *)&((type *)0L)->member)
#define cbdataLock(a)       cbdataLockDbg(a,__FILE__,__LINE__) 
#define cbdataUnlock(a)     cbdataUnlockDbg(a,__FILE__,__LINE__)
//static const int kLINK_DATA_READ_COUNT_MAX = 3;     /* 最多读取kLINK_DATA_READ_COUNT_MAX个link.data文件中的数据 */
//static const int kLINK_DATA_IGNORE_COUNT = 1;       /* 忽略掉最差的几个结果 */
enum
{
    kFC_LINK_DATA_READ_COUNT_MAX = 1,           /* 最多读取kLINK_DATA_READ_COUNT_MAX个link.data文件中的数据 */
    kFC_LINK_DATA_IGNORE_COUNT = 1              /* 忽略掉最差的几个结果 */
};
enum {
	RF_OK,
	RF_ERR,
	RF_ERR_EPOLL,
	RF_EPOLL_TIMEOUT,
};

enum {
	CON_INPROGRESS,
	CON_OK,
	CON_ERROR,
};

enum {
	FD_STS_UNCONNECT,
	FD_STS_CONNECTED,
	FD_STS_READY_SEND,
	FD_STS_HAVE_SENDED,
	FD_STS_READY_RCV,
	FD_STS_HAVE_RCVED,
};


struct DetectOrigin
{
	int fd;
	int inuse;
	char host[128];
	int info;   //写到哪一个文件。n--写到anyhost里，否则写到anyhost.info里。
	int recent_added;
	int detect;   //是否需要检测，yes为1，侦测，no为0
	int times;    //侦测次数
	double warning_time;  //告警时间
	double good_time;   //小于这个时间的响应时间认为比较好
	int ignoreDetectTimeChgPercent; //good ip有多个时侯,如果小于规定百分比，排序不变
	int length;   //取源站内容的长度
	int goodIp;   //如果有好的ip，最多写几个
	int modify;   //是否按照探测结果修改，yes为1，按照探测结果修改；修改，no为0
	char backup[320];  //
	int backupIsIp;		//XXX
	struct IpDetect* bkip;  //一个主机对应的多个ip
	int nbkip; /* backup 的个数 */
	int method;   //方法，1为get，3为head；如果为default，为head
	char rHost[128];  //发请求的主机，从方法得到，否则取host
	char filename[1024]; //请求的文件名，从方法得到。默认为时间加上.jpg
	char* code;   //返回代码，四个一组，*通配，第一个字符为0表示结束
	int ipNumber; //ip个数，下面的数组对应的ip比这个多一个，为了适应增加监察backup的ip
	struct IpDetect* ip;  //一个主机对应的多个ip
	//Added by chinacache zenglj
	int method2;	//XXX
	char filename2[1024];
	char rHost2[128];
	int port;   //port for detect
	int origDomain;   //1: use dns detect, 0: not use
	int weight0;    //for example, 0:100, 20:80, 100:0
	int weight1;
	int hasRefer;   //1:detect has refer; 0: no refer;
	int historyCount; //counts for retry
	int hasOrigDomain;//有外部ip
	int detected;		//已经detect ok
	int ready;		//ip 准备好了
	int origIpNum;	//外部ip的个数
	struct IpDetect* origip; //外部ip列表
#ifdef ROUTE
    int final_ip_number;    //最终ip的数量
    uint32_t *final_ip;  // network byte order /* TODO do free() */
    uint16_t *final_ip_port;    // network byte order
#endif /* ROUTE */
	int sortIp;
	int handled_ip_index;	//已经处理了的ip序号	Xcell
	int handled_times;		//已经测试的次数	Xcell

	int flag_http_tcp;  // 0:http 1:tcp
	int tcp_detect_time;
	int http_detect_time;
	int detect_time;

	char rtn_code[32];

	int ip_index;	
	int task_index;
	
	struct timeval start;
	struct timeval end;
	double usedtime;

	int flag_refer;
	int flag_fd_sts;

	int send_len;
	int need_send_len;
	char * current_write_buf;

	int read_len;
	int need_read_len;
	char * current_read_buf;

};
typedef struct DetectOrigin DetectOrigin_fd; 

typedef struct _DetectOriginIndex
{
    //struct DetectOrigin *detect_origin;
    char *hostname;
    HashKey sum;
    HashKey xor;
} DetectOriginIndex; /* - - - - - - end of struct DetectOriginIndex - - - - - - */

/*一个要检查的ip的结构*/
struct IpDetect
{
	char ip[16];
	int upper_ip; /* 0: source ip, 1: upper ip */
	int reuse_nodetect_flag; /* 0: detect, 1: no detect, but use other host[same IP] detect result */
	int reuse_task_index;
	int reuse_ip_index;
	int sockfd ; //added by huangyong
	int ok;   //检查结果，0--失败，1--成功
	char returnCode[4];
    unsigned int used_time_count;
    double used_time[kFC_LINK_DATA_READ_COUNT_MAX];
	double usedtime;  //检查花费的时间，以秒为单位
	double usedtime1;
#ifdef ROUTE
    uint32_t final_ip; //存储usedtime对应的最终服务的ip
    uint16_t final_ip_port;
#endif /* ROUTE */
};

//added by chinacache zenglj
struct IpDnsDetect
{
	char ip[16];
	int ok;
	//int usedIp; //record which ip is good
};

struct origDomain
{
	char host[128];
	char origDomain[256];
	int useDefault;
	int ipNum;
	struct IpDnsDetect* ip;
	int usedIP;
	int success;
	int needDetect;
	double usedtime;
	int working;
};

struct origDomainHistory
{
	char host[128];
	char origDomain[256];
	//struct IpHistory* ip;
	struct IpDetect* ip;
	int ipNum;
	int failtimes;
	int sequence; //added for multi origdomain in history file
};

typedef void CNCB(int );
typedef void EVH(void *);
typedef void FREE(void *);
typedef struct
{
	char *host;
	int  port;
	struct sockaddr_in S;
	CNCB *callback;
	CNCB *callback_no;
//	void *data;
	int data;
	struct in_addr in_addr;
	int fd;
	int tries;
	struct timeval start;
//	int addrcount;
//	int connstart;
}ConnectStateData;

typedef struct detect_domain_st
{
	char detect_domain[MAX_HOST_LEN];
} detect_domain_t;

typedef struct detect_custom_st
{
	char ip[16];
	int domain_count;
	int reuse_task_index;
	int reuse_ip_index;
	detect_domain_t *domain;
} detect_custom_t;

struct config_st
{
	int model; /* 0: -f, 1: -F */
	int processNumber;
	double timer;
	char confFilename[128];
	char OrigDomainFileName[128];
	char origDomainIPHistoryFileName[128];
	char anyhost[128];
	char anyhosttmp[256];
};

typedef struct detect_custom_header_st
{
	char host[128];
	char hdr_name[128];
	char header[256];
} custom_hdr_t;

struct detect_custom_conf_st
{
	long detect_timeout;
	double proportion;
	custom_hdr_t *hdrs;
	int hdr_count;
};

struct detect_upper_ip_st
{
	char ip[16];
};

//global
extern int max_fd;
extern int mod_fc_debug_level;
extern float conf_timer;
extern DetectOrigin_fd *fd_table; 
extern struct config_st config;
extern struct detect_custom_conf_st custom_cfg;
extern int g_iOrigDomainItem;
extern struct origDomain **g_pstOrigDomain;
extern struct origDomainHistory **g_pstOrigDomainHistory;
extern int g_iOrigDomainHistoryItem;

extern HashTable *g_detect_tasks_index;
struct DetectOrigin **g_detect_tasks;           //record all the hosts information
int max_tasks_capacity;                         //current memory capacity of the tasks which malloced in need
//int increase_step;   //increase size for realloc
//extern detect_domain_t *g_detect_domain;
extern HashTable *g_detect_domain_table;
int max_detectDomain_capacity;
extern int g_detect_task_count;                 //配置文件里实际有多少条记录
extern int g_detect_anyhost_count;
extern int handled_task_index;                  //detect中上次执行的task的序号
extern int count;                               //detect中上次执行的task的序号
extern int detect_time;

/* fixed by xin.yao: 2011-11-14 */
int max_origDomain_capacity;
int max_origDomainHistory_capacity;
//int have_recent_added;
int max_detectCustom_capacity;
int max_detectUpperIp_capacity;
/* end by xin.yao: 2011-11-14 */

extern float conf_timer;
extern struct timeval current_time;
extern time_t detect_curtime;
extern double current_dtime;
//extern long g_detect_timeout;
extern char *custom_keyword[];

void initLog(const char* processName);
void addOneLog(const char* str);
int getDetectOrigin(const char * filename);
int getRecentAddedFromAnyhost(const char *filename);
int checkRepeatRecordFromDomainConf(void);
int getOrigDomainConf(FILE* fp);
int getDetectUpperIpConf(FILE *fp);
int getOrigDomainHistory(FILE* fp);
int getDetectCustomConf(FILE* fp);
void printDetectOrigin();
int detectOrigin(int processNumber, const char* processName, double timer);
void modifyUsedTime();
int getNewAnyhost(int info, FILE* fpNamed, FILE* fpNew);
int detect(double timer1);
#ifndef FRAMEWORK
int addResultLog(void);
#else /* FRAMEWORK */
int ResultLogInit( void );
int ResultLogAdd( const struct DetectOrigin *detect_origin, const struct IpDetect *ip );
int ResultLogEnd( void );
#endif /* FRAMEWORK */
int addInfoLog(int type, const char* str);
int addAlertLog(int type, char* host, char* ipBuf);
int writeOrigDomainIP(void);
void str_rtrim(char* str, const char* tokens);

int getCode(const char* str, char** code);
int getIp(char* str, struct IpDetect** ip);
int checkOneIp(const char* ip);
int inCode(const char* oneCode, const char* code);
int getOneProcessOutput(char *const argv[], char* buffer, int length);
int write_pid_file(const char* filename);
int make_new_ip(struct DetectOrigin* pstDetectOrigin);
void free_rfc_detectOrigin(struct DetectOrigin* pstDetectOrigin);

int clean_detect_mem(void);
int start_connect(struct DetectOrigin *task);
void handle_accept(int listen_fd);
int handle_read_event(int fd);
int after_handle_read_event(int fd);
int handle_write_event(int fd);
void copySameIPDetectRes(void);
void dealwithDomainHostModelF(int info, FILE* fpNew,  struct DetectOrigin *pstDetectOrigin);


//epoll start
typedef int epoll_handle_read_event(int fd);
typedef int epoll_handle_write_event(int fd);

void dump_detectorigin(void);
int detect_epoll_add(int fd,int flag);
int detect_epoll_mod(int fd,int flag);
int detect_epoll_remove(int fd);
void detect_epoll_init(void) ;
void detect_epoll_dest(void);
int detect_epoll_wait(epoll_handle_read_event *read_handler,epoll_handle_write_event *write_handler);
void disconnect_fd(int fd);
//epoll end
time_t getCurrentTime(void);
int handle_detect_connect(int fd, void *data);
void eventAdd(const char *name, EVH * func, void *arg, double when, int weight);
void eventRun(void);
void eventDump(void);
void eventFreeMemory(void);
int DetectOriginIndexFill( DetectOriginIndex *index, char *host );
struct DetectOrigin* SearchHost( char *host );
HashKey DetectOriginIndexGetKey( const void *data );
int DetectOriginIndexIsEqual( const void *data1, const void *data2 );
int CalculateUsedTime( void );
void check_named(void);
void modify_anyhost_timestamp();

#endif	//DETECT_ORIG_H

