#include "flexicache.h"

char *xstrdup(const char *src)
{
	size_t l = 0;
	char *dst = NULL;

	assert(src != NULL);
	l = strlen(src);
	dst = malloc(l + 1);
	memcpy(dst, src, l);
	dst[l] = '\0';

	return dst;
}

int xisdir(const char *path)
{
	struct stat st;

	assert(path != NULL);
	if (stat(path, &st) < 0)
	{
		return 0;
	}

	return S_ISDIR(st.st_mode);
}

void xchmod(const char *path, mode_t mode)
{
	struct stat st;

	assert(path != NULL);
	if (stat(path, &st) < 0)
	{
		/* Change file permissions directly */
		chmod(path, mode);
		return;
	}
	if (st.st_mode ^ mode)
	{
		chmod(path, mode);
	}
}

static int xmkdir(const char *path, mode_t mode)
{
	int s = 0;
	char *p = NULL;
	char *dir = NULL;
	mode_t old_mask = 0;

	assert(path != NULL);
	dir = xstrdup(path);
	/* parse the directory */
	p = strrchr(dir, '/');
	*p = '\0';
	old_mask = umask(022);
	if (mkdir(dir, mode) < 0)
	{
		if (EEXIST == errno && xisdir(dir))
		{
			/* EEXIST:
			 * Path already exists, but not necessarily as a directory.
			 * So, calling stat() to get the file status, then S_ISDIR(st_mode).
			 * PS: S_ISDIR(st_mode) is a POSIX macro.
			 * Here, xisdir() implements the function.
			 */
			xchmod(dir, mode);
			s = 0;
		}
		else
		{
			s = -1;
			fprintf(stderr, "xmkdir: mkdir(%s) error: %s\n", dir, strerror(errno));
		}
	}
	else
	{
		s = 0;
	}
	umask(old_mask);
	free(dir);

	return s;
}

void logInit(void)
{
	g_debug_log = fopen(LOG_FILE, "a+");
	if (NULL != g_debug_log)
	{
		return;
	}
	if (ENOENT != errno && ENOTDIR != errno)
	{   
		goto OPEN_FAILED;
	}   
	/* If the directory is not exist, create it. */
	if (xmkdir(LOG_FILE, 0755) < 0)
	{   
		goto OPEN_FAILED;
	}   
	g_debug_log = fopen(LOG_FILE, "a+");
	if (!g_debug_log)
	{
		goto OPEN_FAILED;
	}
	return;

OPEN_FAILED:
	fprintf(stderr, "WARNING: Cannot write log file: %s\n", LOG_FILE);
	perror(LOG_FILE);
	fprintf(stderr, "         messages will be sent to 'stderr'.\n");
	fflush(stderr);
	g_debug_log = stderr; 
	return;
}

void close_log()
{
	if( g_debug_log ) {
		fclose(g_debug_log);
		g_debug_log = NULL;
	}

}

void scs_log(int level,char* file, int line, char *Format, ...)
{
	if(level > g_config.log_level){
		return;
	}

	va_list arg;
	time_t  local;
	struct  tm *t;
	int     len;
	int     sp = 0;

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

	//len += sprintf(buff2 + len ,"\n");


	if( (g_debug_log == NULL) )
		logInit();

	assert(g_debug_log);

	fwrite(buff2, 1, len, g_debug_log);
	fflush(g_debug_log);
}

void log_rotate()
{
	static char from[256];
	static char to[256];

	memset(from,0,256);
	memset(to,0,256);

	int i = LOG_ROTATE_NUMBER;
	while(i){
		snprintf(from, 256, "%s.%d", LOG_FILE, i - 1); 
		snprintf(to, 256, "%s.%d", LOG_FILE, i);
		rename(from, to);
		i--;    
	}       

	close_log();
	rename(LOG_FILE, from);
	logInit();
}


