#include "flexiget.h"


char fn[MAX_STRING] = "";

static void stop( int signal );
static char *size_human( uint64_t value );
static char *time_human( int value );
static void print_commas( int bytes_done );
static void print_help();
static void print_version();
static void print_messages( flexiget_t *flexiget );

int run = 1;

#ifdef NOGETOPTLONG
#define getopt_long( a, b, c, d, e ) getopt( a, b, c )
#else
static struct option flexiget_options[] =
{
    /* name         has_arg flag    val */
    { "limit-speed",        1,  NULL,   'L' },  
    { "how_many",   1,  NULL,   'm' },  
    { "output-document",        1,  NULL,   'O' },  
    { "print_reply",     0,  NULL,   'S' },  
    { "no-proxy",       0,  NULL,   'N' },  
    { "quiet",      0,  NULL,   'q' },  
    { "verbose",        0,  NULL,   'v' },  
    { "help",       0,  NULL,   'h' },  
    { "version",        0,  NULL,   'V' },  
    { "header", 1, NULL, 'H'}, 
    { "socket", 1, NULL, 's'},
    { "continue", 0, NULL, 'c'},
    { NULL, 0, NULL, 'o'},
    { "prefixheader", 0, NULL, 'P'},
	{"delegate-program", 0, NULL, 'd'},
	{"retry",	1,	NULL,	'r'},
    { NULL,         0,  NULL,   0 }
	
};
#endif


void print_version()
{
    printf( _("flexiget version %s (%s)\n"), flexiget_VERSION_STRING, ARCH ); 
    printf( "\nCopyright 2007 ChinaCache\n" );
}


/* For returning string values from functions				*/
static char string[MAX_STRING];


void print_process(flexiget_t * flexiget, uint64_t prev)
{
	static pid_t pid = 0;
    unsigned int  rate;
	
	if (flexiget->conf->verbose > 0) {
		time_t now = time(NULL);
		static time_t last_print = 0;

		if (pid == 0)
			pid = getpid();

		if (now - last_print > 1) {
			//yangrb added
			rate = (unsigned int) prev/(gettime() - flexiget->start_time + 1);  //add by sxlxb 1224
			rate = (unsigned int)rate / 1024;
			debug( "[pid=%d][%3u%%]  [%6.1fKB/s]\n", pid, (unsigned int)min(100, prev / ((flexiget->size/100) + 1)), rate );//modify 1224
			last_print = now;
			//end added
		}
	}
}



int init_conf(conf_t * conf, int argc, char ** argv)
{
	debug_ouputfp = stderr;
	while( 1 )
	{
		int option;

		option = getopt_long( argc, argv, "o:cPL:s:m:O:SNqvhr:H:Vd", flexiget_options, NULL );
		if( option == -1 )
			break;

		switch( option )
		{
			case 'L':
				if( !sscanf( optarg, "%i", &conf->limit_speed ) )
				{
					print_help();
					return( 1 );
				}
				break;
			case 'm':
				if( !sscanf( optarg, "%i", &conf->how_many ) )
				{
					print_help();
					return( 1 );
				}
				break;
			case 'O':
				strncpy( fn, optarg, MAX_STRING );
				strncpy(conf->filename, fn, MAX_STRING);	
				break;
			case 'S':
				conf->print_reply = 1;				
				break;
			case 'c':
				conf->resume_get = 1;
				break;
			case 'o':
				debug_ouputfp = fopen(optarg, "w+");
				break;
			case 'N':
				*conf->http_proxy = 0;
				break;
			case 'h':
				print_help();
				return( 0 );
			case 'v':
				debug_level = 1;
				break;
			case 'V':
				print_version();
				return( 0 );
		    case 'd':
				conf->handle_header = 1;
		   		break;		
			case 'q':
				close( 1 );
				conf->verbose = -1;
				if( open( "/dev/null", O_WRONLY ) != 1 )
				{
					fprintf( stderr, _("Can't redirect stdout to /dev/null.\n") );
					return( 1 );
				}
				break;
			case 'H': 
				if(NULL == optarg) {
					print_help();
					return 1;
				}
				if (strlen(conf->headers) != 0) 
					strcat(conf->headers, "\r\n");
				strcat(conf->headers, optarg);
				break;
			case 'r':
				conf->retry = atoi(optarg);
				break;	
			case 's':
				if( optarg != NULL ) {
					if( !sscanf( optarg, "%s", conf->dst_ip ) )
					{
						print_help();
						return( 1 );
					}
				}
				break;
			case 'P':
				conf->prefixheader = 1;
				break;

			default:
				print_help();
				return( 1 );
		}
	}
	return 0;
}


int main( int argc, char *argv[] )
{
	conf_t conf[1];
	flexiget_t *flexiget=NULL;
	int i = 0;
	char *s=NULL;

	if( !conf_init( conf ) )
		return( 1 );

	opterr = 0;
	debug_level = 0;

	//Xcell 参数支持
	init_conf(conf, argc, argv);

	if( debug_level > -1 )
		conf->verbose = 1;
	else 
		conf->verbose = 0;

	if( argc - optind == 0 ) {
		print_help();
		return( 1 );
	} else if( strcmp( argv[optind], "-" ) == 0 ) {
		s = calloc_safe(1, MAX_STRING );
		scanf( "%127[^\n]s", s );
	} else {
		s = argv[optind];
	}

	debug("Initializing download: %s\n", s );
	if( argc - optind == 1 )
	{
		//Xcell 建立一个多线程下载任务
		//我们使用本方法
		flexiget = flexiget_new( conf, 0, s );
		if( flexiget->ready == -1 )
		{
			print_messages( flexiget );
			flexiget_close( flexiget );
			return( 1 );
		}
	}
	print_messages( flexiget );
	if( s != argv[optind] )
		safe_free( s );

	/*
	 * Xcell
	 * 		以下代码完成设置flexiget->filename，即下载的文件写向何处
	 */
	if( fn && strlen(fn)!=0 ) {
		struct stat buf;

		if( stat( fn, &buf ) == 0 ) {
			if( S_ISDIR( buf.st_mode ) ) {
				strncat( fn, "/", MAX_STRING );
				strncat( fn, flexiget->filename, MAX_STRING );
			}
		}
		sprintf( string, "%s.st", fn );
		if( access( fn, F_OK ) == 0 ) {
			if( access( string, F_OK ) != 0 ) {
				//Xcell modify 
				//到这里说明文件已经存在，且可能下载完成，但即使文件存在，覆盖它重新下载
				if (strcasecmp(fn, "/dev/null") != 0)
					unlink(fn);
			}
		}
		if( access( string, F_OK ) == 0 ) {
			if( access( fn, F_OK ) != 0 ) {
				//存在state文件，但没有实际数据文件，删除它
				fprintf(stderr, "State file found, but no downloaded data. Starting from scratch.\n" );
				unlink( string );
			}
		}
		strcpy( flexiget->filename, fn );
	} else {
		/* Local file existence check					*/
		i = 0;
		s = flexiget->filename + strlen( flexiget->filename );
		while( 1 ) {
			sprintf( string, "%s.st", flexiget->filename );
			if (flexiget->conf->resume_get == 0) {
				unlink(string);
				if (strcasecmp(flexiget->filename, "/dev/null") != 0)
					unlink(flexiget->filename);
				break;
			}
			if( access( flexiget->filename, F_OK ) == 0 ) {
				if( flexiget->conn[0].supported ) {
					if( access( string, F_OK ) == 0 )
						break;
				}
			} else {
				if( access( string, F_OK ) )
					break;
			}
			sprintf( s, ".%i", i );
			i ++;
		}
	}
	strncpy(conf->filename, flexiget->filename, MAX_STRING);	

	//Xcell 下载任务线程信息建立
	if(flexiget_open( flexiget ) <= 0) //add by sxlxb 1224
	{
		print_messages( flexiget );
		return( 1 );
	}
	print_messages( flexiget );

	//Xcell 建立多线程TCP
	flexiget_start( flexiget );
	print_messages( flexiget );

	if( flexiget->bytes_done > 0 ) {
		/* Print first dots if resuming	*/
		if (conf->verbose) {
			debug("\n");
			print_commas( flexiget->bytes_done );
		}
	}
	flexiget->start_byte = flexiget->bytes_done;

	/* Install save_state signal handler for resuming support	*/
	signal( SIGINT, stop );
	signal( SIGTERM, stop );

	while( !flexiget->ready && run ) {
		unsigned int prev;

		prev = flexiget->bytes_done;

		//Xcell 主循环中反复接收多个连接的TCP reply
		flexiget_do( flexiget );

		/*
		 * Xcell
		 * 	以下为外围操作，主要是负责向用户输出信息
		 * 	以方便让用户了解下载状态
		 */
		print_process(flexiget, flexiget->bytes_done);
	}

	strcpy( string + MAX_STRING / 2,
			size_human( flexiget->bytes_done - flexiget->start_byte ) );

	debug("\nDownloaded %s in %s. (%.2f KB/s)\n",
			string + MAX_STRING / 2,
			time_human( gettime() - flexiget->start_time ),
			(double) flexiget->bytes_per_second / 1024 );

	i = flexiget->ready ? 0 : 2;
	
	if (flexiget->ready == 1)
		printf("flexiget download ok!\n");
	else
		printf("flexiget download failed!\n");
		
	//Xcell 下载任务完成
	flexiget_close( flexiget );

	return( i );
}

/* SIGINT/SIGTERM handler						*/
void stop( int signal )
{
	run = 0;
}

/* Convert a number of bytes to a human-readable form			*/
char *size_human( uint64_t value )
{
	if( value == 1 )
		sprintf( string, _("%" PRINTF_UINT64_T " byte"), value );
	else if( value < 1024 )
		sprintf( string, _("%" PRINTF_UINT64_T " bytes"), value );
	else if( value < 10485760 )
		sprintf( string, _("%.1f kilobytes"), (float) value / 1024 );
	else
		sprintf( string, _("%.1f megabytes"), (float) value / 1048576 );

	return( string );
}

/* Convert a number of seconds to a human-readable form			*/
char * time_human( int value )
{
	if( value == 1 )
		sprintf( string, _("%i second"), value );
	else if( value < 60 )
		sprintf( string, _("%i seconds"), value );
	else if( value < 3600 )
		sprintf( string, _("%i:%02i seconds"), value / 60, value % 60 );
	else
		sprintf( string, _("%i:%02i:%02i seconds"), value / 3600, ( value / 60 ) % 60, value % 60 );

	return( string );
}

/* Part of the infamous wget-like interface. Just put it in a function
   because I need it quite often..					*/
void print_commas( int bytes_done )
{
	int i, j;

	j = ( bytes_done / 1024 ) % 50;
	if( j == 0 ) 
		j = 50;
	
	for( i = 0; i < j; i ++ ) {
		if( ( i % 10 ) == 0 ) {
			debug( " " );
		}
		debug( "," );
	}
	fflush( stdout );
}


static void
print_help (void)
{
 //  We split the help text this way to ease translation of individual
    // entries. 
  static const char *help[] = {
    "\n",
    "Usage: flexiget [options] url\n",
    ("\
Mandatory arguments to long options are mandatory for short options too.\n\n"),
   ("\
Startup:\n"),
   ("\
  -V,  --version                display the version of flexiget and exit.\n"),
   ("\
  -h,  --help                   print this help and exit.\n"),
   ("\
  -o,                           output is redirected to ./flexiget.log.\n"),
   ("\
  -D,  --debug                  turn on debug output, don't print any debug info unless requested with -d.\n"),
   ("\
  -v,  --verbose                turn on verbose output, with all the available data.The default output is verbose.\n"),
   ("\
  -O,  --output-document=file   user can assigne the filename\n"),
   ("\
  -S,  --server-response        Print the headers sent by HTTP servers.\n"),
   ("\
  -p,  --prefix-server-reponse  Prefix the server headers to output-document.\n"),
   ("\
  -d   --delegate-program       point to program which deel with server-response-header.\n"),
   ("\
       --limit-rate             Limit the download speed to amount bytes per second.\n"),
   ("\
  -m   --how-many               how many threads to run  in parallel.\n"),
   ("\
       --header                 Send header-line along with the rest of the headers in each HTTP request.\n"),
   ("\
  -c   --continue               Continue getting a partially-downloaded file.\n"),
   ("\
  -t   --tries                  Set number of retries to number.\n"),
   ("\
  -s   --socket                 assign the IP address and port number.\n"),
   ("\
       --return-code            erifying the return code,if match the return-code list, success.else,failed.\n")
};

    int i;

    for (i = 0; i < 20; i++)
    fputs ((help[i]), stdout);

    exit (0);
}


/* Print any message in the flexiget structure				*/
void print_messages( flexiget_t *flexiget )
{
	message_t *m;

	while( flexiget->message )
	{
		if (flexiget->conf->verbose)
			debug( "%s\n", flexiget->message->text );
		m = flexiget->message;
		flexiget->message = flexiget->message->next;
		safe_free( m );
	}
}
