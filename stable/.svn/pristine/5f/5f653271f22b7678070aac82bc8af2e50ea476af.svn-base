#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "child_mgr.h"
#include "misc.h"


//开子进程，并用管道联通
static int startupOneProcess(struct OneProcessInfo* pstOneProcessInfo, char* const argv[])
{
	int infd[2];
	int outfd[2];

	if(-1 == pipe(infd))
		return -1;

	if(-1 == pipe(outfd))
		return -1;

	//fork子进程
	pstOneProcessInfo->pid = fork();

	if(-1 == pstOneProcessInfo->pid) {
		return -1;
	}

	if(0 == pstOneProcessInfo->pid) { //子进程
		dup2(outfd[0], 0);
		dup2(infd[1], 1);
		int devnull = open("/dev/null", O_WRONLY);
		dup2(devnull, 2);
		close(devnull);

		int i;
		for(i=3 ; i<=getdtablesize() ; i++)
			close(i);
		execv(argv[0], argv);
		exit(-1);
	} else { //父进程
		debug(3, "Create child process pid(%d), fd=%d\n", pstOneProcessInfo->pid, infd[0]);
		pstOneProcessInfo->pipeout = outfd[1];
		pstOneProcessInfo->pipein = infd[0];
		close(outfd[0]);
		close(infd[1]);
		pstOneProcessInfo->time = 0;	//表示这个进程空闲
	}
	return 0;
}




/*初始化函数，启动子进程
 *输入：
 *    processNumber----子进程个数
 *    argv----子进程名和输入参数
 *    callback----得到一个完成任务后的处理工作
 *返回值：
 *    ProcessInfo指针----成功
 *    NULL----失败
*/
struct ProcessInfo* initProcess(int processNumber, char* const argv[], void (*callback)(void* task, int fd))
{
	//得到子进程个数，并为记录它的元素分配内存
	struct ProcessInfo* pstProcessInfo;

	if( (pstProcessInfo = (struct ProcessInfo*)malloc(sizeof(struct ProcessInfo))) == NULL )
		return NULL;

	debug(1, "Will init %d process for %s\n", processNumber, argv[0]);
	pstProcessInfo->iProcessNumber = processNumber;
	pstProcessInfo->iFreeProcessNumber = processNumber;
	pstProcessInfo->callback = callback;
	pstProcessInfo->pstOneProcessInfo = (struct OneProcessInfo*)calloc(processNumber, sizeof(struct OneProcessInfo));
	if(NULL == pstProcessInfo->pstOneProcessInfo) {
		free(pstProcessInfo);
		return NULL;
	}
	pstProcessInfo->piFreeProcessIndex = (int*)calloc(processNumber, sizeof(int));
	if(NULL == pstProcessInfo->piFreeProcessIndex) {
		free(pstProcessInfo->pstOneProcessInfo);
		free(pstProcessInfo);
		return NULL;
	}

	int i = 0;
	int ret = 0;
	//启动子进程
	for(i=0; i<processNumber; i++) {
		//启动一个子进程
		ret = startupOneProcess(pstProcessInfo->pstOneProcessInfo+i, argv);
		if(-1 == ret) {
			debug(1, "start the %d process failed ! for %s\n", i, argv[0]);
			free(pstProcessInfo->pstOneProcessInfo);
			free(pstProcessInfo);
			return NULL;
		}
		debug(1, "start the %d process ok ! for %s\n", i, argv[0]);
		//记录这个子进程为空闲
		pstProcessInfo->piFreeProcessIndex[i] = i;
	}

	return pstProcessInfo;
}



/*给一个进程分配命令。取最后一个空闲的进程
 *输入：
 *    command----给子进程下达的命令
 *    pstProcessInfo----进程信息指针
 *    data----这个命令的相关信息
 *输出：
 *    rset----监控的读文件描述符集合
 *返回值：
 *    正整数----处理结果从这个文件描述符返回
 *    -1----严重错误，应该停止程序
*/
int distributeOneCommand(const char* command, struct ProcessInfo* pstProcessInfo, void* data, fd_set* rset)
{
	//得到空闲进程,给空闲进程发命令
	int index = pstProcessInfo->piFreeProcessIndex[pstProcessInfo->iFreeProcessNumber-1];
	struct OneProcessInfo * pstOneProcessInfo = pstProcessInfo->pstOneProcessInfo + index;
	int length = strlen(command);
	int offset = 0;
	int ret=0;

	while(offset < length) {
		ret = write(pstOneProcessInfo->pipeout, command+offset, length-offset);
		if ((-1==ret) && (errno==EINTR))
			continue;
		if (-1 == ret)
			return -1;
		offset += ret;
	}

	//把这个进程标记为工作状态
	pstOneProcessInfo->time = time(NULL);
	pstOneProcessInfo->task = data;

	//观察来自该fd
	debug(2, "FD Set for fd=%d, free child count =%d\n", pstOneProcessInfo->pipein, pstProcessInfo->iFreeProcessNumber);
	FD_SET(pstOneProcessInfo->pipein, rset);
	pstProcessInfo->iFreeProcessNumber--;

	return pstOneProcessInfo->pipein;
}



/*把一个进程的状态改为空闲（把这个进程的时间改为0，并且把序号放到空闲列表里）
 *输入：
 *    pstProcessInfo----进程信息指针
 *    index----这个进程在进程信息列表中的位置
*/
static void freeOneProcess(struct ProcessInfo* pstProcessInfo, int index)
{
	if(0 == pstProcessInfo->pstOneProcessInfo[index].time)	//已经是空闲的了
		return;

	pstProcessInfo->pstOneProcessInfo[index].time = 0;
	pstProcessInfo->piFreeProcessIndex[pstProcessInfo->iFreeProcessNumber] = index;
	pstProcessInfo->iFreeProcessNumber++;
}



/*等待子进程完成任务，有完成的或者到达timeout返回，否则阻塞
 *输入：
 *    rset----监控的文件描述符集合（对于完成的任务，清理这个文件描述符）
 *    maxFd----监控的文件描述符集合里的最大文件描述符值
 *    timeout----等待时间，null表示不限时
 *    ppstProcessInfo----多个进程信息指针
 *    processInfoNumber----共有几个不同的进程
 *返回值：
 *    正整数----这次清理出的空闲子进程个数
 *    0----timeout到期
 *    -1----严重错误
*/
int waitFreeProcess(fd_set* rset, int maxFd, struct timeval* timeout, struct ProcessInfo ** ppstProcessInfo, int processInfoNumber)
{
	int ret = 0;
	struct ProcessInfo * pstProcessInfo = NULL;
	int i=0, j=0;
	int count = 0;

	fd_set rtempset;
	memcpy(&rtempset, rset, sizeof(fd_set));

	//Xcell add
	struct timeval to;
	to.tv_sec = 60;
	to.tv_usec = 0;
	if (timeout != NULL) {
		memcpy(&to, timeout, sizeof(struct timeval));
	}
	//end

	while (1) {
		ret = select(maxFd+1, &rtempset, NULL, NULL, &to);
		debug(2, "select rtn=%d\n", ret);
		if ((-1==ret) && (errno==EINTR))
			continue;
		if (-1 == ret)
			return -1;
		if (0 == ret) {
			//时间到了
			debug(2, "timeout for select\n");
			return 0;
		}

		count = 0;

		for (i=0; i<processInfoNumber; i++) {
			pstProcessInfo = ppstProcessInfo[i];
			for (j=0;j<pstProcessInfo->iProcessNumber;j++) {
				if (pstProcessInfo->pstOneProcessInfo[j].time > 0) {
					//只要是有输出，则被认为是空闲的
					//FIXME:不从该进程读取吗？这样会不会一直认为是空闲的?
					if(FD_ISSET(pstProcessInfo->pstOneProcessInfo[j].pipein, &rtempset)) {
						count++;

						//可能是processDetectResult
						pstProcessInfo->callback(pstProcessInfo->pstOneProcessInfo[j].task, pstProcessInfo->pstOneProcessInfo[j].pipein);
						//标志该进程空闲
						freeOneProcess(pstProcessInfo, j);
						FD_CLR(pstProcessInfo->pstOneProcessInfo[j].pipein, rset);
					}
				}
			}
		}
		return count;
	}
}


/*等待所有的子进程完成任务
 *输入：
 *    rset----监控的文件描述符集合
 *    maxFd----监控的文件描述符集合里的最大文件描述符值
 *    ppstProcessInfo----多个进程信息指针
 *    processInfoNumber----共有几个不同的进程
*/
void waitAllFreeProcess(fd_set* rset, int maxFd, struct ProcessInfo** ppstProcessInfo, int processInfoNumber)
{
	int i=0;
	int rtn = 0;
	struct ProcessInfo * pstProcessInfo = NULL;

	for(i=0;i<processInfoNumber;i++) {
		pstProcessInfo = ppstProcessInfo[i];
		while (pstProcessInfo->iFreeProcessNumber < pstProcessInfo->iProcessNumber) {
			rtn = waitFreeProcess(rset, maxFd, NULL, ppstProcessInfo, processInfoNumber);
			if (rtn > 0)
				continue;
			else
				return;
		}
	}
}



/*得到一组子进程里空闲子进程的个数
 *输入：
 *    pstProcessInfo----进程信息指针
 *返回值：
 *    整数----空闲子进程的个数
*/
int getFreeProcessNumber(const struct ProcessInfo* pstProcessInfo)
{
	return pstProcessInfo->iFreeProcessNumber;
}



/*返回一组子进程里的最大读管道文件描述符数
 *输入：
 *    pstProcessInfo----进程信息指针
 *返回值：
 *    整数----最大读管道文件描述符数
*/
int getMaxFileno(const struct ProcessInfo* pstProcessInfo)
{
	int maxFd = 0;
	int i = 0;

	for(i=0;i<pstProcessInfo->iProcessNumber;i++) {
		if(pstProcessInfo->pstOneProcessInfo[i].pipein > maxFd) {
			maxFd = pstProcessInfo->pstOneProcessInfo[i].pipein;
		}
	}
	return maxFd;
}


void killAllProcess(struct ProcessInfo** ppstProcessInfo, int processInfoNumber)
{
	int i, j;
	struct OneProcessInfo * pstOneProcessInfo = NULL;
	for(i=0;i<processInfoNumber;i++) {
		pstOneProcessInfo = ppstProcessInfo[i]->pstOneProcessInfo;
		for(j=0;j<ppstProcessInfo[i]->iProcessNumber;j++) {
			close(pstOneProcessInfo[j].pipein);
			close(pstOneProcessInfo[j].pipeout);
			kill(pstOneProcessInfo[j].pid, 9);
		}
	}
}
