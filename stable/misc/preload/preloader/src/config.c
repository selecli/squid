#include "preloader.h"

static void * get_global_member_addr(char * directive)
{
	if (0 == strcmp(directive, "sched_policy"))
	{
		return &g_global.cfg.sched_policy;
	}

	if (0 == strcmp(directive, "task_limit_number"))
	{
		return &g_global.cfg.task_limit_number;
	}

	if (0 == strcmp(directive, "task_concurrency"))
	{
		return &g_global.cfg.task_concurrency;
	}

	if (0 == strcmp(directive, "dns_nameserver"))
	{
		return &g_global.cfg.dns_addr;
	}

	if (0 == strcmp(directive, "access_log"))
	{
		return g_global.cfg.access_log;
	}

	if (0 == strcmp(directive, "debug_log"))
	{
		return g_global.cfg.debug_log;
	}

	if (0 == strcmp(directive, "error_log"))
	{
		return g_global.cfg.error_log;
	}

	if (0 == strcmp(directive, "http"))
	{
        return &g_global.cfg.http;
	}

	return NULL;
}

static void * get_listen_member_addr(char * directive)
{
	if (0 == strcmp(directive, "listen_on"))
	{
		return &g_listen.cfg.addr;
	}

	return NULL;
}

static void * get_preload_member_addr(char * directive)
{
	if (0 == strcmp(directive, "preload_for"))
	{
		return &g_preload.cfg.addr;
	}

	if (0 == strcmp(directive, "worker_number"))
	{
		return &g_preload.cfg.worker_number;
	}

	if (0 == strcmp(directive, "m3u8_engine"))
	{
		return &g_preload.cfg.m3u8_engine_enabled;
	}

	return NULL;
}

static void * get_grab_member_addr(char * directive)
{
	if (0 == strcmp(directive, "grab_from"))
	{
		return &g_grab.cfg.addr;
	}

	if (0 == strcmp(directive, "interval_time"))
	{
		return &g_grab.cfg.interval_time;
	}

	return NULL;
}

static void global_handler(void * cfg_data, void * data_var)
{
	int i;
	void * var;
	struct directive_st * drt;
	struct directive_st * drt1;
	struct array_st * block_array;

	drt = cfg_data;
	block_array = drt->params;

	for (i = 0; i < block_array->count; ++i)
	{
		drt1 = ARRAY_ITEM_DATA(block_array, i);
		var = get_global_member_addr(drt1->directive);

		switch (drt1->type)
		{
			case CFG_DIRECTIVE_ATOMIC:
				drt1->handler(ARRAY_ITEM_DATA(drt1->params, 0), var);
				break;

			case CFG_DIRECTIVE_COMPOSITE:
				drt1->handler(drt1, var);
				break;

			case CFG_DIRECTIVE_NONE:
				/* full through */

			default:
				/* do nothing */
				break;
		}
	}
}

static void listen_handler(void * cfg_data, void * cfg_var)
{
	int i;
	void * var;
	struct directive_st * drt;
	struct directive_st * drt1;
	struct array_st * block_array;

	drt = cfg_data;
	block_array = drt->params;

	for (i = 0; i < block_array->count; ++i)
	{
		drt1 = ARRAY_ITEM_DATA(block_array, i);
		var = get_listen_member_addr(drt1->directive);

		switch (drt1->type)
		{
			case CFG_DIRECTIVE_ATOMIC:
				drt1->handler(ARRAY_ITEM_DATA(drt1->params, 0), var);
				break;

			case CFG_DIRECTIVE_COMPOSITE:
				drt1->handler(drt1, var);
				break;

			case CFG_DIRECTIVE_NONE:
				/* full through */

			default:
				/* do nothing */
				break;
		}
	}
}

static void preload_handler(void * cfg_data, void * cfg_var)
{
	int i;
	void * var;
	struct directive_st * drt;
	struct directive_st * drt1;
	struct array_st * block_array;

	drt = cfg_data;
	block_array = drt->params;

	for (i = 0; i < block_array->count; ++i)
	{
		drt1 = ARRAY_ITEM_DATA(block_array, i);
		var = get_preload_member_addr(drt1->directive);

		switch (drt1->type)
		{
			case CFG_DIRECTIVE_ATOMIC:
				drt1->handler(ARRAY_ITEM_DATA(drt1->params, 0), var);
				break;

			case CFG_DIRECTIVE_COMPOSITE:
				drt1->handler(drt1, var);
				break;

			case CFG_DIRECTIVE_NONE:
				/* full through */

			default:
				/* do nothing */
				break;
		}
	}
}

static void grab_handler(void * cfg_data, void * cfg_var)
{
	int i;
	void * var;
	struct directive_st * drt;
	struct directive_st * drt1;
	struct array_st * block_array;

	drt = cfg_data;
	block_array = drt->params;

	for (i = 0; i < block_array->count; ++i)
	{
		drt1 = ARRAY_ITEM_DATA(block_array, i);
		var = get_grab_member_addr(drt1->directive);

		switch (drt1->type)
		{
			case CFG_DIRECTIVE_ATOMIC:
				drt1->handler(ARRAY_ITEM_DATA(drt1->params, 0), var);
				break;

			case CFG_DIRECTIVE_COMPOSITE:
				drt1->handler(drt1, var);
				break;

			case CFG_DIRECTIVE_NONE:
				/* full through */

			default:
				/* do nothing */
				break;
		}
	}
}

static void log_handler(void * cfg_data, void * cfg_var)
{
	int i;
	void * var;
    struct log_st * log;
	struct directive_st * drt;
	struct directive_st * drt1;
	struct array_st * block_array;

	xassert(NULL != cfg_var);

	log = cfg_var;
	drt = cfg_data;
	block_array = drt->params;

	for (i = 0; i < block_array->count; ++i)
	{
		drt1 = ARRAY_ITEM_DATA(block_array, i);

		if (0 == strcmp(drt1->directive, "file_path"))
		{
			var = &log->fmsg.path;
		}
		else if (0 == strcmp(drt1->directive, "print_level"))
		{
			var = &log->print_level;
		}
		else if (0 == strcmp(drt1->directive, "rotate_size"))
		{
			var = &log->rotate_size;
		}
		else if (0 == strcmp(drt1->directive, "rotate_number"))
		{
			var = &log->rotate_number;
		}
		else
		{
			LogAbort("config parsing log option error.");
			exit(1);
		}

		drt1->handler(ARRAY_ITEM_DATA(drt1->params, 0), var);

	}
}

static void http_handler(void * cfg_data, void * cfg_var)
{
	int i;
	void * var;
	struct directive_st * drt;
	struct directive_st * drt1;
	struct http_option_st * http;
	struct array_st * block_array;

	http = cfg_var;
	drt = cfg_data;
	block_array = drt->params;

	for (i = 0; i < block_array->count; ++i)
	{
		drt1 = ARRAY_ITEM_DATA(block_array, i);

		if (0 == strcmp(drt1->directive, "send_timeout"))
		{
			var = &http->send_timeout;
		}
		else if (0 == strcmp(drt1->directive, "recv_timeout"))
		{
			var = &http->recv_timeout;
		}
		else if (0 == strcmp(drt1->directive, "connect_timeout"))
		{
			var = &http->connect_timeout;
		}
		else if (0 == strcmp(drt1->directive, "keepalive_timeout"))
		{
			var = &http->keepalive_timeout;
		}
        else if (0 == strcmp(drt1->directive, "max_request_header_size"))
		{
			var = &http->max_request_header_size;
		}
        else if (0 == strcmp(drt1->directive, "max_request_body_size"))
		{
			var = &http->max_request_body_size;
		}
		else
		{
			LogAbort("config parsing error for http.");
			exit(1);
		}

		drt1->handler(ARRAY_ITEM_DATA(drt1->params, 0), var);
	}
}

static void address_handler(void * cfg_data, void * cfg_var)
{
	long port;
	char * p_ip, * p_port;
	struct sockaddr_in * addr;

	addr = cfg_var;
	p_ip = cfg_data;
    xmemzero(addr, g_tsize.sockaddr_in);
	p_port = strchr(p_ip, ':');

	if (NULL == p_port)
	{
        LogAbort("config parsing error for address: address format error.");
		exit(1);
	}

	*p_port = '\0';
	p_port++;

	if ('\0' == *p_port)
	{
        LogAbort("config parsing error for address: address lack port.");
		exit(1);
	}

	if (inet_pton(AF_INET, p_ip, (void *)&addr->sin_addr) < 0)
	{
        LogAbort("cofig parsing error for address: %s.", xerror());
		exit(1);
	}

	port = strtol(p_port, NULL, 10);

	if (port < 0 || port > 65535)
	{
        LogAbort("cofig parsing error for address port: %ld.", port);
		exit(1);
	}

	addr->sin_port = htons((unsigned short)port);
	addr->sin_family = AF_INET;
}

static void address_handler_2(void * cfg_data, void * cfg_var)
{
	long port;
	int n1, n2, n3, n4;
	char * p_ip, * p_port;
    struct config_addr_st * addr;

	addr = cfg_var;
	p_ip = cfg_data;
    xmemzero(addr, g_tsize.config_addr_st);
	p_port = strchr(p_ip, ':');

	if (NULL == p_port)
	{
        LogAbort("config parsed error for address: address format error.");
		exit(1);
	}

	*p_port = '\0';
	p_port++;

	if ('\0' == *p_port)
	{
        LogAbort("config parsed error for address: lacked port.");
		exit(1);
	}

	if (4 != sscanf(p_ip, "%d.%d.%d.%d", &n1, &n2, &n3, &n4))
	{
        /* it's dns */
        addr->type = SOCKADDR_DNS;
		addr->sockaddr.dns.host = xstrdup(p_ip);
		port = strtol(p_port, NULL, 10);
		if (port < 0 || port > 65535)
		{
            LogAbort("config parsed error for address port: %ld", port);
			exit(1);
		}
		addr->sockaddr.dns.port = htons((unsigned short)port);
		return;
	}

    addr->type = SOCKADDR_IP;
	if (inet_pton(AF_INET, p_ip, (void *)&addr->sockaddr.ip.sin_addr) < 0)
	{
        LogAbort("config parsed error for address: %s.", xerror());
		exit(1);
	}
	port = strtol(p_port, NULL, 10);
	if (port < 0 || port > 65535)
	{
        LogAbort("config parsed error for address port: %ld.", port);
		exit(1);
	}
	addr->sockaddr.ip.sin_port = htons((unsigned short)port);
	addr->sockaddr.ip.sin_family = AF_INET;
}

static void strtoi_handler(void * cfg_data, void * cfg_var)
{
	*((unsigned int *)cfg_var) = (unsigned int)strtol((char *)cfg_data, NULL, 10);
}

static void sched_policy_handler(void * cfg_data, void * cfg_var)
{
	if (0 == strcasecmp((char *)cfg_data, "FIFO"))
	{
		*((int *)cfg_var) = SCHED_POLICY_FIFO;
		return;
	}

	if (0 == strcasecmp((char *)cfg_data, "HPFP"))
	{
		*((int *)cfg_var) = SCHED_POLICY_HPFP;
		return;
	}

	*((int *)cfg_var) = SCHED_POLICY_FIFO;
}

static void on_off_handler(void * cfg_data, void * cfg_var)
{
	if (0 == strcmp((char *)cfg_data, "on"))
	{
		*((unsigned int *)cfg_var) = 1;
		return;
	}

	if (0 == strcmp((char *)cfg_data, "off"))
	{
		*((unsigned int *)cfg_var) = 0;
		return;
	}

	*((unsigned int *)cfg_var) = 0;
}

static void strdup_handler(void * cfg_data, void * cfg_var)
{
    if (NULL != *(char **)cfg_var)
    {
        safe_process(free, *(char **)cfg_var);
    }
    *((char ** )cfg_var) = xstrdup((char *)cfg_data);
}

static struct config_directive_st config_directives[] = {
	{
		"global",
		CFG_DIRECTIVE_COMPOSITE,
		0,
		global_handler
	},
	{
		"listen",
		CFG_DIRECTIVE_COMPOSITE,
		0,
		listen_handler
	},
	{ "preload",
		CFG_DIRECTIVE_COMPOSITE,
		0,
		preload_handler
	},
	{
		"grab",
		CFG_DIRECTIVE_COMPOSITE,
		0,
		grab_handler
	},
	{
		"access_log",
		CFG_DIRECTIVE_COMPOSITE,
		0,
		log_handler
	},
	{
		"debug_log",
		CFG_DIRECTIVE_COMPOSITE,
		0,
		log_handler
	},
	{
		"error_log",
		CFG_DIRECTIVE_COMPOSITE,
		0,
		log_handler
	},
	{
		"http",
		CFG_DIRECTIVE_COMPOSITE,
		0,
		http_handler
	},
	{
		"listen_on",
		CFG_DIRECTIVE_ATOMIC,
		1,
		address_handler
	},
	{
		"preload_for",
		CFG_DIRECTIVE_ATOMIC,
		1,
		address_handler
	},
	{
		"grab_from",
		CFG_DIRECTIVE_ATOMIC,
		1,
		address_handler_2
	},
	{
		"worker_number",
		CFG_DIRECTIVE_ATOMIC,
		1,
		strtoi_handler
	},
	{
		"interval_time",
		CFG_DIRECTIVE_ATOMIC,
		1,
		strtoi_handler
	},
	{
		"sched_policy",
		CFG_DIRECTIVE_ATOMIC,
		1,
		sched_policy_handler
	},
	{
		"task_limit_number",
		CFG_DIRECTIVE_ATOMIC,
		1,
		strtoi_handler
	},
	{
		"task_concurrency",
		CFG_DIRECTIVE_ATOMIC,
		1,
		strtoi_handler
	},
	{
		"dns_nameserver",
		CFG_DIRECTIVE_ATOMIC,
		1,
		address_handler
	},
	{
		"m3u8_engine",
		CFG_DIRECTIVE_ATOMIC,
		1,
		on_off_handler
	},
	{
		"file_path",
		CFG_DIRECTIVE_ATOMIC,
		1,
		strdup_handler
	},
	{
		"print_level",
		CFG_DIRECTIVE_ATOMIC,
		1,
		strtoi_handler
	},
	{
		"rotate_size",
		CFG_DIRECTIVE_ATOMIC,
		1,
		strtoi_handler
	},
	{
		"rotate_number",
		CFG_DIRECTIVE_ATOMIC,
		1,
		strtoi_handler
	},
	{
		"send_timeout",
		CFG_DIRECTIVE_ATOMIC,
		1, 
		strtoi_handler
	},
	{
		"recv_timeout",
		CFG_DIRECTIVE_ATOMIC,
		1,
		strtoi_handler
	},
	{
		"connect_timeout",
		CFG_DIRECTIVE_ATOMIC,
		1,
		strtoi_handler
	},
	{
		"keepalive_timeout",
		CFG_DIRECTIVE_ATOMIC,
		1,
		strtoi_handler
	},
	{
		"max_request_header_size",
		CFG_DIRECTIVE_ATOMIC,
		1,
		strtoi_handler
	},
	{
        "max_request_body_size",
		CFG_DIRECTIVE_ATOMIC,
		1,
		strtoi_handler
	},
	{
		NULL,
		CFG_DIRECTIVE_NONE,
		0,
		NULL
	},
};

int check_conf_line_invalid(const char * cfg_line)
{
	const char * p; 

	for (p = cfg_line; xisspace(*p); ++p) ;

	return ('#' == *p || '\n' == *p || '\r' == *p || '\0' == *p); 
}

static struct directive_st * directive_create(void)
{
	struct directive_st * drt;

	drt = xmalloc(g_tsize.directive_st);
	drt->directive = NULL;
	drt->params = NULL;
	drt->handler = NULL;
	drt->type = CFG_DIRECTIVE_NONE;

	return drt;
}

void directive_destroy(struct directive_st * drt)
{
	int i;
	struct directive_st * drt1;

	safe_process(free, drt->directive);

	for (i = 0; i < drt->params->count; ++i)
	{
		drt1 = ARRAY_ITEM_DATA(drt->params, i);

		switch (drt1->type)
		{
			case CFG_DIRECTIVE_ATOMIC:
				safe_process(free, drt1);
				break;

			case CFG_DIRECTIVE_COMPOSITE:
				directive_destroy(drt1);
				break;

			case CFG_DIRECTIVE_NONE:
				/* full through */

			default:
				/* do nothing */
				break;
		}
	}
}

static struct directive_st * parse_composite_directive(FILE * fp, unsigned int * ln, ssize_t offset)
{
	size_t line_length;
	int i, directive_matched;
	char * delim, * saveptr;
	char * token1, * token2;
	char cfg_line[SIZE_4KB];
	struct config_directive_st * drt;
	struct directive_st * drt_atomic;
	struct directive_st * drt_block;
	struct directive_st * drt_block_recursive;

	delim = " ;\t\r\n";
	drt_block = NULL;
	drt_atomic = NULL;
	drt_block_recursive = NULL;

	fseek(fp, offset, SEEK_CUR);

	while (0 == feof(fp))
	{    
		if (NULL == fgets(cfg_line, SIZE_4KB, fp)) 
		{    
			continue;
		}    

		(*ln)++; /* read one line, add it. */
		directive_matched = 0;
		line_length = strlen(cfg_line);

		if (check_conf_line_invalid(cfg_line))
		{    
			/* skip the invalid line. */
			continue;
		}    

		token1 = strtok_r(cfg_line, delim, &saveptr);

		if (NULL == token1)
		{
			continue;
		}

		if ('}' == token1[0])
		{
			return drt_block;
		}

		for (drt = config_directives; NULL != drt->directive; ++drt)
		{
			if (0 == strcmp(drt->directive, token1))
			{
				directive_matched = 1;

				switch (drt->type)
				{
					case CFG_DIRECTIVE_ATOMIC:

						if (NULL == drt_block)
						{
							/* atomic directive must in the block directive. */

                            fprintf(stderr, "preloader parsing error at line NO.%d.", *ln);
							LogError(1, "preloader parsing error at line NO.%d.", *ln);

							return NULL;
						}

						drt_atomic = directive_create();
						drt_atomic->directive = xstrdup(token1);
						drt_atomic->type = drt->type;
						drt_atomic->handler = drt->handler;
						drt_atomic->params = array_create();

						for (i = 0; i < drt->param_number; ++i)
						{
							token2 = strtok_r(NULL, delim, &saveptr);

							if (NULL == token2)
							{
								break;
							}

							/* directive param */
							array_append(drt_atomic->params, xstrdup(token2), free);
						}

						if (drt_atomic->params->count != drt->param_number)
						{
							/* error */
							directive_destroy(drt_atomic);
							directive_destroy(drt_block);

							fprintf(stderr, "preloader parsing error at line NO.%d.", *ln);
							LogError(1, "preloader parsing error at line NO.%d.", *ln);

							return NULL;	
						}

						/* join into the block directive */
						array_append(drt_block->params, drt_atomic, (FREE *)directive_destroy);

						break;

					case CFG_DIRECTIVE_COMPOSITE:

						token2 = strtok_r(NULL, delim, &saveptr);

						if (NULL == token2 || '{' != token2[0])
						{
							/* error */
							fprintf(stderr, "preloader parsing error at line NO.%d.", *ln);
							LogError(1, "preloader parsing error at line NO.%d.", *ln);

							return NULL;	
						}

						if (NULL != drt_block)
						{
							/* unspport nested with same directive. */
							if (0 == strcmp(drt_block->directive, drt->directive))
							{
								directive_destroy(drt_block);

								fprintf(stderr, "preloader parsing error at line NO.%d.", *ln);
								LogError(1, "preloader parsing error at line NO.%d.", *ln);

								return NULL;
							}

							drt_block_recursive = parse_composite_directive(fp, ln, -line_length);

							if (NULL == drt_block_recursive)
							{
								directive_destroy(drt_block);

								return NULL;
							}

							array_append(drt_block->params,
									drt_block_recursive,
									(FREE *)directive_destroy);

							break;
						}

						drt_block = directive_create();
						drt_block->directive = xstrdup(token1);
						drt_block->type = drt->type;
						drt_block->handler = drt->handler;
						drt_block->params = array_create();

						break;

					case CFG_DIRECTIVE_NONE:
						/* full through */

					default:
						LogAbort("config directive type error.");
						break;
				}
				break;
			}
		}

		if (0 == directive_matched)
		{
            fprintf(stderr, "preloader parsing error: unknown directive \"%s\".", token1);
            LogError(1, "preloader parsing error: unknown directive \"%s\".", token1);
		}
	}

	if (NULL != drt_block)
	{
		/* FIXME: more other to do here. */
		directive_destroy(drt_block);
	}

	return NULL;
}

void parse_conf_done(void)
{
    log_destroy(g_log.access);
    log_destroy(g_log.debug);
    log_destroy(g_log.error);

    g_log.access = log_clone(g_global.cfg.access_log);
    g_log.debug  = log_clone(g_global.cfg.debug_log);
    g_log.error  = log_clone(g_global.cfg.error_log);

    log_open(g_log.access);
    log_open(g_log.debug);
    log_open(g_log.error);

    if (g_preload.cfg.worker_number > MAX_PRELOAD_WKR)
    {
        g_preload.cfg.worker_number = MAX_PRELOAD_WKR;
    }
    if (g_global.cfg.task_concurrency > MAX_CONCURRENCY)
    {
        g_global.cfg.task_concurrency = MAX_CONCURRENCY;
    }
    if (g_global.cfg.task_limit_number > MAX_TASK_LIMIT)
    {
        g_global.cfg.task_limit_number = MAX_TASK_LIMIT;
    }
}

void parse_conf(void)
{
	int i;
	FILE * fp;
	unsigned int ln;
	struct array_st * drts;
	struct directive_st * drt;
	struct directive_st * drt_block;

	fp = xfopen(g_cfg_file.path, "r");

	if (NULL == fp)
	{
        LogAbort("open config file failed: %s.", g_cfg_file.path);
		exit(1);
	}

	/* Althought we know that fopen("r") will set the stream pointer
	 * to the beginning of the file, but we still do it by fseek().
	 */

	fseek(fp, 0, SEEK_SET);

	ln = 0; /* current file line number */
	drts = NULL;

	do {
		drt_block = parse_composite_directive(fp, &ln, 0);

		if (NULL == drt_block)
		{
			break;	
		}

		if (NULL == drts)
		{
			drts = array_create();
		}

		array_append(drts, drt_block, (FREE *)directive_destroy);

	} while (1);

	xfclose(fp);

	if (NULL == drts)
	{
        fprintf(stderr, "preloader parsing error: config format error.");
        LogError(1, "preloader parsing error: config format error.");
		exit(1);
	}

	for (i = 0; i < drts->count; ++i)
	{
		drt = ARRAY_ITEM_DATA(drts, i);
		drt->handler(drt, NULL);
	}

	parse_conf_done();
}

