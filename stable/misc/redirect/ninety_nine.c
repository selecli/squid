/* 
 * Author: chenqi
 * Date: 12 July 2012
 * */
#ifdef  NNPAN
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <dlfcn.h>
#include <sys/utsname.h>
#include <setjmp.h>
#include <time.h>
#include "redirect_conf.h"
#include "md5.h"

#if 1
extern int g_fdDebug;
extern FILE *g_fpLog;
#define CRITICAL_ERROR(args, ...)    fprintf(g_fpLog, args, ##__VA_ARGS__);fflush(g_fpLog);
#define NOTIFICATION(args,...)       fprintf(g_fpLog, args, ##__VA_ARGS__);fflush(g_fpLog);
#define DEBUG(args, ...)             if(g_fdDebug > 3) {fprintf(g_fpLog, args, ##__VA_ARGS__);fflush(g_fpLog);}
#else   /* else */
#define CRITICAL_ERROR(args, ...)    printf(args, ##__VA_ARGS__);fflush(stdout);
#define NOTIFICATION(args,...)       printf(args, ##__VA_ARGS__);fflush(stdout);
#define DEBUG(args, ...)             printf(args, ##__VA_ARGS__);fflush(stdout);
#endif

#define MAX_FILENAME_LENGTH 100
#define MAX_URL_LENGTH 1024
#define MAX_TOKEN_SIZE 1024
#define MAX_MD5_SIZE 2048


/*
 * d.99pan.com none 99pan key http://www.chinache.com
 * */
int analysisNinetyNineCfg(struct redirect_conf *pstRedirectConf)
{
	char *pTemp = NULL;
	if (NULL == (pTemp = xstrtok(NULL, " \t\r\n")))
	{
		CRITICAL_ERROR("99pan: Can not get md5 key\n");
		return -1;
	}
    pstRedirectConf->key = strdup(pTemp);
	DEBUG("99pan: we get md5 key: %s\n", pTemp);

	if (NULL == (pTemp = xstrtok(NULL, " \t\r\n")))
	{
		CRITICAL_ERROR("99pan: Can not get fial dst_url\n");
		return -1;
	}
	pstRedirectConf->fail_dst = strdup(pTemp);
    DEBUG("99pan: we get fail url: %s\n", pTemp);
    return 1;
}

// 1 means: url is invalid or timeout
// 0 means: url is valid and not timeout
static int url_timeout(char *expire)
{
    char *token = NULL;
    time_t now;
    struct tm *global_time;

    // got current time
    time(&now);
    global_time = gmtime(&now);

    // compare year
    if (NULL == (token = xstrtok(expire, "-")))
        return 1;
    int year = atoi(token) - 1900;
    if (year > global_time->tm_year)
        return 0;
    else if (year < global_time->tm_year)
        return 1;

    // compare month
    if (NULL == (token = xstrtok(NULL, "-")))
        return 1;
    int month = atoi(token) - 1;
    if (month > global_time->tm_mon)
        return 0;
    else if (month < global_time->tm_mon)
        return 1;

    // compare day
    if (NULL == (token = xstrtok(NULL, "-")))
        return 1;
    int day = atoi(token);
    if (day >= global_time->tm_mday)
        return 0;
    return 1;
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

struct url_info{
    char filepath[256];
    char requestip[64];
    char timeout[64];
    char md5[128];

    // unused
    char datakey[512];
    char filename[128];
    char xScale[16];
    char yScale[16];
};

jmp_buf env;
static void fill_request_info(const char *url, const char *member, char *info)
{
    char *field = NULL;
    char *field_end = NULL;
    if (NULL == (field = strstr(url, member)))
    {
        DEBUG("field [%s] cannot find url failed\n", member);
        longjmp(env, -1);
    }
    field += strlen(member);
    if (NULL == (field_end = strchr(field, '&')))
    {
        if (strcasecmp(info, "filename")||strcasecmp(info, "yScale")) // maybe the last parameter
        {
            strcpy(info, field);
        }
        else
        {
            DEBUG("field [%s] parsed form url failed\n", member);
            longjmp(env, -1);
        }
    }
    else
        strncpy(info, field, field_end - field);
    DEBUG("%s %s\n", member, info);
}

/* input */
/* http://domain1/RequestDownload?filepath=x&requestip=x&datakey=x&timeout=2012-08-17&MD5=x&filename=x */
/* output */
/* http://domain2/RequestDownload?filepath=x&key=x&filename=x */
int NinetyNineVerify(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other)
{
	char *fail_dst = NULL;
    char *field = NULL;
    struct url_info request_info;
    memset(&request_info, 0, sizeof(struct url_info));

    DEBUG("client input url is [%s]\n", url);

    if (-1 == setjmp(env))
        goto fail403;

    // fill request_info
    fill_request_info(url, "filepath=", request_info.filepath);
    fill_request_info(url, "requestip=", request_info.requestip);
    fill_request_info(url, "timeout=", request_info.timeout);
    fill_request_info(url, "MD5=", request_info.md5);
    // fill_request_info(url, "filename=", request_info.filename);
    // fill_request_info(url, "datakey=", request_info.datakey);

    // got xscale and yscale
    /*
    if (strstr(url,"RequestThumbnail") && strstr(url,"xScale=") && strstr(url,"yScale="))
    {
        fill_request_info(url, "xScale=", request_info.xScale);
        fill_request_info(url, "yScale=", request_info.yScale);
    }
    */

    /* calculate md5: MD5(filepath|requestip|timeout|encryptkey) */
    char Digest[MAX_MD5_SIZE];
    memset(Digest, 0, MAX_MD5_SIZE);
    char md5[MAX_MD5_SIZE];
    memset(md5, 0, MAX_MD5_SIZE);
    strcpy(md5, request_info.filepath);
    strcat(md5, request_info.requestip);
    strcat(md5, request_info.timeout);
    strcat(md5, pstRedirectConf->key);

    // step1. verify ip
    char ip_from_fc[32];
    memset(ip_from_fc, 0, 32);
    if (NULL != (field = strstr(ip, "/-")))
        strncpy(ip_from_fc, ip, field-ip);
    else
        strcpy(ip_from_fc, ip);

    if (strcmp(request_info.requestip, ip_from_fc))
    {
        DEBUG("ip is not matched\n");
        goto fail403; 
    }

    // step2. verify time
    if (url_timeout(request_info.timeout))
    {
        DEBUG("url is timeout\n");
        goto fail403;
    }

    // step3. verify md5
    getMD5((unsigned char*)md5, strlen(md5), Digest);
    DEBUG("MD5 factor is %s\nMd5 Digest is %s\n", md5,Digest);
    if (strcasecmp(Digest, request_info.md5))
    {
        DEBUG("md5 verify failed\n");
        goto fail403;
    }

    // at last, generate new url after rewrite
    /*
    char new_url[MAX_URL_LENGTH];
    memset(new_url, 0, MAX_URL_LENGTH);
    field = strchr(url, '&');
    field++;
    strncpy(new_url, url, field - url);
    snprintf(new_url + strlen(new_url), MAX_URL_LENGTH - strlen(new_url), "key=%s",request_info.datakey);
    if (strlen(request_info.filename))
        snprintf(new_url + strlen(new_url), MAX_URL_LENGTH - strlen(new_url), "&filename=%s",request_info.filename);
    if (strstr(url,"RequestThumbnail") && strlen(request_info.xScale) && strlen(request_info.yScale))
        snprintf(new_url + strlen(new_url), MAX_URL_LENGTH - strlen(new_url), "&xScale=%s&yScale=%s",request_info.xScale, request_info.yScale);
        */
	printf("%s %s %s", url, ip, other);
	fflush(stdout);
	DEBUG("VERIFY SUCCESS OUTPUT: %s %s %s\n", url, ip, other);
	return 1;

fail403:
	if (NULL == (fail_dst = pstRedirectConf->fail_dst))
	{
		fail_dst = "http://www.chinacache.com";
	    CRITICAL_ERROR("No redirect url, use default [%s]\n", fail_dst);
	}
	DEBUG("fail_dst: %s\n",fail_dst);
	printf("403:%s %s", fail_dst, other);
	fflush(stdout);
	DEBUG("VERIFY FAILED OUTPUT:%s %s %s\n", fail_dst, ip, other);
	return 0;
}
#endif
