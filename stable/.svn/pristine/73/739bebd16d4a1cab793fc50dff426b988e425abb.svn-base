/* Author: xin.yao
 * Date: 2012-12-14
 * Description: microsoft hosts switch
 * when last origin response a config http code, switch the next host
 * Config: mod_automatic_switch www.test1.com 403|404 allow test_acl
 */

#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK

#define MY_HOST_SIZE 256

cc_module *mod = NULL;

typedef struct mod_config_params_st {
	int ncodes;                      /* number of codes */
	int nhosts;                      /* number of hosts */
	http_status *codes;
	char (*hosts)[MY_HOST_SIZE + 1]; /* switch hosts */
} ConfigParams;

typedef struct switch_fde_data_st {
	int switch_count;                /* switch count */
	int reforward;                   /* reforward or not */
} SwitchFdeData;

static MemPool *mem_params_pool = NULL;

static void *memParamsPoolAlloc(void)
{
	void *obj = NULL;

	if (NULL == mem_params_pool)
	{
		mem_params_pool = memPoolCreate("mod_automatic_switch ConfigParams", sizeof(ConfigParams));	
	}
	return obj = memPoolAlloc(mem_params_pool);
}

static int freeModuleParams(void *data)
{
	ConfigParams *params = data;

	if (NULL != params)
	{
		if (NULL != params->codes)
		{
			safe_free(params->codes);
		}
		if (NULL != params->hosts)
		{
			safe_free(params->hosts);
		}
		memPoolFree(mem_params_pool, params);
	}
	return 0;
}

static int freeSDA(void *data)
{
	SwitchFdeData *sda = data;

	if (NULL != sda)
	{
		safe_free(sda);
	}

	return 0;
}

static int moduleCleanup(cc_module *module)
{
	debug(224, 3)("mod_automatic_switch: module cleanup\n");

	if (NULL != mem_params_pool)
		memPoolDestroy(mem_params_pool);

	return 0;
}

static int parseModuleParamHosts(ConfigParams *params, char *hosts)
{
	char *token = NULL;
	char *saveptr = NULL;
	size_t length = 0;
	size_t tsize = MY_HOST_SIZE + 1;

	for (;;)
	{
		if (params->nhosts)
			token = strtok_r(NULL, "|", &saveptr);
		else
			token = strtok_r(hosts, "|", &saveptr);
		if (NULL == token)
			break;
		params->hosts = xrealloc(params->hosts, (params->nhosts + 1) * tsize);
		memset(params->hosts[params->nhosts], 0, MY_HOST_SIZE);
		length = strlen(token);
		length = length > MY_HOST_SIZE ? MY_HOST_SIZE : length;
		strncpy(params->hosts[params->nhosts], token, length);
		params->hosts[params->nhosts][length] = '\0';
		debug(224, 3)("mod_automatic_switch: config Host: %s\n", params->hosts[params->nhosts]);
		params->nhosts++;
	}
	if (0 == params->nhosts)
	{
		debug(224, 3)("mod_automatic_switch: config params lack hosts\n");
		return -1;
	}
	debug(224, 3)("mod_automatic_switch: config hosts number: %d\n", params->nhosts);

	return 0;
}

static int parseModuleParamCodes(ConfigParams *params, char *codes)
{
	char *token = NULL;
	char *saveptr = NULL;
	size_t tsize = sizeof(http_status);

	for (;;)
	{
		if (params->ncodes)
			token = strtok_r(NULL, "|", &saveptr);
		else
			token = strtok_r(codes, "|", &saveptr);
		if (NULL == token)
			break;
		params->codes = xrealloc(params->codes, (params->ncodes + 1) * tsize);
		params->codes[params->ncodes] = atoi(token);
		debug(224, 3)("mod_automatic_switch: config Code: %d\n", params->codes[params->ncodes]);
		params->ncodes++;
	}
	if (0 == params->ncodes)
	{
		debug(224, 3)("mod_automatic_switch: config params lack codes\n");
		return -1;
	}
	debug(224, 3)("mod_automatic_switch: config codes number: %d\n", params->ncodes);

	return 0;
}

static int parseModuleParams(char *cfg_line)
{
	char *token = NULL;
	ConfigParams *params = NULL;

	token = strtok(cfg_line, w_space);
	if (NULL == token)
	{
		debug(224, 3)("mod_automatic_switch: config error when parsing module name\n");
		return 0;
	}
	if (strcmp(token, "mod_automatic_switch"))
	{
		debug(224, 3)("mod_automatic_switch: module name is not 'mod_automatic_switch'\n");
		return 0;
	}
	token = strtok(NULL, w_space);
	if (NULL == token)
	{
		debug(224, 3)("mod_automatic_switch: config error when parsing hosts\n");
		return 0;
	}
	params = memParamsPoolAlloc();
	params->ncodes = 0;
	params->nhosts = 0;
	params->codes = NULL;
	params->hosts = NULL;
	if (parseModuleParamHosts(params, token) < 0)
	{
		debug(224, 3)("mod_automatic_switch: config error when parsing hosts name\n");
		return 0;
	}
	token = strtok(NULL, w_space);
	if (NULL == token)
	{
		debug(224, 3)("mod_automatic_switch: config error when parsing codes\n");
		return 0;
	}
	if (parseModuleParamCodes(params, token) < 0)
	{
		debug(224, 3)("mod_automatic_switch: config error when parsing codes value");
		return 0;
	}
	cc_register_mod_param(mod, params, freeModuleParams);

	return 0;
}

static int switchCheckMatchCode(ConfigParams *params, http_status code)
{
	int i;

	for (i = 0; i < params->ncodes; ++i)
	{
		if (params->codes[i] == code)
		{
			debug(224, 3)("mod_automatic_switch: matched config code: %d\n", code);
			return 1;
		}
	}

	return 0;
}

static int autoSwitchReforwordable(FwdState *fwdState)
{
	http_status s = fwdState->entry->mem_obj->reply->sline.status;
	SwitchFdeData *sda = cc_get_mod_private_data(FWDSTATE_PRIVATE_DATA, fwdState, mod);

	if (NULL == sda || 0 == sda->reforward)
	{
		debug(224, 3)("mod_automatic_switch: reforwardable NO, code: %d\n", s);
		return 0;
	}
	debug(224, 3)("mod_automatic_switch: reforwardable YES, code: %d\n", s);
	return 1;
}

static int autoSwitchReforwardStart(HttpStateData* httpState, char * buf, ssize_t len)
{
	ConfigParams *params = cc_get_mod_param_by_FwdState(httpState->fwd, mod);
	http_status s = httpState->entry->mem_obj->reply->sline.status;
	SwitchFdeData *sda = cc_get_mod_private_data(FWDSTATE_PRIVATE_DATA, httpState->fwd, mod);

	if (NULL == sda)
	{
		debug(224, 3)("mod_automatic_switch: sda is NULL, register it now\n");
		sda = xcalloc(1, sizeof(SwitchFdeData));
		sda->switch_count = 0;
		sda->reforward = 0;
		cc_register_mod_private_data(FWDSTATE_PRIVATE_DATA, httpState->fwd, sda, freeSDA, mod);
	}
	if (switchCheckMatchCode(params, s))
	{
		if (sda->switch_count >= params->nhosts)
		{
			sda->reforward = 0;
			debug(224, 3)("mod_automatic_switch: reforward NO, code: %d, sda->switch_count: %d, params->nhosts: %d\n", s, sda->switch_count, params->nhosts);
			return 0;
		}
		sda->reforward = 1;
		debug(224, 3)("mod_automatic_switch: reforward YES, code: %d\n", s);
		/* force to complete current forward, then start the next one */
		fwdComplete(httpState->fwd);
		return 1;
	}
	sda->reforward = 0;
	debug(224, 3)("mod_automatic_switch: reforward NO, code: %d\n", s);
	return 0;
}

static int autoSwitchStoreAppendCheck(HttpStateData *httpState)
{
	ConfigParams *params = cc_get_mod_param_by_FwdState(httpState->fwd, mod);
	http_status s = httpState->entry->mem_obj->reply->sline.status;
	SwitchFdeData *sda = cc_get_mod_private_data(FWDSTATE_PRIVATE_DATA, httpState->fwd, mod);

	if (switchCheckMatchCode(params, s))
	{
		if (NULL != sda && sda->switch_count >= params->nhosts)
		{
			sda->reforward = 0;
			debug(224, 3)("mod_automatic_switch: store append YES, code: %d, sda->switch_count: %d, params->nhosts: %d\n", s, sda->switch_count, params->nhosts);
			return 1;
		}
		debug(224, 3)("mod_automatic_switch: store append NO, code: %d\n", s);
		return 0;
	}
	debug(224, 3)("mod_automatic_switch: store append YES, code: %d\n", s);
	return 1;
}

static void autoSwitchChangeHost(FwdState *fwdState, char **host, unsigned short *port)
{
	ConfigParams *params = cc_get_mod_param_by_FwdState(fwdState, mod);
	SwitchFdeData *sda = cc_get_mod_private_data(FWDSTATE_PRIVATE_DATA, fwdState, mod);

	if (NULL == sda || 0 == sda->reforward)
	{
		debug(224, 3)("mod_automatic_switch: switch host NO\n");
		return;
	}
	if (sda->switch_count >= params->nhosts)
	{
		debug(224, 3)("mod_automatic_switch: switched hosts around, sda->switch_count: %d, params->nhosts: %d\n", sda->switch_count, params->nhosts);
		return;
	}
	*host = params->hosts[sda->switch_count];
	sda->switch_count++;
	debug(224, 3)("mod_automatic_switch: switched host is: %s\n", *host);
}

int mod_register(cc_module *module)
{
	debug(224, 1)("mod_automatic_switch: init module\n");
	strcpy(module->version, "7.0.R.16488.i686");

	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
			parseModuleParams);

	cc_register_hook_handler(HPIDX_hook_func_store_append_check,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_store_append_check),
			autoSwitchStoreAppendCheck);

	cc_register_hook_handler(HPIDX_hook_func_buf_recv_from_s_header,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_buf_recv_from_s_header),
			autoSwitchReforwardStart);

	cc_register_hook_handler(HPIDX_hook_func_private_reforwardable,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_reforwardable),
			autoSwitchReforwordable);

	cc_register_hook_handler(HPIDX_hook_func_private_preload_change_hostport,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_preload_change_hostport),
			autoSwitchChangeHost);

	cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_cleanup),
			moduleCleanup);

	if (reconfigure_in_thread)
		mod = (cc_module *)cc_modules.items[module->slot];
	else
		mod = module;

	return 0;
}

#endif
