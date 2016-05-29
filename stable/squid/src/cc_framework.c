#include <dlfcn.h>
#include "cc_framework.h"

#ifdef CC_FRAMEWORK
/*********************************************************
 * module manage
 *
 *********************************************************/
static MemPool * cc_module_pool = NULL; 
void * cc_module_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == cc_module_pool)
		cc_module_pool = memPoolCreate("cc_module", sizeof(cc_module));
	return obj = memPoolAlloc(cc_module_pool);
}

void * cc_run_state_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == cc_run_state_pool)
		cc_run_state_pool = memPoolCreate("fde_cc_run_state", Config.cc_all_mod_num*sizeof(short int));
	return obj = memPoolAlloc(cc_run_state_pool);
}

void * matched_mod_params_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == matched_mod_params_pool)
		matched_mod_params_pool = memPoolCreate("fde_matched_mod_params", Config.cc_all_mod_num*sizeof(Array));
	return obj = memPoolAlloc(matched_mod_params_pool);
}

	static inline void
free_acl_access(acl_access ** head)
{
	aclDestroyAccessList(head);
}

void cc_print_mod_ver(int flag)
{
	DIR *dp = NULL;
	struct  dirent *dirp = NULL;
	char buff[MAX_PATH_LEN] = {0};
	int len = 0;
	char mod_path[MAX_PATH_LEN] = {0};
	int (*mod_register)(cc_module *);
	void *lib = NULL;
	cc_module mod;


	/* get the full path of moduels */
	len = readlink("/proc/self/exe", buff, MAX_PATH_LEN); 
	if (len < 0 || len >= MAX_PATH_LEN) 
	{
		return; 
	}
	buff[len-5] = 0; 
	strcat(buff, "modules");

	/* get the num of modules */
	if (!(dp=opendir(buff)))
	{
		return;
	}
	while ((dirp=readdir(dp)))
	{
		if (strstr(dirp->d_name, CC_MOD_TAG) && strstr(dirp->d_name, ".so"))
		{
			memset(mod_path, 0, sizeof(mod_path));
			memset(&mod, 0, sizeof(mod));

			snprintf(mod_path, MAX_PATH_LEN, "%s/%s", buff, dirp->d_name);
			if (!(lib = dlopen(mod_path, RTLD_NOW|RTLD_GLOBAL)))
			{
				printf("cc_print_mod_ver: dlopen module %s err %s\n", dirp->d_name, dlerror());
				break;
			}
			else
			{
				mod_register = dlsym(lib, "mod_register");
				if (dlerror())
				{
					printf("cc_print_mod_ver: dlsym module %s err %s\n", dirp->d_name, dlerror());
					break;
				}
				else 
				{
					mod_register(&mod);
				}
				if (flag)       // squid -V
					printf("%s version: %s\n", dirp->d_name, mod.version);
				else            // squid -v
				{
					char *version = xstrdup(mod.version);
					int len = strlen(version) - 1;
					int i = 0;
					while (version[len])
					{
						if (version[len] == '.')
							i += 1;
						if (i == 2)
						{
							version[len] = '\0';
							break;
						}
						len -= 1;
					}
					printf("%s version: %s\n", dirp->d_name, version ? version : mod.version);
					xfree(version);
				}
				dlclose(lib);
			}
		}
	}

	closedir(dp);
}

/* dlclose lib of module,  xfree mod_param and global variable:cc_modules\cc_old_mod_params */
void cc_modules_cleanup()
{
	int i, j;
	cc_module *mod = NULL;
	mod_domain_access *cur_domain_access, *mda;

	/* xfree cc_old_mod_param */
	debug(90, 1)("cc_modules_cleanup: free cc_modules\n");

	/* xfree cc_modules*/
	for(i = 0; i < cc_modules.count; i++)
	{
		mod = cc_modules.items[i];
		if (0 == mod->flags.enable) 
			continue;

		for (j = 0; mod->flags.param && j < mod->mod_params.count; j++)
			free_one_mod_param(mod->mod_params.items[j]);
		arrayClean(&mod->mod_params);

		if (mod->lib) 
		{
			dlclose(mod->lib);
			mod->lib = NULL;
		}

		if (mod->flags.config_on) 
			continue;

		if (1 == mod->has_acl_rules) 
			free_acl_access(&mod->acl_rules);

		for (j = 0; j < 100; j++) 
		{    
			cur_domain_access = mod->domain_access[j];
			while (cur_domain_access)
			{    
				mda = cur_domain_access->next;
				free_acl_access(&cur_domain_access->acl_rules);
				safe_free(cur_domain_access->mod_params);
				safe_free(cur_domain_access->domain);
				safe_free(cur_domain_access); 
				cur_domain_access = mda; 
			}    
		}    
	}

	cc_call_hook_func_sys_cleanup();

	for(i=0; i<cc_modules.count; i++) 
	{    
		memPoolFree(cc_module_pool, cc_modules.items[i]);
	}  
	arrayClean(&cc_modules);
	/* xfree cc_modules end */

	/* wo'nt execute hook func*/
	if(NULL != cc_module_pool)
		memPoolDestroy(cc_module_pool);
	if(NULL != cc_mod_param_pool)
		memPoolDestroy(cc_mod_param_pool);
}


// create cc_modules, scan modules then set cc_modules's name
int cc_create_modules()
{
	DIR *dp = NULL;
	struct  dirent *dirp = NULL;
	char buff[MAX_PATH_LEN] = {0};
	cc_module *mod = NULL;
	cc_module *mod_r = NULL;
	int len = 0;
	int mod_num = 0;

	len = readlink("/proc/self/exe", buff, MAX_PATH_LEN);
	if (len < 0 || len >= MAX_PATH_LEN)
	{
		debug(90, 0) ("cc_create_modules: squid path is large then MAX_PATH_LEN\n");
		return -1;
	}
	buff[len-5] = 0;        // strlen(squid) == 5
	strcat(buff, "modules");

	xstrncpy(Config.cc_mod_dir, buff, MAX_PATH_LEN);
	xstrncpy(Config_r.cc_mod_dir,buff,MAX_PATH_LEN);
	debug(90, 2) ("cc_create_modules: cc_mod_dir = %s\n", Config.cc_mod_dir);

	if (!(dp=opendir(buff)))
	{
		debug(90, 0) ("cc_create_modules: can't opendir %s\n", buff);
		return -1;
	}
	arrayInit(&cc_modules);
	arrayInit(&cc_modules_r);
	while ((dirp=readdir(dp)))
	{
		if (strstr(dirp->d_name, CC_MOD_TAG) && strstr(dirp->d_name, ".so"))
		{
			// dlopen all modules
			mod = cc_module_pool_alloc();
			mod_r = cc_module_pool_alloc();
			arrayAppend(&cc_modules, mod);
			arrayAppend(&cc_modules_r, mod_r);
			strncpy(mod->name, dirp->d_name, strlen(dirp->d_name)-3);
			strcpy(mod_r->name, mod->name);
			mod_r->slot = mod->slot = mod_num;
			mod_num++;

		}
	}
	closedir(dp);

	Config.cc_all_mod_num = mod_num;
	Config_r.cc_all_mod_num = mod_num;
	return mod_num;
}


/*********************************************************
 * recofigure support
 *
 *********************************************************/
void cc_set_modules_configuring_r()
{
	int i, j;
	cc_module *mod = NULL;

	for (i=0; i<cc_modules_r.count; i++) 
	{    
		mod = cc_modules_r.items[i];
		mod->flags.init = ((cc_module*)cc_modules.items[mod->slot])->flags.init;
		/* only reset about configed*/
		mod->flags.configed = 0; 
		mod->flags.enable = 0; 
		mod->flags.param = 0; 
		mod->flags.config_on = 0; 
		mod->flags.config_off = 0; 
		mod->flags.c_check_reply = 0; 

		mod->has_acl_rules = 0; 
		mod->acl_rules = NULL;
		mod->lib = NULL;
		mod->cc_mod_param_call_back = NULL;
		arrayInit(&mod->mod_params);
		for (j = 0; j < 100; j++) mod->domain_access[j] = NULL;
		memset( ADDR_OF_HOOK_FUNC(mod,hook_func_sys_init), 0, HPIDX_last* sizeof(void*) );
	}    
}

void free_one_mod_param(cc_mod_param* mod_param)
{
	debug(90,2)("free %s: cc_mod_param->param=%p,cc_mod_param->call_back=%p\n", 
			mod_param->acl_name, mod_param->param, mod_param->call_back);
	if (mod_param->call_back && mod_param->param)  
		mod_param->call_back(mod_param->param);  
	mod_param->call_back = NULL;
	mod_param->param = NULL;
	if (cc_mod_param_pool)
		memPoolFree(cc_mod_param_pool, mod_param);        // free cc_mod_param self
}

static void fd_in_using(cc_module *mod, cc_mod_param *mod_param)
{
	cc_module *mod_r = cc_modules_r.items[mod->slot];
	mod_param->call_back = mod_r->cc_mod_param_call_back;
	/* If There is a module be deleted after reload, 
	   this may introduce memory leak.
	   Take it into your consideration!
	 */
	if (NULL == mod_param->call_back)
	{
		debug(90,1)("fd_in_using: memroty leak here\n");
		return;
	}
	dlink_node *new_node = dlinkNodeNew();
	dlinkAdd(mod_param, new_node, &unfree_fd_param);
}

void cc_free_mod_param_r()
{
	debug(90,2)("cc_free_mod_param_r: try to free cc_mod_param for closed fd\n");
	int i, none_flag = 0;
	cc_mod_param *mod_param;
	dlink_node *current, *next;

	current = unfree_fd_param.head;
	while (current)
	{
		next = current->next;
		mod_param = (cc_mod_param*)current->data;
		if (mod_param->count > 0)       // still using, try agian at next eventRun
		{
			current = next;
			none_flag++;
			continue;               // skip current node
		}

		// no fd use this mod_param, let's free it
		free_one_mod_param(mod_param);
		dlinkDelete(current, &unfree_fd_param);
		dlinkNodeDelete(current);
		current = next;
	}

	if (none_flag)
	{
		debug(90,2)("cc_free_mod_param: There are still %d cc_mod_param not be freed\n", none_flag);
		eventAdd("cc_free_mod_param", cc_free_mod_param_r, NULL, 6, 1); 
	}
	else
	{
		debug(90,1)("cc_free_mod_param: all fd closed since last reload\n", none_flag);
		memset(&unfree_fd_param, 0, sizeof(dlink_list));     // all mod_param have been freed now
	}
}

static void cc_clear_unfree_fd_param()
{
	if (unfree_fd_param.head)
		debug(90,1)("cc_clear_unfree_fd_param: something wrong !\n");
	cc_mod_param *mod_param;
	dlink_node *current, *next;
	int i;

	current = unfree_fd_param.head;
	while (current)
	{
		next = current->next;
		mod_param = (cc_mod_param*)current->data;
		free_one_mod_param(mod_param);
		dlinkDelete(current, &unfree_fd_param);
		dlinkNodeDelete(current);
		current = next;
	}
	memset(&unfree_fd_param, 0, sizeof(dlink_list));     // all mod_param have been freed now

}

/* 
 * this routine may consume cpu resource, 
 * take it into your consideration ! ----chenqi
 * */
void cc_set_modules_configuring()
{
	int i, j;
	cc_module *mod = NULL;
	cc_mod_param *mod_param = NULL;
	mod_domain_access *cur_domain_access = NULL;
	mod_domain_access *mda = NULL;

	//cc_clear_unfree_fd_param(); // this is some bug

	for (i=0; i<cc_modules.count; i++)
	{
		mod = cc_modules.items[i];
		if (0 == mod->flags.enable)
			continue;

		debug(90,2)("cc_set_modules_configure: %d/%d --> free module %s from cc_modules\n", i, cc_modules.count, mod->name);

		// free mod->mod_params
		debug(90,2)("cc_set_modules_configure: free mod->mod_params of %s\n", mod->name);
		for (j = 0; mod->flags.param && j < mod->mod_params.count; j++)  
		{    
			mod_param = mod->mod_params.items[j];
			if (mod_param->count > 0)
				fd_in_using(mod,mod_param);
			else
				free_one_mod_param(mod_param);
		}    
		arrayClean(&mod->mod_params);
		arrayInit(&mod->mod_params);

		//  mod_xxx on
		if (mod->flags.config_on)
			continue;

		// mod_xxx allow/deny
		// free mod->acl_rulest
		if (1 == mod->has_acl_rules)
		{
			debug(90,2)("cc_set_modules_configure: free mod->acl_rules of %s\n", mod->name);
			free_acl_access(&mod->acl_rules);
		}

		// free mod->domain_access
		debug(90,2)("cc_set_modules_configure: free mod->domain_access of %s\n", mod->name);
		for (j = 0; j < 100; j++)
		{
			cur_domain_access = mod->domain_access[j];
			while (cur_domain_access)
			{
				mda = cur_domain_access->next;
				free_acl_access(&cur_domain_access->acl_rules);
				safe_free(cur_domain_access->mod_params);
				safe_free(cur_domain_access->domain);
				safe_free(cur_domain_access);   // this must be at last
				cur_domain_access = mda;
			}
		}
		dlclose(mod->lib);
	}
}

void set_cc_effective_modules()
{
	// clear cc_effective_modules
	arrayClean(&cc_effective_modules);
	arrayInit(&cc_effective_modules);
	debug(90,2)("set_cc_effective_modules: clear cc_effective_modules\n");

	// reset cc_effective_modules
	int i;
	cc_module *mod = NULL;
	for (i=0; i<cc_modules.count; i++) 
	{    
		mod = cc_modules.items[i];
		if (1 == mod->flags.enable)
		{    
			arrayAppend(&cc_effective_modules, mod);
			debug(90,2)("set_cc_effective_modules: add mod %s to cc_effective_modules\n",mod->name);
		}    
	}    
}

/*********************************************************/


void cc_copy_acl_access(acl_access *src, acl_access *dst)
{
	assert(dst && src);
	/***********************************************************/
	/***********************************************************/

	acl_list *L = NULL;
	acl_list **Tail = &dst->acl_list;
	dst->allow = src->allow;
	assert(src->cfgline);
	dst->cfgline = xstrdup(src->cfgline);

	acl_list *L2 = NULL;
	for(L2 = src->acl_list; L2; L2=L2->next)
	{
		L = memAllocate(MEM_ACL_LIST);
		L->op = L2->op;
		L->acl = L2->acl;
		*Tail = L;
		Tail = &L->next;

	}

}

/*********************************************************
 * support acl and para
 *
 *********************************************************/
/* parse mod's acl to cc_mod_acls*/
const char *cc_private_parse_acl_access(const char *cfg_line, cc_module *mod)
{
	char *t = NULL;
	static const char *reason = NULL;
	reason = NULL;
	acl_access *A = NULL;
	acl_access *B = NULL;
	acl_access **T = NULL;
	acl_access ** head = &mod->acl_rules;
	int found = 0;
	int allow = 0;
	char *tmp = xstrdup(cfg_line);

	if (!strtok(tmp, w_space))
	{
		reason = "white_space line";
		goto err;
	}

	/* first expect either 'allow' or 'deny' */
	while ((t = strtok(NULL, w_space)))
	{
		if (0==strcmp(t, "allow"))
		{
			allow = 1;
			found = 1;
			break;
		}
		else if (0==strcmp(t, "deny"))
		{
			allow = 0;	
			found = 1;
			break;
		}
	}

	if (0==found)
	{
		debug(90, 2) ("cc_private_parse_acl_access: mod havn't acl config_input_line=%s\n", config_input_line);
		reason = "Not Found allow or deny";
		goto err;
	}

	A = cbdataAlloc(acl_access);
	A->allow = allow;

	aclParseAclList(&A->acl_list);
	if (!A->acl_list)
	{
		debug(90, 2) ("cc_private_parse_acl_access: %s line %d: %s\n", cfg_filename, config_lineno, config_input_line);
		debug(90, 2) ("cc_private_parse_acl_access: Access line contains no ACL's, skipping\n");
		cbdataFree(A);
		reason = "Not Found ACL_NAME";
		goto err;
	}
	A->cfgline = xstrdup(cfg_line);
	/* Append to the end of this list */
	for (B = *head, T = head; B; T = &B->next, B = B->next);
	*T = A;

	safe_free(tmp);
	tmp = NULL;
	/* added for rcms */
	mod_domain_access *cur = NULL;
	if(reconfigure_in_thread)
		cur = cc_get_domain_access(Config_r.current_domain, mod);
	else
		cur = cc_get_domain_access(Config.current_domain, mod);
	assert(cur);
	acl_access *C = cbdataAlloc(acl_access);
	cc_copy_acl_access(A,C);
	head = &cur->acl_rules;

	for(B = *head, T=head; B; T=&B->next, B=B->next);
	*T=C;

	/* added end */
	mod->has_acl_rules = 1;
	return NULL;

err:
	safe_free(tmp);
	tmp = NULL;
	return reason;
}

mod_domain_access *cc_get_domain_access(char* domain, cc_module *mod)
{
	int slot;
	slot = myHash(domain,100);
	debug(90,2)("the domain_access slot is: %d\n",slot);

	mod_domain_access *cur = mod->domain_access[slot];
	while(cur)
	{
		if(strcmp(cur->domain, domain) == 0)
		{
			debug(90,2)("cc_get_domain_access -->find the proper mod_domain_access: %p\n",cur);
			return cur;
		}
		cur = cur->next;
	}
    return NULL;
}

void cc_insert_domain_access(mod_domain_access *cur, cc_module *mod)
{
	int slot;
	assert(cur);
    slot = myHash(cur->domain,100);
    debug(90,2)(" cc_insert_domain_access : the slot is %d\n",slot);
    cur->next = mod->domain_access[slot];
    mod->domain_access[slot] = cur;
}

/* parse tmp_line to mod para*/
void cc_parse_param_and_acl(char *tmp_line, cc_module *mod)
{
	char *new_tmp_line = xstrdup(tmp_line);
	int param_count = mod->mod_params.count;
	int result =  0;
	const char *reason = NULL;

	/* configed on */
	if (mod->flags.config_on)
	{    
		/* if have mod para */
		if (mod->flags.param) 
		{
			assert(0 == mod->mod_params.count);
			result = mod->hook_func_sys_parse_param(new_tmp_line); // sucess if return 0
		}
	}    

	else if (strstr(tmp_line, " allow ") || strstr(tmp_line, " deny "))/* configed have acl */
	{    
		/*added for rcms */
		mod_domain_access *cur_domain_access ;
		if (reconfigure_in_thread)
			cur_domain_access = cc_get_domain_access(Config_r.current_domain,mod);
		else 
			cur_domain_access = cc_get_domain_access(Config.current_domain,mod);
		if (cur_domain_access == NULL)
		{    
			cur_domain_access = xcalloc(1,sizeof(mod_domain_access));
			if (reconfigure_in_thread)
				cur_domain_access->domain = xstrdup(Config_r.current_domain);
			else 
				cur_domain_access->domain = xstrdup(Config.current_domain);

			debug(90,2)("cc_parse_param_and_acl : create domain_access %s @ %p\n",cur_domain_access->domain, cur_domain_access);

			cc_insert_domain_access(cur_domain_access, mod);
		}    
		/* added end*/

		if (0 == mod->flags.param)      // no param, just parse acl
			reason = cc_private_parse_acl_access(tmp_line, mod);
		else 
		{    
			/* parse param success  */
			if (0 == (result = mod->hook_func_sys_parse_param(new_tmp_line)))
			{
				cc_mod_param *mod_param = NULL;
				mod_param = mod->mod_params.items[mod->mod_params.count-1];
				strcpy(mod_param->acl_name, tmp_line);
				if (NULL == (reason = cc_private_parse_acl_access(tmp_line, mod)))     /* parse acl success */
				{
					/* if module have para then malloc new mod_para and init */
					if (0 == cur_domain_access->param_num)
						cur_domain_access->mod_params = xcalloc(1,sizeof(cc_mod_param*));
					else
						cur_domain_access->mod_params = xrealloc(cur_domain_access->mod_params, (1 + cur_domain_access->param_num) * sizeof(cc_mod_param*));

					cur_domain_access->mod_params[cur_domain_access->param_num++] = mod_param;
					debug(90,2)("cc_parse_param_and_acl: %s cur->param_num = %d\n", mod->name, cur_domain_access->param_num);
				}
				else /* parse acl failed */
				{
					debug(90, 1) ("cc_parse_param_and_acl:  %s acl parse err %s\n", mod->name, tmp_line);
					memPoolFree(cc_mod_param_pool, mod_param);
					mod->mod_params.count--;
				}
			}
			else /* parse param failed */
			{
				mod->flags.enable = 0;
				debug(90, 1) ("cc_parse_param_and_acl:  %s param parse err %s\n", mod->name, tmp_line);
			}
		}
	}

	safe_free(new_tmp_line);
	new_tmp_line = NULL;

	//error
	if (mod->flags.param && 0 == mod->mod_params.count)
	{
		mod->flags.enable = 0;
		debug(90, 1) ("cc_parse_param_and_acl:  param exist but param_num=0\n");
	}

	if(NULL == reason && (mod->flags.param && param_count +1 != mod->mod_params.count))
	{
		if (NULL == strstr(tmp_line, " allow") && NULL == strstr(tmp_line, " deny"))
			reason = "Not found Switch";
		else if(result)
			reason = "Param parse Error";
		else 
			reason = "Not found acl_name";
		cc_framework_check_start(reason);
	}
}


char* cc_removeTabAndBlank(char* tmp_line)
{
	char* start = NULL;
	char* loop = tmp_line;
	char* last = NULL;
	int haveChar = 0; //0 for never meet Char
	int i,len = strlen(tmp_line);

	if(len > BUFSIZ)
	{
		debug(90, 0) ("cc_load_modules: a too much long line config [%s]\n", tmp_line);
		return NULL;
	}

	for(i = 0;*loop != '\0' && i < len; i++, loop++) 
	{       
		if(!haveChar && (*loop != ' ' && *loop != '\t'))
		{       
			haveChar = 1;
			start = loop;
			last = loop;
			continue;
		}       

		if(*loop == '\t')
		{       
			*loop = ' ';
		}       

		if(*loop != '\t' && *loop != ' ') 
		{       
			last = loop; 
		}       
	}

	if(start != last)	
		*(last+1) = '\0'; 

	return start;	
}


/*
 *Set Default func that default to open. and set the configure to on
 */

void cc_default_load_modules()
{
	char* def_mod_open_name[] = 
	{
		"mod_disable_sendfile",
		"mod_disable_vary",
		"mod_use_server_date",
		"mod_refresh_pattern",
		"mod_do_not_unlink",
		"mod_client_304_expires",
		"mod_helper_defer",
		"mod_multisquid_lscs",
		NULL
	};

	int i;
	char default_line[MAX_NAME_LEN+5] = {0};
	for (i = 0; def_mod_open_name[i]; i++)
	{
		memset(default_line, 0, MAX_NAME_LEN+5);
		snprintf(default_line, MAX_NAME_LEN+5, "%s on", def_mod_open_name[i]);	
		cc_load_modules(default_line);
	}
}

void cc_load_modules(char *tmp_line)
{
	int i = 0; 
	cc_module *mod = NULL;
	char mod_name_tag[MAX_NAME_LEN] = {0}; 
	int configed_module_appeared_in_disk = 0; 

	for (i=0; i<Config.cc_all_mod_num; i++) 
	{    
		if(reconfigure_in_thread)
			mod = cc_modules_r.items[i];
		else 
			mod = cc_modules.items[i];

		memset(mod_name_tag, 0, MAX_NAME_LEN);
		snprintf(mod_name_tag, MAX_NAME_LEN, "%s ", mod->name);
		if (strstr(tmp_line, mod_name_tag))
		{    
			if (mod->flags.config_on || mod->flags.config_off)      // only 1st config line would be used
			{
				debug(90,1)("cc_load_modules: ignore %s, because that %s has been configured before\n", tmp_line,mod->name);
				return;
			}

			configed_module_appeared_in_disk = 1; 
			mod->flags.configed = 1; 
			if (!mod->lib)
			{    
				int (*mod_register)(cc_module *);

				char mod_path[MAX_PATH_LEN] = {0}; 
				snprintf(mod_path, MAX_PATH_LEN, "%s/%s.so", Config.cc_mod_dir, mod->name);
				if (!(mod->lib = dlopen(mod_path, RTLD_NOW|RTLD_GLOBAL)))
				{    
					debug(90, 0) ("cc_load_modules: dlopen module %s err %s\n", mod->name, dlerror());
					return;
				}    

				debug(90, 1) ("cc_modules_init:  dlopen success mod %s\n", mod->name);
				if (!(mod_register = dlsym(mod->lib, "mod_register")))
				{
					debug(90, 0) ("cc_load_modules: dlsym module %s err %s\n", mod->name, dlerror());
					return;
				}
				mod_register(mod);

				if (mod->lib)
					mod->flags.enable = 1;
				if (mod->hook_func_sys_parse_param)
					mod->flags.param = 1;

				char *matchstr = NULL;
				if(strstr(tmp_line, " on"))
				{
					matchstr = tmp_line + strlen(tmp_line) - 3;
					if(strcasecmp(matchstr," on") == 0)
						mod->flags.config_on = 1;
				}
				else if (strstr(tmp_line, " off"))
				{
					matchstr = tmp_line + strlen(tmp_line) - 4;
					if(strcasecmp(matchstr," off") == 0)
					{
						mod->flags.config_off = 1;
						mod->flags.enable = 0;
						if (dlclose(mod->lib))
							debug(90, 1)("cc_load_modules: dlclose %s failed because %s\n", mod->name,dlerror());
						mod->lib = NULL;
					}
				}

				debug(90, 1) ("cc_load_modules: load new module '%s'(slot=%d) to cc_modules_r\n", mod->name,mod->slot);
			}


			if (strstr(tmp_line, REPLY_CHECK))
				mod->flags.c_check_reply = 1;
			else
				mod->flags.c_check_reply = 0;

			if (mod->flags.enable)      // add this condition
				cc_parse_param_and_acl(tmp_line, mod);
			break;
		}
	}

	if(!configed_module_appeared_in_disk)
	{
		cc_framework_check_start("Not appeared in disk");
		debug(90, 0)("Warning: The config line [%s] not appeared in disk[%s]!\n", tmp_line, Config.cc_mod_dir);
	}
}


/* acl check and return cfgline if matched*/
int cc_aclCheckFastRequest(aclCheck_t *ch, cc_module *mod, cc_acl_check_result *result)
{
	int param_pos = 0;
	int answer = 0;
	//acl_access *A = mod->acl_rules;
	char* host = ch->request->host;
	mod_domain_access *cur = cc_get_domain_access(host,mod);
	acl_access *A;
	if(cur == NULL)
	{
		debug(90,2)("cc_aclCheckFastRequest : %s has no domain config, we use common domain\n",host);
		cur = cc_get_domain_access("common",mod);
	}
	assert(cur);
	A = cur->acl_rules;
#ifdef CC_FRAMEWORK
	int ret = cc_call_hook_func_private_before_loop(NULL,ch,A,result,0,mod);
	if(ret ==1)
	{
		return result->param_count > 0;
	}
#endif
	while (A)
	{
		answer = aclMatchAclList(A->acl_list, ch);
		if (answer > 0 && A->allow == ACCESS_ALLOWED)
		{
			/* matched acl then set the acl_run*/
			debug(90, 3) ("cc_aclCheckFastRequest: fd match A and A->cfgline = %s\n", A->cfgline);
			result->param_poses = xrealloc(result->param_poses, (++result->param_count) * sizeof(int));
			result->param_poses[result->param_count - 1] = param_pos;
		}
		param_pos++;
		A = A->next;
	}

#ifdef CC_FRAMEWORK
	//	cc_call_hook_func_private_cc_aclCheckFastRequestDone(ch,mod, mod->acl_rules, result,0);
	cc_call_hook_func_private_cc_aclCheckFastRequestDone(ch,mod, cur->acl_rules, result,0);

#endif
	return result->param_count > 0;
}


/*  client acl check the request from client to fc, then set cc_req_run
 *  type = 0 is request check
 *  type = 1 is reply check
 */
void cc_client_acl_check(clientHttpRequest *http, int type)
{
	int i, j;
	int fd = http->conn->fd;
	char *host = http->request->host;
	cc_module *mod = NULL;
	cc_mod_param *mod_param = NULL;
	aclCheck_t ch;
	memset(&ch, 0, sizeof(ch));
	ch.request = http->request;
	if (1==type)
		ch.reply = http->reply;
	aclChecklistCacheInit(&ch);

	if (NULL == fd_table[fd].cc_run_state)
		fd_table[fd].cc_run_state = cc_run_state_pool_alloc();
	else if (0==type)
		memset(fd_table[fd].cc_run_state, 0, Config.cc_all_mod_num*sizeof(short int));

	if (NULL == fd_table[fd].matched_mod_params)	
		fd_table[fd].matched_mod_params = matched_mod_params_pool_alloc();	
	else if (0 == type)
	{
		for (i = 0; i < Config.cc_all_mod_num; i++) 
		{     
			for (j = 0; j < fd_table[fd].matched_mod_params[i].count; j++) 
			{    
				mod_param = fd_table[fd].matched_mod_params[i].items[j];
				mod_param->count--;   
			}    
			arrayClean(&fd_table[fd].matched_mod_params[i]);
			arrayInit(&fd_table[fd].matched_mod_params[i]);
		}  
	}

	for (i=0; i<cc_effective_modules.count; i++) 
	{    
		mod = cc_effective_modules.items[i];
		assert(mod->flags.enable);
		assert(!(mod->flags.param && 0 == mod->mod_params.count));
		if (mod->flags.c_check_reply ^ type) continue;

		debug(90, 3) ("cc_client_acl_check %d: modname =%s  param=%d c_check_reply=%d\n", type, mod->name, mod->flags.param, mod->flags.c_check_reply);

		if (mod->flags.config_on)
		{    
			fd_table[fd].cc_run_state[mod->slot] = 1; 
			debug(90,3)("cc_client_acl_check: fd=%d has matched [%s on]\n", fd, mod->name);
			if (mod->flags.param)
			{    
				mod_param = mod->mod_params.items[0];
				arrayAppend(&fd_table[fd].matched_mod_params[mod->slot], mod_param);
				mod_param->count++;
				debug(90,2)("cc_client_acl_check: fd[%d].matched_mod_params[%d][0]===== %p, param ===%p, call_back == %p\n",fd,mod->slot,mod_param,mod_param->param,mod_param->call_back);
			}    
			continue;
		}    

		mod_domain_access *mda = cc_get_domain_access(host,mod);
		if (mda == NULL)
		{    
			debug(90,2)("cc_client_acl_check: fd=%d has matched domain %s [common]\n", fd, mod->name);
			mda = cc_get_domain_access("common",mod);
		}    
		else 
			debug(90,2)("cc_client_acl_check: fd=%d has matched domain %s [%s]\n", fd, mod->name, host);
		assert(mda && mod->has_acl_rules);

		acl_access *A;
		A = mda->acl_rules;
		int answer = 0;
		int pos = 0;
		while (A)
		{   
			answer = aclMatchAclList(A->acl_list, &ch);
			if (answer > 0 && A->allow == ACCESS_ALLOWED)
			{
				fd_table[fd].cc_run_state[mod->slot] = 1;
				if (0 == mod->flags.param || pos >= mda->param_num) break;
				mod_param = mda->mod_params[pos];
				arrayAppend(&fd_table[fd].matched_mod_params[mod->slot], mod_param);
				mod_param->count++;
				debug(90,2)("cc_client_acl_check: fd[%d].matched_mod_params[%d][%d]===== %p, param ===%p, call_back == %p\n",fd,mod->slot,pos,mod_param,mod_param->param,mod_param->call_back);
                // mod_header should support multiple config line
				// if (strncmp(mod->name,"mod_header",strlen("mod_header"))) break;
				if (NULL == strstr(mod->name,"header")) break;
			}
			pos++;
			A = A->next;
		}   
	}

	aclCheckCleanup(&ch);
}


// client ==> server
void cc_copy_fd_params(fde *client, fde* server, int is_server_fd)
{
	int j, k;
	cc_mod_param *mod_param;

	if (NULL == server->cc_run_state)
		server->cc_run_state = (short int*)cc_run_state_pool_alloc();

	if (client->cc_run_state)
		memcpy(server->cc_run_state, client->cc_run_state, Config.cc_all_mod_num*sizeof(short int));

	if (NULL == server->matched_mod_params) 
		server->matched_mod_params = (Array*)matched_mod_params_pool_alloc();
	else if (is_server_fd)   // server fd is persistent connection
	{
		for (j=0; j<Config.cc_all_mod_num; j++)
		{
			for (k=0; k<server->matched_mod_params[j].count; k++)
			{
				mod_param = server->matched_mod_params[j].items[k];
				mod_param->count--;
			}
			arrayClean(&server->matched_mod_params[j]);
			arrayInit(&server->matched_mod_params[j]);
		}
	}

	if (client->matched_mod_params)
	{
		for (j=0; j<Config.cc_all_mod_num; j++)
		{
			for (k=0; k<client->matched_mod_params[j].count; k++)
			{
				mod_param = client->matched_mod_params[j].items[k];
				arrayAppend(&server->matched_mod_params[j], mod_param);
				mod_param->count++;
			}
		}
	}

	if (is_server_fd)
		server->cctype = 1; //zds add at 2009-11-18 for distinct the client or server fd : 0 == client 1 == server
}

/*********************************************************/

void cc_init_hook_to_module()
{
	int i;
	for(i = 0; i < HPIDX_last ; i ++)
	{
		arrayInit(&cc_hook_to_module[i]);
		arrayInit(&cc_hook_to_module_r[i]);
	}
}

void cc_set_hook_to_module_r()
{
	int i;
	Array *a;
	for(i = 0; i < HPIDX_last ; i ++)
	{
		a = &cc_hook_to_module_r[i];
		arrayClean(a);
		arrayInit(a);
	}
}

void cc_destroy_hook_to_module()
{
	int i, j;
	Array *a;
	for(i = 0; i < HPIDX_last ; i++)
	{
		a = &cc_hook_to_module[i];
		for(j = 0; j < a->count; j ++)
			safe_free(a->items[j]);     // int*
		arrayClean(a);
		arrayInit(a);
	}
}
void cc_destroy_hook_to_module_r()
{
	int i, j;
	Array *a;
	for(i = 0; i < HPIDX_last ; i++)
	{
		a = &cc_hook_to_module_r[i];
		for(j = 0; j < a->count; j ++)
			safe_free(a->items[j]);
		arrayClean(a);
		arrayInit(a);
	}
}

cc_module * cc_get_next_module_by_hook(HPIDX_t hook_point_index, int *mod_index)
{
	int index = *mod_index;
	Array *mod_arr = &cc_hook_to_module[hook_point_index];
	if(index >= mod_arr->count)
	{
		return NULL;
	}
	*mod_index = index + 1;
	return cc_modules.items[*(int *)(mod_arr->items[index])];
}
/****added for optimize_for_rcms by jiangbo.tian******/

/*
 *myHash: a hash function found in google
 */
unsigned int myHash(void *value,unsigned int size)
{
	char* str = (char*)(value);
	unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
	unsigned int hash = 0;

	while (*str)
	{
		hash = hash * seed + (*str++);
	}

	return (hash & 0x7FFFFFFF)%size;
}

acl_access* locateAccessType(char* token,access_array* array_p)
{
	debug(90,2)("locateAccessType: %s , %p\n",token,array_p);
	if (!strcmp(token, "http_access"))
	{
		return (array_p->http);
	}
	else if (!strcmp(token, "http_access2"))
		return (array_p->http2);
	else if (!strcmp(token, "http_reply_access"))
		return (array_p->reply);
	else if (!strcmp(token, "icp_access"))
		return (array_p->icp);
#if USE_HTCP
	else if (!strcmp(token, "htcp_access"))
		return (array_p->htcp);
#endif
#if USE_HTCP
	else if (!strcmp(token, "htcp_clr_access"))
		return (array_p->htcp_clr);
#endif
	else if (!strcmp(token, "miss_access"))
		return (array_p->miss);
#if USE_IDENT
	else if (!strcmp(token, "ident_lookup_access"))
		return (array_p->identLookup);
#endif
#if FOLLOW_X_FORWARDED_FOR
	else if (!strcmp(token, "follow_x_forwarded_for"))
		return (array_p->followXFF);
#endif
	else if (!strcmp(token, "log_access"))
		return (array_p->log);
	else if (!strcmp(token, "url_rewrite_access"))
		return (array_p->url_rewrite);
	else if (!strcmp(token, "redirector_access"))
		return (array_p->url_rewrite);
	else if (!strcmp(token, "storeurl_access"))
		return (array_p->storeurl_rewrite);
	else if (!strcmp(token, "location_rewrite_access"))
		return (array_p->location_rewrite);
	else if (!strcmp(token, "cache"))
		return (array_p->noCache);
	else if (!strcmp(token, "no_cache"))
		return (array_p->noCache);
	else if (!strcmp(token, "broken_posts"))
		return (array_p->brokenPosts);
	else if (!strcmp(token, "upgrade_http0.9"))
		return (array_p->upgrade_http09);
	else if (!strcmp(token, "broken_vary_encoding"))
		return (array_p->vary_encoding);
#if SQUID_SNMP
	else if (!strcmp(token, "snmp_access"))
		return (array_p->snmp);
#endif
	else if (!strcmp(token, "always_direct"))
		return (array_p->AlwaysDirect);
	else if (!strcmp(token, "never_direct"))
		return (array_p->NeverDirect);
	//else return NULL;
}

void cc_setHead(char* token,access_array *array_p,acl_access *** head)
{
	debug(90,2)("cc_setHead: %s , %p\n",token,array_p);
	if (!strcmp(token, "http_access"))
		*head = (&array_p->http);
	else if (!strcmp(token, "http_access2"))
		*head = (&array_p->http2);
	else if (!strcmp(token, "http_reply_access"))
		*head = (&array_p->reply);
	else if (!strcmp(token, "icp_access"))
		*head = (&array_p->icp);
#if USE_HTCP
	else if (!strcmp(token, "htcp_access"))
		*head = (&array_p->htcp);
#endif
#if USE_HTCP
	else if (!strcmp(token, "htcp_clr_access"))
		*head = (&array_p->htcp_clr);
#endif
	else if (!strcmp(token, "miss_access"))
		*head = (&array_p->miss);
#if USE_IDENT
	else if (!strcmp(token, "ident_lookup_access"))
		//*head = (&array_p->identLookup);
		if(reconfigure_in_thread)
			*head = &Config_r.accessList.identLookup;
		else
			*head = &Config.accessList.identLookup;
#endif
#if FOLLOW_X_FORWARDED_FOR
	else if (!strcmp(token, "follow_x_forwarded_for"))
		*head = (&array_p->followXFF);
#endif
	else if (!strcmp(token, "log_access"))
		*head = (&array_p->log);
	else if (!strcmp(token, "url_rewrite_access"))
		*head = (&array_p->url_rewrite);
	else if (!strcmp(token, "redirector_access"))
		*head = (&array_p->url_rewrite);
	else if (!strcmp(token, "storeurl_access"))
		*head = (&array_p->storeurl_rewrite);
	else if (!strcmp(token, "location_rewrite_access"))
		*head = (&array_p->location_rewrite);
	else if (!strcmp(token, "cache"))
		*head = (&array_p->noCache);
	else if (!strcmp(token, "no_cache"))
		*head = (&array_p->noCache);
	else if (!strcmp(token, "broken_posts"))
		*head = (&array_p->brokenPosts);
	else if (!strcmp(token, "upgrade_http0.9"))
		*head = (&array_p->upgrade_http09);
	else if (!strcmp(token, "broken_vary_encoding"))
		*head = (&array_p->vary_encoding);
#if SQUID_SNMP
	else if (!strcmp(token, "snmp_access"))
		//*head = (&array_p->snmp);
		if(reconfigure_in_thread)
			*head = &Config_r.accessList.snmp;
		else
			*head = &Config.accessList.snmp;
#endif
	else if (!strcmp(token, "always_direct"))
		*head = (&array_p->AlwaysDirect);
	else if (!strcmp(token, "never_direct"))
		*head = (&array_p->NeverDirect);
	//else return NULL;

}

void cc_parse_acl_access_r(char* token)
{
	int slot;
	int find = 0;
	access_array * array_p;
	slot = myHash(Config_r.current_domain,SQUID_CONFIG_ACL_BUCKET_NUM);
	debug(90,2)("cc_parse_acl_access: slot is: %d\n",slot);
	array_p = Config_r.accessList2[slot];
    acl_access ** head ;
    debug(90,2)("cc_parse_acl_access: token is : %s\n",token);
    while(array_p)
    {
        debug(90,2)("current_domain is: %s\n",Config_r.current_domain);
        if(!strcmp(array_p->domain,Config_r.current_domain))
        {
            find = 1;	
            /*
               acl_access *aa = locateAccessType(token,array_p);
             *head = aa;
             */
            cc_setHead(token,array_p,&head);
            break;
        }
        array_p = array_p->next;
    }
    if(find == 0)
    {
        debug(90,2)("cc_parse_acl_access: token is : find ==0, current_domain is: %s\n",Config_r.current_domain);
        array_p = xcalloc(1,sizeof(access_array));
        array_p->domain = xstrdup(Config_r.current_domain);
        array_p->next = Config_r.accessList2[slot];
        Config_r.accessList2[slot]=array_p;
        /*
           acl_access *aa = locateAccessType(token,array_p);
         *head = aa;
         */

        cc_setHead(token,array_p,&head);
        debug(90,2)("cc_parse_acl_access: token is : find ==0, current_domain is: %s, and locateAccessType end\n",Config.current_domain);
    }
    aclParseAccessLine(head);
}

void cc_parse_acl_access(char* token)
{
    int slot;
    int find = 0;
    access_array * array_p;
    slot = myHash(Config.current_domain,SQUID_CONFIG_ACL_BUCKET_NUM);
    debug(90,2)("cc_parse_acl_access: slot is: %d\n",slot);
    array_p = Config.accessList2[slot];
    acl_access ** head ;
    while(array_p)
    {
        debug(90,2)("current_domain is: %s\n",Config.current_domain);
        if(!strcmp(array_p->domain,Config.current_domain))
        {
            find = 1;	
            /*
               acl_access *aa = locateAccessType(token,array_p);
             *head = aa;
             */
            cc_setHead(token,array_p,&head);
            break;
        }
        array_p = array_p->next;
    }
    if(find == 0)
    {
        debug(90,2)("cc_parse_acl_access: token is : find ==0, current_domain is: %s\n",Config.current_domain);
        array_p = xcalloc(1,sizeof(access_array));
        array_p->domain = xstrdup(Config.current_domain);
        array_p->next = Config.accessList2[slot];
        Config.accessList2[slot]=array_p;
        /*
           acl_access *aa = locateAccessType(token,array_p);
         *head = aa;
         */

        cc_setHead(token,array_p,&head);
        debug(90,2)("cc_parse_acl_access: token is : find ==0, current_domain is: %s, and locateAccessType end\n",Config.current_domain);
    }
    aclParseAccessLine(head);
}

/*
 * this function only used for Config_r when reload
 *
 */
acl_access *cc_get_common_acl_access(int first_time, char* token)
{
	acl_access* acl = NULL;
	access_array *aa = NULL;

	char host[] = "common";
	//char *host = "common";
	int slot = myHash(host,SQUID_CONFIG_ACL_BUCKET_NUM);
	if (first_time)
		aa = Config.accessList2[slot];
	else
		aa = Config_r.accessList2[slot];
	while(aa)
	{
		if(strncmp(aa->domain,host,strlen(host)) == 0)
		{
			acl = locateAccessType(token,aa);
			break;
		}
		aa = aa->next;
	}
	return acl;
}

acl_access *cc_get_acl_access_by_token_and_host(char* token, char* host)
{
	int slot;
	acl_access* acl = NULL;
	slot = myHash(host,SQUID_CONFIG_ACL_BUCKET_NUM);
	access_array *aa = Config.accessList2[slot];
	while(aa)
	{
		if(strncmp(aa->domain,host,strlen(host)) == 0)
		{
			acl = locateAccessType(token,aa);
			break;
		}
		aa = aa->next;
		//printf("cc_clientAclChecklistCreate: aa\n");
	}

	if(acl == NULL)
	{
		char new_host[] = "common";
		slot = myHash(new_host,SQUID_CONFIG_ACL_BUCKET_NUM);
		aa = Config.accessList2[slot];
		while(aa)
		{
			if(strncmp(aa->domain,new_host,strlen(new_host)) == 0)
			{
				acl = locateAccessType(token,aa);
				break;
			}
			aa = aa->next;
		}
	}
	return acl;
}

acl_access *cc_get_acl_access_by_token(char* token, const clientHttpRequest *http)
{
	int slot;
	acl_access* acl = NULL;
	request_t *r =http->request;
	char* host = r->host;
	slot = myHash(host,SQUID_CONFIG_ACL_BUCKET_NUM);
	access_array *aa = Config.accessList2[slot];
	while(aa)
	{
		if(strncmp(aa->domain,host,strlen(host)) == 0)
		{
			acl = locateAccessType(token,aa);
			break;
		}
		aa = aa->next;
	}

	if(acl == NULL)
	{

		host = "common";
		slot = myHash(host,SQUID_CONFIG_ACL_BUCKET_NUM);
		aa = Config.accessList2[slot];
		while(aa)
		{
			if(strncmp(aa->domain,host,strlen(host)) == 0)
			{
				acl = locateAccessType(token,aa);
				break;
			}
			aa = aa->next;
		}
	}
	return acl;
}

	aclCheck_t *
cc_clientAclCheckListCreate(char* token, const clientHttpRequest * http)
{
	int slot;
	aclCheck_t *ch;
	acl_access* acl = NULL;
	request_t *r = http->request;
    char* host = r->host;
	debug(90,2)("cc_clientAclChecklistCreate : the request->host is: %s\n",host);
	slot = myHash(host,SQUID_CONFIG_ACL_BUCKET_NUM);
	access_array *aa = Config.accessList2[slot];

	/* get the domain access_arrya if exist */
	while(aa)
	{
		if(strncmp(aa->domain,host,strlen(host)) == 0)
		{
			acl = locateAccessType(token,aa);
			break;
		}
		aa = aa->next;
	}

	/* if acl is NULL , which dedicates there are no right access_array for the domain, we set the right domain as common*/
	if(acl == NULL)
	{
		slot = myHash("common",SQUID_CONFIG_ACL_BUCKET_NUM);
		aa = Config.accessList2[slot];
		while(aa)
		{
			if(strcmp("common",aa->domain) == 0)
			{
				acl = locateAccessType(token,aa);
				break;
			}
			aa = aa->next;

		}
	}
	ConnStateData *conn = http->conn;
	ch = aclChecklistCreate(acl,
							http->request,
							conn->rfc931);

	/*
	 * hack for ident ACL. It needs to get full addresses, and a
	 * place to store the ident result on persistent connections...
	 */
	/* connection oriented auth also needs these two lines for it's operation. */
	ch->conn = conn;
	cbdataLock(ch->conn);

	return ch;
}

void copyAndTailAclAccess(acl_access **src, acl_access **dst)
{
	acl_access *cur, *tail, *C;

	if(*src == NULL)
	{
		return;
	}
	if(*dst == NULL)
	{
		//*dst = cbdataAlloc(acl_access);
		assert(*src);

		//tail = *dst;
		cur = *src;
		while(cur)
		{
			C = cbdataAlloc(acl_access);
			cc_copy_acl_access(cur,C);
			if(*dst == NULL)
			{
				tail = *dst = C;
			
			}
			else
			{
				tail = tail->next = C;
			}
			cur = cur->next;

		}
		
	}
	else
	{
		tail = *dst;
		while(tail&& tail->next)
		{
			tail = tail->next;
		}
		cur = *src;
		while(cur)
		{
			C = cbdataAlloc(acl_access);
			cc_copy_acl_access(cur,C);
			tail = tail->next = C;
			cur = cur->next;

		}

	}
}

void copyAndHeadAclAccess(acl_access **src, acl_access **dst)
{
	acl_access *cur, *tail, *C;
	if(*src == NULL)
	{
		return;
	}
	if(*dst == NULL)
	{
		//*dst = cbdataAlloc(acl_access);
		assert(*src);

		//tail = *dst;
		cur = *src;
		while(cur)
		{
			C = cbdataAlloc(acl_access);
			cc_copy_acl_access(cur,C);
			if(*dst == NULL)
			{
				tail = *dst = C;
			
			}
			else
			{
				tail = tail->next = C;
			}
			cur = cur->next;

		}
		
	}
	else
	{
		tail = *dst;
		//tail = *dst;
		/* 
		   while(tail&& tail->next)
		   {
		   tail = tail->next;
		   }
		   */
		int count = 0;
		cur = *src;
		acl_access *copy_p = NULL;
		acl_access *head = NULL;
		while(cur)
		{
			C = cbdataAlloc(acl_access);

			cc_copy_acl_access(cur,C);
			if(count == 0)
			{
				head = copy_p = C;

			}
			else
			{
				copy_p = copy_p->next = C;
			}
			cur = cur->next;
			count++;

		}
		if(copy_p != NULL)
			copy_p->next = tail;
		/* transerve */
		*dst = head;

	}
}

/* add for refresh pattern optimize by xin.yao: 2012-06-11 */
void cc_copy_refresh(refresh_t *src, refresh_t *dst)
{
	if (NULL == src || NULL == dst)
		return;
	dst->pattern = xstrdup(src->pattern);
	dst->min = src->min;
	dst->pct = src->pct;
	dst->max = src->max;
	dst->next = NULL;
	memcpy(&(dst->flags), &(src->flags), sizeof(src->flags));
	dst->max_stale = src->max_stale;
	dst->stale_while_revalidate = src->stale_while_revalidate;
	dst->negative_ttl = src->negative_ttl;
	dst->regex_flags = src->regex_flags;
	regcomp(&dst->compiled_pattern, dst->pattern, dst->regex_flags);
}

static refresh_t *cc_find_refresh2(const char *domain, const char *url)
{
	int slot;
	refresh_t *R = NULL;
	access_array *array = NULL; 
#if 0
    if(Config.onoff.debug_80_log)
    {
        debug(90,0)("[DEBUG] %s: domain %p, url %p\n", __func__, domain, url);
        debug(90,0)("[DEBUG] %s: domain %s, url %s\n", __func__, domain, url);
    }
#endif    
	slot = myHash((void *)domain, SQUID_CONFIG_ACL_BUCKET_NUM);
	debug(90, 3)("cc_find_refresh2(): domain=[%s], slot=[%d]\n", domain, slot);
#if 0	
	if (reconfigure_in_thread)
		array = Config_r.accessList2[slot];
	else    
#endif	
		array = Config.accessList2[slot];
#if 0		
	if(Config.onoff.debug_80_log)
	{
        debug(90,0)("[DEBUG] %s: reconfigure_in_thread %d, slot %d, array %p\n", __func__, reconfigure_in_thread, slot, array);
    }
#endif    
	while (NULL != array)
	{
		if (!strcmp(array->domain, domain)) 
		{       
			for (R = array->refresh; NULL != R; R = R->next)
			{       
				if (!regexec(&(R->compiled_pattern), url, 0, 0, 0))
				{
					debug(90, 3)("cc_find_refresh2(): pattern=[%s]\n", R->pattern);
					return R;
				}
			}       
		}       
		array = array->next;
	}

	return NULL;
}

refresh_t *cc_find_refresh(const char *url)
{
	int slot;
	char domain[512];
	memset(domain,0,512);
	char *p_port = NULL;
	char *domain_start, *domain_end;
	access_array *array = NULL; 
	refresh_t *entry = NULL;
	size_t copy_sz = 0;

	domain_start = strstr(url, "://"); 
	if (NULL == domain_start)
		return NULL;
	domain_start += 3;
	domain_end = strstr(domain_start, "/");
	if (NULL == domain_end)
		copy_sz = 511;
	else    
		copy_sz = domain_end - domain_start;
	strncpy(domain, domain_start, copy_sz);
	domain[copy_sz] = '\0';
	p_port = strchr(domain, ':');
	if (p_port != NULL)
	{
		//remove port
		*p_port = '\0';
	}
	entry = cc_find_refresh2(domain, url);
	if (NULL == entry)
	{
		//if not found by domain, find in common
		entry = cc_find_refresh2("common", url);
	}
	
	return entry;
}

static void copyBeginAndHeadRefresh(refresh_t **src, refresh_t **dst)
{
	refresh_t *R = NULL;
	refresh_t *cur = NULL;
	refresh_t *tmp = NULL;
	refresh_t *tail = NULL;
	refresh_t *head = NULL;

	if (NULL == *src)
		return;
	if (NULL == *dst)
	{
		cur = *src;
		while (NULL != cur)
		{
			R = xcalloc(1, sizeof(refresh_t));
			cc_copy_refresh(cur, R);
			if (NULL == *dst)
			{
				*dst = R;
				tail = *dst;
			}
			else
			{
				tail->next = R;
				tail = tail->next;
			}
			cur = cur->next;
		}
		return;
	}
	tail = *dst;
	cur = *src;
	while (cur)
	{
		R = xcalloc(1, sizeof(refresh_t));
		cc_copy_refresh(cur, R);
		if (NULL == head)
		{
			head = R;
			tmp = head;
		}
		else
		{
			tmp->next = R;
			tmp = tmp->next;
		}
		cur = cur->next;
	}
	if (NULL != tmp)
		tmp->next = tail;
	*dst = head;
}

static void copyEndAndTailRefresh(refresh_t **src, refresh_t **dst)
{
	refresh_t *R = NULL;
	refresh_t *cur = NULL;
	refresh_t *tail = NULL;

	if (NULL == *src)
		return;
	if (NULL == *dst)
	{
		cur = *src;
		while (NULL != cur)
		{
			R = xcalloc(1, sizeof(refresh_t));
			cc_copy_refresh(cur, R);
			if (NULL == *dst)
			{
				*dst = R;
				tail = *dst;
			}
			else
			{
				tail->next = R;
				tail = tail->next;
			}
			cur = cur->next;
		}
		return;
	}
	tail = *dst;
	while (NULL != tail->next)
	{
		tail = tail->next;	
	}
	cur = *src;
	while (NULL != cur)
	{
		R = xcalloc(1, sizeof(refresh_t));
		cc_copy_refresh(cur, R);
		tail->next = R;
		tail = tail->next;
		cur = cur->next;
	}
}
/* end by xin.yao: 2012-06-11 */

void combineAccessArray(access_array *a, access_array *b,access_array *c)
{
	if(a == NULL)
	{
		a=b;
		return;
	}
	//acl_access *cur, *next;
	/* link http_access */
	if(b != NULL)
	{
		copyAndTailAclAccess(&b->http,&a->http);
	}
	if(c != NULL)
	{
		copyAndHeadAclAccess(&c->http,&a->http);
	}
	/*
	   if(a->http == NULL)
	   a->http = b->http;
	   else
	   {
	   cur = next = a->http;
	   while(next)
	   {
	   cur = next;
	   next = next->next;
	   }
	//	cur->next = b->http;
	next = b->http;
	while(next)
	{
	acl_access *C = cbdataAlloc(acl_access);
	cc_copy_acl_access(next,C);
	cur->next = C;
	next = next->next;
	}
	}
	 */
	/* link http_access2 */
	if(b != NULL)
		copyAndTailAclAccess(&b->http2,&a->http2);
	if(c != NULL)
		copyAndHeadAclAccess(&c->http2,&a->http2);
	/* link http_reply_access */
	if(b != NULL)
		copyAndTailAclAccess(&b->reply,&a->reply);
	if(c != NULL)
		copyAndHeadAclAccess(&c->reply,&a->reply);
	/* link icp_access */
	if(b != NULL)
		copyAndTailAclAccess(&b->icp,&a->icp);
	if(c != NULL)
		copyAndHeadAclAccess(&c->icp,&a->icp);
#if USE_HTCP
	/* link htcp */
	if(b != NULL)
		copyAndTailAclAccess(&b->hctp,&a->hctp);
	if(c != NULL)
		copyAndHeadAclAccess(&c->hctp,&a->hctp);
#endif
#if USE_HTCP
	/* link htcp_clr_access */
	if(b != NULL)
		copyAndTailAclAccess(&b->htcp_clr,&a->htcp_clr);
	if(c != NULL)
		copyAndHeadAclAccess(&c->htcp_clr,&a->htcp_clr);
#endif
	/* link miss_access */
	if(b != NULL)
		copyAndTailAclAccess(&b->miss,&a->miss);
	if(c != NULL)
		copyAndHeadAclAccess(&c->miss,&a->miss);
#if USE_IDENT
	/* link ident_lookup_access*/
	if(b != NULL)
		copyAndTailAclAccess(&b->identLookup,&a->identLookup);
	if(c != NULL)
		copyAndHeadAclAccess(&c->identLookup,&a->identLookup);
#endif
#if FOLLOW_X_FORWARDED_FOR
	/* link follow_x_forwarded_for*/
	if(b != NULL)
		copyAndTailAclAccess(&b->followXFF,&a->followXFF);
	if(c != NULL)
		copyAndHeadAclAccess(&c->followXFF,&a->followXFF);
#endif
	/* link log_access*/
	if(b != NULL)
		copyAndTailAclAccess(&b->log,&a->log);
	if(c != NULL)
		copyAndHeadAclAccess(&c->log,&a->log);
	/* link url_rewrite_access*/
	if(b != NULL)
		copyAndTailAclAccess(&b->url_rewrite,&a->url_rewrite);
	if(c != NULL)
		copyAndHeadAclAccess(&c->url_rewrite,&a->url_rewrite);
	/* link storeurl_access*/
	if(b != NULL)
		copyAndTailAclAccess(&b->storeurl_rewrite,&a->storeurl_rewrite);
	if(c != NULL)
		copyAndHeadAclAccess(&c->storeurl_rewrite,&a->storeurl_rewrite);
	/* link location_rewrite_access*/
	if(b != NULL)
		copyAndTailAclAccess(&b->location_rewrite,&a->location_rewrite);
	if(c != NULL)
		copyAndHeadAclAccess(&c->location_rewrite,&a->location_rewrite);
	/* link broken_posts*/
	if(b != NULL)
		copyAndTailAclAccess(&b->brokenPosts,&a->brokenPosts);
	if(c != NULL)
		copyAndHeadAclAccess(&c->brokenPosts,&a->brokenPosts);
	/* link upgrade_http0*/
	if(b != NULL)
		copyAndTailAclAccess(&b->upgrade_http09,&a->upgrade_http09);
	if(c != NULL)
		copyAndHeadAclAccess(&c->upgrade_http09,&a->upgrade_http09);
	/* link broken_vary_encoding*/
	if(b != NULL)
		copyAndTailAclAccess(&b->vary_encoding,&a->vary_encoding);
	if(c != NULL)
		copyAndHeadAclAccess(&c->vary_encoding,&a->vary_encoding);
#if SQUID_SNMP
	/* link snmp_access*/
	if(b != NULL)
		copyAndTailAclAccess(&b->snmp,&a->snmp);
	if(c != NULL)
		copyAndHeadAclAccess(&c->snmp,&a->snmp);
#endif
	/* link always_direct*/
	if(b != NULL)
		copyAndTailAclAccess(&b->AlwaysDirect,&a->AlwaysDirect);
	if(c != NULL)
		copyAndHeadAclAccess(&c->AlwaysDirect,&a->AlwaysDirect);
	/* link never_direct*/
	if(b != NULL)
		copyAndTailAclAccess(&b->NeverDirect,&a->NeverDirect);
	if(c != NULL)
		copyAndHeadAclAccess(&c->NeverDirect,&a->NeverDirect);
	/* link no_cache*/

	if(b != NULL)
		copyAndTailAclAccess(&b->noCache,&a->noCache);
	if(c != NULL)
		copyAndHeadAclAccess(&c->noCache,&a->noCache);
	/* add for refresh pattern optimize by xin.yao: 2012-06-11 */
	if (b != NULL)
		copyEndAndTailRefresh(&b->refresh, &a->refresh);
	if (c != NULL)
		copyBeginAndHeadRefresh(&c->refresh, &a->refresh);
	/* end by xin.yao: 2012-06-11 */
}

/*commbine the module's domain_access, link the common domain acl_rules and mod_params to the tail of the normal domain*/

void cc_combineModuleAclAccess()
{
	int i,j;
	cc_module *mod;
	mod_domain_access *cur, *common_domain_access_begin, *common_domain_access_end;

	for(i=0;i<cc_modules.count;i++)
    {
        mod = cc_modules.items[i];
        if (0 == mod->has_acl_rules || 0 == mod->flags.enable)
            continue;
        /* tail old mod_param to cc_old_mod_param */
        common_domain_access_begin = cc_get_domain_access("common_begin",mod);
        common_domain_access_end = cc_get_domain_access("common_end",mod);

        for (j = 0; j < 100; j++)
        {
            cur = mod->domain_access[j];
            while (cur)
            {
                if (strcmp(cur->domain,"common_begin") && strcmp(cur->domain,"common_end"))
                {
                    /* we should copy the acl_rules of common_domain_access_end to the tail of cur->acl_rules*/
                    if (common_domain_access_end)
                    {
                        copyAndTailAclAccess(&common_domain_access_end->acl_rules, &cur->acl_rules);
                        cur->mod_params = xrealloc(cur->mod_params, (cur->param_num + common_domain_access_end->param_num) * sizeof(cc_mod_param*));
                        memcpy(cur->mod_params + cur->param_num, common_domain_access_end->mod_params, sizeof(cc_mod_param*)*common_domain_access_end->param_num);
                        cur->param_num += common_domain_access_end->param_num;
                    }

                    /********** now we should copy the acl_rules of common_domain-access_begin to the head of the cur->acl_rules */
                    if (common_domain_access_begin)
                    {
                        copyAndHeadAclAccess(&common_domain_access_begin->acl_rules,&cur->acl_rules);
                        cc_mod_param **orig_params = cur->mod_params;        // need modified, noted by chenqi
                        cur->mod_params = xcalloc(cur->param_num + common_domain_access_begin->param_num, sizeof(cc_mod_param*));
                        memcpy(cur->mod_params,common_domain_access_begin->mod_params,sizeof(cc_mod_param*)*common_domain_access_begin->param_num);
                        memcpy(cur->mod_params + common_domain_access_begin->param_num, orig_params,cur->param_num * sizeof(cc_mod_param*));
                        cur->param_num += common_domain_access_begin->param_num;
                        safe_free(orig_params);
                    }
                }
                cur = cur->next;
            }
        }
        /* add the common domain access if none*/
        mod_domain_access *common_domain_access = cc_get_domain_access("common",mod);
        int slot = myHash("common",100);
		if (common_domain_access == NULL)
        {
            common_domain_access = xcalloc(1,sizeof(mod_domain_access));
            common_domain_access->domain = xstrdup("common");
            int total_num = 0;
            int begin_num =0;
            int end_num = 0;
            if(common_domain_access_begin)
            {
                total_num += common_domain_access_begin->param_num;
                begin_num = common_domain_access_begin->param_num;
            }

            if(common_domain_access_end)
            {
                total_num += common_domain_access_end->param_num;
                end_num = common_domain_access_end->param_num;
            }
            common_domain_access->mod_params = xcalloc(1,sizeof(cc_mod_param*)*total_num);
            if(common_domain_access_begin)
            {
                copyAndHeadAclAccess(&common_domain_access_begin->acl_rules,&common_domain_access->acl_rules);
                memcpy(common_domain_access->mod_params,common_domain_access_begin->mod_params,sizeof(cc_mod_param*) * begin_num);
            }
            if(common_domain_access_end)
            {
                copyAndTailAclAccess(&common_domain_access_end->acl_rules, &common_domain_access->acl_rules);
                memcpy(common_domain_access->mod_params + begin_num, common_domain_access_end->mod_params, sizeof(cc_mod_param*) * end_num);
            }
            common_domain_access->param_num = total_num;
            common_domain_access->next = mod->domain_access[slot];
            mod->domain_access[slot] = common_domain_access;
            common_domain_access = NULL;
            /*
               if(common_domain_access_begin == NULL && common_domain_access_end == NULL)
               {
               safe_free(common_domain_access);
               }
               */
        }
    }
}

void cc_combineAclAccessWithCommon()
{

	int slot,len_begin,len_end;
	access_array *cur,*next;
	access_array *common_begin, *common_end;
	common_begin = common_end = NULL;
	acl_access *cur_acl,*next_acl,*common_acl;

	/* set common_begin */
	slot = myHash("common_begin",SQUID_CONFIG_ACL_BUCKET_NUM);
	cur = next = Config.accessList2[slot];
	len_begin = strlen("common_begin");
	len_end = strlen("common_end");
	while(cur)
	{
		if(strncmp(cur->domain,"common_begin",len_begin) == 0)
		{
			common_begin = cur;
			break;

		}
		cur = cur->next;
	}

	/* set common_end  */
	slot = myHash("common_end",SQUID_CONFIG_ACL_BUCKET_NUM);
	cur = next = Config.accessList2[slot];
	while(cur)
	{
		if(strncmp(cur->domain,"common_end",len_end) == 0)
		{
			common_end = cur;
			break;

		}
		cur = cur->next;
	}

	for(slot = 0;slot<SQUID_CONFIG_ACL_BUCKET_NUM;slot++)
	{
		cur = next = Config.accessList2[slot];
		while(cur)	
		{

			/*every entry should link with the common*/

			if(strncmp(cur->domain,"common_begin",len_begin) != 0 && strncmp(cur->domain,"common_end",len_end)!=0)
			{
                //debug(190,0)("%s: combine to %s beg\n", __func__, cur->domain);
				combineAccessArray(cur,common_end,common_begin);
				//debug(190,0)("%s: combine to %s end\n", __func__, cur->domain);
			}
			cur = cur->next;
		}
	}
	/* now set the common access_array as common_begin + common_end 
	 * */
	slot = myHash("common",SQUID_CONFIG_ACL_BUCKET_NUM);
	cur = next =Config.accessList2[slot];
	while(cur)
	{

		if(strncmp(cur->domain,"common",6) ==0 && strlen(cur->domain) == 6)
		{
			break;
		}
		cur = cur->next;
	}
	if(cur == NULL)
	{
		cur = xcalloc(1,sizeof(access_array));
		cur->domain = xstrdup("common");
	}
	cur->next = Config.accessList2[slot];
	Config.accessList2[slot] = cur;
	combineAccessArray(cur,common_end,common_begin);
}
/* added*/
#endif
