#include "squid.h"
#include <string.h>
#include "fhr_base.h"

static cc_module *mod = NULL;
static int g_iConfNum = 0;
fhr_main_date_t *main_date;

#define MAX_PATH_SIZE 256
#define MAX_CONF_NUM 20
ST_ZONE_CFG zone_cfg[MAX_CONF_NUM];

struct mod_config
{
//	char	*ipset_path;
//	char	*territory_path;
//	char	*redirect_path;
	char	ipset_path[MAX_PATH_SIZE];
	char	territory_path[MAX_PATH_SIZE];
	char	redirect_path[MAX_PATH_SIZE];
};

static int free_cfg(void *data)
{
	struct mod_config* cfg = (struct mod_config*)data;
	if(cfg)
	{
	//	safe_free(cfg->ipset_path);
	//	safe_free(cfg->territory_path);
	//	safe_free(cfg->redirect_path);
		safe_free(cfg);
		cfg = NULL;
	}
	return 0;
}
static int free_zone_cfg(void *data)
{
	PRIVATE_DATA *cfg = (PRIVATE_DATA *)data;
	if (cfg)
	{
		safe_free(cfg);
		cfg = NULL;
	}
	return 0;
}

static int getCityFormattedToArray(char *str , ST_ZONE_CFG* pstC)
{   
	int in=0;
	char *p[200];
	char *buf=str;
	char *outer_ptr=NULL;
	char *inner_ptr=NULL;
	while((p[in]=strtok_r(buf,"|",&outer_ptr))!=NULL) {
		buf=p[in];
		while((p[in]=strtok_r(buf,":",&inner_ptr))!=NULL) {
			in++;   
			buf=NULL;
		}       
		buf=NULL; 
	}
	int j = 0;
	for(;j<in;j++)
	{
		if(!(j%2))
		{       
            if(pstC->item[j/2].territory)
			    strcpy(pstC->item[j/2].territory,p[j]);
			debug(164,9)("pstC->item[%d].territory = %s\n",j/2,pstC->item[j/2].territory);     
		}       
		else    
		{       
            if(pstC->item[j/2].dstdomain)
			    strcpy(pstC->item[j/2].dstdomain,p[j]);
			debug(164,9)("pstC->item[%d].dstdomain = %s\n",j/2,pstC->item[j/2].dstdomain);     
		}       
	}
	pstC->municipal_city_num = in/2; 
	return 0;
}
/*
 0 ---: OK
 -1 --: ERR
 1 ---: ERR
*/
static int getZoneConfInfo2(char *line,ST_ZONE_CFG *pstC)
{
	char buf[20480];
	strcpy(buf,line);
	char* str = strtok(buf, " \t"); 
	if(NULL == str) 
	{
		debug(164,0)("get domain failure in conf=[%.*s]\n", (int)strlen(line)-1, line);
		return -1;
	}
	strcpy(pstC->dstdomain,str);

	str = strtok(NULL, " \t"); 
	if(NULL == str) 
	{
		debug(164,0)("get cult failure in conf=[%.*s]\n", (int)strlen(line)-1, line);
		return -1;
	}
	
	str = strtok(NULL, " \t"); 
	if(NULL == str) 
	{
		debug(164,0)("get operate failure in conf=[%.*s]\n", (int)strlen(line)-1, line);
		return -1;
	}
	if(0 == strcasecmp(str, "zone"))
	{
		debug(164,5)("get line[%s] ok\n",line);
	}
	else
	{
		debug(164,0)("this line[%s] contains no zone,go next\n",line);
		return 1;
	}

	str = strtok(NULL, " \t\r\n");
	if(NULL == str)
	{
		debug(164,0)("Get use 302 redirect flag error,please check it\n");
		return -1;
	}
	pstC->use302 = atoi(str);

	str = strtok(NULL, " \t\r\n");
	if(NULL == str)
	{
		debug(164,0)("Get sub domain flag error,please check it\n");
		return -1;
	}
	pstC->subdomain = atoi(str);

	str = strtok(NULL, " \t\r\n");
	if(NULL == str)
	{
		debug(164,0)("Get service_needed error,please check it\n");
		return -1;
	}
	if(-1 == getCityFormattedToArray(str,pstC))
	{
		debug(164,0)("Get service needed city error,please check it\n");
		return -1;
	}

	//for(;i<pstC->municipal_city_num && pstC->municipal_city[i][0]!=0;i++)
	//  DEBUG("out:pstC->municipal_city[%d]=%s\n",i,pstC->municipal_city[i]);

	str = strtok(NULL, " \t\r\n");
	if(NULL == str)
	{
		debug(164,0)("Get default_needed error,please check it\n");
		return -1;
	}
	strcpy(pstC->default_city,str);
	debug(164,5)("default city [%s]\n",pstC->default_city);

	/*add by cwg start*/
	str = strtok(NULL, " \t\r\n");
	if (str)
	{
		if (0 == strncmp(str, "no_default", strlen("no_default")))
			pstC->no_default = 1;
	}
	debug(164,5)("no_default  [%d]\n",pstC->no_default);
	/*add by cwg end*/

	return 0;
}
/*
 0 -- ok
 else -- err
*/
static int  getZoneConfInfo(char *filename)
{

	debug(164,1)("in getZoneConfInfo filename %s\n",filename);
    g_iConfNum = 0;
//	memset(pstC, 0, MAX_CONF_NUM*sizeof(ST_ZONE_CFG));

	FILE* fp = fopen(filename, "r"); int ret = -1;
	if(NULL == fp)
	{
		debug(164,0)("cannot open file=[%s]\n", filename);
		return -1;
	}
	char line[20480];
	while(!feof(fp))
	{
		if(NULL == fgets(line,20480, fp))
		{
			continue;
		}
		if(('#'==line[0]) ||('\r'==line[0]) ||('\n'==line[0]) || '$' == line[0])
		{
			continue;
		}
		ret = strlen(line);
		if('\n' != line[ret-1])
		{
			debug(164,0)("file=[%s] have line long\n", filename);
			fclose(fp);
			return -1;
		}

		ret = getZoneConfInfo2(line,&zone_cfg[g_iConfNum]);
		if(ret ==  -1)
		{
			continue;
		}
		else if(ret == 1)	
		{// 1
			continue;
		}
		else
		{//0
			g_iConfNum++;
		}

		if(g_iConfNum > MAX_CONF_NUM)
			break;

	}
	fclose(fp);
	
	return ret;	
}

static int analysisZoneCfg(char *filename)
{
	int ret = 0;

	ret = getZoneConfInfo(filename);
	
	return ret;

}


static int hook_init()
{
	struct mod_config *cfg = cc_get_global_mod_param(mod);
	int ret = 0;
	assert(cfg);
	debug(164, 5) ("mod_zone hook_init: paramter ipset = %s territory= %s,redirect.conf =%s\n", cfg->ipset_path, cfg->territory_path,cfg->redirect_path);	
	
	main_date = NEW_ZERO(fhr_main_date_t);
	main_date->territory = load_territory_db(cfg->territory_path);
	if (main_date->territory == NULL)
	{
		debug(164,0)("load %s eroor.please check!\n",cfg->territory_path);
		return -1;
	}
	main_date->ipset = load_ipset_db(cfg->ipset_path) ;
	if (main_date->ipset == NULL)
	{
		debug(164,0)("load %s eroor.please check!\n",cfg->ipset_path);
		return -1;
	}

	make_fhr_ip_find_tree(main_date->ipset, main_date->territory);

	ret = analysisZoneCfg(cfg->redirect_path);
	if(ret == -1)
		return -1;

	return 0;
}

static int func_sys_parse_param(char *cfg_line)
{

	if( strstr(cfg_line, "mod_zone") == NULL )
		return 0;

	char *token = NULL;

	if ((token = strtok(cfg_line, w_space)) == NULL)
		return 0;


	struct mod_config *cfg = xmalloc(sizeof(struct mod_config));
	memset(cfg, 0, sizeof(struct mod_config));
	
	token = strtok(NULL, w_space);
	//cfg->ipset_path = xmalloc(MAX_PATH_SIZE);
	memset(cfg->ipset_path, '\0', MAX_PATH_SIZE);
	strncpy(cfg->ipset_path, token, strlen(token));

	token = strtok(NULL, w_space);
	//cfg->territory_path = xmalloc(MAX_PATH_SIZE);
	memset(cfg->territory_path, '\0', MAX_PATH_SIZE);
	strncpy(cfg->territory_path, token, strlen(token));

	token = strtok(NULL, w_space);
	//cfg->redirect_path= xmalloc(MAX_PATH_SIZE);
	memset(cfg->redirect_path, '\0', MAX_PATH_SIZE);
	strncpy(cfg->redirect_path, token, strlen(token));

	
	debug(164, 5) ("mod_zone: paramter ipset = %s territory= %s redirect_path = %s \n", cfg->ipset_path, cfg->territory_path,cfg->redirect_path);	

	cc_register_mod_param(mod, cfg, free_cfg);

	analysisZoneCfg(cfg->redirect_path);

	return 0;
}

static int getIpAndTerritoryInfo(request_t* request,fhr_main_date_t *main_date,ST_ZONE_CFG* pstC)
{
	unsigned int ip_seek = 0;
	const _cidr_t *cidr = NULL;
	_territory_t *territory;
	const _cidr_t *p_c;
	GSList * p_t;
	int have_found_or_not = 0;
	int i = 0;
	int sequence = 0;


	ip_str2socket(inet_ntoa(request->client_addr), &ip_seek); 

	/*
	struct mod_config *cfg = cc_get_global_mod_param(mod);
	if(cfg == NULL)
	{
		debug(164,3)("find no coresponed configure in redirect.conf\n");
		return 0;
	}
	*/

	char *header = "X-FC-ZONEIP";
	String sb = StringNull;
	char *str = NULL;
	sb = httpHeaderGetByName(&request->header, header);
    str = (char *)strBuf(sb);
	if( str != NULL )
	{
		ip_str2socket(str, &ip_seek); 
		debug(164,3)("mod_zone: use test IP [%s]!!!\n",str);
	}

	debug(164,3)("mod_zone: actual ip [%s]test IP [%s]!!!\n",inet_ntoa(request->client_addr),str);
	stringClean(&sb);

	cidr = trie_tree_lookup(main_date->ipset->trie_tree, ip_seek);

	if(cidr == NULL)//cidr
	{
		debug(164,3)("get no ip in ipset,use default\n");
		return -1;
	}

	for(p_c = cidr; p_c != NULL; p_c = p_c->father)
	{
		char c_cidr[sizeof("XXX.XXX.XXX.XXX/XX")];
		cidr_int_to_char(p_c, c_cidr);

		for (p_t = p_c->t_list; p_t != NULL; p_t = p_t->next)
		{
			territory = p_t->data;
			debug(164,5)("territory:'%s'\tweight:'%d'\n", territory->name, territory->weight);
			for(i=0;i<pstC->municipal_city_num;i++)
			{
				if(!strcasecmp(territory->name,pstC->item[i].territory))
				{
					debug(164,3)("found!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
					have_found_or_not = 1;
				    	sequence = i;
					pstC->item[i].weight = territory->weight;
					break;
				}
				else
				{
				}
			}
			if(1 == have_found_or_not)
				break;
		}
		if(1 == have_found_or_not)
			break;
	}

	if(1 == have_found_or_not)
	{
		pstC->sequence = sequence;
		debug(164,3)("pstC->territoty = %s ; pstC->item[%d].dstdomain = %s\n", pstC->item[sequence].territory,sequence,pstC->item[sequence].dstdomain);
		return 1;	
	}
	else
	{
		debug(164,3)("no find use default %s\n", pstC->default_city);
		return 0;
	}
}

static ST_ZONE_CFG* findZoneCfg(ST_ZONE_CFG *zone_cfg,char *host)
{
	int i = 0;
	ST_ZONE_CFG *tmp = NULL;
	for(;i<g_iConfNum;i++)
	{
		if(!strcmp(zone_cfg[i].dstdomain,host))
		{
			tmp = &zone_cfg[i];
			break;
		}
	
	}
	return tmp;

}
static int getDomain(char* url, ZONE_URL* pstURL)
{
	char* str = strstr(url, "://");
	if(NULL == str)
	{
		return -1;
	}
	if(str - url >= PROTOCOL_LENGTH)
	{
		return -1;
	}
	memset(pstURL->protocol, '\0', PROTOCOL_LENGTH);
	memcpy(pstURL->protocol, url, str - url);
	
	str += 3; // strlen('://')  == 3
	char* str2 = strchr(str, '/');
	if(NULL == str2)
	{
		return -1;
	}
	if(str2 - str >= MAX_DOMAIN_SIZE)
	{
		return -1;
	}
	memset(pstURL->domain, '\0', MAX_DOMAIN_SIZE);
	memcpy(pstURL->domain, str, str2 - str);

	//pstURL->other = xstrdup(str2);
	pstURL->other = str2;
	return 0;
}

static inline void modify_request(clientHttpRequest * http)
{
	debug(164, 3)("modify_request: start, uri=[%s]\n", http->uri);
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
		if(old_request->cc_request_private_data)
		{
			new_request->cc_request_private_data = old_request->cc_request_private_data;
			old_request->cc_request_private_data = NULL;
		}
		requestUnlink(old_request);
		http->request = requestLink(new_request);
	}
}


static int actualRedirect(request_t** new_request,char* result, clientHttpRequest* http)
{
	debug(164,5)("actual Redirect begin\n");
	debug(164,5)("http->uri %s \n",http->uri);	
	int fd = http->conn->fd;

	ZONE_URL st_url; 
	int ret = 0;
	char domain_substitude[MAX_DOMAIN_SIZE];
	int sequence;
	char tmp[MAX_URL_LENGTH];
	
	ret = getDomain(http->uri,&st_url);
	if(ret < 0)
		return 0;
	debug(164,5)("st_url.other = %s\n",st_url.other);

	PRIVATE_DATA* private_data = cc_get_mod_private_data(FDE_PRIVATE_DATA, &fd,mod);
	if(!private_data)
		return 0;
	else
	{
		memset(domain_substitude, '\0', MAX_DOMAIN_SIZE);
		debug(164,3)("private->find_or_not [%d],host =%s\n",private_data->find_or_not,private_data->pointer->dstdomain);
		sequence = private_data->pointer->sequence;
		if(private_data->find_or_not)
		{
			strcpy(domain_substitude,private_data->pointer->item[sequence].dstdomain);
			
		}
		else
		{
			strcpy(domain_substitude,private_data->pointer->default_city);
		}

		//tmp = (char *)xmalloc(sizeof(char)*MAX_URL_LENGTH);
		memset(tmp, '\0', MAX_URL_LENGTH);
		if(0 == private_data->pointer->subdomain)
		{
			strcpy(tmp,"http://");
			strcat(tmp,domain_substitude);
			strcat(tmp,st_url.other);
		}
		else if(1 == private_data->pointer->subdomain)
		{// 此时配置的不是替换域名 而是替换的URL
			strcpy(tmp,"http://");
			strcat(tmp,domain_substitude);
		}
		else if (2 == private_data->pointer->subdomain)
		{
			if(1 == private_data->find_or_not)
			{
				strcpy(tmp,"http://");
				strcat(tmp,domain_substitude);
			}
			else
			{
				strcpy(tmp,"http://");
				strcat(tmp,domain_substitude);
				strcat(tmp,st_url.other);
			}
		}
		else
		{
			if(0 == private_data->find_or_not)
			{
				strcpy(tmp,"http://");
				strcat(tmp,domain_substitude);
			}
			else
			{
				strcpy(tmp,"http://");
				strcat(tmp,domain_substitude);
				strcat(tmp,st_url.other);
			}
		}
		debug(164,5)("private_data->pointer->subdomain %d, private_data->find_or_not [%d]\n",private_data->pointer->subdomain,private_data->find_or_not);

		debug(164, 5) ("private_data->pointer->no_default = [%d]\n", private_data->pointer->no_default);
		/*if it has no found and no_default set, do nothing*/
		if ((1 == private_data->pointer->no_default) && (0 == private_data->find_or_not))
		{}
		else if((1 == private_data->pointer->use302) || (( 2 == private_data->pointer->use302)&&(1 == private_data->find_or_not) )||(( 3 == private_data->pointer->use302)&&(0 == private_data->find_or_not)) )
		{
			http->redirect.status = 302;
			if(http->redirect.location)
				safe_free(http->redirect.location);
			http->redirect.location = xstrdup(tmp);
		//	safe_free(tmp);
		}
		else
		{
			/*
			if(*new_request){
				if((*new_request)->link_count > 0){
					requestUnlink(*new_request);
				}else{
					requestDestroy(*new_request);
				}
			}
			new_request = urlParse(http->request->method,tmp);
			*/

			if(http->uri)
				safe_free(http->uri);
			http->uri = xstrdup(tmp);
			modify_request(http);
		//	safe_free(tmp);
		}
				
		debug(164,3)("uri = %s location [%d] %s\n",http->uri,http->redirect.status,http->redirect.location);
	}
	return 0;
}
static int addZoneHeader(clientHttpRequest* http)
{
	assert(http->request);
	request_t* request = http->request;
	int ret;
	int fd = http->conn->fd;
	debug(164, 5) ("addZoneHeader: indirect_client_addr=[%s] client_addr=[%s]\nhost = [%s]\n",inet_ntoa(request->indirect_client_addr),inet_ntoa(request->client_addr),request->host);

	ST_ZONE_CFG* cfg = findZoneCfg(zone_cfg,request->host);
	if(cfg == NULL)
	{
		debug(164,3)("find no coresponed configure in redirect.conf\n");
		return 0;
	}
	else
	{
		debug(164,3)("find coresponed configure in redirect.conf\n");
		PRIVATE_DATA *info = (PRIVATE_DATA*)xmalloc(1*sizeof(PRIVATE_DATA));
		assert(cfg);
		info->pointer = cfg;	
	
		ret = getIpAndTerritoryInfo(request,main_date,cfg);
		if(-1 == ret )
				info->find_or_not = 0;
		else 
		{
			if(ret ==0)
				info->find_or_not = 0;
			else
				info->find_or_not = 1;

		}
		cc_register_mod_private_data(FDE_PRIVATE_DATA, &fd, info,free_zone_cfg , mod);

	}
	return 0;
}
int mod_register(cc_module *module)
{
	debug(164, 1)("(mod_zone) -> mod_register:\n");

	strcpy(module->version, "7.0.R.16488.i686");

	cc_register_hook_handler(HPIDX_hook_func_sys_init,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_init),
			hook_init);
	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
			func_sys_parse_param);
	cc_register_hook_handler(HPIDX_hook_func_http_before_redirect,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_before_redirect),
			addZoneHeader);
	cc_register_hook_handler(HPIDX_hook_func_private_clientRedirectDone,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module,hook_func_private_clientRedirectDone),
			actualRedirect);

	mod = module;

	return 0;
}

