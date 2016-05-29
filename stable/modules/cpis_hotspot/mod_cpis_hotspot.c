/**************************************************************************
 *  Copyright (C) 2012
 *
 *  File ID:
 *  File name: mod_cpis_hotspot.c
 *  Summary:
 *  Description: 1. Use to Change and Control Accessing Condition 
 *  Configure: 
 *  Version: V1.0
 *  Author: Yandong.Nian
 *  Date: 2013-06-09
 *  Last Modify Time : [2013-07-27 Yandong.Nian] []
 **************************************************************************/

#include "cc_framework_api.h"
#ifdef CC_FRAMEWORK 

#define P_MOD_NAME "mod_cpis_hotspot"

#define  ACCESS_SINGLE    1
#define  ACCESS_MULTIPLE  2
#define  DEBUG_NO         123

static cc_module* mod = NULL;

typedef struct _mod_obj_hot_check_config
{
    short access_count;
}mohtcCfg;

static long long g_MobCheckConfig_Count = 0;

static void *
moh_ConfigParamAlloc(void)
{
    mohtcCfg *cfg = xcalloc(1, sizeof(mohtcCfg));
    if (cfg) g_MobCheckConfig_Count++;
    return cfg;
}

static int
moh_ConfigParamFree(void *data)
{
    mohtcCfg *cfg = data;
    if (cfg)
    {
        safe_free(cfg);
        g_MobCheckConfig_Count++;
        debug(DEBUG_NO, 2)("%s %s g_MobCheckConfig_Count=%lld\n",
                P_MOD_NAME, __func__, g_MobCheckConfig_Count);
    }
    cfg = data = NULL;
    return 0;
}

static int 
hookMohParseConfigParam(char *cfg_line)
{
    short int p_len = sizeof("hot_count=")-1;
    short int p_count = 0;
    char *v = cfg_line ? strstr(cfg_line, " allow "): NULL;
    char *t = strtok(cfg_line, w_space);
    if(NULL == v|| NULL == t || strcmp(t, P_MOD_NAME))
    {   
        debug(DEBUG_NO, 1)("%s %s %s don't match %s line[%s]\n",
                P_MOD_NAME, __func__, t, P_MOD_NAME, cfg_line);
        return -1;
    }
    *v = '\0';
    if (NULL == (v =  strtok(NULL, w_space)))
    {
        debug(DEBUG_NO, 1)("%s %s access_count is NULL\n", P_MOD_NAME, __func__);
        return -1;
    }

    if (!strncasecmp(v, "hot_count=", p_len) && strlen(v) > p_len)
        p_count = atoi(v+p_len);
    if (0 >= p_count)
    {
        debug(DEBUG_NO, 1)("%s %s fail c=%d s=%s\n", P_MOD_NAME, __func__, p_count,v);
        return -1;
    }

    debug(DEBUG_NO, 1)("%s %s success access_count=%d\n",P_MOD_NAME, __func__,p_count);
    mohtcCfg *pConfig = moh_ConfigParamAlloc(); 
    pConfig->access_count = p_count;
    cc_register_mod_param(mod, pConfig, moh_ConfigParamFree);
    return 0;
}

static int 
hookMohCheckHotSpotCount(clientHttpRequest*  http)
{
    int fd = http->conn->fd;
    mohtcCfg *pConfig = cc_get_mod_param(fd, mod);
    if (NULL == http->entry && pConfig && pConfig->access_count)
    {
        fd_table[fd].cc_run_state[mod->slot] = 2;
    }
    if (http->entry && pConfig && http->entry->refcount < pConfig->access_count)
    {
        u_short f = http->entry->refcount += 1;
        if (f == pConfig->access_count)
        {
            fd_table[fd].cc_run_state[mod->slot] = 3;
        }
        else
        {
            http->request->flags.cachable = 0;
            fd_table[fd].cc_run_state[mod->slot] = 2;
        }
        http->log_type = LOG_TCP_MISS;
        http->entry = NULL;
        debug(DEBUG_NO, 3)("%s %s %u < %d %s\n",
                P_MOD_NAME, __func__,f, pConfig->access_count,http->uri);
    }
    return 0;
}

static int 
hookMohCheckHotSpotHeader(clientHttpRequest* http)
{
    int fd = http->conn->fd;
    if (fd_table[fd].cc_run_state && 2 == fd_table[fd].cc_run_state[mod->slot])
    {
        httpHeaderDelByName(&http->reply->header, "Powered-By-ChinaCache"); 
        httpHeaderDelByName(&http->reply->header, "FlexiCache-Error"); 
        httpHeaderDelByName(&http->reply->header, "FlexiCache-Lookup"); 
        httpHeaderDelByName(&http->reply->header, "X-ChinaCache-Geo"); 
        debug(DEBUG_NO, 3)("%s %s delete [Powered-By-ChinaCache] %s\n",
                P_MOD_NAME, __func__,http->uri);
    }
    if (fd_table[fd].cc_run_state && 3 == fd_table[fd].cc_run_state[mod->slot])
    {
        mohtcCfg *pConfig = cc_get_mod_param(fd, mod);
        if (http->entry && pConfig && pConfig->access_count > 0)
            http->entry->refcount = pConfig->access_count+1;
    }
    return 0;
}

static int 
hookMohCheckHotSpotAccessLog(clientHttpRequest* http)
{
    int fd = http->conn->fd;
    if (fd_table[fd].cc_run_state && 2 == fd_table[fd].cc_run_state[mod->slot])
    {
        http->out.size = 0;
        http->log_type = 0;
        debug(DEBUG_NO, 3)("%s %s %s\n",
                P_MOD_NAME, __func__,http->uri);
    }
    return 0;
}

int mod_register(cc_module *module)
{
    debug(DEBUG_NO, 1)("(%s) ->  init: init module\n",P_MOD_NAME);

    strcpy(module->version, "7.0.R.16488.i686");

    cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
            module->slot, 
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
            hookMohParseConfigParam);

    cc_register_hook_handler(HPIDX_hook_func_http_req_process,
            module->slot, 
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_process), 
            hookMohCheckHotSpotCount);

    cc_register_hook_handler(HPIDX_hook_func_private_process_send_header,
            module->slot, 
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_process_send_header), 
            hookMohCheckHotSpotHeader);

    cc_register_hook_handler(HPIDX_hook_func_private_change_accesslog_info,
            module->slot, 
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_change_accesslog_info), 
            hookMohCheckHotSpotAccessLog);

    if(reconfigure_in_thread)
        mod = (cc_module*)cc_modules.items[module->slot];
    else
        mod = module;
    return 0;
}
#undef P_MOD_NAME
#undef ACCESS_SINGLE
#undef ACCESS_MULTIPLE
#endif 
