#include "h264_streamd.h"
#include "log.h"

#include <stdarg.h>
static FILE* debug_log = NULL;
#define	DEBUG_LOG_FILE		"/data/proclog/log/squid/moov_generator_for_pplive.log"
#define LOG_ROTATE_NUMBER 3

#ifdef	__THREAD_SAFE__
static pthread_mutex_t l_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

static uint64_t logfile_size = 0;

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
		struct stat buf;
		stat(DEBUG_LOG_FILE,&buf);
        	logfile_size = buf.st_size;
	}
}


void close_log()
{
	if( debug_log ) {
		fclose(debug_log);
		debug_log = NULL;
	}

}

void r_log(int level,char* file, int line, char *Format, ...)
{
	if(level != 3){
		return;
	}

	if(level > g_cfg.log_level){
		return;
	}

#ifdef	__THREAD_SAFE__
	pthread_mutex_lock(&l_mutex);
#endif
	va_list arg;
	time_t	local;
	struct	tm *t;
	int	len;
	int	sp = 0;

	static char buff[1024];
	static char buff2[1024];

	memset(buff, 0, 1024);
	memset(buff2, 0, 1024);

	time(&local);
	t = (struct tm*)localtime(&local);
	sp += sprintf(buff+sp, "[%4d-%02d-%02d %02d:%02d:%02d][%s:%d]", t->tm_year+1900, t->tm_mon+1,
			t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, file, line);


	snprintf(buff+sp, 1024-sp, ":%s",Format);

	va_start(arg, Format);
	len = vsnprintf(buff2, 1024-1, buff, arg);
	va_end (arg);

	len += sprintf(buff2 + len ,"\n");


	if( (debug_log == NULL) )
		open_log();

	assert(debug_log);

	fwrite(buff2, 1, len, debug_log);
	fflush(debug_log);
	
	logfile_size +=len;
	//printf("log file size %lld \n",logfile_size);

	if(logfile_size > 100000000){
		log_rotate();
		logfile_size = 0;
	}

#ifdef	__THREAD_SAFE__
	pthread_mutex_unlock(&l_mutex);
#endif
		

}


void log_rotate()
{
	static char from[256];
	static char to[256];

	memset(from,0,256);
	memset(to,0,256);

	int i = LOG_ROTATE_NUMBER;
	while(i){
		snprintf(from, 256, "%s.%d", DEBUG_LOG_FILE, i - 1);
		snprintf(to, 256, "%s.%d", DEBUG_LOG_FILE, i);
		rename(from, to);
		i--;
	}

#ifdef	__THREAD_SAFE__
	//pthread_mutex_lock(&l_mutex);
#endif
	close_log();
	rename(DEBUG_LOG_FILE, from);
	open_log();

#ifdef	__THREAD_SAFE__
	//pthread_mutex_unlock(&l_mutex);
#endif
}
