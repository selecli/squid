#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <dlfcn.h>
#include <sys/utsname.h>
#include <time.h>
#include <math.h>
#include "redirect_conf.h"
#include "md5.h"

#define MAX_PATH_SIZE 1024
#define MAX_SECTION_SIZE 100
#define MAX_URL_SIZE 4096

#if 1
extern int g_fdDebug;
extern FILE *g_fpLog;
#define CRITICAL_ERROR(args, ...)    fprintf(g_fpLog, args, ##__VA_ARGS__);fflush(g_fpLog);
#define NOTIFICATION(args,...)       fprintf(g_fpLog, args, ##__VA_ARGS__);fflush(g_fpLog);
#define DEBUG(args, ...)             if(g_fdDebug > 3) {fprintf(g_fpLog, args, ##__VA_ARGS__);fflush(g_fpLog);}

#else

#define CRITICAL_ERROR(args, ...)    printf(args, ##__VA_ARGS__);fflush(stdout);
#define NOTIFICATION(args,...)       printf(args, ##__VA_ARGS__);fflush(stdout);
#define DEBUG(args, ...)             printf(args, ##__VA_ARGS__);fflush(stdout);

#endif
static int parse_url(char *url, char *md5, char *time ,char *path)
{
	char *p_url = NULL;

	char url_bak[MAX_URL_SIZE];
	strcpy(url_bak,url);

	char *pstr = strtok_r(url_bak,"/",&p_url);
	if ( p_url == NULL )
	{
		DEBUG("url format error");
		return -1;
	}

	strtok_r(NULL,"/",&p_url);

	if ( p_url == NULL )
	{
		DEBUG("url format error");
		return -1;
	}

	strtok_r(NULL,"/",&p_url);
	if ( p_url == NULL )
	{
		DEBUG("url format error");
		return -1;
	}
	//get md5
	pstr = strtok_r(NULL,"/",&p_url);
	if(pstr!=NULL && p_url!=NULL)
	{
		strcpy(md5,pstr);
		DEBUG("md5 = %s\n",pstr);
	}else
	{
		return -1;

	}
	// get time	
	pstr = strtok_r(NULL,"/",&p_url);

	if(pstr!=NULL && p_url!=NULL)
	{
		strcpy(time,pstr);
		DEBUG("time = %s\n",pstr);
	}
	else
	{
		DEBUG("url format error");
		return -1;
	}

//	DEBUG("p_url = %s\n",p_url);

	char *end = strchr(p_url,'?');
	if (end) *end = 0;

	strncpy(path+1,p_url,strlen(p_url)); //path start with '/'

	DEBUG("path = %s\n",p_url);

	return 1;
}

long  hex_to_dec(char*s)
{
	int i,t;
	long sum=0;
	for(i=0;s[i];i++)
	{
		if(s[i]<='9')t=s[i]-'0';
		else  t=s[i]-'a'+10;
		sum=sum*16+t;
	}
	return sum;
}

int is_time_valid(time_t time_request,int seconds_before , int seconds_after)
{
	time_t now = time(NULL);
	DEBUG("[time now] = %d\t[request_time] = %d\t[seconds_before]= %d\t [seconds_after]= %d\n",(int)now,(int)time_request,seconds_before,seconds_after);
	if((now > time_request + seconds_before) || (now < time_request - seconds_after))
		return -1;

	return 1;
}

int lighttpd_secdownload_verify(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other)
{
	char *fail_dst = NULL;
	char md5[MAX_SECTION_SIZE] = "0";
	char time[MAX_SECTION_SIZE] = "0"; 
	char path[MAX_PATH_SIZE] = "/"; //path start with '/' 

	int url_ok = 0;
	url_ok = parse_url(url,md5,time,path);
	if (url_ok < 0)
	{
		goto fail403;
	}
	struct valid_period* pstValidPeriod = (struct valid_period*)pstRedirectConf->other;
	time_t time_request = (time_t)hex_to_dec(time);
	int ret = is_time_valid(time_request, pstValidPeriod->seconds_before, pstValidPeriod->seconds_after);
	if(-1 == ret)
	{
		DEBUG("time in url is no in range\n");
		goto fail403;
	}

	int hash_checker = -1;
	char hash_origin[MAX_URL_SIZE]; 
	char md5_digest[32];
	if(NULL != pstRedirectConf->key)
	{
		ret = snprintf(hash_origin, MAX_URL_SIZE, "%s%s%s", pstRedirectConf->key, path, time);
		DEBUG("hash origin=[%s]\n", hash_origin);
		getMd5Digest32(hash_origin, ret, (unsigned char *)md5_digest);
		DEBUG("hash=[%.*s]\n", 32, md5_digest);
		if(0 == strncasecmp(md5_digest+pstRedirectConf->md5_start, md5, pstRedirectConf->md5_length))
		{
			hash_checker = 0;
		}
	}
	//如果第一个密码没有通过，检查第二个密码
	if((-1==hash_checker) && (NULL!=pstRedirectConf->key2))
	{
		ret = snprintf(hash_origin, MAX_URL_SIZE, "%s%s%s", pstRedirectConf->key, path, time);
		DEBUG("second hash origin=[%s]\n", hash_origin);

		getMd5Digest32(hash_origin, ret, (unsigned char *)md5_digest);
		DEBUG("second hash=[%.*s]\n", 32, md5_digest);

		if(0 == strncasecmp(md5_digest+pstRedirectConf->md5_start, md5, pstRedirectConf->md5_length))
		{
			hash_checker = 0;
		}
	}
	if (-1 == hash_checker )
	{
		DEBUG("hash check not compiltile\n");
		goto fail403;
	}

	//url now have parsed,so we can do this assert
	char *pstr = url;
	int i =0 ;
	for(;i<3;i++)
	{
		assert(pstr ++);
		pstr = strchr(pstr,'/');
	}

	DEBUG("pstr = %s\n",pstr);
	*pstr = 0;
	strcat(url,path);

	printf("%s %s %s", url, ip, other);
	DEBUG("OUTPUT: %s %s %s\n", url, ip, other);
	fflush(stdout);
	return 1;

fail403:
	fail_dst = pstRedirectConf->fail_dst;
	DEBUG("fail_dst %s\n",fail_dst);
	if(NULL == fail_dst)
	{
		fail_dst = "http://www.chinacache.com";
	    CRITICAL_ERROR("No redirect url, use default [%s]\n", fail_dst);
	}
	printf("%s %s %s", fail_dst, ip, other);
	fflush(stdout);
	DEBUG("VERIFY FAILED OUTPUT:%s %s %s\n", fail_dst, ip, other);
	return 0;
}
