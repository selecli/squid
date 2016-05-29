#include "base64_urlencode.h"

static void char_lwr(char *c)
{
    if (*c >= 'A' && *c <= 'Z')
	    *c += 'a' - 'A';
}


// URLEncode Algorithm
unsigned char* urlencode(unsigned char *s, int len)
{
	if (NULL == s || 0 == len)
		return NULL;
	unsigned char *from = s; 
	unsigned char *end = s + len;
	unsigned char *start = NULL; 
	unsigned char *to = NULL;
	unsigned char c;
	start = to = (unsigned char *) calloc(1, 3 * len + 1);
	if (NULL == start)
		return NULL;

	unsigned char hexchars[] = "0123456789ABCDEF";

	while (from < end) 
	{
		c = *from++;
		if (c == ' ') 
		{
			*to++ = '+';
		} 
		else if ((c < '0' && c != '-' && c != '.')
				||(c < 'A' && c > '9')
				||(c > 'Z' && c < 'a' && c != '_')
				||(c > 'z')) {
			to[0] = '%';
			to[1] = hexchars[c >> 4];
			to[2] = hexchars[c & 15];
			// is this transform necessary ? i do not think so
			char_lwr(&to[1]);
			char_lwr(&to[2]);
			// end
			to += 3;
		} 
		else 
		{
			*to++ = c;
		}
	}
	*to = 0;
	return start;
}

int urldecode(unsigned char *str, int len)
{
	unsigned char *dest = str;
	unsigned char *data = str;

	int value;
	int c;

	while (len--) {
		if (*data == '+') {
			*dest = ' ';
		}
		else if (*data == '%' && len >= 2 && isxdigit((int) *(data + 1))
				&& isxdigit((int) *(data + 2)))
		{

			c = ((unsigned char *)(data+1))[0];
			if (isupper(c))
				c = tolower(c);
			value = (c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10) * 16;
			c = ((unsigned char *)(data+1))[1];
			if (isupper(c))
				c = tolower(c);
			value += c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10;

			*dest = (unsigned char)value ;
			data += 2;
			len -= 2;
		} else {
			*dest = *data;
		}
		data++;
		dest++;
	}
	*dest = '\0';
	return dest - str;
}

// base64 Algorithm

#define     CHECK_RETRUN(value)     do{if(0 == (value)) return NULL;}while(0) 
#define     IS_BASE64VALUE(ch)      (base64_index_char[ch] >= 0)
#define     IS_IGNORABLE(ch)        ((ch) == '\n' || (ch) == '\r')  /* ignore CRLF */
#define     BASE64_LINE             76

/* base64字符集合 */
static const char   base64_alphabet[] = 
{
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',   /*  0- 9 */
	'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',   /* 10-19 */
	'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd',   /* 20-29 */
	'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',   /* 30-39 */
	'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x',   /* 40-49 */
	'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7',   /* 50-59 */
	'8', '9', '+', '/'                                  /* 60-63 */
};

/* 解码表 */
static const char   base64_index_char[128] = 
{
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  /*  0-15 */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  /*  16-31 */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,  /*  32-47 ('+', '/'   */
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,  /*  48-63 ('0' - '9') */
	-1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,  /*  64-79 ('A' - 'Z'  */
	15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,  /*  80-95 */
	-1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,  /*  96-111 ('a' - 'z') */
	41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, -1, -1, -1, -1,  /*  112-127 */
};

unsigned char* base64_encode(const unsigned char* ptext, size_t bytes)
{
	CHECK_RETRUN(bytes);
	CHECK_RETRUN(ptext);

	size_t loop = bytes / 3;
	size_t buf_size = 4 * ((bytes + 2) / 3) ; 
	unsigned char* pbuf = NULL;
	unsigned char* pos = NULL; /* to skip CRLF */
	unsigned char* ret = NULL;

	buf_size += buf_size / 76;  /* CRLF length  */
	ret = pos = pbuf = (unsigned char*) calloc(1, buf_size + 1);
	CHECK_RETRUN(pbuf);


	/* 处理编码转换的部分 */        
	while (loop--)
	{
		pbuf[0] = base64_alphabet[ptext[0] >> 2];
		pbuf[1] = base64_alphabet[(ptext[0] & 0x03) << 4 | ptext[1] >> 4];
		pbuf[2] = base64_alphabet[(ptext[1] & 0x0F) << 2 | ptext[2] >> 6];
		pbuf[3] = base64_alphabet[ptext[2] & 0x3F];
		pbuf += 4;
		ptext += 3;

		if (pbuf - pos == BASE64_LINE) /* 检查是否应该换行 */
		{
			*pbuf++ = '\n';
			pos = pbuf;
		}
	}
	/* 对于最后"特殊"字节的处理*/
	switch(bytes % 3)
	{
		case 1:
			pbuf[0] = base64_alphabet[ptext[0] >> 2];
			pbuf[1] = base64_alphabet[ptext[0] << 4 & 0x30];
			pbuf[2] = '=';
			pbuf[3] = '=';
			break;

		case 2:
			pbuf[0] = base64_alphabet[ptext[0] >> 2];
			pbuf[1] = base64_alphabet[(ptext[0] << 4 & 0x30) | ptext[1] >> 4];
			pbuf[2] = base64_alphabet[(ptext[1] & 0X0F) << 2];
			pbuf[3] = '=';
			break;

		default:
			break;
	}
	pbuf[4] = '\0';  /* null terminated */
	return ret; 
}

/*
 * 对bytes个字节的ptext解码，返回解码后的字节序列
 */
unsigned char* base64_decode(const unsigned char* ptext, size_t bytes)
{
	size_t  buf_size = 3 * (bytes - bytes/(BASE64_LINE+1)) / 4;
	const unsigned char* end = ptext + bytes; 
	unsigned char*       pbuf;
	unsigned char*       ret;    
	unsigned char        quad[4] = {0}; /*四个字节一组 */
	unsigned char        byte;
	int         cnt;

	CHECK_RETRUN(ptext);
	CHECK_RETRUN(bytes);
	ret = pbuf = (unsigned char*) malloc(buf_size + 1);
	CHECK_RETRUN(pbuf);

	do
	{
		cnt = 0;
		while (cnt < 4)
		{
			byte = *ptext++;

			if (IS_BASE64VALUE(byte) && !IS_IGNORABLE(byte))
			{
				quad[cnt++] = base64_index_char[byte];
			}
		}
		/* 解码部分 */
		pbuf[0] = quad[0] << 2 | quad[1] >> 4;
		pbuf[1] = quad[1] << 4 | quad[2] >> 2;
		pbuf[2] = quad[2] << 6 | quad[3];
		pbuf += 3;
	}while (ptext < end);

	*pbuf = '\0';  /* null terminated */
	return ret;
}

