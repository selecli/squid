
/*
 * $Id: helper.c,v 1.67.2.1 2008/05/04 23:23:13 hno Exp $
 *
 * DEBUG: section 84    Helper process maintenance
 * AUTHOR: Harvest Derived?
 *
 * SQUID Web Proxy Cache          http://www.squid-cache.org/
 * ----------------------------------------------------------
 *
 *  Squid is the result of efforts by numerous individuals from
 *  the Internet community; see the CONTRIBUTORS file for full
 *  details.   Many organizations have provided support for Squid's
 *  development; see the SPONSORS file for full details.  Squid is
 *  Copyrighted (C) 2001 by the Regents of the University of
 *  California; see the COPYRIGHT file for full details.  Squid
 *  incorporates software developed and/or copyrighted by other
 *  sources; see the CREDITS file for full details.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 *
 */

#include "squid.h"

#define HELPER_MAX_ARGS 64

static PF helperHandleRead;
static PF helperStatefulHandleRead;
static PF helperServerFree;
static PF helperStatefulServerFree;
static void Enqueue(helper * hlp, helper_request *);
static helper_request *Dequeue(helper * hlp);
static helper_stateful_request *StatefulDequeue(statefulhelper * hlp);
#ifndef CC_FRAMEWORK
static helper_server *GetFirstAvailable(helper * hlp);
#endif
static helper_stateful_server *StatefulGetFirstAvailable(statefulhelper * hlp);

#ifdef CC_FRAMEWORK
void helperDispatch(helper_server * srv, helper_request * r);
#else
static void helperDispatch(helper_server * srv, helper_request * r);
#endif

static void helperStatefulDispatch(helper_stateful_server * srv, helper_stateful_request * r);
static void helperKickQueue(helper * hlp);
static void helperStatefulKickQueue(statefulhelper * hlp);
static void helperRequestFree(helper_request * r);
static void helperStatefulRequestFree(helper_stateful_request * r);
static void StatefulEnqueue(statefulhelper * hlp, helper_stateful_request * r);

void
helperOpenServers(helper * hlp)
{
	char *s;
	char *progname;
	char *shortname;
	char *procname;
	const char *args[HELPER_MAX_ARGS];
	char fd_note_buf[FD_DESC_SZ];
	helper_server *srv;
	int nargs = 0;
	int k;
	pid_t pid;
	int rfd;
	int wfd;
	wordlist *w;
	void *hIpc;

	if (hlp->cmdline == NULL)
		return;
	progname = hlp->cmdline->key;
	if ((s = strrchr(progname, '/')))
		shortname = xstrdup(s + 1);
	else
		shortname = xstrdup(progname);
	debug(84, 1) ("helperOpenServers: Starting %d '%s' processes, ipc_type = %d\n",
				  hlp->n_to_start, shortname, hlp->ipc_type);
	procname = xmalloc(strlen(shortname) + 3);
	snprintf(procname, strlen(shortname) + 3, "(%s)", shortname);
	args[nargs++] = procname;
	for (w = hlp->cmdline->next; w && nargs < HELPER_MAX_ARGS; w = w->next)
		args[nargs++] = w->key;
	args[nargs++] = NULL;
	assert(nargs <= HELPER_MAX_ARGS);
	for (k = 0; k < hlp->n_to_start; k++)
	{
		getCurrentTime();
		rfd = wfd = -1;
		pid = ipcCreate(hlp->ipc_type,
						progname,
						args,
						shortname,
						&rfd,
						&wfd,
						&hIpc);
		if (pid < 0)
		{
			debug(84, 1) ("WARNING: Cannot run '%s' process.\n", progname);
			continue;
		}
		hlp->n_running++;
		hlp->n_active++;
		srv = cbdataAlloc(helper_server);
#ifdef CC_FRAMEWORK
		srv->flags.reload_stats = REDIRECT_TO_RELOAD;
#endif
		srv->hIpc = hIpc;
		srv->pid = pid;
		srv->index = k;
		srv->rfd = rfd;
		srv->wfd = wfd;
		srv->rbuf = memAllocBuf(20488, &srv->rbuf_sz);
		srv->roffset = 0;
		srv->requests = xcalloc(hlp->concurrency ? hlp->concurrency : 1, sizeof(*srv->requests));
		srv->parent = hlp;
		cbdataLock(hlp);	/* lock because of the parent backlink */
		dlinkAddTail(srv, &srv->link, &hlp->servers);
		if (rfd == wfd)
		{
			snprintf(fd_note_buf, FD_DESC_SZ, "%s #%d", shortname, k + 1);
			fd_note(rfd, fd_note_buf);
		}
		else
		{
			snprintf(fd_note_buf, FD_DESC_SZ, "reading %s #%d", shortname, k + 1);
			fd_note(rfd, fd_note_buf);
			snprintf(fd_note_buf, FD_DESC_SZ, "writing %s #%d", shortname, k + 1);
			fd_note(wfd, fd_note_buf);
		}
		commSetNonBlocking(rfd);
		if (wfd != rfd)
			commSetNonBlocking(wfd);
		comm_add_close_handler(rfd, helperServerFree, srv);
		commSetSelect(srv->rfd, COMM_SELECT_READ, helperHandleRead, srv, 0);
	}
	hlp->last_restart = squid_curtime;
	safe_free(shortname);
	safe_free(procname);
#ifdef CC_FRAMEWORK
	cc_call_hook_func_private_register_deferfunc_for_helper(hlp);
#endif
	helperKickQueue(hlp);
	debug(84, 1) ("helperOpenServers: start %d processes done~\n",
				  hlp->n_to_start);	
}

void
helperStatefulOpenServers(statefulhelper * hlp)
{
	char *s;
	char *progname;
	char *shortname;
	char *procname;
	const char *args[HELPER_MAX_ARGS];
	char fd_note_buf[FD_DESC_SZ];
	helper_stateful_server *srv;
	int nargs = 0;
	int k;
	pid_t pid;
	int rfd;
	int wfd;
	wordlist *w;
	void *hIpc;

	if (hlp->cmdline == NULL)
		return;
	progname = hlp->cmdline->key;
	if ((s = strrchr(progname, '/')))
		shortname = xstrdup(s + 1);
	else
		shortname = xstrdup(progname);
	debug(84, 1) ("helperStatefulOpenServers: Starting %d '%s' processes\n",
				  hlp->n_to_start, shortname);
	procname = xmalloc(strlen(shortname) + 3);
	snprintf(procname, strlen(shortname) + 3, "(%s)", shortname);
	args[nargs++] = procname;
	for (w = hlp->cmdline->next; w && nargs < HELPER_MAX_ARGS; w = w->next)
		args[nargs++] = w->key;
	args[nargs++] = NULL;
	assert(nargs <= HELPER_MAX_ARGS);
	for (k = 0; k < hlp->n_to_start; k++)
	{
		getCurrentTime();
		rfd = wfd = -1;
		pid = ipcCreate(hlp->ipc_type,
						progname,
						args,
						shortname,
						&rfd,
						&wfd,
						&hIpc);
		if (pid < 0)
		{
			debug(84, 1) ("WARNING: Cannot run '%s' process.\n", progname);
			continue;
		}
		hlp->n_running++;
		hlp->n_active++;
		srv = cbdataAlloc(helper_stateful_server);
		srv->hIpc = hIpc;
		srv->pid = pid;
		srv->flags.reserved = 0;
		srv->stats.submits = 0;
		srv->index = k;
		srv->rfd = rfd;
		srv->wfd = wfd;
		srv->buf = memAllocate(MEM_8K_BUF);
		srv->buf_sz = 8192;
		srv->offset = 0;
		srv->parent = hlp;
		if (hlp->datapool != NULL)
			srv->data = memPoolAlloc(hlp->datapool);
		cbdataLock(hlp);	/* lock because of the parent backlink */
		dlinkAddTail(srv, &srv->link, &hlp->servers);
		if (rfd == wfd)
		{
			snprintf(fd_note_buf, FD_DESC_SZ, "%s #%d", shortname, k + 1);
			fd_note(rfd, fd_note_buf);
		}
		else
		{
			snprintf(fd_note_buf, FD_DESC_SZ, "reading %s #%d", shortname, k + 1);
			fd_note(rfd, fd_note_buf);
			snprintf(fd_note_buf, FD_DESC_SZ, "writing %s #%d", shortname, k + 1);
			fd_note(wfd, fd_note_buf);
		}
		commSetNonBlocking(rfd);
		if (wfd != rfd)
			commSetNonBlocking(wfd);
		comm_add_close_handler(rfd, helperStatefulServerFree, srv);
		commSetSelect(srv->rfd, COMM_SELECT_READ, helperStatefulHandleRead, srv, 0);
	}
	hlp->last_restart = squid_curtime;
	safe_free(shortname);
	safe_free(procname);
	helperStatefulKickQueue(hlp);
}


void
helperSubmit(helper * hlp, const char *buf, HLPCB * callback, void *data)
{
	helper_request *r = memAllocate(MEM_HELPER_REQUEST);
	helper_server *srv;
	if (hlp == NULL)
	{
		debug(84, 3) ("helperSubmit: hlp == NULL\n");
		callback(data, NULL);
		return;
	}
	r->callback = callback;
	r->data = data;
	r->buf = xstrdup(buf);
	cbdataLock(r->data);
	if ((srv = GetFirstAvailable(hlp)))
		helperDispatch(srv, r);
	else
		Enqueue(hlp, r);
	debug(84, 9) ("helperSubmit: %s\n", buf);
}

void
helperStatefulSubmit(statefulhelper * hlp, const char *buf, HLPSCB * callback, void *data, helper_stateful_server * srv)
{
	helper_stateful_request *r = memAllocate(MEM_HELPER_STATEFUL_REQUEST);
	if (hlp == NULL)
	{
		debug(84, 3) ("helperStatefulSubmit: hlp == NULL\n");
		callback(data, 0, NULL);
		return;
	}
	r->callback = callback;
	r->data = data;
	if (buf)
		r->buf = xstrdup(buf);
	cbdataLock(r->data);
	if (!srv)
		srv = helperStatefulGetServer(hlp);
	if (srv)
	{
		debug(84, 5) ("helperStatefulSubmit: server %p, buf '%s'.\n", srv, buf ? buf : "NULL");
		assert(!srv->request);
		assert(!srv->flags.busy);
		helperStatefulDispatch(srv, r);
	}
	else
	{
		debug(84, 9) ("helperStatefulSubmit: enqueued, buf '%s'.\n", buf ? buf : "NULL");
		StatefulEnqueue(hlp, r);
	}
}

helper_stateful_server *
helperStatefulGetServer(statefulhelper * hlp)
/* find a server for this request */
{
	helper_stateful_server *srv = NULL;
	if (hlp == NULL)
	{
		debug(84, 3) ("helperStatefulGetServer: hlp == NULL\n");
		return NULL;
	}
	debug(84, 5) ("helperStatefulGetServer: Running servers %d.\n", hlp->n_running);
	if (hlp->n_running == 0)
	{
		debug(84, 1) ("helperStatefulGetServer: No running servers!. \n");
		return NULL;
	}
	srv = StatefulGetFirstAvailable(hlp);
	if (srv)
		srv->flags.reserved = 1;
	debug(84, 5) ("helperStatefulGetServer: Returning %p\n", srv);
	return srv;
}

/* puts this helper forcibly back in the queue. */
void
helperStatefulReset(helper_stateful_server * srv)
{
	statefulhelper *hlp = srv->parent;
	helper_stateful_request *r;
	debug(84, 5) ("helperStatefulReset: %p\n", srv);
	r = srv->request;
	if (r != NULL)
	{
		/* reset attempt DURING an outstaning request */
		debug(84, 1) ("helperStatefulReset: RESET During request %s \n",
					  hlp->id_name);
		srv->flags.busy = 0;
		srv->offset = 0;
		helperStatefulRequestFree(r);
		srv->request = NULL;
	}
	srv->flags.busy = 0;
	srv->flags.reserved = 0;
	if ((srv->parent->Reset != NULL) && (srv->data))
		srv->parent->Reset(srv->data);
	if (srv->flags.shutdown)
	{
		int wfd = srv->wfd;
		srv->wfd = -1;
		comm_close(wfd);
	}
	else
	{
		helperStatefulKickQueue(hlp);
	}
}

/* puts this helper back in the queue. */
void
helperStatefulReleaseServer(helper_stateful_server * srv)
{
	debug(84, 5) ("helperStatefulReleaseServer: %p\n", srv);
	assert(!srv->request);
	assert(srv->flags.reserved);
	helperStatefulReset(srv);
}

void *
helperStatefulServerGetData(helper_stateful_server * srv)
/* return a pointer to the stateful routines data area */
{
	return srv->data;
}

void
helperStats(StoreEntry * sentry, helper * hlp)
{
	dlink_node *link;
	storeAppendPrintf(sentry, "program: %s\n",
					  hlp->cmdline->key);
	storeAppendPrintf(sentry, "number running: %d of %d\n",
					  hlp->n_running, hlp->n_to_start);
	storeAppendPrintf(sentry, "requests sent: %d\n",
					  hlp->stats.requests);
	storeAppendPrintf(sentry, "replies received: %d\n",
					  hlp->stats.replies);
	storeAppendPrintf(sentry, "queue length: %d\n",
					  hlp->stats.queue_size);
	storeAppendPrintf(sentry, "avg service time: %.2f msec\n",
					  (double) hlp->stats.avg_svc_time / 1000.0);
	storeAppendPrintf(sentry, "\n");
	storeAppendPrintf(sentry, "%7s\t%7s\t%7s\t%11s\t%9s\t%s\t%7s\t%7s\t%7s\n",
					  "#",
					  "FD",
					  "PID",
					  "# Requests",
					  "# Pending",
					  "Flags",
					  "Time",
					  "Offset",
					  "Request");
	for (link = hlp->servers.head; link; link = link->next)
	{
		helper_server *srv = link->data;
		double tt = srv->requests[0] ? 0.001 *
					tvSubMsec(srv->requests[0]->dispatch_time, current_time) : 0.0;
		storeAppendPrintf(sentry, "%7d\t%7d\t%7d\t%11d\t%9d\t%c%c%c\t%7.3f\t%7d\t%s\n",
						  srv->index + 1,
						  srv->rfd,
						  srv->pid,
						  srv->stats.uses,
						  srv->stats.pending,
						  srv->stats.pending ? 'B' : ' ',
						  srv->flags.closing ? 'C' : ' ',
						  srv->flags.shutdown ? 'S' : ' ',
						  tt < 0.0 ? 0.0 : tt,
						  srv->roffset,
						  srv->requests[0] ? log_quote(srv->requests[0]->buf) : "(none)");
	}
	storeAppendPrintf(sentry, "\nFlags key:\n\n");
	storeAppendPrintf(sentry, "   B = BUSY\n");
	storeAppendPrintf(sentry, "   C = CLOSING\n");
	storeAppendPrintf(sentry, "   S = SHUTDOWN\n");
}

void
helperStatefulStats(StoreEntry * sentry, statefulhelper * hlp)
{
	helper_stateful_server *srv;
	dlink_node *link;
	double tt;
	storeAppendPrintf(sentry, "program: %s\n",
					  hlp->cmdline->key);
	storeAppendPrintf(sentry, "number running: %d of %d\n",
					  hlp->n_running, hlp->n_to_start);
	storeAppendPrintf(sentry, "requests sent: %d\n",
					  hlp->stats.requests);
	storeAppendPrintf(sentry, "replies received: %d\n",
					  hlp->stats.replies);
	storeAppendPrintf(sentry, "queue length: %d\n",
					  hlp->stats.queue_size);
	storeAppendPrintf(sentry, "avg service time: %.2f msec\n",
					  (double) hlp->stats.avg_svc_time / 1000.0);
	storeAppendPrintf(sentry, "\n");
	storeAppendPrintf(sentry, "%7s\t%7s\t%7s\t%11s\t%s\t%7s\t%7s\t%7s\n",
					  "#",
					  "FD",
					  "PID",
					  "# Requests",
					  "Flags",
					  "Time",
					  "Offset",
					  "Request");
	for (link = hlp->servers.head; link; link = link->next)
	{
		srv = link->data;
		tt = 0.001 * tvSubMsec(srv->dispatch_time,
							   srv->flags.busy ? current_time : srv->answer_time);
		storeAppendPrintf(sentry, "%7d\t%7d\t%7d\t%11d\t%c%c%c%c\t%7.3f\t%7d\t%s\n",
						  srv->index + 1,
						  srv->rfd,
						  srv->pid,
						  srv->stats.uses,
						  srv->flags.busy ? 'B' : ' ',
						  srv->flags.closing ? 'C' : ' ',
						  srv->flags.reserved ? 'R' : ' ',
						  srv->flags.shutdown ? 'S' : ' ',
						  tt < 0.0 ? 0.0 : tt,
						  srv->offset,
						  srv->request ? log_quote(srv->request->buf) : "(none)");
	}
	storeAppendPrintf(sentry, "\nFlags key:\n\n");
	storeAppendPrintf(sentry, "   B = BUSY\n");
	storeAppendPrintf(sentry, "   C = CLOSING\n");
	storeAppendPrintf(sentry, "   R = RESERVED or DEFERRED\n");
	storeAppendPrintf(sentry, "   S = SHUTDOWN\n");
}

void
helperShutdown(helper * hlp)
{
	dlink_node *link = hlp->servers.head;
#ifdef _SQUID_MSWIN_
	HANDLE hIpc;
	pid_t pid;
	int no;
#endif
	while (link)
	{
		int wfd;
		helper_server *srv;
		srv = link->data;
		link = link->next;
		if (srv->flags.shutdown)
		{
			debug(84, 3) ("helperShutdown: %s #%d is already shut down\n",
						  hlp->id_name, srv->index + 1);
			continue;
		}
		hlp->n_active--;
		assert(hlp->n_active >= 0);
		srv->flags.shutdown = 1;	/* request it to shut itself down */
		if (srv->flags.writing)
		{
			debug(84, 3) ("helperShutdown: %s #%d is BUSY.\n",
						  hlp->id_name, srv->index + 1);
			continue;
		}
		if (srv->flags.closing)
		{
			debug(84, 3) ("helperShutdown: %s #%d is CLOSING.\n",
						  hlp->id_name, srv->index + 1);
			continue;
		}
		if (srv->stats.pending)
		{
			debug(84, 3) ("helperShutdown: %s #%d is BUSY.\n",
						  hlp->id_name, srv->index + 1);
			continue;
		}
		srv->flags.closing = 1;
#ifdef _SQUID_MSWIN_
		hIpc = srv->hIpc;
		pid = srv->pid;
		no = srv->index + 1;
		shutdown(srv->wfd, SD_BOTH);
#endif
		wfd = srv->wfd;
		srv->wfd = -1;
		comm_close(wfd);
#ifdef _SQUID_MSWIN_
		if (hIpc)
		{
			if (WaitForSingleObject(hIpc, 5000) != WAIT_OBJECT_0)
			{
				getCurrentTime();
				debug(84, 1) ("helperShutdown: WARNING: %s #%d (%s,%ld) "
							  "didn't exit in 5 seconds\n",
							  hlp->id_name, no, hlp->cmdline->key, (long int) pid);
			}
			CloseHandle(hIpc);
		}
#endif
	}
}

void
helperStatefulShutdown(statefulhelper * hlp)
{
	dlink_node *link = hlp->servers.head;
	helper_stateful_server *srv;
#ifdef _SQUID_MSWIN_
	HANDLE hIpc;
	pid_t pid;
	int no;
#endif
	int wfd;
	while (link)
	{
		srv = link->data;
		link = link->next;
		if (srv->flags.shutdown)
		{
			debug(84, 3) ("helperStatefulShutdown: %s #%d is already shut down.\n",
						  hlp->id_name, srv->index + 1);
			continue;
		}
		hlp->n_active--;
		assert(hlp->n_active >= 0);
		srv->flags.shutdown = 1;	/* request it to shut itself down */
		if (srv->flags.busy)
		{
			debug(84, 3) ("helperStatefulShutdown: %s #%d is BUSY.\n",
						  hlp->id_name, srv->index + 1);
			continue;
		}
		if (srv->flags.closing)
		{
			debug(84, 3) ("helperStatefulShutdown: %s #%d is CLOSING.\n",
						  hlp->id_name, srv->index + 1);
			continue;
		}
		if (srv->flags.reserved)
		{
			debug(84, 3) ("helperStatefulShutdown: %s #%d is RESERVED.\n",
						  hlp->id_name, srv->index + 1);
			continue;
		}
		srv->flags.closing = 1;
#ifdef _SQUID_MSWIN_
		hIpc = srv->hIpc;
		pid = srv->pid;
		no = srv->index + 1;
		shutdown(srv->wfd, SD_BOTH);
#endif
		wfd = srv->wfd;
		srv->wfd = -1;
		comm_close(wfd);
#ifdef _SQUID_MSWIN_
		if (hIpc)
		{
			if (WaitForSingleObject(hIpc, 5000) != WAIT_OBJECT_0)
			{
				getCurrentTime();
				debug(84, 1) ("helperStatefulShutdown: WARNING: %s #%d (%s,%ld) "
							  "didn't exit in 5 seconds\n",
							  hlp->id_name, no, hlp->cmdline->key, (long int) pid);
			}
			CloseHandle(hIpc);
		}
#endif
	}
}


helper *
helperCreate(const char *name)
{
	helper *hlp;
	hlp = cbdataAlloc(helper);
	hlp->id_name = name;
	return hlp;
}

statefulhelper *
helperStatefulCreate(const char *name)
{
	statefulhelper *hlp;
	hlp = cbdataAlloc(statefulhelper);
	hlp->id_name = name;
	return hlp;
}


void
helperFree(helper * hlp)
{
	if (!hlp)
		return;
	/* note, don't free hlp->name, it probably points to static memory */
	if (hlp->queue.head)
		debug(84, 0) ("WARNING: freeing %s helper with %d requests queued\n",
					  hlp->id_name, hlp->stats.queue_size);
	cbdataFree(hlp);
}

void
helperStatefulFree(statefulhelper * hlp)
{
	if (!hlp)
		return;
	/* note, don't free hlp->name, it probably points to static memory */
	if (hlp->queue.head)
		debug(84, 0) ("WARNING: freeing %s helper with %d requests queued\n",
					  hlp->id_name, hlp->stats.queue_size);
	cbdataFree(hlp);
}


/* ====================================================================== */
/* LOCAL FUNCTIONS */
/* ====================================================================== */

static void
helperServerFree(int fd, void *data)
{
	helper_server *srv = data;
	helper *hlp = srv->parent;
	helper_request *r;
	int i, concurrency = hlp->concurrency;
	if (!concurrency)
		concurrency = 1;
	assert(srv->rfd == fd);
	debug(84, 1) ("%s: %s #%d (FD %d) to free\n",
				  __func__, hlp->id_name, srv->index + 1, fd);
	if (srv->rbuf)
	{
		memFreeBuf(srv->rbuf_sz, srv->rbuf);
		srv->rbuf = NULL;
	}
	if (!memBufIsNull(&srv->wqueue))
		memBufClean(&srv->wqueue);
	for (i = 0; i < concurrency; i++)
	{
		if ((r = srv->requests[i]))
		{
			if (cbdataValid(r->data))
				r->callback(r->data, NULL);
			helperRequestFree(r);
			srv->requests[i] = NULL;
		}
	}
	safe_free(srv->requests);
	if (srv->wfd != srv->rfd && srv->wfd != -1)
		comm_close(srv->wfd);
	dlinkDelete(&srv->link, &hlp->servers);
	hlp->n_running--;
	assert(hlp->n_running >= 0);
	if (!srv->flags.shutdown)
	{
		hlp->n_active--;
		assert(hlp->n_active >= 0);
		debug(84, 0) ("WARNING: %s #%d (FD %d) exited\n",
					  hlp->id_name, srv->index + 1, fd);
		if (hlp->n_active <= hlp->n_to_start / 2)
		{
			debug(84, 0) ("Too few %s processes are running\n", hlp->id_name);
			if (hlp->last_restart > squid_curtime - 30)
				fatalf("The %s helpers are crashing too rapidly, need help!\n", hlp->id_name);
			debug(84, 0) ("Starting new helpers\n");
			helperOpenServers(hlp);
		}
	}
	cbdataUnlock(srv->parent);
	cbdataFree(srv);
}

static void
helperStatefulServerFree(int fd, void *data)
{
	helper_stateful_server *srv = data;
	statefulhelper *hlp = srv->parent;
	helper_stateful_request *r;
	assert(srv->rfd == fd);
	if (srv->buf)
	{
		memFree(srv->buf, MEM_8K_BUF);
		srv->buf = NULL;
	}
	if ((r = srv->request))
	{
		if (cbdataValid(r->data))
			r->callback(r->data, srv, srv->buf);
		helperStatefulRequestFree(r);
		srv->request = NULL;
	}
	/* TODO: walk the local queue of requests and carry them all out */
	if (srv->wfd != srv->rfd && srv->wfd != -1)
		comm_close(srv->wfd);
	dlinkDelete(&srv->link, &hlp->servers);
	hlp->n_running--;
	assert(hlp->n_running >= 0);
	if (!srv->flags.shutdown)
	{
		hlp->n_active--;
		assert(hlp->n_active >= 0);
		debug(84, 0) ("WARNING: %s #%d (FD %d) exited\n",
					  hlp->id_name, srv->index + 1, fd);
		if (hlp->n_active <= hlp->n_to_start / 2)
		{
			debug(84, 0) ("Too few %s processes are running\n", hlp->id_name);
			if (hlp->last_restart > squid_curtime - 30)
				fatalf("The %s helpers are crashing too rapidly, need help!\n", hlp->id_name);
			debug(84, 0) ("Starting new helpers\n");
			helperStatefulOpenServers(hlp);
		}
	}
	if (srv->data != NULL)
		memPoolFree(hlp->datapool, srv->data);
	cbdataUnlock(srv->parent);
	cbdataFree(srv);
}


static void
helperHandleRead(int fd, void *data)
{
	int len;
	char *t = NULL;
	helper_server *srv = data;
	helper *hlp = srv->parent;
	assert(fd == srv->rfd);
	assert(cbdataValid(data));
	statCounter.syscalls.sock.reads++;
	/* XXX srv->rbuf should be reallocated if needed.. and start out quite small (not fixed 8KB as now..) */
	assert(srv->roffset < srv->rbuf_sz);
	len = FD_READ_METHOD(fd, srv->rbuf + srv->roffset, srv->rbuf_sz - srv->roffset - 1);
	fd_bytes(fd, len, FD_READ);
	debug(84, 5) ("helperHandleRead: %d bytes from %s #%d.\n",
				  len, hlp->id_name, srv->index + 1);
	if (len == 0)
	{
		comm_close(fd);
		return;
	}
	commSetSelect(fd, COMM_SELECT_READ, helperHandleRead, srv, 0);
	if (len < 0)
	{
		if (!ignoreErrno(errno))
		{
			debug(84, 1) ("helperHandleRead: FD %d read: %s\n", fd, xstrerror());
			comm_close(fd);
		}
		return;
	}
	srv->roffset += len;
	srv->rbuf[srv->roffset] = '\0';
	debug(84, 9) ("helperHandleRead: '%s'\n", srv->rbuf);
	if (!srv->stats.pending)
	{
		/* someone spoke without being spoken to */
		debug(84, 1) ("helperHandleRead: unexpected read from %s #%d, %d bytes '%s'\n",
					  hlp->id_name, srv->index + 1, len, srv->rbuf);
		srv->roffset = 0;
		srv->rbuf[0] = '\0';
	}
	while ((t = strchr(srv->rbuf, '\n')))
	{
		helper_request *r;
		char *msg = srv->rbuf;
		int i = 0;
		/* end of reply found */
		debug(84, 3) ("helperHandleRead: end of reply found: %s\n", srv->rbuf);
		if (t > srv->rbuf && t[-1] == '\r')
			t[-1] = '\0';
		*t++ = '\0';
		if (hlp->concurrency)
		{
			errno = 0;
			i = strtol(msg, &msg, 10);
			if (msg == srv->rbuf || errno)
				i = -1;
			while (*msg && xisspace(*msg))
				msg++;
		}
		if ((!hlp->concurrency) || (i >= 0 && i < hlp->concurrency))
			r = srv->requests[i];
		else
			r = NULL;
		if (r)
		{
			srv->requests[i] = NULL;
			if (cbdataValid(r->data))
				r->callback(r->data, msg);
			srv->stats.pending--;
			hlp->stats.replies++;
			hlp->stats.avg_svc_time =
				intAverage(hlp->stats.avg_svc_time,
						   tvSubUsec(r->dispatch_time, current_time),
						   hlp->stats.replies, REDIRECT_AV_FACTOR);
			helperRequestFree(r);
		}
		else
		{
			debug(84, 1) ("helperHandleRead: unexpected reply on channel %d from %s #%d '%s'\n",
						  i, hlp->id_name, srv->index + 1, srv->rbuf);
		}
		srv->roffset -= (t - srv->rbuf);
		memmove(srv->rbuf, t, srv->roffset + 1);
	}
	if (srv->flags.shutdown && !srv->stats.pending)
	{
		if (!srv->flags.closing)
		{
			int wfd = srv->wfd;
			srv->flags.closing = 1;
			srv->wfd = -1;
			comm_close(wfd);
		}
	}
	else
	{
		helperKickQueue(hlp);
	}
}

static void
helperStatefulHandleRead(int fd, void *data)
{
	int len;
	char *t = NULL;
	helper_stateful_server *srv = data;
	helper_stateful_request *r;
	statefulhelper *hlp = srv->parent;
	assert(fd == srv->rfd);
	assert(cbdataValid(data));
	statCounter.syscalls.sock.reads++;
	len = FD_READ_METHOD(fd, srv->buf + srv->offset, srv->buf_sz - srv->offset);
	fd_bytes(fd, len, FD_READ);
	debug(84, 5) ("helperStatefulHandleRead: %d bytes from %s #%d.\n",
				  len, hlp->id_name, srv->index + 1);
	if (len <= 0)
	{
		if (len < 0)
			debug(84, 1) ("helperStatefulHandleRead: FD %d read: %s\n", fd, xstrerror());
		comm_close(fd);
		return;
	}
	commSetSelect(srv->rfd, COMM_SELECT_READ, helperStatefulHandleRead, srv, 0);
	srv->offset += len;
	srv->buf[srv->offset] = '\0';
	r = srv->request;
	if (r == NULL)
	{
		/* someone spoke without being spoken to */
		debug(84, 1) ("helperStatefulHandleRead: unexpected read from %s #%d, %d bytes\n",
					  hlp->id_name, srv->index + 1, len);
		srv->offset = 0;
	}
	else if ((t = strchr(srv->buf, '\n')))
	{
		/* end of reply found */
		debug(84, 3) ("helperStatefulHandleRead: end of reply found\n");
		if (t > srv->buf && t[-1] == '\r')
			t[-1] = '\0';
		*t = '\0';
		srv->flags.busy = 0;
		srv->offset = 0;
		srv->request = NULL;
		hlp->stats.replies++;
		srv->answer_time = current_time;
		hlp->stats.avg_svc_time =
			intAverage(hlp->stats.avg_svc_time,
					   tvSubUsec(srv->dispatch_time, current_time),
					   hlp->stats.replies, REDIRECT_AV_FACTOR);
		if (cbdataValid(r->data))
		{
			r->callback(r->data, srv, srv->buf);
		}
		else
		{
			debug(84, 1) ("helperStatefulHandleRead: no callback data registered\n");
		}
		helperStatefulRequestFree(r);
	}
}

static void
Enqueue(helper * hlp, helper_request * r)
{
	dlink_node *link = memAllocate(MEM_DLINK_NODE);
	dlinkAddTail(r, link, &hlp->queue);
	hlp->stats.queue_size++;
	if (hlp->stats.queue_size < hlp->n_running)
		return;
	if (hlp->stats.queue_size > hlp->stats.max_queue_size)
		hlp->stats.max_queue_size = hlp->stats.queue_size;
	if (squid_curtime - hlp->last_queue_warn < 30)
		return;
	if (shutting_down || reconfiguring)
		return;
	debug(84, 1) ("WARNING: All %s processes are busy.\n", hlp->id_name);
	debug(84, 1) ("WARNING: up to %d pending requests queued\n", hlp->stats.max_queue_size);
	if (hlp->stats.queue_size > hlp->n_running * 2)
		fatalf("Too many queued %s requests (%d on %d)", hlp->id_name, hlp->stats.queue_size, hlp->n_running);
	if (squid_curtime - hlp->last_queue_warn < 300)
		debug(84, 1) ("Consider increasing the number of %s processes to at least %d in your config file.\n", hlp->id_name, hlp->n_running + hlp->stats.max_queue_size);
	hlp->last_queue_warn = squid_curtime;
	hlp->stats.max_queue_size = hlp->stats.queue_size;
}

static void
StatefulEnqueue(statefulhelper * hlp, helper_stateful_request * r)
{
	dlink_node *link = memAllocate(MEM_DLINK_NODE);
	dlinkAddTail(r, link, &hlp->queue);
	hlp->stats.queue_size++;
	if (hlp->stats.queue_size < hlp->n_running)
		return;
	if (hlp->stats.queue_size > hlp->stats.max_queue_size)
		hlp->stats.max_queue_size = hlp->stats.queue_size;
	if (hlp->stats.queue_size > hlp->n_running * 5)
		fatalf("Too many queued %s requests (%d on %d)", hlp->id_name, hlp->stats.queue_size, hlp->n_running);
	if (squid_curtime - hlp->last_queue_warn < 30)
		return;
	if (shutting_down || reconfiguring)
		return;
	debug(84, 1) ("WARNING: All %s processes are busy.\n", hlp->id_name);
	debug(84, 1) ("WARNING: up to %d pending requests queued\n", hlp->stats.max_queue_size);
	if (squid_curtime - hlp->last_queue_warn < 300)
		debug(84, 1) ("Consider increasing the number of %s processes to at least %d in your config file.\n", hlp->id_name, hlp->n_running + hlp->stats.max_queue_size);
	hlp->last_queue_warn = squid_curtime;
	hlp->stats.max_queue_size = hlp->stats.queue_size;
}

static helper_request *
Dequeue(helper * hlp)
{
	dlink_node *link;
	helper_request *r = NULL;
	if ((link = hlp->queue.head))
	{
		r = link->data;
		dlinkDelete(link, &hlp->queue);
		memFree(link, MEM_DLINK_NODE);
		hlp->stats.queue_size--;
	}
	return r;
}

static helper_stateful_request *
StatefulDequeue(statefulhelper * hlp)
{
	dlink_node *link;
	helper_stateful_request *r = NULL;
	if ((link = hlp->queue.head))
	{
		r = link->data;
		dlinkDelete(link, &hlp->queue);
		memFree(link, MEM_DLINK_NODE);
		hlp->stats.queue_size--;
	}
	return r;
}
#ifdef CC_FRAMEWORK
helper_server *
#else
static helper_server *
#endif
GetFirstAvailable(helper * hlp)
{
	dlink_node *n;
	helper_server *srv;
	helper_server *selected = NULL;
	if (hlp->n_running == 0)
		return NULL;
	/* Find "least" loaded helper (approx) */
	for (n = hlp->servers.head; n != NULL; n = n->next)
	{
		srv = n->data;
		if (selected && selected->stats.pending <= srv->stats.pending)
			continue;
		if (srv->flags.shutdown)
			continue;
		if (srv->flags.closing)
			continue;
#ifdef CC_FRAMEWORK
		if (srv->flags.reload_stats == REDIRECT_RELOADING ||
				srv->flags.reload_stats == REDIRECT_TO_RELOAD_FULLLOAD)				
			continue;
#endif
		if (selected)
		{
			selected = srv;
			break;
		}
		selected = srv;
		if (!selected->stats.pending)
			break;
	}
	/* Check for overload */
	if (!selected)
		return NULL;
	if (selected->stats.pending >= (hlp->concurrency ? hlp->concurrency : 1))
		return NULL;

	return selected;
}

static helper_stateful_server *
StatefulGetFirstAvailable(statefulhelper * hlp)
{
	dlink_node *n;
	helper_stateful_server *srv = NULL;
	debug(84, 5) ("StatefulGetFirstAvailable: Running servers %d.\n", hlp->n_running);
	if (hlp->n_running == 0)
		return NULL;
	for (n = hlp->servers.head; n != NULL; n = n->next)
	{
		srv = n->data;
		if (srv->flags.busy)
			continue;
		if (srv->flags.reserved)
			continue;
		if (srv->flags.shutdown)
			continue;
		if ((hlp->IsAvailable != NULL) && (srv->data != NULL) && !(hlp->IsAvailable(srv->data)))
			continue;
		return srv;
	}
	debug(84, 5) ("StatefulGetFirstAvailable: None available.\n");
	return NULL;
}


static void
helperDispatch_done(int fd, char *buf, size_t size, int status, void *data)
{
	helper_server *srv = data;
	if (status != COMM_OK)
	{
		/* Helper server has crashed.. */
		debug(84, 0) ("ERROR: Helper on fd %d has crashed!\n", fd);
	}
	else if (!memBufIsNull(&srv->wqueue))
	{
		MemBuf mb = srv->wqueue;
		srv->wqueue = MemBufNull;
		comm_write_mbuf(srv->wfd,
						mb,
						helperDispatch_done,	/* Handler */
						srv);
	}
	else
	{
		helper *hlp = srv->parent;
		srv->flags.writing = 0;	/* done */
		if (srv->flags.shutdown)
		{
			int wfd;
			debug(84, 3) ("helperDispatch_done: %s #%d is shutting down.\n",
						  hlp->id_name, srv->index + 1);
			if (srv->flags.closing)
			{
				debug(84, 3) ("helperDispatch_done: %s #%d is CLOSING.\n",
							  hlp->id_name, srv->index + 1);
				return;
			}
			if (srv->stats.pending)
			{
				debug(84, 3) ("helperDispatch_done: %s #%d is BUSY.\n",
							  hlp->id_name, srv->index + 1);
				return;
			}
			srv->flags.closing = 1;
			wfd = srv->wfd;
			srv->wfd = -1;
			comm_close(wfd);
		}
	}
}

#ifdef CC_FRAMEWORK
void 
#else 
static void
#endif
helperDispatch(helper_server * srv, helper_request * r)
{
	helper *hlp = srv->parent;
	helper_request **ptr = NULL;
	int slot = -1;
	if (!cbdataValid(r->data))
	{
		debug(84, 1) ("helperDispatch: invalid callback data\n");
		helperRequestFree(r);
		return;
	}
	for (slot = 0; slot < (hlp->concurrency ? hlp->concurrency : 1); slot++)
	{
		if (!srv->requests[slot])
		{
			ptr = &srv->requests[slot];
			break;
		}
	}
	assert(ptr);
	*ptr = r;
	srv->stats.pending += 1;
	r->dispatch_time = current_time;
	if (memBufIsNull(&srv->wqueue))
		memBufDefInit(&srv->wqueue);
	if (hlp->concurrency)
		memBufPrintf(&srv->wqueue, "%d %s", slot, r->buf);
	else
		memBufAppend(&srv->wqueue, r->buf, strlen(r->buf));
	if (!srv->flags.writing)
	{
		MemBuf mb = srv->wqueue;
		srv->wqueue = MemBufNull;
		srv->flags.writing = 1;
		comm_write_mbuf(srv->wfd,
						mb,
						helperDispatch_done,	/* Handler */
						srv);
	}
	debug(84, 5) ("helperDispatch: Request sent to %s #%d[%d], %d bytes\n",
				  hlp->id_name, srv->index + 1, slot, (int) strlen(r->buf));
	srv->stats.uses++;
	hlp->stats.requests++;
}

static void
helperStatefulDispatch(helper_stateful_server * srv, helper_stateful_request * r)
{
	statefulhelper *hlp = srv->parent;
	if (!cbdataValid(r->data))
	{
		debug(84, 1) ("helperStatefulDispatch: invalid callback data\n");
		helperStatefulRequestFree(r);
		return;
	}
	if (!r->buf)
	{
		if (cbdataValid(r->data))
		{
			r->callback(r->data, srv, NULL);
		}
		else
		{
			debug(84, 1) ("helperStatefulDispatch: no callback data registered\n");
		}
		helperStatefulRequestFree(r);
		return;
	}
	debug(84, 9) ("helperStatefulDispatch busying helper %s #%d\n", hlp->id_name, srv->index + 1);
	srv->flags.busy = 1;
	srv->request = r;
	srv->dispatch_time = current_time;
	comm_write(srv->wfd,
			   r->buf,
			   strlen(r->buf),
			   NULL,			/* Handler */
			   NULL,			/* Handler-data */
			   NULL);			/* free */
	debug(84, 5) ("helperStatefulDispatch: Request sent to %s #%d, %d bytes\n",
				  hlp->id_name, srv->index + 1, (int) strlen(r->buf));
	srv->stats.uses++;
	hlp->stats.requests++;
}


static void
helperKickQueue(helper * hlp)
{
	helper_request *r;
	helper_server *srv;
	while ((srv = GetFirstAvailable(hlp)) && (r = Dequeue(hlp)))
		helperDispatch(srv, r);
}

static void
helperStatefulKickQueue(statefulhelper * hlp)
{
	helper_stateful_request *r;
	helper_stateful_server *srv;
	while ((srv = StatefulGetFirstAvailable(hlp)) && (r = StatefulDequeue(hlp)))
	{
		srv->flags.reserved = 1;
		helperStatefulDispatch(srv, r);
	}
}

static void
helperRequestFree(helper_request * r)
{
	cbdataUnlock(r->data);
	xfree(r->buf);
	memFree(r, MEM_HELPER_REQUEST);
}

static void
helperStatefulRequestFree(helper_stateful_request * r)
{
	cbdataUnlock(r->data);
	xfree(r->buf);
	memFree(r, MEM_HELPER_STATEFUL_REQUEST);
}
#ifdef CC_FRAMEWORK
helper_server *helperServerOpenOne(int ipc_type, int idx, wordlist *cmdline, int concurrency)
{
	char *s;
	char *progname;
	char *shortname;
	char *procname;
	const char *args[HELPER_MAX_ARGS];
	char fd_note_buf[FD_DESC_SZ];
	helper_server *srv;
	int nargs = 0;
	pid_t pid;
	int rfd;
	int wfd;
	wordlist *w;
	void *hIpc;
	if (NULL == cmdline)
	{
		return (NULL);
	}
	progname = cmdline->key;
	s = strrchr(progname, '/');
	if (NULL != s)
	{
		shortname = xstrdup(s + 1);
	}
	else
	{
		shortname = xstrdup(progname);
	}
	debug(84, 1) ("%s: Starting '%s' processes, ipc_type = %d\n", __func__, shortname, ipc_type);
	procname = xmalloc(strlen(shortname) + 3);
	snprintf(procname, strlen(shortname) + 3, "(%s)", shortname);
	args[nargs ++] = procname;
	for (w = cmdline->next; w && nargs < HELPER_MAX_ARGS; w = w->next)
	{
		args[nargs ++] = w->key;
	}
	args[nargs ++] = NULL;
	assert(nargs <= HELPER_MAX_ARGS);
	if( 1 )
	{
		getCurrentTime();
		rfd = wfd = -1;
		pid = ipcCreate(ipc_type,
						progname,
						args,
						shortname,
						&rfd,
						&wfd,
						&hIpc);
		if (0 > pid)
		{
			debug(84, 1) ("WARNING: %s: Cannot run '%s' process.\n", __func__, progname);
        	safe_free(shortname);
        	safe_free(procname);
			return (NULL);/*give up*/
		}
		srv = cbdataAlloc(helper_server);
		srv->flags.reload_stats = REDIRECT_TO_RELOAD;
		srv->hIpc     = hIpc;
		srv->pid      = pid;
		srv->index    = idx;
		srv->rfd      = rfd;
		srv->wfd      = wfd;
		srv->rbuf     = memAllocBuf(20488, &srv->rbuf_sz);
		srv->roffset  = 0;
		srv->requests = xcalloc(concurrency ? concurrency : 1, sizeof(*srv->requests));
		srv->parent   = NULL;/*set invalid*/
		if (rfd == wfd)
		{
			snprintf(fd_note_buf, FD_DESC_SZ, "%s #%d", shortname, idx + 1);
			fd_note(rfd, fd_note_buf);
		}
		else
		{
			snprintf(fd_note_buf, FD_DESC_SZ, "reading %s #%d", shortname, idx + 1);
			fd_note(rfd, fd_note_buf);
			snprintf(fd_note_buf, FD_DESC_SZ, "writing %s #%d", shortname, idx + 1);
			fd_note(wfd, fd_note_buf);
		}
		commSetNonBlocking(rfd);
		if (wfd != rfd)
		{
			commSetNonBlocking(wfd);		
		}
	}
	safe_free(shortname);
	safe_free(procname);
    return (srv);				  
}
void helperServerOpenAll(helper *hlp)
{
	char *progname;
	helper_server *srv;
	int k;
	if (hlp->cmdline == NULL)
	{
		return;
	}
	progname = hlp->cmdline->key;
	for (k = hlp->n_running; k < hlp->n_to_start; k ++)
	{
	    srv = helperServerOpenOne(hlp->ipc_type, k, hlp->cmdline, hlp->concurrency);
	    if(NULL == srv)
	    {
			debug(84, 1) ("WARNING: %s: Cannot run '%s' process.\n", __func__, progname);
			continue;
		}
		srv->index  = k;  /*revise*/
		srv->parent = hlp;
		hlp->n_running ++;
		hlp->n_active ++;		
		cbdataLock(hlp);	/* lock because of the parent backlink */
		dlinkAddTail(srv, &srv->link, &hlp->servers);
		comm_add_close_handler(srv->rfd, helperServerFree, srv);
		commSetSelect(srv->rfd, COMM_SELECT_READ, helperHandleRead, srv, 0);
	}
	hlp->last_restart = squid_curtime;
	cc_call_hook_func_private_register_deferfunc_for_helper(hlp);
	helperKickQueue(hlp);
	debug(84, 1) ("%s: start %d processes done~\n", __func__, hlp->n_to_start);	
	return;
}
void helperServerOpenAllDone(helper *hlp)
{
	hlp->last_restart = squid_curtime;
	cc_call_hook_func_private_register_deferfunc_for_helper(hlp);
	helperKickQueue(hlp);
	debug(84, 1) ("%s: start %d processes done~\n", __func__, hlp->n_to_start);	
    return;
}
void helperServerOpenAllStart(helper *hlp)
{
	char *progname;
	helper_server *srv;
	int k;
	if (hlp->cmdline == NULL)
	{
		return;
	}
	progname = hlp->cmdline->key;
	k = hlp->n_running;
    if(Config.onoff.debug_80_log)
    {
        debug(84, 1) ("[DEBUG] %s: n_running %d, n_to_start %d\n", 
                      __func__, hlp->n_running, hlp->n_to_start);	
	}
	if(k < hlp->n_to_start)
	{
	    srv = helperServerOpenOne(hlp->ipc_type, k, hlp->cmdline, hlp->concurrency);
	    if(NULL == srv)
	    {
			debug(84, 1) ("WARNING: %s: Cannot run '%s' process.\n", __func__, progname);
			eventAdd("helperServerOpenAllStart", (EVH *)helperServerOpenAllStart, hlp, 0.1, 0);
			return;
		}
		srv->index  = k;  /*revise*/
		srv->parent = hlp;
		hlp->n_running ++;
		hlp->n_active ++;		
		cbdataLock(hlp);	/* lock because of the parent backlink */
		dlinkAddTail(srv, &srv->link, &hlp->servers);
		comm_add_close_handler(srv->rfd, helperServerFree, srv);
		commSetSelect(srv->rfd, COMM_SELECT_READ, helperHandleRead, srv, 0);
        eventAdd("helperServerOpenAllStart", (EVH *)helperServerOpenAllStart, hlp, 0.1, 0);		
        return;
	}
	helperServerOpenAllDone(hlp);
	return;
}
#define HELPER_SRV_WAS_SHUTDOWN     ((int) 1)
#define HELPER_SRV_IS_BUSY          ((int) 2)
#define HELPER_SRV_HAS_PENDING      ((int) 3)
#define HELPER_SRV_IS_CLOSING       ((int) 4)
#define HELPER_SRV_DO_SHUTDOWN      ((int) 5)
static int __helperServerShutdownOne(helper_server *srv)
{
	int wfd;
	assert(1 == srv->flags.shutdown);
	if (srv->flags.writing)
	{
	    eventAdd("__helperServerShutdownOne", (EVH *)__helperServerShutdownOne, srv, 0.1, 0);		
	    return (HELPER_SRV_IS_BUSY);
	}
	if (srv->flags.closing)
	{   
	    return (HELPER_SRV_IS_CLOSING);
	}
	if (srv->stats.pending)
	{
	    eventAdd("__helperServerShutdownOne", (EVH *)__helperServerShutdownOne, srv, 0.1, 0);
	    return (HELPER_SRV_HAS_PENDING);
	}
	srv->flags.closing = 1;
	debug(84, 1) ("%s: %s #%d to close\n", __func__, srv->parent->id_name, srv->index + 1);
	wfd = srv->wfd;
	srv->wfd = -1;
	comm_close(wfd);
	return (HELPER_SRV_DO_SHUTDOWN);
}
int helperServerShutdownOne(helper_server *srv)
{
	if (srv->flags.shutdown)
	{
	    return (HELPER_SRV_WAS_SHUTDOWN);
	}
	srv->flags.shutdown = 1;	/* request it to shut itself down */
	return __helperServerShutdownOne(srv);
}

void helperServerShutdownRange(helper *hlp, const int srv_idx_from, const int srv_idx_to)
{
    int srv_idx;
    if(Config.onoff.debug_80_log)
    {
        debug(84, 1) ("[DEBUG] %s: n_running %d, n_to_start %d, srv_idx_from %d, srv_idx_to %d\n", 
                      __func__, hlp->n_running, hlp->n_to_start, srv_idx_from, srv_idx_to);    
    }
    for(srv_idx = srv_idx_from; srv_idx < srv_idx_to; srv_idx ++)
    {
        helper_server *srv;
        srv = helperServerFetchByIndex(hlp, srv_idx);
        if(NULL == srv)
        {
            continue;
        }
        helperServerShutdownOne(srv);
    }
    return;
}
void helperServerShutdownAll(helper *hlp)
{
    helperServerShutdownRange(hlp, 0, hlp->n_running);
    return;
}
int  helperServerIsIdle(helper_server *srv)
{
    if(0 == srv->stats.pending) /*no more request pending in srv*/
    {
        return (1);/*is idle*/
    }
    return (0);/*not idle*/
}
void helperServerClose(helper_server *srv)
{
	helper *hlp = srv->parent;
	helper_request *r;
	int i;
	int concurrency = hlp->concurrency;
	if (!concurrency)
	{
		concurrency = 1;
	}
	if (srv->rbuf)
	{
		memFreeBuf(srv->rbuf_sz, srv->rbuf);
		srv->rbuf = NULL;
	}
	if (!memBufIsNull(&srv->wqueue))
	{
		memBufClean(&srv->wqueue);
	}
	for (i = 0; i < concurrency; i++)
	{
		if ((r = srv->requests[i]))
		{
			if (cbdataValid(r->data))
			{
				r->callback(r->data, NULL);
			}
			helperRequestFree(r);
			srv->requests[i] = NULL;
		}
	}
	safe_free(srv->requests);
	if (srv->wfd != srv->rfd && srv->wfd != -1)
	{
		comm_close(srv->wfd);
	}
	dlinkDelete(&srv->link, &hlp->servers);
	cbdataUnlock(srv->parent);
	cbdataFree(srv);
	return;
}
helper_server *helperServerFetchByIndex(helper *hlp, int srv_idx)
{
    dlink_node *link;
	for (link = hlp->servers.head; NULL != link; link = link->next)
	{
		helper_server *srv;
		srv = link->data;
		if(srv_idx == srv->index)
		{
		    debug(84, 1) ("%s: find %s idx #%d\n", __func__, hlp->id_name, srv_idx + 1);
		    return (srv);
		}
	}
	debug(84, 1) ("%s: NOT find %s idx #%d\n", __func__, hlp->id_name, srv_idx + 1);
	return (NULL);
}
void helperServerReplaceOneStart(helper_server *srv_new)
{
    int         srv_idx;
    helper     *hlp;    
    helper_server *srv_old;
    srv_idx = srv_new->index;
    hlp     = srv_new->parent;
    srv_old = helperServerFetchByIndex(hlp, srv_idx);
	if(NULL == srv_old)
	{
	    debug(84, 1) ("error:%s: NOT find %s idx #%d\n", __func__, hlp->id_name, srv_idx + 1);
	    return;
	}
    if(helperServerIsIdle(srv_old))/*when old is idle, replace it*/
    {
        helperServerReplaceOneDone(srv_new, srv_old);
        return;
    }
    eventAdd("helperServerReplaceOneStart", (EVH *)helperServerReplaceOneStart, srv_new, 0.1, 0);
	return;
}
int helperServerReplaceOneDone(helper_server *srv_new, helper_server *srv_old)
{
    int         srv_idx;
    helper     *hlp;
    srv_idx = srv_old->index;
    hlp     = srv_old->parent;
	if(srv_old != helperServerFetchByIndex(hlp, srv_idx))
	{
	    debug(84, 1) ("%s: NOT find %s idx #%d\n", __func__, hlp->id_name, srv_idx + 1);
	    return (-1);
	}
	cbdataLock(hlp);	/* lock because of the parent backlink */
	dlinkAddTail(srv_new, &srv_new->link, &hlp->servers);
	hlp->n_running ++;
	hlp->n_active ++;		
	helperServerShutdownOne(srv_old);
	comm_add_close_handler(srv_new->rfd, helperServerFree, srv_new);
	commSetSelect(srv_new->rfd, COMM_SELECT_READ, helperHandleRead, srv_new, 0);	
	debug(84, 1) ("%s: %s idx #%d was replaced\n", __func__, hlp->id_name, srv_idx + 1);
	eventAdd("helperServerReplaceAll", (EVH *)helperServerReplaceAll, hlp, 0.1, 0);	
	return (0);
}
void helperServerReplaceOne(helper_server *srv_old)
{
    helper        *hlp;
    helper_server *srv_new;
    int            srv_idx;
    srv_idx = srv_old->index;
    hlp     = srv_old->parent;
    srv_new = helperServerOpenOne(hlp->ipc_type, srv_idx, hlp->cmdline, hlp->concurrency);
    if(NULL == srv_new)
    {
        debug(84, 1) ("error: %s: open server %d# failed\n", __func__, srv_idx);
        return;
    }
	srv_new->index  = srv_idx;  /*revise*/
	srv_new->parent = hlp;    
	helperServerReplaceOneStart(srv_new);
    return;
}
static void __helperServerReplaceOne(helper_server *srv_old)
{
    eventAdd("helperServerReplaceOne", (EVH *)helperServerReplaceOne, srv_old, 0.1, 0);
    return;
}
void helperServerReplaceAll(helper *hlp)
{
    static int srv_idx_to_replace = 0;

    helper_server *srv;

    if(Config.onoff.debug_80_log)
    {
        debug(84, 1) ("[DEBUG] %s: srv_idx_to_replace %d, n_running %d, n_to_start %d\n", 
                      __func__, srv_idx_to_replace, hlp->n_running, hlp->n_to_start);
    }
    if(srv_idx_to_replace >= hlp->n_running || srv_idx_to_replace >= hlp->n_to_start)
    {
        srv_idx_to_replace = 0;/*no more to replace, clear idx for next time replace action*/

        if(hlp->n_running < hlp->n_to_start) /*which means need to open some new helpers*/
        {
            helperServerOpenAllStart(hlp);
            return;
        }
        if(hlp->n_running > hlp->n_to_start) /*which means need to close some helpers*/
        {
            helperServerShutdownRange(hlp, hlp->n_to_start, hlp->n_running);
            return;
        }
        return;
    }
    srv = helperServerFetchByIndex(hlp, srv_idx_to_replace);
    srv_idx_to_replace ++;
	if(NULL != srv)
	{
	    __helperServerReplaceOne(srv);
	    return;
	}
	return;
}
void helperReloadServers(helper *hlp)
{
    return helperServerReplaceAll(hlp);
}
#endif/*CC_FRAMEWORK*/
