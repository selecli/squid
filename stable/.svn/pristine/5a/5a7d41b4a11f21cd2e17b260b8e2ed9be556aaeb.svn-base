#include "preloader.h"

struct list_st * list_create(void)
{
	struct list_st * list;
	pthread_mutexattr_t attr;

    list = xmalloc(g_tsize.list_st);
	list->number = 0;
	list->head = NULL;
	list->tail = NULL;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&list->mutex, &attr);
	pthread_mutexattr_destroy(&attr);

	return list; 	
}

static void list_insert(struct list_st * list, void * data, int mode)
{
	struct list_node_st *node;

    node = xmalloc(g_tsize.list_node_st);
	node->data = data;
	node->next = NULL;
	node->prev = NULL;

	switch (mode)
	{
		case LIST_HEAD:
			pthread_mutex_lock(&list->mutex);
			if (list->number)
			{
				node->next = list->head;
				list->head->prev = node;
				list->head = node;
				list->number++;
			}
			else
			{
				list->head = node;
				list->tail = node;
				list->number++;
			}
			pthread_mutex_unlock(&list->mutex);
			return;

		case LIST_TAIL:
			pthread_mutex_lock(&list->mutex);
			if (list->number)
			{
				list->tail->next = node;
				node->prev = list->tail;
				list->tail = node;
				list->number++;
			}
			else
			{
				list->head = node;
				list->tail = node;
				list->number++;
			}
			pthread_mutex_unlock(&list->mutex);
			return;

		default:
            LogAbort("list doesn't have this mode: %s.", mode);
			return;
	}
}

void list_insert_head(struct list_st * list, void * data)
{
	list_insert(list, data, LIST_HEAD);
}

void list_insert_tail(struct list_st * list, void * data)
{
	list_insert(list, data, LIST_TAIL);
}

int list_remove(struct list_st * list, void * key, LIST_CMP * cmp)
{
	struct list_node_st *cur;
	struct list_node_st *prev;
	struct list_node_st *next;

	pthread_mutex_lock(&list->mutex);

	for (prev = cur = list->head; NULL != cur; prev = cur, cur = next)
	{
		next = cur->next;

		if (0 == cmp(key, cur->data))
		{
			if (cur == list->head)
			{
				if (cur == list->tail)
				{
					list->head = NULL;
					list->tail = NULL;
				}
				else
				{
					list->head = next;
					next->prev = NULL;
				}
			}
			else if (cur == list->tail)
			{
				list->tail = prev;
				prev->next = NULL;
			}
			else
			{
				prev->next = next;
				next->prev = prev;
			}

			safe_process(free, cur);
			list->number--;
			pthread_mutex_unlock(&list->mutex);

			return 0;
		}
	}

	pthread_mutex_unlock(&list->mutex);

	return -1;
}

void list_delete(struct list_st * list, void * key, LIST_CMP * cmp)
{
	struct list_node_st *cur;
	struct list_node_st *prev;
	struct list_node_st *next;

	pthread_mutex_lock(&list->mutex);

	for (prev = cur = list->head; NULL != cur; prev = cur, cur = next)
	{
		next = cur->next;

		if (0 == cmp(key, cur->data))
		{
			if (cur == list->head)
			{
				list->head = next;
				next->prev = NULL;
			}
			else
			{
				prev->next = next;
				next->prev = prev;
			}
			safe_process(free, cur->data);
			safe_process(free, cur);
			list->number--;
			break;
		}
	}

	pthread_mutex_unlock(&list->mutex);
}

void * list_fetch(struct list_st * list, void * key, LIST_CMP * cmp)
{
	void * data;
	struct list_node_st *cur;
	struct list_node_st *prev;
	struct list_node_st *next;

	pthread_mutex_lock(&list->mutex);

	for (prev = cur = list->head; NULL != cur; prev = cur, cur = next)
	{
		next = cur->next;

		if (0 == cmp(key, cur->data))
		{
			if (cur == list->head)
			{
				if (cur == list->tail)
				{
					list->head = NULL;
					list->tail = NULL;
				}
				else
				{
					list->head = cur->next;
					next->prev = NULL;
				}
			}
			else if (cur == list->tail)
			{
				list->tail = cur->prev;
				prev->next = NULL;
			}
			else
			{
				prev->next = next;
				next->prev = prev;
			}

			data = cur->data;
			safe_process(free, cur);
			list->number--;

			pthread_mutex_unlock(&list->mutex);

			return data;
		}
	}

	pthread_mutex_unlock(&list->mutex);

	return NULL;
}

static int list_fetch_head_compare(void * key, void * data)
{
	return 0;
}

void * list_fetch_head(struct list_st * list, void * key, LIST_CMP * cmp)
{
	return list_fetch(list, NULL, list_fetch_head_compare);
}

void * list_find(struct list_st * list, void * key, LIST_CMP * cmp)
{
	struct list_node_st *cur;

	pthread_mutex_lock(&list->mutex);

	for (cur = list->head; NULL != cur; cur = cur->next)
	{
		if (0 == cmp(key, cur->data))
		{
			pthread_mutex_unlock(&list->mutex);
			return cur->data;
		}
	}

	pthread_mutex_unlock(&list->mutex);

	return NULL;
}

int list_find_2(struct list_st * list, void * key, LIST_CMP * cmp)
{
	struct list_node_st *cur;

	pthread_mutex_lock(&list->mutex);

	for (cur = list->head; NULL != cur; cur = cur->next)
	{   
		if (0 == cmp(key, cur->data))
		{   
			pthread_mutex_unlock(&list->mutex);
			return 1;
		}   
	}   

	pthread_mutex_unlock(&list->mutex);

	return 0;
}

void list_modify(struct list_st * list, void * key, LIST_MOD * modify)
{
	struct list_node_st *cur;

	pthread_mutex_lock(&list->mutex);

	for (cur = list->head; NULL != cur; cur = cur->next)
	{
		if (0 == modify(key, cur->data))
		{
			break;
		}
	}

	pthread_mutex_unlock(&list->mutex);
}

void list_travel(struct list_st * list, LIST_TRL * travel)
{
	struct list_node_st *cur;

	pthread_mutex_lock(&list->mutex);

	for (cur = list->head; NULL != cur; cur = cur->next)
	{
		if (travel(cur->data) < 0)
		{
			break;
		}
	}

	pthread_mutex_unlock(&list->mutex);
}

void list_travel_2(struct list_st * list, LIST_TRL * travel)
{
	struct list_node_st *cur;

	pthread_mutex_lock(&list->mutex);

	for (cur = list->head; NULL != cur; cur = cur->next)
	{
		travel(cur->data);
	}

	pthread_mutex_unlock(&list->mutex);
}

uint32_t list_member_number(struct list_st * list)
{
	uint32_t number;

	pthread_mutex_lock(&list->mutex);
	number = list->number;
	pthread_mutex_unlock(&list->mutex);

	return number;
}

void list_destroy(struct list_st * list)
{
	struct list_node_st *cur;
	struct list_node_st *next;

	pthread_mutex_lock(&list->mutex);

	for (cur = list->head; NULL != cur; cur = next)
	{
		next = cur->next;
		safe_process(free, cur->data);
		safe_process(free, cur);
	}

	pthread_mutex_unlock(&list->mutex);
	pthread_mutex_destroy(&list->mutex);

	safe_process(free, list);
}
