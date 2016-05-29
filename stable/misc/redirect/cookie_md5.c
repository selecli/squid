#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include "redirect_conf.h"
#include "md5.h"
#include "cookie_md5.h"

extern int g_fdDebug;
extern FILE *g_fpLog;
#define CRITICAL_ERROR(args, ...)	 fprintf(g_fpLog, args, ##__VA_ARGS__);fflush(g_fpLog);
#define NOTIFICATION(args,...)		 fprintf(g_fpLog, args, ##__VA_ARGS__);fflush(g_fpLog);
#define DEBUG(args, ...)			 if(g_fdDebug > 3) {fprintf(g_fpLog, args, ##__VA_ARGS__);fflush(g_fpLog);}

/* 本文件提供对cookie_md5认证方式的校验包含
 * 1 解析cookie中的md5认证字符串
 * 2 校验md5认证字符串
 *
 * 算法说明：
 * 1 过期时间为T
 * 2 密码为pass1和pass2
 *
 * server在生成cookie时将pass1或者pass2的密码，
 * 与当前13位当前time(NULL)时间除T得到的字符串
 * 拼接起来进行md5计算得到string，以约定的关键字key作为标识
 * 保存在cookie中key=string;
 *
 * FC在处理时，计算当前时间除T得到时间T1，使用pass1和pass2与
 * T1，T1-1，T1+1拼接后计算出6组md5值，只要string与这6组任意
 * 一个匹配，即校验通过
 * */


#define TOLOWER(x)   (((x) >=  'A' && (x) <= 'Z' ) ? (x) - 'A'+ 'a' : (x))

/* 解析配置行
 * 配置行：
 * ms.m.mop.com  none  cookie_md5 key1 key2 interval __CC_COOKIE www.chinache.com Referer: mop.com | xiaonei.com | none
 * cookie_md5前由上层解析
 * */
int analyseCookieMd5Cfg(struct redirect_conf *pstRedirectConf)
{
	static char *pcSep = " \t\r\n";
	char 	*pTemp;
	char	*paKey[2];
	int 	iT;
	int  	iReferCount;
	int		iReferLen;
	char 	*pStart;
	COOKIE_MD5_CFG_ST	*pstC = NULL;
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
		assert(strlen(pTemp)<=32);
		if(0 == strcasecmp("null", pTemp))
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
	
	pstC = (COOKIE_MD5_CFG_ST*)malloc(sizeof(COOKIE_MD5_CFG_ST));
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

	/* 解析key */
	pTemp = xstrtok(NULL, pcSep);
	if(NULL == pTemp)
	{
		CRITICAL_ERROR("No key string\n");
		goto error;
	}
	assert(strlen(pTemp) < sizeof(pstC->key));
	strcpy(pstC->key, pTemp);

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

	/* 解析Referer */
	pTemp = xstrtok(NULL, pcSep);
	if(NULL == pTemp)
	{
		DEBUG("No key referer\n");
		pstC->iReferCount = 0;
	}
	else
	{
		for(iT=0; iT<strlen(pTemp); iT++)
		{
			*(pTemp + iT) = TOLOWER(*(pTemp + iT));
		}
		pTemp += strlen("referer:");

		iReferCount = 0;
		pStart = pTemp;

		/* referer:www.cc.com|www.dd.com|www.ee.com */
		while('\0' != *pTemp
				&& '\r' != *pTemp
				&& '\n' != *pTemp
				&& '\t' != *pTemp
				&& ' ' != *pTemp)
		{
			if('|' == *pTemp)
			{
				iReferLen = pTemp - pStart;
				if(iReferLen > sizeof(pstC->Referer[iReferCount]))
				{
					CRITICAL_ERROR("Rreferer is too large\n");
					goto error;
				}

				strncpy(pstC->Referer[iReferCount], pStart, iReferLen);
				pStart = pTemp + 1;
				iReferCount++;
			}
			pTemp++;
		}

		/* 最后一个referer */
		if('\0' == *pTemp
				|| '\r' == *pTemp
				|| '\n' == *pTemp
				|| '\t' == *pTemp
				|| ' ' == *pTemp)
		{
			iReferLen = pTemp - pStart;

			if(iReferLen > sizeof(pstC->Referer[iReferCount]))
			{
				CRITICAL_ERROR("Rreferer is too large\n");
				goto error;
			}

			strncpy(pstC->Referer[iReferCount], pStart, iReferLen);
			iReferCount++;
		}

		pstC->iReferCount = iReferCount;

	}	
	pstRedirectConf->other = (void*)pstC;

	DEBUG("interval [%d] key1 [%s] key2 [%s] refer count [%d]\n", (int)pstC->interval, paKey[0], paKey[1], pstC->iReferCount);
    for(iReferCount = 0; iReferCount < pstC->iReferCount; iReferCount++)
    {
        DEBUG("Refer[%d] [%s] \n", iReferCount, pstC->Referer[iReferCount]);
    }

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

/* 返回值：
 * 1  校验通过
 * 0  校验失败
 * 输入参数：
 * char *cfgRefer: 配置文件中的refer字符串
 * char *orgRefer: 用户http request携带的refer
 */
int verifyReferer(char cfgRefer[][128], char *orgRefer, const int referCount)
{
	if(NULL == orgRefer)
	{
		return 0;
	}
	else
	{
		/* 改成小写 */
		int i;
		for(i=0; i< referCount; i++)
		{
            if(strstr(orgRefer, cfgRefer[i]))
            {
                return 1;
            }
		}
	}

	return 0;
}

/* 返回值：
 * 1  校验通过
 * 0  校验失败
 * 输入参数：
 * key1：	密码
 * key2：	备份密码
 * interval：过期时间
 * orgmd5：	cookie中的原始md5值
 */
int verifyMD5(char *key1, char *key2, time_t interval, const char *orgmd5)
{
	time_t	curtime;
	int 	multiple = 0;
	static	char buf[128];
	char 	*pKeyTable[2];
	int   	multipleTable[3];
	int 	i;
	MD5_CTX	M;
	static unsigned char digest[16];
	
	curtime = time(NULL);
    assert(interval > 0);
	multiple = curtime/interval;
	assert(multiple > 0);

	pKeyTable[0] = key1;
	pKeyTable[1] = key2;
	multipleTable[0] = multiple;
	multipleTable[1] = multiple + 1;
	multipleTable[2] = multiple - 1;
	
	for(i=0; i<sizeof(pKeyTable)/sizeof(pKeyTable[0]); i++)
	{
		char *processKey = pKeyTable[i];
		if(NULL != processKey)
		{
			int tj;
			for(tj=0;tj<sizeof(multipleTable)/sizeof(multipleTable[0]);tj++)
			{
				int temp = multipleTable[tj];
				int count;
				sprintf(buf, "%d", temp);
				MD5Init(&M);
				MD5Update(&M, (unsigned char*)processKey, strlen(processKey));
				MD5Update(&M, (unsigned char*)buf, strlen(buf));
				MD5Final(digest, &M);
				for(count=0;count<16;count++)
				{
					sprintf(buf+count* 2, "%02x", digest[count]);
				}

				DEBUG("new md5 [%s]\n", buf);

				if(0 == strncasecmp(buf, orgmd5, 32))
				{
					DEBUG("verifymd5 <hit>\n");
					return 1;
				}
			}
		}
	}

	return 0;
}

/* 返回值：
 * 1  校验通过
 * 0  校验失败
 * 输入参数：
 * pstRedirectConf: 配置信息
 * url
 * ip
 * other 结构体struct Request中的三个成员
 */
int CookieMd5Verfiy(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other)
{
	char	*pRefer = NULL;
	char	referBuf[256];
	int 	ret = 0;
	char	Md5[33];
	char 	*fail_dst;
	char	*pTmp;
	char	*key;
	char	*pvalue;
	static  char noRefer[] = "none";
	static  char emptyRefer[] = "null";
	COOKIE_MD5_CFG_ST	*pstC = NULL;

	pstC = (COOKIE_MD5_CFG_ST*)pstRedirectConf->other;

	assert(NULL != pstC);

	if(0 != pstC->iReferCount)
	{
		/* 解析referer */
		pTmp = strstr(other, "Referer:");
		if(NULL != pTmp)
		{
			int len;
			pTmp += strlen("Referer:");
			pRefer = pTmp;

			while(*pTmp != ' ' && *pTmp != '\t' 
					&& *pTmp != '\n' && *pTmp != '\r' 
					&& *pTmp != '\0')
			{
				pTmp++;
			}

			len = pTmp - pRefer;

			if(0 == len)
			{
				DEBUG("Empty referer\n");
				pRefer = emptyRefer;    /* referer字段为空，使用null表示 */
			}
			else
			{
				if(len < sizeof(referBuf) - 1)
				{
					strncpy(referBuf, pRefer, len);
					referBuf[len] = '\0';
					pRefer = referBuf;
					DEBUG("Get referer successfully: [%s]\n", pRefer);
				}
				else
				{
					DEBUG("Get referer failed for too large: [%s]\n", pRefer);
					goto fail;
				}
			}
		}
		else
		{
			pRefer = noRefer;
		}
	}
	/* 解析md5 */
	key = pstC->key;
	pTmp = strstr(other, key);
	if(NULL == pTmp)
	{
	    DEBUG("No key [%s] in cookie\n", pstC->key);
		goto fail;
	}

	/* key=md5 value */
	pvalue = pTmp = pTmp + strlen(key) + 1;
	while(';'!=*pTmp && '\r'!=*pTmp && '\n'!=*pTmp && '\t'!=*pTmp && ' '!=*pTmp && '\0'!=*pTmp)
	{
		pTmp++; 
	}
	if(NULL != pTmp)
	{
		if(32 != (pTmp - pvalue))
		{
		    DEBUG("MD5 [%s] format is wrong len [%ld]\n", pvalue, (long int)(pTmp - pvalue));
			Md5[0] = '\0';
            goto fail;
		}
		else
		{
			strncpy(Md5, pvalue, 32);
			Md5[32] = '\0';
		}
	}
	else
	{
		DEBUG("No MD5 Value after key [%s]\n", key);
		Md5[0] = '\0';
        goto fail;
	}

	if(0 != pstC->iReferCount)
	{
		ret = verifyReferer(pstC->Referer, pRefer, pstC->iReferCount);
		if(0 == ret)
		{
			DEBUG("verify referer failed Refer: [%s]\n", pRefer);
			goto fail;
		}
	}
	else
	{
		DEBUG("No need to verify refer\n");
	}

	ret = verifyMD5(pstRedirectConf->key, pstRedirectConf->key2, pstC->interval, Md5);
	if(0 == ret)
	{
		DEBUG("verify md5 failed md5: [%s]\n", Md5);
		goto fail;
	}

    /* request is allright, go through without any rewrite */
	printf("%s %s %s", url, ip, other);
	fflush(stdout);
	DEBUG("OUTPUT:%s %s %s", url, ip, other);
	return 1;
fail:
	fail_dst = pstRedirectConf->fail_dst;
	if(NULL == fail_dst)
	{
		fail_dst = "www.chinacache.com";
	}
	printf("%s %s %s", fail_dst, ip, other);
	fflush(stdout);
	DEBUG("OUTPUT:%s %s %s", fail_dst, ip, other);
	return 0;	
}
#if 0
int main()
{
	verifyCookieMD5(NULL, "123451", 991,"e41b906c17cb99ef83d64636b5797fe9"); 
	return -1;
}
#endif
