/*
 * Developed for MSN download
 * Author: xin.yao
 * Date:   2012-04-17
 *------------------------------------------------------
 * based on time:
 * hashID = SHA256(URL+token+time+secureID)
 *------------------------------------------------------
 * configure line example:
 * www.msn.cn	none    msn_download   RaA695LBHXNpcbnkdd5q    http://www.chinacache.com
 */

#include <time.h>
#include <dlfcn.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include "SHA256.h"
#include "redirect_conf.h"


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
#define MAX_MD5_SIZE 512
#define MAX_PATH_SIZE 1024
#define MAX_URL_SIZE 1024

#define BUF_SIZE_512 512

typedef struct conf_param_st {
	char secureID[BUF_SIZE_512];
} conf_param_t;

int analysisMSNCfg(struct redirect_conf *pstRedirectConf)
{	
	char *wspace = " \t\r\n";
	char *p_secureID = NULL;
	char *failed_jump_url = NULL;
	conf_param_t *params = NULL;

	params = malloc(sizeof(conf_param_t));
	if (NULL == params) 
	{
		CRITICAL_ERROR("malloc for params is 'NULL'\n");
		goto err;
	}
	memset(params, 0, sizeof(conf_param_t));
	p_secureID = xstrtok(NULL, wspace);
	if(NULL == p_secureID)
	{   
		DEBUG("configure line error for 'MSN', feild 'secureID'");
		goto err;
	}  
	strcpy(params->secureID, p_secureID);
	failed_jump_url = xstrtok(NULL, wspace);
	if(NULL == failed_jump_url)
	{
		DEBUG("configure line error for 'MSN', feild 'failed_jump_url'");
		goto err;
	}
	pstRedirectConf->fail_dst = malloc(strlen(failed_jump_url) + 1);
	if(NULL == pstRedirectConf->fail_dst)
	{
		CRITICAL_ERROR("pstRedirectConf->fail_dst is 'NULL'\n");
		goto err;
	}
	memset(pstRedirectConf->fail_dst, 0, sizeof(pstRedirectConf->fail_dst));
	strcpy(pstRedirectConf->fail_dst, failed_jump_url);
	pstRedirectConf->other = (void*)params;

	return 0;	

err:
	if(NULL != params)
		free(params);
	if(NULL != pstRedirectConf->fail_dst)
		free(pstRedirectConf->fail_dst);
	return -1;
}

static void getSHA256(char *buf, int datalen, char *digest)
{
	int i;
	Sha256Calc cal;

	memset(digest, 0, 64);
	Sha256Calc_reset(&cal);
	Sha256Calc_calculate(&cal, (SZ_CHAR *)buf, datalen);
	for(i = 0; i < 32; ++i)
	{
		sprintf(digest + i * 2, "%02X", (SZ_UCHAR)(cal.Value[i]));
	}
	Sha256Calc_uninit(&cal);
}

static int isValidInTime(const char *timeID)
{
	struct tm tm;
	time_t tt, curtime;

	curtime = time(NULL);
	memset(&tm, 0, sizeof(tm));
	tm.tm_year = 2000 - 1900;
	tm.tm_mon = 0;
	tm.tm_mday = 1;
	tm.tm_hour = 0;
	tm.tm_min = 0;
	tm.tm_sec = 0;
	tt = atol(timeID) + mktime(&tm);
	if (curtime > tt + 120)
	{
		DEBUG("time expire. reject\n");
		return -1;
	}

	return 0;
}

int verifyMSN(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other)
{
	char *pos, *pos1, *pos2;
	char *fail_dst = NULL;
	char dst_url[MAX_URL_SIZE];
	char time[BUF_SIZE_512];
	char token[BUF_SIZE_512];
	char sha256[MAX_MD5_SIZE];
	char SHAstr[MAX_MD5_SIZE];
	conf_param_t *params = NULL;

	pos = strstr(url, "?");
	if (NULL == pos)
		goto fail403;	
	memset(dst_url, 0, MAX_URL_SIZE);
	strncpy(dst_url, url, pos - url);
	pos1 = strstr(pos, "token=");
	if (NULL == pos1)
		goto fail403;
	pos1 += 6;
	pos2 = strstr(pos1, "&"); 
	if (NULL == pos2)
		strcpy(token, pos1);
	else
		strncpy(token, pos1, pos2 - pos1);
	pos1 = strstr(pos, "time=");
	if (NULL == pos1)
		goto fail403;
	pos1 += 5;
	pos2 = strstr(pos1, "&"); 
	if (NULL == pos2)
		strcpy(time, pos1);
	else
		strncpy(time, pos1, pos2 - pos1);
	if (isValidInTime(time) < 0)
		goto fail403;
	params = pstRedirectConf->other;
	assert(NULL != params);
	snprintf(SHAstr, MAX_MD5_SIZE, "%s%s%s", dst_url, time, params->secureID);
	DEBUG("SHAstr is %s\n", SHAstr);
	getSHA256(SHAstr, strlen(SHAstr), sha256);
	DEBUG("sha256 is %s\n", sha256);
	if(strncasecmp(token, sha256, strlen(token)))
	{   
		DEBUG("token not match. reject\n");
		goto fail403;
	}   
	printf("%s %s", dst_url, other);
	DEBUG("OUTPUT: %s\n", dst_url);
	fflush(stdout);
	return 1;

fail403:
	fail_dst = pstRedirectConf->fail_dst;
	DEBUG("fail_dst %s\n", fail_dst);
	if(NULL == fail_dst)
	{
		fail_dst = "http://www.chinacache.com";
		CRITICAL_ERROR("No redirect url, use default [%s]\n", fail_dst);
	}
	printf("403:%s %s", fail_dst, other);
	fflush(stdout);
	DEBUG("VERIFY FAILED OUTPUT:%s %s\n", fail_dst, other);
	return 0;
}

