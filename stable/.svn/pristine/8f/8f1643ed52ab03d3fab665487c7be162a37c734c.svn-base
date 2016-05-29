/* Writtern by chenqi @2013/01/07 */

#include "cc_framework_api.h"
#include "store_coss.h"
#ifdef CC_FRAMEWORK
static cc_module* mod = NULL;

struct mod_conf_param {
	int membuf;
};

static MemPool * mod_config_pool = NULL; 

static void * mod_config_pool_alloc(void)
{
	void * obj = NULL; 
	if (NULL == mod_config_pool) {   
		mod_config_pool = memPoolCreate("mod_coss_membuf", sizeof(struct mod_conf_param));
	}   
	return obj = memPoolAlloc(mod_config_pool);
}

static int free_callback(void* param)
{
	struct mod_conf_param* data = (struct mod_conf_param*) param;
	if(data) {
		memPoolFree(mod_config_pool, data);
		data = NULL;
	}
	return 0;
}

static int func_sys_parse_param(char *cfg_line)
{
	char* token = NULL;
	if (NULL == (token = strtok(cfg_line, w_space)))
		goto err;

	struct mod_conf_param *data = NULL;
	data = mod_config_pool_alloc();
	memset(data,0, sizeof(struct mod_conf_param));

	if (NULL == (token = strtok(NULL, w_space)))
		goto err;
	data->membuf = atoi(token);
	cc_register_mod_param(mod, data, free_callback);
	debug(151, 1)("mod_coss_membuf ->  parse OK [membuf=%d]\n",data->membuf);
	return 0;

err:
	free_callback(data);
	debug(151, 1)("mod_coss_membuf ->  parse line error [%s]\n",cfg_line);
	return -1;
}


static void func_coss_membuf()
{
	struct mod_conf_param *param = (struct mod_conf_param *) cc_get_global_mod_param(mod);
	if (NULL == param || param->membuf < 3) {
		return;
	}

	int i, dirn;
	off_t max_offset;
	SwapDir *sd;
	CossInfo *cs;

	for (dirn = 0; dirn < Config.cacheSwap.n_configured; dirn++)
	{ 
		sd = &Config.cacheSwap.swapDirs[dirn];
		if (strcmp(sd->type, "coss")) {
			continue;
		}
		cs = sd->fsdata;
		cs->nummemstripes = param->membuf;
		max_offset = (off_t) 0xFFFFFFF << cs->blksz_bits;
		if ((sd->max_size + (cs->nummemstripes * (COSS_MEMBUF_SZ >> 10))) > (unsigned long) (max_offset >> 10)) {
			debug(47, 1) ("COSS block-size = %d bytes\n", 1 << cs->blksz_bits);
			debug(47, 1) ("COSS largest file offset = %lu KB\n", (unsigned long) max_offset >> 10);
			debug(47, 1) ("COSS cache_dir size = %d KB\n", sd->max_size);
			fatal("COSS cache_dir size exceeds largest offset\n");
		}

		debug(47, 2) ("COSS: number of memory-only stripes %d of %d bytes each\n", cs->nummemstripes, COSS_MEMBUF_SZ);
		xfree(cs->memstripes);
		cs->memstripes = xcalloc(cs->nummemstripes, sizeof(struct _cossstripe));
		for (i = 0; i < cs->nummemstripes; i++) {
			cs->memstripes[i].id = i;
			cs->memstripes[i].membuf = NULL;
			cs->memstripes[i].numdiskobjs = -1;
		}
	}
}


/* module init */
int mod_register(cc_module *module)
{
	debug(98, 1)("(mod_coss_membuf) ->  init: init module\n");
	strcpy(module->version, "7.0.R.16488.i686");

	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
			func_sys_parse_param);

	cc_register_hook_handler(HPIDX_hook_func_coss_membuf,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_coss_membuf),
			func_coss_membuf);

	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	return 1;
}
#endif
