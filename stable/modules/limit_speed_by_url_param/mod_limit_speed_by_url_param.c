#include "cc_framework_api.h"
#include "mod_limit_speed_by_url_param.h"

#ifdef CC_FRAMEWORK
static long g_iLimitSpeedTimer = 0;
static cc_module* mod = NULL;
#define CC_DEFER_FD_MAX SQUID_MAXFD
static struct cc_defer_fd_st g_stDeferWriteFd[CC_DEFER_FD_MAX];
static int modifySpeedControl(speed_control *control);
static void cc_clear_defer_fd(int fd);

//typedef char *FUNC(char *, void *);

typedef struct 
{
    char *limit_str;
    long limit_speed; 
} limit_type;

limit_type limit_arr[] =
{
    {"l=a", 28672},
    {"l=b", 34816},
    {"l=c", 40960},
    {"l=d", 48128},
    {"l=e", 54272},
    {"l=f", 60416},
    {"l=g", 66560},
    {"l=h", 73728},
    {"l=i", 79872},
    {"l=j", 86016},
    {"l=k", 92160},
    {"l=l", 98304},
    {"l=m", 105472},
    {NULL, 0},
};

/*
 * 该函数会在系统初始化时被调用，然后在事件队列中一秒种执行一次
 *
 */
static int disable_mod(int fd)
{
	fd_table[fd].cc_run_state[mod->slot]=0;
	return 0;
}


static void resumeDeferWriteFd(void* unused)
{

	//printf("entered resumeDeferWriteFd for limit speed control\n");
	g_iLimitSpeedTimer++;
	if(g_iLimitSpeedTimer < squid_curtime)
	{
		eventAdd("resumeDeferWriteFd", resumeDeferWriteFd, NULL, 0.9, 0);
	}
	else if(g_iLimitSpeedTimer > squid_curtime)
	{
		eventAdd("resumeDeferWriteFd", resumeDeferWriteFd, NULL, g_iLimitSpeedTimer-squid_curtime, 0);
		return;
	}
	else
	{
		eventAdd("resumeDeferWriteFd", resumeDeferWriteFd, NULL, 1.1-(double)current_time.tv_usec/1000000.0, 0);
	}

	int i;
	int fd;
	//int defer=0;
	for(i=0;i<CC_DEFER_FD_MAX;i++)
	{
			fd = g_stDeferWriteFd[i].fd;
			if (fd == 0|| g_stDeferWriteFd[fd].is_client == 0)
					continue;

			if(g_stDeferWriteFd[fd].is_client && fd_table[fd].cc_run_state[mod->slot]>0)
			{
				debug(223,3)("(mod_limit_speed_by_url_param) the fd have limit speed data is: %d\n",fd);
			//	limit_speed_private_data* private_data =(limit_speed_private_data*)(fd_table[fd].cc_fde_private_data[mod->slot].private_data);
				limit_speed_private_data* private_data =cc_get_mod_private_data(FDE_PRIVATE_DATA, &fd,mod);
				if(private_data==NULL)
				{
					debug(223,1)("(mod_limit_speed_by_url_param)--->>>resumeDeferWriteFd: the private_data is NULL \n");
					disable_mod(fd);
					cc_clear_defer_fd(fd);
					continue;
				}
				if(private_data->client_write_defer)
				{
					private_data->client_write_defer = 0;
					debug(223,3)("fd_table[fd].write_handler=%p\n",fd_table[fd].write_handler);
					commSetSelect(fd, COMM_SELECT_WRITE, g_stDeferWriteFd[fd].resume_handler, g_stDeferWriteFd[i].data, 0);
				}
			}
	}
}

/*
 * 负责模块相关数据的初始化工作
 *
 */
static int func_sys_init()
{
	//printf("limit_speed func_sys_init entered\n");
					debug(223,1)("(mod_limit_speed_by_url_param):-----> init\n");
	int i=0;
	for(i = 0;i<CC_DEFER_FD_MAX;i++)
	{
		g_stDeferWriteFd[i].is_client =0;
		g_stDeferWriteFd[i].fd = 0;
		g_stDeferWriteFd[i].data = NULL;
	}
	eventAdd("resumeDeferWriteFd", resumeDeferWriteFd, NULL, 1, 0);	
	g_iLimitSpeedTimer = squid_curtime;
	return 0;
}

/*
 * 释放模块配置变量
 */
static int free_mod_config(void *data)
{
	mod_config* cfg = (mod_config*)data;
	if(cfg)
	{
		if(cfg->limit_speed_individual)
		safe_free(cfg->limit_speed_individual);
		safe_free(cfg);
	}
	return 0;
}
/*
 * 配置参数在这里解析
 * 
 */
static int func_sys_parse_param(char* cfgline)
{
    char* token= NULL;
    if(NULL != (token = strtok(cfgline,w_space)))
    {
        if(strcmp(token,"mod_limit_speed_by_url_param"))
        {
            debug(223,1)("(mod_limit_speed_by_url_param): func_sys_parse_param error,mod_name:[%s]\n", token);
            return -1;
        }
    }
    else
    {
        debug(223,1)("(mod_limit_speed_by_url_param): func_sys_parse_param error");
        return -1;
    }

    mod_config* cfg = xcalloc(1,sizeof(mod_config));
    memset(cfg,0,sizeof(mod_config));

    cfg->limit_speed_individual = (individual_speed_control*)xcalloc(1,sizeof(individual_speed_control));
    memset(cfg->limit_speed_individual,0,sizeof(individual_speed_control));
//add by xueye.zhao start
    if(NULL != (token = strtok(NULL, w_space)))
    {
        if(strcmp(token, "NULL"))
        {
            int a = sizeof(token);
            int b = sizeof("NULL");
            debug(223, 1)("(mod_limit_speed_by_url_param): token=[%s] a=[%d]\n", token, a);
            debug(223, 1)("(mod_limit_speed_by_url_param): NULL=[%s] b=[%d]\n", "NULL", b);
            int len = strlen(token);

            char *tmp = xcalloc(1, len);
            memcpy(tmp, token, len);
            cfg->limit_speed_individual->param_str = (const char *)tmp;
            debug(223, 1)("(mod_limit_speed_by_url_param): found param [%s] in config line\n", tmp);
            tmp =NULL;
        }
        else    
        {
            debug(223, 3)("(mod_limit_speed_by_url_param): no found param in config line\n");
        }
    }
    else
    {
        debug(223,1)("(mod_limit_speed_by_url_param): func_sys_parse_param error in init_bytes\n");
        safe_free(cfg);
        return -1;
    }
//add by xueye.zhao end

    if(NULL != (token = strtok(NULL,w_space)))
    {
        cfg->limit_speed_individual->init_bytes = atoi(token);
    }
    else
    {
        debug(223,1)("(mod_limit_speed_by_url_param): func_sys_parse_param error in init_bytes\n");
        safe_free(cfg);
        return -1;
    }

    if(NULL != (token = strtok(NULL,w_space)))
    {
        int i;
        i=cfg->limit_speed_individual->restore_bps = atoi(token);	
        if(i<=0)
        {
            debug(223, 1)("(mod_limit_speed_by_url_param): func_sys_parse_param error,the restore_bps value is not available\n"); 
            safe_free(cfg);
            return -1;
        }
    }
    else
    {
        debug(223,0)("(mod_limit_speed_by_url_param): func_sys_parse_param error in restore_bps\n");
        safe_free(cfg);
        return -1;
    }

    if(NULL != (token = strtok(NULL,w_space)))
    {
        int i = cfg->limit_speed_individual->max_bytes = atoi(token);
        if(i < cfg->limit_speed_individual->restore_bps)
        {
            debug(223,1)("(mod_limit_speed_by_url_param): func_sys_parse_param warning,the max_bytes value is not fit,max_bytes<restore_bps, now we set max_bytes = restore_bps\n"); 
            cfg->limit_speed_individual->max_bytes = cfg->limit_speed_individual->restore_bps;
        }
    }
    else
    {
        debug(223,1)("(mod_limit_speed_by_url_param): func_sys_parse_param error in max_bytes\n");
        safe_free(cfg);
        return -1;
    }

    if(NULL != (token = strtok(NULL,w_space)))
    {
        cfg->limit_speed_individual->no_limit_bytes= atoi(token);
        debug(223,5)("cfg->no_limit_bytes = %d\n",cfg->limit_speed_individual->no_limit_bytes);
    }
    else
    {
        debug(223,1)("(mod_limit_speed_by_url_param): func_sys_parse_param error in no_limit_bytes\n");
        safe_free(cfg);
        return -1;
    }

    debug(223,5)("(mod_limit_speed_by_url_param): func_sys_parse_param end,and enter acl parse\n");

    cc_register_mod_param(mod,cfg,free_mod_config);	
    return 0;

}

/*
 * decrease the last_bytes of the limit_speed struct owned by 
 * a fde
 *
 */
static int limit_speed_fd_bytes(int fd, int len, unsigned int type)
{

	if(FD_WRITE == type)
	{
		if(g_stDeferWriteFd[fd].is_client ==0)
		{
			return -1;
		}
		limit_speed_private_data* private_data = (limit_speed_private_data*)(fd_table[fd].cc_fde_private_data[mod->slot].private_data);
		if(private_data==NULL)
		{
			disable_mod(fd);
			debug(223,1)("(mod_limit_speed_by_url_param)--->>>limit_speed_fd_bytes: the private_data is NULL \n");
			return 0;
		}

		speed_control* sc = private_data->sc;

        if(sc->no_limit_size < sc->no_limit_bytes)
        {   
        }   
        else if(sc->restore_bps >0) 
        {   
            sc->last_bytes -=len;
        }   

		//if(sc->no_limit_size >= sc->no_limit_bytes)
		//{
        //    if (sc->restore_bps > 0)
        //    {
        //        sc->last_bytes -=len;
        //    }
		//}
		sc->no_limit_size += len;
		debug(223,5)("fd [%d],len [%d],sc->last_bytes[%d][%d]\n",fd,len,sc->last_bytes+len,sc->last_bytes);
	}
	return 0;
}

static int 
cleanup(cc_module* mod)
{
	eventDelete(resumeDeferWriteFd,NULL);
	g_iLimitSpeedTimer = 0;
	return 0;


}

static int free_limit_speed_control(void *data)
{

	limit_speed_private_data* private_data = (limit_speed_private_data*) data;
	if(private_data !=NULL)
	{
		speed_control* sc = private_data->sc;
		if(sc!=NULL)
		{
			xfree(sc);
			sc=NULL;
		}

		safe_free(private_data);
	}
	return 0;
}

static void get_speed_control_from_url(clientHttpRequest* http,limit_speed_private_data * p_data)
{
    int tmp;
    char *p ,*q;
    char *part;
    //    const char * key = "aa7f2e";
    const char *key = p_data->sc->sc_param_str;
    p = strchr(http->uri,'?');
    if(NULL != p)
    {
        part = xstrdup(p);    
        if (NULL != key)
        {
            q = strstr(part,key);
            if(NULL != q)
            {
            
                tmp = atoi(q+strlen(key));
                if (tmp <= 0)
                {
                    debug(223,1)("(mod_limit_speed_by_url_param) URL's param is: [%s]\n", q);
                    debug(223,1)("(mod_limit_speed_by_url_param) tmp is: [%d]\n", tmp);
                    goto finish;
                }
                p_data->sc->restore_bps = tmp;
                p_data->sc->max_bytes = tmp;
            }
            else
            {
                debug(223,1)("(mod_limit_speed_by_url_param)%s has no key %s\n", part, key);
            }
        }
        else
        {
            int i=0;
            debug(223,0)("(mod_limit_speed_by_url_param) key is NULL, now tarvel limit array\n");
            for(;NULL != limit_arr[i].limit_str;)
            {
                q = strstr(part, limit_arr[i].limit_str);
                if (NULL == q)
                {
                    i++;
                    continue;
                }
                else
                {
                    debug(223,0)("(mod_limit_speed_by_url_param): found [%s] in limit_array, limit speed is [%ld]\n", limit_arr[i].limit_str, limit_arr[i].limit_speed);
                    p_data->sc->restore_bps = limit_arr[i].limit_speed; 
                    p_data->sc->max_bytes = limit_arr[i].limit_speed;
                    break;
                }
            }
        }
finish:
        safe_free(part);
    }

    debug(223,5)("get_speed_control_from_url:entered,uri[%s],restore_limit[%d]\n",http->uri,p_data->sc->restore_bps);

    return;
}
/*
static char *modify_url(char *url, void *data)
{
    debug(223,5)("(mod_limit_speed_by_url_param):need modify url is [%s]\n", url);

    const char *tmp = (const char *)data;
    char *tmp2;
    char *ptr;

    int url_len = strlen(url);
    int tmp_len = (int)(tmp-url);

    tmp2 = strstr(url, "?");
    if (NULL != tmp2)
    {
        if (*(tmp-1) == '&')
        {
            if(url_len-(tmp2+4-url+1) == 0)
            {
                ptr = xcalloc(url_len-4, 1);
                xmemcpy(ptr, url, tmp_len-2);
                *(ptr+tmp_len-2) = '\0';
            }
            else
            {
                ptr = xcalloc(url_len-3, 1);
                xmemcpy(ptr, url, tmp_len-1);
                xmemcpy(ptr+tmp_len-1, tmp+3, url_len-tmp_len-3);
                *(ptr+url_len-4) = '\0';
            }
        }
        else
        {
            if (url_len-(tmp2+3 -url+1) == 0)
            {   
                ptr = xcalloc(url_len-3, 1); 
                xmemcpy(ptr, url, tmp_len-1);
                *(ptr+tmp_len-1) = '\0';
            }   
            else
            {   
                ptr = xcalloc(url_len-2, 1); 
                xmemcpy(ptr, url, tmp_len);
                xmemcpy(ptr+tmp_len, tmp+3, url_len-tmp_len-3);
                *(ptr+url_len-1) = '\0';
            } 
        }
    }
    else
    {
        ptr = url;
    }
    debug(223,5)("(mod_limit_speed_by_url_param): modified url is [%s]\n", ptr);

    return ptr;
}

static char *get_new_url(char *url, FUNC fun, void *data)
{
    return fun(url, data);
}

static void modify_request(clientHttpRequest * http,char *new_uri){

    debug(222, 9)("(mod_limit_speed_by_url_param): modify_request, uri=[%s]\n", http->uri);
    request_t* old_request = http->request;

    request_t* new_request = urlParse(old_request->method, new_uri);

    debug(222, 9)("(mod_limit_speed_by_url_param): modify_request, new_uri=[%s]\n", new_uri);
    if (new_request)
    {
        safe_free(http->uri);
        safe_free(http->request->store_url);
        http->uri = xstrdup(urlCanonical(new_request));
        safe_free(http->log_uri);
        http->log_uri = xstrdup(urlCanonicalClean(old_request));
        new_request->http_ver = old_request->http_ver;
        httpHeaderAppend(&new_request->header, &old_request->header);
        new_request->client_addr = old_request->client_addr;
        new_request->client_port = old_request->client_port;
#if FOLLOW_X_FORWARDED_FOR
        new_request->indirect_client_addr = old_request->indirect_client_addr;
#endif // FOLLOW_X_FORWARDED_FOR 
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

        if(old_request->store_url)
            new_request->store_url = xstrdup(old_request->store_url);

        requestUnlink(old_request);
        http->request = requestLink(new_request);
    }
}


static void del_speed_parameter(clientHttpRequest* http)
{
    char *new_url;
    char *old_url = http->uri;
    char *p, *q; 
    int i=0;
    debug(223,5)("(mod_limit_speed_by_url_param): fd is [%d]\n", http->conn->fd);

    p = strchr(http->uri,'?');
    if(NULL != p)
    {   
        for(; NULL != limit_arr[i].limit_str ; ) 
        {   
            q = strstr(p, limit_arr[i].limit_str);
            if (NULL == q)
            {   
                i++;
                continue;
            }   
            else
            {   
                debug(223,1)("(mod_limit_speed_by_url_param): found [%s] in limit_array, limit speed is [%ld]\n", limit_arr[i].limit_str, limit_arr[i].limit_speed);
                new_url = get_new_url(old_url, modify_url, q); 
                debug(223,5)("(mod_limit_speed_by_url_param): new url is [%s]\n", new_url);
                modify_request(http, new_url);
                break;
            }   
        }   
    }   
}

*/
static int setSpeedLimit(clientHttpRequest* http)
{

	int fd = http->conn->fd;
	if(fd_table[fd].cc_run_state[mod->slot]>0)
	{
		mod_config *cfg = (mod_config*)cc_get_mod_param(http->conn->fd,mod);
		assert(cfg);
		

		if(cfg)
		{
			individual_speed_control *individual = cfg->limit_speed_individual;
			limit_speed_private_data* p_data = (limit_speed_private_data*)xcalloc(1,sizeof(limit_speed_private_data));
			memset(p_data, 0 , sizeof(limit_speed_private_data));
			if(p_data->sc==NULL)
			{
				p_data->sc = (speed_control*)xcalloc(1,sizeof(speed_control));
			}

			p_data->client_write_defer = 0;
			speed_control *sc = (speed_control*)(p_data->sc);
            
            //add by xueye.zhao
            sc->sc_param_str = individual->param_str;

			sc->init_bytes = individual->init_bytes;
			sc->restore_bps = individual->restore_bps;
			sc->max_bytes = individual->max_bytes;
			sc->now = squid_curtime;
			sc->last_bytes = individual->init_bytes;
			sc->no_limit_bytes = individual->no_limit_bytes;

			get_speed_control_from_url(http,p_data);
//            del_speed_parameter(http);
			cc_register_mod_private_data(FDE_PRIVATE_DATA, &fd, p_data,free_limit_speed_control , mod);
			assert(g_stDeferWriteFd[fd].is_client == 0);
			g_stDeferWriteFd[fd].is_client = 1; //set the is_client bit, which cleared by fd_close

		}
	}

	return 0;

}

/*
 * clear fd's data for limit_speed
 */
static void cc_clear_defer_fd(int fd)
{
	g_stDeferWriteFd[fd].fd = 0;   
	if(g_stDeferWriteFd[fd].data!=NULL)
	{
		g_stDeferWriteFd[fd].data = NULL;
	}
}

/*
 * set fd's data for limit_speed
 */
static void cc_set_defer_fd(int fd, void * data, PF *resume_handler)
{
	debug(223,5)("cc_set_defer_fd, fd=%d\n", fd);
	if(g_stDeferWriteFd[fd].is_client)
	{
		g_stDeferWriteFd[fd].fd = fd;  
		g_stDeferWriteFd[fd].data = data;
		g_stDeferWriteFd[fd].resume_handler = resume_handler;
	}
}

/*
 * get the bytes need to send to client, and saved as nleft
 *
 */
static int getMayWriteBytes(int fd, void* data,int *nleft, PF *resume_handler)
{

	cc_clear_defer_fd(fd);
	if(g_stDeferWriteFd[fd].is_client==0)
	{
		return *nleft;
	}
	limit_speed_private_data* private_data =cc_get_mod_private_data(FDE_PRIVATE_DATA, &fd,mod);

	if(private_data == NULL)
	{
		disable_mod(fd);
		debug(223,1)("(mod_limit_speed_by_url_param)--->>>getMayWriteBytes: the private_data is NULL,and the nleft is: %d\n",*nleft);
		return *nleft;
	}
	else
	{
		assert(!private_data->client_write_defer);

		speed_control *sc = (speed_control*)private_data->sc;

		int len = 0;
		if(sc != NULL)
		{
			if(sc->restore_bps > 0)
			{
//				debug(223,3)("sc->now [%ld],squid_curtime [%ld],sc->last_bytes [%ld] sc->no_limit_size[%d]\n",sc->now,squid_curtime,sc->last_bytes,sc->no_limit_size);
				if(sc->no_limit_size < sc->no_limit_bytes)
				{
					return *nleft;
				}

				if(sc->last_bytes <=0)
				{
//					printf("getMayWriteBytes: sc->last_bytes<=0\n");
					if(sc->now != squid_curtime)
					{
						modifySpeedControl(sc);
					}
				}
				if(sc->last_bytes <= 0 )
				{
					cc_set_defer_fd(fd,data,resume_handler);
					private_data->client_write_defer = 1;

					len = 0;
					*nleft =len;
//					printf("the fd is set defered and nleft is set as 0\n");
					return len;

				}
				len = sc->last_bytes;

			}
		}
		else
		{
			return *nleft;
		}
		if(len <*nleft)
		{
			*nleft =len;
		}
//		printf("the left is: %d\n",*nleft);
		return len;
	}
}

static int modifySpeedControl(speed_control *control)
{

	assert(control->restore_bps > 0);
	if(control->now < squid_curtime)
	{
		int ret = squid_curtime - control->now;
		if(ret > 10)
		{
			ret = 10;
		}
		control->now = squid_curtime;
		control->last_bytes += ret * control->restore_bps;
		if(control->last_bytes > control->max_bytes)
		{
			control->last_bytes = control->max_bytes;
		}
		control->now = squid_curtime;
	}
	else if(control->now > squid_curtime)
	{
		control->now = squid_curtime;
	}

	return 0;
}

static int func_fd_close(int fd)
{
	g_stDeferWriteFd[fd].is_client = 0;
	return 0;
}

static int http_req_free(clientHttpRequest *http)
{
	g_stDeferWriteFd[http->conn->fd].is_client = 0;
	return 0;
}


static void func_internalStoreUrlRewriteImplement(clientHttpRequest *http)
{
	if (NULL != http->request->store_url)
		safe_free(http->request->store_url);
	char * uri = xstrdup(http->uri);    
	char *tmpuri = strstr(uri,"?");
	if (NULL != tmpuri)
		*tmpuri = 0;
	http->request->store_url = uri;
	debug(223, 3)("internalStoreUrlRewriteImplement: original url is %s and store url is %s\n", http->uri, uri);
}

int mod_register(cc_module *module)
{
	strcpy(module->version, "7.0.R.16488.i686");
	//module->hook_func_sys_init = func_sys_init;
	cc_register_hook_handler(HPIDX_hook_func_sys_init,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_init),
			func_sys_init);
	//module->hook_func_sys_parse_param = func_sys_parse_param;
	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
			func_sys_parse_param);
	//module->hook_func_http_req_read = setSpeedLimit;
	cc_register_hook_handler(HPIDX_hook_func_http_req_read,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_read),
			setSpeedLimit);
	//module->hook_func_fd_bytes = limit_speed_fd_bytes;
	cc_register_hook_handler(HPIDX_hook_func_fd_bytes,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_fd_bytes),
			limit_speed_fd_bytes);
	//module->hook_func_sys_cleanup = cleanup;
	cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_cleanup),
			cleanup);
	//module->hook_func_private_individual_limit_speed = getMayWriteBytes;
	cc_register_hook_handler(HPIDX_hook_func_private_individual_limit_speed,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_individual_limit_speed),
			getMayWriteBytes);
	cc_register_hook_handler(HPIDX_hook_func_http_req_read_second,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_read_second),
			func_internalStoreUrlRewriteImplement);
	//module->hook_func_fd_close = func_fd_close;
	cc_register_hook_handler(HPIDX_hook_func_fd_close,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_fd_close),
			func_fd_close);
	//module->hook_func_http_req_free = http_req_free;
	cc_register_hook_handler(HPIDX_hook_func_http_req_free,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_free),
			http_req_free);

	mod = module;
	return 0;
}
#endif
