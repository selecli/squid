/*
 * Developed for Neulion TV
 * Author: xin.yao
 * Date:   2012-06-14
 * ------------------------------------------------------
 * auth by Url or Cookie: if url contain param 'auth_key', use url auth; else use Cookie auth
 * Url auth: time and md5
 * Cookie auth: time only
 * ------------------------------------------------------
 * MD5 based on time and ip:
 * md5("client_ip-uri-timeID-secureID-filelist")
 * ------------------------------------------------------
 * configuration example: mod_setcookie neuliontv123|neuliontv456 allow acl_neulion
 */

#include "squid.h"

#ifdef CC_FRAMEWORK

#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21
#define BUF_SIZE_128   128
#define BUF_SIZE_256   256
#define BUF_SIZE_2048  2048
#define BUF_SIZE_4096  4096
#define MAX_SECUREID_NUM 20

struct auth_params_st {
	int index;
	char md5[BUF_SIZE_128];
	char timestamp[BUF_SIZE_128];
	char client_ip[BUF_SIZE_128];
	char filelist[BUF_SIZE_2048];
	char uri[BUF_SIZE_2048];
};

typedef struct {
	unsigned int state[4];
	unsigned int count[2];
	unsigned char buffer[64];
} MY_MD5_CTX;

static cc_module *mod = NULL;
static int secureID_num = 0;
static char secureID[MAX_SECUREID_NUM][BUF_SIZE_256];

static void MD5Init(MY_MD5_CTX *);
static void MD5Update(MY_MD5_CTX *, unsigned char *, unsigned int);
static void MD5Final(unsigned char [16], MY_MD5_CTX *);
static void MD5Transform(unsigned int[4], unsigned char[64]);
static void Encode(unsigned char *, unsigned int *, unsigned int);
static void Decode(unsigned int *, unsigned char *, unsigned int);
static void MD5_memcpy(unsigned char*, unsigned char*, unsigned int);
static void MD5_memset(unsigned char*, int, unsigned int);

static unsigned char PADDING[64] = {
	0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* F, G, H and I are basic MD5 functions.
 */
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

/* ROTATE_LEFT rotates x left n bits.
 */
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

/* FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
   Rotation is separate from addition to prevent recomputation.
 */
#define FF(a, b, c, d, x, s, ac) { \
	(a) += F ((b), (c), (d)) + (x) + (unsigned int)(ac); \
	(a) = ROTATE_LEFT ((a), (s)); (a) += (b); }
#define GG(a, b, c, d, x, s, ac) { \
	(a) += G ((b), (c), (d)) + (x) + (unsigned int)(ac); \
	(a) = ROTATE_LEFT ((a), (s)); (a) += (b);}
#define HH(a, b, c, d, x, s, ac) { \
	(a) += H ((b), (c), (d)) + (x) + (unsigned int)(ac); \
	(a) = ROTATE_LEFT ((a), (s)); (a) += (b);  }
#define II(a, b, c, d, x, s, ac) { \
	(a) += I ((b), (c), (d)) + (x) + (unsigned int)(ac); \
	(a) = ROTATE_LEFT ((a), (s)); (a) += (b);  }

/* MD5 initialization. Begins an MD5 operation, writing a new context.
 */
static void MD5Init(MY_MD5_CTX *context)
{
	context->count[0] = context->count[1] = 0;
	/* Load magic initialization constants.*/
	context->state[0] = 0x67452301;
	context->state[1] = 0xefcdab89;
	context->state[2] = 0x98badcfe;
	context->state[3] = 0x10325476;
}

/* MD5 block update operation. Continues an MD5 message-digest
   operation, processing another message block, and updating the
   context.
 */
static void MD5Update (MY_MD5_CTX *context, unsigned char *input, unsigned int inputLen)
{
	unsigned int i, index, partLen;

	/* Compute number of bytes mod 64 */
	index = (unsigned int)((context->count[0] >> 3) & 0x3F);

	/* Update number of bits */
	if ((context->count[0] += ((unsigned int)inputLen << 3)) < ((unsigned int)inputLen << 3))
		context->count[1]++;
	context->count[1] += ((unsigned int)inputLen >> 29);

	partLen = 64 - index;

	/* Transform as many times as possible.*/
	if(inputLen >= partLen)
	{
		MD5_memcpy((unsigned char*)&context->buffer[index], (unsigned char*)input, partLen);
		MD5Transform (context->state, context->buffer);

		for (i = partLen; i + 63 < inputLen; i += 64)
			MD5Transform (context->state, &input[i]);

		index = 0;
	}
	else
		i = 0;

	/* Buffer remaining input */
	MD5_memcpy((unsigned char*)&context->buffer[index], (unsigned char*)&input[i],
			inputLen-i);
}

/* MD5 finalization. Ends an MD5 message-digest operation, writing the
   the message digest and zeroizing the context.
 */
static void MD5Final (unsigned char* digest, MY_MD5_CTX *context)
{
	unsigned char bits[8];
	unsigned int index, padLen;

	/* Save number of bits */
	Encode(bits, context->count, 8);

	/* Pad out to 56 mod 64.*/
	index = (unsigned int)((context->count[0] >> 3) & 0x3f);
	padLen = (index < 56) ? (56 - index) : (120 - index);
	MD5Update (context, PADDING, padLen);

	/* Append length (before padding) */
	MD5Update (context, bits, 8);
	/* Store state in digest */
	Encode (digest, context->state, 16);

	/* Zeroize sensitive information.*/
	MD5_memset((unsigned char*)context, 0, sizeof (*context));
}

/* MD5 basic transformation. Transforms state based on block.
 */
static void MD5Transform (state, block)
	unsigned int state[4];
	unsigned char block[64];
{
	unsigned int a = state[0], b = state[1], c = state[2], d = state[3], x[16];

	Decode (x, block, 64);

	/* Round 1 */
	FF (a, b, c, d, x[ 0], S11, 0xd76aa478); /* 1 */
	FF (d, a, b, c, x[ 1], S12, 0xe8c7b756); /* 2 */
	FF (c, d, a, b, x[ 2], S13, 0x242070db); /* 3 */
	FF (b, c, d, a, x[ 3], S14, 0xc1bdceee); /* 4 */
	FF (a, b, c, d, x[ 4], S11, 0xf57c0faf); /* 5 */
	FF (d, a, b, c, x[ 5], S12, 0x4787c62a); /* 6 */
	FF (c, d, a, b, x[ 6], S13, 0xa8304613); /* 7 */
	FF (b, c, d, a, x[ 7], S14, 0xfd469501); /* 8 */
	FF (a, b, c, d, x[ 8], S11, 0x698098d8); /* 9 */
	FF (d, a, b, c, x[ 9], S12, 0x8b44f7af); /* 10 */
	FF (c, d, a, b, x[10], S13, 0xffff5bb1); /* 11 */
	FF (b, c, d, a, x[11], S14, 0x895cd7be); /* 12 */
	FF (a, b, c, d, x[12], S11, 0x6b901122); /* 13 */
	FF (d, a, b, c, x[13], S12, 0xfd987193); /* 14 */
	FF (c, d, a, b, x[14], S13, 0xa679438e); /* 15 */
	FF (b, c, d, a, x[15], S14, 0x49b40821); /* 16 */

	/* Round 2 */
	GG (a, b, c, d, x[ 1], S21, 0xf61e2562); /* 17 */
	GG (d, a, b, c, x[ 6], S22, 0xc040b340); /* 18 */
	GG (c, d, a, b, x[11], S23, 0x265e5a51); /* 19 */
	GG (b, c, d, a, x[ 0], S24, 0xe9b6c7aa); /* 20 */
	GG (a, b, c, d, x[ 5], S21, 0xd62f105d); /* 21 */
	GG (d, a, b, c, x[10], S22,  0x2441453); /* 22 */
	GG (c, d, a, b, x[15], S23, 0xd8a1e681); /* 23 */
	GG (b, c, d, a, x[ 4], S24, 0xe7d3fbc8); /* 24 */
	GG (a, b, c, d, x[ 9], S21, 0x21e1cde6); /* 25 */
	GG (d, a, b, c, x[14], S22, 0xc33707d6); /* 26 */
	GG (c, d, a, b, x[ 3], S23, 0xf4d50d87); /* 27 */
	GG (b, c, d, a, x[ 8], S24, 0x455a14ed); /* 28 */
	GG (a, b, c, d, x[13], S21, 0xa9e3e905); /* 29 */
	GG (d, a, b, c, x[ 2], S22, 0xfcefa3f8); /* 30 */
	GG (c, d, a, b, x[ 7], S23, 0x676f02d9); /* 31 */
	GG (b, c, d, a, x[12], S24, 0x8d2a4c8a); /* 32 */

	/* Round 3 */
	HH (a, b, c, d, x[ 5], S31, 0xfffa3942); /* 33 */
	HH (d, a, b, c, x[ 8], S32, 0x8771f681); /* 34 */
	HH (c, d, a, b, x[11], S33, 0x6d9d6122); /* 35 */
	HH (b, c, d, a, x[14], S34, 0xfde5380c); /* 36 */
	HH (a, b, c, d, x[ 1], S31, 0xa4beea44); /* 37 */
	HH (d, a, b, c, x[ 4], S32, 0x4bdecfa9); /* 38 */
	HH (c, d, a, b, x[ 7], S33, 0xf6bb4b60); /* 39 */
	HH (b, c, d, a, x[10], S34, 0xbebfbc70); /* 40 */
	HH (a, b, c, d, x[13], S31, 0x289b7ec6); /* 41 */
	HH (d, a, b, c, x[ 0], S32, 0xeaa127fa); /* 42 */
	HH (c, d, a, b, x[ 3], S33, 0xd4ef3085); /* 43 */
	HH (b, c, d, a, x[ 6], S34,  0x4881d05); /* 44 */
	HH (a, b, c, d, x[ 9], S31, 0xd9d4d039); /* 45 */
	HH (d, a, b, c, x[12], S32, 0xe6db99e5); /* 46 */
	HH (c, d, a, b, x[15], S33, 0x1fa27cf8); /* 47 */
	HH (b, c, d, a, x[ 2], S34, 0xc4ac5665); /* 48 */

	/* Round 4 */
	II (a, b, c, d, x[ 0], S41, 0xf4292244); /* 49 */
	II (d, a, b, c, x[ 7], S42, 0x432aff97); /* 50 */
	II (c, d, a, b, x[14], S43, 0xab9423a7); /* 51 */
	II (b, c, d, a, x[ 5], S44, 0xfc93a039); /* 52 */
	II (a, b, c, d, x[12], S41, 0x655b59c3); /* 53 */
	II (d, a, b, c, x[ 3], S42, 0x8f0ccc92); /* 54 */
	II (c, d, a, b, x[10], S43, 0xffeff47d); /* 55 */
	II (b, c, d, a, x[ 1], S44, 0x85845dd1); /* 56 */
	II (a, b, c, d, x[ 8], S41, 0x6fa87e4f); /* 57 */
	II (d, a, b, c, x[15], S42, 0xfe2ce6e0); /* 58 */
	II (c, d, a, b, x[ 6], S43, 0xa3014314); /* 59 */
	II (b, c, d, a, x[13], S44, 0x4e0811a1); /* 60 */
	II (a, b, c, d, x[ 4], S41, 0xf7537e82); /* 61 */
	II (d, a, b, c, x[11], S42, 0xbd3af235); /* 62 */
	II (c, d, a, b, x[ 2], S43, 0x2ad7d2bb); /* 63 */
	II (b, c, d, a, x[ 9], S44, 0xeb86d391); /* 64 */

	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;

	/* Zeroize sensitive information.
	 */
	MD5_memset ((unsigned char*)x, 0, sizeof (x));
}

/* Encodes input (unsigned int) into output (unsigned char). Assumes len is
   a multiple of 4.
 */
static void Encode (output, input, len)
	unsigned char *output;
	unsigned int *input;
	unsigned int len;
{
	unsigned int i, j;

	for (i = 0, j = 0; j < len; i++, j += 4) {
		output[j] = (unsigned char)(input[i] & 0xff);
		output[j+1] = (unsigned char)((input[i] >> 8) & 0xff);
		output[j+2] = (unsigned char)((input[i] >> 16) & 0xff);
		output[j+3] = (unsigned char)((input[i] >> 24) & 0xff);
	}
}

/* Decodes input (unsigned char) into output (unsigned int). Assumes len is
   a multiple of 4.
 */
static void Decode (output, input, len)
	unsigned int *output;
	unsigned char *input;
	unsigned int len;
{
	unsigned int i, j;

	for (i = 0, j = 0; j < len; i++, j += 4)
		output[i] = ((unsigned int)input[j]) | (((unsigned int)input[j+1]) << 8) |
			(((unsigned int)input[j+2]) << 16) | (((unsigned int)input[j+3]) << 24);
}

/* Note: Replace "for loop" with standard memcpy if possible.
 */

static void MD5_memcpy (output, input, len)
	unsigned char* output;
	unsigned char* input;
	unsigned int len;
{
	unsigned int i;

	for (i = 0; i < len; i++)
		output[i] = input[i];
}

/* Note: Replace "for loop" with standard memset if possible.
 */
static void MD5_memset (output, value, len)
	unsigned char* output;
	int value;
	unsigned int len;
{
	unsigned int i;

	for (i = 0; i < len; i++)
		((char *)output)[i] = (char)value;
}

static void getMD5(unsigned char *mybuffer, int buf_len, char *digest)
{
	MY_MD5_CTX context;
	unsigned char szdigtmp[16];

	memset(digest, 0, BUF_SIZE_128);
	MD5Init (&context);
	MD5Update (&context, mybuffer, buf_len);
	MD5Final (szdigtmp, &context);
	snprintf(digest, BUF_SIZE_128,
			"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
			szdigtmp[0], szdigtmp[1], szdigtmp[2], szdigtmp[3], szdigtmp[4],
			szdigtmp[5], szdigtmp[6], szdigtmp[7], szdigtmp[8], szdigtmp[9],
			szdigtmp[10], szdigtmp[11], szdigtmp[12], szdigtmp[13], szdigtmp[14], szdigtmp[15]);
}

static unsigned char hexchars[] = "0123456789ABCDEF";  

static int ccHtoi(char *s)  
{  
	int c, value;  

	c = ((unsigned char *)s)[0];  
	if (isupper(c))  
		c = tolower(c);  
	value = (c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10) * 16;  

	c = ((unsigned char *)s)[1];  
	if (isupper(c))  
		c = tolower(c);  
	value += c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10;  

	return (value);  
}  

char *urlEncode(char *s, int len, int *new_length)  
{  
	unsigned char c;  
	unsigned char *to, *start;  
	unsigned char *from, *end;  

	from = (unsigned char *)s;  
	end  = (unsigned char *)s + len;  
	start = to = xcalloc(1, 3 * len + 1);  
	//must be free later: xfree()
	if (NULL == start)
		return NULL;

	while (from < end)   
	{  
		c = *from++;  

		if (c == ' ')   
		{  
			*to++ = '+';  
		}   
		else if ((c < '0' && c != '-' && c != '.') ||  
				(c < 'A' && c > '9') ||  
				(c > 'Z' && c < 'a' && c != '_') ||  
				(c > 'z'))   
		{  
			to[0] = '%';  
			to[1] = hexchars[c >> 4];  
			to[2] = hexchars[c & 15];  
			to += 3;  
		}  
		else   
		{  
			*to++ = c;  
		}  
	}  
	*to = 0;  
	if (new_length)   
	{  
		*new_length = to - start;  
	}  
	return (char *) start;  
}  

int urlDecode(char *str, int len)  
{  
	char *dest = str;  
	char *data = str;  

	while (len--)   
	{  
		if (*data == '+')   
		{  
			*dest = ' ';  
		}  
		else if (*data == '%' && len >= 2 && isxdigit((int) *(data + 1)) && isxdigit((int) *(data + 2)))   
		{  
			*dest = (char) ccHtoi(data + 1);  
			data += 2;  
			len -= 2;  
		}   
		else   
		{  
			*dest = *data;  
		}  
		data++;  
		dest++;  
	}  
	*dest = '\0';  
	return dest - str;  
}

int parseSecureID(char *line)
{	
	int i;
	char *lasts = NULL;
	char *token = NULL;

	secureID_num = 0;

	for (i = 0; i < MAX_SECUREID_NUM; ++i)
	{
		if (0 == i)
			token = strtok_r(line, "|", &lasts);
		else
			token = strtok_r(NULL, "|", &lasts);
		if (NULL == token)
		{
			if (0 == i)
				return 0;
			break;
		}
		strncpy(secureID[i], token, BUF_SIZE_256);
		secureID_num++;
	}

	return 1;
}

static long checkTimeValid(char *timestamp)
{
	return (atol(timestamp) - time(NULL));
}

static int parseAuthParams(clientHttpRequest *http, struct auth_params_st *params)
{
	char *token, *lasts;
	char *pos1, *pos2, *pos3, *pos4;
	char authKey[BUF_SIZE_4096];
	char url[BUF_SIZE_4096]; 

	memset(url, 0, BUF_SIZE_4096);
	strncpy(url, http->uri, BUF_SIZE_4096);
	pos1 = strchr(url, '?');
	if (NULL == pos1)
	{
		debug(229, 3)("mod_setcookie: request url lack '?'\n");
		return 0;
	}
	pos2 = strstr(url, "://");
	if (NULL == pos2)
	{
		debug(229, 3)("mod_setcookie: request url lack '://'\n");
		return 0;
	}
	pos2 += 3;
	pos3 = strchr(pos2, '/');
	if (NULL == pos3)
	{
		debug(229, 3)("mod_setcookie: request url lack uri\n");
		return 0;
	}
	memset(params, 0, sizeof(struct auth_params_st));
	strncpy(params->uri, pos3, pos1 - pos3);
	pos4 = strstr(pos1, "auth_key=");
	if (NULL == pos4)
	{
		debug(229, 3)("mod_setcookie: request url lack param 'auth_key'\n");
		return 0;
	}
	pos4 += 9;
	memset(authKey, 0, BUF_SIZE_4096);
	pos3 = strchr(pos4, '&');
	if (NULL == pos3)
		strcpy(authKey, pos4);
	else
		strncpy(authKey, pos4, pos3 - pos4);
	debug(229, 3)("mod_setcookie: url's auth_key is: %s\n", authKey);
	token = strtok_r(authKey, "-", &lasts);
	if (NULL == token)
	{
		debug(229, 3)("mod_setcookie: auth_key wrong when parsing for md5\n");
		return 0;
	}
	strcpy(params->md5, token);
	debug(229, 3)("mod_setcookie: url's md5: %s\n", params->md5);
	token = strtok_r(NULL, "-", &lasts);
	if (NULL == token)
	{
		debug(229, 3)("mod_setcookie: auth_key wrong when parsing for timestamp\n");
		return 0;
	}
	strcpy(params->timestamp, token);
	debug(229, 3)("mod_setcookie: url's timestamp is: %s\n", params->timestamp);
	if (checkTimeValid(params->timestamp) < 0)
	{
		debug(229, 3)("mod_setcookie: expired, url's timestamp is: %s\n", params->timestamp);
		return 0;
	}
	token = strtok_r(NULL, "-", &lasts);
	if (NULL == token)
	{
		debug(229, 3)("mod_setcookie: auth_key wrong when parsing for secureID number\n");
		return 0;
	}
	params->index = atoi(token);
	if (params->index >= secureID_num)
	{
		debug(229, 3)("mod_setcookie: index[%d] bigger than configured secureID number[%d]\n", params->index, secureID_num);
		return 0;
	}
	token = strtok_r(NULL, "-", &lasts);
	if (NULL == token)
	{
		debug(229, 3)("mod_setcookie: auth_key wrong when parsing for filelist\n");
		return 0;
	}
	strcpy(params->filelist, token);
	debug(229, 3)("mod_setcookie: url's filelist is: %s\n", params->filelist);
	strcpy(params->client_ip, inet_ntoa(http->request->client_addr));
	debug(229, 3)("mod_setcookie: client ip is: %s\n", params->client_ip);

	return 1;
}

static int updateSetcookie(clientHttpRequest *http)
{
	int keylen = 0;
	HttpReply* reply;
	HttpHeaderEntry e;
	char key[BUF_SIZE_2048];
	char path[BUF_SIZE_2048];
	char authkey[BUF_SIZE_4096];
	char *uri, *ptr, *pos, *value;

	assert(http);
	assert(http->request);
	uri = http->uri;
	ptr = strstr(uri, "auth_key=");
	if (NULL == ptr)
		return 1;
	reply = http->reply; 
	stringInit(&e.name, "Set-Cookie");
	e.id = HDR_SET_COOKIE;
	if(reply->sline.status >= HTTP_OK && reply->sline.status < HTTP_BAD_REQUEST )
	{
		ptr += strlen("auth_key=");
		pos = strchr(ptr, '&');
		memset(authkey, 0, BUF_SIZE_4096);
		strcat(authkey, "auth_key=");
		memset(key, 0, BUF_SIZE_2048);
		if (NULL == pos)
			strcpy(key, ptr);
		else
			strncpy(key, ptr, pos - ptr);
		memset(path, 0, BUF_SIZE_2048);
		ptr = strrchr(key, '-');
		if (NULL == ptr)
		{
			strcpy(path, "/");
		}
		else
		{
			ptr++;
			pos = strrchr(ptr, '/');
			if (NULL == pos)
				strcpy(path, "/");
			else
				strncpy(path, ptr, pos - ptr + 1);
		}
		value = urlEncode(key, strlen(key), &keylen);
		if (NULL == value)
		{
			strcat(authkey, key);
		}
		else
		{
			strcat(authkey, value);
			xfree(value);
		}
		strcat(authkey, "; Path=");
		strcat(authkey, path);
		debug(229, 3)("mod_setcookie: cookie value is: %s\n", authkey);
		stringInit(&e.value, authkey);
		httpHeaderAddEntry(&reply->header, httpHeaderEntryClone(&e));
		stringClean(&e.value);
	}
	else
	{
		;//do nothing
	}
	stringClean(&e.name);

	return 1;
}

static HttpHeaderEntry* httpHeaderFindEntry2(const HttpHeader * hdr, http_hdr_type id) 
{
	HttpHeaderPos pos = HttpHeaderInitPos;
	HttpHeaderEntry *e; 
	assert(hdr);

	/* check mask first */
	if (!CBIT_TEST(hdr->mask, id))
		return NULL;
	/* looks like we must have it, do linear search */
	while ((e = httpHeaderGetEntry(hdr, &pos))) {
		if (e->id == id) 
			return e;
	}   
	/* hm.. we thought it was there, but it was not found */
	assert(0);
	return NULL;        /* not reached */
}

static const char * httpHeaderGetStr2(const HttpHeader * hdr, http_hdr_type id) 
{
	HttpHeaderEntry *e; 
	if ((e = httpHeaderFindEntry2(hdr, id)))
	{   
		return strBuf(e->value);
	}   
	return NULL;
}

static int authByCookie(clientHttpRequest *http)
{
	int authkey_len;
	request_t *request;
	const char *hdr_cookie;
	char *pos, *ptr, *token, *lasts;
	char authkey[BUF_SIZE_4096];
	char cookie[BUF_SIZE_4096];

	request = http->request;
	hdr_cookie = httpHeaderGetStr2(&request->header, HDR_COOKIE);
	if(NULL == hdr_cookie)
	{
		debug(229, 3)("mod_setcookie: no Cookie found\n");
		return 0;
	}
	memset(cookie, 0, BUF_SIZE_4096);
	strncpy(cookie, hdr_cookie, BUF_SIZE_4096);
	urlDecode(cookie, strlen(cookie));
	pos = strstr(cookie, "auth_key=");
	pos += strlen("auth_key=");
	ptr = strstr(pos, ";");
	if (NULL == ptr)
		authkey_len = strlen(pos);
	else
		authkey_len = ptr - pos;
	memset(authkey, 0, BUF_SIZE_4096);
	strncpy(authkey, pos, authkey_len);
	token = strtok_r(authkey, "-", &lasts);
	if (NULL == token)
	{
		debug(229, 3)("mod_setcookie: Cookie parsing 1 for timestamp error\n");
		return 0;
	}
	token = strtok_r(NULL, "-", &lasts);
	if (NULL == token)
	{
		debug(229, 3)("mod_setcookie: Cookie parsing 2 for timestamp error\n");
		return 0;
	}
	debug(229, 3)("mod_setcookie: Cookie parsed timestamp is: %s\n", token);
	if (checkTimeValid(token) < 0)
	{
		debug(229, 3)("mod_setcookie: Cookie time expired, timestamp: %s\n", token);
		return 0;
	}

	return 1;
}

static int authByUrl(clientHttpRequest *http)
{
	char md5[BUF_SIZE_128];
	char md5str[BUF_SIZE_4096];
	struct auth_params_st params;

	if (parseAuthParams(http, &params) <= 0)
		return 0;

	memset(md5str, 0, BUF_SIZE_4096);
	snprintf(md5str, BUF_SIZE_4096,
			"%s-%s-%s-%s-%s",
			params.client_ip, params.uri, params.timestamp, secureID[params.index], params.filelist);
	debug(229, 3)("mod_setcookie: local md5 string: %s\n", md5str);
	getMD5((unsigned char *)md5str, strlen(md5str), md5);
	debug(229, 3)("mod_setcookie: local md5 value: %s\n", md5);
	if(strncasecmp(params.md5, md5, strlen(params.md5)) || 32 != strlen(params.md5))
	{   
		debug(229, 3)("mod_setcookie: md5 does not match: USER-MD5=[%s], LOCAL-MD5=[%s]\n", params.md5, md5);
		return 0;
	}   

	return 1;
}

static int authByRequest(clientHttpRequest *http)
{
	int ret = 0;

	debug(229, 3)("mod_setcookie: request url is: %s\n", http->uri);
	if (NULL == strstr(http->uri, "auth_key="))
		ret = authByCookie(http);
	else
		ret = authByUrl(http);
	if (ret <= 0)
	{
		ErrorState *err = errorCon(ERR_ACCESS_DENIED, HTTP_FORBIDDEN, http->request);
		http->log_type = LOG_TCP_MISS;
		http->entry = clientCreateStoreEntry(http, http->request->method, null_request_flags);
		errorAppendEntry(http->entry, err);
		debug(229, 3)("mod_setcookie: auth failed, so forbid!\n");
	}

	return 1;
}

static int free_callback(void *data)
{
	return 0;
}

static int parseConfigParams(char *cfg_line)
{
	debug(229, 1)("mod_setcookie: cfg line--> %s\n", cfg_line);

	char *token = NULL;
	char *wspace = " \t\r\n";

	if (NULL == (token = strtok(cfg_line, wspace)))
		goto err;
	else if (strcmp("mod_setcookie", token))
		goto err;
	if (NULL == (token = strtok(NULL, wspace)))
		goto err;
	if (parseSecureID(token) <= 0)
		goto err;
	if (NULL == (token = strtok(NULL, wspace)))
		goto err;
	if (strcmp(token, "allow") && strcmp(token, "deny"))
		goto err;
	cc_register_mod_param(mod, &secureID, free_callback);
	debug(229, 1)("mod_setcookie: secureID number is %d\n", secureID_num);
	return 0;

err:
	debug(229, 1)("mod_setcookie: error occured when parsing cfg line\n");
	return -1;
}

int mod_register(cc_module *module)
{
	debug(229, 1)("mod_token_auth: mod_register now\n");
	strcpy(module->version, "7.0.R.16488.i686");

	cc_register_hook_handler(HPIDX_hook_func_http_req_read_second,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_read_second),
			authByRequest);
	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
			parseConfigParams);
	cc_register_hook_handler(HPIDX_hook_func_http_repl_send_start,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_repl_send_start),
			updateSetcookie);

	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;

	return 1;
}
#endif

