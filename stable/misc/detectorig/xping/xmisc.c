#include "xping.h"

void xabort(const char *info)
{
	xlog(ERROR, "%s", info);
	abort();
}

void *xmalloc(size_t size)
{
	void *tmp = NULL;

	tmp = malloc(size);
	if (NULL == tmp)
	{
		xlog(WARNING, "malloc() failed, ERROR=[%s]", strerror(errno));
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
		xlog(WARNING, "calloc() failed, ERROR=[%s]", strerror(errno));
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
		xlog(WARNING, "realloc() failed, ERROR=[%s]", strerror(errno));
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
			xlog(WARNING, "no this type, TYPE=[%d]", type);
			break;  
	}
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
			strncpy(xping.localaddr, inet_ntoa(sin->sin_addr), BUF_SIZE_16);
			return;
		}    
		++ifr;
	}    
	xlog(WARNING, "get localhost IP failed");
	//default use 0.0.0.0
	strncpy(xping.localaddr, "0.0.0.0", BUF_SIZE_16);
}

FILE *openFile(const char *path, const char *mode)
{
	FILE *fp = NULL;

	fp = fopen(path, mode);
	if (NULL == fp) 
	{
		if (NULL == xping.logfp)
			fprintf(stderr, "openFile() failed, FILE=[%s], ERROR=[%s]", path, strerror(errno));
		else
			xlog(WARNING, "openFile() failed, FILE=[%s], ERROR=[%s]", path, strerror(errno));
	}

	return fp; 
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

