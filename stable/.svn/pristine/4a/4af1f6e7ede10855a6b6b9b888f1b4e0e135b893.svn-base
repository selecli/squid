#include "detect.h"

struct node {
	void *data;
	struct node *next;
};

struct header {
	int num;
	int size;
	struct node *cur;
	struct node *tail;
	struct node *head;
};

static int node_sz;

LLIST *llistCreate(int size)
{
	struct header *head;

	node_sz = sizeof(struct node);
	head = xcalloc(1, sizeof(struct header));
	head->num = 0;
	head->size = size;
	head->cur = NULL;
	head->head = NULL;
	head->tail = NULL;

	return head; 	
}

void *llistInsert(LLIST *head, void *data, int mode)
{
	struct node *node;
	struct header *ptr = head;

	assert(head != NULL);
	node = xcalloc(1, node_sz);
	node->data = xcalloc(1, ptr->size);
	memcpy(node->data, data, ptr->size);

	switch (mode)
	{
		case HEAD:
			node->next = ptr->head;
			if (NULL == ptr->head)
				ptr->tail = node;
			ptr->head = node;
			ptr->num++;
			break;
		case TAIL:
			node->next = NULL;
			if (NULL == ptr->tail)
				ptr->head = node;
			else
				ptr->tail->next = node;
			ptr->tail = node;
			ptr->num++;
			break;
		default:
			xabort("llist_insert: no this mode");
	}

	return node->data;
}

int llistDelete(LLIST *head, void *key, COMP *cmp)
{
	struct header *ptr = head;
	struct node *cur, *prev, *next;

	assert(ptr != NULL);

	for (cur = ptr->head, prev = ptr->head; cur != NULL; prev = cur, cur = next)
	{
		next = cur->next;
		if (!cmp(key, cur->data))
		{
			if (cur == ptr->head)
				ptr->head = next;
			else
				prev->next = next;
			ptr->cur = next;
			free(cur->data);
			free(cur);
			ptr->num--;
			return 0;
		}
	}

	return -1;
}

void *llistFind2(LLIST *head, void *key, COMP *cmp)
{
	struct node *cur;
	struct header *ptr = head;

	assert(head != NULL);

	for (cur = ptr->head; cur != NULL; cur = cur->next)
	{
		if (!cmp(key, cur->data))
			return cur->data;
	}

	return NULL;
}

int llistFind(LLIST *head, void *key, COMP *cmp, void *ret)
{
	struct node *cur;
	struct header *ptr = head;

	assert(head != NULL);

	for (cur = ptr->head; cur != NULL; cur = cur->next)
	{
		if (!cmp(key, cur->data))
		{
			memcpy(ret, cur->data, ptr->size);
			return 0;
		}
	}

	return -1;
}

int llistTravel(LLIST *head, TRCB *travel)
{
	struct node *cur;
	struct header *ptr = head;

	assert(head != NULL);

	for (cur = ptr->head; cur != NULL; cur = cur->next)
	{
		travel(cur->data);
	}

	return ptr->num;
}

void llistTravel2(LLIST *head, TRCB *travel, int number)
{
	struct node *cur, *next;
	struct header *ptr = head;

	assert(head != NULL);

	if (0 == ptr->num)
		return;

	cur = (NULL == ptr->cur) ? ptr->head : ptr->cur;

	for ( ; number-- > 0; cur = next)
	{
		if (0 == ptr->num)
			return;
		if (NULL == cur)
			cur = ptr->head;
		next = cur->next;
		travel(cur->data);
	}
}

int llistModify(LLIST *head, void *key, COMP *cmp, void *modify)
{
	struct node *cur;
	struct header *ptr = head;

	assert(head != NULL);

	for (cur = ptr->head; cur != NULL; cur = cur->next)
	{
		if (!cmp(key, cur->data))
		{
			memcpy(cur->data, modify, ptr->size);  
			return 0;
		}
	}

	return -1;	
}

int llistNumber(LLIST *head)
{
	return ((struct header *)head)->num;
}

static int cmpDelete(void *key, void *data)
{
	return 0;
}

int llistEmpty(LLIST *head)
{
	struct node *cur;
	struct header *ptr = head;

	assert(head != NULL);

	for (cur = ptr->head; cur != NULL; cur = cur->next)
	{
		if (llistDelete(ptr, cur->data, cmpDelete) < 0)
			return -1; 
	}

	return 0;
}

int llistIsEmpty(LLIST *head)
{
	struct header *ptr = head;

	assert(ptr != NULL);

	return (0 == ptr->num);
}

void llistDestroy(LLIST *head)
{
	struct node *cur, *next;
	struct header *ptr = head;

	if (NULL == ptr)
		return;
	for (cur = ptr->head; cur != NULL; cur = next)
	{
		next = cur->next;
		free(cur->data);
		free(cur);
	}
	free(ptr);
}

