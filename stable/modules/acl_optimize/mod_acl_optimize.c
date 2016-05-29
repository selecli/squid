#include "cc_framework_api.h" 
#ifdef CC_FRAMEWORK 
#define ACL_NAME_MATCHED_MAX 5000 
#define MAX_PATTERN_NUM 20000
#define MAX_RESULT_NUM 100000
#define FREE_RESULT_NODE_NUM 2000
/*
*pattern_node is designed for acl ,type of url_regex , 
*type: is the squid's acl_type
*host: is the url's host, example: http://www.sina.com
*pattern : the pattern_line
*acl_name: the squid's acl_name
*next: the next point in  the list
*/


struct pattern_node {
	squid_acl type; 
	char* host;
	char* pattern;
	regex_t comp;
	char acl_name[ACL_NAME_SZ];
	struct pattern_node* next;
};

struct acl_access_packer
{
	acl_access *aa;
	struct acl_access_packer *next;
};
/*
 * struct req_rel_aa:
 * aa: the acl_access list that before the current match acl_access. 
 */
struct req_rel
{
//	acl_access* aa;
	struct acl_access_packer *aap;
	int *param_poses;
	int count;
};
/*
   *type: 1 stand for the squid origin aclCheck, 0 for the FC module cc_aclCheckFastRequest
   *access_type: the point to the access_list of the aclCheck_t
   *allow: allow or deny of the request access
   *result: the module's cc_client_acl_check result
   */
struct result_node
{

	int type;
	acl_access* access_type;
	struct req_rel * req_rel_aa;      //the request relevant acl_access
	//acl_access* req_rel_aa;      //the request relevant acl_access
	allow_t allow;
	struct result_node *next;
	cc_acl_check_result result;

};

/* struct aclCheck_result, keep the aclCheck's result for a request
   *digest: the url's md5 key
   *head: the head of result_list
   *len: the length of result_list
   */
struct aclCheck_result
{
	cache_key *digest;
	struct result_node *head;
	int len;
	char *AclMatchedName;
	
};

/*
*result_array: the aclCheck_result Array
*/
static struct aclCheck_result *result_array = NULL/*[MAX_RESULT_NUM]*/;


/*
 *the matched_acl_name is attached to clientHttpRequest *
 */

struct matched_acl_name
{
	squid_acl type;
	char acl_name[ACL_NAME_SZ];
	struct matched_acl_name *next;

};

static cc_module *mod=NULL;

//static int on = 1;
static struct pattern_node *pattern_array[MAX_PATTERN_NUM];

/*
   *join_pattern_array: join the pattern_node into the pattern_array
   */
//static void join_pattern_array(struct pattern_node** pattern_array, struct pattern_node** node)
static void join_pattern_array(struct pattern_node** node)
{
	unsigned int i = myHash((*node)->host,MAX_PATTERN_NUM);
	debug(145,3)("join_pattern_array: the hash is: %d, node->host is: %s\t,node->pattern is: %s \n",i,(*node)->host, (*node)->pattern );
	(*node)->next = pattern_array[i];
	pattern_array[i] = *node;
}




/*
   *func_sys_parse_param: 
   *parse the config line in the squid.conf
   */
/*
static int func_sys_parse_param(char *cfg_line)
{
	//Must be [mod_acl_optimize on] or [mod_acl_optimize off]
	assert(cfg_line);
	char *t = strtok(cfg_line, w_space);
	if(strcmp(t, "mod_acl_optimize"))
	{
		debug(114,0)("mod_acl_optimize cfg_line not begin with mod_acl_optimize!\n");
		return -1;
	}
	t = strtok(NULL, w_space);
	on = 1;
	if(!strcmp(t, "on"))
	{
		on = 1;
		return 0;
	}
	else if(!strcmp(t, "off"))
	{
		on = 0;
		return 0;
	}
	debug(145,0)("mod_acl_optimize cfg_line not end with on or off!\n");
	return -1;
}

*/
struct _resultArrayState
{
	struct aclCheck_result *array; // the result_array pointer
	int start; // where to start
	int len; // the length to free from the start
	int capacity; // the array capacity
};
typedef struct _resultArrayState resultArrayState;
/* free the result_node in event */
CBDATA_TYPE(resultArrayState);
static void free_result_array(void *data)
{
	resultArrayState * state= data;
	assert(state!=NULL);
	int len = state->len;
	struct aclCheck_result* array = state->array;
	int start = state->start;
	int capacity = state->capacity;
	int end = start+len-1;
	int free_array = 0;

	if(end >capacity -1)
	{
		end = capacity -1;
		free_array = 1;

	}
	int i;
	for(i=start;i<=end;i++) // free all the result_node if reload
	{
		safe_free(array[i].digest);
		safe_free(array[i].AclMatchedName);
		struct result_node* node = array[i].head;
		struct result_node* next = node;
		while(node)
		{
			next = node->next;
			safe_free(node->result.param_poses);
			struct req_rel * rel_tmp = node->req_rel_aa;
			if(rel_tmp)
			{
				struct acl_access_packer *rel_aa = rel_tmp->aap;
				struct acl_access_packer *rel_next;
				while(rel_aa)
				{

					rel_next = rel_aa->next;
					//cbdataFree(rel_aa);
					safe_free(rel_aa);
					rel_aa = rel_next;
				}

				safe_free(rel_tmp->param_poses);
				safe_free(rel_tmp);
			}
			safe_free(node);
			node = next;
		}
		array[i].head = NULL;

	}
	if(free_array)
	{
		debug(145,3)("free_result_array: the result nodes have been freed all, safe_free array and state\n");
		safe_free(array);
		state->array = NULL;
		cbdataFree(state);
		//safe_free(state);
	}
	else
	{
		cbdataLock(state);
		state->start +=FREE_RESULT_NODE_NUM ;
		state->len = FREE_RESULT_NODE_NUM;
		debug(145,3)("free_result_array: free start from %d, and end is:%d, we add event free_result_array\n ",state->start,state->start + state->len-1);
		cbdataUnlock(state);
		eventAdd("free_result_array", free_result_array, state, 10, 0);
	}
}

/*
   *hook_after_parse_param: in this func, organize all the url_regex acl into the needed style by squid
   */
static void hook_after_parse_param()
{

	int i;
	for(i=0;i<MAX_PATTERN_NUM;i++)   // free all the pattern_node if reload
	{
		if(pattern_array[i])
		{
			struct pattern_node* node = pattern_array[i];
			struct pattern_node* next = node;
			while(node)
			{
				debug(145,9)("i= %d\t pattern is: %s\t host is: %s\n",i,node->pattern,node->host);
				safe_free(node->host);
				safe_free(node->pattern);
				next = node->next;
				safe_free(node);
				node=next;
			}
			pattern_array[i] = NULL;
		}
	}

	if(result_array)
	{
		/* add event to free the result_array */ 
		CBDATA_INIT_TYPE(resultArrayState);
		resultArrayState * ras = cbdataAlloc(resultArrayState);/*xcalloc(1,sizeof(struct resultArrayState)); */
		cbdataLock(ras);
		ras->array = result_array;
		ras->start = 0;
		ras->len = FREE_RESULT_NODE_NUM;
		ras->capacity = MAX_RESULT_NUM;

		cbdataUnlock(ras);
		eventAdd("free_result_array", free_result_array, ras, 60, 0);
		ras = NULL;
		result_array= NULL;

		/*
		   for(i=0;i<MAX_RESULT_NUM;i++) // free all the result_node if reload
		   {
		   safe_free(result_array[i].digest);
		   struct result_node* node = result_array[i].head;
		   struct result_node* next = node;
		   while(node)
		   {
		   next = node->next;
		   safe_free(node->result.param_poses);
		   struct req_rel * rel_tmp = node->req_rel_aa;
		   if(rel_tmp)
		   {
		   acl_access *rel_aa = rel_tmp->aa;
		   acl_access *rel_next;
		   while(rel_aa)
		   {

		   rel_next = rel_aa->next;
		   cbdataFree(rel_aa);
		   rel_aa = rel_next;
		   }

		   safe_free(rel_tmp->param_poses);
		   safe_free(rel_tmp);
		   }
		   safe_free(node);
		   node = next;
		   }
		   result_array[i].head = NULL;

		   }
		   */
	}
	//else
	{
		result_array = xcalloc(MAX_RESULT_NUM,sizeof(struct aclCheck_result));
	}
	hash_conflict = 0;
	cached_result = 0;
	//if(on == 1)
	if(mod->flags.config_on)
	{
		acl* cur;

		for(cur=Config.aclList;cur;cur=cur->next)
		{
			if(cur->type == ACL_URL_REGEX)
			{
				relist* cur_relist;  
				for(cur_relist = cur->data;cur_relist;cur_relist = cur_relist->next)
				{

					debug(145,3)("the pattern is: %s\n",cur_relist->pattern);
					struct pattern_node *node = xcalloc(1,sizeof(struct pattern_node));
					char* pattern = xstrdup(cur_relist->pattern);
					if(*pattern=='^')
					{
						if(strncmp(pattern,"^http://",strlen("^http://")) == 0 )
						{
							int errcode = 0;
							int flags = REG_EXTENDED | REG_NOSUB | REG_ICASE;
							if((errcode = regcomp(&node->comp,pattern,flags))!=0)
							{
								char errbuf[256];
								regerror(errcode,&node->comp,errbuf,sizeof errbuf);
								debug(145,1)("mod_acl_optimize: Invalid regular expression pattern (%s): %s \n", pattern, errbuf);
								//printf("mod_acl_optimize: Invalid regular expression pattern (%s): %s \n", pattern, errbuf);

								safe_free(node);
								safe_free(pattern);
								continue;
							}
							char* host_start = pattern+8;
							char* host_end = strchr(host_start,'/');
							if(host_end == NULL)
								continue;
							node->host = xcalloc(1,host_end - pattern);
							debug(145,3)("the host_start is  : %s ,and the size of node->host is: %lu\n",host_start,(unsigned long)sizeof(node->host));
							strncpy(node->host,pattern+1,host_end - pattern-1);
							//*pattern = '^';
							node->pattern = pattern;
							node->type = ACL_URL_REGEX;
							strncpy(node->acl_name, cur->name, strlen(cur->name));
							debug(145,3)("the node->acl_name is: %s, node->pattern is: %s, node->host is: %s \n",node->acl_name,node->pattern,node->host);
							//printf("the node->acl_name is: %s, node->pattern is: %s, node->host is: %s \n",node->acl_name,node->pattern,node->host);
							join_pattern_array(&node);
						}

					}


				}

			}


		}
	}
}



/*
   *free_matched_acl_name: free the matched_acl_name list
   */

static int free_matched_acl_name(void* data)
{
	struct matched_acl_name* head = data;
	struct matched_acl_name *next=NULL;
	while(head)
	{
		next = head->next;
		xxfree(head);
		head =next;
	}
	return 0;
}

/*
   *traverse_pattern_array: 
   *traverse the pattern_array for test
   */
/*
static void traverse_pattern_array(struct pattern_node** pattern_array, int size)
{
	int i;
	for(i=0;i<size;i++)
	{
		if(pattern_array[i])
		{
			 struct pattern_node* node = pattern_array[i];
			 while(node)
			 {
				 debug(145,9)("i= %d\t pattern is: %s\t host is: %s\n",i,node->pattern,node->host);
				 node=node->next;
			 }
		}
	}
}
*/

/*
   *before_client_acl_check:
   *check the url_regex ,and find the acl name which the request match
   */
static void before_client_acl_check(clientHttpRequest* http)
{
	//traverse_pattern_array(pattern_array,MAX_PATTERN_NUM);
	if(mod->flags.config_on)
	{
		char* url = xstrdup(urlCanonical(http->request));

		char* host_start = strstr(url,"://");
		host_start+=3;
		char* host_end = strchr(host_start,'/');
		char *host = xcalloc(1,host_end - url+1);
		strncpy(host,url,host_end - url);

		unsigned int bucket = myHash(host,MAX_PATTERN_NUM);


		debug(145,3)("before_client_acl_check: enter, the url (%s), and host (%s) ,and the bucket is: %d \n",url,host,bucket);
		struct matched_acl_name * name_list=NULL;
		safe_free(host);
		if(pattern_array[bucket])
		{
			struct pattern_node* node = pattern_array[bucket];
			struct matched_acl_name * head=NULL;
			debug(145,3)("node->host: %s\t ,node->acl_name is: %s\t\n",node->host,node->acl_name);

			int i=0;
			while(node)
			{

				if(!regexec(&(node->comp),url,0,0,0))
				{
					i++;
					/*****add the acl_name to the clientHttpRequest's private_data **********/
					head = xcalloc(1,sizeof(struct matched_acl_name));
					strncpy(head->acl_name,node->acl_name,strlen(node->acl_name));
					head->type = node->type;

					head->next = name_list;
					name_list = head;
				}
				node = node->next;
			}


			/*****register the matched_acl_name list in the http->request   *************/
			if(name_list)
			{
				cc_register_mod_private_data(REQUEST_T_PRIVATE_DATA,http->request,name_list,free_matched_acl_name,mod);
			}
			debug(145,3)("before_client_acl_check: find %d times, we finaly find it\n",i);
		}
		else
		{
			debug(145,3)("before_client_acl_check: pattern_array[bucket] is: null\n");
		}

		debug(145,3)("the http->uri is: %s\n",url);
		safe_free(url);
	}
}



/*
   *private_aclMatchRegex:  before aclMatchRegex, we can look up the request's private_data for the matched acl names
   */
static int private_aclMatchRegex(acl* ae,request_t *r)
{

	if(mod->flags.config_on )
	{
		struct matched_acl_name* head =(struct matched_acl_name*)cc_get_mod_private_data(REQUEST_T_PRIVATE_DATA,r,mod);
		while(head !=NULL)
		{

			if(!strcmp(head->acl_name,ae->name))
			{
				debug(145,3)(" have matched acl_name: %s\n",ae->name);

				return 1;
			}
			head = head->next;

		}
	}
	else
	{
		debug(145,3)(" have no match\n");
	}
	return 0;
}


/*
 *storeKeyPublicUrl: calculate the md5 key according the url, learn from the storeKeyPublic function
 */
cache_key *
storeKeyPublicByUrl(char *url)
{

	debug(145,5)("storeKeyPublicByUrl: the url is %s\n",url);
	cache_key *digest = xcalloc(sizeof(cache_key),SQUID_MD5_DIGEST_LENGTH);
	SQUID_MD5_CTX M;
	SQUID_MD5Init(&M);
	SQUID_MD5Update(&M, (unsigned char *) url, strlen(url));
	SQUID_MD5Final(digest, &M);
	//debug(145,5)("digest's text is: %s\n",storeKeyText(digest));
	return digest;
}

/*
 * cc_aclCheck:
 * check the result's req_rel_aa, before fetching the cache result value'
 */
static int cc_aclCheck(allow_t *allow,aclCheck_t* checklist, struct acl_access_packer *aap/*acl_access* req_rel_aa*/)
{
	int match;
	const acl_access *A;
	ipcache_addrs *ia;
	struct acl_access_packer* my_packer = aap;
	//checklist->access_list = my_packer->aa;
	//cbdataLock(checklist->access_list);
	//A = checklist->access_list;
//	while((A=checklist->access_list)!=NULL)
	while (( my_packer/*checklist->access_list*/) != NULL)
	{
		A =checklist->access_list = my_packer->aa;
		cbdataLock(A);
		/*
		 * If the _acl_access is no longer valid (i.e. its been
		 * freed because of a reconfigure), then bail on this
		 * access check.  For now, return ACCESS_DENIED.
		 */
		if (!cbdataValid(A))
		{
			cbdataUnlock(A);
			checklist->access_list = NULL;
			break;
		}
		debug(145, 2) ("cc_aclCheck: checking '%s'\n", A->cfgline);
		*allow = A->allow;
		match = aclMatchAclList(A->acl_list, checklist);
		if (match == -1)
			*allow = ACCESS_DENIED;
		if (checklist->state[ACL_DST_IP] == ACL_LOOKUP_NEEDED)
		{
			checklist->state[ACL_DST_IP] = ACL_LOOKUP_PENDING;
			ipcache_nbgethostbyname(checklist->request->host,
					aclLookupDstIPDone, checklist);
			return 2;
		}
		else if (checklist->state[ACL_DST_ASN] == ACL_LOOKUP_NEEDED)
		{
			checklist->state[ACL_DST_ASN] = ACL_LOOKUP_PENDING;
			ipcache_nbgethostbyname(checklist->request->host,
					aclLookupDstIPforASNDone, checklist);
			return 2;
		}
		else if (checklist->state[ACL_SRC_DOMAIN] == ACL_LOOKUP_NEEDED)
		{
			checklist->state[ACL_SRC_DOMAIN] = ACL_LOOKUP_PENDING;
			fqdncache_nbgethostbyaddr(checklist->src_addr,
					aclLookupSrcFQDNDone, checklist);
			return 2;
		}
		else if (checklist->state[ACL_DST_DOMAIN] == ACL_LOOKUP_NEEDED)
		{
			ia = ipcacheCheckNumeric(checklist->request->host);
			if (ia == NULL)
			{
				checklist->state[ACL_DST_DOMAIN] = ACL_LOOKUP_DONE;
				return 2;
			}
			checklist->dst_addr = ia->in_addrs[0];
			checklist->state[ACL_DST_DOMAIN] = ACL_LOOKUP_PENDING;
			fqdncache_nbgethostbyaddr(checklist->dst_addr,
					aclLookupDstFQDNDone, checklist);
			return 2;
		}
		else if (checklist->state[ACL_PROXY_AUTH] == ACL_LOOKUP_NEEDED)
		{
			debug(145, 3)
				("cc_aclCheck: checking password via authenticator\n");
			aclLookupProxyAuthStart(checklist);
			checklist->state[ACL_PROXY_AUTH] = ACL_LOOKUP_PENDING;
			return 2;
		}
		else if (checklist->state[ACL_PROXY_AUTH] == ACL_PROXY_AUTH_NEEDED)
		{
			/* Client is required to resend the request with correct authentication
			 * credentials. (This may be part of a stateful auth protocol.
			 * The request is denied.
			 */
			debug(145, 2) ("cc_aclCheck: requiring Proxy Auth header.\n");
			*allow = ACCESS_REQ_PROXY_AUTH;
			match = -1;
		}
#if USE_IDENT
		else if (checklist->state[ACL_IDENT] == ACL_LOOKUP_NEEDED)
		{
			debug(145, 2) ("cc_aclCheck: Doing ident lookup\n");
			if (cbdataValid(checklist->conn))
			{
				identStart(&checklist->conn->me, &checklist->conn->peer,
						aclLookupIdentDone, checklist);
				checklist->state[ACL_IDENT] = ACL_LOOKUP_PENDING;
				return 2;
			}
			else
			{
				debug(145, 2) ("cc_aclCheck: Can't start ident lookup. No client connection\n");
				cbdataUnlock(checklist->conn);
				checklist->conn = NULL;
				*allow = ACCESS_DENIED;
				match = -1;
			}
		}
#endif
		else if (checklist->state[ACL_EXTERNAL] == ACL_LOOKUP_NEEDED)
		{
			acl *acl = checklist->current_acl;
			assert(acl->type == ACL_EXTERNAL);
			externalAclLookup(checklist, acl->data, aclLookupExternalDone, checklist);
			return 2;
		}
		/*
		 * We are done with this _acl_access entry.  Either the request
		 * is allowed, denied, requires authentication, or we move on to
		 * the next entry.
		 */
		if (match)
		{
			debug(145, 2) ("cc_aclCheck: match found, returning %d\n", *allow);
			cbdataUnlock(A);
			//aclCheckCallback(checklist, allow);
			return 1;
		}
		my_packer = my_packer->next;
		//checklist->access_list = A->next;
		/*
		 * Lock the next _acl_access entry
		 */
		/*
		if (A->next)
			cbdataLock(A->next);
			*/
		cbdataUnlock(A);
	}
	return 0;
}

static int cc_aclCheckFastRequest2(aclCheck_t *ch,/* cc_module *mod,*/ struct result_node *node ,cc_acl_check_result *result)
{
	int param_pos = 0;
	int answer = 0;
	//	acl_access *A = mod->acl_rules;
	if(node->req_rel_aa == NULL)
		return 0;
	//acl_access *A = node->req_rel_aa->aa;
	acl_access *A;
	struct acl_access_packer * my_packer = node->req_rel_aa->aap;
	//	cbdataLock(A);
	while (my_packer)
	{
		A = my_packer->aa;
		answer = aclMatchAclList(A->acl_list, ch);
		if (answer > 0 && A->allow == ACCESS_ALLOWED)
		{
			/* matched acl then set the acl_run*/
			debug(145, 3) ("cc_aclCheckFastRequest: fd match A and A->cfgline = %s\n", A->cfgline);
			result->param_poses = xrealloc(result->param_poses, (++result->param_count) * sizeof(int));
			//result->param_poses[result->param_count - 1] = param_pos;
			result->param_poses[result->param_count - 1] = node->req_rel_aa->param_poses[param_pos];
		}
		param_pos++;
		my_packer = my_packer->next;
		//A = A->next;
	}

	return result->param_count > 0;
}

/*
 *before_loop: 
 *loop is dedicated to the every comparision to the node of acl_access list's 
 *before_loop try to find the cached result, aim to not enter the loop.
 */
static int before_loop(allow_t *allow,aclCheck_t* checklist,acl_access* A,cc_acl_check_result *result, int type, cc_module* mod_p)
{

	if(mod->flags.config_on )
	{

		debug(145,3)("before_loop: type is %d, acl_access is %p \n",type,A);

		debug(145,9)("before_loop: hash_confilict is: %u , the cached_result is : %u \n",hash_conflict,cached_result);
		/****
		 *if type == 1 , it's the squid's origin aclCheck.
		 *if type == 0, it's the FC's module cc_aclCheckFastRequest
		 */
		char* url = xstrdup(urlCanonical(checklist->request));
		/*
		 *if the url contain the @@@, which is added by mod_header_combination or store_multi_url, we must remove it in
		 *the url.
		 **/
		char* pos = NULL;
		if( NULL != (pos =strstr(url,"@@@")))
		{
			*pos = '\0';
		}
		cache_key* key = storeKeyPublicByUrl(url);
		unsigned int bucket = myHash(url,MAX_RESULT_NUM);
		struct aclCheck_result* node;
		node = &result_array[bucket];
		debug(145,3)("before_loop and the bucket is: %d, the url is: %s, and the request method is: %d\n",bucket,url,checklist->request->method);

		   if(node->digest)
		   {
		   debug(145,3)("before_loop: the node->digest is: %s\t, the key is: %s \n",storeKeyText(node->digest), storeKeyText(key));
		   }
		if((node->digest)&&!storeKeyHashCmp(node->digest,key)) // if the key equal to the node->digest, then find the result
		{
		   debug(145,3)("before_loop: (node->digest)&&!storeKeyHashCmp(node->digest,key) is true \n");
			int found_count = 0;
			struct result_node* head = node->head;
			while(head)
			{
					debug(145,3)("before_loop: (node->digest)&&!storeKeyHashCmp(node->digest,key) is true , and head->access == %p , A == %p\n",head->access_type, A);
				if(head->access_type == A)
				{
					debug(145,3)("before_loop: (node->digest)&&!storeKeyHashCmp(node->digest,key) is true and if(head->access_type == A) is ture\n");
					if(type ==1) // type == 1 represents this is a aclCheck result, we should set the allow param
					{
						acl_cache_hit++;
						int ret = 0;
						if(head->req_rel_aa)
						{
							ret =cc_aclCheck(allow,checklist,head->req_rel_aa->aap);
							if(ret == 1)
							{
								safe_free(url);
								safe_free(key);
								return 1;
							}
							if(ret == 2)
							{
								safe_free(url);
								safe_free(key);
								return 2;
							}
						}
						*allow = head->allow;	
						AclMatchedName = node->AclMatchedName;

						debug(145,2)("before_loop: type is 1,  url match: %s, found_count is: %d, AclMatchName == %s\n",url,found_count,node->AclMatchedName);
						safe_free(url);
						safe_free(key);
						//						acl_cache_hit++;
						return 1;
					}
					if(type == 0)
					{

						int ret = cc_aclCheckFastRequest2(checklist,head,result);

						//						if(ret)
						{
							result->param_poses = xrealloc(result->param_poses, (result->param_count + head->result.param_count)* sizeof(int));
							memcpy(result->param_poses + result->param_count,head->result.param_poses, sizeof(int) * (head->result.param_count));
							result->param_count += head->result.param_count;
							debug(145,2)("before_loop: type is 0, url match: %s , found_count is: %d\n",url,found_count);
							debug(145,2)("before_loop: param_count : %d , ret is: %d\n",result->param_count, ret);
							safe_free(url);
							safe_free(key);
						}
						acl_cache_hit++;
						return 1;
						//return 0;

					}
				}
				head = head->next;
				found_count++;
			}
		}
		/* the key not equal to the node->digest, then we have to delete the existed result, we can also do it in before_acl		  *_callback
		*/
		///////////////////////////////
		else
		{
			if(node->digest && storeKeyHashCmp(node->digest,key))
			{
				debug(145,2)("before_loop: url ( %s ) hash conflict, and the bucket is: %d \n",url,bucket);
				hash_conflict++;

				if(node->head)
				{
					struct result_node *head = node->head;
					struct result_node *next;
					while(head)
					{
						//			printf("%p\n",head);
						next = head->next;
						safe_free(head->result.param_poses);
						/* safe_free the req_rel_aa struct*/
						struct req_rel * rel_tmp = head->req_rel_aa;
						if(rel_tmp)
						{
							/*
							acl_access *rel_aa = rel_tmp->aa;
							acl_access *rel_next;
							while(rel_aa)
							{

								rel_next = rel_aa->next;
								cbdataFree(rel_aa);
								rel_aa = rel_next;
							}
							*/
							struct acl_access_packer  *aap = rel_tmp->aap;
							struct acl_access_packer  *aap_next ;

							while(aap)
							{
								aap_next = aap->next;
								safe_free(aap);
								aap = aap_next;

							}

							safe_free(rel_tmp->param_poses);
							safe_free(rel_tmp);
						}
						safe_free(head);


						head = next;

					}

				}
				safe_free(result_array[bucket].digest);
				result_array[bucket].head = NULL;
				debug(145,2)("before_loop: before cached_result--:\n");
				cached_result--;
				//safe_free(result_array[bucket].head);
			}

		}
		/////////////////////////
		debug(145,2)("before_loop: url  mismatch: %s\n",url);
		safe_free(url);
		safe_free(key);
	}
	return 0;
}

static int construct_req_rel_aa(/*cc_module *mod_p,*/ struct result_node* node, const acl_access* aa_end, acl_access* aa_start)
{
	acl_access* aa_p = aa_start;
	int param_pos = 0;
	while(aa_p != aa_end && aa_p!=NULL && aa_end != NULL)
	{
		debug(145,2)("construct_req_rel_aa: aa_start is: (%s), and aa_end is: (%s)\n",aa_start->cfgline, aa_end->cfgline);

		if(aa_p)
		{
			acl_list *a_list = aa_p->acl_list;
			while(a_list)
			{
				if(!(a_list->acl->type == ACL_DST_DOM_REGEX || a_list->acl->type == ACL_DST_DOMAIN || a_list->acl->type == ACL_URLPATH_REGEX || a_list->acl->type == ACL_URL_REGEX || a_list->acl->type == ACL_URL_PORT || a_list->acl->type == ACL_MY_PORT || a_list->acl->type == ACL_MY_PORT_NAME || !strncmp(a_list->acl->name,"all",3)))
				{
					debug(145,2)("construct_req_rel_aa: add acl_access ( %s )to req_rel_aa \n",aa_p->cfgline);

					if(node->req_rel_aa == NULL)
					{
						node->req_rel_aa = xcalloc(1,sizeof(struct req_rel));
					}
					struct req_rel *node_req_rel = node->req_rel_aa; // get the node's req_rel struct

					struct acl_access_packer* rel_aa= node_req_rel->aap; // get the req_rel's aa
					struct acl_access_packer* next =rel_aa; 
					/* get the end of the aa list, next points to the NULL, and the rel_aa points to the end
					 * we must realize that the rel_aa and next points to the same NULL, when the list is empty*/
					while(next)
					{
						rel_aa = next;
						next = rel_aa->next;
					}
					//				next = xcalloc(1,sizeof(acl_access));
					//
					//next = cbdataAlloc(acl_access);
					next = xcalloc(1,sizeof(struct acl_access_packer));
					debug(145,3)("construct_req_rel_aa: next pionter %p\n",next);
					//cbdataLock(next);
					next->aa = aa_p;
					next->next = NULL;
					debug(145,3)("construct_req_rel_aa: add acl_access to req_rel_aa ,.....p1, node is %p\n",node);
					if(node->req_rel_aa->aap == NULL)
					{
						/*if the node->req_rel_aa is NULL , meaning that we should xcalloc mem for the point
						 * and the acl_access list must point to the next, the req_rel_aa->count = 1; req_rel_aa->pos 
						 * has only one member which equels to param_pos;
						 */
						debug(145,3)("construct_req_rel_aa: add acl_access to req_rel_aa ,........p2, \n");
						//xcalloc(1,sizeof(struct req_rel));
						node->req_rel_aa->aap = next; 
						node->req_rel_aa->count = 1;
						node->req_rel_aa->param_poses = xcalloc(1,sizeof(int));
						node->req_rel_aa->param_poses[0] = param_pos;
					}
					else
					{
						rel_aa->next = next;
						node->req_rel_aa->param_poses = xrealloc(node->req_rel_aa->param_poses, (++node->req_rel_aa->count) * sizeof(int));

						node->req_rel_aa->param_poses[node->req_rel_aa->count - 1] = param_pos;

					}
					next = NULL;
					debug(145,3)("construct_req_rel_aa: add acl_access to req_rel_aa ,........p3\n");
					//		aa_p = aa_p->next;

					break;

				}
				a_list = a_list->next;
			}
		}

		debug(145,2)("construct_req_rel_aa: aa_p cfgline is: (%s) param_pos is %d\n",aa_p->cfgline,param_pos);
		aa_p = aa_p->next;
		param_pos++;

	}
	debug(145,2)("construct_req_rel_aa: param_pos is: %d\n",param_pos);
	return 0;
}

static 
acl_access* locate_access_type(char *token, const request_t *request)
{
	char* host = (char *)request->host;
	int slot = myHash(host,SQUID_CONFIG_ACL_BUCKET_NUM);
	access_array *aa = Config.accessList2[slot];
	acl_access *acl = NULL;

	/* get the domain access_arrya if exist */
	while(aa)
	{
		if(strncmp(aa->domain,host,strlen(host)) == 0)
		{
			acl = locateAccessType(token,aa);
			break;
		}
		aa = aa->next;
	}
	/* if acl is NULL , which dedicates there are no right access_array for the domain, we set the right domain as common*/
	if(acl == NULL)
	{
		slot = myHash("common",SQUID_CONFIG_ACL_BUCKET_NUM);
		aa = Config.accessList2[slot];
		while(aa)
		{
			if(strcmp("common",aa->domain) == 0)
			{
				acl = locateAccessType(token,aa);
				break;
			}
			aa = aa->next;

		}
	}
	return acl;
}
/*
 *before_acl_callback: check if the aclCheck or cc_aclCheckFastRequest result is in the  result_array, if it is, return. 
 * or put it into the array
 */
static int before_acl_callback(allow_t allow,aclCheck_t* checklist, acl_access* aa)
{

	//traverse_pattern_array(pattern_array, MAX_PATTERN_NUM);
	debug(145,5)("before_acl_callback, enter . aa == %p\n",aa);
	if(mod->flags.config_on)
	{
		if(checklist->request)
		{
			/* acl result cache must not contain the acl ,type of time  and user-agent
			 * */

			const acl_access *A = checklist->access_list;
			if(A)
			{
				acl_list *a_list = A->acl_list;
				while(a_list)
				{
					if(!(a_list->acl->type == ACL_DST_DOM_REGEX || a_list->acl->type == ACL_DST_DOMAIN || a_list->acl->type == ACL_URLPATH_REGEX || a_list->acl->type == ACL_URL_REGEX || a_list->acl->type == ACL_URL_PORT || a_list->acl->type == ACL_MY_PORT || a_list->acl->type == ACL_MY_PORT_NAME || 0==strncmp(a_list->acl->name,"all",3)))
					{
						debug(145,2)("before_acl_callback: the acl type %d  can't be cached \n",a_list->acl->type);
						return 0;

					}
					a_list = a_list->next;
				}
			}

			/* */
			char* url = xstrdup(urlCanonical(checklist->request));
			char* pos = NULL;
			if( NULL != (pos =strstr(url,"@@@")))
			{
				*pos = '\0';
			}
			cache_key* key = storeKeyPublicByUrl(url);

			unsigned int bucket = myHash(url,MAX_RESULT_NUM);
			debug(145,4)("before_acl_callback, buchet is: %d \n",bucket);
			struct aclCheck_result* node;
			node = &result_array[bucket];
			int find_count = 0;

			//if(aa == Config.accessList.http  && allow == ACCESS_DENIED && node->digest && !storeKeyHashCmp(key,node->digest))
			if(aa == locate_access_type("http_access",checklist->request)  && allow == ACCESS_DENIED && node->digest && !storeKeyHashCmp(key,node->digest))
			{
				debug(145,3)("before_acl_callback: set AclMatchedName as :%s\n",AclMatchedName);
				result_array[bucket].AclMatchedName = xstrdup(AclMatchedName);

			}
			if(node->digest && !storeKeyHashCmp(key,node->digest))
			{
				struct result_node* head = node->head;
				while(head)
				{
					debug(145,3)("before_acl_callback, result_array[bucket] is not null, and head->access-type is : %p \t, checklist->access_list is: %p \n",head->access_type, aa);
					find_count++;
					if(head->access_type == aa && head->access_type != NULL && head->type ==1)
					{
						debug(145,3)("before_acl_callback, match found, and find-count is: %d\n",find_count);
						safe_free(key);
						safe_free(url);
						return 0;

					}
					head = head->next;
				}
				/*now we find the matched reulst bucket, but no the matched access_type, so we add it 
				*/
				struct result_node* new_result = xcalloc(1,sizeof(struct result_node));
				new_result->allow = allow;
				new_result->type = 1;
				new_result->access_type = aa;
				new_result->next = result_array[bucket].head;
				debug(145,2)("before_acl_callback: before construct_req_rel_aa, aa is : %p ,and aa's cfgline is : %s'\n",aa,aa->cfgline);

				construct_req_rel_aa(new_result,checklist->access_list,aa); // construct the req_rel_aa 
				result_array[bucket].head = new_result;
				debug(145,3)("now we find the matched reulst bucket, but no the matched access_type, so we add it\n ");
				safe_free(key);
				safe_free(url);
				return 0;
			}
			find_count=0;
			if(aa ==NULL)
			{
				debug(145,3)("before_acl_callback: checklist->access_list is NULL\n");
				safe_free(key);
				safe_free(url);
				return 0;
			}
			struct result_node* new_result = xcalloc(1,sizeof(struct result_node));


			//dstrncpy(result->uri,url,strlen(url));
			new_result->allow = allow;
			new_result->access_type = aa;
			new_result->type = 1;

			new_result->next = result_array[bucket].head;
			debug(145,2)("before_acl_callback: before construct_req_rel_aa, aa is : %p ,and aa's cfgline is : %s'\n",aa,aa->cfgline);
			construct_req_rel_aa(new_result,checklist->access_list,aa); // construct the req_rel_aa 
			result_array[bucket].head = new_result;
			//cache_key* key = xcalloc(SQUID_MD5_DIGEST_LENGTH,sizeof(cache_key));
			//memcpy(key,key_orig,SQUID_MD5_DIGEST_LENGTH);
			if(result_array[bucket].digest)
			{
				debug(145,3)("before_acl_callback: result_array[bucket].digest is not null: and we safe_free the digest\n");
				safe_free(result_array[bucket].digest);
				safe_free(result_array[bucket].AclMatchedName);
			}
			result_array[bucket].digest = key;
			if(aa == locate_access_type("http_access",checklist->request) && allow == ACCESS_DENIED )
			{
				debug(145,3)("before_acl_callback: set AclMatchedName as :%s\n",AclMatchedName);
				result_array[bucket].AclMatchedName = xstrdup(AclMatchedName);

			}
			safe_free(url);
			key =NULL;
			debug(145,3)("before_acl_callback: before cached_result++\n");
			cached_result++;
			//debug(145,3)("before_acl_callback:  result->allow is: %d, the result_array[bucket]->access_type is: %p, the bucket is: %d\n", result_array[bucket]->allow, result_array[bucket]->access_type,bucket);
		}
	}
	return 0;
}

static acl_access * locateAcl(acl_access* list,int pos, int size)
{
	acl_access* p=NULL;
	int i;
	if(pos>=size)
	{
		debug(145,3)("locateAcl: ERROR:  pos >= size\n");
		return NULL;
	}
	p = list;
	for(i=0;i<size;i++)
	{
		debug(145,3)("locateAcl: cfgline of %d is %s\n",i,p->cfgline);
		if(pos == i)
			return p;
		p = p->next;
	}
	return NULL;
}


int cc_aclCheckFastRequestDone(aclCheck_t* checklist,cc_module *mod_p, acl_access* aa, cc_acl_check_result *result, int type)
{
	debug(145,4)("cc_aclCheckFastRequestDone enter\n");
	if(mod->flags.config_on)
	{
		if(checklist->request)
		{
			/* 
			 * the time , browser, acl type ,we must not cache them*/
			int i;
			int count = result->param_count;

			debug(145,3)("cc_aclCheckFastRequestDone: result->param_count is : %d\n",count);
			if(count==0)
			{
				debug(145,3)("cc_aclCheckFastRequestDone: no matched param, so we don't cached the result\n'");
				return 0;
			}
			if(strcmp(mod_p->name,"mod_header")==0 || strcmp(mod_p->name,"mod_merge_header") ==0 || strcmp(mod_p->name, "mod_offline") == 0)
			{
				debug(145,3)("cc_aclCheckFastRequestDone: mod_p->name is mod_header == %s\n, so we don't cached the result\n",mod_p->name);
				return 0;
			}
			acl_access *A=NULL;


			mod_domain_access *cur = cc_get_domain_access(checklist->request->host,mod_p);
			if(cur == NULL)
			{
				cur = cc_get_domain_access("common",mod_p);
			}
			assert(cur);
			int param_num = cur->param_num;
			for(i=0;i<count;i++)
			{
				debug(145,3)("cc_aclCheckFastRequestDone: i is : %d, mod_p->param_num %d, mod_p->name == %s\n",i,mod_p->mod_params.count,mod_p->name);
				A = locateAcl(aa,result->param_poses[i],mod_p->mod_params.count);
				if(A)
				{
					acl_list *a_list = A->acl_list;
					debug(145,3)("acl_aclCheckFastRequestDone: cfgline is : %s\n",A->cfgline);
					while(a_list)
					{
						debug(145,2)("cc_aclCheckFastRequestDone:acl name is: %s\n",a_list->acl->name);
						if(!( a_list->acl->type == ACL_DST_DOM_REGEX || a_list->acl->type == ACL_DST_DOMAIN || a_list->acl->type == ACL_URLPATH_REGEX || a_list->acl->type == ACL_URL_REGEX || a_list->acl->type == ACL_URL_PORT || a_list->acl->type == ACL_MY_PORT || a_list->acl->type == ACL_MY_PORT_NAME || 0==strncmp(a_list->acl->name,"all",3) ))
						{
							debug(145,2)("cc_aclCheckFastRequestDone: acl type ( %d) , name (%s) cannot cached:\n",a_list->acl->type,a_list->acl->name);
							return 0;
						}
						a_list = a_list->next;

					}
				}
				A= NULL;
			}
			/* */
			char* url = xstrdup(urlCanonical(checklist->request));
			char* pos = NULL;
			if( NULL != (pos =strstr(url,"@@@")))
			{
				*pos = '\0';
			}
			cache_key* key = storeKeyPublicByUrl(url);

			unsigned int bucket = myHash(url,MAX_RESULT_NUM);
			debug(145,4)("cc_aclCheckFastRequestDone, buchet is: %d \n",bucket);
			struct aclCheck_result* node;
			node = &result_array[bucket];
			int find_count = 0;
			if(node->digest && !storeKeyHashCmp(key,node->digest))
			{
				struct result_node* head = node->head;
				while(head)
				{
					find_count++;
					//////fprintff(stderr,"cc_aclCheckFastRequestDone, result_array[bucket] is not null, and head->access-type is : %p \t, checklist->access_list is: %p \n",head->access_type, checklist->access_list);
					if(head->access_type == aa && head->access_type != NULL && head->type == type)
					{
						debug(145,3)("cc_aclCheckFastRequestDone, match found, and find-count is: %d\n",find_count);
						safe_free(key);

						safe_free(url);
						return 0;

					}
					head = head->next;
				}
				struct result_node* new_result = xcalloc(1,sizeof(struct result_node));
				new_result->type = 0;
				new_result->access_type = aa;
				/* locate the the last match acl_access , and construct the req_rel_aa */

				A = locateAcl(aa,result->param_poses[count-1],param_num);
				construct_req_rel_aa(new_result,A,aa);
				/* */
				new_result->result.param_count = result->param_count;
				new_result->result.param_poses = xcalloc(result->param_count,sizeof(int));
				memcpy(new_result->result.param_poses,result->param_poses, sizeof(int) * (result->param_count));
				new_result->next = result_array[bucket].head;
				result_array[bucket].head = new_result;
				debug(145,3)("cc_aclCheckFastRequestDone:now we find the matched reulst bucket, but no the matched access_type, so we add it\n ");
				safe_free(key);
				safe_free(url);
				return 0;

			}
			find_count=0;
			if(aa ==NULL)
			{
				debug(145,3)("cc_aclCheckFastRequestDone: checklist->access_list is NULL\n");
				safe_free(key);
				safe_free(url);
				return 0;
			}

			////////////////////////////////////
			struct result_node* new_result = xcalloc(1,sizeof(struct result_node));

			//strncpy(new_result->uri,url,strlen(url));
			new_result->access_type = aa;
			new_result->type = type;
			A = locateAcl(aa,result->param_poses[count-1],param_num); // locate the acl_access of last mod_param
			construct_req_rel_aa(new_result,A,aa); // construct_req_rel_aa for the mod acl_access list 
			new_result->result.param_count = result->param_count;
			new_result->result.param_poses = xcalloc(result->param_count,sizeof(int));
			memcpy(new_result->result.param_poses,result->param_poses, sizeof(int) * (result->param_count));
			new_result->next = result_array[bucket].head;
			result_array[bucket].head = new_result;
			//cache_key* key = xcalloc(SQUID_MD5_DIGEST_LENGTH,sizeof(cache_key));
			//memcpy(key,key_orig,SQUID_MD5_DIGEST_LENGTH);
			if(result_array[bucket].digest !=NULL)
				safe_free(result_array[bucket].digest);
			result_array[bucket].digest = key;
			safe_free(url);
			debug(145,2)("cc_aclCheckFastRequestDone: before cached_result++\n");
			cached_result++;
		}
	}
	return 0;
}
/* module init */
int mod_register(cc_module *module)
{
	debug(145, 1)("(mod_refresh_pattern) ->	mod_register:\n");

	strcpy(module->version, "7.0.R.16488.i686");

	//module->hook_func_sys_parse_param = func_sys_parse_param;
	/*
	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
			func_sys_parse_param);
			*/
	//module->hook_func_sys_after_parse_param = hook_after_parse_param;
	cc_register_hook_handler(HPIDX_hook_func_sys_after_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_after_parse_param),
			hook_after_parse_param);
	//module->hook_func_before_client_acl_check = before_client_acl_check;
	cc_register_hook_handler(HPIDX_hook_func_before_client_acl_check,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_before_client_acl_check),
			before_client_acl_check);
	//module->hook_func_sys_init = hook_init;
	//module->hook_func_private_aclMatchRegex = private_aclMatchRegex;
	cc_register_hook_handler(HPIDX_hook_func_private_aclMatchRegex,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_aclMatchRegex),
			private_aclMatchRegex);
	//module->hook_func_private_before_loop = before_loop;
	cc_register_hook_handler(HPIDX_hook_func_private_before_loop,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_before_loop),
			before_loop);
	//module->hook_func_private_before_acl_callback = before_acl_callback;
	cc_register_hook_handler(HPIDX_hook_func_private_before_acl_callback,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_before_acl_callback),
			before_acl_callback);
	//module->hook_func_private_cc_aclCheckFastRequestDone = cc_aclCheckFastRequestDone;
	cc_register_hook_handler(HPIDX_hook_func_private_cc_aclCheckFastRequestDone,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_cc_aclCheckFastRequestDone),
			cc_aclCheckFastRequestDone);

	//	module->hook_func_private_refresh_limits = hook_refresh_pattern;

//	mod = module;
	if(reconfigure_in_thread)
	mod = (cc_module*)cc_modules.items[module->slot];
	else
	mod = module;
	return 0;
}
#endif

