#include "flexicache.h"

static char temp_conf[128];

static void generatelscsConf()
{

	snprintf(temp_conf, 128, "/usr/local/squid/bin/lscs/conf/default.inc");
	FILE *fpdst = fopen(temp_conf, "w");
	if(fpdst == NULL)
	{
		fclose(fpdst);
		return;
	}
	char confline[1024];
	int i;
	for(i = 0; i < g_config.cache_processes ; i ++)
	{
		snprintf(confline, 1024, "\t\tbackends /tmp/xiaosi%d;\n", i + 1);
		fputs(confline, fpdst);
	}
	fclose(fpdst);
	
}

static void lscsStartOne()
{
	int nullfd=open("/dev/null",O_RDWR, 0644);
	dup2(nullfd, 0);
	dup2(nullfd, 1);
	dup2(nullfd, 2);
	
	execl(g_config.lb_path,"(fc-lscs)", NULL);
    debug(0, "execl lscs failed: %s\n", strerror(errno));
}

static int lscsCheckConfig(void)
{
	chmod("/usr/local/squid/bin/lscsConfig", 0755);
	if (-1 == system("/usr/local/squid/bin/lscsConfig"))
	{
		debug(0, "system(/usr/local/squid/bin/lscsConfig) failed: %s\n", strerror(errno));
	}
	return 0;
}

static int lscsStart()
{
	generatelscsConf();
	lscsCheckConfig();
	pid_t pid;
	if((pid = fork()) < 0)
	{
		fatalf("error when starting lscs, %s\n", strerror(errno));
		return 1;
	}
	else if(pid > 0)    // parent
	{
		g_lb_manager.pids[0] = pid;
		return 0;
	}
	else        // child
	{
		lscsStartOne();
		debug(0, "lscs execl return\n");
		return 0;
	}
}


static int lscsTerminate()
{
	//int i;
	if(kill(g_lb_manager.pids[0], SIGTERM) < 0)
	{
		debug(1, "Killing lscs %d failed! because %s\n", g_lb_manager.pids[0], strerror(errno));
		return errno;
	}
	else
	{
		debug(1, "Killed lscs %d\n", g_lb_manager.pids[0]);
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

static int lscsReload()
{
    if(kill(g_lb_manager.pids[0], SIGHUP) < 0)
    {
        debug(1, "reloading lscs %d failed! because %s\n", g_lb_manager.pids[0], strerror(errno));
        return errno;
    }
    else
    {
        debug(1, "reload lscs %d successfully\n", g_lb_manager.pids[0]);
    }
    return 0;
}

static int lscsCheckRestart(pid_t pid)
{
	generatelscsConf();
	lscsCheckConfig();
	if(pid == g_lb_manager.pids[0])
	{
		debug(1, "loadbalancer in pid %d aborting detected, restarting\n", pid);
		pid_t pid2;
		if((pid2 = fork()) < 0)
		{
			fatalf("error when starting lscs, %s\n", strerror(errno));
		}
		else if(pid2 > 0)
		{
			g_lb_manager.pids[0] = pid2;
		}
		else
		{
			lscsStartOne();
			return 0;
		}
	}
	return 1;
}
void lscsInit(LbManager *l)
{
	l->starter = lscsStart;
	l->reloader = lscsReload;
	l->terminator = lscsTerminate;
	l->checkRestarter = lscsCheckRestart;
	debug(1, "loadbalancer init ok, type is lscs\n");
}
