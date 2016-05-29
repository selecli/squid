#include "h264_streamd.h"
#include "proc.h"

pid_t r_waitpid(pid_t pid, int *stat_loc, int options) 
{
	pid_t retval;
	while (((retval = waitpid(pid, stat_loc, options)) == -1) && (errno == EINTR));
		return retval;
}


int do_sigchld_check(pid_t pid, int stat)
{
	int termsig = WTERMSIG(stat);
	int signaled = WIFSIGNALED(stat);
	int stopped = WIFSTOPPED(stat);
	
	if (g_val.cur_proc_num>0) 
		g_val.cur_proc_num--;
	if (termsig){
		do_log(LOG_ERR, "ERR: Attention: child with pid %i died with abnormal termsignal (%i)! This is probably a bug. Please report to the author (see 'Support' at http://p3scan.sourceforge.net). numprocs is now %i", pid, termsig, g_val.cur_proc_num);
	} else if(signaled){
		do_log(LOG_DEBUG, "waitpid: child %i died with a signal! %i, numprocs is now %i", pid, WTERMSIG(stat), g_val.cur_proc_num);
	} else if(stopped){
		do_log(LOG_DEBUG, "waitpid: child %i stopped! %i, numprocs is now %i", pid, WSTOPSIG(stat), g_val.cur_proc_num);
	} else 
		do_log(LOG_DEBUG, "waitpid: child %i died with status %i, numprocs is now %i", pid, WEXITSTATUS(stat), g_val.cur_proc_num);
	return 1;
}

