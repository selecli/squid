/*
 * optimized by xin.yao: 2012-05-24
 * replace 'bsearch' and 'qsort' by 'myHash' for rapid positioning
 * add pointer 'tail' for rapid insertion
 * ------------------------------------------------------------------
 */

#include "squid.h"
#include <regex.h>

#define	PATTERNLINESSIZE	10240

struct defaultLines_st {
	refresh_t *tail;      //mark the end of refresh linklist
	refresh_t *refresh;   //refresh linklist
} defaultLines;

struct optimizedLines_st {
	String host;          //hostname
	refresh_t *tail;      //mark the end of refresh linklist
	refresh_t *refresh;   //refresh linklist
} optimizedLines[PATTERNLINESSIZE];

//optimzed refresh pattern host number
uint32_t refresh_pattern_host_count = 0;
static MemPool * refresh_t_pool = NULL;

static void *refresh_t_pool_alloc(void)
{
	void * obj = NULL;

	if (NULL == refresh_t_pool)
	{
		refresh_t_pool = memPoolCreate("mod_refresh_pattern other_data refresh_t", sizeof(refresh_t));
	}
	return obj = memPoolAlloc(refresh_t_pool);
}

static refresh_t *hook_find_refresh_pattern(const char *url)
{
	unsigned int i;
	char* end = "/";
	char *pChr = NULL;
	const char *host = NULL;
	struct optimizedLines_st *result;
	refresh_t *R;

	pChr = strstr(url, "://");
	if (pChr)
		pChr = strstr(pChr + 3, end);
	if (pChr)
		host = getStringPrefix(url, pChr + 1);
	else
		host = getStringPrefix(url, NULL);
	if (!host)
	{
		debug(117, 1)("(mod_refresh_pattern)->hook_find_refresh_pattern: !find host, url = %s\n", url);
		return NULL;
	}
	i = myHash((char *)host, PATTERNLINESSIZE);
	result = optimizedLines + i;
	if (result)
	{
		for (R = result->refresh; R; R = R->next)
		{
			if (!regexec(&(R->compiled_pattern), url, 0, 0, 0))
				return R;
		}
	}
	for (R = defaultLines.refresh; R; R = R->next)
	{
		if (!regexec(&(R->compiled_pattern), url, 0, 0, 0))
			return R;
	}

	return NULL;
}


void add_optimizedLines(refresh_t *R)
{
	uint32_t i;
	char *pChr = NULL;
	char *endstr = "/";
	char host[128] = "";
	char b[10240];
	regex_t comp;

	//no need to check null ,already checked outside
	pChr = strstr(R->pattern, "://");
	pChr = strstr(pChr + 3, endstr);
	strncpy(host, R->pattern+1, pChr - (R->pattern + 1) + 1);
	i = myHash(host, PATTERNLINESSIZE);
	if(NULL == strBuf(optimizedLines[i].host))
	{
		assert(0 == strLen(optimizedLines[i].host));
		assert(NULL == optimizedLines[i].refresh);
		stringInit(&optimizedLines[i].host, host);
		optimizedLines[i].refresh = refresh_t_pool_alloc();
		strcpy(b, R->pattern);
		assert('@' == b[0]);
		b[0] = '^';
		optimizedLines[i].refresh->pattern = xstrdup(b);
		optimizedLines[i].refresh->min = R->min;
		optimizedLines[i].refresh->pct = R->pct;
		optimizedLines[i].refresh->max = R->max;
		optimizedLines[i].refresh->next = NULL;
		memcpy(&(optimizedLines[i].refresh->flags), &(R->flags), sizeof(R->flags));
		optimizedLines[i].refresh->max_stale = R->max_stale;
		optimizedLines[i].refresh->stale_while_revalidate = R->stale_while_revalidate;
		optimizedLines[i].refresh->negative_ttl = R->negative_ttl;
		optimizedLines[i].refresh->regex_flags = R->regex_flags;
		regcomp(&comp, optimizedLines[i].refresh->pattern, optimizedLines[i].refresh->regex_flags);
		optimizedLines[i].refresh->compiled_pattern = comp;
		optimizedLines[i].tail = optimizedLines[i].refresh;
		refresh_pattern_host_count++;
		return;
	}
	refresh_t *end = optimizedLines[i].tail;
	end->next = refresh_t_pool_alloc();
	end = end->next;
	strcpy(b, R->pattern);
	assert('@' == b[0]);
	b[0] = '^';
	end->pattern = xstrdup(b);
	//compiled_pattern;
	end->min = R->min;
	end->pct = R->pct;
	end->max = R->max;
	end->next = NULL;
	memcpy(&(end->flags), &(R->flags), sizeof(R->flags));
	end->max_stale = R->max_stale;
	end->stale_while_revalidate = R->stale_while_revalidate;
	end->negative_ttl = R->negative_ttl;
	end->regex_flags = R->regex_flags;
	regcomp(&comp, end->pattern, end->regex_flags);
	end->compiled_pattern = comp;
	optimizedLines[i].tail = optimizedLines[i].tail->next;
	return;
}

static void cleanup(void)
{
	uint32_t i;
	refresh_t *backup;

	debug(117, 1)("(mod_refresh_pattern) ->	cleanup for reconfiguring or shutting down:\n");

	for (i = 0; i < PATTERNLINESSIZE; i++)
	{
		if (0 == strLen(optimizedLines[i].host))
		{
			assert(NULL == optimizedLines[i].refresh);
			continue;
		}
		stringClean(&optimizedLines[i].host);
		while (NULL != optimizedLines[i].refresh)
		{
			backup = optimizedLines[i].refresh->next;
			xfree((void*)(optimizedLines[i].refresh->pattern));
			regfree(&optimizedLines[i].refresh->compiled_pattern);
			memPoolFree(refresh_t_pool, optimizedLines[i].refresh);
			optimizedLines[i].refresh = backup;
		}
	}
	while (NULL != defaultLines.refresh)
	{
		backup = defaultLines.refresh->next;
		xfree((void*)defaultLines.refresh->pattern);
		regfree(&defaultLines.refresh->compiled_pattern);
		memPoolFree(refresh_t_pool, defaultLines.refresh);
		defaultLines.refresh = backup;
	}
	memset(optimizedLines, 0, sizeof(optimizedLines));
	assert(NULL == defaultLines.refresh);
}

void add_defaultLines(refresh_t *tmp)
{
	regex_t comp;
	refresh_t *new_refresh;

	new_refresh = refresh_t_pool_alloc();
	new_refresh->pattern = xstrdup(tmp->pattern);
	//compiled_pattern;
	new_refresh->min = tmp->min;
	new_refresh->pct = tmp->pct;
	new_refresh->max = tmp->max;
	new_refresh->next = NULL;
	memcpy(&(new_refresh->flags), &(tmp->flags), sizeof(tmp->flags));
	new_refresh->max_stale = tmp->max_stale;
	new_refresh->stale_while_revalidate = tmp->stale_while_revalidate;
	new_refresh->negative_ttl = tmp->negative_ttl;
	new_refresh->regex_flags = tmp->regex_flags;
	regcomp(&comp, new_refresh->pattern, new_refresh->regex_flags);
	new_refresh->compiled_pattern = comp;
	if (NULL == defaultLines.refresh)
	{
		defaultLines.refresh = new_refresh;
		defaultLines.tail = defaultLines.refresh;
		return;
	}
	defaultLines.tail->next = new_refresh;
	defaultLines.tail = new_refresh;
	return;
}

static void hook_after_parse_param(void)
{
	char *pChr;
	refresh_t *tmp;

	cleanup();
	refresh_pattern_host_count = 0;
	memset(optimizedLines, 0, sizeof(optimizedLines));
	for (tmp = Config.Refresh; tmp; tmp = tmp->next)
	{
		pChr = strstr(tmp->pattern, "://");
		if (pChr)
			pChr = strstr(pChr + 3, "/");
		if (!pChr && '@' == tmp->pattern[0])
			tmp->pattern[0] = '^';
		if ('@' == tmp->pattern[0])
			add_optimizedLines(tmp);
		else
			add_defaultLines(tmp);
	}
}

static int hook_init(void)
{
	return 0;
}

static int hook_cleanup(cc_module *module)
{
	cleanup();	
	if (NULL != refresh_t_pool)
		memPoolDestroy(refresh_t_pool);

	return 0;
}

/* module init */
int mod_register(cc_module *module)
{
	debug(117, 1)("(mod_refresh_pattern) ->	mod_register:\n");

	strcpy(module->version, "7.0.R.16488.i686");

	//module->hook_func_sys_after_parse_param = hook_after_parse_param;
	cc_register_hook_handler(HPIDX_hook_func_sys_after_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_after_parse_param),
			hook_after_parse_param);

	//module->hook_func_sys_init = hook_init;
	cc_register_hook_handler(HPIDX_hook_func_sys_init,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_init),
			hook_init);

	//module->hook_func_sys_cleanup = hook_cleanup;
	cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_cleanup),
			hook_cleanup);

	//module->hook_func_private_refresh_limits = hook_find_refresh_pattern;
	cc_register_hook_handler(HPIDX_hook_func_private_refresh_limits,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_refresh_limits),
			hook_find_refresh_pattern);

	return 0;
}

