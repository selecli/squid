#include "squid.h"

#define MAX_TYPE_LEN 10
typedef struct _UAcache_entry UAcache_entry;

struct _UAcache_entry
{
	hash_link hash;		/* must be first */
	dlink_node lru;
	char type[MAX_TYPE_LEN];
};

static dlink_list lru_list;

static FREE UAcacheFreeEntry;
static UAcache_entry *UAcache_get(const char *);
static void UAcacheRelease(UAcache_entry *);

static hash_table *UA_table = NULL;

static long UAcache_low = 3000;

static char *default_type = "default";


void del_white(char *str);

/* removes the given UAcache entry */
static void
UAcacheRelease(UAcache_entry * i)
{
	debug(184, 3) ("(mod_ca) -> UAcacheRelease: Releasing entry for '%s'\n", storeKeyText((unsigned const char *) i->hash.key));
	hash_remove_link(UA_table, (hash_link *) i);
	dlinkDelete(&i->lru, &lru_list);
	UAcacheFreeEntry(i);
}

static const cache_key * storeKey(const char *UA) {
    static cache_key digest[SQUID_MD5_DIGEST_LENGTH];

    SQUID_MD5_CTX M;
    SQUID_MD5Init(&M);
    SQUID_MD5Update(&M, (unsigned char *) UA, strlen(UA));
    SQUID_MD5Final(digest, &M);
    return digest;
}   

int hit_UA(const char *ua)
{
	if(UAcache_get(ua) != NULL)
		return 1;
	else
		return 0;
}

static UAcache_entry *
UAcache_get(const char *name)
{
	static char buf[1024];

	if(name == NULL)
		return NULL;

	strncpy(buf, name, 1023);
	del_white(buf);

	if(strlen(buf) > 0)
		debug(184, 9) ("(mod_ca) -> UAcache_get: search key: [%s] for [%s] of [%s]\n", storeKeyText(storeKey(name)), buf, name);
	else
	{
		debug(184, 9) ("(mod_ca) -> UAcache_get: search key failed\n");
		return NULL;
	}

	if (UA_table != NULL)
		return (UAcache_entry *) hash_lookup(UA_table, storeKey(buf));
	else
		return NULL;
}

void
UAcache_purgelru(void *voidnotused)
{
	dlink_node *m;
	dlink_node *prev = NULL;
	UAcache_entry *i;
	int removed = 0;
	eventAdd("UAcache_purgelru", UAcache_purgelru, NULL, 10.0, 1);
	for (m = lru_list.tail; m; m = prev)
	{
		if (UA_table->count < UAcache_low)
			break;
		prev = m->prev;
		i = m->data;
		UAcacheRelease(i);
		removed++;
	}
	debug(184, 9) ("(mod_ca) -> UAcache_purgelru: removed %d entries\n", removed);
}

/* create blank UAcache_entry */
UAcache_entry *
UAcacheCreateEntry(const char *name, const char *type)
{
	static UAcache_entry *i;
	i = xmalloc(sizeof(UAcache_entry));
	i->hash.key = storeKeyDup(storeKey(name));
	strncpy(i->type, type, MAX_TYPE_LEN -1);
	return i;
}

static void
UAcacheAddEntry(UAcache_entry * i)
{
	hash_link *e = hash_lookup(UA_table, i->hash.key);
	if (NULL != e)
	{
		/* avoid colission */
		UAcache_entry *q = (UAcache_entry *) e;
		UAcacheRelease(q);
	}
	hash_join(UA_table, &i->hash);
	dlinkAdd(i, &i->lru, &lru_list);
}

void UAcache_add(const char* UA, const char* type)
{
	UAcacheAddEntry(UAcacheCreateEntry(UA, type));
	return;
}


/* initialize the UAcache */
void
UAcache_init(void)
{
	int n = 0;
	debug(184, 3) ("(mod_ua) -> Initializing UA Cache...\n");
//	memset(&lru_list, '\0', sizeof(lru_list));
/*
	memset(&static_addrs, '\0', sizeof(UAcache_addrs));
        static_addrs.in_addrs = xcalloc(1, sizeof(struct in_addr));
        static_addrs.bad_mask = xcalloc(1, sizeof(unsigned char));
*/
/*
	UAcache_high = (long) (((float) Config.UAcache.size *
							(float) Config.UAcache.high) / (float) 100);
	UAcache_low = (long) (((float) Config.UAcache.size *
						   (float) Config.UAcache.low) / (float) 100);
*/
//	n = hashPrime(UAcache_high / 4);
	if(!UA_table)
		UA_table = hash_create((HASHCMP *) storeKeyHashCmp, n, storeKeyHashHash);
/*	cachemgrRegister("UAcache",
					 "IP Cache Stats and Contents",
					 stat_UAcache_get, 0, 1);

	memDataInit(MEM_IPCACHE_ENTRY, "UAcache_entry", sizeof(UAcache_entry), 0);
*/
}

/*
const UAcache_addr_pair*
UAcache_get_ip_pair_by_name(const char *name, int flags)
{
	UAcache_entry *i = NULL;
//	UAcache_addr_pair *addrs;
	assert(name);
	debug(184, 3) ("(mod_ca) -> UAcache_gethostbyname: '%s', flags=%x\n", name, flags);
	i = UAcache_get(name);
	if (NULL == i)
	{
		(void) 0;
	}
	else if (UAcacheExpiredEntry(i))
	{
		UAcacheRelease(i);
		i = NULL;
	}
	else
	{
		i->lastref = squid_curtime;
		return i->addrs;
	}

	return NULL;
}
*/

/*
void ip_cache_release_by_name(const char *name)
{
	UAcache_entry *i = NULL;
	i = UAcache_get(name);

	if(i)
	{
		UAcacheRelease(i);
	}

	return;
}
*/

static void
UAcacheFreeEntry(void *data)
{
	UAcache_entry *i = data;
//	safe_free(i->addrs.local_addrs);
//	safe_free(i->addrs.dst_addrs);
//	safe_free(i->addrs.bad_mask);
	safe_free(i->hash.key);
//	memFree(i, MEM_IPCACHE_ENTRY);
	if(i)
		xfree(i);
	
}

void
UAcacheFreeMemory(void)
{
	hashFreeItems(UA_table, UAcacheFreeEntry);
	hashFreeMemory(UA_table);
	UA_table = NULL;
}

void del_white(char *str)
{
        static char tmp[1024];

        strncpy(tmp, str, 1023);

        char * tok = strtok(tmp, w_space);

        if(tok)
                strcpy(str, tok);
        else
                return;

        while((tok = strtok(NULL, w_space)) != NULL)
        {
                strcat(str, tok);
        }

        return;
}

static void split_buffer(char *line, char *type, char *ua)
{
	static char buf[1024];
	static char tmp[1024];
	char *pos = NULL;
	strncpy(buf, line, 1023);

	char * tok = strtok(buf, w_space);
	
	if(tok != NULL)
	{
		strncpy(tmp, tok, 1023);

		//has type ?
		if(isdigit(tmp[0]))
		{
			strncpy(type, tmp, 1023);
	
			strncpy(buf, line, 1023);
			pos = strstr(buf, tmp);
			pos += strlen(tmp);
			strncpy(ua, pos, 1023);
		}
		else
		{
			strcpy(type, default_type);
			strncpy(ua, line, 1023);
		}
	}
	else
	{
		debug(184, 1) ("(mod_ca) -> split UA config line failed for: %s\n", line);
		abort();
	}

	del_white(ua);

	debug(184, 9) ("(mod_ca) -> get type: [%s], UA: [%s]\n", type, ua);

	return;
}


int parse_UA(char *UA_line)
{
	static char ua_buf[1024];
	static char type_buf[1024];


	split_buffer(UA_line, type_buf, ua_buf);

	if((UAcache_get(ua_buf)) == NULL)
	{
		UAcache_add(ua_buf, type_buf);
		return 0;
	}
	else
	{
		return -1;
	}
}

int parse_UA_file(char *file_name)
{
	FILE *fp = NULL;
	int count;
	int result;
	static char buffer[1024];

/*
	if(UA_table == NULL)			
		UAcache_init();
*/
	if(UA_table != NULL)
		UAcacheFreeMemory();

	UAcache_init();

	if((fp = fopen(file_name, "r")) == NULL)
	{	
		debug(184, 0) ("(mod_ca) -> fopen file %s failed for: %s\n", file_name, strerror(errno));
		return -1;
	}

	count = 0;
	result = 0;
	memset(buffer, 0, sizeof(buffer));
	while(fgets(buffer, 1024, fp) != NULL)
	{
		if(buffer[strlen(buffer) -2] == '\r')
		{
			buffer[strlen(buffer) -2] = '\0';
		}
		else if(buffer[strlen(buffer) -1] == '\n') 
		{
			buffer[strlen(buffer) -1] = '\0';
		}

		if(strlen(buffer) <= 0)
			continue;

		debug(184, 9) ("(mod_ca) -> parse UA [%s] \n", buffer);

		if(parse_UA(buffer) == 0)
		{
			debug(184, 9) ("(mod_ca) -> UA [%s] is added in hash table\n", buffer);
			result++;
		}
		else
		{
			debug(184, 9) ("(mod_ca) -> UA [%s] is already in hash table\n", buffer);
		}

		count++;
		memset(buffer, 0, sizeof(buffer));
	}

	fclose(fp);

	debug(184, 3) ("(mod_ca) -> after parse the UA_file we got [%d] UAs in [%d] \n", result, count);
	debug(184, 3) ("(mod_ca) -> after parse the UA_file we got [%d] UAs in hash \n", UA_table->count);
	
	if(result == 0)
	{
		return -1;
	}

	return 0;
}
			

