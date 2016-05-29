#ifndef REQUEST_H
#define REQUEST_H

#include <sys/time.h>

#define MAX_HEAD_LENGTH 1024
#define MAX_CONTENT_LENGTH 1024

struct Request
{
	unsigned int ip;
	int port;
	int type;	//1--get,2--post,3--head
	char filename[512];
	char host[128];
	int length;
	char header[MAX_HEAD_LENGTH];
	char content[MAX_CONTENT_LENGTH];
};

#define	DNS_INSPECT		0X00000001
#define	CONNECT_INSPECT		0X00000002
#define	WRITE_INSPECT		0X00000004
#define	READ_INSPECT		0X00000008

unsigned int getIpFromHost(char* host);
int dealwithOneRequest(struct Request* pstRequest, int fgHeader, char* filename, int length, const struct timeval* tv, unsigned int inspect, char* responseFirstLine);
int dealwithOneLine(char* line, int fgHeader, char* filename, int length, const struct timeval* tv, unsigned int inspect);

#endif	//REQUEST_H
