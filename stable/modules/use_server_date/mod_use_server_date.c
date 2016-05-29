#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK

static cc_module* mod = NULL;
static int use_server_date_on = 1;

////////////////////// hook functions ////////////////

/**
  * return 0 if parse ok, -1 if error
  */
/*
static int func_sys_parse_param(char *cfg_line)
{
	//Must be [mod_use_server_date on] or [mod_use_server_date off]
	assert(cfg_line);
	char *t = strtok(cfg_line, w_space);
	if(strcmp(t, "mod_use_server_date"))
	{
		debug(105,0)("mod_use_server_date cfg_line not begin with mod_use_server_date!\n");
		return -1;
	}

	t = strtok(NULL, w_space);
	if(!strcmp(t, "on"))
	{
		use_server_date_on = 1;
		return 0;
	}
	else if(!strcmp(t, "off"))
	{
		use_server_date_on = 0;
		return 0;
	}
	debug(105,0)("mod_use_server_date cfg_line not end with on or off!\n");
	return -1;
}
*/

static int reply_cannot_find_date(const HttpReply * reply)
{
	debug(105, 4)("reply_cannot_find_date\n");
	if (use_server_date_on && reply->date < 0)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

static int modifyServedDate(StoreEntry* entry)
{
	const HttpReply *reply = entry->mem_obj->reply;
	int age = httpHeaderGetInt(&reply->header, HDR_AGE);

	if(use_server_date_on && -1 == age)
		return 1;
	else if(use_server_date_on && age && age < squid_curtime)
		return 2;
	else
		return 0;
}


// module init 
int mod_register(cc_module *module)
{
	debug(105, 1)("(mod_use_server_date) ->  init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");
	//mod = module;
	//mod = (cc_module*)cc_modules.items[module->slot];
	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	
	//module->hook_func_private_modify_entry_timestamp = modifyServedDate;
	cc_register_hook_handler(HPIDX_hook_func_private_modify_entry_timestamp,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_modify_entry_timestamp),
			modifyServedDate);

	cc_register_hook_handler(HPIDX_hook_func_reply_cannot_find_date,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_reply_cannot_find_date),
 			reply_cannot_find_date);

	return 0;
}

#endif
