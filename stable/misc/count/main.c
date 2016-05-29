/*
 * =====================================================================================
 *
 *       Filename:  main.c
 *
 *    Description:  
 *
 *        Created:  12/06/2011 02:40:36 PM
 *       Compiler:  gcc
 *         Author:  XuJian
 *        Company:  China Cache
 *
 * =====================================================================================
 */
#define MY_DEBUG
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>
#include "hash.h"
#include "linked_list.h"
#define kSTREAM debug_stream
const static char *const kCONF_FILE_NAME = "/usr/local/squid/etc/count.conf";
//const static int kHASH_SIZE = 1<<20;
const static int kLINE_MAX_SIZE = 1<<13;
const static char *const kWHITE_SPACE = " \t";
const static int kFILE_NAME_LENGTH = 256;
typedef struct _Table2D {
    int amount;
    void *to_be_free;
    char **data;
} Table2D; /* - - - - - - end of struct Table2D - - - - - - */
typedef struct _Configure {
    char *log_file_name;                       /* MISS log */
    char *bak_file_name;                       /* copy MISS log to here */
    char *data_file_name;                      /* data remained after last analysing */
    char *tmp_file_name;                       /* copy data_file_name to here */
    char *hot_file_name;                       /* store hot urls */
    FILE *own_log_name;                        /* log own */
    char *call_system;
    Table2D drag_keyword;
    int is_truncate_log;
    int fix_time;
    int max_times;
    int duration;
    int max_server;
    int max_request;
    int debug_level;
    int time_valid;
    int max_urls;
    int hd_block_size;
} Configure; /* - - - - - - end of struct Configure - - - - - - */
typedef struct _Url {
    time_t atime;
    char *access_url;
    char *store_url;
    int count;
    unsigned int xor;
    unsigned int sum;
} Url; /* - - - - - - end of struct Url - - - - - - */
typedef struct _AnalyseInfo {
    Hash *hash;
    FILE *record_fp;
    Url **hot;
    int hot_total;
    const Configure *conf;
} AnalyseInfo; /* - - - - - - end of struct AnalyseInfo - - - - - - */
typedef struct _HotServer {
    char *name;
    int count;
    LinkedList *url_list;
} HotServer; /* - - - - - - end of struct HotServer - - - - - - */
typedef struct _HotUrl {
    char *url;
    void *hot_server;
    int is_sending;
    FILE *fp;
} HotUrl; /* - - - - - - end of struct HotUrl - - - - - - */
int debug_level = 0;
FILE *debug_stream = NULL;
#ifdef MY_DEBUG
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  Debug
 *  Description:  
 * =====================================================================================
 */
#if 0
    static int
Debug( int level, const char *string )
{
    if( level <= debug_level )
        perror( string );
    return 0;
} /* - - - - - - - end of function Debug - - - - - - - - - - - */
#else /* not 0 */
    static int
Debug( int level, FILE *stream, const char *format, ... )
{
    const static int kSIZE = 1<<10;
    int size;
    char buf[kSIZE];
    va_list arg_list;
    if( level > debug_level )
    {
        return 0;
    }
    va_start( arg_list, format );
    size  =  snprintf( buf, kSIZE, "%d %d ", (int)time(NULL), level );
    size += vsnprintf( buf + size, kSIZE - size, format, arg_list );
    if( size > kSIZE - 1 )
    {
        buf[kSIZE - 5] = buf[kSIZE - 3] = buf[kSIZE - 4] = '.';
        buf[kSIZE - 2] = '\n';
        size = kSIZE;
    }
    fputs( buf, stream );
    va_end( arg_list );
    return size;
} /* - - - - - - - end of function Debug - - - - - - - - - - - */
#endif /* not 0 */
#else /* not MY_DEBUG */
#define Debug(x,y)  
#endif /* not MY_DEBUG */
    static char*
rf_url_hostname(const char *url,char *hostname)
{
    char *a = strstr(url,"//");
    if(a == NULL)
        return NULL;
    a += 2;
    strcpy(hostname,a);
    char *c = hostname;
    while(*c && *c != '/')
        c++;
    if(*c != '/')
        return NULL;
    *c = '\0';
    return hostname;
}
    static int
drop_startend(char* start, char* end, char* url)
{
    char *s;
    char *e;

    s = strstr(url, start);
    if(s)
    {
        e = strstr(s, "&");
        if( e == 0 )
            *(s-1) = 0;
        else
        {
            memmove(s, e+1, strlen(e+1));
            *(s+strlen(e+1)) = 0;
        }
    }
    else
        return 1;

    s = strstr(url, end);
    if( s )
    {
        e = strstr(s, "&");

        if( e == 0 )
            *(s-1) = 0;
        else
        {
            memmove(s, e+1, strlen(e+1));
            *(s+strlen(e+1)) = 0;
        }
    }
    return 0;
}
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  StringDuplicate
 *  Description:  
 * =====================================================================================
 */
    static char*
StringDuplicate( const char *string )
{
    char *dup;
    size_t size;
    size = strlen( string ) + 1;
    dup = (char *)malloc( size );
    if( dup == NULL )
    {
        Debug( 1, kSTREAM, "malloc failed.\n" );
        return NULL;
    }
    memcpy( dup, string, size );
    return dup;
} /* - - - - - - - end of function StringDuplicate - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  Table2DDestroy
 *  Description:  
 * =====================================================================================
 */
    static int
Table2DDestroy( Table2D *table )
{
    if( table->amount != 0 )
    {
        free( table->to_be_free );
        free( table->data );
        table->data = NULL;
        table->amount = 0;
    }
    return 0;
} /* - - - - - - - end of function Table2DDestroy - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  Table2DInit
 *  Description:  
 * =====================================================================================
 */
    static int
Table2DInit( Table2D *table, char *string )
{
    const char group_separator = ';';
    const char separator = ',';
    static char imposible_string[] = "\n\a";
    int i, k, group_count;
    int string_length;
    char *p;
    char **data;
    group_count = 0;
    string_length = 0;
    for( p = string; *p; ++p )
    {
        ++string_length;
        if( *p == group_separator )
            ++group_count;
    }
    if( group_count == 0 )
    {
        table->amount = 0;
        table->data = NULL;
        table->to_be_free = NULL;
        return 0;
    }
    ++string_length;
    data = (char**)malloc( sizeof(char*) * ( group_count << 1 ) );
    if( data == NULL )
    {
        Debug( 1, kSTREAM, "malloc failed.\n" );
        return 1;
    }
    p = (char *)malloc( sizeof(char) * string_length );
    if( p == NULL )
    {
        Debug( 1, kSTREAM, "malloc failed.\n" );
        free( data );
        return 1;
    }
    memcpy( p, string, sizeof(char) * string_length );
    table->to_be_free = p;
    k = 0;
    for( i = 0; i < group_count; ++i )
    {
        data[k] = p;
        while( *p != separator )
            ++p;
        *p = '\0';
        if( *data[k] == '\0' )
            data[k] = imposible_string;
        ++k; ++p;
        data[k] = p;
        while( *p != group_separator )
            ++p;
        *p = '\0';
        if( *data[k] == '\0' )
            data[k] = imposible_string;
        ++k; ++p;
    }
    table->amount = group_count;
    table->data = data;
    return 0;
} /* - - - - - - - end of function Table2DInit - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ConfigureDestroy
 *  Description:  
 * =====================================================================================
 */
    static int
ConfigureDestroy( Configure *conf )
{
    free( conf->log_file_name );
    free( conf->bak_file_name );
    free( conf->data_file_name );
    free( conf->tmp_file_name );
    free( conf->hot_file_name );
    fclose( conf->own_log_name );
    free( conf->call_system );
    Table2DDestroy( &conf->drag_keyword );
    return 0;
} /* - - - - - - - end of function ConfigureDestroy - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  UrlDestroy
 *  Description:  
 * =====================================================================================
 */
    static int
UrlDestroy( Url *url )
{
    free( url->store_url );
    free( url->access_url );
    url->store_url = NULL;
    url->access_url = NULL;
#ifdef MY_DEBUG
    memset( url, 0, sizeof(Url) );
#else /* not MY */
#endif /* not MY */
    return 0;
} /* - - - - - - - end of function UrlDestroy - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  UrlFree
 *  Description:  
 * =====================================================================================
 */
    static int
UrlHashDoFree( void *p )
{
    if( p == NULL )
        return 0;
    return UrlDestroy( (Url*)p );
} /* - - - - - - - end of function UrlFree - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  UrlPrint
 *  Description:  
 * =====================================================================================
 */
    static int
UrlPrint( Url *url )
{
    printf( "%d %s %s %d\n", (int)url->atime, url->access_url, url->store_url, url->count );
    return 0;
} /* - - - - - - - end of function UrlPrint - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  UrlHashElementPrint
 *  Description:  
 * =====================================================================================
 */
    static int
UrlHashElementPrint( void *p )
{
    UrlPrint( (Url*)p );
    return 0;
} /* - - - - - - - end of function UrlHashElementPrint - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  UrlInit
 *  Description:  
 * =====================================================================================
 */
    static int
UrlInit( Url *url, char *string, time_t time_valid )
{
    char *param, *p;
    size_t size;
    unsigned int xor, sum;

    xor = sum = 0;
    param = strtok( string, kWHITE_SPACE );
    url->atime = (time_t)atoi( param );
    if( url->atime < time_valid )
    {
        Debug( 11, kSTREAM, "beyond duration, ignore it.\n" );
        return 1;
    }

    param = strtok( NULL, kWHITE_SPACE );
    *( param - 1 ) = ' ';
    size = strlen( param ) + 1;
    p = (char *)malloc( size );
    if( p == NULL )
    {
        perror( "malloc failed." );
        memset( url, 0, sizeof(Url) );
        return 1;
    }
    url->access_url = p;
    memcpy( p, param, size );

    param += size;
    *( param - 1 ) = ' ';
    size = strlen( param );
    p = (char *)malloc( size );
    if( p == NULL )
    {
        perror( "malloc failed." );
        free( url->access_url );
        memset( url, 0, sizeof(Url) );
        return 1;
    }
    memcpy( p, param, size );
    p[size-1] = '\0';
    param = url->store_url = p;
    /* create xor and sum */
    {
        for( ; *p != '\0'; ++p )
        {
            xor = ( xor << 5 ) - xor + 29;
            xor ^= *p;
            sum = ( sum << 4 ) + sum + 39;
            sum += *p;
        }
        //        param += size - 5;
        //        while( ( (int)p & 3 ) != 0 )
        //        {
        //            xor ^= *p;
        //            sum += *p;
        //            p++;
        //        }
        //        while( p <= param )
        //        {
        //            xor ^= *(int*)p;
        //            sum += *(int*)p;
        //            p += 4;
        //        }
        //        param += 4;
        //        while( p < param )
        //        {
        //            xor ^= *p;
        //            sum += *p;
        //            p++;
        //        }
    }
    url->count = 0;
    url->xor = xor;
    url->sum = sum;
    return 0;
} /* - - - - - - - end of function UrlInit - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  UrlGetKey
 *  Description:  
 * =====================================================================================
 */
    static unsigned int
UrlHashGetKey( const void *p )
{
    Url *url = (Url*)p;
    return url->xor;
} /* - - - - - - - end of function UrlGetKey - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  UrlIsEqual
 *  Description:  
 * =====================================================================================
 */
    static int
UrlHashIsEqual( const void *p1, const void *p2 )
{
    Url *url1 = (Url*)p1;
    Url *url2 = (Url*)p2;
    return url1->sum == url2->sum || !strcmp( url1->store_url, url2->store_url );
} /* - - - - - - - end of function UrlIsEqual - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  UrlHashIsNull
 *  Description:  
 * =====================================================================================
 */
    static int
UrlHashIsNull( const void *p )
{
    return( ((Url*)p)->atime == 0 );
} /* - - - - - - - end of function UrlHashIsNull - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ReadConfigureFile
 *  Description:  
 * =====================================================================================
 */
    static int
ReadConfigureFile( const Configure *configure )
{
    int i;
    typedef enum{kNULL,kBOOL,kSTRING,kINT,kGROUP,kWRITE_FILE} RCFType;
    typedef struct _RCFSolve {
        char *name;
        void *value;
        char *default_value;
        RCFType type;
        int is_modified;
    } RCFSolve; /* - - - - - - end of struct RCFSolve - - - - - - */
    FILE *file;
    char buffer[1024];
    RCFSolve solve[] = {
        {"log_file_name",   (void*)&configure->log_file_name,    "/data/hotspot/hot_spot.log",      kSTRING},
        {"bak_file_name",   (void*)&configure->bak_file_name,    "/data/hotspot/bak.txt",           kSTRING},
        {"data_file_name",  (void*)&configure->data_file_name,   "/data/hotspot/data.txt",          kSTRING},
        {"tmp_file_name",   (void*)&configure->tmp_file_name,    "/data/hotspot/tmp.txt",           kSTRING},
        {"hot_file_name",   (void*)&configure->hot_file_name,    "/data/hotspot/hot_spot.result",   kSTRING},
        {"own_log_name",    (void*)&configure->own_log_name,     "/data/hotspot/count.log",         kWRITE_FILE},
        {"call_system",     (void*)&configure->call_system,      "echo %s",  	    kSTRING},
        {"drag_keyword",    (void*)&configure->drag_keyword,     "",	            kGROUP},
        {"is_truncate_log", (void*)&configure->is_truncate_log,  "true",  	        kBOOL},
        {"fix_time",        (void*)&configure->fix_time,         "0",	            kINT},
        {"max_times",       (void*)&configure->max_times,        "3",	            kINT},
        {"duration",        (void*)&configure->duration,         "3600",	        kINT},
        {"max_server",      (void*)&configure->max_server,       "2",	            kINT},
        {"max_request",     (void*)&configure->max_request,      "1",	            kINT},
        {"debug_level",     (void*)&configure->debug_level,      "0",	            kINT},
        {"max_urls",        (void*)&configure->max_urls,         "1048576",	        kINT},
        {"hd_block_size",   (void*)&configure->hd_block_size,    "4096",	        kINT},
        {NULL,              NULL,                                NULL,	            kNULL}};
    file = fopen( kCONF_FILE_NAME, "r" );
    if( file != NULL )
    {
        while( fgets( buffer, 1024, file ) != NULL )
        {
            i = -1;
            int isComment = 0;
            char *name;
            char *value;
            switch( *buffer )
            {
                case '#':
                case ';':
                case '/':
                case '\n':
                case '\0':
                case ' ':
                case '\t':
                    isComment = 1;
                    break;
                default:
                    break;
            }
            if( isComment )
                continue;
            name = strtok( buffer, kWHITE_SPACE );
            if( name == NULL )
                break;
            value = name + strlen( name ) + 1;
            if( value == NULL )
            {
//                char warning[256];
//                sprintf( warning, "syntex error in conf.txt, %s should have a value", name );
//                Debug( 0, kSTREAM, warning );
                Debug( 0, kSTREAM, "syntax error in conf.txt, %s should have a value.\n", name );
                break;
            }
            {
                int end = strlen( value );
                if( value[end-1] == '\n' )
                    value[end-1] = '\0';
            }
            while( solve[++i].name != NULL )
            {
                if( strcasecmp( solve[i].name, name ) == 0 )
                {
                    solve[i].is_modified = 1;
                    switch( solve[i].type )
                    {
                        case kSTRING:
                            {
                                size_t size;
                                void *p;
                                size = strlen( value );
                                p = (char *)malloc( size );
                                if( p == NULL )
                                {
                                    Debug( 1, kSTREAM, "malloc failed.\n" );
                                    break;
                                }
                                memcpy( p, value, size );
                                *(char**)solve[i].value = p;
                            }
                            break;
                        case kINT:
                            *(int*)solve[i].value = atoi( value );
                            break;
                        case kBOOL:
                            *(int*)solve[i].value = strcasecmp( value, "true" ) == 0;
                            break;
                        case kGROUP:
                            Table2DInit( (Table2D*)solve[i].value, value );
                            break;
                        case kWRITE_FILE:
                            *(FILE**)solve[i].value = fopen( value, "a" );
                            if( *(FILE**)solve[i].value == NULL )
                            {
                                perror( "cannot open file for writing." );
                            }
                            break;
                        default:
                            Debug( 1, kSTREAM, "unknown type" );
                            break;
                    }
                    break;
                }
            }
        }
        fclose( file );
    }
    for( i = 0; solve[i].name != NULL; ++i )
        if( solve[i].is_modified != 1 )
        {
            switch( solve[i].type )
            {
                case kSTRING:
                    {
                        size_t size;
                        void *p;
                        char *value;
                        value = solve[i].default_value;
                        size = strlen( value ) + 1;
                        p = (char *)malloc( size );
                        if( p == NULL )
                        {
                            Debug( 1, kSTREAM, "malloc failed.\n" );
                            break;
                        }
                        memcpy( p, value, size );
                        *(char**)solve[i].value = p;
                    }
                    break;
                case kINT:
                    *(int*)solve[i].value = atoi( solve[i].default_value );
                    break;
                case kBOOL:
                    *(int*)solve[i].value = strcasecmp( solve[i].default_value, "true" ) == 0;
                    break;
                case kGROUP:
                    Table2DInit( (Table2D*)solve[i].value, solve[i].default_value );
                    break;
                case kWRITE_FILE:
                    *(FILE**)solve[i].value = fopen( solve[i].default_value, "a" );
                    if( *(FILE**)solve[i].value == NULL )
                    {
                        perror( "cannot open file for writing." );
                    }
                    break;
                default:
                    Debug( 1, kSTREAM, "unknown type" );
                    break;
            }
        }
    debug_level = configure->debug_level;
    debug_stream = configure->own_log_name;
    return 0;
} /* - - - - - - - end of function ReadConfigureFile - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  CopyAndTruncateLog
 *  Description:  
 * =====================================================================================
 */
    static int
CopyAndTruncateLog( const Configure *configure )
{
    //return system( "logrotate -s /dev/null count-logrotate.conf" );
    char *buf;
    size_t size_buf, size;
    int log_fd, bak_fd;

    log_fd = open( configure->log_file_name, O_RDWR );
    if( log_fd == -1 )
    {
        Debug( 1, kSTREAM, "cannot open log file.\n" );
        return 1;
    }
    bak_fd = open( configure->bak_file_name,
            O_WRONLY | O_CREAT | O_TRUNC,
            0644 );
    if( bak_fd == -1 )
    {
        Debug( 1, kSTREAM, "cannot open bak file" );
        close( bak_fd );
        return 1;
    }

    size_buf = configure->hd_block_size;
    buf = (char *)malloc( size_buf );
    if( buf == NULL )
    {
        Debug( 1, kSTREAM, "malloc failed.\n" );
        close( bak_fd );
        close( log_fd );
        return 1;
    }

    while( ( size = read( log_fd, buf, size_buf ) ) != 0 )
        write( bak_fd, buf, size );
    if( configure->is_truncate_log )
    {
        if( ftruncate( log_fd, 0 ) != 0 )
            Debug( 1, kSTREAM, "truncate failed.\n" );
    }
    else
    {
        char cmd[1024];
        sprintf( cmd, "> %s", configure->data_file_name );
        system( cmd );
        Debug( 10, kSTREAM, "> %s\n", configure->data_file_name );
    }

    free( buf );
    close( bak_fd );
    close( log_fd );
    return 0;
} /* - - - - - - - end of function CopyAndTruncateLog - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  Analyse
 *  Description:  
 * =====================================================================================
 */
    static int
Analyse( const char *data_file_name, AnalyseInfo *info )
{
    int max_times;
    FILE *fp;
    char line[kLINE_MAX_SIZE];
    Url url;
    time_t time_valid;

    fp = fopen( data_file_name, "r" );
    if( fp == NULL )
        return 1;

    max_times = info->conf->max_times;
    time_valid = info->conf->time_valid;
    while( fgets( line, kLINE_MAX_SIZE, fp ) != NULL )
    {
        Url *url_in_hash;
        if( UrlInit( &url, line, time_valid ) != 0 )
        {
            Debug( 20, kSTREAM, "ignore one access url\n" );
            continue;
        }
        fputs( line, info->record_fp );
        url_in_hash = (Url*)HashSearch( info->hash, (void*)&url );
        if( url_in_hash == NULL )
        {
            Debug( 10, kSTREAM, "Inserting to hash failed.\n" );
//            fputs( line, info->record_fp );
            continue;
        }
        ++url_in_hash->count;
        if( url_in_hash->count != 1 )
        {
            char *swap;
            swap = url_in_hash->access_url;
            url_in_hash->access_url = url.access_url;
            url.access_url = swap;
            UrlDestroy( &url );
        }
//        if( url_in_hash->count < max_times )
//            fputs( line, info->record_fp );
//        else if( url_in_hash->count == max_times )
        if( url_in_hash->count == max_times )
            info->hot[info->hot_total++] = url_in_hash;
    }
    fclose( fp );
    return 0;
} /* - - - - - - - end of function Analyse - - - - - - - - - - - */
#define WORK
#ifdef WORK
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  HotServerDestroy
 *  Description:  
 * =====================================================================================
 */
    static int
HotServerDestroy( void *p )
{
    HotServer *server = (HotServer*)p;
    if( server->name != NULL )
    {
        free( server->name );
        server->name = NULL;
        LinkedListDestroy( server->url_list );
    }
    return 0;
} /* - - - - - - - end of function HotServerDestroy - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  HotUrlDestroy
 *  Description:  
 * =====================================================================================
 */
    static int
HotUrlDestroy( void *p )
{
    HotUrl *url;
    if( p )
    {
        url = (HotUrl*)p;
        url->url = NULL;
        url->hot_server = NULL;
        if( url->fp != NULL )
        {
            fflush( url->fp );
            pclose( url->fp );
            url->fp = NULL;
        }
        free( p );
    }
    return 0;
} /* - - - - - - - end of function HotUrlDestroy - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  CreateServerList
 *  Description:  
 * =====================================================================================
 */
    static LinkedList*
CreateServerList( Url **hot_url, int amount )
{
    int i;
    LinkedList *server_list;
    char server_name[1024];
    server_list = LinkedListCreate( HotServerDestroy );
    if( server_list == NULL )
        return NULL;
    for( i = 0; i < amount; ++i )
    {
        int is_existed;
        void *start;
        HotUrl *url;
        HotServer *server;
        LinkedList *url_list;
        rf_url_hostname( hot_url[i]->access_url, server_name );
        start = LinkedListGet( server_list );
        is_existed = 0;
        if( start != NULL )
        {
            void *p;
            p = start;
            do
            {
                char *name;
                name = ((HotServer*)LinkedListData(p))->name;
                if( !strcmp( name, server_name ) )
                {
                    is_existed = 1;
                    break;
                }
                p = LinkedListGet( server_list );
            }while( p != start );
        }
        if( !is_existed )
        {
            server = (HotServer *)malloc( sizeof(HotServer) );
            if( server == NULL )
            {
                Debug( 1, kSTREAM, "malloc failed.\n" );
                LinkedListDestroy( server_list );
                return NULL;
            }
            server->name = StringDuplicate( server_name );
            if( server->name == NULL )
            {
                LinkedListDestroy( server_list );
                free( server );
                return NULL;
            }
            url_list = LinkedListCreate( HotUrlDestroy );
            if( url_list == NULL )
            {
                LinkedListDestroy( server_list );
                free( server );
                return NULL;
            }
            server->url_list = url_list;
            server->count = 0;
            start = LinkedListCycleInsert( server_list, (void*)server );
        }
        url = (HotUrl *)malloc( sizeof(HotUrl) );
        if( url == NULL )
        {
            LinkedListDestroy( server_list );
            return NULL;
        }
        url->url = hot_url[i]->access_url;
        url->hot_server = start;
        url->is_sending = 0;
        url->fp = NULL;
        LinkedListCycleInsert( ((HotServer*)LinkedListData( start ))->url_list, (void*)url );
    }
    return server_list;
} /* - - - - - - - end of function CreateServerList - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  SendSingleHot
 *  Description:  
 * =====================================================================================
 */
static int
SendSingleHot( const char *command, void *node, int epoll_fd )
{
    struct epoll_event event;
    int fd;
    FILE *fp;
    HotUrl *url;

    fp = popen( command, "r" );
    if( fp == NULL )
    {
        Debug( 1, kSTREAM, "popen failed.\n" );
        return 1;
    }
    url = LinkedListData( node );
    url->fp = fp;
    fd = fileno( fp );
    event.events = EPOLLHUP | EPOLLIN;
    event.data.ptr = node;
    url->is_sending = 1;
    if( epoll_ctl( epoll_fd, EPOLL_CTL_ADD, fd, &event ) == -1 )
    {
        Debug( 1, kSTREAM, "epoll_ctl cannot add.\n" );
        return 1;
    }
    return 0;
} /* - - - - - - - end of function SendSingleHot - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  DropStartEnd
 *  Description:  
 * =====================================================================================
 */
static void
DropStartEnd( const AnalyseInfo *info, char *url )
{
    int amount, i;
    char **keyword;

    amount = info->conf->drag_keyword.amount;
    keyword = info->conf->drag_keyword.data;
    for( i = 0; i < amount; ++i )
    {
        int n = i << 1;
        if( drop_startend( keyword[n], keyword[n+1], url ) == 0 )
            break;
    }
    return;
} /* - - - - - - - end of function DropStartEnd - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  SendHotEx
 *  Description:  
 * =====================================================================================
 */
/*
    static int
SendHotEx( AnalyseInfo *info )
{
    int amount, i,k;
    int is_finished;
    const int max_server = info->conf->max_server;
    const int max_request = info->conf->max_request;
    const int max_total = max_server * max_request;
    int epoll_fd;
    struct epoll_event *events;
    LinkedList *server_list;
    char *command_format;
    char command[1024];

    // initialize 
    command_format = info->conf->call_system;
    server_list = CreateServerList( info->hot, info->hot_total );
    if( server_list == 0 )
        return 1;
    epoll_fd = epoll_create( max_total );
    if( epoll_fd == -1 )
    {
        Debug( 1, kSTREAM, "epoll_create failed.\n" );
        LinkedListDestroy( server_list );
        return 1;
    }
    events = (struct epoll_event *)malloc( sizeof(struct epoll_event) * max_total );
    if( events == NULL )
    {
        Debug( 1, kSTREAM, "malloc failed.\n" );
        close( epoll_fd );
        LinkedListDestroy( server_list );
        return 1;
    }
    // first batch of urls 
    for( i = max_server; i > 0; --i )
    {
        HotServer *server;
        server = (HotServer*)LinkedListGetData( server_list );
        if( server == NULL )
        {
            Debug( 1, kSTREAM, "hot server is empty.\n" );
            free( events );
            close( epoll_fd );
            LinkedListDestroy( server_list );
            return 1;
        }
        if( server->count != 0 )
            break;
        for( k = max_request; k > 0; --k )
        {
            void *p;
            HotUrl *url;
            p = LinkedListGet( server->url_list );
            url = (HotUrl*)LinkedListData( p );
            if( url->is_sending )
                break;
            url->is_sending = 1;
            DropStartEnd( info, url->url );
            sprintf( command, command_format, url->url );
            SendSingleHot( command, p, epoll_fd );
            Debug( 10, kSTREAM, "%s\n", command );
            //puts( url->url );
            ++server->count;
        }
    }
    // when finished, send more 
    is_finished = 0;
    while( !is_finished )
    {
        amount = epoll_wait( epoll_fd, events, max_total, 1000 );
        if( amount == -1 )
        {
            if( errno == EINTR )
            {
                Debug( 20, kSTREAM, "epoll_wait unknown.\n" );
                continue;
            }
            Debug( 1, kSTREAM, "epoll_wait failed.\n" );
            break;
        }
        else if( amount == 0 )
        {
            Debug( 20, kSTREAM, "epoll_wait timeout.\n" );
            continue;
        }
        else if( amount > 0 )
        {
            for( i = 0; i < amount; ++i )
            {
                if( (events[i].events & EPOLLIN ) != 0 )
                {
                    char buf[1024];
                    HotUrl *url;
                    url = (HotUrl*)LinkedListData( events[i].data.ptr );
                    fgets( buf, 1024, url->fp );
                    Debug( 15, kSTREAM, buf );
                }
                else if( ( events[i].events & EPOLLHUP ) != 0 )
                {
                    void *p;
                    HotUrl *url;
                    HotServer *server;
                    url = (HotUrl*)LinkedListData( events[i].data.ptr );
                    if( epoll_ctl( epoll_fd, EPOLL_CTL_DEL, fileno( url->fp ), NULL ) == -1 )
                    {
                        Debug( 1, kSTREAM, "epoll_ctl cannot del.\n" );
                        is_finished = 1;
                        LinkedListDestroy( server_list );
                        close( epoll_fd );
                        return 1;
                    }
                    p = url->hot_server;
                    server = (HotServer*)LinkedListData( p );
                    LinkedListDelete( server->url_list, events[i].data.ptr );
                    url = NULL;
                    --server->count;
                    // this server is completed 
                    if( server->url_list->amount == 0 )
                    {
                        LinkedListDelete( server_list, p );
                        free( server );
                        server = (HotServer*)LinkedListGetData( server_list );
                        //some server else have url to be sent 
                        if( server )
                        {
                            if( server->count == 0 )
                            {
                                for( k = max_request; k > 0; --k )
                                {
                                    void *p;
                                    HotUrl *url;
                                    p = LinkedListGet( server->url_list );
                                    url = (HotUrl*)LinkedListData( p );
                                    if( url->is_sending )
                                        break;
                                    url->is_sending = 1;
                                    DropStartEnd( info, url->url );
                                    sprintf( command, command_format, url->url );
                                    SendSingleHot( command, p, epoll_fd );
                                    Debug( 10, kSTREAM, "%s\n", command );
                                    //puts( url->url );
                                    ++server->count;
                                }
                            }
                        }
                        else
                            is_finished = 1;
                    }
                    // have some url to be sent 
                    else if( server->count < server->url_list->amount )
                    {
                        void *start;
                        void *p;
                        p = start = LinkedListGet( server->url_list );
                        while(1)
                        {
                            url = LinkedListData( p );
                            if( !url->is_sending )
                            {
                                DropStartEnd( info, url->url );
                                sprintf( command, command_format, url->url );
                                SendSingleHot( command, p, epoll_fd );
                                Debug( 10, kSTREAM, "%s\n", command );
                                ++server->count;
                                break;
                            }
                            p = (HotUrl*)LinkedListGet( server->url_list );
                            if( p == start )
                            {
                                Debug( 1, kSTREAM, "count of HotUrl error.\n" );
                                server->count = server->url_list->amount;
                                break;
                            }
                        }
                    }
                    // no more to be sent 
                    else if( server->count == server->url_list->amount )
                        continue;
                    else
                        Debug( 1, kSTREAM, "count of HotUrl unknown.\n" );
                }
            }
        }
        else
        {
            Debug( 1, kSTREAM, "epoll_wait unknown.\n" );
            break;
        }
    }
    // free 
    free( events );
    close( epoll_fd );
    LinkedListDestroy( server_list );
    return 0;
}// - - - - - - - end of function SendHotEx - - - - - - - - - - - //
*/

#endif /* WORK */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  SendHot
 *  Description:  
 * =====================================================================================
 */
#if 0
    static int
SendHot( AnalyseInfo *info )
{
    Url **hot;
    int hot_total;
    int index;
    int i;
    int amount;
    char **keyword;
    char command[1<<12];
    const char *command_format;

    command_format = info->conf->call_system;
    if( command_format == NULL )
    {
        Debug( 10, kSTREAM, "i think we need not to send request for hot url.\n" );
        return 0;
    }
    hot = info->hot;
    hot_total = info->hot_total;
    amount = info->conf->drag_keyword.amount;
    keyword = info->conf->drag_keyword.data;

    srand( (unsigned) time(NULL) );
    while( hot_total > 0 )
    {
        char *tmp;
        index = rand() % hot_total--;
        for( i = 0; i < amount; ++i )
        {
            int n = i << 1;
            if( drop_startend( keyword[n], keyword[n+1], hot[index] ) == 0 )
                break;
        }
        sprintf( command, command_format, hot[index] );
        Debug( 10, kSTREAM, command );
        system( command );
        sleep( 1 );
        tmp = hot[index];
        hot[index] = hot[hot_total];
        hot[hot_total] = tmp;
    }
    return 0;
} /* - - - - - - - end of function SendHot - - - - - - - - - - - */
#endif /* 0 */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  SaveHot
 *  Description:  
 * =====================================================================================
 */
static int
SaveHot( AnalyseInfo *info )
{
    Url **hot;
    int hot_total;
    FILE *fp;
    int i;
    fp = fopen( info->conf->hot_file_name, "w" );
    if( fp == NULL )
    {
        Debug( 1, kSTREAM, "save failed. cannot open %s.\n", info->conf->hot_file_name );
        return 1;
    }
    hot = info->hot;
    hot_total = info->hot_total;
    for( i = 0; i < hot_total; ++i )
        fprintf( fp, "%s\n", hot[i]->store_url );
    fclose( fp );
    return 0;
} /* - - - - - - - end of function SaveHot - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  CacheRatioOptimize
 *  Description:  
 * =====================================================================================
 */
    static int
CacheRatioOptimize( Configure *configure )
{
    int ret;
    HashConfigure hash_conf;
    Hash *hash;
    FILE *record_fp;
    Url **hot;
    AnalyseInfo info;


    /* read conf file */
    ret = ReadConfigureFile( configure );
    if( ret != 0 )
    {
        Debug( 1, kSTREAM, "error occured while configure file.\n" );
        return ret;
    }

    /* copy and truncate log file */
    ret = CopyAndTruncateLog( configure );
    if( ret != 0 )
    {
        Debug( 1, kSTREAM, "copy and truncating failed.\n" );
        return ret;
    }

    /* get time */
    if( configure->fix_time )
        configure->time_valid = configure->fix_time - configure->duration;
    else
        configure->time_valid = time(NULL) - configure->duration;

    /* create data file */
    rename( configure->data_file_name, configure->tmp_file_name );
    record_fp = fopen( configure->data_file_name, "w" );
    if( record_fp == NULL )
    {
        Debug( 1, kSTREAM, "cannot open data file" );
        return 1;
    }

    /* prepare analysing space for read data file */
    //    hash_conf.total = kHASH_SIZE;
    hash_conf.total = configure->max_urls;
    hash_conf.element_size = sizeof(Url);
    hash_conf.GetKey = UrlHashGetKey;
    hash_conf.IsEqual = UrlHashIsEqual;
    hash_conf.IsNull = UrlHashIsNull;
    hash_conf.DoFree = UrlHashDoFree;
    hash_conf.ElementPrint = UrlHashElementPrint;
    hash = HashCreate( &hash_conf );
    if( hash == NULL )
    {
        Debug( 1, kSTREAM, "cannot create hash" );
        fclose( record_fp );
        return 1;
    }
    //    hot = (char**)malloc( sizeof(char*) * kHASH_SIZE );
    hot = (Url**)malloc( sizeof(Url*) * configure->max_urls );
    if( hot == NULL )
    {
        Debug( 1, kSTREAM, "cannot get space for hot.\n" );
        HashDestroy( hash );
        fclose( record_fp );
        return 1;
    }
    //    memset( hot, 0, sizeof(char*) * kHASH_SIZE );
    memset( hot, 0, sizeof(Url*) * configure->max_urls );
    info.hash = hash;
    info.record_fp = record_fp;
    info.hot = hot;
    info.hot_total = 0;
    info.conf = configure;

    /* read data file */
    Analyse( configure->tmp_file_name, &info );

    /* read log file */
    Analyse( configure->bak_file_name, &info );
    fclose( record_fp );

    /* send or save result */
    SaveHot( &info );
    //SendHotEx( &info );
    Debug( 1, kSTREAM, "hash report %s\n", HashReport( hash ) );

    /* free */
    free( hot );
    HashDestroy( hash );
    ConfigureDestroy( configure );

    return 0;
} /* - - - - - - - end of function CacheRatioOptimize - - - - - - - - - - - */
void test(void)
{
    return;
}
    int
main( int argc, char *argv[] )
{
    Configure configure = {0};
    return CacheRatioOptimize( &configure );
} /* - - - - - - - end of function main - - - - - - - - - - - - - */
