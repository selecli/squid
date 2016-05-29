#include "detect.h"

const char *logstr[LOG_LAST] = {"COMMON", "WARNING", "ERROR", "FATAL"};

static void logFileRotate(void)
{
	int index;
	char to[BUF_SIZE_256];
	char from[BUF_SIZE_256];

	xmemzero(to, BUF_SIZE_256);
	xmemzero(from, BUF_SIZE_256);
	index = LOG_ROTATE_NUMBER;
	do {
		snprintf(from, BUF_SIZE_256, "%s.%d", DETECT_LOG_FILE, index - 1);
		snprintf(to, BUF_SIZE_256, "%s.%d", DETECT_LOG_FILE, index);
		rename(from, to);
	} while(--index);
	closeLogFile();
	rename(DETECT_LOG_FILE, from);
	openLogFile();
}

void openLogFile(void)
{
	struct stat st; 

	if (stat(DETECT_LOG_DIR, &st) < 0)
	{
		if (mkdir(DETECT_LOG_DIR, 0755) < 0)
		{
			fprintf(stderr,
					"mkdir() failed, DIR=[%s], ERROR=[%s]",
					DETECT_LOG_DIR, xerror());
			abort();
		}
	}
	if(NULL == detect.other.logfp) 
	{
		detect.other.logfp = xfopen(DETECT_LOG_FILE, "a+");
		if (NULL == detect.other.logfp)
			abort();
	}
	writeDetectIdentify(STATE_START);
	xmemzero(&st, sizeof(st));
	if (stat(DETECT_LOG_FILE, &st) < 0)
	{
		dlog(WARNING, NOFD,
				"stat() failed, FILE=[%s], ERROR=[%s]",
				DETECT_LOG_FILE, xerror());
		abort();
	}
	if(st.st_size > LOG_ROTATE_SIZE)   
	{
		logFileRotate();
		dlog(COMMON, NOFD, "log file rotate, SIZE=[%#x]", st.st_size);
	}
}

void closeLogFile(void)
{
	if(NULL != detect.other.logfp) 
	{
		fclose(detect.other.logfp);
		detect.other.logfp = NULL;
	}
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
	{
		fprintf(detect.other.logfp, "%s| ERROR: no this log type", time_buf);
	}
	else if (COMMON == logtype)
	{
		snprintf(logbuf, BUF_SIZE_4096, "%s| %s", time_buf, log_info);
	}
	else
	{
		snprintf(logbuf, BUF_SIZE_4096,
				"%s| %s: FILE=[%s:%d], %s",
				time_buf, logstr[logtype], filename, nline, log_info);
	}
	if (detect.opts.F.dblevel > logtype)
		puts(logbuf);
	fprintf(detect.other.logfp, "%s", logbuf);
	fflush(detect.other.logfp);
}

static int addDetailLogInfo(const int sockfd, char *log_info)
{
	char info[BUF_SIZE_4096];
	detect_event_t *event;

	event = detect.data.index.idx[sockfd].event;
	xmemzero(info, BUF_SIZE_4096);
	snprintf(info, BUF_SIZE_4096, 
			", HOSTNAME=[%s], IP=[%s]",
			event->dip->rfc->hostname, event->dip->ip);
	if ('\n' == log_info[strlen(log_info) - 1])
		log_info[strlen(log_info) - 1] = '\0';
	strcat(log_info, info);

	return SUCC;
}

/*type: 0:common; 1:warning; 2:error; 3:fatal */
void detectlog(const int logtype, const int sockfd, const char *filename, const int nline, char *format, ...)
{
	va_list ap;
	char buf[BUF_SIZE_4096];
	char log_info[BUF_SIZE_4096];

	va_start(ap, format);
	xmemzero(buf, BUF_SIZE_4096);
	xmemzero(log_info, BUF_SIZE_4096);

	//parse the format string, get correct substring
	while (*format) 
	{
		if ('%' != *format) 
		{
			snprintf(buf, BUF_SIZE_4096, "%c", *format);
			strcat(log_info, buf);
			++format;
			continue;
		}
		switch (*(++format)) 
		{
			case 'c': //char
				snprintf(buf, BUF_SIZE_4096, "%c", (char)va_arg(ap, int));
				strcat(log_info, buf);
				break;
			case 'd': //integer
				snprintf(buf, BUF_SIZE_4096, "%d", va_arg(ap, int));
				strcat(log_info, buf);
				break;
			case 'l': //char
				switch (*(++format)) 
				{
					case 'd': //long integer
						snprintf(buf, BUF_SIZE_4096, "%ld", va_arg(ap, long));
						strcat(log_info, buf);
						break;
					case 'f': //long integer
						snprintf(buf, BUF_SIZE_4096, "%.3lf", va_arg(ap, double));
						strcat(log_info, buf);
						break;
					default: //default
						snprintf(buf, BUF_SIZE_4096, "no this data type format[%c]", *format);
						break;
				}
				break;
			case 's': //string
				snprintf(buf, BUF_SIZE_4096, "%s", va_arg(ap, char *));
				strcat(log_info, buf);
				break;
			default: //default
				snprintf(buf, BUF_SIZE_4096, "no this data type format[%c]", *format);
				break;
		}
		++format;
	}
	va_end(ap);
	if (sockfd != NOFD)
		addDetailLogInfo(sockfd, log_info);
	//here check whether the string have '\n' at the end, if not, add it
	if ('\0' == *log_info || '\n' != log_info[strlen(log_info) - 1])
		strcat(log_info, "\n");
	writeByLogType(logtype, filename, nline, log_info);
}

