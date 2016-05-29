#include "cc_framework_api.h"
#include "squid.h"
#ifdef CC_FRAMEWORK

static cc_module *mod = NULL;
static MemPool *g_mod_HAE_Config_t = NULL;

typedef struct _httpExtent_t
{
    wordlist *head;
    short c_minor;
    short s_minor;
}httpExtent_t;

/********************************* Module Interface Start************************************/
static inline httpExtent_t * 
heaConfigParamInit(httpExtent_t *pConfig) 
{
    if(pConfig)
    {
        pConfig->head = NULL;
        pConfig->c_minor= -1;
        pConfig->s_minor= -1;
    }
    return pConfig;
}

static httpExtent_t *haeConfigParamAlloc(void)
{
    if (NULL == g_mod_HAE_Config_t)
    {   
        g_mod_HAE_Config_t = memPoolCreate("mod_httpversion config_struct", sizeof(httpExtent_t));
    }   
    return heaConfigParamInit(memPoolAlloc(g_mod_HAE_Config_t));
}

static int haeConfigParamFree(void* data)
{
    httpExtent_t *pConfig = data; 
    if(pConfig)
    {
        wordlistDestroy(&pConfig->head);
        heaConfigParamInit(pConfig);
        memPoolFree(g_mod_HAE_Config_t, pConfig);
    }
    return 0;
}

/********************************* Module Interface End ************************************/

static int haeParseParam(char *t, short *c_minor, short *s_minor, wordlist **list)
{
    if(NULL == t)
        return 0;
    if(c_minor && 0 == strcmp(t,  "client_http11"))
    {
        *c_minor = 1;
        debug(225, 5)("mod_httpAclExtend parse %s\n", t);
        return 1;
    }
    if(c_minor && 0 == strcmp(t, "client_http10"))
    {
        *c_minor = 0;
        debug(225, 5)("mod_httpAclExtend parse %s\n", t);
        return 1;
    }
    if(s_minor && 0 == strcmp(t, "server_http11"))
    {
        *s_minor = 1;
        debug(225, 5)("mod_httpAclExtend parse %s\n", t);
        return 1;
    }
    if(s_minor && 0 == strcmp(t, "server_http10"))
    {
        *s_minor = 0;
        debug(225, 5)("mod_httpAclExtend parse %s\n", t);
        return 1;
    }
    if(list && 0 == strncmp(t, "header|", strlen("header|")))
    {
        t = strtok(t, "|");
        while((t = strtok(NULL, "|")))
        {
            wordlistAdd(list, t);
            debug(225, 5)("mod_httpAclExtend parse Header %s\n", t);
        }
        return 1;
    }
    return 0;
}

static int 
hookFuncHAETryParseParam(char *cfg_line)
{
    short c_minor = -1;
    short s_minor = -1;
    wordlist *head = NULL;
    char *t0 = strtok(cfg_line, w_space);
    char *t1 = strtok(NULL, w_space);
    char *t2 = strtok(NULL, w_space);
    char *t3 = strtok(NULL, w_space);

    if(NULL == t0 || strcmp(t0, "mod_httpAclExtend"))
    {   
        return -1; 
    } 

    haeParseParam(t1, &c_minor, &s_minor, &head);
    haeParseParam(t2, &c_minor, &s_minor, &head);
    haeParseParam(t3, &c_minor, &s_minor, &head);

    if(-1 == c_minor && -1 == s_minor && NULL == head)
    {
        return -1;
    }

    httpExtent_t *pConfig = haeConfigParamAlloc();
    pConfig->c_minor = c_minor;
    pConfig->s_minor = s_minor;
    pConfig->head = head;
    cc_register_mod_param(mod, pConfig, haeConfigParamFree);
    return 0;
}

static int 
hookFuncHAEFixRequestHeader(clientHttpRequest *http)
{
    httpExtent_t *pConfig = cc_get_mod_param(http->conn->fd, mod);
    if(pConfig && pConfig->head && 0 == http->http_ver.minor)
    {
        int count = 0;
        wordlist *list =  pConfig->head;
        while(list && list->key)
        {
            if(httpHeaderDelByName(&http->request->header,list->key))
            {
                debug(225, 5)("mod_httpAclExtend del Header %s %s\n", list->key, http->uri);
                count++;
            }
            list = list->next;
        }
    }
    return 0;
}

static int 
hookFuncHAEFixClientVersion(clientHttpRequest* http)
{
    httpExtent_t *pConfig = cc_get_mod_param(http->conn->fd, mod);
    if(pConfig && pConfig->c_minor >= 0)
    {
        http->reply->sline.version.minor = pConfig->c_minor;
        debug(225, 5)("mod_httpAclExtend modify Reply Version %d %s\n", pConfig->c_minor, http->uri);
    }
    return 0;
}

static int 
hookFuncHAEFixServerVersion(HttpStateData *hs)
{
    httpExtent_t *pConfig = cc_get_mod_param(hs->fd, mod);
    if(pConfig && pConfig->s_minor >= 0)
    {
        hs->flags.http11 =  pConfig->s_minor;
        debug(225, 5)("mod_httpAclExtend modify user %d to server %s\n", pConfig->c_minor, storeUrl(hs->entry));
    }
    return 0;
}



int mod_register(cc_module *module)
{
    debug(225, 1)("mod_httpAclExtend --> module init\n");
    strcpy(module->version, "7.0.R.16488.i686");

    cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
            module->slot, 
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
            hookFuncHAETryParseParam);

    cc_register_hook_handler(HPIDX_hook_func_http_before_redirect,
            module->slot,
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_before_redirect),
            hookFuncHAEFixRequestHeader);

    cc_register_hook_handler(HPIDX_hook_func_private_process_send_header,
            module->slot,
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_process_send_header),
            hookFuncHAEFixClientVersion);

    cc_register_hook_handler(HPIDX_hook_func_http_req_send,
            module->slot,
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_send),
            hookFuncHAEFixServerVersion);

    if(reconfigure_in_thread)
        mod = (cc_module*)cc_modules.items[module->slot];
    else 
        mod = module;
    return 0;
}
#endif
