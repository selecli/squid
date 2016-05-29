#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <dlfcn.h>
#include <ctype.h>
#include <sys/utsname.h>
#include <time.h>
#include "redirect_conf.h"
//#include "md5.h"
#include <openssl/evp.h>
#define MAX_TYPE_NUM 2
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

int analysisBaiduXcodeCfg(struct redirect_conf *pstRedirectConf)
{
	char *pTemp;
	pTemp = xstrtok(NULL, " \t\r\n");
	if(NULL == pTemp)
	{
		CRITICAL_ERROR("Can not get key\n");
		return -1;
	}
	pstRedirectConf->key = strdup(pTemp);

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



	return 1;
}

#if 0
/* 认证加密字段 
 * 1  校验通过
 * 0  校验失败
 * 输入参数：
 * */
int verifyBaiduXcode(char *pvalue)
{
	if(NULL == gpf_GreenMP3_verify)
	{
		CRITICAL_ERROR("Not baidu xcode verify process!\n");
		return 0;
	}
	if(0 == gpf_fcrypt_hstr_2data(0, pvalue, strlen(pvalue)))
	{
		return 1;
	}
	return 0;
}
#endif


int char2int(char p)
{
	int k = 0;
	if(p >= 'a')
		k = (int)(p -87);
	else 
		k = (int)(p -48);

	return k;
}

unsigned char * hex2data(char *hex, unsigned char *buf,int *data_len)
{
//	char *hex = "0123456789abcdef";
	int i = 0 ;
	int k ,j;
	char *p = hex;

	while( i< *data_len)
	{

		k = char2int(p[i]) * 16;
		j = char2int(p[++i]);
		k += j;
		buf[(i+1)/2-1] = k;
		i++;
	}

	*data_len = i/2;

	return buf;
}

int baiduXcodeVerify(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other)
{
	char 	*fail_dst = NULL;
	char	*pcTmp;
	int i =0;
	char *key_word = "xcode=";
	char *key_type[MAX_TYPE_NUM] ={".mp3",".wma"} ;
	char crypt_word[2048];
	memset(crypt_word,0,100);

	pcTmp = strstr(url,key_word);
	if(!pcTmp)
	{
		DEBUG("have no ?xcode, return fail400\n");
		goto fail400;
	}
	else{
		pcTmp += strlen(key_word);
		while(isxdigit(*(pcTmp)))
		{
			crypt_word[i++] = *pcTmp;	
			pcTmp ++;
		}
	}
	if(!strcmp(pcTmp,key_type[0])||!strcmp(pcTmp,key_type[1])||(*pcTmp == '&') || !strlen(pcTmp))
	{
		DEBUG("pcTmp = [%d]%s\n",(int)strlen(pcTmp),pcTmp);
	}
	else
	{
		DEBUG("pcTmp = [%d]%s\n",(int)strlen(pcTmp),pcTmp);
		goto fail400;
	}
	DEBUG("crypt_word  = [%s] \n",crypt_word);
	DEBUG("pstRedirectConf->key = %s\n",pstRedirectConf->key);

	EVP_CIPHER_CTX ctx;
	int crypt_len = strlen(crypt_word);  //xcode的长度
	unsigned char code[100];
	int out_len_tail;
	unsigned char out_buf[30];
	int out_len = 0;
	int rc;

	EVP_CIPHER_CTX_init(&ctx);

	rc = EVP_DecryptInit_ex(&ctx,EVP_bf_ecb(), NULL, NULL, NULL);
	if(0 == rc)
	{
		DEBUG("error in evp_decryptinit_ex\n");
		goto fail401;
	}
	rc = EVP_CIPHER_CTX_set_key_length(&ctx, strlen(pstRedirectConf->key));
	rc = EVP_DecryptInit_ex(&ctx, NULL, NULL, (unsigned char *)pstRedirectConf->key, NULL);
	if(rc == 0) {
		DEBUG("error in EVP_EncryptInit_ex!\n");
		goto fail401;
	}
	hex2data(crypt_word,code,&crypt_len); //xcode长度变化了，成为code的长度了
	rc = EVP_DecryptUpdate(&ctx, out_buf, &out_len, code, crypt_len);
	if(rc == 0) {
		DEBUG("error in EVP_EncryptInit_ex!\n");
		goto fail401;
	}


	rc = EVP_DecryptFinal_ex(&ctx, out_buf+out_len, &out_len_tail);
	if(rc == 0) {
		DEBUG("error in EVP_EncryptFinal_ex!\n");
		goto fail401;
	}
	out_len += out_len_tail;

	out_buf[out_len] = 0;

	DEBUG("out_buf(%d->%d) : %s\n", crypt_len,out_len, out_buf);
	rc = EVP_CIPHER_CTX_cleanup(&ctx);
	if(rc == 0) {
		DEBUG("error in EVP_CIPHER_CTX_cleanup!\n");
		goto fail401;
	}


	char buf[100];
	unsigned char *p = out_buf;
	i = 0;
	while(isdigit(p[i]))
	{
		buf[i] = p[i];
		i ++;
	}
	int decypt_time = atoi(buf);
	int time_now = time(0);

	if(time_now > decypt_time)
	{
		DEBUG("timenow[%d] > decypt_time[%d]\n",time_now,decypt_time);
		goto fail401;
	}

	printf("%s %s %s", url, ip, other);
	DEBUG("OUTPUT: %s %s %s\n", url, ip, other);
	fflush(stdout);
	return 1;

fail400:

	fail_dst = pstRedirectConf->fail_dst;
	DEBUG("fail_dst %s\n",fail_dst);
	if(NULL == fail_dst)
	{
		fail_dst = "http://www.chinacache.com";
	    CRITICAL_ERROR("No redirect url, use default [%s]\n", fail_dst);
	}
	printf("400:%s %s %s", fail_dst, ip, other);
	fflush(stdout);
	DEBUG("VERIFY FAILED OUTPUT:400:%s %s %s\n", fail_dst, ip, other);
	return 0;

fail401:
	fail_dst = pstRedirectConf->fail_dst;
	DEBUG("fail_dst %s\n",fail_dst);
	if(NULL == fail_dst)
	{
		fail_dst = "http://www.chinacache.com";
	    CRITICAL_ERROR("No redirect url, use default [%s]\n", fail_dst);
	}
	printf("401:%s %s %s", fail_dst, ip, other);
	fflush(stdout);
	DEBUG("VERIFY FAILED OUTPUT:401:%s %s %s\n", fail_dst, ip, other);
	return 0;

}
