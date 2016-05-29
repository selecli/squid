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
#include "display.h"
#include "misc.h"

int debug_level = 100;

/*控制只打开一个进程
 *输入：filename——标识文件名
 *返回值：0——成功
 *        -1——失败或者已经有一个进程存在了
*/
int write_pid_file(const char* filename)
{
	//写打开标识文件，文件权限为读、写
	int fd = open(filename, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);
	if(fd < 0)
		return -1;

	//设置文件锁内容（写锁，整个文件加锁）
	struct flock lock;
	lock.l_type = F_WRLCK;
	lock.l_start = 0;
	lock.l_whence = SEEK_SET;
	lock.l_len = 0;

	//给文件加锁
	if(fcntl(fd, F_SETLK, &lock) < 0)
		return -1;

	//清空文件内容
	if(ftruncate(fd, 0) < 0)
		return -1;

	//取得进程id,把进程id写入标识文件
	char buf[20];
	sprintf(buf, "%d\n", getpid());
	if(write(fd, buf, strlen(buf)) != strlen(buf))
		return -1;

	//取得文件描述符标志
	int val = fcntl(fd, F_GETFD, 0);
	if(val < 0)
		return -1;

	//设置文件描述符标志，增加exec关闭文件标志
	val |= FD_CLOEXEC;
	if(fcntl(fd, F_SETFD, val) < 0)
		return -1;

	//设置成功返回0
	return 0;
}


void _debug(int level, char * file, int line, char * func, char * format, ...)
{
	va_list list;

	if (level >= debug_level) {
		fprintf(stderr, "\n[%s][%d][%s]", file, line, func);
		va_start(list,format);
		vfprintf(stderr, format, list);
		va_end(list);
	}
}


int getOneProcessOutput(char *const argv[], char* buffer, int length)
{
	int pipefd[2];
	if(-1 ==pipe(pipefd))
	{
		return -1;
	}
	pid_t pid = fork();
	if(-1 == pid)	//fork失败
	{
		return -1;
	}
	else if(0 == pid)	//子进程
	{
		dup2(pipefd[1], 1);
		int i;
		for(i=3;i<64;i++)
		{
			close(i);
		}
		execv(argv[0], argv);
		return -1;
	}
	else	//父进程
	{
		close(pipefd[1]);
		int offset = 0;
		int ret;
		while(offset < length)
		{
			ret = read(pipefd[0], buffer+offset, length-offset);
			if(-1 == ret)
			{
				if(EINTR == errno)
				{
					continue;
				}
				return -1;
			}
			if(0 == ret)
			{
				break;
			}
			offset += ret;
		}
		return offset;
	}
}

