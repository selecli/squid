#ifndef _NGX_LSCS_CONFIG_H_INCLUDED_
#define _NGX_LSCS_CONFIG_H_INCLUDED_

typedef struct {
	void	**main_conf;
	void	**srv_conf;
} ngx_lscs_conf_ctx_t;

#define NGX_LSCS_MAIN_CONF        0x02000000
#define NGX_LSCS_SRV_CONF         0x04000000

#define NGX_LSCS_MAIN_CONF_OFFSET  offsetof(ngx_lscs_conf_ctx_t, main_conf)
#define NGX_LSCS_SRV_CONF_OFFSET   offsetof(ngx_lscs_conf_ctx_t, srv_conf)

#define ngx_lscs_get_module_main_conf(r, module)	(r)->main_conf[module.ctx_index]
#define ngx_lscs_get_module_srv_conf(r, module)  (r)->srv_conf[module.ctx_index]

#endif
