/* This is redirect for tudou
 * 算法：
 * 1. 加密形式： 在原始url 后面添加 ?key=value 字符串，如下：
 *    原始 url:  	http://www.tudou.com/flv/001/123/123/1123123.flv
 *    加密后 url :	http://www.tudou.com/flv/001/123/123/1123123.flv?key=2635516242f4eee5631b40487ace6e
 *
 * 2. 加密算法：
 *    value1= md5( password + base_url + int2hex（timestamp）)
 *    value2= int2hex（timestamp）
 *    value = value1 前22位 + int2hex（timestamp）(8位，例：487ae02c)
 *    例子：
 *    password = “whoamI”
 *    base_url = “/flv/001/123/123/1123123.flv”
 *    int2hex(timestamp)=487ace6e= int2hex(1216007785) ( 当前时间【以秒位单位‘1216007785’】的16进制表现形式)
 *
 *    clear text: ‘whoamI/flv/001/123/123/1123123.flv487ace6e’
 *    http://www.tudou.com/flv/001/123/123/1123123.flv?key=2635516242f4eee5631b40487ace6e
 *对CDN厂商的要求
 *	1. 双密码、时间偏移值和启用防盗链目录应该可配置
 *	|ChinaCacheTimestamp - TudouTimestamp| <时间间隔秒数
 *
 *	2. 动态防盗链功能可以通过配置文件开关
 *	3. 原来的根据网站引用URL防盗链功能应并存
 *	4. CDN服务器的时间应该正确配置，定期校对时间，ntp。 
 *	5. 只对防盗链目录内的文件进行动态防盗链，如果文件不在防盗链目录内，不能使用动态防盗链。 redirect_access
 *
 *	   当前，我们的防盗链目录是 /flv 和 /mp4 ，以后可能会增加其它目录。
 *  6. 防盗链流程：请看防盗链流程图.
 *
 *	   hex2int的C算法：
 *
 *	   time_t ts =o;
 *	   ts_str = 16进制的时间;
 *	   for (i = 0; i < 8; i++) {
 *	      ts = (ts << 4) + hex2int(*(ts_str + i));
 *
 * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "redirect_conf.h"
#include "md5.h"

#if 1
extern int g_fdDebug;
extern FILE *g_fpLog;
#define CRITICAL_ERROR(args, ...)	 fprintf(g_fpLog, args, ##__VA_ARGS__);fflush(g_fpLog);
#define NOTIFICATION(args,...)		 fprintf(g_fpLog, args, ##__VA_ARGS__);fflush(g_fpLog);
#define DEBUG(args, ...)			 if(g_fdDebug > 3) {fprintf(g_fpLog, args, ##__VA_ARGS__);fflush(g_fpLog);}
#else
#define CRITICAL_ERROR(args, ...)	 printf(args, ##__VA_ARGS__);fflush(stdout);
#define NOTIFICATION(args,...)		 printf(args, ##__VA_ARGS__);fflush(stdout);
#define DEBUG(args, ...)			 printf(args, ##__VA_ARGS__);fflush(stdout);

#endif

#define VALID_MD5_LEN		(22)
#define VALID_MD5_OFFSET	(0)
#define VALID_TIME_LEN      (8)
#define VALID_TIME_OFFSET   VALID_MD5_LEN
#define GET_MD5_VALUE(puc)	(char*)((puc) + VALID_MD5_OFFSET)
#define GET_TIME_VALUE(puc)	(char*)((puc) + VALID_MD5_LEN)

#define MAX_BASE_URL_LEN	(1024)
#define MAX_TIME_LEN	(VALID_TIME_LEN + 1)
#define MAX_MD5_LEN		(VALID_MD5_LEN + 1)


typedef struct tudou_cfg
{
    time_t  interval;   /* 过期时间 */
}ST_TUDOU_CFG;



/* 解析土豆配置行
 * 配置行：
 * www.tudou.com  none  tudou key1 key2 interval www.chinache.com
 * tudou前由上层解析
 * */
int analyseTudouCfg(struct redirect_conf *pstRedirectConf)
{
	static char *pcSep = " \t\r\n";
	char 	*pTemp;
	char	*paKey[2];
	int 	iT;

	ST_TUDOU_CFG	*pstC = NULL;
    pstRedirectConf->fail_dst = NULL;
	
	/* 解析key */
	for(iT=0; iT<sizeof(paKey)/sizeof(paKey[0]); iT++)
	{
		pTemp = xstrtok(NULL, pcSep);
		if(NULL == pTemp)
		{
			CRITICAL_ERROR("No password\n");
			return -1;
		}
		assert(strlen(pTemp)<=128);
#if 1
		if(0 == strcasecmp("null", pTemp))
#else
		if(0 == strcmp("null", pTemp))
#endif
		{
			paKey[iT] = NULL;
		}
		else
		{
			paKey[iT] = malloc(strlen(pTemp)+1);
			assert(NULL != paKey[iT]);
			strcpy(paKey[iT], pTemp);
		}
	}
	pstRedirectConf->key 	= paKey[0];
	pstRedirectConf->key2 	= paKey[1];
	
	pstC = (ST_TUDOU_CFG*)malloc(sizeof(ST_TUDOU_CFG));
	if(NULL == pstC)
	{
		CRITICAL_ERROR("No memory\n");
		return -1;
	}

	/* 解析 interval */
	pTemp = xstrtok(NULL, pcSep);
	if(NULL == pTemp)
	{
		CRITICAL_ERROR("No interval\n");
		goto error;
	}
	pstC->interval = atoi(pTemp);
	if(pstC->interval <= 0)
	{
		pstC->interval = 100;
		CRITICAL_ERROR("illegal interval\n");
	}


	/* 解析重定向链接 */
	pTemp = xstrtok(NULL, pcSep);
	if(NULL == pTemp)
	{
		CRITICAL_ERROR("No key redirect url\n");
		goto error;
	}
	pstRedirectConf->fail_dst = malloc(strlen(pTemp) + 1);
	if(NULL == pstRedirectConf->fail_dst)
    {
        CRITICAL_ERROR("No memory\n");
        goto error;
    }
	strcpy(pstRedirectConf->fail_dst, pTemp);

	pstRedirectConf->other = (void*)pstC;

	DEBUG("tudou config interval [%d] key1 [%s] key2 [%s]\n", (int)pstC->interval, paKey[0], paKey[1]);

	return 0;
error:
	if(NULL != pstC)
	{
		free(pstC);
	}
    if(NULL != pstRedirectConf->fail_dst)
    {
        free(pstRedirectConf->fail_dst);
    }
	return -1;
}


/* 将16进制字符转换成时间
 * 输入:
 * hex: 16进制时间
 * len: 长度
 * 输出:
 * 10进制数字
 */
time_t hex2time(char *hex, int len)
{
	int iValue = 0;
	iValue = strtol(hex, NULL, 16);
	return (time_t)iValue;
}

/* 检查时间是否合法
 * 输入:
 * timeoftudou:土豆的时间
 * interval:时间间隔
 * 单位: s
 * 输出:
 * 0 非法时间
 * 1 合法时间
 * */
int verifyTime(time_t timeoftudou, time_t interval)
{
	time_t now;
	int	ret = 0;

	now = time(NULL);
	
	DEBUG("Time of tudou [%ld], time of us [%ld], interval [%ld]\n", timeoftudou, now, interval);
	
	if((time_t)labs(timeoftudou - now) < interval)
	{
		ret = 1;
	}
	
	DEBUG("Time check of tudou result[%d]\n", ret);
	
	return ret;
}

/* 验证土豆的md5值
 * 输入:
 * passwd1,2:约定主备密码
 * baseurl:baseurl
 * timeoftudou:土豆的时间
 * orgmd5:  土豆的22位md5
 * */
int verifyTudouMd5(char *passwd1, char *passwd2, char *baseurl, char *timeoftudou, char *orgmd5)
{
#if 1
	MD5_CTX	M;
	static 	unsigned char digest[16];
	char 	*pKeyTable[2];
	int 	i;
	
	assert(NULL != baseurl);
	assert(NULL != timeoftudou);

	pKeyTable[0] = passwd1;
	pKeyTable[1] = passwd2;
	
	for(i=0; i<sizeof(pKeyTable)/sizeof(pKeyTable[0]); i++)
	{
		char *processKey = pKeyTable[i];
		if(NULL != processKey)
		{
			static	char buf[128];
			int 	count;
			MD5Init(&M);
			MD5Update(&M, (unsigned char*)processKey, strlen(processKey));
			MD5Update(&M, (unsigned char*)baseurl, strlen(baseurl));
			MD5Update(&M, (unsigned char*)timeoftudou, strlen(timeoftudou));
			MD5Final(digest, &M);
			for(count=0;count<16;count++)
			{
				sprintf(buf + count*2, "%02x", digest[count]);
			}

			DEBUG("new tudou md5 [%s]\n", buf);
			if(0 == strncasecmp(GET_MD5_VALUE(buf), orgmd5, VALID_MD5_LEN))
			{
				DEBUG("check tudou md5 <hit>\n");
				return 1;
			}
		}
	}
#endif
	return 0;
}

/* 检查加密串是否合法
 * 输入:
 *  md5: 22位md5值(16进制字符串)
 *  timeOfTudou: 8为16进制时间值(16进制字符串)
 * 输出:
 *  0   校验失败
 *  1   校验成功
 */
int isValidKeyOfTudou(const char *md5, const char *timeOfTudou)
{
#define IS_VALID_TUDOU_KEY(c)  ((c>='0' && c<='9') || (c>='a' && c<='f') || (c>='A' && c<='F')) ? 1 : 0
    int i;
    if(NULL == md5 || NULL == timeOfTudou)
    {
        CRITICAL_ERROR("NUll md5 of time of tudou\n");
        return 0;
    }

    if(VALID_MD5_LEN != strlen(md5) || VALID_TIME_LEN != strlen(timeOfTudou))
    {
        DEBUG("Invalid length of md5 [%lu] nor time [%lu]", (unsigned long)strlen(md5), (unsigned long)strlen(timeOfTudou));
        return 0;
    }

    for(i=0;i<strlen(md5);i++)
    {
        if(!IS_VALID_TUDOU_KEY(md5[i]))
        {
			DEBUG("tudou: md5 [%s] is invalid. Invalid [%c]\n",md5, md5[i]);
            return 0;
        }
    }

    for(i=0;i<strlen(timeOfTudou);i++)
    {
        if(!IS_VALID_TUDOU_KEY(timeOfTudou[i]))
        {
			DEBUG("tudou: time [%s] is invalid. Invalid [%c]\n", timeOfTudou, timeOfTudou[i]);
            return 0;
        }
    }

    return 1;
}

/* 删除url中的key字段并重组url
 * 输入:    url     http://www.tudou.com/flv/00/00/00/8172.flv?19191&skke=09112&key=ksksksksks0912&lslslsl=xxx
 * 输出:    url     去除key=xxx后的url链接
 *          baseurl 土豆规定的url
 *          md5     md5值
 *          timeoftudou 土豆的服务器时间
 * 返回值:  0   处理成功
 *          1   处理失败
 *          2	不需要认证
 * log:
 * 			增加对?后数字的判断，92001或者91001时，不需要认证
 */
int stripKeyOfTudou(char *url, char baseurl[], char md5[], char timeoftudou[])
{
    char    *ptmp;
    char    *pdomain;
    char    *pstartofbaseurl;
    int     lenOfBaseUrl;
    int     lenOfMd5AndTime;
    char    *pkey;
    char    *pmd5;
	static  char *specialCode[] = {"92001", "91001", "13001", "13002", "13003", 
				"14000", "20000", "20002", "20003", "20004", "20005", "20006", "80000"};
	char	code[32];
	char	*pcode;
	int 	i;
	
	if(NULL == url)
	{
		return 0;
	}
    
    /* 找到域名开始位置 */
    ptmp = strstr(url, "://");

    if(NULL == ptmp)
    {
        DEBUG("No domain in tudou url\n");
        return 0;
    }
    ptmp += strlen("://");
    pdomain = ptmp;

    /* 从domain开始寻找目录起始位置 */
    pstartofbaseurl = strstr(pdomain, "/");
    if(NULL == pstartofbaseurl)
    {
        DEBUG("No sub dir in tudou url\n");
        return 0;
    }

    /* 从sub dir 开始找到? */
    ptmp = strstr(pstartofbaseurl, "?");
    if(NULL == ptmp)
    {
        DEBUG("No ? in tudou url\n");
        return 0;
    }

    lenOfBaseUrl = ptmp - pstartofbaseurl;

    if(MAX_BASE_URL_LEN < lenOfBaseUrl)
    {
        CRITICAL_ERROR("base url is too large [%d]\n", lenOfBaseUrl);
        return 0;
    }

    strncpy(baseurl, pstartofbaseurl, lenOfBaseUrl);
    baseurl[lenOfBaseUrl] = '\0';
	
	/* 截断? */
	*ptmp = '\0';
	++ptmp;

	pcode = ptmp;
	while('\0'!=*ptmp && ' '!=*ptmp && '&'!=*ptmp && '\r'!=*ptmp && '\n'!=*ptmp && '\t'!=*ptmp)
	{
		ptmp++;
	}
	if(ptmp-pcode > 0 && ptmp-pcode < sizeof(code) -1 )
	{
		strncpy(code, pcode, ptmp-pcode);
		code[ptmp-pcode]='\0';
		DEBUG("Get code [%s]\n", code);
		for(i=0;i<sizeof(specialCode)/sizeof(specialCode[0]);i++)
		{
			if(0 == strcmp(code, specialCode[i]))
			{   
				DEBUG("Special code [%s]\n", specialCode[i]);
				return 2;
			}       
		}
	}
	ptmp = pcode;
    pkey = strstr(ptmp, "key=");
    if(NULL == pkey)
    {
        DEBUG("Can not find key in tudou url\n");
        return 0;
    }

    /* 解析md5 and time */
    ptmp = pmd5 = pkey + strlen("key=");

    while(' ' != *ptmp && '&' != *ptmp && '\0' != *ptmp 
      && '\r' != *ptmp && '\n' != *ptmp && '\t' != *ptmp)
    {
        ptmp++;
    }

    lenOfMd5AndTime = ptmp - pmd5;
    if(lenOfMd5AndTime != VALID_MD5_LEN + VALID_TIME_LEN)
    {
        DEBUG("Invalid key length [%d]\n", lenOfMd5AndTime);
        return 0;
    }

    strncpy(md5, GET_MD5_VALUE(pmd5), VALID_MD5_LEN);
    md5[VALID_MD5_LEN] = '\0';
    strncpy(timeoftudou, GET_TIME_VALUE(pmd5), VALID_TIME_LEN);
    timeoftudou[VALID_TIME_LEN] = '\0';

    DEBUG("Get md5 [%s] time [%s] from tudou url\n", md5, timeoftudou);

    DEBUG("Now strip md5 and value in url\n [%s]\n", url);

    /* 优先检查是否是字符串尾部 */
    if('\0' == *ptmp)
    {
        *(pkey - 1) = '\0';
    }
    else
    {
        /* url未结束前 */
        while('\0' != *ptmp)
        {
            ptmp++;
            *pkey = *ptmp;
            pkey++;
        }
        *pkey = '\0';
    }

    DEBUG("New url:\n[%s]\n", url);

    return 1;
}

/* 由于土豆的拖拽功能，导致在http头部添加了Range:字段
 * 在认证失败时导致squid崩溃，因此在失败处理时删除Range:字段
 * 输入：可能包含Range:字段的字符串
 * 输出: 删除Range的字符串*/
void stripTudouRange(char *other)
{
	char *pRange;
	char *ptmp;

	pRange = strstr(other, "Range:");
	if(NULL != pRange)
	{
		ptmp = pRange+strlen("Range:");
		/* 找到Range的结尾 */
		while(' '!=*ptmp && '\n'!=*ptmp && '\r'!=*ptmp && '\0'!=*ptmp && '\t'!=*ptmp)
		{
			ptmp++;
		}

		while('\0' != *ptmp && (' '==*ptmp || '\t'==*ptmp))
		{
			ptmp++;
		}

		if('\0'==*ptmp)
		{
			*pRange = '\0';
		}
		else
		{
			while('\0' != *ptmp)
			{
				*pRange = *ptmp;
				ptmp++;
				pRange++;
			}
			*pRange = '\0';
		}
		stripTudouRange(other);
	}
	
	return;
}

/* 检验土豆的url是否有效
 * 输入参数：
 *  pstRedirectConf: 配置信息
 *  url
 *  ip
 *  other 结构体struct Request中的三个成员
 * 输出:
 *  校验成功: 删除key=xxxx字段的url及其他部分
 *  校验失败: 
 *      加密串非法: 403:http://fail_url ip other
 *      时间无效:   405:http://fail_url ip other
 *      加密串不符: 403:http://fail_url ip other
 * 返回值:
 * 1  校验通过
 * 0  校验失败
 */
int verifyTudou(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other)
{
    static char baseurl[MAX_BASE_URL_LEN + 1];
    static char md5[MAX_MD5_LEN + 1];
    static char timeoftudou[MAX_TIME_LEN + 1];
    int    status = 404;
    ST_TUDOU_CFG	*pstC = pstRedirectConf->other;
	int		ret;

    assert(NULL != pstC);

	ret = stripKeyOfTudou(url, baseurl, md5, timeoftudou);

    if(0 == ret)
    {
		DEBUG("tudou: strip key failed\n");
        status = 403;
        goto failed;
    }
	else if(2 == ret)
	{
		DEBUG("tudou: all right with special code\n");
		goto succeed;
	}

    if(!isValidKeyOfTudou(md5, timeoftudou))
    {
		DEBUG("tudou: invalid key\n");
        status = 403;
        goto failed;
    }

    if(0 == verifyTime(hex2time(timeoftudou, strlen(timeoftudou)), pstC->interval))
    {
		DEBUG("tudou: invalid time\n");
        status = 405;
        goto failed;
    }

    if(0 == verifyTudouMd5(pstRedirectConf->key, pstRedirectConf->key2, baseurl, timeoftudou, md5))
    {
		DEBUG("tudou: invalid md5\n");
        status = 403;
        goto failed;
    }

succeed:
    DEBUG("OUTPUT:[%s %s %s]\n", url, ip, other);
    printf("%s %s %s", url, ip, other);
    fflush(stdout);
    return 1;
    
failed:
	stripTudouRange(other);
	DEBUG("VERFIY FAILED OUTPUT:[%d:%s %s %s]\n", status, url, ip, other);
    printf("%d:%s %s %s", status, url,/* pstRedirectConf->fail_dst,*/ ip, other);
    fflush(stdout);
	return 0;
}

#if 0
int main()
{
	char a[]= "a1c009781";
	int i;
	char url[][1024] = {"http://www.tudou.com/flv/uil1/992/slo1/1.flv?ksks=9892&key=09876543211234567890123456AFBC",
		"http://www.tudou.com/flv/uil1/992/slo1/1.flv?key=09876543211234567890123456AFBC&ksks=9892&ll1=s91",
		"http://www.tudou.com/flv/uil1/992/slo1/1.flv?key=09876543211234567890123456AFBC",
		"http://www.tudou.com/flv/uil1/992/slo1/1.flv?key=09876543211234567890123456AFBC&&ksks=9892&ll1=s91",
		"http://www.tudou.com/flv/uil1/992/slo1/1.flv",
		"http://www.tudou.com/flv/uil1/992/slo1/1.flv?",
		"http://www.tudou.com/flv/uil1/992/slo1/1.flv?key=09876543211234567890123456AFB&&kks3=091&lkks=091",
		"http://www.tudou.com/flv/uil1/992/slo1",
		"https://www.tudou.com/flv/uil1/992/slo1/1.flv?key=09876543211234567890123456AFBC&&ksks=9892&ll1=s91",
		"http:/www.tudou.com/flv/uil1/992/slo1/1.flv?key=09876543211234567890123456AFBC&&ksks=9892&ll1=s91",
		"ftp:://www.tudou.com/flv/uil1/992/slo1/1.flv?key=09876543211234567890123456AFBC&&ksks=9892&ll1=s91",
		NULL,
		""};
	char baseurl[1024];
	char md5[23];
	char time[11];

	for(i=0;i<sizeof(url)/sizeof(url[0]);i++)
	{
		stripKeyOfTudou(url[i], baseurl, md5, time);
	}
	

	printf("%ld\n", hex2time(a, strlen(a)));
	printf("<verfiy time>%d\n", verifyTime(hex2time(a, strlen(a)),86400));
	return 0;
}
#endif
