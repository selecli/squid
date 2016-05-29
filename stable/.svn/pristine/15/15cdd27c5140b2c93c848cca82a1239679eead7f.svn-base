#include "detect.h"

struct file_handler {
	int num;
	FILE *fp[MAX_FP_NUM];
};

static int hashtableFind1(detect_cfg_t *cfg);
static int hashtableCheck1(const char *hostname, data_ips_t *ips, uint32_t ftimes);

static int checkConfigNumberOver(struct file_handler *fps)
{
	return (fps->num >= MAX_FP_NUM);
}

//check whether exist or have access permission for configuration files
static void checkConfigFilesValid(void)
{
	if (MODE_FC & detect.config.domain.mode)
	{
		if (access(detect.config.domain.domainFile, F_OK|R_OK) < 0)
		{
			dlog(WARNING, NOFD,
					"config file not exist or permission denied, FILE=[%s]\n",
					detect.config.domain.domainFile);
			xabort("config file not exist or permission denied");
		}
	}
	if (MODE_TTA & detect.config.domain.mode)
	{
		if (access(detect.config.domain.ttaConfFile, F_OK|R_OK) < 0)
		{
			dlog(WARNING, NOFD,
					"config file not exist or permission denied, FILE=[%s]\n",
					detect.config.domain.ttaConfFile);
			xabort("config file not exist or permission denied");
		}
	}
	if (access(detect.config.domain.dnsIpFile, R_OK|F_OK) < 0)
	{
		dlog(WARNING, NOFD,
				"dnsip file not exist or permission denied, FILE=[%s]\n",
				detect.config.domain.dnsIpFile);
	}
}

static int hostnameCmp(void *key, void *data)
{
	return strcmp((char *)key, ((detect_cfg_t *)data)->hostname);
}

//0: invalid ip; 1: 0.0.0.0; 2: valid ip
static int checkIpValid(const char *ip)
{
	struct in_addr addr;

	xmemzero(&addr, sizeof(addr));
	if (inet_pton(AF_INET, ip, &addr) <= 0)
		return 0;
	if (0 == addr.s_addr)
		return 1;

	return 2;
}

static int checkRepeatIp(data_ips_t *ips, const char *ip, const int iptype)
{
	int i;

	for (i = 0; i < ips->num; ++i)
	{   
		if (!strcmp(ips->ip[i].ip, ip) && ips->ip[i].iptype == iptype)
			return 1;
	}   

	return 0;
}

static int parseForIps(const int iptype, char *ipstr, detect_cfg_t *cfg)
{
	int i, ret;
	size_t size;
	char *saveptr, *ip;

	ip = strtok_r(ipstr, ":|", &saveptr);	
	if (NULL == ip)
		return FAILED;
	for (i = cfg->ips.num; i < CLINE_IP_MAX; ++i)
	{
		ret = checkIpValid(ip);
		if (0 == ret)
		{
			//here backup is alias name of hostname
			cfg->backup = xstrdup(ip);
			return SUCC;
		}
		if (1 == ret)
		{
			dlog(WARNING, NOFD, "IP=[%s] is invalid, HOSTNAME=[%s]", ip, cfg->hostname);
			--i;
			ip = strtok_r(NULL, ":|", &saveptr);	
			if (NULL == ip)
				break;
			continue;
		}
		if (checkRepeatIp(&cfg->ips, ip, iptype))
		{
			dlog(WARNING, NOFD, "IP=[%s] is repeat, HOSTNAME=[%s]", ip, cfg->hostname);
			--i;
			ip = strtok_r(NULL, ":|", &saveptr);	
			if (NULL == ip)
				break;
			continue;
		}
		size = (i + 1) * detect.tsize.data_ip_t_size;
		cfg->ips.ip = xrealloc(cfg->ips.ip, size);
		xmemzero(cfg->ips.ip + i, detect.tsize.data_ip_t_size);
		strncpy(cfg->ips.ip[i].ip, ip, BUF_SIZE_16);
		cfg->ips.ip[i].iptype = iptype;
		cfg->ips.num++;
		ip = strtok_r(NULL, ":|", &saveptr);	
		if (NULL == ip)
			break;
	}

	return SUCC;
}

static int parseMethod(char *src, struct method_st *method, char *hostname)
{
	char *pp, *token, *lasts;
	char buffer[BUF_SIZE_512];

	token = strtok_r(src, ":|", &lasts);
	if (NULL == token)
	{
		dlog(ERROR, NOFD, "parseMethod: configuration error");
		return -1;	
	}
	//defualt can only configuration: y/Y/yes/YES/n/N/no/NO
	if (!strcasecmp(token, "Y") ||
			!strcasecmp(token, "N") ||
			!strcasecmp(token, "NO") ||
			!strcasecmp(token, "YES"))
	{
		method->method = METHOD_GET;
		goto DEFAULT;
	}
	if (!strcasecmp(token, "GET"))
		method->method = METHOD_GET;
	else if (!strcasecmp(token, "POST"))
		method->method = METHOD_POST;
	else if (!strcasecmp(token, "HEAD"))
		method->method = METHOD_HEAD;
	else
	{
		dlog(ERROR, NOFD, "parseMethod: not support this method");
		goto EXCEPTION;
	}
	token = strtok_r(NULL, "|", &lasts);
	if (NULL == token)
	{
		if (METHOD_POST == method->method)
		{
			dlog(ERROR, NOFD, "parseMethod: method 'POST' need a post file and file type");
			goto EXCEPTION;
		}
		goto DEFAULT;
	}
	//not have hostname
	if ('/' == *(token))
	{
		if ('\0' == *(token + 1))
			goto DEFAULT;
		method->hostname = xstrdup(hostname);
		method->filename = xstrdup(token);
		goto NEXT;
	}
	//have hostname
	pp = strchr(token, '/');
	if (NULL == pp)
	{
		method->hostname = xstrdup(token);
		snprintf(buffer, BUF_SIZE_512, "/%u.jpg", (unsigned int)time(NULL));
		method->filename = xstrdup(buffer);
		goto NEXT;
	}
	method->hostname = xstrndup(token, pp - token);
	if ('\0' == *(pp + 1))
		snprintf(buffer, BUF_SIZE_512, "/%u.jpg", (unsigned int)time(NULL));
	else
		strncpy(buffer, pp, BUF_SIZE_512);
	method->filename = xstrdup(buffer);
	goto NEXT;
	return 0;

NEXT:
	if (method->method != METHOD_POST)
		return 0;
	token = strtok_r(NULL, "|", &lasts);
	if (NULL == token)
	{
		dlog(ERROR, NOFD, "parseMethod: method 'POST' need a post file");
		return -1;
	}
	method->postfile = xstrdup(token);
	token = strtok_r(NULL, "|", &lasts);
	method->posttype = (NULL == token) ? xstrdup(POST_DFT_TYPE) : xstrdup(token);
	return 0;

DEFAULT:
	method->hostname = xstrdup(hostname);
	snprintf(buffer, BUF_SIZE_512, "/%u.jpg", (unsigned int)time(NULL));
	method->filename = xstrdup(buffer);
	return 0;

EXCEPTION:
	if (method->hostname != NULL)
		xfree((void *)&method->hostname);
	if (method->filename != NULL)
		xfree((void *)&method->filename);
	if (method->postfile != NULL)
		xfree((void *)&method->postfile);
	if (method->posttype != NULL)
		xfree((void *)&method->posttype);
	return -1;
}

static int getSuccCodes(detect_cfg_t *cfg, const char *str)
{   
	int i, num;

	num = (strlen(str) + 1) / 4;
	cfg->codes.code = xcalloc(num, detect.tsize.codes_t_size);
	for (i = 0; i < num; ++i)
	{
		if (i < num - 1 && '|' != str[(i + 1) * 4 - 1])
		{
			dlog(WARNING, NOFD, "codes configuration format error");
			return FAILED;
		}
		strncpy(cfg->codes.code[i].code, str + 4 * i, 3);
		if ('*' == cfg->codes.code[i].code[1])
			cfg->codes.code[i].code[1] = 0;
		if ('*' == cfg->codes.code[i].code[2])
			cfg->codes.code[i].code[2] = 0;
		cfg->codes.code[i].code[3] = 0;
		cfg->codes.num++;
	}

	return SUCC;
}

static size_t requestContentLength(const size_t len)
{
	if (len < DETECT_OBJLEN_MIN)
		return DETECT_OBJLEN_MIN;
	if (len > DETECT_OBJLEN_MAX)
		return DETECT_OBJLEN_MAX;

	return len;
}

static int getDetectTimes(const int times)
{
	if (times < 1)
		return DETECT_TIMES_DFT;
	if (times > DETECT_TIMES_MAX)
		return DETECT_TIMES_MAX;

	return times;
}

static int getDetectPort(const int port)
{
	return (port < DETECT_PORT_MIN || port > DETECT_PORT_MAX) ? DETECT_PORT_DFT : port;
}

static void chooseDetectWay(detect_cfg_t *cfg)
{
	time_t min = detect.runstate.starttime / 60;

	if (0 == cfg->http_itime)
	{
		if (0 == cfg->tcp_itime)
		{
			cfg->flags.protocol = PROTOCOL_NONE;
			return;
		}
		if (!(min % cfg->tcp_itime))
			cfg->flags.protocol = PROTOCOL_TCP;
		else
			cfg->flags.protocol = PROTOCOL_NONE;
		return;
	}
	if (min % cfg->http_itime)
	{
		if (0 == cfg->tcp_itime)
		{
			cfg->flags.protocol = PROTOCOL_NONE;
			return;
		}
		if (!(min % cfg->tcp_itime))
			cfg->flags.protocol = PROTOCOL_TCP;
		else
			cfg->flags.protocol = PROTOCOL_NONE;
		return;
	}
	cfg->flags.protocol = (HTTPS_PORT == cfg->port) ? PROTOCOL_HTTPS : PROTOCOL_HTTP;
}

static int isInvalidConfigLine(const char *line)
{
	const char *ptr = line;

	while (xisspace(*ptr)) ++ptr;

	return ('#' == line[0]
			|| '\n' == line[0]
			|| '\r' == line[0]
			|| '\0' == line[0]);
}

static void clearNewlineSymbol(char *line)
{
	size_t end;

	end = strlen(line) - 1;
	if ('\n' == line[end])
		line[end] = '\0';
}

static int parseDnsIpRecordFileOneLine(char *line)
{
	int i, ret;
	size_t size;
	uint32_t ftimes;
	char *token, *saveptr, *ip;
	char ipstr[BUF_SIZE_4096];
	char hostname[BUF_SIZE_256];
	data_ips_t *ips;

	if (NULL == (token = strtok(line, " \t")))
		return FAILED;
	xmemzero(hostname, BUF_SIZE_256);
	strcpy(hostname, token);
	if ('.' != hostname[strlen(hostname) - 1])
		strcat(hostname, ".");
	if (NULL == (token = strtok(NULL, " \t")))
		return FAILED;
	if (NULL == (token = strtok(NULL, " \t")))
		return FAILED;
	xmemzero(ipstr, BUF_SIZE_4096);
	strncpy(ipstr, token, BUF_SIZE_4096);
	if (NULL == (token = strtok(NULL, " \t\n")))
		return FAILED;
	ftimes = atoi(token);
	ip = strtok_r(ipstr, ":|", &saveptr);	
	if (NULL == ip)
		return FAILED;
	ips = xcalloc(1, sizeof(data_ips_t));
	for (i = 0; i < CLINE_IP_MAX; ++i)
	{
		ret = checkIpValid(ip);
		if (0 == ret)
		{
			continue;
		}
		if (1 == ret)
		{
			dlog(WARNING, NOFD, "dns IP=[%s] is invalid", ip);
			--i;
			ip = strtok_r(NULL, ":|", &saveptr);	
			if (NULL == ip)
				break;
			continue;
		}
		if (checkRepeatIp(ips, ip, IPTYPE_DNS))
		{
			dlog(WARNING, NOFD, "dns IP=[%s] is repeat", ip);
			--i;
			ip = strtok_r(NULL, ":|", &saveptr);	
			if (NULL == ip)
				break;
			continue;
		}
		size = (i + 1) * detect.tsize.data_ip_t_size;
		ips->ip = xrealloc(ips->ip, size);
		xmemzero(ips->ip + i, detect.tsize.data_ip_t_size);
		strncpy(ips->ip[i].ip, ip, BUF_SIZE_16);
		ips->ip[i].iptype = IPTYPE_DNS;
		ips->num++;
		ip = strtok_r(NULL, ":|", &saveptr);	
		if (NULL == ip)
			break;
	}
	if (0 == ips->num)
		return SUCC;
	hashtableCheck1(hostname, ips, ftimes);
	return SUCC;
}

static void customDnsFileOpen(struct file_handler *fps)
{
	FILE * fp;
	int i, ret;
	glob_t glob_res;
	char pattern[BUF_SIZE_1024];

	xmemzero(pattern, BUF_SIZE_1024);
	snprintf(pattern, BUF_SIZE_1024,
			"%s/*_origdomain.conf", CUSTOM_CONF_DIR);

	ret = glob(pattern, GLOB_NOSORT, NULL, &glob_res);

	if (ret != 0)
	{
		dlog(WARNING, NOFD, "glob() failed, PATTERN=[%s]", pattern);
		return;
	}

	for (i = 0; i < glob_res.gl_pathc; ++i)
	{
		fp = xfopen(glob_res.gl_pathv[i], "r");
		if(NULL == fp) 
			xabort("customDnsFileOpen: fopen failed for FC origdomain.conf");
		fps->fp[fps->num] = fp;
		fps->num++;
		if (checkConfigNumberOver(fps))
			break;
	}
	globfree(&glob_res);
}

static void getDnsConfFileHandler(struct file_handler *fps)
{
	FILE *fp = NULL;

	xmemzero(fps, sizeof(struct file_handler));
	fp = xfopen(detect.config.domain.dnsIpFile, "r");
	if(NULL == fp) 
		return;
	fps->fp[fps->num] = fp;
	fps->num++;
	if (checkConfigNumberOver(fps))
		return;
	customDnsFileOpen(fps);
}

static int parseDnsIpFile(void)
{
	FILE *fp;
	int index, nline;
	char line[BUF_SIZE_8192];
	char backup[BUF_SIZE_8192];
	struct file_handler fps;

	index= 0;
	nline = 1;
	getDnsConfFileHandler(&fps);
	fp = fps.fp[index];

	if (fps.num <= 0)
		return 0;

	do {   
		xmemzero(line, BUF_SIZE_8192);
		if (NULL == fgets(line, BUF_SIZE_8192, fp))
		{   
			if (!feof(fp))
			{
				nline--;
				continue;
			}
			fclose(fp);
			if (++index >= fps.num)
				break;
			nline = 0;
			fp = fps.fp[index];
			continue;
		}    
		if (isInvalidConfigLine(line))
			continue;
		clearNewlineSymbol(line);
		strncpy(backup, line, BUF_SIZE_8192);
		if (FAILED == parseDnsIpRecordFileOneLine(line))
		{
			dlog(WARNING, NOFD, 
					"parseDnsIpRecordFileOneLine() failed, LINENO=[%d], DNSLINE=[%s]\n", 
					nline, backup);
			continue;
		}
	} while (++nline);

	return SUCC;
}

static int findDnsIp(detect_cfg_t *cfg)
{
	if (NULL == hashtable1)
	{
		hashtableCreate1();
		parseDnsIpFile();
	}
	return hashtableFind1(cfg);
}

//static int hashtableCheck(detect_cfg_t *cfg)
static int hashtableCheck(const char *hostname)
{
	unsigned int pos = 0; 
	hash_factor_t **head, *node, *cur;

	pos = hashKey(hostname, MAX_HASH_NUM);
	head = &hashtable[pos];
	for (cur = *head; cur != NULL; cur = cur->next)
	{    
		if (!strcmp(cur->uri, hostname))
			return 1;
	}    
	node = xcalloc(1, detect.tsize.hash_factor_t_size);
	strcpy(node->uri, hostname);
	if (NULL == *head)
		node->next = NULL;
	else
		node->next = *head;
	*head = node;

	return 0;
}

static void hashtableSafeFree(void)
{
	int i;
	hash_factor_t *cur, *next;

	for (i = 0; i < MAX_HASH_NUM; ++i)
	{
		for (cur = hashtable[i]; cur != NULL; cur = next)
		{
			next = cur->next;
			xfree((void *)&cur);
		}
		hashtable[i] = NULL;
	}
	xfree((void *)&hashtable);
}

static int hashtableCheck1(const char *hostname, data_ips_t *ips, uint32_t ftimes)
{
	unsigned int pos = 0; 
	hash_factor_t **head, *node, *cur;

	pos = hashKey(hostname, MAX_HASH_NUM);
	head = &hashtable1[pos];
	for (cur = *head; cur != NULL; cur = cur->next)
	{    
		if (!strcmp(cur->uri, hostname))
			return 1;
	}    
	node = xcalloc(1, detect.tsize.hash_factor_t_size);
	strcpy(node->uri, hostname);
	node->rfc = ips;
	node->ftimes = ftimes;
	if (NULL == *head)
		node->next = NULL;
	else
		node->next = *head;
	*head = node;

	return 0;
}

static void backupIpAppend(data_ips_t *ips, data_ip_t *dip, int num)
{
	int i;
	size_t size = 0;

	if (NULL == ips || num <= 0 || NULL == dip)
	{
		return;
	}
	for (i = 0; i < num; ++i)
	{
		if (IPTYPE_BACKUP == dip[i].iptype)			
		{
			size = (ips->num + 1) * detect.tsize.data_ip_t_size;
			ips->ip = xrealloc(ips->ip, size);
			xmemzero(ips->ip + ips->num, detect.tsize.data_ip_t_size);
			strncpy(ips->ip[ips->num].ip, dip[i].ip, BUF_SIZE_16);
			ips->ip[ips->num].iptype = IPTYPE_BACKUP;
			ips->num++;
		}
	}
}

static int hashtableFind1(detect_cfg_t *cfg)
{
	int num = 0;
	unsigned int pos = 0; 
	hash_factor_t **head, *cur;
	const char *hostname = cfg->hostname;
	data_ips_t *ips = NULL;
	data_ip_t *dip = NULL;

	pos = hashKey(hostname, MAX_HASH_NUM);
	head = &hashtable1[pos];
	for (cur = *head; cur != NULL; cur = cur->next)
	{    
		if (!strcmp(cur->uri, hostname))
		{
			ips = cur->rfc;
			num = cfg->ips.num;
			dip = cfg->ips.ip;
			cfg->ips.num = 0;
			cfg->ips.ip = NULL;
			if (cfg->dns_times < cur->ftimes)
			{
				backupIpAppend(&cfg->ips, dip, num);
				free(dip);
				return 0;
			}
			cfg->ips.num = ips->num;
			cfg->ips.ip = ips->ip;
			backupIpAppend(&cfg->ips, dip, num);
			free(dip);
			return 1;
		}
	}    

	return 0;
}

static void hashtableSafeFree1(void)
{
	int i;
	hash_factor_t *cur, *next;

	if (NULL == hashtable1)
		return;

	for (i = 0; i < MAX_HASH_NUM; ++i)
	{
		for (cur = hashtable1[i]; cur != NULL; cur = next)
		{
			if (cur->rfc != NULL)
				free(cur->rfc);
			next = cur->next;
			xfree((void *)&cur);
		}
		hashtable1[i] = NULL;
	}
	xfree((void *)&hashtable1);
}

static int resultParsing(detect_ip_t *dip, char *line, const char *backup)
{
	int i = 0;
	char *lasts, *token;
	const char *delimit = " |\t\r\n";

	do {
		token = i ? strtok_r(NULL, delimit, &lasts) : strtok_r(line, delimit, &lasts);
		switch (i)
		{
			case 0:
				if (NULL == token)
				{
					dlog(ERROR, NOFD, "resultParsing <domain name> error, LINE=[%s]\n", backup);
					return FAILED;
				}
				if (strcmp(dip->rfc->hostname, token))
					return FAILED;
				break;
			case 2:
				if (NULL == token)
				{
					dlog(ERROR, NOFD, "resultParsing <server ip> error, LINE=[%s]\n", backup);
					return FAILED;
				}
				if (strcmp(dip->ip, token))
					return FAILED;
				break;
			case 4:
				if (NULL == token)
				{
					dlog(ERROR, NOFD, "resultParsing <connect time> error, LINE=[%s]\n", backup);
					return FAILED;
				}
				dip->dop.cutime = atof(token);
				break;
			case 5:
				if (NULL == token)
				{
					dlog(ERROR, NOFD, "resultParsing <first byte> error, LINE=[%s]\n", backup);
					return FAILED;
				}
				dip->dop.fbtime = atof(token);
				break;
			case 6:
				if (NULL == token)
				{
					dlog(ERROR, NOFD, "resultParsing <download time> error, LINE=[%s]\n", backup);
					return FAILED;
				}
				dip->dop.dltime = atof(token);
				break;
			case 7:
				if (NULL == token)
				{
					dlog(ERROR, NOFD, "resultParsing <total time> error, LINE=[%s]\n", backup);
					return FAILED;
				}
				dip->dop.ustime = atof(token);
				break;
			case 8:
				if (NULL == token)
				{
					dlog(ERROR, NOFD, "resultParsing <detect times> error, LINE=[%s]\n", backup);
					return FAILED;
				}
				dip->dop.times = atoi(token);
				break;
			case 9:
				if (NULL == token)
				{
					dlog(ERROR, NOFD, "resultParsing <detect oktimes> error, LINE=[%s]\n", backup);
					return FAILED;
				}
				dip->dop.oktimes = atoi(token);
				break;
			case 10:
				if (NULL == token)
				{
					dlog(ERROR, NOFD, "resultParsing <port> error, LINE=[%s]\n", backup);
					return FAILED;
				}
				dip->dop.port = atoi(token);
				break;
			case 11:
				if (NULL == token)
				{
					dlog(ERROR, NOFD, "resultParsing <port state> error, LINE=[%s]\n", backup);
					return FAILED;
				}
				dip->dop.pstate = xstrdup(token);
				break;
			case 12:
				if (NULL == token)
				{
					dlog(ERROR, NOFD, "resultParsing <detect way> error, LINE=[%s]\n", backup);
					return FAILED;
				}
				dip->dop.dway = xstrdup(token);
				break;
			case 13:
				if (NULL == token)
				{
					dlog(ERROR, NOFD, "resultParsing <response code> error, LINE=[%s]\n", backup);
					return FAILED;
				}
				strncpy(dip->dop.code, token, 3);
				dip->dop.code[3] = '\0';
				//all last result data parsed, return now
				return SUCC;
			default:
				if (NULL == token)
				{
					dlog(ERROR, NOFD, "resultParsing <other field> error, LINE=[%s]\n", backup);
					return FAILED;
				}
				break;
		}
	} while (++i);
	//if parse ok, will return in front
	return FAILED;
}

static int lastResultParsing(detect_ip_t *dip)
{
	char *ptr;
	int end; 
	char line[BUF_SIZE_4096];
	char backup[BUF_SIZE_4096];

	//reset the lastfp to the beginning
	fseek(detect.other.lastfp, 0, SEEK_SET);

	do {
		xmemzero(line, BUF_SIZE_4096);
		if (NULL == fgets(line, BUF_SIZE_4096, detect.other.lastfp))
		{
			if (feof(detect.other.lastfp))
				break;
			continue;
		}    
		ptr = line;
		while (' ' == *ptr || '\t' == *ptr) ++ptr;
		if ('\0' == *ptr || '\n' == *ptr || '\r' == *ptr)
			continue;
		end = strlen(line) - 1;
		if ('\n' == line[end])
			line[end] = '\0';
		strncpy(backup, line, BUF_SIZE_4096);
		if (SUCC == resultParsing(dip, line, backup))
			return SUCC;
	} while (1);
	//if parse ok, will return in front
	return FAILED;
}

int keepLastResult(detect_ip_t *dip)
{
	if (NULL == detect.other.lastfp)
		return FAILED;
	return lastResultParsing(dip);
}

static int parseDomainConfOneLine(detect_cfg_t *cfg, char *line, const char *backup, int nline)
{
	int ret;
	char *token, *hostname;
	char info[40960] = {0};
	char *errstr = NULL;	

	xmemzero(cfg, detect.tsize.detect_cfg_t_size);
	//1: domain name
	if (NULL == (token = strtok(line, " \t")))
	{
		dlog(ERROR, NOFD,
				"parsing <domain name> error:strtok is NULL, LINENO=[%d], CONFLINE=[%s]\n",
				nline, backup);
		errstr = "<domain name> is NULL";
		goto fail_info;
		//return FAILED;
	}
	ret = strlen(token);
	if (token[ret - 1] != '.')
	{
		hostname = xcalloc(ret + 2, 1);
		memcpy(hostname, token, ret);
		//append '.' to full domain name
		hostname[ret] = '.';
		hostname[ret + 1] = '\0';
	}
	else
	{
		hostname = xstrndup(token, ret);
	}
	//check repeat domain name
	if (hashtableCheck(hostname))
	{
		xfree((void *)&hostname);
		dlog(ERROR, NOFD,
				"parsing <domain> error:domain name repeat, LINENO=[%d], CONFLINE=[%s]\n",
				nline, backup);
		errstr = "<domain name> is repeat";
		goto fail_info;
		//return FAILED;
	}
	cfg->hostname = hostname;
	//2: info
	if (NULL == strtok(NULL, " \t"))
	{
		dlog(ERROR, NOFD,
				"parsing <info> error, LINENO=[%d], CONFLINE=[%s]\n",
				nline, backup);
		errstr = "<info> is NULL";
		goto fail_info;
		//return FAILED;
	}
	//3: detect, [value: Y: detect; N: no detect]
	if (NULL == (token = strtok(NULL, " \t")))
	{
		dlog(ERROR, NOFD,
				"parsing <detect> error, LINENO=[%d], CONFLINE=[%s]\n",
				nline, backup);
		errstr = "<detect> is NULL";
		goto fail_info;
		//return FAILED;
	}
	//0: no detect; 1: detect; 2: keep using lastest detect result
	cfg->flags.detect = !strncasecmp(token, "Y", 1);
	//4: detect times
	if (NULL == (token = strtok(NULL, " \t")))
	{
		dlog(ERROR, NOFD,
				"parsing <detect times> error, LINENO=[%d], CONFLINE=[%s]\n",
				nline, backup);
		errstr = "<detect times> is NULL";
		goto fail_info;
		//return FAILED;
	}
	cfg->times = getDetectTimes(atoi(token));
	//5: wntime
	if (NULL == (token = strtok(NULL, " \t")))
	{
		dlog(ERROR, NOFD,
				"parsing <warning time> error, LINENO=[%d], CONFLINE=[%s]\n",
				nline, backup);
		errstr = "<warning time> is NULL";
		goto fail_info;
		//return FAILED;
	}
	cfg->wntime = atof(token);
	//6: gdtime
	if (NULL == (token = strtok(NULL, " \t")))
	{
		dlog(ERROR, NOFD,
				"parsing <good time> error, LINENO=[%d], CONFLINE=[%s]\n",
				nline, backup);
		errstr = "<good time> is NULL";
		goto fail_info;
		//return FAILED;
	}
	cfg->gdtime = atof(token);
	//7: rlength
	if (NULL == (token = strtok(NULL, " \t")))
	{
		dlog(ERROR, NOFD,
				"parsing <request length> error, LINENO=[%d], CONFLINE=[%s]\n",
				nline, backup);
		errstr = "<request length> is NULL";
		goto fail_info;
		//return FAILED;
	}
	cfg->rlength = requestContentLength(atoi(token));
	//8: ngood
	if (NULL == (token = strtok(NULL, " \t")))
	{
		dlog(ERROR, NOFD,
				"parsing <good ip number> error, LINENO=[%d], CONFLINE=[%s]\n",
				nline, backup);
		errstr = "<good ip number> is NULL";
		goto fail_info;
		//return FAILED;
	}
	cfg->ngood = atoi(token);
	//9: modify, [value: Y:modify; N:no modify]
	if (NULL == strtok(NULL, " \t"))
	{
		dlog(ERROR, NOFD,
				"parsing <modify> error, LINENO=[%d], CONFLINE=[%s]\n",
				nline, backup);
		errstr = "<modify> is NULL";
		goto fail_info;
		//return FAILED;
	}
	//10: backup
	if (NULL == (token = strtok(NULL, " \t")))
	{
		dlog(ERROR, NOFD,
				"parsing <backup> error:strtok is NULL, LINENO=[%d], CONFLINE=[%s]\n",
				nline, backup);
		errstr = "<backup> is NULL";
		goto fail_info;
		//return FAILED;
	}
	if (FAILED == parseForIps(IPTYPE_BACKUP, token, cfg))
	{
		dlog(ERROR, NOFD,
				"parsing <backup> error:backup ip, LINENO=[%d], CONFLINE=[%s]\n",
				nline, backup);
		errstr = "<back ip> is error";
		goto fail_info;
		//return FAILED;
	}
	//11: request1
	if (NULL == (token = strtok(NULL, " \t")))
	{
		dlog(ERROR, NOFD,
				"parsing <request1> error:strtok is NULL, LINENO=[%d], CONFLINE=[%s]\n",
				nline, backup);
		errstr = "<request1> is NULL";
		goto fail_info;
		//return FAILED;
	}
	//method1
	//ret = parseMethod(token, METHOD_ONE, cfg); 
	cfg->method1 = xcalloc(1, sizeof(struct method_st));
	if (parseMethod(token, cfg->method1, cfg->hostname) < 0)
	{
		dlog(ERROR, NOFD,
				"parsing <request1> error:method1, LINENO=[%d], CONFLINE=[%s]\n",
				nline, backup);
		xfree((void *)&cfg->method1);
		errstr = "<request1 formate> is error";
		goto fail_info;
		//return FAILED;
	}
	//12: codes
	if (NULL == (token = strtok(NULL, " \t")))
	{
		dlog(ERROR, NOFD,
				"parsing <codes> error, LINENO=[%d], CONFLINE=[%s]\n",
				nline, backup);
		errstr = "<codes> is NULL";
		goto fail_info;
		//return FAILED;
	}
	getSuccCodes(cfg, token);
	//13: ip
	if (NULL == (token = strtok(NULL, " \t\r\n")))
	{
		dlog(ERROR, NOFD,
				"parsing <ip> error:strtok is NULL, LINENO=[%d], CONFLINE=[%s]\n",
				nline, backup);
		errstr = "<ip> is NULL";
		goto fail_info;
		//return FAILED;
	}
	if (FAILED == parseForIps(IPTYPE_IP, token, cfg))
	{
		dlog(ERROR, NOFD,
				"parsing <ip> error:ip, LINENO=[%d], CONFLINE=[%s]\n",
				nline, backup);
		errstr = "<ip> is error";
		goto fail_info;
		//return FAILED;
	}
	//set default value
	cfg->rwt = 0;                  //detect with referer in penetrate detection
	cfg->cwt = 100;                //default use common detect in 13 fields configuration
	cfg->tcp_itime = 0;
	cfg->http_itime = 0;
	cfg->flags.sort = 1;           //default: sort by detect time
	cfg->flags.referer = 0;        //detect without referer header
	cfg->port = DETECT_PORT_DFT;   //default use port 80
	cfg->flags.protocol = PROTOCOL_HTTP;
	//if already set global detect timeout, first use it 
	cfg->timeout = (detect.opts.timeout < DOUBLE_NUMBER_MIN) ? DETECT_TIMEOUT : detect.opts.timeout;
	if(NULL == (token = strtok(NULL, " \t\r\n")))
		return SUCC;              //if 13 feilds configuration, here return; else, continue
	//14: request2[method2]
	//ret = parseMethod(token, METHOD_TWO, cfg);
	cfg->method2 = xcalloc(1, sizeof(struct method_st));
	if (parseMethod(token, cfg->method2, cfg->hostname) < 0)
	{
		dlog(ERROR, NOFD,
				"parsing <request2> error:method2, LINENO=[%d], CONFLINE=[%s]\n",
				nline, backup);
		xfree((void *)&cfg->method2);
		errstr = "<reqeust2> is NULL";
		goto fail_info;
		//return FAILED;
	}
	//15: port
	if(NULL == (token = strtok(NULL, " \t")))
	{
		dlog(ERROR, NOFD,
				"parsing <port> error, LINENO=[%d], CONFLINE=[%s]\n",
				nline, backup);
		errstr = "<port> is NULL";
		goto fail_info;
		//return FAILED;
	}
	cfg->port = getDetectPort(atoi(token));
	cfg->flags.protocol = (HTTPS_PORT == cfg->port) ? PROTOCOL_HTTPS : PROTOCOL_HTTP;
	//16: whether use domain resolution
	if (NULL == (token = strtok(NULL, " \t")))
	{
		dlog(ERROR, NOFD,
				"parsing <use dns> error, LINENO=[%d], CONFLINE=[%s]\n",
				nline, backup);
		errstr = "<use dns> is NULL";
		goto fail_info;
		//return FAILED;
	}
	cfg->flags.usedns = !strncasecmp(token, "Y", 1);
	//proportion between common and penetrate detection
	//17: cwt: common; rwt: penetrate; cwt + rwt = 100
	if (NULL == (token = strtok(NULL, " \t")))
	{
		dlog(ERROR, NOFD,
				"parsing <weight> error:strtok is NULL, LINENO=[%d], CONFLINE=[%s]\n",
				nline, backup);
		errstr = "<weight> is NULL";
		goto fail_info;
		//return FAILED;
	}
	char* p1 = strstr(token, ":");
	if (NULL == p1)
	{
		dlog(ERROR, NOFD,
				"parsing <weight> error:lacked ':', LINENO=[%d], CONFLINE=[%s]\n",
				nline, backup);
		errstr = "<weight> is NULL";
		goto fail_info;
		//return FAILED;
	}
	cfg->cwt = atoi(token);
	cfg->rwt = atoi(p1 + 1);
	if (cfg->cwt + cfg->rwt != 100)
	{
		dlog(ERROR, NOFD,
				"parsing <weight> error:cwt + rwt != 100, LINENO=[%d], CONFLINE=[%s]\n",
				nline, backup);
		errstr = "<weitht>error: cwt + rwt != 100";
		goto fail_info;
		//return FAILED;
	}
	if (cfg->rwt > 0)
		cfg->flags.referer = 1;
	//18: dns reuse times
	if (NULL == (token = strtok(NULL, " \t")))
	{
		dlog(ERROR, NOFD,
				"parsing <dns reuse times> error, LINENO=[%d], CONFLINE=[%s]\n",
				nline, backup);
		errstr = "<dns resuse times >is error";
		goto fail_info;
		//return FAILED;
	}
	cfg->dns_times = atoi(token);
	//19: ip sort
	if (NULL == (token = strtok(NULL, " \t\r\n")))
	{
		dlog(ERROR, NOFD,
				"parsing <ip sort> error, LINENO=[%d], CONFLINE=[%s]\n",
				nline, backup);
		errstr = "<ip sort> is NULL";
		goto fail_info;
		//return FAILED;
	}
	cfg->flags.sort = !strncasecmp(token, "Y", 1);
	//20: how long to use tcp detect
	if (NULL != (token = strtok(NULL, " \t")))
	{
		int itime = atoi(token);
		cfg->tcp_itime = (itime < 0) ? 0 : itime;
		//21: how long to use http detect
		if (NULL == (token = strtok(NULL, " \t\r\n")))
		{
			dlog(ERROR, NOFD,
					"parsing <http interval time> error, LINENO=[%d], CONFLINE=[%s]\n",
					nline, backup);
			errstr = "<http interval time> is NULL";
			goto fail_info;
			//return FAILED;
		}
		itime = atoi(token);
		cfg->http_itime = (itime < 0) ? 0 : itime;
		chooseDetectWay(cfg);
		if (PROTOCOL_NONE == cfg->flags.protocol)
			cfg->flags.detect = 2;
		if (PROTOCOL_TCP == cfg->flags.protocol && cfg->flags.referer)
			dlog(WARNING, NOFD, "configuration conflict: TCP and Referer coexist");
		if (NULL != (token = strtok(NULL, " \t\r\n")))
		{
			if (detect.opts.timeout < DOUBLE_NUMBER_MIN)
				cfg->timeout = atof(token);
		}
	}
	if (cfg->flags.usedns)
		findDnsIp(cfg);
	if (0 == cfg->ips.num)
	{
		dlog(ERROR, NOFD,
				"parsing error:ip number is 0, LINENO=[%d], CONFLINE=[%s]\n",
				nline, backup);
		errstr = "ip number is 0";
		goto fail_info;
		//return FAILED;
	}
	return SUCC;
fail_info:
	if (detect.opts.check_flag)
	{
		snprintf(info, 40960, "[%d] [%s] [%s]\n", nline, backup, errstr);
		write_rcms_log(RCMS_LOG, info);
		fprintf(stderr,"'%s' ERROR\n", backup);
	}
	return FAILED;

}

static void customDomainFileOpen(struct file_handler *fps)
{
	FILE * fp;
	int i, ret;
	glob_t glob_res;
	char pattern[BUF_SIZE_1024];

	xmemzero(pattern, BUF_SIZE_1024);
	snprintf(pattern, BUF_SIZE_1024,
			"%s/*_domain.conf", CUSTOM_CONF_DIR);

	ret = glob(pattern, GLOB_NOSORT, NULL, &glob_res);

	if (ret != 0)
	{
		dlog(WARNING, NOFD, "glob() failed, PATTERN=[%s]", pattern);
		return;
	}

	for (i = 0; i < glob_res.gl_pathc; ++i)
	{
		fp = xfopen(glob_res.gl_pathv[i], "r");
		if(NULL == fp) 
			xabort("customDomainFileOpen: fopen failed for FC custom domain.conf");
		fps->fp[fps->num] = fp;
		fps->num++;
		if (checkConfigNumberOver(fps))
			break;
	}
	globfree(&glob_res);
}

static void getConfFileHandler(struct file_handler *fps)
{
	FILE *fp = NULL;

	xmemzero(fps, sizeof(struct file_handler));

	if (MODE_FC & detect.config.domain.mode)
	{
		fp = xfopen(detect.config.domain.domainFile, "r");
		if(NULL == fp) 
			xabort("getConfFileHandler: fopen failed for FC domain.conf");
		fps->fp[fps->num] = fp;
		fps->num++;
		if (checkConfigNumberOver(fps))
			return;
	}
	if (MODE_TTA & detect.config.domain.mode)
	{
		fp = xfopen(detect.config.domain.ttaConfFile, "r");
		if(NULL == fp)
			xabort("getConfFileHandler: fopen failed for TTA domain.conf");
		fps->fp[fps->num] = fp;
		fps->num++;
		if (checkConfigNumberOver(fps))
			return;
	}

	customDomainFileOpen(fps);
}

static void domainParser(void)
{
	FILE *fp = NULL;
	int nline, index, count; 
	char line[BUF_SIZE_8192];
	char backup[BUF_SIZE_8192];
	struct file_handler fps;
	detect_cfg_t cfg;

	index= 0;
	nline = 1;
	count = 0;
	getConfFileHandler(&fps);
	fp = fps.fp[index];
	hashtableCreate();

	do {
		runningSched(50);
		xmemzero(line, BUF_SIZE_8192);
		if (NULL == fgets(line, BUF_SIZE_8192, fp))
		{
			if (!feof(fp))
			{
				nline--;
				continue;
			}
			fclose(fp);
			if (++index >= fps.num)
				break;
			nline = 0;
			fp = fps.fp[index];
			continue;
		}    
		count += 1;
		if (isInvalidConfigLine(line))
			continue;
		clearNewlineSymbol(line);
		strncpy(backup, line, BUF_SIZE_8192);
		if (FAILED == parseDomainConfOneLine(&cfg, line, backup, nline))
		{
			continue;
		}
		llistInsert(detect.data.cllist, &cfg, TAIL);
	} while (++nline);

	hashtableSafeFree();
	hashtableSafeFree1();
}

static int registerKeyword(const char *keyword, CKCB *handler)
{
	size_t size;
	keyword_t *keys;

	keys = &detect.config.ctl.keys;
	if (keys->num <= 0)
	{
		keys->num = 0;
		keys->keywords = xcalloc(1, detect.tsize.ctl_keyword_t_size);
	}
	else
	{
		size = (keys->num + 1) * detect.tsize.ctl_keyword_t_size;
		keys->keywords = xrealloc(keys->keywords, size);
	}
	xmemzero(keys->keywords + keys->num, detect.tsize.ctl_keyword_t_size);
	keys->keywords[keys->num].handler = handler;
	strncpy(keys->keywords[keys->num].keyword, keyword, BUF_SIZE_128);
	keys->num++;

	return SUCC;
}

static int ctlAddHeader(char *hostname, const char *name, const char *value)
{
	size_t size;
	detect_cfg_t *cfg;
	http_headers_t *hdrs;

	cfg = llistFind2(detect.data.cllist, hostname, hostnameCmp);
	if (NULL == cfg)
	{
		dlog(WARNING, NOFD, 
				"no this hostname in domain.conf for header in control.conf, HOSTNAME=[%s]",
				hostname);
		return FAILED;
	}
	hdrs = &cfg->hdrs;
	if (hdrs->num <= 0)
	{
		hdrs->num = 0;
		hdrs->hdrs = xcalloc(1, detect.tsize.http_header_t_size);
	}
	else
	{
		size = (hdrs->num + 1) * detect.tsize.http_header_t_size;
		hdrs->hdrs = xrealloc(hdrs->hdrs, size);
	}
	xmemzero(hdrs->hdrs + hdrs->num, detect.tsize.http_header_t_size);
	strncpy(hdrs->hdrs[hdrs->num].name, name, BUF_SIZE_256);
	strncpy(hdrs->hdrs[hdrs->num].value, value, BUF_SIZE_512);
	hdrs->num++;

	return SUCC;
}

static int ctlHeaderHandle(char *str)
{
	int i;
	char *ptr, *ptr1;
	char name[BUF_SIZE_256];
	char value[BUF_SIZE_512];
	char hostname[BUF_SIZE_256];

	i = 0;
	ptr1 = str;

	do {
		ptr = strchr(ptr1, '\"');
		if (NULL == ptr)
			return FAILED;
		++ptr;
		ptr1 = strchr(ptr, '\"');
		if (NULL == ptr1)
			return FAILED;
		switch (i)
		{
			case 0:
				xmemzero(hostname, BUF_SIZE_256);
				strncpy(hostname, ptr, ptr1 - ptr);
				break;
			case 1:
				xmemzero(name, BUF_SIZE_256);
				strncpy(name, ptr, ptr1 - ptr);
				break;
			case 2:
				xmemzero(value, BUF_SIZE_512);
				strncpy(value, ptr, ptr1 - ptr);
				break;
			default:
				dlog(ERROR, NOFD, "no this type, TYPE=[%d]", i);
				break;
		}
		++ptr1;
	} while (++i < 3);
	if ('.' != hostname[strlen(hostname) - 1])
		strcat(hostname, ".");
	ctlAddHeader(hostname, name, value);

	return SUCC;
}

static int ctlCentralHandle(char *str)
{
	char *ptr = str;

	while (' ' == *ptr || '\t' == *ptr) ++ptr;

	if (!strncasecmp(ptr, "on", 2))
		detect.opts.F.central = 1;
	else if (!strncasecmp(ptr, "off", 3))
		detect.opts.F.central = 0;
	else
	{
		dlog(WARNING, NOFD, "configuration for keyword 'central' error");
		return FAILED;
	}

	return SUCC;
}

static int ctlTravelHandle(char *str)
{
	detect.opts.travel = atoi(str);
	return SUCC;
}

static int ctlDNSserviceHandle(char *str)
{
	if (strlen(str) > 0 && NULL != detect.opts.DNSservice)
		free(detect.opts.DNSservice);
	detect.opts.DNSservice = xstrdup(str);
	return SUCC;
}

static int ctlRlimitHandle(char *str)
{
	detect.opts.rlimit = (rlim_t)atol(str);
	return SUCC;
}

static int ctlEpollwaitHandle(char *str)
{
	detect.opts.epollwait = atoi(str);
	return SUCC;
}

static int ctlKilltimeHandle(char *str)
{
	detect.opts.killtime = atol(str);
	return SUCC;
}

static int ctlSavetimeHandle(char *str)
{
	char *ptr = &str[strlen(str) - 1];

	while (' ' == *ptr || '\t' == *ptr) --ptr;

	switch (*ptr)
	{       
		case 's':
		case 'S':
			detect.opts.savetime = atol(str);
			break;  
		case 'm':
		case 'M':
			detect.opts.savetime = atol(str) * 60;
			break;  
		case 'h':
		case 'H':
			detect.opts.savetime = atol(str) * 60 * 60;
			break;  
		case 'd':
		case 'D':
			detect.opts.savetime = atol(str) * 24 * 60 * 60;
			break;  
		default:
			detect.opts.savetime = SAVETIME_MIN;
			break;  
	}       
	if (detect.opts.savetime < SAVETIME_MIN)
		detect.opts.savetime = SAVETIME_MIN;

	return SUCC;
}

static int ctlResultHandle(char *str)
{
	char *ptr = str;

	while (' ' == *ptr || '\t' == *ptr) ++ptr;

	if (!strncasecmp(ptr, "on", 2))
		detect.opts.F.result = 1;
	else if (!strncasecmp(ptr, "off", 3))
		detect.opts.F.result = 0;
	else
	{
		dlog(WARNING, NOFD, "configuration for keyword 'result' error");
		return FAILED;
	}

	return SUCC;
}

static int ctlCleanupHandle(char *str)
{
	char *ptr = str;

	while (' ' == *ptr || '\t' == *ptr) ++ptr;

	if (!strncasecmp(ptr, "on", 2))
		detect.opts.F.cleanup = 1;
	else if (!strncasecmp(ptr, "off", 3))
		detect.opts.F.cleanup = 0;
	else
	{
		dlog(WARNING, NOFD, "configuration for keyword 'cleanup' error");
		return FAILED;
	}

	return SUCC;
}

static void ctlRegisterKeywords(void)
{
	//add new keyword here, register keyword name and parse handler
	registerKeyword("header", ctlHeaderHandle);
	registerKeyword("travel", ctlTravelHandle);
	registerKeyword("rlimit", ctlRlimitHandle);
	registerKeyword("result", ctlResultHandle);
	registerKeyword("central", ctlCentralHandle);
	registerKeyword("cleanup", ctlCleanupHandle);
	registerKeyword("killtime", ctlKilltimeHandle);
	registerKeyword("savetime", ctlSavetimeHandle);
	registerKeyword("epollwait", ctlEpollwaitHandle);
	registerKeyword("dnsservice", ctlDNSserviceHandle);
}

static int handleCtlKeyValue(const char *keyword, char *str)
{
	int i;
	keyword_t *keys;

	keys = &detect.config.ctl.keys;
	for (i = 0; i < keys->num; ++i)
	{
		if (!strcmp(keys->keywords[i].keyword, keyword))	
			return keys->keywords[i].handler(str);
	}
	return FAILED;
}

static int parseControlOneLine(char *line)
{
	int flag;
	char *token, *str;

	//printf("line: %s\n", line);
	flag = (NULL == strchr(line, '}'));
	token = strtok(line, " =\t\n");
	if (NULL == token)
		return FAILED;
	str = flag ? strtok(NULL, "=;\n") : strtok(NULL, "=}\n");
	if (NULL == str || '\0' == str[0])
		return FAILED;
	//printf("keyword: %s, value: %s\n", token, str);
	handleCtlKeyValue(token, str);

	return SUCC;
}

static int parseForControl(void)
{
	FILE *fp;
	int nline;
	char *ptr;
	char line[BUF_SIZE_4096];
	char backup[BUF_SIZE_4096];

	if (NULL == (fp = xfopen(CONFIG_CONTROL, "r")))
		return FAILED;
	nline = 0;
	while (!feof(fp))
	{
		++nline;
		xmemzero(line, BUF_SIZE_4096);
		if (NULL == fgets(line, BUF_SIZE_4096, fp))
			continue;
		ptr = line;
		while (' ' == *ptr || '\t' == *ptr) ++ptr;
		if ('#' == *ptr || '\n' == *ptr || '\r' == *ptr || '\0' == *ptr)
			continue;
		xmemzero(backup, BUF_SIZE_4096);
		strncpy(backup, line, BUF_SIZE_4096);
		if (FAILED == parseControlOneLine(line))
			continue;
	}

	return SUCC;
}

static int ctlParsing(void)
{
	ctlRegisterKeywords();
	parseForControl();

	return SUCC;
}

//parse all configuration files
static int configParsing(void)
{
	checkConfigFilesValid();
	domainParser();
	ctlParsing();

	return SUCC;
}

static void printConfigOptions(void)
{
	if (detect.opts.F.print)
	{
		fprintf(stdout, " ==> result: %s\n", 
				detect.opts.F.result ? "on" : "off");
		fprintf(stdout, " ==> cleanup: %s\n", 
				detect.opts.F.cleanup ? "on" : "off");
		fprintf(stdout, " ==> central: %s\n",
				detect.opts.F.central ? "on" : "off");
		fprintf(stdout, " ==> travel: %d\n",
				detect.opts.travel);
		fprintf(stdout, " ==> rlimit: %ld\n",
				(long)detect.opts.rlimit);
		fprintf(stdout, " ==> killtime: %ld seconds\n",
				(long)detect.opts.killtime);
		fprintf(stdout, " ==> savetime: %ld seconds\n",
				detect.opts.savetime);
		fprintf(stdout, " ==> epollwait: %d milliseconds\n",
				detect.opts.epollwait);
		fflush(stdout);
	}
}

static void configStart(void)
{
	debug(BEFORE_PARSE);
	configParsing();
	//here reset progress recource limits after configuration parse
	setResourceLimit();
	printConfigOptions();
	debug(AFTER_PARSE);
}

int configCall(const int calltype)
{
	switch (calltype)
	{
		case START_CONFIG:
			configStart();
			if (detect.opts.check_flag)
				return CONFIG_CHECK;
			return STATE_DETECT;
		default:
			dlog(WARNING, NOFD,
					"configCall: no this calltype, TYPE=[%d]",
					calltype);
			return FAILED;
	}

	return SUCC;
}

