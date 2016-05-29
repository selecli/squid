#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include "mempool.h"
#include "log.h"

uint64_t cc_malloc_count = 0;
uint64_t cc_malloc_times = 0;
uint64_t cc_free_times = 0;


#ifdef	__THREAD_SAFE__
static pthread_mutex_t memory_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

//inline void lock_memory()
//{
//#ifdef	__THREAD_SAFE__
//	pthread_mutex_lock(&memory_mutex);
//#endif
//}
//
//inline void unlock_memory()
//{
//#ifdef	__THREAD_SAFE__
//	pthread_mutex_unlock(&memory_mutex);
//#endif
//}

void* cc_malloc(size_t size)
{
	void *ptr = NULL;

	while( (ptr = calloc(1,size)) == NULL ) {
		cclog(0,"malloc get null!!");
		//FIXME 须要一个接口来触发lru，sleep(2)是防止磁盘暴掉
		sleep(2);
	}

	//lock_memory();
	cc_malloc_count++;
	cc_malloc_times++;
	//unlock_memory();

	return ptr;
}


void cc_free(void *data)
{
	assert( data != NULL );

	//lock_memory();
	cc_malloc_count--;
	cc_free_times++;
	//unlock_memory();

	free(data);
	data = NULL;
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

