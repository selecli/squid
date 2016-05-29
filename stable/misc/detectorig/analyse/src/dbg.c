/*
 * =====================================================================================
 *
 *       Filename:  dbg.c
 *
 *    Description:  debug functions
 *
 *        Created:  03/15/2012 12:05:10 PM
 *       Compiler:  gcc
 *         Author:  Xu Jian (XJ), xujian@chinacache.com
 *        COMPANY:  ChinaCache
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "dbg.h"
static const char kDEBUG_LOG_FILE[] = "/var/log/chinacache/analyse.log";
enum{
    kDEBUG_LOG_ROTATE_COUNT = 5,
    kDEBUG_LOG_ROTATE_SIZE = 1<<27              /* 128MB */
};
int debug_level = 1;
static FILE *log_stream = NULL;
//#define XjPrint(fmt,arg...) debug(77,8)("xj:%s:%d "fmt, __FUNCTION__, __LINE__, ##arg)
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  SetDebugOutputToStderr
 *  Description:
 * =====================================================================================
 */
    void
SetDebugOutputToStderr( void )
{
    log_stream = stderr;
} /* - - - - - - - end of function SetDebugOutputToStderr - - - - - - - - - - - */
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  SetDebugLevel
 *  Description:
 * =====================================================================================
 */
    int
SetDebugLevel( int level )
{
    int old_level = debug_level;
    debug_level = level;
    return old_level;
} /* - - - - - - - end of function SetDebugLevel - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  _Debug
 *  Description:  
 * =====================================================================================
 */
    int
_Debug( const char *format, ... )
{
    const static int kSIZE = 1<<10;
    int size;
    char buf[kSIZE];
    va_list arg_list;
    va_start( arg_list, format );
    size =  vsnprintf( buf, kSIZE, format, arg_list );
    if( size > kSIZE - 1 )
    {
        buf[kSIZE - 5] = buf[kSIZE - 3] = buf[kSIZE - 4] = '.';
        buf[kSIZE - 2] = '\n';
        size = kSIZE;
    }
    fputs( buf, log_stream );
    va_end( arg_list );
    return size;

} /* - - - - - - - end of function _Debug - - - - - - - - - - - */
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  LogRotate
 *  Description:
 * =====================================================================================
 */
    static int
LogRotate( void )
{
    struct stat info;
    if( stat( kDEBUG_LOG_FILE, &info ) == 0 )
    {
        if( info.st_size >= kDEBUG_LOG_ROTATE_SIZE )
        {
            int i;
            char from[1024];
            char to[1024];
            for( i = kDEBUG_LOG_ROTATE_COUNT - 2; i >= 0; --i )
            {
                if( snprintf( from, 1024, "%s.%d", kDEBUG_LOG_FILE, i     ) >= 1024 ||
                        snprintf( to  , 1024, "%s.%d", kDEBUG_LOG_FILE, i + 1 ) >= 1024 )
                {
                    fprintf( stderr, "Name of rotate-file is too long.\n" );
                    return 1;
                }
                rename( from, to );
            }
            rename( kDEBUG_LOG_FILE, from );
        }
        return 0;
    }
    else if( errno == ENOENT )
        return 0;
    else
    {
        fprintf( stderr, "Error when do stat(). %s.\n", strerror( errno ) );
        return 1;
    }
} /* - - - - - - - end of function LogRotate - - - - - - - - - - - */
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  DebugInit
 *  Description:
 * =====================================================================================
 */
    int
DebugInit( void )
{
    if( log_stream == NULL )
    {
        if( LogRotate() != 0 )
            return 1;
        log_stream = fopen( kDEBUG_LOG_FILE, "a" );
        if( log_stream == NULL )
        {
            fprintf( stderr, "Cannot open %s\n", kDEBUG_LOG_FILE );
            return 1;
        }
    }
    return 0;
} /* - - - - - - - end of function DebugInit - - - - - - - - - - - */
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  DebugEnd
 *  Description:
 * =====================================================================================
 */
    int
DebugEnd( void )
{
    if( log_stream != stderr )
        fclose( log_stream );
    return 0;
} /* - - - - - - - end of function DebugEnd - - - - - - - - - - - */
