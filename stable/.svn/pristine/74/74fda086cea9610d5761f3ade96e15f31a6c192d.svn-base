#include "flexicache.h"

static void fatalvf(const char *fmt, va_list args);

/* printf-style interface for fatal */
void fatalf(const char *fmt,...)
{
	va_list args;
	va_start(args, fmt);
	fatalvf(fmt, args);
	va_end(args);
}

	/* used by fatalf */
static void fatalvf(const char *fmt, va_list args)
{
	static char fatal_str[BUFSIZ];
	vsnprintf(fatal_str, sizeof(fatal_str), fmt, args);
	fatal(fatal_str);
}


static void fatal_common(const char *message)
{
	fprintf(g_debug_log, "FATAL: %s\n", message);
	fprintf(g_debug_log, "flexicache (Version %s): Terminated abnormally.\n",
			g_version_string);
	fflush(g_debug_log);
}


/* fatal */
void fatal(const char *message)
{
	fatal_common(message);
	abort();
}

pid_t readPidFile(void)
{
	FILE *pid_fp = NULL;
	pid_t pid = -1;
	int i;

	pid_fp = fopen(PID_FILE, "r");
	if (pid_fp != NULL)
	{
		pid = 0;
		if (fscanf(pid_fp, "%d", &i) == 1)
			pid = (pid_t) i;
		fclose(pid_fp);
	}
	else
	{
		if (errno != ENOENT)
		{
			fprintf(stderr, "flexicache: ERROR: Could not read pid file\n");
			fprintf(stderr, "\t%s: %s\n", PID_FILE, strerror(errno));
			exit(1);
		}
	}
	return pid;
}

int writePidFile(void)
{
	int fd,fp; 
	mode_t old_umask;
	char buf[32];
	old_umask = umask(022);
	fd = open(PID_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	fp = open(SQUID_PID_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	umask(old_umask);
	if (fd < 0 || fp < 0)
	{       
		debug(0, "%s: %s\n", PID_FILE, strerror(errno));
		return errno;
	}       
	snprintf(buf, 32, "%d\n", (int) getpid());
	write(fd, buf, strlen(buf));
	write(fp, buf, strlen(buf));
	close(fd);
	close(fp);

	return 0;
}

void delPidFile()
{
	unlink(PID_FILE);
}
