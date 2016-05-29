#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "write_rcms_log.h"


int write_rcms_log(const char *filename, char *content)
{
	int fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR|S_IWUSR);
	if (fd < 0)
		return -1;

	struct flock lock;
	lock.l_type = F_WRLCK;
	lock.l_start = 0;
	lock.l_whence = SEEK_SET;
	lock.l_len = 0;

	if(fcntl(fd, F_SETLK, &lock) < 0)
		return -1;

	if(write(fd, content, strlen(content)) != strlen(content))
		return -1;

	int val = fcntl(fd, F_GETFD, 0);
	if(val < 0)
		return -1;

	val |= FD_CLOEXEC;
	if(fcntl(fd, F_SETFD, val) < 0)
		return -1;

	close(fd);
	return 0;
}
