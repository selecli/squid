#include "preloader.h"

/******************** alloc() family functions *********************
 * The alloc() family functions:                                   *
 * 1. xassert: In order to ensure that alloc is effective.         *
 * 2. We will not check the end character '\0'.                    *
 * 3. We will add a null terminating character '\0'.               *
 *******************************************************************/

void * xmalloc(size_t sz)
{
	void * m;

	xassert(0 != sz);
	m = malloc(sz);
	xassert(NULL != m);

	return m;
}

void * xzalloc(size_t sz)
{
	void * m;

	xassert(0 != sz);
	m = malloc(sz);
	xassert(NULL != m);
	xmemzero(m, sz);

	return m;
}

void * xcalloc(size_t n, size_t sz)
{
	void * m;

	xassert(0 != n);
	xassert(0 != sz);
	m = calloc(n, sz);
	xassert(NULL != m);

	return m;
}

void * xrealloc(void * ptr, size_t sz)
{
	void * m;

	/* We don't want sz == 0, but ptr can be NULL.
	   If ptr is NULL, the call(realloc()) is equivalent to malloc(). */

	xassert(0 != sz);
	m = realloc(ptr, sz);
	xassert(NULL != m);

	return m;
}


/******************** strdup() family functions ********************
 * The strdup() family functions:                                  *
 * 1. xassert: In order to ensure that copy is effective.          *
 * 2. We will not check the end character '\0'.                    *
 * 3. We will add a null terminating character '\0'.               *
 *******************************************************************/

char * xstrdup(const char * src)
{
	char * dst;
	size_t len;

	xassert(NULL != src);

	len = strlen(src);
	/* + 1 for the terminating '\0' character */
	dst = xmalloc(len + 1);
	memcpy(dst, src, len);
	dst[len] = '\0';

	return dst;
}

char * xstrndup(const char * src, size_t n)
{
	char * dst;
	size_t len;

	xassert(0 != n);
	xassert(NULL != src);

	len = strlen(src);
	if (len > n)
	{
		len = n;
	}
	dst = xmalloc(len + 1);
	memcpy(dst, src, len);
	dst[len] = '\0';

	return dst;
}

char * xstrncpy(char * dst, const char * src, size_t n)
{
	size_t n_copy;
	char * p_dst;
	const char * p_src;

	xassert(0 < n);
	xassert(NULL != src);
	xassert(NULL != dst);

	n_copy = n;
	p_src = src;
	p_dst = dst;

	while ((0 < --n_copy) && ('\0' != *p_src))
	{
		*p_dst++ = *p_src++;
	}

	*p_dst = '\0';

	return dst;
}


/*******************************************************************
 *********************** file family functions *********************
 *******************************************************************/

void xunlink(const char * path)
{
	xassert(NULL != path);

	if (unlink(path) < 0)
	{
        LogError(1, "unlink(%s) error: %s.", path, xerror());
	}
}

void xrmdir(const char * path)
{
	glob_t gb;
	int i, gb_ret;
	char pattern[SIZE_4KB];

	if (NULL == path)
	{
		return;
	}

	snprintf(pattern, SIZE_4KB, "%s/*", path);
	gb_ret = glob(pattern, GLOB_NOSORT, NULL, &gb);

	if (0 != gb_ret)
	{
		if (GLOB_NOMATCH != gb_ret)
		{
            LogError(1, "glob(%s) error: %s.", pattern, xerror());
		}
		globfree(&gb);
		/* Remove it directly */
		rmdir(path);
		return;
	}

	for (i = 0; i < gb.gl_pathc; ++i)
	{
		if (xisdir(gb.gl_pathv[i]))
		{
			/* Recursive calling xrmdir(), then remove it. */
			xrmdir(gb.gl_pathv[i]);

			if (rmdir(gb.gl_pathv[i]) < 0)
			{
                LogError(1, "rmdir(%s) error: %s.", gb.gl_pathv[i], xerror());
			}
		}
		else
		{
			unlink(gb.gl_pathv[i]);	
		}
	}

	/* Don't forget to free it. */
	globfree(&gb);

	/* Remove the parent directory after all
	   the subfiles and subdirectories have been cleared. */
	if (rmdir(path) < 0)
	{
        LogError(1, "rmdir(%s) error: %s.", path, xerror());
	}
}

int xopen(const char * path, int flags, mode_t mode)
{
	int fd;
	mode_t old_mask;

	xassert(NULL != path);

	old_mask = umask(022);
	fd = open(path, flags, mode);
	umask(old_mask);

	if(fd < 0)
	{   
        LogError(1, "open(%s) error: %s.", path, xerror());
		return -1;
	} 

	return fd;
}

int xopen2(const char * path, int flags, mode_t mode)
{
	int fd;

	xassert(NULL != path);

	fd = xopen(path, flags, mode);

	if (fd < 0)
	{
		if (ENOENT != errno && ENOTDIR != errno)
		{
			return -1;
		}
		/* If the directory is not exist, create it. */
		if (xmkdir(path, 0755, MKDIR_PARSE) < 0)
		{
			return -1;
		}
		return xopen(path, flags, mode);
	}

	return fd;
}

FILE * xfopen(const char * path, const char * mode)
{
	FILE * fp;

	xassert(NULL != path);

	fp = fopen(path, mode);

	if (NULL == fp)
	{
		return NULL;
	}

	return fp;
}

FILE * xfopen2(const char * path, const char * mode)
{
	FILE * fp;

	fp = xfopen(path, mode);

	if (NULL == fp)
	{
		if (ENOENT != errno && ENOTDIR != errno)
		{
			return NULL;
		}
		/* If the directory is not exist, create it. */
		if (xmkdir(path, 0755, MKDIR_PARSE) < 0)
		{
			return NULL;
		}
		return xfopen(path, mode);
	}

	return fp;
}

void xfclose(FILE * fp)
{
	if (NULL == fp)
	{
		return;
	}

	/* fclose(): only flushes the user space buffers.
	 * So, if we want to ensure that the data is physically storedon disk,
	 * the kernel buffers must be flushed too with sync() or fsync().
	 * Here, we choose the function fsync(), reasons following:
	 * According to the standard specification:
	 * fsync(): the call blocks until the device reports that the transfer
	 *          has completed.
	 * sync(): schedules the writes, but may return before the actual writing is done.
	 *         Some kernel version implements it actually wait(e.g, Linux 1.3.20).
	 * In summary, fsync() is more reliably than sync().
	 * xfflush(): flush all the output stream buffers, and fsync().
	 */

	xfflush(fp);

	fclose(fp);
}

void xfflush(FILE * fp)
{
	int fd;

	xassert(NULL != fp);

	fflush(fp);

	fd = fileno(fp);

	if (fsync(fd) < 0)
	{
        LogError(1, "fsync(%d) error: %s.", fd, fp, xerror());
	}
}

int xisdir(const char * path)
{
	struct stat st;

	xassert(NULL != path);

	if (stat(path, &st) < 0)
	{
        LogError(1, "stat(%s) error: %s.", path, xerror());
		return 0;
	}

	return S_ISDIR(st.st_mode);
}

void xchmod(const char * path, mode_t mode)
{
	struct stat st;

	xassert(NULL != path);

	if (stat(path, &st) < 0)
	{
		/* Change file permissions directly */
		chmod(path, mode);
        LogError(1, "stat(%s) error: %s.", path, xerror());
		return;
	}

	if (st.st_mode ^ mode)
	{
		chmod(path, mode);
	}
}

int xmkdir(const char * path, mode_t mode, int type)
{
	char * dir;
	char * ptr;
	int status;
	mode_t old_mask;

	status = 0;

	dir = xstrdup(path);

	switch (type)
	{
		case MKDIR_NONE:
		case MKDIR_DIRECT:
			/* do nothing */
			break;
		case MKDIR_PARSE:
			/* parse the directory */
			ptr = strrchr(dir, '/');
			*ptr = '\0';
			break;
		default:
			safe_process(free, dir);
            LogAbort("unknown type: %d.", type);
			break;
	}

	old_mask = umask(022);

	if (mkdir(dir, mode) < 0)
	{
		if (EEXIST == errno && xisdir(dir))
		{
			/* EEXIST:
			 * Path already exists, but not necessarily as a directory.
			 * So, calling stat() to get the file status, then S_ISDIR(st_mode).
			 * PS: S_ISDIR(st_mode) is a POSIX macro.
			 * Here, xisdir() implements the function.
			 */
			xchmod(dir, mode);
			status = 0;
		}
		else
		{
			status = -1;
            LogError(1, "mkdir(%s) error: %s.", dir, xerror());
		}
	}
	else
	{
		status = 0;
	}

	umask(old_mask);
	safe_process(free, dir);

	return status;
}

off_t xfsize(const char * path)
{
	struct stat st;

	if (stat(path, &st) < 0)
	{
		return 0;
	}

	return st.st_size;
}


/*******************************************************************
 ****************************** Array ******************************
 *******************************************************************/

struct array_st * array_create(void)
{
	struct array_st * a;

    a = xmalloc(g_tsize.array_st);
    array_init(a);

	return a;
}

void array_init(struct array_st * a)
{
	a->items = NULL;
	a->count = 0;
	a->capacity = 0;
	pthread_mutex_init(&a->mutex, NULL);
}

void array_grow(struct array_st * a, uint32_t capacity)
{
	void * items;
	size_t copy_size;

	pthread_mutex_lock(&a->mutex);
	items = xcalloc(capacity, g_tsize.pointer);
	copy_size = a->capacity * g_tsize.pointer;
	memcpy(items, a->items, copy_size);
	safe_process(free, a->items);
	a->items = items;
	a->capacity = capacity;
	pthread_mutex_unlock(&a->mutex);
}

void array_append(struct array_st * a, void * data, FREE * data_free)
{
	void * items;
	size_t copy_size;
	uint32_t capacity;
	struct array_item_st * item;

	pthread_mutex_lock(&a->mutex);
	if (a->count >= a->capacity)
	{
		/* array space grow */
		capacity = a->count + 1;
		items = xcalloc(capacity, g_tsize.pointer);
		copy_size = a->capacity * g_tsize.pointer;
		memcpy(items, a->items, copy_size);
		safe_process(free, a->items);
		a->items = items;
		a->capacity = capacity;
	}
	/* array data append */
	item = xmalloc(g_tsize.array_item_st);
	item->data = data;
	item->data_free = data_free;
	a->items[a->count] = item;
	a->count++;
	pthread_mutex_unlock(&a->mutex);
}

void array_insert(struct array_st * a, int index, void * data, FREE * data_free)
{
	void * items;
	size_t copy_size;
	uint32_t capacity;
	struct array_item_st * item;

	pthread_mutex_lock(&a->mutex);
	if (index >= a->capacity)
	{
		/* array space grow */
		capacity = index + 1;
		items = xcalloc(capacity, g_tsize.pointer);
		copy_size = a->capacity * g_tsize.pointer;
		memcpy(items, a->items, copy_size);
		safe_process(free, a->items);
		a->items = items;
		a->capacity = capacity;
	}
	/* array data insert */
	item = xmalloc(g_tsize.array_item_st);
	item->data = data;
	item->data_free = data_free;
	a->items[index] = item;
	a->count++;
	pthread_mutex_unlock(&a->mutex);
}

void array_remove(struct array_st * a, void * key)
{
	int i;
	struct array_item_st * item;

	pthread_mutex_lock(&a->mutex);
	for (i = 0; i < a->count; ++i)
	{
		item = a->items[i];
		if (NULL == item)
		{
			continue;
		}
		if (key == item->data)
		{
			a->items[i] = NULL;
			a->count--;
		}
	}
	pthread_mutex_unlock(&a->mutex);
}

void array_delete(struct array_st * a, void * key)
{
	int i;
	FREE * data_free;
	struct array_item_st * item;

	pthread_mutex_lock(&a->mutex);
	for (i = 0; i < a->count; ++i)
	{
		item = a->items[i];
		if (NULL == item)
		{
			continue;
		}
		if (item->data == key)
		{
			data_free = item->data_free;

			if (NULL != data_free)
			{
				data_free(item->data);
			}
			safe_process(free, item);
			a->items[i] = NULL;
			a->count--;
		}
	}
	pthread_mutex_unlock(&a->mutex);
}

void array_clean(struct array_st *a)
{
    int i;
    FREE * data_free;
    struct array_item_st * item;

    pthread_mutex_lock(&a->mutex);
    for (i = 0; i < a->count; ++i)
    {
        item = a->items[i];
        if (NULL != item)
        {
            data_free = item->data_free;
            if (NULL != data_free)
            {
                data_free(item->data);
            }
            safe_process(free, item);
        }
    }
    safe_process(free, a->items);
    a->count = 0;
    a->capacity = 0;
    pthread_mutex_unlock(&a->mutex);
}

void array_destroy(struct array_st * a)
{
	int i;
	FREE * data_free;
	struct array_item_st * item;

	pthread_mutex_lock(&a->mutex);
	for (i = 0; i < a->count; ++i)
	{
		item = a->items[i];
		if (NULL != item)
		{
			data_free = item->data_free;
			if (NULL != data_free)
			{
				data_free(item->data);
			}
			safe_process(free, item);
		}
	}
	safe_process(free, a->items);
	pthread_mutex_unlock(&a->mutex);
	pthread_mutex_destroy(&a->mutex);
	safe_process(free, a);
}

/*******************************************************************
 ************************** other functions ************************
 *******************************************************************/

int xisspace(const char c)
{
	return ((' ' == c) || ('\t' == c));
}

int xisCRLF(const char c)
{
	return (('\r' == c) || ('\n' == c));
}

int is_matched(void *key, void *data)
{
	return (key != data);
}

int always_matched(void * key, void * data)
{
	return 0;
}

void current_gmttime(char * buffer, size_t buffer_size)
{
	struct tm * gmt;
	time_t cur_time;
	const char * gmt_format;

	gmt_format = "%a, %d %b %G %H:%M:%S GMT";
	cur_time = time(NULL);
	gmt = gmtime(&cur_time);
	strftime(buffer, buffer_size, gmt_format, gmt);
}

void server_name(char * buffer, size_t buffer_size)
{
	snprintf(buffer, buffer_size, "%s/%s", APP_NAME, APP_VERSION);
}

int ignore_errno(const int errnum)
{
	switch (errnum)
	{   
		case EINTR:
			/* full through */
		case EAGAIN:
			/* full through */
		case EINPROGRESS:
			/* full through */
		case EALREADY:
			return 1;
		default:
			return 0;
	}   
	return 0;
}

void AssertAbort(const char * msg, const char * file, const int line)
{
	assert(NULL != msg);
	assert(NULL != file);

    LogAbort("AssertAbort( %s:%d ): %s.", file, line, msg);
}

void data_link(void *data)
{
    int *links;

    links = data;
    (*links)++;
}

void data_unlink(void *data)
{
    int *links;

    links = data;
    (*links)--;
}

#if 0

static void ElevateSuid(void)
{
	/* Root or Owner's permission */
}

static void LeaveSuid(void)
{
	/* Minimum Permission Model */
}

#endif

