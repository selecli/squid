#include "squid.h"
#include <string.h>
#include "IP2Location.h"

#define BUFFSIZE 1024
#define MAX_PATH_SIZE 512

struct mod_config
{
    HttpHeaderEntry e;
    IP2Location *IP2LocationObj;
	char ip_zone_database_path[MAX_PATH_SIZE];
};

static cc_module *mod = NULL;
static MemPool * mod_config_pool = NULL;

static void * mod_config_pool_alloc(void)
{
    void * obj = NULL;
    if (NULL == mod_config_pool)
    {   
        mod_config_pool = memPoolCreate("mod_zone_ip2location config_struct", sizeof(struct mod_config));
    }   
    return obj = memPoolAlloc(mod_config_pool);
}

static int free_cfg(void *data)
{
	struct mod_config* cfg = (struct mod_config*)data;
	if (cfg)
	{
        if(strLen(cfg->e.name))
            stringClean(&cfg->e.name);
        if(strLen(cfg->e.value))
            stringClean(&cfg->e.value);
        IP2Location_close(cfg->IP2LocationObj);	
        memPoolFree(mod_config_pool, cfg);
        cfg = NULL;
	}
	return 0;
}

static int func_sys_parse_param(char *cfg_line)
{
    char *token = NULL;
    if ((token = strtok(cfg_line, w_space)) == NULL)
        goto err;

    struct mod_config *cfg = mod_config_pool_alloc();
    memset(cfg, 0, sizeof(struct mod_config));

    if (NULL == (token = strtok(NULL, w_space)))
        goto err;
    stringInit(&cfg->e.name, token);

    int len = strLen(cfg->e.name);
    int i = httpHeaderIdByNameDef(strBuf(cfg->e.name), len);    
    if(-1 == i)
        cfg->e.id = HDR_OTHER;
    else
        cfg->e.id = i;

    if (NULL == (token = strtok(NULL, w_space)))
        goto err;
    strncpy(cfg->ip_zone_database_path, token, MAX_PATH_SIZE);

	//IP2Location_close(cfg->IP2LocationObj);	
	cfg->IP2LocationObj = IP2Location_open(cfg->ip_zone_database_path);
	if(cfg->IP2LocationObj == NULL)
    {
		debug(151, 5) ("mod_zone_ip2location: IP2Location_open %s faild!!!\n",cfg->ip_zone_database_path);
        goto err;
	}

    debug(151, 5) ("mod_zone_ip2location: IP2Location_open %s success!!!\n",cfg->ip_zone_database_path);
    cc_register_mod_param(mod, cfg, free_cfg);
    return 0;

err:
    free_cfg(cfg);
    return -1;
}

static int get_client_area_use_ip(clientHttpRequest *http)
{
    debug(151,4)("mod_zone_ip2location: client ip = %s\n", inet_ntoa(http->request->client_addr));
    int fd = http->conn->fd;
    struct mod_config *param = (struct mod_config *)cc_get_mod_param(fd, mod);
    assert(param);
    char buf[BUFFSIZE] = "\0";
    IP2LocationRecord *record = IP2Location_get_all(param->IP2LocationObj, inet_ntoa(http->request->client_addr));
    if (record == NULL)
    {
        snprintf(buf,BUFFSIZE, "country_code = not found!,city = not found!");
        stringClean(&param->e.value);
        stringInit(&param->e.value, buf);
        return 0;
    }
    //snprintf(buf, BUFFSIZE, "georegion=na,country_code=%s,city=%s,lat=na,long=na,timezone=na,continent=na,throughput=na,bw=na,network=na,asnum=na,location_id=na,network_type=na",record->country_short, record->city);
    snprintf(buf, BUFFSIZE, "georegion=na,country_code=%s,city=na,lat=na,long=na,timezone=na,continent=na,throughput=na,bw=na,network=na,asnum=na,location_id=na",record->country_short);
    stringClean(&param->e.value);
    stringInit(&param->e.value, buf);
    debug(151,6)("mod_zone_ip2location: %s\n",buf);
    IP2Location_free_record(record);
    return 1;
}

static void add_http_req_header(HttpStateData* data,HttpHeader* hdr_out)
{
    debug(151,4)("mod_zone_ip2location: Add HttpHeader @ fc->origin_server\n");
    int fd = data->fd;
    struct mod_config *param = (struct mod_config *)cc_get_mod_param(fd, mod);
    assert(param);
    if(!strLen(param->e.name) || !strLen(param->e.value))
        return;
    httpHeaderAddEntry(hdr_out, httpHeaderEntryClone(&param->e));	
}

int mod_register(cc_module *module)
{
	debug(151, 1)("mod_zone_ip2location -> mod_register\n");
	strcpy(module->version, "7.0.R.16488.i686");

	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
			func_sys_parse_param);
	cc_register_hook_handler(HPIDX_hook_func_get_client_area_use_ip,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_get_client_area_use_ip),
			get_client_area_use_ip);
	cc_register_hook_handler(HPIDX_hook_func_http_req_send_exact,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_send_exact),
			add_http_req_header);

    if(reconfigure_in_thread)
        mod = (cc_module*)cc_modules.items[module->slot];
    else
        mod = module;
	return 0;
}
