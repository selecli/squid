#ifndef BILLING_H_
#define BILLING_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <squid.h>
#include "mod_billing_hashtable.h"


#define OFFSETOF(type, f) ((size_t)((char *)&((type *)0)->f - (char *)(type *)0))


//通过记费类型直接定位该类型计费字段在结构中的位置
#define	LOCAL_CLIENT	OFFSETOF(struct hash_entry, local.client)		//本地客户端如preload等产生的client流量
#define	LOCAL_SOURCE	OFFSETOF(struct hash_entry, local.source)		//本地客户端如preload等产生的回源站流量
#define	REMOTE_CLIENT	OFFSETOF(struct hash_entry, remote.client)		//远程客户端产生的client流量
#define	REMOTE_SOURCE	OFFSETOF(struct hash_entry, remote.source)		//远程客户端产生的回源站流量
#define	FC_CLIENT		OFFSETOF(struct hash_entry, fc.client)			//远程fc产生的client流量
#define	FC_SOURCE		OFFSETOF(struct hash_entry, fc.source)			//远程fc产生的回源站注量


//squid在reload时，不用重新调动该函数
//host_ip 必须为ip地址，原因1. squid的域名解析太复杂了，有可能出错 2.域名解析太耗时
extern void billing_init(const char* host_ip, const unsigned short port);


//销毁计费功能，squid在reload时，不用重新调用本函数
extern void billing_destroy();


//将一个host与fd绑定，在绑定前，该fd生产的流量仍会被计入
extern void billing_bind(const char* host, int fd, int type);


extern void billing_unbind(int fd);


//查看一个fd所对应的host
extern char* fd2host(int fd);


//创建一个fd后调用
extern void billing_open(int fd);


//socket读流量，在fc系统read后调用
extern void   billing_flux_read(int fd, uint32_t read_size);


//socket写流量，在fc系统write后调用
extern void   billing_flux_write(int fd, uint32_t read_size);


//在系统close(fd)后调用
//关闭一次计费周期，在发送完reply并计费后，调用本函数
extern void billing_close(int fd);


//billing定时同步，包括链接，维护等一系统工作，约每6秒钟调用一次为宜
extern int32_t billing_sync(bool donow);

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

//转换工作，将http://www.xxx.com:99/xxxx 转将host
static inline char* url2host(char* des, char* src)
{
//	assert(src);	//skip asset, squid define it
//	assert(des);

	char *start = src;

	if( memcmp(src, "http://", 7) == 0 ) {
		start += 7;
	}

	if( memcmp(src, "https://", 8) == 0 ) {
		start += 8;
	}

	while( (*start != ':') && (*start != '/') && (*start != 0) ) {
		*des = *start;
		des++;
		start++;
	}

	*des = 0;
	return des;
}


void parse_list();
#define channel_max 150
struct mod_conf_param{
    char type[128];
    char black_list_path[512];
    wordlist *channel;
};

#endif /*BILLING_H_*/

