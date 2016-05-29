/*
 * =====================================================================================
 *
 *       Filename:  mod_show.c
 *
 *    Description:  
 *
 *        Created:  03/16/2012 11:22:38 PM
 *       Compiler:  gcc
 *         Author:  Xu Jian (XJ), xujian@chinacache.com
 *        Company:  ChinaCache
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include "framework.h"
#include "dbg.h"
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  show
 *  Description:  
 * =====================================================================================
 */
int
show( const void *data )
{
    const DataInfo *d = (DataInfo*) data;
    char ip_from[16], ip_to[16];
    unsigned long ip;
    ip = ntohl( d->ip_from );
    inet_ntop( AF_INET, &ip, ip_from, 16 );
    ip = ntohl( d->ip_to );
    inet_ntop( AF_INET, &ip, ip_to, 16 );
    Debug( 30, "%s %s %s %.3lf %.3lf %.3lf %.3lf %.3lf %u %u %u %u",
            d->hostname,
            ip_from,
            ip_to,
            d->connect_time,
            d->first_byte_time,
            d->download_time,
            d->detect_used_time,
            d->packet_loss_rate,
            d->port,
            d->http_status,
            d->port_status
          );
    return 0;
} /* - - - - - - - end of function show - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  end
 *  Description:  
 * =====================================================================================
 */
int
end( const void *data )
{
    Debug( 40, "Show end!" );
    return 0;
} /* - - - - - - - end of function end - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  init
 *  Description:  
 * =====================================================================================
 */
int
init( const void *data )
{
    Debug( 40, "Show init! data = %p", data );
    return 0;
} /* - - - - - - - end of function init - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  RegisterModule
 *  Description:  
 * =====================================================================================
 */
int
RegisterModule( void )
{
    int ret = 0;
    Debug( 40, "Register show module!" );
    ret |= AddHookFunction( kHOOK_INIT, init );
    ret |= AddHookFunction( kHOOK_DATA_INPUT, show );
    ret |= AddHookFunction( kHOOK_END, end );
    return 0;
} /* - - - - - - - end of function RegisterModule - - - - - - - - - - - */
