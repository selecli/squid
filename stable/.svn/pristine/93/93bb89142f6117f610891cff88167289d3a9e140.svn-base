#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include "refresh_header.h"

FILE* debug_log = NULL;
FILE* access_log = NULL;

#define ACCESS_LOG

#define	DEBUG_LOG_FILE		"/data/proclog/log/refreshd/refreshd.log"

#ifdef ACCESS_LOG
#define ACCESS_LOG_FILE		"/data/proclog/log/refreshd/refreshd_access.log"
#endif

#ifdef  __THREAD_SAFE__
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
#ifdef ACCESS_LOG
static pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
#endif
#endif
#define LOG_ROTATE_NUMBER 3

static void open_log()
{
	struct stat sb;
	char log_path[256];
	strcpy(log_path,DEBUG_LOG_FILE);
	char *p = log_path + strlen(log_path);
	while(*p != '/') p--;
	*p = '\0';

	if (stat(log_path, &sb) != 0){
		mkdir(log_path, S_IRWXU);
	}

	if( debug_log == NULL ) {
		debug_log = fopen(DEBUG_LOG_FILE, "a+");
	}

#ifdef ACCESS_LOG
	if( access_log == NULL) {
		access_log = fopen(ACCESS_LOG_FILE, "a+");
	}
#endif
}


void close_log()
{
	if( debug_log ) {
		fclose(debug_log);
		debug_log = NULL;
	}

#ifdef ACCESS_LOG
	if(access_log) {
		fclose(access_log);
		access_log = NULL;
	}
#endif

}

void sig_log(int sig)
{
	char buffer[128];
	int len = 0;
	memset(buffer, 0, sizeof(buffer));

	if((len = sprintf(buffer, "recv signal: %d\n", sig)) <= 0)
		return;

	if( debug_log == NULL ) {
                debug_log = fopen(DEBUG_LOG_FILE, "a+");
        }

	fwrite(buffer, 1, len, debug_log);
        fflush(debug_log);
	
	return;
}

void scs_log(int level,char* file, int line, char *Format, ...)
{
	if(level > config.log_level){
		return;
	}

#ifdef	__THREAD_SAFE__
	pthread_mutex_lock(&mutex);
#endif
	va_list arg;
	time_t	local;
	struct	tm *t;
	int	len;
	int	sp = 0;

	static char buff[10240];
	static char buff2[10240];

	memset(buff, 0, 10240);
	memset(buff2, 0, 10240);

	time(&local);
	t = (struct tm*)localtime(&local);
	sp += sprintf(buff+sp, "[%4d-%02d-%02d %02d:%02d:%02d][%s:%d]", t->tm_year+1900, t->tm_mon+1,
			t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, file, line);


	snprintf(buff+sp, 10240-sp-1, ":%s",Format);

	va_start(arg, Format);
	len = vsnprintf(buff2, 10240-2, buff, arg);
	va_end (arg);

	len += sprintf(buff2 + len ,"\n");


	if( (debug_log == NULL) )
		open_log();

	assert(debug_log);

	fwrite(buff2, 1, len, debug_log);
	fflush(debug_log);

#ifdef	__THREAD_SAFE__
	pthread_mutex_unlock(&mutex);
#endif
}

#ifdef ACCESS_LOG
void scs_log2(int level,char* file, int line, char *Format, ...)
{
/*
	if(level > config.log_level){
//		return;
		0;
	}
*/

#ifdef	__THREAD_SAFE__
	pthread_mutex_lock(&mutex2);
#endif
	va_list arg;
	time_t	local;
	struct	tm *t;
	int	len;
	int	sp = 0;

	static char buff[10240];
	static char buff2[10240];

	memset(buff, 0, 10240);
	memset(buff2, 0, 10240);

	time(&local);
	t = (struct tm*)localtime(&local);
	sp += sprintf(buff+sp, "[%4d-%02d-%02d %02d:%02d:%02d][%s:%d]", t->tm_year+1900, t->tm_mon+1,
			t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, file, line);


	snprintf(buff+sp, 10240-sp-1, ":%s",Format);

	va_start(arg, Format);
	len = vsnprintf(buff2, 10240-2, buff, arg);
	va_end (arg);

	len += sprintf(buff2 + len ,"\n");


	if( (access_log == NULL) )
		open_log();

	assert(access_log);

	fwrite(buff2, 1, len, access_log);
	fflush(access_log);

#ifdef	__THREAD_SAFE__
	pthread_mutex_unlock(&mutex2);
#endif
}
#endif


void scs_log_rotate()
{
	static char from[256];
	static char to[256];

	memset(from,0,256);
	memset(to,0,256);

#ifdef ACCESS_LOG
	static char from2[256];
        static char to2[256];

        memset(from2,0,256);
        memset(to2,0,256);
#endif

	int i = LOG_ROTATE_NUMBER;
	while(i){
		snprintf(from, 256, "%s.%d", DEBUG_LOG_FILE, i - 1);
		snprintf(to, 256, "%s.%d", DEBUG_LOG_FILE, i);
		rename(from, to);
#ifdef ACCESS_LOG
		snprintf(from2, 256, "%s.%d", ACCESS_LOG_FILE, i - 1);
                snprintf(to2, 256, "%s.%d", ACCESS_LOG_FILE, i);
                rename(from2, to2);
#endif
		i--;
	}

#ifdef	__THREAD_SAFE__
	pthread_mutex_lock(&mutex);
#ifdef ACCESS_LOG
	pthread_mutex_lock(&mutex2);
#endif
#endif
	close_log();
	rename(DEBUG_LOG_FILE, from);
#ifdef ACCESS_LOG
	rename(ACCESS_LOG_FILE, from2);
#endif
	open_log();

#ifdef	__THREAD_SAFE__
	pthread_mutex_unlock(&mutex);
#ifdef ACCESS_LOG
	pthread_mutex_unlock(&mutex2);
#endif
#endif
}
