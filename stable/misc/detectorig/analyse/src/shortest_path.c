/*
 * =====================================================================================
 *
 *       Filename:  shortest_path.c
 *
 *    Description:  analyse shortest path
 *
 *        Created:  05/15/2012 11:49:33 AM
 *       Compiler:  gcc
 *         Author:  Xu Jian (XJ), xujian@chinacache.com
 *        COMPANY:  ChinaCache
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <assert.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>
#include "hashtable.h"
#include "shortest_path.h"
#include "dbg.h"
#ifndef INFINITY
#define INFINITY 1E8
#endif /* not INFINITY */
static HashTable *route_table;
/* file name: link_route_data.<upstream_ip> */
static const char kUPSTREAM_DATA_PATH[] = "/usr/local/detectorig/route_result/";
static const char kUPSTREAM_DATA_FILE_NAME_PREFIX[] = "link_route_data";
static const char kOUTPUT_DATA_FILE_NAME[] = "/usr/local/detectorig/route_result/link_route_result";
static const char kTEMP_DATA_FILE_NAME[] = "/usr/local/detectorig/route_result/link_route_result.tmp";
static const char kFILE_HEADER_TEXT[] = ".XJ";
static const short kFILE_HEADER_VERSION_MAJOR = 1;
static const short kFILE_HEADER_VERSION_MINOR = 0;
FILE *output_file;
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  KeyMix
 *  Description:
 * =====================================================================================
 */
    static inline int
KeyMix( int x, int y )
{
    return x * 0x10001 + y;
} /* - - - - - - - end of function KeyMix - - - - - - - - - - - */
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  RoutePathKeyGen
 *  Description:
 * =====================================================================================
 */
    static inline HashKey
RoutePathKeyGen( RoutePath *path )
{
    return (HashKey)( KeyMix (\
                KeyMix( path->ip_from, path->ip_to ),\
                path->ip_to_port ) );
} /* - - - - - - - end of function RoutePathKeyGen - - - - - - - - - - - */
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  RoutePathGetKey
 *  Description:
 * =====================================================================================
 */
    static HashKey
RoutePathGetKey( const void *data )
{
    assert( data );
    return RoutePathKeyGen( (RoutePath*)data );
} /* - - - - - - - end of function RoutePathGetKey - - - - - - - - - - - */
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  RoutePathIsEqual
 *  Description:
 * =====================================================================================
 */
    static int
RoutePathIsEqual( const void *data1, const void *data2 )
{
    int is_equall = 0;
    RoutePath *s1, *s2;
    assert( data1 );
    assert( data2 );
    s1 = (RoutePath*)data1;
    s2 = (RoutePath*)data2;
    if( s1->ip_from == s2->ip_from &&
            s1->ip_to == s2->ip_to &&
            s1->ip_to_port == s2->ip_to_port )
    {
        is_equall = 1;
    }
    return is_equall;
} /* - - - - - - - end of function RoutePathIsEqual - - - - - - - - - - - */
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  RoutePathIsNull
 *  Description:
 * =====================================================================================
 */
    static int
RoutePathIsNull( const void *data )
{
    RoutePath *r;
    assert( data );
    r = (RoutePath*) data;
    return r->ip_to_port == 0;
} /* - - - - - - - end of function RoutePathIsNull - - - - - - - - - - - */
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  RoutePathDoFree
 *  Description:  Nothing needed to do.
 * =====================================================================================
 */
    static int
RoutePathDoFree( void *data )
{
    free( data );
    return 0;
} /* - - - - - - - end of function RoutePathDoFree - - - - - - - - - - - */
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  ShortestPathWriteFileHeader
 *  Description:
 * =====================================================================================
 */
    static int
ShortestPathWriteFileHeader( void )
{
    RoutePathFileHeader header;
    assert( output_file );
    if( output_file == NULL )
        return 0;
    assert( ftell( output_file ) == 0 );
    header.text[0] = kFILE_HEADER_TEXT[0];
    header.text[1] = kFILE_HEADER_TEXT[1];
    header.text[2] = kFILE_HEADER_TEXT[2];
    header.text[3] = '\0';
    header.version.major = kFILE_HEADER_VERSION_MAJOR;
    header.version.minor = kFILE_HEADER_VERSION_MINOR;
    return !fwrite( &header, sizeof(header), 1, output_file );
} /* - - - - - - - end of function ShortestPathWriteFileHeader - - - - - - - - - - - */
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  ShortestPathWriteInit
 *  Description:
 * =====================================================================================
 */
    static int
ShortestPathWriteInit( void )
{
    output_file = fopen( kTEMP_DATA_FILE_NAME, "w" );
    if( output_file == NULL )
    {
        Debug( 10, "Cannot open %s to write.", kTEMP_DATA_FILE_NAME );
        return 1;
    }
    return ShortestPathWriteFileHeader();
} /* - - - - - - - end of function ShortestPathWriteInit - - - - - - - - - - - */
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  ShortestPathWriteFinish
 *  Description:  rename data file
 * =====================================================================================
 */
    static int
ShortestPathWriteFinish( void )
{
    assert( output_file );
    fclose( output_file );
    rename( kTEMP_DATA_FILE_NAME, kOUTPUT_DATA_FILE_NAME );
    Debug( 50, "mv %s %s", kTEMP_DATA_FILE_NAME, kOUTPUT_DATA_FILE_NAME );
    return 0;
} /* - - - - - - - end of function ShortestPathWriteFinish - - - - - - - - - - - */
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  ShortestPathWriteFileData
 *  Description:
 * =====================================================================================
 */
    int
ShortestPathWriteFileData( RoutePath *path )
{
    static const size_t size = sizeof(RoutePath) - sizeof(uint32_t);
    assert( path );
    assert( output_file );
    return !fwrite( &path->ip_to, size, 1, output_file );
} /* - - - - - - - end of function ShortestPathWriteFileData - - - - - - - - - - - */
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  ShortestPathWriteTest
 *  Description:
 * =====================================================================================
 */
    int
ShortestPathWriteTest( void )
{
    char file_name[1024] = {0};
    uint32_t ip;
    time_t t;
    static const size_t size = sizeof(RoutePath) - sizeof(uint32_t);
    RoutePath path;
    if( output_file != NULL )
        return 0;
    inet_pton( AF_INET, "192.168.100.186", &ip );
    time( &t );
    snprintf( file_name, 1024, "%s%s.%u.%u", kUPSTREAM_DATA_PATH, kUPSTREAM_DATA_FILE_NAME_PREFIX, ip, (int)t );
    output_file = fopen( file_name, "w" );
    ShortestPathWriteFileHeader();
    path.ip_from = 0;
    path.ip_to_port = htons(80);
    inet_pton( AF_INET, "192.168.100.135", &path.ip_to );
    path.state.used_time = 0.01;
    fwrite( &path.ip_to, size, 1, output_file );
    inet_pton( AF_INET, "192.168.100.133", &path.ip_to );
    path.state.used_time = 0.001;
    fwrite( &path.ip_to, size, 1, output_file );
    fclose( output_file );
    return 0;
} /* - - - - - - - end of function ShortestPathWriteTest - - - - - - - - - - - */
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  ShortestPathReadFile
 *  Description:  Do malloc(), need call free()
 * =====================================================================================
 */
    static int
ShortestPathReadFile( FILE *fp, uint32_t ip )
{
    RoutePath *path, *same_path;
    RoutePathFileHeader header;
    static const size_t size = sizeof(RoutePath) - sizeof(uint32_t);
    assert( fp );
    /* Is empty file? */
    if( fread( &header, sizeof(header), 1, fp ) != 1 )
        return 0;
    /* Is not valid file? */
    if( strcmp( header.text, kFILE_HEADER_TEXT ) != 0 ||
            header.version.major != kFILE_HEADER_VERSION_MAJOR ||
            header.version.minor != kFILE_HEADER_VERSION_MINOR
      )
    {
        Debug( 50, "File is not my data." );
        return 0;
    }
    while( !feof( fp ) )
    {
        path = (RoutePath*)malloc( sizeof(RoutePath) );
        if( path == NULL )
        {
            Debug( 10, "Failed! %d! malloc()!", __LINE__ );
            return 1;
        }
        if( fread( &path->ip_to, size, 1, fp ) == 0 )
        {
            free( path );
            if( feof( fp ) )
                break;
            else
            {
                Debug( 20, "Unknown file format." );
                return 0;
            }
        }
        path->ip_from = ip;
        same_path = HashSearch( route_table, path );
        if( same_path )
        {
            Debug( 40, "Read same path from %u to %u:%d. Ignore it.", path->ip_from, path->ip_to, path->ip_to_port );
            Debug( 50, "The old used_time is %lf, and the new one is %lf", same_path->state.used_time, path->state.used_time );
            free( path );
        }
        else
            HashInsert( route_table, path );
    }
    return 0;
} /* - - - - - - - end of function ShortestPathReadFile - - - - - - - - - - - */
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  ParseFileName
 *  Description:  return 1: 文件名无法解析，后续工作应该忽略掉这个文件
 * =====================================================================================
 */
    static int
ParseFileName( const char *file_name, uint32_t *ip, time_t *t )
{
    const char *p;
    assert( file_name );
    assert( ip );
    assert( t );
    for( p = file_name; *p != 0 && *p != '.'; ++p );
    if( strncmp( file_name, kUPSTREAM_DATA_FILE_NAME_PREFIX, p - file_name ) )
        return 1;
    ++p;
    //if( sscanf( p, "%u.%u", ip, (int*)t ) != 2 )
    if( sscanf( p, "%u", ip ) != 1 )
        return 1;
    return 0;
} /* - - - - - - - end of function ParseFileName - - - - - - - - - - - */
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  ShortestPathReadUpstreamData
 *  Description:
 * =====================================================================================
 */
    int
ShortestPathReadUpstreamData( void )
{
    DIR *dir;
    char data_file_name[1024], *ptr_file_name;
    char tmp_file_name[1024];
    size_t size;
    struct dirent *file_name;
    dir = opendir( kUPSTREAM_DATA_PATH );
    if( dir == NULL )
    {
        if( errno != ENOENT )
        {
            Debug( 10, "Cannot open upstream data directory. %s.", strerror(errno) );
            return 1;
        }
        else if( mkdir( kUPSTREAM_DATA_PATH, 0755 ) != 0 )
        {
            Debug( 10, "Cannot make upstream data directory. %s.", strerror(errno) );
            return 1;
        }
        else
        {
            Debug( 40, "Do mkdir %s.", kUPSTREAM_DATA_PATH );
            dir = opendir( kUPSTREAM_DATA_PATH );
            if( dir == NULL )
            {
                Debug( 1, "Unknown error, when opening %s.", kUPSTREAM_DATA_PATH );
                return 1;
            }
        }
    }
    ptr_file_name = data_file_name;
    ptr_file_name += size = snprintf( ptr_file_name, 1024, "%s", kUPSTREAM_DATA_PATH );
    snprintf( tmp_file_name, 1024, "%stmp", kUPSTREAM_DATA_PATH );
    size = 1024 - size;
    while(( file_name = readdir( dir )) != NULL )
    {
        uint32_t ip;
        time_t t;
        FILE *fp;
        char *name = file_name->d_name;
        /* if not a upstream data file, which have a '.' after prefix strings */
        if( ParseFileName( name, &ip, &t ) != 0 )
        {
            Debug( 40, "File %s is ignored.", name );
            continue;
        }
        name = file_name->d_name;
        snprintf( ptr_file_name, size, "%s", name );
        rename( data_file_name, tmp_file_name );
        Debug( 50, "mv %s %s", data_file_name, tmp_file_name );
        fp = fopen( tmp_file_name, "r" );
        if( fp == NULL )
        {
            Debug( 40, "Cannot open tmp file, which is from %s.", data_file_name );
            continue;
        }
        if( ShortestPathReadFile( fp, ip ) != 0 )
            Debug( 30, "Error, when reading %s.", name );
        fclose( fp );
    }
    closedir( dir );
    unlink( tmp_file_name );
    return 0;
} /* - - - - - - - end of function ShortestPathReadUpstreamData - - - - - - - - - - - */
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  ShortestPathFinish
 *  Description:
 * =====================================================================================
 */
    int
ShortestPathFinish( void )
{
    ShortestPathWriteFinish();
    HashDestroy( route_table );
    free( route_table );
    route_table = NULL;
    return 0;
} /* - - - - - - - end of function ShortestPathFinish - - - - - - - - - - - */
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  ShortestPathInit
 *  Description:
 * =====================================================================================
 */
    int
ShortestPathInit( void )
{
    HashConfigure hash_conf = {0};
    memset( &hash_conf, 0, sizeof(HashConfigure) );
    hash_conf.total = 1<<10;
    hash_conf.name = "shortest_path_route_table";
    hash_conf.GetKey = RoutePathGetKey;
    hash_conf.IsEqual = RoutePathIsEqual;
    hash_conf.IsNull = RoutePathIsNull;
    hash_conf.DoFree = RoutePathDoFree;
    route_table = HashCreate( &hash_conf );
    if( route_table == NULL )
    {
        Debug( 10, "Cannot create hash table." );
        return 1;
    }
    if( ShortestPathReadUpstreamData() != 0 ||
            ShortestPathWriteInit() != 0 )
    {
        HashDestroy( route_table );
        free( route_table );
        route_table = NULL;
        return 1;
    }
    return 0;
} /* - - - - - - - end of function ShortestPathInit - - - - - - - - - - - */
///*
// * ===  FUNCTION  ======================================================================
// *         Name:  ShortestPathInsert
// *  Description:
// * =====================================================================================
// */
//    static int
//ShortestPathInsert( RoutePath *route_path )
//{
//    assert( route_path );
//    HashInsert( route_table, route_path );
//    return 0;
//} /* - - - - - - - end of function ShortestPathInsert - - - - - - - - - - - */
///*
// * ===  FUNCTION  ======================================================================
// *         Name:  ShortestPathInputData
// *  Description:
// * =====================================================================================
// */
//    int
//ShortestPathInputData( RoutePath *route_path )
//{
//    RoutePath *same_path;
//    assert( route_path );
//    same_path = HashSearch( route_table, route_path );
//    /* 如果没有相同的路径（照理来说应该没有） */
//    if( same_path == NULL )
//        HashInsert( route_table, route_path );
//    else
//    {
//        /* TODO */
//        Debug( 20, "Get same path!" );
//    }
//    return 0;
//} /* - - - - - - - end of function ShortestPathInputData - - - - - - - - - - - */
///*
// * ===  FUNCTION  ======================================================================
// *         Name:  ShortestPathGet
// *  Description:
// * =====================================================================================
// */
//    static RoutePath*
//ShortestPathGet( uint32_t middle_ip, uint32_t final_ip, uint16_t final_port )
//{
//    RoutePath *path;
//    RoutePath path_info;
//    path_info.ip_from = middle_ip;
//    path_info.ip_to = final_ip;
//    path_info.ip_to_port = final_port;
//    path_info.key = RoutePathKeyGen( &path_info );
//    path = HashSearch( route_table, &path_info );
//    return path;
//} /* - - - - - - - end of function ShortestPathGet - - - - - - - - - - - */
///*
// * ===  FUNCTION  ======================================================================
// *         Name:  ShortestPathCalcUsedTime
// *  Description:
// * =====================================================================================
// */
//    double
//ShortestPathCalcUsedTime( void *ip, void *detect )
//{
//    double min_used_time;
//    int i;
//    int total;
//    RoutePath *path;
//    RoutePath path_info;
//    struct IpDetect *ip_detect = (struct IpDetect*)ip;
//    struct DetectOrigin *detect_origin = (struct DetectOrigin*)detect;
//    assert( ip );
//    assert( detect );
//    //min_used_time = 1e10;
//    min_used_time = INFINITY;
//    inet_pton( AF_INET, ip_detect->ip, (void*)&path_info.ip_from );
//    total = detect_origin->final_ip_number;
//    for( i = 0; i < total; ++i )
//    {
//        double time;
//        path_info.ip_to = detect_origin->final_ip[i];
//        path_info.ip_to_port = detect_origin->final_ip_port[i];
//        if( path_info.ip_from == path_info.ip_to )
//        {
//            ip_detect->final_ip = path_info.ip_to;
//            ip_detect->final_ip_port = path_info.ip_to_port;
//            return 0.0;
//        }
//        path = HashSearch( route_table, &path_info );
//        /* 如果有对应的记录 */
//        if( path != NULL )
//        {
//            Debug( 40, "Find path, from %X to %X:%X, %lfs(min %lfs)",
//                    path->ip_from,
//                    path->ip_to,
//                    path->ip_to_port,
//                    path->state.used_time,
//                    min_used_time );
//            time = path->state.used_time;
//            if( time < min_used_time )
//            {
//                ip_detect->final_ip = path->ip_to;
//                ip_detect->final_ip_port = path->ip_to_port;
//                min_used_time = time;
//            }
//        }
//    }
//    return min_used_time;
//} /* - - - - - - - end of function ShortestPathCalcUsedTime - - - - - - - - - - - */
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  ShortestPathGetRecord
 *  Description:
 * =====================================================================================
 */
    RoutePath*
ShortestPathGetRecord( const RoutePath *path )
{
    assert( path );
    assert( route_table );
    return HashSearch( route_table, path );
} /* - - - - - - - end of function ShortestPathGetRecord - - - - - - - - - - - */
