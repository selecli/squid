#ifndef __DEFINES_H__
#define __DEFINES_H__

static const char *const w_space = " \t\r\n";
#define PRINTF_FORMAT_ARG1 __attribute__ ((format (printf, 1, 2)))

#define CONF_PATH           "/usr/local/squid/etc/flexicache.conf"

#define CACHE_PATH          "/usr/local/squid/sbin/squid"
#define CACHE_CONF_PATH     "/usr/local/squid/etc/squid.conf"

#define LB_PATH             "/usr/local/squid/bin/lscs/sbin/nginx"
#define LB_CONF_PATH        "/usr/local/squid/bin/lscs/conf/nginx.conf"

#define PID_FILE            "/var/run/flexicache.pid"
#define SQUID_PID_FILE      "/var/run/squid.pid"
#define LOG_FILE            "/data/proclog/log/flexicache/flexicache.log"

#define debug(level,fmt, args...) scs_log(level, __FILE__, __LINE__, fmt, ## args )

#define LOG_ROTATE_NUMBER 10

#define PORT_NUM   16

#define MAX_CACHE_PROCS 1024
typedef int STARTER();
typedef int RELOADER();
typedef int TERMINATOR();
typedef int CHECK_RESTARTER(pid_t pid);

#define CACHE_WATCH_INTERVAL_DEFAULT    60
#define CACHE_WATCH_TIMEOUT_DEFAULT     1000

#define CACHE_WATCH_FAIL_RETRY          3
#define CACHE_WATCH_EVENTS              1024
#define CACHE_WATCH_SEND_MSG            "watch_squid_request"
#define CACHE_WATCH_SEND_MSG_LENGTH      sizeof(CACHE_WATCH_SEND_MSG)
#define CACHE_WATCH_RECV_MSG            "watch_squid_response"
#define CACHE_WATCH_RECV_MSG_LENGTH      sizeof(CACHE_WATCH_RECV_MSG)

#define VERSION "7.0.6030"
#endif
