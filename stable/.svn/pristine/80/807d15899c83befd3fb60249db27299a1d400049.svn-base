/*
 * =====================================================================================
 *
 *       Filename:  shortest_path.h
 *
 *    Description:  
 *
 *        Created:  05/16/2012 09:55:15 AM
 *       Compiler:  gcc
 *         Author:  Xu Jian (XJ), xujian@chinacache.com
 *        COMPANY:  ChinaCache
 *
 * =====================================================================================
 */
#ifndef _SHORTEST_PATH_H_BF8C49F6_946F_4653_B09D_020BF914BCA5_
#define _SHORTEST_PATH_H_BF8C49F6_946F_4653_B09D_020BF914BCA5_ 
#include "hashtable.h"
#include <stdint.h>
#define ROUTE
typedef struct _RoutePath 
{
    uint32_t ip_from;                           /* network byte order */
    uint32_t ip_to;                             /* network byte order */
    uint16_t ip_to_port;                        /* network byte order */
    struct
    {
        double used_time;
    } state;
}__attribute__((packed)) RoutePath; /* - - - - - - end of struct RoutePath - - - - - - */
typedef struct _RoutePathFileHeader 
{
    char text[4];
    struct
    {
        unsigned short major;
        unsigned short minor;
    } version;
} RoutePathFileHeader; /* - - - - - - end of struct RoutePathFileHeader - - - - - - */
int ShortestPathInit( void );
RoutePath* ShortestPathGetRecord( const RoutePath *path );
int ShortestPathWriteFileData( RoutePath *path );
int ShortestPathWriteTest( void );
int ShortestPathFinish( void );
#endif /* _SHORTEST_PATH_H_BF8C49F6_946F_4653_B09D_020BF914BCA5_ */
