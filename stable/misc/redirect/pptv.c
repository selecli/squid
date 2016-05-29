#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <dlfcn.h>
#include <sys/utsname.h>
#include <time.h>
#include <ctype.h> 
#include "redirect_conf.h"
#include "md5.h"
#include <sys/types.h>

#define MAX_KEY_SIZE 100
#define MAX_TIME_SIZE 100
#define MAX_MD5_SIZE 1024

typedef unsigned int u_int;

static const size_t ENCRYPT_ROUNDS = 32; // at least 32
static const u_int DELTA = 0x9E3779B9;
static const u_int FINAL_SUM = 0xC6EF3720;
static const size_t BLOCK_SIZE = (sizeof(unsigned int) << 1);
static const size_t BLOCK_SIZE_TWICE = ((sizeof(unsigned int) << 1) << 1);
static const size_t BLOCK_SIZE_HALF = ((sizeof(unsigned int) << 1) >> 1);

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

typedef struct pplive
{
    char ppkey1[MAX_KEY_SIZE];
    char ppkey2[MAX_KEY_SIZE];
    char old_key[MAX_KEY_SIZE];
    long  time_to_invalid;
}ST_PPTV_CFG;

typedef struct requestHeader
{
    char referer[MAX_KEY_SIZE];
    char user_agent[MAX_KEY_SIZE];
}ReqHrd;

typedef struct _KEY
{
    char k1[50];
    char k2[10];        // 4 fixed-characters
    char k3[20];
    char file[256];
    char type[50];
}KEY_IN_URL;

int analysisPPTVCfg(struct redirect_conf *pstRedirectConf)
{
	char *pTemp;
	char *pcSep=" \t\r\n";
	ST_PPTV_CFG *pstC = NULL;
    char time_count_as_min[MAX_TIME_SIZE];
	pstC = (ST_PPTV_CFG*) malloc(sizeof(ST_PPTV_CFG));
	if(NULL == pstC)
	{
		CRITICAL_ERROR("No memory\n");
		goto error;
	}
	
/*得到PPTV设定的密码*/
	pTemp = xstrtok(NULL, pcSep);
	if(NULL == pTemp)
	{
		DEBUG("pplive: configure file is error,lack of salt1");
		goto error;
	}
	DEBUG("pplive: pstC->ppkey1 is %s\n",pTemp);
	strcpy(pstC->ppkey1,pTemp);

	pTemp = xstrtok(NULL, pcSep);
	if(NULL == pTemp)
	{
		DEBUG("pplive: configure file is error,lack of salt2");
		goto error;
	}
	DEBUG("pplive: pstC->ppkey2 is %s\n",pTemp);
	strcpy(pstC->ppkey2,pTemp);
    
    /* got key and time for old_antihijack */
    /*得到PPTV设定的密码*/
    pTemp = xstrtok(NULL, pcSep);
    if (NULL == pTemp)
    {   
        DEBUG("configure file is error,lack of old_Key");
        goto error;
    }   
    DEBUG("pplive: pstC->old_key is %s\n",pTemp);
    strcpy(pstC->old_key,pTemp);
    /*得到PPTV设定的过期时间*/
    pTemp = xstrtok(NULL, pcSep);
    if(NULL == pTemp)
    {   
        DEBUG("configure file is error,lack of time to invalid");
        goto error;
    }   
    DEBUG("pplive: pstC->invalid_time is %s\n",pTemp);
    strcpy(time_count_as_min,pTemp);
    pstC->time_to_invalid = atol(time_count_as_min);
    /* got key and time for old_antihijack end*/

/*得到PPTV设定的错误返回地址*/
	pTemp = xstrtok(NULL, pcSep);
	DEBUG("pplive: ptemp = %s\n",pTemp);
	pstRedirectConf->fail_dst = malloc(strlen(pTemp) + 1);
	if(NULL == pstRedirectConf->fail_dst)
	{
		CRITICAL_ERROR("No memory\n");
		goto error;
	}
	strcpy(pstRedirectConf->fail_dst, pTemp);
	pstRedirectConf->other = (void*)pstC;

	return 0;

error:
	if(pstC)
	{
		free(pstC);
		pstC = NULL;
	}
    if(pstRedirectConf->fail_dst)
    {
        free(pstRedirectConf->fail_dst);
		pstRedirectConf->fail_dst = NULL;
    }
	return -1;
}



/* ======================================================================================== */
/* ======================================================================================== */
/* ======================================================================================== */
// new algorithim

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

/* 
 * success: return 1
 * failed:  return 0
 */
static int parse_key_from_url(char *url ,KEY_IN_URL *key)
{
    char *p1, *p2, *p3;

    // get filename, start with '/', end with '?'
    if (NULL == (p1 = strrchr(url, '/')) || NULL == (p2 = strrchr(url, '?')))
        return 0;
    strncpy(key->file, p1+1, p2-p1-1);

    if (NULL == (p1 = strstr(url, "k=")) || NULL == (p2 = strstr(url, "type=")))
        return 0;
    p1 += strlen("k=");
    p2 += strlen("type=");

    // get type, end with '&' or null
    if (NULL != (p3 = strchr(p2, '&')))
        strncpy(key->type, p2, p3-p2);
    else
        strcpy(key->type, p2);

    // get k1, end with '-'
    if (NULL == (p2 = strchr(p1, '-')))
        return 0;
    strncpy(key->k1, p1, p2-p1);

    // get k2, end with '-', (fixed 4 char)
    p1 = p2 + 1;
    if (NULL == (p2 = strchr(p1, '-')))
        return 0;
    int count = p2 - p1;
    if (count == 4)
        strncpy(key->k2, p1, 4);
    else if (count < 4)
    {
        memset(key->k2, 0, strlen(key->k2));        // write 0 to high bits if count < 4
        strncpy(key->k2+4-count, p1, p2-p1);
    }
    else
        return 0;

    // get k3, end with '&' or null
    p1 = p2 + 1;
    if (NULL != (p2 = strchr(p1, '&')))
        strncpy(key->k3, p1, p2-p1);
    else
        strcpy(key->k3, p1);

    // ok now
    return 1;
}

static void getRequireHttpHeader(char *buf, ReqHrd* hdr)
{
    char *p1, *p2;
    // get Header "Referer"
    if (NULL != (p1 = strstr(buf, "Referer:")))
    {
        if (NULL != (p2 = strchr(p1, '\t')))
        {
            p1 += strlen("Referer:");
            strncpy(hdr->referer, p1, p2-p1);
        }
    }
    else
    {
        DEBUG("Request has no Referer Header\n");
        strcpy(hdr->referer, "none");
    }

    // get Header "User-Agent"
    if (NULL != (p1 = strstr(buf, "User-Agent:")))
    {
        if (NULL != (p2 = strchr(p1, '\t')))
        {
            p1 += strlen("User-Agent:");
            strncpy(hdr->user_agent, p1, p2-p1);
        }
    }
    else
    {
        DEBUG("Request has no User-Agent Header\n");
        strcpy(hdr->user_agent, "none");
    }
}

static void filterIP(char *ip)
{
    char *tmp = ip + strlen(ip) - 2;
    if (0 == strncmp(tmp, "/-", 2))
        *tmp = '\0';
}

// parse file name
static char x2c(char *what) 
{
    char digit;
    digit = ((what[0] >= 'A') ? ((what[0] & 0xdf) - 'A')+10 : (what[0] - '0'));
    digit *= 16;
    digit += (what[1] >= 'A' ? ((what[1] & 0xdf) - 'A')+10 : (what[1] - '0'));
    return(digit);
}

static char* unescape(char *file)
{
    int i, j;
    for (i = 0; file[i]; i++)
        if(file[i] == '%')
            file[i+1] = x2c(&file[i+1]);

    for (i = 0, j = 0; file[j]; ++i, ++j) 
    {
        if((file[i] = file[j]) == '%') 
        {
            file[i] = file[j+1];
            j += 2;
        }
    }
    file[i] = '\0';
    DEBUG("And Video Name is %s\n", file);
    return file;
}


int pptv_antihijack_verify(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other)
{
	char *fail_dst = NULL;
    char status_code[5];
	ST_PPTV_CFG* pstc = pstRedirectConf->other;
	assert(pstc);

    // parse client IP
    char client_ip[30];
    strcpy(client_ip, ip);
    DEBUG("IP in url is %s\t", client_ip);
    filterIP(client_ip);
    DEBUG("And client IP is %s\n", client_ip);

    // parse httpHeader
    ReqHrd hdr;
    memset(&hdr,0,sizeof(ReqHrd));
    getRequireHttpHeader(other, &hdr);
    DEBUG("Request url is %s\n", url);
    DEBUG("Header info: Referer:%s\tUser-Agent:%s\n", hdr.referer, hdr.user_agent);

    // parse key in url
    KEY_IN_URL url_key;
    memset(&url_key,0,sizeof(KEY_IN_URL));
	if(!parse_key_from_url(url, &url_key))
    {
        DEBUG("pplive: reqeust %s is bad\n", url);
        strcpy(status_code,"400");      // Bad Request
        goto end;
    }
    DEBUG("Key info: k1-k2-k3: %s-%s-%s,\ttype=%s, \tvideo name is: %s\n", 
            url_key.k1, url_key.k2, url_key.k3, url_key.type, url_key.file);

    // calculate md5
    char md5str[MAX_MD5_SIZE];
    char md5[MAX_MD5_SIZE];
    memset(md5str, 0, MAX_MD5_SIZE);
    memset(md5, 0, MAX_MD5_SIZE);
    strcpy(md5str, pstc->ppkey2);
    strcat(md5str, url_key.k3);
    getMD5((unsigned char *)md5str, strlen(md5str), md5);

    // calculate flags
    char hexstr[10];
    memset(hexstr,0,10);
    strncpy(hexstr, md5, 4);
    char *p_stop;
    int flags;
    flags = (int)strtol(hexstr, &p_stop, 16);
    flags ^= (int)strtol(url_key.k2, &p_stop, 16);
    flags &= 0x3f;
    DEBUG("the flags of the bit map is: %d\n", flags);

    // compare key
    memset(md5str, 0, MAX_MD5_SIZE);
    memset(md5, 0, MAX_MD5_SIZE);
    strcpy(md5str, pstc->ppkey1);
    if (flags & 0x0020)
        strcat(md5str, client_ip);              // client ip
    if (flags & 0x0010)
        strcat(md5str, url_key.k3);       // expire time;
    if ((flags & 0x0008) && strcmp(hdr.referer, "none"))     
        strcat(md5str, hdr.referer);     // Referer header
    if ((flags & 0x0004) && strcmp(hdr.user_agent, "none"))
        strcat(md5str, hdr.user_agent);  // User-Agent hader
    if (flags & 0x0002)
        strcat(md5str, url_key.type);    // type
    if (flags & 0x0001)
        strcat(md5str, unescape(url_key.file));    // filename

    DEBUG("MD5 factor is: %s\n", md5str);
    getMD5((unsigned char *)md5str, strlen(md5str), md5);
    DEBUG("MD5 Digest is: %s\n", md5);
    if (strncmp(url_key.k1, md5, strlen(url_key.k1)))
    {
        DEBUG("Reqeust %s is forbidden\n", url);
        strcpy(status_code,"405");      // Not Allowed
        goto end;
    }

    // calculate time
    time_t now = time(NULL);
    long expire = atol(url_key.k3);
    time_t expire_t = (time_t) expire;
    if (difftime(now, expire_t) > 0.0)
    {
        DEBUG("Request %s time out!\n", url);
        strcpy(status_code,"407");      // Request Time Out
        goto end;
    }

    // OK
	printf("%s %s", url, other);
	fflush(stdout);
	DEBUG("pplive: VERIFY SUCCESS OUTPUT: %s %s %s\n", url, ip, other);
	return 1;

end:
    // NOT OK
	fail_dst = pstRedirectConf->fail_dst;
	if(NULL == fail_dst)
	{
		fail_dst = "http://www.chinacache.com";
	    CRITICAL_ERROR("No redirect url, use default [%s]\n", fail_dst);
	}
	printf("%s: %s %s", status_code, fail_dst, other);
	fflush(stdout);
	DEBUG("pplive: VERIFY FAILED OUTPUT: %s %s %s\n", status_code, fail_dst, other);
	return 0;
}

/* ======================================================================================== */
/* ======================================================================================== */
/* ======================================================================================== */
// old algorithim


unsigned int GetkeyFromstr(char* str, size_t len)
{
    size_t i=0;
    union tagkey
    {
        char ch[4];
        unsigned int key;
    }tmp_key;
    memset(&tmp_key,0,sizeof(tmp_key));

    for(i=0;i<len;i++)
    {
        tmp_key.ch[i%4] ^= str[i];
    }
    return tmp_key.key;
}

void TGetKey(const unsigned int *k0, unsigned int *k1, unsigned int *k2, unsigned int *k3 )
{
    *k1 = *k0<<8|*k0>>24;
    *k2 = *k0<<16|*k0>>16;
    *k3 = *k0<<24|*k0>>8;
}

unsigned int TDecrypt(char *hexstr, char* key)
{
    size_t i = 0;
    unsigned int k0 = GetkeyFromstr(key, strlen(key)), k1, k2, k3;
    unsigned int buf_size = 256;
    unsigned char buffer[256];
    unsigned int timet = 0;

    memset(buffer,0,buf_size);

    if (2*buf_size < strlen(hexstr))
        return 1;

    for (i = 0; i < strlen(hexstr)/2; i++)
    {
        buffer[i] = (hexstr[2*i]-(hexstr[2*i]>'9'?'a'-10:'0')) | ((hexstr[2*i+1]-(hexstr[2*i+1]>'9'?'a'-10:'0'))<<4);
    }

    TGetKey(&k0, &k1, &k2, &k3);
    for (i = 0; i + BLOCK_SIZE_TWICE <= buf_size; i += BLOCK_SIZE_TWICE)
    {
        unsigned int *v = (unsigned int*) (buffer + i);
        unsigned int v0 = v[0], v1 = v[1], sum = FINAL_SUM;
        size_t j;
        for (j = 0; j < ENCRYPT_ROUNDS; j++)
        {
            v1 -= ((v0<<4) + k2) ^ (v0 + sum) ^ ((v0>>5) + k3);
            v0 -= ((v1<<4) + k0) ^ (v1 + sum) ^ ((v1>>5) + k1);
            sum -= DELTA;
        }
        v[0] = v0; v[1] = v1;
    }

    for(i=0;i<buf_size && i < 8;i++)
    {
        timet |= (unsigned int)(buffer[i]-(buffer[i]>'9'?'a'-10:'0'))<<(28-i%8*4);
    }
    return timet;
}

static int get_key_from_url(char *url ,char *key)
{
    DEBUG("url=%s\n",url);
    char *p = strstr(url,"key=");
    if ( NULL == p )
    {
        DEBUG("url format is wrong in get_key_from_url\n");
        return 0;
    }
   // *(--p) = 0; //this p point to & or ?
    //this we change  the original url
    p += strlen("key=") ;
    if (NULL == p)
    {
        DEBUG("url format is wrong in get_key_from_url,has no value\n");
        return 0;
    }
    DEBUG("after url is %s\n",url);
    char *q = p;
    while(isalnum(*q)) q++;
    strncpy(key,p,q-p);
    return 1;
}

static int is_has_cc_header(char *other,char *key_ignore)
{
    return ( strstr(other,key_ignore) != NULL);
}

int pptv_antihijack_verify_old(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other)
{
    char *fail_dst = NULL;
    ST_PPTV_CFG* pstC = pstRedirectConf->other;
    char key[MAX_KEY_SIZE]="0";
    assert(pstC != NULL);

    DEBUG("ST_PPTV_CFG key = [%s]\n",pstC->old_key);
    DEBUG("ST_PPTV_CFG time = [%ld]\n",pstC->time_to_invalid);

    DEBUG("other = [%s]\n",other);
    char key_ignore[20] = "X-CC-Media-Prefetch";
    if (is_has_cc_header(other,key_ignore))
    {
        DEBUG("pass,has header X-CC-Media-Prefetch\n");
        goto end;

    }
    if(!get_key_from_url(url,key))
    {
        goto fail404;
    }
    DEBUG("get_key_from_url = [%s]\n",key);


    long time_caculated = 0;
    long time_now = 0;
    time_caculated = TDecrypt(key,pstC->old_key);
    time_now = (long)(time(0));

    DEBUG("time now is %ld \t, time from url is %ld\n",time_now,time_caculated );
    DEBUG("time now - time_caculated = %ld ",time_now - time_caculated );

    if ( abs(time_now - time_caculated) > pstC->time_to_invalid *60 )
    {
        DEBUG("their gap %ld > %ld (%ld minutes)\n",time_now-time_caculated ,pstC->time_to_invalid*60,pstC->time_to_invalid);
        goto fail404;
    }

end:
    printf("%s %s", url, other);
    fflush(stdout);
    DEBUG("VERIFY SUCCESS OUTPUT: %s %s %s\n", url, ip, other);
    return 1;

fail404:
    fail_dst = pstRedirectConf->fail_dst;
    DEBUG("fail_dst %s\n",fail_dst);
    if(NULL == fail_dst)
    {
        fail_dst = "http://www.chinacache.com";
        CRITICAL_ERROR("No redirect url, use default [%s]\n", fail_dst);
    }
    printf("403: %s %s", fail_dst, other);
    fflush(stdout);
    DEBUG("VERIFY FAILED OUTPUT:%s %s %s\n", fail_dst, ip, other);
    return 0;
}

