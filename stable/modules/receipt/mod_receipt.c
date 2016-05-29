#include "cc_framework_api.h"
#include "squid.h"

#ifdef CC_FRAMEWORK

//这是本模块在主程序的索引信息的结构指针
static cc_module* mod = NULL;

typedef struct _mod_receipt_param{
	char *type;
	char *source;	
	char *dtype;

}mod_receipt_param;
typedef struct _microsoft_sendmsg {
		char *domain;
		char *source;
		char *utc;
		unsigned int btime;
		unsigned int ftime;
		String uri;
		char *devent;
		char *dsize;
		int win32;
		int http;
		int firstbyte;
		int lastbyte;
		char *id;
		String msid; 
		char *cip;
		char *scip;
		char *dtime;
		char *ruri;
		char *ua;
		char *dtype;
		int  isaccept;
		int  rangefirst;
		int  rangelast;
		long long int sendbytes;
		int msflag;
		int	aliveflag;
		int rtype;
} micromsg;   //add by sxlxb for microsoft


enum receiptype {
	COMMON,
	MICROSOFT
};

time_t oldtime;
int microfd; /* -1 */
char filenamebuf[512];
char filenamenew[512];
static int do_microsoft = 0;
static MemPool * micromsg_pool = NULL;
static MemPool * mod_config_pool = NULL;
int fdemsg_uri_rlen;

static void * micromsg_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == micromsg_pool)
	{
		micromsg_pool = memPoolCreate("mod_receipt private_data micromsg", sizeof(micromsg));
	}
	return obj = memPoolAlloc(micromsg_pool);
}

static void * mod_config_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == mod_config_pool)
	{
		mod_config_pool = memPoolCreate("mod_receipt config_struct", sizeof(mod_receipt_param));
	}
	return obj = memPoolAlloc(mod_config_pool);
}

int free_fde_private_data(void* data){
	
	micromsg *fdemsg = (micromsg*)data;
	
	if(fdemsg != NULL){
		safe_free(fdemsg->source);
		safe_free(fdemsg->utc);
		stringClean(&fdemsg->uri);
		safe_free(fdemsg->devent);
		safe_free(fdemsg->dsize);
		safe_free(fdemsg->id);
		safe_free(fdemsg->cip);
		safe_free(fdemsg->dtime);
		safe_free(fdemsg->ruri);
		safe_free(fdemsg->ua);
		safe_free(fdemsg->dtype);
		stringClean(&fdemsg->msid);
		safe_free(fdemsg->scip);
		safe_free(fdemsg->domain);
		memPoolFree(micromsg_pool, fdemsg);
		fdemsg = NULL;
	}

	return 0;
	
}

//default 0 is common ,1 is microsoft	

enum receiptype checkReceiptType(){
	
	mod_receipt_param* receiptparam = (mod_receipt_param*)cc_get_global_mod_param(mod);
	
	if(receiptparam == NULL){

    	debug(114, 3) ("mod_receipt: receipt param is NULL" );
		return COMMON;
	}
	

	if(!strcasecmp(receiptparam->type,"common")){
		return COMMON;
	}
	else if(!strcasecmp(receiptparam->type,"microsoft")){
		return MICROSOFT;
	}	

	return COMMON;
	
}


//add by sxlxb for microsoft
void timetransfer(time_t timesec, char *timebuf, int rtype)
{
	
        struct tm *firsttime;
		if(rtype == 1){
        	firsttime = gmtime(&timesec);
		}
		else{
			firsttime = localtime(&timesec);
		}
        snprintf(timebuf, 3,"%02d",firsttime->tm_year-100);
        snprintf(timebuf+2,3,"%02d",firsttime->tm_mon+1);
        snprintf(timebuf+4,3, "%02d",firsttime->tm_mday);
        snprintf(timebuf+6,3, "%02d",firsttime->tm_hour);
        snprintf(timebuf+8,3,"%02d",firsttime->tm_min);
        snprintf(timebuf+10,3,"%02d",firsttime->tm_sec);
        *(timebuf+12) = 'Z';
        *(timebuf+13) = '\0';
}

static HttpHeaderEntry *httpHeaderFindEntry2(const HttpHeader * hdr, http_hdr_type id)
{
    HttpHeaderPos pos = HttpHeaderInitPos;
    HttpHeaderEntry *e;
    assert(hdr);
    //assert_eid(id);

    /* check mask first */
    if (!CBIT_TEST(hdr->mask, id))
		return NULL;
    /* looks like we must have it, do linear search */
    while ((e = httpHeaderGetEntry(hdr, &pos))) {
		if (e->id == id)
		    return e;
    }
   /* hm.. we thought it was there, but it was not found */
   assert(0);
   return NULL;
   /* not reached */

}

static void clientObtainRequestMessage(clientHttpRequest *http , request_t *request)
{
	const int count = request->link_count;
	char timebuf[16];
	HttpHeaderEntry *findname = NULL;
	int fd;
	int len;
	http_hdr_type id;
	char clientaddr[128];
	char serveraddr[128];
	char buff[512];
	fde *F = NULL;
	fd = http->conn->fd; 
	F = &fd_table[fd];

	mod_receipt_param* receiptparam = (mod_receipt_param*) cc_get_mod_param(fd, mod);
	
	micromsg *fdemsg = NULL;
	fdemsg = cc_get_mod_private_data(FDE_PRIVATE_DATA, &fd, mod);
	
	if(F->cctype == 1 ){
	    //debug(114,5) ("mod_receipt: close %d fd  cctype = 1\n", fd);
        return ;
 	}

	if(receiptparam == NULL){

    	debug(114, 3) ("mod_receipt: receiptparam is NULL" );
		return;
	}
	
	if(fdemsg == NULL){

    	debug(114, 3) ("mod_receipt: fdemsg is NULL" );
		return;
	}
	

	if (fdemsg->isaccept == 1) {
		fdemsg->msflag = 1;
		fdemsg->source = xstrdup(receiptparam->source);
		fdemsg->domain = xstrdup(request->host);
		char* orig_uri = urlCanonicalClean(http->orig_request);
		if(fdemsg->rtype == 1 ){ //microsoft type
			char * uripathpos = strstr(orig_uri+7, "/");
			char * uriportpos = strstr(orig_uri+7, ":");
			if(uriportpos>uripathpos || uriportpos==NULL)
			{
				stringInit(&fdemsg->uri, orig_uri);
				debug(114,3)("mod_receipt: the fdemsg->uri is: %s\n",strBuf(fdemsg->uri));
			}
			else
			{
				fdemsg->uri = StringNull;	
				stringAppend(&fdemsg->uri, orig_uri, uriportpos - orig_uri);
				stringAppend(&fdemsg->uri,uripathpos,strlen(uripathpos));
			}
		}
		else{//common
			char * uripos = strstr(orig_uri+7, "/");
			if (uripos != NULL)
			{
				stringInit(&fdemsg->uri, uripos);	
				debug(114,3)("mod_receipt common: the fdemsg->uri is: %s\n",strBuf(fdemsg->uri));
			}
		}	

		fdemsg->id = xstrdup("UNK");
		if (strncmp(receiptparam->dtype, "public", 6) == 0) 
			fdemsg->dtype = xstrdup("1");
		else
			fdemsg->dtype = xstrdup("0");
		inet_ntop(AF_INET, &request->client_addr, clientaddr, sizeof(clientaddr));
		inet_ntop(AF_INET, &request->my_addr, serveraddr, sizeof(serveraddr));
		snprintf(buff, 512, "%s", clientaddr);
		
		String xff = httpHeaderGetList(&http->request->header, HDR_X_FORWARDED_FOR);
		if(strLen(xff) > 0)
		{
			fdemsg->cip = xstrdup(xff.buf);
		}
		else
		{
			fdemsg->cip = xstrdup(buff);
		}
		stringClean(&xff);
		memset(buff, 0, 512);
		snprintf(buff, 512, "%sAND%s", clientaddr, serveraddr); 
		fdemsg->scip = xstrdup(buff);

		len = strlen("Cookie");
		id = httpHeaderIdByNameDef("Cookie", len);
		findname = httpHeaderFindEntry(&request->header, id);
		if (findname != NULL) {
			char *strpos;
			strpos = strstr(findname->value.buf, "MC1=GUID=");
			if (strpos != NULL) {
				stringLimitInit(&fdemsg->msid, strpos+9, 32);
			}
			else 
				fdemsg->msid = StringNull;
		} 
		else 
			fdemsg->msid = StringNull;

		len = strlen("Referer");
		id = httpHeaderIdByNameDef("Referer", len);
		findname = NULL;
		findname = httpHeaderFindEntry2(&request->header, id);
		if (findname != NULL)
			fdemsg->ruri = xstrdup(findname->value.buf);
		else 
			fdemsg->ruri = NULL;

		len = strlen("User-Agent");
		id = httpHeaderIdByNameDef("User-Agent", len);
		findname = NULL;
		findname = httpHeaderFindEntry2(&request->header, id);
		if (findname != NULL){
			fdemsg->ua = xstrdup(findname->value.buf);
		 }
		else 
			fdemsg->ua = NULL;


		fdemsg->btime = time(NULL);
		timetransfer(fdemsg->btime, timebuf, fdemsg->rtype);
		fdemsg->utc = xstrdup(timebuf);
		
	} //if end
	//have problem for loop  and what meaning
	request->link_count = 100;
    request->link_count = count;
	
    debug(114, 5) ("mod_receipt: fdemsg->source= %s dtype= %s msid=%s \n", fdemsg->source, fdemsg->dtype, fdemsg->ruri );
}

static void clientObtainReplyMessage(clientHttpRequest *http, HttpReply *rep)
{
	int fd = http->conn->fd;
	int len;
	http_hdr_type id;
	char *isrange = NULL;
	int rangebegin;
	int rangeend;
	int total;

	fde *F = &fd_table[fd];	
	if(F->cctype == 1 ){
	    //debug(114,5) ("mod_receipt: close %d fd  cctype = 1\n", fd);
        return;
 	}


	micromsg *fdemsg = NULL;
	fdemsg = cc_get_mod_private_data(FDE_PRIVATE_DATA, &fd, mod);
	
	if(fdemsg == NULL){
		return;
	}

	HttpHeaderEntry *findname = NULL;
	{
		if (fdemsg->isaccept == 1) 
		{
			fdemsg->http = rep->sline.status;
			len = strlen("Content-Length");
			id = httpHeaderIdByNameDef("Content-Length", len);
			findname = NULL;
			findname = httpHeaderFindEntry2(&rep->header, id);
			if (findname != NULL) 
			{
				fdemsg->dsize = xstrdup(findname->value.buf);
				len = strlen("Content-Range");
				id = httpHeaderIdByNameDef("Content-Range", len);
				findname = NULL;
				findname = httpHeaderFindEntry2(&rep->header, id);
				if (findname != NULL) 
				{
					isrange = strstr(findname->value.buf, "-");
					if (isrange != NULL)
					{
						char *p = isrange;
						fdemsg->http = 206;
						while(*(--p) != ' ');
						rangebegin = atoi(p);
						if (rangebegin == 0)
							fdemsg->rangefirst = 1;
						p = isrange + 1;
						rangeend = atoi(p);
						p = strstr(findname->value.buf, "/") +1;
						total = atoi(p);
						if (rangeend + 1 == total)
							fdemsg->rangelast = 1;

					}
				}
			}
			else	
				fdemsg->http = 404;
		}
	}

    debug(114, 5) ("mod_receipt: fdemsg->rangelast= %d http= %d rangefirst=%d \n", fdemsg->rangelast, fdemsg->http, fdemsg->rangefirst );
	
}
static void initMicrosoftlog(void)
{
	DIR *fdp = NULL;
	struct dirent  *dirp;
	char unlinkbuf[128];
	char test[128];
	char *pos = NULL;
	char *spos = NULL;
	microfd = -1;
	int testfd = -1;
	char testbuf[4096];
	int readln;
	int i = 0;
	if ((fdp = opendir("/data/proclog/log/squid/flexicache")) == NULL) {
		system("mkdir -p /data/proclog/log/squid/flexicache");
		system("chown squid:squid /data/proclog/log/squid/flexicache");
		system("chmod 755 /data/proclog/log/squid/flexicache");
		while (((fdp = opendir("/data/proclog/log/squid/flexicache")) == NULL) && (i++ < 5));
	}

	while ((dirp = readdir(fdp)) != NULL) {
        if (strcmp(dirp->d_name, ".") == 0 ||
            strcmp(dirp->d_name, "..") == 0)
                continue;
		int isMicrosoft = 0;		
		if((pos = strstr(dirp->d_name, "down_ms")) != NULL){
			//pos = strstr(dirp->d_name, "down_ms");
			isMicrosoft = 1;
		}
		else{
			pos = strstr(dirp->d_name, "down_receipt");
			isMicrosoft = 0;
		}
#ifdef CC_MULTI_CORE
		if (pos != NULL) {
			if(opt_squid_id <= 0)
			{
				spos = strstr(dirp->d_name, ".runing");
			}
			else
			{
				char runingBuf[128];
				memset(runingBuf, 0, 128);
				snprintf(runingBuf, strlen(".runing.") + 3, ".runing.%d", opt_squid_id); // 3 bit of id
				spos = strstr(dirp->d_name, runingBuf);
			}
			if (spos != NULL) {
				memset(unlinkbuf, 0, 128);
				memset(test, 0 , 128);
				snprintf(unlinkbuf, 128, "/data/proclog/log/squid/flexicache/%s", dirp->d_name);
		
				if(isMicrosoft){
					if(opt_squid_id <= 0)
					{
						snprintf(test, 128, "/data/proclog/log/squid/microsoft%d.log",(int) time(NULL));
					}
					else
					{
						snprintf(test, 128, "/data/proclog/log/squid/microsoft%d.log.%d",(int) time(NULL), opt_squid_id);
					}
				}
				else{
					if(opt_squid_id <= 0)
					{
						snprintf(test, 128, "/data/proclog/log/squid/receipt%d.log", (int)  time(NULL));
					}
					else
					{
						snprintf(test, 128, "/data/proclog/log/squid/receipt%d.log.%d", (int)  time(NULL), opt_squid_id);
					}
				}

				i = 0; 
				while (((microfd = open(unlinkbuf, O_RDWR)) <= 0) && (i++ < 5));
				i = 0;
				while (((testfd = open(test, O_RDWR|O_CREAT, S_IWUSR|S_IRUSR|S_IWGRP|S_IRGRP|S_IROTH)) <= 0)&&(i++ < 5));
				while ((readln=read(microfd, testbuf, 4096))!= 0)
					write(testfd, testbuf, readln);
				close(microfd);
				unlink(unlinkbuf);
				close(testfd);
				char tmp[4096];
				snprintf(tmp,4096, "chown squid:squid %s", test);
				system(tmp);
				microfd = -1;
				pos = NULL;
				spos = NULL;
				break;
			}
		}
#else
		if (pos != NULL) {
			spos = strstr(dirp->d_name, ".runing");
			if (spos != NULL) {
				memset(unlinkbuf, 0, 128);
				memset(test, 0 , 128);
				snprintf(unlinkbuf, 128, "/data/proclog/log/squid/flexicache/%s", dirp->d_name);

				if(isMicrosoft){
					snprintf(test, 128, "/data/proclog/log/squid/microsoft%d.log",(int) time(NULL));
				}   
				else{
					snprintf(test, 128, "/data/proclog/log/squid/receipt%d.log", (int)  time(NULL));
				}   

				i = 0;  
				while (((microfd = open(unlinkbuf, O_RDWR)) <= 0) && (i++ < 5));
				i = 0;
				while (((testfd = open(test, O_RDWR|O_CREAT, S_IWUSR|S_IRUSR|S_IWGRP|S_IRGRP|S_IROTH)) <= 0)&&(i++ < 5))
					;
				while ((readln=read(microfd, testbuf, 4096))!= 0)
					write(testfd, testbuf, readln);
				close(microfd);
				unlink(unlinkbuf);
				close(testfd);
				char tmp[4096];
				snprintf(tmp,4096, "chown squid:squid %s", test);
				system(tmp);
				microfd = -1; 
				pos = NULL;
				spos = NULL;
				break;
			}
		}
#endif
    }
	
    debug(114, 5) ("initMicrosoftlog: init log data \n");
	closedir(fdp);
	fdp = NULL;
} 

void wipeoffnull(char * sendbuf)
{
	char *pos = NULL;
	while ((pos = strstr(sendbuf, "(null)")) != NULL) {
		char *p = pos;
	 	while ( *p != '\0') {
			*p = *(p+6);
			p++;
		}
		*(p+1) = '\0';
	}	
}

void microsoftEvent(void *arg)
{
	time_t timecur;
	struct stat buf;
	int i = 0;
	int outfd;
	if (fstat(microfd, &buf) < 0)
		fstat(microfd, &buf);
	timecur = time(NULL);
	if ((timecur - oldtime > 80) && (buf.st_size > 0)) {
		memset(filenamenew, 0, 512);
		
#ifdef CC_MULTI_CORE	
		if(strstr(filenamebuf,"down_ms") != NULL ){
			if(opt_squid_id <= 0)
			{
				snprintf(filenamenew, 128,"/data/proclog/log/squid/microsoft%d.log", (int) time(NULL));
			}
			else
			{
				snprintf(filenamenew, 128,"/data/proclog/log/squid/microsoft%d.log.%d", (int) time(NULL), opt_squid_id);
			}
		}
		else{
			if(opt_squid_id <= 0)
			{
				snprintf(filenamenew, 128,"/data/proclog/log/squid/receipt%d.log", (int) time(NULL));
			}
			else
			{
				snprintf(filenamenew, 128,"/data/proclog/log/squid/receipt%d.log.%d", (int) time(NULL), opt_squid_id);
			}
		}
#else
		if(strstr(filenamebuf,"down_ms") != NULL ){
			snprintf(filenamenew, 128,"/data/proclog/log/squid/microsoft%d.log", (int) time(NULL));
		}
		else{
			snprintf(filenamenew, 128,"/data/proclog/log/squid/receipt%d.log", (int) time(NULL));
		}
#endif

		lseek(microfd, 0, SEEK_SET);
		while (((outfd = open(filenamenew, O_RDWR|O_CREAT, S_IWUSR|S_IRUSR|S_IRGRP|S_IWGRP|S_IROTH)) <= 0) && (i++ <5));
			char readbuf[4096];
			int writeln;
			memset(readbuf, 0 , 4096);
			int j = 0;
			while ((writeln=read(microfd, readbuf, 4096)) != 0) //modify by sxlxb
			{
				if (writeln == -1) 
				{
					j++;
					cc_usleep(5000);
				}
				if (j > 5) 
					break;
				write(outfd, readbuf, writeln);
			}
			ftruncate(microfd, 0);
			close(microfd);
			close(outfd);
			microfd = -2;
			microfd = open(filenamebuf, O_RDWR);
			i = 0;
			while ((microfd <= 0) && (i++ < 5))
				microfd = open(filenamebuf, O_RDWR);
			oldtime = time(NULL);
	}
	if (buf.st_size == 0)
		oldtime = time(NULL);
	eventAdd("microsoftEvent", microsoftEvent, NULL, (double) 80, 0);
}

void writeMessagetoFile(int fd)
{
	fde *F = NULL;
	struct stat statbuf = {0};
    debug(114, 5) ("writeMessgaetoFile: FD %d\n", fd);
    assert(fd >= 0);
    assert(fd < Squid_MaxFD);
    F = &fd_table[fd];
	
	micromsg *fdemsg = NULL;
	fdemsg = cc_get_mod_private_data(FDE_PRIVATE_DATA, &fd, mod);
	
	if(fdemsg == NULL){
		return;
	}


	if ((fdemsg->msflag == 1) && strncmp(fdemsg->cip, "127.0.0.1", 9)) {
		char sendbuf[8192];
		char buftime[128];
		uint32_t fileloca = 0;
		uint32_t size = 0;
		uint64_t dsize = 0;
#define MICROFILESIZE 1024*1024*1024

		char sendbytesbuf[128];
		fdemsg->ftime = time(NULL);  //add by sxlxb for microsoft
		int test = fdemsg->ftime - fdemsg->btime;
		snprintf(buftime, 128,"%d", test);
		fdemsg->dtime = xstrdup(buftime);
		if(fdemsg->dsize != NULL)
			dsize = atoi(fdemsg->dsize);
		else 
			dsize = 0;
		if ( fdemsg->sendbytes < dsize) {
			if (fdemsg->http == 200) {
				fdemsg->win32 = 64;
				fdemsg->devent = xstrdup("abort");
				xfree(fdemsg->dsize);
				fdemsg->dsize = NULL;
				snprintf(sendbytesbuf, 128, "%lld", fdemsg->sendbytes);
				fdemsg->dsize = xstrdup(sendbytesbuf);
			}
			else if (fdemsg->http == 206) {
				if(fdemsg->rtype == 0){
					if(1 == fdemsg->rangefirst){
						fdemsg->win32 = 64;	
						fdemsg->devent = xstrdup("abort");
						xfree(fdemsg->dsize);
						fdemsg->dsize = NULL;
						snprintf(sendbytesbuf, 128, "%lld", fdemsg->sendbytes);
						fdemsg->dsize = xstrdup(sendbytesbuf);	
					}
					else{
						fdemsg->rangelast = 0;		
						fdemsg->rangefirst =0;	
					}
				}
				else {
					fdemsg->rangelast = 0;
					fdemsg->rangefirst =0;
				}
			}

		}
		else {
			if (fdemsg->rangefirst == 1)
				fdemsg->devent = xstrdup("begin");
			else
				fdemsg->devent = xstrdup("success");
			fdemsg->win32 = 0;
		}

		if (fdemsg->rangelast == 1) 
			fdemsg->lastbyte = 1;
		
		if(fdemsg->rtype == 0){
			if(1 == fdemsg->rangefirst)
				fdemsg->firstbyte = 1;
			if(200 == fdemsg->http)
				fdemsg->lastbyte = fdemsg->firstbyte = 1;
				
		}

		memset(sendbuf, 0 , 8192);
		if ((fdemsg->http == 200) || ((fdemsg->http == 206) && ((fdemsg->rangefirst == 1) ||(fdemsg->rangelast == 1))))
		{
			if(fdemsg->rtype == 1){
				snprintf(sendbuf, 8192,"/downloads/?Source=%s&UTC=%s&URI=%s&dEvent=%s&dsize=%s&win32=%d&http=%3d&lastbyte=%d&geoid=%s&MSID=%s&cip=%s&Dtime=%s&ruri=%s&UA=%s&DType=%sscip=%s\n", fdemsg->source, fdemsg->utc, strBuf(fdemsg->uri), fdemsg->devent, fdemsg->dsize,fdemsg->win32, fdemsg->http, fdemsg->lastbyte, fdemsg->id, strBuf(fdemsg->msid), fdemsg->cip, fdemsg->dtime, fdemsg->ruri, fdemsg->ua, fdemsg->dtype,fdemsg->scip);
			}
			else{
				snprintf(sendbuf, 8192,"Domain=%s|Source=%s|UTC=%s|URI=%s|dEvent=%s|dsize=%s|win32=%d|http=%3d|firstbyte=%d|lastbyte=%d|geoid=%s|MSID=%s|cip=%s|Dtime=%s|ruri=%s|UA=%s|DType=%s|scip=%s\n",fdemsg->domain, fdemsg->source, fdemsg->utc, strBuf(fdemsg->uri), fdemsg->devent, fdemsg->dsize, fdemsg->win32, fdemsg->http, fdemsg->firstbyte, fdemsg->lastbyte, fdemsg->id, strBuf(fdemsg->msid), fdemsg->cip, fdemsg->dtime, fdemsg->ruri, fdemsg->ua, fdemsg->dtype,fdemsg->scip);	
			}
		}	

		wipeoffnull(sendbuf);
		
    	debug(114, 5) ("receipt buffer:  %s\n", sendbuf);
		size = strlen(sendbuf);
		time_t curtime;
		int outfd;
		int i = 0;
		
		//check if mod_receipt conf is modified
		char *receipttype = NULL;
		if(checkReceiptType() == MICROSOFT){
			receipttype = "down_ms";
		}
		else{
			receipttype = "down_receipt";
		}
		
    	debug(114, 5) ("receipt type:  %s\n", receipttype);
    	debug(114, 5) ("receipt type:  %s\n", filenamebuf);
		
		if((strlen(filenamebuf) > 0) &&(strstr(filenamebuf,receipttype)==NULL)){
			//deal with the the old data
			if(microfd > 0){
				close(microfd);
			}
			
			initMicrosoftlog(); 
			
			microfd = -1;
			
	    }

		if ((microfd == -1)) {
			memset(filenamebuf, 0, 512);
#ifdef CC_MULTI_CORE	
			if(checkReceiptType() == MICROSOFT){
				if(opt_squid_id <= 0)
				{
					snprintf(filenamebuf, 128, "/data/proclog/log/squid/flexicache/down_ms%d.runing", (int) time(NULL));
				}
				else
				{
					snprintf(filenamebuf, 128, "/data/proclog/log/squid/flexicache/down_ms%d.runing.%d", (int) time(NULL), opt_squid_id);
				}
			}
			else{
				if(opt_squid_id <= 0)
				{
					snprintf(filenamebuf, 128, "/data/proclog/log/squid/flexicache/down_receipt%d.runing", (int) time(NULL));
				}
				else
				{
					snprintf(filenamebuf, 128, "/data/proclog/log/squid/flexicache/down_receipt%d.runing.%d", (int) time(NULL), opt_squid_id);
				}
			}
#else
			if(checkReceiptType() == MICROSOFT){
				snprintf(filenamebuf, 128, "/data/proclog/log/squid/flexicache/down_ms%d.runing", (int) time(NULL));
			}
			else{
				snprintf(filenamebuf, 128, "/data/proclog/log/squid/flexicache/down_receipt%d.runing", (int) time(NULL));
			}
#endif
			while ((microfd <= 0) && (i++ <5))
				microfd = open(filenamebuf, O_RDWR|O_CREAT, S_IWUSR|S_IRUSR|S_IWGRP|S_IROTH);
			oldtime = time(NULL);
		}
		
		curtime = time(NULL);
		fileloca = lseek(microfd, 0, SEEK_CUR);
		if (((curtime - oldtime > 60)&&(fileloca != 0)) || (size + fileloca > MICROFILESIZE)) {
			memset(filenamenew, 0, 512);
			
#ifdef CC_MULTI_CORE	
			if( strstr(filenamebuf,"down_ms") != NULL ){
				if(opt_squid_id <= 0)
				{
					snprintf(filenamenew, 512,"/data/proclog/log/squid/microsoft%d.log",(int) time(NULL));
				}
				else
				{
					snprintf(filenamenew, 512,"/data/proclog/log/squid/microsoft%d.log.%d",(int) time(NULL), opt_squid_id);
				}
			}
			else{
				if(opt_squid_id <= 0)
				{
					snprintf(filenamenew, 512,"/data/proclog/log/squid/receipt%d.log", (int) time(NULL));
				}
				else
				{
					snprintf(filenamenew, 512,"/data/proclog/log/squid/receipt%d.log.%d", (int) time(NULL), opt_squid_id);
				}
			}
#else
			if( strstr(filenamebuf,"down_ms") != NULL ){
				snprintf(filenamenew, 512,"/data/proclog/log/squid/microsoft%d.log",(int) time(NULL));
			}
			else{
				snprintf(filenamenew, 512,"/data/proclog/log/squid/receipt%d.log", (int) time(NULL));
			}
#endif
			
			i = 0;
			while (((outfd = open(filenamenew, O_RDWR|O_CREAT, S_IWUSR|S_IRUSR|S_IRGRP|S_IWGRP|S_IROTH)) <= 0) && (i++ <5));
			char readbuf[4096];
			int writeln;
			memset(readbuf, 0 , 4096);
			fileloca = lseek(microfd, 0, SEEK_SET);
			int j = 0;
			while ((writeln=read(microfd, readbuf, 4096)) != 0) //modify by sxlxb
			{
				if (writeln == -1) 
				{
					j++;
					cc_usleep(5000);
				}
				if (j > 5) 
					break;
				write(outfd, readbuf, writeln);
			}
			ftruncate(microfd, 0);
			close(microfd);
			fstat(outfd, &statbuf);
			close(outfd);
			if (statbuf.st_size == 0)
				unlink(filenamenew);
			microfd = -2;
			microfd = open(filenamebuf, O_RDWR);
			i = 0 ;
			while ((microfd <= 0) && (i++ < 5))
				microfd = open(filenamebuf, O_RDWR);

			oldtime = time(NULL);
		}
		if (write(microfd, sendbuf, size) != size)
			debug (114, 0) ("writeMessgaeToFile: receipts write file error, please check!");
		
		static int count = 0;
		if (count == 0) {
			eventAdd("microsoftEvent", microsoftEvent, NULL, (double) 60, 0);
			count ++;
		}
	}
	//microsoft_free(F);
	
	//free_fde_private_data(fdemsg);
}


/* callback: free the data */
static int free_mod_config(void *data)
{
	mod_receipt_param *cfg = (mod_receipt_param *)data;
	if (cfg)
	{
		safe_free(cfg->type);
		safe_free(cfg->source);
		safe_free(cfg->dtype);
		memPoolFree(mod_config_pool, cfg);
		cfg = NULL;
	}
	return 0;
}

/////////////////////// hook functions ////////////////

/**
  * System init , call initMicrosoftlog();
  */

static int sys_init()
{
/*#ifdef CC_MULTI_CORE	
	int do_not_use_mod_receipt_in_fc6 = (opt_squid_id <= 0);
	assert(do_not_use_mod_receipt_in_fc6);
#endif	*/
	if (!do_microsoft){
		initMicrosoftlog();  //add by sxlxb for microsoft
		do_microsoft = 1;
	}
	return 0;
}


/**
  * parse config line to mod_config
  * return 0 if parse ok, -1 if error
  */
static int sys_parse_param(char *cfg_line)
{
	
	char *token = NULL;
	if ((token = strtok(cfg_line, w_space)) == NULL)
		return -1;
	else if(strcmp(token,"mod_receipt"))
		return -1;

	mod_receipt_param *cfg = mod_config_pool_alloc();
	
   	debug(114, 5) ("mod_receipt: parse paramter: %s \n", cfg_line);
	token = strtok(NULL, w_space);
	if (!strcasecmp(token, "common"))
		cfg->type = xstrdup(token);
	else if(!strcasecmp(token, "microsoft"))
		cfg->type = xstrdup(token);
	else{
    	debug(114, 2) ("mod_receipt: paramter type is error:conf line %s \n", cfg_line);
		goto err;
	}
	
	token = strtok(NULL, w_space);
	if(token == NULL){
    	debug(114, 2) ("mod_receipt: paramter source is error:conf line %s \n", cfg_line);
		goto err;
	}
	else 
		cfg->source = xstrdup(token);
	
	token = strtok(NULL, w_space);
	if(token ==NULL){
    	debug(114, 2) ("mod_receipt: paramter dtype is error:conf line %s \n", cfg_line );
		goto err;
	}	
	else 
		cfg->dtype = xstrdup(token);

	
	cc_register_mod_param(mod, cfg, free_mod_config);
	
    debug(114, 5) ("mod_receipt: paramter type= %s source= %s dtype= %s \n", cfg->type, cfg->source, cfg->dtype );
	
	
	return 0;

err:
	if (cfg)
	{
		safe_free(cfg->type);
		safe_free(cfg->source);
		safe_free(cfg->dtype);
		memPoolFree(mod_config_pool, cfg);
		cfg = NULL;
	}

	return -1;
}

static int cleanup(cc_module *mod)
{
    eventDelete(microsoftEvent,NULL);
    if(NULL != mod_config_pool)
	    memPoolDestroy(mod_config_pool);
    if(NULL != micromsg_pool)
	    memPoolDestroy(micromsg_pool);
	//free mod point	
	return 0;
}

static int writeMsgForClose(int fd){
	
	micromsg *fdemsg = NULL;
	fdemsg = cc_get_mod_private_data(FDE_PRIVATE_DATA, &fd, mod);
	
	fde *F = &fd_table[fd];	
	if(F->cctype == 1 ){
	    //debug(114,5) ("mod_receipt: close %d fd  cctype = 1\n", fd);
        return 1;
 	}

	
	if(fdemsg != NULL && fdemsg->aliveflag != 1){
		
    	debug(114,5) ("mod_receipt: close %d fd and write the receipt log \n", fd );
		writeMessagetoFile(fd);  //add by sxlxb for microsoft
	}
	return 0;
}

static int writeMsgForKeepaliveRequest(int fd){
	
	micromsg *fdemsg = NULL;
	fdemsg = cc_get_mod_private_data(FDE_PRIVATE_DATA, &fd, mod);
	
	fde *F = &fd_table[fd];	
	if(F->cctype == 1 ){
	    //debug(114,5) ("mod_receipt: close %d fd  cctype = 1\n", fd);
        return 1;
 	}


	if(fdemsg != NULL){
		
    	debug(114,5) ("mod_receipt: set keepalive flag %d \n", fd );

		fdemsg->aliveflag = 1;
		writeMessagetoFile(fd);  //add by sxlxb for microsoft
	}
	return 0;

}

static int setMessageBytes(int fd, int len, unsigned int type){

	if(type == FD_WRITE){
		
		fde *F = &fd_table[fd];	
		if(F->cctype == 1 ){
		    //debug(114,5) ("mod_receipt: close %d fd  cctype = 1\n", fd);
        	return 1;
 		}


		micromsg *fdemsg = NULL;
		
		fdemsg = cc_get_mod_private_data(FDE_PRIVATE_DATA, &fd, mod);
	
		if(fdemsg != NULL){
	 		
    		//debug(114,5) ("mod_receipt: set %d bytes for %d \n", len, fd);

			fdemsg->sendbytes += len; //add by sxlxb for Microsoft
		}
	}
	return 0;
}

//static int setHttpAcceptStatus(int fd){
	
//	micromsg *fdemsg = NULL;
//	fdemsg = cc_get_mod_private_data(FDE_PRIVATE_DATA, &fd, mod);
//	
//	if(fdemsg != NULL){
//		
  //  	debug(114,5) ("mod_receipt: set accept begin %d \n", fd );
//
//		fdemsg->isaccept = 1;
//	}
	
	/*return value maybe use the private httpAccept*/
//	return 0; 
//}



int initMicroMsg(clientHttpRequest* http){
	fde *F = NULL;
	int fd = http->conn->fd;
	
	F = &fd_table[fd];
	
	micromsg *fdemsg = NULL;
	fdemsg = cc_get_mod_private_data(FDE_PRIVATE_DATA, &fd, mod);
	
    //debug(114, 5)("mod_receipt: read request begin %d \n", fd );

	if(fdemsg == NULL){
		fdemsg = micromsg_pool_alloc();

		fdemsg->isaccept = 1;
		fdemsg->http = 0;
		fdemsg->win32 =0;
		fdemsg->rangefirst = 0;
		fdemsg->rangelast = 0;
		fdemsg->msflag = 0;
		fdemsg->firstbyte = 0;
		fdemsg->lastbyte = 0;
		if(checkReceiptType() == MICROSOFT)
			fdemsg->rtype = 1;
		else
			fdemsg->rtype = 0;

		cc_register_mod_private_data(FDE_PRIVATE_DATA, &fd, fdemsg, free_fde_private_data, mod);
	}

	//call the http request
	
	clientObtainRequestMessage(http,http->request);	
	
	/*return value maybe use the private httpAccept*/
	return 0; 
}


// module init 
int mod_register(cc_module *module)
{
	debug(114, 1)("(mod_receipt) ->  init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");
	
	//hook 功能入口函数
	//module->hook_func_sys_init = sys_init;
	cc_register_hook_handler(HPIDX_hook_func_sys_init,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_init),
			sys_init);
	//module->hook_func_sys_parse_param = sys_parse_param;
	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
			sys_parse_param);

	//module->hook_func_sys_cleanup = cleanup;
	cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_cleanup),
			cleanup);

	//module->hook_func_fd_close = writeMsgForClose;
	cc_register_hook_handler(HPIDX_hook_func_fd_close,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_fd_close),
			writeMsgForClose);

	//module->hook_func_fd_bytes = setMessageBytes;
	cc_register_hook_handler(HPIDX_hook_func_fd_bytes,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_fd_bytes),
			setMessageBytes);

	//module->hook_func_http_req_read = initMicroMsg;
	cc_register_hook_handler(HPIDX_hook_func_http_req_read,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_read),
			initMicroMsg);

	//module->hook_func_private_client_build_reply_header = clientObtainReplyMessage;
	cc_register_hook_handler(HPIDX_hook_func_private_client_build_reply_header,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_client_build_reply_header),
			clientObtainReplyMessage);

	//module->hook_func_private_client_keepalive_next_request = writeMsgForKeepaliveRequest;
	cc_register_hook_handler(HPIDX_hook_func_private_client_keepalive_next_request,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_client_keepalive_next_request),
			writeMsgForKeepaliveRequest);

	//mod = module;
	//mod = (cc_module*)cc_modules.items[module->slot];
	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	return 0;

}

#endif


