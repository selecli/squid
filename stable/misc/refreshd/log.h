#ifndef __LOG_H__
#define __LOG_H__

#define cclog(level,fmt, args...) scs_log(level, __FILE__, __LINE__, fmt, ## args )
#define cc_access_log(level,fmt, args...) scs_log2(level, __FILE__, __LINE__, fmt, ## args )

void scs_log( int level,char* file, int line, char *fmt, ...);
void scs_log2( int level,char* file, int line, char *fmt, ...);

void close_log();
void scs_log_rotate();

//sig_log used to replace cclog to record the signal msg the refreshd has received to avoid the cclog into error loop status when ittouch off error signal itself
void sig_log(int sig);
#endif
