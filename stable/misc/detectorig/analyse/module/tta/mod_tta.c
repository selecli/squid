/*
 * =====================================================================================
 *
 *       Filename:  main.c
 *
 *    Description:  
 *
 *        Created:  03/28/2012 05:07:50 PM
 *       Compiler:  gcc
 *         Author:  Xu Jian (XJ), xujian@chinacache.com
 *        Company:  ChinaCache
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <arpa/inet.h>
#include <assert.h>
#include "framework.h"
#include "hashtable.h"
#include "linkedlist.h"
#include "shortest_path.h"
#include "dbg.h"
#ifndef INFINITY
#define INFINITY (1E8)
#endif /* not INFINITY */
#ifdef ROUTE
typedef struct _FinalIp 
{
    uint32_t ip;                                /* net byte order */
    uint16_t port;                              /* net byte order */
} FinalIp; /* - - - - - - end of struct FinalIp - - - - - - */
#endif /* ROUTE */
typedef struct _IpStatus
{
    uint32_t ip_from;
    uint32_t ip_to;
    uint32_t mark;
    uint32_t final_ip;
    uint16_t final_ip_port;
    double detect_used_time;
} IpStatus; /* - - - - - - end of struct IpStatus - - - - - - */
typedef struct _TtaChannel
{
    HashKey key;
    char *name;
    uint16_t port;                              /* host byte order */
    IpStatus *best_ip;                          /* no need to call free() */
    IpStatus *best_backup_ip;                   /* no need to call free() */
    LinkedList *ip_status;
    LinkedList *backup_ip_status;
#ifdef ROUTE
    LinkedList *final_ip;
#endif /* ROUTE */
} TtaChannel; /* - - - - - - end of struct TtaChannel - - - - - - */
typedef struct _ChannelConf
{
    TtaChannel channel;
    int is_enable_modify;
    char ip[2][16];
    char *buffer;
    int ip_count;                               /* 存放ip中共有几个有效ip */
    int ip_backup;                              /* 记录ip中第几个是backup ip（0 or 1） */
    char *point_ip_in_buffer;                   /* buffer中，ip的大概起始位置 */
    char *point_backup_ip_in_buffer;            /* buffer中，backup ip的大概起始位置 */
} ChannelConf; /* - - - - - - end of struct ChannelConf - - - - - - */
enum
{
    kTTA_LINK_DATA_READ_COUNT_MAX = 1 /* 最多读取kLINK_DATA_READ_COUNT_MAX个link.data文件中的数据 */
};
static const int kIPSTATUS_RESERVE = 0;
static const char kINPUT_FILE[] = "/usr/local/haproxy/etc/tta_domain.conf";
static const char kOUTPUT_FILE[] = "/usr/local/haproxy/etc/haproxy.cfg";
static const char kENABLE_MODIFY_KEYWORD[] = "tta-detect";
static int module_enable = 1;
static HashTable *all_channel;
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  FinalIpIsEqual
 *  Description:
 * =====================================================================================
 */
    static int
FinalIpIsEqual( const void *data1, const void *data2 )
{
    FinalIp *ip1 = (FinalIp*)data1;
    FinalIp *ip2 = (FinalIp*)data2;
    assert( ip1 );
    assert( ip2 );
    if( ip1->ip != ip2->ip )
        return 0;
    if( ip1->port != ip2->port )
        return 0;
    return 1;
} /* - - - - - - - end of function FinalIpIsEqual - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  IpStatusIsEqual
 *  Description:  
 * =====================================================================================
 */
    static int
IpStatusIsEqual( const void *data1, const void *data2 )
{
    IpStatus *ip1, *ip2;
    assert( data1 );
    assert( data2 );
    ip1 = (IpStatus*)data1;
    ip2 = (IpStatus*)data2;
    if( ip1->ip_to != ip2->ip_to )
        return 0;
    if( ip1->ip_from != kIPSTATUS_RESERVE && ip2->ip_from != kIPSTATUS_RESERVE )
        if( ip1->ip_from != ip2->ip_from )
            return 0;
    return 1;
} /* - - - - - - - end of function IpStatusIsEqual - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  TtaChannelKeyGen
 *  Description:  
 * =====================================================================================
 */
    static inline HashKey
TtaChannelKeyGen( char *name, uint16_t port )
{
    char *p;
    HashKey key;
    key = 0;
    for( p = name; *p != '\0'; ++p )
        key = ( key << 4 ) + key + *p;
    key += port;
    return key;
} /* - - - - - - - end of function TtaChannelKeyGen - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  TtaChannelGetKey
 *  Description:  
 * =====================================================================================
 */
    HashKey
TtaChannelGetKey( const void *data )
{
    TtaChannel *channel = (TtaChannel*)data;
    assert( data );
    return channel->key;
} /* - - - - - - - end of function TtaChannelGetKey - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  TtaChannelIsEqual
 *  Description:  频道名和端口都相同，才认为相同
 * =====================================================================================
 */
    int
TtaChannelIsEqual( const void *data1, const void *data2 )
{
    TtaChannel *c1, *c2;
    assert( data1 );
    assert( data2 );
    c1 = (TtaChannel*)data1;
    c2 = (TtaChannel*)data2;
    if( c1->key != c2->key )
        return 0;
    else if( c1->port != c2->port )
        return 0;
    else if( strcmp( c1->name, c2->name ) )
        return 0;
    else
        return 1;
} /* - - - - - - - end of function TtaChannelIsEqual - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  TtaChannelDestroy
 *  Description:  Destroy all in data, but not free data.
 *                Call free() if needed
 * =====================================================================================
 */
    static int
TtaChannelDestroy( void *data )
{
    TtaChannel *channel = (TtaChannel*)data;
    assert( data );
    if( channel->name != NULL )
        free( channel->name );
    if( channel->ip_status != NULL )
    {
        LinkedListDestroy( channel->ip_status );
        free( channel->ip_status );
    }
    if( channel->backup_ip_status != NULL )
    {
        LinkedListDestroy( channel->backup_ip_status );
        free( channel->backup_ip_status );
    }
    if( channel->final_ip != NULL )
    {
        LinkedListDestroy( channel->final_ip );
        free( channel->final_ip );
    }
    memset( channel, 0, sizeof(TtaChannel) );
    return 0;
} /* - - - - - - - end of function TtaChannelDestroy - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  TtaChannelDoFree
 *  Description:  Call free() on data and all in data.
 * =====================================================================================
 */
    int
TtaChannelDoFree( void *data )
{
    assert( data );
    TtaChannelDestroy( data );
    free( data );
    return 0;
} /* - - - - - - - end of function TtaChannelDoFree - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  SelectBestIp
 *  Description:  
 * =====================================================================================
 */
    static int
SelectBestIp( void *local, void *param )
{
    IpStatus *ip_linked, **ip_optimize;
    assert( param );
    if( local == NULL ) 
    {
        Debug( 30, "Cannot do optimizing, while data is empty in traversal." );
        return 1;
    }
    ip_linked = local;
    ip_optimize = param;
    if( *ip_optimize == NULL )
        *ip_optimize = local;
    else if( ip_linked->mark > (*ip_optimize)->mark )
    {
        Debug( 30, "find mark %u is larger than %u.", ip_linked->mark, (*ip_optimize)->mark );
        *ip_optimize = ip_linked;
    }
    return 0;
} /* - - - - - - - end of function SelectBestIp - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  TtaCannelOptimizeSigle
 *  Description:  
 * =====================================================================================
 */
    static int
TtaCannelOptimizeSigle( void *local, void *must_be_null )
{
    TtaChannel *channel;
    assert( must_be_null == NULL );
    channel = (TtaChannel*)local;
    assert( channel->best_ip == NULL );
    assert( channel->best_backup_ip == NULL );
    LinkedListTraversal( channel->ip_status, SelectBestIp, &channel->best_ip );
    LinkedListTraversal( channel->backup_ip_status, SelectBestIp, &channel->best_backup_ip );
    return 0;
} /* - - - - - - - end of function TtaCannelOptimizeSigle - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  TtaChannelOptimize
 *  Description:  
 * =====================================================================================
 */
    static int
TtaChannelOptimize( HashTable *channel_table )
{
    HashTraversal( channel_table, TtaCannelOptimizeSigle, NULL );
    return 0;
} /* - - - - - - - end of function TtaChannelOptimize - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ParseLine
 *  Description:  
 * =====================================================================================
 */
    static int
ParseLine( TtaChannel *data, char *line )
{
    typedef struct _ParseLineDataType
    {
        enum
        {
            kPARSE_UINT16,
            kPARSE_STRING,
            kPARSE_IPGROUP,
#ifdef ROUTE
            kPARSE_IPPORT,
#endif /* ROUTE */
            /* other */
            kPARSE_NULL,
            kPARSE_END,
            kPARSE_RESERVE
        } type;
        void *data;
    } ParseLineDataType; /* - - - - - - end of struct ParseLineDataType - - - - - - */
    char *token;
    static const char kIP_DELIMITER[] = "|";
    static const char kDELIMITER[] = " \t\n";
    ParseLineDataType *format;
    ParseLineDataType line_format[] = {
        {kPARSE_STRING,(void*)&data->name},
        {kPARSE_NULL,NULL},
        {kPARSE_NULL,NULL},
        {kPARSE_NULL,NULL},
        {kPARSE_NULL,NULL},
        {kPARSE_NULL,NULL},
        {kPARSE_NULL,NULL},
        {kPARSE_NULL,NULL},
        {kPARSE_NULL,NULL},
        {kPARSE_IPGROUP,(void*)&data->backup_ip_status},
        {kPARSE_NULL,NULL},
        {kPARSE_NULL,NULL},
        {kPARSE_IPGROUP,(void*)&data->ip_status},
        {kPARSE_NULL,NULL},
        {kPARSE_UINT16,(void*)&data->port},
#ifdef ROUTE
        {kPARSE_NULL,NULL},
        {kPARSE_NULL,NULL},
        {kPARSE_NULL,NULL},
        {kPARSE_NULL,NULL},
        {kPARSE_NULL,NULL},
        {kPARSE_NULL,NULL},
        {kPARSE_NULL,NULL},
        {kPARSE_IPPORT, (void*)&data->final_ip},
#endif /* ROUTE */
        {kPARSE_END,      NULL}
    };
    memset( data, 0, sizeof(TtaChannel) );
    data->port = 80;                            /* default port */
    for( format = line_format; format->type != kPARSE_END; ++ format )
    {
RETRY:
        token = strsep( &line, kDELIMITER );
        /* 没有信息可以读取了 */
        if( token == NULL )
            break;
        else if( *token == '\0' )
            goto RETRY;
        /* 正常读取 */
        switch( format->type )
        {
            case kPARSE_UINT16:
                {
                    char *cannot_be_read = 0;
                    long l = strtol( token, &cannot_be_read, 10 );
                    if( *cannot_be_read != '\0' )
                    {
                        Debug( 20, "Read a non-uint16_t data." );
                        goto FAILED;
                    }
                    if( l >= 0xFFFF )
                        *(uint16_t*)format->data = 0xFFFF;
                    else if ( l <= 0 )
                        *(uint16_t*)format->data = 0;
                    else
                        *(uint16_t*)format->data = (uint16_t)l;
                }
                break;
            case kPARSE_STRING:
                {
                    *(char**)format->data = strdup( token );
                }
                break;
            case kPARSE_IPGROUP:
                {
                    char *ip;
                    LinkedList **ip_status = (LinkedList**)format->data;
                    LinkedListConfigure linked_conf;
                    /* if do not want detect, token is "no" */
                    if( strcmp( token, "no" ) == 0 )
                    {
                        *ip_status = NULL;
                        break;
                    }
                    linked_conf.IsEqual = IpStatusIsEqual;
                    linked_conf.DoFree = (LinkedListDoFree)free;
                    *ip_status = LinkedListCreate( &linked_conf );
                    if( *ip_status == NULL )
                    {
                        Debug( 20, "Cannot create linked list for ip_status." );
                        goto FAILED;
                    }
                    if( strcmp( token, "0.0.0.0" ) == 0 )
                        break;
                    while(( ip = strsep( &token, kIP_DELIMITER )) != NULL )
                    {
                        IpStatus *status;
                        status = (IpStatus *)malloc( sizeof(IpStatus) );
                        if( status == NULL )
                        {
                            Debug( 10, "Failed! %d! malloc()!", __LINE__ );
                            goto FAILED;
                        }
                        memset( status, 0, sizeof(IpStatus) );
                        if( inet_pton( AF_INET, ip, (void*)&status->ip_to ) != 1 )
                        {
                            free( status );
                        }
                        else
                        {
                            status->ip_from = kIPSTATUS_RESERVE;
                            status->mark = 0;
                            LinkedListInsert( *ip_status, status );
                        }
                    }
                }
                break;
#ifdef ROUTE
            case kPARSE_IPPORT:
                {
                    char *ip_port;
                    LinkedList **final_ip_list = (LinkedList**)format->data;
                    LinkedListConfigure linked_conf;
                    linked_conf.IsEqual = FinalIpIsEqual;
                    linked_conf.DoFree = (LinkedListDoFree)free;
                    *final_ip_list = LinkedListCreate( &linked_conf );
                    if( *final_ip_list == NULL )
                    {
                        Debug( 20, "Cannot create linked list for final_ip_list." );
                        goto FAILED;
                    }
                    while(( ip_port = strsep( &token, kIP_DELIMITER )) != NULL )
                    {
                        char *ip;
                        uint16_t port;
                        FinalIp *final_ip;
                        final_ip = (FinalIp*)malloc( sizeof(FinalIp) );
                        if( final_ip == NULL )
                        {
                            Debug( 10, "Failed! %d! malloc()!", __LINE__ );
                            goto FAILED;
                        }
                        ip = strsep( &ip_port, ":" );
                        port = *ip_port != '\0' ? atoi( ip_port ): 80;
                        port = htons( port );
                        if( inet_pton( AF_INET, ip, (void*)&final_ip->ip ) != 1 )
                        {
                            free( final_ip );
                        }
                        else
                        {
                            final_ip->port = port;
                            LinkedListInsert( *final_ip_list, final_ip );
                        }
                    }
                }
                break;
#endif /* ROUTE */
            case kPARSE_NULL:
                Debug( 50, "Igore token in parse tta input configure." );
                break;
            default:
                Debug( 1, "Unknown error!" );
                TtaChannelDoFree( data );
                return 1;
        }
    }
    token = data->name + strlen( data->name ) - 1;
    if( *token == '.' )
        *token = '\0';
    data->key = TtaChannelKeyGen( data->name, data->port );
    Debug( 30, "module tta, parse domain.conf, get %s:%u.", data->name, data->port );
    return 0;
FAILED:
#if 0
    if( data->name )
    {
        free( data->name );
        if( data->backup_ip_status )
        {
            LinkedListDestroy( data->backup_ip_status );
            if( data->ip_status )
                LinkedListDestroy( data->ip_status );
        }
        memset( data, 0, sizeof(TtaChannel) );
    }
#else /* not 0 */
    TtaChannelDestroy( data );
#endif /* not 0 */
    return 1;
} /* - - - - - - - end of function ParseLine - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ReadInputFile
 *  Description:  
 * =====================================================================================
 */
    static int
ReadInputFile( FILE *file, HashTable *channel )
{
    const int kLINE_LEGNTH = 1024;
    char line[kLINE_LEGNTH];
    TtaChannel *data;
    while( fgets( line, kLINE_LEGNTH, file ) != NULL )
    {
        char *p;
        /* 去除前部分的空格和制表符 */
        for( p = line; *p != '\0'; ++p )
            if( *p != ' ' && *p != '\t' )
                break;
        /* 忽略空行和行注释 */
        if( *p == '#' || *p == '\n' )
            continue;
        Debug( 50, "%s", p );
        /* parse data */
        data = (TtaChannel *)malloc( sizeof(TtaChannel) );
        if( data == NULL )
        {
            Debug( 10, "Failed! %d! malloc()!", __LINE__ );
            break;
        }
        memset( data, 0, sizeof(TtaChannel) );
        if( ParseLine( data, p ) == 0 )
            HashInsert( channel, data );
    }
    return 0;
} /* - - - - - - - end of function ReadInputFile - - - - - - - - - - - */
typedef struct _BestFinalIp 
{
    uint32_t ip_from;
    FinalIp *ip;
    double min_detect_used_time;
} BestFinalIp; /* - - - - - - end of struct BestFinalIp - - - - - - */
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  TraversalFinalIpToFindBestOne
 *  Description:
 * =====================================================================================
 */
    static int
TraversalFinalIpToFindBestOne( void *final_ip_data, void *best_one )
{
    RoutePath *path, path_info;
    FinalIp *final_ip = (FinalIp*)final_ip_data;
    BestFinalIp *best_final_ip = (BestFinalIp*)best_one;
    assert( final_ip );
    assert( best_final_ip );
    path_info.ip_from = best_final_ip->ip_from;
    path_info.ip_to = final_ip->ip;
    path_info.ip_to_port = final_ip->port;
    if( path_info.ip_to == path_info.ip_from )
    {
        Debug( 40, "Detected itself 0x%X! Something wrong?", path_info.ip_to );
        best_final_ip->ip = final_ip;
        best_final_ip->min_detect_used_time = 0.0;
        return 1;
    }
    path = ShortestPathGetRecord( &path_info );
    if( path == NULL )
        return 0;
    Debug( 50, "Tta find path, from %X to %X:%X, %lfs(min %lfs)",
            path->ip_from,
            path->ip_to,
            path->ip_to_port,
            path->state.used_time,
            best_final_ip->min_detect_used_time );
    if( path->state.used_time < best_final_ip->min_detect_used_time )
    {
        best_final_ip->ip = final_ip;
        best_final_ip->min_detect_used_time = path->state.used_time;
    }
    return 0;
} /* - - - - - - - end of function TraversalFinalIpToFindBestOne - - - - - - - - - - - */
#ifndef ROUTE
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  CalculateMark
 *  Description:  
 * =====================================================================================
 */
    static uint32_t
CalculateMark( const DataInfo *data )
{
    uint32_t mark = 0;
    double d;
    if( data->port_status == kPORT_DOWN )
        return 0;
    d = data->detect_used_time;
    if( !isnan( d ) )
    {
        d = 10 - d;
        d *= 1e6;
        mark += d;
    }
    Debug( 30, "%s, from %u to %u, d = %lf, mark is %u", data->hostname, data->ip_from, data->ip_to, data->detect_used_time, mark );
    return mark;
} /* - - - - - - - end of function CalculateMark - - - - - - - - - - - */
#else /* ROUTE */
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  CalculateMark
 *  Description:
 * =====================================================================================
 */
    static int
CalculateMark( const DataInfo *data, LinkedList *final_ip, IpStatus *ip )
{
    uint32_t mark = 0;
    double used_time;
    BestFinalIp best_final_ip;
    assert( data );
    assert( ip );
    if( data->port_status == kPORT_DOWN )
        return 0;
    /* double DataInfo.detect_used_time */
    memset( &best_final_ip, 0, sizeof(BestFinalIp) );
    best_final_ip.ip_from = data->ip_to;
    best_final_ip.min_detect_used_time = INFINITY;
    LinkedListTraversal( final_ip, TraversalFinalIpToFindBestOne, (void*)&best_final_ip );
    if( best_final_ip.ip != NULL )
    {
        ip->final_ip = best_final_ip.ip->ip;
        ip->final_ip_port = best_final_ip.ip->port;
        used_time = best_final_ip.min_detect_used_time + data->detect_used_time;
    }
    else
    {
        ip->final_ip = 0;
        ip->final_ip_port = 0;
        used_time = data->detect_used_time;
    }
    ip->detect_used_time = used_time;
    if( !isnan( used_time ) )
    {
        used_time = 10 - used_time;
        used_time *= 1E6;
        mark += used_time;
    }
    Debug( 50, "%s, from %u to %u, d = %lf, mark is %u", data->hostname, data->ip_from, data->ip_to, data->detect_used_time, mark );
    ip->mark = mark;
    return 0;
} /* - - - - - - - end of function CalculateMark - - - - - - - - - - - */
#endif /* ROUTE */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ReadInputData
 *  Description:  
 * =====================================================================================
 */
    static int
ReadInputData( const void *data )
{
    TtaChannel receive_channel;
    TtaChannel *search_channel;
    IpStatus receive_ip;
    IpStatus *search_ip;
    const DataInfo *input_data = data;
    assert( data );
    if( input_data->file_order >= kTTA_LINK_DATA_READ_COUNT_MAX )
        return 0;
    /* 搜索所有频道记录 */
    receive_channel.name = strdup( input_data->hostname );
    receive_channel.port = input_data->port;
    receive_channel.key = TtaChannelKeyGen( receive_channel.name, receive_channel.port );
    receive_channel.ip_status = NULL;
    receive_channel.backup_ip_status = NULL;
    Debug( 50, "Module tta get a input data, %s:%u.", receive_channel.name, receive_channel.port );
    search_channel = (TtaChannel*)HashSearch( all_channel, (void*)&receive_channel );
    free( receive_channel.name );
    /* 找不到对应的频道，这条记录无用 */
    if( search_channel == NULL )
        return 0;
    /* 在频道中搜索ip_to */
    receive_ip.ip_to = input_data->ip_to;
    receive_ip.ip_from = kIPSTATUS_RESERVE;
    receive_ip.mark = 0;
    /* 在ip_status中搜索 */
    search_ip = (IpStatus*)LinkedListSearch( search_channel->ip_status, (void*)&receive_ip );
    /* 如果找不到这个ip */
    if( search_ip == NULL )
    {
        /* 在backup_ip_status中搜索 */
        search_ip = (IpStatus*)LinkedListSearch( search_channel->backup_ip_status, (void*)&receive_ip );
        /* 找不到对应的ip_to，但这条记录是做域名解析的记录 */
        if( search_ip == NULL && input_data->ip_type == kIP_TYPE_DNS_IP )
        {
            IpStatus *insert_ip = (IpStatus*)malloc( sizeof(IpStatus) );
            if( insert_ip == NULL )
            {
                Debug( 10, "Failed! %d! malloc()!", __LINE__ );
                return 0;
            }
            memcpy( insert_ip, &receive_ip, sizeof(IpStatus) );
            LinkedListInsert( search_channel->ip_status, (void*)insert_ip );
            search_ip = (IpStatus*)LinkedListSearch( search_channel->ip_status, (void*)&receive_ip );
        }
        else
            return 0;
    }
    /* 该记录有效，为记录进行评分，然后将ip_from和评分计入频道记录中 */
    search_ip->ip_from = input_data->ip_from;
#ifndef ROUTE
    search_ip->mark = CalculateMark( input_data );
#else /* ROUTE */
    CalculateMark( input_data, search_channel->final_ip, search_ip );
#endif /* ROUTE */
    return 0;
} /* - - - - - - - end of function ReadInputData - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  IsBackupIp
 *  Description:  Will modify content in conf
 * =====================================================================================
 */
    static int
IsBackupIp( const char *conf )
{
    char *buffer, *p;
    char *token;
    p = buffer = strdup( conf );
    while(( token = strsep( &buffer, " \t\n" )) != NULL )
        if( strcmp( token, "backup" ) == 0 )
            return 1;
    free( p );
    return 0;
} /* - - - - - - - end of function IsBackupIp - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  StringSubstitute
 *  Description:  string申请的大小不足的话，可能会有内存越界，慎用
 * =====================================================================================
 */
    static char*
StringSubstitute( char *string, char *old_part, char *new_part )
{
    char *p, *to_be_move;
    size_t length;
    size_t old_length, new_length;
    assert( string );
    assert( old_part );
    assert( new_part );
    old_length = strlen( old_part );
    new_length = strlen( new_part );
    p = strstr( string, old_part );
    if( p == NULL )
        return NULL;
    to_be_move = p + old_length;
    length = strlen( to_be_move );
    memmove( to_be_move + new_length - old_length, to_be_move, length + 1 );
    memcpy( p, new_part, new_length );
    return p + new_length;
} /* - - - - - - - end of function StringSubstitute - - - - - - - - - - - */
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  WriteToShortestPath
 *  Description:
 * =====================================================================================
 */
    static int
WriteToShortestPath( IpStatus *ip_status, uint16_t port )
{
    RoutePath path;
    assert( ip_status );
    path.ip_from = 0;
    path.ip_to = ip_status->final_ip;
    path.ip_to_port = port;
    path.state.used_time = ip_status->detect_used_time;
    Debug( 50, "Tta write route path. ip is %u:%u, used time is %lf", path.ip_to, path.ip_to_port, path.state.used_time );
    return ShortestPathWriteFileData( &path );
} /* - - - - - - - end of function WriteToShortestPath - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ModifyAndWriteToFile
 *  Description:  
 * =====================================================================================
 */
    static int
ModifyAndWriteToFile( FILE *fp, ChannelConf *conf, HashTable *channel )
{
    TtaChannel *search_channel;
    char ip[16];
    int ip_num, backup_ip_num;
    int is_modified = 0;
    assert( fp );
    assert( conf );
    assert( channel );
    if( conf->is_enable_modify != 0 )
    {
        conf->channel.key = TtaChannelKeyGen( conf->channel.name, conf->channel.port );
        search_channel = (TtaChannel*)HashSearch( channel, &conf->channel );
        Debug( 40, "Ready to modify channel: %s, port: %u", conf->channel.name, conf->channel.port );
        if( search_channel != NULL )
        {
            backup_ip_num = conf->ip_backup;
            ip_num = 1 - backup_ip_num;
            if( conf->ip_backup == -1 )
                ip_num = 0;
            else
            {
                if( search_channel->best_backup_ip != NULL )
                {
                    if( inet_ntop( AF_INET, &search_channel->best_backup_ip->ip_to, ip, 16 ) == NULL )
                        Debug( 1, "Error in data from framework!" );
                    else if( strncmp( conf->ip[backup_ip_num], ip, 16 ) == 0 )
                        Debug( 40, "No need to modify backup ip %s", ip );
                    else if( StringSubstitute( conf->point_backup_ip_in_buffer, conf->ip[backup_ip_num], ip ) == NULL )
                        Debug( 10, "Cannot substitute old backup ip %s by new ip %s", conf->ip[backup_ip_num], ip );
                    else
                    {
                        is_modified = 1;
                        Debug( 40, "Tta, channel %s, set backup ip to %s ( %lfs, %dp )",
                                conf->channel.name,
                                ip,
                                search_channel->best_backup_ip->detect_used_time,
                                search_channel->best_backup_ip->mark );
                    }
                }
            }
            if( search_channel->best_ip != NULL )
            {
                if( inet_ntop( AF_INET, &search_channel->best_ip->ip_to, ip, 16 ) == NULL )
                    Debug( 1, "Error in data from framework!" );
                else if( strncmp( conf->ip[ip_num], ip, 16 ) == 0 )
                    Debug( 40, "No need to modify ip %s", ip );
                else if( StringSubstitute( conf->point_ip_in_buffer, conf->ip[ip_num], ip ) == NULL )
                    Debug( 10, "Cannot substitute old ip %s by new ip %s", conf->ip[ip_num], ip );
                else
                {
                    WriteToShortestPath( search_channel->best_ip, htons( search_channel->port ) );
                    is_modified = 1;
                    Debug( 40, "Tta, channel %s, set ip to %s ( %lfs, %dp )",
                            conf->channel.name,
                            ip,
                            search_channel->best_ip->detect_used_time,
                            search_channel->best_ip->mark );
                }
            }
            else if( search_channel->best_backup_ip != NULL )
            {
                WriteToShortestPath( search_channel->best_backup_ip, htons( search_channel->port ) );
            }
        }
    }
    fprintf( fp, "%s", conf->buffer );
    return is_modified;
} /* - - - - - - - end of function ModifyAndWriteToFile - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  WriteOutputFile
 *  Description:  
 * =====================================================================================
 */
    static int
WriteOutputFile( const char *file_name_from, const char *file_name_to, HashTable *channel )
{
    typedef enum {
        kOUTPUT_FILE_WAIT,
        kOUTPUT_FILE_LISTEN,
        kOUTPUT_FILE_SERVER,
        kOUTPUT_FILE_SERVER_NAME,
        kOUTPUT_FILE_RESERVE
    } OutputPhase; /* - - - - - - end of enum OutputPhase - - - - - - */
    const int kLINE_LEGNTH = 1024;
    char line[kLINE_LEGNTH];
    FILE *file_from, *file_to;
    char *buffer_point;
    OutputPhase conf_phase;
    ChannelConf conf;
    int is_modified = 0;
    assert( file_name_from );
    assert( file_name_to );
    assert( channel );
    /* 初始化 */
    memset( &conf, 0, sizeof(ChannelConf) );
    conf.buffer = (char*) malloc( 1<<20 );
    if( conf.buffer == NULL )
    {
        Debug( 10, "Failed! %d! malloc().", __LINE__ );
        return 1;
    }
    memset( conf.buffer, 0, 1<<20 );
    file_from = fopen( file_name_from, "r" );
    if( file_from == NULL )
    {
        Debug( 10, "Cannot open file %s", file_name_from );
        free( conf.buffer );
        return 1;
    }
    file_to = fopen( file_name_to, "w" );
    if( file_from == NULL )
    {
        Debug( 10, "Cannot open file %s", file_name_to );
        fclose( file_from );
        free( conf.buffer );
        return 1;
    }
    /* 修改haproxy的配置文件 */
    conf_phase = kOUTPUT_FILE_WAIT;
    buffer_point = conf.buffer;
    while( fgets( line, kLINE_LEGNTH, file_from ) != NULL )
    {
        char *p, *token;
        char *buffer;
        char *buffer_tmp;
        int count = 0;
        Debug( 50, "%s", line );
        buffer = strdup( line );
        buffer_tmp = buffer_point;
        buffer_point += sprintf( buffer_point, "%s", buffer );
        //        strcat( conf.buffer, line );
        /* 忽略掉注释 */
        p = strchr( buffer, '#' );
        if( p != NULL )
            *p = '\0';
        p = buffer;
        while(( token = strsep( &p, " \t\n" )) != NULL )
        {
            if( *token == '\0' )
                continue;
            ++ count;
            switch( conf_phase )
            {
                case kOUTPUT_FILE_WAIT:
                    if( count == 1 )
                    {
                        if( strcmp( token, "listen" ) == 0 )
                        {
                            if( conf.channel.name != NULL )
                            {
                                is_modified |= ModifyAndWriteToFile( file_to, &conf, channel );
                                TtaChannelDestroy( &conf.channel );
                                *conf.buffer = '\0';
                                buffer_point = conf.buffer;
                            }
                            conf.is_enable_modify = 0;
                            conf.ip_count = 0;
                            conf.ip_backup = -1;
                            conf_phase = kOUTPUT_FILE_LISTEN;
                        }
                        else if( strcmp( token, "server" ) == 0 )
                        {
                            if( conf.channel.name ==NULL )
                            {
                                Debug( 10, "Get server without listen." );
                                goto PARSE_ERROR;
                            }
                            if( !IsBackupIp( p ) )
                                conf_phase = kOUTPUT_FILE_SERVER;
                        }
                        else if( strcmp( token, kENABLE_MODIFY_KEYWORD ) == 0 )
                        {
                            if( conf.channel.name ==NULL )
                            {
                                Debug( 10, "Get %s without listen.", kENABLE_MODIFY_KEYWORD );
                                goto PARSE_ERROR;
                            }
                            conf.is_enable_modify = 1;
                        }
                        else if( strcmp( token, "frontend" ) == 0 ||
                                 strcmp( token, "backend" ) == 0 )
                        {
                            Debug( 50, "Find keyword %s.", token );
                            if( conf.channel.name != NULL )
                            {
                                is_modified |= ModifyAndWriteToFile( file_to, &conf, channel );
                                TtaChannelDestroy( &conf.channel );
                                *conf.buffer = '\0';
                                buffer_point = conf.buffer;
                            }
                        }
                    }
                    else
                    {
                        p = NULL;
                        if( conf.channel.name == NULL )
                            goto PARSE_ERROR;
                    }
                    break;
                case kOUTPUT_FILE_LISTEN:
                    if( count == 2 )
                    {
                        conf.channel.name = strdup( token );
                        conf_phase = kOUTPUT_FILE_WAIT;
                    }
                    else
                    {
                        Debug( 10, "Get listen without channel name." );
                        goto PARSE_ERROR;
                    }
                    break;
                case kOUTPUT_FILE_SERVER:
                    if( count == 2 )
                        conf_phase = kOUTPUT_FILE_SERVER_NAME;
                    else
                    {
                        Debug( 10, "Get server without server name." );
                        goto PARSE_ERROR;
                    }
                    break;
                case kOUTPUT_FILE_SERVER_NAME:
                    if( count == 3 )
                    {
                        char *tmp;
                        tmp = strsep( &token, ":" );
                        strcpy( conf.ip[conf.ip_count], tmp );
                        tmp = strsep( &token, " \t" );
                        if( conf.channel.port == 0 )
                            conf.channel.port = 80;
                        if( tmp != NULL )
                            conf.channel.port = atoi( tmp );
                        conf.point_ip_in_buffer = buffer_tmp;
                        if( conf.ip_count == 1 && conf.ip_backup == -1 )
                        {
                            Debug( 10, "Get server with 2 ip." );
                            goto PARSE_ERROR;
                        }
                        ++ conf.ip_count;
                        if( conf.ip_count == 2 )
                        {
                            is_modified |= ModifyAndWriteToFile( file_to, &conf, channel );
                            TtaChannelDestroy( &conf.channel );
                            *conf.buffer = '\0';
                            buffer_point = conf.buffer;
                        }
                        p = NULL;
                        conf_phase = kOUTPUT_FILE_WAIT;
                    }
                    else
                    {
                        Debug( 10, "Get server without ip." );
                        goto PARSE_ERROR;
                    }
                    break;
                default:
                    Debug( 1, "Unknown Error!" );
                    break;
            }
        }
        free( buffer );
        continue;
PARSE_ERROR:
        fprintf( file_to, "%s", conf.buffer );
        *conf.buffer = '\0';
        buffer_point = conf.buffer;
        TtaChannelDestroy( &conf.channel );
        conf_phase = kOUTPUT_FILE_WAIT;
        free( buffer );
    }
    if( conf.channel.name != NULL )
        is_modified |= ModifyAndWriteToFile( file_to, &conf, channel );
    else
        fprintf( file_to, "%s", conf.buffer );
    fclose( file_from );
    fclose( file_to );
    TtaChannelDestroy( &conf.channel );
    free( conf.buffer );
    return is_modified;
} /* - - - - - - - end of function WriteOutputFile - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ModTtaInit
 *  Description:  
 * =====================================================================================
 */
    int
ModTtaInit( const void *data )
{
    const unsigned int kHASH_SIZE = 1024;
    FILE *input_file;
    HashConfigure channel_conf = {0};
    SetLinkDataReadCountMax( kTTA_LINK_DATA_READ_COUNT_MAX );
    channel_conf.name = "tta_all_channel";
    channel_conf.total = kHASH_SIZE;
    channel_conf.GetKey = TtaChannelGetKey;
    channel_conf.IsEqual = TtaChannelIsEqual;
    channel_conf.DoFree = TtaChannelDoFree;
    all_channel = HashCreate( &channel_conf );
    if( all_channel == NULL )
    {
        Debug( 10, "Cannot create hash table." );
        module_enable = 0;
        return 1;
    }
    /* 读取配置文件 */
    input_file = fopen( kINPUT_FILE, "r" );
    if( input_file == NULL )
    {
        Debug( 10, "Cannot read configure file in tta module." );
        module_enable = 0;
        HashDestroy( all_channel );
        free( all_channel );
        all_channel = NULL;
        return 0;
    }
    ReadInputFile( input_file, all_channel );
    fclose( input_file );
    /* 准备好各个频道的表格 */
    return 0;
} /* - - - - - - - end of function ModTtaInit - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ModTtaEnd
 *  Description:  
 * =====================================================================================
 */
    int
ModTtaEnd( const void *data )
{
    char call_shell[1024];
    char tmp_file_name[128];
    //sed haproxy.conf -e '/https/,/listen/s=443=hi=gp'
    if( module_enable == 0 )
        return 0;
    /* 筛选 */
    TtaChannelOptimize( all_channel );
    sprintf( tmp_file_name, "%s.tmp", kOUTPUT_FILE );
    if( WriteOutputFile( kOUTPUT_FILE, tmp_file_name, all_channel ) == 0 )
        Debug( 30, "Configure file of haproxy did not moified." );
    else
    {
        int ret;
        sprintf( call_shell, "mv %s %s", tmp_file_name, kOUTPUT_FILE );
        //        sprintf( call_shell, "cat %s", tmp_file_name );
        ret = system( call_shell );
        Debug( 30, "%s, returned %d.", call_shell, ret );
        //ret = system( "/sbin/service haproxy reload 1>/dev/null 2>&1" );
        ret = system( "/etc/init.d/haproxy reload 1>/dev/null 2>&1" );
        Debug( 30, "Service haproxy reloaded, returned %d.", ret );
    }
    HashDestroy( all_channel );
    free( all_channel );
    all_channel = NULL;
    return 0;
} /* - - - - - - - end of function ModTtaEnd - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ModTtaInputData
 *  Description:  
 * =====================================================================================
 */
    int
ModTtaInputData( const void *data )
{
    if( module_enable == 0 )
        return 0;
    return ReadInputData( data );
} /* - - - - - - - end of function ModTtaInputData - - - - - - - - - - - */
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
    Debug( 40, "Register TTA module!" );
    ret |= AddHookFunction( kHOOK_INIT, ModTtaInit );
    ret |= AddHookFunction( kHOOK_DATA_INPUT, ModTtaInputData );
    ret |= AddHookFunction( kHOOK_END, ModTtaEnd );
    return ret;
} /* - - - - - - - end of function RegisterModule - - - - - - - - - - - */
