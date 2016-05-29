#include "preloader.h"

unsigned int hash_key_calculate(const void * hash_str, unsigned int hash_sz)
{
	const char * str; 
	unsigned int hash;
	unsigned int seed;

	hash = 0;
	seed = 131;

	for (str = hash_str; '\0' != *str; ++str)
	{   
		hash = hash * seed + (*str);
	}   

	return (hash & 0x7FFFFFFF) % hash_sz;
}

struct hash_link_st * hash_link_create(void * key, const uint32_t size)
{
	struct hash_link_st * hlk;

	if (NULL == key)
	{
		/* hash link key can't be NULL. */
		return NULL;
	}

    hlk = xzalloc(g_tsize.hash_link_st);
	hlk->key = xzalloc(size);
	hlk->next = NULL;

	memcpy(hlk->key, key, size);

	return hlk;
}

void hash_link_destroy(struct hash_link_st * hlk)
{
	safe_process(free, hlk);
}

struct hash_link_st * hash_link_create_2(void * key)
{
	struct hash_link_st * hlk;

	if (NULL == key)
	{
		/* hash link key can't be NULL. */
		return NULL;
	}

	hlk = xzalloc(g_tsize.hash_link_st);
	hlk->key = key;
	hlk->next = NULL;

	return hlk;
}

struct hash_table_st * hash_table_create(COMPARE * cmp, HASH * hash, uint32_t size)
{
	struct hash_table_st * hash_table;

    hash_table = xzalloc(g_tsize.hash_table_st);
	hash_table->number = 0;
	hash_table->cmp = cmp;
	hash_table->hash = hash;
    hash_table->size = size;
    hash_table->buckets = xcalloc(hash_table->size, g_tsize.pointer);
	pthread_mutex_init(&hash_table->mutex, NULL);

	return hash_table;
}

void hash_table_join(struct hash_table_st * hash_table, struct hash_link_st * hlk)
{
	uint32_t pos;

	pthread_mutex_lock(&hash_table->mutex);

	pos = hash_table->hash(hlk->key, hash_table->size);
	hlk->next = hash_table->buckets[pos];
	hash_table->buckets[pos] = hlk;
	hash_table->number++;

	pthread_mutex_unlock(&hash_table->mutex);
}

void hash_table_remove(struct hash_table_st * hash_table, struct hash_link_st * hlk)
{
	uint32_t pos;
	struct hash_link_st ** hl;

	pthread_mutex_lock(&hash_table->mutex);

	pos = hash_table->hash(hlk->key, hash_table->size);

	for (hl = hash_table->buckets + pos; NULL != (*hl); hl = &(*hl)->next)
	{
		/* only remove from hash table, don't destroy it here. */
		if ((*hl) == hlk)
		{
			(*hl) = hlk->next;
			hash_table->number--;
			break;
		}
	}

	pthread_mutex_unlock(&hash_table->mutex);
}

void hash_table_remove_2(struct hash_table_st * hash_table, void * key)
{
	uint32_t pos;
	struct hash_link_st ** hlk;
	struct hash_link_st * remove_hlk;

	pthread_mutex_lock(&hash_table->mutex);

	pos = hash_table->hash(key, hash_table->size);

	for (hlk = hash_table->buckets + pos; NULL != (*hlk); hlk = &(*hlk)->next)
	{
		if (0 == hash_table->cmp(key, (*hlk)->key))
		{
			remove_hlk = *hlk;
			*hlk = (*hlk)->next;
			hash_link_destroy(remove_hlk);
			hash_table->number--;
			break;
		}
	}

	pthread_mutex_unlock(&hash_table->mutex);
}

void hash_table_remove_3(struct hash_table_st * hash_table, void * key, HASH_HDL * handler)
{
	uint32_t pos;
	struct hash_link_st ** hlk;
	struct hash_link_st * remove_hlk;

	pthread_mutex_lock(&hash_table->mutex);

	pos = hash_table->hash(key, hash_table->size);

	for (hlk = hash_table->buckets + pos; NULL != (*hlk); hlk = &(*hlk)->next)
	{
		if (0 == hash_table->cmp(key, (*hlk)->key))
		{
			if (0 == handler((*hlk)->key))
			{
				remove_hlk = *hlk;
				*hlk = (*hlk)->next;
				hash_link_destroy(remove_hlk);
				hash_table->number--;
			}
			break;
		}
	}

	pthread_mutex_unlock(&hash_table->mutex);
}

void * hash_table_lookup(struct hash_table_st * hash_table, const void * key)
{
	uint32_t pos;
	struct hash_link_st * hlk;

	pthread_mutex_lock(&hash_table->mutex);

	hlk = NULL;
	pos = hash_table->hash(key, hash_table->size);

	for (hlk = hash_table->buckets[pos]; NULL != hlk; hlk = hlk->next)
	{
		if (0 == hash_table->cmp(key, hlk->key))
		{
			break;
		}
	}

	pthread_mutex_unlock(&hash_table->mutex);

	return hlk;
}

int hash_table_lookup_2(struct hash_table_st * hash_table, const void * key, HASH_HDL * handler)
{
	uint32_t pos;
	struct hash_link_st * hlk;

	pthread_mutex_lock(&hash_table->mutex);

	hlk = NULL;
	pos = hash_table->hash(key, hash_table->size);

	for (hlk = hash_table->buckets[pos]; NULL != hlk; hlk = hlk->next)
	{
		if (0 == hash_table->cmp(key, hlk->key))
		{
			handler(hlk->key);
			pthread_mutex_unlock(&hash_table->mutex);
			return 0;
		}
	}

	pthread_mutex_unlock(&hash_table->mutex);

	return -1;
}

int hash_table_lookup_3(struct hash_table_st * hash_table, const void * key, COMPARE * cmp)
{
	uint32_t pos;
	struct hash_link_st * hlk;

	pthread_mutex_lock(&hash_table->mutex);

	hlk = NULL;
	pos = hash_table->hash(key, hash_table->size);

	for (hlk = hash_table->buckets[pos]; NULL != hlk; hlk = hlk->next)
	{
		if (0 == cmp(key, hlk->key))
		{
			pthread_mutex_unlock(&hash_table->mutex);
			return 0;
		}
	}

	pthread_mutex_unlock(&hash_table->mutex);

	return -1;
}

unsigned int hash_table_number(struct hash_table_st * hash_table)
{
	unsigned int number;

	pthread_mutex_lock(&hash_table->mutex);
	number = hash_table->number;
	pthread_mutex_unlock(&hash_table->mutex);

	return number;
}

void hash_table_destroy(struct hash_table_st * hash_table)
{
	int i;
	struct hash_link_st * hlk;
	struct hash_link_st * hlk_next;

	if (NULL == hash_table)
	{
		return;
	}

	pthread_mutex_lock(&hash_table->mutex);

	if (NULL == hash_table->buckets)
	{
		pthread_mutex_unlock(&hash_table->mutex);
		return;
	}

	for (i = 0; i < hash_table->size; ++i)
	{
		hlk = hash_table->buckets[i];

		if (NULL == hlk)
		{
			continue;
		}

		for (hlk_next = hlk; NULL != hlk; hlk = hlk_next->next)
		{
			hlk = hlk->next;
			safe_process(free, hlk->key);
			safe_process(free, hlk);

			if (NULL == hlk_next)
			{
				break;
			}
		}
	}

	safe_process(free, hash_table->buckets);
	pthread_mutex_unlock(&hash_table->mutex);
	pthread_mutex_destroy(&hash_table->mutex);
	safe_process(free, hash_table);
}

