#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/utsname.h>
#include "redirect_conf.h"
#include "libVerifyTS.h"

extern int g_fdDebug;
extern FILE *g_fpLog;
#define CRITICAL_ERROR(args, ...)	 fprintf(g_fpLog, args, ##__VA_ARGS__);fflush(g_fpLog);
#define NOTIFICATION(args,...)		 fprintf(g_fpLog, args, ##__VA_ARGS__);fflush(g_fpLog);
#define DEBUG(args, ...)			 if(g_fdDebug > 3) {fprintf(g_fpLog, args, ##__VA_ARGS__);fflush(g_fpLog);}

/* QQ的认证函数指针 */
int (*gpf_qqtopstream_verify)(int, char*, const int);
static void *handle = NULL;

int QQTopStreamLoadModule()
{
	char *error;
    char *pMachine;
    char *pLibName = "/usr/local/squid/bin/libVerifyTS.so";
    struct utsname stUname;

    if(0 != uname(&stUname))
    {
        CRITICAL_ERROR("Get system information failed\n");
        return -1;
    }

    pMachine = stUname.machine;

    if(NULL != strstr(pMachine, "x86_64"))
    {
        /* 64位系统 */
        pLibName = "/usr/local/squid/bin/libVerifyTS_64.so";
    }

	handle = dlopen (pLibName, RTLD_LAZY);

	if (!handle) {
		CRITICAL_ERROR("Open module error. [%s]\n", dlerror());
		return -1;
	}

	dlerror();    /* Clear any existing error */

	gpf_qqtopstream_verify = dlsym(handle, "qmusic_verifyTstreamKey");
	if ((error = dlerror()) != NULL)  {
		CRITICAL_ERROR ("Load module %s error. [%s]\n", "qmusic_verifyTstreamKey", error);
		return -1;
	}

	return 0;
}

int QQTopStreamReleaseModule()
{
	if(NULL != handle)
		dlclose(handle);
	return 0;
}

/* 解析qq topstream绿钻配置行 
 * topstream.music.qq.com none qq_topstream www.chinache.com
 * 只需要解析出重定向链接即可
 * */
int analyseQQTopStreamCfg(struct redirect_conf *pstRedirectConf)
{
	char *pTemp;

	pTemp = xstrtok(NULL, " \t\r\n");
	if(NULL == pTemp)
	{
		CRITICAL_ERROR("Can not get redirect url\n");
		return -1;
	}

	pstRedirectConf->fail_dst = malloc(strlen(pTemp) + 1);
	if(NULL == pstRedirectConf->fail_dst)
	{
		CRITICAL_ERROR("No memory\n");
		return -1;
	}
	strcpy(pstRedirectConf->fail_dst, pTemp);

	return QQTopStreamLoadModule();
}

/* 认证加密字段 
 * 1  校验通过
 * 0  校验失败
 * 输入参数：
 * */
int verifyQQTopStreamKey(char *pvalue)
{
	if(NULL == gpf_qqtopstream_verify)
	{
		CRITICAL_ERROR("Not qq topstream verify process!\n");
		return 0;
	}
	if(0 == gpf_qqtopstream_verify(0, pvalue, strlen(pvalue)))
	{
		return 1;
	}
	return 0;
}

/* 解析qq topstream请求并认证
 * 返回值：
 * 1  校验通过
 * 0  校验失败
 * 输入参数：
 * pstRedirectConf: 配置信息
 * url
 * ip
 * other 结构体struct Request中的三个成员
 * */
int qqTopStreamVerify(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other)
{
	char 	*fail_dst = NULL;
	char	*pcValue = NULL;
	char	*pcTmp;
	const char *pcKey = "key=";

	pcTmp = strstr(url, "?");
	if(NULL == pcTmp)
	{
		goto fail404;
	}

    /* 截断url，准备正常的输出 */
	*pcTmp = '\0';
	pcTmp++;

    /* key=存在么? */
	if (0 != strncasecmp(pcTmp, pcKey, strlen(pcKey)))
	{
		goto fail404;
	}

    /* 获得value，可能为字符串结尾'\0' */
	pcValue = pcTmp + strlen(pcKey);
	{
		char *p = NULL;
		int len = 0;
		p = strchr(pcValue, '&');
		if (!p)
		{	
		    /* 认证value字符串 */
			if(1 != verifyQQTopStreamKey(pcValue))
			{
				DEBUG("QQ top stream: verified faild. url [%s], value [%s]\n", url, pcValue);
				goto fail403;
			}
		}
		else 
		{
			len = strlen(pcValue) - strlen(p);
			p = malloc(len+1);
			memset(p, 0, len+1);
			memcpy(p, pcValue, len);
			if(1 != verifyQQTopStreamKey(p))
            {
                 DEBUG("QQ top stream: verified faild. url [%s], value [%s]\n", url, p);
				 free(p);
				 p = NULL;	
                 goto fail403;
            }
			free(p);
			p = NULL;	
		}
	}

	printf("%s %s %s", url, ip, other);
	fflush(stdout);
	DEBUG("OUTPUT: %s %s %s\n", url, ip, other);
	return 1;

fail404:
	fail_dst = pstRedirectConf->fail_dst;
	if(NULL == fail_dst)
	{
		fail_dst = "www.chinacache.com";
	    CRITICAL_ERROR("No redirect url, use default [%s]\n", fail_dst);
	}
	printf("%s %s %s", fail_dst, ip, other);
	fflush(stdout);
	DEBUG("VERIFY FAILED OUTPUT:%s %s %s\n", fail_dst, ip, other);
	return 0;

fail403:
	fail_dst = pstRedirectConf->fail_dst;
	if(NULL == fail_dst)
	{
		fail_dst = "www.chinacache.com";
	    CRITICAL_ERROR("No redirect url, use default [%s]\n", fail_dst);
	}
	printf("403;%s %s %s", fail_dst, ip, other);
	fflush(stdout);
	DEBUG("VERIFY FAILED OUTPUT:%s %s %s\n", fail_dst, ip, other);
	return 0;
}
