#include "squid.h"

typedef struct _ipcache_pair_entry ipcache_pair_entry;
typedef struct _ipcache_addr_pair ipcache_addr_pair;

struct _ipcache_addr_pair
{
	struct in_addr local_ip;
	struct in_addr dst_ip;
};

struct _ipcache_pair_entry
{
	hash_link hash;		/* must be first */
	time_t lastref;
	time_t expires;
	ipcache_addr_pair *addrs;
	struct timeval request_time;
	dlink_node lru;
};

static dlink_list lru_list;

static FREE ipcacheFreeEntry;
static int ipcacheExpiredEntry2(ipcache_pair_entry *);
static ipcache_pair_entry *ipcache_get2(const char *);
static void ipcacheRelease2(ipcache_pair_entry *);

static hash_table *ip_table = NULL;
//static ipcache_addrs static_addrs;

/*
static long ipcache_low = 180;
static long ipcache_high = 200;
*/

static long ipcache_low = 180;
static int m_ttl;

/* removes the given ipcache entry */
static void
ipcacheRelease2(ipcache_pair_entry * i)
{
	debug(186, 3) ("(mod_dst_ip) -> ipcacheRelease: Releasing entry for '%s'\n", (const char *) i->hash.key);
	hash_remove_link(ip_table, (hash_link *) i);
	dlinkDelete(&i->lru, &lru_list);
	ipcacheFreeEntry(i);
}

static ipcache_pair_entry *
ipcache_get2(const char *name)
{
	if (ip_table != NULL)
		return (ipcache_pair_entry *) hash_lookup(ip_table, name);
	else
		return NULL;
}

static int
ipcacheExpiredEntry2(ipcache_pair_entry * i)
{
	if (i->expires > squid_curtime)
		return 0;
	return 1;
}

void
ipcache_pair_purgelru(void *voidnotused)
{
	dlink_node *m;
	dlink_node *prev = NULL;
	ipcache_pair_entry *i;
	int removed = 0;
	eventAdd("ipcache_pair_purgelru", ipcache_pair_purgelru, NULL, 10.0, 1);
	for (m = lru_list.tail; m; m = prev)
	{
		if (ip_table->count < ipcache_low)
			break;
		prev = m->prev;
		i = m->data;
		ipcacheRelease2(i);
		removed++;
	}
	debug(186, 9) ("(mod_dst_ip) -> ipcache_pair_purgelru: removed %d entries\n", removed);
}

/* create blank ipcache_pair_entry */
ipcache_pair_entry *
ipcachePairCreateEntry(const char *name, struct in_addr local, struct in_addr dst)
{
	static ipcache_pair_entry *i;
	i = xmalloc(sizeof(ipcache_pair_entry));
	i->hash.key = xstrdup(name);
	i->expires = squid_curtime + m_ttl;
	debug(186, 3)("(mod_dst_ip) ->ipcachePairCreateEntry %d\n",m_ttl); 
	i->addrs = xmalloc(sizeof(ipcache_addr_pair));
	i->addrs->local_ip = local;
	i->addrs->dst_ip = dst;
	return i;
}

static void
ipcachePairAddEntry(ipcache_pair_entry * i)
{
	hash_link *e = hash_lookup(ip_table, i->hash.key);
	if (NULL != e)
	{
		/* avoid colission */
		ipcache_pair_entry *q = (ipcache_pair_entry *) e;
		ipcacheRelease2(q);
	}
	hash_join(ip_table, &i->hash);
	dlinkAdd(i, &i->lru, &lru_list);
	i->lastref = squid_curtime;
}

void ipcache_pair_add(const char* host, struct in_addr local_addr, struct in_addr dst_addr)
{
	ipcachePairAddEntry(ipcachePairCreateEntry(host, local_addr, dst_addr));
	return;
}


/* initialize the ipcache */
void
ipcache_pair_init(void)
{
	int n = 0;
	debug(186, 3) ("(mod_dst_ip) -> Initializing IP Pair Cache...\n");
//	memset(&lru_list, '\0', sizeof(lru_list));
/*
	memset(&static_addrs, '\0', sizeof(ipcache_addrs));
        static_addrs.in_addrs = xcalloc(1, sizeof(struct in_addr));
        static_addrs.bad_mask = xcalloc(1, sizeof(unsigned char));
*/
/*
	ipcache_high = (long) (((float) Config.ipcache.size *
							(float) Config.ipcache.high) / (float) 100);
	ipcache_low = (long) (((float) Config.ipcache.size *
						   (float) Config.ipcache.low) / (float) 100);
*/
//	n = hashPrime(ipcache_high / 4);
	if(!ip_table)
		ip_table = hash_create((HASHCMP *) strcmp, n, hash4);
/*	cachemgrRegister("ipcache",
					 "IP Cache Stats and Contents",
					 stat_ipcache_get, 0, 1);

	memDataInit(MEM_IPCACHE_ENTRY, "ipcache_pair_entry", sizeof(ipcache_pair_entry), 0);
*/
}

const ipcache_addr_pair*
ipcache_get_ip_pair_by_name(const char *name, int flags)
{
	ipcache_pair_entry *i = NULL;
//	ipcache_addr_pair *addrs;
	assert(name);
	debug(186, 3) ("(mod_dst_ip) -> ipcache_gethostbyname: '%s', flags=%x\n", name, flags);
	i = ipcache_get2(name);
	if (NULL == i)
	{
		(void) 0;
	}
	else if (ipcacheExpiredEntry2(i))
	{
		ipcacheRelease2(i);
		i = NULL;
	}
	else
	{
		i->lastref = squid_curtime;
		return i->addrs;
	}

	return NULL;
}

void ip_cache_release_by_name(const char *name)
{
	ipcache_pair_entry *i = NULL;
	i = ipcache_get2(name);

	if(i)
	{
		ipcacheRelease2(i);
	}

	return;
}
static void
ipcacheFreeEntry(void *data)
{
	ipcache_pair_entry *i = data;
//	safe_free(i->addrs.local_addrs);
//	safe_free(i->addrs.dst_addrs);
//	safe_free(i->addrs.bad_mask);
	safe_free(i->hash.key);
//	memFree(i, MEM_IPCACHE_ENTRY);
	if(i)
		xfree(i);
	
}

void
ipcacheFreeMemory(void)
{
	hashFreeItems(ip_table, ipcacheFreeEntry);
	hashFreeMemory(ip_table);
	ip_table = NULL;
}

