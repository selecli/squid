#ifndef __CONN_H
#define __CONN_H

#define PROTO_FTP	1
#define PROTO_HTTP	2
#define PROTO_DEFAULT	PROTO_FTP

//Xcell
enum {
	GET_NONE = 0,
	GET_GETTING = 2,
	GET_DONE = 5,
};
//end

typedef struct
{
	conf_t *conf;

	int proto;
	int port;
	int proxy;
	char host[MAX_STRING];
	char dir[MAX_STRING];
	char file[MAX_STRING];
	char user[MAX_STRING];
	char pass[MAX_STRING];

	ftp_t ftp[1];
	http_t http[1];
	uint64_t size;		/* File size, not 'connection size'..	*/
	uint64_t currentbyte;
	uint64_t lastbyte;
	int fd;
	int enabled;
	int supported;
	int last_transfer;
	char *message;
	//char *local_if;

	int state;
	pthread_t setup_thread[1];
	int getting_size;	//conn index by Xcell
	//char * filename;	//Xcell
} conn_t;

int conn_set( conn_t *conn, char *set_url );
char *conn_url( conn_t *conn );
void conn_disconnect( conn_t *conn );
int conn_init( conn_t *conn );
int conn_setup( conn_t *conn );
int conn_exec( conn_t *conn );
int conn_info( conn_t *conn );


#endif

