#include "flexicache.h"

static int squidTerminate();
static int squidStartOne(int i)
{
	char cache_processes[32];
	char cache_process_id[32];
	snprintf(cache_processes, 32, "%d", g_config.cache_processes);
	snprintf(cache_process_id, 32, "%d", i + 1);
	debug(1, "starting squid cache in pid %d, id %d\n", getpid(), i);
	int nullfd=open("/dev/null",O_RDWR, 0644);
	dup2(nullfd, 0);
	dup2(nullfd, 1);
	dup2(nullfd, 2);

	execl(g_config.cache_path, 
            "(fc-cache)", 
            "-N", 
            "-D", 
			"-A", cache_processes,
			"-a", cache_process_id,
			"-f", g_config.cache_conf_path,
			NULL);

    // should not be executed here
    debug(0, "exec squid failed!%s\n", strerror(errno));
    return -1;
}

static int squidStart()
{
	int i;
	for(i = 0; i < g_config.cache_processes ; i ++)
	{
		pid_t pid;
		if((pid = fork()) < 0)
		{
			fatalf("error when starting squid, %s\n", strerror(errno));
		}
		else if(pid > 0)        // child
		{
			g_cache_manager.pids[i] = pid;
			g_cache_manager.num_pids ++;
		}
		else                // parent
		{
			if(squidStartOne(i))
			{
				exit(1);
			}
		}
	}
	
	return 0;
}
static int squidReload()
{
	debug(1, "Reconfiguring squid\n");

	if(g_config.cache_processes == g_cache_manager.num_pids)
	{
		int i;
		for(i = 0; i < g_config.cache_processes ; i ++)
		{
			debug(2, "i < g_cache_manager.num_pids, %d < %d, configed %d\n", i, g_cache_manager.num_pids, g_config.cache_processes);
			if(kill(g_cache_manager.pids[i], SIGHUP) < 0)
			{
				debug(1, "reloading squid failed because %s\n", strerror(errno));
			}
		}
	}
	else
	{
		debug(2, "restarting squid processes g_config.cache_processes != g_cache_manager.num_pids, %d != %d\n", g_config.cache_processes, g_cache_manager.num_pids);
		squidTerminate();
		squidStart();
		sleep(3);
	}
	return 0;
}

static int squidCheckRestart(pid_t pid)
{	
	int i;
	for(i = 0; i < g_config.cache_processes ; i ++)
	{
		if(pid == g_cache_manager.pids[i])
		{
			debug(2, "cache in pid %d aborting detected, now %d processes left , restarting\n",
				       	pid, g_cache_manager.num_pids - 1);
			g_cache_manager.num_pids --;
			pid_t pid2;
			if((pid2 = fork()) < 0)
			{
				fatalf("error when starting squid, %s\n", strerror(errno));
			}
			else if(pid2 > 0)
			{
				g_cache_manager.pids[i] = pid2;
				g_cache_manager.num_pids ++;
			}
			else
			{
				if(squidStartOne(i))
				{
					exit(1);
				}
			}

		}
				
	}
	return 0;
}

static int squidTerminate()
{
	int i;
	int temp_num_pids = g_cache_manager.num_pids;
	for(i = 0; i < temp_num_pids; i ++)
	{

		
		if(kill(g_cache_manager.pids[i], SIGTERM) < 0)
		{
			debug(1, "Killing squid %d failed! because %s\n", g_cache_manager.pids[i], strerror(errno));
			return errno;
		}
		else
		{
			g_cache_manager.num_pids --;
			debug(1, "Killed squid %d\n", g_cache_manager.pids[i]);
		}
		
		//Please preserve the code below, as squid may change its shutdown signal some day
		/*
		char cache_processes[32];
		char cache_process_id[32];
		snprintf(cache_processes, 32, "%d", g_config.cache_processes);
		snprintf(cache_process_id, 32, "%d", i + 1);
		debug(1, "shutting down squid %d/%d\n", i + 1 , g_config.cache_processes);
		pid_t forkedPid;
		if((forkedPid = fork()) < 0)
		{
			exit(1);
		}
		else if(forkedPid == 0)
		{
			if(execl(g_config.cache_path, "(flexicache-squid)", 
						"-k", "shutdown", 
						"-A", cache_processes,
						"-a", cache_process_id,
						"-f", g_config.cache_conf_path,
						NULL) != 0)
			{
				debug(0, "shutdown squid failed!%s\n", strerror(errno));
				return errno;
			}
		}
		*/

	}
	return 0;
}


void squidInit(CacheManager *c)
{
	c->starter = squidStart;
	c->reloader = squidReload;
	c->terminator = squidTerminate;
	c->checkRestarter = squidCheckRestart;

	debug(1, "cache init ok, type is squid\n");
}

