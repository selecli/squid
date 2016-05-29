#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK

#define SIZE_4096 4096
#define MATCH_DEFAULT  0
#define MATCH_WILDCARD 1


static cc_module *mod = NULL;
static int hash_size = 65535;
static hash_table *hashtable = NULL;
static struct stat domain_last_stat;
static struct stat domain_whitelist_last_stat;
static const char *domain_conf = "/usr/local/squid/etc/domain.conf";
static const char *domain_whitelist_conf = "/usr/local/squid/etc/domain_whitelist.conf";
static const char *special_domain[] = {
	"*.",
	"*.cn.",
	"*.com.",
	"*.com.cn.",
	"*.gov.",
	"*.gov.cn.",
	"*.edu.",
	"*.edu.cn.",
	"*.net.",
	"*.net.cn.",
	NULL
};

static int checkSpecialDomain(const char *domain)
{
	int i;
	const char *host;

	for (i = 0; NULL != special_domain[i]; ++i)
	{
		host = special_domain[i];
		if (0 == strcmp(host, domain))
		{
			return 1;
		}
	}

	return 0;
}

static void domain_free(void * data)
{
	hash_link *lnk = data;
	xfree(lnk->key);
	xfree(lnk);
}

static int domainHashtableCreate(void)
{
	hashtable = hash_create((HASHCMP *)strcmp, hash_size, hash_string);	
	stat(domain_conf, &domain_last_stat);
	stat(domain_whitelist_conf, &domain_whitelist_last_stat);

	return 0;
}

static void domainHashJoin(const char *domain, const int flag)
{
	size_t len = 0;
	char *host = NULL;
	hash_link *hlk = NULL;

	len = strlen(domain);
	host = xcalloc(1, len + 2); /* "xxx.\0" */
	memcpy(host, domain, len);
	host[len] = '\0';

	/* remove the trailing dots,
	   because of squid remove the dots when parsing request Host */
	while ((len = strlen(host)) > 0 && '.' == host[--len])
	{
		host[len] = '\0';
	}

	/* append one dot '.' */
	len = strlen(host);
	host[len] = '.';
	host[len + 1] = '\0';

	char *p_wildcard_char;

	p_wildcard_char = strchr(host, '*');
	if (NULL != p_wildcard_char)
	{
		switch (flag)
		{
			case MATCH_WILDCARD:
				p_wildcard_char++;
				len = strlen(p_wildcard_char);
				memmove(host, p_wildcard_char, len);
				host[len] = '\0';
				break;
			case MATCH_DEFAULT:
				/* full through */
			default:
				if (0 == checkSpecialDomain(host))
				{
					p_wildcard_char++;
					len = strlen(p_wildcard_char);
					memmove(host, p_wildcard_char, len);
					host[len] = '\0';
				}
				break;
		}
	}

	hlk = xcalloc(1, sizeof(hash_link));
	hlk->key = host;
	hash_join(hashtable, hlk);

	debug(133, 5)("mod_access_filter: hash_join domain: %s\n", host);
}

static void domainHashtableDestroy(void)
{
	debug(133, 5)("mod_access_filter: hashtable destroy\n");

	hashFreeItems(hashtable, domain_free);
	hashFreeMemory(hashtable);
	hashtable = NULL;
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

static int domainHashtableCreateByConfigOne(const char *cfg_file, const int flag)
{
	FILE *fp = NULL;
	char line[SIZE_4096];
	char *domain = NULL;
	char *saveptr = NULL;
	const char *delim = " \t\r\n";

	fp = fopen(cfg_file, "r");
	if (NULL == fp)
	{
		debug(133, 3)("mod_access_filter: fopen(%s) error: %s\n", cfg_file, strerror(errno));
		return -1;
	}

	do {    
		memset(line, 0, SIZE_4096);
		if (NULL == fgets(line, SIZE_4096, fp)) 
		{    
			if (feof(fp))
				break;
			continue;
		}    
		if (isInvalidConfigLine(line))
			continue;
		domain = strtok_r(line, delim, &saveptr);
		if (NULL == domain || '\0' == domain[0])
			continue;
		domainHashJoin(domain, flag);
	} while (1);

	fclose(fp);

	return 0;
}

static int domainHashtableCreateByConfig(void)
{
	/* create the domains hashtable */
	domainHashtableCreate();
	domainHashtableCreateByConfigOne(domain_conf, MATCH_DEFAULT);
	domainHashtableCreateByConfigOne(domain_whitelist_conf, MATCH_WILDCARD);

	return 1;
}

static void domainHashtableRecreate(void)
{
	debug(133, 5)("mod_access_filter: hashtable recreate\n");

	domainHashtableDestroy();
	domainHashtableCreateByConfig();
}

static int hook_func_sys_init(cc_module *module)
{
	debug(133, 3)("mod_access_filter: module init\n");

	domainHashtableCreateByConfig();

	return 1;
}

static int hook_func_sys_cleanup(cc_module *module)
{
	debug(133, 3)("mod_access_filter: module cleanup\n");

	/* destroy the domains hashtable */
	domainHashtableDestroy();

	return 1;
}

static int checkDomainHashtableRecreatable(void)
{
	struct stat st;

	if (NULL == hashtable)
	{
		return 1;
	}

	if (stat(domain_conf, &st) < 0)
	{
		return 1;
	}

	if (st.st_mtime != domain_last_stat.st_mtime
			|| st.st_size != domain_last_stat.st_size)
	{
		return 1;
	}

	if (stat(domain_whitelist_conf, &st) < 0)
	{
		return 1;
	}

	if (st.st_mtime != domain_whitelist_last_stat.st_mtime
			|| st.st_size != domain_whitelist_last_stat.st_size)
	{
		return 1;
	}

	return 0;
}

static int domainHashLookup(const char *host)
{
	const char *p_host = host;

	while (NULL != p_host)
	{
		if (NULL != hash_lookup(hashtable, p_host))
		{
			debug(133, 7)("mod_access_filter: matched host: %s\n", p_host);
			return 1;
		}
		p_host = strchr(p_host + 1, '.');
	}

	return 0;
}

static int func_access_filter(clientHttpRequest *http) 
{
	size_t len;
	char *host;

	if (NULL == http || NULL == http->request)
	{
		return 0;
	}

	if (checkDomainHashtableRecreatable())
	{
		domainHashtableRecreate();
	}

	len = strlen(http->request->host);
	host = xcalloc(1, len + 2);
	memcpy(host, http->request->host, len);
	host[len] = '.';
	host[len + 1] = '\0';

	/* find domain in hashtable */
	if (0 == domainHashLookup(host))
	{
		debug(133, 5)("mod_access_filter: hash_lookup none for Host: %s\n", host);

		ErrorState *err = errorCon(ERR_ACCESS_DENIED, HTTP_FORBIDDEN, http->request);
		http->log_type = LOG_TAG_NONE;
		http->entry = clientCreateStoreEntry(http, http->request->method, null_request_flags);
		errorAppendEntry(http->entry, err);

		xfree(host);
		return 3;
	}

	xfree(host);

	return 0;
}

int mod_register(cc_module *module)
{
	debug(133, 1)("mod_access_filter: init module\n");
	strcpy(module->version, "7.0.R.16488.i686");

	cc_register_hook_handler(HPIDX_hook_func_sys_init,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_init),
			hook_func_sys_init);

	cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_cleanup),
			hook_func_sys_cleanup);

	cc_register_hook_handler(HPIDX_hook_func_http_req_parsed,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_parsed),
			func_access_filter);

	if (reconfigure_in_thread)
	{
		mod = (cc_module *)cc_modules.items[module->slot];
	}
	else
	{
		mod = module;
	}

	return 0;
}

#endif

