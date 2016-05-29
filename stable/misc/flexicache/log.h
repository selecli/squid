#ifndef __LOG_H__
#define __LOG_H__

void logInit();

//void debug();

void scs_log(int level,char* file, int line, char *Format, ...);
void close_log();
void log_rotate();
#endif
