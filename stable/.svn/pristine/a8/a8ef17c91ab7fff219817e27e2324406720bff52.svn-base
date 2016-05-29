#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK
#define DELIMIT	"@@@"

static cc_module* mod = NULL;

enum {                          
	HEADER_DEFAULT = 1, 
	HEADER_DEFAULT_ALL,
	HEADER_ORIGINAL,
	HEADER_NONE         
};


typedef struct _value_group
{
	String *values; 
	int size;
	struct _value_group *next;
} value_group;


typedef struct _oneHttpSpecialHeader
{
	int id; 
	String header_name;
	value_group *value;
	int sensitive; 
	int isOrder;
	int rigid;
	int other;
	String other_value;

	struct _oneHttpSpecialHeader *next;
} oneHttpSpecialHeader;


typedef struct _url_header_combination
{
	struct _oneHttpSpecialHeader *headers;
	char *compress_delimit;
} url_header_combination;

static MemPool * value_group_pool = NULL;
static MemPool * oneHttpSpecialHeader_pool = NULL;
static MemPool * url_header_combination_pool = NULL;

static void * value_group_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == value_group_pool)
	{
		value_group_pool = memPoolCreate("mod_hdr_combination config_struct value_group", sizeof(value_group));
	}
	return obj = memPoolAlloc(value_group_pool);
}

static void * oneHttpSpecialHeader_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == oneHttpSpecialHeader_pool)
	{
		oneHttpSpecialHeader_pool = memPoolCreate("mod_hdr_combination config_struct oneHttpSpecialHeader",sizeof(oneHttpSpecialHeader));
	}
	return obj = memPoolAlloc(oneHttpSpecialHeader_pool);
}

static void * url_header_combination_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == url_header_combination_pool)
	{
		url_header_combination_pool = memPoolCreate("mod_hdr_combination config_struct url_header_combination", sizeof(url_header_combination));
	}
	return obj = memPoolAlloc(url_header_combination_pool);
}

static int strchrcount(const char *str, const char chr)
{
	int count=0;
	const char *t=NULL;
	const char *pos=str;
	while((t=strchr(pos, chr)))
	{
		pos = t;
		pos++;
		count++;
	}
	++count;
	debug(101, 3)("(mod_header_combination) -> strchrcount: %d\n", count);
	return count;
}


HttpHeaderEntry *httpHeaderEntryCreate(http_hdr_type id, const char *name, const char *value);

/* only parse value in 'xxxx'. For example, xxx. */
static void parse_value(value_group **head, const char *value)
{
	debug(101, 3)("(mod_header_combination) -> parse_value: value {%s}\n", value);
	value_group *last=NULL;
	value_group * tmp = value_group_pool_alloc();
	if(tmp==NULL)
	{
		debug(101, 3)("(mod_header_combination) -> parse_value: tmp failed to memPoolAlloc\n");
		return ;
	}

	int count=strchrcount(value, ',');
	tmp->size=count;
	String *values=xcalloc(0, count*sizeof(String));
	const char *t=NULL;
	const char *pos=value;
	int c=0;

	while(pos)
	{
		if((t=strchr(pos, ',')))
		{
			stringLimitInit(&values[c], pos, t-pos);
			debug(101, 3)("(mod_header_combination) -> parse_value: values[%d] %s\n", c, strBuf(values[c]));
			pos=t;
			pos++;
			c++;
		}
		else
		{
			stringLimitInit(&values[c], pos, strlen(pos));
			pos=NULL;
			++c;
		}
	}
	tmp->values = values;
	if(!*head)
		*head = tmp;
	else
	{
		last = *head;
		while(last->next)
			last = last->next;
		last->next = tmp;
	}
}


static int free_url_header_combination(void *data)
{
	assert(data);
	url_header_combination *tmp_header_combination = data;
	oneHttpSpecialHeader *temp = NULL; 
	oneHttpSpecialHeader *node = NULL; 
	temp = tmp_header_combination->headers;
	while(temp)
	{
		stringClean(&(temp->header_name));
		value_group *value = NULL; 
		/* free value_group struction */
		if(temp->value)
			value = temp->value;
		while(value)
		{
			stringClean(value->values);
			value->size = 0;
			value = value->next;
		}       
		if(NULL != temp->value)
		{
			memPoolFree(value_group_pool, temp->value);
		}
		node = temp;
		temp->value = NULL; 
		/* fetch next object */
		temp = temp->next;
		memPoolFree(oneHttpSpecialHeader_pool, node);
		node = NULL;
	}       
	memPoolFree(url_header_combination_pool, tmp_header_combination);
	tmp_header_combination = NULL;
	return 0;
}


static int func_sys_parse_param(char *cfgline)
{
	assert(cfgline);
	debug(101, 1)("(mod_header_combination) -> func_sys_parse_param: %s\n", cfgline);

	url_header_combination *cur = url_header_combination_pool_alloc();
	if(!cur)
	{
		debug(101, 1)("(mod_header_combination) -> func_sys_parse_param: failed to xcalloc url_header_combination\n");
		return -1;
	}

	/* fill oneHttpSpecialHeader */
	char *token;
	int parse_header_count=0;
	oneHttpSpecialHeader *oneHttp = NULL;

	/* parse mod_header_combination */
	token = strtok(cfgline, w_space);
	if(strcmp(token, "mod_header_combination"))
	{
		return -1;
	}

	cur->compress_delimit = xstrdup(DELIMIT);

	while((token = strtok(NULL, w_space)))
	{
		if(!strncmp("%", token, 1))
		{
			debug(101, 3)("(mod_header_combination) -> func_sys_parse_param: xcalloc oneHttpSpecialHeader...\n");
			oneHttp = oneHttpSpecialHeader_pool_alloc();
			if(oneHttp==NULL)
			{
				debug(101, 1)("(mod_header_combination) -> func_sys_parse_param: failed to xcalloc(...oneHttpSepcialHeader)\n");
				return -1;
			}
			oneHttp->sensitive=1;
			stringInit(&oneHttp->header_name, token+1);
			oneHttp->id=httpHeaderIdByNameDef(token+1, strlen(token)-1);
			if(oneHttp->id<0)
			{
				debug(101, 1)("(mod_header_combination) -> func_sys_parse_param: %s is not exist\n", token+1);
				stringClean(&oneHttp->header_name);
				memPoolFree(oneHttpSpecialHeader_pool, oneHttp);
				oneHttp = NULL;
				memPoolFree(url_header_combination_pool, cur);
				cur = NULL;
				return -1;
			}
			parse_header_count++;
			debug(101, 3)("(mod_header_combination) -> func_sys_parse_param: func_sys_parse_param: count %d, header_name %s, header_id %d\n", parse_header_count, strBuf(oneHttp->header_name), oneHttp->id);
		}
		else if(!strncmp("-i", token, 2) && oneHttp)
			oneHttp->sensitive = 0;
		else if(!strncmp("-s", token, 2) && oneHttp)
			oneHttp->isOrder = 1;
		else if(!strncmp("-r", token, 2) && oneHttp)
			oneHttp->rigid = 1;
		else if(!strcmp("_default", token) && oneHttp)
		{
			oneHttp->other = HEADER_DEFAULT;
			if((token=strtok(NULL, w_space)))
			{
				if(!strncmp(token, "'", 1) && !strncmp((token + strlen(token) - 1), "'", 1))
				{
					*(token+strlen(token+1))='\0';
					stringInit(&oneHttp->other_value, token+1);
				}
				else
				{

					debug(101, 1)("(mod_header_combination) -> func_sys_parse_param: can not find value after _default\n");
					return -1;
				}

			}
			else
			{
				debug(101, 1)("(mod_header_combination) -> func_sys_parse_param: can not find value after _default\n");
				return -1;
			}
		}
		else if(!strcmp("_defaultall", token))
		{
			oneHttp->other = HEADER_DEFAULT_ALL;
			if((token=strtok(NULL, w_space)))
			{
				if(!strncmp(token, "'", 1) && !strncmp(token, "'", 1))
				{
					*(token+strlen(token+1))='\0';
					stringInit(&oneHttp->other_value, token+1);
				}
			}
			else
			{
				debug(101, 1)("(mod_header_combination) -> func_sys_parse_param: can not find value after _default\n");
				return -1;
			}
		}
		else if(!strcmp("_original", token))
			oneHttp->other = HEADER_ORIGINAL;
		else if(!strcmp("_none", token))
			oneHttp->other = HEADER_NONE;
		else if(!strncmp("'", token, 1) && !strncmp("'", token+strlen(token)-1, 1) && oneHttp)
		{
			*(token + strlen(token) - 1)= '\0';
			parse_value(&oneHttp->value, token+1);
		}
		else if(!strcmp("|", token) && oneHttp)
		{
			oneHttpSpecialHeader **last=&cur->headers;
			if(!*last)
				*last = oneHttp;
			else{
				while((*last)->next)
					*last = (*last)->next;
				(*last)->next = oneHttp;
			}
		}
		else if(!strcmp("END", token))
		{
			debug(101, 3)("(mod_header_combination) -> func_sys_parse_param: END. Begin to parse acl\n");
			break;
		}
		else
			debug(101, 3)("(mod_header_combination) -> func_sys_parse_param: unkonwn token %s\n", token);
	}

	cc_register_mod_param(mod, cur, free_url_header_combination);
	return 0;
}


static void  ridUrlBlank(char* buffer)
{                   
	char* str = buffer;
	while (NULL != (str=strchr(str, ' ')))
	{
		*str = '#';
	}
}



static void dealwithHeaderDefaultAll(oneHttpSpecialHeader* header, request_t* req, String* url_append, const char* delimit, String* list)
{
	//ÐÞ¸Äurl
	HttpHeaderEntry *e = NULL;
	stringAppend(url_append, delimit, strlen(delimit));
	stringAppend(url_append, header->header_name.buf, header->header_name.len);
	stringAppend(url_append, ":", 1);
	stringAppend(url_append, header->other_value.buf, header->other_value.len);
	//Ôö¼ÓÒ»¸öheader
	if((1==header->rigid) || (NULL==list))	//²»ÒªÇó×·¼ÓÄÚÈÝ»òÕßÃ»ÓÐÄÚÈÝ¿ÉÒÔ×·¼Ó
	{
		e = httpHeaderEntryCreate(header->id, header->header_name.buf, header->other_value.buf);
		httpHeaderAddEntry(&req->header, e);
	}
	else	//°ÑÔ­À´µÄÄÚÈÝ¼Óµ½headerÀï
	{
		String tempStr;
		stringInit(&tempStr, header->other_value.buf);
		stringAppend(&tempStr, ", ", 2);
		stringAppend(&tempStr, list->buf, list->len);
		e = httpHeaderEntryCreate(header->id, header->header_name.buf, tempStr.buf);
		httpHeaderAddEntry(&req->header, e);
		stringClean(&tempStr);
	}
}


struct StrAndLength
{
	const char* str;
	int length;
};


#define HEADER_VALUES_MAX_NUMBER 50
static struct StrAndLength g_stHeaderValues[HEADER_VALUES_MAX_NUMBER];
static int  g_iResult[HEADER_VALUES_MAX_NUMBER];
static const char* g_pszdelimit[] = {NULL, "_##_", "_##_", "_##_"};

//modified by ChinaCache wsg
static int
compareInt(const void *ptr1, const void *ptr2)
{
	return *(int*)ptr1-*(int*)ptr2;
}


//modified by ChinaCache wsg
//°ÑÒ»¸öheaderµÄÖµ·Ö½âÎª¸÷¸ö×ÓÏî£¬·µ»ØÖµ·Åµ½È«¾Ö±äÁ¿g_stHeaderValuesÀï
static int
getStrAndLengthFromString(String* list)
{
	const char *pos = NULL;
    const char *item;
    int ilen = 0;
	int values_number = 0;
	while (strListGetItem(list, ',', &item, &ilen, &pos))
	{
		g_stHeaderValues[values_number].str = item;
		//È¥µô;ºÍËüºóÃæµÄÄÚÈÝ
		int i;
		for(i=0;i<ilen;i++)
		{
			if(';' == item[i])
			{
				break;
			}
		}
		g_stHeaderValues[values_number].length = i;
		values_number++;
		if(values_number >= HEADER_VALUES_MAX_NUMBER)
		{
			break;
		}
    }
	return values_number;
}

//modified by ChinaCache wsg
//¼ì²éheaderµÄÄÚÈÝÊÇ·ñ°üº¬ÅäÖÃÎÄ¼þ¶ÔÓ¦µÄÄÚÈÝ£¬
//Í¬Ê±ÓÃÈ«¾Ö±äÁ¿g_iResult±ê¼ÇÅäÖÃÏîµÄÄÚÈÝÓëheaderµÄÄÚÈÝµÄ¶ÔÓ¦¹ØÏµ
static value_group*
getMatchValue(value_group* value, int values_number, int sensitive)
{
	int ret;
	int i;
	for(i=0;i<value->size;i++)
	{
		int j;
		//¼ì²éÅäÖÃÏîµÄÒ»¸öÖµ
		for(j=0;j<values_number;j++)
		{
			if(g_stHeaderValues[j].length != value->values[i].len)
			{
				continue;
			}
			if(0 == sensitive)
			{
				ret = strncasecmp(g_stHeaderValues[j].str, value->values[i].buf, value->values[i].len);
			}
			else
			{
				ret = strncmp(g_stHeaderValues[j].str, value->values[i].buf, value->values[i].len);
			}
			if(0 == ret)
			{
				break;
			}
		}
		//ÅäÖÃÏîµÄÄ³Ò»¸öÖµ²»Æ¥Åä
		if(j == values_number)
		{
			return NULL;
		}
		g_iResult[i] = j;
	}
	return value;
}


//modified by ChinaCache wsg
//´¦ÀíÃ»ÓÐÆ¥ÅäµÄÇé¿ö£¬ÐÞ¸ÄurlÁË·µ»Ø1£¬·ñÔò·µ»Ø0
static int
dealwithNoMatch(oneHttpSpecialHeader* header, request_t* req, String* url_append, String* list, const char* delimit)
{
	if((HEADER_DEFAULT==header->other) || (HEADER_DEFAULT_ALL==header->other))	//Ìæ»»Öµ
	{
		//É¾³ýÔ­À´µÄheader£¬¼ÓÐÂµÄheader
		httpHeaderDelById(&req->header, header->id);
		dealwithHeaderDefaultAll(header, req, url_append, delimit, list);
		return 1;
	}
	else if(HEADER_ORIGINAL == header->other)	//°ÑÔ­À´µÄÖµ·ÅÔÚurlÉÏ
	{
		stringAppend(url_append, delimit, strlen(delimit));
		stringAppend(url_append, header->header_name.buf, header->header_name.len);
		stringAppend(url_append, ":", 1);
		stringAppend(url_append, list->buf, list->len);
		return 1;
	}
	else if(HEADER_NONE == header->other)	//ÈÓµôheaderµÄÇé¿ö
	{
		httpHeaderDelById(&req->header, header->id);
		return 0;
	}
	else	//Ã»ÓÐÅäÖÃµÄÇé¿ö
	{
		return 0;
	}
}

//modified by ChinaCache wsg
//´¦ÀíÅäÖÃÎÄ¼þºÍheaderÄÚÈÝÆ¥ÅäµÄÇé¿ö
static void
dealwithMatch(oneHttpSpecialHeader* header, request_t* req, String* url_append, int values_number, value_group* value, const char* delimit)
{
	//Èç¹ûÐèÒªÓÃµ½Ô­À´µÄheaderµÄË³Ðò£¬½øÐÐÅÅÐò
	if((0==header->rigid) || (0==header->isOrder))
	{
		qsort(g_iResult, value->size, sizeof(int), compareInt);
	}
	//É¾³ýÕâ¸öheader
	httpHeaderDelById(&req->header, header->id);
	String resultStr;

	if(header->isOrder)		//°´ÕÕÅäÖÃÎÄ¼þµÄË³Ðò£¬°ÑÅäÖÃÎÄ¼þµÄÄÚÈÝ×éºÏ³ÉÒ»¸ö×Ö·û´®
	{
		stringInit(&resultStr, value->values[0].buf);
		int i;
		for(i=1;i<value->size;i++)
		{
			stringAppend(&resultStr, ", ", 2);
			stringAppend(&resultStr, value->values[i].buf, value->values[i].len);
		}
	}
	else		//°´ÕÕheaderµÄË³Ðò£¬°ÑheaderµÄÄÚÈÝ×éºÏ³ÉÒ»¸ö×Ö·û´®
	{
		stringLimitInit(&resultStr, g_stHeaderValues[g_iResult[0]].str, g_stHeaderValues[g_iResult[0]].length);
		int i;
		for(i=1;i<value->size;i++)
		{
			stringAppend(&resultStr, ", ", 2);
			stringAppend(&resultStr, g_stHeaderValues[g_iResult[i]].str, g_stHeaderValues[g_iResult[i]].length);
		}
	}
	//ÐÞ¸Äurl
	stringAppend(url_append, delimit, strlen(delimit));
	stringAppend(url_append, header->header_name.buf, header->header_name.len);
	stringAppend(url_append, ":", 1);
	stringAppend(url_append, resultStr.buf, resultStr.len);
	//ÐèÒª±£ÁôÔ­À´headerµÄÄÚÈÝ£¬×·¼Ó²»Ò»ÖÂµÄheaderÄÚÈÝ
	if(0 == header->rigid)
	{
		int i;
		int j;
		for(i=0,j=0;i<values_number;i++)
		{
			if(j < value->size)
			{
				if(g_iResult[j] == i)
				{
					j++;
					continue;
				}
				assert(i<g_iResult[j]);
			}
			stringAppend(&resultStr, ", ", 2);
			stringAppend(&resultStr, g_stHeaderValues[i].str, g_stHeaderValues[i].length);
		}
	}
	//Ôö¼Ó´¦ÀíºóµÄheader
	httpHeaderAddEntry(&req->header, httpHeaderEntryCreate(header->id, header->header_name.buf, resultStr.buf));
	//Çå³ý×Ö·û´®ÄÚ´æ
	stringClean(&resultStr);
}


//modified by ChinaCache wsg
typedef struct _nameAndValue nameAndValue;
//modified by ChinaCache wsg
//cookieÃûÖµ¶Ô
struct _nameAndValue
{
	String name;
	String value;
	nameAndValue* next;
};

static int dealwithOneSpecialHeader(oneHttpSpecialHeader* header, request_t* req, String* url_append, const char* delimit)
{
	//Õâ¸öÇëÇó²»°üº¬Õâ¸öheaderµÄÇé¿ö
	if(!CBIT_TEST(req->header.mask, header->id))
	{
		if(HEADER_DEFAULT_ALL == header->other)
		{
			dealwithHeaderDefaultAll(header, req, url_append, delimit, NULL);
			return 1;
		}
		else
		{
			return 0;
		}
	}
	//µÃµ½ÏÖÔÚµÄheaderµÄÖµ
	String list = httpHeaderGetList(&req->header, header->id);
	//Èç¹ûÏÖÔÚµÄheader¶ÔÓ¦µÄÖµÎª¿Õ£¬µ±×öÃ»ÓÐÃ»ÓÐÆ¥ÅäµÄÇé¿ö
	if(0 == list.len)
	{
		stringClean(&list);
		return dealwithNoMatch(header, req, url_append, &list, delimit);
		//return 0;
	}
	//·Ö½âheaderµÄÄÚÈÝÎª¸÷¸öÏî
	int values_number = getStrAndLengthFromString(&list);

	value_group* value = header->value;
	//ÕÒµ½Ò»×éÆ¥ÅäµÄÇé¿ö
	while(value)
	{
		if(NULL != getMatchValue(value, values_number, header->sensitive))
		{
			break;
		}
		value = value->next;
	}

	int ret = 0;
	if(NULL == value)	//Ã»ÓÐÆ¥ÅäµÄÇé¿ö
	{
		ret = dealwithNoMatch(header, req, url_append, &list, delimit);
	}
	else	//´¦ÀíÆ¥ÅäµÄÇé¿ö
	{
		dealwithMatch(header, req, url_append, values_number, value, delimit);
		ret = 1;
	}
	//ÇåÄÚ´æ
	stringClean(&list);
	return ret;
}

static int httpHeaderDealwith(clientHttpRequest* http, url_header_combination *header)
{
	debug(101, 3)("(mod_header_combination) -> httpHeaderDealwith: start\n");
	int move_len = 0;

	//Ö»ÓÐgetºÍhead²Å´¦Àí
	if((METHOD_GET!=http->request->method) && (METHOD_HEAD!=http->request->method))
	{
		return move_len;
	}

	g_pszdelimit[0] = header->compress_delimit;

	int flag = 0;	//±êÊ¶ÓÃÊ²Ã´·Ö¸ô·û
	int fgDealwith = 0;		//±ê¼ÇÊÇ·ñÓÐheader»òÕßcookie´¦Àí¹æÔò£¬0--Ã»ÓÐ£¬1--ÓÐ
	String url_append = StringNull;
	stringInit(&url_append, http->uri);
	int uri_length = url_append.len;

	{
		//´¦Àíheader
		if(header)
		{
			fgDealwith = 1;
		}

		//±éÀúheaderÁ´±í
		if(dealwithOneSpecialHeader(header->headers, http->request, &url_append, g_pszdelimit[flag]) > 0)
		{
			flag = 1;
		}

		//Èç¹û
		if(1 == flag)
		{
			flag = 2;
		}

		//ÊÍ·Å·µ»ØÖµµÄÄÚ´æ£¨8¸öbyte£©
	}

	//Èç¹û×·¼ÓÁË@£¬Òª±£Ö¤urlÀï²»°üº¬¿Õ¸ñ
	if(url_append.len > uri_length)
	{
		ridUrlBlank(url_append.buf+uri_length);
	}
	
	//ÎÄ¼þÃûÃ»ÓÐÌí¼ÓÈÎºÎºó×ºµÄ£¬¼Ó@
	if((1==fgDealwith) && (url_append.len==uri_length))
	{
		stringAppend(&url_append, g_pszdelimit[0], strlen(g_pszdelimit[0]));
	}

	if((url_append.len>uri_length) && (url_append.len<MAX_URL))
	{
		move_len++;
		safe_free(http->uri);
		http->uri = xstrdup(url_append.buf);
	}
	stringClean(&url_append);

	if(move_len > 0)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


static inline void modify_request(clientHttpRequest * http)
{
	debug(101, 3)("(mod_header_combination) -> modify_request: start, uri=[%s]\n", http->uri);
	request_t* old_request = http->request;
	request_t* new_request = urlParse(old_request->method, http->uri);
	safe_free(http->uri);

	if (new_request)
	{
		safe_free(http->uri);
		http->uri = xstrdup(urlCanonical(new_request));
		if(!http->log_uri)
			http->log_uri = xstrdup(urlCanonicalClean(old_request));
		new_request->http_ver = old_request->http_ver;
		httpHeaderAppend(&new_request->header, &old_request->header);
		new_request->client_addr = old_request->client_addr;
		new_request->client_port = old_request->client_port;
#if FOLLOW_X_FORWARDED_FOR
		new_request->indirect_client_addr = old_request->indirect_client_addr;
#endif /* FOLLOW_X_FORWARDED_FOR */
		new_request->my_addr = old_request->my_addr;
		new_request->my_port = old_request->my_port;
		new_request->flags = old_request->flags;
		new_request->flags.redirected = 1;
		if (old_request->auth_user_request)
		{
			new_request->auth_user_request = old_request->auth_user_request;
			authenticateAuthUserRequestLock(new_request->auth_user_request);
		}
		if (old_request->body_reader)
		{
			new_request->body_reader = old_request->body_reader;
			new_request->body_reader_data = old_request->body_reader_data;
			old_request->body_reader = NULL;
			old_request->body_reader_data = NULL;
		}
		new_request->content_length = old_request->content_length;
		if (strBuf(old_request->extacl_log))
			new_request->extacl_log = stringDup(&old_request->extacl_log);
		if (old_request->extacl_user)
			new_request->extacl_user = xstrdup(old_request->extacl_user);
		if (old_request->extacl_passwd)
			new_request->extacl_passwd = xstrdup(old_request->extacl_passwd);
		//added by jiangbo.tian to solve the conflict bettween mod_customized_server_side_error_page and mod_header_combination
		if(old_request->cc_request_private_data)
		{
			new_request->cc_request_private_data = old_request->cc_request_private_data;
			old_request->cc_request_private_data = NULL;
		}
		//added end

		//added by zh for mp4 store_url
		if(old_request->store_url)
		{
			new_request->store_url = xstrdup(old_request->store_url);
		}
		//add end
		requestUnlink(old_request);
		http->request = requestLink(new_request);
	}
}


static int func_http_req_read(clientHttpRequest *http)
{
	assert(http);
	int fd = http->conn->fd;
	int ret = 0;
	url_header_combination *param = (url_header_combination *)cc_get_mod_param(fd, mod);
	assert(param);
	ret = httpHeaderDealwith(http, param);

	if (ret > 0){
		modify_request(http);

		char* str = strstr(strBuf(http->request->urlpath), DELIMIT);
		if(NULL != str)
		{
			*str = 0;
		}
			
	}
	return 1;	
}


static char g_szBuffer[MAX_URL];
static const char *const crlf = "\r\n";
static int func_buf_send_to_s(HttpStateData *httpState, MemBuf *mb)
{
	request_t *request = httpState->request;
	
    debug(101,3)("mod_header_combination---->>func_buf_send_to_s:mb->buf: %s\n",mb->buf);
    debug(101, 3)("(mod_header_combination) -> func_buf_send_to_s: thre url is: %s\n", strBuf(request->urlpath));
	memset(g_szBuffer, 0, sizeof(g_szBuffer));
	if(strLen(request->urlpath))
	{
		snprintf(g_szBuffer, MAX_URL, "%s", strBuf(request->urlpath));
		char* str = strstr(g_szBuffer, DELIMIT);
		if(NULL != str)
		{
			*str = 0;
		}
		const char *new_path = xstrdup(g_szBuffer);
		debug(101, 3)("(mod_header_combination) -> func_buf_send_to_s: new_path is: %s\n", new_path);
		stringReset(&request->urlpath, new_path);
		safe_free(new_path);
	}
	else
	{
		strcpy(g_szBuffer, "/");
	}

	if(mb->size==0)
	{
		memBufPrintf(mb, "%s %s HTTP/1.%d\r\n",
				RequestMethods[request->method].str,
				g_szBuffer,
				httpState->flags.http11);
	}
	debug(101, 3)("(mod_header_combination) -> func_buf_send_to_s: after deal with,thre url is: %s\n", g_szBuffer);
		debug(101,3)("mod_header_combination---->>func_buf_send_to_s end:mb->buf: %s\n",mb->buf);
    return 1;	
}

//added for destroy the mempool of config_struct
static int hook_cleanup(cc_module *module)
{
	debug(101, 1)("(mod_header_combination) ->     hook_cleanup:\n");
	if(NULL != value_group_pool)
		memPoolDestroy(value_group_pool);
	if(NULL != oneHttpSpecialHeader_pool)
		memPoolDestroy(oneHttpSpecialHeader_pool);
	if(NULL != url_header_combination_pool)
		memPoolDestroy(url_header_combination_pool);
	return 0;
}

/*
//remove Vary here
static int func_buf_recv_from_s(int fd, char *buf, int len)
{
	int t_hdr_size = 0;
	char* vary_start = NULL;
	if (buf)
		t_hdr_size = headersEnd(buf, 4096);
	if (t_hdr_size )
	{
		while(1)
		{
			vary_start = strstr(buf, "Vary:");
			if (!vary_start)
				vary_start = strstr(buf, "vary:");

			if (vary_start) 
			{
				while(*vary_start != 'a') 
				{
					vary_start++;
					if (*vary_start == ':')
						break;
				}
				*vary_start = 'e';
				vary_start = NULL;
			}
			else
				break;
		}
	}
	return len;
}
*/


// module init 
int mod_register(cc_module *module)
{
	debug(101, 1)("(mod_header_combination) ->  init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");

	//module->hook_func_http_req_read = func_http_req_read;
	cc_register_hook_handler(HPIDX_hook_func_http_req_read,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_read),
			func_http_req_read);
	//module->hook_func_sys_parse_param = func_sys_parse_param;
	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
			func_sys_parse_param);
	//module->hook_func_buf_send_to_s = func_buf_send_to_s;
	cc_register_hook_handler(HPIDX_hook_func_buf_send_to_s,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_buf_send_to_s),
			func_buf_send_to_s);
	//module->hook_func_sys_cleanup = hook_cleanup;
	cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_cleanup),
			hook_cleanup);

	//mod = module;
	//mod = (cc_module*)cc_modules.items[module->slot];
	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	return 0;
}

#endif
