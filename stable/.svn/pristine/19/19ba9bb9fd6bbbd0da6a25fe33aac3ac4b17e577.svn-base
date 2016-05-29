#ifndef LIB_H
#define LIB_H

#include <stdio.h>
#include <stdlib.h>

int safe_fwrite(void *buffer, int size, FILE * fp);
int safe_write(int fd, void *buffer,int length);
//int safe_read(int fd,void *buffer,int length);

void * realloc_safe(void * addr, int size);
void safe_free(void * mem);
void * calloc_safe(int num, int size);

#endif
