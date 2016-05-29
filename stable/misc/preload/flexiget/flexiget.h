#ifndef __FLEXIGET_H
#define __FLEXIGET_H

#include "config.h"
#include "lib.h"

#include <time.h>
#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <netdb.h>
#ifndef	NOGETOPTLONG
#define _GNU_SOURCE
#include <getopt.h>
#endif
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <pthread.h>

#define _( x )			x

/* Compiled-in settings							*/
#define MAX_STRING		256
#define MAX_REDIR		5
#define flexiget_VERSION_STRING	"3.0"
#define USER_AGENT		"flexiget " flexiget_VERSION_STRING " (" ARCH ")"

#ifdef CC_X86_64
#define PRINTF_UINT64_T "lu"
#else
#define PRINTF_UINT64_T "llu"
#endif


typedef struct
{
	void *next;
	char text[MAX_STRING];
} message_t;

typedef message_t url_t;
//typedef message_t if_t;

#include "conf.h"
#include "tcp.h"
#include "ftp.h"
#include "http.h"
#include "conn.h"
#include "search.h"

#define min( a, b )		( (a) < (b) ? (a) : (b) )
#define max( a, b )		( (a) > (b) ? (a) : (b) )

extern int debug_level;
extern FILE * debug_ouputfp;

/*
#define debug(b, args...) \
		{	\
			if (debug_level || debug_ouputfp!=stderr) {\
				fprintf(debug_ouputfp, b, ## args);\
			}	\
		}
*/
void debug(char * format, ...);

typedef struct
{
	conn_t *conn;	//Xcell 多个连接的信息，是一个数组
	conf_t * conf;	//配置文件信息
	char filename[MAX_STRING];	//存放到的文件
	double start_time;			//任务开始时间
	uint32_t next_state, finish_time;	//
	uint64_t bytes_done, start_byte, size;
	uint32_t bytes_per_second;		//统计下载速率
	uint32_t delay_time;				//推荐下载的时间
	//int outfd;					//output file fd
	FILE * outfp;					//output file fd
	int ready;					//是否程序中止时，已经完成下载
	message_t *message;
	url_t *url;					//下载资料的url
} flexiget_t;		//一个下载任务的全部信息



/*
 * Xcell
 * 初始化flexiget_t这个下载任务的全部信息核心数据结构
 * 其中会获取一次filesize，即先建立一次连接请求
 */
flexiget_t *flexiget_new( conf_t *conf, int count, void *url );


/*
 * Xcell
 * 	初始化每一个下载线程的任务信息，可能从临时文件中获取断点续传信息
 */
int flexiget_open( flexiget_t *flexiget );



/*
 * Xcell 下载任务初始化，多个任务分配管理，建立多个线程，每个线程建立TCP，发送请求
 */
void flexiget_start( flexiget_t *flexiget );



/*
 * Xcell
 * 	多线程下载的接收工作
 *
 * 统一在一个线程中接收多个TCP连接的数据，并统一写文件
 * 这种设计非常巧妙，在不降低太多性能的情况下，让设计简化
 */
void flexiget_do( flexiget_t *flexiget );



/*
 * Xcell 多线程下载结束，清除信息，保存断点文件等
 */
void flexiget_close( flexiget_t *flexiget );


/* Xcell 获取当前时标 */
double gettime();

#endif

