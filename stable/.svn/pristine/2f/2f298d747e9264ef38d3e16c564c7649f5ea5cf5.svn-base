#ifdef NOKIA

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <sys/utsname.h>
#include "redirect_conf.h"
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

/*  ½âÎöNokia·ÀµÁÁ´ÅäÖÃÐÐ
 *  origin.stg.mrsmon.lbs.ovi.com.cn none nokia _xMavbG_zALVSpHB1lVW hUBoV-w-FrXHd0ywDK4G2g 
 * */
int analysisNokiaCfg(struct redirect_conf *pstRedirectConf)
{
	char *pstC;
	
    if (NULL == (pstC = xstrtok(NULL, " \t\r\n")))
		goto error;
    pstRedirectConf->key = strdup(pstC);   // app_id

    if (NULL == (pstC = xstrtok(NULL, " \t\r\n")))
		goto error;
    pstRedirectConf->key2 = strdup(pstC);  // app_code
    DEBUG("Nokia Redirect: app_id=%s,app_code=%s\n",pstRedirectConf->key,pstRedirectConf->key2);
    return 0;

error:
    DEBUG("Nokia Redirect: analysisOupengCfg failed\n");
    if (NULL != pstRedirectConf->key)
        free(pstRedirectConf->key);
    if (NULL != pstRedirectConf->key2)
        free(pstRedirectConf->key2);
	return -1;
}

static inline void param_process(const struct redirect_conf *pstRedirectConf,char *param, char *uri)
{
    char buffer[100];
    memset(buffer,0,100);
    if (strstr(param, "app_id="))
    {
        snprintf(buffer,100,"app_id=%s&", pstRedirectConf->key);
        strcat(uri, buffer);
    }
    else if (strstr(param,"app_code="))
    {
        snprintf(buffer,100,"app_code=%s&", pstRedirectConf->key2);
        strcat(uri, buffer);
    }
    else if (strstr(param, "token="))
    {
        ; 
    }
    else 
    {
        snprintf(buffer,100,"%s&",param);
        strcat(uri,buffer);
    }
}

// delete: app_id, app_code, token
// add: app_id=_xMavbG_zALVSpHB1lVW  app_code=hUBoV-w-FrXHd0ywDK4G2g¡±
int NokiaVerify(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other)
{
    int one_param = 0;
    char *tmp = NULL;
    char *param = NULL;
    char uri[BUFSIZ];
    memset(uri, 0, BUFSIZ);
    char buffer[100];
    memset(buffer,0,100);

    DEBUG("client url is %s \n", url);
    if (NULL == (tmp = strchr(url, '?')))
        goto donot_verify;
    tmp++;
    if ('\0' == *tmp)
        goto donot_verify;

    strncpy(uri,url,tmp-url);
    if (NULL == (param = strtok(tmp, "&")))
    {
        param = tmp;    // only one parameter
        one_param = 1;
    }
    param_process(pstRedirectConf, param, uri);    
    while (0 == one_param)
    {
        if (NULL == (param = strtok(NULL, "&")))
            break;
        param_process(pstRedirectConf, param, uri);    
    }

    if (!strstr(uri, "app_id="))
    {
        snprintf(buffer,100,"app_id=%s&", pstRedirectConf->key);
        strcat(uri, buffer);
    }
    if (!strstr(uri, "app_code="))
    {
        snprintf(buffer,100,"app_code=%s&", pstRedirectConf->key2);
        strcat(uri, buffer);
    }
    int len = strlen(uri) - 1;
    if ('&' == uri[len])
        uri[len] = '\0';

    DEBUG("url after redirect is %s \n", uri);
    printf("%s %s", uri, other);
    DEBUG("REDIRECT SUCCESS OUTPUT: %s %s %s\n", uri, ip, other);
    fflush(stdout);
    return 1;

donot_verify:
    DEBUG("DO NOT REDIRECT\n");
    printf("%s %s",url,other); 
    fflush(stdout);
    return 1;
}

#endif
