#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK
//这是本模块在主程序的索引信息的结构指针
//这是框架要求的：由框架维护。指向模块在squid内的结构。
static cc_module* mod = NULL;

struct mod_conf_param {
	    char nod[2];   //特殊的符号
	    int mark;		//0代表仅执行简化操作 1代表仅进行去问号操作 2代表两者都进行
};
static MemPool * mod_config_pool = NULL;

static void * mod_config_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == mod_config_pool)
	{
		mod_config_pool = memPoolCreate("mod_url_simplify config_struct", sizeof(struct mod_conf_param));
	}
	return obj = memPoolAlloc(mod_config_pool);
}

/*
 * 移除掉额外的 /和.等。同时移除问号
 *  关键一点是如果修改了url等， 需要重新构建一次request结构
 */


/*
 * 在某些参数之后, 可能在特殊符号之后, 有http://. 此时的//不能被简化
 */


/*
 *释放申请的结构
 */
static int free_callback(void* param)
{
	struct mod_conf_param* data = (struct mod_conf_param*) param;
	if (data)
	{
		memPoolFree(mod_config_pool, param);
		data = NULL;
	}

	return 0;
}


/*
 * 配置说明
 * mod_url_simpify simplify|rid_question|both [special_mark(only exist in simplify and both mod)] allow|deny acl
 */
static int func_sys_parse_param(char *cfg_line)
{
	struct mod_conf_param* data = NULL; 
	char* token = NULL; 

	if ((token = strtok(cfg_line, w_space)) != NULL)
	{
		if(strcmp(token, "mod_url_simplify"))
			goto err;    
	}
	else    
	{
		debug(97, 3)("(mod_url_simplify) ->  parse line error\n");
		goto err;    
	}

	data = mod_config_pool_alloc();
	
	if (NULL == (token = strtok(NULL, w_space)))
	{
		debug(97, 3)("(mod_url_simplify) ->  parse line data does not existed\n");
		goto err;
	}
	
	if(NULL == data)
	{
		debug(97, 2)("(mod_url_simplify) ->  alloc failed\n");
		goto err;
	}
	//parse mod
	if(!strcmp(token, "simplify"))
	{
		data->mark = 0;
	}
	else if(!strcmp(token, "rid_question"))
	{
		data->mark = 1;
	}
	else if(!strcmp(token, "both"))
	{
		data->mark = 2;
	}
	else
	{
		debug(97, 2)("(mod_url_simplify) ->  parse line %s error\n", cfg_line);
		goto err;
	}

	//parse special mark
	if (NULL != (token = strtok(NULL, w_space)))
	{
		if((strcmp(token, "allow")!=0) && (strcmp(token, "deny")!=0))
		{

			strncpy(data->nod, token, 1);

			data->nod[1] = '\0';
		}

	}

	cc_register_mod_param(mod, data, free_callback);
	
	//FIXME:
	//callback free
	return 0;
err:
	if (data)
	{
		memPoolFree(mod_config_pool, data);
		data = NULL;
	}
	return -1;
}


/*
 * 去除//
 */
static int simplePath1(char* file)
{
	char* str1 = file;
	char* str2;
	int move_len = 0;
	while(1)
	{
		str1 = strstr(str1, "//");
		if(NULL == str1)
		{
			break;
		}
		str1++;
		str2 = str1+1;
		while(1)
		{
			if('/' == *str2)
			{
				str2++;
			}
			else
			{
				break;
			}
		}
		memmove(str1, str2, strlen(str2)+1);
		move_len++;
	}
	if(move_len > 0)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


/*
 * 去掉文件名/./现象
 */
static int simplePath2(char* file)
{
	char* str = file;
	int move_len = 0;
	while(1)
	{
		str = strstr(str, "/./");
		if(NULL == str)
		{
			break;
		}
		memmove(str+1, str+3, strlen(str+3)+1);
		move_len++;
	}
	if(move_len > 0)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


/*
 * 去除文件名/../的现象
 */
static int simplePath3(char* file)
{
	char* str1 = file;
	char* str2;
	int move_len = 0;
	while(1)
	{
		str1 = strstr(str1, "/../");
		if(NULL == str1)
		{
			break;
		}
		str2 = str1;
		while(str2 > file)
		{
			str2--;
			if('/' == *str2)
			{
				break;
			}
		}
		memmove(str2+1, str1+4, strlen(str1+4)+1);
		move_len++;
		str1 = str2;
	}
	if(move_len > 0)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


/*
 * 简化路径
 */
static int simplePath(char* file, char* nod)
{
	debug(97, 5)("simplePath: start, uri=[%s]\n", file);

	//跳过协议
	char* str = strstr(file, "://");
	if(str)
	{
		str += 3;
	}
	else
	{
		debug(97, 1)("simplePath: uri=[%s] has not protocol\n", file);
		return 0;
	}

	//跳过主机名
	str = strchr(str, '/');
	if(!str)
	{
		debug(97, 1)("simplePath: uri=[%s] has not host\n", file);
		return 0;
	}

	//先去掉? # 等特殊意义的字符
	int length = strcspn(str, nod);
	char ch = str[length];
	str[length] = 0;
	int move_len = 0;

	move_len += simplePath1(str);
	move_len += simplePath2(str);
	move_len += simplePath3(str);

	//恢复特殊字符
	if(ch)
	{
		str[length] = ch;
		int len = strlen(str);
		if(length > len)
		{
			memmove(str+len, str+length, strlen(str+length)+1);
		}
	}
	if(move_len > 0)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


/*
 * 去问号函数
 */
static inline int rid_question(request_t* request, char* uri)
{
	debug(97, 3)("rid_question: start, uri=[%s]\n", uri);
	
	char* str = strchr(uri, '?');
	if(str)
	{
		debug(97, 3)("rid_question: uri=[%s] have question\n", uri);
		*str = 0;
		return 1; 
	}   
	else
	{
		debug(97, 3)("rid_question: uri=[%s] have no question\n", uri);
		return 0;
	}
}


static inline void modify_request(clientHttpRequest * http)
{
	debug(97, 3)("modify_request: start, uri=[%s]\n", http->uri);
	request_t* old_request = http->request;
	request_t* new_request = urlParse(old_request->method, http->uri);
	safe_free(http->uri);

	if (new_request)
	{
		safe_free(http->uri);
		http->uri = xstrdup(urlCanonical(new_request));
		if(!http->log_uri)
			http->log_uri = xstrdup(urlCanonicalClean(old_request));
		new_request->http_ver = old_request->http_ver;
		httpHeaderAppend(&new_request->header, &old_request->header);
		new_request->client_addr = old_request->client_addr;
		new_request->client_port = old_request->client_port;
#if FOLLOW_X_FORWARDED_FOR
		new_request->indirect_client_addr = old_request->indirect_client_addr;
#endif /* FOLLOW_X_FORWARDED_FOR */
		new_request->my_addr = old_request->my_addr;
		new_request->my_port = old_request->my_port;
		new_request->flags = old_request->flags;
		new_request->flags.redirected = 1;
		if (old_request->auth_user_request)
		{
			new_request->auth_user_request = old_request->auth_user_request;
			authenticateAuthUserRequestLock(new_request->auth_user_request);
		}
		if (old_request->body_reader)
		{
			new_request->body_reader = old_request->body_reader;
			new_request->body_reader_data = old_request->body_reader_data;
			old_request->body_reader = NULL;
			old_request->body_reader_data = NULL;
		}
		new_request->content_length = old_request->content_length;
		if (strBuf(old_request->extacl_log))
			new_request->extacl_log = stringDup(&old_request->extacl_log);
		if (old_request->extacl_user)
			new_request->extacl_user = xstrdup(old_request->extacl_user);
		if (old_request->extacl_passwd)
			new_request->extacl_passwd = xstrdup(old_request->extacl_passwd);
		if(old_request->cc_request_private_data)
		{
			new_request->cc_request_private_data = old_request->cc_request_private_data;
			old_request->cc_request_private_data = NULL;
		}
		requestUnlink(old_request);
		http->request = requestLink(new_request);
	}
}


/*
 * 主功能函数
 */
static int func_http_req_read(clientHttpRequest* http)
{
	assert(http);
	assert(http->request);
	assert(http->uri);

	int ret = 0;
	int mark = 3; //0-2 有意义
	int fmodify = 0;
	
	char* nod_config;
	int fd= http->conn->fd;

	//取配置内容
	struct mod_conf_param *param = (struct mod_conf_param *)cc_get_mod_param(fd, mod);
	
	nod_config = param->nod;
	mark = param->mark;
	
	if(0 == mark || 2 == mark)
	{
	ret = simplePath(http->uri, nod_config);
	fmodify += ret;
	}
	if(1 == mark || 2 == mark)
	{
	ret = rid_question(http->request, http->uri);
	fmodify += ret;
	}
	if(mark < 0 || mark > 2 )
	{
		debug(97, 2)("(mod->url_simplify: mark = %d, cannot be this!\n)", mark);
		return -1;
	}
	//如果我们更改了，需要重构request	
	if(fmodify > 0)
	{
		modify_request(http);
		debug(97, 3)("(mod->url_simplify) -> url_simplify: uri=[%s]\n", http->uri);
	}
	
	return 0;
}

static int hook_cleanup(cc_module *module)
{                       
	debug(97, 1)("(mod->url_simplify) -> hook_cleanup:\n");
	if(NULL != mod_config_pool)
		memPoolDestroy(mod_config_pool);
	return 0;       
}
/* module init 
 */
int mod_register(cc_module *module)
{
	debug(97, 1)("(mod_url_simplify) ->  init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");
	//hook 功能入口函数
	//module->hook_func_http_before_redirect = func_http_req_read; //rid request & simplify 
	cc_register_hook_handler(HPIDX_hook_func_http_before_redirect,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_before_redirect),
			func_http_req_read);


	//hook配置
	//module->hook_func_sys_parse_param = func_sys_parse_param; //获取特殊符号	
	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
			func_sys_parse_param);

	//module->hook_func_sys_cleanup = hook_cleanup;
	cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_cleanup),
			hook_cleanup);
	//mod = module;
	//mod = (cc_module*)cc_modules.items[module->slot];
	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	return 0;
}
#endif
