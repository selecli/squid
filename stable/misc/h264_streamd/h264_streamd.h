/*
 * this file define globle variable and enum.
 */

#ifndef H264_STREAMD_H
#define H264_STREAMD_H 

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>

#include "define.h"
#include "log.h"
#include "queue.h"

extern struct glb_val g_val;
extern struct config g_cfg;
extern pthread_mutex_t mutex;
#endif

 
