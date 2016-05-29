#include "cc_framework_api.h"
#include <stdbool.h>

#ifdef CC_FRAMEWORK

#define PARHSIZE 1024

static cc_module* mod = NULL;

typedef struct _mod_config_param
{
    char path[1024];
    char name[256];
    short flag;
} mod_conf_param;

static MemPool *mod_config_pool = NULL;

static void *mod_config_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == mod_config_pool)
	{   
		mod_config_pool = memPoolCreate("mod_use_script config_struct", sizeof(mod_conf_param));
	}   
	return obj = memPoolAlloc(mod_config_pool);
}

static int free_callback(void* param)
{
	struct mod_conf_param* data = (struct mod_conf_param*) param;
	if(data)
	{    
		memPoolFree(mod_config_pool, data);
		data = NULL;
	}
	return 0;
}

static mod_conf_param *cfg = NULL;
static pid_t pid = 0;

static int func_sys_parse_param(char *cfg_line)
{
    debug(224,1)("(mod_use_script): func_sys_parse_param parse param begin\n");
	mod_conf_param *data = mod_config_pool_alloc();
	memset(data,0,sizeof(mod_conf_param));
	cfg = data;

    char *token = NULL;
    if ((token = strtok(cfg_line, w_space)) == NULL)
		goto err;

    if (NULL == (token = strtok(NULL, w_space)))
		goto err;
	data->flag = atoi(token);
	if (data->flag < 0)
	{
		debug(224,1)("(mod_use_script): func_sys_parse_param error in flag [%d]\n", data->flag);
		goto err;
	}

	if (NULL == (token = strtok(NULL, w_space)))
		goto err;
	strcpy(data->path,token);

	if (NULL == (token = strtok(NULL, w_space)))
		goto err;
	strcpy(data->name,token);

	if (access(data->path, F_OK) < 0 || access(data->path, X_OK) < 0)
	{
		debug(224,1)("mod_use_script: [%s]: %s\n", data->path, strerror(errno));
		goto err;
	}

	debug(224,1)("mod_use_script: parse success, type=%s\tpath=%s\n",data->flag?"launch":"kill",data->path);
	cc_register_mod_param(mod, data, free_callback);
    return 0;

err:
	debug(224,1)("mod_use_script: parse error\n");
	free_callback(data);
	return -1;
}

static void hook_init()
{
    debug(224, 1)("(mod_use_script): hook_init begin\n");
    int ret;
    char buff[1024];
    FILE *procfd;
    memset(buff, 0, 1024);

    debug(224,1)("(mod_use_script): path is [%s], name is [%s], flag is [%d]\n", cfg->path, cfg->name, cfg->flag);

    if (cfg->flag > 0 && pid >= 0)		// launch a process
    {
        debug(224, 1)("(mod_use_script): pid is [%d]\n", pid);
        if (pid > 0)
        {
            snprintf(buff, sizeof(buff), "/proc/%d/status", pid);
            debug(224, 1)("(mod_use_script): pid'status path is [%s]\n", buff);
            ret = access(buff, F_OK);
            debug(224, 1)("(mod_use_script): ret is [%d]\n", ret);
            if (ret == 0)
            {
                if( (procfd = fopen(buff,"r")) == NULL ) 
                { 
                    debug(224, 1)("(mod_use_script): fopen [%s] failed\n", buff);
                    return;
                }

                fgets(buff, sizeof(buff), procfd);
                debug(224, 1)("(mod_use_script): buff is [%s]\n", buff);
                fclose(procfd);
                if (strstr(buff, cfg->name) != NULL ) 
                {
                    debug(224, 1)("(mod_use_script): found same process:[%s]\n", cfg->name);
                    return;
                }   
            }
        }

        enter_suid();
        pid = fork(); 
        if (pid == 0)		// child
        {
            debug(224,1)("mod_use_script: execl [%s]\n", cfg->name);
            if  (execl(cfg->path, cfg->name, (char *)0) < 0)
            {
                debug(224, 1)("(mod_use_script): execl [%s]\n", strerror(errno));
                exit(-1);
            }
        }
        else if (pid < 0)		 
        {
            debug(224, 1)("(mod_use_script): pid < 0 [%s]\n", strerror(errno));
            leave_suid();
            exit(-1);
        }
		// parent
        debug(224, 1)("(mod_use_script): child pid is [%d]\n", pid);
        leave_suid();
    }
    else		// kill a process
    {
        if(cfg->flag == 0 && pid > 0)
        {
            snprintf(buff, sizeof(buff), "/proc/%d/status", pid);
            debug(224, 1)("(mod_use_script): pid'status path is [%s]\n", buff);

            if( (procfd = fopen(buff,"r")) == NULL ) { 
                debug(224, 1)("(mod_use_script): fopen [%s] failed\n", buff);
                return;
            }

            fgets(buff, sizeof(buff), procfd);
            debug(224, 1)("(mod_use_script): buff is [%s]\n", buff);
            fclose(procfd);
            if (strstr(buff, cfg->name) == NULL ) { 
                debug(224, 1)("(mod_use_script): buff is [%s] , cfg->name is[%s]\n", buff, cfg->name);
                return;
            }      

            ret = kill(pid, 9);
            if (ret < 0)
            {   
                debug(224, 1)("(mod_use_script): kill failed [%s]\n", strerror(errno));
                exit(-1);
            }
            debug(224, 1)("(mod_use_script): kill [%s] success\n", cfg->name);
            pid = 0;
        }
    }

    return;
}

int mod_register(cc_module *module)
{
    debug(224, 1)("(mod_use_script) ->	mod_register:\n");
    strcpy(module->version, "7.0.R.16488.i686");

    //module->hook_func_sys_parse_param = func_sys_parse_param;
    cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
            module->slot, 
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
            func_sys_parse_param);

    //module->hook_func_sys_init = hook_init;
    cc_register_hook_handler(HPIDX_hook_func_before_sys_init,
            module->slot, 
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_before_sys_init),
            hook_init);

	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
    return 0;
}

#endif
