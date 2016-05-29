/*
 * =====================================================================================
 *
 *       Filename:  framework.h
 *
 *    Description:  
 *
 *        Created:  03/15/2012 02:38:57 PM
 *       Compiler:  gcc
 *         Author:  Xu Jian (XJ), xujian@chinacache.com
 *        Company:  ChinaCache
 *
 * =====================================================================================
 */
#ifndef _FRAMEWORK_H_882022A0_F52F_40BB_92A5_F0F386E09642_
#define _FRAMEWORK_H_882022A0_F52F_40BB_92A5_F0F386E09642_ 
#include <stdint.h>
#include <unistd.h>
#include "dbg.h"
#ifndef FRAMEWORK
#define FRAMEWORK
#endif /* not FRAMEWORK */
#ifndef INFINITY
#define INFINITY (1e308)                       /* for old gcc */
#endif /* not INFINITY */
typedef enum {
    kHOOK_INIT,
    kHOOK_SET_LINK_DATA_COUNT,
    kHOOK_DATA_INPUT,
    kHOOK_END,
    kHOOK_RESERVE
} HookType; /* - - - - - - end of enum HookType - - - - - - */
typedef enum {
    kPORT_SOMETHING=0,
    kPORT_WORK,
    kPORT_DOWN,
    kPORT_RESERVE
} PortStatus; /* - - - - - - end of enum PortStatus - - - - - - */
typedef enum
{
    kDETECT_TYPE_HTTP,
    kDETECT_TYPE_TCP,
    kDETECT_TYPE_HTTPS,
    kDETECT_TYPE_RESERVE
} DetectType; /* - - - - - - end of enum DetectType - - - - - - */
typedef enum {
    kIP_TYPE_IP,
    kIP_TYPE_BACKUP_IP,
    kIP_TYPE_DNS_IP,
    kIP_TYPE_RESERVE
} IpType; /* - - - - - - end of enum IpType - - - - - - */
typedef struct _DataInfo
{
    int file_order;                                /* used when read multi files. it shows order number, which begin from 0 */
    char *hostname;
    uint32_t ip_from;                           /* net byte order */
    uint32_t ip_to;                             /* net byte order */
    IpType ip_type;
    double connect_time;
    double first_byte_time;
    double download_time;
    double detect_used_time;
    uint16_t detect_times;
    uint16_t detect_success_times;
    double packet_loss_rate;
    uint16_t port;                              /* host byte order */
    uint16_t http_status;
    PortStatus port_status;
    DetectType detect_type;
    /* 未分类，临时 */
    //char *ip_type;
    //char *detect_times;
    //char *detect_success_times;
} DataInfo; /* - - - - - - end of struct DataInfo - - - - - - */
typedef int (*HookFunction)( const void *data );
unsigned int GetLinkDataReadCountMax( void );
int SetLinkDataReadCountMax( unsigned int count );
int LoadModule( void );
int ReleaseModule( void );
int AddHookFunction( HookType type, HookFunction Function );
int CallHookFunction( HookType type, const void *data );
int ReleaseCpu( void );
#endif /* _FRAMEWORK_H_882022A0_F52F_40BB_92A5_F0F386E09642_ */
