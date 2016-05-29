#include "refresh_header.h"
#define white_space " \t\r\n"

rf_config config = {
	2,	//log level
	21108,	//port
	5,	//db_commit_inteval
	2000,	//db_commit_count
	5,	//url_wait_timeout
	1800,	//dir_wait_timeout -- for dir refresh thread timeout
	180,	//dir_wait_timeout2 -- for dir refresh wait timeout from all request has been add to the write list 
	0,	//whether hash db: 1:yes 0:no
	0,	//whether enable dir purge refresh
	"",					//report ip
	"",	//test_url
	"@@@",	//compress_delimiter
	"no",	//purge_success_del
	"200M",
	"4K",
	true,	//is_dir_refresh
	{},	//compress_formats
	0,	//compress_formats_count
	NULL,
	0
};

void dump_config(){
	cclog(3,
			"\nlog_level %d\n"
			"port %d\n"
			"report_ip %s\n"
//			"db_commit_interval %d\n"
//			"db_commit_count %d\n"
			"url_wait_timeout %d\n"
			"dir_wait_timeout %d\n"
			"dir_wait_timeout2 %d\n"
			"compress_delimiter %s\n"
//			"purge_success_del %s\n"
//			"bdb_cache_size %s\n"
//			"bdb_page_size %s\n"
			"is_dir_refresh %d\n"
			"if_hash_db %d\n"
			"compress_formats_count %d\n"
			,
			config.log_level,
			config.port,
			config.report_ip,
//			config.db_commit_interval,
//			config.db_commit_count,
			config.url_wait_timeout,
			config.dir_wait_timeout,
			config.dir_wait_timeout2,
			config.compress_delimiter,
//			config.purge_success_del,
//			config.bdb_cache_size,
//			config.bdb_page_size,
			config.is_dir_refresh,
			config.if_hash_db,
			config.compress_formats_count
	     );
}

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

static bool parse_compressFormats(char **compress_formats, int *count) {
	assert(compress_formats);
	assert(count);

	//free old values
	int i =0;
	for(;i<*count;i++){
		cc_free(compress_formats[i]);
	}
	
	char *token;
	*count =0;
	while((token = strtok(NULL,";")) != NULL && *count < 128){
		compress_formats[(*count)] = cc_malloc(strlen(token) + 1);
		if(token[strlen(token) - 1] == '\n')
			token[strlen(token) - 1] = '\0';
		strcpy(compress_formats[(*count)],token);
		(*count) ++;
		
		cclog(5,"token is:%s, while *count is:%d\n",token,*count);
			
	}
	
	return (*count)<128;
	
}

static bool parseUrlRemoveHost(void)
{
	char **tmp = NULL;
	char *tmp1 = NULL;
	char *token = NULL;
	size_t cp_size = sizeof(char *);
	size_t len = 0;
	size_t size = 0;

	config.url_remove_host = NULL;
	config.url_remove_host_count = 0;

	do {
		token = strtok(NULL, white_space);
		if (NULL == token)
			break;
		size = (config.url_remove_host_count + 1) * cp_size;
		tmp = realloc(config.url_remove_host, size);
		if (NULL == tmp)
			break;
		config.url_remove_host = tmp;
		len = strlen(token);
		tmp1 = malloc(len + 1);
		if (NULL == tmp1)
			break;
		memcpy(tmp1, token, len);
		tmp1[len] = '\0';
		config.url_remove_host[config.url_remove_host_count] = tmp1;
		config.url_remove_host_count++;
	} while (1);

	return (NULL == config.url_remove_host) ? false : true; 
}

//parse config file and store it into global variable config
//return 0 on success, or -1 error occurred
int config_parse(const char *confile) {
	char buf[1024];
	int line = 0;
	int retval = RF_OK;
	FILE *cf = NULL;

	if((confile == NULL) || (strlen(confile) == 0)) {
		return RF_ERR_CONFIG_FILE;
	}

	if( (cf = fopen(confile, "r")) == NULL ) {
		return RF_ERR_CONFIG_FILE;
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
		else if(!strcasecmp(token, "db_commit_interval")) {
			ret = parse_int(&config.db_commit_interval);
		}
		else if(!strcasecmp(token, "db_commit_count")) {
			ret = parse_int(&config.db_commit_count);
		}
		else if(!strcasecmp(token, "url_wait_timeout")) {
			ret = parse_int(&config.url_wait_timeout);
		}
		else if(!strcasecmp(token, "dir_wait_timeout")) {
			ret = parse_int(&config.dir_wait_timeout);
		}
		else if(!strcasecmp(token, "dir_wait_timeout2")) {
			ret = parse_int(&config.dir_wait_timeout2);
		}
		else if(!strcasecmp(token, "if_hash_db")) {
			ret = parse_int(&config.if_hash_db);
		}
		else if(!strcasecmp(token, "enable_dir_purge")) {
			ret = parse_int(&config.enable_dir_purge);
		}
		else if( !strcasecmp(token, "report_ip") ) {
			ret = parse_ip(config.report_ip);
		}
		else if( !strcasecmp(token, "test_url") ) {
			ret = parse_string(config.test_url);
		}
		else if( !strcasecmp(token, "compress_delimiter") ) {
			ret = parse_string(config.compress_delimiter);
		}
		else if( !strcasecmp(token, "purge_success_del") ) {
			ret = parse_string(config.purge_success_del);
		}
		else if( !strcasecmp(token, "bdb_cache_size") ) {
			ret = parse_string(config.bdb_cache_size);
		}
		else if( !strcasecmp(token, "bdb_page_size") ) {
			ret = parse_string(config.bdb_page_size);
		}
		else if( !strcasecmp(token, "is_dir_refresh")){
			ret = parse_onoff(&config.is_dir_refresh);
		}
		else if( !strcasecmp(token, "compress_formats")){
			ret = parse_compressFormats(config.compress_formats,&config.compress_formats_count);
		}
		else if ( !strcasecmp(token, "url_remove_host") ) {
			ret = parseUrlRemoveHost();	
		}
		else{
			cclog(3, "unknown token at line: %d, token:%s\n", line, token);
			ret = false;
		}

		if(!ret){
			cclog(2, "parsed failed at line: %d, token:%s\n", line, token);
			retval = RF_ERR_CONFIG_FILE;
			break;
		}
	}

	fclose(cf);
	dump_config();
	return retval;
}

