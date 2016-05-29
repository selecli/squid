#include "flexiget.h"
#include <stdio.h>

int debug_level = 0;
FILE * debug_ouputfp = NULL;

/* flexiget */
static void save_state( flexiget_t *flexiget );
static void *setup_thread( void * );
static void flexiget_message( flexiget_t *flexiget, char *format, ... );
static void flexiget_divide( flexiget_t *flexiget );

static char *buffer = NULL;

void debug(char * format, ...)
{
	va_list list;

	if (debug_level || debug_ouputfp!=stderr) {
		va_start(list,format);
		vfprintf(debug_ouputfp, format, list);
		va_end(list);
	}
}
							

/* Create a new flexiget_t structure					*/
flexiget_t *flexiget_new( conf_t *conf, int count, void *url )
{
	search_t *res;
	flexiget_t *flexiget;
	url_t *u;
	char *s;
	int i;

	flexiget = calloc_safe(1, sizeof( flexiget_t ) );
	memset( flexiget, 0, sizeof( flexiget_t ) );
	flexiget->conf = conf;
	flexiget->conn = calloc_safe(1, sizeof( conn_t ) * flexiget->conf->how_many );
	memset( flexiget->conn, 0, sizeof( conn_t ) * flexiget->conf->how_many );
	if( flexiget->conf->limit_speed > 0 )
	{
		if( (float) flexiget->conf->limit_speed / flexiget->conf->buffer_size < 0.5 )
		{
			if( flexiget->conf->verbose >= 2 )
				flexiget_message( flexiget, _("Buffer resized for this speed.") );
			flexiget->conf->buffer_size = flexiget->conf->limit_speed;
		}
		flexiget->delay_time = (int) ( (float) 1000000 / flexiget->conf->limit_speed * flexiget->conf->buffer_size * flexiget->conf->how_many );
	}
	if( buffer == NULL )
		buffer = calloc_safe(1, max( MAX_STRING, flexiget->conf->buffer_size ) );

	if( count == 0 )
	{
		flexiget->url = calloc_safe(1, sizeof( url_t ) );
		flexiget->url->next = flexiget->url;
		strcpy( flexiget->url->text, (char *) url );
	}
	else
	{
		res = (search_t *) url;
		u = flexiget->url = calloc_safe(1, sizeof( url_t ) );
		for( i = 0; i < count; i ++ )
		{
			strcpy( u->text, res[i].url );
			if( i < count - 1 )
			{
				u->next = calloc_safe(1, sizeof( url_t ) );
				u = u->next;
			}
			else
			{
				u->next = flexiget->url;
			}
		}
	}

	//Xcell noted
	flexiget->conn[0].conf = flexiget->conf;

	/* Convert an URL to a conn_t structure */
	if( !conn_set( &flexiget->conn[0], flexiget->url->text ) ) {
		flexiget_message( flexiget, _("Could not parse URL.\n") );
		flexiget->ready = -1;
		return( flexiget );
	}

	strcpy( flexiget->filename, flexiget->conn[0].file );
	http_decode( flexiget->filename );	//可能会修改filename
	if( *flexiget->filename == 0 )	/* Index page == no fn		*/
		strcpy( flexiget->filename, flexiget->conf->default_filename );
	if( ( s = strchr( flexiget->filename, '?' ) ) != NULL && flexiget->conf->strip_cgi_parameters )
		*s = 0;		/* Get rid of CGI parameters		*/

	//get name
	if (strlen(flexiget->conf->filename) == 0) {
		strcpy(flexiget->conf->filename, flexiget->filename);	//Xcell
	}

	//Xcell 建立TCP连接
	if( !conn_init( &flexiget->conn[0] ) )
	{
		flexiget_message( flexiget, flexiget->conn[0].message );
		flexiget->ready = -1;
		return( flexiget );
	}

	/* This does more than just checking the file size, it all depends
	   on the protocol used.					*/
	
	//Xcell added some flag
	if (flexiget->conf[0].prefixheader == 1 && GET_NONE == flexiget->conn[0].getting_size) {
		flexiget->conn[0].getting_size = GET_GETTING;
	}
	//end

	/* Get file size and other information					*/
	//Xcell 获取文件大小，建立一次连接，并收到reply
	flexiget->conn[0].conf = flexiget->conf;
	if( !conn_info( &flexiget->conn[0] ) ) {
		flexiget_message( flexiget, flexiget->conn[0].message );
		flexiget->ready = -1;
		return( flexiget );
	}
	//Xcell
	flexiget->conf[0].prefixheader = 0;
	flexiget->conn[0].getting_size = GET_DONE;
	//end
	s = conn_url( flexiget->conn );
	strcpy( flexiget->url->text, s );
	if( ( flexiget->size = flexiget->conn[0].size ) == 0 )
	{
		if( flexiget->conf->verbose > 0 ) {
			flexiget_message( flexiget, _("File size: %u bytes"), flexiget->size );
		}
	}

	/* Wildcards in URL --> Get complete filename			*/
	if( strchr( flexiget->filename, '*' ) || strchr( flexiget->filename, '?' ) )
		strcpy( flexiget->filename, flexiget->conn[0].file );

	return( flexiget );
}

/* Open a local file to store the downloaded data			*/
int flexiget_open( flexiget_t *flexiget )
{
	int i, fd;
	uint64_t mysize;
	int rtn = 0;

	if( flexiget->conf->verbose > 0 )
		flexiget_message( flexiget, _("Opening output file %s"), flexiget->filename );
	snprintf( buffer, MAX_STRING, "%s.st", flexiget->filename );

	flexiget->outfp = NULL;

	/* Check whether server knows about RESTart and switch back to
	   single connection download if necessary			*/
	if( !flexiget->conn[0].supported ) {
		flexiget_message( flexiget, _("Server unsupported, "
					"starting from scratch with one connection.") );
		flexiget->conf->how_many = 1;	//不支持多线程下载
		//Xcell 按线程数据分配数组大小
		flexiget->conn = realloc_safe( flexiget->conn, sizeof( conn_t ) );	

		//Xcell 分配每个下载线程的小任务
		flexiget_divide( flexiget );
	} else if (flexiget->conf->resume_get == 0) {
		unlink(buffer);
		if (strcasecmp(flexiget->filename, "/dev/null") != 0)
			unlink(flexiget->filename);
	} else if( ( fd = open( buffer, O_RDONLY ) ) != -1 ) {
		//Xcell 从临时文件获取下载任务的断点续传信息，并初始化每一个线程的任务信息
		rtn = read( fd, &flexiget->conf->how_many, sizeof( flexiget->conf->how_many ) );
		if (rtn <= 0) {
			fprintf(stderr, "failed to load state file\n");
			close( fd );
			return -1;
		}

		flexiget->conn = realloc_safe( flexiget->conn, sizeof( conn_t ) * flexiget->conf->how_many );
		memset( flexiget->conn + 1, 0, sizeof( conn_t ) * ( flexiget->conf->how_many - 1 ) );

		flexiget_divide( flexiget );

		//FIXME for read failed
		rtn = read( fd, &flexiget->bytes_done, sizeof( flexiget->bytes_done ) );
		if (rtn <= 0) {
			fprintf(stderr, "failed to load state file\n");
			close( fd );
			return -1;
		}

			
		for( i = 0; i < flexiget->conf->how_many; i ++ ) {
			rtn = read( fd, &flexiget->conn[i].currentbyte, sizeof( flexiget->conn[i].currentbyte ) );
			if (rtn <= 0) {
				fprintf(stderr, "failed to load state file\n");
				close( fd );
				return -1;
			}
		}
		flexiget_message( flexiget, _("State file found: %u bytes downloaded, %u to go."),
				flexiget->bytes_done, flexiget->size - flexiget->bytes_done );

		close( fd );

	//	if( ( flexiget->outfp = fopen( flexiget->filename, "wb" ) ) == NULL ) {
	//		flexiget_message( flexiget, _("Error opening local file") );
	//		return( 0 );
	//	}
	//	add by sxlxb 1224
		int  j = 0;
		while ( (( flexiget->outfp = fopen( flexiget->filename, "wb" ) ) == NULL ) && (j < 5))
			j ++;
		if (flexiget->outfp == NULL)
			return 0;
	}

	/* If outfd == -1 we have to start from scrath now	
	 * Xcell 新任务，没有status文件	*/
	if( flexiget->outfp == NULL ) {
		flexiget_divide( flexiget );

    //		if( ( flexiget->outfp = fopen( flexiget->filename, "wb" ) ) == NULL ) {
	//		flexiget_message( flexiget, _("Error opening local file") );
	//		return( 0 );
	//	}
	//	add by sxlxb 1224
		int  j = 0;
		while ( (( flexiget->outfp = fopen( flexiget->filename, "wb" ) ) == NULL ) && (j < 5))
			j ++;
		if (flexiget->outfp == NULL)
			return 0;


		/* And check whether the filesystem can handle seeks to
		   past-EOF areas.. Speeds things up. :) AFAIK this
		   should just not happen:				*/
		if( fseeko( flexiget->outfp, flexiget->size, SEEK_SET ) == -1 
											&& flexiget->conf->how_many > 1 ) {
			/* But if the OS/fs does not allow to seek behind
			   EOF, we have to fill the file with zeroes before
			   starting. Slow..				*/
			flexiget_message( flexiget, _("Crappy filesystem/OS.. Working around. :-(") );
			fseeko( flexiget->outfp, 0, SEEK_SET );
			memset( buffer, 0, flexiget->conf->buffer_size );
			mysize = flexiget->size;
			while( mysize > 0 ) {
				//FIXME
				int numwrite = 0;
				numwrite = safe_fwrite(buffer, min( mysize, flexiget->conf->buffer_size ), flexiget->outfp );
				if (numwrite < 0) {
					fprintf(stderr, "failed to write download file\n");
			    	return -1;
				}	
				mysize -= flexiget->conf->buffer_size;
			}
		}
	}

	return( 1 );
}

/* Start downloading							*/
void flexiget_start( flexiget_t *flexiget )
{
	int i;

	/* HTTP might've redirected and FTP handles wildcards, so
	   re-scan the URL for every conn				*/
	for( i = 0; i < flexiget->conf->how_many; i ++ ) {
		conn_set( &flexiget->conn[i], flexiget->url->text );
		flexiget->url = flexiget->url->next;
		flexiget->conn[i].conf = flexiget->conf;
		flexiget->conn[i].supported = 1;
	}

	if( flexiget->conf->verbose > 0 )
		flexiget_message( flexiget, _("Starting download") );

	for( i = 0; i < flexiget->conf->how_many; i ++ )
		if( flexiget->conn[i].currentbyte <= flexiget->conn[i].lastbyte )
		{
			if( flexiget->conf->verbose >= 2 )
				flexiget_message( flexiget, _("Connection %i downloading from %s:%i"),
						i, flexiget->conn[i].host, flexiget->conn[i].port);
			//Xcell 建立小任务线程setup_thread，线程中建立tcp,send request, recv reply
			if( pthread_create( flexiget->conn[i].setup_thread, NULL, setup_thread, &flexiget->conn[i] ) != 0 )
			{
				flexiget_message( flexiget, _("pthread error!!!") );
				flexiget->ready = -1;
			}
			else
			{
				flexiget->conn[i].last_transfer = gettime();
				flexiget->conn[i].state = 1;
			}
		}

	/* The real downloading will start now, so let's start counting	*/
	flexiget->start_time = gettime();
	flexiget->ready = 0;
}

/* Main 'loop'								*/
/*
 * Xcell
 *   在该线程中统计使用select监视所有线程中的tcp，并在该线程中统一接收reply
 */
void flexiget_do( flexiget_t *flexiget )
{
	fd_set fds[1];
	int hifd, i;
	uint64_t j = 0;
	int size = 0;
	struct timeval timeval[1];

	/* Create statefile if necessary				*/
	if( gettime() > flexiget->next_state )
	{
		//Xcell 保存当前下载的状态信息到xxxxxx.st文件，里面有各线程的信息
		save_state( flexiget );
		flexiget->next_state = gettime() + flexiget->conf->save_state_interval;
	}

	/* Wait for data on (one of) the connections			*/
	/* Xcell
	 * 	统一使用select临控所有的线程中建立的tcp连接
	 */
	FD_ZERO( fds );
	hifd = 0;
	for( i = 0; i < flexiget->conf->how_many; i ++ )
	{	
		if(flexiget->conn[i].fd > 2 ) {
			if( flexiget->conn[i].enabled )
				FD_SET( flexiget->conn[i].fd, fds );
			else {
		//		fprintf(stderr, "enable=0, but:flexiget->conn[i].fd=%d\n", flexiget->conn[i].fd);
		//		flexiget->conn[i].fd = -1;
			}
			hifd = max( hifd, flexiget->conn[i].fd );
		}
	}
	if( hifd == 0 )
	{
		/* No connections yet. Wait...				*/
		usleep( 100000 );
		goto conn_check;
	}
	else
	{
		timeval->tv_sec = 0;
		timeval->tv_usec = 100000;
		/* A select() error probably means it was interrupted
		   by a signal, or that something else's very wrong...	*/
		if( select( hifd + 1, fds, NULL, NULL, timeval ) == -1 )
		{
			flexiget->ready = -1;
			return;
		}
	}

	/* Handle connections which need attention			*/
	for( i = 0; i < flexiget->conf->how_many; i ++ ) {
		if( flexiget->conn[i].enabled && flexiget->conn[i].fd > 2 ) {
			if( FD_ISSET( flexiget->conn[i].fd, fds ) )
			{
				flexiget->conn[i].last_transfer = gettime();
				size = read( flexiget->conn[i].fd, buffer, flexiget->conf->buffer_size );
				if( size == -1 )
				{
					if( flexiget->conf->verbose )
					{
						flexiget_message( flexiget, _("Error on connection %i! "
									"Connection closed"), i );
					}
					flexiget->conn[i].enabled = 0;
					conn_disconnect( &flexiget->conn[i] );
					continue;
				} else if( size == 0 ) {
					if( flexiget->conf->verbose )
					{
						/* Only abnormal behaviour if:		*/
						if( flexiget->conn[i].currentbyte < flexiget->conn[i].lastbyte && flexiget->size != 0 )
						{
							//对于连接中断的错误，应该重新建立连接并重新下载
							//似乎在conn_check中是这样处理的
							flexiget_message( flexiget, _("Connection %i unexpectedly closed"), i );
						} else {
							flexiget_message( flexiget, _("Connection %i finished"), i );
						}
						//printf("XCELL flexiget->size=%u\n", flexiget->size);
					}
					if( !flexiget->conn[0].supported )
					{
						flexiget->ready = 1;
					}
					flexiget->conn[i].enabled = 0;
					conn_disconnect( &flexiget->conn[i] );
					continue;
				}
				/* j == Bytes to go					*/
				j = flexiget->conn[i].lastbyte - flexiget->conn[i].currentbyte + 1;
				if( j < size )
				{
					if( flexiget->conf->verbose )
					{
						flexiget_message( flexiget, _("Connection %i finished"), i );
					}
					flexiget->conn[i].enabled = 0;
					conn_disconnect( &flexiget->conn[i] );
					size = j;
					/* Don't terminate, still stuff to write!	*/
				}
				/* This should always succeed..				*/
				/*
				 * Xcell 对于某一个TCP，利用其小任务信息获取offset，写同一个目标文件
				 */
				int rtn = fseeko(flexiget->outfp, flexiget->conn[i].currentbyte, SEEK_SET);
				rtn = safe_fwrite( buffer, size, flexiget->outfp );
				int mytemp = 0;
				while ((rtn == -1) && (mytemp < 5))
				{
					rtn = safe_fwrite( buffer, size, flexiget->outfp );
					mytemp ++;
					sleep(mytemp);
				}
				
				if (rtn != size ) {
					//FIXME:	对于没有写成功的，应该重试
					fprintf(stderr, "safe_write failed:rtn=%d, size=%u, errno=%d\n", rtn, size, errno);
					flexiget_message( flexiget, _("Write error!") );
					flexiget->ready = -1;
					return;
				}
				flexiget->conn[i].currentbyte += size;
				flexiget->bytes_done += size;
			} else {
				if( gettime() > flexiget->conn[i].last_transfer + flexiget->conf->connection_timeout )
				{
					if( flexiget->conf->verbose )
						flexiget_message( flexiget, _("Connection %i timed out"), i );
					conn_disconnect( &flexiget->conn[i] );
					flexiget->conn[i].enabled = 0;
				}
			}
		}
	}

	if( flexiget->ready )
		return;

conn_check:
	/* Look for aborted connections and attempt to restart them.	*/
	for( i = 0; i < flexiget->conf->how_many; i ++ )
	{
		if( !flexiget->conn[i].enabled && flexiget->conn[i].currentbyte < flexiget->conn[i].lastbyte )
		{
			if( flexiget->conn[i].state == 0 )
			{
				conn_set( &flexiget->conn[i], flexiget->url->text );
				flexiget->url = flexiget->url->next;
				/* flexiget->conn[i].local_if = flexiget->conf->interfaces->text;
				   flexiget->conf->interfaces = flexiget->conf->interfaces->next; */
				if( flexiget->conf->verbose >= 2 )
					flexiget_message( flexiget, _("Connection %i downloading from %s:%i"),
							i, flexiget->conn[i].host, flexiget->conn[i].port);
				//Xcell 对于下载失败的线程，重新下载
				if( pthread_create( flexiget->conn[i].setup_thread, NULL, setup_thread, &flexiget->conn[i] ) == 0 )
				{
					flexiget->conn[i].state = 1;
					flexiget->conn[i].last_transfer = gettime();
				}
				else
				{
					flexiget_message( flexiget, _("pthread error!!!") );
					flexiget->ready = -1;
				}
			}
			else
			{
				if( gettime() > flexiget->conn[i].last_transfer + flexiget->conf->reconnect_delay )
				{
					pthread_cancel( *flexiget->conn[i].setup_thread );
					flexiget->conn[i].state = 0;
				}
			}
		}
	}

	/* Calculate current average speed and finish_time		*/
	//Xcell 统计下载信息，并进行速度限制
	flexiget->bytes_per_second = (int) ( (double) ( flexiget->bytes_done - flexiget->start_byte ) / ( gettime() - flexiget->start_time ) + 1 ); //add by sxlxb 1224
	flexiget->finish_time = (int) ( flexiget->start_time + (double) ( flexiget->size - flexiget->start_byte ) / flexiget->bytes_per_second + 1); 

	/* Check speed. If too high, delay for some time to slow things
	   down a bit. I think a 5% deviation should be acceptable.	*/
	//Xcell 限速
	if( flexiget->conf->limit_speed > 0 )
	{
		if( (float) flexiget->bytes_per_second / flexiget->conf->limit_speed > 1.05 )
			flexiget->delay_time += 10000;
		else if( ( (float) flexiget->bytes_per_second / flexiget->conf->limit_speed < 0.95 ) && ( flexiget->delay_time >= 10000 ) )
			flexiget->delay_time -= 10000;
		else if( ( (float) flexiget->bytes_per_second / flexiget->conf->limit_speed < 0.95 ) )
			flexiget->delay_time = 0;
		usleep( flexiget->delay_time );
	}

	/* Ready?							*/
	//Xcell 下载完成的判断，后面会删除临时文件,否则临时文件一直会存大，以供断点续传
	if( flexiget->bytes_done == flexiget->size )
		flexiget->ready = 1;
}

/* Close an flexiget connection						*/
void flexiget_close( flexiget_t *flexiget )
{
	int i;
	message_t *m;

	/* Terminate any thread still running				*/
	//FIXME:可能flexiget->conn[i].setup_thread == NULL，内存错误，因为线程可能已经退出
	//当任务完成或主线程中止，将其它线程关闭
	for( i = 0; i < flexiget->conf->how_many; i ++ ) {
		//Xcell
		/*
		if (flexiget->conn[i].setup_thread)
			pthread_cancel( *flexiget->conn[i].setup_thread ); */
	}

	/* Delete state file if necessary				*/
	if( flexiget->ready == 1 || strcasecmp(flexiget->filename, "/dev/null") == 0)
	{
		snprintf( buffer, MAX_STRING, "%s.st", flexiget->filename );
		unlink( buffer );
	}
	/* Else: Create it.. 						*/
	else if( flexiget->bytes_done > 0 )
	{
		//Xcell 任务没有完成，而主线程中止时,保存已经下载的信息，供断点续传
		save_state( flexiget );
	}

	/* Delete any message not processed yet				*/
	while( flexiget->message )
	{
		m = flexiget->message;
		flexiget->message = flexiget->message->next;
		safe_free( m );
	}

	/* Close all connections and local file				*/
	if (flexiget->outfp)
		fclose( flexiget->outfp );
	for( i = 0; i < flexiget->conf->how_many; i ++ )
		conn_disconnect( &flexiget->conn[i] );

	safe_free( flexiget->conn );
	safe_free( flexiget );
}

/* time() with more precision						*/
double gettime()
{
	struct timeval time[1];

	gettimeofday( time, 0 );
	return( (double) time->tv_sec + (double) time->tv_usec / 1000000 );
}

/* Save the state of the current download				*/
void save_state( flexiget_t *flexiget )
{
	int fd, i;
	char fn[MAX_STRING+4];

	/* No use for such a file if the server doesn't support
	   resuming anyway..						*/
	if( !flexiget->conn[0].supported )
		return;
	//Xcell 把状态信息存放到xxxxx.st文件,每次写都清空文件后写

	if (strcasecmp(flexiget->filename, "/dev/null") != 0)
		snprintf( fn, MAX_STRING, "%s.st", flexiget->filename );
	else
		snprintf( fn, MAX_STRING, "%s", "/dev/null");
		
	if( ( fd = open( fn, O_CREAT | O_TRUNC | O_WRONLY, 0666 ) ) == -1 )
	{
		return;		/* Not 100% fatal..			*/
	}
	write( fd, &flexiget->conf->how_many, sizeof( flexiget->conf->how_many ) );
	write( fd, &flexiget->bytes_done, sizeof( flexiget->bytes_done ) );
	for( i = 0; i < flexiget->conf->how_many; i ++ )
	{
		//Xcell 依次保存每个线程的任务信息
		write( fd, &flexiget->conn[i].currentbyte, sizeof( flexiget->conn[i].currentbyte ) );
	}
	//Xcell 写完状态信息后，关闭文件
	close( fd );
}

/* Thread used to set up a connection					*/
void *setup_thread( void *c )
{
	conn_t *conn = c;
	int oldstate;

	/* Allow this thread to be killed at any time.			*/
	pthread_setcancelstate( PTHREAD_CANCEL_ENABLE, &oldstate );
	pthread_setcanceltype( PTHREAD_CANCEL_ASYNCHRONOUS, &oldstate );

	if( conn_setup( conn ) )	//Xcell 组织Request Header并建立TCP
	{
		conn->last_transfer = gettime();
		if( conn_exec( conn ) )		//Xcell 发送Request
		{
			conn->last_transfer = gettime();
			conn->enabled = 1;
			conn->state = 0;
			//Xcell 线程退出，但已经建立的TCP没有关闭，由flexiget_do统一管理
			return( NULL );
		}
	}

	conn_disconnect( conn );
	conn->state = 0;
	return( NULL );
}

/* Add a message to the flexiget->message structure				*/
static void flexiget_message( flexiget_t *flexiget, char *format, ... )
{
	message_t *m = calloc_safe(1, sizeof( message_t ) ), *n = flexiget->message;
	va_list params;

	memset( m, 0, sizeof( message_t ) );
	va_start( params, format );
	vsnprintf( m->text, MAX_STRING, format, params );
	va_end( params );

	if( flexiget->message == NULL )
	{
		flexiget->message = m;
	}
	else
	{
		while( n->next != NULL )
			n = n->next;
		n->next = m;
	}
}

/* Divide the file and set the locations for each connection		*/
/* Xcell
 * 按照线程数据来分配每个线程的下载Range
 * 如果我们要修改下载策略，则可能从这里下手
 */
static void flexiget_divide( flexiget_t *flexiget )
{
	int i;

	flexiget->conn[0].currentbyte = 0;

	//Xcell add
	flexiget->conn[0].conf = &flexiget->conf[0];
	//end
	//XXX: flexiget->siz是谁赋的值??
	flexiget->conn[0].lastbyte = flexiget->size / flexiget->conf->how_many - 1;
	for( i = 1; i < flexiget->conf->how_many; i++) {
#ifdef DEBUG
		printf( "Downloading %i-%i using conn. %i\n", flexiget->conn[i-1].currentbyte, flexiget->conn[i-1].lastbyte, i - 1 );
#endif
		flexiget->conn[i].currentbyte = flexiget->conn[i-1].lastbyte + 1;
		flexiget->conn[i].lastbyte = flexiget->conn[i].currentbyte + flexiget->size / flexiget->conf->how_many;
		//Xcell add
		//XXX:断开的连接重联是否没有再赋值？
		flexiget->conn[i].conf = &flexiget->conf[0];	
		//end
	}
	flexiget->conn[flexiget->conf->how_many-1].lastbyte = flexiget->size - 1;
#ifdef DEBUG
	printf( "Downloading %i-%i using conn. %i\n", flexiget->conn[i-1].currentbyte, flexiget->conn[i-1].lastbyte, i - 1 );
#endif
}



