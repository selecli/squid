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
typedef struct sdo
{
    char sdokey[MAX_MD5_SIZE];
}ST_SDO_CFG;

/*  解析盛大防盗链配置行
 * sdo.download.ccgslb.net none sdo 3325467230CABCC0 http://61.135.207.203/
 * */
int analysisSDOCfg(struct redirect_conf *pstRedirectConf)
{
	char *pTemp;
	char *pcSep=" \t\r\n";
	ST_SDO_CFG *pstC = NULL;
	pstC = (ST_SDO_CFG *)malloc(sizeof(ST_SDO_CFG));
	if(NULL == pstC)
	{
		CRITICAL_ERROR("No memory\n");
		goto error;
	}
	
/*得到盛大网设定的密码*/
	pTemp = strtok(NULL, pcSep);
	if(NULL == pTemp)
	{
		DEBUG("configure file is error,lack of Key");
		goto error;
	}
	DEBUG("pstC->sdokey is %s\n",pTemp);

	strcpy(pstC->sdokey,pTemp);

/*得到盛大网设定的错误返回的地址*/
	pTemp = strtok(NULL, pcSep);
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
static int getPathFromUrl(char *url,char *path)
//static int getPathAndIpFromUrl(char *url,char *path)
{
	char *str = "http://";
	char *p =strstr(url,str);
	if ( NULL == p)
	{   
		DEBUG("url format is wrong \n");
		return 0;
	}   
	p = p + strlen(str) + 1;

	char *q = NULL;
	q = strchr(p,'/');
	if ( NULL == q ) 
	{   
		DEBUG("url format is wrong \n");
		return 0;
	}   

	char *r = NULL;
	r = strstr(url,"?key");
	if(NULL == r)
	{
		 DEBUG("url format is wrong \n");
		 return 0;
	}
	strncpy(path,q+1,r-q-1);
	path[r-q-1] = '\0';
	return 1;
}
static int getMd5FromUrl(char *url ,char *md5)
{
	char *str = "?key=";
	DEBUG("url=%s\n",url);
	char *p =strstr(url,str);
	if ( NULL == p)
	{   
		DEBUG("url format is wrong in getMd5FromUrl\n");
		return 0;
	}   
	p = p + strlen(str);

	char *q = NULL;
	q = strchr(p,'&');
	if ( NULL == q ) 
	{   
		DEBUG("url format is wrong,lack of &\n");
		return 0;
	}   
	strncpy(md5,p,q-p);
	return 1;

}
void getMD5(unsigned char *mybuffer,int buf_len,char *digest)
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
int verifySdo(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other)
{
	char *fail_dst = NULL;
	char path[MAX_PATH_SIZE];
	char origin_url[MAX_URL_SIZE];
	memset(path,'0',MAX_PATH_SIZE);
	memset(origin_url,'0',MAX_MD5_SIZE);
	strcpy(origin_url,url);
	ST_SDO_CFG* pstC = pstRedirectConf->other;
	assert(pstC != NULL);
	
//	if( 0 == getPathAndIpFromUrl(url,path,ip_in_url))
	if( 0 == getPathFromUrl(url,path))
	{
		DEBUG("Sdo url has no MD5 sum\n")
		goto fail404;
	}

	char ip_in_header[MAX_MD5_SIZE];
	char *pos = strchr(ip,'/');
	if (pos == NULL)
	{
		goto fail404;
	}
	else
	{
		strncpy(ip_in_header,ip,pos-ip);
		ip_in_header[pos-ip] = '\0';
	}

	DEBUG("ip_in_header is %s\n",ip_in_header);
	DEBUG("path is %s\n",path);

	char key[MAX_MD5_SIZE];
	strcpy(key,pstC->sdokey);
	DEBUG("pstRedirectConf->key is %s\n",key);

//	char newUrl[MAX_URL_SIZE];
//	char *keyword = "?key=";
	char szMd5[MAX_MD5_SIZE];
	char origin[MAX_MD5_SIZE];
	snprintf(origin,MAX_MD5_SIZE,"%s%s",key,ip_in_header);
	DEBUG("origin is %s\n",origin);
	getMD5((unsigned char *)origin,strlen(origin),szMd5);
	DEBUG("Md5 is %s\n",szMd5);

	char md5_request[MAX_MD5_SIZE];
	if(!getMd5FromUrl(url,md5_request))
	{
		DEBUG("can't get md5 in the url");
		goto fail404;

	}
	DEBUG("Md5 in request is %s\n",md5_request);

	if(strncasecmp(md5_request,szMd5,strlen(szMd5)) != 0)
	{
		DEBUG("not match .reject\n");
		goto fail404;

	}

//	snprintf(url,MAX_URL_SIZE,"%s%s%s&%s",url,keyword,szMd5,path);

	printf("%s %s %s", origin_url, ip, other);
	DEBUG("OUTPUT: %s %s %s\n", origin_url, ip, other);
	fflush(stdout);
/*	if (pstRedirectConf->fail_dst != NULL)
	{
		free(pstRedirectConf->fail_dst);
	}
*/
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
