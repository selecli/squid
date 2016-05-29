/*
 *把程序转换为daemon形式
 *设置程序只有一个版本可以运行
*/

#ifndef MISC_H
#define MISC_H

#include <stdlib.h>
#include <stdio.h>


extern int debug_level;

#define debug(a, s, args...)	_debug((a), (char *)__FILE__, (int)__LINE__, (char *)__func__, s, ## args)
void _debug(int level, char * file, int line, char * func, char * format, ...);



//检查是否只有这一个进程在执行。输入参数为进程对应的标识文件名
//算法是如果这个文件被其它进程加锁，返回-1；否则加锁这个文件，返回0；
int write_pid_file(const char* filename);
int getOneProcessOutput(char *const argv[], char* buffer, int length);

#endif	
