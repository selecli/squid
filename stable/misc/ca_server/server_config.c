#include "server_header.h"

server_config config =
{
	1,
	21109,

	{},
	{},
	80,
	{},
	{},
	{},
	{},
	false
};

//get ip from dns or config file
int ca_host_type = 0;

char req_list[MAX_HEADER_NUM][MAX_HEADER_LEN];
char rep_list[MAX_HEADER_NUM][MAX_HEADER_LEN];

char content_type_list[MAX_HEADER_NUM][MAX_HEADER_LEN];

unsigned int rep_status_list[128];
int rep_status_num = 0;


static bool parse_onoff(bool * onoff){
	assert(onoff);
	char *token;
	token = strtok(NULL, white_space);
	if(!token) {
		return false;
	}

	*onoff = !strncmp(token, "on",2);
	return true;
}

static bool parse_int(unsigned int * val) {
	assert(val);

	char *token ;
	token = strtok(NULL, white_space);
	if(!token) {
		return false;
	}

	*val = atoi(token);
	return true;
}

static bool parse_short(short* val) {
	assert(val);
	unsigned int nval;
	if(parse_int(&nval)){
		*val = (short)nval;
		return true;
	}
	else
		return false;
}

/*
static bool parse_ip(char* ip) {
	assert(ip);
	char* token;
	token = strtok(NULL, white_space);
	if(token == NULL){
		return false;
	}

	if(strlen(token) >= 16)
		return false;

	strcpy(ip, token);
	return true;
}
*/

static bool parse_string(char* str) {
	assert(str);
	char* token;
	token = strtok(NULL, white_space);
	if(token == NULL){
		return false;
	}

	if(strlen(token) == 0)
		return false;

	strcpy(str, token);
	return true;
}

static void dump_header_list(char list[][MAX_HEADER_LEN])
{
	int i = 0;
	assert(list);

	while(*list[i])
		cclog(3, "%s", list[i++]);

	return;
}

static void dump_rep_status_list()
{
	int i = 0;

	for(i = 0; i < rep_status_num; i++)
		cclog(3, "%d", rep_status_list[i]);

	return;
}
static void dump_config()
{
	cclog(3,
                        "\nlog_level %d\n"
                        "port %d\n"
			"CA host: %s\n"
			"CA ip file: %s\n"
			"CA port: %d\n"
			"req_list_file: %s\n"
			"rep_list_file: %s\n"
			"content_type_list_file: %s\n"

			"hash_request_url: %s\n",

			config.log_level,
			config.port,
			(config.ca_host?config.ca_host:null_string),
			(config.ca_ip_file?config.ca_ip_file:null_string),
			config.ca_port,
			config.req_list_file,
			config.rep_list_file,
			config.content_type_list_file,
			(config.hash_request_url?"Yes":"no")
		);

	cclog(3, "req_list_file:");
	dump_header_list(req_list);

	cclog(3, "rep_list_file: ");
	dump_header_list(rep_list);

	cclog(3, "content_type_list_file: ");
	dump_header_list(content_type_list);

	cclog(3, "rep_status_list_file: ");
	dump_rep_status_list();

	cclog(3, "ca_host_ip: ");
	dump_ca_host_ip();
	
}

//  parse header list (req_list: will send to CA; rep_list: will not send to CA)
//  0: success !0: failed
int header_list_parse(const char *file)
{
	FILE *fp;
	int i = 0;
	char (*list)[MAX_HEADER_LEN];

	assert(file);	

	if((fp = fopen(file, "r")) == NULL)
	{
		cclog(0, "can not open file: %s", file);
		goto failed;
	}
	if(!strcmp(file, config.req_list_file))
		list = req_list;
	else if(!strcmp(file, config.rep_list_file))
		list = rep_list;
	else if(!strcmp(file, config.content_type_list_file))
		list = content_type_list;
	else
	{
		cclog(1, "unknown file[%s] type !", file);
			goto failed;
	}
	memset(list, 0, (MAX_HEADER_NUM * MAX_HEADER_LEN));

	while(fgets(list[i], MAX_HEADER_LEN - 1, fp))
	{
		if(list[i][strlen(list[i]) - 1] == '\n')
		{
			if(list[i][strlen(list[i]) - 2] == '\r')
				list[i][strlen(list[i]) - 2] = '\0';
			else
				list[i][strlen(list[i]) - 1] = '\0';
		}

		if(strlen(list[i]) == 0)
			continue;

		if(i < MAX_HEADER_NUM - 1)
			i++;
		else
		{
			cclog(1, "headers list full, out now!");
			break;
		}
	}

	if(fp)
                fclose(fp);

	return OK;

failed:
	if(fp)
		fclose(fp);
	return ERR;
}	

//parse status list file : file: filename, num: status number
int rep_status_list_parse(const char * file)
{
	FILE *fp;
	char *token ;
	char line[128];

        assert(file);

        if((fp = fopen(file, "r")) == NULL)
        {
                cclog(0, "can not open file: %s", file);
                goto failed;
        }

	memset(rep_status_list, 0, sizeof(rep_status_list));
	rep_status_num = 0;

	memset(line, 0, sizeof(line));

	while(fgets(line, sizeof(line) - 1, fp))
	{
		if(line[strlen(line) - 1] == '\n')
		{
			if(line[strlen(line) - 2] == '\r')
				line[strlen(line) - 2] = '\0';
			else
				line[strlen(line) - 1] = '\0';
		}

		if(strlen(line) == 0)
			continue;

		token = strtok(line, white_space);
		if(token == NULL) 
			continue;

		if(isdigit(token[0]))
			rep_status_list[rep_status_num++] = atoi(token);
	}

	if(fp)
                fclose(fp);
	return OK;

failed:
        if(fp)
                fclose(fp);
        return ERR;
}


//parse config file and store it into global variable config
//return 0 on success, or -1 error occurred
int config_parse(const char *confile) {
	char buf[1024];
	int line = 0;
	int retval = OK;
	FILE *cf = NULL;

	if((confile == NULL) || (strlen(confile) == 0)) {
		return ERR_CONFIG_FILE;
	}

	if( (cf = fopen(confile, "r")) == NULL ) {
		return ERR_CONFIG_FILE;
	}

	while( fgets(buf, sizeof(buf) - 1, cf) ) {
		line++;
		if( strlen(buf) == 0 )
			continue;

		if(buf[0] == ' ' || buf[0] == '#')
			continue;

		char *token = strtok(buf, white_space);

		if( !token )
			continue;

		bool ret = true;
		if( !strcasecmp(token, "log_level") ) {
			ret = parse_int( &config.log_level );
			if(config.log_level > 9)
				config.log_level = 9;
		}
		else if(!strcasecmp(token, "port")) {
			ret = parse_short((short*)&config.port);
			if(config.port < 1025)
				config.port = 1025;
		}
		else if( !strcasecmp(token, "ca_host") ) {
			ret = parse_string(config.ca_host);
			if(ret)
				ca_host_type = HOST_IP;
		}
		else if( !strcasecmp(token, "ca_ip_file") ) {
			ret = parse_string(config.ca_ip_file);
			if(ret)
				ca_host_type = FILE_IP;
		}
		else if(!strcasecmp(token, "ca_port")) {
			ret = parse_short((short*)&config.ca_port);
			if(!ret)
				config.port = 80;
		}
		else if( !strcasecmp(token, "req_list_file") ) {
			ret = parse_string(config.req_list_file);
			if(ret)
				ret = (header_list_parse(config.req_list_file) == OK) ? 1 : 0;
		}
		else if( !strcasecmp(token, "rep_list_file") ) {
			ret = parse_string(config.rep_list_file);
			if(ret)
				ret = (header_list_parse(config.rep_list_file) == OK) ? 1 : 0;
		}
		else if( !strcasecmp(token, "content_type_list_file") ) {
			ret = parse_string(config.content_type_list_file);
			if(ret)
				ret = (header_list_parse(config.content_type_list_file) == OK) ? 1 : 0;
		}
		else if( !strcasecmp(token, "rep_status_list_file") ) {
                        ret = parse_string(config.rep_status_list_file);
                        if(ret)
                                ret = (rep_status_list_parse(config.rep_status_list_file) == OK) ? 1 : 0;
                }
		else if( !strcasecmp(token, "hash_request_url") ) {
			parse_onoff(&config.hash_request_url);
		}
		else{
			cclog(3, "unknown token at line: %d, token:%s\n", line, token);
			ret = false;
		}

		if(!ret){
			cclog(2, "parsed failed at line: %d, token:%s\n", line, token);
			retval = ERR_CONFIG_FILE;
			break;
		}
	}

	fclose(cf);

	if(!(*config.req_list_file) || !(*config.rep_list_file))
	{
		cclog(0, "can not find header file, exit ....");
		abort();
	}

	//init ca host ip
	if(ca_host_type == HOST_IP)
	{
		init_ca_host_ip(config.ca_host, ca_host_type);
	}
	else
	{
		init_ca_host_ip(config.ca_ip_file, ca_host_type);
	}

	dump_config();
	return retval;
}

