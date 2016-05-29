#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK

#define allow 1
#define deny  0
#define MAX_DIR_LEN 100
#define ws " :"

typedef struct dir_path_st {
	int dirn;        /* dirn is the path index in cache dirs */
	char *path;      /* cache dir path */
} dir_path_t;

typedef struct _mod_assign_dir_cfg {
	int num;
	int dirn_init;   /* 1: initialized by cache dirs; 0: otherwise */
	dir_path_t *dirs;
} mod_assign_dir_cfg;

typedef struct _mod_se_private_data{
	int client_fd;
} mod_se_private_data;

typedef int LOCAL_DIRSELECT(const StoreEntry *, const mod_assign_dir_cfg *);

static cc_module *mod=NULL;
static MemPool *mod_assign_dir_cfg_pool=NULL;
static MemPool *mod_se_private_data_pool=NULL;
static mod_assign_dir_cfg assign_dirs = {0, 0, NULL};
static mod_assign_dir_cfg g_cfg = {0, 0, NULL};

static void dirsSafeFree(mod_assign_dir_cfg *cfg)
{
	int i;

	if (NULL != cfg)
	{
		if (NULL != cfg->dirs)
		{
			for (i = 0; i < cfg->num; ++i)
			{
				if (NULL != cfg->dirs[i].path)
				{
					xfree(cfg->dirs[i].path);
				}
			}
			xfree(cfg->dirs);
			cfg->dirs = NULL;
		}
	}
}

static void assignDirsfree(void)
{
	int i;

	if (assign_dirs.dirs != NULL)
	{
		for (i = 0; i < assign_dirs.num; ++i)
		{
			if (assign_dirs.dirs[i].path != NULL)
			{
				xfree(assign_dirs.dirs[i].path);
			}
		}
		xfree(assign_dirs.dirs);
	}
	assign_dirs.num = 0;
	assign_dirs.dirn_init = 0;
	assign_dirs.dirs = NULL;
}

/****************Free the mod's config data*******/
static int mod_assign_dir_cfg_free(void *data)
{
	mod_assign_dir_cfg *cfg=(mod_assign_dir_cfg *)data;

	if(cfg)
	{
		dirsSafeFree(cfg);
		memPoolFree(mod_assign_dir_cfg_pool, cfg);
		cfg = NULL;
	}
	return 0;
}


/****************Free the mod's se_private_data*******/
static int mod_se_private_data_pool_free(void *data)
{
	mod_se_private_data *se_private_data=(mod_se_private_data *)data;
	if(se_private_data)
	{
		memPoolFree(mod_se_private_data_pool,se_private_data);
		se_private_data=NULL;	
	}
	return 0;
}

/*static int param_value_free(void *data)
  {
  param_value *value=(param_value*)data;

  if(value)
  {
  memPoolFree(param_value_pool,value);
  value=NULL;
  }
  return 0;
  }*/


/******************Alloc the config data*********************/
static void* mod_assign_dir_cfg_pool_alloc(void)
{
	void *obj;

	if(mod_assign_dir_cfg_pool==NULL)
		mod_assign_dir_cfg_pool=memPoolCreate("mod_assign_dir mod_assign_dir_cfg_struct",sizeof(mod_assign_dir_cfg));
	return obj=memPoolAlloc(mod_assign_dir_cfg_pool);
}


/*******************Alloc the StoreEntry's  private data*******/
static void* mod_se_private_data_pool_alloc(void)
{
	void *obj;
	if(mod_se_private_data_pool==NULL)
		mod_se_private_data_pool=memPoolCreate("mod_assign_dir private_data",sizeof(mod_se_private_data));

	return obj=memPoolAlloc(mod_se_private_data_pool);
}

static void assignCacheDirsAdd(const char *dir)
{
	int i;

	assert(dir != NULL);

	for (i = 0; i < assign_dirs.num; ++i)
	{
		if (!strcmp(assign_dirs.dirs[i].path, dir))
		{
			/* already record the dir */
			return;
		}
	}
	if (0 == assign_dirs.num)
	{
		assign_dirs.dirs = xcalloc(1, sizeof(dir_path_t));
	}
	else
	{
		assign_dirs.dirs = xrealloc(assign_dirs.dirs, (assign_dirs.num + 1) * sizeof(dir_path_t));
	}
	assign_dirs.dirs[assign_dirs.num].path = xstrdup(dir);
	assign_dirs.num++;
	debug(157, 3)("mod_assign_dir: assign_dirs add dir: %s; number: %d\n", dir, assign_dirs.num);
}

static int func_sys_parse_param(char *cfg_line)
{
	int i;
	char *token = NULL;
	mod_assign_dir_cfg *cfg = NULL;

	debug(157, 1)("mod_assign_dir: cfg_line is: %s\n",cfg_line);

	token = strtok(cfg_line, ws);
	if (NULL == token)
	{
		goto out;
	}
	if (strcmp(token, "mod_assign_dir"))
	{
		debug(157, 1)("mod_assign_dir: module name error, name: %s\n", token);
		goto out;
	}

	cfg = mod_assign_dir_cfg_pool_alloc();
	cfg->num = 0;
	cfg->dirn_init = 0;
	cfg->dirs = NULL;

	do {
		token = strtok(NULL, ws);
		if (NULL == token)
		{
			if (0 == cfg->num)
			{
				debug(157, 1)("mod_assign_dir: config error, no cache dir configed!\n");
				goto out;
			}
			debug(157, 1)("mod_assign_dir: config error, no allow/deny!\n");
			goto out;
		}
		if (!strcmp(token, "allow") || !strcmp(token, "deny"))
		{
			if (0 == cfg->num)
			{
				debug(157, 1)("mod_assign_dir: config error, no cache dir configed!\n");
				goto out;
			}
			goto parse_ok;
		}
		if (0 == cfg->num)
		{
			cfg->dirs = xcalloc(1, sizeof(dir_path_t));
		}
		else
		{
			cfg->dirs = xrealloc(cfg->dirs, (cfg->num + 1) * sizeof(dir_path_t));
		}
		cfg->dirs[cfg->num].path = xstrdup(token);
		cfg->dirs[cfg->num].dirn = -1;
		cfg->num++;
		debug(157, 1)("mod_assign_dir: config cache dir %d is: %s\n", cfg->num, token);
	} while (1);

parse_ok:
	assert(cfg != NULL && cfg->num != 0);
	for (i = 0; i < cfg->num; ++i)
	{
		assignCacheDirsAdd(cfg->dirs[i].path); /* record into assgin_cache_dirs */
	}
	cc_register_mod_param(mod, cfg, mod_assign_dir_cfg_free);
	return 0;

out:
	debug(157, 1)("mod_assign_dir: config error\n");
	if (cfg)
	{
		dirsSafeFree(cfg);
		memPoolFree(mod_assign_dir_cfg_pool, cfg);
		cfg = NULL;
	}

	return -1;
}

static int func_private_assign_dir_set_private_data(HttpStateData *httpState)
{
	mod_se_private_data *se_private_data=mod_se_private_data_pool_alloc();
    se_private_data->client_fd = httpState->fwd->client_fd;
	//se_private_data->client_fd = httpState->fwd->server_fd;
	debug(157, 3)("mod_assign_dir-> func_private_assign_dir_set_private_data, server_fd: %d\n",se_private_data->client_fd);
	cc_register_mod_private_data(STORE_ENTRY_PRIVATE_DATA,httpState->entry,se_private_data,mod_se_private_data_pool_free,mod);
	return 1;
}

static int storeDirValidSwapDirSize(int swapdir, squid_off_t objsize)
{
	/*  
	 * If the object size is -1, then if the storedir max_objsize isn't -1
	 * we can't store it (min_objsize intentionally ignored here)
	 */
	if (objsize == -1) 
		return Config.cacheSwap.swapDirs[swapdir].max_objsize == -1; 

	/*  
	 * Else, make sure that min_objsize <= objsize < max_objsize
	 */
	if ((Config.cacheSwap.swapDirs[swapdir].min_objsize <= objsize) &&
			((Config.cacheSwap.swapDirs[swapdir].max_objsize > objsize) ||
			 (Config.cacheSwap.swapDirs[swapdir].max_objsize == -1)))
		return 1;
	else
		return 0;
}

static int selectDirRoundRobin(const StoreEntry *e, const mod_assign_dir_cfg *cfg)
{
	int i;
	int load;
	SwapDir *sd;
	static int dirn = 0;
	squid_off_t objsize = objectLen(e);

	for (i = 0; i <= cfg->num; ++i)
	{
		if (++dirn >= cfg->num)
			dirn = 0;

		sd = &Config.cacheSwap.swapDirs[cfg->dirs[dirn].dirn];
		if (sd->flags.read_only)
		{
			debug(157, 2)("storeDirSelectSwapDirRoundRobin: sd read_only\n");
			continue;
		}
		if (sd->cur_size > sd->max_size)
		{
			debug(157, 2)("storeDirSelectSwapDirRoundRobin: sd cur_size > sd->max_size\n");
			continue;
		}
		if (!storeDirValidSwapDirSize(cfg->dirs[dirn].dirn, objsize))
		{
			debug(157, 2)("storeDirSelectSwapDirRoundRobin: not valid store dir\n");
			continue;
		}
		/* check for error or overload condition */
		if (sd->checkobj(sd, e) == 0)
		{
			debug(157, 2)("storeDirSelectSwapDirRoundRobin: checkobj(sd,e) == 0\n");
			continue;
		}
		load = sd->checkload(sd, ST_OP_CREATE);
#ifdef CC_FRAMEWORK
		int max_load = 1000; 
		max_load = cc_call_hook_func_storedir_set_max_load(max_load);
		debug(157, 3)("max_load is changed to %d\n", max_load);
		if (load < 0 || load > max_load)
#else
			if (load < 0 || load > 1000)
#endif
			{
				continue;
			}
		debug(157, 3)("mod_assign_dir: selected round-robin assign dir: %s\n",
				Config.cacheSwap.swapDirs[cfg->dirs[dirn].dirn].path);
		return cfg->dirs[dirn].dirn;
	}
	return -1;
}

static int selectDirLeastLoad(const StoreEntry *e, const mod_assign_dir_cfg *cfg)
{
	squid_off_t objsize;
	int most_free = 0, cur_free;
	squid_off_t least_objsize = -1;
	int least_load = INT_MAX;
	int load;
	int dirn = -1;
	int i;
	SwapDir *SD;

	/* Calculate the object size */
	objsize = objectLen(e);
	if (objsize != -1)
		objsize += e->mem_obj->swap_hdr_sz;

	for (i = 0; i < cfg->num; ++i)
	{
		SD = &Config.cacheSwap.swapDirs[cfg->dirs[i].dirn];
		SD->flags.selected = 0;
		if (SD->checkobj(SD, e) == 0)
		{
			continue;
		}
		load = SD->checkload(SD, ST_OP_CREATE);
		if (load < 0 || load > 1000)
		{
			continue;
		}
		if (!storeDirValidSwapDirSize(cfg->dirs[i].dirn, objsize))
			continue;
		if (SD->flags.read_only)
			continue;
		if (SD->cur_size > SD->max_size)
			continue;
		if (load > least_load)
			continue;
		cur_free = SD->max_size - SD->cur_size;
		/* If the load is equal, then look in more details */
		if (load == least_load)
		{
			/* closest max_objsize fit */
			if (least_objsize != -1)
				if (SD->max_objsize > least_objsize || SD->max_objsize == -1)
					continue;
			/* most free */
			if (cur_free < most_free)
				continue;
		}
		least_load = load;
		least_objsize = SD->max_objsize;
		most_free = cur_free;
		dirn = cfg->dirs[i].dirn;
	}
	if (dirn >= 0)
	{
		debug(157, 3)("mod_assign_dir: selected least_load assign dir: %s\n",
				Config.cacheSwap.swapDirs[dirn].path);
		Config.cacheSwap.swapDirs[dirn].flags.selected = 1;
	}
	return dirn;
}

static int dirSelectAlgorithm(const StoreEntry *e, mod_assign_dir_cfg *cfg)
{
	LOCAL_DIRSELECT * selectSwapDir = selectDirLeastLoad;

	if (0 == strcasecmp(Config.store_dir_select_algorithm, "round-robin"))
	{   
		selectSwapDir = selectDirRoundRobin;
		debug(157, 3) ("mod_assign_dir: Using Round Robin store dir selection\n");
	}   
	else
	{   
		selectSwapDir = selectDirLeastLoad;
		debug(157, 3) ("mod_assign_dir: Using Least Load store dir selection\n");
	} 

	return selectSwapDir(e, cfg);
}

static int func_private_assign_dir(StoreEntry* e)
{
	int fd;
	mod_assign_dir_cfg * cfg;
	mod_se_private_data * se_private_data;

	se_private_data = cc_get_mod_private_data(STORE_ENTRY_PRIVATE_DATA,e,mod);

	if(se_private_data == NULL)
	{
		return -1;
	}

	fd = se_private_data->client_fd;

	cfg = cc_get_mod_param(fd, mod);

	debug(157, 3)("mod_assign_dir: cc_run_state: %p, enable_or_not: %d\n",
			fd_table[fd].cc_run_state, fd_table[fd].cc_run_state[mod->slot]);

	if (NULL == cfg)
	{
		debug(157, 3)("mod_assign_dir: cc_get_mod_param() for FD %d is NULL\n", fd);
		return -1;
	}

	if (NULL != fd_table[fd].cc_run_state && fd_table[fd].cc_run_state[mod->slot])
	{
		return dirSelectAlgorithm(e, cfg);
	}

	return -1;
}

static int func_private_is_assign_dir(StoreEntry* e, int dirn)
{
	int i;


	/* dirn must in a valid range */
	if (dirn < 0 && dirn > Config.cacheSwap.n_configured)
	{
		debug(157, 3)("mod_assign_dir: dirn(%d) is not in cache dirs!\n", dirn);
		return -1; 
	}

	for (i = 0; i < g_cfg.num; ++i)
	{
        if (g_cfg.dirs[i].dirn == dirn)
		{
			// dirn is the specified dir, so don't use it in the common dirs
			debug(157, 3)("mod_assign_dir: dir '%s' is assign dir\n", g_cfg.dirs[i].path);
			return 1;
		}
	}

	debug(157, 3)("mod_assign_dir: dir '%s' is not assign dir.\n", Config.cacheSwap.swapDirs[dirn].path);

	return -1;
}

static void func_before_sys_init(void)
{
	int i, j, k;
	int swapdir_matched;
	mod_assign_dir_cfg * cfg;
	cc_mod_param * mod_param;

	debug(157, 1)("mod_assign_dir: before_sys_init handling ...\n");

    if (NULL != g_cfg.dirs)
    {
        for (i = 0; i < g_cfg.num; ++i)
        {
            safe_free(g_cfg.dirs[i].path);
        }
        safe_free(g_cfg.dirs);
        g_cfg.dirs = NULL;
    }
    g_cfg.num = 0;

	if (0 == mod->flags.param || 0 == mod->mod_params.count)
	{
		return;
	}

	for (k = 0; k < mod->mod_params.count; ++k)
	{
		mod_param = mod->mod_params.items[k];
		cfg = mod_param->param;

		if (NULL == cfg)
		{
			continue;
		}

		if (0 == cfg->dirn_init)
		{
			/* initialize the dirn with cache dirs index */
			for (i = 0; i < cfg->num; ++i)
			{
				swapdir_matched = 0;
				for (j = 0; j < Config.cacheSwap.n_configured; ++j)
				{
					if(!strncmp(Config.cacheSwap.swapDirs[j].path, cfg->dirs[i].path, strlen(cfg->dirs[i].path)))
					{
						swapdir_matched = 1;
						cfg->dirs[i].dirn = j;
                        g_cfg.dirs = xrealloc(g_cfg.dirs, g_cfg.num + 1);
                        g_cfg.dirs[g_cfg.num].dirn = j;
                        g_cfg.dirs[g_cfg.num].path = xstrdup(cfg->dirs[i].path);
                        g_cfg.num++;
						debug(157, 3)("mod_assign_dir: config dir %d is: %s; %s\n",
								j, Config.cacheSwap.swapDirs[j].path, cfg->dirs[i].path);
						break;
					}
				}
				if (0 == swapdir_matched)
				{
					debug(157, 1)("mod_assign_dir: Warning ==> assign dir [%s] is not a valid cache_dir, it will be removed from assgin dirs, please check it! perhaps you should restart the FC to create the new assign cache_dir, but be careful on this operation please.\n", cfg->dirs[i].path);
					memcpy(cfg->dirs + i, cfg->dirs + (i + 1), (cfg->num - (i + 1)) * sizeof(dir_path_t));
					cfg->num--;
					i--;
				}
			}
			cfg->dirn_init = 1;
		}
	}
}

static int func_sys_cleanup(void)
{
	debug(157,1)("mod_assign_dir-> cleanup module\n");

	if(NULL != mod_assign_dir_cfg_pool)
	{
		memPoolDestroy(mod_assign_dir_cfg_pool);
	}
	assignDirsfree();

	return 0;
}

int mod_register(cc_module *module)
{
	debug(157,1)("mod_assgin_dir -> init module\n)");
	strcpy(module->version, "7.0.R.16488.i686");

	cc_register_hook_handler(HPIDX_hook_func_before_sys_init,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_before_sys_init),
			func_before_sys_init);
	cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module,hook_func_sys_cleanup),
			func_sys_cleanup);
	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module,hook_func_sys_parse_param),
			func_sys_parse_param);
	cc_register_hook_handler(HPIDX_hook_func_private_assign_dir,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module,hook_func_private_assign_dir),
			func_private_assign_dir);
	cc_register_hook_handler(HPIDX_hook_func_private_is_assign_dir,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_is_assign_dir),
			func_private_is_assign_dir);
	cc_register_hook_handler(HPIDX_hook_func_private_assign_dir_set_private_data,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module,hook_func_private_assign_dir_set_private_data),
			func_private_assign_dir_set_private_data);

	// reconfigure
	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	return 0;
}
#endif

