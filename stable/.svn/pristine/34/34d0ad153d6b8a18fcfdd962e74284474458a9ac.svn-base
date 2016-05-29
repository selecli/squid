#ifndef __LOG_H__
#define __LOG_H__

#define cclog(level,fmt, args...) scs_log(level, __FILE__, __LINE__, fmt, ## args )

void scs_log( int level,char* file, int line, char *fmt, ...);
void close_log();
void scs_log_rotate();
#endif
