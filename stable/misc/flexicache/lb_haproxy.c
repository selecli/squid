#include "flexicache.h"

static char temp_conf[128];

static void generateHaproxyConf()
{
	pid_t pid = getpid();
	snprintf(temp_conf, 128, "/tmp/haproxy.conf.tmp.%d", pid);
	FILE *fpsrc = fopen(g_config.lb_conf_path, "r");
	if(fpsrc == NULL)
	{
		return;
	}
	FILE *fpdst = fopen(temp_conf, "w");
	if(fpdst == NULL)
	{
		fclose(fpsrc);
		return;
	}

	char confline[1024];
	while(fgets(confline, 1024, fpsrc))
	{
		fputs(confline, fpdst);
	}

	int i;
	for(i = 0; i < g_config.cache_processes ; i ++)
	{
		snprintf(confline, 1024, "\nserver s%d 127.0.0.1:%d", i + 1, 80 + i + 1);
		fputs(confline, fpdst);
	}
	fclose(fpdst);
	fclose(fpsrc);
	
}

static void haproxyStartOne()
{
	int nullfd=open("/dev/null",O_RDWR, 0644);
	dup2(nullfd, 0);
	dup2(nullfd, 1);
	dup2(nullfd, 2);
	
	execl(g_config.lb_path, "(flexicache-haproxy)",
			"-f", temp_conf,
			NULL);

}

static int haproxyStart()
{
	generateHaproxyConf();
	pid_t pid;
	if((pid = fork()) < 0)
	{
		fatalf("error when starting haproxy, %s\n", strerror(errno));
		return 1;
	}
	else if(pid > 0)
	{
		g_lb_manager.pids[0] = pid;
		return 0;
	}
	else
	{
		//close(0);
		//close(1);
		//close(2);
		haproxyStartOne();
		return 0;
	}
}


static int haproxyTerminate()
{
	//int i;
	if(kill(g_lb_manager.pids[0], SIGTERM) < 0)
	{
		debug(1, "Killing haproxy %d failed! because %s\n", g_lb_manager.pids[0], strerror(errno));
		return errno;
	}
	else
	{
		debug(1, "Killed haproxy %d\n", g_lb_manager.pids[0]);
	}
	
	if(unlink(temp_conf))
	{
		debug(1, "Unlinking conf file %s failed! because %s\n", temp_conf, strerror(errno));
		return errno;
	}
	else
	{
		debug(1, "Unlinked conf file %s\n", temp_conf);
	}
	free(g_lb_manager.pids);
	return 0;
}

static int haproxyReload()
{
	/*
	pid_t pid = fork();
	if(pid < 0)
	{
		fatalf("error when reloading haproxy, %s\n", strerror(errno));
	}
	else if(pid > 0)
	{
		debug(1, "Reconfiguring haproxy\n");
	}
	else
	{
		generateHaproxyConf();
		FILE * pp = popen(g_config.lb_path);
		
		execl("haproxy-reloader", g_config.lb_path,
				"-D",
				"-f", temp_path;, 
				"-p", "/var/run/haproxy.pid", 
				"-sf", "/var/run/haproxy.pid", 
				NULL);
		
		
	}
	return 0;
	*/
	return haproxyTerminate();
}

static int haproxyCheckRestart(pid_t pid)
{
	generateHaproxyConf();
	if(pid == g_lb_manager.pids[0])
	{
		debug(1, "loadbalancer in pid %d aborting detected, restarting\n", pid);
		pid_t pid2;
		if((pid2 = fork()) < 0)
		{
			fatalf("error when starting haproxy, %s\n", strerror(errno));
		}
		else if(pid2 > 0)
		{
			g_lb_manager.pids[0] = pid2;
		}
		else
		{
			//close(0);
			//close(1);
			//close(2);	
			haproxyStartOne();
			return 0;
		}
	}
	return 1;
}
void haproxyInit(LbManager *l)
{
	l->starter = haproxyStart;
	l->reloader = haproxyReload;
	l->terminator = haproxyTerminate;
	l->checkRestarter = haproxyCheckRestart;
	debug(1, "loadbalancer init ok, type is haproxy\n");
}
