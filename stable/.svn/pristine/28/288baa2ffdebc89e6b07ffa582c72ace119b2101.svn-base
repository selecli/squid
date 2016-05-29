#include "flexicache.h"

void lbManagerInit()
{
	g_lb_manager.pids = calloc(1, sizeof(pid_t));	
	switch(g_config.lb_type)
	{
		case lt_haproxy:
			haproxyInit(&g_lb_manager);
			break;
		case lt_lscs:
			lscsInit(&g_lb_manager);
			break;
	}

}

void lbStart()
{
	g_lb_manager.starter();
}

void lbCheckRestart(pid_t pid)
{
	g_lb_manager.checkRestarter(pid);
}

void lbTerminate()
{
	g_lb_manager.terminator();
}

void lbReconfigure()
{
	g_lb_manager.reloader();
}
