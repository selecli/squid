/*
 * Developed for wanglong
 * Author: xin.yao
 * Date:   2012-06-27
 * --------------------------------------------------
 * based on IP:
 * hashID = md5(secureID+IP)
 * --------------------------------------------------
 * configure line example:
 * www.wanglong.com    none    wanglong	  ib8qw6zb$remote_addr    http://www.chinacache.com
 */

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

#else

#define CRITICAL_ERROR(args, ...)    printf(args, ##__VA_ARGS__);fflush(stdout);
#define NOTIFICATION(args,...)       printf(args, ##__VA_ARGS__);fflush(stdout);
#define DEBUG(args, ...)             printf(args, ##__VA_ARGS__);fflush(stdout);

#endif
#define MAX_MD5_SIZE   1024
#define MAX_URL_SIZE   1024

typedef struct param_data_st {
	char secureID[MAX_MD5_SIZE];
} param_data_t;

int analysisWanglongCfg(struct redirect_conf *pstRedirectConf)
{
	char *pTemp;
	char *wspace=" \t\r\n";
	param_data_t *pstC = NULL;

	pstC = malloc(sizeof(param_data_t));
	if(NULL == pstC)
	{
		CRITICAL_ERROR("malloc for 'patC' failed\n");
		goto error;
	}
	pTemp = strtok(NULL, wspace);
	if(NULL == pTemp)
	{
		DEBUG("configure error, lack secureID\n");
		goto error;
	}
	DEBUG("pstC->secureID is %s\n", pTemp);
	strcpy(pstC->secureID, pTemp);
	pTemp = strtok(NULL, wspace);
	if(NULL == pTemp)
	{
		DEBUG("configure error, lack fail_dst\n");
		goto error;
	}
	pstRedirectConf->fail_dst = malloc(strlen(pTemp) + 1);
	if(NULL == pstRedirectConf->fail_dst)
	{
		CRITICAL_ERROR("malloc for 'fail_dst' failed\n");
		goto error;
	}
	strcpy(pstRedirectConf->fail_dst, pTemp);
	pstRedirectConf->other = pstC;

	return 0;

error:
	if(NULL != pstC)
		free(pstC);
	if(NULL != pstRedirectConf->fail_dst)
		free(pstRedirectConf->fail_dst);

	return -1;
}

static void getMD5(unsigned char *mybuffer, int buf_len, char *digest)
{
	MD5_CTX context;
	unsigned char szdigtmp[16];

	bzero(digest,64);
	MD5Init(&context);
	MD5Update(&context, mybuffer, buf_len);
	MD5Final(szdigtmp, &context);
	sprintf(digest,
			"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
			szdigtmp[0], szdigtmp[1], szdigtmp[2], szdigtmp[3], szdigtmp[4], 
			szdigtmp[5], szdigtmp[6], szdigtmp[7], szdigtmp[8], szdigtmp[9], 
			szdigtmp[10], szdigtmp[11], szdigtmp[12], szdigtmp[13], szdigtmp[14],
			szdigtmp[15]);
}

int verifyWanglong(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other)
{
	char szMd5[MAX_MD5_SIZE];
	char ip_sec[MAX_MD5_SIZE];
	char dst_url[MAX_URL_SIZE];
	char md5_key[MAX_MD5_SIZE];
	char md5origin[MAX_MD5_SIZE];
	char *fail_dst = NULL;
	char *p, *p0, *p1;
	param_data_t* pstC = NULL;

	memset(dst_url,0,MAX_URL_SIZE);
	p0 = strstr(url, "://");
	if (NULL == p0)
	{
		DEBUG("URL lack '://'\n");
		goto fail403;
	}
	p0 += 3;
	p1 = strstr(p0, "/");
	if (NULL == p1)
	{
		DEBUG("URL lack 'md5 key'\n");
		goto fail403;
	}
	strncpy(dst_url, url, p1 - url);
	p1++;
	p0 = strstr(p1, "/");
	if (NULL == p1)
	{
		DEBUG("URL lack uri\n");
		goto fail403;
	}
	memset(md5_key, 0, MAX_MD5_SIZE);
	strncpy(md5_key, p1, p0 - p1);
	strcat(dst_url, p0);
	pstC = pstRedirectConf->other;
	assert(NULL != pstC);
	DEBUG("md5 key in request is: %s\n", md5_key );
	DEBUG("secureID is: %s\n", pstC->secureID);
	p = strchr(ip, '/');
	if (NULL == p)
		p = ip + strlen(ip);
	memset(ip_sec, 0, MAX_MD5_SIZE);
	memcpy(ip_sec, ip, p - ip);
	snprintf(md5origin, MAX_MD5_SIZE, "%s%s", pstC->secureID, ip_sec);
	DEBUG("md5 param string is: %s\n", md5origin);
	getMD5((unsigned char *)md5origin, strlen(md5origin), szMd5);
	DEBUG("Local Md5 is: %s\n", szMd5);
	if(strncasecmp(md5_key, szMd5, strlen(md5_key)) || 32 != strlen(md5_key))
	{
		DEBUG("no match the request key, now reject it\n");
		goto fail403;
	}
	printf("%s %s %s", dst_url, ip, other);
	DEBUG("OUTPUT: %s %s %s\n", dst_url, ip, other);
	fflush(stdout);
	return 1;

fail403:
	fail_dst = pstRedirectConf->fail_dst;
	DEBUG("fail_dst_url: %s\n", fail_dst);
	if(NULL == fail_dst)
	{
		fail_dst = "http://www.chinacache.com";
		CRITICAL_ERROR("no redirect url in configure line, use default[%s]\n", fail_dst);
	}
	printf("403:%s %s %s", fail_dst, ip, other);
	fflush(stdout);
	DEBUG("VERIFY FAILED OUTPUT:%s %s %s\n", fail_dst, ip, other);
	return 0;
}

