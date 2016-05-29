#include "flexicache.h"

#define CONFIG_LINE_LEN 4096
static void loadDefaultConfig(Config *c);

void loadConfigFile(char * path, Config * c)
{
	loadDefaultConfig(c);

	FILE *fp = fopen(path, "r");
	if (NULL == fp)
		fatalf("flexicache.conf open failed\n");

	char config_input_line[CONFIG_LINE_LEN];
	char *token = NULL;
	int multisquid = 0;	
	while (fgets(config_input_line, CONFIG_LINE_LEN, fp)) {

		if ((token = strrchr(config_input_line, '\n')))
			*token = '\0';
		if ((token = strrchr(config_input_line, '\r')))
			*token = '\0';
		if (config_input_line[0] == '#')
			continue;
		if (config_input_line[0] == '\0')
			continue;


		token = strtok(config_input_line, w_space);
		if(!strcmp(token, "cache_type"))
		{
			token = strtok(NULL, w_space);
			if(!token)
				fatalf("Missing cache_type!\n");
			
			if(!strcmp(token, "squid"))
				c->cache_type = ct_squid;
			else
				fatalf("Unrecognized cache_type!\n");
		}
		else if(!strcmp(token, "cache_path"))
		{
			token = strtok(NULL, w_space);
			if(!token)
				fatalf("Missing cache_path!\n");
			c->cache_path = strdup(token);
			c->need_free_pos++;
			c->need_free_ptr[c->need_free_pos] = c->cache_path;
		}
		else if(!strcmp(token, "cache_processes"))
		{
			token = strtok(NULL, w_space);
			if(!token)
				fatalf("Missing cache_processes!\n");

			c->cache_processes = atoi(token);
			if(c->cache_processes > MAX_CACHE_PROCS)
				c->cache_processes = MAX_CACHE_PROCS;
		}
		else if(!strcmp(token, "lb_type"))
		{
			token = strtok(NULL, w_space);
			if(!token)
				fatalf("Missing lb_type!\n");
			
			if(!strcmp(token, "haproxy"))
				c->lb_type = lt_haproxy;
			else if(!strcmp(token, "lscs"))
				c->lb_type = lt_lscs;
			else
				fatalf("Unrecognized lb_type %s!\n", token);
		}
		else if(!strcmp(token, "lb_path"))
		{
			token = strtok(NULL, w_space);
			if(!token)
				fatalf("Missing lb_path!\n");
			c->lb_path = strdup(token);
			c->need_free_pos++;
			c->need_free_ptr[c->need_free_pos] = c->lb_path;
		}	
		else if(!strcmp(token, "log_level"))
		{
			token = strtok(NULL, w_space);
			if(!token)
				fatalf("Missing log_level!\n");
			c->log_level = atoi(token);
		}
		else if(!strcmp(token, "cache_conf_path"))
		{
			token = strtok(NULL, w_space);
			if(!token)
				fatalf("Missing cache_conf_path!\n");
			c->cache_conf_path = strdup(token);
			c->need_free_pos++;
			c->need_free_ptr[c->need_free_pos] = c->cache_conf_path;
		}	
		else if(!strcmp(token, "lb_conf_path"))
		{
			token = strtok(NULL, w_space);
			if(!token)
				fatalf("Missing lb_conf_path!\n");
			c->lb_conf_path = strdup(token);
			c->need_free_pos++;
			c->need_free_ptr[c->need_free_pos] = c->lb_conf_path;
		}
		else if(!strcmp(token, "kill_port"))
		{
			token = strtok(NULL, w_space);
			if(!token)
				fatalf("Missing kill_port!\n");
			c->kill_port = atoi(token);
		}
		else if (!strcmp(token,"cc_multisquid")){
			token = strtok(NULL,w_space);
			if (!token)
				fatalf("Missing cc_multisquid!\n");
			c->cc_multisquid = atoi(token);
			multisquid = 1;
		}
        else if (!strcmp(token, "watch_interval")){
            token = strtok(NULL, w_space);
			if (!token)
				fatalf("Missing watch_interval!\n");
			c->watch_interval = atoi(token);
	    } else if (!strcmp(token, "watch_timeout")){
            token = strtok(NULL, w_space);
			if (!token)
				fatalf("Missing watch_timeout!\n");
			c->watch_timeout = atoi(token);
        }
    }

	fclose(fp);

	if (0 == multisquid) {
		if (NULL == (fp=fopen(path,"a"))) 
			fatalf("flexicache.conf write failed\n");
		fprintf(fp, "cc_multisquid\t%d\n", c->cache_processes);
		fclose(fp);
	}

	if (multisquid && c->cc_multisquid != c->cache_processes)
		fatalf("cc_multisquid = %d, not equal %d\n", c->cc_multisquid, c->cache_processes);

    /*Start to Get the backend Cache port*/
	FILE *cache_conf_fp = fopen(CACHE_CONF_PATH, "r");
    int j = 0;
    int k = 1;
    int *ports = calloc(1, PORT_NUM *sizeof(int));

	while (fgets(config_input_line, CONFIG_LINE_LEN, cache_conf_fp)) {

		if (config_input_line[0] == '#' || config_input_line[0] == '\0')
			continue;
		if ((token = strrchr(config_input_line, '\n')))
			*token = '\0';
		if ((token = strrchr(config_input_line, '\r')))
			*token = '\0';

		token = strtok(config_input_line, w_space);
        if (NULL == token ) 
            continue;
        while ( *token == ' ' && token++ ); /*eat blanks*/

        if ( strncmp(token, "http_port", 9) == 0 || 
             strncmp(token, "https_port", 10) == 0  ) { /*Get a port*/

            token = strtok(NULL, w_space); /*Get the port field*/ 
            if (NULL == token ) 
                continue;
            while ( *token == ' ' && token++ ); /*eat blanks*/
            char *pos = NULL;
            int port  = -1;
            if ( (pos = strchr(token, ':')) != NULL) {/*http_port 192.168.100.188:1024*/
                port = atoi(++pos); 
            } else {                        /*http_port 1024*/
                port = atoi(token); 
            }

            if (port > 0) {                 /*Add a port to the port array*/
                if (j >= 16) {
                    k++;
                    ports = realloc(ports, (k * PORT_NUM + 1) *sizeof(int) );
                }
                ports[j] = port;
                j++; 
            }
        } else {
            continue; 
        }
    }
    c->cache_ports = ports;
    c->ports_num = j;
    c->need_free_pos++;
    c->need_free_ptr[c->need_free_pos] = ports;
}

static void loadDefaultConfig(Config *c)
{
	c->cache_type = ct_squid;
	c->cache_path = CACHE_PATH;
	c->cache_conf_path = CACHE_CONF_PATH;
	/* default value = the number of cpu core  */
	c->cache_processes = sysconf(_SC_NPROCESSORS_CONF);
	c->cc_multisquid = c->cache_processes;
	c->lb_type = lt_lscs;
	c->lb_path = LB_PATH;
	c->lb_conf_path = LB_CONF_PATH;
   	
	c->log_level = 1;
	c->kill_port = 80;
	c->need_free_pos = -1;

    c->watch_interval   = CACHE_WATCH_INTERVAL_DEFAULT;
    c->watch_timeout    = CACHE_WATCH_TIMEOUT_DEFAULT;   
}
