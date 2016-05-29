#include "cc_framework_api.h"
#include <stdbool.h>

#ifdef CC_FRAMEWORK
static helper *media_prefetchers = NULL;

void clientWriteComplete(int fd, char *bufnotused, size_t size, int errflag, void *data);
void clientSendMoreData(void *data, char *buf, ssize_t size);
char *storeAufsDirFullPath(SwapDir * SD, sfileno filn, char *fullpath);

void clientProcessRequest(clientHttpRequest *);
static int mod_close_mp4_server(int fd);
typedef void CHRP(clientHttpRequest *);
static cc_module *mod = NULL;

static int is_start_helper = 0;
static int helper_proc_num = 60;

//static int mp4_sock_flag = 2;
typedef struct _media_prefetch_state_data{
	clientHttpRequest * http; 
	CHRP *callback;
}media_prefetch_state_data;

CBDATA_TYPE(media_prefetch_state_data);

typedef struct _mp4WriteStateData {	
	int fd;	
	char *buf;	
	size_t size;	
	CWCB *handler;	
	void *handler_data;
	FREE *free_func;
} mp4WriteStateData;

//CBDATA_TYPE(mp4WriteStateData);

typedef struct _mod_config
{
	char	start[128];
	char	end[128];
	char    type[128];
	int     flag;  //1 start seconds 2 percent 3 offset
//	int	is_miss_wait;
	int     miss_strategy; //miss strategy for 0 : query_total :query total object, 1 miss_wait  2 miss_start 3 query_start
} mod_config;

typedef struct _request_param
{
	int mp4_flag; //add by sxlxb for mp4
	squid_off_t mp4_start; //add by sxlxb for mp4
	squid_off_t mp4_end;  //add by sxlxb for mp4
	int mp4_rep_sz; //add by sxlxb for mp4
	char *uri;
	int  succ;

} request_param;

typedef struct _fde_data
{
	int mp4_moov_ok; //add by sxlxb for mp4
	int mp4_fd; //add by sxlxb for mp4

  	mp4WriteStateData *mp4_data; //add by sxlxb for mp4
} fde_data;

typedef struct _mp4_req {
	char obj_path[256];
	int mp4_offset;
	char url[256];
	int start;
	int end;
	int percent;
} mp4_req;

#define MOOV_PID_FILE	"/var/run/moov_generator_for_pplive.pid"

static int httpHeaderCheckExistByName(HttpHeader *h, char *name)
{
	HttpHeaderPos pos = HttpHeaderInitPos;
	HttpHeaderEntry *curHE;
	while ((curHE = httpHeaderGetEntry(h, &pos)))
	{
		if(!strcmp(strBuf(curHE->name), name))
		{
			debug(189,3)("Internal X-CC-Media-Prefetch request!\n");
			return 1;
		}
	}

	debug(189,3)("Not internal X-CC-Media-Prefetch request!\n");
	return 0;
}

static int free_cfg(void *data)
{
	safe_free(data);
	return 0;
}

static int free_fde_data(void *data){
	fde_data* fd_data = data;
	if(fd_data != NULL){
		fd_data->mp4_fd = 0;
          	fd_data->mp4_moov_ok = 2;
          
		//mp4WriteStateData *state = fd_data->mp4_data;
		//if(state != NULL){
			
		//	safe_free(state->buf);
		//}
		//safe_free(fd_data->mp4_data);
		safe_free(fd_data);
	}
	return 0;
}

static int free_request_param(void* data)
{
	request_param *req_param = (request_param *) data;
	if(req_param != NULL){
		safe_free(req_param->uri);
		safe_free(req_param);
		req_param = NULL;
	}
	return 0;
}
static void parse_programline(wordlist ** line)
{
	if (*line) 
		self_destruct();
	parse_wordlist(line);
}

static int close_client_fd(int fd){
	
	fde *F = &fd_table[fd];
	if (F->flags.open){
		comm_close(fd);
		return 0;
	}	
	return 1;


}
static int close_mp4_fd(int mp4_sock,int client_fd){
         fd_close(mp4_sock); //add by sxlxb 1223
         close(mp4_sock);
         fde_data* fd_data = cc_get_mod_private_data(FDE_PRIVATE_DATA, &client_fd, mod);
         if(fd_data == NULL){
        
               debug(189,5)("mod_pplive: when close mp4 fd fde_data param is null\n");
               return 1;
          }

          fd_data->mp4_fd = 0;
          fd_data->mp4_moov_ok = 2;
          return 0;
}

//derDelById(hdr, HDR_RANGE);add by sxlxb for mp4
static void wipeOutHeader(request_t *request)
{
	HttpHeader *hdr = &request->header;
	httpHeaderDelById(hdr, HDR_CONTENT_RANGE);
   	httpHeaderDelById(hdr, HDR_RANGE);
	httpHeaderDelById(hdr,HDR_IF_MODIFIED_SINCE);
	request->range = NULL;
}

/* 去掉url参数中的start和end，但保留其它的 */
static void drop_startend(char* start, char* end, char* url)
{
	char *s;
	char *e;

	//去掉start
	s = strstr(url, start);
	if(s == NULL)
                return;

	e = strstr(s, "&");
	if( e == 0 )
		*(s-1) = 0;
	else {
		memmove(s, e+1, strlen(e+1));
		*(s+strlen(e+1)) = 0;
	}

	//去掉end，如果有的话
	s = strstr(url, end);
	if( s ) {
		e = strstr(s, "&");
		if( e == 0 )
			*(s-1) = 0;
		else {
			memmove(s, e+1, strlen(e+1));
			*(s+strlen(e+1)) = 0;
		}
	}

}

/*
static int remove_url_start(clientHttpRequest *http, mod_config *cfg)
{
	debug(189, 3)("(mod_pplive) -> read request finished, remove url start: \n");


	//参数不完整，忽略该请求
	if( !http || !http->uri || !http->request ) {
		debug(189, 3)("mod_pplive: http = NULL, return \n");
		return 0;
	}

	char *tmp;

	//没有?，返回
	if( (tmp = strchr(http->uri, '?')) == NULL ) {
		debug(189, 3)("mod_pplive: <%s> not find ?, return \n", http->uri);
		return 0;
	}


	//没有找到start的变量名，返回
	if( (tmp = strstr(tmp, cfg->start)) == NULL ) {
		debug(189, 5) ("mod_pplive: not find start return %s %s %s \n",http->uri,tmp,cfg->start);
		return 0;
	}

	tmp--;
	if( *tmp != '?' && *tmp != '&' ) {
		debug(189, 5)("mod_pplive: <%s> has not start variable, return \n", http->uri);
		return 0;
	}
	tmp++;

	//找起始偏移量
*/
	/*int start_value;
	tmp += strlen(cfg->start);
	start_value = atol(tmp);
*/
/*
	int end_value = 0;
	if( (tmp = strstr(tmp, cfg->end)) ) {
		tmp--;
		if( *tmp == '?' || *tmp == '&' ) {
			tmp++;
			tmp += strlen(cfg->end);
			end_value = atoi(tmp);
		}
	}

	drop_startend(cfg->start, cfg->end, http->uri);
	if( http->uri[strlen(http->uri)-1] == '?')
		http->uri[strlen(http->uri)-1] = 0;


//	debug(189, 1)("http->urlpath.bur = %s\n", http->request->urlpath.buf);
	drop_startend(cfg->start, cfg->end, http->request->urlpath.buf);
	if( http->request->urlpath.buf[strlen(http->request->urlpath.buf)-1] == '?')
		http->request->urlpath.buf[strlen(http->request->urlpath.buf)-1] = 0;
	http->request->urlpath.len=strlen(http->request->urlpath.buf);


//	debug(189, 1)("http->request->canonical = %s\n", http->request->canonical);
	drop_startend(cfg->start, cfg->end, http->request->canonical);
	if( http->request->canonical[strlen(http->request->canonical)-1] == '?')
		http->request->canonical[strlen(http->request->canonical)-1] = 0;

	return 0;
}
*/

//safeguard chain: 1 remove ip 2 remove the part after "?"
static char *store_key_url(char* url){
                
        char *new_url = xstrdup(url);
        char *s;
        char *e;
        
        s = strstr(new_url, "://");
        s += 3;
        e = strstr(s, "/");
        if( e != NULL)
        {
             //memmove(s, e+1, strlen(e+1));
             //*(s+strlen(e+1)) = 0;
        }
                
        char *quesurl = strchr(new_url,'?');
        if(quesurl != NULL){
                *quesurl = 0;
        }

        debug(121, 3)("store_url=%s \n", new_url);
        //drop_startend("tm=","key=",new_url);

        return new_url;
}

static int clientHandleMp4(clientHttpRequest *http)
{
	char *ch_mp4 = NULL;
	char *ch_mp4_start = NULL;
	
	request_t *request = http->request;
	mod_config *cfg = cc_get_mod_param(http->conn->fd, mod);
	if(cfg == NULL){
		return 0;
	}

	safe_free(http->request->store_url);
	http->request->store_url = store_key_url(http->uri);

	ch_mp4 = strstr(request->urlpath.buf, cfg->type);

	
	if (ch_mp4) {
		ch_mp4_start = strstr(ch_mp4, cfg->start);

		if(ch_mp4_start){
			request_param *req_param = xmalloc(sizeof(request_param));
			memset(req_param,0,sizeof(request_param));

			req_param->mp4_flag = cfg->flag;
			req_param->succ = 0;
			req_param->uri = xstrdup(http->uri);
			
			req_param->mp4_start = atoi(ch_mp4_start + strlen(cfg->start));
			req_param->mp4_flag  = cfg->flag;

			char *ch_mp4_end = NULL;
			if((ch_mp4_end = strstr(ch_mp4_start, cfg->end))!=NULL) {
                                req_param->mp4_end = atoi(ch_mp4_end + strlen(cfg->end));
                        }

			wipeOutHeader(request);

			cc_register_mod_private_data(REQUEST_PRIVATE_DATA, http, req_param, free_request_param, mod);
			
			//remove_url_start(http,cfg);
		
			debug(189,3)("mod_pplive: request param start= %" PRINTF_OFF_T "\n",req_param->mp4_start);
			debug(189,3)("mod_pplive: request param end  = %" PRINTF_OFF_T "\n",req_param->mp4_end  );

			debug(189,3)("mod_pplive: request param flag=%d\n",req_param->mp4_flag);
			debug(189,3)("mod_pplive: request url=%s\n",http->uri);
		}
	}
	
	return 0;

}


static int safe_read(int fd,void *buffer,int length)
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

static int safe_write(int fd, void *buffer,int length)
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
				cc_usleep(100000);
				continue;
			} else {
				retry_times++;
				if (retry_times < 5) {
					cc_usleep(100000);
					continue;
				} else {
					debug(189,2)("mod_pplive: fwrite error(%d) for safe_write\n", errno);
					return -1;
				}
			}
		}
		bytes_left -= written_bytes;
		ptr += written_bytes;
	}
	return length;
}

static bool is_moov_alive(){
	FILE *fd = NULL;
	char pid[128];
	char buff[128];
	FILE  *procfd;
	char *pos = NULL;
	static int pidnum;

	debug(189,3) ("pidnum : %d\n",pidnum);

	if(pidnum == 0){
		if ((fd = fopen(MOOV_PID_FILE, "r")) != NULL) {
			fgets(pid, 128, fd);
			pidnum = atoi(pid);
			fclose(fd);
		}
	}

	if(pidnum != 0){
		snprintf(buff, 128, "/proc/%d/status", pidnum);
		if ((procfd = fopen(buff,"r")) != NULL) {
			memset(pid, 0, 128);
			fgets(pid, 128, procfd);

			debug(189,3)("%s -> %s\n",buff,pid);

			fclose(procfd);
			if ((pos = strstr(pid, "moov_generator_")) != NULL) {
				return true;
			}
		}
	}
	else{
		debug(189, 3) ("cannot get moov_generator_for_pplive pidnum!\n");
	}

	pidnum = 0;
	return false;
}


static void mp4Event(void *args)
{
	if(!is_moov_alive()){
		debug(189,3)("moov_generator_for_pplive dont run,now starting it!\n");
		enter_suid();

		int cid = fork();
		if (cid == 0)   //child
		{
			int ret = execl("/usr/local/squid/bin/moov_generator_for_pplive", "moov_generator_for_pplive","-D",(char *)0);
			if (ret < 0)
			{
				debug(189,3)("(mp4) --> execl error : %s\n",xstrerror());
			}

			exit(-1);
		}
		leave_suid();
	}

	eventAdd("mp4Event", mp4Event, NULL, 30, 0);	

	/*	do not needed!
	char buff[512] = {0};
	FILE *ftp = NULL;
	char *p = NULL;
	ftp = popen("netstat -anp | grep 5001 | grep LISTEN", "r");
	fgets(buff, 512, ftp);
	p = strstr(buff, "moov_gene");
	pclose(ftp);
	if(p)
		mp4_sock_flag = 2;
	else 
		mp4_sock_flag = 1;

	if(mp4_sock_flag == 1)
		eventAdd("mp4Event", mp4Event, NULL, (double) 10, 0);	
	*/
}

static void mp4ReadMoovWrite(int fd, const char *buf, int size, CWCB * handler, void *handler_data, FREE * free_func, mp4WriteStateData *mp4_data)
{
	assert(buf);
	CommWriteStateData *state = &fd_table[fd].rwstate;
	debug(189, 3) ("mp4ReadMoovWrite: FD %d: sz %d: hndl %p: data %p.\n",
			fd, size, handler, handler_data);
	
	
	fde_data* fd_data = cc_get_mod_private_data(FDE_PRIVATE_DATA, &fd, mod);
	if(fd_data == NULL){
		debug(189,5)("mod_pplive: when mp4 read moov write, fd param is null\n");
		free_func((void*)buf);
		return;
	}

	if (fd_data->mp4_moov_ok == 0)
		fd_data->mp4_moov_ok = 1;

	if (state->valid){
		debug(189, 1) ("mod_pplive: fd_table[%d].rwstate.valid == true!\n", fd);
		fd_table[fd].rwstate.valid = 0; 
	}
	//state = memPoolAlloc(mp4_comm_write_pool);
	//fd_table[fd].rwstate = *state;
	state->buf = (char *) buf; 
	state->size = size; 
	state->header_size = 0;
	state->offset = 0;
	state->handler = handler;
	state->handler_data = handler_data;
	state->free_func = free_func;
	state->valid = 1;
	cbdataLock(handler_data);
	//state->mp4_data = mp4_data;
	commSetSelect(fd, COMM_SELECT_WRITE, commHandleWrite, NULL, 0);
}


static void mp4ReadMoovStart(int mp4_sock, void *data)
{
	int Rsize = 0; 
	char Rbuf[4096];
	squid_off_t moov_size = 0;
	squid_off_t mdata_offset = 0;
	squid_off_t mdata_size = 0;
	mp4WriteStateData *mp4Data = data;
	int fd = mp4Data->fd;
	
	int valid = cbdataValid(mp4Data->handler_data);
        cbdataUnlock(mp4Data->handler_data);
	
	if(!valid){
		
		debug(189,3)("mod_pplive: mp4 header callback valid is %d \n",valid);
		goto free_data;
	}

	CWCB *handler = mp4Data->handler;
	void *handler_data = mp4Data->handler_data;
	clientHttpRequest *http = handler_data; 
	
	memset(Rbuf, 0 , 4096);
	Rsize = safe_read(mp4_sock, Rbuf, 24);
	
	debug(189,3)("mod_pplive: request mp4 data from moov_generor is %s \n",Rbuf);
/*
	fde_data* fd_data = cc_get_mod_private_data(FDE_PRIVATE_DATA, &fd, mod);
	if(fd_data == NULL){
		
		debug(189,3)("mod_pplive: when read moov header, fd param is null\n");
		goto free_data;
	}
*/
	if (Rsize > 0) {
		//读长度结构，改写content-length
		memcpy(&moov_size, Rbuf, 8);
		memcpy(&mdata_offset,Rbuf+8, 8);
		memcpy(&mdata_size, Rbuf+16, 8);
		
		if(!http->entry)
			goto free_data;
		if(!http->entry->mem_obj)
			goto free_data;

		request_param* req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);
		if( req_param == NULL){
			debug(189,3)("mod_pplive: when read moov header, request param is null\n");

			goto free_data;
		}

		req_param->mp4_rep_sz = moov_size - mdata_size;
		http->out.offset = mdata_offset - http->entry->mem_obj->swap_hdr_sz; 
	 	
		char *buf = mp4Data->buf;
		int size = mp4Data->size;
	
		debug(189,3)("mod_pplive: mp4 rep size  = %d http out offset = %" PRINTF_OFF_T " \n",req_param->mp4_rep_sz,http->out.offset);
	
	        char new_buf[4096];
		memset(new_buf,0 ,4096);

		char *con_len = NULL;
		char tmpbuf[4096] = {0};
		char tmp[128] = {0};
		con_len = strstr(buf, "Content-Length:");
		
		
		if (con_len) {
			strncpy(new_buf,buf,con_len-buf);

			char *clen = con_len;
			int len = 0;
			while(*clen != '\r') {
				len++;
				clen++;
			}

			strncpy(tmpbuf, con_len+len, 4095);
			snprintf(tmp, 128, "Content-Length: %" PRINTF_OFF_T, moov_size);
			strncat(new_buf,tmp,strlen(tmp));
			strncat(new_buf,tmpbuf,strlen(tmpbuf));

			size = size+strlen(tmp)-len;
		}
		else{
			debug(189,0)("mod_pplive: FATAL ... ");
			assert(0);
		}

		fde *F = &fd_table[fd];
		if (F->flags.open){

			char *mod_buf = xmalloc(size);
			memset(mod_buf,0,size);
			strncpy(mod_buf,new_buf,size);

	 		mp4ReadMoovWrite(fd, mod_buf, size, handler, handler_data, xfree, NULL); //FIXME  for leak out
	 		safe_free(mp4Data->buf);

			return;
		}
		else 
		{
			debug(189,3)("mod_pplive: close mp4_sock for client close connection\n");
			goto free_data;
		}
	}else if(Rsize == 0) {
		debug(189,3)("mod_pplive: recv header Rsize = 0 and close client connection\n");
		close_client_fd(fd);
		goto free_data;
	
	}else {
		debug(189,2)("mod_pplive: recv header Rsize < 0 and close client connection %d\n",Rsize);
		close_client_fd(fd);
		goto free_data;
	}
	
	free_data:
		close_mp4_fd(mp4_sock,fd);
		if ((mp4Data != NULL)&& mp4Data->free_func)
		{
			//FREE *free_func = mp4Data->free_func;
			debug(189,3)("mod_pplive: free mp4 buffer in recving header %s \n",mp4Data->buf);
			//void *free_buf = mp4Data->buf;
			mp4Data->free_func = NULL;
			safe_free(mp4Data->buf);
			safe_free(mp4Data);
			//cbdataFree(mp4Data);
		}
} 

static int mp4_connect(){
	int mp4_fd;
	struct hostent      *site;  
	struct sockaddr_in  myaddress;
	if( (site = gethostbyname("127.0.0.1")) == NULL )
		return -1;

	if( (mp4_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		return -1;
	}
	memset(&myaddress, 0, sizeof(struct sockaddr_in));
	myaddress.sin_family = AF_INET;
	myaddress.sin_port   = htons(5002);
	memcpy(&myaddress.sin_addr, site->h_addr_list[0], site->h_length);

	/*
	commSetNonBlocking(mp4_fd);
	commSetCloseOnExec(mp4_fd);
	*/

	int ret = -1;
	ret = connect(mp4_fd, (struct sockaddr *)&myaddress, sizeof(struct sockaddr)); 
	if (ret != 0) {
		if ( errno != EINPROGRESS )
		{
			//comm_close(mp4_fd);
        		 close(mp4_fd);
			return -1;
		}
		else{
			//connection INPROGRESS
			return -2;
		}
	}

	fd_open(mp4_fd, FD_SOCKET, "connect to moov_generator_for_pplive");

	return mp4_fd;
}

static int mp4_write_mbuf(int fd, MemBuf *mb, CWCB * handler, void *handler_data)
{
	//保存结构
	clientHttpRequest *http = handler_data;
	StoreEntry *entry = http->entry;

	if(!entry || entry->swap_dirn == -1){
		debug(189,1)("entry->swap_dirn is null and close client connection\n");

		//clientWriteComplete(fd, NULL, 0, COMM_OK, handler_data);

		close_client_fd(fd);
		goto free_mb;
		//return 1;
	}
	
	int mp4_fd = mp4_connect();
	if(mp4_fd <= -1){
		debug(189,3)("mod_pplive: mp4 moov_generor is not connected, please check moov_generor \n");
		//clientWriteComplete(fd, NULL, 0, COMM_OK, handler_data);
		return 0;
	}

	//register the fde private data of mp4 server fd
        fde_data* fd_data = cc_get_mod_private_data(FDE_PRIVATE_DATA, &fd, mod);
	if(fd_data == NULL){
		 fd_data = xmalloc(sizeof(fde_data));
		 memset(fd_data,0,sizeof(fde_data));	
		 cc_register_mod_private_data(FDE_PRIVATE_DATA, &fd, fd_data, free_fde_data, mod);
	}

	fd_data->mp4_fd = mp4_fd;
	fd_data->mp4_moov_ok = 0;

	debug(189,3)("mod_pplive: connected mp4 moov's fd param is %d \n",fd_data->mp4_fd);
	//get http private data
	
	request_param* req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);
	if( req_param == NULL){
		debug(189,5)("mod_pplive: when mp4_write mbuf ,request param is null\n");
		close_mp4_fd(mp4_fd,fd);
		return 0;
	}

	//发写命令
	int cwrite = 0;
	SwapDir *SD;
	mp4_req local_mp4_req;
	SD = &Config.cacheSwap.swapDirs[entry->swap_dirn];
	storeAufsDirFullPath(SD, entry->swap_filen, local_mp4_req.obj_path);
	local_mp4_req.mp4_offset = entry->mem_obj->swap_hdr_sz + req_param->mp4_rep_sz;
	strncpy(local_mp4_req.url, http->uri, 256);
	switch (req_param->mp4_flag) {
		case 1:
			local_mp4_req.start = req_param->mp4_start;
			local_mp4_req.end = req_param->mp4_end;;
			local_mp4_req.percent = 0;
			break;
		case 2:
			local_mp4_req.start = 0;
			local_mp4_req.end = 0;
			local_mp4_req.percent =req_param->mp4_start;
			break;
		case 3:
			local_mp4_req.start = 0;
			local_mp4_req.end = req_param->mp4_start;
			local_mp4_req.percent = 0;
			break;
	}
	cwrite = safe_write(mp4_fd, &local_mp4_req, sizeof(mp4_req));
	if(cwrite != sizeof(mp4_req)){
		
		debug(189,3)("mod_pplive: size which write to moov_generor is wrong,request all cache object content \n");
		
		close_mp4_fd(mp4_fd,fd);
		return 0; 
	}
	//注册读事件
	mp4WriteStateData *mp4Data = xmalloc(sizeof(mp4WriteStateData));
	mp4Data->fd = fd;

	mp4Data->buf = xmalloc(mb->size);
	strncpy(mp4Data->buf,mb->buf,mb->size);
//	mp4Data->buf = mb->buf;
	mp4Data->size = mb->size;
	mp4Data->handler = handler;
	mp4Data->handler_data = handler_data;
	//mp4Data->free_func = (FREE*) memBufFreeFunc;
	mp4Data->free_func = (FREE*) xfree;

	fd_data->mp4_data = mp4Data;
        cbdataLock(mp4Data->handler_data);

	commSetSelect(mp4_fd, COMM_SELECT_READ, mp4ReadMoovStart, mp4Data, 0);

	free_mb:
	{
		FREE* free_func = memBufFreeFunc(mb);
                void *free_buf  = mb->buf;
                free_func(free_buf);
		return 1;
	}
}

static void mp4ReadMoovBody(int mp4_sock, void *data)
{
	int Rsize = 0; 
	char *Rbuf = NULL;
	mp4WriteStateData *mp4Data = data;
	int fd = mp4Data->fd;
	CWCB *handler = mp4Data->handler;
	void *handler_data = mp4Data->handler_data;
	clientHttpRequest *http = handler_data;
       	
	int valid = cbdataValid(mp4Data->handler_data);
        cbdataUnlock(mp4Data->handler_data);
	
	if(!valid){
		debug(189,3)("mod_pplive: mp4 body callback valid is %d \n",valid);
		goto free_buf;
	}

	StoreEntry *entry = http->entry;
		
	Rbuf = (char *)xmalloc(4096);
	memset(Rbuf, 0 , 4096);
	Rsize = read(mp4_sock, Rbuf, 4095);
	
	request_param* req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);
	if( req_param == NULL){
		debug(189,3)("mod_pplive: when read mp4 moov body ,request param is null\n");
		close_mp4_fd(mp4_sock,fd);
		goto free_buf;
	}

	
	if( Rsize > 0) {
	
		req_param->mp4_rep_sz = req_param->mp4_rep_sz - Rsize; 
		mp4ReadMoovWrite(fd, Rbuf, Rsize, handler, handler_data, xfree, NULL);
	}
	else if (Rsize == 0) {
		debug(189,5)("closed mp4_sock %d, sending clientfd %d, http->mp4_rep_sz %d\n", mp4_sock, fd, req_param->mp4_rep_sz);
		close_mp4_fd(mp4_sock,fd);
	 	
		if(req_param->mp4_rep_sz) {
			close_client_fd(fd);
			goto free_buf;
		}

 
		storeClientCopy(http->sc, entry,
				http->out.offset,
				http->out.offset,
				STORE_CLIENT_BUF_SZ,
				memAllocate(MEM_STORE_CLIENT_BUF),
				clientSendMoreData,
				http); 
		goto free_buf;
	} 
	else {

		debug(189,5)("receive moov data exception and close the connection clientfd = %d \n", fd);
		close_mp4_fd(mp4_sock,fd);

		close_client_fd(fd);
		goto free_buf;
	}
	return;	

	free_buf:
		
		debug(189,5)("mod_pplive: free body buf \n");
		safe_free(Rbuf);
		if ((mp4Data != NULL)&& mp4Data->free_func)
		{
			debug(189,3)("mod_pplive: free mp4 buffer\n");
			mp4Data->free_func = NULL;
			//safe_free(mp4Data->buf);
			//cbdataFree(mp4Data);
			safe_free(mp4Data);
		}
}

static int hook_init()
{

	debug(189, 1)("(mod_pplive) ->	hook_init:----:)\n");
	eventAdd("mp4Event", mp4Event, NULL, 60, 0);	
	return 0;
}


static int hook_cleanup(cc_module *module)
{
	debug(189, 1)("(mod_pplive) ->	hook_cleanup:\n");
	eventDelete(mp4Event, NULL);
	return 0;
}

static int func_sys_parse_param(char *cfg_line)
{
	debug(189,3)("mod_pplive config=[%s]\n", cfg_line);
	if( strstr(cfg_line, "allow") == NULL )
		return 0;

	if( strstr(cfg_line, "mod_pplive") == NULL )
		return 0;
	
	int miss_strategy = 0;

        is_start_helper = 0;
        if( strstr(cfg_line, "miss_wait") != NULL )
        {
                miss_strategy = 1;
                is_start_helper = 1;
        }
        else if(strstr(cfg_line, "miss_start") != NULL )
        {
                miss_strategy = 2;
                is_start_helper = 1;
        }
        else if(strstr(cfg_line, "query_start") != NULL )
        {
                miss_strategy = 3;
                is_start_helper = 0;
        }
        else if(strstr(cfg_line, "query_whole") != NULL )
        {
                miss_strategy = 0;
                is_start_helper = 0;
	}

	char *token = NULL;

	if ((token = strtok(cfg_line, w_space)) == NULL)
		return 0;


	mod_config *cfg = xmalloc(sizeof( mod_config));
	memset(cfg, 0, sizeof( mod_config));
	

	token = strtok(NULL, w_space);
	strncpy(cfg->start, token, 126);
	strcat(cfg->start, "=");

	token = strtok(NULL, w_space);
	strncpy(cfg->end, token, 126);
	strcat(cfg->end, "=");

	
	token = strtok(NULL, w_space);
	int isflag = 1;
	//check if the flag type
	if((token != NULL) && ((strcasecmp(token,"allow")!=0) && ( strcasecmp(token,"deny")!=0 ))){
		cfg->flag = atoi(token);
		isflag = 0;
	}
	//default support standard mp4 start seconds
	if(isflag){
		cfg->flag = 1;
	}
	
	strncpy(cfg->type, "mp4",127);

	token = strtok(NULL, w_space);
        if((token != NULL) && ((strcasecmp(token,"allow")!=0)
                        && ( strcasecmp(token,"deny")!=0 )
                        && ( strcasecmp(token,"miss_wait")!=0 )
                        && ( strcasecmp(token,"miss_start")!=0 )
                        && ( strcasecmp(token,"query_start")!=0 )
                        && ( strcasecmp(token,"query_total")!=0 )
                        && ( strcasecmp(token,"youku")!=0 )
                        && ( strcasecmp(token,"tudou")!=0 )
                        && ( strcasecmp(token,"ku6")!=0 )
                        && ( strcasecmp(token,"qqvideo")!=0 )
                        && ( strcasecmp(token,"sohu")!=0 )
                        && ( strcasecmp(token,"sina")!=0 )
                        && ( strcasecmp(token,"standard")!=0))){
                helper_proc_num = atoi(token);
        }

/*
	cfg->is_miss_wait = is_miss_wait;
    	debug(189, 3) ("mod_pplive: paramter start= %s end= %s type= %s flag= %d miss_wait=%d \n", cfg->start, cfg->end, cfg->type,cfg->flag, cfg->is_miss_wait);
	cc_register_mod_param(mod, cfg, free_cfg);
*/

	cfg->miss_strategy = miss_strategy;
        debug(189, 3) ("mod_pplive: paramter start= %s end= %s type= %s flag= %d miss_strategy=%d \n", cfg->start, cfg->end, cfg->type,cfg->flag, cfg->miss_strategy);
        cc_register_mod_param(mod, cfg, free_cfg);

	return 0;
}

static int mod_close_mp4_server(int fd){
	
	fde_data* fd_data = cc_get_mod_private_data(FDE_PRIVATE_DATA, &fd, mod);
	if(fd_data == NULL){
		//debug(189,5)("mod_pplive: fd param is null\n");
		return 0;
	}

	fde *F = &fd_table[fd];
        if(F->cctype == 1 ){
		debug(189,2)("mod_pplive: this fd is server fd and not free the mp4Data %d \n",fd);
        	return 0;
        }

	if(fd_data->mp4_moov_ok && fd_data->mp4_fd) {
		debug(189,3)("mod_pplive : close mp4_fd in comm.c for client closed\n");
		
		mp4WriteStateData *mp4Data = fd_data->mp4_data;
		if(mp4Data !=NULL && mp4Data->handler_data !=NULL){

			debug(189,3)("mod_pplive: mp4 close mp4 callback fd is %d \n",fd);

                	safe_free(mp4Data);
			
			fd_close(fd_data->mp4_fd);
			close(fd_data->mp4_fd);
	
			fd_data->mp4_fd = 0;
       			fd_data->mp4_moov_ok = 2;
		}
        
	}
	return 0;
	
}

static int mod_pplive_read_moov_body(int fd, CommWriteStateData *state)
{
	fde *F = &fd_table[fd];
	fde_data* fd_data = cc_get_mod_private_data(FDE_PRIVATE_DATA, &fd, mod);
	if(fd_data == NULL){
		//debug(189,5)("mod_pplive: fd param is null\n");
		return 0;
	}
	if (fd_data->mp4_moov_ok == 1) {

		mp4WriteStateData *mp4Data = fd_data->mp4_data;
		//if(F->flags.open && mp4Data !=NULL && cbdataValid(mp4Data->handler_data)){
		if(F->flags.open && mp4Data !=NULL && mp4Data->handler_data !=NULL){
		        cbdataLock(mp4Data->handler_data);
			commSetSelect(fd_data->mp4_fd, COMM_SELECT_READ, mp4ReadMoovBody, fd_data->mp4_data, 0);
		}
	}
	return 0;

}

static int mod_pplive_set_header_size(clientHttpRequest * http)
{
	request_param* req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);
	if( req_param == NULL){
		//debug(189,5)("mod_pplive: when set header size ,request param is null\n");
		return 0;
	}
	
	if(req_param->mp4_flag == 0){
                return 0;
        }

	req_param->mp4_rep_sz = http->reply->hdr_sz; //add by sxlxb for mp4
	return 0;
}

static int mod_pplive_client_send_more_data(clientHttpRequest * http, MemBuf *mb,char *buf, ssize_t size){
	
		
	ConnStateData *conn = http->conn;
	int fd = conn->fd;
	//fix one bug for http 404 NOT FOUND buf ,only 200 ok http reply we can support mp4 drag and drop
	if(http->reply->sline.status != HTTP_OK ){
		return 0;
	}
	request_param* req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);
	if( req_param == NULL){
		//debug(189,5)("mod_pplive: when send more data ,request param is null\n");
		return 0;
	}
//	if(!req_param->mp4_flag || (req_param->mp4_flag && (mp4_sock_flag != 2))) {
	if(!req_param->mp4_flag) {
		
		/*
		if(req_param->mp4_flag)
			eventAdd("mp4Event", mp4Event, NULL, (double) 10, 0);
		*/
		return 0;
	}
	else {
		int ret = mp4_write_mbuf(fd, mb, clientWriteComplete, http);
		return ret;
	}

	return 0;
}

static int mod_pplive_client_write_complete(int fd, char *bufnotused, size_t size, int errflag, void *data){
	
	fde_data* fd_data = cc_get_mod_private_data(FDE_PRIVATE_DATA, &fd, mod);
	if(fd_data == NULL){
		//debug(189,5)("mod_pplive: when write complete, fd param is null\n");
		return 0;
	}

	switch (fd_data->mp4_moov_ok) {
		case 0:
			break;
		case 1:
			return 1;
		case 2:
		//	comm_close(fd);
			break;
	}

	return 0;
}
/*
static void modify_request(clientHttpRequest * http,char *new_uri){

	debug(189, 5)("mod_pplive: start, uri=[%s]\n", http->uri);
	request_t* old_request = http->request;
	
	request_t* new_request = urlParse(old_request->method, new_uri);

	debug(189, 5)("mod_pplive: start, new_uri=[%s]\n", new_uri);
	if (new_request)
	{
		safe_free(http->uri);
		http->uri = xstrdup(urlCanonical(new_request));
		safe_free(http->log_uri);
		http->log_uri = xstrdup(urlCanonicalClean(old_request));
		new_request->http_ver = old_request->http_ver;
		httpHeaderAppend(&new_request->header, &old_request->header);
		new_request->client_addr = old_request->client_addr;
		new_request->client_port = old_request->client_port;
#if FOLLOW_X_FORWARDED_FOR
		new_request->indirect_client_addr = old_request->indirect_client_addr;
#endif // FOLLOW_X_FORWARDED_FOR 
		new_request->my_addr = old_request->my_addr;
		new_request->my_port = old_request->my_port;
		new_request->flags = old_request->flags;
		new_request->flags.redirected = 1;
		if (old_request->auth_user_request)
		{
			new_request->auth_user_request = old_request->auth_user_request;
			authenticateAuthUserRequestLock(new_request->auth_user_request);
		}
		if (old_request->body_reader)
		{
			new_request->body_reader = old_request->body_reader;
			new_request->body_reader_data = old_request->body_reader_data;
			old_request->body_reader = NULL;
			old_request->body_reader_data = NULL;
		}
		new_request->content_length = old_request->content_length;
		if (strBuf(old_request->extacl_log))
			new_request->extacl_log = stringDup(&old_request->extacl_log);
		if (old_request->extacl_user)
			new_request->extacl_user = xstrdup(old_request->extacl_user);
		if (old_request->extacl_passwd)
			new_request->extacl_passwd = xstrdup(old_request->extacl_passwd);
		requestUnlink(old_request);
		http->request = requestLink(new_request);
	}

}
*/

static void mediaPrefetchHandleReply(void *data, char *reply)
{
	media_prefetch_state_data* state = data;
		
	request_param* req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, state->http, mod);
	if( req_param == NULL){
		//debug(189,5)("mod_pplive: request param is null\n");
		return ;
	}

/*	
	if(strcmp(reply, "OK"))
	{
		debug(189,1)("mod_pplive can not prefetch media!\n");
		request_t *r = state->http->request;
		ErrorState *err = errorCon(ERR_FTP_NOT_FOUND, HTTP_NOT_FOUND, r);
		err->url = xstrdup(state->http->uri);
		state->http->entry = clientCreateStoreEntry(state->http, r->method, null_request_flags);
		errorAppendEntry(state->http->entry, err);
		return;
	}

*/
	if(strcmp(reply, "OK"))
	{
		debug(189,1)("mod_pplive can not prefetch media! reply=[%s] \n",reply);
	        req_param->succ = 1;	
	}	
	debug(189,3)("mod_pplive prefetched media, and it should be a hit!\n");

	int valid = cbdataValid(state->http);
	cbdataUnlock(state->http);
	cbdataUnlock(state);
	if(valid)
	{
		state->callback(state->http);
	}
	cbdataFree(state);
}

static void mediaPrefetchHandleReplyForMissStart(void *data, char *reply)
{
        media_prefetch_state_data* state = data;
                        
        if(strcmp(reply, "OK"))
        {               
                debug(189,1)("mod_pplive can not prefetch media! reply=[%s] \n",reply);
        }               
        else{           
                debug(189,1)("mod_pplive quering whole object is success reply=[%s] \n",reply);
        }       

        cbdataFree(state);
}

static int mod_pplive_http_req_process(clientHttpRequest *http){
	
	fde *F = NULL;
        int fd = -1;
        static char tmp_buf[32];
        int local_port = -1;
	
	request_param* req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);
	if( req_param == NULL){
		//debug(189,5)("mod_pplive: request param is null\n");
		return 0;
	}

	mod_config *cfg = cc_get_mod_param(http->conn->fd, mod);
	assert(cfg);
	
	
	if (http->log_type == LOG_TCP_MISS && req_param->succ == 1){
		debug(189,3)("mod_pplive: curl reqeust is fail and fetch all whole mp4 file \n");
		req_param->mp4_flag = 0;
		
	}
	else if (http->log_type == LOG_TCP_MISS && req_param->mp4_flag != 0 && cfg->miss_strategy == 0){
		debug(189,3)("mod_pplive: reques mp4 url  and miss to web server \n");
		req_param->mp4_flag = 0;
		/*char new_uri[1024];
		if(strstr(http->uri,"?") == NULL){
			
			snprintf(new_uri,1024,"%s%s%s%s%lld",http->uri,"?",cfg->start,"=",req_param->mp4_start);
		}
		else{
			snprintf(new_uri,1024,"%s%s%s%lld",http->uri,cfg->start,"=",req_param->mp4_start);
		}

		modify_request(http,new_uri);*/
	}
	else if( http->log_type == LOG_TCP_MISS && !httpHeaderCheckExistByName(&http->request->header, "X-CC-Media-Prefetch") && cfg->miss_strategy == 1)
	{
		debug(189,3)("sending data to helper because http->log_type=%d, cfg->miss_strategy=%d\n ", http->log_type, cfg->miss_strategy);
		debug(189,3)("client address : %s,url : %s\n", inet_ntoa(http->request->client_addr),http->uri);
		CBDATA_INIT_TYPE(media_prefetch_state_data);
		media_prefetch_state_data *state = cbdataAlloc(media_prefetch_state_data);
		state->http = http;
		state->callback = clientProcessRequest;

		cbdataLock(state);
		cbdataLock(state->http);

		char *new_uri = xstrdup(http->uri);
                drop_startend(cfg->start, cfg->end, new_uri);

		static char buf[4096];
		snprintf(buf, 4096, "%s", new_uri);
		safe_free(new_uri);

		fd = http->conn->fd;
		F = &fd_table[fd];

		if(!F->flags.open)
		{
			debug(189, 2)("(mod_pplive) ->: FD is not open!\n");
			local_port = 80;
		}
		else
		{
			local_port = F->local_port;
		}

		debug(189, 3) ("(mod_pplive) ->: http->uri: %s\n", buf);
		debug(189, 3) ("(mod_pplive) ->: http->port: %d\n", local_port);

		memset(tmp_buf, 0, sizeof(tmp_buf));

		snprintf(tmp_buf, 32, "@@@%d", local_port);
		strcat(buf, tmp_buf);
		strcat(buf, "\n");

		debug(189, 3) ("(mod_pplive) ->: helper buf: %s\n", buf);

		helperSubmit(media_prefetchers, buf, mediaPrefetchHandleReply, state);
		return 1;
	}
	else if( http->log_type == LOG_TCP_MISS && cfg->miss_strategy == 2 && !httpHeaderCheckExistByName(&http->request->header, "X-CC-Media-Prefetch"))
	{
		debug(189,3)("mod_pplive: request miss_start object because http->log_type=%d, cfg->miss_strategy=%d\n ", http->log_type, cfg->miss_strategy);
                StoreEntry *e = NULL;

		if(http->request->store_url)
                {
			debug(189,3)("mod_pplive: request store_url is: %s\n", http->request->store_url);
                        e = storeGetPublic(http->request->store_url,METHOD_GET);
                }
                else
                {
                        debug(189,0)("Warnning mod_pplive: http->uri: [%s] in pplive mod, but has no store_url!!!\n", http->uri);
                        return 0;
                }

                if(e == NULL){//query whole object
                        CBDATA_INIT_TYPE(media_prefetch_state_data);
                        media_prefetch_state_data *state = cbdataAlloc(media_prefetch_state_data);
                        state->http = NULL;
                        state->callback = NULL;

                        char *new_uri = xstrdup(http->uri);
                        drop_startend(cfg->start, cfg->end, new_uri);

                        debug(189, 2) ("mod_pplive: mp4 miss and redirect to upper layer to query whole data %s \n",new_uri);
                        static char buf[4096];
                        snprintf(buf, 4096, "%s", new_uri);
                        safe_free(new_uri);

			fd = http->conn->fd;
                        F = &fd_table[fd];

                        if(!F->flags.open)
                        {
                                debug(189, 2)("(mod_pplive) ->: FD is not open!\n");
                                local_port = 80;
                        }
                        else
                        {
                                local_port = F->local_port;
                        }

                        debug(189, 3) ("(mod_pplive) ->: http->uri: %s\n", buf);
                        debug(189, 3) ("(mod_pplive) ->: http->port: %d\n", local_port);

                        memset(tmp_buf, 0, sizeof(tmp_buf));

                        snprintf(tmp_buf, 32, "@@@%d", local_port);
                        strcat(buf, tmp_buf);
                        strcat(buf, "\n");

                        debug(189, 3) ("(mod_pplive) ->: helper buf: %s\n", buf);

                        helperSubmit(media_prefetchers, buf, mediaPrefetchHandleReplyForMissStart, state);
                }

		if(e == NULL|| e->store_status != STORE_OK || e->swap_status != SWAPOUT_DONE ){

                        debug(189, 2) ("mod_pplive: mp4 pending and redirect to upper layer to query start data %s \n",http->uri);
                        req_param->mp4_flag = 0; //add for internal miss query whole implement
                        http->request->flags.cachable = 0;//no cache
                        safe_free(http->request->store_url);
                        //modify_request(http,req_param->uri);
                        http->entry = NULL;
                        http->log_type = LOG_TCP_MISS;

                }
                else{
                        debug(189, 3) ("mp4 hit request http->uri = %s\n", http->uri);
                }

               return 0;
        }

	return 0;
}

static void hook_before_sys_init()
{
	if(is_start_helper == 0){

		debug(189,3)("mod_pplive: module cfg is_start_helper=%d and don't start helper program\n",is_start_helper);

		if (media_prefetchers)
                {
                        helperShutdown(media_prefetchers);
                        helperFree(media_prefetchers);
                        media_prefetchers = NULL;
                }

		return;	
	}

	debug(189,3)("mod_pplive: module cfg is_start_helper=%d and start helper program\n",is_start_helper);
	debug(189,3)("mod_pplive: helper process number  =%d \n",helper_proc_num);

	if (media_prefetchers)
	{       
		helperShutdown(media_prefetchers);
		helperFree(media_prefetchers);
		media_prefetchers = NULL; 
	}       

	media_prefetchers = helperCreate("pplive_mp4_media_prefetcher");
	wordlist *pwl = NULL; 
	char cfg_line2[128];
	strcpy(cfg_line2,"a /usr/local/squid/bin/pplive_media_prefetch.pl");
	strtok(cfg_line2, w_space);
	parse_programline(&pwl);

	media_prefetchers->cmdline = pwl;
	media_prefetchers->n_to_start = helper_proc_num;
	media_prefetchers->concurrency = 0;
	media_prefetchers->ipc_type = IPC_STREAM;
	helperOpenServers(media_prefetchers);

}

/*
static void cleanup_registered_deferfunc_when_reconfig()
{                       
        if (media_prefetchers)
        {
                helperShutdown(media_prefetchers);
                helperFree(media_prefetchers);
                media_prefetchers = NULL;
        }               
}
*/

int mod_register(cc_module *module)
{
	debug(189, 1)("(mod_pplive) ->	mod_register:\n");

	strcpy(module->version, "7.0.R.16488.i686");

        //module->hook_func_sys_init = hook_init;
        cc_register_hook_handler(HPIDX_hook_func_sys_init,
                        module->slot, 
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_init),
                        hook_init);

        //module->hook_func_sys_parse_param = func_sys_parse_param;
        cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
                        module->slot, 
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
                        func_sys_parse_param);

        //module->hook_func_sys_cleanup = hook_cleanup;
        cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
                        module->slot, 
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_cleanup),
                        hook_cleanup);

        //module->hook_func_fd_close = mod_close_mp4_server;
        cc_register_hook_handler(HPIDX_hook_func_fd_close,
                        module->slot, 
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_fd_close),
                        mod_close_mp4_server);

        //module->hook_func_private_mp4_read_moov_body = mod_pplive_read_moov_body;
        cc_register_hook_handler(HPIDX_hook_func_private_mp4_read_moov_body,
                        module->slot, 
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_mp4_read_moov_body),
                        mod_pplive_read_moov_body);

        //module->hook_func_http_repl_send_start = mod_pplive_set_header_size;
        cc_register_hook_handler(HPIDX_hook_func_http_repl_send_start,
                        module->slot, 
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_repl_send_start),
                        mod_pplive_set_header_size);

        //module->hook_func_private_mp4_send_more_data = mod_pplive_client_send_more_data;
        cc_register_hook_handler(HPIDX_hook_func_private_mp4_send_more_data,
                        module->slot, 
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_mp4_send_more_data),
                        mod_pplive_client_send_more_data);

        //module->hook_func_private_client_write_complete       = mod_pplive_client_write_complete;
        cc_register_hook_handler(HPIDX_hook_func_private_client_write_complete,
                        module->slot, 
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_client_write_complete),
                        mod_pplive_client_write_complete);

        //module->hook_func_http_req_process = mod_pplive_http_req_process;
        cc_register_hook_handler(HPIDX_hook_func_http_req_process,
                        module->slot, 
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_process),
                        mod_pplive_http_req_process);

        //module->hook_func_http_before_redirect = clientHandleMp4;
        cc_register_hook_handler(HPIDX_hook_func_http_before_redirect,
                        module->slot, 
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_before_redirect),
                        clientHandleMp4);
	
	//module->hook_func_before_sys_init = hook_before_sys_init;

	cc_register_hook_handler(HPIDX_hook_func_before_sys_init,
                        module->slot, 
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_before_sys_init),
                        hook_before_sys_init);
	
/*
	cc_register_hook_handler(HPIDX_hook_func_module_reconfigure,
                        module->slot,
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_module_reconfigure),
                        cleanup_registered_deferfunc_when_reconfig);
*/

	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;

	return 0;
}
#endif
