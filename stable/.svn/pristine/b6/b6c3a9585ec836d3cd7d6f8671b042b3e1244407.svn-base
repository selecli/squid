#include "sunday.h"
#include <string.h>
#include <stdlib.h>
#include "cc_framework_api.h"

#define MAX_HX_CONTENT_LEN 256

static MemPool *  sunday_specific_pool = NULL;

static void * sunday_specific_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == sunday_specific_pool)
	{
		sunday_specific_pool = memPoolCreate("mod_polityword_filter other_data algo_specific", 256*sizeof(int));
	}
	return obj = memPoolAlloc(sunday_specific_pool);
}

static int lastindexof(const char * haystack, int haystacklen, const unsigned char needle)
{
	const char *temp = haystack + haystacklen - 1;
	while((unsigned char)*temp != needle && temp > haystack)
	{
		temp --;
	}

	if((unsigned char)*temp == needle)
	{
		return temp - haystack;
	}
	else
	{
		return -1;
	}
}

static int QfindPos(const char * str, int str_length, const char * find, int pos, int fin_length, void *specific)
{
	int returnPos = str_length;

	int *lastindexes = (int *)specific;

	if ((pos + fin_length) < str_length)
	{
		unsigned char chrFind = str[pos + fin_length];
		if (fin_length >= 1)
		{
			int lastindex = lastindexes[(unsigned int)chrFind];
			if (lastindex > -1)
			{
				returnPos = pos + fin_length - lastindex;
			}
			else
			{
				returnPos = pos + fin_length + 1;
			}
		}
	}
	//printf("QfindPos returning %d,by searching [%s] in [%s]\n", returnPos, find, str);
	return returnPos;
}

int sunday(const char* str, const int str_length, const char* Sfind, const int fin_length, void *specific)
{
	int start = 0;

	if (str_length < fin_length)
	{
		return 0;
	}

	while (start + fin_length <= str_length)
	{

		if (!memcmp(str + start, Sfind, fin_length))
		{

			return 1;
		}
		int forwardPos = QfindPos(str, str_length, Sfind, start, fin_length, specific);
		start = forwardPos;
	}
	return 0;
}

void * sunday_getSpecific(const char *str, const int str_length)
{
	int *data = sunday_specific_pool_alloc();
	int i;
	for(i =0; i < 256; i++)
	{
		unsigned char c = (unsigned char)i;
		data[i] = lastindexof(str, str_length, c);
	}

	return data;
}

void sunday_freeSpecific(void * specific)
{
	int * spec = specific;
	if(NULL != spec)
	{
		memPoolFree(sunday_specific_pool, spec);
		spec = NULL;
	}
}

void sunday_destroySpecificPool(void)
{
	if(NULL != sunday_specific_pool)
	{
		memPoolDestroy(sunday_specific_pool);
	}
}
