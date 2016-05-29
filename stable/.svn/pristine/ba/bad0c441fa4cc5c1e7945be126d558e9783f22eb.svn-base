/*
 * store_aufs.h
 *
 * Internal declarations for the aufs routines
 */

#ifndef __ASYNC_IO_H__
#define __ASYNC_IO_H__

extern int n_asyncufs_dirs;
extern int squidaio_nthreads;
extern int squidaio_magic1;
extern int squidaio_magic2;

/* Base number of threads if not specified to configure.
 * Weighted by number of directories (see aiops.c) */
#define THREAD_FACTOR 16

/* Queue limit where swapouts are deferred (load calculation) */
#define MAGIC1_FACTOR 10
#define MAGIC1 squidaio_magic1
/* Queue limit where swapins are deferred (open/create fails) */
#define MAGIC2_FACTOR 20
#define MAGIC2 squidaio_magic2

struct _squidaio_result_t {
    int aio_return;
    int aio_errno;
    void *_data;		/* Internal housekeeping */
    void *data;			/* Available to the caller */
};

typedef struct _squidaio_result_t squidaio_result_t;

typedef void AIOCB(int fd, void *cbdata, const char *buf, int aio_return, int aio_errno);
#ifdef CC_FRAMEWORK
void squidaio_init(int index);
#else
void squidaio_init(void);
#endif
void squidaio_shutdown(void);
int squidaio_cancel(squidaio_result_t *);
#ifdef CC_FRAMEWORK
int squidaio_open(const char *, int, mode_t, squidaio_result_t *, int);
#else
int squidaio_open(const char *, int, mode_t, squidaio_result_t *);
#endif
#ifdef CC_FRAMEWORK
int squidaio_read(int, char *, int, off_t, int, squidaio_result_t *, int);
#else
int squidaio_read(int, char *, int, off_t, int, squidaio_result_t *);
#endif
#ifdef CC_FRAMEWORK
int squidaio_zcopy(int, int, int, off_t, squidaio_result_t *, int);
#endif
#ifdef CC_FRAMEWORK
int squidaio_write(int, char *, int, off_t, int, squidaio_result_t *, int);
#else
int squidaio_write(int, char *, int, off_t, int, squidaio_result_t *);
#endif
#ifdef CC_FRAMEWORK
int squidaio_close(int, squidaio_result_t *, int);
#else
int squidaio_close(int, squidaio_result_t *);
#endif
#ifdef CC_FRAMEWORK
int squidaio_stat(const char *, struct stat *, squidaio_result_t *, int);
#else
int squidaio_stat(const char *, struct stat *, squidaio_result_t *);
#endif
#ifdef CC_FRAMEWORK
int squidaio_unlink(const char *, squidaio_result_t *, int, int);
#else
int squidaio_unlink(const char *, squidaio_result_t *);
#endif
int squidaio_truncate(const char *, off_t length, squidaio_result_t *);
int squidaio_opendir(const char *, squidaio_result_t *);
squidaio_result_t *squidaio_poll_done(void);
int squidaio_operations_pending(void);
int squidaio_sync(void);
int squidaio_get_queue_len(void);
void *squidaio_xmalloc(int size);
void squidaio_xfree(void *p, int size);
void squidaio_stats(StoreEntry *);
#ifdef CC_FRAMEWORK
void aioInit(void);
void aioDone(void);
void aioCancel(int);
void aioOpen(const char *, int, mode_t, AIOCB *, void *, int);
void aioClose(int, int);
void aioWrite(int, off_t offset, char *, int size, AIOCB *, void *, FREE *, int);
void aioRead(int, off_t offset, int size, AIOCB *, void *, int);
void aioStat(char *, struct stat *, AIOCB *, void *, int);
void aioUnlink(const char *, AIOCB *, void *, int, int);
#else
void aioInit(void);
void aioDone(void);
void aioCancel(int);
void aioOpen(const char *, int, mode_t, AIOCB *, void *);
void aioClose(int);
void aioWrite(int, off_t offset, char *, int size, AIOCB *, void *, FREE *);
void aioRead(int, off_t offset, int size, AIOCB *, void *);
void aioStat(char *, struct stat *, AIOCB *, void *);
void aioUnlink(const char *, AIOCB *, void *);

#endif
void aioTruncate(const char *, off_t length, AIOCB *, void *);
int aioCheckCallbacks(SwapDir *);
void aioSync(SwapDir *);
int aioQueueSize(void);
#ifdef CC_FRAMEWORK
void aioZCopy(int , off_t , int, AIOCB *, void *, int, int);

#define INDEX_OF_SD(A) ((((char *)A) - ((char *)(Config.cacheSwap.swapDirs)))/(sizeof(SwapDir)))
#endif
#endif
