
/*
 * Array is an array of (void*) items with unlimited capacity
 *
 * Array grows when arrayAppend() is called and no space is left
 * Currently, array does not have an interface for deleting an item because
 *     we do not need such an interface yet.
 */


#include "server_array.h"
#include "mempool.h"

#if HAVE_ASSERT_H
#include <assert.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif

static void arrayGrow(Array * a, int min_capacity);

Array *
arrayCreate(void)
{
	Array *a = cc_malloc(sizeof(Array));
	arrayInit(a);
	return a;
}

void
arrayInit(Array * a)
{
	assert(a != NULL);
	memset(a, 0, sizeof(Array));
}

void
arrayClean(Array * a)
{
	assert(a != NULL);
	/* could also warn if some objects are left */
	cc_free(a->items);
	a->items = NULL;
}

void
arrayDestroy(Array * a)
{
	assert(a != NULL);
	arrayClean(a);
//	cc_free(a);
}

void
arrayAppend(Array * a, void *obj)
{
	assert(a != NULL);
	if (a->count >= a->capacity)
		arrayGrow(a, a->count + 1);
	a->items[a->count++] = obj;
}

void arrayInsert(Array *a, void *obj, int position)
{
	assert(a != NULL);
	if (a->count >= a->capacity)
		arrayGrow(a, a->count + 1);
	if (position > a->count)
		position = a->count;
	if (position < a->count)
		memmove(&a->items[position + 1], &a->items[position], (a->count - position) * sizeof(void *));
	a->items[position] = obj;
	a->count++;
}

/* if you are going to append a known and large number of items, call this first */
void
arrayPreAppend(Array * a, int app_count)
{
	assert(a != NULL);
	if (a->count + app_count > a->capacity)
		arrayGrow(a, a->count + app_count);
}

/* grows internal buffer to satisfy required minimal capacity */
static void
arrayGrow(Array * a, int min_capacity)
{
	const int min_delta = 16;
	int delta;
	assert(a->capacity < min_capacity);
	delta = min_capacity;
	/* make delta a multiple of min_delta */
	delta += min_delta - 1;
	delta /= min_delta;
	delta *= min_delta;
	/* actual grow */
	assert(delta > 0);
	a->capacity += delta;
	a->items = a->items ?
			   realloc(a->items, a->capacity * sizeof(void *)) :
		           cc_malloc(a->capacity * sizeof(void *));
	/* reset, just in case */
	memset(a->items + a->count, 0, (a->capacity - a->count) * sizeof(void *));
}

void
arrayShrink(Array *a, int new_count)
{
	assert(new_count <= a->capacity);
	assert(new_count >= 0);
	a->count = new_count;
}
