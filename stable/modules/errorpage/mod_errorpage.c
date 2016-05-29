#include "cc_framework_api.h"
#include "squid.h" 

#ifdef CC_FRAMEWORK
//这是本模块在主程序的索引信息的结构指针
//这是框架要求的：由框架维护。指向模块在squid内的结构。
static cc_module* mod = NULL;

/*
 * 在组装错误页面之前，覆盖err结构为404错误
 * 这样在组装错误页面时即变成404的错误页面
 * 并将回复的header中的 X-Squid-Error删除来避免用户发现squid的存在
 */

err_type cc_aclGetDenyInfoPage()
{
	acl_deny_info_list **head = &Config.denyInfoList;
	const char *name = AclMatchedName;
	if(!name)
	{
		return ERR_NONE;
	}
	acl_deny_info_list *A = NULL;

	for (A = *head; A; A = A->next)
	{
		acl_name_list *L = NULL;
		for (L = A->acl_list; L; L = L->next)
		{
			if (!strcmp(name, L->name))
				return A->err_page_id;
		}
	}
	return ERR_NONE;
}


/*
 * 修改err结构的主要的hook函数
 */
static int private_func_error_page(ErrorState* err, request_t *request)
{
	assert(err);
	if (cc_aclGetDenyInfoPage() != ERR_NONE)
		return 0;
	if (request && mod->acl_rules)
	{
		aclCheck_t ch;
		acl_access *A = NULL;
		A = mod->acl_rules;
		memset(&ch, 0, sizeof(ch));
		ch.request = request;
		aclChecklistCacheInit(&ch);	
		while (A)
		{
			int answer = 0;
			answer = aclMatchAclList(A->acl_list, &ch);
			if (answer) 
			{
				if (answer < 0)
				{
					aclCheckCleanup(&ch);
					return 0;
				}
				if (A->allow == ACCESS_ALLOWED)
				{
					err->page_id = ERR_FTP_NOT_FOUND;
					err->type = ERR_FTP_NOT_FOUND;
					err->http_status = HTTP_NOT_FOUND;
					aclCheckCleanup(&ch);
					return 0;
				}
				else
				{
					aclCheckCleanup(&ch);
					return 0;
				}
			}
			A = A->next;
		}
		aclCheckCleanup(&ch);
	}
	if (mod->flags.config_on)
	{
		err->page_id = ERR_FTP_NOT_FOUND;
		err->type = ERR_FTP_NOT_FOUND;
		err->http_status = HTTP_NOT_FOUND;
	}
	return 0;
}


/*
 * 在组装完squid reply的header之后，将其中的X-Squide-Error抽离
 */
static int func_http_repl_send_start(clientHttpRequest* http)
{
	assert(http);
	assert(http->reply);

	httpHeaderDelByName(&http->reply->header, "FlexiCache-Error");

	return 0;
}


/* module init 
 * FIXME:
 * 	module这个函数传入时已经分配好空间了吗？
 */
int mod_register(cc_module *module)
{
	debug(96, 1)("(mod_errorpage) ->  init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");
	//hook 功能入口函数
	//module->hook_func_private_error_page = private_func_error_page; //modify error code etc.
	cc_register_hook_handler(HPIDX_hook_func_private_error_page,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_error_page),
			private_func_error_page);
	//module->hook_func_http_repl_send_start = func_http_repl_send_start; //denide the X-Squid-Error header
	cc_register_hook_handler(HPIDX_hook_func_http_repl_send_start,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_repl_send_start),
			func_http_repl_send_start);
	//mod = module;
	//mod = (cc_module*)cc_modules.items[module->slot];
	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	return 0;
}
#endif
