#include "mod_billing_hashtable.h"
#include "mod_billing_mempool.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static struct hash_entry*	hash_table[TABLESIZE];

uint32_t hashtable_entry_count = 0;

static inline int32_t ELFhash(char *host)
{
    uint32_t h = 0;
    uint32_t g;

    while( *host ) {
        h =( h<< 4) + *host++;
        g = h & 0xf0000000L;
        if( g ) h ^= g >> 24;
        h &= ~g;
    }

    return h;
}

static inline void entry_init(struct hash_entry *entry)
{
	memset(entry, 0, sizeof(struct hash_entry));
}

void hashtable_init()
{
	uint32_t i;
	for( i = 0 ; i < TABLESIZE ; i++ )
		hash_table[i] = NULL;

	assert(hashtable_entry_count == 0);
}

void hashtable_dest()
{
	assert(hashtable_entry_count == 0);
}

struct hash_entry* hashtable_create(const char* host)
{
	hashtable_entry_count++;
	assert(host);

	struct hash_entry *entry = cc_malloc(sizeof(struct hash_entry));
	entry_init(entry);
	entry->key = ELFhash((char*)host);
	assert(strlen(host) < 4095);
	strcpy(entry->host, host);

	const uint32_t val = entry->key & (TABLESIZE - 1);

	struct hash_entry *ptr = hash_table[val];

	//该链表为空，插入新结节，返回
	if(ptr == NULL) {
		hash_table[val] = entry;
	//	hash_table[val]->next = NULL;	//初始化即为NULL
		return entry;
	}

	struct hash_entry *prev = NULL;

	while( ptr != NULL ) { //由小到大排列

		int ret = strcmp(ptr->host, entry->host);

		if( ret > 0 ) {	//当前节点的key大于插入节点，就在此处插入，break
			break;
		} else if( ret < 0 ) {	//未找到位置，继续查找
			prev = ptr;
			ptr = ptr->next;
		} else {
			//新建前必需查找，找不到再新建，不会出现host碰撞
			assert(0);
		}
	}

	entry->next = ptr;	// ptr有可能为空

	if (prev == NULL)	//newPtr最小，放在最前面
		hash_table[val] = entry;
	else
		prev->next = entry;

	return entry;
}

struct hash_entry* hashtable_get(const char* host)
{
	int32_t key = ELFhash((char*)host);

	const uint32_t val = key & (TABLESIZE-1);

	if( hash_table[val] == NULL )
		return NULL;

	struct hash_entry *ptr = hash_table[val];

		//遍历hashtable上的链表
	while( ptr != NULL ) {

		int ret = strcmp(ptr->host, host);

		if(ret == 0) {
			//找到
            return ptr;
		} else if ( ret < 0 ) {
            ptr = ptr->next;
		} else {
			//未找到
            return NULL;
		}
	}

	return NULL;
}


void hashtable_delete(struct hash_entry *entry)
{
	hashtable_entry_count--;

	const uint32_t val = entry->key & (TABLESIZE-1);

	struct hash_entry *ptr = hash_table[val], *prev = NULL;

	assert(ptr);

	while(ptr != NULL) {

		int ret = strcmp(ptr->host, entry->host);

		if( ret == 0 ) {	//找到该节点

			if( prev == NULL ) {
				hash_table[val] = ptr->next;
				cc_free(ptr);
				return;
			}


			if( prev != NULL ) {
				prev->next = ptr->next;
				cc_free(ptr);
				return;
			}

			assert(0);

		} else if ( ret < 0 ) {
			prev = ptr;
			ptr = ptr->next;
		} else {
			break;
		}
	}

	//不可能未找到
	assert(0);
	return;
}

uint32_t get_all_entry(struct hash_entry ** entry_array)
{
	uint32_t i;
	uint32_t offset = 0;
	
	struct hash_entry* entry = NULL;
	for( i = 0 ; i < TABLESIZE ; i++ ) {

		if( hash_table[i] == NULL )
			continue;

		
		entry = hash_table[i];
		hash_table[i] = NULL;
		
		while( entry != NULL ) {
			entry_array[offset] = entry;
			offset++;
			entry = entry->next;
		}
	}

	assert( offset == hashtable_entry_count );
//diffrent as mod_billing;when writelog in billingd, clear the hashtable.	
	hashtable_entry_count = 0;

	return hashtable_entry_count;
}


