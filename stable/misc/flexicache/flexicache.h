#ifndef __FLEXICACHE_H__
#define __FLEXICACHE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <va_list.h>
#include <stdarg.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/klog.h>
#include <linux/unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <assert.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <sys/epoll.h>

#include "define.h"
#include "enums.h"
#include "structs.h"
#include "globals.h"
#include "protos.h"
#include "config.h"
#include "log.h"
#include "cache_squid.h"
#include "lb_haproxy.h"
#include "lb_lscs.h"
#include "lb_manager.h"
#include "cache_manager.h"
#include "watch_manager.h"
#include "watch_squid.h"

#endif
