/* 
 * Author: GSP/chenqi
 * Data: 2013/03/13
 * Anti-hijack between lower FC and upper FC 
 */
#include "cc_framework_api.h"
#ifdef CC_FRAMEWORK

static cc_module* mod = NULL;
static MemPool * mod_config_pool = NULL;

struct mod_conf_param
{
	short send;
	short check;
};

static void * mod_config_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == mod_config_pool) {
		mod_config_pool = memPoolCreate("mod_check_response config_struct", sizeof(struct mod_conf_param));
	}
	return obj = memPoolAlloc(mod_config_pool);
}

static int free_callback(void* param)
{
	struct mod_conf_param* data = (struct mod_conf_param*) param;
	if (data) {
		memPoolFree(mod_config_pool, data);
	}
	data = NULL;
	return 0;
}

struct send_random {
	char number[10];
};

int free_random(void *data)
{
	struct send_random *param  = (struct send_random*) data;
	if (NULL != param) {
		safe_free(param);
		param = NULL;
	}
	return 0;
}

static const char* httpHeaderGetValue(HttpHeader *h, const char *name)
{
	HttpHeaderPos pos = HttpHeaderInitPos;
	HttpHeaderEntry *curHE; 
	while ((curHE = httpHeaderGetEntry(h, &pos))) {
		if(!strcasecmp(strBuf(curHE->name), name)) {
			return strBuf(curHE->value);
		}   
	}   
	return NULL;
}

static char* url2host(char* des, char* src)
{
	char *start = src;

	if (memcmp(src, "http://", 7) == 0 ) { 
		start += 7;
	} else if (memcmp(src, "https://", 8) == 0 ) { 
		start += 8;
	}   

	while ((*start != ':') && (*start != '/') && (*start != 0) ) { 
		*des = *start;
		des++;
		start++;
	}   

	*des = 0;
	return des;
}

// md5 algorithm from squid self
// I do not verify this function...
static void GetMD5Digest(const char *key, unsigned char *digest)
{
	SQUID_MD5_CTX M;
	SQUID_MD5Init(&M);
	SQUID_MD5Update(&M, (unsigned char *) key, strlen(key));
	SQUID_MD5Final(digest, &M);
}

// mod_check_response send=1 check=1 allow acl_name
// send, control send or not send X-CC-UP-CHECK header to client
// check, decide whether check X-CC-UP-CHECK value from origin
static int func_sys_parse_param(char *cfg_line)
{
	struct mod_conf_param *data = NULL;
	data = mod_config_pool_alloc();
	memset(data, 0, sizeof(struct mod_conf_param));

	char *token = NULL;
	if (NULL == (token = strtok(cfg_line, w_space)))
		goto error;
	if (strcmp(token, "mod_check_response"))
		goto error;

	while (NULL != (token = strtok(NULL, w_space))) {
		if (!strcasecmp(token, "allow") || !strcasecmp(token, "deny"))
			break;
		else if (!strcasecmp(token, "send=1")) 
			data->send = 1;
		else if (!strcasecmp(token, "check=1")) 
			data->check = 1;
	}

	debug(107,1)("mod_check_response: parse successfully, send=%d\tcheck=%d\n", data->send, data->check);
	cc_register_mod_param(mod, data, free_callback);
	return 0;

error:
	debug(107, 1)("mod_check_response: [%s] parse error\n", cfg_line);
	free_callback(data);
	return -1;		
}


/* the action taken by lower FC */
// send a special header to upper FC
static int SendRandomNumber(HttpStateData *httpState, HttpHeader* hdr)
{
	struct mod_conf_param *cfg = cc_get_mod_param(httpState->fd, mod);
	if (NULL == cfg)
		return -1;

	struct timeval tp;
	if (gettimeofday(&tp, NULL)) {
		debug(107, 2)("mod_check_response: gettimeofday error\n");
		return -1;
	}

	struct send_random *data = cc_get_mod_private_data(FWDSTATE_PRIVATE_DATA, httpState->fwd, mod);
	if (!data) {
		data = xcalloc(1, sizeof(struct send_random));
		snprintf(data->number, 10, "%ld", tp.tv_usec);
		cc_register_mod_private_data(FWDSTATE_PRIVATE_DATA, httpState->fwd, data, free_random, mod);
		httpHeaderAddEntry(hdr, httpHeaderEntryCreate(HDR_OTHER, "X-CC-DOWN-CHECK", data->number));
		debug(107,3)("mod_check_response: send X-CC-DOWN-CHECK=[%s] to orgin\n", data->number);
	}
	return 0;
}

// check X-CC-UP-CHECK header
static int check_and_mark(int fd, HttpStateData *httpState)
{
	struct mod_conf_param *cfg = cc_get_mod_param(httpState->fd, mod);
	if (NULL == cfg)
		return -1;

	struct send_random *data = cc_get_mod_private_data(FWDSTATE_PRIVATE_DATA, httpState->fwd, mod);
	if (NULL == data)
		return -1;

	char key[512];  
	memset(key, 0, 512);
	unsigned char md5[SQUID_MD5_DIGEST_LENGTH];
	memset(md5, 0, SQUID_MD5_DIGEST_LENGTH);

	snprintf(key, 511, "%s%s", data->number, httpState->request->host);
	GetMD5Digest(key, md5);

	// got check infomation
	HttpReply* reply = httpState->entry->mem_obj->reply;
	const char *up_buf = httpHeaderGetValue(&reply->header, "X-CC-UP-CHECK");

	if (1 == cfg->check) {
		if (!up_buf) {
			debug(107,1)("mod_check_response: not received X-CC-UP-CHECK header, do not cache it\n");
			return 5;
		}
		else if ((memcmp(md5, (unsigned char*)up_buf, SQUID_MD5_DIGEST_LENGTH))) {
			debug(107,1)("mod_check_response: X-CC-UP-CHECK[%s] not equal [%s], do not cache it\n", up_buf, md5);
			return 5;
		}
		else {
			debug(107,5)("mod_check_response: X-CC-UP-CHECK[%s] match [%s]\n", up_buf, md5);
			return 0;
		}
	} else {
		debug(107,5)("mod_check_response: [%s] do not need to check\n", httpState->request->urlpath.buf);
		return 0;
	}
}

/* the action taken by upper FC */
// send check header  "X-CC-DOWN-CHECK" to lower FC
static int SendCheckHeader(clientHttpRequest *http)
{
	// do not send X-CC-UP-CHECK to client as possible
	httpHeaderDelByName(&http->reply->header, "X-CC-UP-CHECK");

	struct mod_conf_param *cfg = cc_get_mod_param(http->conn->fd, mod);
	if (NULL == cfg || !cfg->send) {
		debug(107,3)("mod_check_response: no need to send X-CC-UP-CHECK header\n");
		return -1;
	}

	const char *down_buf = httpHeaderGetValue(&http->request->header, "X-CC-DOWN-CHECK");
	if (NULL == down_buf) {
		if (cfg->send)
			debug(107,2)("mod_check_response: do not send X-CC-UP-CHECK header, for not received X-CC-DOWN-CHECK header\n");
		return -1;
	}

	char key[512];  
	memset(key, 0, 512);
	unsigned char md5[SQUID_MD5_DIGEST_LENGTH];
	memset(md5, 0, SQUID_MD5_DIGEST_LENGTH);

	strncpy(key, down_buf, 511);
	char *host = key + strlen(key);
	url2host(host, http->uri);
	GetMD5Digest(key, md5);

	assert(cfg->send);
	httpHeaderAddEntry(&http->reply->header, httpHeaderEntryCreate(HDR_OTHER, "X-CC-UP-CHECK", (char*)md5));
	debug(107, 3)("mod_check_response: send X-CC-UP-CHECK=[%s] header to client\n", md5);
	return 0;
}


/* module init */
int mod_register(cc_module *module)
{
	debug(107, 1)("(mod_check_response) ->  init: init module\n");
	strcpy(module->version, "7.0.R.16488.i686");

	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
			func_sys_parse_param);

	// FC ==> client
	cc_register_hook_handler(HPIDX_hook_func_http_repl_send_start,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_repl_send_start),
			SendCheckHeader);

	// FC ==> origin
	cc_register_hook_handler(HPIDX_hook_func_http_req_send_exact,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_send_exact),
			SendRandomNumber);

	// origin ==> FC
	cc_register_hook_handler(HPIDX_hook_func_http_repl_cachable_check,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_repl_cachable_check),
			check_and_mark);

	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	return 0;
}
#endif
