#ifndef LOG_H
#define LOG_H
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#define LOG_ERR	0
#define LOG_WARN	1
#define LOG_DEBUG	2
#define LOG_FATAL	3

#define do_log(level,fmt, args...) r_log(level, __FILE__, __LINE__, fmt, ## args )
#define debug(fmt,args...) r_log(LOG_DEBUG, __FILE__, __LINE__, fmt, ## args )
#define warn(fmt,args...) r_log(LOG_WARN, __FILE__, __LINE__, fmt, ## args )
#define errlog(fmt,args...) r_log(LOG_ERR, __FILE__, __LINE__, fmt, ## args )
#define fatallog(fmt,args...) r_log(LOG_FATAL, __FILE__, __LINE__, fmt, ## args )

void close_log();
void r_log(int level,char* file, int line, char *Format, ...);
void log_rotate();

#endif
