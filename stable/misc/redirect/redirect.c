#define _GNU_SOURCE
#include <assert.h>
#include <time.h>
#include <ctype.h>
#include <iconv.h>
#include <stdarg.h>
#include <errno.h>
#include "redirect.h"
#include "redirect_conf.h"
#include "md5.h"
#include "display.h"
#include "three_to_four.h"
#include "MyspaceAuthMusic.h"
//#include "pptv.c"

#define PROTOCOL_LENGTH 16
#define DOMAIN_LENGTH 128
#define MAX_URL_LENGTH 4096
#define FILENAME_LENGTH 4096
#define MD5_BUFFER_SIZE 4096

#define KEY_ID_LENGTH 256

#ifdef QQ_MUSIC 
static void qq_cookie_parse(char *cookie, int * qm_uin, char *qm_key, \
				char *qm_privatekey, int *qm_fromtag);
#endif
static int code_convert(char *from_charset,char *to_charset,char *inbuf,size_t inlen,char *outbuf,size_t outlen);
extern char* g_szDelimit;
extern int g_fdDebug;
extern FILE* g_fpLog;

/*分解的用户请求，目前实施并不分配内存，而是通过把请求的空格改为0、成员指向对应的位置来实施的*e
 */
struct Request
{
	char* url;
	char* ip;
	char* other;
};
/*请求的url各项*/
struct URL
{
	char protocol[PROTOCOL_LENGTH];
	char domain[DOMAIN_LENGTH];
	char filename[FILENAME_LENGTH];
	char* other;	//第一次分析url去域名时，指向剩余字符串的位置（为了性能）
};

static int deal_with_sina(const struct redirect_conf *pstRedirectConf, struct URL *pst_url);
static char * sina_file_get(const char *str);
static unsigned int twHash(const char* str,int size);

static struct URL g_stURL;	//存储url结构的临时变量
static struct Request g_stRequest;	//存储请求结构的临时变量
static char g_szBuffer[MD5_BUFFER_SIZE];	//临时变量，buffer
static char g_szDigest[32];		//临时变量，存储md5摘要

/* add by xt for 17173 */
struct hash_id {
	char **items;
	int offset;
	int size;
};
static char *ip_str_get(const long long int val);
static struct hash_id* cookie_handle(struct hash_id *h, const int sec, const char *cookie, int size) ;
static void hash_id_clean(struct hash_id *h) ;
static void hash_id_append(struct hash_id *h, const char *ip);
static struct hash_id * hash_id_create(const int size);

/* end */

static const char g_szDefaultWeb[] = "http://www.chinacache.com/antihijack/blankfaildst.jpg";

void log_output(int level, char * format, ...)                                          
{
	va_list list;

	if (g_fdDebug >= level) {
		va_start(list,format);
		vfprintf(g_fpLog, format, list);
		va_end(list);
//		fflush(g_fpLog);
	}   
}
void log_flush()
{
	if(g_fdDebug)
		fflush(g_fpLog);
}

/*分析请求行，按照空格分隔的规则拆为url、ip、和其它，结果指针指向输入的字段
 *输入：
 *    line----请求行（修改了）
 *输出：
 *    request----请求的三个部分
 *返回值：
 *    0----完成请求拆分
 *    -1----请求字段不够
*/
static inline int analysisRequest(char* line, struct Request* request)
{
	//第一个字段为url
	request->url = line;
	char* str = strchr(line, ' ');
	if(NULL == str)
	{
		return -1;
	}
	*str++ = 0;

	//第二个字段为ip
	request->ip = str;
	str = strchr(str, ' ');
	if(NULL == str)
	{
		return -1;
	}
	*str++ = 0;

	//第三个字段为用户名
	request->other = str;
	return 0;
}

/*从url得到请求的域名，并记录已处理字符串的位置（为了速度）
 *输入：
 *    url----url
 *输出：
 *    pstURL----包含了域名和位置指针
 *返回值：
 *    0----成功
 *    -1----url有问题
*/
static inline int getDomain(char* url, struct URL* pstURL)
{
	//得到协议类型
	char* str = strstr(url, "://");
	if(NULL == str)
	{
		return -1;
	}
	//检查长度
	if(str-url >= PROTOCOL_LENGTH)
	{
		return -1;
	}
	memcpy(pstURL->protocol, url, str-url);
	pstURL->protocol[str-url] = 0;

	//得到域名
	str += 3;
	char* str2 = strchr(str, '/');
	if(NULL == str2)
	{
		return -1;
	}
	if(str2-str >= DOMAIN_LENGTH)
	{
		return -1;
	}
	memcpy(pstURL->domain, str, str2-str);
	pstURL->domain[str2-str] = 0;
	
	//记录指针移动到的新的位置
	pstURL->other = str2+1;
	return 0;
}

/*把三到四编码的url进行解码，解码到后缀为止
 *输入：
 *    url----url编码部分的开始位置
 *输出：
 *    url----直接在上面进行解码，并且把后缀移到新的位置，并且结尾置0
 *返回值：
 *    0----成功
 *    -1----失败
*/
static inline int decodeURL(char* url)
{
	//找到后缀位置，因为编码里不包含.，所以可以用strchr，这也防止了类似.tar.z的情况
	char* str = strchr(url, '.');
	if(NULL == str)
	{
		str = url + strlen(url);
		//return -1;
	}
	int ret = multiFourToThree(url, str-url, url);
	if(ret <= 0)
	{
		return -1;
	}
	//移动未解码的后缀
	int length = strlen(str);
	memmove(url+ret, str, length);
	url[ret+length] = 0;
	return 0;
}
static int code_convert(char *from_charset,char *to_charset,char *inbuf,size_t inlen,char *outbuf,size_t outlen)
{
	iconv_t cd;
	char **pin = &inbuf;
	char **pout = &outbuf;

	cd = iconv_open(to_charset,from_charset);
	if (cd==0) return -1;
	memset(outbuf,0,outlen);
	if (iconv(cd,pin,&inlen,pout,&outlen)==-1)
	{
		log_output(3,"iconv: %s\n", strerror(errno));
		iconv_close(cd);
		return -1;
	}
	iconv_close(cd);
	return 0;
}
/*
 *返回值：
 *    0----成功
 *    -1----失败
*/
static char * u2g(char* url)
{
	char buf_in[MAX_URL_LENGTH] ; 
	char buf_out[MAX_URL_LENGTH]; 
	memset(buf_in,0,MAX_URL_LENGTH);
	memset(buf_out,0,MAX_URL_LENGTH);
	char *ptr;
	int ret ;
	strcpy(buf_in,url);
	log_output(6,"in u2g:\nbuf_in:%s\n",buf_in);
	ret = code_convert("utf-8","gb2312",buf_in,MAX_URL_LENGTH,buf_out,MAX_URL_LENGTH);
	if(-1 == ret) 
	{
		strcpy(buf_out,buf_in);
		ret = 0;
	}
	log_output(6,"buf_out:%s\n",buf_out);
	ptr = rfc1738_escape(buf_out);
	log_output(3,"url = %s\n",ptr);
	return ptr;
}
/*
 *返回值：
 *    0----成功
 *    -1----失败
*/
static inline int g2u(char* url)
{
	char buf_in[MAX_URL_LENGTH] = "0"; 
	char buf_out[MAX_URL_LENGTH*2] = "0"; 
	char *ptr;
	int ret = 0;
	strcpy(buf_in,url);
	log_output(6,"in g2u :buf_in:%s\n",buf_in);
	ret = code_convert("gb2312","utf-8",buf_in,MAX_URL_LENGTH,buf_out,MAX_URL_LENGTH*2);
	if(-1 == ret)
	{
		strcpy(buf_out,buf_in);
		ret = 0;
	}
	log_output(6,"buf_out:%s\n",url);
	ptr = rfc1738_escape(buf_out);
	strcpy(url,ptr);
	return ret;
}
/*检查url里的时间与当前的时间之差是否在控制范围内
 *输入：
 *    pstURL----url分解结构
 *    validPeriod----时间的控制范围
 *返回值：
 *    0----在控制范围内
 *    -1----不在控制范围内
*/
static inline int checkTime(const char* request_time, int seconds_before, int seconds_after)
{
	time_t now = time(NULL);
	char str[16];
	strcpy(str, request_time);
	struct tm stTm;
	stTm.tm_sec = 0;
	stTm.tm_min = atoi(str+10);
	str[10] = 0;
	stTm.tm_hour = atoi(str+8);
	str[8] = 0;
	stTm.tm_mday = atoi(str+6);
	str[6] = 0;
	stTm.tm_mon = atoi(str+4)-1;
	str[4] = 0;
	stTm.tm_year = atoi(str)-1900;
	time_t requestTime = mktime(&stTm);

	if((now>requestTime+seconds_before) || (now<requestTime-seconds_after))
	{
		return -1;
	}
	
	return 0;
}
/* add by xt  for ip filter range */
/* return 0, Range: bytes=s-e, s > 0
 * return 1, Range: bytes=0-e or do not find Range
 */
static int check_zero_range(struct Request *req)
{
	if(NULL == req) 
		return 1;
	if(NULL == req->other)
		return 1;
	const char *other = req->other;
	int range_offset = 0;
	char *pos = NULL;
	if(NULL == (pos = strcasestr(other, "bytes=")))
		return 1;
	pos += 6;
	if(NULL == pos) 
		return 1;
	range_offset = atoi(pos);
	if(range_offset > 0)
		return 0;
	else 
		return 1;
}
/* end */
int checkIpFilter(struct redirect_conf* pstRedirectConf, struct Request* pstRequest, struct URL* pstURL)
{
	int ret;
	char ip[16];
	char *str = NULL;
	int retval = -1;  /* add by xt */
	char *hash = NULL; /* add by xt */
	char *filename = NULL; /* add by xt */
	//得到ip
	str = strchr(pstRequest->ip, '/');
	if(NULL == str)
	{
		str = pstRequest->ip + strlen(pstRequest->ip);
	}
	memcpy(ip, pstRequest->ip, str-pstRequest->ip);
	ip[str-pstRequest->ip] = 0;

	//得到url中的ip
	str = strchr(pstURL->other, '/');
	if(NULL == str)
	{
		log_output(2,"no ip in url\n");
		return -1;
	}
	hash = str + 1;  /* get HASH/FILE add by xt */
	filename = strrchr(pstURL->other, '/');
	//比较两个ip
	if(strlen(ip) != str-pstURL->other)
	{
		log_output(2,"ip no match \n");
		goto error;
	}
	if(0 != memcmp(ip, pstURL->other, str-pstURL->other))
	{
		log_output(2,"ip no match 2\n");
		goto error;
	}

	pstURL->other = str + 1;
	str = strchr(pstURL->other, '/');
	if(NULL == str)
	{
		log_output(2,"no hash in url\n");
		goto error;
	}
	if(str-pstURL->other !=  pstRedirectConf->md5_length)
	{
		log_output(2,"hash in url is error\n");
		return -1;
	}
	int hash_checker = -1;
    char key_question = '?';
    char urlChecker[MD5_BUFFER_SIZE];
    char *p = strchr(str,key_question);
    strcpy(urlChecker,str);
    fprintf(g_fpLog,"urlChecker =[%s]",urlChecker);
    if (p != NULL)
    {
        log_output(2, "IN checkIpFilter we remove the ? to the end  to caculate md5sum\n");
        strncpy(urlChecker,str,p-str);
        *(urlChecker+(p-str)) = 0;
        log_output(3, "urlChecker = %s \n",urlChecker);

    }
	if(NULL != pstRedirectConf->key)
	{
		ret = snprintf(g_szBuffer, MD5_BUFFER_SIZE, "%s%s%s", pstRedirectConf->key, ip, urlChecker);
		log_output(3,"hash origin=[%s]\n", g_szBuffer);
		getMd5Digest32(g_szBuffer, ret, (unsigned char *)g_szDigest);
		log_output(3,"hash=[%.*s]\n", 32, g_szDigest);
		if(0 == strncasecmp(g_szDigest+pstRedirectConf->md5_start, pstURL->other, pstRedirectConf->md5_length))
		{
			hash_checker = 0;
		}
	}
	//如果第一个密码没有通过，检查第二个密码
	if((-1==hash_checker) && (NULL!=pstRedirectConf->key2))
	{
		ret = snprintf(g_szBuffer, MD5_BUFFER_SIZE, "%s%s%s", pstRedirectConf->key2, ip, urlChecker);
		log_output(3,"second hash origin=[%s]\n", g_szBuffer);

		getMd5Digest32(g_szBuffer, ret, (unsigned char *)g_szDigest);
		log_output(3,"second hash=[%.*s]\n", 32, g_szDigest);

		if(0 == strncasecmp(g_szDigest+pstRedirectConf->md5_start, pstURL->other, pstRedirectConf->md5_length))
		{
			hash_checker = 0;
		}
	}
	if(0 == hash_checker)
	{
		strcpy(pstURL->filename, str);
	}
	return hash_checker;
	/* add by xt */
error: 
	if(strcasestr(pstRequest->other, "Range")) {
		if(1 == pstRedirectConf->cookie_flag) {
			char *pos = strcasestr(pstRequest->other, "Cookie");
			char *start = NULL;
			int size = 0;
			char line[2048];
			struct hash_id *h = NULL;
			int i;
			int hash_checker2 = -1;

			if(NULL == pos)
				return -1;
			/* get cookie value */
			start = strchr(pos, ':');
			if(!start)
				return -1;
			start++;
			while(isspace(*start)) start++;
		
			while(*(start + size) != '\t' && *(start + size) != '\0') size++;
	
			memcpy(line, start, size);
			line[size] = ' ';
			line[size + 1] = '\0';
		
			h = hash_id_create(4);
		
			if(NULL == cookie_handle(h, pstRedirectConf->cookie_pass, line, size + 1))
				return -1;
			assert(hash);
			assert(filename);
			for(i = 0; i < h->offset; i++) {
					char *client_id_ip = h->items[i];
					if(NULL != pstRedirectConf->key)
					{
							ret = snprintf(g_szBuffer, MD5_BUFFER_SIZE, "%s%s%s", pstRedirectConf->key, client_id_ip, filename);
							log_output(3,"hash origin=[%s]\n", g_szBuffer);

							getMd5Digest32(g_szBuffer, ret, (unsigned char *)g_szDigest);
							log_output(3,"hash=[%.*s]\n", 32, g_szDigest);

							if(0 == strncasecmp(g_szDigest+pstRedirectConf->md5_start, hash, pstRedirectConf->md5_length))
							{
									hash_checker2 = 0;
							}
					}
					//如果第一个密码没有通过，检查第二个密码
					if((-1==hash_checker2) && (NULL!=pstRedirectConf->key2))
					{
							ret = snprintf(g_szBuffer, MD5_BUFFER_SIZE, "%s%s%s", pstRedirectConf->key2, client_id_ip, filename);
							log_output(3,"second hash origin=[%s]\n", g_szBuffer);

							getMd5Digest32(g_szBuffer, ret, (unsigned char *)g_szDigest);
							log_output(3,"second hash=[%.*s]\n",32,g_szDigest);

							if(0 == strncasecmp(g_szDigest+pstRedirectConf->md5_start, hash, pstRedirectConf->md5_length))
							{
									hash_checker2 = 0;
							}
					}
					if(0 == hash_checker2)
					{
							strcpy(pstURL->filename, filename);
							break;
					}
			}
			hash_id_clean(h);
			return hash_checker2;
		}
		else {
			/* begin to check range offset */
			if(1 == pstRedirectConf->range_flag) {
				if(!check_zero_range(pstRequest)) {
					if(NULL == filename) {
						char *pos = strrchr(pstURL->other, '/');
						filename = pos;
					}
					assert(filename);
					strcpy(pstURL->filename, filename);
					return 0;
				}
				else 
					return -1;
			}
		}
	}
	else {
		/* do not include Range Header */
		log_output(2,"can not find Range header\n");
		return -1;
	}
	return retval;
	/* end */
}

int checkIpAbortFilter(struct redirect_conf* pstRedirectConf, struct Request* pstRequest, struct URL* pstURL)
{
	int ret;
	int retval = -1;  /* add by xt */
	char *hash = NULL; /* add by xt */
	char *filename = NULL; /* add by xt */
	//得到ip
	char* str = strchr(pstRequest->ip, '/');
	if(NULL == str)
	{
		str = pstRequest->ip + strlen(pstRequest->ip);
	}
	char ip[16];
	memcpy(ip, pstRequest->ip, str-pstRequest->ip);
	ip[str-pstRequest->ip] = 0;

	//使得pstURL->other为hash，str为文件名
	str = strchr(pstURL->other, '/');
	if(NULL == str)
	{
		log_output(2,"no hash in url\n");
		return -1;
	}
	filename = str; /* add by xt */
	hash = pstURL->other; /* add by xt */
	if(str-pstURL->other !=  pstRedirectConf->md5_length)
	{
		log_output(2,"hash in url is error, md5 len %d\n", pstRedirectConf->md5_length);
		return -1;
	}
	int hash_checker = -1;
    
    char key_question = '?';
    char urlChecker[MD5_BUFFER_SIZE];
    char *p = strchr(str,key_question);
    strcpy(urlChecker,str);
    fprintf(g_fpLog,"urlChecker =[%s]",urlChecker);
    if (p != NULL)
    {
        log_output(2, "IN checkIpAbortFilter we remove the ? to the end  to caculate md5sum\n");
        strncpy(urlChecker,str,p-str);
        *(urlChecker+(p-str)) = 0;
        log_output(3, "urlChecker = %s \n",urlChecker);

    }
	if(NULL != pstRedirectConf->key)
	{
		/* 
		 * Original author Wang ShaoGang
		 * New author xiaotao
		 * Previously error hash src. 
		 */
		ret = snprintf(g_szBuffer, MD5_BUFFER_SIZE, "%s%s%s", pstRedirectConf->key, ip, urlChecker);
		log_output(3,"hash origin=[%s]\n", g_szBuffer);

		getMd5Digest32(g_szBuffer, ret, (unsigned char *)g_szDigest);
		log_output(3,"hash=[%.*s]\n", 32, g_szDigest);

		if(0 == strncasecmp(g_szDigest+pstRedirectConf->md5_start, pstURL->other, pstRedirectConf->md5_length))
		{
			hash_checker = 0;
		}
	}
	//如果第一个密码没有通过，检查第二个密码
	if((-1==hash_checker) && (NULL!=pstRedirectConf->key2))
	{
		ret = snprintf(g_szBuffer, MD5_BUFFER_SIZE, "%s%s%s", pstRedirectConf->key2, ip, urlChecker);
		log_output(3,"second hash origin=[%s]\n", g_szBuffer);
		getMd5Digest32(g_szBuffer, ret, (unsigned char *)g_szDigest);
		log_output(3,"second hash=[%.*s]\n", 32, g_szDigest);

		if(0 == strncasecmp(g_szDigest+pstRedirectConf->md5_start, pstURL->other, pstRedirectConf->md5_length))
		{
			hash_checker = 0;
		}
	}
	if(0 == hash_checker)
	{
		strcpy(pstURL->filename, str);
		return 0;
	}
	else 
		goto error;
error: 
	if(1 == pstRedirectConf->cookie_flag) {
		/* begin to check hash_id */
		char *pos = strcasestr(pstRequest->other, "Cookie");
		char *start = NULL;
		int size = 0;
		struct hash_id *h = NULL;
		int i;
		int hash_checker2 = -1;
		//char line[2048];
		char buffer[4097];
		char *line = NULL;
		size_t memsize = 0;

		if(NULL == pos)
			return -1;
		/* get cookie value */
		start = strchr(pos, ':');
		if(!start)
			return -1;
		start++;
		while(isspace(*start)) start++;
		
		while(*(start + size) != '\t'  && *(start + size) != '\0') size++;

		memsize = size > 4096 ? (size + 1) : 4097;
		line = memsize > 4097 ? malloc(memsize) : buffer;
		if (NULL == line)
		{
			line = buffer;
			size = 4096;
			memsize = 4097;
		}
		memset(line, 0, memsize);	
		memcpy(line, start, size);
		line[size] = ' ';
		line[size + 1] = '\0';
		
		h = hash_id_create(4);
		
		if(NULL == cookie_handle(h, pstRedirectConf->cookie_pass, line, size + 1))
		{
			if (size > 4096 && line != NULL)
			{
				free(line);
				line = NULL;
			}
			return -1;
		}

		assert(hash);
		assert(filename);
		for( i = 0; i < h->offset; i++) {
			char *client_id_ip = h->items[i];
			if(NULL != pstRedirectConf->key)
			{
				ret = snprintf(g_szBuffer, MD5_BUFFER_SIZE, "%s%s%s", pstRedirectConf->key, client_id_ip, filename);
				log_output(3,"hash origin=[%s]\n", g_szBuffer);

				getMd5Digest32(g_szBuffer, ret, (unsigned char *)g_szDigest);
				log_output(3,"hash=[%.*s]\n", 32, g_szDigest);

				if(0 == strncasecmp(g_szDigest+pstRedirectConf->md5_start, hash, pstRedirectConf->md5_length))
				{
					hash_checker2 = 0;
				}
			}
			//如果第一个密码没有通过，检查第二个密码
			if((-1==hash_checker2) && (NULL!=pstRedirectConf->key2))
			{
				ret = snprintf(g_szBuffer, MD5_BUFFER_SIZE, "%s%s%s", pstRedirectConf->key2, client_id_ip, filename);
				log_output(3,"second hash origin=[%s]\n", g_szBuffer);

				getMd5Digest32(g_szBuffer, ret, (unsigned char *)g_szDigest);
				log_output(3,"second hash=[%.*s]\n", 32, g_szDigest);

				if(0 == strncasecmp(g_szDigest+pstRedirectConf->md5_start, hash, pstRedirectConf->md5_length))
				{
					hash_checker2 = 0;
				}
			}
			if(0 == hash_checker2)
			{
				strcpy(pstURL->filename, filename);
				break;
			}
		}
		hash_id_clean(h);
		if (size > 4096 && line != NULL)
		{
			free(line);
			line = NULL;
		}
		return hash_checker2;
	}
	else {
		/* begin to check range offset */
		if(1 == pstRedirectConf->range_flag) {
			if(!check_zero_range(pstRequest)) {
				if(NULL == filename) {
					char *pos = strrchr(pstURL->other, '/');
					filename = pos;
				}
				assert(filename);
				strcpy(pstURL->filename, filename);
				return 0;
			}
			else 
			{
				return -1;
			}
		}
	}
	return retval;
	/* end */
}

/* add by xt for sina */
static int deal_with_sina(const struct redirect_conf *pstRedirectConf, struct URL *pstURL)
{
	char* str = strchr(pstURL->other, '/');
	if(NULL == str)
	{
		log_output(2,"no time in url\n");
		return -1;
	}
	if(str-pstURL->other != 12)
	{
		log_output(2,"time in url is error\n");
		return -1;
	}
	char requestTime[16];
	memcpy(requestTime, pstURL->other, 12);
	requestTime[12] = 0;
	struct valid_period* pstValidPeriod = (struct valid_period*)pstRedirectConf->other;
	int ret = checkTime(requestTime, pstValidPeriod->seconds_before, pstValidPeriod->seconds_after);
	if(-1 == ret)
	{
		log_output(2,"time in url is no in range\n");
		return -1;
	}

	/* get hash value */
	pstURL->other = str + 1;
	str = strchr(pstURL->other, '/');
	if(NULL == str)
	{
		log_output(2,"no hash in url\n");
		return -1;
	}
	if(str-pstURL->other !=  pstRedirectConf->md5_length)
	{
		log_output(2,"hash in url is error\n");
		return -1;
	}
	/* check hash value */
	int hash_checker = -1;
	if(NULL != pstRedirectConf->key)
	{
		ret = snprintf(g_szBuffer, MD5_BUFFER_SIZE, "%s%s%s", pstRedirectConf->key, requestTime, str);
		log_output(3,"hash origin=[%s]\n", g_szBuffer);

		getMd5Digest32(g_szBuffer, ret, (unsigned char *)g_szDigest);
		log_output(3,"hash=[%.*s]\n", 32, g_szDigest);

		if(0 == strncasecmp(g_szDigest+pstRedirectConf->md5_start, pstURL->other, pstRedirectConf->md5_length))
		{
			hash_checker = 0;
		}
	}

	/* check second key */
	if((-1==hash_checker) && (NULL!=pstRedirectConf->key2))
	{
		ret = snprintf(g_szBuffer, MD5_BUFFER_SIZE, "%s%s%s", pstRedirectConf->key2, requestTime, str);
		log_output(3,"second hash origin=[%s]\n", g_szBuffer);

		getMd5Digest32(g_szBuffer, ret, (unsigned char *)g_szDigest);
		log_output(3,"second hash=[%.*s]\n", 32, g_szDigest);

		if(0 == strncasecmp(g_szDigest+pstRedirectConf->md5_start, pstURL->other, pstRedirectConf->md5_length))
		{
			hash_checker = 0;
		}
	}
	if(0 == hash_checker)
	{
		/* add by xt */
		char *real_file_path = sina_file_get(str);
		strcpy(pstURL->filename, real_file_path);

		free(real_file_path);
	}
	return hash_checker;
}

static char * sina_file_get(const char *str) 
{
	assert(str);

	log_output(3,"[%s %d] sina_file_get: %s\n", __FILE__, __LINE__, str);

	if(*str == '/') 
		++str;

	char key[16]; /* key of video id, don't excess 16 bit. */
	int video_id;
	int len = 0;
	unsigned char id_md5[32];
	char *prev;
	char *next;
	char *retv = calloc(1, FILENAME_LENGTH);  /* remind to free */

	memset(id_md5, 0, 32);
	memset(key, 0, 16);

	video_id = atoi(str);

	len = snprintf(key, 16, "%d", video_id);

	getMd5Digest32(key, len, (unsigned char *)id_md5);

	id_md5[32] = '\0';

	int i;
	for(i = 0; i < 32; i++)  {
		if(isupper(id_md5[i]))
			id_md5[i] = tolower(id_md5[i]);
	}

	prev = strndup((char *)id_md5, 16);

	next = strdup((char *)id_md5 + 16);
	log_output(3,"[%s %d] sina_file_get: key %s, len %d, id md5 %s\n", __FILE__, __LINE__, key, len, id_md5);

	snprintf(retv, FILENAME_LENGTH, "/%u/%u/%s.flv", twHash(prev, 3000), twHash(next, 3000), id_md5);
	log_output(3,"[%s %d] sina_file_get: return %s\n", __FILE__, __LINE__, retv);

	free(prev);	
	free(next);

	return retv;
}

static unsigned int twHash(const char* str,int size)
{
	unsigned int n=0;
	int i;
	char* b=(char*)&n;
	int len = strlen(str);
	for (i=0;i<len;i++)
	{
		b[i%4]^=str[i];
	}

	return n%size;
}


int checkTime1Filter(struct redirect_conf* pstRedirectConf, struct URL* pstURL)
{
	//得到url中的时间
	char* str = strchr(pstURL->other, '/');
	//	char *key_start = "?start";
	char key_question = '?';
	if(NULL == str)
	{
		log_output(2,"no time in url\n");
		return -1;
	}
	if(str-pstURL->other != 12)
	{
		log_output(2,"time in url is error\n");
		return -1;
	}
	char requestTime[16];
	memcpy(requestTime, pstURL->other, 12);
	requestTime[12] = 0;
	struct valid_period* pstValidPeriod = (struct valid_period*)pstRedirectConf->other;
	int ret = checkTime(requestTime, pstValidPeriod->seconds_before, pstValidPeriod->seconds_after);
	if(-1 == ret)
	{
		log_output(2,"time in url is no in range\n");
		return -1;
	}

	//使得pstURL->other为hash，str为文件名
	pstURL->other = str + 1;
	str = strchr(pstURL->other, '/');
	if(NULL == str)
	{
		log_output(2,"no hash in url\n");
		return -1;
	}
	//case 3789 add by weiguo.chen
	char *str_rand = strchr(str+1, '/');
	char *p = strchr(str,key_question);
	char urlChecker[MD5_BUFFER_SIZE];
	//计算md5值时候，需要把？以后的部分去掉，否则防盗链会阻止这些请求。
	//case 3789 add by weiguo.chen
	//strcpy(urlChecker,str);
	if (NULL == str_rand)
	{
		log_output(2," no rand string\n");
		return -1;
	}
	strcpy(urlChecker,str_rand);
	if (p != NULL)
	{
		log_output(2,"IN checkTimeFilter we remove the ? to the end  to caculate md5sum\n");
		//strncpy(urlChecker,str,p-str);
		//*(urlChecker+(p-str)) = 0;
		//case 3789 add by weiguo.chen
		strncpy(urlChecker,str_rand,p-str_rand);
		*(urlChecker+(p-str_rand)) = 0;
	}
	if(str-pstURL->other !=  pstRedirectConf->md5_length)
	{
		log_output(2,"hash in url is error\n");
		return -1;
	}
	int hash_checker = -1;
	if(NULL != pstRedirectConf->key)
	{
		ret = snprintf(g_szBuffer, MD5_BUFFER_SIZE, "%s%s%s", pstRedirectConf->key, requestTime, urlChecker);
		log_output(3,"hash origin=[%s]\n", g_szBuffer);

		getMd5Digest32(g_szBuffer, ret, (unsigned char *)g_szDigest);
		log_output(3,"hash=[%.*s]\n", 32, g_szDigest);

		if(0 == strncasecmp(g_szDigest+pstRedirectConf->md5_start, pstURL->other, pstRedirectConf->md5_length))
		{
			hash_checker = 0;
		}
	}
	//如果第一个密码没有通过，检查第二个密码
	if((-1==hash_checker) && (NULL!=pstRedirectConf->key2))
	{
		ret = snprintf(g_szBuffer, MD5_BUFFER_SIZE, "%s%s%s", pstRedirectConf->key2, requestTime, urlChecker);
		log_output(3,"second hash origin=[%s]\n", g_szBuffer);

		getMd5Digest32(g_szBuffer, ret, (unsigned char *)g_szDigest);
		log_output(3,"second hash=[%.*s]\n", 32, g_szDigest);

		if(0 == strncasecmp(g_szDigest+pstRedirectConf->md5_start, pstURL->other, pstRedirectConf->md5_length))
		{
			hash_checker = 0;
		}
	}
	if(0 == hash_checker)
	{
		//strcpy(pstURL->filename, str);
		//case 3789 add by weiguo.chen
		strcpy(pstURL->filename, str_rand);
	}
	return hash_checker;
}


int checkTime2Filter(struct redirect_conf* pstRedirectConf, struct URL* pstURL)
{
	//得到url中的时间
	char* str = strchr(pstURL->other, '/');
	//	char *key_start = "?start";
	//char key_question = '?';
	if(NULL == str)
	{
		log_output(2,"no time in url\n");
		return -1;
	}
	if(str-pstURL->other != 12)
	{
		log_output(2,"time in url is error\n");
		return -1;
	}
	char requestTime[16];
	memcpy(requestTime, pstURL->other, 12);
	requestTime[12] = 0;
	struct valid_period* pstValidPeriod = (struct valid_period*)pstRedirectConf->other;
	int ret = checkTime(requestTime, pstValidPeriod->seconds_before, pstValidPeriod->seconds_after);
	if(-1 == ret)
	{
		log_output(2,"time in url is no in range\n");
		return -1;
	}

	//使得pstURL->other为hash，str为文件名
	pstURL->other = str + 1;
	str = strchr(pstURL->other, '/');
	if(NULL == str)
	{
		log_output(2,"no hash in url\n");
		return -1;
	}
	//char *p = strchr(str,key_question);
	char urlChecker[MD5_BUFFER_SIZE];
	//计算md5值时候，需要把？以后的部分去掉，否则防盗链会阻止这些请求。
	strcpy(urlChecker,str);

	//if (p != NULL)
	//{
	//	log_output(2,"IN checkTimeFilter we remove the ? to the end  to caculate md5sum\n");
	//	strncpy(urlChecker,str,p-str);
	//	*(urlChecker+(p-str)) = 0;
	//}
	if(str-pstURL->other !=  pstRedirectConf->md5_length)
	{
		log_output(2,"hash in url is error\n");
		return -1;
	}
	int hash_checker = -1;
	if(NULL != pstRedirectConf->key)
	{
		ret = snprintf(g_szBuffer, MD5_BUFFER_SIZE, "%s%s%s", pstRedirectConf->key, requestTime, urlChecker);
		log_output(3,"hash origin=[%s]\n", g_szBuffer);

		getMd5Digest32(g_szBuffer, ret, (unsigned char *)g_szDigest);
		log_output(3,"hash=[%.*s]\n", 32, g_szDigest);

		if(0 == strncasecmp(g_szDigest+pstRedirectConf->md5_start, pstURL->other, pstRedirectConf->md5_length))
		{
			hash_checker = 0;
		}
	}
	//如果第一个密码没有通过，检查第二个密码
	if((-1==hash_checker) && (NULL!=pstRedirectConf->key2))
	{
		ret = snprintf(g_szBuffer, MD5_BUFFER_SIZE, "%s%s%s", pstRedirectConf->key2, requestTime, urlChecker);
		log_output(3,"second hash origin=[%s]\n", g_szBuffer);

		getMd5Digest32(g_szBuffer, ret, (unsigned char *)g_szDigest);
		log_output(3,"second hash=[%.*s]\n", 32, g_szDigest);

		if(0 == strncasecmp(g_szDigest+pstRedirectConf->md5_start, pstURL->other, pstRedirectConf->md5_length))
		{
			hash_checker = 0;
		}
	}
	if(0 == hash_checker)
	{
		strcpy(pstURL->filename, str);
	}
	return hash_checker;
}

int checkTimeFilter(struct redirect_conf* pstRedirectConf, struct URL* pstURL)
{
	//得到url中的时间
	char* str = strchr(pstURL->other, '/');
	//	char *key_start = "?start";
	char key_question = '?';
	if(NULL == str)
	{
		log_output(2,"no time in url\n");
		return -1;
	}
	if(str-pstURL->other != 12)
	{
		log_output(2,"time in url is error\n");
		return -1;
	}
	char requestTime[16];
	memcpy(requestTime, pstURL->other, 12);
	requestTime[12] = 0;
	struct valid_period* pstValidPeriod = (struct valid_period*)pstRedirectConf->other;
	int ret = checkTime(requestTime, pstValidPeriod->seconds_before, pstValidPeriod->seconds_after);
	if(-1 == ret)
	{
		log_output(2,"time in url is no in range\n");
		return -1;
	}

	//使得pstURL->other为hash，str为文件名
	pstURL->other = str + 1;
	str = strchr(pstURL->other, '/');
	if(NULL == str)
	{
		log_output(2,"no hash in url\n");
		return -1;
	}
	char *p = strchr(str,key_question);
	char urlChecker[MD5_BUFFER_SIZE];
	//计算md5值时候，需要把？以后的部分去掉，否则防盗链会阻止这些请求。
	strcpy(urlChecker,str);

	if (p != NULL)
	{
		log_output(2,"IN checkTimeFilter we remove the ? to the end  to caculate md5sum\n");
		strncpy(urlChecker,str,p-str);
		*(urlChecker+(p-str)) = 0;
	}
	if(str-pstURL->other !=  pstRedirectConf->md5_length)
	{
		log_output(2,"hash in url is error\n");
		return -1;
	}
	int hash_checker = -1;
	if(NULL != pstRedirectConf->key)
	{
		ret = snprintf(g_szBuffer, MD5_BUFFER_SIZE, "%s%s%s", pstRedirectConf->key, requestTime, urlChecker);
		log_output(3,"hash origin=[%s]\n", g_szBuffer);

		getMd5Digest32(g_szBuffer, ret, (unsigned char *)g_szDigest);
		log_output(3,"hash=[%.*s]\n", 32, g_szDigest);

		if(0 == strncasecmp(g_szDigest+pstRedirectConf->md5_start, pstURL->other, pstRedirectConf->md5_length))
		{
			hash_checker = 0;
		}
	}
	//如果第一个密码没有通过，检查第二个密码
	if((-1==hash_checker) && (NULL!=pstRedirectConf->key2))
	{
		ret = snprintf(g_szBuffer, MD5_BUFFER_SIZE, "%s%s%s", pstRedirectConf->key2, requestTime, urlChecker);
		log_output(3,"second hash origin=[%s]\n", g_szBuffer);

		getMd5Digest32(g_szBuffer, ret, (unsigned char *)g_szDigest);
		log_output(3,"second hash=[%.*s]\n", 32, g_szDigest);

		if(0 == strncasecmp(g_szDigest+pstRedirectConf->md5_start, pstURL->other, pstRedirectConf->md5_length))
		{
			hash_checker = 0;
		}
	}
	if(0 == hash_checker)
	{
		strcpy(pstURL->filename, str);
	}
	return hash_checker;
}

int checkIpTimeFilter(struct redirect_conf* pstRedirectConf, struct Request* pstRequest, struct URL* pstURL)
{
	//得到ip
	char* str = strchr(pstRequest->ip, '/');
	if(NULL == str)
	{
		str = pstRequest->ip + strlen(pstRequest->ip);
	}
	char ip[16];
	memcpy(ip, pstRequest->ip, str-pstRequest->ip);
	ip[str-pstRequest->ip] = 0;

	//得到url中的ip
	str = strchr(pstURL->other, '/');
	if(NULL == str)
	{
		log_output(2,"no ip in url\n");
		return -1;
	}
	//比较两个ip
	if(strlen(ip) != str-pstURL->other)
	{
		log_output(2,"ip no match\n");
		return -1;
	}
	if(0 != memcmp(ip, pstURL->other, str-pstURL->other))
	{
		log_output(2,"ip no match\n");
		return -1;
	}

	//得到url中的时间
	pstURL->other = str+1;
	str = strchr(pstURL->other, '/');
	if(NULL == str)
	{
		log_output(2,"no time in url\n");
		return -1;
	}
	if(str-pstURL->other != 12)
	{
		log_output(2,"time in url is error\n");
		return -1;
	}
	char requestTime[16];
	memcpy(requestTime, pstURL->other, 12);
	requestTime[12] = 0;
	struct valid_period* pstValidPeriod = (struct valid_period*)pstRedirectConf->other;
	int ret = checkTime(requestTime, pstValidPeriod->seconds_before, pstValidPeriod->seconds_after);
	if(-1 == ret)
	{
		log_output(2,"time is no in range\n");
		return -1;
	}

	//使得pstURL->other为hash，str为文件名
	pstURL->other = str + 1;
	str = strchr(pstURL->other, '/');
	if(NULL == str)
	{
		log_output(2,"no hash in url\n");
		return -1;
	}
	if(str-pstURL->other !=  pstRedirectConf->md5_length)
	{
		log_output(2,"hash in url is error\n");
		return -1;
	}
	int hash_checker = -1;
	if(NULL != pstRedirectConf->key)
	{
		ret = snprintf(g_szBuffer, MD5_BUFFER_SIZE, "%s%s%s%s", pstRedirectConf->key, ip, requestTime, str);
		log_output(3,"hash origin=[%s]\n", g_szBuffer);

		getMd5Digest32(g_szBuffer, ret, (unsigned char *)g_szDigest);
		log_output(3,"hash=[%.*s]\n", 32, g_szDigest);

		if(0 == strncasecmp(g_szDigest+pstRedirectConf->md5_start, pstURL->other, pstRedirectConf->md5_length))
		{
			hash_checker = 0;
		}
	}
	//如果第一个密码没有通过，检查第二个密码
	if((-1==hash_checker) && (NULL!=pstRedirectConf->key2))
	{
		ret = snprintf(g_szBuffer, MD5_BUFFER_SIZE, "%s%s%s%s", pstRedirectConf->key2, ip, requestTime, str);
		log_output(3,"second hash origin=[%s]\n", g_szBuffer);

		getMd5Digest32(g_szBuffer, ret, (unsigned char *)g_szDigest);
		log_output(3,"second hash=[%.*s]\n", 32, g_szDigest);

		if(0 == strncasecmp(g_szDigest+pstRedirectConf->md5_start, pstURL->other, pstRedirectConf->md5_length))
		{
			hash_checker = 0;
		}
	}
	if(0 == hash_checker)
	{
		strcpy(pstURL->filename, str);
	}
	return hash_checker;
}


int checkKeyFilter(struct redirect_conf* pstRedirectConf, struct Request* pstRequest, struct URL* pstURL)
{
	char key_id[KEY_ID_LENGTH];
	char md5[KEY_ID_LENGTH];
	char urlChecker[MD5_BUFFER_SIZE];
	char key_question = '?';
	int hash_checker = -1;
	int ret = 0;

	log_output(3, "pstURL->other = %s\n",pstURL->other);

	char* str = strchr(pstURL->other, '/');
	if(NULL == str)
	{
		log_output(2, "no key in url\n");
		return -1;
	}
	memcpy(key_id, pstURL->other,str-pstURL->other);
	key_id[str-pstURL->other] = 0;

	log_output(3, "key_id =[%s]\n",key_id);

	//使得pstURL->other为hash，str为文件名
	pstURL->other = str + 1;
	log_output(3, "pstURL->other = [%s]\n",pstURL->other);
	str = strchr(pstURL->other, '/');
	if(NULL == str)
	{
		log_output(2, "no md5 in url\n");
		return -1;
	}
	while(isspace(*pstURL->other))
	{
		pstURL->other++;
	}
	memcpy(md5, pstURL->other,str-pstURL->other);
	md5[str-pstURL->other] = 0;

	log_output(3, "md5 =[%s]\n",md5);

	char *p = strchr(str,key_question);
	//计算md5值时候，需要把？以后的部分去掉，否则防盗链会阻止这些请求。
	strcpy(urlChecker,str);
	fprintf(g_fpLog,"urlChecker =[%s]",urlChecker);
	if (p != NULL)
	{
		log_output(2, "IN checkTimeFilter we remove the ? to the end  to caculate md5sum\n");
		strncpy(urlChecker,str,p-str);
		*(urlChecker+(p-str)) = 0;
		log_output(3, "urlChecker = %s \n",urlChecker);

	}
	if(NULL == str)
	{
		log_output(2, "no hash in url\n");
		return -1;
	}

	log_output(3, "str-pstURL->other = %d\n",str-pstURL->other);
	if(str-pstURL->other !=  pstRedirectConf->md5_length)
	{
		log_output(2, "hash in url is error\n");
		return -1;
	}
	if(NULL != pstRedirectConf->key)
	{
		ret = snprintf(g_szBuffer, MD5_BUFFER_SIZE, "%s%s%s", pstRedirectConf->key,key_id, urlChecker);
		log_output(3, "hash origin=[%s]\n", g_szBuffer);
		getMd5Digest32(g_szBuffer, ret, (unsigned char *)g_szDigest);
		log_output(3, "hash=[%.*s]\n", 32, g_szDigest);
		if(0 == strncasecmp(g_szDigest+pstRedirectConf->md5_start, pstURL->other, pstRedirectConf->md5_length))
		{
			hash_checker = 0;
		}
	}
	//如果第一个密码没有通过，检查第二个密码
	if((-1==hash_checker) && (NULL!=pstRedirectConf->key2))
	{
		ret = snprintf(g_szBuffer, MD5_BUFFER_SIZE, "%s%s%s", pstRedirectConf->key2, key_id, urlChecker);
		log_output(3, "second hash origin=[%s]\n", g_szBuffer);
		getMd5Digest32(g_szBuffer, ret, (unsigned char *)g_szDigest);
		log_output(3, "second hash=[%.*s]\n", 32, g_szDigest);
		if(0 == strncasecmp(g_szDigest+pstRedirectConf->md5_start, pstURL->other, pstRedirectConf->md5_length))
		{
			hash_checker = 0;
		}
	}
	if(0 == hash_checker)
	{
		strcpy(pstURL->filename, str);
	}
	return hash_checker;
}


#ifdef MYSPACE_CHECK
int checkMyspace(struct redirect_conf* pstRedirectConf, struct Request* pstRequest, struct URL* pstURL)
{
	assert(pstRequest);
	int retval = 200;
	char *url = pstRequest->url;
	char *filename = NULL;
	char *query = NULL;
	char other[4096] = "";
	char *tmp;

	if(!url) {
		log_output(2,"checkMyspace url is null\n");
		return -1;
	}

	filename = pstURL->other;
	/* gain file format */
	tmp = strrchr(url, '/');
	if(!tmp) {
		/* pass it, not check */
		snprintf(pstURL->filename, FILENAME_LENGTH - 1, "/%s", filename);
		return 0;
	}
	if(NULL == strcasestr(tmp+1, ".mp3")) {
		/* not check */
		snprintf(pstURL->filename, FILENAME_LENGTH - 1, "/%s", filename);
		return 0;
	}

	query = strchr(filename, '?');
	if(query == NULL) {
		log_output(2,"checkMyspace not find query\n");
		snprintf(pstURL->filename, FILENAME_LENGTH - 1, "/%s", filename);
	}
	if(query) {
		char *qline = strdup(query + 1); /* remaind to free */
		char *token = strtok(qline, "&"); 
		while(token) {
			if(!strncasecmp(token, "token", 5)) {
				char temp[1024];
				char url_temp[4096];
				snprintf(url_temp, 4095, "%s", url);
				char *new_token = getTokenForDC(url_temp, strlen(url_temp)); /* free ? */
				if(!new_token)  {
					log_output(2,"checkMyspace '%s' can not gain new token\n", url);
					return -1;
				}
				snprintf(temp, 1023, "token=%s", new_token);
				strcat(other, temp);
			}
			else 
				strcat(other, token);

			token = strtok(NULL, "&");
			if(token)
				strcat(other, "&");
		}
		free(qline);
		qline = NULL;

		log_output(2,"checkMyspace other {%s}\n", other);
	}

	retval = authenticMyspaceMusicRequest(url, strlen(url));
	if(retval == 200) {
		log_output(2,"checkMyspace filename %s, res %d\n", filename, !(200 == retval));
		*query = '\0';
		snprintf(pstURL->filename, FILENAME_LENGTH - 1, "/%s?%s", filename, other);
		return 0;
	}
	else  {
		log_output(2,"checkMyspace failed {%s}\n");
		return -1;
	}

	return !(200 == retval);
}
#endif

int checkIpTimeAbortFilter(struct redirect_conf* pstRedirectConf, struct Request* pstRequest, struct URL* pstURL)
{
	//得到ip
	char* str = strchr(pstRequest->ip, '/');
	if(NULL == str)
	{
		str = pstRequest->ip + strlen(pstRequest->ip);
	}
	char ip[16];
	memcpy(ip, pstRequest->ip, str-pstRequest->ip);
	ip[str-pstRequest->ip] = 0;

	//得到url中的时间
	str = strchr(pstURL->other, '/');
	if(NULL == str)
	{
		log_output(2,"no time in url\n");
		return -1;
	}
	if(str-pstURL->other != 12)
	{
		log_output(2,"time in url is error\n");
		return -1;
	}
	char requestTime[16];
	memcpy(requestTime, pstURL->other, 12);
	requestTime[12] = 0;
	struct valid_period* pstValidPeriod = (struct valid_period*)pstRedirectConf->other;
	int ret = checkTime(requestTime, pstValidPeriod->seconds_before, pstValidPeriod->seconds_after);
	if(-1 == ret)
	{
		log_output(2,"time is no in range\n");
		return -1;
	}

	//使得pstURL->other为hash，str为文件名
	pstURL->other = str + 1;
	str = strchr(pstURL->other, '/');
	if(NULL == str)
	{
		log_output(2,"no hash in url\n");
		return -1;
	}
	if(str-pstURL->other !=  pstRedirectConf->md5_length)
	{
		log_output(2,"hash in url is error\n");
		return -1;
	}
	int hash_checker = -1;
	if(NULL != pstRedirectConf->key)
	{
		ret = snprintf(g_szBuffer, MD5_BUFFER_SIZE, "%s%s%s%s", pstRedirectConf->key, ip, requestTime, str);
		log_output(3,"hash origin=[%s]\n", g_szBuffer);

		getMd5Digest32(g_szBuffer, ret, (unsigned char *)g_szDigest);
		log_output(3,"hash=[%.*s]\n", 32, g_szDigest);

		if(0 == strncasecmp(g_szDigest+pstRedirectConf->md5_start, pstURL->other, pstRedirectConf->md5_length))
		{
			hash_checker = 0;
		}
	}
	//如果第一个密码没有通过，检查第二个密码
	if((-1==hash_checker) && (NULL!=pstRedirectConf->key2))
	{
		ret = snprintf(g_szBuffer, MD5_BUFFER_SIZE, "%s%s%s%s", pstRedirectConf->key2, ip, requestTime, str);
		log_output(3,"second hash origin=[%s]\n", g_szBuffer);

		getMd5Digest32(g_szBuffer, ret, (unsigned char *)g_szDigest);
		log_output(3,"second hash=[%.*s]\n", 32, g_szDigest);

		if(0 == strncasecmp(g_szDigest+pstRedirectConf->md5_start, pstURL->other, pstRedirectConf->md5_length))
		{
			hash_checker = 0;
		}
	}
	if(0 == hash_checker)
	{
		strcpy(pstURL->filename, str);
	}
	return hash_checker;
}

static regmatch_t g_pmatch[REPLACE_REGEX_SUBSTR_NUMBER+1];

static void dealwithReplaceRegex(struct redirect_conf* pstRedirectConf, const char* line)
{
	char buffer[4096];
	char ch = 0;
	char* limit = strstr(g_stURL.other, g_szDelimit);
	if(limit)
	{
		ch = *limit;
		*limit = 0;
	}
	snprintf(buffer, 4096, "%s://%s/%s", g_stURL.protocol, g_stURL.domain, g_stURL.other);
	struct replace_regex* replace = (struct replace_regex*)pstRedirectConf->other;
	int ret = regexec(&replace->reg, buffer, REPLACE_REGEX_SUBSTR_NUMBER+1, g_pmatch, 0);
	if(0 == ret)
	{
		printf("%s", replace->replace[0]);
		log_output(1,"output:%s", replace->replace[0]);

		int i = 1;
		while(0 != (ret=(int)((long)replace->replace[i])))
		{
			printf("%.*s", g_pmatch[ret].rm_eo-g_pmatch[ret].rm_so, 
					buffer+g_pmatch[ret].rm_so);
			log_output(1,"%.*s", g_pmatch[ret].rm_eo-g_pmatch[ret].rm_so,buffer+g_pmatch[ret].rm_so);
			i++;
			if(replace->replace[i])
			{
				printf("%s", replace->replace[i]);
				log_output(1,"%s", replace->replace[i]);
				i++;
			}
			else
			{
				break;
			}
		}
		if(limit)
		{
			*limit = ch;
			printf("%s", limit);
			log_output(1,"%s", limit);
		}
		printf(" %s %s", g_stRequest.ip, g_stRequest.other);
		log_output(1," %s %s", g_stRequest.ip, g_stRequest.other);
	}
	else
	{
		printf("%s", line);
		log_output(1,"output:%s", line);
	}
	fflush(stdout);
	log_flush();
}
#ifdef QQ_MUSIC
/* modified by zhangxiaoyang for 128K mp3 of tencent.
 * The filenames of qqmusic consist of eight digits and filetype
 * Exg. 12000000.wma
 * size of qq_type should equal size of qq_type_name plus 1 */
static char* qq_type[] = {"64kdrm", "64k", "mp3", "30s", "videodrm"};
static int qq_type_name[] = {12000000, 30000000, 50000000, 60000000};
static void qq_filename_class(const char* filename)
{
	int file = atoi(filename);
	char* type = NULL;
	int i;
	int len = sizeof(qq_type_name)/sizeof(int);

	assert(len + 1== sizeof(qq_type)/sizeof(char*));

	for(i=0; i < len; i++)
	{
		if(file < qq_type_name[i])
		{
			type = qq_type[i];
			break;
		}
	}
	if(NULL == type)
	{
		type = qq_type[len];
	}
#if 0	
	if(file < 12000000)
	{
		type = qq_type[0];
	}
	else if(file < 50000000)
	{
		type = qq_type[1];
	}
	else if(file < 60000000)
	{
		type = qq_type[2];
	}
	else
	{
		type = qq_type[3];
	}
#endif
	int subdir = (file%1000)/10;
	printf("/%s/%d/%s", type, subdir, filename);
	log_output(1,"/%s/%d/%s", type, subdir, filename);
	return;
}

/* check this request is in white paper */
static inline int isWhiteQQ(struct redirect_conf* pstRedirectConf)
{
	if(NULL == pstRedirectConf->other)
	{
		log_output(3,"have no qq while conf\n");
		return -1;
	}
	char** pstr = (char**)pstRedirectConf->other;
	char* str = strstr(g_stRequest.other, "Referer:");
	if(NULL == str)
	{
		log_output(3,"no Referer\n");
		while(*pstr)
		{
			if(1 == (*pstr)[0])
			{
				return 0;
			}
			pstr++;
		}
		return -1;
	}
	str += 8;
	while(' ' == *str)
	{
		str++;
	}
	char* str2 = strchr(str, '\t');
	if(NULL == str2)
	{
		str2 = strchr(str, '\r');
		if(NULL == str2)
		{
			log_output(1,"no normal delimit(\\t or \\r) after Referer\n");
			return -1;
		}
	}
	if(str == str2)
	{
		log_output(3,"Referer is blank\n");
		/* comment by xt 
		 * if Refer is blank, the request need to check again with qq lib.
		 while(*pstr)
		 {
		 if(2 == (*pstr)[0])
		 {
		 return 0;
		 }
		 pstr++;
		 }
		 */
		return -1;
	}
	if(str2-str >= MD5_BUFFER_SIZE)
	{
		log_output(1,"Referer string is too long, lenhth=[%d]\n", str2-str);
		return -1;
	}
	memcpy(g_szBuffer, str, str2-str);
	g_szBuffer[str2-str] = 0;
	int len2 = str2-str;
	int len;
	while(*pstr)
	{
		len = strlen(*pstr);
		if(len2 >= len)
		{
			if(0 == strcasecmp(g_szBuffer+len2-len, *pstr))
			{
				return 0;
			}
		}
		pstr++;
	}
	log_output(3,"Referer=[%s] is not in white\n", g_szBuffer);
	return -1;
}
#endif
#ifdef QQ_MUSIC
/* modified by zhangxiaoyang for qq music verify lib 2008-06-02 */
/*************************************************
Function:       qmusic_verify
Description:   检查用户请求是否合法
Input:			const int qqmusic_uin			:  cookie名称为"qqmusic_uin="的整形数
const char * qqmusic_key		:  cookie名称为"qqmusic_key="的字符串
const char * qqmusic_privatekey	:  cookie名称为"qqmusic_privatekey="的字符串
const int qqmusic_fromtag		:  cookie名称为"qqmusic_fromtag="的整形数
const char * qqmusic_sosokey	:  cookie名称为"qqmusic_sosokey="的字符串
const char * qqmusic_guidi  	:  cookie名称为"qqmusic_guid="的字符串
const char * qqmusic_gkey   	:  cookie名称为"qqmusic_gkey="的字符串
Output:         
Return:         int  :  0 为校验合法 ,  <0 为校验不合法
Others:          
 *************************************************/
extern int qmusic_verify(const int qqmusic_uin, 
		const char * qqmusic_key, 
		const char * qqmusic_privatekey, 
		const int qqmusic_fromtag,
		const char * qqmusic_sosokey,
		const char * qqmusic_guid,
		const char * qqmusic_gkey
		);

static void qq_cookie_parse(char *cookie, int * qm_uin, char *qm_key, \
		char *qm_privatekey, int *qm_fromtag)
{
	int uin = -1;
	int fromtag = -1;
	char key[1024] ={0};
	char privatekey[1024] = {0};
	char *str;
	char *str2;
	char buf[4096];

	if(!cookie)
		return;
	if(strlen(cookie) > 4095)
		return;
	strncpy(buf, cookie, 4096);
	str = strtok(buf, ";");
	while(str)
	{
		str2 = strchr(str, '=');
		if(str2)
		{
			*str2 = 0;
			while(' ' == *str)
			{
				str++;
			}
			if(0 == strcmp(str, "qqmusic_uin"))
			{
				if (-1 == uin)
					uin = atoi(str2+1);
				log_output(3,"qqmusic_uin = [%d]\n", uin);
			}
			else if(0 == strcmp(str, "qqmusic_key"))
			{
				if (0 == key[0])
					memcpy(key, str2+1, 1024);

				log_output(3,"qqmusic_key = [%s]\n", key);
			}
			else if(0 == strcmp(str, "qqmusic_privatekey"))
			{
				if (0 == privatekey[0])
					memcpy(privatekey, str2 + 1, 1024);

				log_output(3,"qqmusic_privatekey = [%s]\n", privatekey);
			}
			else if(0 == strcmp(str, "qqmusic_fromtag"))
			{
				if (-1 == fromtag)
					fromtag = atoi(str2+1);

				log_output(3,"qqmusic_fromtag = [%d]\n", fromtag);
			}
		}
		str = strtok(NULL, ";");
	}

	if(qm_uin)
		*qm_uin = uin;
	if(qm_fromtag)
		*qm_fromtag = fromtag;
	if(qm_key)
		memcpy(qm_key, key, 1024);
	if(qm_privatekey)
		memcpy(qm_privatekey, privatekey, 1024);
}

static int qq_verify(int *fromtag)
{
	char* str = strstr(g_stRequest.other, "Cookie:");
	int cookie_flag = 0;  /* add by xt */

	if(NULL == str)
	{
		log_output(1,"no cookie in header\n");
		return -1;
	}
	str += 7;
	while(' ' == *str)
	{
		str++;
	}
	char* str2 = strchr(str, '\t');
	if(NULL == str2)
	{
		str2 = strchr(str, '\r');
		if(NULL == str2)
		{
			log_output(1,"no normal delimit(\\t or \\r) after Cookie\n");
			return -1;
		}
	}
	if(str2-str >= MD5_BUFFER_SIZE)
	{
		log_output(1,"cookie string is too long, lenhth=[%d]\n", str2-str);
		return -1;
	}
	memcpy(g_szBuffer, str, str2-str);
	g_szBuffer[str2-str] = 0;

	int qqmusic_uin = -1; 
	char* qqmusic_key = NULL;
	char* qqmusic_privatekey = NULL;
	int qqmusic_fromtag = -1;
	char default_string[] = "";
	char* qqmusic_sosokey = NULL;	/* added by zhangxiaoyang for qq verifylib 2008-06-02 */
	char* qqmusic_guid    = NULL;   /* added by zhoudshu for qq verifylib 2009-09-11 */
	char* qqmusic_gkey    = NULL;   /* added by zhoudshu for qq verifylib 2009-09-11 */

	str = strtok(g_szBuffer, ";");
	while(str)
	{
		str2 = strchr(str, '=');
		if(str2)
		{
			*str2 = 0;
			while(' ' == *str)
			{
				str++;
			}
			if(0 == strcmp(str, "qqmusic_uin"))
			{
				if (-1 == qqmusic_uin)
					qqmusic_uin = atoi(str2+1);
				log_output(3,"qqmusic_uin = [%d]\n", qqmusic_uin);
			}
			else if(0 == strcmp(str, "qqmusic_key"))
			{
				if (NULL == qqmusic_key)
					qqmusic_key = str2+1;
				log_output(3,"qqmusic_key = [%s]\n", qqmusic_key);
			}
			else if(0 == strcmp(str, "qqmusic_privatekey"))
			{
				if (NULL == qqmusic_privatekey)
					qqmusic_privatekey = str2+1;
				log_output(3,"qqmusic_privatekey = [%s]\n", qqmusic_privatekey);
			}
			else if(0 == strcmp(str, "qqmusic_fromtag"))
			{
				if (-1 == qqmusic_fromtag)
					qqmusic_fromtag = atoi(str2+1);
				log_output(3,"qqmusic_fromtag = [%d]\n", qqmusic_fromtag);
				cookie_flag = 1; /* add by xt */
				//flag |= 8;
			}
			else if(0 == strcmp(str, "qqmusic_sosokey"))
			{
				if (NULL == qqmusic_sosokey)
					qqmusic_sosokey = str2 + 1;
				log_output(3,"qqmusic_sosokey = [%s]\n",qqmusic_sosokey);

			}

			else if(0 == strcmp(str, "qqmusic_guid"))
			{
				if (NULL == qqmusic_guid)
					qqmusic_guid = str2 + 1;
				log_output(3, "qqmusic_guid = [%s]\n",qqmusic_guid);
			}
			else if(0 == strcmp(str, "qqmusic_gkey"))
			{
				if (NULL == qqmusic_gkey)
					qqmusic_gkey = str2 + 1;
				log_output(3,"qqmusic_gkey = [%s]\n",qqmusic_gkey);
			}
		}
		str = strtok(NULL, ";");
	}
	/* add by xt */
	if(!cookie_flag) {
		log_output(3,"qqmusic can not find 'qqmusic_fromtag' in the cookie\n");
		return -1;
	}
	/* end */
	if(NULL == qqmusic_key)
	{
		qqmusic_key = default_string;
	}
	if(NULL == qqmusic_privatekey)
	{
		qqmusic_privatekey = default_string;
	}
	/* begin: added by zhangxiaoyang for qqmuisc new lib 2008-06-02 
	 * add the two parameters:qqmusic_guid,qqmusic_gkey of cookie 
	 * by zhoudshu for qqmusic new lib 2009-09-10 */
	if(NULL == qqmusic_sosokey)
	{
		qqmusic_sosokey = default_string;
	}

	if(NULL == qqmusic_gkey)
	{
		qqmusic_gkey = default_string;
	}
	if(NULL == qqmusic_guid)
	{
		qqmusic_guid = default_string;
	}
	/* end */

	/* modified by zhangxiaoyang for qqmusic new lib 2008-06-02
	 *  add qqmuisc_sosokey.
	 */
	log_output(2,"qqmusic_uin=[%d] qqmusic_key=[%s] \
			qqmusic_privatekey=[%s] qqmusic_fromtag = [%d] qqmuisc_sosokey = [%s] \
			qqmusic_gkey = [%s] qqmusic_guid = [%s] \n", \
			qqmusic_uin, qqmusic_key, qqmusic_privatekey, qqmusic_fromtag, qqmusic_sosokey, qqmusic_gkey, qqmusic_guid);
	*fromtag = qqmusic_fromtag;
	int ret = qmusic_verify(qqmusic_uin, qqmusic_key, qqmusic_privatekey, qqmusic_fromtag, qqmusic_sosokey, qqmusic_guid, qqmusic_gkey);
	/* end */
	if(0 != ret)
	{
		log_output(1,"qmusic_verify failure, ret=[%d]\n", ret);
		return -1;
	}
	return 0;
}
#endif
#ifdef QQ_MUSIC
static void dealwithQQMusic(struct redirect_conf* pstRedirectConf, const char* line)
{
	char* limit = strstr(g_stURL.other, g_szDelimit);
	char* str;
	char  acRefer[DOMAIN_LENGTH] = {0};
	char* pcRefer = NULL;

	/* analyse Referer:
	 * 
	 */
	pcRefer = strstr(line, "Referer:");
	if(NULL != pcRefer)
	{
		char *pcReferEnd;
		int iReferLength;

		pcRefer += strlen("Referer:");
		pcReferEnd = pcRefer;

		while(*pcReferEnd != ' ' && *pcReferEnd != '\t' 
				&& *pcReferEnd != '\n' && *pcReferEnd != '\r' 
				&& *pcReferEnd != '\0')
		{
			pcReferEnd++;
		}

		iReferLength = pcReferEnd - pcRefer;

		if(iReferLength > 0 && iReferLength < (sizeof(acRefer) - 1))
		{
			memcpy(acRefer, pcRefer, iReferLength);
			acRefer[iReferLength] = '\0';
		}
	} 

	if(-1 == isWhiteQQ(pstRedirectConf))
	{
		log_output(2,"is not in qq white\n");
		if((0!=strncmp(g_stRequest.ip, "127.0.0.1", 9)) && (1==pstRedirectConf->md5_start))
		{
			int fromtag = -1;
			int ret = qq_verify(&fromtag);

			if(ret < 0)
			{
				goto qq_music_error;
			}
		}
	}
	else {
		int fromtag = -1;
		/* begin to check fromtag */
		char* str1 = strstr(g_stRequest.other, "Cookie:");

		if(NULL == str1)
		{
			log_output(1,"no cookie in header\n");
			goto qq_music_error;
		}
		str1 += 7;
		while(' ' == *str1)
			str1++;
		char* str2 = strchr(str1, '\t');
		if(NULL == str2)
		{
			str2 = strchr(str1, '\r');
			if(NULL == str2)
			{
				log_output(1,"no normal delimit(\\t or \\r) after Cookie\n");
				goto qq_music_error;
			}
		}
		if(str2-str1 >= MD5_BUFFER_SIZE)
		{
			log_output(1,"cookie string is too long, lenhth=[%d]\n", str2-str1);
			goto qq_music_error;
		}
		memcpy(g_szBuffer, str1, str2-str1);
		g_szBuffer[str2-str1] = 0;

		qq_cookie_parse(g_szBuffer, NULL, NULL, NULL, &fromtag);

		log_output(3,"qqmuisc_input:checkDomainAndFromtag [%s] [%d]\n", acRefer, fromtag);
		if(checkDomainAndFromtag(acRefer, fromtag) != 0) {
			log_output(3,"qqmusic_ouput:checkDomainAndFromtag failed\n");
			goto qq_music_error;
		}
		log_output(3,"qqmusic_ouput:checkDomainAndFromtag succeed\n");
	}

	log_output(2,"is in qq while\n");

	printf("%s://%s", g_stURL.protocol, g_stURL.domain);

	log_output(2,"output:%s://%s", g_stURL.protocol, g_stURL.domain);

	char ch = 0;
	if(limit)
	{
		ch = *limit;
		*limit = 0;
	}
	char* question = strchr(g_stURL.other, '?');
	if(question)
	{
		*question = 0;
	}
	str = strrchr(g_stURL.other, '/');
	if(str)
	{
		*str = 0;
		str++;
		printf("/%s", g_stURL.other);
		log_output(1,"/%s", g_stURL.other);
	}
	else
	{
		str = g_stURL.other;
	}
	qq_filename_class(str);
	if(!pstRedirectConf->decode && question)
	{
		*question = '?';
		printf("%s", question);
		log_output(1,"%s", question);
	}
	if(limit)
	{
		*limit = ch;
		printf("%s", limit);
		log_output(1,"%s", limit);
	}

	printf(" %s %s", g_stRequest.ip, g_stRequest.other);
	fflush(stdout);
	log_output(1," %s %s", g_stRequest.ip, g_stRequest.other);
	return;

qq_music_error:
	if(pstRedirectConf->fail_dst)
	{
		printf("%s", pstRedirectConf->fail_dst);
		log_output(1,"output:%s", pstRedirectConf->fail_dst);
	}
	else
	{
		printf("%s", g_szDefaultWeb);
		log_output(1,"output:%s", g_szDefaultWeb);
	}
	if(limit)
	{
		printf("%s", limit);
		log_output(1,"%s", limit);
	}
	printf(" %s %s", g_stRequest.ip, g_stRequest.other);
	fflush(stdout);
	log_output(1," %s %s", g_stRequest.ip, g_stRequest.other);
	log_flush();
	return;
}
#endif

#ifdef DUOWAN
static void dealwithDuoWan(struct redirect_conf* pstRedirectConf, const char* line)
{

	struct duowanData* other= (struct duowanData*)pstRedirectConf->other;
	//srand((int)(time(0)));
	int n = rand()%100 +1;
	//	int low,high;
	int percentValue = other->percentValue;
	//	low = (RAND_MAX+1)%100 - percentValue/2;
	//	high = 68 + percentValue/2;
	//if(n>= low && n<=high)
	if(n<=percentValue)
	{

		//fprintf(g_fpLog,"redirect to new url %s\n",other->replaceUrl);
		log_output(5,"output: %s %s %s\n",other->replaceUrl,g_stRequest.ip,g_stRequest.other);
		printf("%s %s %s",other->replaceUrl,g_stRequest.ip,g_stRequest.other);
		fflush(stdout);


	}
	else
	{
		log_output(5,"output: %s %s %s\n",g_stRequest.url,g_stRequest.ip,g_stRequest.other);
		printf("%s %s %s",g_stRequest.url,g_stRequest.ip,g_stRequest.other);
		fflush(stdout);
	}
	log_flush();
}
#endif
static char g_szLine[20488];
/*处理一个请求，按照规则进行输出
 *输入：
 *    line----一个请求，占一行
 *返回值：
 *    0----请求合法，标准输出输出新的url
 *    -1----请求非法，标准输出或者输出空行，或者输出错误页面
 */
int dealwithRequest(const char* line)
{
	char url_new[MAX_URL_LENGTH];
	memset(url_new,0,MAX_URL_LENGTH);
	strcpy(g_szLine, line);
	int valid_flag = 0;		
	//标识，0--合法，-1--输出默认url（可能是用户请求不合法，但是找到了对应的规则，或者是operate为deny），
	//-2--用户请求不合法，也没有对应的规则，-3--不需要防盗链的，或者operate为bypass的，-4--域名部分替换（operate为replace_host）
	struct redirect_conf* pstRedirectConf = NULL;
	//把用户输入拆为三个字段
	int ret = analysisRequest(g_szLine, &g_stRequest);
	if(-1 == ret)
	{
		log_output(0,"error request:%s", line);
		valid_flag = -2;
		goto end;
	}

	//从url得到域名
	ret = getDomain(g_stRequest.url, &g_stURL);
	if(-1 == ret)
	{
		log_output(1,"no domain\n");
		valid_flag = -3;
		goto end;
	}

	//找到配置文件里匹配这条请求的规则
	pstRedirectConf = findOneRedirectConf(g_stURL.domain, g_stURL.other);
	if(NULL == pstRedirectConf)
	{
		log_output(1,"no match rule\n");
		valid_flag = -3;
		goto end;
	}
	log_output(1,"match rule:%s\n", pstRedirectConf->conf_string);

	if(pstRedirectConf->cust_size > 0)
	{
		g_stURL.other += pstRedirectConf->cust_size + 1;
	}
	else if(0 == pstRedirectConf->cust_size)
	{
		g_stURL.other = strchr(g_stURL.other, '/');
		if(NULL == g_stURL.other)
		{
			valid_flag = -1;
			goto end;
		}
		else
		{
			g_stURL.other++;
		}
	}

	if(1 == pstRedirectConf->decode)		//如果需要解码
	{
		//把url里的cultid到.之间的内容进行解码
		ret = decodeURL(g_stURL.other);
		if(-1 == ret)
		{
			log_output(2,"four to three error\n");
			log_output(3,"four to three=[%s]\n", g_stURL.other);
			valid_flag = -1;
			goto end;
		}
		log_output(3,"four to three=[%s]\n", g_stURL.other);
	}
	else if(2 == pstRedirectConf->decode)		//utf8 - gb2312
	{
		log_output(3,"before unescape: %s\n",g_stURL.other);
		rfc1738_unescape(g_stURL.other);
		strcpy(url_new,u2g(g_stURL.other));
		log_output(3,"utf-8 convert to gb2312=[%s]\n", url_new);
	}
	else if(3 == pstRedirectConf->decode)		//gb2312 - utf8
	{
		rfc1738_unescape(g_stURL.other);
		g2u(g_stURL.other);
		rfc1738_escape(g_stURL.other);
		log_output(3,"gb2312 convert to utf8=[%s]\n", g_stURL.other);
	}

	if(BYPASS_FILTER == pstRedirectConf->operate)
	{
		valid_flag = -3;
		goto end;
	}
	else if(DENY_FILTER == pstRedirectConf->operate)
	{
		valid_flag = -1;
		goto end;
	}
	else if(RID_QUESTION_FILTER == pstRedirectConf->operate)
	{
		valid_flag = -5;
		goto end;
	}
#ifdef OUOU_DECODE	
	else if(OUOU_DECODE_FILTER == pstRedirectConf->operate)
	{
		valid_flag = -6;
		goto end;
	}
#endif

	else if(REPLACE_REGEX_FILTER == pstRedirectConf->operate)
	{
		valid_flag = -8;
		goto end;
	}
#ifdef QQ_MUSIC
	else if(QQ_MUSIC_FILTER == pstRedirectConf->operate)
	{
		valid_flag = -9;
		goto end;
	}
#endif
#ifdef DUOWAN
	else if(DUOWAN_FILTER == pstRedirectConf->operate)
	{
		valid_flag = -10;
		goto end;
	}
#endif

	// added by chenqi @2012/07/02
	// case 3254
#ifdef OUPENG
	else if (OUPENG_FILTER == pstRedirectConf->operate)
	{
		extern int OupengVerify(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other);
		return OupengVerify(pstRedirectConf,g_stRequest.url, g_stRequest.ip, g_stRequest.other);
	}
#endif
	// added end

#ifdef QQ_GREEN_MP3
	else if (QQ_GREEN_HIGH_PERFORM_FILTER == pstRedirectConf->operate)
	{
		extern int qqGreenMp3Verify(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other);
		return qqGreenMp3Verify(pstRedirectConf,g_stRequest.url, g_stRequest.ip, g_stRequest.other);
	}
#endif

#ifdef NNPAN
	else if (NINETY_NINE_FILTER == pstRedirectConf->operate)
	{
		extern int NinetyNineVerify(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other);
		return NinetyNineVerify(pstRedirectConf,g_stRequest.url, g_stRequest.ip, g_stRequest.other);

	}
#endif

	// added by chenqi 
#ifdef PPTV
	else if (PPTV_ANTIHIJACK_FILTER == pstRedirectConf->operate)
	{    
		if (strstr(g_stRequest.url, "k=")) // new algorithm
		{
			extern int pptv_antihijack_verify(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other);
			return pptv_antihijack_verify(pstRedirectConf,g_stRequest.url, g_stRequest.ip, g_stRequest.other);
		}
		else if (strstr(g_stRequest.url, "key="))   // old algorithm
		{
			extern int pptv_antihijack_verify_old(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other);
			return pptv_antihijack_verify_old(pstRedirectConf,g_stRequest.url, g_stRequest.ip, g_stRequest.other);
		}
	}    
#endif
	// added end

#ifdef NOKIA
	else if (NOKIA_FILTER == pstRedirectConf->operate)
	{
		extern int NokiaVerify(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other);
		return NokiaVerify(pstRedirectConf, g_stRequest.url, g_stRequest.ip, g_stRequest.other);
	}
#endif
#ifdef BAID	
	else if (BAIDU_XCODE_FILTER == pstRedirectConf->operate)
	{
		extern int baiduXcodeVerify(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other);
		return baiduXcodeVerify(pstRedirectConf,g_stRequest.url, g_stRequest.ip, g_stRequest.other);
	}
#endif

#ifdef OOYALA
	else if (OOYALA_FILTER == pstRedirectConf->operate)
	{
		extern int OoyalaVerify(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other);
		return OoyalaVerify(pstRedirectConf,g_stRequest.url, g_stRequest.ip, g_stRequest.other);
	}
#endif
	else if (LONGYUAN_FILTER == pstRedirectConf->operate)
	{
		extern int verifyLongyuan(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other);
		return verifyLongyuan(pstRedirectConf,g_stRequest.url, g_stRequest.ip, g_stRequest.other);
	}
#ifdef SDO
	else if (SDO_FILTER == pstRedirectConf->operate)
	{
		extern int verifySdo(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other);
		return verifySdo(pstRedirectConf,g_stRequest.url, g_stRequest.ip, g_stRequest.other);
	}
#endif
	else if (MFW_MUSIC_FILTER == pstRedirectConf->operate)
	{    
		extern int verifyMFW(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other);
		return verifyMFW(pstRedirectConf,g_stRequest.url, g_stRequest.ip, g_stRequest.other);
	}    
	else if (SDO_MUSIC_FILTER == pstRedirectConf->operate)
	{
		extern int verifySdoMusic(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other);
		return verifySdoMusic(pstRedirectConf,g_stRequest.url, g_stRequest.ip, g_stRequest.other);
	}
	else if (MSN_DOWNLOAD_FILTER == pstRedirectConf->operate)
	{    
		extern int verifyMSN(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other);
		return verifyMSN(pstRedirectConf,g_stRequest.url, g_stRequest.ip, g_stRequest.other);
	}

	// add for microsoft by chenqi
	else if (MICROSOFT_FILTER == pstRedirectConf->operate)
	{
		extern int verifyMicrosoft(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other);
		return verifyMicrosoft(pstRedirectConf,g_stRequest.url, g_stRequest.ip, g_stRequest.other);
	}
	else if (BOKECC_FILTER == pstRedirectConf->operate) 
	{
		extern int verifyBokecc(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other);
		return verifyBokecc(pstRedirectConf, g_stRequest.url, g_stRequest.ip, g_stRequest.other);
	}
	else if (MICROSOFT_MD5_FILTER == pstRedirectConf->operate)
	{
		extern int verifyMicrosoft_MD5(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other);
		return verifyMicrosoft_MD5(pstRedirectConf,g_stRequest.url, g_stRequest.ip, g_stRequest.other);
	}
	// add end
	else if (CWG_FILTER == pstRedirectConf->operate)
	{
		extern int verifyCWG(const struct redirect_conf *pstRedirectConf, char *rul, char *ip, char *other);
		return verifyCWG(pstRedirectConf, g_stRequest.url, g_stRequest.ip, g_stRequest.other);
	}
#ifdef IQILU
	else if (IQILU_FILTER == pstRedirectConf->operate)
	{
		extern int verifyIqilu(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other);
		return verifyIqilu(pstRedirectConf,g_stRequest.url, g_stRequest.ip, g_stRequest.other);
	}
#endif
	else if (MUSIC163_FILTER == pstRedirectConf->operate)
	{
		extern int verify163Music(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other);
		return verify163Music(pstRedirectConf,g_stRequest.url, g_stRequest.ip, g_stRequest.other);
	}
	else if (WANGLONG_FILTER == pstRedirectConf->operate)
	{
		extern int verifyWanglong(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other);
		return verifyWanglong(pstRedirectConf, g_stRequest.url, g_stRequest.ip, g_stRequest.other);
	}
	//按照规则方法进行处理
	else if(IP_FILTER == pstRedirectConf->operate)	//只检查ip
	{
		valid_flag = checkIpFilter(pstRedirectConf, &g_stRequest, &g_stURL);
		log_output(1,"valid_flag = %d\n",valid_flag);
		goto end;

	}
	else if(IP_ABORT_FILTER == pstRedirectConf->operate)
	{
		valid_flag = checkIpAbortFilter(pstRedirectConf, &g_stRequest, &g_stURL);
		goto end;
	}
	else if(TIME_FILTER == pstRedirectConf->operate)	//只检查ip
	{
		valid_flag = checkTimeFilter(pstRedirectConf, &g_stURL);
		goto end;
	}
	else if (TIME1_FILTER == pstRedirectConf->operate)
	{
		valid_flag = checkTime1Filter(pstRedirectConf, &g_stURL);
		goto end;
	}
	else if (TIME2_FILTER == pstRedirectConf->operate)
	{
		valid_flag = checkTime2Filter(pstRedirectConf, &g_stURL);
		goto end;
	}
	else if(IP_TIME_FILTER == pstRedirectConf->operate)	//只检查ip
	{
		valid_flag = checkIpTimeFilter(pstRedirectConf, &g_stRequest, &g_stURL);
		goto end;
	}
	else if(IP_TIME_ABORT_FILTER == pstRedirectConf->operate)	//只检查ip
	{
		valid_flag = checkIpTimeAbortFilter(pstRedirectConf, &g_stRequest, &g_stURL);
		goto end;
	}
	else if(REPLACE_HOST_FILTER == pstRedirectConf->operate)
	{
		valid_flag = -4;
		goto end;
	}
	else if(REPLACE_HOST_ALL_FILTER == pstRedirectConf->operate)
	{
		valid_flag = -7;
		goto end;
	}
	else if(SINA_FILTER == pstRedirectConf->operate) 
	{
		/* add by xt for sina */
		valid_flag = deal_with_sina(pstRedirectConf, &g_stURL);
	}
#ifdef MYSPACE_CHECK
	else if(MYSPACE_FILTER == pstRedirectConf->operate) 
	{
		/* add by xt for myspace */
		valid_flag = checkMyspace(pstRedirectConf, &g_stRequest, &g_stURL);
		log_output(1,"%d\n",valid_flag);
		goto end;

	}
#endif
	else if(COOKIE_MD5_FILTER == pstRedirectConf->operate) 
	{
		extern int CookieMd5Verfiy(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other);
		return CookieMd5Verfiy(pstRedirectConf,g_stRequest.url, g_stRequest.ip, g_stRequest.other); 
	}
	else if(QQ_TOPSTREAM_FILTER == pstRedirectConf->operate) 
	{
		extern int qqTopStreamVerify(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other);
		return qqTopStreamVerify(pstRedirectConf, g_stRequest.url, g_stRequest.ip, g_stRequest.other); 
	}
	else if(TUDOU_FILTER == pstRedirectConf->operate) 
	{
		extern int verifyTudou(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other);
		return verifyTudou(pstRedirectConf, g_stRequest.url, g_stRequest.ip, g_stRequest.other);
	}
#ifdef QQ_GREEN_MP3
	else if (QQ_GREEN_HIGH_PERFORM_FILTER == pstRedirectConf->operate)
	{
		extern int qqGreenMp3Verify(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other);
		return qqGreenMp3Verify(pstRedirectConf,g_stRequest.url, g_stRequest.ip, g_stRequest.other);
	}
#endif
	else if (LIGHTTPD_SECDOWNLOAD_FILTER == pstRedirectConf->operate)
	{
		extern int lighttpd_secdownload_verify(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other);
		return lighttpd_secdownload_verify(pstRedirectConf,g_stRequest.url, g_stRequest.ip, g_stRequest.other);
	}
	else if(DECODE_FILTER == pstRedirectConf->operate)
	{
		printf("%s://", g_stURL.protocol);
		log_output(1,"output: %s://", g_stURL.protocol);
		if(NULL != pstRedirectConf->replace_host)
		{
			printf("%s", pstRedirectConf->replace_host);
			log_output(1,"%s", pstRedirectConf->replace_host);
		}
		else
		{
			printf("%s", g_stURL.domain);
			log_output(1,"%s", g_stURL.domain);
		}

		if(NULL != pstRedirectConf->replace_dir)
		{
			printf("/%s", pstRedirectConf->replace_dir);
			log_output(1,"/%s", pstRedirectConf->replace_dir);
		}
		printf("/%s %s %s",url_new, g_stRequest.ip, g_stRequest.other);
		fflush(stdout);
		log_output(1,"/%s %s %s\n",url_new, g_stRequest.ip, g_stRequest.other);
		log_flush();
		return 0;
	}
end:
	if(0 == valid_flag)		//请求url合法
	{
		printf("%s://", g_stURL.protocol);
		log_output(1,"output: %s://", g_stURL.protocol);
		if(NULL != pstRedirectConf->replace_host)
		{
			printf("%s", pstRedirectConf->replace_host);
			log_output(1,"%s", pstRedirectConf->replace_host);
		}
		else
		{
			printf("%s", g_stURL.domain);
			log_output(1,"%s", g_stURL.domain);
		}
		if(NULL != pstRedirectConf->replace_dir)
		{
			printf("/%s", pstRedirectConf->replace_dir);
			log_output(1,"/%s", pstRedirectConf->replace_dir);
		}
		printf("%s %s %s", g_stURL.filename, g_stRequest.ip, g_stRequest.other);
		fflush(stdout);
		log_output(1,"%s %s %s", g_stURL.filename, g_stRequest.ip, g_stRequest.other);
		log_flush();
		return 0;
	}
	else if(-1 == valid_flag)	//用户请求不合法，但是找到了对应的规则
	{
		char* str = strstr(g_stRequest.url, g_szDelimit);
		if(NULL != pstRedirectConf->fail_dst)
		{
			if(NULL == str)
			{
				if (FAIL_DST_TYPE_CODE == pstRedirectConf->fail_dst_type)
				{
					printf("%s:%s %s %s",
							pstRedirectConf->fail_dst,
							g_szDefaultWeb,
							g_stRequest.ip,
							g_stRequest.other);
					log_output(1,
							"output:%s:%s %s %s",
							pstRedirectConf->fail_dst,
							g_szDefaultWeb,
							g_stRequest.ip,
							g_stRequest.other);

				}
				else
				{
					printf("%s %s %s",
							pstRedirectConf->fail_dst,
							g_stRequest.ip,
							g_stRequest.other);
					log_output(1,
							"output:%s %s %s",
							pstRedirectConf->fail_dst,
							g_stRequest.ip,
							g_stRequest.other);
				}
				//printf("%s %s %s", pstRedirectConf->fail_dst, g_stRequest.ip, g_stRequest.other);
				//log_output(1,"output:%s %s %s", pstRedirectConf->fail_dst, g_stRequest.ip, g_stRequest.other);
			}
			else
			{
				if (FAIL_DST_TYPE_CODE == pstRedirectConf->fail_dst_type)
				{
					printf("%s:%s%s %s %s",
							pstRedirectConf->fail_dst,
							g_szDefaultWeb,
							str,
							g_stRequest.ip,
							g_stRequest.other);
					log_output(1,
							"output:%s:%s%s %s %s",
							pstRedirectConf->fail_dst,
							g_szDefaultWeb,
							str,
							g_stRequest.ip,
							g_stRequest.other);
				}
				else
				{
					printf("%s%s %s %s",
							pstRedirectConf->fail_dst,
							str,
							g_stRequest.ip,
							g_stRequest.other);
					log_output(1,
							"output:%s%s %s %s",
							pstRedirectConf->fail_dst,
							str,
							g_stRequest.ip,
							g_stRequest.other);
				}
			//	printf("%s%s %s %s", pstRedirectConf->fail_dst, str, g_stRequest.ip, g_stRequest.other);
			//	log_output(1,"output:%s%s %s %s", pstRedirectConf->fail_dst, str, g_stRequest.ip, g_stRequest.other);
			}
		}
		else
		{
			if(NULL == str)
			{
				printf("%s %s %s", g_szDefaultWeb, g_stRequest.ip, g_stRequest.other);
				log_output(1, "output:%s %s %s", g_szDefaultWeb, g_stRequest.ip, g_stRequest.other);
			}
			else
			{
				printf("%s%s %s %s", g_szDefaultWeb, str, g_stRequest.ip, g_stRequest.other);
				log_output(1, "output:%s%s %s %s", g_szDefaultWeb, str, g_stRequest.ip, g_stRequest.other);
			}
		}
		fflush(stdout);
		log_flush();
		return -1;
	}
	else if(-4 == valid_flag)	//operate为replace_host
	{
		if(NULL != pstRedirectConf->replace_host)
		{
			char* str = strstr(g_stURL.domain, pstRedirectConf->domain);
			if(0 == pstRedirectConf->return_code)
			{
				printf("%s://%.*s%s%s/", g_stURL.protocol, (int)(str-g_stURL.domain), g_stURL.domain, 
						pstRedirectConf->replace_host, str+strlen(pstRedirectConf->domain));
				log_output(1,"output:%s://%.*s%s%s/", g_stURL.protocol, (int)(str-g_stURL.domain), g_stURL.domain,	pstRedirectConf->replace_host, str+strlen(pstRedirectConf->domain));
				if(NULL != pstRedirectConf->replace_dir)
				{
					printf("%s/", pstRedirectConf->replace_dir);
					log_output(1,"%s/", pstRedirectConf->replace_dir);
				}
				printf("%s %s %s", g_stURL.other, g_stRequest.ip, g_stRequest.other);
				log_output(1, "%s %s %s", g_stURL.other, g_stRequest.ip, g_stRequest.other);
			}
			else
			{
				printf("%d:%s://%.*s%s%s/", pstRedirectConf->return_code,g_stURL.protocol, (int)(str-g_stURL.domain), g_stURL.domain, pstRedirectConf->replace_host, str+strlen(pstRedirectConf->domain));

				log_output(1, "output:%d:%s://%.*s%s%s/",pstRedirectConf->return_code, g_stURL.protocol, (int)(str-g_stURL.domain), g_stURL.domain,	pstRedirectConf->replace_host, str+strlen(pstRedirectConf->domain));

				if(NULL != pstRedirectConf->replace_dir)
				{
					printf("%s/", pstRedirectConf->replace_dir);
					if(g_fdDebug)
					{
						fprintf(g_fpLog, "%s/", pstRedirectConf->replace_dir);
					}
				}
				printf("%s %s %s", g_stURL.other, g_stRequest.ip, g_stRequest.other);
				log_output(1, "%s %s %s", g_stURL.other, g_stRequest.ip, g_stRequest.other);
			}
		}
		else
		{
			printf("%s", line);
			log_output(1, "output:%s", line);
		}
		fflush(stdout);
		log_flush();
		return -1;
	}
	else if(-7 == valid_flag)	//operate为replace_host_all
	{
		if(NULL != pstRedirectConf->replace_host)
		{
			if(0 == pstRedirectConf->return_code )
			{
				printf("%s://%s/", g_stURL.protocol, pstRedirectConf->replace_host);
				log_output(1, "output:%s://%s/", g_stURL.protocol, pstRedirectConf->replace_host);

				if(NULL != pstRedirectConf->replace_dir)
				{
					printf("%s/", pstRedirectConf->replace_dir);
					log_output(1, "%s/", pstRedirectConf->replace_dir);
				}
				printf("%s %s %s", g_stURL.other, g_stRequest.ip, g_stRequest.other);
				log_output(1, "%s %s %s", g_stURL.other, g_stRequest.ip, g_stRequest.other);
			}
			else
			{
				printf("%d:%s://%s/", pstRedirectConf->return_code,g_stURL.protocol, pstRedirectConf->replace_host);
				log_output(1, "output:%d:%s://%s/",pstRedirectConf->return_code, g_stURL.protocol, pstRedirectConf->replace_host);

				if(NULL != pstRedirectConf->replace_dir)
				{
					printf("%s/", pstRedirectConf->replace_dir);
					log_output(1, "%s/", pstRedirectConf->replace_dir);
				}
				printf("%s %s %s", g_stURL.other, g_stRequest.ip, g_stRequest.other);
				log_output(1, "%s %s %s", g_stURL.other, g_stRequest.ip, g_stRequest.other);

			}
		}
		else
		{
			printf("%s", line);
			log_output(1, "output:%s", line);
		}
		fflush(stdout);
		log_flush();
		return -1;
	}
	else if(-5 == valid_flag)	//operate为rid_question
	{
		char* str = strchr(g_stURL.other, '?');
		if(NULL != str)
		{
			char* str2 = strstr(str, g_szDelimit);
			if(NULL != str2)
			{
				memmove(str, str2, strlen(str2)+1);
			}
			else
			{
				*str = 0;
			}
		}

		printf("%s://%s/%s %s %s", g_stURL.protocol, g_stURL.domain, g_stURL.other, g_stRequest.ip, g_stRequest.other);
		log_output(1, "output:%s://%s/%s %s %s", g_stURL.protocol, g_stURL.domain, g_stURL.other, g_stRequest.ip, g_stRequest.other);
		fflush(stdout);
		log_flush();
		return -1;
	}
	else if(-8 == valid_flag)	//operate为replace_regex
	{
		dealwithReplaceRegex(pstRedirectConf, line);
		return -1;
	}
#ifdef QQ_MUSIC
	else if(-9 == valid_flag)	//operate为qq_music
	{
		dealwithQQMusic(pstRedirectConf, line);
		return -1;
	}
#endif
#ifdef DUOWAN
	else if(-10 == valid_flag)
	{
		dealwithDuoWan(pstRedirectConf,line);
		return -1;
	}
#endif
	else	//valid_flag为-2或者-3
	{
		printf("%s", line);
		fflush(stdout);
		log_output(1,"output:%s", line);
		log_flush();
		return -1;
	}
}

/* add by xt for 17173 */
static struct hash_id * hash_id_create(const int size)
{
	struct hash_id *h = calloc(1, sizeof(*h));
	assert(h);

	h->items = calloc(size, sizeof(void *));
	h->size = size;
	h->offset = 0;

	return h;
}

static void hash_id_append(struct hash_id *h, const char *ip)
{
	assert(ip);
	assert(h);
	if(h->offset > h->size) 
		return;
	h->items[h->offset++] = (void *)ip;
}

static void hash_id_clean(struct hash_id *h) 
{
	if(!h)
		return;

	int i;
	for(i = 0; i< h->offset; i++) 
		free(h->items[i]);

	free(h);
}

static struct hash_id* cookie_handle(struct hash_id *h, const int sec, const char *cookie, int size) 
{
	assert(h);
	assert(cookie);

	char *token;
	char *start;
	long long int ip_value;
	char t[16];
	long long int ip;
	//char line[2048];
	char buffer[4097];
	char *line = NULL;
	size_t memsize = 0;

	/* if data size bigger than stack buffer size, malloc it */
	memsize = (size > 4096) ? (size + 1) : 4097;
	line = (memsize > 4097) ? malloc(memsize) : buffer;		
	if (NULL == line)
	{
		/* if malloc failed, force to use buffer */
		line = buffer;
		/* cut data to 4096 */
		size = 4096;
		/* set memsize to buffer size 4097 */
		memsize = 4097;
	}
	memset(line, 0, memsize);
	memcpy(line, cookie, size);

	token = strtok(line, "; ");
	if(token == NULL)
		goto err_end;
	//return NULL;

	if(strcasestr(token, "hash_id")) {
		start = strchr(token, '=');
		if(start == NULL) 
			goto err_end;
		//return NULL;
		start++;

		snprintf(t, 10, "%s", start);
		ip = strtoll(t, NULL, 16);
		ip_value = ip - sec;
		/* get ip string */
		hash_id_append(h, ip_str_get(ip_value));

		log_output(1, "cookie_handle: hash_id %s %s,ip %lld, ip_value %lld, secure %d\n", \
				start, t, ip, ip_value, sec);
	}
	while((token = strtok(NULL, "; "))) {
		if(!strcasestr(token, "hash_id"))
			continue;
		start = strchr(token, '=');
		if(!start)
			continue;
		start++;

		snprintf(t, 10, "%s", start);
		ip = strtoll(t, NULL, 16);
		ip_value = ip - sec;
		hash_id_append(h, ip_str_get(ip_value));
	}
	if (size > 4096 && line != NULL)
	{
		free(line);
		line = NULL;
	}
	return h;			

err_end:
	if (size > 4096 && line != NULL)
	{
		free(line);
		line = NULL;
	}
	return NULL;
}

static char *ip_str_get(const long long int val)
{
	char temp[16];
	char i1[3];
	int iv1;
	char i2[3];
	int iv2;
	char i3[3];
	int iv3;
	char i4[3];
	int iv4;

	memset(i1, 0, sizeof(i1));
	memset(i2, 0, sizeof(i2));
	memset(i3, 0, sizeof(i3));
	memset(i4, 0, sizeof(i4));


	snprintf(temp, 15, "%08x", (unsigned int)val);

	memcpy(i1, temp, 2);
	memcpy(i2, temp + 2, 2);
	memcpy(i3, temp + 4, 2);
	memcpy(i4, temp + 6, 2);	

	log_output(1,"ip_str_get: %s, %lld\n" \
			"%s %s %s %s\n", temp, val, i1, i2, i3, i4);

	iv1 = strtol(i1, NULL, 16);
	iv2 = strtol(i2, NULL, 16);
	iv3 = strtol(i3, NULL, 16);
	iv4 = strtol(i4, NULL, 16);

	snprintf(temp, 16, "%d.%d.%d.%d", iv1, iv2, iv3, iv4);

	log_output(1,"ip_str_get: %s\n", temp);
	return strdup(temp);
}
