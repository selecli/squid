
#include "cc_framework_api.h"

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>

#include "mod_cpis_dns_modify.h"
#ifdef CC_FRAMEWORK


typedef struct
{
	char *host;
	u_short port;
	struct sockaddr_in S;
	CNCB *callback;
	void *data;
	struct in_addr in_addr;
	int fd;
	int tries;
	int addrcount;
	int connstart;
} ConnectStateData;


static cc_module* mod = NULL;

struct mod_conf_param {
	char* cidr_dir;	
	LIST_MEM *list_header;
	int type;
};

static MEM_VALUE*
getValueList(char *line, MEM_VALUE * header)
{
	char *p, *q;
	MEM_VALUE * value_mem;

	q = line;
	skip_whitespace(q);
	do{
		if(!(p=strchr(q, ';')))
			break;
		/* malloc a value struct */
		value_mem = (MEM_VALUE *)xmalloc(sizeof(MEM_VALUE));
		if(value_mem == NULL)
		{
			printf("Error Malloc Mem_value\n");
			break;
		}
		memset(value_mem, 0, sizeof(MEM_VALUE));
		memcpy(value_mem->value, q, p-q);

		/* adjust the value list */
		if(header== NULL)
		{
			header = value_mem;
			init_list_head(&header->list);
		} else
		{
			list_add_tail(&value_mem->list, 
						&header->list);
		}
		q = ++p;
	}while(*q != '\0');
	return header;

}

/* destroy the list */
static MEM_VALUE *
destroyList(MEM_VALUE * header)
{	
	MEM_VALUE  *tmp_list;
	struct list_head * tmp;

	if(header == NULL)
	{
		debug(170, 3)("(mod_cpis_dns_modify) -> Destory Is NULL\n");
		return NULL;
	}
	tmp_list = header;
	/* ajust the cur_pbliblist */
	do{
		tmp_list = header;
		tmp = &header->list;
		if(tmp == tmp->next)
			header = NULL;
		else {
			tmp = tmp->next;
			header = list_entry(tmp, MEM_VALUE, list);
			list_del(&tmp_list->list);
		}
		debug(170, 3)("(mod_cpis_dns_modify) -> destroy mem:%s.\n", tmp_list->value);
		safe_free(tmp_list);
	}while(header != NULL);
	return NULL;
}

/* real parse the configure */
static int
parseConfig(FILE *fd, char * end_flag, int type, LIST_MEM * list_header)
{
	char buf[1024], *tmp;
	LIST_MEM *list_mem;

	list_mem = list_header;
	if(type == CIDR_FLAG)
	{
		while(1)
		{
			/* the end of the file */
			if(feof(fd))
				return 0;
			fgets(buf, 1024, fd);
			tmp = buf;
			skip_whitespace(tmp);
			/* empty line or with the start of '#' */
			if(strlen(tmp)==0 || *tmp=='\n'
						|| *tmp=='#')
				continue;
			if(strstr(tmp, end_flag))
				break;

			list_mem->cidrlist = getValueList(tmp, list_mem->cidrlist);
		}

	}
	else if (type == PBLIP_FLAG)
	{
		while(1)
		{
			/* the end of the file */
			if(feof(fd))
				return 0;
			fgets(buf, 1024, fd);
			tmp = buf;
			skip_whitespace(tmp);
			/* empty line or with the start of '#' */
			if(strlen(tmp)==0 || *tmp=='\n'
						|| *tmp=='#')
				continue;
			if(strstr(tmp, end_flag))
				break;
			list_mem->pbliblist = getValueList(tmp, list_mem->pbliblist);
		}
		/* initialize the current pbliblist */
		list_mem->cur_pbliblist = list_mem->pbliblist;
	}

	return 0;
}

static int
print_config(LIST_MEM * list_header)
{
	LIST_MEM  *tmp_list;
	MEM_VALUE *tmp_value;
	tmp_list = list_header;
	struct list_head * tmp;

	if(tmp_list->cidrlist == NULL)
		debug(170, 3)("NULL cidrlist\n");
	else{
		debug(170, 3)("\tCIDR LIST:\n");
		tmp_value = tmp_list->cidrlist;
		do{
			debug(170, 3)("\t\t%s\n", tmp_value->value);
			tmp = &tmp_value->list;
			tmp = tmp->next;
			tmp_value = list_entry(tmp, MEM_VALUE, list);
		}while(tmp_value!=tmp_list->cidrlist);
	}

	if(tmp_list->pbliblist == NULL)
		debug(170, 3)("NULL pbliblist\n");
	else
	{
		debug(170, 3)("\tPBLIB LIST:\n");
		tmp_value = tmp_list->pbliblist;
		do{
			debug(170, 3)("\t\t%s\n", tmp_value->value);
			tmp = &tmp_value->list;
			tmp = tmp->next;
			tmp_value = list_entry(tmp, MEM_VALUE, list);
		}while(tmp_value!=tmp_list->pbliblist);
	}

	return 0;
}


/* parse the configure file */
static int 
init_configure(char * path, struct mod_conf_param *data)
{
	FILE * fd;
	char cidrlist_start[32], cidrlist_end[32];
	char pbliplist_start[32], pbliplist_end[32];
	char buf[1024], *tmp;

	/* get the cidrlist key */
	strcpy(cidrlist_start, "[");
	strcat(cidrlist_start, CIDRLIST);
	strcat(cidrlist_start, " start]");

	strcpy(cidrlist_end, "[");
	strcat(cidrlist_end, CIDRLIST);
	strcat(cidrlist_end, " end]");

	/* get the pbliplist key */
	strcpy(pbliplist_start, "[");
	strcat(pbliplist_start, PBLIPLIST);
	strcat(pbliplist_start, " start]");

	strcpy(pbliplist_end, "[");
	strcat(pbliplist_end, PBLIPLIST);
	strcat(pbliplist_end, " end]");

	if ((fd = fopen(path, "r")) == NULL)
	{
		debug(170, 3)("(mod_cpis_dns_modify) -> open configure file fault:%s\n", path);
		if ((fd = fopen(CONFIGEFILE, "r")) == NULL)
		{
			debug(170, 3)("(mod_cpis_dns_modify) -> open configure file fault:%s\n", CONFIGEFILE);
			return -1;
		} else 
		{
			debug(170, 3)("(mod_cpis_dns_modify) -> use default config file:%s\n", CONFIGEFILE);
		}
	} 

	data->list_header->cidrlist = destroyList(data->list_header->cidrlist);
	data->list_header->pbliblist= destroyList(data->list_header->pbliblist);

	while(1)
	{
		/* test the end of the file */
		if (feof(fd))
			return 0;

		memset(buf, 0, sizeof(buf));
		fgets(buf, 1024, fd);
		tmp = buf;
		skip_whitespace(tmp);
		if(strlen(tmp)==0 || *tmp=='\n')
			continue;
		/* parse cidrlist */
		else if(strstr(tmp, cidrlist_start))
		{
			parseConfig(fd, cidrlist_end, CIDR_FLAG, data->list_header);
		}
		/* parse cpliplist */
		else if(strstr(tmp,  pbliplist_start))
		{
			parseConfig(fd, pbliplist_end, PBLIP_FLAG, data->list_header);
		}
		
	}

	return 0;
}

/***
  * return value:
  * 1: ip fit cidr
  * 0: ip donot fit cidr
  */
static int
checkIP_CIDR(char *ip, char *cidr)
{
	char cidrIP[64], cidrN[10], *q,*p;
	struct in_addr ip_number;
	unsigned int hostip, mask, network;
	int num;

	p = cidr;
	skip_whitespace(p);
	/* parse cidr ip */
	if(!(q = strchr(cidr, '/')))
		return 0;
	memcpy(cidrIP, p, q-p);
	cidrIP[q-p] = '\0';

	/* bad ip */
	if(cli_check_keyword_single_ip(cidrIP))
	{
		debug(170,6)("(mod_cpis_dns_modify) ->Ignore Invalid CIDR:%s\n", cidr);
		return 0;
	}
	
	p = ++q;
	/* parse cidr N*/
	if(!(q = strchr(cidr, '\0')))
		return 0;
	memcpy(cidrN, p, q-p);
	cidrN[q-p] = '\0';

	/* check for the cidr number */
	for(num=0; num<q-p; num++)
	{
		if(!isdigit(cidrN[num]))
		{
			debug(170,6)("(mod_cpis_dns_modify) ->Ignore Invalid CIDR:%s\n", cidr);
			return 0;
		}
	}
	num = atoi(cidrN);

	/* invalid cidr number */
	if(num<0 || num >32)
		return 0;
	/* xxx.xxx.xxx.xxx/0 */
	if(num == 0)
		return 1;

	/* get the CIDR network */
	inet_pton(AF_INET, cidrIP,  &hostip);
	ip_number.s_addr=htonl(hostip);

	mask= 0xffffffff<<(32-num);
	network = mask&ip_number.s_addr;

	/* get the ip Number */
	inet_pton(AF_INET, ip,  &hostip);
	ip_number.s_addr=htonl(hostip);
	hostip = mask&ip_number.s_addr;

	return hostip==network;
}

/**
  * return value:
  * success: 1 
  * fail: 0
  */
static int
checkIPfitCIDR(char *IP, LIST_MEM *list_header)
{
	LIST_MEM  *tmp_list;
	MEM_VALUE *tmp_value;
	struct list_head * tmp;
	tmp_list = list_header;
	int flag = 0;

	if(tmp_list->cidrlist == NULL)
	{
		return flag;
	}
	else{
		tmp_value = tmp_list->cidrlist;
		flag = 0;
		do{
			/* check the ip and cidr */
			if(checkIP_CIDR(IP, tmp_value->value))
			{
				flag = 1;
				debug(170, 3)("(mod_cpis_dns_modify) ->  ip:%s fit CIDR:%s \n", 
					IP, tmp_value->value);	
				break;
			} 
			/* go to the next value */
			else{
				tmp = &tmp_value->list;
				tmp = tmp->next;
				tmp_value = list_entry(tmp, MEM_VALUE, list);
			}
		}while(tmp_value!=tmp_list->cidrlist);
	}

	return flag;
}

/** 
 * 1. get a IP from pblibIP list
 * 2. ajust the cur_pbliblist point
 */
static char *
getpblibIP(LIST_MEM *list_header)
{
	LIST_MEM  *tmp_list;
	struct list_head * tmp;
	tmp_list = list_header;
	char *returnValue = NULL;

	/* ajust the cur_pbliblist */
	if(tmp_list->cur_pbliblist)
	{
		returnValue = tmp_list->cur_pbliblist->value;
		tmp = &tmp_list->cur_pbliblist->list;
		tmp = tmp->next;
		tmp_list->cur_pbliblist= list_entry(tmp, MEM_VALUE, list);
	}
	return returnValue;
}

static int free_callback(void* param)
{
	struct mod_conf_param* data = (struct mod_conf_param*) param;
	debug(170, 3)("(mod_cpis_dns_modify) -> free_callback:----:)\n");

	data->list_header->cidrlist = destroyList(data->list_header->cidrlist);
	data->list_header->pbliblist= destroyList(data->list_header->pbliblist);
	if(data){
		safe_free(data->list_header);
		safe_free(data);
	}

	return 0;
}

static int hook_init(char *path, struct mod_conf_param *data)
{

	debug(170, 3)("(mod_cpis_dns_modify) -> hook_init:----:)\n");
	init_configure(path, data);
	print_config(data->list_header);
	return 0;
}


// confige line example:
// mod_cpis_dns_modify [cidr_dir] [always] allow/deny acl
// mod_cpis_dns_modify [cidr_dir] [always] on/off
static int func_sys_parse_param(char *cfg_line)
{
	char* token = NULL;

	struct mod_conf_param  *data = NULL;
	data = xmalloc(sizeof(struct mod_conf_param));
	memset(data, 0, sizeof(struct mod_conf_param));
	data->list_header = xmalloc(sizeof(LIST_MEM));
	memset(data->list_header, 0, sizeof(LIST_MEM));

	//mod_cpis_dns_modify
	if ((token = strtok(cfg_line, w_space)) != NULL)
	{
		if(strcmp(token, "mod_cpis_dns_modify"))
			goto err;	
	}
	else
	{
		debug(170, 3)("(mod_cpis_dns_modify) ->  parse line error\n");
		goto err;	
	}

	//cidr file name
	if (NULL == (token = strtok(NULL, w_space)))
	{
		debug(170, 3)("(mod_cpis_dns_modify) ->  parse line data does not existed\n");
		goto err;
	}

	/* case the always paramter */
	if(!strcmp(token, "always"))
	{
		debug(170, 6)("parse the always patamter:%s\n", cfg_line);
		data->type = 100;
	}

	if (!(strcmp(token, "allow") && strcmp(token, "deny") &&  strcmp(token, "on") 
		&& strcmp(token, "off")&& strcmp(token, "always")))
	{
		debug(170, 3)("(mod_cpis_dns_modify) ->  missing cidr file name \n");
		hook_init(CONFIGEFILE, data);

	} else
	{
		hook_init(token, data);

		/* parse the always parameter */
		if (NULL != (token = strtok(NULL, w_space)) &&
			data->type == 0)
		{
			/* case the always parameter */
			if(!strcmp(token, "always"))
			{
				debug(170, 6)("parse the always patamter:%s\n", cfg_line);
				data->type = 100;
			}
		}
	}

	cc_register_mod_param(mod, data, free_callback);

	return 0;		
err:
	return -1;
}


static void modify_dns_ip(ipcache_addrs * ia, void *data)
{
	debug(170, 3)("(mod_cpis_dns_modify) ->  modify_dns_ip \n");
	ConnectStateData *cs = data;
	struct in_addr *ip = &cs->in_addr;
	char *ip_char, *validIP;
	FwdState *fwdState = cs->data;

	ip_char = inet_ntoa(*ip);

	struct mod_conf_param  *cfg = cc_get_mod_param(fwdState->client_fd, mod);

	/* set debug for the NULL munipulation */
	if(cfg==NULL || cfg->list_header==NULL)
	{
		debug(170, 3)("(mod_cpis_dns_modify) -> CFG NULL\n");
		return;
	}

	/* case the always parameter */
	if(cfg->type == 100)
	{
		debug(170, 3)("(mod_cpis_dns_modify) -> recieve the always convert ip parameter.\n");
		do
		{
			validIP = getpblibIP(cfg->list_header);
			/* check the valid ip */
			if(cli_check_keyword_single_ip(validIP))
			{
				debug(170,6)("(mod_cpis_dns_modify) ->Ignore Invalid Ip:%s\n", validIP);
			} else break;
		} while(1);
		inet_aton(validIP, ip);
		return;
	}

	if(!cli_check_keyword_single_ip(ip_char))
	{
		/* check wether fit the CIDR */
		if(!checkIPfitCIDR(ip_char, cfg->list_header))
		{
			do
			{
				validIP = getpblibIP(cfg->list_header);
				/* check the valid ip */
				if(cli_check_keyword_single_ip(validIP))
				{
					debug(170,6)("(mod_cpis_dns_modify) ->Ignore Invalid Ip:%s\n", validIP);
 				} else break;
			} while(1);
			inet_aton(validIP, ip);
		}
	} else
	{
		debug(170, 3)("(mod_cpis_dns_modify) ->  ip check failed:%s\n", ip_char);	
	}
}

static int hook_cleanup(cc_module *module)
{
	debug(170, 3)("(mod_cpis_dns_modify) ->  hook_cleanup \n");

	return 0;
}

/* module init */
int mod_register(cc_module *module)
{
	debug(170, 1)("(mod_cpis_dns_modify) ->  init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");
	
	//module->hook_func_http_req_send = func_http_req_send; //modify server side func

    cc_register_hook_handler(HPIDX_hook_func_private_comm_connect_polling_serverIP,
            module->slot, 
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_comm_connect_polling_serverIP),
            modify_dns_ip);
    
    /*
    cc_register_hook_handler(HPIDX_hook_func_private_comm_connect_dns_handle,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_comm_connect_dns_handle),
			modify_dns_ip);
    */
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

	mod = module;
	return 0;
}

#endif
