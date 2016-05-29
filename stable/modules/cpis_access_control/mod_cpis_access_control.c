/**************************************************************************
 *  Copyright (C) 2012
 *
 *  File ID:
 *  File name: mod_cpis_access_control.c
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

#define P_MOD_NAME "mod_cpis_access_control"

#define  ACCESS_SINGLE    1
#define  ACCESS_MULTIPLE  2
#define  DEBUG_NO         123

static cc_module* mod = NULL;

typedef struct _mod_cpis_control 
{
    //1. ACCESS_SINGLE   mean to set change access_count = 0 in Access once  
    //2. ACCESS_MULTIPLE mean to decrease one stop until access_count equel zero;
    uint8_t access_type;
    short access_count;
    short access_status;
}mpcCfg;

static long long g_McacppConfig_Count = 0;

static void *
mcp_ConfigParamAlloc(void)
{
    mpcCfg *cfg = xcalloc(1, sizeof(mpcCfg));
    if (cfg) g_McacppConfig_Count++;
    return cfg;
}

static int
mcp_ConfigParamFree(void *data)
{
    mpcCfg *cfg = data;
    if (cfg)
    {
        safe_free(cfg);
        g_McacppConfig_Count++;
        debug(DEBUG_NO, 2)("%s %s g_McacppConfig_Count=%lld\n",
                P_MOD_NAME, __func__, g_McacppConfig_Count);
    }
    cfg = data = NULL;
    return 0;
}

static int 
hookMcpParseConfigParam(char *cfg_line)
{
    short int p_len = sizeof("access=")-1;
    short int p_count = 0;
    short int p_type  = 0;
    short int p_status= 0;
    char *v = cfg_line ? strstr(cfg_line, " allow "): NULL;
    char *t = strtok(cfg_line, w_space);
    if(NULL == v|| NULL == t || strcmp(t, P_MOD_NAME))
    {   
        debug(DEBUG_NO, 1)("%s %s %s don't match %s line[%s]\n",
                P_MOD_NAME, __func__, t, P_MOD_NAME, cfg_line);
        return -1;
    }
    *v = '\0';
    v = NULL;
    if (NULL == (t = strtok(NULL, w_space)) || NULL == (v =  strtok(NULL, w_space)))
    {
        debug(DEBUG_NO, 1)("%s %s type=%s access_count=%s\n",
                P_MOD_NAME, __func__, t, v);
        return -1;
    }

    if (!strncasecmp(v, "access=", p_len) && strlen(v) > p_len)
        p_count = atoi(v+p_len);
    if (!strcasecmp(t, "single"))   p_type = ACCESS_SINGLE;
    if (!strcasecmp(t, "multiple")) p_type = ACCESS_MULTIPLE;

    if (0 >= p_count || 0 == p_type)
    {
        debug(DEBUG_NO, 1)("%s %s fail type=%d from %s; access_count=%d from %s\n",
                P_MOD_NAME, __func__,p_type, t,p_count,v);
        return -1;
    }

    p_len = sizeof("status=")-1;
    v =  strtok(NULL, w_space);
    if (v && !strncasecmp(v, "status=", p_len) && strlen(v) > p_len)
        p_status = atoi(v+p_len);
    if (p_status < 400 || p_status > 600) p_status = 403;
    debug(DEBUG_NO, 1)("%s %s success type=%d access_count=%d status=%d\n",
            P_MOD_NAME, __func__,p_type, p_count, p_status);
    mpcCfg *pConfig = mcp_ConfigParamAlloc(); 
    pConfig->access_type  = p_type;
    pConfig->access_count = p_count;
    pConfig->access_status= p_type;
    cc_register_mod_param(mod, pConfig, mcp_ConfigParamFree);
    return 0;
}

static int 
hookMcpCheckAccessCount(clientHttpRequest*  http)
{
    int wo_allow_do = 1;
    int fd = http->conn->fd;
    mpcCfg *pConfig = cc_get_mod_param(fd, mod);
    if (pConfig && pConfig->access_count > 0)
    {
        wo_allow_do = 0;
        if (ACCESS_SINGLE == pConfig->access_type) pConfig->access_count = 0;
        if (ACCESS_MULTIPLE == pConfig->access_type) pConfig->access_count--;
    }

    if (0 == wo_allow_do)
    {
        fd_table[fd].cc_run_state[mod->slot] = 2;
        debug(DEBUG_NO, 3)("%s %s deny access_count=%d status=%d\n",
                P_MOD_NAME, __func__,pConfig->access_count,pConfig->access_status);
    }
#if 0
    {
        http->request->flags.cachable = 0;
        http->redirect.status = pConfig->access_status;
        debug(DEBUG_NO, 3)("%s %s deny access_count=%d status=%d\n",
                P_MOD_NAME, __func__,pConfig->access_count,pConfig->access_status);
    }
#endif
    return 0;
}

static int 
hookMcpCheckResponse(clientHttpRequest* http,int* ret)
{
    int fd = http->conn->fd;
    if (fd_table[fd].cc_run_state && 2 == fd_table[fd].cc_run_state[mod->slot])
    {
        fd_table[fd].cc_run_state[mod->slot] = 1;
        ErrorState *err = errorCon(ERR_INVALID_REQ, HTTP_FORBIDDEN, http->orig_request);
        storeClientUnregister(http->sc, http->entry, http);
        http->sc = NULL;
        if (http->reply) httpReplyDestroy(http->reply);
        http->reply = NULL;
        storeUnlockObject(http->entry);
        http->log_type = LOG_TCP_DENIED;
        http->entry = clientCreateStoreEntry(http, http->request->method,
                null_request_flags);
        errorAppendEntry(http->entry, err);
        *ret = 1;
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
            hookMcpParseConfigParam);

    cc_register_hook_handler(HPIDX_hook_func_before_clientProcessRequest2,
            module->slot, 
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_before_clientProcessRequest2), 
            hookMcpCheckAccessCount);

    cc_register_hook_handler(HPIDX_hook_func_private_repl_send_start,
            module->slot, 
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_repl_send_start), 
            hookMcpCheckResponse);

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
