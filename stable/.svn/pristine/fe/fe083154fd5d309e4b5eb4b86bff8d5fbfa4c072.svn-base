#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <dlfcn.h>
#include <sys/utsname.h>
#include <time.h>
#include <sys/types.h>
#include "redirect_conf.h"
#include "md5.h"
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
#define MAX_TIME_SIZE MAX_MD5_SIZE

typedef struct cwg_data_st
{
		char cwgkey[MAX_MD5_SIZE];
	//	long int cwg_active_time;
}cwg_data_t;

int analysisCWGCfg(struct redirect_conf *pstRedirectConf)
{
	char *pTemp;
	char *wspace = " \t\r\n";
	cwg_data_t *pstC = NULL;
	
	pstC = malloc(sizeof(cwg_data_t));
	if (pstC == NULL)
	{
		CRITICAL_ERROR("malloc for 'pstC' failed!\n");
		goto error;
	}
	memset(pstC, 0 , sizeof(cwg_data_t));

	pTemp = xstrtok(NULL, wspace);
	if (NULL == pTemp)
	{
		CRITICAL_ERROR("configureation error for security error!\n");
		goto error;
	}
//	DEBUG("pstC -> cwgkey is %s\n",pTemp);
	strcpy(pstC->cwgkey, pTemp);

	pTemp = xstrtok(NULL, wspace);
	if (NULL == pTemp)
	{
		CRITICAL_ERROR("configureation error for ErrorPage!\n");
		goto error;
	}
//	DEBUG("pstC -> ErrorPage is %s\n",pTemp);
	pstRedirectConf->fail_dst = malloc(strlen(pTemp) + 1);
	if(NULL == pstRedirectConf->fail_dst)
	{
		CRITICAL_ERROR("malloc for 'fail_dst' failed!\n");
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
    sprintf(digest,"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",szdigtmp[0], szdigtmp[1], szdigtmp[2], szdigtmp[3], szdigtmp[4],szdigtmp[5], szdigtmp[6], szdigtmp[7], szdigtmp[8], szdigtmp[9],szdigtmp[10], szdigtmp[11], szdigtmp[12], szdigtmp[13], szdigtmp[14], szdigtmp[15]);
}

static int getKey1(char *url, char *dst_url, char *final_dst_url, char *dst_uri, char *filename, time_t *pt,char *key)
{
	int len = 0;
	char *p0, *p1, *p2, *p3, *p4, *p5, *p6;

	p0 = strchr(url, '?');
	if (NULL == p0)
	{
		DEBUG("Do not have '?'\n");
		return -1;
	}
	strncpy(dst_url, url, p0 - url);

	if (strstr(p0, "&k=") || (strstr(p0, "?k=")))
	{
		char *tmp = NULL;
		tmp = strstr(p0, "&k=");
		if (tmp != NULL)
		{
			p2 = tmp+1;
			/*确定MD5是否是16位*/
			char *key_tmp1 = NULL;
			key_tmp1 = strchr(tmp+1, '&');
			if (key_tmp1 != NULL)
			{
				DEBUG("key_tmp1-tmp-3:%d\n",key_tmp1-tmp-3);
				if (key_tmp1-tmp-3 != 16)
				{
					DEBUG("md5 is large than 16!\n")
					return -1;
				}
			strncpy(key, tmp+3, 16);
			}
			else
			{
		//		int tmp_len = strlen(url);
		//		if (url[tmp_len - 17] != '=')
		//		{
		//			DEBUG("md5 is large than 16!\n")
		//			return -1;
		//		}
			strcpy(key, tmp+3);
			}
		}
		else
		{
			tmp = strstr(p0, "?k=");
			if (tmp != NULL)
			{
				p2 = tmp+1;
				strncpy(key, tmp+3, 16);
			}
			/*确定MD5是否是16位*/
			char *key_tmp = NULL;
			key_tmp = strchr(tmp, '&');
			if (key_tmp == NULL)
			{
				return -1;
			}
			else
			{
				if (key_tmp-tmp-3 != 16)
				{
					DEBUG("key_tmp-tmp-3:%d\n",key_tmp-tmp-3);
					DEBUG("md5 is large than 16!\n")
					return -1;
				}
			}

		}
	}
	else
	{
		DEBUG("Do not have inival key!\n");
		return -1;
	}
	 
	if (strstr(p0, "?t=") || strstr(p0, "&t="))
	{
		char *ttmp = NULL;
		ttmp = strstr(p0, "?t=");
		if (ttmp != NULL)
		{
			p1 = ttmp+1;
			*pt = atoll(ttmp+3);
		}
		else
		{
			ttmp = strstr(p0, "&t=");
			if (ttmp != NULL)
			{
				p1 = ttmp+1;
				*pt = atoll(ttmp+3);
			}
		}
	}
	else
	{
		DEBUG("Do not have inival time!\n");
		return -1;
	}

	if (p1 < p2)
	{
		strncat(dst_url, p0, p1-p0);
		p6 = strchr(p1, '&');
		if (0 != strncmp(p6, "&k=",3))
		{
			len = strlen(dst_url);
			if (*(dst_url + len - 1) == '?')
			{
				strncat(dst_url, p6+1, p2-p6-1);
			}
			else 
			{
				strncat(dst_url, p6+1, p2-p6-1);
			}
		}
		len = strlen(dst_url);
		if (*(dst_url+len - 1) == '?' || *(dst_url+len - 1) == '&')
		{
			strcat(dst_url, p2+18);
		}
		else
		{
			strcat(dst_url, p2+18);
		}
	}
	else if (p1 > p2)
	{
		strncat(dst_url, p0 , p2-p0);
		p6 = strchr(p2, '&');
		if (0 != strncmp(p6, "&t=", 3))
		{
			len = strlen(dst_url);
			if (*(dst_url+len - 1) == '?' || *(dst_url+len - 1) == '&')		
			{
				strncat(dst_url, p6+1, (p1-p6-1));
			}
			else
			{
				strncat(dst_url, p6+1, (p1-p6-1));
			}
		}
		len = strlen(dst_url);
		if (*(dst_url+len - 1) == '?' || *(dst_url+len - 1) == '&')
		{
			strcat(dst_url, p1+12);
		}
		else
		{
			strcat(dst_url, p1+12);
		}
	}
	else
	{
		 fprintf(stderr, "Something is wrong");
		 abort();
	}
	len = strlen(dst_url);
	if ('?' == dst_url[len - 1] || '&' == dst_url[len - 1])
		dst_url[len - 1] = '\0';
	char *deal = NULL;

	if (strstr(dst_url, "?&") || strstr(dst_url, "&&"))
	{
		deal = strstr(dst_url, "?&");
		if (deal != NULL)
		{
			strncpy(final_dst_url, dst_url, deal-dst_url+1);
			strcat(final_dst_url, deal+2);
		}
		else
		{
			deal = strstr(dst_url, "&&");
				if (deal != NULL)
				{
					strncpy(final_dst_url, dst_url, deal-dst_url+1);
					strcat(final_dst_url, deal+2);
					
				}
		}
	}
	else
	{
		strcpy(final_dst_url, dst_url);
	}

	p3 = strstr(url, "http://");
	if (NULL == p3)
	{
		DEBUG("Do not have http://--!\n");
		return -1;
	}
	p4 = strrchr(p3+7, '/');
	if (NULL == p4)
	{
		DEBUG("Do not have full_fileame!\n");
		return -1;
	}
	strncpy(dst_uri, p4+1, p0-p4);
	
	p5 = strchr(dst_uri, '.');
	if (NULL == p5)
	{
		DEBUG("Do not have '.' in full_name!\n");
		return -1;
	}
	strncpy(filename, dst_uri, p5 - dst_uri);
	return 0;
}

int verifyCWG(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other)
{
	char szMd5[MAX_MD5_SIZE];
	char dst_url[MAX_URL_SIZE];
	char dst_uri[MAX_URL_SIZE];
	char filename[MAX_URL_SIZE];
	char md5_key[MAX_MD5_SIZE];
	char final_dst_url[MAX_URL_SIZE];
	memset(final_dst_url, '\0', MAX_URL_SIZE);
	long int t = 0;
	char timestr[MAX_MD5_SIZE];
	char md5origin[MAX_MD5_SIZE];
	char *fail_dst = NULL;
	cwg_data_t *pstC = NULL;

	pstC = pstRedirectConf->other;
	assert(pstC != NULL);

	memset(dst_url, 0, MAX_URL_SIZE);
	memset(dst_uri, 0, MAX_URL_SIZE);
	memset(md5_key, 0, MAX_MD5_SIZE);
	memset(filename, 0, MAX_URL_SIZE);
	if (getKey1(url, dst_url, final_dst_url, dst_uri, filename, &t, md5_key) < 0)
	{
		DEBUG("URL format is wrong!\n");
		goto fail401;
	}
	DEBUG("md5 key in request is %s\n", md5_key );
	DEBUG("secret in redirect.conf is %s\n", pstC->cwgkey);

	if (t && md5_key)
	{
		DEBUG("time in request is %ld\n", t);
		if (t >= time(NULL))
		{
			memset(timestr, 0, MAX_URL_SIZE);
			snprintf(timestr, MAX_URL_SIZE, "%ld", (long)t);
		}
		else
		{
			DEBUG("time error is %ld\n",t);
			goto fail401;
		}
	}
	else
	{
		DEBUG("Do not have the param time!\n");
		goto fail401;
	}
	if (md5_key)
	{
		snprintf(md5origin, 
				MAX_MD5_SIZE, 
				"%s%s%s", 
				pstC->cwgkey, 
				filename, 
				timestr);
	}
	else
	{
		DEBUG("[%s] is illegally", url);
		goto fail401;
	}
	
	DEBUG("md5orgin is %s\n", md5origin);
	getMD5((unsigned char *)md5origin, strlen(md5origin), szMd5);
	DEBUG("Md5 is %s\n", szMd5);
	if(strncasecmp(md5_key, szMd5+8, 16) || 16 != strlen(md5_key))
	{
		DEBUG("no match the request key, now reject it\n");
		goto fail401;
	}
	printf("%s %s %s", final_dst_url, ip, other);
	DEBUG("OUTPUT: %s %s %s\n", final_dst_url, ip, other);
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
//	printf("%s %s %s", fail_dst, ip, other);
	fflush(stdout);
	DEBUG("VERIFY FAILED OUTPUT:%s %s %s\n", fail_dst, ip, other);
	return 0;
}
