/*
 * Developed for mafengwo music
 * Author: xin.yao
 * Date:   2012-02-09
 *------------------------------------------------------
 * based on time:
 * hashID = md5(secureID+timeID+filename(uri without timeID and hashID))
 *------------------------------------------------------
 * configure line example:
 * mp3file6.mafengwo.net  none    mfw_music   49e059fec1243ae92c23adfd33194185    http://www.chinacache.com
 */

#include <time.h>
#include <dlfcn.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include "md5.h"
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
#define MAX_MD5_SIZE  1024
#define MAX_PATH_SIZE 1024
#define MAX_URL_SIZE  1024

#define BUF_SIZE_256  256

typedef struct conf_param_st {
	char secureID[BUF_SIZE_256];
} conf_param_t;

int analysisMFWMusicCfg(struct redirect_conf *pstRedirectConf)
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
		DEBUG("configure line error for 'MFW', feild 'secureID'");
		goto err;
	}  
	strcpy(params->secureID, p_secureID);
	failed_jump_url = xstrtok(NULL, wspace);
	if(NULL == failed_jump_url)
	{
		DEBUG("configure line error for 'MFW', feild 'failed_jump_url'");
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

static void getMD5(unsigned char *mybuffer,int buf_len,char *digest)
{
	MD5_CTX context;
	unsigned char szdigtmp[16];

	bzero(digest,64);
	MD5Init (&context);
	MD5Update (&context, mybuffer, buf_len);
	MD5Final (szdigtmp, &context);
	sprintf(digest, 
			"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
			szdigtmp[0], szdigtmp[1], szdigtmp[2], szdigtmp[3], szdigtmp[4],
			szdigtmp[5], szdigtmp[6], szdigtmp[7], szdigtmp[8], szdigtmp[9],
			szdigtmp[10], szdigtmp[11], szdigtmp[12], szdigtmp[13], szdigtmp[14],
			szdigtmp[15]);
}

static int isValidInTime(const char *timeID)
{
	int len;
	struct tm tm;
	time_t curtime;
	char *p = NULL;
	char timeID_buf[BUF_SIZE_256];
	int year, month, mday, hour, min;

	//format: YYYYMMDDHHMM
	len = strlen(timeID);
	if (12 != len)
	{
		DEBUG("parse timeID: timeID format must be 'YYYYMMDDHHMM'\n");
		return 0;
	}
	strncpy(timeID_buf, timeID, len);
	p = timeID_buf + len - 2;
	min = atoi(p);
	*p = '\0';
	p -= 2;
	hour = atoi(p);
	*p = '\0';
	p -= 2;
	mday = atoi(p);
	*p = '\0';
	p -= 2;
	month = atoi(p);
	*p = '\0';
	p = timeID_buf;
	year = atoi(p);
	//DEBUG("parse timeID: year: %d, month: %d, mday: %d, hour: %d, minute: %d\n", year, month, mday, hour, min);
	memset(&tm, 0, sizeof(tm));
	tm.tm_year = year - 1900;
	tm.tm_mon = month - 1;
	tm.tm_mday = mday;
	tm.tm_hour = hour;
	tm.tm_min = min;
	tm.tm_sec = 0;
	time_t t = mktime(&tm);
	curtime = time(NULL);
	//DEBUG("parse timeID: t[%ld], curtime[%ld]", t, curtime);
	if (t >= curtime)
		return 1;

	return 0;
}

int verifyMFW(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other)
{
	char *pos1, *pos2, *pos3, *pos4;
	char *fail_dst = NULL;
	char uri[MAX_URL_SIZE];
	char dst_url[MAX_URL_SIZE];
	char timeID[BUF_SIZE_256];
	char hashID[BUF_SIZE_256];
	char md5str[MAX_MD5_SIZE];
	char md5[MAX_MD5_SIZE];
	conf_param_t *params = NULL;

	pos1 = strstr(url, "://");
	if (NULL == pos1)
	{
		DEBUG("lack ://");
		goto fail403;	
	}
	pos1 = strstr(pos1 + 3, "/");
	if (NULL == pos1)
	{
		DEBUG("lack domain");
		goto fail403;	
	}
	memset(dst_url, 0, MAX_URL_SIZE);
	strncpy(dst_url, url, pos1 - url);
	pos1++;
	pos2 = strstr(pos1, "/");
	if (NULL == pos2)
		goto fail403;	
	memset(timeID, 0, BUF_SIZE_256);
	strncpy(timeID, pos1, pos2 - pos1);
	if (!isValidInTime(timeID))
	{
		DEBUG("time expired: %s\n", timeID);
		goto fail403;	
	}
	pos2++;
	pos3 = strstr(pos2, "/");
	if (NULL == pos3)
	{
		DEBUG("lack hashID");
		goto fail403;	
	}
	memset(hashID, 0, BUF_SIZE_256);
	strncpy(hashID, pos2, pos3 - pos2);
	memset(uri, 0, BUF_SIZE_256);
	pos4 = strrchr(url, '?');
	if (NULL == pos4) 
		strcpy(uri, pos3);
	else
		strncpy(uri, pos3, pos4 - pos3);	
	strcat(dst_url, uri);
	params = pstRedirectConf->other;
	assert(NULL != params);
	snprintf(md5str, MAX_MD5_SIZE, "%s%s%s", params->secureID, timeID, uri);
	DEBUG("md5str is %s\n", md5str);
	getMD5((unsigned char *)md5str, strlen(md5str), md5);
	DEBUG("md5 is %s\n", md5);
	if(strncasecmp(hashID, md5, strlen(hashID)) || 32 != strlen(hashID))
	{   
		DEBUG("hashID not match. reject\n");
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

