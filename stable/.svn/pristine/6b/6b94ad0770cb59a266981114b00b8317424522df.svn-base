#ifndef CC_FRAMEWORK_API_H
#define CC_FRAMEWORK_API_H

#include "squid.h"
#ifdef CC_FRAMEWORK
/*
 * FIXME:
 * 	Please help describe the function as a guidance for API user
 */
void * cc_mod_param_pool_alloc(void);
int cc_register_mod_param(cc_module *mod, void *param, FREE_CALLBACK *call_back);
void *cc_debug_get_mod_param(cc_module *mod);
void *cc_get_mod_param(int fd, cc_module *mod);
void *cc_get_mod_param_by_FwdState(FwdState *fwd, cc_module *mod);
int cc_register_mod_private_data(mod_private_data_t data_t, void *tag, void *private_data, FREE_CALLBACK *call_back, cc_module *mod);
void *cc_get_mod_private_data(mod_private_data_t data_t, void *tag, cc_module *mod);
void *cc_get_global_mod_param(cc_module *mod);

//cc_mod_param **cc_get_mod_param_array(int fd, cc_module *mod);
Array * cc_get_mod_param_array(int fd, cc_module *mod);
int cc_get_mod_param_count(int fd, cc_module *mod);
PF commHandleWrite;

void cc_register_hook_handler(HPIDX_t hook_index, int mod_slot, void ** mod_hook_ptr, void * handler);

#define OFFSET_OF(type, member) ((size_t)(char *)&((type *)0L)->member)

#define ADDR_OF_HOOK_FUNC(mod, func) (((char *)mod) + OFFSET_OF(cc_module, func))

void * cc_private_data_pool_alloc(void);
#endif
#endif
