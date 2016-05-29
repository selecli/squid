#include "cc_framework_api.h"
#ifdef CC_FRAMEWORK

//#define ARRAY_SIZE 10000
#define DELIMIT "@@@@"
static cc_module* mod = NULL;

static int private_clientRedirectDone(request_t** new_request,char* result, clientHttpRequest* http)
{
	static unsigned int accCount=0;
	int fd = http->conn->fd;
	if(fd_table[fd].cc_run_state[mod->slot] >0)
	{
		String new_result=StringNull;
		char* temp=NULL;
		if(result)
		{

			/*************deal with redirecting url*******************/

			temp = result;
		}
		else
		{
			/********deal with no redirecting url **************/
			temp = http->uri;
		}
		stringInit(&new_result, temp);

		/***store the url by the big random style *******************/
		stringAppend(&new_result,DELIMIT,sizeof(DELIMIT));
		int n = accCount%Config.cacheSwap.n_configured;
		stringAppend(&new_result,xitoa(n),sizeof(xitoa(n)));
		/***store the url by the big random style *******************/


		if(*new_request)
		{
			if((*new_request)->link_count > 0)
			{
				requestUnlink(*new_request);
			}
			else
			{
				requestDestroy(*new_request);
			}
		}

		*new_request = urlParse(http->request->method,new_result.buf);
		debug(131,3)("mod_store_multi_url---->>private_clientRedirectDone :new_request is build, the url is: %s\n",strBuf(new_result));
		debug(131,4)("mod_store_multi_url---->>private_clientRedirectDone :the access count is: %u\n",accCount);
		accCount++;
		stringClean(&new_result);

		return 0;
	}
	else
	{
		return -1;
	}
}

static int func_sys_init()
{
	debug(131,5)("mod_store_multi_url-->func_sys_init: enter\n");
	return 0;
}

static int select_swap_dir_round_robin(const StoreEntry* e)
{
	
	char* url = e->mem_obj->url;
	debug(131,3)("mod_store_multi_url---->>select_swap_dir_round_robin: the url is: %s\n",url);
	char* str = NULL;
	if(NULL!=(str=strstr(url,DELIMIT)))
	{
		/*
		char* old_url = xcalloc(1,strlen(url)-3);
		xstrncpy(old_url,url,strlen(url)-4);
		*/
		//	int n = url[strlen(url)-1] - '0';
		char* pos = str+strlen(DELIMIT);
		
		int n=-1;
		if(*pos >= '0' && *(pos+1)>='0' && *pos <='9' && *(pos+1)<='9')
		{
			n = atoi(pos);
			debug(131,3)("mod_store_multi_url---->>select_swap_dir_round_robin: has cache_dir more than 10, and the select num is: %d \n",n);
		}
		else if(*(pos+1) <'0' || *(pos+1)>'9')
		{
			debug(131,3)("mod_store_multi_url---->>select_swap_dir_round_robin:  pos vaule num is: %d \n",*(pos)-'0');
			n = *pos -'0';
		}
		debug(131,3)("mod_store_multi_url---->>select_swap_dir_round_robin: the selected dirn is: %d, the pos value is: %c\n",n,*pos);
		return n;
	}
	else
	{
		return -1;
	}
}


/************copy from mod_header_combination.c ************************/

static int func_buf_send_to_s(HttpStateData *httpState, MemBuf *mb)
{
	 char g_szBuffer[MAX_URL];
	request_t *request = httpState->request;
	
		debug(131,3)("mod_store_multi_url---->>func_buf_send_to_s:mb->buf: %s\n",mb->buf);
		debug(131,3)("mod_store_multi_url---->>func_buf_send_to_s:the url is: %s\n",strBuf(request->urlpath));
	memset(g_szBuffer, 0, sizeof(g_szBuffer));
	if(strLen(request->urlpath))
	{
		snprintf(g_szBuffer, MAX_URL, "%s", strBuf(request->urlpath));
		char* str = strstr(g_szBuffer, DELIMIT);
		if(NULL != str)
		{
			*str = 0;
		}
	}
	else
	{
		strcpy(g_szBuffer, "/");
	}

	if(mb->size==0)
	{
		debug(131,3)("mod_store_multi_url---->>func_buf_send_to_s end:mb->buf is NULL \n");
		memBufPrintf(mb, "%s %s HTTP/1.%d\r\n",
				RequestMethods[request->method].str,
				g_szBuffer,
				httpState->flags.http11);
	}
		debug(131,3)("mod_store_multi_url---->>func_buf_send_to_s:after dealwith the url is: %s\n",g_szBuffer);
		debug(131,3)("mod_store_multi_url---->>func_buf_send_to_s end:mb->buf: %s\n",mb->buf);
    return 1;	
}
/************copy from mod_header_combination.c ************************/


int mod_register(cc_module *module)
{
	debug(131,5)("mod_store_multi_url-->mod_register: enter\n");
	strcpy(module->version, "7.0.R.16488.i686");
	
	//module->hook_func_sys_init = func_sys_init;
	cc_register_hook_handler(HPIDX_hook_func_sys_init,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_init),
			func_sys_init);

	//module->hook_func_private_clientRedirectDone = private_clientRedirectDone;
	cc_register_hook_handler(HPIDX_hook_func_private_clientRedirectDone,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_clientRedirectDone),
			private_clientRedirectDone);

	//module->hook_func_private_select_swap_dir_round_robin = select_swap_dir_round_robin;
	cc_register_hook_handler(HPIDX_hook_func_private_select_swap_dir_round_robin,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_select_swap_dir_round_robin),
			select_swap_dir_round_robin);

	//module->hook_func_buf_send_to_s = func_buf_send_to_s;
	cc_register_hook_handler(HPIDX_hook_func_buf_send_to_s,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_buf_send_to_s),
			func_buf_send_to_s);

	//mod = module;
	//mod = (cc_module*)cc_modules.items[module->slot];
	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	return 0;
}

#endif
