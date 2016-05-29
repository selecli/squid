#include "detect.h"

void xabort(const char *info)
{
	fprintf(stderr, "FATAL: %s\n", info);
	fprintf(stderr, "aborting now, you can find detailed info in log file!\n");
	fflush(NULL);
	abort();
}

void *xmalloc(size_t size)
{
	void *tmp = NULL;

	tmp = malloc(size);
	if (NULL == tmp)
	{
		dlog(WARNING, NOFD, "malloc() failed, ERROR=[%s]", xerror());
		abort();
	}
	return tmp;
}

void *xcalloc(size_t n, size_t sz)
{
	void *tmp = NULL;

	tmp = calloc(n, sz);
	if (NULL == tmp)
	{
		dlog(WARNING, NOFD, "calloc() failed, ERROR=[%s]", xerror());
		abort();
	}
	return tmp;
}

void *xrealloc(void *ptr, size_t size)
{
	void *tmp = NULL;

	tmp = realloc(ptr, size);
	if (NULL == tmp)
	{
		dlog(WARNING, NOFD, "realloc() failed, ERROR=[%s]", xerror());
		abort();
	}
	return tmp;
}

void xfree(void **ptr)
{
	free(*ptr);
	*ptr = NULL;
}

void xtime(const int type, void *curtime)
{
	struct timeval tv;

	switch (type)
	{
		case TIME_TIMEVAL:
			gettimeofday((struct timeval *)curtime, NULL);
			break;  
		case TIME_TIME:
			gettimeofday(&tv, NULL);
			*((double *)curtime) = tv.tv_sec + tv.tv_usec / 1000000.0;
			break;  
		default:
			dlog(WARNING, NOFD, "no this type, TYPE=[%d]", type);
			break;  
	}
}

void *xstrdup(const char *src)
{
	char *dst;
	size_t len; 

	len = strlen(src);
	dst = xcalloc(len + 1, 1);
	memcpy(dst, src, len);
	dst[len] = '\0';

	return dst;
}

void *xstrndup(const char *src, size_t n)
{
	char *dst;

	dst = xcalloc(n + 1, 1);
	memcpy(dst, src, n);
	dst[n] = '\0';

	return dst;
}

int xisspace(const char c)
{
	return (' ' == c || '\t' == c);
}

const char *xerror(void)
{
	return strerror(errno);
}

void hashtableCreate(void)
{
	hashtable = xcalloc(MAX_HASH_NUM, sizeof(hash_factor_t *)); 
}

void hashtableCreate1(void)
{
	hashtable1 = xcalloc(MAX_HASH_NUM, sizeof(hash_factor_t *)); 
}
void getLocalAddress(void)
{
	int i, num, sfd; 
	struct ifconf conf;
	struct ifreq *ifr;
	char buf[BUF_SIZE_1024];
	struct sockaddr_in *sin;

	sfd = socket(PF_INET, SOCK_DGRAM, 0);  
	conf.ifc_len = BUF_SIZE_1024;
	conf.ifc_buf = buf; 
	ioctl(sfd, SIOCGIFCONF, &conf);
	ifr = conf.ifc_req;
	num = conf.ifc_len / sizeof(struct ifreq);
	for(i = 0; i < num; ++i) 
	{    
		sin = (struct sockaddr_in *)(&ifr->ifr_addr);
		ioctl(sfd, SIOCGIFFLAGS, ifr);
		if(!(ifr->ifr_flags & IFF_LOOPBACK) && (ifr->ifr_flags & IFF_UP))
		{    
			strncpy(detect.other.localaddr, inet_ntoa(sin->sin_addr), BUF_SIZE_16);
			return;
		}    
		++ifr;
	}    
	dlog(WARNING, NOFD, "get localhost IP failed");
	//default use 0.0.0.0
	strncpy(detect.other.localaddr, "0.0.0.0", BUF_SIZE_16);
}

FILE *xfopen(const char *path, const char *mode)
{
	FILE *fp = NULL;

	fp = fopen(path, mode);
	if (NULL == fp) 
	{
		if (NULL == detect.other.logfp)
		{
			fprintf(stderr,
					"xfopen() failed, FILE=[%s], ERROR=[%s]",
					path, xerror());
		}
		else
		{
			dlog(WARNING, NOFD,
					"xfopen() failed, FILE=[%s], ERROR=[%s]",
					path, xerror());
		}
	}

	return fp; 
}

void xfclose(FILE **fp)
{
	if (*fp != NULL)
	{
		fclose(*fp);
		*fp = NULL;
	}
}

void setResourceLimit(void)
{
	struct rlimit rlmt;

	getrlimit(RLIMIT_NOFILE, &rlmt); 
	dlog(COMMON, NOFD,
			"current resource limits: rlim_cur=[%ld], rlim_max=[%ld]",
			rlmt.rlim_cur, rlmt.rlim_max);
	if (rlmt.rlim_cur >= detect.opts.rlimit)
		return;
	if (rlmt.rlim_max < detect.opts.rlimit)
		rlmt.rlim_max = detect.opts.rlimit; 
	rlmt.rlim_cur = detect.opts.rlimit;
	//reset the file discriptor maximum number
	if (setrlimit(RLIMIT_NOFILE, &rlmt) < 0)
	{   
		dlog(ERROR, NOFD, "setrlimit() failed, ERROR=[%s]", xerror());
		return;
	}   
	dlog(COMMON, NOFD,
			"reset recource limits: rlim_cur=[%ld], rlim_max=[%ld]",
			rlmt.rlim_cur, rlmt.rlim_max);
}

unsigned int hashKey(const void *value, unsigned int size)
{
	const char *str = value;
	unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
	unsigned int hash = 0;

	while (*str)
	{
		hash = hash * seed + (*str++);
	}

	return (hash & 0x7FFFFFFF) % size;
}

void runningSched(const int interval)
{
	if (!(++detect.runstate.count % interval))
		usleep(1);
}

int ignoreErrno(const int eno)
{
	switch(eno)
	{
		//block: EALREADY when connection in progressing
		//non-block: EINPROGRESS when connection in progressing
		case EALREADY:
		case EINPROGRESS:
		case EAGAIN:
		case EINTR:
			return 1;
		default:
			return 0;
	}
	/* we don't think it can reach here */
	return 0;
}

void debug(const int type)
{
	double time;

	if (!detect.opts.F.print)
		return;

	xtime(TIME_TIME, &time);

	switch (type)
	{
		case PRINT_START:
			fprintf(stdout, "DETECTORIG DEBUG INFORMATION:\n");
			fprintf(stdout, "----------------------------------------\n");
			printVersion();
			fprintf(stdout, "----------------------------------------\n");
			break;
		case BEFORE_GLOBAL:
			fprintf(stdout, "----------------------------------------\n");
			fprintf(stdout, "GLOBAL starting at: %.6lf\n", time);
			break;
		case AFTER_GLOBAL:
			fprintf(stdout, "GLOBAL finished at: %.6lf\n", time);
			fprintf(stdout, "----------------------------------------\n");
			break;
		case BEFORE_PARSE:
			fprintf(stdout, "----------------------------------------\n");
			fprintf(stdout, "PARSE starting at: %.6lf\n", time);
			break;
		case AFTER_PARSE:
			fprintf(stdout, "PARSE finished at: %.6lf\n", time);
			fprintf(stdout, "----------------------------------------\n");
			break;
		case BEFORE_READY:
			fprintf(stdout, "----------------------------------------\n");
			fprintf(stdout, "READY starting at: %.6lf\n", time);
			break;
		case AFTER_READY:
			fprintf(stdout, "READY finished at: %.6lf\n", time);
			fprintf(stdout, "----------------------------------------\n");
			break;
		case BEFORE_DETECT:
			fprintf(stdout, "----------------------------------------\n");
			fprintf(stdout, "DETECT starting at: %.6lf\n", time);
			break;
		case AFTER_DETECT:
			fprintf(stdout, "DETECT finished at: %.6lf\n", time);
			fprintf(stdout, "----------------------------------------\n");
			break;
		case BEFORE_OUTPUT:
			fprintf(stdout, "----------------------------------------\n");
			fprintf(stdout, "OUTPUT starting at: %.6lf\n", time);
			break;
		case AFTER_OUTPUT:
			fprintf(stdout, "OUTPUT finished at: %.6lf\n", time);
			fprintf(stdout, "----------------------------------------\n");
			break;
		case BEFORE_CLEAN:
			fprintf(stdout, "----------------------------------------\n");
			fprintf(stdout, "CLEAN starting at: %.6lf\n", time);
			break;
		case AFTER_CLEAN:
			fprintf(stdout, "CLEAN finished at: %.6lf\n", time);
			fprintf(stdout, "----------------------------------------\n");
			break;
		case BEFORE_RESULT:
			fprintf(stdout, "----------------------------------------\n");
			fprintf(stdout, "RESULT starting at: %.6lf\n", time);
			break;
		case AFTER_RESULT:
			fprintf(stdout, "RESULT finished at: %.6lf\n", time);
			fprintf(stdout, "----------------------------------------\n");
			break;
		default:
			xabort("printRunningState: no this type");
	}
	fflush(stdout);
}

void writeDetectIdentify(const int type)
{
	switch (type) 
	{
		case STATE_START:
			dlog(COMMON, NOFD, "detectorig Starting -------------------------");
			break;
		case STATE_TERMINATE:
			dlog(COMMON, NOFD, "detectorig Finished -------------------------");
			break;
		default:
			dlog(WARNING, NOFD, "no this type, TYPE=[%d]", type);
			break;
	}
}

