#include "cc_framework_api.h"
#include <stdbool.h>

#ifdef CC_FRAMEWORK

static cc_module *mod = NULL;

struct mod_config_st
{
    int flag;
};

static int hook_init()
{
    debug(225, 1)("(mod_modify_http_version) -> hook_init:----:)\n");
    return 0;
}

static int free_cfg(void *data)
{
    struct mod_config_st *tmp = NULL;
    tmp = (struct mod_config_st *)data;
    if (tmp)
        safe_free(tmp);
    return 0;
}

static int func_sys_parse_param(char *cfg_line)
{
    char *token = NULL;
    int flag = -1; 

    debug(225, 2)("(mod_modify_http_version): begin parse parameter\n");

    if (strstr(cfg_line, "allow") == NULL)
        return -1; 
    if (strstr(cfg_line, "mod_modify_http_version") == NULL)
        return -1; 

    if ((token = strtok(cfg_line, w_space)) == NULL)
        return -1;
    if ((token = strtok(NULL, w_space)) == NULL)
        return -1;

    int len = strlen(token);
    if (1 != len)
    {
        debug(225, 1)("(mod_modify_http_version): token is [ %s ]\n", token);
        abort();
    }

    flag = atoi(token);
    debug(225, 2)("(mod_modify_http_version): flag is [ %d ]\n", flag);
    if (0 != flag && 1 != flag) 
    {
        debug(225, 1)("(mod_modify_http_version): flag is [ %d ]\n", flag);
        abort();
    }

    struct mod_config_st *cfg = xmalloc(sizeof(struct mod_config_st));
    memset(cfg, 0, sizeof(struct mod_config_st));

    cfg->flag = flag;
    
    debug(225, 2)("(mod_modify_http_version): cfg->flag is [ %d ]\n", cfg->flag);

    cc_register_mod_param(mod, cfg, free_cfg);

    return 0;
}

static int hook_req_send(HttpStateData * httpState)
{
    int fd = httpState->fd;
    
    struct mod_config_st *cfg = (struct mod_config_st *)cc_get_mod_param(fd, mod);

    assert(0 == cfg->flag || 1 == cfg->flag);
    
    debug(225, 2)("(mod_modify_http_version): flag is [ %d ]\n", cfg->flag);

    httpState->flags.http11 = cfg->flag;
    
    return 0;
}

int mod_register(cc_module *module)
{
    debug(225, 1)("(mod_modify_http_version) -> mod_register:\n");

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

    //module->hook_func_http_req_send = hook_req_send;
    cc_register_hook_handler(HPIDX_hook_func_http_req_send,
            module->slot,
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_send),
            hook_req_send);

    mod = module;

    return 0;
}
#endif
