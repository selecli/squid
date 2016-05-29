#ifndef __HTTP_H
#define __HTTP_H

#define MAX_QUERY	2048		/* Should not grow larger..	*/

typedef struct
{
	char host[MAX_STRING];
	char auth[MAX_STRING];
	char request[MAX_QUERY];
	char headers[MAX_QUERY];
	int proto;			/* FTP through HTTP proxies	*/
	int proxy;
	uint64_t firstbyte;
	uint64_t lastbyte;
	int status;
	int fd;
	//char *local_if;

	char * filename;	//Xcell object的文件名
	int getting_size;	//本次是取得size的操作
	int handle_header;
	int print_reply;
	int retry;			//重传次数
} http_t;

int http_connect( http_t *conn, int proto, char *proxy, char *host, int port, char *user, char *pass ,char * dst_ip);
void http_disconnect( http_t *conn );
void http_get( http_t *conn, char *lurl ,char * extra_hdr);
void http_addheader( http_t *conn, char *format, ... );
int http_exec( http_t *conn );
char *http_header( http_t *conn, char *header );
uint64_t http_size( http_t *conn );
void http_encode( char *s );
void http_decode( char *s );

#endif

