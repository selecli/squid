#include "xping.h"

struct file_handler {
	int num;
	FILE *fp[MAX_FP_NUM];
};

//check whether exist or have access permission for configuration files
static void checkConfigFileValid(void)
{
	if (MODE_FC & xping.config.mode)
	{
		if (access(xping.config.domainFile, F_OK|R_OK) < 0)
		{
			fprintf(stderr,
					"checkConfigFileValid() failed, FILE=[%s], not exist or permission denied\r\n",
					xping.config.domainFile);
			xlog(WARNING,
					"checkConfigFileValid() failed, FILE=[%s], not exist or permission denied",
					xping.config.domainFile);
			abort();
		}
	}
	if (MODE_TTA & xping.config.mode)
	{
		if (access(xping.config.ttaConfFile, F_OK|R_OK) < 0)
		{
			fprintf(stderr,
					"checkConfigFileValid() failed, FILE=[%s], not exist or permission denied\r\n",
					xping.config.ttaConfFile);
			xlog(WARNING,
					"checkConfigFileValid() failed, FILE=[%s], not exist or permission denied",
					xping.config.ttaConfFile);
			abort();
		}
	}
	if (access(xping.config.dnsIpFile, R_OK|F_OK) < 0)
	{
		xlog(WARNING,
				"checkConfigFileValid() failed, FILE=[%s], not exist or permission denied",
				xping.config.dnsIpFile);
	}
}

static unsigned int hashKey(const void *value, unsigned int size)
{
	const char *str = value;
	unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
	unsigned int hash = 0;

	while (*str)
	{   
		hash = hash * seed + (*str++);
	}   

	return (hash & 0x7FFFFFFF) % size;
}

static int hashtableCheck(const char *ip)
{
	hash_factor_t **head, *node, *cur;

	head = &hashtable[hashKey(ip, MAX_HASH_NUM)];

	for (cur = *head; cur != NULL; cur = cur->next)
	{    
		if (!strcmp(cur->ip, ip))
			return 1;
	}    
	node = xcalloc(1, xping.tsize.hash_factor_t_size);
	strcpy(node->ip, ip);
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
			free(cur);
		}
		hashtable[i] = NULL;
	}
}

static int checkIpValid(const char *ip)
{
	struct in_addr addr;

	memset(&addr, 0, sizeof(addr));
	if (inet_pton(AF_INET, ip, &addr) <= 0)
		return 0;
	if (0 == addr.s_addr)
		return 0;

	return 1;
}

static void parseForIps(char *ipstr)
{
	char *saveptr, *ip;
	ping_packet_t packet;

	if (NULL == (ip = strtok_r(ipstr, ":|", &saveptr)))
		return;

	do {
		if (!checkIpValid(ip))
		{
			xlog(WARNING, "ip invalid, IP=[%s]", ip);
			continue;
		}
		if (hashtableCheck(ip))
			continue;
		memset(&packet, 0, xping.tsize.ping_packet_t_size);
		strncpy(packet.ip, ip, BUF_SIZE_16);
		packet.npack = 10;
		packet.psize = 64;
		packet.itime = 10;
		llistInsert(xping.list, &packet, TAIL);
	} while (NULL != (ip = strtok_r(NULL, ":|", &saveptr)));
}

static int parseDnsIpRecordFileOneLine(char *line, int times)
{
	char *token;
	char ips[BUF_SIZE_4096];

	if (NULL == strtok(line, " \t"))
		return FAILED;
	if (NULL == strtok(NULL, " \t"))
		return FAILED;
	if (NULL == (token = strtok(NULL, " \t")))
		return FAILED;
	memset(ips, 0, BUF_SIZE_4096);
	strncpy(ips, token, BUF_SIZE_4096);
	if (NULL == (token = strtok(NULL, " \t\n")))
		return FAILED;
	if (times < atoi(token))
		return SUCC;
	parseForIps(ips);

	return SUCC;
}

static int parseDnsIpFile(int times)
{
	FILE *fp;
	char *ptr;
	int ret, nline = 1;
	char line[BUF_SIZE_4096];
	char backup[BUF_SIZE_4096];

	if (NULL == (fp = fopen(xping.config.dnsIpFile, "r")))
		return FAILED;

	do {   
		memset(line, 0, BUF_SIZE_4096);
		if (NULL == fgets(line, BUF_SIZE_4096, fp))
		{   
			if (0 == feof(fp))
				continue;
			fclose(fp);
			break;
		}    
		ptr = line;
		while (' ' == *ptr || '\t' == *ptr) ++ptr;
		if ('#' == *ptr || '\n' == *ptr || '\r' == *ptr || '\0' == *ptr)
			continue;
		ret = strlen(line) - 1;
		if ('\n' == line[ret])
			line[ret] = '\0';
		strcpy(backup, line);
		parseDnsIpRecordFileOneLine(line, times);
	} while (++nline);

	return SUCC;
}

static void parseDomainConfOneLine(char *line)
{
	char *token;

	if (NULL == line)
		return;
	//domain
	if (NULL == strtok(line, " \t"))
		return;
	//isinfo
	if (NULL == strtok(NULL, " \t"))
		return;
	if (NULL == strtok(NULL, " \t"))
		return;
	//detect times
	if (NULL == strtok(NULL, " \t"))
		return;
	//wntime
	if (NULL == strtok(NULL, " \t"))
		return;
	//gdtime
	if (NULL == strtok(NULL, " \t"))
		return;
	//rlen
	if (NULL == strtok(NULL, " \t"))
		return;
	//ngip
	if (NULL == strtok(NULL, " \t"))
		return;
	//ismodify, [value: Y:modify; N:no modify]
	if (NULL == strtok(NULL, " \t"))
		return;
	//backup
	if (NULL == (token = strtok(NULL, " \t")))
		return;
	if (strncasecmp(token, "Y", 1) && strncasecmp(token, "N", 1))
		parseForIps(token);
	//method1
	if (NULL == strtok(NULL, " \t"))
		return;
	//codes
	if (NULL == (token = strtok(NULL, " \t")))
		return;
	//ips
	if (NULL == (token = strtok(NULL, " \t\r\n")))
		return;
	parseForIps(token);
	//set default value
	if(NULL == strtok(NULL, " \t\r\n"))
		return;              //if 13 feilds configuration, here return; else, continue
	//parse for 'port'
	if(NULL == strtok(NULL, " \t"))
		return;
	//whether use domain resolution
	if (NULL == (token = strtok(NULL, " \t")))
		return;
	int usedns = !strncasecmp(token, "Y", 1);
	if (NULL == strtok(NULL, " \t"))
		return;
	if (NULL == (token = strtok(NULL, " \t")))
		return;
	int times = atoi(token);

	if (usedns)
		parseDnsIpFile(times);
}

static void getConfFileHandler(struct file_handler *fps)
{
	FILE *fp = NULL;

	memset (fps, 0, sizeof(struct file_handler));

	if (MODE_FC & xping.config.mode)
	{
		fp = fopen(xping.config.domainFile, "r");
		if(NULL == fp) 
			xabort("getConfFileHandler: fopen failed for FC domain.conf");
		fps->fp[fps->num] = fp;
		fps->num++;
	}
	if (MODE_TTA & xping.config.mode)
	{
		fp = fopen(xping.config.ttaConfFile, "r");
		if(NULL == fp)
			xabort("getConfFileHandler: fopen failed for TTA domain.conf");
		fps->fp[fps->num] = fp;
		fps->num++;
	}
}

static void parseForDomain(void)
{
	char *ptr;
	FILE *fp = NULL;
	int ret, nline, index; 
	char line[BUF_SIZE_4096];
	char backup[BUF_SIZE_4096];
	struct file_handler fps;

	index= 0;
	nline = 1;
	getConfFileHandler(&fps);
	fp = fps.fp[index];
	memset(hashtable, 0, sizeof(hashtable));

	do {
		memset(line, 0, BUF_SIZE_4096);
		if (NULL == fgets(line, BUF_SIZE_4096, fp))
		{
			if (!feof(fp))
				continue;
			fclose(fp);
			if (++index >= fps.num)
				break;
			fp = fps.fp[index];
			continue;
		}    
		ptr = line;
		while (' ' == *ptr || '\t' == *ptr) ++ptr;
		if ('#' == *ptr || '\n' == *ptr || '\r' == *ptr || '\0' == *ptr)
			continue;
		ret = strlen(line) - 1;
		if ('\n' == line[ret])
			line[ret] = '\0';
		strncpy(backup, line, BUF_SIZE_4096);
		parseDomainConfOneLine(line);
	} while (++nline);

	hashtableSafeFree();
}

static void parseConfigFiles(void)
{
	parseForDomain();
}

void configStart(void)
{
	checkConfigFileValid();
	parseConfigFiles();
}

