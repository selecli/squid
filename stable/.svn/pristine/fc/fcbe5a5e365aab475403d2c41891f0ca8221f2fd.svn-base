#ifdef QQ_GREEN_MP3
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <time.h>
#include "redirect_conf.h"
#include "md5.h"

extern unsigned long MusicDecryptSongId(char const * chId);
extern int download_verify(unsigned int uin,const int songid,const int curtime, const int expire_time, const char * downkey);

typedef struct qq_mp3_cfg
{
	int expire_time;
	int flag;
    char file[2048];
} QQ_GREEN_MP3_CFG;

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
#define MAX_FILENAME_LENGTH 100
#define MAX_URL_LENGTH 1024
#define MAX_TOKEN_SIZE 1024


/*  解析QQ绿钻高品质音乐配置行
 * topstream.music.qq.com none qq_topstream http://www.chinache.com
 * */

int analysisQQGreenMP3Cfg(struct redirect_conf *pstRedirectConf)
{
	QQ_GREEN_MP3_CFG *pstC = (QQ_GREEN_MP3_CFG*)malloc(sizeof(QQ_GREEN_MP3_CFG));
	if(NULL == pstC) goto err;

    // get expire time
	char *pTemp = xstrtok(NULL, " \t\r\n");
	if(NULL == pTemp) goto err;
	pstC->expire_time = atoi(pTemp);
	DEBUG("we get expire time %d seconds\n",pstC->expire_time);

    // get auth mode
	pTemp = xstrtok(NULL, " \t\r\n");
	if(NULL == pTemp) goto err;
	pstC->flag = atoi(pTemp);
	DEBUG("we get MODE [%s]\n",pstC->flag == 0?"WEAK":"STRONG");

    // get file path
	pTemp = xstrtok(NULL, " \t\r\n");
	if(NULL == pTemp) goto err;
    strcpy(pstC->file, pTemp);
    DEBUG("we get file [%s]\n", pstC->file);

    // create directory
    struct stat info;
    int ret;
    if (stat(pstC->file, &info))
    {
        ret = mkdir(pstC->file,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if(ret ==  -1 && errno != EEXIST)
        {
            DEBUG("cannot create %s: %s\n", pstC->file, strerror(errno));
            goto err;
        }
    }
    if (pstC->file[strlen(pstC->file)-1] != '/')
        strcat(pstC->file, "/");

    // /data/proclog/log/squid/qq_green/fc-qq_green_abnormal.log.$deviced.$hostname.$date
    strcat(pstC->file, "tencent_green.log");
    // get fail url
	pTemp = xstrtok(NULL, " \t\r\n");
	if(NULL == pTemp) goto err;
	pstRedirectConf->fail_dst = (char*)malloc(strlen(pTemp) + 1);
	if(NULL == pstRedirectConf->fail_dst) goto err;
	strcpy(pstRedirectConf->fail_dst, pTemp);

	pstRedirectConf->other = (void*)pstC;
    return 1;
err:
    CRITICAL_ERROR("analysisQQGreenMP3Cfg Error!\n");
    return -1;
}

static int get_songname(char *url, char **songname)
{
	char *pos_dic = NULL;
	if ((pos_dic= strrchr(url, '/')) == NULL) {   
		NOTIFICATION("The url is invalid %s \n",url);
		return -1; 
	}   
	else {   
		*songname = strdup(pos_dic+1);
		DEBUG("songname: %s\n", *songname);
	}
	memset(pos_dic+1, 0, strlen(pos_dic+1)); 
	return 0;
}

static int get_songid(char *url, unsigned long *songid)
{
    char *pos_dic = NULL;
    if ((pos_dic= strrchr(url,'/')) == NULL) {   
        NOTIFICATION("The url is invalid %s \n",url);
        return -1; 
    }   
    else {   
        *songid = (unsigned long)atol(pos_dic+1);
        DEBUG("songid: %ld\n",*songid);
    }
    return 0;
}   

static int qq_header_parse(char *other,  unsigned int* qm_uin, char *qm_key, \
char *qm_privatekey,int* qm_qqmusic_fromtag) {
	char uin_char[100]={0};
	char qqmusic_fromtag_char[1024]={0};
	char qqkey[1024] ={0};
	char downkey[1024] ={0};
	int qqmusic_fromtag =-1;
	char *str;
	char *str2;
	char buf[4096];
	char *header_name ="Down-Param:"; 
	char *pcheckHeader = NULL;
	

	if(!other)
		return -1;
	if(strlen(other) > 4095)
		return -1;
	strncpy(buf, other, 4096);

	pcheckHeader = strstr(buf,header_name);
    str = strtok(pcheckHeader,"&");
	str = str + strlen(header_name);
	if(!strchr(str,'='))
	{
		DEBUG("QQ GREEN MP3 has no verify infomation %s\n",str);
		return -1;

	}
	while(str)
	{    
		str2 = strchr(str,'=');
		if(str2)
		{    
		    str2++;
			while(' ' == *str2) str2++;
			while(' ' == *str) str ++;
			if(0 == strncmp(str, "uin",3))
            {    
                if(*str2== '\0' )
                    fprintf(g_fpLog, "1qqmusic_uin = [%u]\n", *qm_uin);
                memcpy(uin_char, str2, 24);
                *qm_uin = atoll(uin_char);
                if (g_fdDebug > 3) 
                    fprintf(g_fpLog, "2qqmusic_uin = [%u]\n", *qm_uin);
            }    
			else if(0 == strncmp(str, "qqkey",5))
			{    
				if( *str2=='\0' )
				{
					qqkey[0]='&';  //special
					goto end;
				}
				memcpy(qqkey, str2, 1024);
				if(g_fdDebug > 3) 
				{    
					fprintf(g_fpLog, "qqmusic_key = [%s]\n", qqkey);
				}    
			}
			else if(0 == strncmp(str, "downkey",7))
			{    

				if(*str2 == '\0' )
				{
					downkey[0]='&';  //special
					goto end;
				}
				memcpy(downkey, str2, 1024);
				if(g_fdDebug > 3) 
				{    
					fprintf(g_fpLog, "down_key = [%s]\n", downkey);
				}    
			}
			else if (0 == strncmp(str,"qqmusic_fromtag",15))
			{
				if(*str2== '\0' )
				{
					qqmusic_fromtag = -1;
					fprintf(g_fpLog, "qqmusic_fromtag = [%d]\n", qqmusic_fromtag);
				}
			    memcpy(qqmusic_fromtag_char, str2, 1024);
				qqmusic_fromtag = atoi(qqmusic_fromtag_char);
				if(g_fdDebug > 3) 
				{    
					fprintf(g_fpLog, "qqmusic_fromtag = [%d]\n", qqmusic_fromtag);
				}    

			}
	    }
		else
        	break;
	end:
		str = strtok(NULL,"&");
	}

	if(qm_qqmusic_fromtag)
	    *qm_qqmusic_fromtag = qqmusic_fromtag;
	if(qm_key)
		strncpy(qm_key, qqkey, 1024);
	if(qm_privatekey)
		strncpy(qm_privatekey, downkey, 1024);

	return 0;
}


/* 解析qq 绿钻高品质MP3请求并认证
 * 返回值：
 * 1  校验通过
 * 0  校验失败
 * 输入参数：
 * pstRedirectConf: 配置信息
 * url
 * ip
 * other 结构体struct Request中的三个成员
 * */
 /*
*/
int qqGreenMp3Verify(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other)
{
	char *fail_dst = NULL;
	char *pcTmp;
	const char *pcKey = "Down-Param:";
	int valid = 0;
	int qqmusic_fromtag = -1;

	unsigned int uin;
	char qqkey[MAX_TOKEN_SIZE];
	char downkey[MAX_TOKEN_SIZE];
	
	char new_url[MAX_URL_LENGTH] = "";
	int return_value;

	int curtime;
	unsigned long songid = 0;

	curtime = time(0);
    QQ_GREEN_MP3_CFG *pstC = (QQ_GREEN_MP3_CFG*)pstRedirectConf->other;
    assert(NULL != pstC);
	

	uin = 0;
	memset(qqkey,'0',MAX_TOKEN_SIZE);
	memset(downkey,'0',MAX_TOKEN_SIZE);

	strcpy(new_url,url);

	/*case 3958, new domain named "downloady.music.qq.com" */
	char *domain = NULL;
	if (NULL != (domain = strstr(new_url, "downloady.music.qq.com"))) {
		char *songname = NULL;
		char suffix_name[20];
		char *tmp;
		if (-1 == get_songname(new_url, &songname))
			goto fail403;
		if (!songname)
			goto fail403;

		tmp = strrchr(songname, '.');
		strcpy(suffix_name, tmp);
		*tmp = '\0';
		songid = MusicDecryptSongId(songname);
		DEBUG("songname = %s\tsongid = %ld\n", songname,songid);
		sprintf(new_url + strlen(new_url), "%ld%s", songid, suffix_name);
		free(songname);
		songname = NULL;
		// change domain name
		*(domain + strlen("download")) = 'x';
		DEBUG("QQ Green MP3, after translating string to digit: %s\n", new_url);
	}
	/*case 3958, end by chenqi*/

	// int redirect_ok =0;
    //redirect_ok = qqGreenTopStreamUrlRedirect(new_url,&songid);
	if (0 == songid) {
		if (-1 == get_songid(new_url, &songid))
			goto fail403;
	}
    
	pcTmp = strstr(other,pcKey);
    /* Header:Down-Param存在么? */

	if(NULL == pcTmp)
	{
		DEBUG("QQ Green MP3 %s has no header:Down-Param \n",other);
		goto fail403;

	}
	
	return_value = qq_header_parse(pcTmp,&uin,qqkey,downkey,&qqmusic_fromtag);
	if(-1 == return_value)
	{
		goto fail403;
	}
	DEBUG("after qmusic_fromtag = %d\n",qqmusic_fromtag);

	if((0 == qqkey[0])||(0 == downkey[0]))    
		goto fail403;

    if('&'==qqkey[0])  //if "the qqkey=&downkey&&" : qqkey =NULL,not go to fail403
	{
        DEBUG("qqkey NULL");
		qqkey[0]=0;
	}
    if('&'==downkey[0])
	{
        DEBUG("downkey NULL");
		downkey[0]=0;
	}
	DEBUG("===================================\n");
	DEBUG("uin =%u\n",uin);
	DEBUG("curtime = %d\n",curtime);
	DEBUG("expire_time = %d\n",pstC->expire_time);
	DEBUG("downkey= %s\n",downkey);
	DEBUG("songid= %ld\n",songid);
	DEBUG("===================================\n");
	
	if(pstC->flag == 0)
	{
		if((uin > 10000 )&& (strlen(downkey) > 32) && (qqmusic_fromtag >=0 && qqmusic_fromtag <=30))
		{
			DEBUG("In Weak Mode: auth ok\n");
			goto succeed;
		}
		else
		{
			DEBUG("In Weak Mode: auth ng\n");
			goto fail403;
		}
		
	}
	DEBUG("In STRONG Mode: auth ...\n");
	valid = download_verify( uin, songid ,curtime, pstC->expire_time, downkey);

	DEBUG("valid is %d\n",valid)
	if(valid < 0)
	{
        /* record abnormal verify  for case 3306 */
        FILE *fp = fopen(pstC->file, "a+");
        if (NULL == fp)
        {
            DEBUG("QQ Green MP3 abnormal info. recorded failed\n");
            goto fail403;
        }
        fprintf(fp, "download_verify return %d\nuin = %u\ncurrent_time = %d\nexpire_time = %d\n down_key = %s\nmusic_id = %ld\n",valid,uin,curtime,pstC->expire_time,downkey,songid);
        fprintf(fp, "======================================\n");
        fclose(fp);
        /* record abnormal verify  by chenqi */
    	DEBUG("QQ BLUE MP3 VERIFY FAILURE,uin = [%u],qqmusic_fromtag=[%d],qqkey=[%s],downkey=[%s],GOTO 403\n",uin,qqmusic_fromtag,qqkey,downkey);
		goto fail403;
	}

	if(qqmusic_fromtag >=0 && qqmusic_fromtag <=30)//Cookie中没有
	{
		DEBUG("QQ BULE MP3 VERIFY OK, with qqmusic_fromtag in Down-Param, qqmusic_fromtag =%d\n",qqmusic_fromtag);
		goto succeed;
	}
	else
	{
		DEBUG("QQ BULE MP3 VERIFY FAILURE, with no qqmusic_fromtag in Down-Param\n");
		goto fail403;
	}

succeed:
	printf("%s %s %s", new_url, ip, other);
	DEBUG("OUTPUT: %s %s %s\n", new_url, ip, other);
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
	printf("%s %s %s", fail_dst, ip, other);
	fflush(stdout);
	DEBUG("VERIFY FAILED OUTPUT:%s %s %s\n", fail_dst, ip, other);
	return 0;
}
#endif
