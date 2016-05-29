#ifndef CC_FRAMEWORK_STRUCT_H
#define CC_FRAMEWORK_STRUCT_H
#include <sys/epoll.h>

/* define*/
#define MAX_MOD_NUM    	250 
#define MAX_PATH_LEN    256
#define CC_MOD_TAG      "mod_"
#define CC_SUB_MOD_TAG  " sub_mod "
#define CC_MOD_CFG_NAME "module.conf"

#define REPLY_CHECK	" reply_check "

#define SQUID_CONFIG_ACL_BUCKET_NUM 2048

/* typedef*/
typedef struct _cc_module   cc_module;
typedef struct _cc_mod_param     cc_mod_param;
typedef struct _cc_mod_params_list     cc_mod_params_list;
typedef struct _cc_private_data     cc_private_data;
typedef struct _cc_acl_check_result     cc_acl_check_result;
typedef struct _mod_domain_access mod_domain_access;

/* hook fun type */
typedef int HOOK_SYS_INIT();
typedef int HOOK_SYS_CLEANUP(cc_module *module);
typedef int HOOK_SYS_PARSE_PARAM(char *cfg_line);

typedef int HOOK_GET_CLIENT_IP(clientHttpRequest *http);
typedef int HOOK_ADD_HTTP_REQ_HTADER(HttpStateData *httpState,request_t *request,HttpHeader* hdr_out);

typedef int HOOK_FD_BYTES(int fd, int len, unsigned int type);
typedef int HOOK_FD_CLOSE(int fd);

typedef int HOOK_CONN_STATE_FREE(ConnStateData *conn);

typedef int HOOK_BUF_RECV_FROM_S(int fd, char *buf, int len);
typedef int HOOK_BUF_SEND_TO_S(HttpStateData *httpState, MemBuf *mb);

typedef int HOOK_HTTP_REQ_PARSED(clientHttpRequest *http);
typedef int HOOK_HTTP_REQ_READ(clientHttpRequest *http);
typedef int	HOOK_HTTP_REQ_SEND(HttpStateData *data);
typedef int	HOOK_HTTP_REQ_SEND_EXACT(HttpStateData *data, HttpHeader* hdr);
//add by zh for hotspot mod
typedef int	HOOK_PRIVATE_BEFORE_HTTP_REQ_PROCESS(clientHttpRequest *http);
//add end
typedef int	HOOK_HTTP_REQ_PROCESS(clientHttpRequest *http);
typedef int	HOOK_HTTP_REQ_FREE(clientHttpRequest *http);
typedef int HOOK_HTTP_REPL_READ_START(int fd, HttpStateData *data);
typedef int HOOK_FUNC_IS_ACTIVE_COMPRESS(int fd, HttpStateData *data, size_t hdr_size,size_t*);
typedef int HOOK_HTTP_REPL_REFRESH_START(int fd, HttpStateData *data);
typedef int HOOK_HTTP_REPL_CACHABLE_CHECK(int fd, HttpStateData *data);
typedef int HOOK_HTTP_REPL_READ_END(int fd, HttpStateData *data);
typedef int HOOK_HTTP_REPL_SEND_START(clientHttpRequest *http);
typedef int HOOK_HTTP_REPL_SEND_END(clientHttpRequest *http);
typedef int HOOK_HTTP_CLIENT_CACHE_HIT(clientHttpRequest *http, int stale);

typedef int HOOK_PRIVATE_NEGATIVE_TTL(HttpStateData *httpState);
typedef int HOOK_PRIVATE_ERROR_PAGE(ErrorState *err, request_t *request);
typedef int HOOK_PRIVATE_DST_IP(const char *host, unsigned short port, int ctimeout, struct in_addr addr, unsigned char TOS, FwdState* fwdState);
typedef int HOOK_PRIVATE_DST_IP2(FwdState* fwdState);
typedef int HOOK_PRIVATE_HTTP_ACCEPT(int fd);
typedef int	HOOK_PRIVATE_HTTP_REQ_PROCESS2(clientHttpRequest *http);
typedef refresh_t *HOOK_PRIVATE_REFRESH_LIMITS(const char *url);
typedef int HOOK_PRIVATE_MODIFY_ENTRY_TIMESTAMP(StoreEntry* e);
typedef int HOOK_HTTP_BEFORE_REDIRECT(clientHttpRequest *http);
typedef int HOOK_PRIVATE_HTTP_BEFORE_REDIRECT(clientHttpRequest *http);
typedef int HOOK_HTTP_BEFORE_REDIRECT_READ(clientHttpRequest *http);
//added by jiangbo.tian for limit speed
typedef int HOOK_PRIVATE_INDIVIDUAL_LIMIT_SPEED(int fd,void* data,int *nleft, PF *resume_handler);
//added end
//added by jiangbo.tian for do_not_unlink
typedef int HOOK_PRIVATE_PUT_SUGGEST(SwapDir* SD,sfileno fn);
typedef int HOOK_PRIVATE_NEEDUNLINK();
typedef int HOOK_PRIVATE_SETDISKERRORFLAG(StoreEntry *e, int error_flag);
typedef int HOOK_PRIVATE_GETDISKERRORFLAG(StoreEntry *e);
typedef int HOOK_PRIVATE_IS_DISK_OVERLOADING();
typedef int HOOK_PRIVATE_GET_SUGGEST(SwapDir* sd);
//added by chenqi
typedef void HOOK_SUPPORT_COMPLEX_RANGE(clientHttpRequest *http, int *supp);
//added end


//added end
//added by jiangbo.tian for customized_server_side_error_page

typedef int HOOK_PRIVATE_COMM_SET_TIMEOUT(FwdState *fwd, int* timeout);
typedef int HOOK_PRIVATE_PROCESS_ERROR_PAGE(clientHttpRequest *http,HttpReply *rep);
typedef int HOOK_PRIVATE_PROCESS_MISS_IN_ERROR_PAGE(clientHttpRequest* http,int *ret);
typedef int HOOK_PRIVATE_REPL_SEND_START(clientHttpRequest* http,int* ret);
typedef int HOOK_PRIVATE_SET_ERROR_TEXT(ErrorState* err,char** text);
//added end by jiangbo.tian

typedef void HOOK_PRIVATE_PRELOAD_CHANGE_FWDSTAT(request_t *request, FwdState* fwdState);
typedef int HOOK_PRIVATE_ADD_HOST_IP_HEADER_FOR_CA(FwdState* fwdState);
typedef void HOOK_PRIVATE_PRELOAD_CHANGE_HOSTPORT(FwdState* fwdState, const char** host, unsigned short* port);

typedef int HOOK_HTTP_CLIENT_CREATE_STORE_ENTRY(clientHttpRequest *http, StoreEntry *e);
typedef int FREE_CALLBACK(void *private_data);

typedef int HOOK_STORE_CLIENT_COPY(StoreEntry *e, store_client *sc);
typedef squid_off_t HOOK_STORELOWESTMEMREADEROFFSET(const StoreEntry *e, int *shouldReturn);

//added by dongshu.zhou for receipt
typedef void HOOK_PRIVATE_CLIENT_BUILD_REPLY_HEADER(clientHttpRequest *http, HttpReply *rep);
typedef int HOOK_PRIVATE_CLIENT_KEEPALIVE_NEXT_REQUEST(int fd);
//end added by dongshu.zhou
typedef int HOOK_PRIVATE_STORE_CLIENT_COPY_HIT(clientHttpRequest *http);

typedef void HOOK_PRIVATE_CLIENTPACKMORERANGES(clientHttpRequest * http, char* buf, int size); 
typedef void HOOK_SYS_AFTER_PARSE_PARAM();
typedef void HOOK_BEFORE_SYS_INIT();

typedef int HOOK_LIMIT_OBJ();
typedef double HOOK_STOREDIR_MAINT(double f, SwapDir *SD);
typedef int HOOK_STOREDIR_MAINT_BREAK(SwapDir *SD);
typedef int HOOK_STOREDIR_SET_MAX_LOAD(int max_load);
//added by dongshu.zhou for mp4
typedef int HOOK_PRIVATE_MP4_SEND_MORE_DATA(clientHttpRequest * http, MemBuf *mb,char *buf, ssize_t size);
typedef int HOOK_PRIVATE_CLIENT_WRTIE_COMPLETE(int fd, char *bufnotused, size_t size, int errflag, void *data);
typedef int HOOK_PRIVATE_MP4_READ_MOOV_BODY(int fd, CommWriteStateData *state);
typedef int HOOK_STORE_COMPLETE(StoreEntry* e);
typedef int HOOK_INVOKE_HANDLERS(StoreEntry* e);
typedef int HOOK_PRIVATE_SC_UNREGISTER(StoreEntry* e,clientHttpRequest* http);
//end added mp4
typedef int HOOK_PRIVATE_CLIENTREDIRECTDONE(request_t** new_request, char* result, clientHttpRequest* http) ;
typedef int HOOK_PRIVATE_SELECT_SWAP_DIR_ROUND_ROBIN(const StoreEntry* e) ;
typedef int HOOK_CLIENT_SET_ZCOPYABLE(store_client *sc, int fd);

typedef int HOOK_BUF_RECV_FROM_S_BODY(HttpStateData * httpState, char *buf, ssize_t len, int buffer_filled);
typedef int HOOK_BUF_RECV_FROM_S_HEADER(HttpStateData * httpState, char *buf, ssize_t len);

typedef int HOOK_PRIVATE_COMPRESS_RESPONSE_BODY(HttpStateData * httpState, char *buf, ssize_t len, char *comp_buf,ssize_t comp_buf_len, ssize_t *comp_size, int buffer_filled);
//added for toudu flv 
typedef int HOOK_PRIVATE_REPLY_MODIFY_BODY(HttpStateData *httpState,char *buf, ssize_t len,ssize_t alloc_size);
//end added toudu
typedef int HOOK_PRIVATE_CLIENTCACHEHIT_ETAG_MISMATCH (clientHttpRequest * http);
typedef void HOOK_CLIENT_CACHE_HIT_START(int fd, HttpReply *reply, StoreEntry *e);
typedef void HOOK_CLIENT_HANDLE_IMS_REPLY(clientHttpRequest *http);
typedef void HOOK_BEFORE_CLIENT_ACL_CHECK(clientHttpRequest *http);
typedef int HOOK_PRIVATE_ACLMATCHREGEX(acl* ae, request_t *r);
typedef int HOOK_PRIVATE_BEFORE_LOOP(allow_t *allow,aclCheck_t* checklist,acl_access* A,cc_acl_check_result *result, int type,cc_module *mod_p);
typedef int HOOK_PRIVATE_BEFORE_ACL_CALLBACK(allow_t allow, aclCheck_t* checklist,acl_access* aa);
typedef int HOOK_PRIVATE_CC_ACLCHECKFASTREQUESTDONE(aclCheck_t* checklist,cc_module *mod_p,acl_access* A, cc_acl_check_result* result,int type);
typedef void HOOK_CLIENT_PROCESS_HIT_START(clientHttpRequest *http);
typedef void HOOK_CLIENT_PROCESS_HIT_AFTER_304_SEND(clientHttpRequest *http);
typedef void HOOK_CLIENT_PROCESS_HIT_END(clientHttpRequest *http);
typedef void HOOK_REFRESH_STALENESS(StoreEntry *entry);
typedef int HOOK_REFRESH_CHECK_PRE_RETURN(time_t age, time_t delta, request_t *request, StoreEntry *entry, time_t *cc_age);
typedef void HOOK_REFRESH_CHECK_SET_EXPIRES(StoreEntry *entry, time_t cc_age, const refresh_t *R);
//added for helper`s queue overflow problem
typedef void HOOK_PRIVATE_REGISTER_DEFERFUNC_FOR_HELPER(helper * hlp);
typedef void HOOK_MODULE_RECONFIGURE(int redirect_should_reload);
typedef int HOOK_IS_DEFER_ACCEPT_REQ();
typedef int HOOK_IS_DEFER_READ_REQ();
//added end
typedef int HOOK_PRIVATE_SET_CLIENT_KEEPALIVE(clientHttpRequest *http);
typedef int HOOK_CAN_FIND_VARY();
typedef void HOOK_FWD_START(FwdState * fwd);
typedef int HOOK_PRIVATE_CLIENT_KEEPALIVE_SET_TIMEOUT(int fd);
typedef int HOOK_FUNC_PROCESS_MODIFIED_SINCE(StoreEntry * entry, request_t * request);
typedef int HOOK_PRIVATE_QUERY_DIRECT(int fd, request_t* t, StoreEntry* e);
typedef void HOOK_PRIVATE_FWD_COMPLETE_CLEAN(FwdState* fwdState);
typedef int HOOK_IS_WEBLUKER_SET() ;
typedef int HOOK_FC_REPLY_HEADER(clientHttpRequest *http,HttpHeader *hdr);
typedef int HOOK_PRIVATE_CLIENT_SEND_MORE_DATA(clientHttpRequest * http, MemBuf *mb,char *buf, ssize_t size);
typedef void HOOK_TRANSFER_DONE_PRECONDITION(clientHttpRequest* http, int * precondition);
typedef int HOOK_PRIVATE_STORE_ENTRY_VALID_LENGTH(StoreEntry *entry);
typedef void HOOK_HTTP_CREATED(clientHttpRequest *http);
typedef void HOOK_HTTPREQ_NO_CACHE_REASON(clientHttpRequest *http, NOCACHE_REASON ncr);
typedef void HOOK_HTTPREP_NO_CACHE_REASON(MemObject *mem, NOCACHE_REASON ncr);
typedef void HOOK_STORE_NO_CACHE_REASON(StoreEntry *e, NOCACHE_REASON ncr);
typedef int	HOOK_HTTP_REQ_DEBUG_LOG(clientHttpRequest *http);
typedef void HOOK_SET_EPOLL_LOAD(int num,struct epoll_event *events);
typedef void HOOK_CLEAR_DIR_LOAD();
typedef void HOOK_ADD_DIR_LOAD();
typedef void HOOK_SET_FILEMAP_TIME(int count);
typedef int HOOK_PRIVATE_SET_TPROXY_IP(FwdState* fwd);
typedef int HOOK_BEFORE_CLIENTPROCESSREQUEST2(clientHttpRequest *http);
typedef int HOOK_PRIVATE_PROCESS_SEND_HEADER(clientHttpRequest *http);  
typedef int HOOK_PRIVATE_PROCESS_RANGE_HEADER(request_t *request, HttpHeader *hdr_out,const StoreEntry *e); 
typedef int HOOK_HTTP_CHECK_REQUEST_CHANGE(MemBuf *mb, HttpStateData * httpState);
typedef int HOOK_HTTPHDRRANGEOFFSETLIMIT(request_t *request, int* ret);
typedef int HOOK_STOREKEYPUBLICBYREQUESTMETHOD(SQUID_MD5_CTX *M ,request_t *request);
typedef int HOOK_PRIVATE_ADD_HTTP_ORIG_RANGE(clientHttpRequest *http);
typedef int HOOK_PRIVATE_STORE_PARTIAL_CONTENT();
typedef int HOOK_PRIVATE_PRIVATE_CLIENT_CACHE_HIT_START(clientHttpRequest *http,StoreEntry *e);
typedef int	HOOK_PRIVATE_CLIENTBUILDRANGEHEADER(clientHttpRequest *http);
typedef int HOOK_PRIVATE_AFTER_WRITE_HEADER(clientHttpRequest* http);
typedef int HOOK_HTTPSTATEFREE(	HttpStateData *httpState);
typedef void HOOK_PRIVATE_SET_STATUS_LINE(clientHttpRequest *http, HttpReply *rep);
typedef int HOOK_PRIVATE_HTTP_REPL_SEND_END(clientHttpRequest *http);
typedef int HOOK_PRIVATE_BEFORE_SEND_HEADER(clientHttpRequest *http,MemBuf *mb);
typedef void HOOK_CLIENTLSCSCONNECTIONSOPEN(void); 
typedef void HOOK_PRIVATE_CHANGE_ACCESSLOG_INFO(clientHttpRequest *http);
typedef int HOOK_PRIVATE_CHECKMODPARTITIONSTOREENABLE(clientHttpRequest *http);
typedef void HOOK_PRIVATE_GETREPLYUPDATEBYIMSREP(clientHttpRequest *http);
typedef void HOOK_FUNC_PRIVATE_COMM_CONNECT_DNS_HANDLE(const ipcache_addrs * ia, void *data);
typedef void HOOK_FUNC_PRIVATE_COMM_CONNECT_POLLING_SERVERIP(const ipcache_addrs * ia, void *data);
typedef void HOOK_FUNC_PRIVATE_MOBILE_USERAGENT_CHANGE_PORT(int fd, FwdState* fwdState, unsigned short* port);
typedef int HOOK_CLIENT_ACCESS_CHECK_DONE(clientHttpRequest *http);

typedef int HOOK_PRIVATE_STORE_META_URL(store_client *sc,char* new_url);
typedef int HOOK_PRIVATE_CHANGE_REQUEST(request_t *r,clientHttpRequest *http) ;
typedef int HOOK_PRIVATE_SHOULD_CLOSE_FD(clientHttpRequest *http);
typedef int HOOK_HTTP_REQ_READ_THREE(clientHttpRequest *http);
typedef int HOOK_HTTP_AFTER_CLIENT_STORE_REWRITE(clientHttpRequest *http);
typedef int HOOK_PRIVATE_PROCESS_EXCESS_DATA(HttpStateData *httpState);
typedef int HOOK_PRIVATE_RECORD_TREND_ACCESS_LOG(clientHttpRequest *http);
typedef int HOOK_PRIVATE_CLIENT_CACHE_HIT_FOR_REDIRECT_LOCATION(clientHttpRequest *http);
typedef int HOOK_SEND_HEADER_CLONE_REPLY_BEFORE(clientHttpRequest *http, HttpReply * rep);
typedef int HOOK_PRIVATE_360_WRITE_RECEIVE_WARNING_LOG(HttpStateData *httpState,int len);
typedef int HOOK_PRIVATE_360_WRITE_SEND_WARNING_LOG(clientHttpRequest *http);
typedef int HOOK_HTTP_BEFORE_REDIRECT_FOR_BILLING(clientHttpRequest *http);
typedef int HOOK_PRIVATE_SET_NAME_AS_IP(FwdState *fwdState, char** name);
typedef int HOOK_GET_HTTP_BODY_BUF(clientHttpRequest *http, char *buf, ssize_t size);
typedef int HOOK_PRIVATE_STORE_META_URL_FOR_REDIRECT_LOCATION(clientHttpRequest *http);

typedef void HOOK_REPLY_HEADER_MB(clientHttpRequest* http, MemBuf *mb);
typedef void HOOK_PRIVATE_GETMULTIRANGEONESIZE(clientHttpRequest *http,size_t sopy_sz);
typedef void HOOK_PRIVATE_GETONCESENDBODYSIZE(clientHttpRequest *http, size_t send_len);
typedef int HOOK_STORE_URL_REWRITE(clientHttpRequest*);
typedef int HOOK_PRIVATE_END_COMPRESS_RESPONSE_BODY(HttpStateData * httpState, char *buf, ssize_t len );

typedef int HOOK_PRIVATE_OPENIDLECONN( int idle,FwdState *fwdState,char* name,char* domain, int ctimeout);
typedef int HOOK_PRIVATE_HTTP_REPL_READ_END( HttpStateData *httpState,int *keep_alive);
typedef int HOOK_PRIVATE_SET_HOST_AS_IP( int fd,char** host);
typedef void HOOK_PRIVATE_DUP_PERSIST_FD(void* data);
typedef void HOOK_PRIVATE_SET_UPPERLAYER_PERSIST_CONNECTION_TIMEOUT(FwdState *fwdState,int server_fd);

typedef void HOOK_PRIVATE_SET_SUBLAYER_TCP_KEEPALIVE(int fd);
typedef void HOOK_PRIVATE_SET_SUBLAYER_PERSIST_CONNECTION_TIMEOUT(int fd);
typedef void HOOK_PRIVATE_SET_SUBLAYER_PERSIST_REQUEST_TIMEOUT(int fd);
typedef int HOOK_PRIVATE_POST_USE_PERSISTENT_CONNECTION(FwdState *fwdState);
typedef void HOOK_PRIVATE_SET_COSS_MEMBUF();

typedef int HOOK_PRIVATE_ASSIGN_DIR_SET_PRIVATE_DATA (HttpStateData *httpState);
typedef int HOOK_PRIVATE_ASSIGN_DIR(const StoreEntry *e, char **forbid, int *ret);
typedef int HOOK_PRIVATE_IS_ASSIGN_DIR (StoreEntry *e, int dirn);

typedef void HOOK_QUICK_ABORT_CHECK(StoreEntry *entry, int *abort);
typedef int HOOK_REPLY_CANNOT_FIND_DATE (const HttpReply *rep);
typedef int HOOK_GET_CONTENT_ORIG_DOWN(clientHttpRequest *http, http_status status);
typedef int HOOK_PRIVATE_REFORWARDABLE(FwdState *); 
typedef int HOOK_STORE_APPEND_CHECK(HttpStateData *); 

typedef enum 
{
	FDE_PRIVATE_DATA,
	REQUEST_PRIVATE_DATA,
	STORE_ENTRY_PRIVATE_DATA,
	FWDSTATE_PRIVATE_DATA,
	CONN_STATE_DATA,
	REQUEST_T_PRIVATE_DATA,
	MEMOBJECT_PRIVATE_DATA
} mod_private_data_t;



typedef enum 
{
	RET_GO_ON,	
	RET_RETURN
} mod_ret_t;

typedef enum {
	HPIDX_hook_func_sys_init = 0,
	HPIDX_hook_func_sys_cleanup = 1,
	HPIDX_hook_func_sys_parse_param = 2,
	HPIDX_hook_func_sys_after_parse_param = 3,
	HPIDX_hook_func_fd_bytes = 4,
	HPIDX_hook_func_fd_close = 5,
	HPIDX_hook_func_conn_state_free = 6,
	HPIDX_hook_func_buf_recv_from_s = 7,
	HPIDX_hook_func_buf_send_to_s = 8,
	HPIDX_hook_func_http_req_read = 9,
	HPIDX_hook_func_http_req_read_second = 10,
	HPIDX_hook_func_http_private_req_read_second = 11,
	HPIDX_hook_func_http_req_parsed = 12,
	HPIDX_hook_func_http_req_send = 13,
	HPIDX_hook_func_http_req_send_exact = 14,
	HPIDX_hook_func_private_before_http_req_process = 15,
	HPIDX_hook_func_http_req_process = 16,
	HPIDX_hook_func_http_req_free = 17,
	HPIDX_hook_func_http_client_cache_hit = 18,
	HPIDX_hook_func_http_repl_read_start = 19,
	HPIDX_hook_func_http_repl_refresh_start = 20,
	HPIDX_hook_func_http_repl_cachable_check = 21,
	HPIDX_hook_func_http_repl_read_end = 22,
	HPIDX_hook_func_http_repl_send_start = 23,
	HPIDX_hook_func_http_repl_send_end = 24,
	HPIDX_hook_func_store_client_copy = 25,
	HPIDX_hook_func_store_client_copy3 = 26,
	HPIDX_hook_func_http_client_create_store_entry = 27,
	HPIDX_hook_func_http_before_redirect = 28,
	HPIDX_hook_func_private_http_before_redirect = 29,
	HPIDX_hook_func_private_negative_ttl = 30,
	HPIDX_hook_func_private_dst_ip = 31,
	HPIDX_hook_func_private_dst_ip2 = 32,
	HPIDX_hook_func_private_error_page = 33,
	HPIDX_hook_func_private_http_accept = 34,
	HPIDX_hook_func_private_http_req_process2 = 35,
	HPIDX_hook_func_private_refresh_limits = 36,
	HPIDX_hook_func_private_preload_change_fwdState = 37,
	HPIDX_hook_func_private_add_host_ip_header_for_ca = 38,
	HPIDX_hook_func_private_preload_change_hostport = 39,
	HPIDX_hook_func_private_storeLowestMemReaderOffset = 40,
	HPIDX_hook_func_private_modify_entry_timestamp = 41,
	HPIDX_hook_func_private_clientPackMoreRanges = 42,
	HPIDX_hook_func_private_individual_limit_speed = 43,
	HPIDX_hook_func_private_client_build_reply_header = 44,
	HPIDX_hook_func_private_client_keepalive_next_request = 45,
	HPIDX_hook_func_private_store_client_copy_hit = 46,
	HPIDX_hook_func_private_comm_set_timeout = 47,
	HPIDX_hook_func_private_process_error_page = 48,
	HPIDX_hook_func_private_process_miss_in_error_page = 49,
	HPIDX_hook_func_private_repl_send_start = 50,
	HPIDX_hook_func_private_set_error_text = 51,
	HPIDX_hook_func_private_get_suggest = 52,
	HPIDX_hook_func_private_put_suggest = 53,
	HPIDX_hook_func_private_needUnlink = 54,
	HPIDX_hook_func_private_setDiskErrorFlag = 55,
	HPIDX_hook_func_private_getDiskErrorFlag = 56,
	HPIDX_hook_func_private_is_disk_overloading = 57,
	HPIDX_hook_func_storedir_set_max_load = 58,
	HPIDX_hook_func_private_mp4_send_more_data = 59,
	HPIDX_hook_func_private_client_write_complete = 60,
	HPIDX_hook_func_private_mp4_read_moov_body = 61,
	HPIDX_hook_func_private_clientRedirectDone = 62,
	HPIDX_hook_func_private_select_swap_dir_round_robin = 63,
	HPIDX_hook_func_client_set_zcopyable = 64,
	HPIDX_hook_func_buf_recv_from_s_body = 65,
	HPIDX_hook_func_buf_recv_from_s_header = 66,
	HPIDX_hook_func_private_reply_modify_body = 67,
	HPIDX_hook_func_private_clientCacheHit_etag_mismatch = 68,
	HPIDX_hook_func_client_cache_hit_start = 69,
	HPIDX_hook_func_before_sys_init = 70,
	HPIDX_hook_func_client_handle_ims_reply = 71,
	HPIDX_hook_func_before_client_acl_check = 72,
	HPIDX_hook_func_private_aclMatchRegex = 73,
	HPIDX_hook_func_private_before_loop = 74,
	HPIDX_hook_func_private_before_acl_callback = 75,
	HPIDX_hook_func_private_cc_aclCheckFastRequestDone = 76,
	HPIDX_hook_func_client_process_hit_start = 77,
	HPIDX_hook_func_client_process_hit_after_304_send = 78,
	HPIDX_hook_func_client_process_hit_end = 79,
	HPIDX_hook_func_refresh_staleness = 80,
	HPIDX_hook_func_refresh_check_pre_return = 81,
	HPIDX_hook_func_refresh_check_set_expires = 82,
	HPIDX_hook_func_private_register_deferfunc_for_helper = 83,
	HPIDX_hook_func_module_reconfigure = 84,
	HPIDX_hook_func_is_defer_accept_req = 85,
	HPIDX_hook_func_is_defer_read_req = 86,
	HPIDX_hook_func_private_set_client_keepalive = 87,
	HPIDX_hook_func_limit_obj = 88,
	HPIDX_hook_func_can_find_vary = 89,
	HPIDX_hook_func_fwd_start = 90,
	HPIDX_hook_func_process_modified_since = 91,
	HPIDX_hook_func_private_query_direct = 92,
	HPIDX_hook_func_private_fwd_complete_clean = 93,
	HPIDX_hook_func_is_webluker_set = 94,
	HPIDX_hook_func_fc_reply_header = 95,
	HPIDX_hook_func_before_clientProcessRequest2 = 96,
	HPIDX_hook_func_private_http_repl_send_end = 97,
	HPIDX_hook_func_private_before_send_header = 98,
	HPIDX_hook_func_private_process_send_header = 99,
	HPIDX_hook_func_private_process_range_header = 100,
	HPIDX_hook_func_http_check_request_change = 101,
	HPIDX_hook_func_httpHdrRangeOffsetLimit = 102,
	HPIDX_hook_func_storeKeyPublicByRequestMethod = 103,
	HPIDX_hook_func_private_add_http_orig_range = 104,
	HPIDX_hook_func_private_store_partial_content = 105,
	HPIDX_hook_func_private_client_cache_hit_start = 106,
	HPIDX_hook_func_private_clientBuildRangeHeader = 107,
	HPIDX_hook_func_private_after_write_header = 108,
	HPIDX_hook_func_httpStateFree = 109,
	HPIDX_hook_func_private_store_meta_url = 110,
	HPIDX_hook_func_private_change_request = 111,
	HPIDX_hook_func_private_should_close_fd = 112,
	HPIDX_hook_func_private_set_status_line = 113,
	HPIDX_hook_func_private_change_accesslog_info = 114,
	HPIDX_hook_func_private_checkModPartitionStoreEnable = 115,
	HPIDX_hook_func_private_getReplyUpdateByIMSRep = 116,
	HPIDX_hook_func_http_req_read_three = 117,
	HPIDX_hook_func_private_process_excess_data = 118,
	HPIDX_hook_func_private_compress_response_body = 119,
	HPIDX_hook_func_private_end_compress_response_body = 120,
	HPIDX_hook_func_private_client_send_more_data = 121,
	HPIDX_hook_func_transfer_done_precondition = 122,
	HPIDX_hook_func_private_store_entry_valid_length = 123,
	HPIDX_hook_func_http_created = 124,
	HPIDX_hook_func_httpreq_no_cache_reason = 125,
	HPIDX_hook_func_httprep_no_cache_reason = 126,
	HPIDX_hook_func_store_no_cache_reason = 127,
	HPIDX_hook_func_http_req_debug_log = 128,
	HPIDX_hook_func_set_epoll_load = 129,
	HPIDX_hook_func_clear_dir_load = 130,
	HPIDX_hook_func_add_dir_load = 131,
	HPIDX_hook_func_set_filemap_time = 132,
	HPIDX_hook_func_private_record_trend_access_log = 133,
	HPIDX_hook_func_private_set_tproxy_ip = 134,
	HPIDX_hook_func_clientLscsConnectionsOpen = 135,
	HPIDX_hook_func_private_client_cache_hit_for_redirect_location = 136,
	HPIDX_hook_func_get_client_area_use_ip = 137,
	HPIDX_hook_func_send_header_clone_before_reply = 138,
	HPIDX_hook_func_support_complex_range = 139,
	HPIDX_hook_func_private_client_keepalive_set_timeout = 140,
	HPIDX_hook_func_private_360_write_receive_warning_log = 141,
	HPIDX_hook_func_private_360_write_send_warning_log = 142,
	HPIDX_hook_func_private_getMultiRangeOneCopySize = 143,
	HPIDX_hook_func_private_getOnceSendBodySize = 144,
	HPIDX_hook_func_http_after_client_store_rewrite = 145,
	HPIDX_hook_func_store_url_rewrite = 146,
	HPIDX_hook_func_private_http_before_redirect_for_billing = 147,
	HPIDX_hook_func_private_set_name_as_ip = 148,
	HPIDX_hook_func_private_openIdleConn = 149,
	HPIDX_hook_func_private_http_repl_read_end = 150,
	HPIDX_hook_func_private_set_host_as_ip = 151,
	HPIDX_hook_func_private_dup_persist_fd = 152,
	HPIDX_hook_func_private_set_upperlayer_persist_connection_timeout = 153,
	HPIDX_hook_func_private_set_sublayer_tcp_keepalive = 154,
	HPIDX_hook_func_private_set_sublayer_persist_connection_timeout = 155,
	HPIDX_hook_func_private_post_use_persistent_connection = 156,
	HPIDX_hook_func_private_set_sublayer_persist_request_timeout = 157,
	HPIDX_hook_func_get_http_body_buf = 158,
	HPIDX_hook_func_private_comm_connect_dns_handle = 159,
	HPIDX_hook_func_private_comm_connect_polling_serverIP = 160,
	HPIDX_hook_func_private_UserAgent_change_port = 161,
	HPIDX_hook_func_client_access_check_done = 162,
	HPIDX_hook_func_store_complete = 163,
	HPIDX_hook_func_invoke_handlers = 164,
	HPIDX_hook_func_private_sc_unregister = 165,
	HPIDX_hook_func_reply_header_mb = 166,
	HPIDX_hook_func_private_store_meta_url_for_redirect_location = 167,
	HPIDX_hook_func_coss_membuf = 168,
    HPIDX_hook_func_private_assign_dir_set_private_data = 169,
    HPIDX_hook_func_private_assign_dir = 170,
	HPIDX_hook_func_private_is_assign_dir = 171,
	HPIDX_hook_func_http_before_redirect_read = 172,
	HPIDX_hook_func_is_active_compress = 173,
    HPIDX_hook_func_quick_abort_check = 174,
	HPIDX_hook_func_reply_cannot_find_date = 175,
	HPIDX_hook_func_get_content_orig_down = 176,
	HPIDX_hook_func_store_append_check = 177,
	HPIDX_hook_func_private_reforwardable = 178,
	HPIDX_last
} HPIDX_t;

struct _mod_domain_access
{
	cc_mod_param **mod_params;
	acl_access *acl_rules;
	int param_num;
	char* domain;
	mod_domain_access *next;
};

/*struct define*/
struct _cc_module
{
	char    name[MAX_NAME_LEN];
	char    version[MAX_NAME_LEN];
	int     priority;
	struct
	{
		unsigned int configed:1;  // mod is configed in .conf file
		unsigned int enable:1;    // mod can runing
		unsigned int param:1;  // mod have param 
		unsigned int config_on:1;  // mod configed used on, havn't acl eg:mod_billing on
		unsigned int init:1; // mod have inited
		unsigned int c_check_reply:1;  // acl checking flag of client request to fc or fc reply to client
		unsigned int config_off:1;  // mod configed used off, havn't acl eg:mod_billing off
	} flags;
	int		slot;	 // the pos in the global variable:cc_modules
	void    *lib;   // dlopen the mod so
	Array mod_params; // save the all param of modules
	acl_access	*acl_rules; // store acl list
	mod_ret_t ret;	//store every hook_func ret
	FREE_CALLBACK *cc_mod_param_call_back; // free cc_mod_param

	/* added for rcms*/
	mod_domain_access *domain_access[100];
	int has_acl_rules;
	/* callback handle */
	HOOK_SYS_INIT			*hook_func_sys_init; /*this hook must be the first!!!!!!!!!!! */
	HOOK_SYS_CLEANUP 		*hook_func_sys_cleanup;
	HOOK_SYS_PARSE_PARAM  	*hook_func_sys_parse_param;
	HOOK_SYS_AFTER_PARSE_PARAM *hook_func_sys_after_parse_param;

	HOOK_FD_BYTES  *hook_func_fd_bytes;
	HOOK_FD_CLOSE  *hook_func_fd_close;

	HOOK_CONN_STATE_FREE  *hook_func_conn_state_free;

	HOOK_BUF_RECV_FROM_S		*hook_func_buf_recv_from_s;
	HOOK_BUF_SEND_TO_S			*hook_func_buf_send_to_s;

	HOOK_HTTP_REQ_READ			*hook_func_http_private_req_read_second;
	HOOK_HTTP_REQ_PARSED			*hook_func_http_req_parsed;
	HOOK_HTTP_REQ_SEND 			*hook_func_http_req_send;
	HOOK_HTTP_REQ_SEND_EXACT 			*hook_func_http_req_send_exact;
	HOOK_HTTP_REQ_PROCESS		*hook_func_http_req_process;
	HOOK_HTTP_REQ_FREE   		*hook_func_http_req_free;
	HOOK_HTTP_CLIENT_CACHE_HIT	*hook_func_http_client_cache_hit;
	HOOK_HTTP_REPL_READ_START 	*hook_func_http_repl_read_start;
	HOOK_FUNC_IS_ACTIVE_COMPRESS *hook_func_is_active_compress;
	HOOK_HTTP_REPL_REFRESH_START 	*hook_func_http_repl_refresh_start;
	HOOK_HTTP_REPL_CACHABLE_CHECK   *hook_func_http_repl_cachable_check;	
	HOOK_HTTP_REPL_READ_END 	*hook_func_http_repl_read_end;	
	HOOK_HTTP_REPL_SEND_START 	*hook_func_http_repl_send_start;
	HOOK_HTTP_REPL_SEND_END 	*hook_func_http_repl_send_end;
	HOOK_STORE_CLIENT_COPY *hook_func_store_client_copy;
	HOOK_STORE_CLIENT_COPY *hook_func_store_client_copy3;
	HOOK_HTTP_CLIENT_CREATE_STORE_ENTRY *hook_func_http_client_create_store_entry;
	HOOK_HTTP_BEFORE_REDIRECT* hook_func_http_before_redirect;	
	//add by zh for hotspot mod
	HOOK_PRIVATE_BEFORE_HTTP_REQ_PROCESS	*hook_func_private_before_http_req_process;
	//add end
	HOOK_HTTP_BEFORE_REDIRECT_READ *hook_func_http_before_redirect_read;

	/* For private using,please don't use it */
	HOOK_PRIVATE_HTTP_BEFORE_REDIRECT *hook_func_private_http_before_redirect;
	HOOK_PRIVATE_NEGATIVE_TTL	*hook_func_private_negative_ttl;
	HOOK_PRIVATE_ERROR_PAGE		*hook_func_private_error_page;
	HOOK_PRIVATE_HTTP_ACCEPT	*hook_func_private_http_accept;
	HOOK_PRIVATE_HTTP_REQ_PROCESS2	*hook_func_private_http_req_process2;
	HOOK_PRIVATE_REFRESH_LIMITS		*hook_func_private_refresh_limits;
	HOOK_PRIVATE_PRELOAD_CHANGE_FWDSTAT	*hook_func_private_preload_change_fwdState;
	HOOK_PRIVATE_PRELOAD_CHANGE_HOSTPORT *hook_func_private_preload_change_hostport;
	HOOK_STORELOWESTMEMREADEROFFSET *hook_func_private_storeLowestMemReaderOffset;	
	HOOK_PRIVATE_MODIFY_ENTRY_TIMESTAMP *hook_func_private_modify_entry_timestamp;
	/* For private using,please don't use it */
	HOOK_PRIVATE_DST_IP	*hook_func_private_dst_ip;
	HOOK_PRIVATE_DST_IP2	*hook_func_private_dst_ip2;
	HOOK_PRIVATE_ADD_HOST_IP_HEADER_FOR_CA *hook_func_private_add_host_ip_header_for_ca;

	HOOK_PRIVATE_CLIENTPACKMORERANGES	*hook_func_private_clientPackMoreRanges;
	HOOK_PRIVATE_INDIVIDUAL_LIMIT_SPEED *hook_func_private_individual_limit_speed;
	//added by chenqi
	HOOK_SUPPORT_COMPLEX_RANGE *hook_func_support_complex_range;
	//added enda
	//added by dongshu.zhou for receipt 
	HOOK_PRIVATE_CLIENT_BUILD_REPLY_HEADER *hook_func_private_client_build_reply_header;
	HOOK_PRIVATE_CLIENT_KEEPALIVE_NEXT_REQUEST *hook_func_private_client_keepalive_next_request;
	//end dongshu.zhou
	HOOK_PRIVATE_STORE_CLIENT_COPY_HIT *hook_func_private_store_client_copy_hit;	
	//added by jiangbo.tian for customized_server_side_error_page
	HOOK_PRIVATE_COMM_SET_TIMEOUT *hook_func_private_comm_set_timeout;
	HOOK_PRIVATE_PROCESS_ERROR_PAGE *hook_func_private_process_error_page;
	HOOK_PRIVATE_PROCESS_MISS_IN_ERROR_PAGE *hook_func_private_process_miss_in_error_page;
	HOOK_PRIVATE_REPL_SEND_START *hook_func_private_repl_send_start;
	HOOK_PRIVATE_SET_ERROR_TEXT *hook_func_private_set_error_text;
	//added end by jiangbo.tian
	//added by jiangbo.tian for do not unlink
	HOOK_PRIVATE_GET_SUGGEST	*hook_func_private_get_suggest;
	HOOK_PRIVATE_PUT_SUGGEST	*hook_func_private_put_suggest;
	HOOK_PRIVATE_NEEDUNLINK		*hook_func_private_needUnlink;
	HOOK_PRIVATE_SETDISKERRORFLAG   *hook_func_private_setDiskErrorFlag;
	HOOK_PRIVATE_GETDISKERRORFLAG   *hook_func_private_getDiskErrorFlag;
	HOOK_PRIVATE_IS_DISK_OVERLOADING *hook_func_private_is_disk_overloading;
	//added
	HOOK_STOREDIR_SET_MAX_LOAD *hook_func_storedir_set_max_load;
	//adde for mp4
	HOOK_PRIVATE_MP4_SEND_MORE_DATA *hook_func_private_mp4_send_more_data;
	HOOK_PRIVATE_CLIENT_WRTIE_COMPLETE *hook_func_private_client_write_complete;
	HOOK_PRIVATE_MP4_READ_MOOV_BODY *hook_func_private_mp4_read_moov_body;
	//end mp4
	HOOK_LIMIT_OBJ* hook_func_limit_obj;
	/*******************************added by jiangbo.tian for store_multi_url**********************/
	HOOK_PRIVATE_CLIENTREDIRECTDONE *hook_func_private_clientRedirectDone;
	HOOK_PRIVATE_SELECT_SWAP_DIR_ROUND_ROBIN *hook_func_private_select_swap_dir_round_robin;
	/*******************************added by jiangbo.tian for store_multi_url ended**********************/
	HOOK_CLIENT_SET_ZCOPYABLE *hook_func_client_set_zcopyable;
	HOOK_BUF_RECV_FROM_S_BODY *hook_func_buf_recv_from_s_body;
	HOOK_BUF_RECV_FROM_S_HEADER *hook_func_buf_recv_from_s_header;
	HOOK_STORE_COMPLETE	*hook_func_store_complete;
	HOOK_INVOKE_HANDLERS	*hook_func_invoke_handlers;
	HOOK_PRIVATE_SC_UNREGISTER *hook_func_private_sc_unregister;

	HOOK_FUNC_PRIVATE_COMM_CONNECT_DNS_HANDLE * hook_func_private_comm_connect_dns_handle;
	HOOK_FUNC_PRIVATE_COMM_CONNECT_POLLING_SERVERIP *hook_func_private_comm_connect_polling_serverIP;
	HOOK_FUNC_PRIVATE_MOBILE_USERAGENT_CHANGE_PORT * hook_func_private_UserAgent_change_port;
	HOOK_CLIENT_ACCESS_CHECK_DONE *hook_func_client_access_check_done;
	HOOK_REPLY_HEADER_MB *hook_func_reply_header_mb;
	HOOK_TRANSFER_DONE_PRECONDITION * hook_func_transfer_done_precondition;
	HOOK_PRIVATE_COMPRESS_RESPONSE_BODY *hook_func_private_compress_response_body;
	HOOK_PRIVATE_END_COMPRESS_RESPONSE_BODY *hook_func_private_end_compress_response_body;
	//added for toudu flv 
	HOOK_PRIVATE_REPLY_MODIFY_BODY *hook_func_private_reply_modify_body;
	HOOK_PRIVATE_CLIENTCACHEHIT_ETAG_MISMATCH *hook_func_private_clientCacheHit_etag_mismatch;
	HOOK_CLIENT_CACHE_HIT_START *hook_func_client_cache_hit_start;
	HOOK_BEFORE_SYS_INIT *hook_func_before_sys_init;
	HOOK_CLIENT_HANDLE_IMS_REPLY *hook_func_client_handle_ims_reply;
	/*********************************added by jiangbo.tian for acl_optimize***********/
	HOOK_BEFORE_CLIENT_ACL_CHECK  *hook_func_before_client_acl_check;
	HOOK_PRIVATE_ACLMATCHREGEX	*hook_func_private_aclMatchRegex;
	HOOK_PRIVATE_BEFORE_LOOP *hook_func_private_before_loop;
	HOOK_PRIVATE_BEFORE_ACL_CALLBACK *hook_func_private_before_acl_callback;
	HOOK_PRIVATE_CC_ACLCHECKFASTREQUESTDONE *hook_func_private_cc_aclCheckFastRequestDone;
	/*********************************added by jiangbo.tian for acl_optimize***********/
	//added for 304
	HOOK_CLIENT_PROCESS_HIT_START *hook_func_client_process_hit_start;
	HOOK_CLIENT_PROCESS_HIT_AFTER_304_SEND *hook_func_client_process_hit_after_304_send;
	HOOK_CLIENT_PROCESS_HIT_END *hook_func_client_process_hit_end;
	HOOK_REFRESH_STALENESS *hook_func_refresh_staleness;
	HOOK_REFRESH_CHECK_PRE_RETURN *hook_func_refresh_check_pre_return;
	HOOK_REFRESH_CHECK_SET_EXPIRES *hook_func_refresh_check_set_expires;
	HOOK_FUNC_PROCESS_MODIFIED_SINCE *hook_func_process_modified_since;
	HOOK_PRIVATE_CLIENT_SEND_MORE_DATA *hook_func_private_client_send_more_data;
	HOOK_PRIVATE_STORE_ENTRY_VALID_LENGTH *hook_func_private_store_entry_valid_length;
	//added for helper`s queue overflow problem
	HOOK_PRIVATE_REGISTER_DEFERFUNC_FOR_HELPER *hook_func_private_register_deferfunc_for_helper;
	HOOK_MODULE_RECONFIGURE *hook_func_module_reconfigure;
	HOOK_IS_DEFER_ACCEPT_REQ *hook_func_is_defer_accept_req;
	HOOK_IS_DEFER_READ_REQ *hook_func_is_defer_read_req;
	//added end
	HOOK_PRIVATE_SET_CLIENT_KEEPALIVE *hook_func_private_set_client_keepalive;
	HOOK_CAN_FIND_VARY * hook_func_can_find_vary;
	HOOK_FWD_START * hook_func_fwd_start;
	HOOK_PRIVATE_CLIENT_KEEPALIVE_SET_TIMEOUT *hook_func_private_client_keepalive_set_timeout;
	HOOK_PRIVATE_QUERY_DIRECT* hook_func_private_query_direct;
	HOOK_PRIVATE_FWD_COMPLETE_CLEAN* hook_func_private_fwd_complete_clean;
	HOOK_IS_WEBLUKER_SET *hook_func_is_webluker_set;
	HOOK_FC_REPLY_HEADER *hook_func_fc_reply_header;
	HOOK_BEFORE_CLIENTPROCESSREQUEST2 *hook_func_before_clientProcessRequest2;
	HOOK_PRIVATE_HTTP_REPL_SEND_END *hook_func_private_http_repl_send_end;
	HOOK_PRIVATE_BEFORE_SEND_HEADER	*hook_func_private_before_send_header;
	HOOK_PRIVATE_PROCESS_SEND_HEADER *hook_func_private_process_send_header;
	HOOK_PRIVATE_PROCESS_RANGE_HEADER *hook_func_private_process_range_header;
	HOOK_HTTP_CHECK_REQUEST_CHANGE *hook_func_http_check_request_change;
	HOOK_HTTPHDRRANGEOFFSETLIMIT *hook_func_httpHdrRangeOffsetLimit;
	HOOK_STOREKEYPUBLICBYREQUESTMETHOD *hook_func_storeKeyPublicByRequestMethod;
	HOOK_PRIVATE_ADD_HTTP_ORIG_RANGE *hook_func_private_add_http_orig_range;
	HOOK_PRIVATE_STORE_PARTIAL_CONTENT *hook_func_private_store_partial_content;
	HOOK_PRIVATE_PRIVATE_CLIENT_CACHE_HIT_START *hook_func_private_client_cache_hit_start;
	HOOK_PRIVATE_CLIENTBUILDRANGEHEADER *hook_func_private_clientBuildRangeHeader;
	HOOK_PRIVATE_AFTER_WRITE_HEADER *hook_func_private_after_write_header;
	HOOK_HTTPSTATEFREE *hook_func_httpStateFree;
	HOOK_PRIVATE_STORE_META_URL *hook_func_private_store_meta_url;
	HOOK_PRIVATE_CHANGE_REQUEST *hook_func_private_change_request;
	HOOK_PRIVATE_SHOULD_CLOSE_FD *hook_func_private_should_close_fd;
	HOOK_PRIVATE_SET_STATUS_LINE *hook_func_private_set_status_line;
	HOOK_PRIVATE_CHANGE_ACCESSLOG_INFO *hook_func_private_change_accesslog_info;
	HOOK_PRIVATE_CHECKMODPARTITIONSTOREENABLE *hook_func_private_checkModPartitionStoreEnable;
	HOOK_PRIVATE_GETREPLYUPDATEBYIMSREP *hook_func_private_getReplyUpdateByIMSRep;
    HOOK_HTTP_REQ_READ          *hook_func_http_req_read;   
    HOOK_HTTP_REQ_READ          *hook_func_http_req_read_second;
	HOOK_HTTP_REQ_READ_THREE * hook_func_http_req_read_three;
	HOOK_HTTP_AFTER_CLIENT_STORE_REWRITE * hook_func_http_after_client_store_rewrite;
	HOOK_PRIVATE_PROCESS_EXCESS_DATA *hook_func_private_process_excess_data;
	HOOK_HTTP_CREATED *hook_func_http_created;
	HOOK_HTTPREQ_NO_CACHE_REASON *hook_func_httpreq_no_cache_reason;
	HOOK_HTTPREP_NO_CACHE_REASON *hook_func_httprep_no_cache_reason;
	HOOK_STORE_NO_CACHE_REASON *hook_func_store_no_cache_reason;
	HOOK_HTTP_REQ_DEBUG_LOG   		*hook_func_http_req_debug_log;
	HOOK_SET_EPOLL_LOAD * hook_func_set_epoll_load;
	HOOK_CLEAR_DIR_LOAD * hook_func_clear_dir_load;
	HOOK_ADD_DIR_LOAD * hook_func_add_dir_load;
	HOOK_SET_FILEMAP_TIME * hook_func_set_filemap_time;
	HOOK_PRIVATE_RECORD_TREND_ACCESS_LOG *hook_func_private_record_trend_access_log;
	HOOK_PRIVATE_SET_TPROXY_IP *hook_func_private_set_tproxy_ip;
	HOOK_CLIENTLSCSCONNECTIONSOPEN *hook_func_clientLscsConnectionsOpen;
	HOOK_PRIVATE_CLIENT_CACHE_HIT_FOR_REDIRECT_LOCATION *hook_func_private_client_cache_hit_for_redirect_location;
	HOOK_GET_CLIENT_IP * hook_func_get_client_area_use_ip;
    HOOK_SEND_HEADER_CLONE_REPLY_BEFORE *hook_func_send_header_clone_before_reply;
	HOOK_PRIVATE_360_WRITE_RECEIVE_WARNING_LOG  *hook_func_private_360_write_receive_warning_log;
	HOOK_PRIVATE_360_WRITE_SEND_WARNING_LOG  *hook_func_private_360_write_send_warning_log;
	HOOK_PRIVATE_GETMULTIRANGEONESIZE *hook_func_private_getMultiRangeOneCopySize;
	HOOK_PRIVATE_GETONCESENDBODYSIZE *hook_func_private_getOnceSendBodySize;
    HOOK_STORE_URL_REWRITE *hook_func_store_url_rewrite;
	HOOK_HTTP_BEFORE_REDIRECT_FOR_BILLING* hook_func_private_http_before_redirect_for_billing;
	HOOK_PRIVATE_SET_NAME_AS_IP *hook_func_private_set_name_as_ip;
	HOOK_PRIVATE_OPENIDLECONN *hook_func_private_openIdleConn;
	HOOK_PRIVATE_HTTP_REPL_READ_END *hook_func_private_http_repl_read_end;
    HOOK_PRIVATE_SET_HOST_AS_IP *hook_func_private_set_host_as_ip;
	HOOK_PRIVATE_DUP_PERSIST_FD *hook_func_private_dup_persist_fd;
    HOOK_PRIVATE_SET_UPPERLAYER_PERSIST_CONNECTION_TIMEOUT *hook_func_private_set_upperlayer_persist_connection_timeout;
    HOOK_PRIVATE_SET_SUBLAYER_TCP_KEEPALIVE *hook_func_private_set_sublayer_tcp_keepalive;
	HOOK_PRIVATE_SET_SUBLAYER_PERSIST_CONNECTION_TIMEOUT *hook_func_private_set_sublayer_persist_connection_timeout;
	HOOK_PRIVATE_SET_SUBLAYER_PERSIST_REQUEST_TIMEOUT *hook_func_private_set_sublayer_persist_request_timeout;
	HOOK_PRIVATE_POST_USE_PERSISTENT_CONNECTION			*hook_func_private_post_use_persistent_connection; 
	HOOK_PRIVATE_SET_COSS_MEMBUF *hook_func_coss_membuf;
	HOOK_GET_HTTP_BODY_BUF * hook_func_get_http_body_buf;
	HOOK_PRIVATE_STORE_META_URL_FOR_REDIRECT_LOCATION *hook_func_private_store_meta_url_for_redirect_location;
    HOOK_PRIVATE_ASSIGN_DIR_SET_PRIVATE_DATA *hook_func_private_assign_dir_set_private_data;
    HOOK_PRIVATE_ASSIGN_DIR *hook_func_private_assign_dir;
	HOOK_PRIVATE_IS_ASSIGN_DIR *hook_func_private_is_assign_dir;
    HOOK_QUICK_ABORT_CHECK * hook_func_quick_abort_check;
	HOOK_REPLY_CANNOT_FIND_DATE *hook_func_reply_cannot_find_date;
	HOOK_GET_CONTENT_ORIG_DOWN *hook_func_get_content_orig_down;
	HOOK_STORE_APPEND_CHECK *hook_func_store_append_check;
	HOOK_PRIVATE_REFORWARDABLE *hook_func_private_reforwardable;
};

struct _cc_mod_param
{
	char acl_name[2048]; //acl name eg:allow acl1 acl2
	long count; //the number of param is quoted 
	void *param; //mod param
	FREE_CALLBACK *call_back;
};

struct _cc_mod_params_list
{
	Array mod_params;
	cc_mod_params_list *next;
};

struct _cc_private_data
{
	void *private_data;
	FREE_CALLBACK *call_back;	
};

struct _cc_acl_check_result
{
	int* param_poses;
	int param_count;
};
#endif
