#ifndef __LOG_H__
#define __LOG_H__

#define ANALYSE
#ifdef ANALYSE
#include "dbg.h"
#define cclog(level,fmt,args...) Debug( 60, "Old log %d: " fmt, level, ## args )
#define open_log(x)
#define close_log(x)
#else /* not ANALYSE */
#define cclog(level,fmt, args...) scs_log(level, __FILE__, __LINE__, fmt, ## args )
void scs_log( int level,char* file, int line, char *fmt, ...);
void open_log();
void close_log();
void scs_log_rotate();
#endif /* not ANALYSE */

#endif
