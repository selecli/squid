#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "redirect_conf.h"
#include "display.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define CONFIGURE_ERR_DIR "/data/proclog/log/squid/"
#define CONFIGURE_ERR_FILE "/data/proclog/log/squid/redirectConfError.log"
#define BUF_SIZE 1024

///////////////////////////////////////////////////////////////////////////////////

static FILE * gRdtConfFileFp = NULL;
static char *gRdtBuf = NULL;
///////////////////////////////////////////////////////////////////////////////////
static char * cc_redirect_check_obtion(void)
{
    if(NULL == gRdtBuf)
        gRdtBuf = malloc(BUF_SIZE+1);
    memset(gRdtBuf, 0, BUF_SIZE+1);
    return gRdtBuf;
}

void cc_redirect_check_clean(void)
{
    if(cc_redirect_check_obtion())
    {
        snprintf(gRdtBuf, BUF_SIZE, "rm -f %s", CONFIGURE_ERR_FILE);
        system(gRdtBuf);
        free(gRdtBuf);
        gRdtBuf = NULL;
    }
}

static void cc_redirect_check_done(char *buf)
{
    if(gRdtConfFileFp)
    {
        fprintf(gRdtConfFileFp, "Redirect: %s", buf);
        fflush(gRdtConfFileFp);
    }
}

int cc_redirect_check_start(int no_count, int column_num,  char *line)
{
    if(g_iRedirectConfCheckFlag && line && cc_redirect_check_obtion())
    {
        char *p = NULL;
        if(NULL == gRdtConfFileFp)
        {
            if(access(CONFIGURE_ERR_DIR,F_OK))
            {
                snprintf(gRdtBuf, BUF_SIZE, "mkdir -p %s",CONFIGURE_ERR_DIR);
                system(gRdtBuf);
            }
            gRdtConfFileFp = fopen(CONFIGURE_ERR_FILE, "w+");
            snprintf(gRdtBuf, BUF_SIZE,"Redirect Check ConfigureFile Time %ld\nProcess Name: [line-No.  column-No.     Conf-Line]\n", time(NULL));
            if(gRdtConfFileFp)
            {
                fprintf(gRdtConfFileFp, "%s", gRdtBuf);
            }
        }
        if((p = strchr(line, '\r')))
            *p = '\0';
        if((p = strchr(line, '\n')))
            *p = '\0';
        memset(gRdtBuf, 0, BUF_SIZE);
        snprintf(gRdtBuf, BUF_SIZE, "[%-3d %-2d %s]\n", no_count,column_num, line);
        cc_redirect_check_done(gRdtBuf);
    }
    return 0;
}

int cc_redirect_check_close(void)
{
	if(gRdtConfFileFp)
	{
        cc_redirect_check_done("Check End\n\n");
        fflush(gRdtConfFileFp);
		fclose(gRdtConfFileFp);
	}
    if(gRdtBuf)
    {
        free(gRdtBuf);
    }
    gRdtBuf = NULL;
    gRdtConfFileFp = NULL;
	return 0;
}
#undef CONFIGURE_ERR_DIR
#undef CONFIGURE_ERR_FILE
#undef BUF_SIZE
/////////////////////////////////////////////////////////////////////////////////////////////
extern struct redirect_conf* g_pstRedirectConf;
extern int g_iRedirectConfNumber;

extern FILE* g_fpLog;

/*给配置文件的一条记录释放内存，只释放各个成员变量的内容*/
static inline void freeOneRedirectConf(struct redirect_conf* pstRedirectConf)
{
	if (!pstRedirectConf)
		return;

	if(NULL != pstRedirectConf->domain)
	{
		free(pstRedirectConf->domain);
	}
	if(NULL != pstRedirectConf->cust)
	{
		free(pstRedirectConf->cust);
	}
	if(NULL != pstRedirectConf->key)
	{
		free(pstRedirectConf->key);
	}
	if(NULL != pstRedirectConf->key2)
	{
		free(pstRedirectConf->key2);
	}
	if(NULL != pstRedirectConf->replace_host)
	{
		free(pstRedirectConf->replace_host);
	}
	if(NULL != pstRedirectConf->replace_dir)
	{
		free(pstRedirectConf->replace_dir);
	}
	if(NULL != pstRedirectConf->fail_dst)
	{
		free(pstRedirectConf->fail_dst);
	}
	if(NULL != pstRedirectConf->other)
	{
		free(pstRedirectConf->other);
	}
}
/*释放一组配置文件内容的内存，只释放各条记录的成员变量的内存
 *输入：
 *    pstRedirectConf----数组指针
 *    number----记录个数
*/
void freeSomeRedirectConf(struct redirect_conf* pstRedirectConf, int number)
{
	int i=0;
	for(;i<number;i++)
		freeOneRedirectConf(pstRedirectConf+i);
}

static inline void getStrAndLength(char* buffer, struct redirect_conf* pstRedirectConf)
{
	char* str = buffer;
	char* str2;
	struct str_and_length str_length[30];
	int i = 0;
	while(NULL != (str2=strchr(str, '|')))
	{
		str_length[i].len = str2-str;
		str_length[i].str = (char*)malloc(str_length[i].len+1);
		memcpy(str_length[i].str, str, str_length[i].len);
		str_length[i].str[str_length[i].len] = 0;
		str = str2 + 1;
		i++;
	}
	str_length[i].len = strlen(str);
	str_length[i].str = (char*)malloc(str_length[i].len+1);
	memcpy(str_length[i].str, str, str_length[i].len);
	str_length[i].str[str_length[i].len] = 0;
	i++;

	pstRedirectConf->other = calloc(i+1, sizeof(struct str_and_length));
	memcpy(pstRedirectConf->other, str_length, i*sizeof(struct str_and_length));
}
static inline int analysisRegexMatch(const char* buffer, int incase, struct replace_regex* replace)
{
	int ret = 0;
	const char* str = buffer;
	while(NULL != (str=strchr(str, ')')))
	{
		if('\\' != *(str-1))
		{
			ret++;
		}
		str++;
	}
	if(ret > REPLACE_REGEX_SUBSTR_NUMBER)
	{
		return -1;
	}
	int flags = REG_EXTENDED;
	if(incase)
	{
		flags |= REG_ICASE;
	}
	if(0 != regcomp(&replace->reg, buffer, flags))
	{
		return -1;
	}
	return ret;
}

static inline int analysisRegexReplace(const char* buffer, struct replace_regex* replace)
{
	int ret = 0;
	char* temp_replace[200];
	const char* str = buffer;
	const char* str2;
	int i = 0;
	while(NULL != (str2 = strchr(str, '\'')))
	{
		temp_replace[i] = malloc(str2-str+1);
		memcpy(temp_replace[i], str, str2-str);
		temp_replace[i][str2-str] = 0;
		temp_replace[i+1] = (char*)(long)atol(str2+1);
		if((long int)temp_replace[i+1] > ret)
		{
			ret = (long int)temp_replace[i+1];
		}
		if((long int)temp_replace[i+1] > 9)
		{
			str = str2 + 3;
		}
		else
		{
			str = str2 + 2;
		}
		i += 2;
	}
	if(0 != *str)
	{
		temp_replace[i] = malloc(strlen(str)+1);
		strcpy(temp_replace[i], str);
		i++;
	}
	temp_replace[i] = 0;
	i++;
	replace->replace = (char**)malloc(i*sizeof(char*));
	memcpy(replace->replace, temp_replace, i*sizeof(char*));
	return ret;
}

static inline int analysisReplaceRegex(struct redirect_conf* pstRedirectConf)
{
	char* str =xstrtok(NULL, " \t\r\n");
	if(NULL == str)
	{
		fprintf(g_fpLog, "no match string\n");
		return -1;
	}
	char* str2 =xstrtok(NULL, " \t\r\n");
	if(NULL == str)
	{
		fprintf(g_fpLog, "no replace string\n");
		return -1;
	}
	char* str3 =xstrtok(NULL, " \t\r\n");
	pstRedirectConf->regex_incase = 0;
	if((NULL!=str3) && ('i'==*str3))
	{
		pstRedirectConf->regex_incase = 1;
	}

	struct replace_regex* replace = (struct replace_regex*)calloc(1, sizeof(struct replace_regex));
	int number = analysisRegexMatch(str, pstRedirectConf->regex_incase, replace);
	if(number < 0)
	{
		fprintf(g_fpLog, "match string error\n");
		free(replace);
		return -1;
	}
	int number2 = analysisRegexReplace(str2, replace);
	if(number2 < 0)
	{
		fprintf(g_fpLog, "replace string error\n");
		free(replace);
		return -1;
	}
	if(number2 > number)
	{
		fprintf(g_fpLog, "replace string number is too large\n");
		free(replace);
		return -1;
	}

	pstRedirectConf->other = replace;
	return 0;
}

static inline int getRefererValue(char* referer, void** other)
{
	char* pstr[80];
	int i;
	char* str;
	str = strtok(referer, "|");
	if(NULL == str)
	{
		return 0;
	}
	for(i=0;i<79;i++)
	{
		if(NULL == str)
		{
			pstr[i] = NULL;
			break;
		}
		if(0 == strcasecmp(str, "null"))
		{
			pstr[i] = (char*)malloc(4);
			pstr[i][0] = 1;
			pstr[i][1] = 0;
		}
		else if(0 == strcasecmp(str, "blank"))
		{
			pstr[i] = (char*)malloc(4);
			pstr[i][0] = 2;
			pstr[i][1] = 0;
		}
		else
		{
			pstr[i] = (char*)malloc(strlen(str)+1);
			strcpy(pstr[i], str);
		}
		str = strtok(NULL, "|");
	}
	char** pp = (char**)malloc((i+1)*sizeof(char*));
	memcpy(pp, pstr, (i+1)*sizeof(char*));
	*other = (void*)pp;
	return i;
}
#ifdef QQ_MUSIC
static inline int analysisQQMusic(struct redirect_conf* pstRedirectConf)
{
	char* str =xstrtok(NULL, " \t\r\n");
	if(NULL == str)
	{
		fprintf(g_fpLog, "no verify\n");
		return -1;
	}
	if(0 == strcmp(str, "off"))
	{
		pstRedirectConf->md5_start = 0;
	}
	else
	{
		pstRedirectConf->md5_start = 1;
	}

	str =xstrtok(NULL, " \t\r\n");
	if(NULL == str)
	{
		fprintf(g_fpLog, "no fail_dst\n");
		return -1;
	}
	if(0 == strcasecmp(str, "null"))
	{
		pstRedirectConf->fail_dst = NULL;
	}
	else
	{
		pstRedirectConf->fail_dst = malloc(strlen(str)+1);
		strcpy(pstRedirectConf->fail_dst, str);
	}

	pstRedirectConf->decode = 0;
	pstRedirectConf->other = 0;
	str =xstrtok(NULL, " \t\r\n");
	if(NULL == str)
	{
		return 0;
	}
	if(0 != strncasecmp(str, "Referer:", 8))		//rid_question
	{
		pstRedirectConf->decode = atoi(str);
		str =xstrtok(NULL, " \t\r\n");
		if(NULL == str)
		{
			return 0;
		}
		if(0 != strncasecmp(str, "Referer:", 8))		//Referer
		{
			fprintf(g_fpLog, "no Referer\n");
			return -1;
		}
	}
	str += 8;
	if(getRefererValue(str, &pstRedirectConf->other) <= 0)
	{
		fprintf(g_fpLog, "no Referer value\n");
		return -1;
	}
	
	return 0;
}
#endif

/* added by jiangbo.tian for duowan's requirement */
#ifdef DUOWAN
static inline int analysisDuoWanCfg(struct redirect_conf* pstRedirectConf)
{
	srand((int)(time(0)));
	pstRedirectConf->other = (void*)malloc(sizeof(struct duowanData));
	struct duowanData* other = (struct duowanData*)pstRedirectConf->other;
	char* str =xstrtok(NULL, " \t\r\n");
	if(NULL == str)
	{
		fprintf(g_fpLog,"no verify\n");
		return -1;
	}
	char* str2;
	if(NULL==(str2 = strchr(str,'%')))
	{
		fprintf(g_fpLog,"error configure entry for duowan\n");
		return -1;
	}

	/*
	 * char* percentWord = (char*)malloc(str2-str+1);

	strncpy(percentWord,str,str2-str);
	percentWord[str2-str+1] = 0;
	
	char percentValue = atoi(percentWord);

	if(str2<=str)
	{
		fprintf(g_fpLog,"error percentValue,reconfigure the line please\n");
		return -1;
	}
	*/

	//printf("the percentValue of atoi is: \n");
	int percentValue = atoi(str);
	//printf("the percentValue of atoi is: %d\n",percentValue);
	if(percentValue > 100 || percentValue< 0)
	{
		fprintf(g_fpLog,"error percentValue,reconfigure the line please\n");
		return -1;
	}

	other->percentValue = percentValue;
	str =xstrtok(NULL, "\t\r\n");
	if(NULL == str)
	{
		fprintf(g_fpLog, "no replace URL\n");
		return -1;
	}


	other->replaceUrl = str;

	other = NULL;


//		fprintf(g_fpLog,".....................leaving analysisDuoWanCfg\n");
	return 0;

}
#endif

static int column_num = 0;
inline char * xstrtok(char * buf, char * dem)
{
    if(g_iRedirectConfCheckFlag)
    {
        if(buf==NULL)
            column_num++;
        else
            column_num = 1;
    }
	return strtok(buf, dem);
}

static inline int analysisOneConf(const char* line, struct redirect_conf* pstRedirectConf)
{
	char buffer[BUFSIZ];
	memset(buffer, '\0', BUFSIZ);
	strcpy(buffer, line);
	int rtn_code = 0;
	//get domain
	char* str = xstrtok(buffer, " \t");
	if(NULL == str)
	{
		fprintf(g_fpLog, "get domain failure in conf=[%.*s]\n", (int)strlen(line)-1, line);
		return -1;
	}
	if(0 == strcasecmp(str, "null"))
	{
		pstRedirectConf->domain = NULL;
	}
	else
	{
		pstRedirectConf->domain = malloc(strlen(str)+1);
		strcpy(pstRedirectConf->domain, str);
	}
	//get cust
	str = xstrtok(NULL, " \t");
	if(NULL == str)
	{
		fprintf(g_fpLog, "get cult failure in conf=[%.*s]\n", (int)strlen(line)-1, line);
		return -1;
	}
	if(0 == strcasecmp(str, "null"))
	{
		pstRedirectConf->cust = NULL;
		pstRedirectConf->cust_size = 0;
	}
	else if(0 == strcasecmp(str, "none"))
	{
		pstRedirectConf->cust = NULL;
		pstRedirectConf->cust_size = -1;
	}
	else
	{
		pstRedirectConf->cust_size = strlen(str);
		pstRedirectConf->cust = malloc(pstRedirectConf->cust_size+1);
		strcpy(pstRedirectConf->cust, str);		
	}
	//get operate
	str = xstrtok(NULL, " \t\r\n");
	if(NULL == str)
	{
		fprintf(g_fpLog, "get operate failure in conf=[%.*s]\n", (int)strlen(line)-1, line);
		return -1;
	}
	char *ptr = strchr(str,':');
	if(ptr != NULL)
	{
		*ptr = 0;
		rtn_code = atoi(ptr+1);
		if( rtn_code >200 && rtn_code < 600 )
		{
			pstRedirectConf->return_code = atoi(ptr+1);
			log_output(1,"get return code in conf [%d]\n",pstRedirectConf->return_code);
		}
		else
		{
			fprintf(g_fpLog,"error:get no code or out of range[<200 >600][%d]\n",pstRedirectConf->return_code);
			pstRedirectConf->return_code = 0;       
		}
	}

#ifdef OUPENG
    if (0 == strcasecmp(str, OUPENG_FILTER_CHAR))
    {
        pstRedirectConf->operate = OUPENG_FILTER;    
    }
#endif

	if(0 == strcasecmp(str, IP_FILTER_CHAR))
	{
		pstRedirectConf->operate = IP_FILTER;
	}
	else if(0 == strcasecmp(str, IP_ABORT_FILTER_CHAR))
	{
		pstRedirectConf->operate = IP_ABORT_FILTER;
	}
	else if(0 == strcasecmp(str, TIME_FILTER_CHAR))
	{
		pstRedirectConf->operate = TIME_FILTER;
	}
	else if(0 == strcasecmp(str, TIME1_FILTER_CHAR))
	{
		pstRedirectConf->operate = TIME1_FILTER;
	}
	else if(0 == strcasecmp(str, TIME2_FILTER_CHAR))
	{
		pstRedirectConf->operate = TIME2_FILTER;
	}
	else if(0 == strcasecmp(str, IP_TIME_FILTER_CHAR))
	{
		pstRedirectConf->operate = IP_TIME_FILTER;
	}
	else if(0 == strcasecmp(str, IP_TIME_ABORT_FILTER_CHAR))
	{
		pstRedirectConf->operate = IP_TIME_ABORT_FILTER;
	}
	else if(0 == strcasecmp(str, KEY_FILTER_CHAR))
	{
	    pstRedirectConf->operate = KEY_FILTER; 
	}
	else if(0 == strcasecmp(str, REPLACE_HOST_FILTER_CHAR))
	{
		pstRedirectConf->operate = REPLACE_HOST_FILTER;
	}
	else if(0 == strcasecmp(str, DENY_FILTER_CHAR))
	{
		pstRedirectConf->operate = DENY_FILTER;
	}
	else if(0 == strcasecmp(str, BYPASS_FILTER_CHAR))
	{
		pstRedirectConf->operate = BYPASS_FILTER;
//		return 0;
	}
	else if(0 == strcasecmp(str, RID_QUESTION_FILTER_CHAR))
	{
		pstRedirectConf->operate = RID_QUESTION_FILTER;
		return 0;
	}
	else if(0 == strcasecmp(str, REPLACE_HOST_ALL_FILTER_CHAR))
	{
		pstRedirectConf->operate = REPLACE_HOST_ALL_FILTER;
	}
	else if(0 == strcasecmp(str, REPLACE_REGEX_FILTER_CHAR))
	{
		pstRedirectConf->operate = REPLACE_REGEX_FILTER;
		return analysisReplaceRegex(pstRedirectConf);
	}
#ifdef QQ_MUSIC
	/* add by xt for 64bit complie */
	else if(0 == strcasecmp(str, QQ_MUSIC_FILTER_CHAR))
	{
		pstRedirectConf->operate = QQ_MUSIC_FILTER;
		return analysisQQMusic(pstRedirectConf);
	}
#endif
#ifdef NNPAN
    /* add by chenqi for 99pan.com */
    else if (0 == strcasecmp(str, NINETY_NINE_CHAR))
    {
        pstRedirectConf->operate = NINETY_NINE_FILTER;
        return analysisNinetyNineCfg(pstRedirectConf);
    }
#endif
#ifdef NOKIA
    else if (0 == strcasecmp(str, NOKIA_FILTER_CHAR))
    {
        pstRedirectConf->operate = NOKIA_FILTER;
        return analysisNokiaCfg(pstRedirectConf); 
    }
#endif
	else if(0 == strcasecmp(str, MYSPACE_FILTER_CHAR)) {	
		pstRedirectConf->operate = MYSPACE_FILTER;
	}
	else if(0 == strcasecmp(str, SINA_FILTER_CHAR)) {
		/* add by xt for sina */
		pstRedirectConf->operate = SINA_FILTER;
	}
	else if(0 == strcasecmp(str, COOKIE_MD5_FILTER_CHAR)) {
		pstRedirectConf->operate = COOKIE_MD5_FILTER;
		return analyseCookieMd5Cfg(pstRedirectConf);
	}
	else if(0 == strcasecmp(str, QQ_TOPSTREAM_FILTER_CHAR)){
		pstRedirectConf->operate = QQ_TOPSTREAM_FILTER;
		return analyseQQTopStreamCfg(pstRedirectConf);
	}
    else if(0 == strcasecmp(str, TUDOU_FILTER_CHAR)){
        pstRedirectConf->operate = TUDOU_FILTER;
        return analyseTudouCfg(pstRedirectConf);
    }

#ifdef OUPENG
    else if (0 == strcasecmp(str, OUPENG_FILTER_CHAR))
    {
		pstRedirectConf->operate = OUPENG_FILTER;
		return analysisOupengCfg(pstRedirectConf);
    }
#endif
    else if (0 == strcasecmp(str, LONGYUAN_FILTER_CHAR))
    {
        pstRedirectConf->operate = LONGYUAN_FILTER;
        return analysisLongyuanCfg(pstRedirectConf);
    }
#ifdef DUOWAN
	/* add by Jiangbo.tian xt for 64bit complie */
	else if(0 == strcasecmp(str, DUOWAN_FILTER_CHAR))
	{
		pstRedirectConf->operate = DUOWAN_FILTER;
		return analysisDuoWanCfg(pstRedirectConf);
	}
#endif
#ifdef QQ_GREEN_MP3
	else if(0 == strcasecmp(str,QQ_GREEN_HIGH_PERFORM_CHAR))
	{
		pstRedirectConf->operate = QQ_GREEN_HIGH_PERFORM_FILTER;           
		fprintf(g_fpLog,"befor analysisQQGreenHighPerformCfg\n");
		return analysisQQGreenMP3Cfg(pstRedirectConf);
	}
#endif
#ifdef PPTV
    else if(0 == strcasecmp(str,PPTV_ANTIHIJACK_CHAR))
    {   
        pstRedirectConf->operate = PPTV_ANTIHIJACK_FILTER;    
        fprintf(g_fpLog,"befor analysisPPVodCfg\n");
        return analysisPPTVCfg(pstRedirectConf);
    }   
#endif
#ifdef SDO
	else if(0 == strcasecmp(str,SDO_CHAR))
	{
		pstRedirectConf->operate = SDO_FILTER;           
		fprintf(g_fpLog,"befor analysisSDOCfg\n");
		return analysisSDOCfg(pstRedirectConf);
	}
#endif

#ifdef OOYALA
	else if(0 == strcasecmp(str,OOYALA_FILTER_CHAR))
	{
		pstRedirectConf->operate = OOYALA_FILTER;
		fprintf(g_fpLog,"befor analysisOoyalaCfg\n");
		return analysisOoyalaCfg(pstRedirectConf);
	}
#endif

    // added for microsoft by chenqi
    else if (0 == strcasecmp(str,MICROSOFT_FILTER_CHAR))
    {    
        pstRedirectConf->operate = MICROSOFT_FILTER;
        fprintf(g_fpLog, "befor analysisMicrosoftCfg\n");
        return analysisMicrosoftCfg(pstRedirectConf);
    }    
    else if (0 == strcasecmp(str,MICROSOFT_MD5_FILTER_CHAR))
    {    
        pstRedirectConf->operate = MICROSOFT_MD5_FILTER;
        fprintf(g_fpLog, "befor analysisMicrosoftCfg_MD5\n");
        return analysisMicrosoftCfg_MD5(pstRedirectConf);
    }    
    // add end
#ifdef IQILU
    else if(0 == strcasecmp(str,IQILU_CHAR))	
        {
                pstRedirectConf->operate = IQILU_FILTER;
                fprintf(g_fpLog,"befor analysisIQILUCfg\n");
                return analysisIQILUCfg(pstRedirectConf);
        }
#endif
	else if(0 == strcasecmp(str,ICONV_FILTER_CHAR))
	{
		pstRedirectConf->operate = DECODE_FILTER;           
	}
	else if(0 == strcasecmp(str,LIGHTTPD_SECDOWNLOAD_FILTER_CHAR))
	{
        pstRedirectConf->operate = LIGHTTPD_SECDOWNLOAD_FILTER;           
    }
    else if(0 == strcasecmp(str,MFW_MUSIC_FILTER_CHAR))
    {   
        pstRedirectConf->operate = MFW_MUSIC_FILTER;    
        return analysisMFWMusicCfg(pstRedirectConf);
    }  
	else if(0 == strcasecmp(str, BOKECC_FILTER_CHAR))
	{
		pstRedirectConf->operate = BOKECC_FILTER;
		return analysisBokeccCfg(pstRedirectConf);
	}

    else if(0 == strcasecmp(str, MSN_FILTER_CHAR))
    {    
        pstRedirectConf->operate = MSN_DOWNLOAD_FILTER;
        return analysisMSNCfg(pstRedirectConf);
    }
    else if(0 == strcasecmp(str,MUSIC163_FILTER_CHAR))
    {
        pstRedirectConf->operate = MUSIC163_FILTER;           
        return analysis163MusicCfg(pstRedirectConf); 
    }
	else if(0 == strcasecmp(str, WANGLONG_FILTER_CHAR))
	{
		pstRedirectConf->operate = WANGLONG_FILTER;           
		return analysisWanglongCfg(pstRedirectConf);
		
	}
	else if (0 == strcasecmp(str, CWG_FILTER_CHAR))
	{
		pstRedirectConf->operate = CWG_FILTER;
		return analysisCWGCfg(pstRedirectConf);
	}
#ifdef BAID
    else if(0 == strcasecmp(str,BAIDU_XCODE_CHAR))
    {
        pstRedirectConf->operate = BAIDU_XCODE_FILTER;
        fprintf(g_fpLog,"befor analysisBaiduXcodeCfg\n");
        return analysisBaiduXcodeCfg(pstRedirectConf);
    }
#endif


	else
	{
		fprintf(g_fpLog, "operator error in conf=[%.*s]\n", (int)strlen(line)-1, line);;
		return -1;
	}
	//get key
	str = xstrtok(NULL, " \t");
	if(NULL == str)
	{
		fprintf(g_fpLog, "get key failure in conf=[%.*s]\n", (int)strlen(line)-1, line);
		return -1;
	}
	if(0 == strcasecmp(str, "null"))
	{
		pstRedirectConf->key = NULL;
	}
	else
	{
		pstRedirectConf->key = malloc(strlen(str)+1);
		strcpy(pstRedirectConf->key, str);
	}
	//get key2
	str = xstrtok(NULL, " \t");
	if(NULL == str)
	{
		fprintf(g_fpLog, "get key2 failure in conf=[%.*s]\n", (int)strlen(line)-1, line);
		return -1;
	}
	if(0 == strcasecmp(str, "null"))
	{
		pstRedirectConf->key2 = NULL;
	}
	else
	{
		pstRedirectConf->key2 = malloc(strlen(str)+1);
		strcpy(pstRedirectConf->key2, str);
	}
	//get md5_start
	str = xstrtok(NULL, " \t");
	if(NULL == str)
	{
		fprintf(g_fpLog, "get md5 start failure in conf=[%.*s]\n", (int)strlen(line)-1, line);
		return -1;
	}
	pstRedirectConf->md5_start = atoi(str);
	//get md5_length
	str = xstrtok(NULL, " \t");
	if(NULL == str)
	{
		fprintf(g_fpLog, "get md5 length failure in conf=[%.*s]\n", (int)strlen(line)-1, line);
		return -1;
	}
	pstRedirectConf->md5_length = atoi(str);
	//get decode
	str = xstrtok(NULL, " \t");
	if(NULL == str)
	{
		fprintf(g_fpLog, "get decode failure in conf=[%.*s]\n", (int)strlen(line)-1, line);
		return -1;
	}
	pstRedirectConf->decode = atoi(str);
	log_output(3,"get decode = %d\n",pstRedirectConf->decode);
	//get replace_host
	str = xstrtok(NULL, " \t");
	if(NULL == str)
	{
		fprintf(g_fpLog, "get replace host failure in conf=[%.*s]\n", (int)strlen(line)-1, line);
		return -1;
	}
	if(0 == strcasecmp(str, "null"))
	{
		pstRedirectConf->replace_host = NULL;
	}
	else
	{
		pstRedirectConf->replace_host = malloc(strlen(str)+1);
		strcpy(pstRedirectConf->replace_host, str);
		fprintf(g_fpLog, "get replace host %s\n", pstRedirectConf->replace_host);
	}
	//get replace_dir
	str = xstrtok(NULL, " \t");
	if(NULL == str)
	{
		fprintf(g_fpLog, "get replace dir failure in conf=[%.*s]\n", (int)strlen(line)-1, line);
		return -1;
	}
	if(0 == strcasecmp(str, "null"))
	{
		pstRedirectConf->replace_dir = NULL;
	}
	else
	{
		pstRedirectConf->replace_dir = malloc(strlen(str)+1);
		strcpy(pstRedirectConf->replace_dir, str);
	}
	//get fail_dst
	str = xstrtok(NULL, " \t\r\n");
	if(NULL == str)
	{
		fprintf(g_fpLog, "get fail dst failure in conf=[%.*s]\n", (int)strlen(line)-1, line);
		return -1;
	}
	if(0 == strcasecmp(str, "null"))
	{
		pstRedirectConf->fail_dst = NULL;
	}
	else
	{
		/* Fixed start: xin.yao, add code[401/403] for check failed */
		size_t n;
		int code = atoi(str);

		if (401 == code || 403 == code)
		{
			n = 3;
			pstRedirectConf->fail_dst_type = FAIL_DST_TYPE_CODE;
		}
		else
		{
			n = strlen(str);
			pstRedirectConf->fail_dst_type = FAIL_DST_TYPE_URL;
		}
		pstRedirectConf->fail_dst = malloc(n + 1);
		strncpy(pstRedirectConf->fail_dst, str, n);
		pstRedirectConf->fail_dst[n] = '\0';
		/* Fixed end: xin.yao */
	}
	/* get valid interval */
	if((TIME_FILTER==pstRedirectConf->operate) || 
		(TIME1_FILTER==pstRedirectConf->operate) ||	
		(TIME2_FILTER==pstRedirectConf->operate) ||	
		(IP_TIME_FILTER==pstRedirectConf->operate) || 
		(IP_TIME_ABORT_FILTER==pstRedirectConf->operate) ||
		(SINA_FILTER == pstRedirectConf->operate)||
		(LIGHTTPD_SECDOWNLOAD_FILTER ==  pstRedirectConf->operate))
	{
		pstRedirectConf->other = malloc(sizeof(struct valid_period));
		if(NULL == pstRedirectConf->other)
		{
			fprintf(g_fpLog, "cannot malloc\n");
			return -1;
		}
		struct valid_period* pstValidPeriod = (struct valid_period*)pstRedirectConf->other;
		//get seconds_before
		str = xstrtok(NULL, " \t");
		if(NULL == str)
		{
			fprintf(g_fpLog, "get seconds_before failure in conf=[%.*s]\n", (int)strlen(line)-1, line);
			return -1;
		}
		if(0 == strcasecmp(str, "null"))
		{
			pstValidPeriod->seconds_before = 360*60;
		}
		else
		{
			pstValidPeriod->seconds_before = atoi(str)*60;
		}
		//get seconds_after
		str = xstrtok(NULL, " \t\r\n");
		if(NULL == str)
		{
			fprintf(g_fpLog, "get seconds_after failure in conf=[%.*s]\n", (int)strlen(line)-1, line);
			return -1;
		}
		if(0 == strcasecmp(str, "null"))
		{
			pstValidPeriod->seconds_after = 360*60;
		}
		else
		{
			pstValidPeriod->seconds_after = atoi(str)*60;
		}
		log_output(3,"pstValidPeriod->seconds_before = %d\n",pstValidPeriod->seconds_before/60);
		log_output(3,"pstValidPeriod->seconds_after = %d\n",pstValidPeriod->seconds_after/60);

		/* add by xt for ip filter range */
		goto range_flag;
		/* end */
	}
	/* add by xt for ip filter range */
	/* get Flag */
	str = xstrtok(NULL, " \t\r\n");
	str = xstrtok(NULL, " \t\r\n");
range_flag:
	if((str = xstrtok(NULL, " \t\r\n"))) {
		if(1 == atoi(str)) 
			pstRedirectConf->range_flag = 1;
	}
	/* get cookie flag */
	if((str = xstrtok(NULL, " \t\r\n"))) 
		if(1 == atoi(str)) 
			pstRedirectConf->cookie_flag = 1;
	
	/* get cookie pass */
	if((str = xstrtok(NULL, " \t\r\n"))) 
		pstRedirectConf->cookie_pass = strtol(str, NULL, 16);
	/* end */
	return 0;
}

/*初始化读配置文件，给配置项的临时变量分配内存，读配置文件
 *输入：
 *    filename----配置文件名
 *    confNumber----允许的配置项的最大个数
 *返回值：
 *    0----成功
 *    -1----失败
*/
int init_redirect_conf(const char* filename, int confNumber)
{
	int ret;
    int line_num = 0;
	g_pstRedirectConf = (struct redirect_conf*)malloc(confNumber*sizeof(struct redirect_conf));
	if(NULL == g_pstRedirectConf)
	{
        fprintf(g_fpLog, "cannot malloc\n");
        cc_redirect_check_start(0, 0, "cannot malloc");
		return -1;
	}
	memset(g_pstRedirectConf, 0, confNumber*sizeof(struct redirect_conf));
	FILE* fp = fopen(filename, "r");
	if(NULL == fp)
	{
		fprintf(g_fpLog, "cannot open file=[%s]\n", filename);
        cc_redirect_check_start(0, 0, (char *)filename);
		free(g_pstRedirectConf);
		return -1;
	}
	//char line[1024];
	char line[BUFSIZ];
	memset(line, '\0', BUFSIZ);
	while(!feof(fp))
	{
		line_num++;
		
		if(NULL == fgets(line, BUFSIZ, fp))
		{
			continue;
		}
		if(('#'==line[0]) ||('\r'==line[0]) ||('\n'==line[0]) || '$' == line[0])
		{
			continue;
		}
		ret = strlen(line);
		if('\n' != line[ret-1])
		{
            cc_redirect_check_start(line_num, column_num, line);
			fprintf(g_fpLog, "file=[%s] have line long\n", filename);
			fclose(fp);
			goto read_conf_end;
		}
		
//		fprintf(g_fpLog,".....................befor analysisOneConf\n");
		ret = analysisOneConf(line, g_pstRedirectConf+g_iRedirectConfNumber);
//		fprintf(g_fpLog,".....................after analysisOneConf\n");
	
		if(-1 == ret)
		{
            cc_redirect_check_start(line_num, column_num, line);
			fclose(fp);
			goto read_conf_end;
		}
		ret = strlen(line);
		g_pstRedirectConf[g_iRedirectConfNumber].conf_string = (char*)malloc(ret);
		memcpy(g_pstRedirectConf[g_iRedirectConfNumber].conf_string, line, ret-1);
		g_pstRedirectConf[g_iRedirectConfNumber].conf_string[ret-1] = 0;
		g_iRedirectConfNumber++;
		if(g_iRedirectConfNumber >= confNumber)
		{
			break;
		}
	}
	fclose(fp);
	return 0;
read_conf_end:
	freeSomeRedirectConf(g_pstRedirectConf, g_iRedirectConfNumber);
	free(g_pstRedirectConf);
	return -1;
}

static inline int isRedirectConf(struct redirect_conf* pstRedirectConf, const char* domain, const char* cust)
{
	//if((NULL!=pstRedirectConf->domain) && (NULL==strstr(domain, pstRedirectConf->domain)))
	if (NULL == pstRedirectConf->domain)
	{
		return -1;
	}
	if (strncmp(domain, pstRedirectConf->domain, strlen(pstRedirectConf->domain)))
	{
		return -1;
	}
	if(strncmp(domain, pstRedirectConf->domain, strlen(pstRedirectConf->domain)))
	{
		return -1;
	}
	if(pstRedirectConf->cust_size <= 0)
	{
		return 0;
	}
	if(('/'==cust[pstRedirectConf->cust_size]) && (0==strncmp(cust, pstRedirectConf->cust, pstRedirectConf->cust_size)))
	{
		return 0;
	}
	return -1;
}

/*根据域名和cult找到配置文件的一项
 *输入：
 *    domain----域名
 *    cult----cult
 *返回值：
 *    NULL----没有满足要求的项
 *    其它指针----对应的项
*/
struct redirect_conf* findOneRedirectConf(const char* domain, const char* cult)
{
	int i;
	for(i=0;i<g_iRedirectConfNumber;i++)
	{
		if(0 == isRedirectConf(g_pstRedirectConf+i, domain, cult))
		{
			return g_pstRedirectConf+i;
		}
	}
	return NULL;
}

