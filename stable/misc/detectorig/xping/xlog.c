#include "xping.h"

const char *logstr[LOG_LAST] = {"COMMON", "WARNING", "ERROR", "FATAL"};

static void logFileRotate(void);

int openLogFile(void)
{
	struct stat st; 

	if (stat(PING_LOG_DIR, &st) < 0)
	{
		if (mkdir(PING_LOG_DIR, 0755) < 0)
		{
			fprintf(stderr, "mkdir() failed, DIR=[%s], ERROR=[%s]", PING_LOG_DIR, strerror(errno));
			return FAILED;
		}
	}
	if(NULL == xping.logfp) 
	{
		xping.logfp = fopen(PING_LOG_FILE, "a+");
		if (NULL == xping.logfp)
			return FAILED;
	}
	memset(&st, 0, sizeof(st));
	if (stat(PING_LOG_FILE, &st) < 0)
	{
		xlog(WARNING, "stat() failed, FILE=[%s], ERROR=[%s]", PING_LOG_FILE, strerror(errno));
		return FAILED;
	}
	if(st.st_size > LOG_ROTATE_SIZE)   
	{
		logFileRotate();
		xlog(COMMON, "log file rotate, SIZE=[%#x]", st.st_size);
	}

	return SUCC;
}

void closeLogFile(void)
{
	if(NULL != xping.logfp) 
	{
		fclose(xping.logfp);
		xping.logfp = NULL;
	}
}

static void logFileRotate(void)
{
	int index;
	char from[BUF_SIZE_256];
	char to[BUF_SIZE_256];

	memset(to, 0, BUF_SIZE_256);
	memset(from, 0, BUF_SIZE_256);
	index = LOG_ROTATE_NUMBER;
	do {
		snprintf(from, BUF_SIZE_256, "%s.%d", PING_LOG_FILE, index - 1);
		snprintf(to, BUF_SIZE_256, "%s.%d", PING_LOG_FILE, index);
		rename(from, to);
	} while(--index);
	closeLogFile();
	rename(PING_LOG_FILE, from);
	openLogFile();
}

static void writeByLogType(const int logtype, const char *filename, const int nline, const char *log_info)
{
	time_t cur_time;
	struct tm *local_time;
	char time_buf[BUF_SIZE_256];
	char logbuf[BUF_SIZE_4096];

	cur_time = time(NULL);
	local_time = localtime(&cur_time);
	strftime(time_buf, BUF_SIZE_256, "%Y/%m/%d %H:%M:%S", local_time);
	if (logtype < 0 && logtype >= LOG_LAST)
		fprintf(xping.logfp, "%s| ERROR: no this log type", time_buf);
	else if (COMMON == logtype)
		snprintf(logbuf, BUF_SIZE_4096, "%s| %s", time_buf, log_info);
	else
		snprintf(logbuf, BUF_SIZE_4096, "%s| %s: FILE=[%s:%d], %s", time_buf, logstr[logtype], filename, nline, log_info);
	fprintf(xping.logfp, "%s", logbuf);
	fflush(xping.logfp);
}

/*type: 0:common; 1:warning; 2:error; 3:fatal */
void pingLog(const int logtype, const char *filename, const int nline, char *format, ...)
{
	va_list ap;
	char buf[BUF_SIZE_1024];
	char log_info[BUF_SIZE_1024];

	va_start(ap, format);
	memset(buf, 0, BUF_SIZE_1024);
	memset(log_info, 0, BUF_SIZE_1024);

	//parse the format string, get correct substring
	while (*format) 
	{
		if ('%' != *format) 
		{
			snprintf(buf, BUF_SIZE_1024, "%c", *format);
			strcat(log_info, buf);
			++format;
			continue;
		}
		switch (*(++format)) 
		{
			case 'c': //char
				snprintf(buf, BUF_SIZE_1024, "%c", (char)va_arg(ap, int));
				strcat(log_info, buf);
				break;
			case 'd': //integer
				snprintf(buf, BUF_SIZE_1024, "%d", va_arg(ap, int));
				strcat(log_info, buf);
				break;
			case 'l': //char
				switch (*(++format)) 
				{
					case 'd': //long integer
						snprintf(buf, BUF_SIZE_1024, "%ld", va_arg(ap, long));
						strcat(log_info, buf);
						break;
					case 'f': //long integer
						snprintf(buf, BUF_SIZE_1024, "%.3lf", va_arg(ap, double));
						strcat(log_info, buf);
						break;
					default: //default
						snprintf(buf, BUF_SIZE_1024, "no this data type format[%c]", *format);
						break;
				}
				break;
			case 's': //string
				snprintf(buf, BUF_SIZE_1024, "%s", va_arg(ap, char *));
				strcat(log_info, buf);
				break;
			default: //default
				snprintf(buf, BUF_SIZE_1024, "no this data type format[%c]", *format);
				break;
		}
		++format;
	}
	va_end(ap);

	//here check whether the string have '\n' at the end, if not, add it
	if ('\0' == *log_info || '\n' != log_info[strlen(log_info) - 1])
		strcat(log_info, "\n");

	writeByLogType(logtype, filename, nline, log_info);
}

