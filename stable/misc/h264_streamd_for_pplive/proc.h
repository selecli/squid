#ifndef PROC_H
#define PROC_H
#include <sys/types.h>
pid_t r_waitpid(pid_t pid, int *stat_loc, int options); 
int do_sigchld_check(pid_t pid, int stat);
#endif
