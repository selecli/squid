#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <dlfcn.h>
#include <ctype.h>
#include <sys/utsname.h>
#include <time.h>
#include "redirect_conf.h"
#include "base64.c"
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
#define MAX_ITEM 20

/*  ½âÎöÊ¢´ó·ÀµÁÁ´ÅäÖÃÐÐ
 *  apktest.oupeng.com none oupeng http://www.chinacache.com/antihijack/blankfaildst.jpg
 * */
int analysisOupengCfg(struct redirect_conf *pstRedirectConf)
{
	char *pstC;
	
    if (NULL == (pstC = xstrtok(NULL, " \t\r\n")))
		goto error;
    pstRedirectConf->fail_dst = strdup(pstC);
    if (NULL == pstRedirectConf->fail_dst)
        goto error;
    return 0;

error:
    DEBUG("Oupeng Redirect: analysisOupengCfg failed\n");
    if (NULL != pstRedirectConf->fail_dst)
        free(pstRedirectConf->fail_dst);
	return -1;
}


int OupengVerify(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other)
{
    char *fail_dst = NULL;
    char *sid = NULL;
    char *tmp = NULL;
    char *orig = NULL;
    char *domain = NULL;
    char uri[BUFSIZ];
    memset(uri, 0, BUFSIZ);

    DEBUG("client url is %s \n", url);
    if (NULL == (tmp = strstr(url, "://")))
        goto fail403;
    tmp += strlen("://");
    if (NULL == (domain = strchr(tmp, '/')))
        goto fail403;
    if (NULL == (sid = strstr(url, "?sid=")))
        goto donot_verify;
    sid += strlen("?sid=");
    if (NULL == (orig = base64_decode_oupeng(sid)))
        goto fail403;

    strncpy(uri, url, domain - url);
    strcat(uri, orig);
    DEBUG("url after base64_decode is %s \n", uri);

    printf("%s %s", uri, other);
    DEBUG("VERIFY SUCCESS OUTPUT: %s %s %s\n", uri, ip, other);
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
    printf("403:%s %s", fail_dst, other);
    fflush(stdout);
    DEBUG("VERIFY FAILED OUTPUT:%s %s\n", fail_dst, other);
    return 0;

donot_verify:
    DEBUG("DO NOT VERIFY\n");
    printf("%s %s",url,other); 
    fflush(stdout);
    return 1;
}
