#ifndef PING_ICMP_H
#define PING_ICMP_H

#include <time.h>
#include <glob.h>
#include <stdio.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <assert.h>
#include <stdarg.h>
#include <net/if.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <netinet/ip_icmp.h>

#define BUF_SIZE_16 16
#define BUF_SIZE_256 256
#define BUF_SIZE_512 512
#define BUF_SIZE_1024 1024
#define BUF_SIZE_2048 2048
#define BUF_SIZE_4096 4096
#define ICMP_TIMEOUT  100000             //microseconds
#define SAVETIME_DEFAULT 86400           //one day seconds(24 hours)
#define LOG_ROTATE_NUMBER 5
#define ICMP_DFT_PNUM  10                //default icmp send packets number
#define ICMP_DFT_PSIZE 56                //default icmp packet size: 64 = 56 + 8
#define ICMP_DFT_ITIME 20                //default icmp send interval time: microsecond
#define ICMP_MAX_PSIZE 4096              //max packets data length: byte
#define ICMP_MAX_ITIME 500000            //max iterval packet send time: microsecond
#define ICMP_MAX_PNUM  100               //max packets for sending
#define LOG_ROTATE_SIZE 10240000         //10M bytes
#define MAX_HASH_NUM 500000              //hashtable size
#define MODE_FC 0x01                     //detect for FC
#define MODE_TTA 0x02                    //detect for TTA
#define MODE_NOFC ~(0x01)
#define MAX_FP_NUM 20
#define XPING_FD_MAX 204800

#define PING_LOG_DIR                     "/var/log/chinacache"
#define XPING_PID_DIR                    "/var/run/xping"
#define XPING_PID_FILE                   "/var/run/xping/xping.pid"
#define PING_LOG_FILE                    "/var/log/chinacache/xping.log"
#define CONFIG_DOMAIN_FC                 "/usr/local/squid/etc/domain.conf"
#define CONFIG_ORIGIN_DOMAIN_IP          "/usr/local/squid/etc/origin_domain_ip"
#define CONFIG_DOMAIN_TTA                "/usr/local/haproxy/etc/tta_domain.conf"
#define XPINGDATA_DIR                    "/var/log/chinacache/xping"

#define XPING_VERSION                    "xping V1.0"

#define xlog(logtype, fmt, args...) pingLog(logtype, __FILE__, __LINE__, fmt, ##args)

enum {
	TRUE = -100,
	FALSE,
	SUCC,
	FAILED,
};

enum log {                   //here must begin from 0, and order can't changed, reference to debug level
	COMMON = 0,              //log info without any keyword
	WARNING,                 //log info begin with keyword "WARNING"
	ERROR,                   //log info begin with keyword "ERROR"
	FATAL,                   //log info begin with keyword "FATAL"
	LOG_LAST                 //end
};

enum llist {
	HEAD,
	TAIL
};

enum time {
	TIME_TIME,
	TIME_TIMEVAL
};

typedef void LLIST;

typedef struct config_domain_st {                     //configuration data structure
	int mode;                                 //choose mode for FC or TTA
	char dnsIpFile[BUF_SIZE_256];             //dns resolved ip file
	char domainFile[BUF_SIZE_256];            //domain.conf
	char ttaConfFile[BUF_SIZE_256];           //tta_domain.conf
} config_domain_t;

typedef struct hash_factor_st {
	char ip[BUF_SIZE_16];
	struct hash_factor_st *next;
} hash_factor_t;

typedef struct ping_typesize_st {
	int ping_packet_t_size;
	int hash_factor_t_size;
} ping_typesize_t;

typedef struct ping_option_st {
	int print;                                //print debug information
	int travel;                               //travel number
	int dblevel;                              //debug level
	int dlstart;                              //delay start
	int cleanup;                              //if do internal clean or not[on: yes; off: no]
	int central;                              //once when more than one same ip
	int epollwait;                            //epoll_wait timeout
	double timeout;                           //how long to disconnect one connection
	time_t killtime;                          //how long to kill current program instance
	time_t savetime;                          //how long to save link.data files
	time_t interval;                          //interval times to delete file link.data.xxx
} ping_option_t;

typedef struct ping_packet_st {
	int npack;                                //packet number
	int psize;                                //packet size
	unsigned int lossrate;                    //packets loss rate
	unsigned long itime;                      //packet send interval time
	double rtt;                               //round trip time
	char ip[BUF_SIZE_16];                     //ip
} ping_packet_t;

typedef struct ping_icmp_st {
	LLIST *list;
	FILE *logfp;
	char localaddr[BUF_SIZE_16];                     //ip
	ping_option_t opts;
	ping_typesize_t tsize;
	config_domain_t config;
} ping_icmp_t;

extern ping_icmp_t xping;
extern hash_factor_t *hashtable[MAX_HASH_NUM];

typedef void TRCB(void *data);                               //event travel callback
typedef int COMP(void *key, void *data);

void xtime(const int type, void *curtime);
void xfree(void **ptr);
void *xmalloc(size_t size);
void *xcalloc(size_t n, size_t sz);
void *xrealloc(void *ptr, size_t size);
void xabort(const char *info);
void getLocalAddress(void);
void globalDestroy(void);
int xpingIcmp(ping_packet_t *pack);
int openLogFile(void);
void pingLog(const int logtype, const char *filename, const int nline, char *format, ...);
void cleaner(void);
//llist
LLIST *llistCreate(int size);
int llistNumber(LLIST *head);
int llistEmpty(LLIST *head);
int llistIsEmpty(LLIST *head);
void llistDestroy(LLIST *head);
void *llistInsert(LLIST *head, void *data, int mode);
int llistDelete(LLIST *head, void *key, COMP *cmp);
void *llistFind2(LLIST *head, void *key, COMP *cmp);
int llistFind(LLIST *head, void *key, COMP *cmp, void *ret);
int llistTravel(LLIST *head, TRCB *travel);
void llistTravel2(LLIST *head, TRCB *travel, int number);
int llistModify(LLIST *head, void *key, COMP *cmp, void *modify);

void globalStart(int argc, char **argv);
void configStart(void);

#endif

