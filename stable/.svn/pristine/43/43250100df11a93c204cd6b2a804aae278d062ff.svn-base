/*
 * this program is written for microsoft redirect.
 * by chenqi/GSP.
 * Last Modified on 2012/03/17.
 */

#include <string.h>
#include <dlfcn.h>
#include <assert.h>
#include <dlfcn.h>
#include <sys/utsname.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include <setjmp.h>
#include <ctype.h>
#include "md5.h"
#include "redirect_conf.h"
#include "base64_urlencode.h"
#include "hmac_sha1.h"
#include "hmac_sha256.h"


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
#define MAX_MD5_SIZE 100
#define MAX_PATH_SIZE 1024
#define MAX_URL_SIZE 1024

#define BUF_SIZE_256 256
#define KEY_LINE_SIZE 1024

#define KEY_SIZE 256
#define KEY_NUM 10
char MD5_secureID[KEY_SIZE];

typedef struct hmac_key_st {
	int id;
	char value[KEY_SIZE];
} hmac_key_t;

typedef struct hmac_keys_st {
	int num;
	hmac_key_t keys[KEY_NUM];
} hmac_keys_t;

/* 
   domain none operation secureID dst_url, eg:
   microsoft.com none /usr/local/squid/etc/microsoft_redirect_hmac  http://www.chinacache.com
 * */

int analysisMicrosoftCfg(struct redirect_conf *pstRedirectConf)
{	
	char *wspace = " \t\r\n";
	char *p_secureID = NULL;
	hmac_keys_t *hmac_keys = NULL;

	p_secureID = xstrtok(NULL, wspace);
	if (NULL == p_secureID)
	{   
		DEBUG("Microsoft Redirect: configure line error for 'Microsoft'\n");
		goto err;
	}  

	if (access(p_secureID, F_OK) != 0)
	{       
		DEBUG("Microsoft Redirect: miss config file\n");
		goto err;
	}       

	if (access(p_secureID, R_OK) != 0)
	{       
		DEBUG("Microsoft Redirect: can not read file of HMAC key\n");
		goto err;
	}       

	FILE *fp = NULL; 
	if ((fp = fopen(p_secureID, "r")) == NULL)
	{       
		DEBUG("Microsoft Redirect: can not open file of HMAC key\n");
		goto err;
	}       

	hmac_keys = calloc(1, sizeof(hmac_keys_t));
	if (NULL == hmac_keys)
	{
		DEBUG("Microsoft Redirect: calloc() error\n");
		goto err;
	}
	hmac_keys->num = 0;

	// parse line
	char key_line[BUF_SIZE_256];
	while (fgets(key_line, KEY_LINE_SIZE, fp) != NULL)
	{
		if (hmac_keys->num >= KEY_NUM)
		{
			DEBUG("Microsoft Redirect: keys number too many\n");
			break;
		}
		if (NULL == strstr(key_line, "key name"))
			continue;
		if (NULL == strstr(key_line, "value"))
			continue;

		char *p_line = NULL;
		if (NULL == (p_line = strchr(key_line, '=')))
			continue;
		if (NULL == (p_line = strchr(p_line + 1, '\"')))
			continue;

		while (isspace(*++p_line)); 	    // skip space character
		char key_id = *p_line;
		if (!isdigit(key_id))
			continue;
		//int id = atoi(&key_id);
		int id = atoi(p_line);

		if (NULL == (p_line = strchr(p_line, '=')))
			continue;
		if (NULL == (p_line = strchr(p_line + 1, '\"')))
			continue;

		while (isspace(*++p_line));
		char *last = NULL;
		if (NULL == (last = strrchr(p_line, '\"')))
			continue;
		while(isspace(*--last));
		*++last = '\0';	// truncate

		hmac_keys->keys[hmac_keys->num].id = id;
		strncpy(hmac_keys->keys[hmac_keys->num].value, p_line, KEY_SIZE - 1);
		hmac_keys->keys[hmac_keys->num].value[KEY_SIZE - 1] = '\0';
		hmac_keys->num++;
	}

	pstRedirectConf->other = hmac_keys;

	char *fail_jump_url = NULL;
	fail_jump_url = xstrtok(NULL, wspace);
	if(NULL == fail_jump_url)
	{
		DEBUG("Microsoft Redirect: configure line error for 'Microsoft', field 'fail_jump_url'\n");
		goto err;
	}
	pstRedirectConf->fail_dst = strdup(fail_jump_url);
	if(NULL == pstRedirectConf->fail_dst)
	{
		CRITICAL_ERROR("Microsoft Redirect: pstRedirectConf->fail_dst is 'NULL'\n");
		goto err;
	}

	return 0;	

err:
	if(NULL != pstRedirectConf->fail_dst)
		free(pstRedirectConf->fail_dst);
	return -1;
}


int analysisMicrosoftCfg_MD5(struct redirect_conf *pstRedirectConf)
{
	char *wspace = " \t\r\n";
	char *p_secureID = NULL;
	p_secureID = xstrtok(NULL, wspace);
	if (NULL == p_secureID)
	{   
		DEBUG("Microsoft Redirect: configure line error for 'Microsoft'\n");
		goto err;
	}  

	strcpy(MD5_secureID, p_secureID);

	pstRedirectConf->other = (void*) MD5_secureID;

	char *fail_jump_url = NULL;
	fail_jump_url = xstrtok(NULL, wspace);
	if(NULL == fail_jump_url)
	{
		DEBUG("Microsoft Redirect: configure line error for 'Microsoft', field 'fail_jump_url'\n");
		goto err;
	}
	pstRedirectConf->fail_dst = strdup(fail_jump_url);
	if(NULL == pstRedirectConf->fail_dst)
	{
		CRITICAL_ERROR("Microsoft Redirect: pstRedirectConf->fail_dst is 'NULL'\n");
		goto err;
	}

	return 0;	

err:
	if(NULL != pstRedirectConf->fail_dst)
		free(pstRedirectConf->fail_dst);
	return -1;
}

// url encodeing is case-sensitive
static void percent_encoding_lwr_case(char *hashID)
{
	char *ptr = hashID;
	char *p, *c, *d;
	while (NULL != (p = strchr(ptr, '%')) && (ptr - hashID) < (strlen(hashID) - 2))
	{
		c = p + 1;
		d = p + 2;
		if (*c >= 'A' && *c <= 'Z') *c += 'a' - 'A';
		if (*d >= 'A' && *d <= 'Z') *d += 'a' - 'A';
		ptr += 3;
	}
}

jmp_buf env;
static void fill_request_info(const char *url, const char *member, char *info)
{
	char *field = NULL;
	char *field_end = NULL;
	if (NULL == (field = strstr(url, member)))
	{   
		DEBUG("field [%s] cannot find url failed\n", member);
		longjmp(env, -1);
	}   
	field += strlen(member);
	if (NULL == (field_end = strchr(field, '&')))
	{   
		strcpy(info, field);
	}   
	else
		strncpy(info, field, field_end - field);
}

int verifyMicrosoft(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other)
{
	char *pos1, *pos2;
	char *fail_dst = NULL;
	hmac_keys_t *hmac_keys = pstRedirectConf->other;

	// Relative Path
	char dst_url[MAX_URL_SIZE];
	memset(dst_url, 0, MAX_URL_SIZE);

	// Expiration Time
	char expire_time[BUF_SIZE_256];
	memset(expire_time, 0, BUF_SIZE_256);

	// HMAC Digest
	char hashID[BUF_SIZE_256];
	memset(hashID, 0, BUF_SIZE_256);

	// HMAC Algorithm
	// input
	char shastr[MAX_MD5_SIZE];
	memset(shastr, 0, MAX_MD5_SIZE);

	DEBUG("client URL: %s\n", url);
	pos1 = strstr(url, "://");
	if (NULL == pos1)
		goto fail403;

	pos1 = strchr(pos1 + 3, '/');
	if (NULL == pos1)
		goto fail403;

	pos2 = strchr(pos1, '?');
	if (NULL == pos2)
		goto fail403;	

	memcpy(dst_url, pos1, pos2 - pos1);	// dst_url stores RelativePath, the first character is '/'
	DEBUG("Relative Path: %s\n", dst_url);

	if (-1 == setjmp(env))
		goto fail403;
	char p2[4];
	memset(p2,0,4);
	char p3[4];
	memset(p3,0,4);
	fill_request_info(url,"P1=",expire_time);
	fill_request_info(url,"P2=",p2);
	fill_request_info(url,"P3=",p3);
	fill_request_info(url,"P4=",hashID);

	long expire = atol(expire_time);
	int id = atoi(p2);	    // HMAC key id
	int sha = atoi(p3);

	int i = 0;
	int index = -1;

	DEBUG("Key ID Number: %d\n", hmac_keys->num);
	for (i = 0; i < hmac_keys->num; ++i)
	{
		DEBUG("Key ID: %d\n", hmac_keys->keys[i].id);
		if (hmac_keys->keys[i].id == id)
		{
			index = i;
			break;
		}
	}
	if (-1 == index)
	{
		DEBUG("This key_id (%d) not matched any configured Key ID, give 403!\n", id);
		goto fail403;
	}

	DEBUG("Expiration Time: %s\n", expire_time);
	DEBUG("keyID = %d, keyValue = %s\n", id, hmac_keys->keys[index].value);
	DEBUG("Digest in url: %s\n", hashID);

	// decide this url whether expired
	time_t current_time = time(NULL);
	if (difftime(current_time, (time_t) expire) > 0)
	{
		DEBUG("this url has been expired, stop check it\n");
		goto fail403;
	}
	if (sha != 1 && sha != 2)
	{
		DEBUG("P3=%d, not equal 1 or 2\n",sha);
		goto fail403;
	}

	percent_encoding_lwr_case(hashID);
	strcpy(shastr, dst_url);
	strcat(shastr, expire_time);
	DEBUG("Input of HMAC: %s\n", shastr);

	/* 
	 *  ok, "shastr" is the input of hmac algorithm,
	 *  SHA1 or SHA256 depends on the value of "sha"
	 *  the output of hmac is "hmac".
	 */
	// output
	unsigned char hmac[MAX_MD5_SIZE];
	int hmac_len;
	if (sha == 1)
	{
		hmac_sha1((unsigned char*)hmac_keys->keys[index].value, strlen(hmac_keys->keys[index].value), (unsigned char*)shastr, strlen((char*)shastr), hmac);
		hmac_len = 20;	    // length of digest
	}
	else if (sha == 2)
	{
		hmac_sha2((unsigned char*)hmac_keys->keys[index].value, strlen(hmac_keys->keys[index].value), (unsigned char*)shastr, strlen((char*)shastr), hmac);
		hmac_len = 32;
	}
	else
		assert(0);	    // illegal value, this should not happen

	unsigned char *base64 = NULL;
	base64 = base64_encode(hmac, hmac_len);
	if (NULL == base64)
	{
		DEBUG("base64_encode failed\n");
		goto fail403;
	}

	//DEBUG("the output of base64 encode: %s\n", base64);

	unsigned char *Digest = NULL;
	Digest = urlencode(base64, strlen((char*)base64));

	// dont forget free heap
	free(base64);
	base64 = NULL;

	if (NULL == Digest)
	{
		DEBUG("url_encode failed\n");
		goto fail403;
	}

	DEBUG("the output of url_encode: %s\n", Digest);

	// at last, we just compare
	int ret = 1;
	ret = memcmp(hashID, Digest, strlen(hashID));
	if (ret)
	{
		DEBUG("NOT matched, reject\n");
		free(Digest);
		Digest = NULL;
		goto fail403;
	}

	DEBUG("matched, accept\n");
	printf("%s %s", url, other);
	fflush(stdout);
	// dont forget free heap
	free(Digest);
	Digest = NULL;
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
}

static void getMD5(unsigned char *mybuffer,int buf_len,char *digest)
{
	MD5_CTX context;
	unsigned char szdigtmp[16];
	bzero(digest,64);
	MD5Init (&context);
	MD5Update (&context, mybuffer, buf_len);
	MD5Final (szdigtmp, &context);
	sprintf(digest,"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",szdigtmp[0],szdigtmp[1],szdigtmp[2],szdigtmp[3],szdigtmp[4],szdigtmp[5],szdigtmp[6],szdigtmp[7],szdigtmp[8],szdigtmp[9],szdigtmp[10],szdigtmp[11],szdigtmp[12],szdigtmp[13],szdigtmp[14],szdigtmp[15]);
	return ;
}

// case 2967
int verifyMicrosoft_MD5(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other)
{
	char *pos1, *pos2;
	char *fail_dst = NULL;

	// Expiration Time
	char expire_time[BUF_SIZE_256];
	memset(expire_time, 0, BUF_SIZE_256);

	// MD5 Digest in url
	char hashID[BUF_SIZE_256];
	memset(hashID, 0, BUF_SIZE_256);

	// MD5 Digest output
	char md5[MAX_MD5_SIZE];
	memset(md5, 0, MAX_MD5_SIZE);

	// MD5 Algorithm input
	char md5str[MAX_MD5_SIZE];
	memset(md5str, 0, MAX_MD5_SIZE);

	DEBUG("client URL: %s\n", url);

	pos1 = strrchr(url, '?');
	if (NULL == pos1 || *++pos1 != 'e')
		goto fail403;

	pos2 = strrchr(url, '&');
	if (NULL == pos2 || *++pos2 != 'h')
		goto fail403;

	strncpy(expire_time, pos1 + 2, pos2 - pos1 - 3);
	strcpy(hashID, pos2 + 2);
	strcpy(md5str, pstRedirectConf->other);     // secureID
	strncat(md5str, url, pos2 - url - 1);

	// decide this url whether expired
	long expire = atol(expire_time);
	time_t current_time = time(NULL);
	if (difftime(current_time, (time_t) expire) > 0)
	{
		DEBUG("this url has been expired, stop check it\n");
		goto fail403;
	}
	else
		DEBUG("this url is still fresh\n");

	getMD5((unsigned char *)md5str, strlen(md5str), md5);
	DEBUG("MD5 digest in url is: %s\n", hashID);
	DEBUG("the input of MD5 is: %s\n", md5str);
	DEBUG("the output of MD5 is: %s\n", md5);

	// at last, we just compare
	int ret = 1;
	ret = memcmp(hashID, md5,strlen(hashID));
	if (ret)
	{
		DEBUG("NOT matched, reject\n");
		goto fail403;
	}

	DEBUG("matched, accept\n");
	printf("%s %s", url, other);
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
}



