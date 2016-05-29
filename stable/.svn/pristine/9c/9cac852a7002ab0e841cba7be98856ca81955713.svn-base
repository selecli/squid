#include "mod_billing_hashtable.h"
#include "squid.h"
//出于mempool的需要加入看头文件squid.h
//由于hash_table和squid.h中声明的某个头文件中的变量冲突故将其重命名为billing_hash_table
static struct hash_entry*	billing_hash_table[TABLESIZE];

static MemPool * hash_entry_pool = NULL; //声明MemPool变量
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

static void * hash_entry_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == hash_entry_pool)
	{
		hash_entry_pool	= memPoolCreate("mod_billing other_data hash_entry", sizeof(struct hash_entry));
	}
	return obj = memPoolAlloc(hash_entry_pool);
}

void hashtable_init()
{
	uint32_t i;
	for( i = 0 ; i < TABLESIZE ; i++ )
		billing_hash_table[i] = NULL;

	assert(hashtable_entry_count == 0);
}

void hashtable_dest()
{
//	assert(hashtable_entry_count == 0);
// when kill Runcache, when can't connect billingd, hashtable_entry_count!=0
	if(NULL != hash_entry_pool)
	{
		memPoolDestroy(hash_entry_pool);
	}
}

struct hash_entry* hashtable_create(const char* host)
{
	hashtable_entry_count++;
	assert(host);
	//为结构体hash_entry在MemPool中分配内存
	struct hash_entry *entry = hash_entry_pool_alloc();
	entry_init(entry);
	entry->key = ELFhash((char*)host);
	assert(strlen(host) < 4095);
	strcpy(entry->host, host);

	const uint32_t val = entry->key & (TABLESIZE - 1);

	struct hash_entry *ptr = billing_hash_table[val];

	//该链表为空，插入新结节，返回
	if(ptr == NULL) {
		billing_hash_table[val] = entry;
	//	billing_hash_table[val]->next = NULL;	//初始化即为NULL
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
		billing_hash_table[val] = entry;
	else
		prev->next = entry;

	return entry;
}

struct hash_entry* hashtable_get(const char* host)
{
	int32_t key = ELFhash((char*)host);

	const uint32_t val = key & (TABLESIZE-1);

	if( billing_hash_table[val] == NULL )
		return NULL;

	struct hash_entry *ptr = billing_hash_table[val];

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

	struct hash_entry *ptr = billing_hash_table[val], *prev = NULL;

	assert(ptr);

	while(ptr != NULL) {

		int ret = strcmp(ptr->host, entry->host);

		if( ret == 0 ) {	//找到该节点

			if( prev == NULL ) {
				billing_hash_table[val] = ptr->next;
				//从MemPool中free结构体hash_entry
				memPoolFree(hash_entry_pool, ptr);
				return;
			}


			if( prev != NULL ) {
				prev->next = ptr->next;
				//从MemPool中free结构体hash_entry
				memPoolFree(hash_entry_pool, ptr);
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

	for( i = 0 ; i < TABLESIZE ; i++ ) {

		if( billing_hash_table[i] == NULL )
			continue;

		struct hash_entry *entry = billing_hash_table[i];
		while( entry != NULL ) {
			entry_array[offset] = entry;
			offset++;
			entry = entry->next;
		}
	}

	assert( offset == hashtable_entry_count);

	return hashtable_entry_count;
}


