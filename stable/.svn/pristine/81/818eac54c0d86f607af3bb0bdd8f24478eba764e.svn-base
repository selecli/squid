#ifndef __CONF_H
#define __CONF_H

typedef struct
{
	char default_filename[MAX_STRING];
	char http_proxy[MAX_STRING];
	char no_proxy[MAX_STRING];
	int strip_cgi_parameters;
	int save_state_interval;
	int connection_timeout;
	int reconnect_delay;
	int how_many;
	uint32_t buffer_size;
	int limit_speed;
	int verbose;
	//int alternate_output;
	int print_reply;
	int resume_get;
	
	//if_t *interfaces;

	int search_timeout;
	int search_threads;
	int search_amount;
	int search_top;

	//Xcell added
	char headers[2048];
	char dst_ip[16];
	char filename[256];	//Xcell output file
	int prefixheader;   //Xcell 是否写header文件
    int handle_header;	//Xcell 是否写处理header
	int retry;       	//Xcell 重传次数
	//end

} conf_t;

int conf_loadfile( conf_t *conf, char *file );
int conf_init( conf_t *conf );


#endif

