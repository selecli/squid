#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <dlfcn.h>
#include <ctype.h>
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
#define MAX_MD5_SIZE   64 
#define MAX_URL_SIZE   1024
typedef struct _longyuan_data_t
{
    char *key;
    char *fail_url;
}longyuan_data_t;
    
int analysisLongyuanCfg(struct redirect_conf *pstRedirectConf)
{   char *pTemp;
    static char *pcSep = " \t\r\n";
    longyuan_data_t *pstC = NULL;
    
    pstC=malloc(sizeof(longyuan_data_t));
    if (NULL == pstC)
    {
        DEBUG("malloc for 'pstC' failed\n");
        goto error;
    }
    pTemp=xstrtok(NULL,pcSep);
    if (NULL == pTemp)
    {
        DEBUG("configure error,lack secureID!\n");
        goto error;
    }
    DEBUG("pstC->key is %s\n",pTemp);
    pstC->key = strdup(pTemp);

    pTemp = xstrtok(NULL,pcSep);
    if(NULL == pTemp)
    {
        DEBUG("configure error\n");
        goto error;
    }
    pstC->fail_url = strdup(pTemp);

    if(NULL == pstC->fail_url)
    {
        CRITICAL_ERROR("malloc for 'url' failed\n");
        goto error;
    }
    pstRedirectConf->other = pstC;
    return 0;

error:
    if(NULL != pstC->fail_url)
        free(pstC->fail_url);
    if(NULL != pstC->key)
        free(pstC->key);
    if(NULL != pstC)
         free(pstC);
    return -1;
}

int VarifyLongyuanMD5(char *key,char *s_path,char *filename,char *req_md5)
{  
    int ret;
    char szBuffer[2*MAX_URL_SIZE];
    char szDigest[MAX_MD5_SIZE];

    struct tm *p;
    time_t t= time(NULL);
    p = localtime(&t);
    char s[100]; 
    strftime(s, sizeof(s), "%Y%m%d", p);
    DEBUG("The time is %s\n",s);

    ret = snprintf(szBuffer, 2*MAX_URL_SIZE, "%s%s%s%s",
             s_path, filename, key, s);

    DEBUG("hash origin=[%s]\n", szBuffer);
   getMd5Digest32(szBuffer, ret, (unsigned char *)szDigest);
    strncpy(szDigest,key,1);
    DEBUG("hash dest=[%s]\n", szDigest);
    if(0 == strncasecmp(szDigest, req_md5, 32))
    {
        DEBUG("MD5 is right!\n");
        return 0;
    }
    return -1;
}
    
int verifyLongyuan(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other)
{
    char s_path[MAX_URL_SIZE];
    char filename[MAX_URL_SIZE];
    char req_md5[MAX_MD5_SIZE];
    char *str;
    char *str1;
    char *str2;
    char *str3_q = NULL;
    char *str4;  
    int i, length;
    longyuan_data_t *pstC;
    pstC = pstRedirectConf->other;

    if (NULL == url || NULL == (str = strstr(url, "://"))|| NULL == (str1 = strchr(str+3, '/')))
    {
        DEBUG("%s %s Path Not Found\n", __FILE__, __func__);
        goto failed;
    }
    DEBUG("str1: %s \n", str1);
    
    if (NULL == (str3_q = strchr(str1, '?')))
    {
        DEBUG("%s %s '?' Not Found\n", __FILE__, __func__);
        goto failed;
    }
    *str3_q = '\0';
    str2 = strrchr(str1, '/');
    *str3_q = '?';
    
    if(NULL ==  str2)
    {
        DEBUG("str2: %s \n", str2);
        goto failed;
    }

    if (NULL == (str4 = strstr(str3_q+1,"k=")))
    {
        DEBUG("str4: %s \n", str4);
        goto failed;
    }

    str4 += strlen("k=");
    if ('\0' == *str4 || 32 < strlen(str4))
    {
        DEBUG("%s %s 'k'is illegal\n",__FILE__,__func__);
        goto failed;
    }
    strncpy(req_md5,str4,32);
    req_md5[32] = '\0';

    memset(s_path, 0 ,MAX_URL_SIZE);
    memcpy(s_path, str1 ,str2-str1);
    length = strlen(s_path); 

   char  *t_path = s_path;
    for (i=0; i<length; i++) 
    { 
        *t_path= tolower(*t_path); 
        t_path++;
    }
    s_path[str2-str1]= '\0';
    
    memset(filename, 0 ,MAX_URL_SIZE);
    memcpy(filename, str2 ,5);
    filename[5]= '\0';
    
    if(0 == VarifyLongyuanMD5(pstC->key, s_path, filename, req_md5))
    {
        DEBUG("OUTPUT:[%s %s %s]\n", url, ip, other);
        printf("%s %s %s\n",url, ip, other);
        fflush(stdout);
        return 1;
    }
failed:
    DEBUG("VERFIY FAILED OUTPUT:[302:%s %s %s]\n", url, ip, other);
    printf("302:%s %s %s\n", ((longyuan_data_t*)(pstRedirectConf->other))->fail_url, ip, other);
    fflush(stdout);
    return 0;
}
