#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <dlfcn.h>
#include <sys/utsname.h>
#include <time.h>
#include "redirect_conf.h"
#include "md5.h"
#include <sys/types.h>


#if 1
extern int g_fdDebug;
extern FILE *g_fpLog;
#define CRITICAL_ERROR(args, ...)    fprintf(g_fpLog, args, ##__VA_ARGS__);fflush(g_fpLog);
#define NOTIFICATION(args,...)       fprintf(g_fpLog, args, ##__VA_ARGS__);fflush(g_fpLog);
#define DEBUG(args, ...)             if(g_fdDebug > 3) {fprintf(g_fpLog, args, ##__VA_ARGS__);fflush(g_fpLog);}
//#define DEBUG(args, ...)             fprintf(g_fpLog, args, ##__VA_ARGS__);fflush(g_fpLog);

#else

#define CRITICAL_ERROR(args, ...)    printf(args, ##__VA_ARGS__);fflush(stdout);
#define NOTIFICATION(args,...)       printf(args, ##__VA_ARGS__);fflush(stdout);
#define DEBUG(args, ...)             printf(args, ##__VA_ARGS__);fflush(stdout);

#endif
#define MAX_MD5_SIZE 1024
#define MAX_PATH_SIZE 1024
#define MAX_URL_SIZE 1024

//static int timeChangeFrequence = 0; 
typedef struct iqilu
{
    char iqilukey[MAX_MD5_SIZE];
}ST_IQILU_CFG;

/*  解析盛大防盗链配置行
 * iqilu.download.ccgslb.net none iqilu 3325467230CABCC0 http://61.135.207.203/
 * */

int analysisIQILUCfg(struct redirect_conf *pstRedirectConf)
{
	char *pTemp;
	char *pcSep=" \t\r\n";
	ST_IQILU_CFG *pstC = NULL;
	pstC = (ST_IQILU_CFG *)malloc(sizeof(ST_IQILU_CFG));
	if(NULL == pstC)
	{
		CRITICAL_ERROR("No memory\n");
		goto error;
	}
	
/*得到盛大网设定的密码*/
	pTemp = xstrtok(NULL, pcSep);
	if(NULL == pTemp)
	{
		DEBUG("configure file is error,lack of Key");
		goto error;
	}
	DEBUG("pstC->iqilukey is %s\n",pTemp);

	strcpy(pstC->iqilukey,pTemp);

/*得到盛大网设定的错误返回的地址*/
	pTemp = xstrtok(NULL, pcSep);
	if(NULL == pTemp)
	{
		DEBUG("configure file is error,lack of fail_dst");
		goto error;
	}

	pstRedirectConf->fail_dst = malloc(strlen(pTemp) + 1);
	if(NULL == pstRedirectConf->fail_dst)
	{
		CRITICAL_ERROR("No memory\n");
		goto error;
	}
	strcpy(pstRedirectConf->fail_dst, pTemp);
	pstRedirectConf->other = (void*)pstC;

	return 0;
error:
	if(NULL != pstC)
	{
		free(pstC);
	}
    if(NULL != pstRedirectConf->fail_dst)
    {
        free(pstRedirectConf->fail_dst);
    }
	return -1;

}

static void getMD5(unsigned char *mybuffer,int buf_len,char *digest)
{
	MD5_CTX context;
	unsigned char szdigtmp[16];
	bzero(digest,64);
	MD5Init (&context);
	MD5Update (&context, mybuffer, buf_len);
	MD5Final (szdigtmp, &context);
	sprintf(digest,"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",szdigtmp[0],szdigtmp[1],szdigtmp[2],szdigtmp[3],szdigtmp[4],szdigtmp[5],szdigtmp[6],szdigtmp[7],szdigtmp[8],szdigtmp[9],szdigtmp[10],szdigtmp[11],szdigtmp[12],szdigtmp[13],szdigtmp[14],szdigtmp[15]);
	return ;
}

int verifyIqilu(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other)
{
	char *fail_dst = NULL;
	char path[MAX_PATH_SIZE];
	char origin_url[MAX_URL_SIZE];
	memset(path,0,MAX_PATH_SIZE);
	memset(origin_url,0,MAX_URL_SIZE);
	ST_IQILU_CFG* pstC = pstRedirectConf->other;
	assert(pstC != NULL);
	
	char time_key[MAX_PATH_SIZE];
	char md5_key[MAX_PATH_SIZE];
	char file_key[MAX_PATH_SIZE];

	memset(time_key,0,MAX_PATH_SIZE);
	memset(md5_key,0,MAX_PATH_SIZE);
	memset(file_key,0,MAX_PATH_SIZE);

	char *str = "http://";
	char *p = strstr(url,str);
	if ( NULL == p)
	{   
		DEBUG("url format is wrong \n");
		goto fail404;
	}

	p = p + 8;
	//host name
	char *tmp1 = NULL;
	tmp1 = strchr(p,'/');
	if ( NULL == tmp1 ) 
	{   
		DEBUG("url format is wrong \n");
		goto fail404;
	}   
	tmp1++;
	//qitv key
	char *tmp2 = NULL;
	tmp2 = strchr(tmp1,'/');
	if ( NULL == tmp2 ) 
	{   
		DEBUG("url format is wrong \n");
		goto fail404;
	}   

	tmp2++;
	//md5 key
	char *tmp3 = NULL;
	tmp3 = strchr(tmp2,'/');
	if ( NULL == tmp3 ) 
	{   
		DEBUG("url format is wrong \n");
		goto fail404;
	} 
	strncpy(md5_key,tmp2,tmp3-tmp2);
	md5_key[tmp3-tmp2] = '\0';
	  
	tmp3++;
	//time key
	char *tmp4 = NULL;
	tmp4 = strchr(tmp3,'/');
	if ( NULL == tmp4 ) 
	{   
		DEBUG("url format is wrong \n");
		goto fail404;
	}

	strncpy(time_key,tmp3,tmp4-tmp3);
	time_key[tmp4-tmp3] = '\0';
	 
	//video file 
	strcpy(file_key,tmp4);

	char *r = NULL;
	r = strstr(file_key,"?");
	if(NULL != r)
	{
		*r = '\0';
	}


	DEBUG("file_key in request is %s\n",file_key);
	DEBUG("time ley in request is %s\n",time_key);
	DEBUG("md5 key  in request is %s\n",md5_key );

	char key[MAX_MD5_SIZE];
	strcpy(key,pstC->iqilukey);
	DEBUG("pstRedirectConf->key is %s\n",key);

	char szMd5[MAX_MD5_SIZE];
	char origin[MAX_URL_SIZE];
	snprintf(origin,MAX_URL_SIZE,"%s%s%s",key,file_key,time_key);
	DEBUG("origin is %s\n",origin);
	getMD5((unsigned char *)origin,strlen(origin),szMd5);
	DEBUG("Md5 is %s\n",szMd5);

	if(strncasecmp(md5_key,szMd5,strlen(szMd5)) != 0)
	{
		DEBUG("not match .reject\n");
		goto fail404;

	}

	printf("%s %s %s", origin_url, ip, other);
	DEBUG("OUTPUT: %s %s %s\n", origin_url, ip, other);
	fflush(stdout);
	return 1;

fail404:
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
/*
int main()
{
	return 0;
}
*/
