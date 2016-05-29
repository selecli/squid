#include "cc_framework_api.h"
#ifdef CC_FRAMEWORK

typedef struct code_st
{
	char code[4]; 
} code_t;

typedef struct _mod_config
{
	int num;
	code_t *code;
} mod_config;


static cc_module *mod = NULL;
static MemPool * mod_config_pool = NULL;

static void * mod_config_pool_alloc(void)
{
	void * obj = NULL; 
	if (NULL == mod_config_pool)
		mod_config_pool = memPoolCreate("mod_offline_by_code config_struct", sizeof(mod_config));
	return obj = memPoolAlloc(mod_config_pool);
}

static int free_cfg(void * data)
{
	mod_config * cfg = data;
	if (NULL != cfg)
	{
		if (NULL != cfg->code)
			xfree(cfg->code);
		memPoolFree(mod_config_pool, cfg);
		cfg = NULL;
	}
	return 0;
}

static int getFailCodes(mod_config *cfg, char *str)
{
	int i, num;	

	num = (strlen(str) + 1) / 4;
	cfg->code = xcalloc(num, sizeof(code_t));
	
	for (i = 0; i < num; i++)
	{
		if (i < num  - 1 && '|' != str[(i + 1) * 4 - 1])
		{
			debug(222, 3)("mod_offline_by_code: the conf formate is wrong\n");
			return 0;//fail
		}
		strncpy(cfg->code[i].code, str + 4 * i, 3);
		if ('*' == cfg->code[i].code[1])
			cfg->code[i].code[1] = 0; 
		if ('*' == cfg->code[i].code[2])
			cfg->code[i].code[2] = 0; 
		cfg->code[i].code[3] = 0; 
		cfg->num++;
	}

	return 1;
}

static int func_sys_parse_param(char *cfg_line)
{
	debug(222,1)("mod_offline_by_code: config is %s\n", cfg_line);

	mod_config * cfg = NULL;
	char *token = NULL;

	char *p = strstr(cfg_line, "allow");
	if (!p)
		goto err;
	*p = 0;

	if (NULL == (token = strtok(cfg_line, w_space)))
		goto err;
	else if (0 != strcmp("mod_offline_by_code", token))
		goto err;

	cfg = (mod_config *)mod_config_pool_alloc();

	// get code
	if (NULL == (token = strtok(NULL, w_space)))
		goto err;
	else if (!getFailCodes(cfg, token))
		goto err;

	cc_register_mod_param(mod, cfg, free_cfg);
	return 0;
err:
	if (NULL != cfg)
	{
		free_cfg(cfg);
		cfg = NULL;
	}
	debug(222, 1)("mod_offline_by_code: error occured when parsing cfg line\n");
	return -1;
}

int get_content_orig_down(clientHttpRequest *http, http_status status)
{
	debug(222,3) ("mod_offline_by_code: status is %d\n",status);

	if (0 == status)
		return 1;

	char code[10];

	memset(code, '\0', 10);
	snprintf(code, 10, "%d", status);
	mod_config *cfg = (mod_config*)cc_get_mod_param(http->conn->fd, mod);
	if (NULL == cfg)
		return 1;

	int i;
	const char *c; 

	for (i = 0; i < cfg->num; ++i)
	{   
		c = cfg->code[i].code;
		if (!strncmp(c, code, strlen(c)))
			return 1;
	}   

	return 0;
}

static int hook_cleanup(cc_module *module)
{
	debug(222, 1)("(mod_offline_by_code) ->     hook_cleanup:\n");
	if(NULL != mod_config_pool)
		memPoolDestroy(mod_config_pool);
	return 0;
}

int mod_register(cc_module *module)
{
	debug(222, 1)("(mod_offline_by_code) ->   mod_register:\n");
	strcpy(module->version, "7.0.R.16488.i686");

	//module->hook_func_sys_parse_param = func_sys_parse_param;
	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
			func_sys_parse_param);

	//module->hook_func_sys_cleanup = hook_cleanup;
	cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_cleanup),
			hook_cleanup);

	//module->hook_func_get_content_orig_down = get_content_orig_down;
	cc_register_hook_handler(HPIDX_hook_func_get_content_orig_down,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_get_content_orig_down),
			get_content_orig_down);

	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else 
		mod = module;
	return 1;
}

#endif
