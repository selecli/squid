#ifndef CC_FRAMEWORK_H
#define CC_FRAMEWORK_H

#include "squid.h"
#ifdef CC_FRAMEWORK
/* cc_framework.c */
void cc_print_mod_ver(int flag);
void cc_modules_cleanup();
int cc_create_modules();
void cc_set_modules_configuring();
const char *cc_private_parse_acl_access(const char *cfg_line, cc_module *mod);
void cc_parse_param_and_acl(char *tmp_line, cc_module *mod);
void cc_load_modules(char *tmp_line);
void cc_default_load_modules();
int cc_aclCheckFastRequest(aclCheck_t *ch, cc_module *mod, cc_acl_check_result *result);
void cc_client_acl_check(clientHttpRequest *http, int type);
void cc_copy_client_fd_params_server(int client_fd, int server_fd);
char* cc_removeTabAndBlank(char* tmp_line);
void free_one_mod_param(cc_mod_param* mod_param);
void cc_free_mod_param_r();

void cc_init_hook_to_module();
void cc_destroy_hook_to_module();
void set_cc_effective_modules();
void * cc_mod_param_pool_alloc();

cc_module * cc_get_next_module_by_hook(HPIDX_t hook_point_index, int *mod_index);
//aclCheck_t * cc_clientAclChecklistCreate(char* token, const clientHttpRequest * http);
void cc_parse_acl_access(char* token);
acl_access* locateAccessType(char* token,access_array* array_p);
unsigned int myHash(void *value,unsigned int size);
void cc_copy_acl_access(acl_access *src, acl_access *dst);
void cc_copy_refresh(refresh_t *src, refresh_t *dst);
refresh_t *cc_find_refresh(const char *url);
void cc_insert_domain_access(mod_domain_access *cur, cc_module *mod);
mod_domain_access *cc_get_domain_access(char* domain, cc_module *mod);
acl_access *cc_get_acl_access_by_token_and_host(char* token, char* host);
acl_access *cc_get_acl_access_by_token(char* token, const clientHttpRequest *http);
acl_access *cc_get_common_acl_access(int first_time, char* token);
#endif
#endif
