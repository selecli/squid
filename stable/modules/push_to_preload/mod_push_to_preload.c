#include "cc_framework_api.h"
#include <stdbool.h>
#ifdef CC_FRAMEWORK
/*
 * mod_push_to_preload [methed=get|head] [action=refresh|preload|refresh_preload] priority=9 nest_level=3 [check_type=md5|sha1|header] preload_address=127.0.0.1:15101 [need_report=yes|no] report_address=221.109.178.111:8080 allow acl
 */
#define MAX_XML_LEN 65535
#define BUFF_LEN 32

static cc_module* mod = NULL;
static MemPool * mod_config_pool = NULL;
static long num = 0;

struct _mod_config {
	char method[BUFF_LEN];
	char action[BUFF_LEN];

	int priority;
	int nest_level;

	char check_type[BUFF_LEN];
	char preload_ip[BUFF_LEN];
	char preload_port[BUFF_LEN];

	char need_report[BUFF_LEN];
	char report_ip[BUFF_LEN];
	char report_port[BUFF_LEN];
};
typedef struct _mod_config mod_config;

static void *mod_config_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == mod_config_pool)
	{
		mod_config_pool = memPoolCreate("mod_push_to_preload config_struct", sizeof(mod_config));
	}
	return obj = memPoolAlloc(mod_config_pool);
}

static int free_callback(void* data)
{
	mod_config *cfg = (mod_config*) data;
	if (cfg) {
        memPoolFree(mod_config_pool,cfg);
        cfg = NULL;
	}
	return 0;
}

//127.0.0.1:15101
static int checkIpPortValid(char *in_addr,char *out_ip,char *out_port)
{
	int a[4] = {0};	
	int port = 0;
	if (sscanf(in_addr, "%d.%d.%d.%d:%d", &a[0], &a[1], &a[2], &a[3], &port) < 5) {
		debug(209, 0)("ERROR:mod_push_to_preload IP and Port format illegal (%s),e.g.: legal [127.0.0.1:8888]\n", in_addr);
		return -1;
	}

	int i;
	for (i = 0; i < 4; i++) {
		if (a[i] > 255 || a[i] < 0) {
			debug(209, 0)("ERROR:mod_push_to_preload IP num illegal (%d) should in [0,255]\n",a[i]);
			return -1;
		}
	}

	if (port > 65535 || port < 0) {
		debug(209, 0)("ERROR:mod_push_to_preload Port num illegal (%d) should in (0,65535)\n",port);
		return -1;
	}
	
	snprintf(out_ip,BUFF_LEN,"%d.%d.%d.%d",a[0], a[1], a[2], a[3]);
	snprintf(out_port,BUFF_LEN,"%d",port);
		
	return 0;	
}

static int func_sys_parse_param(char *cfg_line)
{
	debug(209, 3)("mod_push_to_preload config=[%s]\n", cfg_line);
	if (strstr(cfg_line, "allow") == NULL)
		return -1;

	char *token = NULL, *pos = NULL;

	if ((token = strtok(cfg_line, w_space)) == NULL)
		return -1;
	else if (strncasecmp(token, "mod_push_to_preload", strlen("mod_push_to_preload"))!=0) {
		debug(209, 0)("Error:mod_push_to_preload parse line [%s] error missing \"mod_push_to_preload\"\n",token);
		return -1;
	}

	if ((token = strtok(NULL, w_space)) == NULL) {
		debug(209, 0)("Error:mod_push_to_preload parse line error\n");
		return -1;
	}
		
	if ((strncasecmp(token, "methed=",7) != 0) || ((pos = token + 7) == NULL)) {
		debug(209, 3)("Error:mod_push_to_preload config [methed] error %s\n",token);
		return -1;
	} 

	char method[BUFF_LEN] = {'\0'};
	if (!strcasecmp(pos, "get"))
		strncpy(method,"get",3);
	else if(!strcasecmp(pos, "head"))
		strncpy(method,"head",4);
	else {
		debug(209, 0)("Error:mod_push_to_preload config methed value error not in [get,head]\n");
		return -1;
	}

	if ((token = strtok(NULL, w_space)) == NULL) {
		debug(209, 0)("Error:mod_push_to_preload missing [action]\n");
		return -1;
	}
	
	if ((strncasecmp(token, "action=",7) != 0) || ((pos = token + 7) == NULL)) {
		debug(209, 3)("Error:mod_push_to_preload parse config [action] error\n");
		return -1;
	} 

	char action[BUFF_LEN] = {'\0'};
	if (!strcasecmp(pos, "refresh"))
		strncpy(action,"refresh",7);
	else if (!strcasecmp(pos, "preload"))
		strncpy(action,"preload",7);
	else if (!strcasecmp(pos, "refresh_preload"))
		strncpy(action,"refresh,preload",15);
	else {
		debug(209, 0)("Error:mod_push_to_preload config action value error should in (refresh,preload)\n");
		return -1;
	}

	if ((token = strtok(NULL, w_space)) == NULL) {
		debug(209, 0)("Error:mod_push_to_preload missing [priority]\n");
		return -1;
	}
	
	if ((strncasecmp(token, "priority=",9) != 0) || ((pos = token + 9) == NULL)) {
		debug(209, 3)("Error:mod_push_to_preload parse config [priority] error \n");
		return -1;
	} 

	int priority = 9;
	priority = atoi(pos);
	if (priority <= 0 || priority > 9) {
		debug(209, 2) ("(mod_push_to_preload) -> paser priority=[%d] illegal should in [1,9]\n",priority);
		return -1;
	}

	if ((token = strtok(NULL, w_space)) == NULL) {
		debug(209, 0)("Error:mod_push_to_preload missing [priority]\n");
		return -1;
	}
	
	if ((strncasecmp(token, "nest_level=",11) != 0) || ((pos = token + 11) == NULL)) {
		debug(209, 3)("Error:mod_push_to_preload parse config [nest_level] error\n");
		return -1;
	} 

	int nest_level = 3;
	nest_level = atoi(pos);
	if (nest_level < 0 || nest_level > 3) {
		debug(209, 2) ("(mod_push_to_preload) -> paser nest_leve =[%d] illegal should in [3]\n",nest_level);
		return -1;
	}

	if ((token = strtok(NULL, w_space)) == NULL) {
		debug(209, 0)("Error:mod_push_to_preload missing [check_type]\n");
		return -1;
	}
	
	if ((strncasecmp(token, "check_type=",11) != 0) || ((pos = token + 11) == NULL)) {
		debug(209, 3)("Error:mod_push_to_preload parse config [check_type] error\n");
		return -1;
	} 

	char check_type[BUFF_LEN] = {'\0'};
	int ck_len = 0;
	if (strcasestr(pos, "md5")) {
		strncpy(check_type + ck_len, "md5",3);
		ck_len += 3;
	}

	if (strcasestr(pos, "shal") != NULL) {
		if (ck_len > 0) {
			strncpy(check_type + ck_len,",", 1); 
			ck_len += 1;
		}
		strncpy(check_type + ck_len, "shal",4);
		ck_len += 4;
	}

	if (strcasestr(pos, "header") != NULL) {
		if (ck_len > 0) {
			strncpy(check_type + ck_len,",", 1); 
			ck_len += 1;
		}
		strncpy(check_type + ck_len, "header", 6);
		ck_len += 6; 
	}

	if ((token = strtok(NULL, w_space)) == NULL) {
		debug(209, 0)("Error:mod_push_to_preload missing [check_type]\n");
		return -1;
	}
	
	if ((strncasecmp(token, "preload_address=",16) != 0) || ((pos = token + 16) == NULL)) {
		debug(209, 3)("Error:mod_push_to_preload parse config [preload_address] error\n");
		return -1;
	} 

	char preload_ip[BUFF_LEN] = {'\0'};
	char preload_port[BUFF_LEN] = {'\0'};

	if (checkIpPortValid(pos,preload_ip,preload_port) < 0) {
		debug(209, 2) ("(mod_push_to_preload) -> paser preload_address error\n");
		return -1;
	}

	if ((token = strtok(NULL, w_space)) == NULL) {
		debug(209, 0)("Error:mod_push_to_preload missing (need_report) token=(%s)\n",token);
		return -1;
	}
	
	if ((strncasecmp(token, "need_report=",12) != 0) || ((pos = token + 12) == NULL)) {
		debug(209, 3)("Error:mod_push_to_preload parse config [need_report] error\n");
		return -1;
	}
	
	char need_report[BUFF_LEN]={'\0'};
	if (strcasecmp(pos, "yes") != NULL)
		strncpy(need_report,"yes",3);
	else if (strcasecmp(pos, "no") != NULL)
		strncpy(need_report,"no",2);
	
	if ((token = strtok(NULL, w_space)) == NULL) {
		debug(209, 0)("Error:mod_push_to_preload missing (report_address) token=(%s)\n",token);
		return -1;
	}

	if ((strncasecmp(token, "report_address=",15) != 0) || ((pos = token + 15) == NULL)) {
		debug(209, 3)("Error:mod_push_to_preload parse config report_address error (%s)\n",token);
		return -1;
	}

	char report_ip[32] = {'\0'};
	char report_port[32] = {'\0'};

	if (checkIpPortValid(pos,report_ip,report_port) < 0) {
		debug(209, 2) ("(mod_push_to_preload) -> paser preload_address error\n");
		return -1;
	}

	mod_config *cfg = mod_config_pool_alloc();
	strncpy(cfg->method, method, strlen(method));
	strncpy(cfg->action, action, strlen(action));
	
	cfg->priority = priority;
	cfg->nest_level = nest_level;

	strncpy(cfg->check_type, check_type, strlen(check_type));
	strncpy(cfg->preload_ip, preload_ip, strlen(preload_ip));
	strncpy(cfg->preload_port, preload_port, strlen(preload_port));

	strncpy(cfg->need_report, need_report, strlen(need_report));
	strncpy(cfg->report_ip, report_ip, strlen(report_ip));
	strncpy(cfg->report_port, report_port, strlen(report_port));

	debug(209, 2)("(mod_push_to_preload) -> paramter method=[%s],action=[%s],priority=[%d],nest_level=[%d],check_type=[%s],preload_ip=[%s],preload_port=[%s],need_report=[%s],report_ip=[%s],report_port=[%s]\n",cfg->method,cfg->action,cfg->priority,cfg->nest_level,cfg->check_type,cfg->preload_ip,cfg->preload_port,cfg->need_report,cfg->report_ip,cfg->report_port);

	cc_register_mod_param(mod, cfg, free_callback);
	return 0;
}

static int m3u8_connect(char *preload_ip,char *preload_port)
{
	assert(preload_ip);
	assert(preload_port);

	int port = atoi(preload_port);
	int fd;
	struct hostent      *site;
	struct sockaddr_in  myaddress;
	if( (site = gethostbyname(preload_ip)) == NULL )
		return -1;

	if( (fd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		return -1;
	}
	memset(&myaddress, 0, sizeof(struct sockaddr_in));
	myaddress.sin_family = AF_INET;
	myaddress.sin_port   = htons(port);
	memcpy(&myaddress.sin_addr, site->h_addr_list[0], site->h_length);

	int ret = -1;
	ret = connect(fd, (struct sockaddr *)&myaddress, sizeof(struct sockaddr)); 
	if (ret != 0) {
		if ( errno != EINPROGRESS ) {
            close(fd);
			return -1;
		}
		else{
			return -2;
		}
	}

	return fd;
}

static int safe_write(int fd, const char *buffer,int length)
{
	int bytes_left = 0;
	int written_bytes = 0;
	const char *ptr=NULL;

	ptr=buffer;
	bytes_left = length;

	if (length == 0)
		return 0;

	int retry_times = 0;

	while (bytes_left > 0) {
		written_bytes = write(fd, ptr, bytes_left);

		if (written_bytes == 0)
			return length - bytes_left;

		if(written_bytes < 0) {
			if(errno == EINTR) {
				usleep(100000);
				continue;
			} else {
				retry_times++;
				if (retry_times < 5) {
					usleep(100000);
					continue;
				} else {
					debug(209,2)("mod_push_to_preload: fwrite error(%d) for safe_write\n", errno);
					return -1;
				}
			}
		}
		bytes_left -= written_bytes;
		ptr += written_bytes;
	}
	return length;
}

static void sendUrlToM3u8Helper(mod_config *cfg, const char *url)
{
	assert(cfg);
    int fd = -1, w_len = -1;

    if ((fd = m3u8_connect(cfg->preload_ip, cfg->preload_port)) < 0) {
        debug(209,1)("Warning:mod_push_to_preload-> connected failed, please check preloader \n");
        return;
    }

	char xml[MAX_XML_LEN]={'\0'};
	int len = 0;
	len += snprintf(xml + len, MAX_XML_LEN, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<method name=\"%s\" sessionid=\"LFC%5ld%10ld%06ld\">\n",cfg->method,(long)getpid(),(long)current_time.tv_sec,(++num)%100000);
	len += snprintf(xml + len, MAX_XML_LEN, "<action>%s</action>\n<priority>%d</priority>\n<nest_level>%d<nest_level>\n",cfg->action,cfg->priority,cfg->nest_level);
	len += snprintf(xml + len, MAX_XML_LEN, "<check_type>%s</check_type>\n<preload_address>%s:%s</preload_address>\n<report_address need=\"%s\">%s:%s</report_address>\n",cfg->check_type,cfg->preload_ip,cfg->preload_port,cfg->need_report,cfg->report_ip,cfg->report_port);

	len += snprintf(xml + len, MAX_XML_LEN, "<url_list>\n<url id=\"1\">%s</url>\n</url_list>\n</method>\n",url);

    if ((w_len = safe_write(fd, xml, len) != len )) {
        debug(209,1)("Warning:mod_push_to_preload: size which write to preload is wrong\n");
        close(fd);
    }
    debug(209,3)("mod_push_to_preload sendUrlToM3u8Helper -->xml=[%s], fd = %d\n",xml,fd);
    close(fd);
}

static int hook_cleanup(cc_module *module)
{
	debug(209, 1)("(mod_push_to_preload) ->	hook_cleanup:\n");
	if(NULL != mod_config_pool)
		memPoolDestroy(mod_config_pool);
	return 0;
}

static int private_clientRedirectDone(request_t** new_request,char* result, clientHttpRequest* http)
{
	debug(209, 1)("mod_push_to_preload -> private_clientRedirectDone redirect.status = %d, url=%s\n",http->redirect.status,http->uri);
	mod_config *cfg = cc_get_mod_param(http->conn->fd, mod);
    assert(cfg);
	sendUrlToM3u8Helper(cfg,http->uri);
	return 0;
}

int mod_register(cc_module *module)
{
	debug(209, 1)("(mod_push_to_preload) ->  init: init module\n");
	strcpy(module->version, "7.0.R.16488.i686");

	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
			func_sys_parse_param);

	cc_register_hook_handler(HPIDX_hook_func_private_clientRedirectDone,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_clientRedirectDone),
			private_clientRedirectDone);

    cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
            module->slot, 
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_cleanup),
            hook_cleanup);

	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	return 0;
}
#endif

