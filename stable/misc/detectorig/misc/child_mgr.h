#ifndef CHILD_MGR_H
#define CHILD_MGR_H

#include <sys/select.h>
#include <stdio.h>
#include <time.h>

/*子进程信息结构*/
struct OneProcessInfo
{
	pid_t	pid;		//子进程pid
	int	pipein;		//与子进程标准输出相连接的管道
	int	pipeout;	//与子进程标准输入相连接的管道
	time_t	time;		//分派任务的时间
	void*	task;
};

struct ProcessInfo
{
	int	iProcessNumber;
	struct	OneProcessInfo* pstOneProcessInfo;
	int*	piFreeProcessIndex;
	int	iFreeProcessNumber;
	void	(*callback)(void* task, int fd);
};

struct ProcessInfo* initProcess(int processNumber, char* const argv[], void (*callback)(void* task, int fd));
int distributeOneCommand(const char* command, struct ProcessInfo* pstProcessInfo, void* data, fd_set* rset);
int waitFreeProcess(fd_set* rset, int maxFd, struct timeval* timeout, struct ProcessInfo** ppstProcessInfo, int processInfoNumber);
void waitAllFreeProcess(fd_set* rset, int maxFd, struct ProcessInfo** ppstProcessInfo, int processInfoNumber);
int getFreeProcessNumber(const struct ProcessInfo* pstProcessInfo);
int getMaxFileno(const struct ProcessInfo* pstProcessInfo);
void killAllProcess(struct ProcessInfo** ppstProcessInfo, int processInfoNumber);

#endif	//CHILD_MGR_H
