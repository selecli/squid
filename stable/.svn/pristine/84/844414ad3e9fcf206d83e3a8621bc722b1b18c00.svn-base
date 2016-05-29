#include <stdio.h>
#include "cc_framework_api.h"

/*
 *  Hook point:
 *
 *  cc_call_hook_func_http_req_parsed
 *
 * */

static cc_module *mod = NULL;

typedef struct _append_param
{
    String ap_str;
} append_param;

static int hook_init()
{
    debug(222, 1)("(mod_append_parameter) -> hook_init:----:)\n");
    return 0;
}

static int free_append_param(void *data)
{
    append_param *ap = (append_param *)data;

    if (ap != NULL)
    {
        stringClean(&ap->ap_str);
        safe_free(ap);
        ap = NULL;
    }

    return 0;
}

static void append_param_str(append_param *ap, const char *str, char *add)
{
    debug(222, 9)("(mod_append_parameter): append str:%s\n", str);
    int str_len = strlen(str);
    int add_len = strlen(add);
    stringAppend(&ap->ap_str, str, str_len);
    stringAppend(&ap->ap_str, add, add_len);
}
static int func_sys_parse_param(char *cfg_line)
{
    debug(222, 1)("(mod_append_parameter): start parse cfg_line\n");
    char *ptr = cfg_line;
    char *delim = " ";
    char *ap_c = "&";
    char *tmp;

    if(strstr(cfg_line, "allow") == NULL)
        return -1;

    if (strstr(cfg_line, "mod_append_parameter") == NULL )
        return -1;
    /* here will wrong like this:
     *
     * mod_append_parameter add=asd allow acl
     *
     * */
    if (strstr(cfg_line, "add") == NULL)
        return -1;

    append_param *ap = xmalloc(sizeof(append_param));
    memset(ap,0,sizeof(append_param)); 

    while ((tmp = strsep(&ptr, delim)) != NULL)
    {
        if (strstr(tmp, "mod_append_parameter") != NULL)
            continue;
        if (strstr(tmp, "add") != NULL)
            continue;
        if (strstr(tmp, "allow") != NULL)
            break;
        // use name=value
        if (strstr(tmp, "=") == NULL)
        {
            debug(222, 1)("(mod_append_parameter):  here no found '=' :%s\n", tmp);
            stringClean(&ap->ap_str);
            safe_free(ap);
            return -1;
        }

        debug(222, 5)("(mod_append_parameter): found string :%s\n", tmp);
        
        append_param_str(ap, tmp, ap_c);
    }

    cc_register_mod_param(mod, ap, free_append_param);

    debug(222, 5)("(mod_append_parameter): end parse cfg_line, add string is [%s] string len is [%d]\n", 
            ap->ap_str.buf, ap->ap_str.len);
    return 0;
}

static void modify_request(clientHttpRequest * http,char *new_uri){

	debug(222, 9)("(mod_append_parameter): modify_request, uri=[%s]\n", http->uri);
	request_t* old_request = http->request;
	
	request_t* new_request = urlParse(old_request->method, new_uri);

	debug(222, 9)("(mod_append_parameter): modify_request, new_uri=[%s]\n", new_uri);
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

typedef char *FUNC(char *, void *);

static char *modify_url(char *url, void *data)
{
    debug(222,5)("(mod_append_parameter):need modify url is [%s]\n", url);

    char *tmp;
    String *str = data;
    int url_len = strlen(url);
    int tmp_len;
    char arr[1024];        
    memset(arr, 0, 1024);
    memcpy(arr, str->buf, str->len-1);
    debug(222,5)("(mod_append_parameter):arr [%s] url [%s]\n", arr, url);

    if(strstr(url, arr) != NULL)
    {
        debug(222,5)("(mod_append_parameter):found [%s] in url [%s]\n", arr, url);
        return NULL;
    }

    char *ptr = xcalloc(url_len+str->len+1, 1); 

    if ((tmp = strstr(url, "?")) == NULL)
    {
        xmemcpy(ptr, url, url_len);
        xmemcpy(ptr+url_len, "?", 1);
        xmemcpy(ptr+url_len+1, str->buf, str->len);
        *(ptr+url_len+str->len) = '\0';
    } 
    else
    {
        if ((url+url_len-1) == tmp)
        {
            debug(222,5)("(mod_append_parameter):only found '?' [%s]\n", ptr);
            memcpy(ptr, url, url_len);
            memcpy(ptr+url_len, arr, str->len-1);
            *(ptr+url_len+str->len-1) = '\0';
        }
        else
        {
            debug(222,5)("(mod_append_parameter):22 [%s]\n", ptr);
            tmp_len = strlen(tmp+1);
            xmemcpy(ptr, url, tmp-url+1);
            xmemcpy(ptr+(tmp-url+1), str->buf, str->len);
            xmemcpy(ptr+(tmp-url+1)+str->len, tmp+1, tmp_len);
            *(ptr+url_len+str->len) = '\0';
        }
    }
    
    debug(222,5)("(mod_append_parameter): modified url is [%s]\n", ptr);
    
    return ptr;
}

static char *get_new_url(char *url, FUNC fun, void *data)
{
    return fun(url, data);
}

static int func_http_before_redirect(clientHttpRequest *http)
{
    char *new_url;
    char *old_url = http->uri;
    
    int fd = http->conn->fd;
    append_param *ap = (append_param *)cc_get_mod_param(fd, mod);
        
    debug(222,5)("(mod_append_parameter): http->uri[%s] ap->ap_str.buf[%s]\n", http->uri, ap->ap_str.buf);
    
    new_url = get_new_url(old_url, modify_url, &ap->ap_str);
    if (new_url == NULL)
    {
        return 0;
    }



    debug(222,5)("(mod_append_parameter): new url is [%s]\n", new_url);
    modify_request(http, new_url);
    
    safe_free(new_url);

    return 0; 
}

int mod_register(cc_module *module)
{
    debug(222, 1)("(mod_append_parameter) -> mod_register:\n");

    strcpy(module->version, "7.0.R.16488.i686");
    //module->hook_func_sys_init = hook_init;
    cc_register_hook_handler(HPIDX_hook_func_sys_init,
            module->slot,
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_init),
            hook_init);

    //module->hook_func_sys_parse_param = func_sys_parse_param;
    cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
            module->slot,
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
            func_sys_parse_param);
    //module->hook_func_http_before_redirect = func_http_before_redirect;
    cc_register_hook_handler(HPIDX_hook_func_http_before_redirect, 
            module->slot,
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_before_redirect),
            func_http_before_redirect);
    
    mod = module;
    return 0;
}
