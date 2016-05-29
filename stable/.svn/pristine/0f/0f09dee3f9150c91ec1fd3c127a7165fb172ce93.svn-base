#include "server_header.h"
#include "server_http.h"

const char* HOST_IP_HEADER = "X-CC-HOST-IP";
const char* HOST_HEADER = "Host";

char relaxed_header_parser = 0;
//static HttpHeaderFieldInfo *Headers = NULL;

void
httpHeaderAddEntry(HttpHeader * hdr, HttpHeaderEntry * e);

static HttpHeaderEntry *
httpHeaderEntryCreate(const char * name, const char * value);

int httpStatusLineParse(HttpStatusLine * sline, const char *start, const char *end)
{
	int maj, min, status;
	const char *s;

	assert(sline);
	sline->status = HTTP_INVALID_HEADER;    /* Squid header parsing error */
	if (strncasecmp(start, "HTTP/", 5))
		return 0;
	start += 5;
	if (!xisdigit(*start))
		return 0;

	/* Format: HTTP/x.x <space> <status code> <space> <reason-phrase> CRLF */
	s = start;
	maj = 0;
	for (s = start; s < end && xisdigit(*s) && maj < 65536; s++)
	{
		maj = maj * 10;
		maj = maj + *s - '0';
	}
	if (s >= end || maj >= 65536)
	{
		cclog(7, "httpStatusLineParse: Invalid HTTP reply status major.\n");
		return 0;
	}
	/* next should be '.' */
	if (*s != '.')
	{
		cclog(7, "httpStatusLineParse: Invalid HTTP reply status line.\n");
		return 0;
	}
	s++;
	/* next should be minor number */
	min = 0;
	for (; s < end && xisdigit(*s) && min < 65536; s++)
	{
		min = min * 10;
		min = min + *s - '0';
	}
	if (s >= end || min >= 65536)
	{
		cclog(7, "httpStatusLineParse: Invalid HTTP reply status version minor.\n");
		return 0;
	}
	/* then a space */
	if (*s != ' ')
	{
	}
	s++;
	/* next should be status start */
	status = 0;
	for (; s < end && xisdigit(*s); s++)
	{
		status = status * 10;
		status = status + *s - '0';
	}
	if (s >= end)
	{
		cclog(7, "httpStatusLineParse: Invalid HTTP reply status code.\n");
		return 0;
	}
	/* then a space */

	/* for now we ignore the reason-phrase */

	/* then crlf */

	sline->version.major = maj;
	sline->version.minor = min;
	sline->status = status;

	/* we ignore 'reason-phrase' */
	return 1;                       /* success */
}

/* handy to printf prefixes of potentially very long buffers */
const char *
getStringPrefix(const char *str, const char *end)
{       
#define SHORT_PREFIX_SIZE 512
        static char buf[SHORT_PREFIX_SIZE];
        const int sz = 1 + (end ? end - str : strlen(str));
        xstrncpy(buf, str, (sz > SHORT_PREFIX_SIZE) ? SHORT_PREFIX_SIZE : sz);
        return buf;
}       

/* returns next valid entry */
HttpHeaderEntry *
httpHeaderGetEntry(const HttpHeader * hdr, HttpHeaderPos * pos)
{
        assert(hdr && pos);
        assert(*pos >= HttpHeaderInitPos && *pos < hdr->entries.count);
        for ((*pos)++; *pos < hdr->entries.count; (*pos)++)
        {
                if (hdr->entries.items[*pos])
                        return hdr->entries.items[*pos];
        }
        return NULL;
}

static void
httpHeaderEntryDestroy(HttpHeaderEntry * e)
{
        assert(e);
        cclog(9, "destroying entry %p: '%s: %s'\n", e, strBuf(e->name), strBuf(e->value));
        /* clean name if needed */
        stringClean(&e->name);
        stringClean(&e->value);
        cc_free(e);
}

/*
 * HttpHeader Implementation
 */

void
httpHeaderInit(HttpHeader * hdr, http_hdr_owner_type owner)
{
        assert(hdr);
        assert(owner > hoNone && owner <= hoReply);
        cclog(7, "init-ing hdr: %p owner: %d\n", hdr, owner);
        memset(hdr, 0, sizeof(*hdr));
        hdr->owner = owner;
        arrayInit(&hdr->entries);
}


void
httpHeaderClean(HttpHeader * hdr)
{
        HttpHeaderPos pos = HttpHeaderInitPos;
        HttpHeaderEntry *e;

        assert(hdr);
        assert(hdr->owner > hoNone && hdr->owner <= hoReply);
        cclog(7, "cleaning hdr: %p owner: %d\n", hdr, hdr->owner);

	while ((e = httpHeaderGetEntry(hdr, &pos)))
	{
		/* yes, this destroy() leaves us in an inconsistent state */
		httpHeaderEntryDestroy(e);
	}
        arrayClean(&hdr->entries);
}

/* just handy in parsing: resets and returns false */
int
httpHeaderReset(HttpHeader * hdr)
{
        http_hdr_owner_type ho;
        assert(hdr);
        ho = hdr->owner;
        httpHeaderClean(hdr);
        httpHeaderInit(hdr, ho);
        return 0;
}

/*
int
httpHeaderIdByName(const char *name, int name_len, const HttpHeaderFieldInfo * info, int end)
{
        int i;
        for (i = 0; i < end; ++i)
        {
                if (name_len >= 0 && name_len != strLen(info[i].name))
                        continue;
                if (!strncasecmp(name, strBuf(info[i].name),
                                                 name_len < 0 ? strLen(info[i].name) + 1 : name_len))
                        return i;
        }
        return -1;
}
*/

// 1: in, 0: not
int entry_in_list(char list[][MAX_HEADER_LEN], HttpHeaderEntry *e)
{
	int i = 0;
	while(*list[i])
	{
		if(strcasestr(list[i], strBuf(e->name)))
			return 1;
		i++;
	}

	return 0;
}

//if need post to ca? if need but the reply has no Content-Length or Transfer-Encoding header, we add Content-Length: 99999999
static int need_post_to_ca(HttpHeader *hdr)
{
	assert(hdr && (hdr->owner == hoReply));
	assert(hdr->data);

	HttpHeaderEntry *e;
	fd_struct *fd_s;
	int i = 0;

	fd_s = hdr->data;
	assert(fd_s->type == S_2_WEB);

	//reply status code is fit ?
	for(i = 0; i < rep_status_num; i++)
	{
		if(rep_status_list[i] == hdr->sline.status)
		{
			cclog(5, "rep status code is %d, need not POST to CA", hdr->sline.status);
			fd_s->need_post = NOT_POST;
        		return 0;
		}
	}

	HttpHeaderPos pos = HttpHeaderInitPos;

	/*
	while ((e = httpHeaderGetEntry(hdr, &pos)))
	{
		i = 0;
		while(*content_type_list[i])
		{
			if(strstr(strBuf(e->name), CONTENT_TYPE))
			{
				cclog(5, "find Content-Type header: %s: %s", strBuf(e->name), strBuf(e->value));
				
				if(strstr(strBuf(e->value), content_type_list[i]))
				{
					cclog(5, "Hit content type list");
					fd_s->need_post = POST_DEF;
					return 1;
				}
			}

			i++;
		}
	}
	*/
	
	int find_content_type = 0;
	int find_content_length = 0;
	int hit_content_type = 0;
//	int find_transfer_encoding = 0;
	int ret = 0;

	while ((e = httpHeaderGetEntry(hdr, &pos)))
	{
		if(strcasestr(strBuf(e->name), "Content-Type"))
		{       
			cclog(5, "find Content-Type header: %s: %s", strBuf(e->name), strBuf(e->value));
			find_content_type = 1;

			i = 0;
			while(*content_type_list[i])
			{
				if(strcasestr(strBuf(e->value), content_type_list[i]))
				{
					cclog(5, "Hit content type list...");
					fd_s->need_post = POST_DEF;
					hit_content_type = 1;
					ret = 1;
				}

				i++;
			}
		}
		else if(strcasestr(strBuf(e->name), "Content-Length"))
		{
			find_content_length = 1;
		}
		/*
		else if(strcasestr(strBuf(e->name), "Transfer-Encoding"))
		{
			find_transfer_encoding = 1;
		}
		*/
	}

	if(!hit_content_type)
	{
		fd_s->need_post = NOT_POST;
		ret = 0;
		cclog(5, "Miss content type list...");
	}

	//need add Content-Length ?
	//if((fd_s->need_post == POST_DEF) && (find_content_length == 0) && (find_transfer_encoding == 0))
	if((fd_s->need_post == POST_DEF) && (find_content_length == 0))
	{
		cclog(4, "not find Content-Length reply header, add it for CA");
		httpHeaderAddEntry(hdr, httpHeaderEntryCreate("Content-Length", "999999999"));
	}

	return ret;
}
				


//parse weather the entry should be add to header
static HttpHeaderEntry *
httpHeaderEntryParse(const HttpHeader *hdr, HttpHeaderEntry *e)
{
	assert(hdr && (hdr->owner != hoNone) && e);

	if(hdr->owner == hoRequest)
	{
		if(entry_in_list(req_list, e))
			return e;
		else
			return NULL;
	}
	else if(hdr->owner == hoReply)
	{
		if(entry_in_list(rep_list, e))
			return NULL;
		else
			return e;
	}
	else
	{
		cclog(1, "Unknown hdr owner type: %d", hdr->owner);
		return NULL;
	}
}
	

/* parses and inits header entry, returns new entry on success */
static HttpHeaderEntry *
httpHeaderEntryParseCreate(const char *field_start, const char *field_end)
{
        HttpHeaderEntry *e;
        /* note: name_start == field_start */
        const char *name_end = memchr(field_start, ':', field_end - field_start);
        int name_len = name_end ? name_end - field_start : 0;
        const char *value_start = field_start + name_len + 1;   /* skip ':' */
        /* note: value_end == field_end */

        /* do we have a valid field name within this field? */
        if (!name_len || name_end > field_end)
                return NULL;
        if (name_len > 65534)
        {
                /* String must be LESS THAN 64K and it adds a terminating NULL */
                cclog(1,"WARNING: ignoring header name of %d bytes\n", name_len);
                return NULL;
        }
        if (relaxed_header_parser && xisspace(field_start[name_len - 1]))
        {
                cclog(1, "NOTICE: Whitespace after header name in '%s'\n", getStringPrefix(field_start, field_end));
                while (name_len > 0 && xisspace(field_start[name_len - 1]))
                        name_len--;
		if (!name_len)
                        return NULL;
        }
        /* now we know we can parse it */
        e = cc_malloc(sizeof(HttpHeaderEntry));
        cclog(9, "creating entry %p: near '%s'\n", e, getStringPrefix(field_start, field_end));
//        /* is it a "known" field? */
//        id = httpHeaderIdByName(field_start, name_len, Headers, HDR_ENUM_END);
//        if (id < 0)
//                id = HDR_OTHER;
//        assert_eid(id);
//        e->id = id;
        /* set field name */
//        if (id == HDR_OTHER)
                stringLimitInit(&e->name, field_start, name_len);
//        else
        /* trim field value */
        while (value_start < field_end && xisspace(*value_start))
                value_start++;
        while (value_start < field_end && xisspace(field_end[-1]))
                field_end--;
        if (field_end - value_start > 65534)
        {
                /* String must be LESS THAN 64K and it adds a terminating NULL */
                cclog(1, "WARNING: ignoring '%s' header of %d bytes\n",
                                          strBuf(e->name), (int) (field_end - value_start));
//                if (e->id == HDR_OTHER)
			stringClean(&e->name);
                cc_free(e);
                return NULL;
        }
        /* set field value */
        stringLimitInit(&e->value, value_start, field_end - value_start);
        cclog(9, "created entry %p: '%s: %s'\n", e, strBuf(e->name), strBuf(e->value));
        return e;
}


static HttpHeaderEntry *
httpHeaderEntryCreate(const char * name, const char * value)
{
	HttpHeaderEntry * e;
        e = cc_malloc(sizeof(HttpHeaderEntry));

	stringLimitInit(&e->name, name, strlen(name));
	stringLimitInit(&e->value, value, strlen(value));

        cclog(9, "created entry %p: '%s: %s'\n", e, strBuf(e->name), strBuf(e->value));
	return e;
}
/* appends an entry;
 * does not call httpHeaderEntryClone() so one should not reuse "*e"
 */
void
httpHeaderAddEntry(HttpHeader * hdr, HttpHeaderEntry * e)
{
        assert(hdr && e);

        cclog(7, "%p adding entry: %d",
                                  hdr, hdr->entries.count);

	arrayAppend(&hdr->entries, e);
        /* increment header length, allow for ": " and crlf */
        hdr->len += strLen(e->name) + 2 + strLen(e->value) + 2;
}

void packerInit(Packer *p, uint32_t size)
{
	p->buf = NULL;
	p->offset = 0;
	p->size = size;
}

void packerAppend(Packer * p, const char *buf, int len)
{
	assert(p->buf);

	if(p->offset + len >= p->size -1)
	{
		cclog(1, "No space left to pack buf: %s", buf);
		p->full = 1;
		return;
	}

	memcpy(p->buf + p->offset, buf, len);
	p->offset += len;

	return;
}
void
httpHeaderEntryPackInto(const HttpHeaderEntry * e, Packer * p)
{
        assert(e && p);
        packerAppend(p, strBuf(e->name), strLen(e->name));
        packerAppend(p, ": ", 2);
        packerAppend(p, strBuf(e->value), strLen(e->value));
        packerAppend(p, "\r\n", 2);
}

/* packs all the entries using supplied packer */
void
httpHeaderPackInto(const HttpHeader * hdr, Packer * p)
{
        HttpHeaderPos pos = HttpHeaderInitPos;
        const HttpHeaderEntry *e;
        assert(hdr && p);
        cclog(7, "packing hdr: (%p)\n", hdr);
        /* pack all entries one by one */
        while ((e = httpHeaderGetEntry(hdr, &pos)))
                httpHeaderEntryPackInto(e, p);
}

/* packs request-line and headers, appends <crlf> terminator */
static void
httpRequestPack(const HttpHeader *req_hdr, const HttpHeader *rep_hdr, Packer * p)
{
	int len;
        assert(req_hdr && rep_hdr && p);
        /* pack request-line */
        //len = sprintf(p->buf + p->offset, "%s %s HTTP/%d.%d\r\n",
        //                       "POST", strBuf(req_hdr->uri), req_hdr->http_ver.major, req_hdr->http_ver.minor);

	len = sprintf(p->buf + p->offset, "POST /ContentAdapation HTTP/1.1\r\n");

	if(len < 0)
	{
		cclog(1, "pack request-line failed");
		p->full = 1;
		return;
	}

	p->offset += len;
        /* headers */
        httpHeaderPackInto(req_hdr, p);
        httpHeaderPackInto(rep_hdr, p);
        /* trailer */
        //packerAppend(p, "\r\n", 2);
}

//remove X-CC-HOST-IP line from buffer
// OK: success; ERR: failed
static int process_req_header_buf(HttpHeader *req_hdr)
{
	fd_struct *fd_s = req_hdr->data;
	char *buf = fd_s->read_buff;

	cclog(6, "in remove HOST_IP_HEADER func, buff: %s, buff_len: %d", getStringPrefix(buf, buf + fd_s->read_buff_pos), fd_s->read_buff_pos);

	cclog(8, "fd_s type: %d", fd_s->type);

	assert(fd_s->type == FC_2_S);

	char *start;
	char *end;
	
	if((start = strcasestr(buf, HOST_IP_HEADER)) == NULL)
	{
		cclog(1, "can not find HOST_IP_HEAER in req_buf: %s", getStringPrefix(buf, buf + fd_s->read_buff_pos));
		return ERR;
	}

	if((end = strstr(start, "\r\n")) == NULL)
	{
		cclog(1, "can not find HOST_IP_HEAER line end in req_buf: %s", getStringPrefix(buf, buf + fd_s->read_buff_pos));
		return ERR;
	}

	end += 2;
	xmemmove(start, end, buf + fd_s->read_buff_pos - end);

	fd_s->read_buff_pos -= (end - start);

	cclog(6, "aftern remove HOST_IP_HEADER buff: %s, buff_len: %d", getStringPrefix(buf, buf + fd_s->read_buff_pos), fd_s->read_buff_pos);
	return OK;
}

//success: 1; failed: 0;
int httpHeaderParse(HttpHeader * hdr, const char *header_start, const char *header_end)
{
	const char *field_ptr = header_start;
	char buf[4096];
	HttpHeaderEntry *e;

	int find_host_ip = 0;
	int find_host = 0;

	assert(hdr);
	assert(header_start && header_end);
	cclog(7, "parsing hdr: (%p)\n%s\n", hdr, getStringPrefix(header_start, header_end));
	if (memchr(header_start, '\0', header_end - header_start))
	{
		cclog(1, "WARNING: HTTP header contains NULL characters {%s}\n",
				getStringPrefix(header_start, header_end));
		return httpHeaderReset(hdr);
	}
	/* common format headers are "<name>:[ws]<value>" lines delimited by <CRLF>.
	 * continuation lines start with a (single) space or tab */
	while (field_ptr < header_end)
	{
		const char *field_start = field_ptr;
		const char *field_end;
		do
		{
			const char *this_line = field_ptr;
			field_ptr = memchr(field_ptr, '\n', header_end - field_ptr);
			if (!field_ptr)
				return httpHeaderReset(hdr);    /* missing <LF> */
			field_end = field_ptr;
			field_ptr++;    /* Move to next line */
			if (field_end > this_line && field_end[-1] == '\r')
			{
				field_end--;    /* Ignore CR LF */
				/* Ignore CR CR LF in relaxed mode */
				if (relaxed_header_parser && field_end > this_line + 1 && field_end[-1] == '\r')
				{
					cclog(1, "WARNING: Double CR characters in HTTP header {%s}\n", getStringPrefix(field_start, field_end));
					field_end--;
				}
			}
			/* Barf on stray CR characters */
			if (memchr(this_line, '\r', field_end - this_line))
			{
				cclog(1, "WARNING: suspicious CR characters in HTTP header {%s}\n",
						getStringPrefix(field_start, field_end));
				if (relaxed_header_parser)
				{
					char *p = (char *) this_line;   /* XXX Warning! This destroys original header content and vi
									   olates specifications somewhat */
					while ((p = memchr(p, '\r', field_end - p)) != NULL)
						*p++ = ' ';
				}
				else
					return httpHeaderReset(hdr);
			}
			if (this_line + 1 == field_end && this_line > field_start)
			{
				cclog(1, "WARNING: Blank continuation line in HTTP header {%s}\n",
						getStringPrefix(header_start, header_end));
				return httpHeaderReset(hdr);
			}
		}
		while (field_ptr < header_end && (*field_ptr == ' ' || *field_ptr == '\t'));
		if (field_start == field_end)
		{
			if (field_ptr < header_end)
			{
				cclog(1, "WARNING: unparseable HTTP header field near {%s}\n",
						getStringPrefix(field_start, header_end));
				return httpHeaderReset(hdr);
			}
			break;          /* terminating blank line */
		}
		e = httpHeaderEntryParseCreate(field_start, field_end);
		if (NULL == e)
		{
			cclog(1, "WARNING: unparseable HTTP header field {%s}\n",
					getStringPrefix(field_start, field_end));
			cclog(1, " in {%s}\n", getStringPrefix(header_start, header_end));
			if (relaxed_header_parser)
				continue;
			else
				return httpHeaderReset(hdr);
		}

		//web server host ip header?
		if(hdr->owner == hoRequest)
		{
			if(!find_host_ip)
			{
				if(!strcmp(HOST_IP_HEADER, strBuf(e->name)))
				{
					cclog(3, "get host: [%s] ----> ip: [%s]", strBuf(e->name), strBuf(e->value));
					hdr->ip = stringDup(&e->value);
					find_host_ip = 1;
				}
			}

			//request host ?
			if(!find_host)
			{
				if(strcasestr(strBuf(e->name), HOST_HEADER))
				{
					find_host = 1;

					if(strstr(strBuf(hdr->uri), "http://") == NULL)
					{
						cclog(3, "get request host ----> [%s]: [%s]", strBuf(e->name), strBuf(e->value));
						strcpy(buf, "http://");
						strcat(buf, strBuf(e->value));
						strcat(buf, strBuf(hdr->uri));

						stringInit(&hdr->url, buf);
					}
					else
					{
						hdr->url = stringDup(&hdr->uri);
					}

					cclog(3, "request url is: %s", strBuf(hdr->url));


					//change host to Host: ca_ip:ca_port
					//stringClean(&e->value);
					//stringInit(&e->value, "221.130.18.220:18000");
				}
			}
		}

		//add entry to header if need
		//httpHeaderAddEntry(hdr, e);
		if(httpHeaderEntryParse(hdr, e))
		{
			cclog(8, "add entry:[%s: %s] to header: %p type: %d", strBuf(e->name), strBuf(e->value), hdr, hdr->owner);
			httpHeaderAddEntry(hdr, e);
		}
		else
		{
			cclog(8, "entry:[%s: %s] do not need add to header: %p type: %d", strBuf(e->name), strBuf(e->value), hdr, hdr->owner);
			httpHeaderEntryDestroy(e);
		}
	}

	return 1;                       /* even if no fields where found, it is a valid header */
}




//success 0; failed !0;
int                     
httpMsgParseRequestHeader(HttpHeader * header, HttpMsgBuf * hmsg)
{                       
        const char *s, *e;
        s = hmsg->buf + hmsg->h_start;
        e = hmsg->buf + hmsg->h_end + 1;

        if(httpHeaderParse(header, s, e))
	{

		if(process_req_header_buf(header) == OK)
		{
			//add ca private header -- client's ori request url
			httpHeaderAddEntry(header, httpHeaderEntryCreate("x-ca-original-url", strBuf(header->url)));
			header->header_ok = 1;
			return OK;
		}
	}

	return ERR;
}


/* Adrian's replacement message buffer code to parse the request/reply line */
                
void            
HttpMsgBufInit(HttpMsgBuf * hmsg, const char *buf, size_t size)
{       
        hmsg->buf = buf;
        hmsg->size = size;
        hmsg->req_start = hmsg->req_end = -1;
        hmsg->h_start = hmsg->h_end = -1;
        hmsg->r_len = hmsg->u_len = hmsg->m_len = hmsg->v_len = hmsg->h_len = 0;
}       

/*
 * Attempt to parse the request line.
 *
 * This will set the values in hmsg that it determines. One may end up
 * with a partially-parsed buffer; the return value tells you whether
 * the values are valid or not.
 *
 * @return 1 if parsed correctly, 0 if more is needed, -1 if error
 *
 * TODO:
 *   * have it indicate "error" and "not enough" as two separate conditions!
 *   * audit this code as off-by-one errors are probably everywhere!
 */
int     
httpMsgParseRequestLine(HttpMsgBuf * hmsg)
{
        int i = 0;
        int retcode;
        int maj = -1, min = -1;
        int last_whitespace = -1, line_end = -1;
        const char *t;
        
        /* Find \r\n - end of URL+Version (and the request) */
        t = memchr(hmsg->buf, '\n', hmsg->size);
        if (!t) 
        {
                retcode = 0;
                goto finish; 
        }
        /* XXX this should point to the -end- of the \r\n, \n, etc. */
        hmsg->req_end = t - hmsg->buf;
        i = 0;

        /* Find first non-whitespace - beginning of method */
        for (; i < hmsg->req_end && (xisspace(hmsg->buf[i])); i++);
        if (i >= hmsg->req_end)
        {
                retcode = 0;
                goto finish;
        }
        hmsg->m_start = i;
	hmsg->req_start = i;
        hmsg->r_len = hmsg->req_end - hmsg->req_start + 1;

        /* Find first whitespace - end of method */
        for (; i < hmsg->req_end && (!xisspace(hmsg->buf[i])); i++);
        if (i >= hmsg->req_end)
        {
                retcode = -1;
                goto finish;
        }
        hmsg->m_end = i - 1;
        hmsg->m_len = hmsg->m_end - hmsg->m_start + 1;

        /* Find first non-whitespace - beginning of URL+Version */
        for (; i < hmsg->req_end && (xisspace(hmsg->buf[i])); i++);
        if (i >= hmsg->req_end)
        {
                retcode = -1;
                goto finish;
        }
        hmsg->u_start = i;

        /* Find \r\n or \n - thats the end of the line. Keep track of the last whitespace! */
        for (; i <= hmsg->req_end; i++)
        {
                /* If \n - its end of line */
		if (hmsg->buf[i] == '\n')
                {
                        line_end = i;
                        break;
                }
                /* we know for sure that there is at least a \n following.. */
                if (hmsg->buf[i] == '\r' && hmsg->buf[i + 1] == '\n')
                {
                        line_end = i;
                        break;
                }
                /* If its a whitespace, note it as it'll delimit our version */
                if (hmsg->buf[i] == ' ' || hmsg->buf[i] == '\t')
                {
                        last_whitespace = i;
                }
        }
        if (i > hmsg->req_end)
        {
                retcode = -1;
                goto finish;
        }
        /* At this point we don't need the 'i' value; so we'll recycle it for version parsing */

        /*
         * At this point: line_end points to the first eol char (\r or \n);
         * last_whitespace points to the last whitespace char in the URL.
         * We know we have a full buffer here!
	*/
        if (last_whitespace == -1)
        {
                maj = 0;
                min = 9;
                hmsg->u_end = line_end - 1;
                assert(hmsg->u_end >= hmsg->u_start);
        }
        else
        {
                /* Find the first non-whitespace after last_whitespace */
                /* XXX why <= vs < ? I do need to really re-audit all of this .. */
                for (i = last_whitespace; i <= hmsg->req_end && xisspace(hmsg->buf[i]); i++);
                if (i > hmsg->req_end)
                {
                        retcode = -1;
                        goto finish;
                }
                /* is it http/ ? if so, we try parsing. If not, the URL is the whole line; version is 0.9 */
                if (i + 5 >= hmsg->req_end || (strncasecmp(&hmsg->buf[i], "HTTP/", 5) != 0))
                {
                        maj = 0;
                        min = 9;
                        hmsg->u_end = line_end - 1;
                        assert(hmsg->u_end >= hmsg->u_start);
                }
                else
                {
                        /* Ok, lets try parsing! Yes, this needs refactoring! */
			hmsg->v_start = i;
                        i += 5;

                        /* next should be 1 or more digits */
                        maj = 0;
                        for (; i < hmsg->req_end && (xisdigit(hmsg->buf[i])) && maj < 65536; i++)
                        {
                                maj = maj * 10;
                                maj = maj + (hmsg->buf[i]) - '0';
                        }
                        if (i >= hmsg->req_end || maj >= 65536)
                        {
                                retcode = -1;
                                goto finish;
                        }
                        /* next should be .; we -have- to have this as we have a whole line.. */
                        if (hmsg->buf[i] != '.')
                        {
                                retcode = 0;
                                goto finish;
                        }
                        if (i + 1 >= hmsg->req_end)
                        {
                                retcode = -1;
                                goto finish;
                        }
                        /* next should be one or more digits */
                        i++;
			min = 0;
                        for (; i < hmsg->req_end && (xisdigit(hmsg->buf[i])) && min < 65536; i++)
                        {
                                min = min * 10;
                                min = min + (hmsg->buf[i]) - '0';
                        }

                        if (min >= 65536)
                        {
                                retcode = -1;
                                goto finish;
                        }

                        /* Find whitespace, end of version */
                        hmsg->v_end = i;
                        hmsg->v_len = hmsg->v_end - hmsg->v_start + 1;
                        hmsg->u_end = last_whitespace - 1;
                }
        }
        hmsg->u_len = hmsg->u_end - hmsg->u_start + 1;

        /*
         * Rightio - we have all the schtuff. Return true; we've got enough.
         */
        retcode = 1;
        assert(maj != -1);
        assert(min != -1);
finish:
	hmsg->v_maj = maj;
        hmsg->v_min = min;
        cclog(2, "Parser: retval %d: from %d->%d: method %d->%d; url %d->%d; version %d->%d (%d/%d)\n",
                                 retcode, hmsg->req_start, hmsg->req_end,
                                 hmsg->m_start, hmsg->m_end,
                                 hmsg->u_start, hmsg->u_end,
                                 hmsg->v_start, hmsg->v_end, maj, min);
        return retcode;
}

/*              
 * A temporary replacement for headersEnd() in this codebase.
 * This routine searches for the end of the headers in a HTTP request
 * (obviously anything > HTTP/0.9.)
 *              
 * It returns buffer length on success or 0 on failure.
 */     
int     
httpMsgFindHeadersEnd(HttpMsgBuf * hmsg)
{       
        int e = 0;
        int state = 1;
        const char *mime = hmsg->buf;
        int l = hmsg->size;
        int he = -1;
                
        /* Always succeed HTTP/0.9 - it means we've already parsed the buffer for the request */
        if (hmsg->v_maj == 0 && hmsg->v_min == 9)
                return 1;
                        
        while (e < l && state < 3)
        {
                switch (state)
                {
                case 0:
                        if ('\n' == mime[e])
                        {
                                he = e;
				state = 1;
                        }
                        break;
                case 1:
                        if ('\r' == mime[e])
                                state = 2;
                        else if ('\n' == mime[e])
                                state = 3;
                        else
                                state = 0;
                        break;
                case 2:
                        if ('\n' == mime[e])
                                state = 3;
                        else
                                state = 0;
                        break;
                default:
                        break;
                }
                e++;
        }
        if (3 == state)
        {
                hmsg->h_end = he;
                hmsg->h_start = hmsg->req_end + 1;

		hmsg->h_len = hmsg->h_end - hmsg->h_start;
                return e;
        }
        return 0;

}
                              
void    
httpBuildVersion(http_version_t * version, unsigned int major, unsigned int minor)
{       
        version->major = major;
        version->minor = minor;
}               

method_t
urlParseMethod(const char *s, int len)
{
        method_t method = METHOD_NONE;
        /*
         * This check for '%' makes sure that we don't
         * match one of the extension method placeholders,
         * which have the form %EXT[0-9][0-9]
         */
        if (*s == '%')
                return METHOD_NONE;
        for (method++; method < METHOD_ENUM_END; method++)
        {
                if (len == RequestMethods[method].len && 0 == strncasecmp(s, RequestMethods[method].str, len))
                        return method;
        }       
        return METHOD_NONE;
}

static int
httpReplyParseStep(HttpHeader * rep, const char *buf, int len);

/*
 * httpReplyParse takes character buffer of HTTP headers (buf),
 * which may not be NULL-terminated, and fills in an HttpReply
 * structure (rep).  The parameter 'end' specifies the offset to
 * the end of the reply headers.  The caller may know where the
 * end is, but is unable to NULL-terminate the buffer.  This function
 * returns true on success.
 */
int
httpReplyParse(HttpHeader* rep, const char *buf, size_t end)
{
        /* The code called by httpReplyParseStep doesn't assume NUL-terminated stuff! */
        return (httpReplyParseStep(rep, buf, end) == 1);
}

size_t  
headersEnd(const char *mime, size_t l)
{       
        size_t e = 0;
        int state = 1;
        while (e < l && state < 3)
        {       
                switch (state)
                {
                case 0:
                        if ('\n' == mime[e])
                                state = 1;
                        break;
                case 1:
                        if ('\r' == mime[e])
                                state = 2;
                        else if ('\n' == mime[e])
                                state = 3;
                        else
                                state = 0;
                        break;
                case 2:
                        if ('\n' == mime[e])
                                state = 3;
                        else
                                state = 0;
                        break;
                default:
                        break;
		 }
                e++;
        }
        if (3 == state)
                return e;
        return 0;
}

static int parseHttpReply(HttpHeader * header, char *in_buf, int len)
{
	static char buf[MAX_HEADER_SIZE];
	int hdr_size;

	if(len >= MAX_HEADER_SIZE - 1)
	{
		cclog(1, "reply header len too large!");
		return ERR;
	}

	memset(buf, 0, sizeof(buf));
	memcpy(buf, in_buf, len);
	hdr_size = headersEnd(buf, len);

	if(!hdr_size)
	{
                cclog(5, "Incomplete reply, waiting for end of headers\n");
		return OK;
	}

	header->size = hdr_size;
	buf[hdr_size] = '\0';

	cclog(5, "reply header size: %d\n", hdr_size);
	cclog(9, "GOT HTTP REPLY HDR:\n---------\n%s\n----------\n", buf);

	return httpReplyParse(header, buf, hdr_size)?OK:ERR;

}

//
static int parseHttpRequest(HttpHeader * header, char *in_buf, int len)
{
	static char buf[MAX_HEADER_SIZE];
	static char urlbuf[MAX_URL];

	if(len >= MAX_HEADER_SIZE)
	{
		cclog(1, "request header len too large!");
		return ERR;
	}

	memset(buf, 0, sizeof(buf));
	memcpy(buf, in_buf, len);

	http_version_t *http_ver = &header->http_ver;
        size_t header_sz;               /* size of headers, not including first line */
        size_t prefix_sz;               /* size of whole request (req-line + headers) */
        size_t req_sz; 
        method_t method;
//	char *t;
        int ret;

	HttpMsgBuf msg;
	HttpMsgBuf * hmsg;

	hmsg = &msg;

	int bb;

	method_t * method_p = &header->method;
	 int *status = &bb;


	/* pre-set these values to make aborting simpler */
        *method_p = METHOD_NONE;
        *status = -1;

	const char *req_hdr = NULL;

	int in_offset = len;

	/* Skip leading (and trailing) whitespace */
        while (in_offset > 0 && xisspace(buf[0]))
        {
                xmemmove(buf, buf + 1, in_offset - 1);
                in_offset--;
        }
        buf[in_offset] = '\0';   /* Terminate the string */

	//re_read
        if (in_offset == 0)
                return 0;

        HttpMsgBufInit(&msg, buf, in_offset);    /* XXX for now there's no deallocation function needed but this may 
change */

	/* Parse the request line */
        ret = httpMsgParseRequestLine(&msg);
        if (ret == -1)
	{
		cclog(1, "parse request line failed");
		return ERR;
	}
        if (ret == 0)
        {
                cclog(5, "Incomplete request, waiting for end of request line");
                *status = 0;
                return OK;
        }

	if (hmsg->v_maj == 0 && hmsg->v_min == 9)
        {
                req_sz = hmsg->r_len;
        }
        else
        {
                req_sz = httpMsgFindHeadersEnd(hmsg);
                if (req_sz == 0)
                {
                        cclog(5, "Incomplete request, waiting for end of headers\n");
                        *status = 0;
                        return 0;
                }
        }
        /* Set version */
        httpBuildVersion(http_ver, hmsg->v_maj, hmsg->v_min);

	/* Look for request method */
        method = urlParseMethod(hmsg->buf + hmsg->m_start, hmsg->m_len);

        if (method != METHOD_GET)
        {
                cclog(1, "parseHttpRequest: Unsupported method '%.*s'\n", hmsg->m_len, hmsg->buf + hmsg->m_start);
                return ERR;
        }
	
	*method_p = method; 

	if (hmsg->u_len >= MAX_URL)
        {
                cclog(1, "parseHttpRequest: URL too big (%d) chars: %s\n", hmsg->u_len, hmsg->buf + hmsg->u_start);
                return ERR;
        }
        xmemcpy(urlbuf, hmsg->buf + hmsg->u_start, hmsg->u_len);
        /* XXX off-by-one termination error? */
        urlbuf[hmsg->u_len] = '\0';
        cclog(5, "parseHttpRequest: URI is '%s'\n", urlbuf);

	stringInit(&header->uri, urlbuf);
        
        /*
         * Process headers after request line
         * XXX at this point we really should just parse the damned headers rather than doing
         * it later, allowing us to then do the URL acceleration stuff withuot too much hackery.
         */
        /* XXX re-evaluate all of these values and use whats in hmsg instead! */
        req_hdr = hmsg->buf + hmsg->r_len;
        header_sz = hmsg->h_len;
        cclog(3, "parseHttpRequest: req_hdr = {%s}\n", req_hdr);

        prefix_sz = req_sz;
        cclog(3, "parseHttpRequest: prefix_sz = %d, req_line_sz = %d\n",
                                  (int) prefix_sz, (int) hmsg->r_len);
        assert(prefix_sz <= in_offset);	

	return httpMsgParseRequestHeader(header, &msg);
}

///////////////////////////////////////////////////////////////

/* find end of headers */
int             
httpMsgIsolateHeaders(const char **parse_start, int l, const char **blk_start, const char **blk_end)
{       
        /* 
         * parse_start points to the first line of HTTP message *headers*,
         * not including the request or status lines
         */
        int end = headersEnd(*parse_start, l);
        int nnl;
        if (end)
        {
                *blk_start = *parse_start;
                *blk_end = *parse_start + end - 1;
                /*
                 * leave blk_end pointing to the first character after the
                 * first newline which terminates the headers
                 */
                assert(**blk_end == '\n');
                while (*(*blk_end - 1) == '\r')
                        (*blk_end)--;
                assert(*(*blk_end - 1) == '\n');
                *parse_start += end;
                return 1;
        }
	/*
         * If we didn't find the end of headers, and parse_start does
         * NOT point to a CR or NL character, then return failure
         */
        if (**parse_start != '\r' && **parse_start != '\n')
                return 0;               /* failure */
        /*
         * If we didn't find the end of headers, and parse_start does point
         * to an empty line, then we have empty headers.  Skip all CR and
         * NL characters up to the first NL.  Leave parse_start pointing at
         * the first character after the first NL.
         */
        *blk_start = *parse_start;
        *blk_end = *blk_start;
        for (nnl = 0; nnl == 0; (*parse_start)++)
        {
                if (**parse_start == '\r')
                        (void) 0;
                else if (**parse_start == '\n')
                        nnl++;
                else
                        break;
        }
        return 1;
}

//change method GET to POST and repack the necessary headers from request of FC and reply of web server 
int process_rep_header_buf(HttpHeader *hdr)
{
	cclog(5, "in process_rep_header_buf: %p", hdr);

	static char buffer[MAX_BUFF_SIZE];
	static char tmp_buffer[MAX_BUFF_SIZE];
	static char ip_port_buf[64];
	Packer p;
	HttpHeader *req;
	int req_fd;		//FC_2_S
	int ori_header_len;

	struct sockaddr_in  maddress;
        memset(&maddress, 0, sizeof(struct sockaddr_in));

	fd_struct *fd_s = hdr->data;
	session_fds *fds = fd_s->data;

	req_fd = fds->fc_2_s_fd;	
	fd_struct *req_fd_s = &fd_table[req_fd];

	if(!req_fd_s->inuse)
	{
		cclog(1, "req fd unused...");
		return ERR;
	}

	req = &req_fd_s->header;

	memset(&p, 0, sizeof(p));
	memset(buffer, 0, sizeof(buffer));

	p.buf = buffer;
	p.size = sizeof(buffer);
	

	httpRequestPack(req, hdr, &p);

	//get ca ip and add Host: ca_ip:ca_port to request header
	if(get_ca_ip(config.ca_host, ca_host_type,strBuf(req_fd_s->header.uri), &maddress.sin_addr) != 0){
		cclog(2,"ca host -> %s is invalid!!(%s), connect to ca failed",config.ca_host,strerror(errno));
		return ERR;
	}

	req_fd_s->ca_addr = maddress.sin_addr;
	sprintf(ip_port_buf, "Host: %s:%d\r\n", inet_ntoa(req_fd_s->ca_addr), config.ca_port);
	cclog(5, "ca Host -> [%s]", ip_port_buf);

	packerAppend(&p, ip_port_buf, strlen(ip_port_buf));
	packerAppend(&p, "\r\n", 2);

	if(p.full)
	{
		cclog(1, "header too large to pack...");
		return ERR;
	}

	cclog(6, "after pack ------------------------>\n""%s\n", p.buf);

	//find web server reply's header end

	ori_header_len = headersEnd(fd_s->read_buff, fd_s->read_buff_pos);
	assert(ori_header_len);

	cclog(6, "reply header len: %d, packer header len: %d", ori_header_len, p.offset);

	if(fd_s->read_buff_pos - ori_header_len + p.offset > fd_s->read_buff_size)
	{
		cclog(1, "read_buff_size is not large enough to place the new request header");
		return ERR;
	}

	memcpy(tmp_buffer, fd_s->read_buff + ori_header_len, fd_s->read_buff_pos - ori_header_len);
	memcpy(fd_s->read_buff, p.buf, p.offset);
	memcpy(fd_s->read_buff + p.offset, tmp_buffer, fd_s->read_buff_pos - ori_header_len);

	fd_s->read_buff_pos = p.offset + fd_s->read_buff_pos - ori_header_len;

	cclog(6, "after repack all headers, we got new header len: %d of: \n%s", 
					fd_s->read_buff_pos, getStringPrefix(fd_s->read_buff, fd_s->read_buff + p.offset));


	return OK;
}
/*
 * parses a 0-terminating buffer into HttpReply.
 * Returns:
 *      +1 -- success
 *       0 -- need more data (partial parse)
 *      -1 -- parse error
 */
static int
httpReplyParseStep(HttpHeader * rep, const char *buf, int len)
{
        const char *parse_start = buf;
        const char *blk_start, *blk_end;
        int i;
        const char *re;
        assert(rep);

        /* For now we'll assume we need to parse the whole lot */

        /* Find end of start line */
        re = memchr(buf, '\n', len);
        if (!re)
                return -1; 

        /* Skip \n */
        re++;
        i = re - buf;
        if (i >= len)
		return -1;

        /* Pass that to the existing Squid status line parsing routine */
        if (!httpStatusLineParse(&rep->sline, buf, re - 1))
                return -1;

        /* All good? Attempt to isolate headers */
        /* The block in question is between re and buf + len */
        parse_start = re;
        if (!httpMsgIsolateHeaders(&parse_start, len - i, &blk_start, &blk_end))
                return -1;

        /* Isolated? parse headers */
        if (!httpHeaderParse(rep, blk_start, blk_end))
                return -1;

        /* Done */
	if(need_post_to_ca(rep))
	{
		cclog(6, "data will be sent to CA");
		if(process_rep_header_buf(rep) != OK)
			return -1;

		rep->header_ok = 1;
		return 1;
	}
	
	cclog(6, "data will be sent to FC");

	rep->header_ok = 1;
        return 1;
}


int process_header_real(HttpHeader * header, char *buf, int len)
{
	if(header->owner == hoRequest)
		return parseHttpRequest(header, buf, len);
	else if(header->owner == hoReply)
		return parseHttpReply(header, buf, len);
	else
	{
		cclog(1, "unknown header owner type!");
		return ERR;
	}
}

void free_http_header(HttpHeader * header)
{
	if(header)
	{
		if(header->owner > hoNone && header->owner <= hoReply)
			httpHeaderClean(header);
		stringClean(&header->uri);
		stringClean(&header->url);
		stringClean(&header->ip);

		if(header->data)
			header->data = NULL;
	}
}
	

	
		
