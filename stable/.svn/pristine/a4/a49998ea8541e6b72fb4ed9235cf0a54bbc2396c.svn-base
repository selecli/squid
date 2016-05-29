#include <dlfcn.h>
#include "cc_framework_hook.h"

#ifdef CC_FRAMEWORK

#define CC_CALL_HOOK(HOOK, args...)  										\
{														\
	int i = 0;												\
	cc_module *mod = NULL;  										\
	if (fd >= 0)                                                                                             \
	{                                                                                                        \
		while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_##HOOK, &i)))  						\
		{										 			\
			if (1==mod->flags.enable && mod->hook_func_##HOOK &&  fd_table[fd].cc_run_state && fd_table[fd].cc_run_state[mod->slot])	\
			{												\
				mod->hook_func_##HOOK(args);								\
			}												\
		}													\
	}                                                                                                               \
}														\

/*********************************************************
 * call hook functions
 *
 *********************************************************/
/* sys layer
 * HOOK_SYS_INIT \ HOOK_SYS_CLEANUP 
 */
int cc_call_hook_func_sys_init()
{
	int i = 0;
	cc_module *mod = NULL;

	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_sys_init, &i)))
	{
		if (mod->hook_func_sys_init && 1== mod->flags.enable && 0 == mod->flags.init)
		{
			debug(90, 3) ("cc_call_hook_func_sys_init: mod name=%s\n", mod->name);
			mod->hook_func_sys_init();
			mod->flags.init = 1;
		}
	}
	return 1;
}

int cc_call_hook_func_sys_cleanup()
{
	int i = 0;
    cc_module *mod = NULL;

    while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_sys_cleanup, &i)))
    {    
        if (1==mod->flags.enable && mod->hook_func_sys_cleanup)
        {    
            debug(90, 3) ("cc_call_hook_func_sys_cleanup: mod name=%s\n", mod->name);
            mod->hook_func_sys_cleanup(mod);
        }    
    } 
	return 1;
}


/* fd layer
 * HOOK_FD_BYTES  \ HOOK_FD_CLOSE
 */
int cc_call_hook_func_fd_bytes(int fd, int len, unsigned int type)
{
	debug(90, 4) ("cc_call_hook_fd_write: start\n");
	CC_CALL_HOOK(fd_bytes, fd, len, type);
	return 1;
}


int cc_call_hook_func_fd_close(int fd)
{
	debug(90, 4) ("cc_call_hook_func_fd_close: start\n");
    CC_CALL_HOOK(fd_close, fd);	
	return 1;
}


/* BUF layer(origal data)
 * HOOK_BUF_RECV_FROM_S
 * THIS IS A PRIVATE AND ALWAYS HOOK FOR DISABLE_VARY
 */
int cc_call_hook_func_buf_recv_from_s(int fd, char *buf, int len)
{
	debug(90, 4) ("cc_call_hook_func_buf_recv_from_s: start\n");
	int i = 0;
	int nlen = len;
	cc_module *mod = NULL;  				
	
	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_buf_recv_from_s, &i)))
	{										 	
		if (1==mod->flags.enable && mod->hook_func_buf_recv_from_s)	
		{											
			nlen = mod->hook_func_buf_recv_from_s(fd, buf, nlen);
		}											
	}

	return nlen;
}


int cc_call_hook_func_buf_send_to_s(HttpStateData *httpState, MemBuf *mb)
{
	debug(90, 3) ("cc_call_hook_func_buf_send_to_s: start\n");
	int fd = httpState->fd;
	CC_CALL_HOOK(buf_send_to_s, httpState, mb);
	return 1;
}


/*
 * CONN_STATE layer
 * HOOK_CONN_STATE_FREE
 */
int cc_call_hook_func_conn_state_free(ConnStateData *connState)
{
	int fd = connState->fd;
	CC_CALL_HOOK(conn_state_free, connState);
	return 1;
}

int cc_call_hook_func_http_req_parsed(clientHttpRequest *http)
{
	debug(90, 4) ("cc_call_hook_func_http_req_parsed: start\n");
	int fd = http->conn->fd;
	int i = 0;                                                                                              
	int ret = 1;
	cc_module *mod = NULL;                                                                                  
	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_http_req_parsed, &i)))                                                  
	{                                                                                                       
		if (1==mod->flags.enable && mod->hook_func_http_req_parsed && NULL != fd_table[fd].cc_run_state && fd_table[fd].cc_run_state[mod->slot])      
		{                                                                                               
			ret = mod->hook_func_http_req_parsed(http);                                                            
		}                                                                                               
	}        

	return ret;
}
/* http layer
 * HOOK_HTTP_REQ_READ \ HOOK_HTTP_REQ_SEND \ HOOK_HTTP_REPL_READ_END \ HOOK_HTTP_REPL_HEADER_PROC \ HOOK_HTTP_REPL_SEND_END
 */
int cc_call_hook_func_http_req_read(clientHttpRequest *http)
{
	debug(90, 4) ("cc_call_hook_func_http_req_read: start\n");
	int fd = http->conn->fd;
	fd_table[fd].cctype = 0;//zds add at 2009-11-18 for distinct the client or server fd : 0 == client 1 == server
	CC_CALL_HOOK(http_req_read, http);
	return 1;
}

int cc_call_hook_func_http_req_read_second(clientHttpRequest *http)
{
	debug(90, 4) ("cc_call_hook_func_http_req_read: start\n");
	int fd = http->conn->fd;
	CC_CALL_HOOK(http_req_read_second, http);
	return 1;
}

int cc_call_hook_func_http_private_req_read_second(clientHttpRequest *http)
{
	int i = 0, ret = 0;
        cc_module *mod = NULL;
	int fd = http->conn->fd;

	debug(90, 4) ("cc_call_hook_func_http_private_req_read: start\n");

        if (fd >= 0)
        {
                while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_http_private_req_read_second, &i)))
                {
                        if (1==mod->flags.enable && mod->hook_func_http_private_req_read_second && NULL != fd_table[fd].cc_run_state&& fd_table[fd].cc_run_state[mod->slot])
                        {
                                ret = mod->hook_func_http_private_req_read_second(http);
                        }
                }
        }
        return ret;
}
	
int cc_call_hook_func_http_before_redirect(clientHttpRequest *http)
{
	debug(90, 4) ("cc_call_hook_func_http_before_redirect: start\n");
	int fd = http->conn->fd;
	CC_CALL_HOOK(http_before_redirect, http);
	return 1;
}

int cc_call_hook_func_http_before_redirect_read(clientHttpRequest *http)
{
	int fd = http->conn->fd;

	debug(90, 4) ("cc_call_hook_func_http_before_redirect_read: start\n");

	CC_CALL_HOOK(http_before_redirect_read, http);

	return 1;
}

int cc_call_hook_func_private_http_before_redirect(clientHttpRequest *http)
{
	int fd = http->conn->fd, ret = 0,i = 0; 

	cc_module *mod = NULL;     
	debug(90, 4) ("cc_call_hook_func_private_http_before_redirect: start\n");

	if (fd >= 0)
	{    
		while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_http_before_redirect, &i)))
		{
			if (1==mod->flags.enable && mod->hook_func_private_http_before_redirect && NULL != fd_table
					[fd].cc_run_state&& fd_table[fd].cc_run_state[mod->slot])
			{
				ret = mod->hook_func_private_http_before_redirect(http);
				return ret;
			}        
		}        
	}        
	return 0;
}

int cc_call_hook_func_http_req_send(HttpStateData *httpState)
{
	debug(90, 3) ("cc_call_hook_req_send: start\n");
	int fd = httpState->fd;
	CC_CALL_HOOK(http_req_send, httpState);
	return 1;
}


int cc_call_hook_func_http_req_free(clientHttpRequest *http)
{
	debug(90, 3) ("cc_call_hook_req_free: start\n");
	int fd = http->conn->fd;
	CC_CALL_HOOK(http_req_free, http);
	return 1;
}


int cc_call_hook_func_http_req_send_exact(HttpStateData *httpState, HttpHeader* hdr)
{
	debug(90, 3) ("cc_call_hook_req_send: start\n");
	int fd = httpState->fd;
	CC_CALL_HOOK(http_req_send_exact, httpState, hdr);
	return 1;
}

// this hook used by mod_active_compress only, added by chenqi
int cc_call_hook_func_is_active_compress(int fd, HttpStateData *httpState, size_t hdr_size, size_t *done)
{
	debug(90, 3) ("cc_call_hook_func_is_active_compress: start\n");
	int i = 0;
	cc_module *mod = NULL;  				
	if (fd >= 0)
	{
		while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_is_active_compress, &i)))
		{										 	
			if (1==mod->flags.enable && mod->hook_func_is_active_compress && NULL != fd_table[fd].cc_run_state&& fd_table[fd].cc_run_state[mod->slot])
			{
				return mod->hook_func_is_active_compress(fd, httpState, hdr_size, done);
			}
		}
	}
	return 0;
}
// added end

int cc_call_hook_func_http_repl_read_start(int fd, HttpStateData *httpState)
{
	int i = 0, ret;
	cc_module *mod = NULL;  				
	debug(90, 3) ("cc_call_hook_func_http_repl_start: start\n");

	int maxret = 0;
	if (fd >= 0)
	{
		while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_http_repl_read_start, &i)))
		{										 	
			if (1==mod->flags.enable && mod->hook_func_http_repl_read_start && NULL != fd_table[fd].cc_run_state&& fd_table[fd].cc_run_state[mod->slot])
			{
				ret = mod->hook_func_http_repl_read_start(fd, httpState);
				if(ret > maxret)
				{
					maxret = ret;
				}
			}
		}
	}
	return maxret;
}

int cc_call_hook_func_http_repl_refresh_start(int fd, HttpStateData *httpState)
{
	int i = 0, ret;
	cc_module *mod = NULL;  				
	debug(90, 3) ("cc_call_hook_func_http_repl_refresh_start: start\n");

	if (fd >= 0)
	{
		while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_http_repl_refresh_start, &i)))
		{										 	
			if (1==mod->flags.enable && mod->hook_func_http_repl_refresh_start && NULL != fd_table[fd].cc_run_state&& fd_table[fd].cc_run_state[mod->slot])
			{
				ret = mod->hook_func_http_repl_refresh_start(fd, httpState);
			}
		}
	}
	return ret;
}


int cc_call_hook_func_http_repl_cachable_check(int fd, HttpStateData *httpState)
{
	int i = 0, ret;
	cc_module *mod = NULL;  				

	debug(90, 3) ("cc_call_hook_func_http_repl_cachable_check: start\n");
	if (fd >= 0)	
	{
		while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_http_repl_cachable_check, &i)))
		{										 	
			if (1==mod->flags.enable && mod->hook_func_http_repl_cachable_check && NULL != fd_table[fd].cc_run_state&&  fd_table[fd].cc_run_state[mod->slot])
			{
				ret = mod->hook_func_http_repl_cachable_check(fd, httpState);
				if(ret > 0)
					return ret;		
			}
		}
	}
	return 1;
}


int cc_call_hook_func_http_repl_read_end(int fd, HttpStateData *httpState)
{
	debug(90, 3) ("cc_call_hook_func_http_repl_read: start\n");
	CC_CALL_HOOK(http_repl_read_end, fd, httpState);
	return 1;
}


int cc_call_hook_func_http_repl_send_start(clientHttpRequest *http)
{
	debug(90, 3) ("cc_call_hook_func_http_repl_send_start: start\n");
	int fd = http->conn->fd;
	CC_CALL_HOOK(http_repl_send_start, http);
	return 1;
}


int cc_call_hook_func_http_repl_send_end(clientHttpRequest *http)
{
	debug(90, 3) ("cc_call_hook_func_http_repl_send_end: start\n");
	int fd = http->conn->fd;
	CC_CALL_HOOK(http_repl_send_end, http);	
	return 1;
}


int	cc_call_hook_func_http_client_cache_hit(clientHttpRequest *http, int stale)
{
	int fd = http->conn->fd;
	int i = 0, ret;		
	cc_module *mod = NULL;  				
	
	debug(90, 3) ("hook_func_http_client_cache_hit: start\n");
	if (fd >= 0)	
	{
		while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_http_client_cache_hit, &i)))
		{										 	
			if (1==mod->flags.enable && mod->hook_func_http_client_cache_hit && NULL != fd_table[fd].cc_run_state&& fd_table[fd].cc_run_state[mod->slot])	
			{											
				if ((ret = mod->hook_func_http_client_cache_hit(http, stale))>=0)
					return ret;			
			}											
		}
	}
	return stale;
}

int	cc_call_hook_func_private_before_http_req_process(clientHttpRequest *http)
{
	int fd = http->conn->fd;
	int i = 0, ret=0;		
	cc_module *mod = NULL;  				
	
	debug(90, 3) ("hook_func_private_before_http_req_process: start\n");
	
	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_before_http_req_process, &i)))
	{										 	
		if (1==mod->flags.enable && mod->hook_func_private_before_http_req_process && fd_table[fd].cc_run_state[mod->slot])
		{											
			ret = mod->hook_func_private_before_http_req_process(http);
			if (ret)
				return ret;			
		}											
	}
	return 0;
}

int	cc_call_hook_func_http_req_process(clientHttpRequest *http)
{
	int fd = http->conn->fd;
	int i = 0, ret=0;		
	cc_module *mod = NULL;  				
	
	debug(90, 3) ("hook_func_http_req_process: start\n");
	if (fd >= 0)	
	{
		while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_http_req_process, &i)))
		{										 	
			if (1==mod->flags.enable && mod->hook_func_http_req_process && NULL != fd_table[fd].cc_run_state&& fd_table[fd].cc_run_state[mod->slot])
			{											
				ret = mod->hook_func_http_req_process(http);
				if (ret)
					return ret;			
			}											
		}
	}
	return 0;
}

int cc_call_hook_func_private_query_direct(int fd, request_t *request, StoreEntry* e)
{
	int i = 0, ret=0;		
	cc_module *mod = NULL;  				

	debug(90, 3) ("hook_func_private_query_direct: start\n");
	if (fd >= 0)
	{
		while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_query_direct, &i)))
		{										 	
			if (1==mod->flags.enable && mod->hook_func_private_query_direct && NULL != fd_table[fd].cc_run_state && fd_table[fd].cc_run_state[mod->slot])
			{											
				ret = mod->hook_func_private_query_direct(fd, request, e);
				if (ret)
					return ret;			
			}											
		}
	}
	return 0;
}


void cc_call_hook_func_private_fwd_complete_clean(FwdState* fwdState)
{
	int fd = fwdState->client_fd;
	CC_CALL_HOOK(private_fwd_complete_clean, fwdState);
}


void cc_call_hook_func_private_preload_change_fwdState(request_t *request, FwdState* fwdState)
{
	int fd = fwdState->client_fd;
	CC_CALL_HOOK(private_preload_change_fwdState, request, fwdState);
}

void cc_call_hook_func_private_preload_change_hostport(FwdState* fwdState, const char** host, unsigned short* port)
{
    int i = 0; 
    cc_module *mod = NULL;          
    if (fwdState && fwdState->old_fd_params.cc_run_state)
    {
        while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_preload_change_hostport, &i)))
        {                     
            if (1==mod->flags.enable && mod->hook_func_private_preload_change_hostport&& fwdState->old_fd_params.cc_run_state[mod->slot])
            {          
                mod->hook_func_private_preload_change_hostport(fwdState, host, port);     
            }          
        } 
    }
}

int cc_call_hook_func_private_add_host_ip_header_for_ca(int fd, FwdState* fwdState)
{
	int i = 0, ret = 0;
	cc_module *mod = NULL;  				

	debug(90, 3) ("cc_call_hook_private_add_host_ip_header_for_ca: start\n");

	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_add_host_ip_header_for_ca, &i)))
	{										 	
		if (1==mod->flags.enable && mod->hook_func_private_add_host_ip_header_for_ca && fd_table[fd].cc_run_state && fd_table[fd].cc_run_state[mod->slot])
		{											
			ret = mod->hook_func_private_add_host_ip_header_for_ca(fwdState);		
		}											
	}	

	return ret;
}

//int cc_call_hook_func_private_dst_ip(FwdState* fwdState)
//!!!must not use for other modules!!!
int cc_call_hook_func_private_dst_ip(const char *host, unsigned short port, int ctimeout,
                                                struct in_addr addr, unsigned char TOS, FwdState* fwdState)
{
	int i = 0, ret = 1, called = 0;
	int fd = fwdState->client_fd;
	cc_module *mod = NULL;  				

	debug(90, 3) ("cc_call_hook_private_dst_ip: start\n");

	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_dst_ip, &i)))
	{										 	
		if (1==mod->flags.enable && mod->hook_func_private_dst_ip && fd_table[fd].cc_run_state && fd_table[fd].cc_run_state[mod->slot])
		{											
			ret = mod->hook_func_private_dst_ip(host, port, ctimeout, addr, TOS, fwdState);		
			called = 1;
		}											
	}	

	if(called)
		return ret;
	else
		return 0;
}

int cc_call_hook_func_private_dst_ip2(FwdState* fwdState)
{
	int i = 0;
//	int fd = fwdState->client_fd;
	cc_module *mod = NULL;  				

	debug(90, 3) ("cc_call_hook_private_dst_ip2: start\n");

	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_dst_ip2, &i)))
	{										 	
	//	if (1==mod->flags.enable && mod->hook_func_private_dst_ip2 && fd_table[fd].cc_run_state && fd_table[fd].cc_run_state[mod->slot])
		if (1==mod->flags.enable && mod->hook_func_private_dst_ip2)
		{											
			 mod->hook_func_private_dst_ip2(fwdState);		
		}											
	}	
	return 0;
}

int	cc_call_hook_func_private_http_req_process2(clientHttpRequest *http)
{
	int fd = http->conn->fd;
	int i = 0, ret=0;
	cc_module *mod = NULL;  				
	
	debug(90, 3) ("hook_func_private_http_req_process2: start\n");
	if (fd >= 0)	
	{
		while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_http_req_process2, &i)))
		{										 	
			if (1==mod->flags.enable && mod->hook_func_private_http_req_process2 && NULL != fd_table[fd].cc_run_state&& fd_table[fd].cc_run_state[mod->slot])
			{											
				ret = mod->hook_func_private_http_req_process2(http);
				if (ret)
					return ret;			
			}											
		}
	}
	return 0;
}


int cc_call_hook_func_private_negative_ttl(HttpStateData * httpState)
{
	int fd = httpState->fd;
	int i = 0, ret = 1;
	cc_module *mod = NULL;  				

	debug(90, 3) ("cc_call_hook_private_negative_ttl: start\n");
	if (fd >= 0)
	{
		while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_negative_ttl, &i)))
		{										 	
			if (1==mod->flags.enable && mod->hook_func_private_negative_ttl && NULL != fd_table[fd].cc_run_state&&  fd_table[fd].cc_run_state[mod->slot])
			{											
				ret = mod->hook_func_private_negative_ttl(httpState);			
			}											
		}	
	}
	return ret;
}

int cc_call_hook_func_http_client_create_store_entry(clientHttpRequest *http, StoreEntry *e)
{
	int fd = http->conn->fd;
	int i=0, ret = 0;
	cc_module *mod = NULL;  				

	debug(90, 4)("cc_call_hook_func_http_client_create_store_entry: start \n");
	//CC_CALL_HOOK(http_client_create_store_entry, http, e);
	if (fd >= 0)
	{
		while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_http_client_create_store_entry, &i)))
		{
			if (1==mod->flags.enable && mod->hook_func_http_client_create_store_entry && NULL != fd_table[fd].cc_run_state && fd_table[fd].cc_run_state[mod->slot])	
			{											
				ret = mod->hook_func_http_client_create_store_entry(http,e);			
			}											
		}	
	}
	
	return ret;
}


int cc_call_hook_func_private_modify_entry_timestamp(StoreEntry* e)
{
	int i = 0;
	cc_module* mod = NULL;
	int ret = 0;
	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_modify_entry_timestamp, &i)))
	{
		if(1==mod->flags.enable && mod->hook_func_private_modify_entry_timestamp)
		{
			ret = mod->hook_func_private_modify_entry_timestamp(e);
		}
	}
	return ret;
}

#define CC_CALL_HOOK_ALWAYS(HOOK, args...)  		\
{											\
	int i = 0;								\
	cc_module *mod = NULL;  				\
	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_##HOOK, &i)))  	\
	{										 	\
		if (1==mod->flags.enable && mod->hook_func_##HOOK)	\
		{				\
			mod->hook_func_##HOOK(args);			\
		}		\
	}		\
}

void cc_call_hook_func_sys_after_parse_param()
{
	CC_CALL_HOOK_ALWAYS(sys_after_parse_param);
}

/* run hook always  */
int cc_call_hook_func_private_http_accept(int fd)
{
	debug(90, 3) ("cc_call_hook_private_http_accept: start\n");
	CC_CALL_HOOK_ALWAYS(private_http_accept, fd);
	return 1;
}


int cc_call_hook_func_private_error_page(ErrorState *err, request_t *request)
{
	debug(90, 3) ("cc_call_hook_private_error_page: start\n");
	CC_CALL_HOOK_ALWAYS(private_error_page, err, request);
	return 1;
}


refresh_t *cc_call_hook_func_private_refresh_limits(const char *url)
{
	debug(90, 3) ("cc_call_hook_private_refresh_limits: start\n");
	int i = 0;
	cc_module *mod = NULL;  				

	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_refresh_limits, &i)))
	{										 	
		if (1==mod->flags.enable && mod->hook_func_private_refresh_limits)	
		{											
			return mod->hook_func_private_refresh_limits(url);			
		}											
	}	

	return NULL;
}

int cc_call_hook_func_store_client_copy(StoreEntry *e, store_client *sc)
{
	debug(90, 3)("cc_call_hook_func_store_client_copy: start\n");
	CC_CALL_HOOK_ALWAYS(store_client_copy, e, sc);
	return 1;
}

int cc_call_hook_func_store_client_copy3(StoreEntry *e, store_client *sc)
{
	debug(90, 3)("cc_call_hook_func_store_client_copy: start\n");
	CC_CALL_HOOK_ALWAYS(store_client_copy3, e, sc);
	return 1;
}


squid_off_t cc_call_hook_func_private_storeLowestMemReaderOffset(const StoreEntry *e, int *shouldReturn)
{
	debug(90, 3)("cc_call_hook_func_private_storeLowestMemReaderOffset: start\n");
	int i = 0;        
	cc_module *mod = NULL;        

	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_storeLowestMemReaderOffset, &i)))
	{        
		if (1==mod->flags.enable && mod->hook_func_private_storeLowestMemReaderOffset)
		{        
			squid_off_t modResult = mod->hook_func_private_storeLowestMemReaderOffset(e, shouldReturn);
			if(*shouldReturn)
			{
				return modResult;
			}
		}        
	}       
	shouldReturn = 0;
	return 0;
}

/* free private data */
void cc_free_clientHttpRequest_private_data(clientHttpRequest *http)
{

	int i = 0;
	cc_private_data *p_data = NULL;
	if (!http->cc_clientHttpRequest_private_data )
		return;

	for (i=0; i<Config.cc_all_mod_num; i++)
	{
		p_data = &http->cc_clientHttpRequest_private_data[i];
		if (p_data && p_data->private_data && p_data->call_back)
			p_data->call_back(p_data->private_data);
	}
    memPoolFree(cc_private_data_pool, http->cc_clientHttpRequest_private_data);
    http->cc_clientHttpRequest_private_data = NULL;
}

void cc_free_StoreEntry_private_data(StoreEntry *e)
{
	if (!e->cc_StoreEntry_private_data) return;

	int i = 0;
	cc_private_data *p_data = NULL;
	for (i=0; i<Config.cc_all_mod_num; i++)
	{
		p_data = &e->cc_StoreEntry_private_data[i];
		if (p_data && ( p_data->private_data != NULL ) && (p_data->call_back != NULL ))
			p_data->call_back(p_data->private_data);
	}
    memPoolFree(cc_private_data_pool, e->cc_StoreEntry_private_data);
    e->cc_StoreEntry_private_data = NULL;
}


void cc_free_connStateData_private_data(ConnStateData *connState)
{
	if (!connState->cc_connStateData_private_data) return;

	int i = 0;
	cc_private_data *p_data = NULL;
	for (i=0; i<Config.cc_all_mod_num; i++)
	{
		p_data = &connState->cc_connStateData_private_data[i];
		if (p_data && p_data->private_data && p_data->call_back)
			p_data->call_back(p_data->private_data);
	}
    memPoolFree(cc_private_data_pool, connState->cc_connStateData_private_data);
    connState->cc_connStateData_private_data = NULL;
}

void cc_free_request_t_private_data(request_t *req)
{
	if (!req->cc_request_private_data) return;

	int i = 0;
	cc_private_data *p_data = NULL;
	for (i=0; i<Config.cc_all_mod_num; i++)
	{
		p_data = &req->cc_request_private_data[i];
		if (p_data && p_data->private_data && p_data->call_back)
			p_data->call_back(p_data->private_data);
	}
    memPoolFree(cc_private_data_pool, req->cc_request_private_data);
    req->cc_request_private_data = NULL;
}

void cc_free_fwdState_private_data(FwdState *fwd)
{
	if (!fwd->cc_fwdState_private_data) return;

	int i = 0;
	cc_private_data *p_data = NULL;
	for (i=0; i<Config.cc_all_mod_num; i++)
	{
		p_data = &fwd->cc_fwdState_private_data[i];
		if (p_data && p_data->private_data && p_data->call_back)
			p_data->call_back(p_data->private_data);
    }
    memPoolFree(cc_private_data_pool, fwd->cc_fwdState_private_data);
    fwd->cc_fwdState_private_data = NULL;
}

void cc_free_MemObject_private_data(MemObject *mem)
{
	if (!mem->cc_MemObject_private_data) return;

	int i = 0;
	cc_private_data *p_data = NULL;
	for (i=0; i<Config.cc_all_mod_num; i++)
	{
		p_data = &mem->cc_MemObject_private_data[i];
		if (p_data && p_data->private_data && p_data->call_back)
			p_data->call_back(p_data->private_data);
	}
	xfree(mem->cc_MemObject_private_data);
	mem->cc_MemObject_private_data = NULL;
}

//added by jiangbo.tian for limit speed
void cc_call_hook_func_private_individual_limit_speed(int fd,void *data,int *nleft, PF *resume_handler)
{
	CC_CALL_HOOK(private_individual_limit_speed,fd,data,nleft, resume_handler);
}
//added end

//added by jiangbo.tian for do not unlink

int cc_call_hook_func_private_get_suggest(SwapDir *SD)
{
	int i = 0;
	int fn;
	cc_module* mod = NULL;
	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_get_suggest, &i)))
	{

		if (1==mod->flags.enable && mod->flags.config_on && mod->hook_func_private_get_suggest)
		{
		 	fn = mod->hook_func_private_get_suggest(SD);
			return fn;
		} 	
	}
	return -1;
}


int cc_call_hook_func_private_put_suggest(SwapDir* SD,sfileno fn)
{
	int i = 0;
	cc_module* mod = NULL;

	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_put_suggest, &i)))
	{
		if (1==mod->flags.enable && mod->flags.config_on && mod->hook_func_private_put_suggest)
		 	 mod->hook_func_private_put_suggest(SD,fn);
	}
	return 0;
}


//int cc_call_hook_func_private_needUnlink(char* full_path)
int cc_call_hook_func_private_needUnlink()
{

	int i = 0;
	cc_module* mod = NULL;
	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_needUnlink, &i)))
	{
		if (1==mod->flags.enable && mod->flags.config_on && mod->hook_func_private_needUnlink)
		 	 return mod->hook_func_private_needUnlink();
	}
	return 1;
}

void cc_call_hook_func_private_setDiskErrorFlag(StoreEntry * e, int error_flag)
{

	int i = 0;
	cc_module* mod = NULL; 
	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_setDiskErrorFlag, &i)))
	{       
		if (1==mod->flags.enable && mod->flags.config_on && mod->hook_func_private_setDiskErrorFlag)
			mod->hook_func_private_setDiskErrorFlag(e, error_flag);
	}       
}

int cc_call_hook_func_private_getDiskErrorFlag(StoreEntry * e)
{

	int i = 0;
	cc_module* mod = NULL;
	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_getDiskErrorFlag, &i)))
	{
		if (1==mod->flags.enable && mod->flags.config_on && mod->hook_func_private_getDiskErrorFlag)
			return mod->hook_func_private_getDiskErrorFlag(e);
	}
	return 0;
}


//added end
//added by jiangbo.tian for customized_server_side_error_page, it is private hook function ,other module shouldn't make use of it
 
void cc_call_hook_func_private_process_error_page(clientHttpRequest *http,HttpReply *rep)
{
	int fd = http->conn->fd;
	CC_CALL_HOOK(private_process_error_page,http,rep);
	//     return 0;
}
void cc_call_hook_func_private_repl_send_start(clientHttpRequest *http,int *ret)
{
	int fd = http->conn->fd;
	CC_CALL_HOOK(private_repl_send_start,http,ret);
}

void cc_call_hook_func_private_process_miss_in_error_page(clientHttpRequest *http,int *ret)
{
	int fd = http->conn->fd;
	CC_CALL_HOOK(private_process_miss_in_error_page,http,ret);
}
int cc_call_hook_func_private_comm_set_timeout(FwdState* fwd, int* timeout)
{
	int fd = fwd->client_fd;
	CC_CALL_HOOK(private_comm_set_timeout,fwd,timeout);
}
void cc_call_hook_func_private_set_error_text( ErrorState *err,char** text)
{
	int i = 0;
	cc_module* mod = NULL;
	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_set_error_text, &i)))
	{
		if(1==mod->flags.enable && mod->hook_func_private_set_error_text)
			mod->hook_func_private_set_error_text(err,text);
	}

}

/*
void cc_call_hook_func_private_errorCon(ErrorState* err)
{
	request_t *request = err->request;

}
*/
//added end by jiangbo.tian for customized_server_side_error_page

void cc_call_hook_func_private_clientPackMoreRanges(clientHttpRequest * http, char* buf, int size)
{

	debug(90, 3)("cc_call_hook_func_private_clientPackMoreRanges: start\n");
	CC_CALL_HOOK_ALWAYS(private_clientPackMoreRanges, http, buf, size);
}


int cc_call_hook_func_limit_obj(int total)
{
	int i = 0;
	int rt = 20;
	cc_module * mod = NULL; 
	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_limit_obj, &i)))
	{       
		if (1==mod->flags.enable && mod->hook_func_limit_obj)
			rt = mod->hook_func_limit_obj();
	}       
	return rt;
	
}


/*********************************************************/
//added by hlv drag and drog
int cc_call_hook_func_private_store_client_copy_hit(clientHttpRequest * http){

	debug(90, 3)("cc_call_hook_func_private_store_client_copy_hit: start\n");
	int i = 0;
	int ret = 0;
	cc_module *mod = NULL;        
	int fd = http->conn->fd;
	if (fd >= 0)
	{
		while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_store_client_copy_hit, &i)))
		{        
			if (1==mod->flags.enable && mod->hook_func_private_store_client_copy_hit && NULL != fd_table[fd].cc_run_state&& fd_table[fd].cc_run_state[mod->slot])
			{        
				ret = mod->hook_func_private_store_client_copy_hit(http);
			}        
		}       
	}
	return ret;

}

//added by dongshu.zhou for receipt

void cc_call_hook_func_private_client_build_reply_header(clientHttpRequest *http, HttpReply *rep)
{
	int fd = http->conn->fd;
	int i = 0;
	cc_module *mod = NULL;  
	debug(90, 3) ("cc_call_hook_func_private_client_build_reply_header: start\n");
	if (http->request->method == METHOD_HEAD && fd >= 0)
	{	
		while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_client_build_reply_header, &i)))  
		{   
			if(1==mod->flags.enable && mod->hook_func_private_client_build_reply_header&&  NULL != fd_table[fd].cc_run_state && http->request->method == METHOD_HEAD)
			{            
				mod->hook_func_private_client_build_reply_header(http,rep);   
			}                                                          
		}                                                                 
		return;
	}
	CC_CALL_HOOK(private_client_build_reply_header,http,rep);
}

int cc_call_hook_func_private_client_keepalive_next_request(int fd){
	
	debug(90, 3) ("cc_call_hook_func_private_client_keepalive_next_request: start\n");
	CC_CALL_HOOK(private_client_keepalive_next_request,fd);
	return 0;
	
}

//end added by dongshu.zhou
int cc_call_hook_func_private_client_keepalive_set_timeout(int fd){
	debug(90, 3)("cc_call_hook_func_private_client_keepalive_set_timeout: start\n");
	int i = 0;
	int ret = 0;
	cc_module *mod = NULL;        

	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_client_keepalive_set_timeout, &i)))
	{        
		if (1==mod->flags.enable && mod->hook_func_private_client_keepalive_set_timeout &&  NULL != fd_table[fd].cc_run_state && fd_table[fd].cc_run_state[mod->slot])
		{        
			ret = mod->hook_func_private_client_keepalive_set_timeout(fd);
		}        
	}       
	return ret;
}

//added by for mp4
int cc_call_hook_func_private_mp4_send_more_data(clientHttpRequest * http, MemBuf *mb,char *buf, ssize_t size){
	debug(90, 3)("cc_call_hook_func_private_mp4_send_more_data: start\n");
	int i = 0;
	int ret = 0;
	cc_module *mod = NULL;        
	int fd = http->conn->fd;
	if (fd >= 0)
	{
		while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_mp4_send_more_data, &i)))
		{        
			if (1==mod->flags.enable && mod->hook_func_private_mp4_send_more_data && NULL != fd_table[fd].cc_run_state&& fd_table[fd].cc_run_state[mod->slot])
			{        
				ret = mod->hook_func_private_mp4_send_more_data(http,mb,buf,size);
			}        
		}       
	}
	return ret;

}
int cc_call_hook_func_private_mp4_read_moov_body(int fd, CommWriteStateData *state){
	debug(90, 3)("cc_call_hook_func_private_mp4_read_moov_body: start\n");
	int i = 0;
	int ret = 0;
	cc_module *mod = NULL;        
	if (fd >= 0)
	{
		while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_mp4_read_moov_body, &i)))
		{        
			if (1==mod->flags.enable && mod->hook_func_private_mp4_read_moov_body && NULL != fd_table[fd].cc_run_state&& fd_table[fd].cc_run_state[mod->slot])
			{        
				ret = mod->hook_func_private_mp4_read_moov_body(fd, state);
			}        
		}       
	}
	return ret;

}

int cc_call_hook_func_private_client_write_complete(int fd, char *bufnotused, size_t size, int errflag, void *data){
        debug(90, 3)("cc_call_hook_func_private_client_write_complete: start\n");
        int i = 0;
        int ret = 0;
        cc_module *mod = NULL;
	if (fd >= 0)
	{
		while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_client_write_complete, &i)))
		{        
			if (1==mod->flags.enable && mod->hook_func_private_client_write_complete && NULL != fd_table[fd].cc_run_state&& fd_table[fd].cc_run_state[mod->slot])
			{ 
				ret = mod->hook_func_private_client_write_complete(fd,bufnotused,size,errflag,data);
			} 
		} 
	}
	return ret;
}

int cc_call_hook_func_store_complete(StoreEntry *e)
{
        int i = 0;
        cc_module * mod = NULL;
        int ret = 0;

        debug(90, 3) ("hook_func_store_complete: start\n");
        while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_store_complete, &i)))
        {
                if (1 == mod->flags.enable /* && mod->flags.config_on */&& mod->hook_func_store_complete)
                {
                        ret = mod->hook_func_store_complete(e);
                        //if(ret >= 1)
                        //        return 1;
                }
        }
        return 0;
}

int cc_call_hook_func_invoke_handlers(StoreEntry *e)
{
        int i = 0;
        cc_module * mod = NULL;
        int ret = 0;

        debug(90, 3) ("hook_func_invoke_handlers: start\n");
        while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_invoke_handlers, &i)))
        {
                if (1 == mod->flags.enable /* && mod->flags.config_on */&& mod->hook_func_invoke_handlers)
                {
                        ret = mod->hook_func_invoke_handlers(e);
                        if(ret >= 1)
                                return 1;
                }
        }
        debug(90, 3) ("hook_func_invoke_handlers: start\n");
        return 0;
}

int cc_call_hook_func_private_sc_unregister(StoreEntry* e,clientHttpRequest* http)
{
    int i = 0;
    cc_module * mod = NULL;
    int ret = 0;
    int fd = http->conn->fd;
    debug(90, 3) ("hook_func_private_sc_unregister: start\n");
    while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_sc_unregister, &i)))
    {
        if (1 == mod->flags.enable && mod->hook_func_private_sc_unregister && NULL != fd_table[fd].cc_run_state && fd_table[fd].cc_run_state[mod->slot])
        {
            ret = mod->hook_func_private_sc_unregister(e,http);
            if(ret >= 1)
                return 1;
        }
    }
    return 0;
}

//end mp4

int cc_call_hook_func_storedir_set_max_load(int max_load)
{
	int i = 0;
	cc_module * mod = NULL;
	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_storedir_set_max_load, &i)))
	{
		if (1==mod->flags.enable && mod->flags.config_on && mod->hook_func_storedir_set_max_load)
		{
			return mod->hook_func_storedir_set_max_load(max_load);
		}
	}

	return max_load;
}


/**************added by jiangbo.tian for store_multi_url*********************************/
int cc_call_hook_func_private_clientRedirectDone(request_t **new_request,char* result,clientHttpRequest* http)
{
	int i = 0;
	int fd = http->conn->fd;
	cc_module * mod = NULL;
	if (fd >= 0)
	{
		while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_clientRedirectDone, &i)))
		{
			//if (1==mod->flags.enable /* && mod->flags.config_on*/ && mod->hook_func_private_clientRedirectDone)
			if (1==mod->flags.enable && mod->hook_func_private_clientRedirectDone && NULL != fd_table[fd].cc_run_state&& fd_table[fd].cc_run_state[mod->slot])
			{
				return mod->hook_func_private_clientRedirectDone(new_request,result,http);
			}
		}
	}
	return 0;

}

int cc_call_hook_func_private_select_swap_dir_round_robin(const StoreEntry* e)
{
	int i = 0;
	cc_module * mod = NULL;
	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_select_swap_dir_round_robin, &i)))
	{
		if (1==mod->flags.enable /* && mod->flags.config_on */&& mod->hook_func_private_select_swap_dir_round_robin)
		{
			return mod->hook_func_private_select_swap_dir_round_robin(e);
		}
	}
	return -1;

}
/**************added by jiangbo.tian for store_multi_url  ended*********************************/

int cc_call_hook_func_client_set_zcopyable(store_client *sc, int fd)
{
	int i = 0;
	cc_module * mod = NULL;
	if (fd >= 0)
	{
		while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_client_set_zcopyable, &i)))
		{
			if (1==mod->flags.enable && NULL != fd_table[fd].cc_run_state&& fd_table[fd].cc_run_state[mod->slot] && mod->hook_func_client_set_zcopyable)
			{
				if( mod->hook_func_client_set_zcopyable(sc, fd) > 0)
				{
					return 1;
				}
			}
		}
	}
	return 0;
}

int cc_call_hook_func_buf_recv_from_s_body(HttpStateData * httpState, char *buf, ssize_t len, int buffer_filled)
{
	debug(90, 3)("cc_call_hook_func_buf_recv_from_s_body: start\n");

	int i = 0;
	cc_module* mod = NULL;
	int ret = 0;
	if (httpState->fd >= 0)
	{
		while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_buf_recv_from_s_body, &i)))
		{
			if(1==mod->flags.enable && mod->hook_func_buf_recv_from_s_body && NULL != fd_table[httpState->fd].cc_run_state&& fd_table[httpState->fd].cc_run_state[mod->slot])
			{
				ret = mod->hook_func_buf_recv_from_s_body(httpState, buf, len, buffer_filled);
			}
		}
	}
	return ret;

}

int cc_call_hook_func_buf_recv_from_s_header(HttpStateData * httpState, char *buf, ssize_t len)
{
	debug(90, 3)("cc_call_hook_func_buf_recv_from_s_header: start\n");

	int i = 0;
	cc_module* mod = NULL;
	int ret = 0;
	if (httpState->fd >= 0)
	{
		while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_buf_recv_from_s_header, &i)))
		{
			if(1==mod->flags.enable && mod->hook_func_buf_recv_from_s_header && NULL != fd_table[httpState->fd].cc_run_state&& fd_table[httpState->fd].cc_run_state[mod->slot])
			{
				ret = mod->hook_func_buf_recv_from_s_header(httpState, buf, len);
			}
		}
	}
	debug(90, 3)("cc_call_hook_func_buf_recv_from_s_header: end\n");
	return ret;
}

int cc_call_hook_func_private_compress_response_body(HttpStateData * httpState, char *buf, ssize_t len, char* comp_buf, ssize_t comp_buf_len, ssize_t* comp_size,int buffer_filled)
{
	debug(90, 3)("cc_call_hook_func_private_compress_response_body: start\n");

	int i = 0;
	cc_module* mod = NULL;
	int ret = 0;
	if (httpState->fd >= 0)
	{
		while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_compress_response_body, &i)))
		{
			if(1==mod->flags.enable && mod->hook_func_private_compress_response_body && NULL != fd_table[httpState->fd].cc_run_state && fd_table[httpState->fd].cc_run_state[mod->slot])
			{
				ret = mod->hook_func_private_compress_response_body(httpState, buf, len, comp_buf,comp_buf_len,comp_size,buffer_filled);
			}
		}
	}
	return ret;

}

int cc_call_hook_func_private_end_compress_response_body(HttpStateData * httpState, char *buf, ssize_t len )
{
	debug(90, 3)("cc_call_hook_func_private_end_compress_response_body: start\n");

	int i = 0;
	cc_module* mod = NULL;
	int ret = 0;
	if (httpState->fd >= 0)
	{
		while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_end_compress_response_body, &i)))
		{
			if(1==mod->flags.enable && mod->hook_func_private_end_compress_response_body && NULL != fd_table[httpState->fd].cc_run_state&& fd_table[httpState->fd].cc_run_state[mod->slot])
			{
				ret = mod->hook_func_private_end_compress_response_body(httpState, buf, len );
			}
		}
	}
	return ret;

}

//add for tudou flv 
int cc_call_hook_func_private_reply_modify_body(HttpStateData *httpState,char *buf, ssize_t len,ssize_t alloc_size){
	debug(90, 3)("cc_call_hook_func_private_reply_modify_body start\n");
	int i = 0;
	int ret = 0;
	cc_module *mod = NULL;
	if (httpState->fd >= 0) 
	{
		while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_reply_modify_body, &i)))
		{       
			if (1==mod->flags.enable && mod->hook_func_private_reply_modify_body && NULL != fd_table[httpState->fd].cc_run_state&& fd_table[httpState->fd].cc_run_state[mod->slot])
			{       
				ret = mod->hook_func_private_reply_modify_body(httpState, buf, len ,alloc_size);
			} 
		}
	}

	if(ret > 0){
		return ret;
	}       

	return len;     

}

int cc_call_hook_func_client_access_check_done(clientHttpRequest *http)
{
        debug(90, 3)("cc_call_hook_func_client_access_check_done: start\n");
        int i = 0;
        cc_module* mod = NULL;
        int ret = 0;
	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_client_access_check_done, &i)))
        {
                if(1==mod->flags.enable && mod->hook_func_client_access_check_done && fd_table[http->conn->fd].cc_run_state[mod->slot])
                {
                        if(mod->hook_func_client_access_check_done(http) > 0)
                        {
                                ret = 1;
                        }
                }
        }


        return ret;
}


int cc_call_hook_func_private_clientCacheHit_etag_mismatch( clientHttpRequest* http )
{

	int i = 0;
	cc_module * mod = NULL;
	int fd = http->conn->fd;
	debug(90, 3)("cc_call_hook_func_private_clientCacheHit_etag_mismatch start\n");
	if (fd >= 0)
	{
		while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_clientCacheHit_etag_mismatch, &i)))
		{
			if (1==mod->flags.enable && mod->hook_func_private_clientCacheHit_etag_mismatch && NULL != fd_table[fd].cc_run_state&& fd_table[fd].cc_run_state[mod->slot])
			{
				if( mod->hook_func_private_clientCacheHit_etag_mismatch( http ) > 0)
				{
					return 1;
				}
			}
		}
	}
	return 0;

}
void cc_call_hook_func_client_cache_hit_start(int fd, HttpReply *reply, StoreEntry *e)
{
	debug(90,3)("cc_call_hook_func_client_cache_hit_start start \n");
	CC_CALL_HOOK(client_cache_hit_start, fd, reply, e);
}


void cc_call_hook_func_before_sys_init()
{
	CC_CALL_HOOK_ALWAYS(before_sys_init);
}
void cc_call_hook_func_client_handle_ims_reply(clientHttpRequest *http)
{
	int fd = http->conn->fd;
	CC_CALL_HOOK(client_handle_ims_reply, http);
}

void cc_call_hook_func_before_client_acl_check(clientHttpRequest *http)
{
	//int fd= http->conn->fd;
	int i=0;
	cc_module* mod = NULL;
	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_before_client_acl_check, &i)))
	{
		if(1==mod->flags.enable && mod->hook_func_before_client_acl_check)
		{
			mod->hook_func_before_client_acl_check(http);
		}

	}
	//CC_CALL_HOOK(before_client_acl_check,http);
}

int cc_call_hook_func_private_aclMatchRegex(acl* ae,request_t *r)
{
	int i =0;
	cc_module* mod=NULL;
	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_aclMatchRegex, &i)))
	{
		if(1==mod->flags.enable && mod->hook_func_private_aclMatchRegex  )
		{

			return mod->hook_func_private_aclMatchRegex(ae,r);
		}

	}
	return 0;
}

int cc_call_hook_func_private_before_loop(allow_t *allow,aclCheck_t* checklist,acl_access* A,cc_acl_check_result *result, int type,cc_module *mod_p)
{
	int i=0;
	cc_module* mod=NULL;
	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_before_loop, &i)))
	{
		if(1==mod->flags.enable && mod->hook_func_private_before_loop )
		{
			return mod->hook_func_private_before_loop(allow,checklist,A,result,type,mod_p);
		}
	}
	return 0;
}
int cc_call_hook_func_private_before_acl_callback(allow_t allow,aclCheck_t* checklist,acl_access* aa)
{
	int i=0;
	cc_module* mod=NULL;
	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_before_acl_callback, &i)))
	{
		if(1==mod->flags.enable && mod->hook_func_private_before_acl_callback )
		{
			return mod->hook_func_private_before_acl_callback(allow,checklist,aa);
		}
	}
	return 0;
}
int cc_call_hook_func_private_cc_aclCheckFastRequestDone(aclCheck_t* checklist,cc_module *mod_p,acl_access* A, cc_acl_check_result* result,int type)
{

	int i=0;
	cc_module* mod=NULL;
	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_cc_aclCheckFastRequestDone, &i)))
	{
		if(1==mod->flags.enable && mod->hook_func_private_cc_aclCheckFastRequestDone )
		{
			return mod->hook_func_private_cc_aclCheckFastRequestDone(checklist,mod_p ,A, result, type);
		}
	}
	return 0;

}

void cc_call_hook_func_client_process_hit_start(clientHttpRequest* http)
{
	int fd = http->conn->fd;
	CC_CALL_HOOK(client_process_hit_start, http);
}

void cc_call_hook_func_client_process_hit_after_304_send(clientHttpRequest* http)
{
	int fd = http->conn->fd;
	CC_CALL_HOOK(client_process_hit_after_304_send, http);
}

void cc_call_hook_func_client_process_hit_end(clientHttpRequest* http)
{
	int fd = http->conn->fd;
	CC_CALL_HOOK(client_process_hit_end, http);
}

int cc_call_hook_func_get_client_area_use_ip(clientHttpRequest *http)
{
        int fd = http->conn->fd;

        debug(90, 3) ("cc_call_hook_func_get_client_ip: start\n");

        CC_CALL_HOOK(get_client_area_use_ip, http);

        return 1;
}

void cc_call_hook_func_refresh_staleness(StoreEntry *entry)
{
	CC_CALL_HOOK_ALWAYS(refresh_staleness, entry)
}

int cc_call_hook_func_refresh_check_pre_return(time_t age, time_t delta, request_t *request, StoreEntry *entry, time_t *cc_age)
{
	int i = 0;
	cc_module* mod=NULL;
	while(mod = cc_get_next_module_by_hook(HPIDX_hook_func_refresh_check_pre_return, &i))
	{
		if(mod->flags.enable && mod->hook_func_refresh_check_pre_return)
		{
			int ret = mod->hook_func_refresh_check_pre_return(age, delta, request, entry, cc_age);
			if(ret >= 0)
			{
				return ret;
			}
		}
	}
	return -1;
}

void cc_call_hook_func_refresh_check_set_expires(StoreEntry *entry, time_t cc_age, const refresh_t *R)
{
	CC_CALL_HOOK_ALWAYS(refresh_check_set_expires, entry, cc_age, R);
}
//hook point used for helper`s queue overflow problem
void cc_call_hook_func_private_register_deferfunc_for_helper(helper * hlp)
{
	debug(90, 3) ("cc_call_hook_func_register_deferfunc_for_helper: start\n");
	CC_CALL_HOOK_ALWAYS(private_register_deferfunc_for_helper, hlp);
}
//hook point executed when the modules reconfigure 
void  cc_call_hook_func_module_reconfigure(int redirect_should_reload)
{
	debug(90, 3) ("cc_call_hook_func_module_reconfigure : start\n");
	CC_CALL_HOOK_ALWAYS(module_reconfigure, redirect_should_reload);
}
//hook point in func httpAcceptDefer
int cc_call_hook_func_is_defer_accept_req()
{
	int i = 0;								
	cc_module *mod = NULL;  				
	int ret = 0;
	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_is_defer_accept_req, &i)))  	
	{										 	
		if (1==mod->flags.enable && mod->hook_func_is_defer_accept_req)	
		{				
			ret = mod->hook_func_is_defer_accept_req();			
		}		
	}		
	return ret;
}
//hook point in func clientReadDefer
int cc_call_hook_func_is_defer_read_req()
{
	int i = 0;								
	cc_module *mod = NULL;  				
	int ret = 0;
	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_is_defer_read_req, &i)))  	
	{										 	
		if (1==mod->flags.enable && mod->hook_func_is_defer_read_req)	
		{				
			ret = mod->hook_func_is_defer_read_req();			
		}		
	}		
	return ret;
}

int cc_call_hook_func_private_set_client_keepalive(clientHttpRequest *http)
{
	int i = 0;                                                              
	cc_module *mod = NULL;                                  
	int ret = 0;
	int fd = http->conn->fd;
	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_set_client_keepalive, &i)))    
	{                                                                                       
		if (1==mod->flags.enable && mod->hook_func_private_set_client_keepalive &&  NULL != fd_table[fd].cc_run_state && fd_table[fd].cc_run_state[mod->slot])    
		{                               
			ret = mod->hook_func_private_set_client_keepalive(http);                        
		}               
	}               
	return ret;
}


void cc_call_hook_func_fwd_start(FwdState * fwd)
{
	int fd = fwd->client_fd;
	CC_CALL_HOOK(fwd_start, fwd);
}

int cc_call_hook_func_process_modified_since(StoreEntry * entry, request_t * request)
{
	int i = 0;
	cc_module* mod = NULL;

    while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_process_modified_since, &i)))     
    {                  
        if (1==mod->flags.enable && mod->hook_func_process_modified_since)  
            return mod->hook_func_process_modified_since(entry,request);     
    }           
    return 0;



    /*
	int ret = 0;
	int off = (int)((char *)&((clientHttpRequest *)0)->request);
	clientHttpRequest *http = (clientHttpRequest *)(request - off);
	if( !http || !http->conn )
	{
		debug(90,1)("get http error\n");
		return 0;
	}
	int fd = http->conn->fd;
	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_process_modified_since, &i)))
	{
		if(1==mod->flags.enable && mod->hook_func_process_modified_since && NULL != fd_table[fd].cc_run_state && fd_table[fd].cc_run_state[mod->slot])
		{
			ret = mod->hook_func_process_modified_since(entry,request);
		}
	}
	return ret;
    */
}

int cc_call_hook_func_private_record_trend_access_log(clientHttpRequest *http)
{
	int fd = http->conn->fd;
	CC_CALL_HOOK(private_record_trend_access_log,http);
	return 0;

}

int cc_call_hook_func_private_360_write_receive_warning_log(HttpStateData *http,int len)
{
	int fd = http->fwd->client_fd;
	CC_CALL_HOOK(private_360_write_receive_warning_log,http,len);
	return 0;
}

int cc_call_hook_func_private_360_write_send_warning_log(clientHttpRequest *http)
{
	int fd = http->conn->fd;
	CC_CALL_HOOK(private_360_write_send_warning_log,http);
	return 0;
}


int cc_call_hook_func_can_find_vary()
{
	int i = 0;
	cc_module *mod = NULL;
	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_can_find_vary, &i)))
	{
		if(1==mod->flags.enable && mod->hook_func_can_find_vary)
		{
			return mod->hook_func_can_find_vary();
		}
	}
	return 1;
}

int cc_call_hook_func_private_set_tproxy_ip(FwdState* fwdState)
{
	int fd = fwdState->client_fd;
	cc_module *mod = NULL;
	int i=0;
	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_set_tproxy_ip, &i)))
	{
	 	if (1==mod->flags.enable && mod->hook_func_private_set_tproxy_ip && NULL != fd_table[fd].cc_run_state && fd_table[fd].cc_run_state[mod->slot])
		{
			if( mod->hook_func_private_set_tproxy_ip(fwdState) > 0)
			{
				return 1;
			}
		}
	}
	return 0;
}

int cc_call_hook_func_is_webluker_set()
{
	int i = 0;
	cc_module *mod = NULL;
	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_is_webluker_set, &i)))
	{
		if(1==mod->flags.enable && mod->hook_func_is_webluker_set)
		{
			return mod->hook_func_is_webluker_set();
		}                                                                                       
	}
	return 0;
} 

int cc_call_hook_func_fc_reply_header(clientHttpRequest *http, HttpHeader *hdr)
{
	int i = 0;
	cc_module *mod = NULL;
	int fd = http->conn->fd;

	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_fc_reply_header, &i)))
	{
		if (1==mod->flags.enable && mod->hook_func_fc_reply_header && NULL != fd_table[fd].cc_run_state && fd_table[fd].cc_run_state[mod->slot])
		{
			mod->hook_func_fc_reply_header(http,hdr);
			return 1;
		}
	}
	return 0;
}

/* added for partition store*/
int cc_call_hook_func_before_clientProcessRequest2(clientHttpRequest *http)
{
	int fd = http->conn->fd;
	CC_CALL_HOOK(before_clientProcessRequest2, http);
	return 0;
}

int cc_call_hook_func_private_checkModPartitionStoreEnable(clientHttpRequest *http)
{
	int fd = http->conn->fd;
	int i = 0;
	cc_module* mod=NULL;
	if (fd >= 0)
	{
		while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_checkModPartitionStoreEnable, &i)))
		{
			if(1==mod->flags.enable && mod->hook_func_private_checkModPartitionStoreEnable && NULL != fd_table[fd].cc_run_state&& fd_table[fd].cc_run_state[mod->slot])
			{
				return mod->hook_func_private_checkModPartitionStoreEnable(http);
			}
		}
	}
	return 0;
}

void cc_call_hook_func_private_getReplyUpdateByIMSRep(clientHttpRequest *http)
{
	int fd = http->conn->fd;
	CC_CALL_HOOK(private_getReplyUpdateByIMSRep, http);
}

int cc_call_hook_func_storeKeyPublicByRequestMethod(SQUID_MD5_CTX *M ,request_t *request)
{

	int i=0;
	cc_module* mod=NULL;
	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_storeKeyPublicByRequestMethod, &i)))
	{
		if(1==mod->flags.enable && mod->hook_func_storeKeyPublicByRequestMethod )
		{
			return mod->hook_func_storeKeyPublicByRequestMethod(M,request);
		}
	}
	return 0;
}

int cc_call_hook_func_httpHdrRangeOffsetLimit(request_t *request,int *ret)
{

	int i=0;
	cc_module* mod=NULL;
	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_httpHdrRangeOffsetLimit, &i)))
	{
		if(1==mod->flags.enable && mod->hook_func_httpHdrRangeOffsetLimit )
		{
			return mod->hook_func_httpHdrRangeOffsetLimit(request,ret);
		}
	}
	return 0;
}

int cc_call_hook_func_private_process_range_header(request_t *request, HttpHeader *hdr_out, StoreEntry *e)
{

	int i=0;
	cc_module* mod=NULL;
	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_process_range_header, &i)))
	{
		if(1==mod->flags.enable && mod->hook_func_private_process_range_header)
		{
			return mod->hook_func_private_process_range_header(request, hdr_out, e);
		}
	}
	return 0;
}

//add by Haobo.Zhang
int cc_call_hook_func_http_check_request_change(MemBuf *mb, HttpStateData * httpState)
{
	int i = 0;
	cc_module* mod = NULL;
	int fd = httpState->fd;
	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_http_check_request_change, &i)))
	{
		if(1==mod->flags.enable && mod->hook_func_http_check_request_change&&  NULL != fd_table[fd].cc_run_state && fd_table[fd].cc_run_state[mod->slot])
		{
			return mod->hook_func_http_check_request_change(mb, httpState);
		}
	}
	return 0;
}

int cc_call_hook_func_private_process_send_header(clientHttpRequest* http)
{
	int fd = http->conn->fd;
	CC_CALL_HOOK(private_process_send_header, http);
	return 0;

}

int cc_call_hook_func_private_before_send_header(clientHttpRequest *http,MemBuf *mb)
{

	int fd = http->conn->fd;
	int i = 0, ret = 0;
	cc_module* mod=NULL;
	if (fd >= 0)
	{
		while (!ret && (mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_before_send_header, &i)))
		{
			if(1==mod->flags.enable && mod->hook_func_private_before_send_header&& NULL != fd_table[fd].cc_run_state && fd_table[fd].cc_run_state[mod->slot])
			{
				ret = mod->hook_func_private_before_send_header(http,mb);
			}
		}
	}
    return ret;
}

int cc_call_hook_func_private_http_repl_send_end(clientHttpRequest *http)
{
    int i = 0, ret = 0;
	cc_module* mod=NULL;
	int fd = http->conn->fd;
	if (fd >= 0)
	{
		while (!ret && (mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_http_repl_send_end, &i)))
		{
			if(1==mod->flags.enable && mod->hook_func_private_http_repl_send_end && NULL != fd_table[fd].cc_run_state &&  fd_table[fd].cc_run_state[mod->slot])
			{
                ret = mod->hook_func_private_http_repl_send_end(http);
			}
		}
	}
	return ret;

}

int cc_call_hook_func_private_add_http_orig_range(clientHttpRequest *http)
{
	int fd = http->conn->fd;
	CC_CALL_HOOK(private_add_http_orig_range, http);
	return 0;
}

int  cc_call_hook_func_private_store_partial_content(HttpStateData *httpState)
{
	int fd = httpState->fd;
	int i = 0;
	cc_module* mod=NULL;
	if (fd >= 0)
	{
		while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_store_partial_content, &i)))
		{
			if(1==mod->flags.enable && mod->hook_func_private_store_partial_content && NULL != fd_table[fd].cc_run_state && fd_table[fd].cc_run_state[mod->slot])
			{
				return mod->hook_func_private_store_partial_content();
			}
		}
	}
	return 0;
}

int cc_call_hook_func_private_client_cache_hit_start(clientHttpRequest *http, StoreEntry* e)
{
	int fd = http->conn->fd;
	int i = 0;
	cc_module* mod=NULL;
	if (fd >= 0)
	{
		while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_client_cache_hit_start, &i)))
		{
			if(1==mod->flags.enable && mod->hook_func_private_client_cache_hit_start && NULL != fd_table[fd].cc_run_state && fd_table[fd].cc_run_state[mod->slot])
			{
				return mod->hook_func_private_client_cache_hit_start(http,e);
			}
		}
	}
	return 0;
}

int cc_call_hook_func_private_clientBuildRangeHeader(clientHttpRequest *http)
{
	int fd = http->conn->fd;
	int i = 0;
	cc_module* mod=NULL;
	if (fd >= 0)
	{
		while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_clientBuildRangeHeader, &i)))
		{
			if(1==mod->flags.enable && mod->hook_func_private_clientBuildRangeHeader && NULL != fd_table[fd].cc_run_state && fd_table[fd].cc_run_state[mod->slot])
			{
				return mod->hook_func_private_clientBuildRangeHeader(http);
			}
		}
	}
	return 0;

}

int cc_call_hook_func_private_after_write_header(clientHttpRequest *http)
{
	debug(90, 4) ("cc_call_hook_func_private_after_write_header: start\n");
	int fd = http->conn->fd;
	int i = 0;
	cc_module* mod=NULL;
	if (fd >= 0)
	{
		while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_after_write_header, &i)))
		{
			if(1==mod->flags.enable && mod->hook_func_private_after_write_header && NULL != fd_table[fd].cc_run_state && fd_table[fd].cc_run_state[mod->slot])
			{
				return mod->hook_func_private_after_write_header(http);
			}
		}
	}
	return 0;
}

int cc_call_hook_func_httpStateFree(HttpStateData *httpState)
{

	int fd = httpState->fd;
	CC_CALL_HOOK(httpStateFree, httpState);
	return 0;
}

int cc_call_hook_func_private_store_meta_url(store_client *sc,char* new_url)
{
	//	int fd = ((clientHttpRequest*)(sc->callback_data))->conn->fd;
	CC_CALL_HOOK_ALWAYS(private_store_meta_url, sc,new_url);
	return 0;

}

int  cc_call_hook_func_private_change_request(request_t *r,clientHttpRequest *http)
{
	int fd = http->conn->fd;
	int i = 0;
	cc_module* mod=NULL;
	if (fd >= 0)
	{
		while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_change_request, &i)))
		{
			if(1==mod->flags.enable && mod->hook_func_private_change_request && NULL != fd_table[fd].cc_run_state && fd_table[fd].cc_run_state[mod->slot])
			{
				return mod->hook_func_private_change_request(r,http);
			}
		}
	}
	return 0;

}

int cc_call_hook_func_private_should_close_fd(clientHttpRequest *http)
{

	int fd = http->conn->fd;
	int i = 0;
	cc_module* mod=NULL;
	if (fd >= 0)
	{
		while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_should_close_fd, &i)))
		{
			if(1==mod->flags.enable && mod->hook_func_private_should_close_fd && NULL != fd_table[fd].cc_run_state && fd_table[fd].cc_run_state[mod->slot])
			{
				return mod->hook_func_private_should_close_fd(http);
			}
		}
	}
	return 1;
}

void cc_call_hook_func_private_set_status_line(clientHttpRequest *http,HttpReply *rep)
{
	int fd = http->conn->fd;
	int i = 0;
	cc_module* mod=NULL;
	if (fd >= 0)
	{
		while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_set_status_line, &i)))
		{
			if(1==mod->flags.enable && mod->hook_func_private_set_status_line&&  NULL != fd_table[fd].cc_run_state && http->request->method == METHOD_HEAD)
			{
				mod->hook_func_private_set_status_line(http,rep);
			}
			else if (1==mod->flags.enable && mod->hook_func_private_set_status_line&&  NULL != fd_table[fd].cc_run_state && fd_table[fd].cc_run_state[mod->slot])
			{
				mod->hook_func_private_set_status_line(http,rep);
			}
		}
	}
}

void cc_call_hook_func_private_change_accesslog_info(clientHttpRequest *http)
{
	int fd = http->conn->fd;	
	debug(90, 4) ("cc_call_hook_func_private_change_accesslog_info: start\n");
	CC_CALL_HOOK(private_change_accesslog_info, http);
}

int cc_call_hook_func_http_req_read_three(clientHttpRequest *http)
{
	int fd = http->conn->fd;

	debug(90, 4) ("cc_call_hook_func_http_req_read_three: start\n");

	CC_CALL_HOOK(http_req_read_three, http);

	return 1;
}

int cc_call_hook_func_store_url_rewrite(clientHttpRequest *http)
{
    int fd = http->conn->fd;

    debug(90, 4) ("cc_call_hook_func_store_url_rewrite: start\n");
    int i = 0;                                              
    cc_module *mod = NULL;                                          
    if (fd >= 0)                                                                                             
    {                                                                                                        
        while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_store_url_rewrite, &i)))                          
        {                                                   
            if (1==mod->flags.enable && mod->hook_func_store_url_rewrite && NULL != fd_table[fd].cc_run_state && fd_table[fd].cc_run_state[mod->slot])

            {                                               
                return mod->hook_func_store_url_rewrite(http);                                
            }                                               
        }                                                   
    }        
    return 0;
}

int cc_call_hook_func_http_after_client_store_rewrite(clientHttpRequest *http)
{
	int fd = http->conn->fd;

	debug(90, 4) ("cc_call_hook_func_http_after_client_store_rewrite: start\n");

	CC_CALL_HOOK(http_after_client_store_rewrite, http);

	return 1;
}

int cc_call_hook_func_private_process_excess_data(HttpStateData * httpState)
{
	int fd = httpState->fwd->client_fd;
	int i = 0;
	cc_module* mod=NULL;
	if (fd >= 0)
	{
		while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_process_excess_data, &i)))
		{
			if(1==mod->flags.enable && mod->hook_func_private_process_excess_data && NULL != fd_table[fd].cc_run_state && fd_table[fd].cc_run_state[mod->slot])
			{
				return mod->hook_func_private_process_excess_data(httpState);
			}
		}
	}
	return 0;


}
void cc_call_hook_func_reply_header_mb(clientHttpRequest *http, MemBuf *mb)
{
   int fd = http->conn->fd;
   CC_CALL_HOOK(reply_header_mb, http, mb);
}
void cc_call_hook_func_transfer_done_precondition(clientHttpRequest *http, int *precondition)
{
	int fd = http->conn->fd;
	CC_CALL_HOOK(transfer_done_precondition, http, precondition);
}


int cc_call_hook_func_private_client_send_more_data(clientHttpRequest * http, MemBuf *mb,char *buf, ssize_t size){

	debug(90, 3)("cc_call_hook_func_private_client_send_more_data: start\n");
	int i = 0;
	int ret = 0;
	cc_module *mod = NULL;        
	int fd = http->conn->fd;
	if (fd >= 0)
	{
		while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_client_send_more_data, &i)))
		{        
			if (1==mod->flags.enable && mod->hook_func_private_client_send_more_data && NULL != fd_table[fd].cc_run_state && fd_table[fd].cc_run_state[mod->slot])
			{        
				ret = mod->hook_func_private_client_send_more_data(http,mb,buf,size);
			}        
		}       
	}
	return ret;

}

int call_hook_func_private_store_entry_valid_length(StoreEntry *entry){
	debug(90, 3)("cc_call_hook_func_private_store_entry_valid_length: start\n");
	int i = 0;
	int ret = 0;
	cc_module *mod = NULL;        

	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_store_entry_valid_length, &i)))
	{        
		if (1==mod->flags.enable && mod->hook_func_private_store_entry_valid_length)
		{        
			ret = mod->hook_func_private_store_entry_valid_length(entry);
		}        
	}  
	return ret;     
}

void cc_call_hook_func_http_created(clientHttpRequest *http)
{
	int fd = http->conn->fd;
	CC_CALL_HOOK_ALWAYS(http_created, http);
}

void cc_call_hook_func_httpreq_no_cache_reason(int fd, clientHttpRequest *http, NOCACHE_REASON ncr)
{
	CC_CALL_HOOK(httpreq_no_cache_reason, http, ncr);
}

void cc_call_hook_func_httprep_no_cache_reason(int fd, MemObject *mem, NOCACHE_REASON ncr)
{
	CC_CALL_HOOK(httprep_no_cache_reason, mem, ncr);
}

void cc_call_hook_func_store_no_cache_reason(StoreEntry *e, NOCACHE_REASON ncr)
{

	debug(90, 3) ("cc_call_hook_store_no_cache_reason: start\n");
	CC_CALL_HOOK_ALWAYS(store_no_cache_reason, e, ncr);
}

int cc_call_hook_func_http_req_debug_log(clientHttpRequest *http)
{
	int fd = http->conn->fd;

	debug(90, 3) ("cc_call_hook_req_debug_log: start\n");

	CC_CALL_HOOK(http_req_debug_log, http);

	return 1;
}

void cc_call_hook_func_set_epoll_load(int num, struct epoll_event *events)
{
	CC_CALL_HOOK_ALWAYS(set_epoll_load, num, events);
}

void cc_call_hook_func_clear_dir_load()
{
	CC_CALL_HOOK_ALWAYS(clear_dir_load);
}

void cc_call_hook_func_add_dir_load()
{
	CC_CALL_HOOK_ALWAYS(add_dir_load);
}

void cc_call_hook_func_set_filemap_time(int count)
{
	CC_CALL_HOOK_ALWAYS(set_filemap_time, count);
}
void cc_call_hook_func_get_http_body_buf(clientHttpRequest *http, char *buf, ssize_t size)
{
	debug(90, 3)("cc_call_hook_func_private_mp4_send_more_data: start\n");
	int i = 0;
	cc_module *mod = NULL;        
	int fd = http->conn->fd;
	if (fd >= 0)
	{
		while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_get_http_body_buf, &i)))
		{        
			if (1==mod->flags.enable && mod->hook_func_get_http_body_buf && NULL != fd_table[fd].cc_run_state&& fd_table[fd].cc_run_state[mod->slot])
			{        
				debug(249, 3)("cc_call_hook_func_private_mp4_send_more_data: executed\n");
				mod->hook_func_get_http_body_buf(http,buf,size);
			}        
		}       
	}
}

void cc_call_hook_func_clientLscsConnectionsOpen(void)
{
	CC_CALL_HOOK_ALWAYS(clientLscsConnectionsOpen);
}

//for redirect_location start
int cc_call_hook_func_private_client_cache_hit_for_redirect_location(clientHttpRequest * http){

	int fd = http->conn->fd;
	debug(90, 3)("cc_call_hook_func_private_client_cache_hit_for_redirect_location: start fd[%d]\n",fd);
	int i = 0;
	cc_module* mod=NULL;
	if (fd >= 0)
	{
		while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_client_cache_hit_for_redirect_location, &i)))
		{
			if(1==mod->flags.enable && mod->hook_func_private_client_cache_hit_for_redirect_location && NULL != fd_table[fd].cc_run_state && fd_table[fd].cc_run_state[mod->slot])
			{
				debug(90, 3)("cc_call_hook_func_private_client_cache_hit_for_redirect_location2: start fd[%d]\n",fd);
				return mod->hook_func_private_client_cache_hit_for_redirect_location(http);
			}
		}
	}
	return 1;
}

int cc_call_hook_func_private_store_meta_url_for_redirect_location(clientHttpRequest * http){

	int fd = http->conn->fd;
	debug(90, 3)("cc_call_hook_func_private_store_meta_url_for_redirect_location: start fd[%d]\n",fd);
	int i = 0;
	cc_module* mod=NULL;
	if (fd >= 0)
	{
		while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_store_meta_url_for_redirect_location, &i)))
		{
			if(1==mod->flags.enable && mod->hook_func_private_store_meta_url_for_redirect_location&& NULL != fd_table[fd].cc_run_state && fd_table[fd].cc_run_state[mod->slot])
			{
				debug(90, 3)("cc_call_hook_func_private_store_meta_url_for_redirect_location2: start fd[%d]\n",fd);
				return mod->hook_func_private_store_meta_url_for_redirect_location(http);
			}
		}
	}
	return 1;
}
//for redirect_location end

int          
cc_call_hook_func_send_header_clone_before_reply(clientHttpRequest *http, HttpReply * rep) 
{
    int fd = http->conn->fd;
    debug(90, 3) ("cc_call_hook_func_check_startvalue_lessthan_contentlength: start\n");
    int i = 0, ret = 0; 
    cc_module* mod=NULL;
    if (fd >= 0)
    {    
        while (!ret && (mod = cc_get_next_module_by_hook(HPIDX_hook_func_send_header_clone_before_reply, &i)))
        {    
            if(1==mod->flags.enable && mod->hook_func_send_header_clone_before_reply&& NULL != fd_table[fd].cc_run_state && fd_table[fd].cc_run_state[mod->slot])
              ret = mod->hook_func_send_header_clone_before_reply(http, rep);
        }    
    }    
    return ret; 
}

void cc_call_hook_func_private_getMultiRangeOneCopySize(clientHttpRequest* http, size_t copy_sz)
{
	int fd = http->conn->fd;
	CC_CALL_HOOK(private_getMultiRangeOneCopySize,http, copy_sz);
}

void cc_call_hook_func_private_getOnceSendBodySize(clientHttpRequest * http, size_t send_len)
{
	int fd = http->conn->fd;
	CC_CALL_HOOK(private_getOnceSendBodySize,http, send_len);
}

//for billing_host start
int cc_call_hook_func_private_http_before_redirect_for_billing( clientHttpRequest *http )
{
      int fd = http->conn->fd;

      debug(90, 4) ("cc_call_hook_func_private_http_before_redirect_for_billing: start\n");

      CC_CALL_HOOK(private_http_before_redirect_for_billing, http);
      return 1;
}
//for billing_host end

/******************************add for persistent connection!**********************************/
int  cc_call_hook_func_private_set_name_as_ip(FwdState *fwdState,char** name)
{
	int fd = fwdState->client_fd;
	int i = 0;
	cc_module* mod=NULL;
	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_set_name_as_ip, &i)))
	{
		if(1==mod->flags.enable && mod->hook_func_private_set_name_as_ip && NULL != fd_table[fd].cc_run_state && fd_table[fd].cc_run_state[mod->slot])
		{
			return mod->hook_func_private_set_name_as_ip(fwdState,name);
		}
	}
	return 0;
}

int cc_call_hook_func_private_openIdleConn( int idle,FwdState *fwdState,char* name,char* domain, int ctimeout)
{
	int fd = fwdState->client_fd;
	CC_CALL_HOOK(private_openIdleConn,idle,fwdState,name,domain,ctimeout);
	return 0;
}

int cc_call_hook_func_private_http_repl_read_end(int fd, HttpStateData *httpState,int *keep_alive)
{
	debug(90, 3) ("cc_call_hook_func_private_http_repl_read: start\n");
	
	CC_CALL_HOOK(private_http_repl_read_end, httpState,keep_alive);
	
	return 1;
}

int cc_call_hook_func_private_set_host_as_ip(int fd,char** host)
{
    CC_CALL_HOOK(private_set_host_as_ip,fd,host);
    return 0;
}

int cc_call_hook_func_private_dup_persist_fd(int fd, void* data)
{
    CC_CALL_HOOK(private_dup_persist_fd,data);
    return 0;

}

int cc_call_hook_func_private_set_upperlayer_persist_connection_timeout(FwdState *fwdState,int server_fd)
{
	int fd=fwdState->client_fd;
    CC_CALL_HOOK(private_set_upperlayer_persist_connection_timeout,fwdState,server_fd);
	return 0;
}



int cc_call_hook_func_private_set_sublayer_persist_connection_timeout(int client_fd)
{
	int fd=client_fd;
   	CC_CALL_HOOK(private_set_sublayer_persist_connection_timeout,client_fd);
	return 0;
}

int cc_call_hook_func_private_set_sublayer_persist_request_timeout(int client_fd)
{
	int fd=client_fd;
   	CC_CALL_HOOK_ALWAYS(private_set_sublayer_persist_request_timeout,client_fd);
	return 0;
}

int cc_call_hook_func_private_set_sublayer_tcp_keepalive(int fd)
{

	CC_CALL_HOOK_ALWAYS(private_set_sublayer_tcp_keepalive,fd);	
	return 0;
}

int cc_call_hook_func_private_post_use_persistent_connection(FwdState *fwdState)
{
	int fd=fwdState->client_fd;
	int i = 0;								
	cc_module *mod = NULL;  			
	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_post_use_persistent_connection, &i)))  	
	{										 	

		if (1==mod->flags.enable && mod->hook_func_private_post_use_persistent_connection)	
		{				

			return mod->hook_func_private_post_use_persistent_connection(fwdState);			

		}		
	}	
    return 0;
}

void  cc_call_hook_func_private_comm_connect_dns_handle(const ipcache_addrs * ia, void *data)
{

	debug(90, 3)("cc_call_hook_func_private_comm_connect_dns_handle: start\n");
	CC_CALL_HOOK_ALWAYS(private_comm_connect_dns_handle,ia, data);
}

void cc_call_hook_func_private_comm_connect_polling_serverIP(const ipcache_addrs * ia, int fd, void *data) 
{
    CC_CALL_HOOK(private_comm_connect_polling_serverIP, ia, data);
}

void cc_call_hook_func_private_UserAgent_change_port(int fd, FwdState* fwdState, unsigned short* port)
{
	CC_CALL_HOOK(private_UserAgent_change_port, fd, fwdState, port);
}
void cc_call_hook_func_coss_membuf()
{
	CC_CALL_HOOK_ALWAYS(coss_membuf);
}

void cc_call_hook_func_quick_abort_check(StoreEntry *entry, int * abort)
{
    CC_CALL_HOOK_ALWAYS(quick_abort_check, entry, abort); 
}

void cc_call_hook_func_private_assign_dir_set_private_data (HttpStateData *httpState)
{
    int fd = httpState->fwd->server_fd;
	CC_CALL_HOOK(private_assign_dir_set_private_data,httpState);
}

int cc_call_hook_func_private_assign_dir(const StoreEntry *e, char **forbid)
{       
    int ret = -1;
    CC_CALL_HOOK_ALWAYS(private_assign_dir, e, forbid, &ret);
    return ret; 
} 

int cc_call_hook_func_private_is_assign_dir(StoreEntry *e, int dirn)
{
	int i = 0;     
	cc_module *mod = NULL;     
	int ret = -1;
	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_is_assign_dir, &i)))   
	{     
		if (1==mod->flags.enable && mod->hook_func_private_is_assign_dir)   
		{
			ret = mod->hook_func_private_is_assign_dir(e, dirn);
		}      
	}      
	return ret;
}

int cc_call_hook_func_support_complex_range(clientHttpRequest* http)
{
	int fd = http->conn->fd , supp = 0; 
	CC_CALL_HOOK(support_complex_range, http, &supp);
	return supp;
}

int cc_call_hook_func_reply_cannot_find_date(const HttpReply * rep)
{
	int ret, i;
	cc_module *mod = NULL;
	ret = 0;
	i = 0;
	while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_reply_cannot_find_date, &i)))
	{
		if (1==mod->flags.enable && mod->hook_func_reply_cannot_find_date)
		{
			ret = mod->hook_func_reply_cannot_find_date(rep);
		}
	}
	return ret;
}

int cc_call_hook_func_get_content_orig_down(clientHttpRequest * http, http_status status)
{
	int fd = http->conn->fd;
	int i = 0; 
	int ret = 0; 
	cc_module *mod = NULL; 
	if (fd >= 0) 
	{    
		while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_get_content_orig_down, &i)))
		{    
			if (1==mod->flags.enable && mod->hook_func_get_content_orig_down &&  fd_table[fd].cc_run_state && fd_table[fd].cc_run_state[mod->slot])
			{    
				ret = mod->hook_func_get_content_orig_down(http, status);
			}
		}
	}
	return ret;
}
int cc_call_hook_func_store_append_check(HttpStateData *httpState)
{
	int i = 0;
	cc_module *mod = NULL;
	int fd = httpState->fwd->client_fd;

	debug(90, 3) ("cc_call_hook_func_store_append_check: start\n");

	if (fd >= 0)
	{
		while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_store_append_check, &i)))
		{
			if(1 == mod->flags.enable &&
					mod->hook_func_store_append_check &&
					NULL != fd_table[fd].cc_run_state &&
					fd_table[fd].cc_run_state[mod->slot])
			{
				return mod->hook_func_store_append_check(httpState);
			}
		}
	}

	return 1;
}

int cc_call_hook_func_private_reforwardable(FwdState *fwdState)
{
	int i = 0;
	cc_module *mod = NULL;
	int fd = fwdState->client_fd;

	debug(90, 3) ("cc_call_hook_func_private_reforwardable: start\n");

	if (fd >= 0)
	{
		while ((mod = cc_get_next_module_by_hook(HPIDX_hook_func_private_reforwardable, &i)))
		{
			if(1 == mod->flags.enable &&
					mod->hook_func_private_reforwardable &&
					NULL != fd_table[fd].cc_run_state &&
					fd_table[fd].cc_run_state[mod->slot])
			{
				return mod->hook_func_private_reforwardable(fwdState);
			}
		}
	}

	return 0;
}

#endif
