#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "md5.h"
#include "three_to_four.h"

#define MAX_LINE 20488

static char g_szLine[MAX_LINE+1];

static char g_szBuffer[MAX_LINE*2+2];

static MD5_CTX g_stMD5_CTX;

static const char* FILTER_TYPE[] = {"ip", "ip_abort", "time", "ip_time", 
	"ip_time_abort", "replace_host", "deny", "bypass", "rid_question"};

static const int FILTER_TYPE_NUMBER = 5;

struct one_request
{
	char* host;
	char* cult;
	char* secret;
	char* ip;
	char* file;
	int start;
	int length;
	int decode;
	char* time;
};

void free_one_request(struct one_request* request)
{
	if(NULL != request->host)
	{
		free(request->host);
		request->host = NULL;
	}
	if(NULL != request->cult)
	{
		free(request->cult);
		request->cult = NULL;
	}
	if(NULL != request->secret)
	{
		free(request->secret);
		request->secret = NULL;
	}
	if(NULL != request->ip)
	{
		free(request->ip);
		request->ip = NULL;
	}
	if(NULL != request->file)
	{
		free(request->file);
		request->file = NULL;
	}
	if(NULL != request->time)
	{
		free(request->time);
		request->time = NULL;
	}
}

static void display_filter_type()
{
	int i;
	printf("please input filter type:");
	for(i=0;i<FILTER_TYPE_NUMBER;i++)
	{
		printf(" %d--%s", i+1, FILTER_TYPE[i]);
	}
	printf("\n");
}

static int get_filter_type(const char* type)
{
	int i;
	for(i=0;i<FILTER_TYPE_NUMBER;i++)
	{
		if(0 == strcasecmp(type, FILTER_TYPE[i]))
		{
			break;
		}
	}
	return i+1;
}

static int get_common_info(struct one_request* request)
{
	printf("please input hostname\n");
	if(NULL == fgets(g_szLine, MAX_LINE, stdin))
	{
		printf("error\n");
		return -1;
	}
	strtok(g_szLine, "\r\n");
	request->host = malloc(strlen(g_szLine)+1);
	strcpy(request->host, g_szLine);

	printf("please input cultid\n");
	if(NULL == fgets(g_szLine, MAX_LINE, stdin))
	{
		printf("error\n");
		return -1;
	}
	strtok(g_szLine, "\r\n");
	if(0 != strcasecmp(g_szLine, "none"))
	{
		request->cult = malloc(strlen(g_szLine)+1);
		strcpy(request->cult, g_szLine);
	}
	
	printf("please input secret\n");
	if(NULL == fgets(g_szLine, MAX_LINE, stdin))
	{
		printf("error\n");
		return -1;
	}
	strtok(g_szLine, "\r\n");
	request->secret = malloc(strlen(g_szLine)+1);
	strcpy(request->secret, g_szLine);

	printf("please input ip\n");
	if(NULL == fgets(g_szLine, MAX_LINE, stdin))
	{
		printf("error\n");
		return -1;
	}
	strtok(g_szLine, "\r\n");
	request->ip = malloc(strlen(g_szLine)+1);
	strcpy(request->ip, g_szLine);

	printf("please input filename\n");
	if(NULL == fgets(g_szLine, MAX_LINE, stdin))
	{
		printf("error\n");
		return -1;
	}
	strtok(g_szLine, "\r\n");
	request->file = malloc(strlen(g_szLine)+1);
	strcpy(request->file, g_szLine);

	printf("please input md5 start, 0 is first\n");
	if(NULL == fgets(g_szLine, MAX_LINE, stdin))
	{
		printf("error\n");
		return -1;
	}
	request->start = atoi(g_szLine);

	printf("please input md5 length, max is 32\n");
	if(NULL == fgets(g_szLine, MAX_LINE, stdin))
	{
		printf("error\n");
		return -1;
	}
	request->length = atoi(g_szLine);

	printf("please input decode, 1--decode, 0--not\n");
	if(NULL == fgets(g_szLine, MAX_LINE, stdin))
	{
		printf("error\n");
		return -1;
	}
	request->decode = atoi(g_szLine);

	return 0;
}

static void get32byteFrom16(unsigned char* digest, char* buffer)
{
	int i;
	for(i=0;i<16;i++)
	{
		sprintf(buffer+i*2, "%02X", digest[i]);
	}
}

static void dealwith_ip_filter()
{
	struct one_request request;
	memset(&request, 0, sizeof(request));
	if(-1 == get_common_info(&request))
	{
		free_one_request(&request);
		return;
	}

	MD5Init(&g_stMD5_CTX);
	MD5Update(&g_stMD5_CTX, (unsigned char *)request.secret, strlen(request.secret));
	MD5Update(&g_stMD5_CTX, (unsigned char *)request.ip, strlen(request.ip));
	MD5Update(&g_stMD5_CTX, (unsigned char *)request.file, strlen(request.file));
	unsigned char digest[16];
	MD5Final(digest, &g_stMD5_CTX);
	char buffer[48];
	get32byteFrom16(digest, buffer);
	printf("request: http://%s", request.host);
	if(NULL != request.cult)
	{
		printf("/%s", request.cult);
	}
	if(request.decode)
	{
		char* str = strrchr(request.file, '.');
		if(str)
		{
			*str = 0;
		}
		snprintf(g_szLine, MAX_LINE, "%s/%.*s%s", request.ip, request.length, buffer+request.start, request.file);
		multiThreeToFour(g_szLine, strlen(g_szLine), g_szBuffer);
		printf("/%s", g_szBuffer);
		if(str)
		{
			*str = '.';
			printf("%s", str);
		}
	}
	else
	{
		printf("/%s/%.*s%s", request.ip, request.length, buffer+request.start, request.file);
	}
	printf(" %s/- - GET\n", request.ip);
	free_one_request(&request);
}

static void dealwith_ip_abort_filter()
{
	struct one_request request;
	memset(&request, 0, sizeof(request));
	if(-1 == get_common_info(&request))
	{
		free_one_request(&request);
		return;
	}

	MD5Init(&g_stMD5_CTX);
	MD5Update(&g_stMD5_CTX, (unsigned char *)request.secret, strlen(request.secret));
	MD5Update(&g_stMD5_CTX, (unsigned char *)request.file, strlen(request.file));
	unsigned char digest[16];
	MD5Final(digest, &g_stMD5_CTX);
	char buffer[48];
	get32byteFrom16(digest, buffer);
	printf("request: http://%s", request.host);
	if(NULL != request.cult)
	{
		printf("/%s", request.cult);
	}
	if(request.decode)
	{
		char* str = strrchr(request.file, '.');
		if(str)
		{
			*str = 0;
		}
		snprintf(g_szLine, MAX_LINE, "%.*s%s", request.length, buffer+request.start, request.file);
		multiThreeToFour(g_szLine, strlen(g_szLine), g_szBuffer);
		printf("/%s", g_szBuffer);
		if(str)
		{
			*str = '.';
			printf("%s", str);
		}
	}
	else
	{
		printf("/%.*s%s", request.length, buffer+request.start, request.file);
	}
	printf(" %s/- - GET\n", request.ip);
	free_one_request(&request);
}

static void dealwith_time_filter()
{
	struct one_request request;
	memset(&request, 0, sizeof(request));
	if(-1 == get_common_info(&request))
	{
		free_one_request(&request);
		return;
	}

	printf("please input time, yyyymmddhhmi\n");
	if(NULL == fgets(g_szLine, MAX_LINE, stdin))
	{
		printf("error\n");
		free_one_request(&request);
		return;
	}
	strtok(g_szLine, "\r\n");
	request.time = malloc(strlen(g_szLine)+1);
	strcpy(request.time, g_szLine);

	MD5Init(&g_stMD5_CTX);
	MD5Update(&g_stMD5_CTX, (unsigned char *)request.secret, strlen(request.secret));
	MD5Update(&g_stMD5_CTX, (unsigned char *)request.time, strlen(request.time)); //fix by xt 
	MD5Update(&g_stMD5_CTX, (unsigned char *)request.file, strlen(request.file));
	unsigned char digest[16];
	MD5Final(digest, &g_stMD5_CTX);
	char buffer[48];
	get32byteFrom16(digest, buffer);
	printf("request: http://%s", request.host);
	if(NULL != request.cult)
	{
		printf("/%s", request.cult);
	}
	if(request.decode)
	{
		char* str = strrchr(request.file, '.');
		if(str)
		{
			*str = 0;
		}
		snprintf(g_szLine, MAX_LINE, "%s/%.*s%s", request.time, request.length, buffer+request.start, request.file);
		multiThreeToFour(g_szLine, strlen(g_szLine), g_szBuffer);
		printf("/%s", g_szBuffer);
		if(str)
		{
			*str = '.';
			printf("%s", str);
		}
	}
	else
	{
		printf("/%s/%.*s%s", request.time, request.length, buffer+request.start, request.file);
	}
	printf(" %s/- - GET\n", request.ip);
	free_one_request(&request);
}

static void dealwith_ip_time_filter()
{
	struct one_request request;
	memset(&request, 0, sizeof(request));
	if(-1 == get_common_info(&request))
	{
		free_one_request(&request);
		return;
	}

	printf("please input time, yyyymmddhhmi\n");
	if(NULL == fgets(g_szLine, MAX_LINE, stdin))
	{
		printf("error\n");
		free_one_request(&request);
		return;
	}
	strtok(g_szLine, "\r\n");
	request.time = malloc(strlen(g_szLine)+1);
	strcpy(request.time, g_szLine);

	MD5Init(&g_stMD5_CTX);
	MD5Update(&g_stMD5_CTX, (unsigned char *)request.secret, strlen(request.secret));
	MD5Update(&g_stMD5_CTX, (unsigned char *)request.ip, strlen(request.ip));
	MD5Update(&g_stMD5_CTX, (unsigned char *)request.time, strlen(request.ip));
	MD5Update(&g_stMD5_CTX, (unsigned char *)request.file, strlen(request.file));
	unsigned char digest[16];
	MD5Final(digest, &g_stMD5_CTX);
	char buffer[48];
	get32byteFrom16(digest, buffer);
	printf("request: http://%s", request.host);
	if(NULL != request.cult)
	{
		printf("/%s", request.cult);
	}
	if(request.decode)
	{
		char* str = strrchr(request.file, '.');
		if(str)
		{
			*str = 0;
		}
		snprintf(g_szLine, MAX_LINE, "%s/%s/%.*s%s", request.ip, request.time, request.length, buffer+request.start, request.file);
		multiThreeToFour(g_szLine, strlen(g_szLine), g_szBuffer);
		printf("/%s", g_szBuffer);
		if(str)
		{
			*str = '.';
			printf("%s", str);
		}
	}
	else
	{
		printf("/%s/%s/%.*s%s", request.ip, request.time, request.length, buffer+request.start, request.file);
	}
	printf(" %s/- - GET\n", request.ip);
	free_one_request(&request);
}

static void dealwith_ip_time_abort_filter()
{
	struct one_request request;
	memset(&request, 0, sizeof(request));
	if(-1 == get_common_info(&request))
	{
		free_one_request(&request);
		return;
	}

	printf("please input time, yyyymmddhhmi\n");
	if(NULL == fgets(g_szLine, MAX_LINE, stdin))
	{
		printf("error\n");
		free_one_request(&request);
		return;
	}
	strtok(g_szLine, "\r\n");
	request.time = malloc(strlen(g_szLine)+1);
	strcpy(request.time, g_szLine);

	MD5Init(&g_stMD5_CTX);
	MD5Update(&g_stMD5_CTX, (unsigned char *)request.secret, strlen(request.secret));
	MD5Update(&g_stMD5_CTX, (unsigned char *)request.ip, strlen(request.ip));
	MD5Update(&g_stMD5_CTX, (unsigned char *)request.time, strlen(request.ip));
	MD5Update(&g_stMD5_CTX, (unsigned char *)request.file, strlen(request.file));
	unsigned char digest[16];
	MD5Final(digest, &g_stMD5_CTX);
	char buffer[48];
	get32byteFrom16(digest, buffer);
	printf("request: http://%s", request.host);
	if(NULL != request.cult)
	{
		printf("/%s", request.cult);
	}
	if(request.decode)
	{
		char* str = strrchr(request.file, '.');
		if(str)
		{
			*str = 0;
		}
		snprintf(g_szLine, MAX_LINE, "%s/%.*s%s", request.time, request.length, buffer+request.start, request.file);
		multiThreeToFour(g_szLine, strlen(g_szLine), g_szBuffer);
		printf("/%s", g_szBuffer);
		if(str)
		{
			*str = '.';
			printf("%s", str);
		}
	}
	else
	{
		printf("/%s/%.*s%s", request.time, request.length, buffer+request.start, request.file);
	}
	printf(" %s/- - GET\n", request.ip);
	free_one_request(&request);
}

int main(int argc, char* argv[])
{
	int ret;
	while(1)
	{
		display_filter_type();
		if(NULL == fgets(g_szLine, MAX_LINE, stdin))
		{
			continue;
		}
		ret = strlen(g_szLine);
		if('\n' != g_szLine[ret-1])
		{
			printf("input is too long\n");
			continue;
		}
		strtok(g_szLine, " \t\r\n");

		ret = atoi(g_szLine);
		if(0 == ret)
		{
			ret = get_filter_type(g_szLine);
		}
		if(ret>FILTER_TYPE_NUMBER || ret < 0)
		{
			printf("type is error\n");
			continue;
		}
		printf("ret=[%d]\n", ret);
		if(1 == ret)
		{
			dealwith_ip_filter();
		}
		else if(2 == ret)
		{
			dealwith_ip_abort_filter();
		}
		else if(3 == ret)
		{
			dealwith_time_filter();
		}
		else if(4 == ret)
		{
			dealwith_ip_time_filter();
		}
		else if(5 == ret)
		{
			dealwith_ip_time_abort_filter();
		}
	}
	return 0;
}

