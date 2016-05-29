#include "cc_framework_api.h"
#ifdef CC_FRAMEWORK

/*
 *Make public
 *return 6 means must cache this object
 *the others value :cant deal with this reply	
 */

static int makePublic(int fd, HttpStateData *httpState)
{
	debug(139, 3)("(mod_store_range) ->  makePublic\n");
	if (httpState->entry->mem_obj->reply->sline.status == HTTP_PARTIAL_CONTENT){
		debug(139, 3)("mod_store_range: store range response beacase of 206 \n");
		return 6;
	}
	return 0;
}

//standard safeguard chain: store_url = the part of url which is before "?"
static char *store_key_url(char* url){

	char * uri = xstrdup(url);
	char *tmpuri = strstr(uri,"?");
	if(tmpuri != NULL)
		*tmpuri = 0;

	//debug(139, 3)("(mod_store_range) -> store_url is %s \n", uri);
	return uri;
}

//qq safeguard chain: 1 remove ip 
static char *store_qq_key_url(char* url){

	char *new_url = url;
	char *s;
	char *e;

	s = strstr(new_url, "://");
	s += 3;
	e = strstr(s, "/");
	if( e != NULL)
	{
		memmove(s, e+1, strlen(e+1));
		*(s+strlen(e+1)) = 0;
	}

	//debug(139, 3)("(mod_store_range:) -> remove ip url is %s \n", new_url);
	return new_url;
}

static int modifyStoreurl(clientHttpRequest *http){

	HttpHeaderEntry *range = httpHeaderFindEntry(&http->request->header, HDR_RANGE);
	if(range == NULL){
		debug(139, 3)("mod_store_range: not range for request not range header : \n");
		return -1;
	}
	assert(range);

	char *rangevalue = range->value.buf;
	char *tmp_quest = store_key_url(http->uri);
	store_qq_key_url(tmp_quest);

	debug(139,3)("mod_store_range store range value =[%s]\n" ,rangevalue);

	StoreEntry *e = NULL;
	e = storeGetPublic(tmp_quest,METHOD_GET);

	if(e == NULL) {		/* MISS */
		char tmparray[2048];
		memset(tmparray,0,2048);
		snprintf(tmparray, 1024, "%s?Range=%s", tmp_quest,rangevalue);
		safe_free(http->request->store_url);
		http->request->store_url = xstrdup(tmparray);
		debug(139, 3)("mod_store_range: object MISS modify store range =%s\n", http->request->store_url);
	}
	else {	/* HIT */
		safe_free(http->request->store_url);
		http->request->store_url = xstrdup(tmp_quest);
		debug(139, 3)("mod_store_range: object HIT modify store_url=%s\n", http->request->store_url);
	}

	safe_free(tmp_quest);
	return 0;
}

/* module init */
int mod_register(cc_module *module)
{
	debug(139, 1)("(mod_store_range) ->  init: init module\n");
	strcpy(module->version, "7.0.R.16488.i686");

	cc_register_hook_handler(HPIDX_hook_func_http_repl_cachable_check,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_repl_cachable_check),
			makePublic);

	cc_register_hook_handler(HPIDX_hook_func_http_req_read_second,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_read_second),
			modifyStoreurl);

	return 0;
}

#endif
