#include "cc_framework_api.h"
#ifdef CC_FRAMEWORK

static cc_module* mod = NULL;
static FILE* trend_log_file;
static time_t last, now;

static int record_trend_access_log(clientHttpRequest* http)
{
    AccessLogEntry *al = &http->al;
    now = time(NULL);
    int interval = now -last;
    if(interval >=300)
    {
        char file_name[256];
        snprintf(file_name,256,"/data/proclog/log/squid/trend_receipt%ld.log",now);
        fclose(trend_log_file);
        char mv_cmd[256];
        snprintf(mv_cmd,256,"mv /data/proclog/log/squid/trend_receipt%ld.log /data/proclog/log/squid/trend_receipt",last);
        system(mv_cmd);
        trend_log_file = fopen(file_name,"a");
        last = now;
    }
    String hdr = httpHeaderGetByName(&al->request->header,"Host");
    fprintf(trend_log_file,"t=%ld&i=%s&c=CN&d=%s&u=%s\n",(current_time.tv_sec),inet_ntoa(al->cache.caddr),strBuf(hdr),strBuf(al->request->urlpath));
    stringClean(&hdr);
    fflush(trend_log_file);
    return 0;
}

static int sys_init()
{

	last = time(NULL);

	system("mkdir -p /data/proclog/log/squid/trend_receipt");
	system("chown -R squid:squid /data/proclog/log/squid/trend_receipt");
	system("mv /data/proclog/log/squid/trend_receipt*.log /data/proclog/log/squid/trend_receipt");
	char file_name[256];
	snprintf(file_name,256,"/data/proclog/log/squid/trend_receipt%ld.log",last);
	trend_log_file = fopen(file_name,"a");
	return 0;
}

static int cleanup(cc_module* mod)
{
	fclose(trend_log_file);
	return 0;
}

int mod_register(cc_module *module)
{
	debug(114, 1)("(mod_trend_receipt) ->  init: init module\n");
	strcpy(module->version, "7.0.R.16488.i686");

	cc_register_hook_handler(HPIDX_hook_func_sys_init,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_init),
			sys_init);

	cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_cleanup),
			cleanup);

	cc_register_hook_handler(HPIDX_hook_func_private_record_trend_access_log,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_record_trend_access_log ),
			record_trend_access_log );

	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	return 0;
}

#endif
