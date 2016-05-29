#include "flexiget.h"
#include <assert.h>

char string[MAX_STRING];

/* Convert an URL to a conn_t structure					*/
int conn_set( conn_t *conn, char *set_url )
{
	char url[MAX_STRING];
	char *i, *j;

	/* protocol://							*/
	if( ( i = strstr( set_url, "://" ) ) == NULL )
	{
		conn->proto = PROTO_DEFAULT;
		strncpy( url, set_url, MAX_STRING );
	}
	else
	{
		if( set_url[0] == 'f' )
			conn->proto = PROTO_FTP;
		else if( set_url[0] == 'h' )
			conn->proto = PROTO_HTTP;
		else
		{
			return( 0 );
		}
		strncpy( url, i + 3, MAX_STRING );
	}

	/* Split							*/
	if( ( i = strchr( url, '/' ) ) == NULL )
	{
		strcpy( conn->dir, "/" );
	}
	else
	{
		*i = 0;
		snprintf( conn->dir, MAX_STRING, "/%s", i + 1 );
		if( conn->proto == PROTO_HTTP )
			http_encode( conn->dir );
	}
	strncpy( conn->host, url, MAX_STRING );
	j = strchr( conn->dir, '?' );
	if( j != NULL )
		*j = 0;
	i = strrchr( conn->dir, '/' );
	*i = 0;
	if( j != NULL )
		*j = '?';
	if( i == NULL )
	{
		strcpy( conn->file, conn->dir );
		strcpy( conn->dir, "/" );
	}
	else
	{
		strcpy( conn->file, i + 1 );
		strcat( conn->dir, "/" );
	}

	/* Check for username in host field				*/
	if( strrchr( conn->host, '@' ) != NULL )
	{
		strncpy( conn->user, conn->host, MAX_STRING );
		i = strrchr( conn->user, '@' );
		*i = 0;
		strcpy( conn->host, i + 1 );
		*conn->pass = 0;
	}
	/* If not: Fill in defaults					*/
	else
	{
		if( conn->proto == PROTO_FTP )
		{
			/* Dash the password: Save traffic by trying
			   to avoid multi-line responses		*/
			strcpy( conn->user, "anonymous" );
			strcpy( conn->pass, "-lara_hack@gmx.co.uk" );
		}
		else
		{
			*conn->user = *conn->pass = 0;
		}
	}

	/* Password?							*/
	if( ( i = strchr( conn->user, ':' ) ) != NULL )
	{
		*i = 0;
		strcpy( conn->pass, i + 1 );
	}
	/* Port number?							*/
	if( ( i = strchr( conn->host, ':' ) ) != NULL )
	{
		*i = 0;
		sscanf( i + 1, "%i", &conn->port );
	}
	/* Take default port numbers from /etc/services			*/
	else
	{
#ifndef DARWIN
		struct servent *serv;

		if( conn->proto == PROTO_FTP )
			serv = getservbyname( "ftp", "tcp" );
		else
			serv = getservbyname( "www", "tcp" );

		if( serv )
			conn->port = ntohs( serv->s_port );
		else
#endif
		if( conn->proto == PROTO_HTTP )
			conn->port = 80;
		else
			conn->port = 21;
	}

	return( conn->port > 0 );
}

/* Generate a nice URL string.						*/
char *conn_url( conn_t *conn )
{
	if( conn->proto == PROTO_FTP )
		strcpy( string, "ftp://" );
	else
		strcpy( string, "http://" );

	if( *conn->user != 0 && strcmp( conn->user, "anonymous" ) != 0 )
		sprintf( string + strlen( string ), "%s:%s@",
			conn->user, conn->pass );

	sprintf( string + strlen( string ), "%s:%i%s%s",
		conn->host, conn->port, conn->dir, conn->file );

	return( string );
}

/* Simple...								*/
/*
 * Xcell 只是关闭连接，没有退出线程
 */
void conn_disconnect( conn_t *conn )
{
	if( conn->proto == PROTO_FTP && !conn->proxy )
		ftp_disconnect( conn->ftp );
	else
		http_disconnect( conn->http );
	conn->fd = -1;
}

//Xcell 建立TCP连接
int conn_init( conn_t *conn )
{
	conn->proxy = 0;

	if( conn->proto == PROTO_FTP && !conn->proxy )
	{
		//conn->ftp->local_if = conn->local_if;
		conn->ftp->ftp_mode = FTP_PASSIVE;
		if( !ftp_connect( conn->ftp, conn->host, conn->port, conn->user, conn->pass ) )
		{
			conn->message = conn->ftp->message;
			conn_disconnect( conn );
			return( 0 );
		}
		conn->message = conn->ftp->message;
		if( !ftp_cwd( conn->ftp, conn->dir ) )
		{
			conn_disconnect( conn );
			return( 0 );
		}
	} else {
		//conn->http->local_if = conn->local_if;
		//Xcell modify host node
		char * host = conn->host;
		if (conn->conf->dst_ip && strlen(conn->conf->dst_ip) != 0) {
			host  = conn->conf->dst_ip;
		}	
		//end 
		if( !http_connect( conn->http, conn->proto, NULL, conn->host, conn->port, conn->user, conn->pass, host) )
		{
			conn->message = conn->http->headers;
			conn_disconnect( conn );
			return( 0 );
		}
		conn->message = conn->http->headers;
		conn->fd = conn->http->fd;
	}
	return( 1 );
}

int conn_setup( conn_t *conn )
{
	if( conn->ftp->fd <= 0 && conn->http->fd <= 0 ) {
		//Xcell 建立TCP连接
		if( !conn_init( conn ) )
			return( 0 );
	}

	if( conn->proto == PROTO_FTP && !conn->proxy )
	{
		if( !ftp_data( conn->ftp ) )	/* Set up data connnection	*/
			return( 0 );
		conn->fd = conn->ftp->data_fd;

		if( conn->currentbyte )
		{
			ftp_command( conn->ftp, "REST %i", conn->currentbyte );
			if( ftp_wait( conn->ftp ) / 100 != 3 &&
			    conn->ftp->status / 100 != 2 )
				return( 0 );
		}
	}
	else
	{
		char s[MAX_STRING];

		snprintf( s, MAX_STRING, "%s%s", conn->dir, conn->file );
		conn->http->firstbyte = conn->currentbyte;
		conn->http->lastbyte = conn->lastbyte;
		//fprintf(stderr, "%s %d %s: getting_size =%d\n", __FILE__, __LINE__, __func__, conn->getting_size);
		conn->http->getting_size = conn->getting_size;
		http_get( conn->http, s , conn->conf->headers);	//Xcell 组织Request Header
	}
	return( 1 );
}


//Xcell 向服务器发送request
int conn_exec( conn_t *conn )
{
	if( conn->proto == PROTO_FTP && !conn->proxy )
	{
		if( !ftp_command( conn->ftp, "RETR %s", conn->file ) )
			return( 0 );
		return( ftp_wait( conn->ftp ) / 100 == 1 );
	} else {
		//Xcell added for write header file
		conn->http->print_reply = conn->conf->print_reply;
		conn->http->filename = conn->conf->filename;
		conn->http->getting_size = conn->getting_size;
		conn->http->handle_header = conn->conf->handle_header;
		//end
		//Xcell 向服务器发送request，并接收回复的header
		if(http_exec( conn->http ) < 0)
			return( 0 );
		return( conn->http->status / 100 == 2 );
	}
}

/* Get file size and other information					*/
int conn_info( conn_t *conn )
{
	/* It's all a bit messed up.. But it works.			*/
	if( conn->proto == PROTO_FTP && !conn->proxy )
	{
		ftp_command( conn->ftp, "REST %i", 1 );
		if( ftp_wait( conn->ftp ) / 100 == 3 ||
		    conn->ftp->status / 100 == 2 )
		{
			conn->supported = 1;
			ftp_command( conn->ftp, "REST %i", 0 );
			ftp_wait( conn->ftp );
		}
		else
		{
			conn->supported = 0;
		}

		if( !ftp_cwd( conn->ftp, conn->dir ) )
			return( 0 );
		conn->size = ftp_size( conn->ftp, conn->file, MAX_REDIR );
		if( conn->size < 0 )
			conn->supported = 0;
		if( conn->size == -1 )
			return( 0 );
		else if( conn->size == -2 )
			conn->size = 0;
	}
	else
	{
		char s[MAX_STRING], *t;
		int i = 0;

		do{
			conn->currentbyte = 1;

			//Xcell http_get 建立TCP，组织get命令
			conn_setup( conn );

			//Xcell 执行 http_exec,向服务器发request，收reply header
			conn_exec( conn );

			conn_disconnect( conn );
			/* Code 3xx == redirect				*/
			if( conn->http->status / 100 != 3 )
				break;
			if( ( t = http_header( conn->http, "location:" ) ) == NULL )
				return( 0 );
			sscanf( t, "%s", s );
				
			if( strstr( s, "://" ) == NULL)
			{
				sprintf( conn->http->headers, "%s%s",
					conn_url( conn ), s );
				strncpy( s, conn->http->headers, MAX_STRING );
			}
			else if( s[0] == '/' )
			{
				sprintf( conn->http->headers, "http://%s:%i%s",
					conn->host, conn->port, s );
				strncpy( s, conn->http->headers, MAX_STRING );
			}
			conn_set( conn, s );
			i ++;
		}
		
		while( conn->http->status / 100 == 3 && i < MAX_REDIR );

		if( i == MAX_REDIR ) {
			sprintf( conn->message, _("Too many redirects.\n") );
			return( 0 );
		}

		conn->size = http_size( conn->http );
		//FIXME: 我们要支持返回200 OK
		if( conn->http->status == 200 && conn->size > 0 ) {
			conn->supported = 1;
		} else if( conn->http->status == 206 && conn->size >= 0 ) {
			conn->supported = 1;
			conn->size++;
		} else if( conn->http->status == 200 || conn->http->status == 206 ) {
			conn->supported = 0;
			conn->size = 0;
		} else {
			t = strchr( conn->message, '\n' );
			if( t == NULL )
				sprintf( conn->message, _("Unknown HTTP error.\n") );
			else
				*t = 0;
			return( 0 );
		}
	}

	return( 1 );
}
