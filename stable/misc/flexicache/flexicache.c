#include "flexicache.h"

static int checkRunningPid(void);
static int daemonize();
static void setSignalHandlers();
static int do_rotatelog = 0;
static int do_not_restart = 0;
static int parseOptions(int argc, char ** argv);
static void handle_shutdown();
static int opt_send_signal = -1;
static void usage();
static int checkCacheIsRunning();
static int checkOneCacheByID(int id);
static void killAllPorts();

int main(int argc, char ** argv)
{
	logInit();
    
	loadConfigFile(CONF_PATH, &g_config);

	debug(1, "loaded config file ok\n");
	parseOptions(argc, argv);

	if(opt_send_signal >= 0)
	{
		pid_t runningPid = readPidFile();
		if(runningPid < 2)
		{
			debug(1, "flexicache Error: No running copy!\n");
			return 1;
		}

		if(kill(runningPid, opt_send_signal) != 0)
		{
			debug(1, "flexicache Error: can not send signal %d to process %d, %s\n", 
					opt_send_signal, runningPid, strerror(errno));
			return 1;
		}
		return 0;
	}
	if(checkRunningPid())
	{
		exit(1);
	}

	
	//killAllPorts();//20140529_1 add by fufei.shang when port resuse
	cacheManagerInit();
	lbManagerInit();
    watchManagerInit();

	int dresult = 0;
	if((dresult = daemonize()) != 0)
	{
		exit(dresult);
	}
	setSignalHandlers();

	writePidFile();

	cacheStart();
	
    checkCacheIsRunning();
    
    g_watch_argv = argv;
    watchStart();
	
    lbStart();
    sleep(2);


	time_t start;
	time_t stop;
	int failcount = 0;
	for(;;)
	{
		time(&start);
		int status;
		pid_t pid = waitpid(-1, &status, 0);

		if(do_not_restart)
		{
			//debug(1, "received SIGTERM, shutting down\n");
			//exit(0);
		}
		else if(!WIFEXITED(status) || ! do_not_restart)
		{
			time(&stop);

			if (stop - start < 10)
			{
				failcount++;
			}
			else
			{	
				failcount = 0;
			}

			if(failcount >= 3)//20140529_2 add by fufei.shang when port resuse
			{
				debug(0, "FATAL: children are crashing too rapidly, need help!");
				handle_shutdown();
				break;//20140529_3 add by fufei.shang when port resuse
			}

			debug(1, "before restarting cache and lb, WIFEXITED=%d, do_not_restart=%d, EXITSTATUS=%d, pid=%d\n", WIFEXITED(status), do_not_restart, WEXITSTATUS(status), pid);
			cacheCheckRestart(pid);
            watchCheckRestart(pid);
			lbCheckRestart(pid);
		}
		else
		{
			debug(1, "process %d normally exited\n", pid);
		}
		
	}
	return 0;
}

static void killOnePort(int port)
{
	char kill_port_cmd[128];
	char buf[1024];
	snprintf(kill_port_cmd, 128, "/sbin/fuser -k -9 -n tcp %d 2>&1", port);
	FILE *pp  = popen(kill_port_cmd, "r");
	if(!pp)
	{
		debug(0, "popen returned NULL!\n");
		exit(1);
	}
	
	while(fgets(buf, sizeof(buf), pp))
	{
		debug(1, "%s\n", buf);
	}

	pclose(pp);
}

static void killAllPorts()
{
	if(g_config.kill_port <= 0)
	{
		return;
	}
	debug(1, "killing main port %d\n", g_config.kill_port);
	killOnePort(g_config.kill_port);
	
	int i = 0;
    int j = 0;
    int port = 0;
    while (g_config.cache_ports != NULL && g_config.cache_ports[i] != 0) 
    {
        port = g_config.cache_ports[i];
        if (port <= 0 || port + g_config.cache_processes >= 65535) {
	        debug(1, "Configed a error port %d and exit\n", port);
            exit(2);
        }

        for(j = port ; j  <= port + g_config.cache_processes; j++)
        {
            debug(1, "killing cache port %d\n", j);
            killOnePort(j);
        }
        i++;
    }
}

static int parseOptions(int argc, char ** argv)
{
	extern char *optarg;
	int c;
	while ((c = getopt(argc, argv, "k:hv")) != -1)
	{
		switch(c)
		{
			case 'k':
				debug(2, "option name = -k\n");
				if(!strcmp(optarg, "shutdown"))
				{
					debug(2, "option name/value = -k/shutdown\n");
					opt_send_signal = SIGTERM;
				}
				else if(!strcmp(optarg, "rotatelog"))
				{
					opt_send_signal = SIGUSR1;
					debug(2, "option name/value = -k/rotatelog\n");
				}
				else if(!strcmp(optarg, "reconfigure"))
				{
					opt_send_signal = SIGHUP;
					debug(2, "option name/value = -k/reconfigure\n");
				}
				else
				{
					usage();
					return 1;
				}
				break;
			case 'h':
				usage();
				break;
			case 'v':
				printf("flexicache version %s\n", VERSION);
				exit(0);
				break;
			default:
				usage();
				return 1;
		}
	}
	return 0;
}

static void usage()
{
	fprintf(stderr, "Usage:flexicache [-k shutdown|rotatelog|reconfigure] \n");
	exit(1);
}

static int checkRunningPid(void)
{
	pid_t pid;
	pid = readPidFile();
	if (pid < 2)
		return 0;
	if (kill(pid, 0) < 0)
		return 0;
	fprintf(stderr, "flexicache is already running!  Process ID %ld\n", (long int) pid);
	return 1;
}

static int daemonize()
{
	pid_t pid = fork();
	if(pid < 0)
	{
        debug(0, "daemonize failed: fork error\n");
		return pid;
	}
	else if(pid > 0)
	{
		exit(0); // parent
	}
	else    // child
	{
		int ssresult;
		if ((ssresult = setsid()) < 0)
		{
            debug(0,"daemonize failed: setsid error\n");
			return ssresult;
		}
#ifdef TIOCNOTTY
		int i;
		if ((i = open("/dev/tty", O_RDWR | O_TEXT)) >= 0)
		{       
			ioctl(i, TIOCNOTTY, NULL);
			close(i);
		}       
#endif
        close(0);
        close(1);
        close(2);
        chroot("/");
		return 0;
	}
}

static void change_nginx_conf_port()
{
	chmod("/usr/local/squid/bin/lscsConfig", 0755);
	if (-1 == system("/usr/local/squid/bin/lscsConfig"))
	{
		debug(0, "system(/usr/local/squid/bin/lscsConfig) failed: %s\n", strerror(errno));
		debug(0, "nginx.conf can not change port!!");
	}
}

static void handle_reconfigure()
{
	debug(1, "received SIGHUP in handle_reconfigure\n");
	do_not_restart = 1;
	loadConfigFile(CONF_PATH, &g_config);
	cacheReconfigure();
	change_nginx_conf_port();
	lbReconfigure();
	debug(3, "set do_not_restart = 0 in handle_reconfigure\n");
	do_not_restart = 0;
}

static void handle_rotatelog()
{
	debug(1, "received SIGUSR1 in handle_rotatelog\n");
	do_rotatelog = 1;
	log_rotate();
	do_rotatelog = 0;
}

static void handle_shutdown()
{
	debug(1, "received SIGTERM in handle_shutdown\n");
	do_not_restart = 1;	
	cacheTerminate();
	lbTerminate();
    watchTerminate();
	delPidFile();
	while(g_config.need_free_pos >= 0)
	{
		free(g_config.need_free_ptr[g_config.need_free_pos]);
		g_config.need_free_pos--;
	}
	exit(0);
}

static void setSignalHandlers()
{
	signal(SIGHUP, handle_reconfigure);
	signal(SIGUSR1, handle_rotatelog);
	struct sigaction saterm;
	saterm.sa_flags = SA_NODEFER | SA_RESETHAND | SA_RESTART;
	saterm.sa_handler = handle_shutdown;
	sigaction(SIGTERM, &saterm, NULL);
	
	struct sigaction sapipe;
	sapipe.sa_flags = SA_RESTART;
	sapipe.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sapipe, NULL);
}

static int 
checkCacheIsRunning()
{
	int i, status, fail_count, ret = -1;
	status = 1; //default is OK;
	fail_count = 0;

	sleep(5); //First sleep 5s, to wait backend Cache start;
	while(1)
	{
        status = 1;
        
        for (i = 1; i <= g_config.cache_processes; i++)
        {
            ret = checkOneCacheByID(i);
            if (-1 == ret)
            {
                status = 0;
            } 
        }

		fail_count++;
		if (fail_count * 10 > 1800){
			debug(1, "Flexicache Exit now, Because backend Squid haven't Started within 20min\n");
            cacheTerminate();
			exit(111);
		}

		if (!status ){
			//debug(1, "Flexicache Start to sleep 10s, and then check Squid at next routine\n");
			sleep(10);
			continue; 
		} else {
            debug(1, "Starting cache successfully\n", i);
			return 1; 
		}
	}

}

static int 
checkOneCacheByID(int id)
{
    int fd;
    struct sockaddr_un addr;
    char path[256];
    
    snprintf(path, 256, "/tmp/xiaosi%d", id);
    
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
    {
		debug(1, "%s, Cache xiaosi%d fail:%s\n", __FUNCTION__, id, strerror(errno));
		return -1;
    }

    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, path);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0)
    {
		debug(1, "%s, Cache %s fail:%s\n", __FUNCTION__, path, strerror(errno));
		return -1;
    }

    close(fd);
    
    debug(1, "Check cache %s successfully\n", path);
    return 1;
}

