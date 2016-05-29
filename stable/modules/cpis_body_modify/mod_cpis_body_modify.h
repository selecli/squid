#ifndef MOD_CPIS_BODY_MODIFY_H
#define MOD_CPIS_BODY_MODIFY_H

#define CONFIGEFILE "/usr/local/squid/etc/cidrlist.conf"
#define CPISLOG		"/data/proclog/log/squid/cpiscidr.log"

/* list for head */
struct list_head {
	struct list_head *next, *prev;
};

/* member of value for the config list */
typedef struct mem_value{
	struct list_head list;		/* list for connect of value */
	char value[64];			/* value */
}MEM_VALUE;

/* entry for list */
#define list_entry(ptr, type, member) \
	((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))


#define is_whitespace(c)		((c) == ' ' || (c) == '\t')
#define skip_whitespace(p)	while (is_whitespace(*(p))) ++p

static inline void init_list_head(struct list_head *list)
{
	list->next = list;
	list->prev = list;
}

/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_add(struct list_head *new,
			      struct list_head *prev,
			      struct list_head *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

/**
 * list_add - add a new entry
 * @new: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void list_add(struct list_head *new, struct list_head *head)
{
	__list_add(new, head, head->next);
}

/**
 * list_add_tail - add a new entry
 * @new: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
	__list_add(new, head->prev, head);
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_del(struct list_head * prev, struct list_head * next)
{
	next->prev = prev;
	prev->next = next;
}

/**
 * list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: list_empty() on entry does not return true after this, the entry is
 * in an undefined state.
 */
static inline void list_del(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
}

/* check keyword: <single_ip> */
static int
cli_check_keyword_single_ip(char *keyword)
{
	char *str, *s;
	struct in_addr in;

	str = xmalloc((strlen(keyword) + 1) * sizeof(char));
	strcpy(str, keyword);
	s = strtok(str, ".");
	if(!s)
	{
		safe_free(str);
		return(1);
	}
	if(strlen(s) > 3)
	{
		safe_free(str);
		return(1);
	}
	if(s[0] == '0' && strlen(s) != 1)
	{
		safe_free(str);
		return(1);
	}
	s = strtok(NULL, ".");
	if(!s)
	{
		safe_free(str);
		return(1);
	}
	if(strlen(s) > 3)
	{
		safe_free(str);
		return(1);
	}
	if(s[0] == '0' && strlen(s) != 1)
	{
		safe_free(str);
		return(1);
	}
	s = strtok(NULL, ".");
	if(!s)
	{
		safe_free(str);
		return(1);
	}
	if(strlen(s) > 3)
	{
		safe_free(str);
		return(1);
	}
	if(s[0] == '0' && strlen(s) != 1)
	{
		safe_free(str);
		return(1);
	}
	s = strtok(NULL, ".");
	if(!s)
	{
		safe_free(str);
		return(1);
	}
	if(strlen(s) > 3)
	{
		safe_free(str);
		return(1);
	}
	if(s[0] == '0' && strlen(s) != 1)
	{
		safe_free(str);
		return(1);
	}
	s = strtok(NULL, ".");
	if(s)
	{
		safe_free(str);
		return(1);
	}
	safe_free(str);
	
	/* invalid IP address, 0.0.0.0 or 255.255.255.255 */
	if(inet_aton(keyword, &in) == 0 || in.s_addr == -1)
	{
		return(1);
	}
	return(0);
}

#endif

