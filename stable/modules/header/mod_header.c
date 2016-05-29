#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK
//???Ǳ?ģ??????????????????Ϣ?Ľṹָ??
static cc_module* mod = NULL;
extern void httpReplyHdrCacheInit(HttpReply * rep);
extern void httpReplyHdrCacheClean(HttpReply * rep);
extern time_t httpReplyHdrExpirationTime(const HttpReply * rep);
//header ?? ??Ӧvalue?Ľṹ. ???ݲ????? action???Է????Ľ??в???.
struct header_info {
	String header;
	String value;
};
//ÿ??acl?????ж???????, ?Ǿ???Ҫ?????ṹ��??Ӧ.??????????ÿ???????ṹ
struct action_part {
	int direct; //0-3: 0:????fc֮ǰ 1:?ƽ???Դվ 2:Դվ??��????fc֮ǰ 3:??fc??ȥ???ͻ?
	int action; //0-3: 0:????һ??header 1:ɾ??һ??header 2:?޸?ָ??header??value 3:updateһ??header
	struct header_info* hdr;
};

//???Ǻ?acl??Ӧ??mod para?ṹ?Ĺ???ֵ???????ܿ???Ϊƥ????Ӧacl??fd????һ????Ӧ??para
struct mod_conf_param {
	int count; //ģ????Ŀ
	int flag;
	struct action_part* acp[128]; //????....Ӧ??û?г???128??header??һ??????Ҫ?????İ??
};


static MemPool * header_info_pool = NULL;
static MemPool * action_part_pool = NULL;
static MemPool * mod_config_pool = NULL;

static void * header_info_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == header_info_pool)
	{
		header_info_pool = memPoolCreate("mod_header config_struct header_info", sizeof(struct header_info));
	}
	return obj = memPoolAlloc(header_info_pool);
}

static void * action_part_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == action_part_pool)
	{
		action_part_pool = memPoolCreate("mod_header config_struct action_part", sizeof(struct action_part));
	}
	return obj = memPoolAlloc(action_part_pool);
}

static void *  mod_config_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == mod_config_pool)
	{
		mod_config_pool = memPoolCreate("mod_header config_struct", sizeof(struct mod_conf_param));
	}
	return obj = memPoolAlloc(mod_config_pool);
}

//???????ͷŵ?callback
static int free_callback(void* param)
{
	int loop;
	struct mod_conf_param* data = (struct mod_conf_param*) param;

	if(data)
	{
		for(loop=0; loop<data->count; loop++)
		{
			if(data->acp[loop])
			{
				if(data->acp[loop]->hdr)
				{
					if(strLen(data->acp[loop]->hdr->header))
						stringClean(&data->acp[loop]->hdr->header);
					if(strLen(data->acp[loop]->hdr->value))
						stringClean(&data->acp[loop]->hdr->value);		
					memPoolFree(header_info_pool, data->acp[loop]->hdr);
					data->acp[loop]->hdr = NULL;
				}
				memPoolFree(action_part_pool, data->acp[loop]);
				data->acp[loop] = NULL;
			}
		}
		memPoolFree(mod_config_pool, data);
		data = NULL;
	}

	return 0;
}

//parse the operation par
static int parse_operation(struct action_part* part, char* token)
{
	assert(token);

	if(!strcmp(token, "add"))
	{
		part->action = 0;
	}
	else if(!strcmp(token, "del"))
	{
		part->action = 1;
	}
	else if(!strcmp(token, "modify"))
	{
		part->action = 2;
	}
	else if(!strcmp(token, "update"))
	{
		part->action = 3;
	}
	else
	{
		return -1;
	}
	return 0;
}


// confige line example:
// mod_header [direction add|del|modify header_name [header_value]]{1,..n} allow|deny acl
// ??????ͷ??ʼ?????? allow|deny ??ֹ
// ????????????
static int func_sys_parse_param(char *cfg_line)
{
	assert(cfg_line);

	struct mod_conf_param* data = NULL;
	char* token = NULL;
	int ret = 0;
	int i;
	
	// add by xueye.zhao
	// 2013/6/19
	
# if 0
	//?ҵ?allow????deny?? Ȼ????֮???Ľ?ȥ
	//zhoudshu add for bug mantis 0002002

	char* tmptoken = NULL;
	char* tmp_line = xstrdup(cfg_line);
	if ((tmptoken = strtok(tmp_line, w_space)) == NULL)
	{	
		debug(98, 3)("(mod_header) ->  parse line error\n");
		safe_free(tmp_line);
		return -1;
	
	}

	int haskey = 0;
	while(NULL != (tmptoken = strtok(NULL, w_space)))
	{
		if(strcmp("allow",tmptoken) == 0 || strcmp("deny",tmptoken) == 0){
			haskey = 1;
		}
	}

	safe_free(tmp_line);
	
	if(haskey != 1){
		debug(98, 3)("(mod_header) ->  has not key of allow or deny in config line \n");
		return -1;
	}	

#endif
	

	if (NULL == strstr(cfg_line, " allow "))
	{
		debug(98, 3)("(mod_header) ->  has not key of allow or deny in config line \n");
		return -1;
	}
	
	//end add

	//开始截取	
	if ((token = strtok(cfg_line, w_space)) != NULL)
	{
		if(strcmp(token, "mod_header"))
			goto errA;	
	}
	else
	{
		debug(98, 3)("(mod_header) ->  parse line error\n");
		goto errA;	
	}

	data = mod_config_pool_alloc();
	data->count = 0;
	//?????￪ʼѭ??ȡ??????..???ǲ???+header+value?ķ?ʽ, ????,ѭ????????
	while(NULL != (token = strtok(NULL, w_space)))
	{
		if(strcmp("allow",token) == 0 || strcmp("deny",token) == 0 || strcmp("reply_check",token) == 0) {
			break;
		}	

		struct action_part* part;
		struct header_info* info;
		part = action_part_pool_alloc();
		info = header_info_pool_alloc();
		data->acp[data->count] = part;
		data->acp[data->count]->hdr = info;
		data->count++;
		
		part->direct = atoi(token); //fetch the direction

		if(part->direct > 3 || part->direct < 0)
		{
			debug(98, 3)("(mod_header) ->  parse direction error, cant be %d\n", part->direct);
			goto err;
		}

		//???￪ʼ?ǽ???????????
		if (NULL == (token = strtok(NULL, w_space)))
		{
			debug(98, 3)("(mod_header) ->  parse line data does not existed\n");
			goto err;
		}
		ret = parse_operation(part, token);//??????????, ???????????ṹ
		if(-1 == ret)
		{
			debug(98, 3)("(mod_header) ->  parse line operation error, cant be %s\n", token);
			goto err;
		}

		//?????и?header, ?????϶??е?
		if (NULL == (token = strtok(NULL, w_space)))
		{
			debug(98, 3)("(mod_header) ->  parse line data does not existed\n");
			goto err;
		}
		stringInit(&info->header, token);	

		//?и???header, value?Ͳ?һ??????. ????Ҫ??????��?ж???. ??????Ҫ?и?value, ?͸???
		if (1 != part->action)
		{
			if (NULL == (token = strtok(NULL, w_space))) {
				debug(98, 3)("(mod_header) ->  parse line data does not existed\n");
				goto err;
			}

			//??Ȼ??value, ?ͱ???????. ?? header һ???????ŵ?????.
			stringInit(&info->value, token);	

			/* case http://sis.ssr.chinacache.com/rms/view.php?id=4124 */
			if (*token == '\"') {
				stringClean(&info->value);
				stringInit(&info->value, token+1);	
				int len = 0;
				while (NULL !=(token = strtok(NULL,w_space))){
					len = strlen(token);
					if (token[len-1] == '\"'){
						stringAppend(&info->value, " ", 1);
						stringAppend(&info->value, token, len-1);
						break;
					}
					else if (strcmp("allow",token) == 0 || strcmp("deny",token) == 0) 
						goto err;
					else {
						stringAppend(&info->value, " ", 1);
						stringAppend(&info->value, token, len);
					}
				}
			}
		}
	}
	//һ??û???κ????ݣ? ֱ?ӱ???
	if(data->count == 0)
		goto err;
	else
		cc_register_mod_param(mod, data, free_callback);
	return 0;

err:
	debug(98,1)("mod_header: parse error\n");
	for(i=0; i<data->count; i++)
	{
		if(data->acp[i]->hdr)
		{
			if(strLen(data->acp[i]->hdr->header))
				stringClean(&data->acp[i]->hdr->header);
			if(strLen(data->acp[i]->hdr->value))
				stringClean(&data->acp[i]->hdr->value);
			memPoolFree(header_info_pool, data->acp[i]->hdr);
			data->acp[i]->hdr = NULL;
		}
		if(data->acp[i])
		{
			memPoolFree(action_part_pool, data->acp[i]);
			data->acp[i] = NULL;
		}
	}
errA:
	if (data)
	{
		memPoolFree(mod_config_pool, data);
		data = NULL;
	}
	return -1;
}

#if 0
/*
 *?µĲ??ҷ?ʽ??һЩsquid??????????header?Ĳ???
 */
static HttpHeaderEntry* httpHeaderFindEntry2(const HttpHeader * hdr, http_hdr_type id)
{
	//zhb:let it be faster
	HttpHeaderPos pos = HttpHeaderInitPos + HDR_ENUM_END;
	HttpHeaderEntry *e;         
	assert(hdr);
	//assert_eid(id);

	/* check mask first */
	if (!CBIT_TEST(hdr->mask, id))
		return NULL;
	
	if(id != HDR_OTHER)
	{
		return hdr->entries.items[hdr->pSearch[id]];
	}
	/* looks like we must have it, do linear search */
	while ((e = httpHeaderGetEntryPlus(hdr, &pos)))
	{
		if (e->id == id)
			return e;
	}

	/* hm.. we thought it was there, but it was not found */
	assert(0);            
	return NULL;        /* not reached */
}
#endif

/*
 *
 */
#if 0
static void httpReplyHdrCacheClean_(HttpReply * rep)
{
	stringClean(&rep->content_type);
	if (rep->cache_control)
		httpHdrCcDestroy(rep->cache_control);
	if (rep->content_range)
		httpHdrContRangeDestroy(rep->content_range);
}

#endif

/*
 *
 */
#if 0
static time_t
httpReplyHdrExpirationTime_(const HttpReply * rep)
{
	if (rep->cache_control)
	{
		if (rep->date >= 0)
		{
			if (rep->cache_control->s_maxage >= 0)
				return rep->date + rep->cache_control->s_maxage;
			if (rep->cache_control->max_age >= 0)
				return rep->date + rep->cache_control->max_age;
		}
		else
		{
			if (rep->cache_control->s_maxage >= 0)
				return squid_curtime;
			if (rep->cache_control->max_age >= 0)
				return squid_curtime;
		}
	}
	if (Config.onoff.vary_ignore_expire &&
			httpHeaderHas(&rep->header, HDR_VARY))
	{
		const time_t d = httpHeaderGetTime(&rep->header, HDR_DATE);
		const time_t e = httpHeaderGetTime(&rep->header, HDR_EXPIRES);
		if (d == e)
			return -1;
	}
	if (httpHeaderHas(&rep->header, HDR_EXPIRES))
	{
		const time_t e = httpHeaderGetTime(&rep->header, HDR_EXPIRES);
		return e < 0 ? squid_curtime : e;
	}
	return -1;
}


#endif
/*
 *
 */
#if 0
static void
httpReplyHdrCacheInit_(HttpReply * rep)
{
	const HttpHeader *hdr = &rep->header;
	const char *str;
	rep->content_length = httpHeaderGetSize(hdr, HDR_CONTENT_LENGTH);
	rep->date = httpHeaderGetTime(hdr, HDR_DATE);
	rep->last_modified = httpHeaderGetTime(hdr, HDR_LAST_MODIFIED);
	str = httpHeaderGetStr(hdr, HDR_CONTENT_TYPE);
	if (str)
		stringLimitInit(&rep->content_type, str, strcspn(str, ";\t "));
	else    
		rep->content_type = StringNull;
	rep->cache_control = httpHeaderGetCc(hdr);
	rep->content_range = httpHeaderGetContRange(hdr);
	rep->keep_alive = httpMsgIsPersistent(rep->sline.version, &rep->header);
	/* be sure to set expires after date and cache-control */
	rep->expires = httpReplyHdrExpirationTime_(rep);
}
#endif

/*
 *?????Ǵ????޸ġ?????һ??header?Ĵ??���??
 */
static int modifyHeader0(struct action_part* acp, request_t* request)
{
	assert(acp);
	assert(request);

	struct header_info* hdr = acp->hdr;
	int act = acp->action;

	int flag = 0;
	HttpHeaderEntry e;
	//HttpHeaderEntry *mye;
	int i;
	HttpHeaderEntry *myheader;
	HttpHeaderPos pos = HttpHeaderInitPos + HDR_ENUM_END;
	e.name = stringDup(&hdr->header);	
	e.value = stringDup(&hdr->value);
	i = httpHeaderIdByNameDef(strBuf(hdr->header), strLen(hdr->header));
	e.id = i;
	if(i == -1)
	{
		e.id = HDR_OTHER;
		if(0 == act)
		{
			httpHeaderAddEntry(&request->header, httpHeaderEntryClone(&e));
		}
		else if(2 == act)
		{
			while ((myheader = httpHeaderGetEntryPlus(&request->header, &pos))) 
			{
				if (myheader->id == HDR_OTHER && strCaseCmp(myheader->name, strBuf(hdr->header)) == 0)
				{
					debug(98, 3)("%s is myheader->value,%s is hdr->value\n",strBuf(myheader->value), strBuf(hdr->value));
					stringReset(&myheader->value, strBuf(hdr->value));

				}
			}
		}
		else if(1 == act)
		{
			httpHeaderDelByName(&request->header,strBuf(hdr->header));
		}
		else if(3 == act)
		{
			while ((myheader = httpHeaderGetEntryPlus(&request->header, &pos))) 
			{
				if (myheader->id == HDR_OTHER && strCaseCmp(myheader->name, strBuf(hdr->header)) == 0)
				{
					debug(98, 3)("%s is myheader->value,%s is hdr->value\n",strBuf(myheader->value), strBuf(hdr->value));
					flag = 1;
					stringReset(&myheader->value, strBuf(hdr->value));

				}
			}

			if(!flag)
				httpHeaderAddEntry(&request->header, httpHeaderEntryClone(&e));
		}
	}
	else
	{
		//mye = httpHeaderFindEntry2(&request->header, i);
		//debug(98, 3) ("%d is i\n", i);

		if(0 == act)
		{
			httpHeaderAddEntry(&request->header, httpHeaderEntryClone(&e));
		}
		else if(2 == act)
		{
			if(httpHeaderDelByName(&request->header,strBuf(hdr->header)))
			{
				httpHeaderAddEntry(&request->header, httpHeaderEntryClone(&e));
			}
			//mye = httpHeaderFindEntry2(&request->header, i);
			//debug(98, 3)("%s is newvalue\n",strBuf(mye->value));
		}
		else if(1 == act)
		{
			httpHeaderDelByName(&request->header,strBuf(hdr->header));
		}
		else if(3 == act)
		{
			httpHeaderDelByName(&request->header,strBuf(hdr->header));
			httpHeaderAddEntry(&request->header, httpHeaderEntryClone(&e));
			//mye = httpHeaderFindEntry2(&request->header, i);
			//debug(98, 3)("%s is newvalue\n",strBuf(mye->value));
		}
	}
	stringClean(&e.name);
	stringClean(&e.value);

	return 0;
}



/*
 *?????Ǵ????޸ġ?????һ??header?Ĵ??���??
 */
static int modifyHeader1(struct action_part* acp, request_t* request, HttpHeader* hdrinfo)
{
	assert(acp);
	assert(request);

	struct header_info* hdr = acp->hdr;

	int flag = 0;
	int act = acp->action;
	HttpHeaderEntry e;
	//HttpHeaderEntry *mye;
	int i;
	HttpHeaderEntry *myheader;
	HttpHeaderPos pos = HttpHeaderInitPos + HDR_ENUM_END;
	e.name = stringDup(&hdr->header);	
	e.value = stringDup(&hdr->value);
	i = httpHeaderIdByNameDef(strBuf(hdr->header), strLen(hdr->header));
	e.id = i;
	if(i == -1)
	{
		e.id = HDR_OTHER;
		if(0 == act)
		{
			httpHeaderAddEntry(hdrinfo, httpHeaderEntryClone(&e));
		}
		else if(2 == act)
		{
			while ((myheader = httpHeaderGetEntryPlus(hdrinfo, &pos))) 
			{
				if (myheader->id == HDR_OTHER && strCaseCmp(myheader->name, strBuf(hdr->header)) == 0)
				{
					debug(98, 3)("%s is myheader->value,%s is hdr->value\n",strBuf(myheader->value), strBuf(hdr->value));
					stringReset(&myheader->value, strBuf(hdr->value));

				}
			}
		}
		else if(1 == act)
		{
			httpHeaderDelByName(hdrinfo, strBuf(hdr->header));
		}
		else if(3 == act)
		{
			while ((myheader = httpHeaderGetEntryPlus(hdrinfo, &pos))) 
			{
				if (myheader->id == HDR_OTHER && strCaseCmp(myheader->name, strBuf(hdr->header)) == 0)
				{
					debug(98, 3)("%s is myheader->value,%s is hdr->value\n",strBuf(myheader->value), strBuf(hdr->value));
					flag = 1;
					stringReset(&myheader->value,strBuf(hdr->value));

				}
			}

			if(!flag)
				httpHeaderAddEntry(hdrinfo, httpHeaderEntryClone(&e));
		}
	}
	else
	{
		//mye = httpHeaderFindEntry2(hdrinfo, i);
		//debug(98, 3) ("%d is i\n", i);

		if(0 == act)
		{
			httpHeaderAddEntry(hdrinfo, httpHeaderEntryClone(&e));
		}
		else if(2 == act)
		{
			if(httpHeaderDelByName(hdrinfo,strBuf(hdr->header)))
			{
				httpHeaderAddEntry(hdrinfo, httpHeaderEntryClone(&e));
			}
			//mye = httpHeaderFindEntry2(hdrinfo, i);
			//debug(98, 3)("%s is newvalue\n",strBuf(mye->value));
		}
		else if(1 == act)
		{
			httpHeaderDelByName(hdrinfo,strBuf(hdr->header));
		}
		else if(3 == act)
		{
			httpHeaderDelByName(hdrinfo,strBuf(hdr->header));
			httpHeaderAddEntry(hdrinfo, httpHeaderEntryClone(&e));
		}
	}
	stringClean(&e.name);
	stringClean(&e.value);

	return 0;
}


/*
 *?????Ǵ????޸ġ?????һ??header?Ĵ??���??
 */
static int modifyHeader2(struct action_part* acp, HttpReply* reply)
{
	assert(acp);
	assert(reply);

	struct header_info* hdr = acp->hdr;

	int flag = 0;
	HttpHeaderEntry e;
	int act = acp->action;
	//HttpHeaderEntry *mye;
	int i;
	HttpHeaderEntry *myheader;
	HttpHeaderPos pos = HttpHeaderInitPos + HDR_ENUM_END;
	e.name = stringDup(&hdr->header);
	e.value = stringDup(&hdr->value);
	i = httpHeaderIdByNameDef(strBuf(hdr->header), strLen(hdr->header));
	e.id = i;
	if(i == -1)
	{
		e.id = HDR_OTHER;
		if(0 == act)
		{
			httpHeaderAddEntry(&reply->header, httpHeaderEntryClone(&e));
		}
		else if(2 == act)
		{
			while ((myheader = httpHeaderGetEntryPlus(&reply->header, &pos))) 
			{
				if (myheader->id == HDR_OTHER && strCaseCmp(myheader->name, strBuf(hdr->header)) == 0)
				{
					debug(98, 0)("%s is myheader->value,%s is hdr->value\n",strBuf(myheader->value), strBuf(hdr->value));
					stringReset(&myheader->value, strBuf(hdr->value));

				}
			}
		}
		else if(1 == act)
		{
			httpHeaderDelByName(&reply->header,strBuf(hdr->header));
		}
		else if(3 == act)
		{
			while ((myheader = httpHeaderGetEntryPlus(&reply->header, &pos))) 
			{
				if (myheader->id == HDR_OTHER && strCaseCmp(myheader->name, strBuf(hdr->header)) == 0)
				{
					debug(98, 3)("%s is myheader->value,%s is hdr->value\n",strBuf(myheader->value), strBuf(hdr->value));
					flag = 1;
					stringReset(&myheader->value, strBuf(hdr->value));

				}
			}
			
			if(!flag)
				httpHeaderAddEntry(&reply->header, httpHeaderEntryClone(&e));
		}
	}
	else
	{
		//mye = httpHeaderFindEntry2(&reply->header, i);
		//debug(98, 3) ("%d is i\n", i);

		if(0 == act)
		{
			httpHeaderAddEntry(&reply->header, httpHeaderEntryClone(&e));
		}
		else if(2 == act)
		{
			//	debug(98, 0)("is newvalue?????????????????????????????\n");
			if(httpHeaderDelByName(&reply->header,strBuf(hdr->header)))
			{
				httpHeaderAddEntry(&reply->header, httpHeaderEntryClone(&e));
			}
			//mye = httpHeaderFindEntry2(&reply->header, i);
			//debug(98, 0)("%s is newvalue?????????????????????????????\n",strBuf(mye->value));
		}
		else if(1 == act)
		{
			httpHeaderDelByName(&reply->header,strBuf(hdr->header));
		}
		else if(3 == act)
		{
			httpHeaderDelByName(&reply->header,strBuf(hdr->header));
			httpHeaderAddEntry(&reply->header, httpHeaderEntryClone(&e));
		}
	}
	stringClean(&e.name);
	stringClean(&e.value);

	return 0;
}


/*
 *?????Ǵ????޸ġ?????һ??header?Ĵ??���??
 */
static int modifyHeader3(struct action_part* acp, HttpReply* reply)
{
	assert(acp);
	assert(reply);

	int flag = 0;
	int act = acp->action;
	struct header_info* hdr = acp->hdr;
	HttpHeaderEntry e;
	//HttpHeaderEntry *mye;
	int i;
	HttpHeaderEntry *myheader;
	HttpHeaderPos pos = HttpHeaderInitPos + HDR_ENUM_END;
	e.name = stringDup(&hdr->header);
	e.value = stringDup(&hdr->value);
	i = httpHeaderIdByNameDef(strBuf(hdr->header), strLen(hdr->header));
	e.id = i;
	if(i == -1)
	{
		e.id = HDR_OTHER;
		if(0 == act)
		{
			httpHeaderAddEntry(&reply->header, httpHeaderEntryClone(&e));
		}
		else if(2 == act)
		{
			while ((myheader = httpHeaderGetEntryPlus(&reply->header, &pos))) 
			{
				if (myheader->id == HDR_OTHER && strCaseCmp(myheader->name, strBuf(hdr->header)) == 0)
				{
					debug(98, 3)("%s is myheader->value,%s is hdr->value\n",strBuf(myheader->value), strBuf(hdr->value));
					stringReset(&myheader->value, strBuf(hdr->value));

				}
			}
		}
		else if(1 == act)
		{
			httpHeaderDelByName(&reply->header,strBuf(hdr->header));
		}
		else if(3 == act)
		{
			while ((myheader = httpHeaderGetEntryPlus(&reply->header, &pos))) 
			{
				if (myheader->id == HDR_OTHER && strCaseCmp(myheader->name, strBuf(hdr->header)) == 0)
				{
					debug(98, 3)("%s is myheader->value,%s is hdr->value\n",strBuf(myheader->value), strBuf(hdr->value));
					flag = 1;
					stringReset(&myheader->value, strBuf(hdr->value));

				}
			}
			
			if(!flag)
				httpHeaderAddEntry(&reply->header, httpHeaderEntryClone(&e));
		}
	}
	else
	{
		//mye = httpHeaderFindEntry2(&reply->header, i);
		//debug(98, 3) ("%d is i\n", i);

		if(0 == act)
		{
			httpHeaderAddEntry(&reply->header, httpHeaderEntryClone(&e));
		}
		else if(2 == act)
		{
			if(httpHeaderDelByName(&reply->header,strBuf(hdr->header)))
			{
				httpHeaderAddEntry(&reply->header, httpHeaderEntryClone(&e));
			}
			//mye = httpHeaderFindEntry2(&reply->header, i);
			//debug(98, 3)("%s is newvalue\n",strBuf(mye->value));
		}
		else if(1 == act)
		{
			httpHeaderDelByName(&reply->header,strBuf(hdr->header));
		}
		else if(3 == act)
		{
			httpHeaderDelByName(&reply->header,strBuf(hdr->header));
			httpHeaderAddEntry(&reply->header, httpHeaderEntryClone(&e));
		}
	}
	stringClean(&e.name);
	stringClean(&e.value);

	return 0;
}


/*
 *???Ǵ??????????û?????????header???? direct = 0 ʱ?Ĵ???
 */
static int func_http_req_read(clientHttpRequest *http)
{
	assert(http);

	int fd = http->conn->fd;
	Array * modparam = cc_get_mod_param_array(fd, mod);
	int paramcount = cc_get_mod_param_count(fd, mod);

	assert(modparam);

	assert(http->request);
	request_t* request = http->request;

	struct mod_conf_param *param = NULL;
	int i = 0;
	int loop = 0;
	int ret = 0;
	int count = 0;

	for(i=0; i<paramcount; i++){
		assert(modparam->items[i]);
		param = (struct mod_conf_param *)((cc_mod_param*)modparam->items[i])->param;
		assert(param);
		count = param->count;

		ret =0;
		for(loop=0; loop<count; loop++)
		{
			if(0 == param->acp[loop]->direct)
			{
				ret = modifyHeader0(param->acp[loop], request);
				ret++;
			}
		}
		//??һ??header???????��� ????
		if(ret < 0)
		{   
			return -1;
		}
	}


	return 0;
}

/*
 *???͸?Դվ???????Ĵ??? direct = 1ʱ?Ĵ???
 */
static int func_http_req_send(HttpStateData *data, HttpHeader* hdr)
{

	assert(data);

	int fd = data->fd;
	Array * modparam = cc_get_mod_param_array(fd, mod);
	int paramcount = cc_get_mod_param_count(fd, mod);

	assert(modparam);

	struct mod_conf_param *param = NULL;
	int count = 0; 
	int loop = 0;
	int ret = 0;
	int i = 0;
	for(i=0; i<paramcount; i++){	
		assert(modparam->items[i]);
		param = (struct mod_conf_param *)((cc_mod_param*)modparam->items[i])->param;

		assert(param);
		count = param->count;
		ret =0;
		for(loop=0; loop<count; loop++)
		{
			if(1 == param->acp[loop]->direct)
			{
				ret = modifyHeader1(param->acp[loop], data->request, hdr);
				ret++;
			}
		}
		//??һ??header???????��� ????
		if(ret < 0)
			return -1;
	}

	return 0;
}

static int modifyHeader2WithReply(int fd, HttpReply *reply)
{	
	Array * modparam = cc_get_mod_param_array(fd, mod);
	int paramcount = cc_get_mod_param_count(fd, mod);
	assert(modparam);
	struct mod_conf_param *param = NULL;
	int count = 0; 
	int loop = 0;
	int ret = 0;
	int i = 0;
	for(i=0; i<paramcount; i++){
		assert(modparam->items[i]);
		param = (struct mod_conf_param *)((cc_mod_param*)modparam->items[i])->param;
		assert(param);
		count = param->count;
		ret =0;
		for(loop=0; loop<count; loop++)
		{
			if(2 == param->acp[loop]->direct)
			{
				ret = modifyHeader2(param->acp[loop], reply);
				//must clean it when we wanna to update HttpReply struct
				ret++;
			}
		}
	}	
	httpReplyHdrCacheClean(reply);	
	httpReplyHdrCacheInit(reply);
	return 0;
}

/*
 *direct = 2ʱ?Ĵ???
 */
static int func_http_repl_read_start(int fd, HttpStateData *data)
{
	assert(data);
	return modifyHeader2WithReply(fd, data->entry->mem_obj->reply);

}

static void client_cache_hit_start(int fd, HttpReply *rep, StoreEntry *e)
{
	assert(rep);
    int modified = fd_table[fd].cc_run_state[mod->slot];
    if(2 != modified)
    {
        modifyHeader2WithReply(fd, rep);
        //storeTimestampsSet(e);
    }
}

/*
 *direct = 3ʱ?Ĵ???
 */
static int func_http_repl_send_start(clientHttpRequest *http)
{

	assert(http);

	int fd = http->conn->fd;

	Array * modparam = cc_get_mod_param_array(fd, mod);
	int paramcount = cc_get_mod_param_count(fd, mod);

	assert(modparam);

	struct mod_conf_param *param = NULL;
	int count = 0; 
	int loop = 0;
	int ret = 0;
	int i = 0;
	for(i=0; i<paramcount; i++){
		assert(modparam->items[i]);
		param = (struct mod_conf_param *)((cc_mod_param*)modparam->items[i])->param;
		assert(param);
		count = param->count;
		ret =0;

		for(loop=0; loop<count; loop++)
		{
			if(3 == param->acp[loop]->direct)
			{
				ret = modifyHeader3(param->acp[loop], http->reply);
				ret++;
			}
		}
		//??һ??header???????��� ????
		if(ret < 0)
			return -1;
	}

	return 0;
}

static int hook_cleanup(cc_module *module)
{                       
	debug(98, 1)("(mod_header) ->     hook_cleanup:\n");
	if(NULL != header_info_pool)
		memPoolDestroy(header_info_pool);
	if(NULL != action_part_pool)
		memPoolDestroy(action_part_pool);
	if(NULL != mod_config_pool)
		memPoolDestroy(mod_config_pool);
	return 0;
}  

static int func_http_req_process(clientHttpRequest * http)
{
	if (NULL != http->entry && NULL != http->entry->mem_obj && NULL != http->entry->mem_obj->reply)
    {
        fd_table[http->conn->fd].cc_run_state[mod->slot] = 2;
	}
	return 0;
}

/* module init */
int mod_register(cc_module *module)
{
	debug(98, 1)("(mod_header) ->  init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");
	//module->hook_func_http_before_redirect = func_http_req_read;
	cc_register_hook_handler(HPIDX_hook_func_http_before_redirect,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_before_redirect),
			func_http_req_read);

	//module->hook_func_http_req_send_exact = func_http_req_send;
	cc_register_hook_handler(HPIDX_hook_func_http_req_send_exact,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_send_exact),
			func_http_req_send);

	//module->hook_func_http_repl_read_start = func_http_repl_read_start;
	cc_register_hook_handler(HPIDX_hook_func_http_repl_read_start,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_repl_read_start),
			func_http_repl_read_start);

	//module->hook_func_http_repl_read_start = func_http_req_process;
	cc_register_hook_handler(HPIDX_hook_func_http_req_process,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_process),
			func_http_req_process);

	//hook_func_client_cache_hit_start
	cc_register_hook_handler(HPIDX_hook_func_client_cache_hit_start,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_client_cache_hit_start),
			client_cache_hit_start);


	//module->hook_func_http_repl_send_start = func_http_repl_send_start;
	cc_register_hook_handler(HPIDX_hook_func_http_repl_send_start,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_repl_send_start),
			func_http_repl_send_start);

	//module->hook_func_sys_parse_param = func_sys_parse_param;
	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
			func_sys_parse_param);
	//module->hook_func_sys_cleanup = hook_cleanup;
	cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_cleanup),
			hook_cleanup);

	//mod = module;
	//mod = (cc_module*)cc_modules.items[module->slot];
	//mod = (cc_module*)cc_modules.items[module->slot];
	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	return 1;
}
#endif
