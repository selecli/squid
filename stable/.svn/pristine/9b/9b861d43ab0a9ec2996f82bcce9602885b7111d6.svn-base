#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "mod_billing_mempool.h"


uint64_t cc_malloc_count = 0;
uint64_t cc_malloc_times = 0;
uint64_t cc_free_times = 0;


void* cc_malloc(size_t size)
{
	void *ptr = malloc(size);

	assert(ptr);

	cc_malloc_count++;
	cc_malloc_times++;

	return ptr;
}

void cc_free(void *data)
{
	assert( data != NULL );

	cc_malloc_count--;
	cc_free_times++;

	free(data);
}

char *cc_strdup(const char *s)
{
	assert(s);

	size_t len = strlen(s);

	char *d = (char*)cc_malloc(len + 1);

	memcpy(d, s, len);

	d[len] = 0;

	return d;
}

