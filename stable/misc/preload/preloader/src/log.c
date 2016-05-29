#include "preloader.h"

struct log_st *log_create(
    const char * path,
    const uint8_t level,
    const uint32_t rotate_number,
    const uint32_t rotate_size)
{
    struct log_st *log;

    log = xcalloc(1, g_tsize.log_st);
    log->print_level   = level;
    log->rotate_size   = rotate_size;
    log->rotate_number = rotate_number;
    log->fmsg.alive    = 0;
    log->fmsg.fd.fd    = -1; 
    log->fmsg.fp.fp    = NULL;
    log->fmsg.path     = NULL;
    log->fmsg.path     = path ? xstrdup(path) : NULL;
    pthread_mutex_init(&log->mutex, NULL);

    return log;
}

void log_init(
    struct log_st *log,
    const char * path,
    const uint8_t level,
    const uint32_t rotate_number,
    const uint32_t rotate_size)
{
    log->print_level   = level;
    log->rotate_size   = rotate_size;
    log->rotate_number = rotate_number;
    log->fmsg.path     = path ? xstrdup(path) : NULL;
}

void log_open(struct log_st *log)
{
    pthread_mutex_lock(&log->mutex);
    log->fmsg.size    = xfsize(log->fmsg.path);
    log->fmsg.fp.mode = xstrdup("a");
    log->fmsg.fp.fp   = xfopen2(log->fmsg.path, "a");
    pthread_mutex_unlock(&log->mutex);
}

void log_close(struct log_st *log)
{
    pthread_mutex_lock(&log->mutex);
    xfclose(log->fmsg.fp.fp);
    safe_process(free, log->fmsg.fp.mode);
    log->fmsg.alive = 0;
    log->fmsg.size  = 0;
    log->fmsg.fd.fd = -1; 
    log->fmsg.fp.fp = NULL;
    pthread_mutex_unlock(&log->mutex);
}

struct log_st *log_clone(struct log_st *log)
{
    struct log_st *new_log;

    new_log = log_create(log->fmsg.path,
                         log->print_level,
                         log->rotate_number,
                         log->rotate_size);

    return new_log;
}

void log_destroy(struct log_st *log)
{
    log_close(log);
    pthread_mutex_destroy(&log->mutex);
    safe_process(free, log->fmsg.path);
    safe_process(free, log);
}

void log_reopen(struct log_st *log)
{
    pthread_mutex_lock(&log->mutex);
    xfflush(log->fmsg.fp.fp);
    xfclose(log->fmsg.fp.fp);
    safe_process(free, log->fmsg.fp.mode);
    log->fmsg.size = xfsize(log->fmsg.path);
    log->fmsg.fp.mode = xstrdup("a");
    log->fmsg.fp.fp = xfopen2(log->fmsg.path, "a");
    pthread_mutex_unlock(&log->mutex);
}

void log_rotate(struct log_st *log)
{
    int i;
    char fr[SIZE_1KB];
    char to[SIZE_1KB];
    struct file_st * fmsg;
    off_t log_size;

    if (log->rotate_number <= 0)
    {
        /* log rotate number <= 0, rotate needlessly. */
        return;
    }

    fmsg = &log->fmsg;
    log_size = xfsize(fmsg->path);

    if (0 == log_size)
    {
        /* 1. log file size is 0, rotate needlessly.
           2. xfsize() error will return 0, rotate needlessly. */
        return;
    }

    if (log_size < log->rotate_size)
    {
        /* log file size < rotate size, rotate needlessly. */
        return;
    }

    for (i = log->rotate_number; i > 0; --i)
    {   
        if (1 == i)
        {
            snprintf(fr, SIZE_1KB, "%s", fmsg->path); 
            snprintf(to, SIZE_1KB, "%s.%d", fmsg->path, 1); 
        }
        else
        {
            snprintf(fr, SIZE_1KB, "%s.%d", fmsg->path, i - 1); 
            snprintf(to, SIZE_1KB, "%s.%d", fmsg->path, i); 
        }
        if (access(fr, F_OK) < 0)
        {
            continue;
        }
        if (rename(fr, to) < 0)
        {
            LogError(1, "rename(%s, %s) error: %s.", fr, to, xerror());
        }
    }   

    LogDebug(1, "log rotate size(%ld), rename (%s) to (%s).", log_size, fmsg->path, to);

    log_reopen(log);

    return;
}

void log_write(
    const char * file_name,
    const int file_line_no,
    const int type,
    struct log_st *log,
    const uint8_t level,
    const char * format,
    ...)
{
    size_t n;
    va_list ap;
    int print_level;
    time_t curtime;
    struct tm * ltime;
    const char * fmt;
    char tbuf[SIZE_256];
    char info[SIZE_32KB + 1];
    char log_buf[SIZE_32KB + 1];
    char abort_buf[SIZE_256];

    pthread_mutex_lock(&log->mutex);

    /* if current log print level less than level, don't write log file,
       only < level's log message can be written to the log file. */

    if (log->print_level < level)
    {
        pthread_mutex_unlock(&log->mutex);
        return;
    }

    print_level = log->print_level;

    /* if the log file fp is NULL, don't write log file,
       just writing the log message to the stderr. */

    if (NULL == log->fmsg.fp.fp)
    {
        pthread_mutex_unlock(&log->mutex);
        fprintf(stderr, "log handler is NULL, check it please.\n");
        return;
    }

    pthread_mutex_unlock(&log->mutex);

    /* SIZE_32KB + 1:
       the last log buffer byte will be terminated with '\0'. */

    info[SIZE_32KB] = '\0';
    log_buf[SIZE_32KB] = '\0';

    /* get current time and convert it to our specified format. */

    curtime = time(NULL);
    ltime = localtime(&curtime);
    strftime(tbuf, SIZE_256, "%Y/%m/%d %H:%M:%S", ltime);

    /* parse the varying parameters with va_xxx functions,
       only support the following implemented formats now. */

    va_start(ap, format);

    for (n = 0, fmt = format; *fmt != '\0'; ++fmt)
    {
        if ('%' != *fmt) 
        {
            n += snprintf(info + n, SIZE_32KB - n, "%c", *fmt);
            continue;
        }

        switch (*(++fmt)) 
        {
            case 'c': /* char */
            n += snprintf(info + n, SIZE_32KB - n, "%c", (char)va_arg(ap, int));
            break;

            case 'd': /* integer */
            n += snprintf(info + n, SIZE_32KB - n, "%d", va_arg(ap, int));
            break;

            case 'u': /* unsigned integer */
            n += snprintf(info + n, SIZE_32KB - n, "%u", va_arg(ap, unsigned int));
            break;

            case 'l': 
            switch (*(++fmt)) 
            {
                case 'd': /* long integer */
                n += snprintf(info + n, SIZE_32KB - n, "%ld", va_arg(ap, long));
                break;

                case 'u': /* unsigned long integer */
                n += snprintf(info + n, SIZE_32KB - n, "%lu", va_arg(ap, unsigned long));
                break;

                case 'f': /* long double */
                n += snprintf(info + n, SIZE_32KB - n, "%.3lf", va_arg(ap, double));
                break;

                default:
                snprintf(info, SIZE_32KB, "log format error: %%l%c\n", *fmt);
                break;
            }
            break;

            case 'p': /* pointer */
            n += snprintf(info + n, SIZE_32KB - n, "%p", va_arg(ap, char *));
            break;

            case 's': /* string */
            n += snprintf(info + n, SIZE_32KB - n, "%s", va_arg(ap, char *));
            break;

            default:
            snprintf(info, SIZE_32KB, "log format error: %%%c\n", *fmt);
            break;
        }
    }

    va_end(ap);

    if (NULL == file_name)
    {
        snprintf(log_buf, SIZE_32KB, "%s| %s", tbuf, info);
    }
    else
    {
        if (print_level < 9)
        {
            snprintf(log_buf, SIZE_32KB, "%s| %s", tbuf, info);
        }
        else
        {
            snprintf(log_buf,
                     SIZE_32KB,
                     "%s| (%s:%d) %s",
                     tbuf, file_name, file_line_no, info);
        }
    }

    pthread_mutex_lock(&log->mutex);
    fprintf(log->fmsg.fp.fp, "%s\n", log_buf);
    xfflush(log->fmsg.fp.fp);
    pthread_mutex_unlock(&log->mutex);

    if (LOG_ABORT == type)
    {
        snprintf(abort_buf, SIZE_256, "%s| ABORT: preloader abort now.", tbuf);
        pthread_mutex_lock(&log->mutex);
        fprintf(log->fmsg.fp.fp, "%s\n", abort_buf);
        xfflush(log->fmsg.fp.fp);
        pthread_mutex_unlock(&log->mutex);

        /* Although POSIX.1 provisions to flush Standard I/O Streams Buffers in abort(),
         * (POSIX.1: The abort() effect of Standard I/O Streams is equal to fclose().
         * fclose() will flush all the Standard I/O Output Streams Buffers, then close
         * all the opened Standard I/O Streams)
         * we flush all the Process's Standard I/O Output Streams Buffers before call it.
        */

        fflush(NULL);

        abort();

        /* abort(): If the SIGABRT signal is blocked or ignore, the abort() will still
         * override it. But the signal handler may have been replaced, so, we must call
         * the function exit() after abort() in order to terminate the program here.
        */
        exit(1);
    }
    }
