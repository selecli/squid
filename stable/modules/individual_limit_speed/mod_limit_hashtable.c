#include "mod_limit_hashtable.h"
#include <squid.h>

uint32_t hash_table_entry_count = 0;

static struct hash_entry *speed_hash_table[TABLESIZE];
static MemPool * hash_entry_pool = NULL;

static inline int32_t ELFhash(char *host)
{
    uint32_t h = 0;
    uint32_t g;

    while (*host) {
        h =( h<< 4) + *host++;
        g = h & 0xf0000000L;
        if (g) h ^= g >> 24;
        h &= ~g;
    }
    return h;
}

static inline void entry_init(struct hash_entry *entry)
{
	memset(entry, 0, sizeof(struct hash_entry));
}

static void * hash_entry_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == hash_entry_pool)
		hash_entry_pool	= memPoolCreate("mod_host_speed_limit hash_entry", sizeof(struct hash_entry));
	return obj = memPoolAlloc(hash_entry_pool);
}

void limit_hashtable_init()
{
	uint32_t i;
	for (i = 0 ; i < TABLESIZE ; i++)
		speed_hash_table[i] = NULL;
	assert(hash_table_entry_count == 0);
}

void limit_hashtable_dest()
{
	if(NULL != hash_entry_pool)
		memPoolDestroy(hash_entry_pool);
}

struct hash_entry* limit_hashtable_create(const char* host)
{
	hash_table_entry_count++;
	assert(host);
	struct hash_entry *entry = hash_entry_pool_alloc();
	entry_init(entry);
	entry->key = ELFhash((char*)host);
	assert(strlen(host) < 4095);
	strcpy(entry->host, host);

	const uint32_t val = entry->key & (TABLESIZE - 1);
	struct hash_entry *ptr = speed_hash_table[val];

	if(ptr == NULL) {
		speed_hash_table[val] = entry;
		return entry;
	}

	struct hash_entry *prev = NULL;

	while (ptr != NULL) { 

		int ret = strcmp(ptr->host, entry->host);

		if( ret > 0 ) {	
			break;
		} else if( ret < 0 ) {
			prev = ptr;
			ptr = ptr->next;
		} else {
			assert(0);
		}
	}
	entry->next = ptr;
	if (prev == NULL)
		speed_hash_table[val] = entry;
	else
		prev->next = entry;

	return entry;
}

struct hash_entry* limit_hashtable_get(const char* host)
{
	int32_t key = ELFhash((char*)host);

	const uint32_t val = key & (TABLESIZE-1);

	if( speed_hash_table[val] == NULL )
		return NULL;

	struct hash_entry *ptr = speed_hash_table[val];
	while (ptr != NULL) {
		int ret = strcmp(ptr->host, host);
		if(ret == 0) {
            return ptr;
		} else if ( ret < 0 ) {
            ptr = ptr->next;
		} else {
            return NULL;
		}
	}

	return NULL;
}


void limit_hashtable_delete(struct hash_entry *entry)
{
	hash_table_entry_count--;

	const uint32_t val = entry->key & (TABLESIZE-1);

	struct hash_entry *ptr = speed_hash_table[val], *prev = NULL;

	assert(ptr);

	while (ptr != NULL) {
		int ret = strcmp(ptr->host, entry->host);
		if( ret == 0 ) {
			if( prev == NULL ) {
				speed_hash_table[val] = ptr->next;
				memPoolFree(hash_entry_pool, ptr);
				return;
			}

			if( prev != NULL ) {
				prev->next = ptr->next;
				memPoolFree(hash_entry_pool, ptr);
				return;
			}

			assert(0);

		} else if (ret < 0) {
			prev = ptr;
			ptr = ptr->next;
		} else {
			break;
		}
	}

	assert(0);
	return;
}

uint32_t limit_get_all_entry(struct hash_entry ** entry_array)
{
	uint32_t i;
	uint32_t offset = 0;

	for (i = 0; i < TABLESIZE; i++) {
		if (speed_hash_table[i] == NULL)
			continue;

		struct hash_entry *entry = speed_hash_table[i];
		while (entry != NULL) {
			entry_array[offset] = entry;
			offset++;
			entry = entry->next;
		}
	}
	assert (offset == hash_table_entry_count);
	return hash_table_entry_count;
}

