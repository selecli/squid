#include "cc_framework_api.h"
#include <fnmatch.h>
#include <pthread.h>

#ifdef CC_FRAMEWORK
#define AUTODETECT_TIME_DEFAULT 300
#define AUTODETECT_COUNT_DEFAULT 5
#define MAX_REG_MATCH 99
#define ASYNC_LOG_PATH "/data/proclog/log/squid/async.log"

//这是本模块在主程序的索引信息的结构指针
static cc_module* mod = NULL;

//This is the Switch of wether use read_anyhost or not
static int have_autodetecton = 0; //0: dont have, 1: have

//This is to protect async.log open more than once
static int need_async_log = 0; //0: dont need, 1: need
static int have_opened_async_log = 0; //0: not open yet, 1: open
static Logfile* asynclog = NULL;

//off_line type
typedef enum
{
	OFF_LINE_NONE,
	OFF_LINE_DEFAULT,
	OFF_LINE_PURE,
	OFF_LINE_REDIRECT,
	OFF_LINE_ASYNC,
	OFF_LINE_ASYNC_REFRESH
}offline_type;

//mod_param
typedef struct _mod_config
{
	// 0: off; 1: on 
	int autodetect_onoff;
	int autodetect_time;
	int autodetect_count;

	offline_type type;
	String original_domain;
	String replace_domain;
	int time_gap; //use in async
}mod_config;

//private data for every req
typedef struct _req_flags
{
	offline_type type;
	String redirect_domain;
	int time_gap_flag; //use in async. 0: not async hit, 1: async hit
	int is_compress;   //1: compress  0:not compress
}req_flags;

typedef struct _offline_domain_t
{
	char *domain;
	// 0: up; 1: down
	int updown;
	int down_num;
	int all_ip_down; // 0: not down; 1: all down
	struct _offline_domain_t *next;
	// 0: not read; 1: have read
	int passby;
}offline_domain_t;

offline_domain_t *offline_domains;
offline_domain_t *offline_domains2;
int offline_domains2_updated = 0;
int auto_detect_num;
pthread_t read_anyhost_ptid;
int thread_should_exit = 0;
int num_threads = 0;
static MemPool * req_flags_pool = NULL;
static MemPool * mod_config_pool = NULL;

static void * req_flags_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == req_flags_pool)
	{
		req_flags_pool = memPoolCreate("mod_offline private_data req_flags", sizeof(req_flags));
	}
	return obj = memPoolAlloc(req_flags_pool);
}

static void * mod_config_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == mod_config_pool)
	{
		mod_config_pool = memPoolCreate("mod_offline config_struct", sizeof(mod_config));
	}
	return obj = memPoolAlloc(mod_config_pool);
}

/**************************** async dealer***************************/

/*
 *This func is to use the set time to check expires or not. only use in async func
 */
static int AsyncExpireCheck(clientHttpRequest *http)
{
	int fd = http->conn->fd;
	Array * modparam = cc_get_mod_param_array(fd, mod);
	int paramcount = cc_get_mod_param_count(fd, mod);
	mod_config *cfg = NULL;
	int i = 0;

	assert(modparam);

	time_t time = http->entry->timestamp;


	for(i=0; i<paramcount; i++)
	{
		cfg = (mod_config*)((cc_mod_param*)modparam->items[i])->param;
		
		assert(cfg);

		if(OFF_LINE_ASYNC != cfg->type)
			continue;
		else
			break;
	}
	
	debug(99, 5)("(mod_offline) ->  **********squid_curtime=%ld, cfg->time_gap=%d, time=%ld  inall=%ld\n", squid_curtime, cfg->time_gap, time, cfg->time_gap + time);
	
	if(time < -1)
		time = squid_curtime;
	else
	{
		if(squid_curtime > cfg->time_gap + time)
			return 1;
		else
			return 0;
	}
	return 0;

}


/*
 *This write the log
 */
static int  asyncLog(clientHttpRequest* http)
{
	AccessLogEntry* al = &http->al;

	if (0 == need_async_log)
		return 0;

	if(NULL == asynclog)
	{
		debug(99, 1)("(mod_offline) ->  WARNNING: async.log not opend\n");
		return -1;
	}

	req_flags *reqFlags = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);

	if(reqFlags && reqFlags->time_gap_flag && al->url)
	{
		logfileLineStart(asynclog);
		if(reqFlags->is_compress)
			logfilePrintf(asynclog, "%9ld  %s  c\n", (long int) current_time.tv_sec, rfc1738_escape_unescaped(al->url));
		else
			logfilePrintf(asynclog, "%9ld  %s  n\n", (long int) current_time.tv_sec, rfc1738_escape_unescaped(al->url));
		logfileLineEnd(asynclog);
	}
	return 0;
}


/*
 *This is to close the log
 */
static void AsyncLogClose(void)
{
	if (NULL == asynclog)
		return;
	logfileClose(asynclog);
	asynclog = NULL;
}

/*
 *This is to flush the log before close
 */
static void AsyncLogFlush(void)
{
	if (NULL == asynclog)
		return;
	logfileFlush(asynclog);
}

/*
 *Check if the request is local one
 */
static int checkLocal(clientHttpRequest* http)
{
	request_t* req = http->request;	

	if (strcmp("127.0.0.1", inet_ntoa(req->client_addr)) == 0)
	{
		String xff = httpHeaderGetList(&http->request->header, HDR_X_FORWARDED_FOR);
		if(!strLen(xff) || strStr(xff, "127.0.0.1"))
		{
			stringClean(&xff);
			return 1;
		}       
		else    
		{       
			stringClean(&xff);
			return 0;
		}  
	}
	else
		return 0;
}


/**************************** anyhost handle ****************************/



/* free private global variable*/
static void free_offline_domain(offline_domain_t *list)
{
	//assert(list);
	offline_domain_t *h = list;
	offline_domain_t *t = NULL;
	while(h) 
	{
		t = h;
		assert(h->domain);
		safe_free(h->domain);
		h = h->next;
		safe_free(t);
	}
}


static offline_domain_t * search_old_host(const char *domain, offline_domain_t * from_which)
{
	debug(99, 6)("search_old_host: domain '%s'\n", domain);
	if(domain == NULL)
		return NULL;
	if(from_which == NULL)
		return NULL;

	offline_domain_t *head;
	for(head = from_which; head != NULL; head = head->next)
	{       
		if(!strcmp(domain, head->domain))
			return head;
	}       
	return NULL;

}

static offline_domain_t * get_old_host(const char *domain)
{
	return search_old_host(domain, offline_domains);
}

/**
static offline_domain_t * get_old_host2(const char *domain)
{
	return search_old_host(domain, offline_domains2);
}
*/
static void print_offline_domains2(void )
{
	offline_domain_t *offline;
	for(offline = offline_domains; offline; offline = offline->next)
	{
		debug(99, 5)("read_anyhost print %p, %s, updown %d, down_num %d\n", \
				offline, offline->domain, offline->updown, offline->down_num);
	}       
}

void set_exit_flag(int sig)
{
    //thread_should_exit = 1;
    debug(99, 6)("mod_offline: set_exit_flag before pthread_exit\n");
    pthread_exit("i will exit");
    debug(99, 6)("mod_offline: set_exit_flag after pthread_exit\n");
}

/**
  * read anyhost
 */
void* read_anyhost(void *vars)
{
	num_threads ++;
	mod_config *ot = (mod_config *)cc_get_global_mod_param(mod);
	if(!ot)
	{
		return NULL;
	}
    signal(SIGQUIT, set_exit_flag);
	int autodetect_time = ot->autodetect_time;
	int autodetect_count = ot->autodetect_count;
	while(1)
	{
		if(thread_should_exit)
		{
			num_threads --;
			free_offline_domain(offline_domains2);
			offline_domains2 = NULL;

			if(num_threads == 0)
			{
				thread_should_exit = 0;
			}

			return NULL;
		}
		
		if(offline_domains2_updated)
		{
			debug(99,9)("pthread %d, offline_domains2_updated not used, num_threads=%d,thread_should_exit=%d\n",(int)pthread_self(),num_threads,thread_should_exit);
			goto sleep3;
		}
		const char *path = "/var/named/anyhost";
		debug(99, 9) ("read_anyhost: %s\n", path);
		FILE *anyhost = fopen(path, "r");
		if(anyhost == NULL){
			debug(99, 9) ("read_anyhost: can not open anyhost file, %s\n", xstrerror());
			goto sleep3;
		}
		char domain_buf[1024];
		char *pos = NULL;
		while((pos = fgets(domain_buf, 1023, anyhost))) {
			debug(99, 9) ("read_anyhost: line '%s'\n", domain_buf);
			/* it's comment */
			if(domain_buf[0] == ';')
				continue;
			int ip_down_mark = 0;
			char *domain = NULL;
			char *token = NULL;
			/* found 'ip_down' in domain_buf */
			if(strstr(pos, "ip_down"))
				ip_down_mark = 1;
			offline_domain_t *host = NULL;
			/* get domain */
			if((token = strtok_r(pos, w_space, &pos))) {
				int tl = strlen(token);
				if(token[--tl] == '.')
					token[tl] = '\0';
				domain = token;
			}
			/* go to third word 'A' */
			if((token = strtok_r(NULL, w_space, &pos)) == NULL)
				continue;
			if((token = strtok_r(NULL, w_space, &pos)) == NULL)
				continue;
			debug(99, 9) ("read_anyhost: token third '%s'\n", token);

			/* this line don't include 'A' */
			if(strcmp(token, "A") != 0){
				continue;
			}
			//host = get_old_host2(domain);
			host = get_old_host(domain);
			debug(99, 9) ("read_anyhost: host %p\n", host);
			
			if(host != NULL) {
				offline_domain_t *host2 = xcalloc(1, sizeof(offline_domain_t));
				memcpy(host2,host,sizeof(offline_domain_t));
				host2->domain = xstrdup(domain);
				host2->passby = 1;
				if(ip_down_mark == 0)
					host2->all_ip_down = 0;

				host2->next = offline_domains2;
				offline_domains2 = host2;
			}
			else {
				/* add a new domain to the list */
				/* first found ip down */
				host = xcalloc(1, sizeof(offline_domain_t));
				host->down_num = 0;
				host->domain = xstrdup(domain);
				host->passby = 1;
				if(ip_down_mark == 0)
					host->all_ip_down = 0;
				else
					host->all_ip_down = 1;
				/* insert new domain */
				host->next = offline_domains2;
				offline_domains2 = host;
			}
		}
		fclose(anyhost);
		offline_domain_t *head = NULL;
		offline_domain_t **h = &offline_domains2;
		offline_domain_t *t = NULL;
		/* delete some domains which are not contain in anyhost */
		while(*h) {
			debug(99, 9) ("read_anyhost: cur %p\n",  *h);
			if((*h)->passby == 0) {
				t = *h;
				*h = t->next;
				safe_free(t->domain);
				safe_free(t);
			}
			else
				h = &(*h)->next;
		}
		for(head = offline_domains2; head; head = head->next) {
			head->passby = 0;
			if(head->all_ip_down){
				head->down_num++;

				//printf("I am here. Line=%d domain=%s down_num=%d\n", __LINE__, head->domain, head->down_num);
				if(head->down_num >= autodetect_count) {
					head->updown = 1;
					head->down_num = 0;
				}
			}
			else {
				//head->down_num++;
				head->down_num = 0;
				head->updown = 0; //ip up
			}
			head->all_ip_down = 1;
		}
		/* count  auto-detect number */
		print_offline_domains2();
		debug(99, 9) ("read_anyhost: detect successfully, %d\n", auto_detect_num);
		++auto_detect_num;
		if(auto_detect_num == autodetect_count)
			auto_detect_num = 0;
		offline_domains2_updated = 1;
sleep3:
		cc_sleep(autodetect_time);

	}
}


static offline_domain_t *get_domain(const char *host)
{
	if(offline_domains2_updated)
	{
		debug(99, 2)("offline_domains updated by thread!\n");
		//free offline_domains
		offline_domain_t **h = &offline_domains;
		offline_domain_t *t = NULL;
		/* delete some domains which are not contain in anyhost */
		while(*h) {
			t = *h;
			*h = t->next;
			safe_free(t->domain);
			safe_free(t);
		}
		//set 2 to 1
		offline_domains = offline_domains2;
		offline_domains2 = NULL;
		//set not updated
		offline_domains2_updated = 0;
	}
	if(offline_domains == NULL)
		return NULL;
	offline_domain_t *head;
	offline_domain_t *retv = NULL;
	int max_domain = 0;
	for(head = offline_domains; head; head = head->next)
	{
		if(!fnmatch(head->domain, host, FNM_NOESCAPE|FNM_PERIOD)) 
		{
			if(strlen(head->domain) > max_domain) 
			{
				retv = head;
				max_domain = strlen(head->domain);
			}
		}
	}
	if(retv == NULL) {
		retv = get_old_host("wild");
		if( retv == NULL)
			return NULL;
	}
	debug(99, 5) ("get_domain: return %p, %s\n", retv, retv->domain);
	return retv;
}


/* 0: ip_up; 1: ip_down */
static int get_domain_stat1(const char *host)
{
	offline_domain_t *domain = get_domain(host);
	if(domain)
	{
		debug(99, 10) ("get_domain_stat1: matched %s,%d\n", host,domain->updown);
		return domain->updown;
	}
	else
		debug(99, 6) ("get_domain_stat1: not match %s\n", host);
	return 0;
}
/****************************************************************************/



/* callback: free the data */
static int free_mod_config(void *data)
{
	mod_config *cfg = (mod_config *)data;
	if (cfg)
	{
		if(strLen(cfg->original_domain))
			stringClean(&cfg->original_domain);
		if(strLen(cfg->replace_domain))
			stringClean(&cfg->replace_domain);
		memPoolFree(mod_config_pool, cfg);
		cfg = NULL;
	}
	return 0;
}


int free_req_flags(void *data)
{
	assert(data);
	req_flags *f = (req_flags *)data;
	if(strLen(f->redirect_domain))
		stringClean(&f->redirect_domain);
	memPoolFree(req_flags_pool, f);
	f = NULL;
	return 0;
}

void check_offline_default(clientHttpRequest *http, req_flags *reqFlags, mod_config *cfg)
{
	assert(reqFlags);
	request_t *req = http->request;
	/* auatodetect_onoff off or ip down*/
	if ((cfg->autodetect_onoff==0) || get_domain_stat1(req->host)==1)
		reqFlags->type = OFF_LINE_DEFAULT;
	
}


void check_offline_pure(clientHttpRequest *http, req_flags *reqFlags, mod_config *cfg)
{
	assert(reqFlags);
	request_t *req = http->request;
	/* auatodetect_onoff off or ip down*/
	if ((cfg->autodetect_onoff==0) || get_domain_stat1(req->host)==1)
		reqFlags->type = OFF_LINE_PURE;
	
}


void check_offline_redirect(clientHttpRequest *http, req_flags *reqFlags, mod_config *cfg)
{
	assert(reqFlags);

	request_t *req = http->request;
	if ((cfg->autodetect_onoff==0) || get_domain_stat1(req->host)==1)
	{
		if(!matchDomainName(req->host, strBuf(cfg->original_domain))) 
		{
			reqFlags->redirect_domain = stringDup(&cfg->replace_domain);
			reqFlags->type = OFF_LINE_REDIRECT;	
			debug(99, 6) ("check_offline_redirect: redirect domain '%s'\n", strBuf(reqFlags->redirect_domain));
		}
		else
			reqFlags->redirect_domain = StringNull;
	}
}


/* -1: error; 0: handle finish */
int is_offline(clientHttpRequest *http)
{
	//FIXME: Is it http->conn->fd by which I can get mod param?

	int fd = http->conn->fd;	
	Array * modparam = cc_get_mod_param_array(fd, mod);
	int paramcount = cc_get_mod_param_count(fd, mod);
	int i = 0;
	mod_config* cfg = NULL;	
	
	assert(modparam);
	
	req_flags *reqFlags = req_flags_pool_alloc();
	cc_register_mod_private_data(REQUEST_PRIVATE_DATA, http, reqFlags, free_req_flags, mod);

	/* begin to modify request flags */

	//offline is advanced of async

	for(i=0; i<paramcount; i++)
	{
		cfg = (mod_config*)((cc_mod_param*)modparam->items[i])->param;
		
		assert(cfg);

		switch(cfg->type)
		{
			case OFF_LINE_ASYNC:
				reqFlags->type = reqFlags->type ? reqFlags->type : OFF_LINE_ASYNC;
				if(reqFlags->type == OFF_LINE_ASYNC)
					if(checkLocal(http) == 1)
						reqFlags->type = OFF_LINE_ASYNC_REFRESH;
				break;
			case OFF_LINE_DEFAULT:
				check_offline_default(http, reqFlags, cfg);
				break;
			case OFF_LINE_PURE:
				check_offline_pure(http, reqFlags, cfg);
				break;
			case OFF_LINE_REDIRECT:
				check_offline_redirect(http, reqFlags, cfg);
				break;
			default:
				break;
		}

		if(reqFlags->type > OFF_LINE_NONE && reqFlags->type < OFF_LINE_ASYNC)
			break;
	}
	debug(99, 2) ("(mod_offline) -> is_offline: offline type=%d'\n", reqFlags->type);
	return 0;
}
static void start_read_anyhost_thread()
{
	if(pthread_create(&read_anyhost_ptid,NULL,read_anyhost,NULL) != 0)
	{
		debug(99,0)("start_read_anyhost_thread failed, all offline functions will be disabled!\n");
	}
	pthread_detach(read_anyhost_ptid);
}

static void check_threads()
{
	eventAdd("check_threads", check_threads, NULL, 15.0, 0);
	if(num_threads == 0)
	{
		debug(99,1)("mod_offline no threads reading anyhost, starting some\n");
		start_read_anyhost_thread();
	}
	else if(num_threads > 1)
	{
		debug(99,1)("mod_offline too much threads reading anyhost, killing some\n");
		thread_should_exit = 1;
	}
}
/*
 * AsyncLogInit
 */
static void AsyncLogInit(void)
{
#ifdef CC_MULTI_CORE
	if(opt_squid_id <= 0)
	{
		asynclog = logfileOpen(ASYNC_LOG_PATH, 0, 1);
	}
	else
	{
		int len = strlen(ASYNC_LOG_PATH) + 5;//5= \0 + . + 3 bit of id
		char *async_log_path = xcalloc(1, len * sizeof(char));
		snprintf(async_log_path, len - 1, ASYNC_LOG_PATH ".%d", opt_squid_id);
		asynclog = logfileOpen(async_log_path, 0, 1);
		safe_free(async_log_path);
	}
#else
	asynclog = logfileOpen(ASYNC_LOG_PATH, 0, 1);
#endif
}

/////////////////////// hook functions ////////////////

/**
  * System init , register read_anyhost
  */

static int func_sys_init()
{
	if(1 == have_autodetecton)
	{
		debug(99, 1) ("(mod_offline) -> func_sys_init: read_anyhost have_autodetecton=%d'\n", have_autodetecton);
		//start_read_anyhost_thread();
	}

	if(0 == have_opened_async_log && 1 == need_async_log)
	{
		AsyncLogInit();
		have_opened_async_log = 1;
	}
	
	//check_threads();
	return 0;
}


/**
  * parse config line to mod_config
  * return 0 if parse ok, -1 if error
  */
static int func_sys_parse_param(char *cfg_line)
{

	//Check is mod_offline
	char *token = NULL;
	if ((token = strtok(cfg_line, w_space)) == NULL)
		return -1;
	else if(strcmp(token,"mod_offline"))
		return -1;
	
	mod_config *cfg = mod_config_pool_alloc();
	
	while ((token = strtok(NULL, w_space)))
	{
		if (!strcasecmp(token, "auto_detect_on"))
		{
			have_autodetecton = 1;
			cfg->autodetect_onoff = 1;
		}
		else if(!strcasecmp(token, "auto_detect_off"))
			cfg->autodetect_onoff = 0;
		else if(!strcasecmp(token, "pure"))
			cfg->type = OFF_LINE_PURE;
		else if(!strcasecmp(token, "default"))
			cfg->type = OFF_LINE_DEFAULT;
		else if(!strcasecmp(token, "async"))
		{
			cfg->type = OFF_LINE_ASYNC;
			if((token = strtok(NULL, w_space)))
				if(strcasecmp(token, "deny"))
				{
					cfg->time_gap = atoi(token);
					need_async_log = 1;
				}
			break;
		}
		else if(!strcasecmp(token, "redirect"))
		{
			cfg->type = OFF_LINE_REDIRECT;
			if ((token = strtok(NULL, w_space)) == NULL)
			{
				debug(99, 0) ("%s line %d: %s\n", cfg_filename, config_lineno, config_input_line);
				debug(99, 0) ("mod_offline: missing original domain\n");
				goto err;
			}
			else
				stringInit(&cfg->original_domain, token);

			if ((token = strtok(NULL, w_space))==NULL)
			{
				debug(99, 0) ("%s line %d: %s\n", cfg_filename, config_lineno, config_input_line);
				debug(99, 0) ("mod_offline: missing replace domain\n");
				goto err;
			}
			else
				stringInit(&cfg->replace_domain, token);
		}
		//Get autodetect_time, which is optional
		else if (!strcmp(token,"autodetect_time"))
		{
			if((token = strtok(NULL, w_space)) == NULL)
			{
				debug(99, 0) ("mod_offline: missing value of autodetect_time\n");
				goto err;
			}
			else
				cfg->autodetect_time = atoi(token);
		}
		else if (!strcmp(token, "autodetect_count"))
		{
			if ((token = strtok(NULL, w_space)) == NULL)
			{//try to find value
				debug(99, 0) ("mod_offline: missing value of autodetect_count\n");
				goto err;
			}
			else
				cfg->autodetect_count = atoi(token);
		}
	}

	if (cfg->autodetect_time ==0)
	{
		cfg->autodetect_time = AUTODETECT_TIME_DEFAULT;
	}

	if (cfg->autodetect_count ==0)
	{
		cfg->autodetect_count = AUTODETECT_COUNT_DEFAULT;
	}
	
	cc_register_mod_param(mod, cfg, free_mod_config);
	
	return 0;

err:
	if (cfg)
	{
		if(strLen(cfg->original_domain))
			stringClean(&cfg->original_domain);
		if(strLen(cfg->replace_domain))
			stringClean(&cfg->replace_domain);
		memPoolFree(mod_config_pool, cfg);
		cfg = NULL;
	}
	return -1;
}

static int cleanup(cc_module *mod)
{
	if(num_threads)
	{
		debug(99,1)("mod_offline,found threads, killing all\n");
		thread_should_exit = 1;
		pthread_kill(read_anyhost_ptid,9);
	}
	eventDelete(check_threads,NULL);

	if(1 == have_autodetecton)
	{	
		free_offline_domain(offline_domains);
		offline_domains = NULL;
	}
	offline_domains = NULL;
	have_autodetecton = 0;
	have_opened_async_log = 0;
	need_async_log = 0;

	AsyncLogFlush();
	AsyncLogClose();
	if(NULL != mod_config_pool)
	{
		memPoolDestroy(mod_config_pool);
	}
	if(NULL != req_flags_pool)
	{
		memPoolDestroy(req_flags_pool);
	}
	return 0;
}


static HttpHeaderEntry* httpRequestHeaderFindEntry(const HttpHeader * hdr, http_hdr_type id)
{
	HttpHeaderPos pos = HttpHeaderInitPos;
	HttpHeaderEntry *e;
	assert(hdr);

	/* check mask first */
	if (!CBIT_TEST(hdr->mask, id))
		return NULL;
	/* find it , return it */
	while ((e = httpHeaderGetEntry(hdr, &pos))) 
	{       
		if (e->id == id)
			return e;
	}      
	return NULL;      
}


static int clientCheckNoCacheStart(clientHttpRequest * http)
{
	debug(99,10)("entering is_offline check\n");
	is_offline(http);
	//here is to check weather a compress quest or not
	request_t* request = http->request;	
	req_flags *reqFlags = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);
	reqFlags->is_compress = 0;

	if(httpRequestHeaderFindEntry(&request->header, HDR_ACCEPT_ENCODING))
		reqFlags->is_compress = 1;

	return 0;
}


/**
  * Code outside the hook point should return my return value if my return value >0
  * Code outside the hook point should continue if my return value == 0
  */
static int clientCheckHitStart(clientHttpRequest * http)
{
	req_flags *reqFlags = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);
	if (OFF_LINE_NONE == reqFlags->type)
	{
		debug(99, 2) ("(mod_offline) -> clientCheckHitStart: isn't offline mode log_type=%d'\n", LOG_TAG_NONE);
		return 0;
	}
	if (OFF_LINE_ASYNC == reqFlags->type)
	{
		return 0;
	}
	if (OFF_LINE_DEFAULT == reqFlags->type)
	{
		debug(99, 2) ("(mod_offline) -> clientCheckHitStart: offline mode:default\n");
		return 0;
	}
	StoreEntry *e = http->entry;
	request_t *r = http->request;
	
	if (NULL == e) 
	{
		/* this object isn't in the cache */
		debug(99, 3) ("clientProcessRequest2: storeGet() MISS\n");
		/* send 404 to client add by xt */
		if (OFF_LINE_PURE == reqFlags->type) 
		{
			//ErrorState *err = errorCon(ERR_FTP_NOT_FOUND, HTTP_NOT_FOUND,r);
			http->log_type = LOG_TCP_HIT;
			//err->url = xstrdup(http->uri);
			//http->entry = clientCreateStoreEntry(http, r->method, null_request_flags);
			//errorAppendEntry(http->entry, err);
			debug(99, 10) ("clientCheckHitStart case 1\n");
			return LOG_TCP_HIT;
		}
		
		/* redirect to offline replace_domain */
		if (OFF_LINE_REDIRECT == reqFlags->type) 
		{
			http->redirect.status = HTTP_MOVED_TEMPORARILY;
			char urlbuf[MAX_URL];
			snprintf(urlbuf, MAX_URL, "%s://%s%s%s:%d%s", \
					ProtocolStr[r->protocol],
					r->login,
					*r->login ? "@" : null_string,
					strBuf(reqFlags->redirect_domain),
					r->port,
					strBuf(r->urlpath));
			http->redirect.location = xstrdup(urlbuf);
			debug(99, 3) ("clientProcessRequest2: offline_redirect HIT, redirect url '%s'\n", http->redirect.location);
		}

		return LOG_TCP_MISS;
	}
	
	if (OFF_LINE_NONE != reqFlags->type) 
	{
		debug(99, 3)("clientProcessRequest2: offline_pure HIT\n");
		return LOG_TCP_HIT;
	}

	return LOG_TAG_NONE;
}


/**
  * Code outside the hook point should continue if this function returns 0
  * Code outside the hook point should return if this function returns 1
  */
static int clientCheckHitOver(clientHttpRequest * http)
{
	req_flags *reqFlags = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);
	request_t *r = http->request;
	/* add by xiaotao */
	if (OFF_LINE_REDIRECT == reqFlags->type && http->log_type == LOG_TCP_MISS)
   	{
		request_t *r = http->request;
		http->entry = clientCreateStoreEntry(http, r->method, null_request_flags);
		http->al.http.code = 302;
		storeReleaseRequest(http->entry);
		storeBuffer(http->entry);
		HttpReply *rep = httpReplyCreate();
		httpRedirectReply(rep, http->redirect.status, http->redirect.location);
		httpReplySwapOut(rep, http->entry);
		storeComplete(http->entry);
		return 1;
	}
	if(!http->entry  && reqFlags->type == OFF_LINE_PURE)
	{
		ErrorState *err = errorCon(ERR_FTP_NOT_FOUND, HTTP_NOT_FOUND,r);
		err->url = xstrdup(http->uri);
		http->entry = clientCreateStoreEntry(http, r->method, null_request_flags);
		errorAppendEntry(http->entry, err);
		debug(99, 10) ("clientCheckHitStart case 1\n");
		return 1;

	}
	return 0;
}


/**
  * Code outside the hook point should check the return value 
  * if >=0, means a log_type is determined
  * if <0, means no log_type determined
  */

static int checkStale(clientHttpRequest *http, int stale)
{

	req_flags *reqFlags = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);
	debug(99, 5)("(mod_offline) ->  **********reqFlags->type=%d\n", reqFlags->type);

	if (!stale)
	{
		goto async;
	}
	
	assert(reqFlags);
	if (OFF_LINE_NONE != reqFlags->type && reqFlags->type < OFF_LINE_ASYNC)
	{
		http->log_type = LOG_TCP_OFFLINE_HIT;
		return 0;
	}
async:
	if (reqFlags->type == OFF_LINE_ASYNC)
	{
		stale = 0;
		if(AsyncExpireCheck(http))
		{
            reqFlags->time_gap_flag = 1;
            http->log_type = LOG_TCP_STALE_HIT;
            return 0;
        }
        else
            http->log_type = LOG_TCP_HIT;
    }
    else if (reqFlags->type == OFF_LINE_ASYNC_REFRESH)
        stale = 1;
    return stale;
}

void start_thread(void)
{
    debug(99,1)("mod_offline: start_thread before\n");
    check_threads();
    debug(99,1)("mod_offline: start_thread after\n");
}

void end_thread(int unused)
{
    eventDelete(check_threads,NULL);
    if(num_threads)
    {
        debug(99,1)("mod_offline,found threads, killing all start\n");
        
        num_threads = 0;
        int rc = pthread_kill(read_anyhost_ptid, 0);
        if(!rc)
        {
            rc = pthread_kill(read_anyhost_ptid, SIGQUIT);
            if(rc)
            {
                debug(99, 1)("mod_offline, pthread_kill: %s\n", strerror(rc));
            }
            //rc = pthread_join(read_anyhost_ptid, NULL); 
        }
        //thread_should_exit = 1;
        //sleep(5);
        debug(99,1)("mod_offline,found threads, killing all over\n");
    }
    debug(99, 1)("mod_offline after eventDelete\n");
}

// module init 
int mod_register(cc_module *module)
{
	debug(99, 1)("(mod_offline) ->  init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");
	
	//module->hook_func_sys_init = func_sys_init;
	cc_register_hook_handler(HPIDX_hook_func_sys_init,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_init),
			func_sys_init);
	//module->hook_func_sys_parse_param = func_sys_parse_param;
	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
			func_sys_parse_param);

	//module->hook_func_sys_cleanup = cleanup;
	cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_cleanup),
			cleanup);

	//module->hook_func_http_req_read = clientCheckNoCacheStart;
	cc_register_hook_handler(HPIDX_hook_func_http_req_read,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_read),
			clientCheckNoCacheStart);

	//module->hook_func_private_http_req_process2 = clientCheckHitStart;
	cc_register_hook_handler(HPIDX_hook_func_private_http_req_process2,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_http_req_process2),
			clientCheckHitStart);

	//module->hook_func_http_req_process = clientCheckHitOver;
	cc_register_hook_handler(HPIDX_hook_func_http_req_process,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_process),
			clientCheckHitOver);

	//module->hook_func_http_client_cache_hit = checkStale;
	cc_register_hook_handler(HPIDX_hook_func_http_client_cache_hit,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_client_cache_hit),
			checkStale);

	//module->hook_func_http_req_free = asyncLog;
	cc_register_hook_handler(HPIDX_hook_func_http_req_free,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_free),
			asyncLog);

	cc_register_hook_handler(HPIDX_hook_func_module_reconfigure,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_module_reconfigure),
			end_thread);

	cc_register_hook_handler(HPIDX_hook_func_sys_after_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_after_parse_param),
			start_thread);
	//mod = module;
//	mod = (cc_module*)cc_modules.items[module->slot];
	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	offline_domains = NULL;
	offline_domains2 = NULL;
	auto_detect_num = 0;
	return 0;
}

#endif
