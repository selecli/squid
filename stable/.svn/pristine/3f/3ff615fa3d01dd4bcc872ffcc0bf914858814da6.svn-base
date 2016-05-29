/*
 * =====================================================================================
 *
 *       Filename:  main.c
 *
 *    Description:  analyse date file.
 *
 *        Created:  03/15/2012 03:52:22 PM
 *       Compiler:  gcc
 *         Author:  Xu Jian (XJ), xujian@chinacache.com
 *        Company:  ChinaCache
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <arpa/inet.h>
#include <math.h>
#include <time.h>
#include <getopt.h>
#include <dirent.h>
#include <errno.h>
#include <sys/file.h>
#include "dbg.h"
#include "framework.h"
#include "version.h"
#include "shortest_path.h"
#ifndef INFINITY
#define INFINITY (1e308)                       /* for old gcc */
#endif /* not INFINITY */
const char kLINK_DATA_DIR[] = "/var/log/chinacache/linkdata/";
const char kLINK_DATA_PREFIX[] = "link.data.";  /* link.data.<timestamp> */
//used for debug
int debug_expire_time = -1;
char *debug_link_data_file = NULL;
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  SortFileByCTimeDescending
 *  Description:
 * =====================================================================================
 */
    static int
SortFileByCTimeDescending( const struct dirent **a, const struct dirent **b )
{
    int ta, tb;
    static const size_t kSIZE = sizeof( kLINK_DATA_PREFIX ) - 1;
    char *p;
    ta = strtol( (*a)->d_name + kSIZE, &p, 10 );
    if( *p != '\0' )
        return 0;
    tb = strtol( (*b)->d_name + kSIZE, &p, 10 );
    if( *p != '\0' )
        return 0;
    return tb - ta;
} /* - - - - - - - end of function SortFileByCTimeDescending - - - - - - - - - - - */
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  LinkDataFileNameFilter
 *  Description:  Get file name with right format within kEXPIRE_TIME seconds.
 * =====================================================================================
 */
    static int
LinkDataFileNameFilter( const struct dirent *d )
{
    int ret;
    time_t t, expire;
    char *p;
    const char *name = d->d_name;
    static const size_t kSIZE = sizeof( kLINK_DATA_PREFIX ) - 1;
    static const int kEXPIRE_TIME = 86400;      /* do not read file 24 hours before */
    ret = strncmp( name, kLINK_DATA_PREFIX, kSIZE );
    if( ret )
        return 0;
    t = (time_t)strtol( d->d_name + kSIZE, &p, 10 );
    if( *p != '\0' )
        return 0;
    expire = time(0) - kEXPIRE_TIME;
    if( debug_expire_time != -1 )
        expire = debug_expire_time;
    if( t < expire )
    {
        Debug( 50, "File %s expired, since expire time is %u.", d->d_name, expire );
        return 0;
    }
    return 1;
} /* - - - - - - - end of function LinkDataFileNameFilter - - - - - - - - - - - */
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  ShowHelp
 *  Description:
 * =====================================================================================
 */
static int
ShowHelp( void )
{
    //static char short_option[] = ":d:hl:T:v";
    printf( "Usage: /usr/local/detectorig/sbin/analyse\n"
            "Options:\n"
            " -d, --debug LEVEL:    Set debug level, with which increase the info increases\n"
            " -h, --help:           Show this help\n"
            " -s, --stdout:         Show debug info in stderr\n"
            " -l, --link-data FILE: Read link data from FILE\n"
            " -T, --expire-time TIMESTAMP:  Set expire time for searching link data\n"
            " -v, --version:        Show version\n"
          );
    return 0;
} /* - - - - - - - end of function ShowHelp - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ParseArgument
 *  Description:  0, continue
 *                1, program should exit
 * =====================================================================================
 */
    static int
ParseArgument( int argc, char *argv[] )
{
    char c;
    int option_index;
    static char short_option[] = ":sd:hl:T:v";
    static struct option long_option[] = {
        {"debug",       required_argument, 0, 'd'},
        {"help",        no_argument, 0, 'h'},
        {"stdout",      no_argument, 0, 's'},
        {"link-data",   required_argument, 0, 'l'},
        {"expire-time", required_argument, 0, 'T'},
        {"version",     no_argument, 0, 'v'},
        {NULL, 0, NULL, 0 }
    };
    opterr = 0;
    while(( c = getopt_long( argc, argv, short_option, long_option, &option_index )) != -1 )
    {
        switch( c )
        {
            case 'd':
                {
                    int debug_level;
                    char *p;
                    debug_level = strtol( optarg, &p, 10 );
                    if( *p != '\0' )
                    {
                        fprintf( stderr, "Unknown option \"%s\" for --debug/-d\n", optarg );
                        return 1;
                    }
                    SetDebugLevel( debug_level );
                }
                break;
            case 'h':
                ShowHelp();
                return 1;
                break;
            case 's':
                SetDebugOutputToStderr();
                break;
            case 'l':
                debug_link_data_file = optarg;
                break;
            case 'T':
            {
                int t;
                char *p;
                t = strtol( optarg, &p, 10 );
                if( *p != '\0' )
                {
                    fprintf( stderr, "Unknown option \"%s\" for --expire-time/-T\n", optarg );
                    return 1;
                }
                if( t >= 0 )
                    debug_expire_time = t;
            }
            break;
            case 'v':
                printf( "analyse %s version %s.%s, %s %s\n"\
                        "gcc version %s\n"\
                        "cmake version %s\n"\
                        "built in %s, %s\n",
                        CMAKE_BUILD_TYPE, VERSION_MAJOR, VERSION_MINOR,
                        __DATE__, __TIME__,
                        __VERSION__,
                        CMAKE_VERSION,
                        OS_TYPE, CMAKE_SYSTEM);
                return 1;
                break;
            case ':':
                /*  missing option */
                fprintf( stderr, "Missing option in command line.\n" );
                return 1;
                break;
            case '?':
                /*  encounters an option that was not in optstring */
                /*  missing option */
                fprintf( stderr, "Unknown option in command line.\n" );
                return 1;
                break;
            case 0:
                /* Get, but flag is NULL */
                break;
            default:
                fprintf( stderr, "Unknown error in %s.\n", __FUNCTION__ );
                break;
        }
    }
    return 0;
} /* - - - - - - - end of function ParseArgument - - - - - - - - - - - */
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  FreeDataInfo
 *  Description:
 * =====================================================================================
 */
    static int
FreeDataInfo( DataInfo *data )
{
    assert( data );
    if( data->hostname != NULL )
        free( data->hostname );
    memset( data, 0, sizeof(DataInfo) );
    return 0;
} /* - - - - - - - end of function FreeDataInfo - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ParseLine
 *  Description:  Do malloc(), must to call FreeDataInfo() when data is no more need
 * =====================================================================================
 */
    static int
ParseLine( DataInfo *data, char *line )
{
    assert( data );
    assert( line );
    typedef struct _ParseLineDataType
    {
        enum
        {
            kPARSE_UINT16,
            kPARSE_UINT32,
            kPARSE_STRING,
            kPARSE_DOUBLE,
            kPARSE_IP,
            kPARSE_PORT_STATUS,
            kPARSE_DETECT_TYPE,
            kPARSE_IP_TYPE,
            /* other */
            kPARSE_NULL,
            kPARSE_END,
            kPARSE_RESERVE
        } type;
        void *data;
    } ParseLineDataType; /* - - - - - - end of struct ParseLineDataType - - - - - - */
    char *token;
    static const char kDELIMITER[] = "|\n";
    ParseLineDataType *format;
    ParseLineDataType line_format[] = {
        {kPARSE_STRING,     (void*)&data->hostname},
        {kPARSE_IP,         (void*)&data->ip_from},
        {kPARSE_IP,         (void*)&data->ip_to},
        {kPARSE_IP_TYPE,    (void*)&data->ip_type},
        {kPARSE_DOUBLE,     (void*)&data->connect_time},
        {kPARSE_DOUBLE,     (void*)&data->first_byte_time},
        {kPARSE_DOUBLE,     (void*)&data->download_time},
        {kPARSE_DOUBLE,     (void*)&data->detect_used_time},
        {kPARSE_UINT16,     (void*)&data->detect_times},
        {kPARSE_UINT16,     (void*)&data->detect_success_times},
        {kPARSE_UINT16,     (void*)&data->port},
        {kPARSE_PORT_STATUS,(void*)&data->port_status},
        {kPARSE_DETECT_TYPE,(void*)&data->detect_type},
        {kPARSE_UINT16,     (void*)&data->http_status},
        {kPARSE_END,        NULL}
    };
    for( format = line_format; format->type != kPARSE_END; ++ format )
    {
        ReleaseCpu();
        /* 还有需要读取的元素，但是这行已经没有可读的了 */
        if( *line == '\n' )
        {
            Debug( 10, "No more content to be read." );
            FreeDataInfo( data );
            return 1;
        }
        /* 读取到了错误的信息 */
        token = strsep( &line, kDELIMITER );
        if( token == NULL || *token == 0 )
        {
            Debug( 10, "Cannot parse line." );
            FreeDataInfo( data );
            return 1;
        }
        /* 正常读取 */
        switch( format->type )
        {
            case kPARSE_UINT16:
                {
                    char *cannot_be_read = 0;
                    long l = strtol( token, &cannot_be_read, 10 );
                    if( *cannot_be_read != '\0' )
                    {
                        Debug( 10, "Read a non-uint16_t data." );
                        FreeDataInfo( data );
                        return 1;
                    }
                    if( l >= 0xFFFF )
                        *(uint16_t*)format->data = 0xFFFF;
                    else if ( l <= 0 )
                        *(uint16_t*)format->data = 0;
                    else
                        *(uint16_t*)format->data = (uint16_t)l;
                }
                break;
            case kPARSE_UINT32:
                {
                    char *cannot_be_read = 0;
                    long l = strtol( token, &cannot_be_read, 10 );
                    if( *cannot_be_read != '\0' )
                    {
                        Debug( 10, "Read a non-uint32_t data." );
                        FreeDataInfo( data );
                        return 1;
                    }
                    *(uint32_t*)format->data = (uint32_t)l;
                }
                break;
            case kPARSE_STRING:
                *(char**)format->data = strdup( token );
                break;
            case kPARSE_DOUBLE:
                {
                    char *cannot_be_read = 0;
                    double d = strtod( token, &cannot_be_read );
                    if( *cannot_be_read != '\0' )
                    {
                        Debug( 10, "Read a non-double data." );
                        FreeDataInfo( data );
                        return 1;
                    }
                    if( isnan( d ) )
                    {
                        Debug( 30, "Read a double number which is NaN." );
                        d = INFINITY;
                    }
                    *(double*)format->data = d;
                }
                break;
            case kPARSE_IP:
                if( inet_pton( AF_INET, token, format->data ) != 1 )
                {
                    Debug( 10, "Found a unknown string when parse ip." );
                    FreeDataInfo( data );
                    return 1;
                }
                break;
            case kPARSE_PORT_STATUS:
                if( token[0] == 'w' )
                    *(PortStatus*)format->data = kPORT_WORK;
                else
                    *(PortStatus*)format->data = kPORT_DOWN;
                break;
            case kPARSE_DETECT_TYPE:
                if( strcasecmp( token, "http" ) == 0 )
                    *(DetectType*)format->data = kDETECT_TYPE_HTTP;
                else if( strcasecmp( token, "tcp" ) == 0 )
                    *(DetectType*)format->data = kDETECT_TYPE_TCP;
                else if( strcasecmp( token, "https" ) == 0 )
                    *(DetectType*)format->data = kDETECT_TYPE_HTTPS;
                else
                    *(DetectType*)format->data = kDETECT_TYPE_RESERVE;
                break;
            case kPARSE_IP_TYPE:
                if( strcmp( token, "ip" ) == 0 )
                    *(IpType*)format->data = kIP_TYPE_IP;
                else if( strcmp( token, "bkip" ) == 0 )
                    *(IpType*)format->data = kIP_TYPE_BACKUP_IP;
                else if( strcmp( token, "dnsip" ) == 0 )
                    *(IpType*)format->data = kIP_TYPE_DNS_IP;
                else
                {
                    Debug( 20, "Unknown ip type: %s.", token );
                    FreeDataInfo( data );
                    return 1;
                }
                break;
            case kPARSE_NULL:
                Debug( 30, "Read a empty token." );
                break;
            default:
                Debug( 1, "Unknown error!" );
                FreeDataInfo( data );
                return 1;
        }
    }
    /* 没有需要读取的元素，但是这行还有东西可以读 */
    token = data->hostname + strlen( data->hostname ) - 1;
    if( *token == '.' )
        *token = '\0';
    return 0;
} /* - - - - - - - end of function ParseLine - - - - - - - - - - - */
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  AnalyseSingleFile
 *  Description:
 * =====================================================================================
 */
static int
AnalyseSingleFile( char *file_name, int order )
{
    const int kLINE_LENGTH = 4096;
    FILE *fp;
    char line[kLINE_LENGTH];
    DataInfo data;
    assert( file_name );
    Debug( 40, "Get data from %s.", file_name );
    fp = fopen( file_name, "r" );
    if( fp == NULL )
    {
        Debug( 20, "Cannot open file %s.", file_name );
        return 0;
    }
    while( fgets( line, kLINE_LENGTH, fp ) != NULL )
    {
        data.file_order = order;
        /* parse data */
        if( ParseLine( &data, line ) != 0 )
        {
            Debug( 50, "Ignore line \"%-30s...\"", line );
            continue;
        }
        /* send to module */
        CallHookFunction( kHOOK_DATA_INPUT, &data );
        FreeDataInfo( &data );
    }
    fclose( fp );
    return 0;
} /* - - - - - - - end of function AnalyseSingleFile - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  Analyse
 *  Description:  假定需要的数据文件，是来自标准输入
 * =====================================================================================
 */
    static int
Analyse( void )
{
    if( debug_link_data_file == NULL )
    {
        struct dirent **name_list;
        int total;
        char buffer[4096];
        int i;
        unsigned int link_data_read_count_max = GetLinkDataReadCountMax();
        errno = 0;
        total = scandir( kLINK_DATA_DIR, &name_list, (void*)LinkDataFileNameFilter, (void*)SortFileByCTimeDescending );
        if( total <= 0 )
        {
            Debug( 10, "Error in scandir() to scan %s. %s.", kLINK_DATA_DIR, strerror( errno ) );
            return 1;
        }
        else if( total == 0 )
        {
            Debug( 10, "There is not any link data file available." );
            return 1;
        }
        if( link_data_read_count_max > total )
        {
            Debug( 40, "No %u file(s) of link.data, will only read %d file(s) of link.data.", link_data_read_count_max, total );
            link_data_read_count_max = total;
        }
        for( i = link_data_read_count_max - 1; i >= 0; --i )
        {
            if( snprintf( buffer, 4096, "%s%s", kLINK_DATA_DIR, name_list[i]->d_name ) >= 4096 )
                Debug( 20, "Do truncate when dealing with file %s.", name_list[0]->d_name );
            if( AnalyseSingleFile( buffer, i ) != 0 )
                break;
        }
        /* Do free() on results of scandir() */
        for( i = 0; i < total; ++i )
        {
            free( name_list[i] );
            name_list[i] = NULL;
        }
        free( name_list );
    }
    else
    {
        AnalyseSingleFile( debug_link_data_file, 0 );
    }
    return 0;
} /* - - - - - - - end of function Analyse - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  Singleton
 *  Description:  通过对文件加锁的方式，确保一个时间只会有一个analyse在运行
 * =====================================================================================
 */
    static int
Singleton( void )
{
    const char pid_file_name[] = "/var/run/analyse.pid";
    int pid_file, ret;
    errno = 0;
    pid_file = open(pid_file_name, O_CREAT | O_RDWR, 0666);
    if (pid_file == -1)
    {
        perror("open pid file failed");
        return 1;
    }
    errno = 0;
    ret = flock(pid_file, LOCK_EX | LOCK_NB);
    if (ret == -1)
    {
        perror("analyse is running");
        return 2;
    }
    return 0;
} /* - - - - - - - end of function Singleton - - - - - - - - - - - */
    int
main( int argc, char *argv[] )
{
    if( Singleton() != 0 )
        return 1;
    if( ParseArgument( argc, argv ) != 0 )
        return 1;
    if( DebugInit() != 0 )
        return 1;
    if( ShortestPathInit() != 0 )
        return 1;
    if( LoadModule() != 0 )
        return 1;
    if( CallHookFunction( kHOOK_INIT, NULL ) != 0 )
        return 1;
    if( Analyse() != 0 )
        return 1;
    if( CallHookFunction( kHOOK_END, NULL ) != 0 )
        return 1;
    if( ReleaseModule() != 0 )
        return 1;
    if( ShortestPathFinish() != 0 )
        return 1;
    if( DebugEnd() != 0 )
        return 1;
    return 0;
} /* - - - - - - - end of function main - - - - - - - - - - - - - */
