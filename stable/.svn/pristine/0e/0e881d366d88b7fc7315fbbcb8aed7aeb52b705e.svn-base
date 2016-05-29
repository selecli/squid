#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <dlfcn.h>
#include <ctype.h>
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
//#define DEBUG(args, ...)             fprintf(g_fpLog, args, ##__VA_ARGS__);fflush(g_fpLog);

#else

#define CRITICAL_ERROR(args, ...)    printf(args, ##__VA_ARGS__);fflush(stdout);
#define NOTIFICATION(args,...)       printf(args, ##__VA_ARGS__);fflush(stdout);
#define DEBUG(args, ...)             printf(args, ##__VA_ARGS__);fflush(stdout);

#endif
#define MAX_ITEM 20
typedef struct music163 
{
    char *replace_url;
	char *cookie_value[MAX_ITEM];
	int cookie_count;

}ST_163MUSIC_CFG;

/*  ½âÎöÊ¢´ó·ÀµÁÁ´ÅäÖÃÐÐ
 * sdo.download.ccgslb.net none 163music refere=idj.163.com  http://listen.idj.126.net/rj/rj.jsp 
 * */
int analysis163MusicCfg(struct redirect_conf *pstRedirectConf)
{
	char *pTemp;
	char *pcSep=" \t\r\n";
	ST_163MUSIC_CFG *pstC = NULL;
	char *cookie_next;
	char *cookie_ptr;
	int i = 0 ;
	
	pstC = (ST_163MUSIC_CFG*)malloc(sizeof(ST_163MUSIC_CFG));
	if(NULL == pstC)
	{
		CRITICAL_ERROR("No memory\n");
		goto error;
	}
	
	pTemp = xstrtok(NULL, pcSep);
	if(NULL == pTemp)
	{
		DEBUG("configure file is error,lack of cookie value");
		goto error;
	}

	while((cookie_ptr = strtok_r(pTemp,"|",&cookie_next))!=NULL) 
	{
		pstC->cookie_value[i] = strdup(cookie_ptr);
		DEBUG("pstC->cookie_value[%d] = %s\t ,cookie_next [%s]\n",i,pstC->cookie_value[i],cookie_next);
		i++;
		pTemp = NULL;
	}
	pstC->cookie_count = i--;
	for(i = 0; i <  pstC->cookie_count ; i ++)
		DEBUG("pstC->cookie_value[%d] = %s\n",i,pstC->cookie_value[i]);

	pTemp = xstrtok(NULL, pcSep);
	if(NULL == pTemp)
	{
		DEBUG("configure file is error,lack of replace url");
		goto error;
	}
	pstC->replace_url= strdup(pTemp);
	DEBUG("pstC->replace_url= %s\n", pstC->replace_url);

	pstRedirectConf->other = (void*)pstC;

	return 0;

error:
	if(NULL != pstC)
	{
		free(pstC);
	}

	for(i=0 ; i < pstC->cookie_count; i++)
	{
		if(NULL != pstC->cookie_value[i])
			free(pstC->cookie_value[i]);
		
	
	}
	if(NULL != pstC->replace_url)
		free(pstC->replace_url);

	return -1;

}

int getCookie(char *str,char **cookie)
{
	const char *header = "Cookie:";
	char *p =strstr(str,header);
	char *value_start = NULL;
	if ( NULL == p)
	{
		DEBUG("url format is wrong in getCookie,no cookie\n");
		return 0;
	}
	p = p + strlen(header);

	value_start = *cookie =  p;

	DEBUG("value_start %s\n",value_start);

//	while(!isspace(*p++));
    while(*p!= ':' && *p!= 0)
	{
		p++;
	}

	return p - value_start+1;

}

int getUri(char *url ,char **uri)
{
	*uri = NULL;
	char *ptr = strstr(url,"http://");
	if(NULL == ptr)
	{
		return 0;
	}
	else 
		ptr += 7;

	ptr = strchr(ptr,'/');
	if(NULL == ptr)
		return 0;
	else
		ptr ++;

	*uri = ptr;

	return 1;
}

int verify163Music(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other)
{
	char *p_cookie;
	//case3544:
	ST_163MUSIC_CFG* pstC = pstRedirectConf->other;
    char *uri = NULL;
	assert(pstC != NULL);
/*
	//case3544
	p_range = strstr(other,"Range:");
	if(p_range)
		goto fail403;
*/
	if(!getUri(url,&uri))
		goto fail301;

	DEBUG("uri = %s\n",uri);

	int len = getCookie(other,&p_cookie);
	if (0 == len)
	{
		DEBUG("no cookie,goto fail\n");
		goto fail301;
		
	}
	int deny_or_not = 1;
	int i = 0 ;
//	char *uri = strchr(strstr(url,"http://")+7,'/')+1;

	char *cookie = (char *)malloc(len);
	memset(cookie,0,len);
	if(NULL == cookie)
	{
		goto fail301;
	}
	else
	{
		memcpy(cookie,p_cookie,len-1);
    }

	DEBUG("cookie [%s],len[%d]\n",cookie,len);


	for(i=0 ; i < pstC->cookie_count;i++)
	{
	//	if(0 == strcmp(cookie,pstC->cookie_value[i]))
    	if(strstr(cookie,pstC->cookie_value[i]))
		{
			deny_or_not = 0;
			break;
		}
	}

	if(cookie)
	{
		free(cookie);
		cookie = NULL;
	}

	DEBUG("deny or not [%d],replace_url[%s],uri[%s]\n", deny_or_not,pstC->replace_url,uri);

	if(!deny_or_not)
	{
		printf("%s %s %s", url, ip, other);
		DEBUG("OUTPUT: %s %s %s", url, ip, other);
		fflush(stdout);
	}
	else
	{
		printf("301:%s?r=%s %s %s", pstC->replace_url ,uri,ip, other);
		DEBUG("VERIFY FAILED OUTPUT:301:%s?r=%s %s %s", pstC->replace_url, uri, ip, other);
		fflush(stdout);
	}
	return 1;

fail301:
	if(NULL == uri)
	{
		printf("301:%s?r= %s %s", pstC->replace_url ,ip, other);
		DEBUG("VERIFY failed output:301:%s %s %s", pstC->replace_url, ip, other);
	}
	else
	{
		printf("301:%s?r=%s %s %s", pstC->replace_url ,uri,ip, other);
		DEBUG("VERIFY failed output:301:%s?r=%s %s %s", pstC->replace_url,uri, ip, other);
	}
	fflush(stdout);
	return 0;
/*
fail403:
	printf("403:%s %s %s", url, ip, other);
	DEBUG("OUTPUT:403: %s %s %s", url, ip, other);
	fflush(stdout);
	return 0;
*/
}
