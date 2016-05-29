#include "flexiget.h"
#include <assert.h>
#include <strings.h>

int http_connect( http_t *conn, int proto, char *proxy, char *host, int port, char *user, char *pass , char * dst_ip)
{
	char base64_encode[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz0123456789+/";
	char auth[MAX_STRING];
	conn_t tconn[1];
	int i;

	strncpy( conn->host, host, MAX_STRING );
	conn->proto = proto;
	if( proxy != NULL ) 
	{ 
		if( *proxy != 0 )
		{
			sprintf( conn->host, "%s:%i", host, port );
			if( !conn_set( tconn, proxy ) )
			{
				sprintf( conn->request, _("Invalid proxy string: %s\n"), proxy );
				return( 0 );
			}
			host = tconn->host;
			port = tconn->port;
			conn->proxy = 1;
		}
		else
		{
			conn->proxy = 0;
		} 
	}

	debug("Connecting to %s\n", dst_ip);
	if( ( conn->fd = tcp_connect( dst_ip, port, NULL ) ) == -1 ) {
		sprintf( conn->request, _("Unable to connect to server %s:%i\n"), host, port );
		return( 0 );
	}
	debug("Connected to %s\n", dst_ip);

	if( *user == 0 ) {
		*conn->auth = 0;
	} else {
		memset( auth, 0, MAX_STRING );
		snprintf( auth, MAX_STRING, "%s:%s", user, pass );
		for( i = 0; auth[i*3]; i ++ ) {
			conn->auth[i*4] = base64_encode[(auth[i*3]>>2)];
			conn->auth[i*4+1] = base64_encode[((auth[i*3]&3)<<4)|(auth[i*3+1]>>4)];
			conn->auth[i*4+2] = base64_encode[((auth[i*3+1]&15)<<2)|(auth[i*3+2]>>6)];
			conn->auth[i*4+3] = base64_encode[auth[i*3+2]&63];
			if( auth[i*3+2] == 0 ) conn->auth[i*4+3] = '=';
			if( auth[i*3+1] == 0 ) conn->auth[i*4+2] = '=';
		}
	}

	return( 1 );
}

void http_disconnect( http_t *conn )
{
	if( conn->fd > 0 )
		close( conn->fd );
	conn->fd = -1;
}

void http_get( http_t *conn, char *lurl , char * extra_hdr)
{
	*conn->request = 0;
	if( conn->proxy )
	{
		http_addheader( conn, "GET %s://%s%s HTTP/1.0",
				conn->proto == PROTO_HTTP ? "http" : "ftp", conn->host, lurl );
	}
	else
	{
		http_addheader( conn, "GET %s HTTP/1.0", lurl );
		http_addheader( conn, "Host: %s", conn->host );
	}
	http_addheader( conn, "User-Agent: %s", USER_AGENT );
	if( *conn->auth )
		http_addheader( conn, "Authorization: Basic %s", conn->auth );
	//fprintf(stderr, "[%s][%d]conn->getting_size=%d\n", __FILE__, __LINE__, conn->getting_size);
	if( conn->firstbyte )
	{
		if( conn->lastbyte )
			http_addheader( conn, "Range: bytes=%lld-%lld", conn->firstbyte, conn->lastbyte );
		else {
			//Xcell modify
			//fprintf(stderr, "[%s][%d]conn->getting_size=%d\n", __FILE__, __LINE__, conn->getting_size);
			if (conn->getting_size != GET_GETTING)
				http_addheader( conn, "Range: bytes=%lld-", conn->firstbyte );
		}
	}

	if(extra_hdr && 0 != strlen(extra_hdr)) 
		http_addheader(conn, extra_hdr);
	//fprintf(stderr, "%s\n", conn->request);
}

void http_addheader( http_t *conn, char *format, ... )
{
	char s[MAX_STRING];
	va_list params;

	va_start( params, format );
	vsnprintf( s, MAX_STRING - 3, format, params );
	strcat( s, "\r\n" );
	va_end( params );

	strncat( conn->request, s, MAX_QUERY );
}


/*
 * 以后具体实现
 */
char * header_handle( char* buf )
{
	return NULL; 
}

int http_exec( http_t *conn )
{
	int i = 0;
	char s[2] = " ", *s2;

#ifdef DEBUG
	fprintf( stderr, "--- Sending request ---\n%s--- End of request ---\n", conn->request );
#endif
	debug("--- Sending request ---\n%s--- End of request ---\n", conn->request );
	//Xcell
	int hdr_fd = -1;
	char hdr_filename[256];
	//fprintf(stderr, "conn->getting_size=%d, filename=%s,\n", conn->getting_size, conn->filename);
	if (strcasecmp(conn->filename, "/dev/null") != 0 	//dev/null not save
			&& conn->getting_size == GET_GETTING 
			&& conn->filename && strlen(conn->filename) != 0) {
		sprintf(hdr_filename, "%s^header", conn->filename);
		//debug("Create header file %s\n", hdr_filename);
		//fprintf(stderr, "Create header file %s\n", hdr_filename);
		hdr_fd = open(hdr_filename, O_CREAT | O_WRONLY | O_TRUNC, 0666);
		//assert(hdr_fd > 0);	//
	}
	//end

	http_addheader( conn, "" );

	if (conn->handle_header == 1)
	    header_handle(conn->request);
	
	int rtn = 0;
	int len = strlen( conn->request);
	rtn = safe_write( conn->fd, conn->request, len);
	if (rtn <= 0 || rtn!=len) {
		fprintf(stderr, "rtn=%d, len=%d\n", rtn, len);
		return -1;
	}

	*conn->headers = 0;
	/* Read the headers byte by byte to make sure we don't touch the
	   actual data							*/

	while( 1 ) {
		if( read( conn->fd, s, 1 ) <= 0 ) {
			fprintf( stderr, "Connection gone.\n");
			sprintf( conn->request, _("Connection gone.\n") );
			return( 0 );
		}
		//Xcell 写文件
		if (hdr_fd > 0) {
			int rtn = safe_write(hdr_fd, s, 1);
			if (rtn <= 0 || rtn!=1) {
				//fprintf(stderr, "rtn=%d, len=%d\n", rtn, len);
				close(hdr_fd);
				return -1;	
			}
		}
		//end
		if( *s == '\r' ) {
			continue;
		} else if( *s == '\n' ) {
			if( i == 0 )
				break;
			i = 0;
		} else {
			i++;
		}
		strncat( conn->headers, s, MAX_QUERY );
	}
	
#ifdef DEBUG
	fprintf( stderr, "--- Reply headers ---\n%s--- End of headers ---\n", conn->headers );
#endif
	if (conn->print_reply == 1)
		printf("--- Reply headers ---\n%s\n", conn->headers);

	sscanf( conn->headers, "%*s %3i", &conn->status );
	s2 = strchr( conn->headers, '\n' ); *s2 = 0;
	strcpy( conn->request, conn->headers );
	*s2 = '\n';

	close(hdr_fd);		//Xcell close hdr file
	
	if (conn->status/100 != 2) {
		if ((strlen(hdr_filename) != 0) && (strcasecmp(hdr_filename, "/dev/null") != 0))
			unlink(hdr_filename);
	} else {
		conn->getting_size = GET_DONE;
	}
	
	return 0;
}

char *http_header( http_t *conn, char *header )
{
	char s[32];
	int i;

	for( i = 1; conn->headers[i]; i ++ ) {
		if( conn->headers[i-1] == '\n' )
		{
			sscanf( &conn->headers[i], "%31s", s );
			if( strcasecmp( s, header ) == 0 )
				return( &conn->headers[i+strlen(header)] );
		}
	}

	return( NULL );
}

uint64_t http_size( http_t *conn )
{
	char *i;
	uint64_t j = 0;

	if( ( i = http_header( conn, "Content-Length:" ) ) == NULL )
		return( 0 );

	sscanf( i, "%" PRINTF_UINT64_T, &j );
	return( j );
}

/* Decode%20a%20file%20name						*/
void http_decode( char *s )
{
	char t[MAX_STRING];
	int i, j, k;

	for( i = j = 0; s[i]; i ++, j ++ )
	{
		t[j] = s[i];
		if( s[i] == '%' )
			if( sscanf( s + i + 1, "%2x", &k ) )
			{
				t[j] = k;
				i += 2;
			}
	}
	t[j] = 0;

	strcpy( s, t );
}

void http_encode( char *s )
{
	char t[MAX_STRING];
	int i, j;

	for( i = j = 0; s[i]; i ++, j ++ )
	{
		t[j] = s[i];
		if( s[i] == ' ' )
		{
			strcpy( t + j, "%20" );
			j += 2;
		}
	}
	t[j] = 0;

	strcpy( s, t );
}
