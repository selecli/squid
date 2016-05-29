#include "flexicache.h"

void watchManagerInit()
{
	g_watch_manager.num_pids = 0;
	switch(g_config.cache_type)
	{
		case ct_squid:
			squidWatchInit(&g_watch_manager);
			break;
	}
}

void watchStart()
{
	g_watch_manager.starter();
}

void watchTerminate()
{
	g_watch_manager.terminator();
}

void watchCheckRestart(pid_t pid)
{
    g_watch_manager.checkRestarter(pid);
}
