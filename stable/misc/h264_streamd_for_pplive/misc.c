#include "h264_streamd.h"
#include "misc.h"

const char *xstrerror(void)
{
	static char xstrerror_buf[BUFSIZ];
	const char *errmsg;

	errmsg = strerror(errno);

	if (!errmsg || !*errmsg)
		errmsg = "Unknown error";

	snprintf(xstrerror_buf, BUFSIZ, "(%d) %s", errno, errmsg);
	return xstrerror_buf;
}

/*pid_t readPidFile(void)
{
    FILE *pid_fp = NULL;
    const char *f = g_cfg.pid_file;
    pid_t pid = -1;
    int i;
	char *appname = "h264_streamd";

    if (f == NULL || !strcmp(g_cfg.pid_file, "none")) {
		fprintf(stderr, "%s: ERROR: No pid file name defined\n", appname);
		exit(1);
    }

    pid_fp = fopen(f, "r");
    if (pid_fp != NULL) {
		pid = 0;
		if (fscanf(pid_fp, "%d", &i) == 1)
	    	pid = (pid_t) i;
		fclose(pid_fp);
    } else {
		if (errno != ENOENT) {
	    	fprintf(stderr, "%s: ERROR: Could not read pid file\n", appname);
	    	exit(1);
		}
    }
    return pid;
}

void check_pid()
{
	pid_t pid;
    
	if (strcmp(g_cfg.pid_file, "none") == 0) {
		do_log(LOG_DEBUG, "No pid_filename specified. Trusting you know what you are doing.\n");
		return;
    }
    
	pid = readPidFile();
    
	if (pid < 2)
		return;
    
	if (kill(pid, 0) < 0)
		return;

	do_log(LOG_DEBUG, "h264_streamd is already running!  Process ID %ld\n", (long int) pid);
} */

void
daemon_handler()
{
	int                 i, fd0, fd1, fd2;
	pid_t               pid;
	struct rlimit       rl;
	struct sigaction    sa;
	umask(0);

	if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
		exit(-1);

	if ((pid = fork()) < 0)
		exit(-1);
	else if (pid != 0) /* parent */
		exit(0);
	setsid();

	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if (sigaction(SIGHUP, &sa, NULL) < 0)
		exit(-1);
	if ((pid = fork()) < 0)
		exit(-1);
	else if (pid != 0) /* parent */
		exit(0);

	if (chdir("/") < 0)
		exit(-1);

	if (rl.rlim_max == RLIM_INFINITY)
		rl.rlim_max = 1024;
	for (i = 0; i < rl.rlim_max; i++)
		close(i);

	fd0 = open("/dev/null", O_RDWR);
	fd1 = dup(0);
	fd2 = dup(0);

	if (fd0 != 0 || fd1 != 1 || fd2 != 2) {
		exit(-1);
	}
}


