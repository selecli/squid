/*
 *
 * $Id: mod_hdr_store_multi.c, v1.0 2012/03/15 
 *
 * DEBUG: section 159  
 *
 */
#include "cc_framework_api.h"
#ifdef CC_FRAMEWORK

enum {
	HEADER_DEFAULT = 1,
	HEADER_DEFAULT_ALL,
	HEADER_ORIGINAL,
	HEADER_NONE
};

static cc_module* mod = NULL;

typedef struct _header_values
{
	char *value; 
	struct _header_values *next;
} header_values;


typedef struct _mod_conf_hdr_store_multi
{
	int id;	/* header type */
	int case_insensitive;
	int replace;
	int other;
	
	char header_name[128];
	char other_value[128]; /*_default 'value' */
	
	header_values *values;
}mod_conf_hdr_store_multi;

static void mod_param_init(mod_conf_hdr_store_multi *mod_param)
{
	mod_param->id = -1;
	bzero(mod_param->header_name, 128);
	bzero(mod_param->other_value, 128);
	mod_param->values = NULL;
	mod_param->case_insensitive = FALSE;
	mod_param->replace = FALSE;
}

static void free_header_values(header_values **head)
{
	debug(159,9)("(mod_hdr_store_multi) -> free_hander_values start\n");
	header_values *cur = NULL;
	if (*head != NULL)
	{
		int i=0;//for debug
		for(cur = *head; cur != NULL; cur = cur->next)
		{
			debug(159,9)("(mod_hdr_store_multi) -> free_hander_values: [%d]'s cur->value = [%s]\n",++i,cur->value);
			safe_free(cur->value);
		}
	}
	safe_free(*head);
	debug(159,9)("(mod_hdr_store_multi) -> free_hander_values end\n");
}

/*travel mod_conf_hdr_store_multi's header_values */
static void travel_value(header_values **head)
{
	header_values *cur = NULL;
	int i=0;	//debug
	if (*head != NULL)
	{
		for(cur = *head; cur != NULL; cur = cur->next)
			debug(159, 9)("(mod_hdr_store_multi) -> travel_values: [%d]'s value=[%s]\n", ++i, cur->value);
	}
}

static void store_header_values(header_values **head, char *val)
{
	assert(val);
	header_values *new = NULL;
	header_values *cur = NULL;
	int len = strlen(val);

	new = xmalloc(sizeof(header_values));
	new->value = xmalloc(len+1);
	strncpy(new->value, val, len);
	*(new->value+len)='\0';
	new->next = NULL;

	if (*head == NULL)
	{
		*head = new;
	}
	else 
	{
		for(cur = *head; cur->next != NULL; cur = cur->next)
			;
		cur->next=new;
	}
}

static int free_callback(void* param)
{
	mod_conf_hdr_store_multi  *mod_param  = (mod_conf_hdr_store_multi*) param;

	if(mod_param)
	{
		free_header_values(&mod_param->values);
		safe_free(mod_param);
	}
	return 0;
}

static int func_sys_parse_param(char *cfg_line)
{
	assert(cfg_line);
	debug(159, 9)("(mod_hdr_store_multi) -> func_sys_parse_param start\n");
	mod_conf_hdr_store_multi *mod_param = NULL;

	char *token = NULL;
	debug(159, 9)("(mod_hdr_store_multi) -> func_sys_parse_param : cfg_line = [%s]\n",cfg_line);	
	if((token = strtok(cfg_line, w_space)) != NULL)
	{
		if(strcmp(token, "mod_hdr_store_multi") != 0)
		return -1;
	}
	else 
	{
		debug(159, 3)("(mod_hdr_store_multi) ->  parse line error!\n");
		return -1;
	}
	
	while ((token = strtok(NULL, ";")))
	{
		if (!strncmp(token, "%", 1))
		{
			mod_param = xmalloc(sizeof(mod_conf_hdr_store_multi));
			mod_param_init(mod_param);

			strncpy(mod_param->header_name, token+1, strlen(token+1));
			debug(159, 9)("(mod_hdr_store_multi) -> func_sys_parse_param:mod_param->header_name=[%s]\n",mod_param->header_name);

			mod_param->id=httpHeaderIdByNameDef(token+1, strlen(token)-1);
		
			debug(159, 9)("(mod_hdr_store_multi) -> func_sys_parse_param:mod_param->id=[%d]\n",mod_param->id);
			if(mod_param->id < 0)
			{
				debug(159, 1)("(mod_hdr_store_multi) -> func_sys_parse_param: %s is not exist\n", token+1);
				xfree(mod_param);
				mod_param = NULL;
				return -1;
			}
		}
		else if(!strncmp(token, "-i", 2) && mod_param)
		{
			mod_param->case_insensitive = TRUE;
		}
		else if(!strncmp(token, "-r", 2) && mod_param)
		{
			mod_param->replace = TRUE; 
		}
		else if(!strcmp(token, "_default") && mod_param)
		{
			mod_param->other = HEADER_DEFAULT;
			if((token=strtok(NULL, ";")))
			{
				int len = strlen(token)-1;
				if(!strncmp(token, "'", 1) && !strncmp((token + len), "'", 1))
				{
					*(token+len)='\0';
					strncpy(mod_param->other_value, token+1,len);
					debug(159, 9)("(mod_hdr_store_multi) -> func_sys_parse_param:default_value = [%s]\n",mod_param->other_value);
				}
				else
				{
					debug(159, 1)("(mod_hdr_store_multi) -> func_sys_parse_param: can not find value after _default\n");
					return -1;
				}
			}
			else
			{
				debug(159, 1)("(mod_hdr_store_multi) -> func_sys_parse_param: can not find value after _default\n");
				return -1;
			}
		}
		else if(!strncmp(token, "'", 1) && !strncmp(token + strlen(token) - 1, "'", 1) && mod_param)
		{
			*(token + strlen(token) - 1) = '\0';
			store_header_values(&mod_param->values, token+1);
		}
		else if(!strcmp("END", token))
		{
			debug(159, 3)("(mod_hdr_store_multi) -> func_sys_parse_param: END. Begin to parse acl\n");
			break;
		}
		else
			debug(159, 3)("(mod_hdr_store_multi) -> func_sys_parse_param: unkonwn token [%s]\n", token);
	}//while

	travel_value(&mod_param->values); //debug
	debug(159, 9)("(mod_hdr_store_multi) -> func_sys_parse_param end!\n");
	cc_register_mod_param(mod, mod_param, free_callback);
	return 0;
}

static char *find_header_from_mod(mod_conf_hdr_store_multi *mod_param, const char *val)
{
	header_values *cur = mod_param->values;
	debug(159, 9)("(mod_hdr_store_multi) -> find_header_from_mod const char *val = [%s]\n", val);
	int i=0;

	for ( ;cur != NULL; cur = cur->next)
	{
		debug(159, 9)("(mod_hdr_store_multi) -> find_header_from_mod [%d] value = [%s]\n",++i, cur->value);
		if (mod_param->case_insensitive == TRUE)
		{
			if(strcasestr(val, cur->value) != NULL)
				return cur->value;
		}
		else 
		{
			if(strstr(val, cur->value) != NULL)
				return cur->value;
		}
	}

	travel_value(&mod_param->values); //debug
	/* if configured '-r' */
	if (mod_param->replace == TRUE)
		return mod_param->other_value;	
	else
		return NULL;//return null and then don't strcat any suffix in http->request->store_url
}

static int 
modify_store_url_for_header(clientHttpRequest *http)
{
	debug(159, 9) ("(mod_hdr_store_multi) -> modify_store_url_for_header start\n");
	int fd = http->conn->fd;
	char delimiter[]="@@@";
	mod_conf_hdr_store_multi *mod_param = (mod_conf_hdr_store_multi *)cc_get_mod_param(fd, mod);
	if(mod_param == NULL){
		debug(159, 1) ("(mod_hdr_store_multi): cc_get_mod_param failed!\n");
		return 0;
	}

	request_t *request = http->request;
	const char *hdr_val = httpHeaderGetStr(&request->header, mod_param->id);
	if(hdr_val == NULL)
	{
		debug(159, 1) ("(mod_hdr_store_multi) header:[%s] is not exist! \n", mod_param->header_name);
		return 0;
	}

	char *conf_param = NULL;
	conf_param = find_header_from_mod(mod_param, hdr_val);

	debug(159, 3)("(mod_hdr_store_multi) -> find_header_from_mod: conf_param = [%s]\n", conf_param);
	
	
    // safe_free(http->request->store_url);
	
    // start modify by xueye.zhao
    if (http->request->store_url)
    {
        safe_free(http->request->store_url);
    }
    // end by xueye.zhao
    
    int store_len = strlen(http->uri) + 1;
	if (conf_param != NULL )
	{
		store_len += strlen(mod_param->header_name) + strlen(delimiter) + strlen(conf_param) + 1;
	}

	http->request->store_url = xcalloc(1, store_len);
	strcpy(http->request->store_url, http->uri);

	if (conf_param != NULL)
	{
		strcat(http->request->store_url, delimiter);
		strcat(http->request->store_url, mod_param->header_name);
		strcat(http->request->store_url, ":");
		strcat(http->request->store_url, conf_param);
	}
	debug(159, 3) ("(mod_hdr_store_multi) -> modify_store_url_for_header: store_url=[%s],store_len=[%d]\n", http->request->store_url, store_len);

	return 0;
}

/* module init */
int mod_register(cc_module *module)
{
	debug(159, 1)("(mod_hdr_store_multi) -> init module start\n");

	strcpy(module->version, "7.0.R.16488.i686");
	//module->hook_func_http_req_read_second = modify_store_url_for_header;
	cc_register_hook_handler(HPIDX_hook_func_http_after_client_store_rewrite,
			module->slot,
            //(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_read_second),
 
            //modify by xueye.zhao       
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_after_client_store_rewrite),
			modify_store_url_for_header);

	//module->hook_func_sys_parse_param = func_sys_parse_param;
	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
			func_sys_parse_param);

	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module; 
	return 0;
}

#endif
