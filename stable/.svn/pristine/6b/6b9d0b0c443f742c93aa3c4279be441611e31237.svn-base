#include "flexicache.h"

void cacheManagerInit()
{
	g_cache_manager.num_pids = 0;
	switch(g_config.cache_type)
	{
		case ct_squid:
			squidInit(&g_cache_manager);
			break;
	}

		
}

void cacheStart()
{
	g_cache_manager.starter();
}

void cacheCheckRestart(pid_t pid)
{
	g_cache_manager.checkRestarter(pid);
}

void cacheTerminate()
{
	g_cache_manager.terminator();
}

void cacheReconfigure()
{
	g_cache_manager.reloader();
}
