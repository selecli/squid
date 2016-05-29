#ifndef CC_FRAMEWORK_HOOK_H
#define CC_FRAMEWORK_HOOK_H

#include "squid.h"

/*
* XXX
* 如果能用文本画张流程点与HOOK点的图就完美??? * 听说有这种用文本画图的软件，多见用于聊天与设计BBS的签名档
*/
int cc_call_hook_func_sys_init();
int cc_call_hook_func_sys_cleanup();

int cc_call_hook_func_fd_bytes(int fd, int len, unsigned int type);
int cc_call_hook_func_fd_close(int fd);

int cc_call_hook_func_buf_recv_from_s(int fd, char *buf, int len);
int cc_call_hook_func_buf_send_to_s(HttpStateData *httpState, MemBuf *mb);

int cc_call_hook_func_http_req_parsed(clientHttpRequest *http);
int cc_call_hook_func_http_req_read(clientHttpRequest *http);
int cc_call_hook_func_http_req_read_second(clientHttpRequest *http);
int cc_call_hook_func_http_req_send(HttpStateData *httpState);
int cc_call_hook_func_http_req_process(clientHttpRequest *http);
//add by zh for hotspot mod
int cc_call_hook_func_private_before_http_req_process(clientHttpRequest *http);
//add end
int cc_call_hook_func_http_req_free(clientHttpRequest *http);

int cc_call_hook_func_http_req_send_exact(HttpStateData *httpState, HttpHeader *hdr);
int cc_call_hook_func_http_repl_read_start(int fd, HttpStateData *httpState);
int cc_call_hook_func_http_repl_refresh_start(int fd, HttpStateData *httpState);
int cc_call_hook_func_http_repl_read_end(int fd, HttpStateData *httpState);
int cc_call_hook_func_http_repl_send_start(clientHttpRequest *http);
int cc_call_hook_func_http_repl_send_end(clientHttpRequest *http);
int cc_call_hook_func_http_client_cache_hit(clientHttpRequest *http, int stale);

int cc_call_hook_func_http_client_create_store_entry(clientHttpRequest *http, StoreEntry *e);
int cc_call_hook_func_store_client_copy(StoreEntry *e, store_client *sc);
int cc_call_hook_func_store_client_copy3(StoreEntry *e, store_client *sc);

void cc_call_hook_func_private_preload_change_fwdState(request_t *request, FwdState* fwdState);
void cc_call_hook_func_private_preload_change_hostport(FwdState* fwdState, const char** host, unsigned short* port); 
int cc_call_hook_func_private_negative_ttl(HttpStateData *httpState);
int cc_call_hook_func_private_http_accept(int fd);
int cc_call_hook_func_private_error_page(ErrorState *err, request_t *request);
void cc_call_hook_func_private_process_error_page(clientHttpRequest *http,HttpReply *rep);
void cc_call_hook_func_private_repl_send_start(clientHttpRequest *http,int *ret);
void cc_call_hook_func_private_process_miss_in_error_page(clientHttpRequest *http,int *ret);
void cc_call_hook_func_private_set_error_text( ErrorState *err,char** text);
int cc_call_hook_func_private_comm_set_timeout(FwdState* fwd, int* timeout);
int cc_call_hook_func_private_http_req_process2(clientHttpRequest *http);
int cc_call_hook_func_private_modify_entry_timestamp(StoreEntry* e);
refresh_t *cc_call_hook_func_private_refresh_limits(const char *url);
squid_off_t cc_call_hook_func_private_storeLowestMemReaderOffset(const StoreEntry *e, int *shouldReturn);
void cc_call_hook_func_private_clientPackMoreRanges(clientHttpRequest * http, char* buf, int size);

void cc_free_clientHttpRequest_private_data(clientHttpRequest *http);
void cc_free_StoreEntry_private_data(StoreEntry *e);
void cc_free_fwdState_private_data(FwdState *fwd);

void cc_call_hook_func_private_individual_limit_speed(int fd,void *data,int *nleft, PF *resume_handler);
void cc_call_hook_func_after_parse_param();
void cc_call_hook_func_private_client_build_reply_header(clientHttpRequest *http, HttpReply *rep);
int cc_call_hook_func_private_store_client_copy_hit(clientHttpRequest * http);
int cc_call_hook_func_private_client_keepalive_next_request(int fd);
int cc_call_hook_func_limit_obj(int total);
int cc_call_hook_func_conn_state_free(ConnStateData *connState);
void cc_free_connStateData_private_data(ConnStateData *connState);
void cc_free_request_t_private_data(request_t *req);
int cc_call_hook_func_http_repl_cachable_check(int fd, HttpStateData *httpState);
void cc_call_hook_func_sys_after_parse_param();
int cc_call_hook_func_storedir_set_max_load(int max_load);
//added by jiangbo.tian for do_not_unlink
int cc_call_hook_func_private_get_suggest(SwapDir *SD);
int cc_call_hook_func_private_put_suggest(SwapDir* SD,sfileno fn);
int cc_call_hook_func_private_needUnlink();
void cc_call_hook_func_private_setDiskErrorFlag(StoreEntry *e, int error_flag);
int cc_call_hook_func_private_getDiskErrorFlag(StoreEntry *e);
//added end
//added for mp4
int cc_call_hook_func_private_client_write_complete(int fd, char *bufnotused, size_t size, int errflag, void *data);
int cc_call_hook_func_private_mp4_send_more_data(clientHttpRequest * http, MemBuf *mb,char *buf, ssize_t size);
int cc_call_hook_func_private_mp4_read_moov_body(int fd, CommWriteStateData *state);

int cc_call_hook_func_store_complete(StoreEntry *e);
int cc_call_hook_func_invoke_handlers(StoreEntry *e);
int cc_call_hook_func_private_sc_unregister(StoreEntry* e,clientHttpRequest* http);
//end mp4
int cc_call_hook_func_http_before_redirect(clientHttpRequest *http);
int cc_call_hook_func_http_before_redirect_read(clientHttpRequest *http);
//added for store_multi_url by jiangbo.tian
int cc_call_hook_func_private_clientRedirectDone(request_t **new_request,char* result,clientHttpRequest* http);
int cc_call_hook_func_private_select_swap_dir_round_robin(const StoreEntry* e);
//added end
int cc_call_hook_func_client_set_zcopyable(store_client *sc, int fd);
int cc_call_hook_func_buf_recv_from_s_body(HttpStateData * httpState, char *buf, ssize_t len, int buffer_filled);
int cc_call_hook_func_buf_recv_from_s_header(HttpStateData * httpState, char *buf, ssize_t len);

int cc_call_hook_func_private_reply_modify_body(HttpStateData *httpState,char *buf, ssize_t len,ssize_t alloc_size);
int cc_call_hook_func_private_clientCacheHit_etag_mismatch( clientHttpRequest * http );
void cc_call_hook_func_client_cache_hit_start(int fd, HttpReply *reply, StoreEntry *e);

int cc_call_hook_func_private_compress_response_body(HttpStateData * httpState, char *buf, ssize_t len, char* comp_buf,ssize_t comp_buf_len, ssize_t *comp_size, int buffer_filled);
int cc_call_hook_func_private_end_compress_response_body(HttpStateData * httpState, char *buf, ssize_t len );

void cc_call_hook_func_before_sys_init();
void cc_call_hook_func_client_handle_ims_reply(clientHttpRequest *http);
//added for acl_optimize
void cc_call_hook_func_before_client_acl_check(clientHttpRequest *http);
int cc_call_hook_func_private_aclMatchRegex(acl* ae,request_t *r);
//added end
//added for 304
void cc_call_hook_func_client_process_hit_start(clientHttpRequest* http);
void cc_call_hook_func_client_process_hit_after_304_send(clientHttpRequest* http);
void cc_call_hook_func_client_process_hit_end(clientHttpRequest* http);
void cc_call_hook_func_refresh_staleness(StoreEntry *entry);
int cc_call_hook_func_refresh_check_pre_return(time_t age, time_t delta, request_t *request, StoreEntry *entry, time_t *cc_age);
void cc_call_hook_func_refresh_check_set_expires(StoreEntry *entry, time_t cc_age, const refresh_t *R);
//added end

//added for trend_receipt
int cc_call_hook_func_private_record_trend_access_log(clientHttpRequest *http);
//added end


//added for 360 warning log
int cc_call_hook_func_private_360_write_receive_warning_log(HttpStateData *httpState,int len);
int cc_call_hook_func_private_360_write_send_warning_log(clientHttpRequest *http);
//added end




//added for partition_store
int cc_call_hook_func_before_clientProcessRequest2(clientHttpRequest *http);

int cc_call_hook_func_http_repl_send_end(clientHttpRequest *http);

int cc_call_hook_func_private_process_send_header(clientHttpRequest* http);

int cc_call_hook_func_private_process_range_header(request_t *request, HttpHeader *hdr_out,StoreEntry *e);
int cc_call_hook_func_http_check_request_change(MemBuf *mb, HttpStateData * httpState);

int cc_call_hook_func_httpHdrRangeOffsetLimit(request_t *request,int *ret);
int cc_call_hook_func_storeKeyPublicByRequestMethod(SQUID_MD5_CTX *M ,request_t *request);

int cc_call_hook_func_private_add_http_orig_range(clientHttpRequest *http);
int cc_call_hook_func_httpStateFree(HttpStateData *httpState);
int  cc_call_hook_func_private_change_request(request_t *r,clientHttpRequest *http);
int cc_call_hook_func_private_should_close_fd(clientHttpRequest *http);
int cc_call_hook_func_private_before_send_header(clientHttpRequest *http,MemBuf *mb);
void cc_call_hook_func_private_change_accesslog_info(clientHttpRequest *http);
int cc_call_hook_func_private_checkModPartitionStoreEnable(clientHttpRequest *http);
void cc_call_hook_func_private_getReplyUpdateByIMSRep(clientHttpRequest *http);

//added end

int cc_call_hook_func_process_modified_since(StoreEntry * entry,request_t *request);

//added for helper`s queue overflow problem
void cc_call_hook_private_register_deferfunc_for_helper(helper * hlp);
void cc_call_hook_func_module_reconfigure();
int cc_call_hook_func_is_defer_accept_req();
int cc_call_hook_func_is_defer_read_req();
//added end
int cc_call_hook_func_private_set_client_keepalive(clientHttpRequest *http);
int cc_call_hook_func_can_find_vary();
void cc_call_hook_func_fwd_start(FwdState * fwd);
int cc_call_hook_func_private_client_keepalive_set_timeout(int fd);

//query direct
int cc_call_hook_func_private_query_direct(int fd, request_t* t, StoreEntry* e);
void cc_call_hook_func_private_fwd_complete_clean(FwdState* fwdState);

int cc_call_hook_func_private_client_send_more_data(clientHttpRequest * http, MemBuf *mb,char *buf, ssize_t size);
void cc_call_hook_func_transfer_done_precondition(clientHttpRequest *http, int *precondition);
int call_hook_func_private_store_entry_valid_length(StoreEntry *entry);

//added for mod_debug_log
void cc_call_hook_func_http_created(clientHttpRequest *http);
void cc_call_hook_func_httpreq_no_cache_reason(int fd, clientHttpRequest *http, NOCACHE_REASON ncr);
void cc_call_hook_func_httprep_no_cache_reason(int fd, MemObject *mem, NOCACHE_REASON ncr);
void cc_call_hook_func_store_no_cache_reason(StoreEntry *e, NOCACHE_REASON ncr);
int cc_call_hook_func_http_req_debug_log(clientHttpRequest *http);
void cc_call_hook_func_set_epoll_load(int num, struct epoll_event *events);
void cc_call_hook_func_clear_dir_load();
void cc_call_hook_func_add_dir_load();
void cc_call_hook_func_set_filemap_time(int count);
int cc_call_hook_func_private_client_keepalive_set_timeout(int fd);
//added end
int cc_call_hook_func_private_http_before_redirect_for_billing(clientHttpRequest *http);

int cc_call_hook_func_private_post_use_persistent_connection(FwdState *fwdState);
void cc_call_hook_func_get_http_body_buf(clientHttpRequest *http, char *buf, ssize_t size);
int cc_call_hook_func_private_http_repl_send_end(clientHttpRequest *http);
int cc_call_hook_func_private_client_cache_hit_for_redirect_location(clientHttpRequest *http);

//add for 56flv
int cc_call_hook_func_send_header_clone_before_reply(clientHttpRequest *http, HttpReply * rep);
//added for multi_range_bytes
void cc_call_hook_func_permit_multi_range_bytes(int *pass);
//added end
//added for mod_store_url_rewrite
int cc_call_hook_func_store_url_rewrite(clientHttpRequest*);
int cc_call_hook_func_private_end_compress_response_body(HttpStateData * httpState, char *buf, ssize_t len );
//added end
void  cc_call_hook_func_private_comm_connect_dns_handle(const ipcache_addrs * ia, void *data);
void cc_call_hook_func_private_UserAgent_change_port(int fd, FwdState* fwdState, unsigned short* port);
int cc_call_hook_func_client_access_check_done(clientHttpRequest *http);
void cc_call_hook_func_reply_header_mb(clientHttpRequest *http, MemBuf *mb);
int cc_call_hook_func_is_webluker_set(); 
int cc_call_hook_func_fc_reply_header(clientHttpRequest *http, HttpHeader *hdr);
void cc_call_hook_func_coss_membuf(void);

int cc_call_hook_func_private_assign_dir(const StoreEntry *e, char **forbid);
void cc_call_hook_func_private_assign_dir_set_private_data (HttpStateData *httpState);
int cc_call_hook_func_private_is_assign_dir(StoreEntry *e, int dirn);
int cc_call_hook_func_is_active_compress(int , HttpStateData*, size_t, size_t*);
void cc_call_hook_func_private_getOnceSendBodySize(clientHttpRequest * http, size_t send_len);
void cc_call_hook_func_quick_abort_check(StoreEntry *entry, int * abort);
void cc_call_hook_func_private_comm_connect_polling_serverIP(const ipcache_addrs * ia, int fd, void *data);
// add by xueye.zhao
// 2014-04-01 10:00
// merge rdb_letv/modules/range
int cc_call_hook_func_support_complex_range(clientHttpRequest* http);
//end add

#endif
