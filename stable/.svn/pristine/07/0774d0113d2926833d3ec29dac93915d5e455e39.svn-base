#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "lib.h"

int safe_fwrite(void *buffer,int length, FILE * fp)
{
	int bytes_left = 0;
	int written_bytes = 0;
	char *ptr=NULL;

	ptr=buffer;
	bytes_left = length;

	if (length == 0)
		return 0;	

	int retry_times = 0;
	
	while (bytes_left > 0) {
		written_bytes = fwrite(ptr, 1, bytes_left, fp);
		if (written_bytes == 0)
			return length - bytes_left;

		if(written_bytes <= 0) {       
			if(errno == EINTR) {
				usleep(100000);
				continue;
			} else {
				retry_times++;
				if (retry_times < 100) {
					usleep(100000);
					continue;
				} else {
					fprintf(stderr, "fwrite error(%d)\n", errno);
					return -1;
				}
			}
		}       
		bytes_left -= written_bytes;
		ptr += written_bytes;
	}
	return length;
}


int safe_write(int fd, void *buffer,int length)
{
	int bytes_left = 0;
	int written_bytes = 0;
	char *ptr=NULL;

	ptr=buffer;
	bytes_left = length;

	if (length == 0)
		return 0;	

	int retry_times = 0;
	
	while (bytes_left > 0) {
		written_bytes = write(fd, ptr, bytes_left);
		
		if (written_bytes == 0)
			return length - bytes_left;

		if(written_bytes < 0) {       
			if(errno == EINTR) {
				usleep(100000);
				continue;
			} else {
				retry_times++;
				if (retry_times < 100) {
					usleep(100000);
					continue;
				} else {
					fprintf(stderr, "fwrite error(%d)\n", errno);
					return -1;
				}
			}
		}       
		bytes_left -= written_bytes;
		ptr += written_bytes;
	}
	return length;
}

#if 0
//FIXME:
int safe_read(int fd,void *buffer,int length)
{
	int bytes_left = length;
	int bytes_read = 0;
	char * ptr = buffer;

	while (bytes_left > 0) {
		bytes_read = read(fd, ptr, bytes_left);
		if (bytes_read < 0) {
			if(errno == EINTR)
				bytes_read = 0;
			else
				return -1;
		} else if (bytes_read == 0)
			break;
		bytes_left -= bytes_read;
		ptr += bytes_read;
	}
	return length - bytes_left;
}
#endif


void * realloc_safe(void * addr, int size)
{
	void * rtn=NULL;

	while (1) {
		if (addr == NULL)
			rtn = calloc_safe(1, size);
		else
			rtn = realloc(addr, size);
		if (rtn != NULL)
			return rtn;
		usleep(500000);
	}
}

void safe_free(void * mem)
{
	if (mem)
		free(mem);
}


void * calloc_safe(int num, int size)
{
	void * rtn=NULL;

	while (1) {
		rtn = calloc(num, size);
		if (rtn != NULL)
			return rtn;
		usleep(500000);
	}
}
