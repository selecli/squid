#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK

static cc_module* mod = NULL;

struct mod_conf_param {
	char *proxy_account;	
	char *proxy_password;

};


static int free_callback(void* param)
{
	struct mod_conf_param* data = (struct mod_conf_param*) param;
	if(data){
		safe_free(data->proxy_account);
		safe_free(data->proxy_password);
		safe_free(data);
	}

	return 0;
}


// confige line example:
// mod_mp3 allow/deny acl
static int func_sys_parse_param(char *cfg_line)
{
	struct mod_conf_param *data=NULL;
	char* token = NULL;
//mod_sina_mp3
	if ((token = strtok(cfg_line, w_space)) != NULL)
	{
		if(strcmp(token, "mod_sina_mp3"))	
			return -1;
	}
	else
	{
		debug(147, 3)("(mod_sina_mp3) ->  parse line error\n");
		return -1;
	}

//proxy-account
	if (NULL == (token = strtok(NULL, w_space)))
	{
		debug(147, 3)("(mod_sina_mp3) ->  parse line data does not existed\n");
		return -1;
	}
	if (!(strcmp(token, "allow") && strcmp(token, "deny")))
	{
		debug(147, 3)("(mod_sina_mp3) ->  missing proxy-account setting\n");
	}
	
	data = xmalloc(sizeof(struct mod_conf_param));
	data->proxy_account = xmalloc(strlen(token) + 1);
	strcpy(data->proxy_account,token);
	
//proxy-password
	if (NULL == (token = strtok(NULL, w_space)))
	{
		debug(147, 3)("(mod_sina_mp3) ->  parse line data does not existed\n");
		goto err;
	}
	if (!(strcmp(token, "allow") && strcmp(token, "deny")))
	{
		debug(147, 3)("(mod_sina_mp3) ->  missing proxy-password setting\n");
	}

	data->proxy_password = xmalloc(strlen(token) + 1);
	strcpy(data->proxy_password,token);
	

	cc_register_mod_param(mod, data, free_callback);

	return 0;		
err:
	if(data)
		free_callback(data);
	return -1;
}

//standard safeguard chain: store_url = the part of url which is before "?"
static char *store_key_url(char* url){

        char * uri = xstrdup(url);
        char *tmpuri = strstr(uri,"?");
        if(tmpuri != NULL)
                *tmpuri = 0;

        debug(147, 3)("(mod_sina_mp3) -> store_url is %s \n", uri);
        return uri;
}

static int func_http_req_read(clientHttpRequest *http)
{
	
	int fd = http->conn->fd;
	struct mod_conf_param *param = (struct mod_conf_param *)cc_get_mod_param(fd, mod);

	if(param != NULL){
		safe_free(http->request->store_url);
                http->request->store_url = store_key_url(http->uri);
	}	
	return 0;

}

static int mod_http_req_process(clientHttpRequest *http){

	struct mod_config *cfg = cc_get_mod_param(http->conn->fd, mod);
        assert(cfg);
/*	
        if (http->log_type == LOG_TCP_MISS){
		//add header
		char header_value[512];
		memset(header_value,0,512);
		char *account = "CHNC";
		int expiretime = 1306487662;
		char *proxy_ssig = "kTf0Vhj27a";
                snprintf(header_value, 512, "%s %d:%s",account ,expiretime,proxy_ssig);

                HttpHeaderEntry eEntry;
                eEntry.id = HDR_PROXY_AUTHENTICATE;
                stringInit(&eEntry.name, "Proxy-Authorization");
                stringInit(&eEntry.value, header_value);
			
		HttpHeaderEntry *oldHeader = httpHeaderFindEntry(&http->request->header, HDR_PROXY_AUTHENTICATE);
	        if(oldHeader != NULL){
        	        httpHeaderDelById(&http->request->header, HDR_PROXY_AUTHENTICATE);
       		}

                httpHeaderAddEntry(&http->request->header, httpHeaderEntryClone(&eEntry));
                stringClean(&eEntry.name);
                stringClean(&eEntry.value);

                debug(147,2)("mod_sina_mp3: miss and return query mp3 file: %s \n",header_value);
        }*/
	return 0;
}


/* module init */
int mod_register(cc_module *module)
{
	debug(147, 1)("(mod_sina_mp3) ->  init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");
	
	//module->hook_func_http_req_read = func_http_req_read; //modify client side func
	cc_register_hook_handler(HPIDX_hook_func_http_req_read,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_read),
			func_http_req_read);

	cc_register_hook_handler(HPIDX_hook_func_http_req_process,
                        module->slot,
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_process),
                        mod_http_req_process);

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
