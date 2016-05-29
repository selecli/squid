#ifndef LIMIT_HASHTABLE_H_
#define LIMIT_HASHTABLE_H_

#include <stdint.h>
#define TABLESIZE	0x100UL	//1024
#define	HOSTSIZE	4096

extern uint32_t hash_table_entry_count;
extern void limit_hashtable_init();
extern void limit_hashtable_dest();
extern struct hash_entry* limit_hashtable_create(const char* host);
extern struct hash_entry* limit_hashtable_get(const char* host);
extern void limit_hashtable_delete(struct hash_entry *entry);
extern uint32_t limit_get_all_entry(struct hash_entry ** entry_array);

struct hash_entry
{
	int32_t key;
	int32_t conn;
	char	host[HOSTSIZE];	
	struct	hash_entry *next;
};

#endif /*HASHTABLE_H_*/
