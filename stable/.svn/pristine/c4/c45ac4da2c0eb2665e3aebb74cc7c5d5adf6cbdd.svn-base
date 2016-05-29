#ifndef DETECT_MACROS_H
#define DETECT_MACROS_H

#ifndef NULL
//we think NULL is 0
#define NULL 0
#endif

#define BUF_SIZE_16   16
#define BUF_SIZE_32   32
#define BUF_SIZE_64   64
#define BUF_SIZE_128  128
#define BUF_SIZE_256  256
#define BUF_SIZE_512  512
#define BUF_SIZE_1024 1024
#define BUF_SIZE_2048 2048
#define BUF_SIZE_4096 4096
#define BUF_SIZE_8192 8192
#define MAX_ALLOC_SIZE 1024000
#define HTTP_VERSION_MAJOR 1             //http version major
#define HTTP_VERSION_MINOR 1             //http version minor
#define HTTPS_PORT    443                //https default port
#define GROW_STEP     300                //object grow step
#define CLINE_IP_MAX  64                 //max ip in one configuration line
#define EPOLL_TIMEOUT 15                 //milliseconds
#define MAX_EVENTS    344800             //max events number for epoll
#define DETECT_TIMEOUT 3.0               //detect default timeout: second
#define DETECT_KILLTIME 300              //if run more than this, kill current program instance
#define SAVETIME_MIN 60                  //save 1 minutes link.data at least
#define SAVETIME_DEFAULT 86400           //one day seconds(24 hours)
#define DELAY_TIME_RAND 20               //delay start interval time
#define DELAY_TIME_BASE 10               //delay start base time
#define LOG_ROTATE_NUMBER 5              //rotate log file number
#define LOG_ROTATE_SIZE   1024000        //log file rotate size: byte
#define DOUBLE_NUMBER_MIN 0.0000001      //min double number
#define DETECT_TIMES_DFT 1               //min detect times for one ip
#define DETECT_TIMES_MAX 10              //max detect times for one ip
#define DETECT_PORT_MIN 0                //min port number
#define DETECT_PORT_DFT 80               //default port number
#define DETECT_PORT_MAX 65535            //max port number
#define DETECT_OBJLEN_MIN 512            //min request content length
#define DETECT_OBJLEN_MAX 4096000        //max request content length
#define DETECT_RLIMIT 344800             //max open files fd
#define TRAVEL_NUMBER 35                 //travel number
#define MAX_FP_NUM 20                    //max file handler
#define MAX_HASH_NUM 500000              //hashtable size
#define MODE_FC 0x01                     //detect for FC
#define MODE_TTA 0x02                    //detect for TTA
#define MODE_NOFC ~(0x01)                //no detect for FC
#define POST_DFT_TYPE "text/plain"       //post file default type
#define MAX_POST_SIZE 4096000            //max post file size
//DNS service script path
#define DNSSERVICE_PATH                  "/etc/init.d/named"
//configuration files
#define CONFIG_CONTROL                   "/usr/local/squid/etc/detectorig.conf"
#define CONFIG_DOMAIN_FC                 "/usr/local/squid/etc/domain.conf"
#define CONFIG_DOMAIN_TTA                "/usr/local/haproxy/etc/tta_domain.conf"
#define CONFIG_ORIGIN_DOMAIN_IP          "/usr/local/squid/etc/origin_domain_ip"
//log file
#define DETECT_LOG_DIR                   "/var/log/chinacache"
#define DETECT_LOG_FILE                  "/var/log/chinacache/detect.log"
//pid file
#define DETECT_PID_DIR                   "/var/run/detectorig"
#define DETECT_PID_FILE                  "/var/run/detectorig/detectorig.pid"
//detect times file
#define DETECT_TIME_FILE                 "/usr/local/squid/etc/detect_time.tmp"
//output file
#define LINKDATA_DIR                     "/var/log/chinacache/linkdata"
#define ANYHOST_NEW                      "/var/named/chroot/var/named/anyhost"
#define ANYHOST_TMP                      "/var/named/chroot/var/named/anyhost.tmp"
#define ANYHOST_DUMP                     "/var/log/chinacache/detectorigin.log"
#define CUSTOM_CONF_DIR                  "/usr/local/squid/etc/custom_domain"
//version
#define DETECTORIG_VERSION               "detectorig V3.3"

#define RCMS_LOG                         "/var/log/chinacache/detectorigConfigError.log"

#define xmemzero(s, n) (void)memset((s), 0, (n))
#define dlog(logtype, sockfd, fmt, args...) detectlog((logtype), (sockfd), __FILE__, __LINE__, (fmt), ##args)

#endif
