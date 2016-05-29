/*
 * Developed for bokecc download
 * Author: xin.yao
 * Date:   2012-02-22
 *--------------------------------------------------
 * based on IP:
 * hashID = md5(secureID+IP+User-Agent)
 *--------------------------------------------------
 * configure line example:
 * www.bokecc.com    none    bokecc_download 49e059fec1243ae92c23adfd33194185    http://www.chinacache.com
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

typedef struct bokecc_data_st {
	char bokecckey[MAX_MD5_SIZE];
} bokecc_data_t;

int analysisBokeccCfg(struct redirect_conf *pstRedirectConf)
{
	char *pTemp;
	char *wspace=" \t\r\n";
	bokecc_data_t *pstC = NULL;

	pstC = malloc(sizeof(bokecc_data_t));
	if(NULL == pstC)
	{
		CRITICAL_ERROR("malloc for 'patC' failed\n");
		goto error;
	}
	pTemp = xstrtok(NULL, wspace);
	if(NULL == pTemp)
	{
		DEBUG("configure error, lack secureID\n");
		goto error;
	}
	DEBUG("pstC->bokecckey is %s\n", pTemp);
	strcpy(pstC->bokecckey, pTemp);
	pTemp = xstrtok(NULL, wspace);
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

static int getKey(char *url, char *dst_url, char *key)
{
	int len;
	char *p0, *p1, *p2;

	p0 = strchr(url, '?');
	if (NULL == p0)
		return -1;
	strncpy(dst_url, url, p0 - url);
	p1 = strstr(url, "key=");
	if (NULL == p1)
		return -1;
	strncat(dst_url, p0, p1 - p0);
	p2 = strchr(p1, '&');
	if (NULL == p2)
	{
		strcpy(key, p1 + 4);
	}
	else
	{
		strncpy(key, p1 + 4, p2 - (p1 + 4));
		strcat(dst_url, p2 + 1);
	}
	len = strlen(dst_url);
	if ('?' == dst_url[len - 1] || '&' == dst_url[len - 1])
		dst_url[len - 1] = '\0';

	return 0;
}

int verifyBokecc(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other)
{
	char szMd5[MAX_MD5_SIZE];
	char ip_sec[MAX_MD5_SIZE];
	char dst_url[MAX_URL_SIZE];
	char md5_key[MAX_MD5_SIZE];
	char md5origin[MAX_MD5_SIZE];
	char user_agent[MAX_URL_SIZE];
	char *fail_dst = NULL;
	char *str, *str2, *p;
	bokecc_data_t* pstC = NULL;


	memset(dst_url,0,MAX_URL_SIZE);
	memset(md5_key, 0, MAX_MD5_SIZE);
	if (getKey(url, dst_url, md5_key) < 0)
	{   
		DEBUG("URL format is wrong\n");
		goto fail401;
	}
	pstC = pstRedirectConf->other;
	assert(NULL != pstC);
	DEBUG("md5 key  in request is %s\n", md5_key );
	DEBUG("pstRedirectConf->key is %s\n", pstC->bokecckey);
	p = strrchr(ip, '/');
	if (NULL == p)
		p = ip + strlen(ip);
	memset(ip_sec, 0, MAX_MD5_SIZE);
	memcpy(ip_sec, ip, p - ip);
	memset(user_agent, 0, MAX_URL_SIZE);
	str = strstr(other, "User-Agent:");
	if (NULL == str) 
		goto fail401;
	str += 11;
	while (' ' == *str) 
		++str;
	if (NULL == (str2 = strchr(str, '\t')) && NULL == (str2 = strchr(str, '\r')))
		goto fail401;
	if(str == str2 || str2 - str >= MAX_URL_SIZE)
		goto fail401;
	strncpy(user_agent, str, str2 - str);
	snprintf(md5origin, 
			MAX_MD5_SIZE, 
			"%s%s%s", 
			pstC->bokecckey, 
			ip_sec, 
			user_agent);
	DEBUG("md5origin is %s\n", md5origin);
	getMD5((unsigned char *)md5origin, strlen(md5origin), szMd5);
	DEBUG("Md5 is %s\n", szMd5);
	if(strncasecmp(md5_key, szMd5, strlen(md5_key)) || 32 != strlen(md5_key))
	{
		DEBUG("no match the request key, now reject it\n");
		goto fail401;
	}
	printf("%s %s %s", dst_url, ip, other);
	DEBUG("OUTPUT: %s %s %s\n", dst_url, ip, other);
	fflush(stdout);
	return 1;

fail401:
	fail_dst = pstRedirectConf->fail_dst;
	DEBUG("fail_dst %s\n", fail_dst);
	if(NULL == fail_dst)
	{
		fail_dst = "http://www.chinacache.com";
		CRITICAL_ERROR("no redirect url in configure line, use default[%s]\n", fail_dst);
	}
	printf("401:%s %s %s", fail_dst, ip, other);
	fflush(stdout);
	DEBUG("VERIFY FAILED OUTPUT:%s %s %s\n", fail_dst, ip, other);
	return 0;
}

