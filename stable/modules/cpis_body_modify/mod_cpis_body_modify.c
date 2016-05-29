#include "cc_framework_api.h"

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include "mod_cpis_body_modify.h"

#ifdef CC_FRAMEWORK

static cc_module* mod = NULL;

typedef struct _mod_conf_param {
	char* replace_str;
	int replace_size;
	char* find_str;
	int find_size;
	MEM_VALUE * header;
	FILE * cpislog_fd;
	int Before_flag;
}mod_conf_param;

typedef struct _store_param
{
	long content_length;
	long remain_clen;
	int  chunk_or_not ;
} store_param;

static int free_store_param(void* data){
	store_param *param = (store_param *) data;

	if(param != NULL){
		param->content_length = -1;
		param->remain_clen = -1;
		safe_free(param);
	}
	return 0;
}

// if reply HTTP_OK:200
static int if_reply_ok(HttpReply *reply)
{
/*
	if(reply && (reply->sline.status) > 0)
	{
		return (reply->sline.status == HTTP_OK );
	}
	else
		//no reply ???
		return 1;
*/

	if(reply != NULL)
	{
		return (reply->sline.status == HTTP_OK )?1:0;
	}
	else
		return 1;
}

/* destroy the list */
static MEM_VALUE *
destroyList(MEM_VALUE * header)
{	
	MEM_VALUE  *tmp_list;
	struct list_head * tmp;

	if(header == NULL)
	{
		debug(142,5)("(mod_cpis_body_modify) ->Destory Is NULL\n");
		return NULL;
	}
	tmp_list = header;
	/* ajust the cur_pbliblist */
	do{
		tmp_list = header;
		tmp = &header->list;
		if(tmp == tmp->next)
			header = NULL;
		else {
			tmp = tmp->next;
			header = list_entry(tmp, MEM_VALUE, list);
			list_del(&tmp_list->list);
		}
		debug(142,5)("(mod_cpis_body_modify) -> destroy mem:%s.\n", tmp_list->value);
		safe_free(tmp_list);
	}while(header != NULL);
	return NULL;
}

static int free_callback(void* param_data)
{
	mod_conf_param *param = (mod_conf_param*) param_data;
	if(param){
		param->header = destroyList(param->header);
		safe_free(param->replace_str);
		safe_free(param->find_str);
		if(param->cpislog_fd != NULL)
			fclose(param->cpislog_fd);
		safe_free(param);
	}
	return 0;
}

/***
  * return value:
  * 1: ip fit cidr
  * 0: ip donot fit cidr
  */
static int
checkIP_CIDR(char *ip, char *cidr)
{
	char cidrIP[64], cidrN[10], *q,*p;
	struct in_addr ip_number;
	unsigned int hostip, mask, network;
	int num;

	p = cidr;
	skip_whitespace(p);
	/* parse cidr ip */
	if(!(q = strchr(cidr, '/')))
		return 0;
	memcpy(cidrIP, p, q-p);
	cidrIP[q-p] = '\0';

	/* bad ip */
	if(cli_check_keyword_single_ip(cidrIP))
	{
		debug(142, 6)("(mod_cpis_body_modify)->Ignore Invalid CIDR:%s\n", cidr);
		return 0;
	}
	
	p = ++q;
	/* parse cidr N*/
	if(!(q = strchr(cidr, '\0')))
		return 0;
	memcpy(cidrN, p, q-p);
	cidrN[q-p] = '\0';

	/* check for the cidr number */
	for(num=0; num<q-p; num++)
	{
		if(!isdigit(cidrN[num]))
		{
			debug(142, 6)("(mod_cpis_body_modify)->->Ignore Invalid CIDR:%s\n", cidr);
			return 0;
		}
	}
	num = atoi(cidrN);

	/* invalid cidr number */
	if(num<0 || num >32)
		return 0;
	/* xxx.xxx.xxx.xxx/0 */
	if(num == 0)
		return 1;

	/* get the CIDR network */
	inet_pton(AF_INET, cidrIP,  &hostip);
	ip_number.s_addr=htonl(hostip);

	mask= 0xffffffff<<(32-num);
	network = mask&ip_number.s_addr;

	/* get the ip Number */
	inet_pton(AF_INET, ip,  &hostip);
	ip_number.s_addr=htonl(hostip);
	hostip = mask&ip_number.s_addr;

	return hostip==network;
}

static int
cpislogWrite(FILE * fd, char *ip, char *URL, int mobile)
{
	debug(142, 6)("(mod_cpis_body_modify)->write log CIDR\n");
	if(fd == NULL)
		return -1;
	char cpislog[4096];
	int count;
	if(mobile)
	{
		count = snprintf(cpislog,4094, "%d qiyi MobileIp %s %s", (int)time(NULL), 
				ip, URL);
	} else {
		count = snprintf(cpislog,4094, "%d qiyi NoMobileIp %s %s", (int)time(NULL), 
				ip, URL);
	}
	cpislog[count++] = '\n';
	cpislog[count] = '\0';
	fputs(cpislog, fd);
	fflush(fd);
	return 0;
}
/**
  * return value:
  * fit : 1 
  * not fit : 0
  */
static int
checkIPfitCIDR(char *IP, MEM_VALUE *list_header)
{
	MEM_VALUE *tmp_value;
	struct list_head * tmp;
	int flag = 0;

	if(list_header == NULL)
	{
		return flag;
	}
	else{
		tmp_value = list_header;
		do{
			/* check the ip and cidr */
			if(checkIP_CIDR(IP, tmp_value->value))
			{
				flag = 1;
				debug(142, 3)("(mod_cpis_body_modify)->  ip:%s fit CIDR:%s \n", 
					IP, tmp_value->value);	
				break;
			} 
			/* go to the next value */
			else{
				tmp = &tmp_value->list;
				tmp = tmp->next;
				tmp_value = list_entry(tmp, MEM_VALUE, list);
			}
		}while(tmp_value!=list_header);
	}

	return flag;
}

static HttpHeaderEntry* httpHeaderFindEntry2(const HttpHeader * hdr, http_hdr_type id)
{
	HttpHeaderPos pos = HttpHeaderInitPos;
	HttpHeaderEntry *e;
	assert(hdr);

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
	return NULL;        /* not reached */
}

static const char * httpHeaderGetStr2(const HttpHeader * hdr, http_hdr_type id)
{
	HttpHeaderEntry *e;
	if ((e = httpHeaderFindEntry2(hdr, id)))
	{       
		return strBuf(e->value);
	}       
	return NULL;
}

static size_t storeModifyBody(char *buf, ssize_t len, HttpStateData * httpState, char *alloc_buf, ssize_t alloc_size,mod_conf_param *cfg, store_param *str_param)
{
	char *http_header = 0;
	char tmp[128] = {0};
	char *tokbuf = buf;
	char rstbuf[65536*2];
	memset(rstbuf, 0, sizeof(rstbuf));

	int pos = 0, log_count;
	char *ip_tmp, ip[64], log_url[1024], *log_tmp1, *log_tmp2;
	int count = 0;
	char tmpbuf[65536];
	memset(tmpbuf,0,65536);

	size_t ret_len = -1;

	assert(cfg != NULL);
	strncpy(tmp,cfg->replace_str,cfg->replace_size);

	int add_len = strlen(tmp);

	memcpy(tmpbuf, buf, len);

	debug(142,5)("mod_cpis_body_modify buf[ %s]  len[%d]  alloc_size[%d]\n",tmpbuf,(int)len,(int)alloc_size);

	tokbuf = tmpbuf;

	debug(142,5)("mod_cpis_body_modify tokenbuf[ %s] cfg->find_str[%s],cfg->Before_flag [%d]\n",tokbuf,cfg->find_str,cfg->Before_flag );

	while ((http_header = strstr(tokbuf,cfg->find_str)) != NULL)
	{
		/* construct the URL for cpis log */
		log_tmp1 = http_header;
		log_tmp2 = strchr(log_tmp1, '"');

		if(log_tmp2 != NULL)
		{
			log_count = log_tmp2-log_tmp1;
			if(log_count < 1024)
			{
				memcpy(log_url, log_tmp1, log_count);
				log_url[log_count] = '\0';
			} else /* if the URL is too long ,just truncate to 1022 byte */
			{
				memcpy(log_url, log_tmp1, 1022);
				log_url[1022] = '\0';
			}
		} else {
			//zh (overflow)		strcpy(log_url, log_tmp1);
			strncpy(log_url, log_tmp1,1023);
			log_url[1023] = '\0';
		}


		debug(142,5)("mod_cpis_body_modify-->find_str[%s]find_size[%d]\n",cfg->find_str,cfg->find_size);
		if(cfg->Before_flag != 1)
			http_header = http_header + cfg->find_size;

		//zh		debug(142,5)("mod_cpis_body_modify #################### rstbuf[ %s] \n",rstbuf);

		/* add by zhang */
		/* check out the ip */
		ip_tmp = strchr(http_header, '/');
		strcpy(ip, "Not IP");
		if(ip_tmp != NULL)
		{
			if(ip_tmp - http_header <= 32) 
			{
				memset(ip, 0, sizeof(ip));
				memcpy(ip, http_header, ip_tmp-http_header);
				ip[ip_tmp-http_header] = '\0';
				debug(142,5)("mod_cpis_body_modify-->ip[%s]\n",ip);
				if(!cli_check_keyword_single_ip(ip))
				{
					/**
					 * ip fit cidr
					 * no add the add url
					 */
					if(checkIPfitCIDR(ip, cfg->header))
					{
						if(cfg->Before_flag != 1)
							http_header = http_header + cfg->find_size;
						memcpy(rstbuf+pos,tokbuf,http_header-tokbuf);
						pos += http_header-tokbuf; 
						tokbuf = http_header;
						if(cfg->header != NULL)
							cpislogWrite(cfg->cpislog_fd, ip, log_url, 1);
						continue;
					}
				}
			}
		}

		debug(142,9)("(mod_cpis_body_modify)->pos: %d\n", pos);
		/* end by zhang */
		debug(142,9)("(mod_cpis_body_modify)->IP:%s,added url:%s\n", ip, tmp);
		//zh		debug(142,5)("mod_cpis_body_modify !!!!!!!!!!!!!!!!! rstbuf[ %s] \n",rstbuf);

		if(cfg->Before_flag == 1)
		{
			memcpy(rstbuf+pos,tokbuf,http_header-tokbuf);
			pos += http_header-tokbuf; 
			memcpy(rstbuf+pos,tmp,add_len);
			pos +=add_len;
			memcpy(rstbuf+pos,cfg->find_str,cfg->find_size);
			pos += cfg->find_size; 

			http_header = http_header + cfg->find_size;
			tokbuf = http_header;
		}
		else
		{
			debug(142,9)("(mod_cpis_body_modify)->pos: %d\n", pos);
			debug(142,9)("(mod_cpis_body_modify)->http_header-tokbuf: %d\n",(int)(http_header-tokbuf));
			memcpy(rstbuf+pos,tokbuf,http_header-tokbuf);
			pos += http_header-tokbuf; 
			debug(142,9)("(mod_cpis_body_modify)->pos: %d\n", pos);
			debug(142,9)("(mod_cpis_body_modify)->add_len: %d\n", add_len);
			memcpy(rstbuf+pos,tmp,add_len);
			pos += add_len;
			debug(142,9)("(mod_cpis_body_modify)->pos: %d\n", pos);
			tokbuf = http_header;

			debug(142,5)("mod_cpis_body_modify current offset in ori buf: %d\n", (int)(http_header - tmpbuf));
			debug(142,5)("mod_cpis_body_modify current offset in rst buf: %d\n", pos);
		}
		/* write log */
		if(cfg->header != NULL)
			cpislogWrite(cfg->cpislog_fd, ip, log_url, 0);

		count ++;
		debug(142,5)("mod_cpis_body_modify tokenbuf[%s] rstbuf[%s] len[%d] count[%d]\n",tokbuf,rstbuf,(int)len,count);
	}

	debug(142,5)("mod_cpis_body_modify len[%d], pos[%d], count[%d],tokbuf offset[%d]\n",  (int)len, pos, count, (int)(tokbuf-tmpbuf));
	memcpy(rstbuf+pos,tokbuf,len-pos+count*add_len);
	len += count*add_len;
	rstbuf[len] = '\0';

	//zh -->
	debug(142,5)("mod_cpis_body_modify buf_end[ %s]  len[%d]  alloc_size[%d]\n",rstbuf,(int)len,(int)alloc_size);

	if(alloc_size >= len)
		memcpy(alloc_buf, rstbuf, len);
	else
	{
		debug(142,1)("mod_cpis_body_modify: alloc_size[%d] < len[%d]\n", (int)alloc_size, (int)len);
		return -1;
	}
	//zh <--

	debug(142,5)("mod_cpis_body_modify str_param->chunk_or_not3: %d\n", str_param->chunk_or_not);

	ret_len = len;

	return ret_len;
}

static int storeDeleteContentLength(char *buf, ssize_t len, HttpStateData * httpState, long *content_length,int *transfer_encoding)
{

	char tmpbuf[65536];
	memset(tmpbuf,0,65536);

	strcpy(tmpbuf, httpState->reply_hdr.buf);

	char* content_line_begin = NULL;
	char* content_line_end = NULL;
//	int content_line_len = -1;


	if((content_line_begin = strstr(tmpbuf,"Content-Length:")) != NULL)
	{
		debug(142, 3)("(mod_cpis_body_modify) -> rm content-length\n");

		if((content_line_end = strchr(content_line_begin, '\n')) == NULL)
		{
			debug(142,0)("mod_cpis_modify_body: FATAL has not complete Content-Length header line \n");
			assert(0);

		}


		*content_length = atol(content_line_begin+15);

/*
		content_line_len = content_line_end - content_line_begin + 1;

		debug(142,5)("(mod_cpis_body_modify) -> content_length line len:[%d]\n", content_line_len);

		memmove(content_line_begin, content_line_end + 1, len - (content_line_end - buf + 1));

		len -= content_line_len;
		buf[len] = '\n';
*/

		return len;
	}
	else if(strstr(tmpbuf,"Transfer-Encoding:") != NULL){

		*transfer_encoding = 1;
	}
	else{
		debug(142,0)("mod_cpis_modify_body: FATAL has not Content-Length or Transfer-Encoding header:[%s] \n", tmpbuf);
		*transfer_encoding = 1;
	}

	return len;
}


static int func_modify_buf_recv_from_s_body(HttpStateData *httpState,char *buf, ssize_t len)
{
	if(!if_reply_ok(httpState->entry->mem_obj->reply))
	{
		debug(142, 1)("(mod_cpis_body_modify) -> httpState entry url: %s, reply status: %d is not 200, so we don't process it\n", storeUrl(httpState->entry), httpState->entry->mem_obj->reply->sline.status);
		return 0;
	}

	char tmp[128];
	char tmpbuf[65536];

	memset(tmp, 0, sizeof(tmp));
	memset(tmpbuf, 0, sizeof(tmpbuf));

	memcpy(tmpbuf, buf, len);

	mod_conf_param *cfg = cc_get_mod_param(httpState->fd, mod);
	if(cfg == NULL){

		debug(142, 3)("(mod_cpis_body_modify) -> mod conf parameter is null \n");
		return 0;
	}
	strncpy(tmp,cfg->replace_str,cfg->replace_size);

	debug(142,5)("mod_cpis_body_modify buf before[ %s]  len[%d]  \n",tmpbuf,(int)len);
	if(len <= 0 || buf == NULL || strlen(tmpbuf) <= 0){
		
		debug(142, 3)("(mod_cpis_body_modify) -> mod conf parameter is null len=%d strlen=%lu \n",(int)len,(unsigned long)strlen(tmpbuf));
		return 0;
	}
//	int add_len = strlen(tmp);

	store_param *str_param = cc_get_mod_private_data(STORE_ENTRY_PRIVATE_DATA, httpState->entry, mod);

	if(str_param == NULL) {

		str_param = xmalloc(sizeof(store_param));
		memset(str_param,0,sizeof(store_param));

		str_param->content_length = -1;
		str_param->remain_clen = -1;
		str_param->chunk_or_not = 0;

		cc_register_mod_private_data(STORE_ENTRY_PRIVATE_DATA, httpState->entry, str_param, free_store_param, mod);
	}

	long content_length = -1;
	int transfer_encoding = 0;

	len = storeDeleteContentLength((char *)buf, len, httpState,&content_length, &transfer_encoding);

	memset(tmpbuf, 0, sizeof(tmpbuf));
	memcpy(tmpbuf, buf, len);

	debug(142,5)("mod_cpis_body_modify buf3[ %s]  len[%d]  content-length[%ld]\n",tmpbuf,(int)len,content_length);

	str_param->chunk_or_not = transfer_encoding;

	debug(142,5)("mod_cpis_body_modify str_param->chunk_or_not1: %d\n", str_param->chunk_or_not);

	if(content_length != -1)
	{
		str_param->content_length = content_length;
		str_param->remain_clen    = content_length;
	}

	return 0;
}


/* parse on line */
static MEM_VALUE*
getValueList(char *line, MEM_VALUE * header)
{
	char *p, *q;
	MEM_VALUE * value_mem;

	q = line;
	skip_whitespace(q);
	do{
		if(!(p=strchr(q, ';')))
			break;
		/* malloc a value struct */
		value_mem = (MEM_VALUE *)xmalloc(sizeof(MEM_VALUE));
		if(value_mem == NULL)
		{
			printf("Error Malloc Mem_value\n");
			break;
		}
		memset(value_mem, 0, sizeof(MEM_VALUE));
		memcpy(value_mem->value, q, p-q);

		/* adjust the value list */
		if(header== NULL)
		{
			header = value_mem;
			init_list_head(&header->list);
		} else
		{
			list_add_tail(&value_mem->list, 
						&header->list);
		}
		q = ++p;
	}while(*q != '\0');
	return header;

}


/* parse the configure file */
static int 
init_configure(char * path, struct _mod_conf_param *data)
{
	FILE * fd;
	char buf[1024], *tmp;

	/* open config file if it exist */
	if ((fd = fopen(path, "r")) == NULL)
	{
		debug(142,5)("(mod_cpis_body_modify) -> open configure file fault:%s\n", path);
		if ((fd = fopen(CONFIGEFILE, "r")) == NULL)
		{
			/* default file doesnot exist */
			debug(142,5)("(mod_cpis_body_modify) -> open configure file fault:%s\n", CONFIGEFILE);
			return -1;
		} else 
		{
			debug(142,5)("(mod_cpis_body_modify) -> use default config file:%s\n", CONFIGEFILE);
		}
	} 

	/* construct the CIDR list */
	while(1)
	{
		/* the end of the file */
		if(feof(fd))
		{
			fclose(fd);
			return 0;
		}
		fgets(buf, 1024, fd);
		tmp = buf;
		skip_whitespace(tmp);

		/* empty line or with the start of '#' */
		if(strlen(tmp)==0 || *tmp=='\n'
					|| *tmp=='#')
			continue;

		data->header = getValueList(tmp, data->header);
	}
	
	fclose(fd);
	return 0;
}

static int
print_config(MEM_VALUE * list_header)
{
	MEM_VALUE *tmp_value;
	struct list_head * tmp;

	if(list_header == NULL)
		return 0;
	tmp_value = list_header;
	debug(142, 3)("(mod_cpis_body_modify) ->config file:\n");
	do{
		debug(142, 3)("\t\t\t%s\n", tmp_value->value);
		tmp = &tmp_value->list;
		tmp = tmp->next;
		tmp_value = list_entry(tmp, MEM_VALUE, list);
	}while(tmp_value!=list_header);

	return 0;
}

static int hook_init(char *path, struct _mod_conf_param *data)
{
	debug(142, 3)("(mod_cpis_body_modify) -> hook_init:----:)\n");
	init_configure(path, data);
	print_config(data->header);
	return 0;
}


/* mod_cpis_body_modify http:// mod_cpis_body_modify/ dynamicIP [DIR] [BeforeAdding]allow all */
static int func_sys_parse_param(char *cfg_line)
{
	mod_conf_param *data = NULL;
	char* token = NULL;

	if ((token = strtok(cfg_line, w_space)) != NULL)
	{
		if(strcmp(token, "mod_cpis_body_modify"))
			return -1;	
	}
	else
	{
		debug(142, 3)("(mod_cpis_body_modify) ->  parse line error\n");
		return -1;	
	}

	if (NULL == (token = strtok(NULL, w_space)))
	{
		debug(142, 3)("(mod_cpis_body_modify) ->  parse line data does not existed\n");
		return -1;
	}

	data = xmalloc(sizeof(mod_conf_param));
	
	data->find_str  = xmalloc(strlen(token)+1); //must have +1 ,or else program is error for :memory corruption
	data->find_size = strlen(token);
	memset(data->find_str,0,data->find_size+1);	
	strcpy(data->find_str,token);
	//data->find_str[data->find_size+1]= '\0';
	/*int tmp_size = strlen(token);
	data->find_str  = xmalloc(tmp_size);
	memset(data->find_str,0,tmp_size);	
	data->find_size = tmp_size;
	memcpy(data->find_str,token,tmp_size);
*/
	if (NULL == (token = strtok(NULL, w_space)))
	{
		debug(142, 3)("(mod_cpis_body_modify) ->  parse line data does not existed\n");
		free_callback(data);
		return -1;
	}
	
	data->replace_str  = xmalloc(strlen(token)+1); //must have +1 ,or else program is error for :memory corruption
	data->replace_size = strlen(token);
	memset(data->replace_str,0,data->replace_size+1);	
	strcpy(data->replace_str,token);
	data->cpislog_fd = NULL;

/* add by zhang */
	/* cpis log fd */
	data->cpislog_fd = fopen(CPISLOG, "a+");
	if(NULL == data->cpislog_fd)
		debug(142, 3)("(mod_cpis_body_modify) ->  open cpis log file failed:%s\n", CPISLOG);

	data->header = NULL;
	//cidr file name
	if (NULL == (token = strtok(NULL, w_space)))
	{
		debug(142, 3)("(mod_cpis_body_modify) ->  parse line data does not existed\n");
		return -1 ;
	}

	/* before adding init */
	data->Before_flag = 0;
	/* case the dynamicIP paramter */
	if(!strcmp(token, "dynamicIP"))
	{
		debug(142, 3)("(mod_cpis_body_modify) ->  parse line data exist dynamicIP\n");

		if (NULL == (token = strtok(NULL, w_space)))
		{
			debug(142, 3)("(mod_cpis_body_modify) ->  parse line data does not existed\n");
			return -1 ;
		}

		if (strcmp(token, "allow") && strcmp(token, "deny") &&  strcmp(token, "on") 
		&& strcmp(token, "off")&& strcmp(token, "BeforeAdding"))
		{
			hook_init(token, data);
		} else
		{
			debug(142, 3)("(mod_cpis_body_modify) ->  missing cidr file name \n");
			hook_init(CONFIGEFILE, data);
		}

		/* case the BeforeAdding parameter */
		if (!strcmp(token, "BeforeAdding"))
		{
			data->Before_flag = 1;
		}
	}
	else if (!strcmp(token, "BeforeAdding"))
	{
		/* case the BeforeAdding parameter */
		data->Before_flag = 1;
	}
/* end by zhang */

	
/*
	tmp_size = 0;
	tmp_size = strlen(token);
	data->replace_str  = xmalloc(tmp_size);
	memset(data->replace_str,0,tmp_size);	
	data->replace_size = tmp_size;
	memcpy(data->replace_str,token,tmp_size);

*/
	debug(142, 2) ("(mod_cpis_body_modify) ->  replase_str=%s replace_size=%d \n", data->replace_str,data->replace_size);

	cc_register_mod_param(mod, data, free_callback);

	return 0;
	
}


static int func_buf_recv_from_s_body(HttpStateData * httpState, char *buf, ssize_t len, char* comp_buf, ssize_t comp_buf_len, size_t *comp_size, int buffer_filled)
{
	if(!if_reply_ok(httpState->entry->mem_obj->reply))
        {
                debug(142, 1)("(mod_cpis_body_modify) -> httpState entry url: %s, reply status: %d is not 200, so we don't process it\n", storeUrl(httpState->entry), httpState->entry->mem_obj->reply->sline.status);
                return 0;
        }

	memset(comp_buf, 0, comp_buf_len);
	store_param *str_param = cc_get_mod_private_data(STORE_ENTRY_PRIVATE_DATA, httpState->entry, mod);
	if(str_param == NULL){
		debug(142,5)("mod_cpis_body_modify, find no store_param\n");
		return 0;
	}

	if(str_param->remain_clen >= 0 )
		str_param->remain_clen = str_param->remain_clen - len;

	debug(142,5)("mod_cpis_body_modify remain_clen[%ld]\n",str_param->remain_clen);
	if(str_param->remain_clen == 0) {
		httpState->eof = 1;
	}

	mod_conf_param *cfg = cc_get_mod_param(httpState->fd, mod);
	if(cfg == NULL){
		debug(142, 1)("(mod_cpis_body_modify) -> mod conf parameter is null \n");
		return 0;
	}

	debug(142,5)("mod_cpis_body_modify str_param->chunk_or_not2: %d\n", str_param->chunk_or_not);
	*comp_size = storeModifyBody((char*)buf,len,httpState,comp_buf,comp_buf_len,cfg,str_param);

	debug(142,5)("mod_cpis_body_modify buf5[%s]  len[%d]  alloc_size[%d]\n",comp_buf,(int)(*comp_size),(int)(comp_buf_len));

	if(*comp_size == -1)
		return 0;

	return 1;

}

static int private_repl_read_start(int fd, HttpStateData *httpState)
{
	if(!if_reply_ok(httpState->entry->mem_obj->reply))
        {
                debug(142, 1)("(mod_cpis_body_modify) -> httpState entry url: %s, reply status: %d is not 200, so we don't process it\n", storeUrl(httpState->entry), httpState->entry->mem_obj->reply->sline.status);
                return 0;
        }

	httpState->entry->mem_obj->reply->content_length = -1;

	return 0;
}

static void private_repl_send_start(clientHttpRequest *http, int *ret)
{
	if(!if_reply_ok(http->reply))
        {
                debug(142, 1)("(mod_cpis_body_modify) -> httpState entry url: %s, reply status: %d is not 200, so we don't process it\n", storeUrl(http->entry), http->reply->sline.status);
                return ;
        }

	const char* transfer_encoding = httpHeaderGetStr2(&http->reply->header, HDR_TRANSFER_ENCODING);
	if(!transfer_encoding)
		httpHeaderPutStr(&http->reply->header, HDR_TRANSFER_ENCODING, "Chunked");
	http->request->flags.chunked_response = 1;
	httpHeaderDelById(&http->reply->header, HDR_CONTENT_LENGTH);

	http->reply->content_length = -1;

	*ret = 0;
}

static int private_repl_send_end(clientHttpRequest *http)
{
	if(http == NULL)
		return 0;

	if(!if_reply_ok(http->reply))
        {
                debug(142, 1)("(mod_cpis_body_modify) -> httpState entry url: %s, reply status is not 200, so we don't process it\n", storeUrl(http->entry));
                return 0;
        }

	if(http && http->reply)
		http->reply->content_length = -1;

	if(http && http->entry && http->entry->mem_obj && http->entry->mem_obj->reply)
		http->entry->mem_obj->reply->content_length= -1;	

	debug(142, 5)("(mod_cpis_body_modify) ->  change http->reply->content_length \n");

	return 0;
}

static void transfer_done_precondition(clientHttpRequest *http, int *precondition)
{
	if(http && http->request)
		*precondition = (http->request->flags.chunked_response == 0);
}


// module init 
int mod_register(cc_module *module)
{
	debug(142, 1)("(mod_cpis_body_modify) ->  init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");
	//module->hook_func_private_reply_modify_body = func_modify_buf_recv_from_s_body;
	/*
	cc_register_hook_handler(HPIDX_hook_func_private_reply_modify_body,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_reply_modify_body),
			func_modify_buf_recv_from_s_body);
	*/

	cc_register_hook_handler(HPIDX_hook_func_buf_recv_from_s_header,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_buf_recv_from_s_header),
			func_modify_buf_recv_from_s_body);


	cc_register_hook_handler(HPIDX_hook_func_private_compress_response_body,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_compress_response_body),
			func_buf_recv_from_s_body);

	//module->hook_func_sys_parse_param = func_sys_parse_param;
	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
			func_sys_parse_param);

	cc_register_hook_handler(HPIDX_hook_func_http_repl_read_end,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_repl_read_end), 
			private_repl_read_start);
	

	cc_register_hook_handler(HPIDX_hook_func_private_repl_send_start,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_repl_send_start), 
			private_repl_send_start);
	
	cc_register_hook_handler(HPIDX_hook_func_http_repl_send_end,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_repl_send_end), 
			private_repl_send_end);


	cc_register_hook_handler(HPIDX_hook_func_transfer_done_precondition,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_transfer_done_precondition),
			transfer_done_precondition);

	mod = module;
	return 0;
}

#endif
