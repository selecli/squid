
/*
 * $Id: cache_cf.c,v 1.480.2.12 2008/06/27 21:52:56 hno Exp $
 *
 * DEBUG: section 3     Configuration File Parsing
 * AUTHOR: Harvest Derived
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
#if HAVE_GLOB_H
#include <glob.h>
#endif

#if SQUID_SNMP
#include "snmp.h"
#endif

typedef struct _FilePacker FilePacker;
struct _FilePacker{
	FILE* fp;
};
CBDATA_TYPE(FilePacker);
static const char *const T_SECOND_STR = "second";
static const char *const T_MINUTE_STR = "minute";
static const char *const T_HOUR_STR = "hour";
static const char *const T_DAY_STR = "day";
static const char *const T_WEEK_STR = "week";
static const char *const T_FORTNIGHT_STR = "fortnight";
static const char *const T_MONTH_STR = "month";
static const char *const T_YEAR_STR = "year";
static const char *const T_DECADE_STR = "decade";

static const char *const B_BYTES_STR = "bytes";
static const char *const B_KBYTES_STR = "KB";
static const char *const B_MBYTES_STR = "MB";
static const char *const B_GBYTES_STR = "GB";

static const char *const list_sep = ", \t\n\r";

/*save event run time interval here*/
static double g_b4RecfgWaitNsec             = 0.300;/* default 300 ms */
static double g_b4DefaultLoadModuleWaitNsec = 0.300;/* default 300 ms*/
static double g_b4DefaultIfNoneRWaitNsec    = 0.300;/* default 300 ms*/
static double g_b4ParseOnePartCfgWaitNsec   = 0.100;/* default 100 ms*/
static double g_b4AccessCleanWaitNsec       = 0.020;/* default  20 ms*/
static double g_b4AccessCombineWaitNsec     = 0.100;/* default 100 ms*/
static double g_b4AccessFreeWaitNsec        = 0.010;/* default  10 ms*/

static void free_acl_access(acl_access ** head);
static void parse_cachedir_option_readonly(SwapDir * sd, const char *option, const char *value, int reconfiguring);
static void dump_cachedir_option_readonly(StoreEntry * e, const char *option, SwapDir * sd);
static void parse_cachedir_option_minsize(SwapDir * sd, const char *option, const char *value, int reconfiguring);
static void dump_cachedir_option_minsize(StoreEntry * e, const char *option, SwapDir * sd);
static void parse_cachedir_option_maxsize(SwapDir * sd, const char *option, const char *value, int reconfiguring);
static void dump_cachedir_option_maxsize(StoreEntry * e, const char *option, SwapDir * sd);
static void parse_logformat(logformat ** logformat_definitions);
static void parse_access_log(customlog ** customlog_definitions);
static void dump_logformat(StoreEntry * entry, const char *name, logformat * definitions);
static void dump_access_log(StoreEntry * entry, const char *name, customlog * definitions);
static void free_logformat(logformat ** definitions);
static void free_access_log(customlog ** definitions);
static void parse_zph_mode(enum zph_mode *mode);
static void dump_zph_mode(StoreEntry * entry, const char *name, enum zph_mode mode);
static void free_zph_mode(enum zph_mode *mode);

static int parse_line_r(char *buff);

static void free_refreshpattern(refresh_t ** head);
static void free_acl_address(acl_address ** head);
static void free_acl_tos(acl_tos ** head);

static struct cache_dir_option common_cachedir_options[] =
{
	{"no-store", parse_cachedir_option_readonly, dump_cachedir_option_readonly},
	{"read-only", parse_cachedir_option_readonly, NULL},
	{"min-size", parse_cachedir_option_minsize, dump_cachedir_option_minsize},
	{"max-size", parse_cachedir_option_maxsize, dump_cachedir_option_maxsize},
	{NULL, NULL}
};


static void update_maxobjsize(void);
static void configDoConfigure(void);
static void parse_refreshpattern(refresh_t **);
static int parseTimeUnits(const char *unit);
static void parseTimeLine(time_t * tptr, const char *units);
static void parse_ushort(u_short * var);
static void parse_string(char **);
static void default_all(void);
static void defaults_if_none(void);
static void defaults_if_none_r(int first_time);
static int parse_line(char *);
static void parseBytesLine(squid_off_t * bptr, const char *units);
static size_t parseBytesUnits(const char *unit);
static void free_all(void);
void requirePathnameExists(const char *name, const char *path);
static OBJH dump_config;
#ifdef CC_FRAMEWORK
static void cc_free_accessList_r();
#endif
#ifdef HTTP_VIOLATIONS
static void dump_http_header_access(StoreEntry * entry, const char *name, header_mangler header[]);
static void parse_http_header_access(header_mangler header[]);
static void free_http_header_access(header_mangler header[]);
static void dump_http_header_replace(StoreEntry * entry, const char *name, header_mangler header[]);
static void parse_http_header_replace(header_mangler * header);
static void free_http_header_replace(header_mangler * header);
#endif
static void parse_denyinfo(acl_deny_info_list ** var);
static void dump_denyinfo(StoreEntry * entry, const char *name, acl_deny_info_list * var);
static void free_denyinfo(acl_deny_info_list ** var);
#if USE_WCCPv2
static void parse_sockaddr_in_list(sockaddr_in_list **);
static void dump_sockaddr_in_list(StoreEntry *, const char *, const sockaddr_in_list *);
static void free_sockaddr_in_list(sockaddr_in_list **);
#if UNUSED_CODE
static int check_null_sockaddr_in_list(const sockaddr_in_list *);
#endif
#endif
static void parse_http_port_list(http_port_list **);
static void dump_http_port_list(StoreEntry *, const char *, const http_port_list *);
static void free_http_port_list(http_port_list **);
#if 0
static int check_null_http_port_list(const http_port_list *);
#endif
#if USE_SSL
static void parse_https_port_list(https_port_list **);
static void dump_https_port_list(StoreEntry *, const char *, const https_port_list *);
static void free_https_port_list(https_port_list **);
#if 0
static int check_null_https_port_list(const https_port_list *);
#endif
#endif /* USE_SSL */
static void parse_programline(wordlist **);
static void free_programline(wordlist **);
static void dump_programline(StoreEntry *, const char *, const wordlist *);

static int parseOneConfigFile(const char *file_name, int depth);
#ifdef CC_FRAMEWORK
#ifdef USE_SSL
static int diffHttpsPortConfig();
#endif
static int diffHttpPortConfig();
#endif

void free_all_r(void);
void
self_destruct(void)
{
#ifdef CC_FRAMEWORK
    cc_framework_check_self_destruct();
#endif
	shutting_down = 1;
	fatalf("Bungled %s line %d: %s",
		   cfg_filename, config_lineno, config_input_line);
}

void
wordlistDestroy(wordlist ** list)
{
	wordlist *w = NULL;
	while ((w = *list) != NULL)
	{
		*list = w->next;
		safe_free(w->key);
		memFree(w, MEM_WORDLIST);
	}
	*list = NULL;
}

const char *
wordlistAdd(wordlist ** list, const char *key)
{
	while (*list)
		list = &(*list)->next;
	*list = memAllocate(MEM_WORDLIST);
	(*list)->key = xstrdup(key);
	(*list)->next = NULL;
	return (*list)->key;
}

void
wordlistJoin(wordlist ** list, wordlist ** wl)
{
	while (*list)
		list = &(*list)->next;
	*list = *wl;
	*wl = NULL;
}

void
wordlistAddWl(wordlist ** list, wordlist * wl)
{
	while (*list)
		list = &(*list)->next;
	for (; wl; wl = wl->next, list = &(*list)->next)
	{
		*list = memAllocate(MEM_WORDLIST);
		(*list)->key = xstrdup(wl->key);
		(*list)->next = NULL;
	}
}

void
wordlistCat(const wordlist * w, MemBuf * mb)
{
	while (NULL != w)
	{
		memBufPrintf(mb, "%s\n", w->key);
		w = w->next;
	}
}

wordlist *
wordlistDup(const wordlist * w)
{
	wordlist *D = NULL;
	while (NULL != w)
	{
		wordlistAdd(&D, w->key);
		w = w->next;
	}
	return D;
}

void
intlistDestroy(intlist ** list)
{
	intlist *w = NULL;
	intlist *n = NULL;
	for (w = *list; w; w = n)
	{
		n = w->next;
		memFree(w, MEM_INTLIST);
	}
	*list = NULL;
}

int
intlistFind(intlist * list, int i)
{
	intlist *w = NULL;
	for (w = list; w; w = w->next)
		if (w->i == i)
			return 1;
	return 0;
}

/*
 * These functions is the same as atoi/l/f, except that they check for errors
 */

static long
xatol(const char *token)
{
	char *end;
	long ret = strtol(token, &end, 10);
	if (end == token || *end)
		self_destruct();
	return ret;
}

int
xatoi(const char *token)
{
	return xatol(token);
}

unsigned short
xatos(const char *token)
{
	long port = xatol(token);
	if (port & ~0xFFFF)
		self_destruct();
	return port;
}

static double
xatof(const char *token)
{
	char *end;
	double ret = strtod(token, &end);
	if (ret == 0 && end == token)
		self_destruct();
	return ret;
}

int
GetInteger(void)
{
	char *token = strtok(NULL, w_space);
	char *end;
	int i;
	double d;
	if (token == NULL)
		self_destruct();
	i = strtol(token, &end, 0);
	d = strtod(token, NULL);
	if (d > INT_MAX || end == token)
		self_destruct();
	return i;
}

#ifdef CC_MULTI_CORE
int GetIntegerPerId(void)
{
	int i = GetInteger();
	return opt_total_squids > 0 ? i / opt_total_squids : i;
}

#endif
static u_short
GetShort(void)
{
	char *token = strtok(NULL, w_space);
	if (token == NULL)
		self_destruct();
	return xatos(token);
}

static squid_off_t
GetOffT(void)
{
	char *token = strtok(NULL, w_space);
	char *end;
	squid_off_t i;
	if (token == NULL)
		self_destruct();
	i = strto_off_t(token, &end, 0);
#if SIZEOF_SQUID_OFF_T <= 4
	{
		double d = strtod(token, NULL);
		if (d > INT_MAX)
			end = token;
	}
#endif
	if (end == token)
		self_destruct();
	return i;
}

static void
update_maxobjsize(void)
{
	int i;
	squid_off_t ms = -1;
	if(reconfigure_in_thread)
	{
		for (i = 0; i < Config_r.cacheSwap.n_configured; i++)
		{
			if (Config_r.cacheSwap.swapDirs[i].max_objsize > ms)
				ms = Config_r.cacheSwap.swapDirs[i].max_objsize;
		}
	}
	else
	{
		for (i = 0; i < Config.cacheSwap.n_configured; i++)
		{
			if (Config.cacheSwap.swapDirs[i].max_objsize > ms)
				ms = Config.cacheSwap.swapDirs[i].max_objsize;
		}
	}
    store_maxobjsize = ms;

}

/* support chinese characters in squid.conf */
#ifdef CC_FRAMEWORK
#include <iconv.h>
static void iconv_by_utf8(const char *tocode, const char *fromcode)
{
	/* we only convert line that starts with "acl" */
	if (strncmp(config_input_line, "acl", 3)){
		return;
	}

	/* start converting... */
	iconv_t convert;
	if ((iconv_t)-1 == (convert = iconv_open(tocode, fromcode))) {
		/* call failed, do nothing! */
		debug(3,2)("icov_open failed: %s\n", strerror(errno));
		return;	
	}

	char *from = config_input_line;
	char *to = (char*) malloc(BUFSIZ);
	memset(to, 0, BUFSIZ);
	size_t inleft = strlen(from);
	size_t outleft = BUFSIZ;
	if (-1 == iconv(convert, &from, &inleft, &to, &outleft)) {
		debug(3,2)("icov failed: %s\n", strerror(errno));
		return;
	}
	iconv_close(convert);
	/* success */
	strcpy(from, to);
	xfree(to);
}
#endif
/* added end */

static void parseOneConfigFilePost_mainReconfigure()
{
    mainReconfigure();
    return;
}

static void parseOneConfigFilePost_cc_default_load_modules()
{
    cc_default_load_modules();
    //do_reconfigure = 1; 

    eventAdd("parseOneConfigFilePost_mainReconfigure",parseOneConfigFilePost_mainReconfigure,NULL,(double) g_b4RecfgWaitNsec, 0);
    
    return;
}
static void parseOneConfigFilePost_defaults_if_none_r()
{
    defaults_if_none_r(0);

    eventAdd("parseOneConfigFilePost_cc_default_load_modules",parseOneConfigFilePost_cc_default_load_modules,NULL, g_b4DefaultLoadModuleWaitNsec,0);

    return;
}

static void parse_one_part_config(void* data)
{
    FilePacker* fp_pk = (FilePacker*)data;
    FILE* fp = fp_pk->fp;
    char *token = NULL;
    char *tmp_line = NULL;
    int tmp_line_len = 0;
    size_t config_input_line_len;
    int err_count = 0;
    int line_count = 0;
    memset(config_input_line, '\0', BUFSIZ);
	static int matched = 0;
    while(line_count <100)
    {
        if(fgets(config_input_line, BUFSIZ, fp))
        {
            // fprintf(stderr,"parse_one_part_config: one line: %s\n",config_input_line);
			// iconv_by_utf8("UTF-8", "GB2312");
            debug(3,2)("%s\n",config_input_line);
            config_lineno++;
            line_count++;
            if ((token = strchr(config_input_line, '\n')))
                *token = '\0';
            if ((token = strchr(config_input_line, '\r')))
                *token = '\0';
#ifdef CC_FRAMEWORK
            if(config_input_line[0] == '#' && config_input_line[1]!='<' && config_input_line[1] != '>')
                continue;
#else
            if (config_input_line[0] == '#')
                continue;
#endif
            if (config_input_line[0] == '\0')
                continue;
#ifdef CC_FRAMEWORK
			if(strncmp(config_input_line,"#<",2) == 0)
			{
				if (++matched > 1) {
					fprintf(stderr,"fatal: not correct begin domain (%s)",config_input_line);
					// abort();
				}
				// transform config_input_line to lower characters
				debug(3,2)("we will transform %s to lower case\n",config_input_line);
				int index = 0;
				for (index=2; index<strlen(config_input_line); index++)
				{
					if (config_input_line[index]>='A' && config_input_line[index]<='Z')
						config_input_line[index] += 'a' - 'A';
				}
				debug(3,2)("after transform: %s\n",config_input_line);

				debug(3,2)("%s\n",config_input_line);
				char* domain = config_input_line;
				//char *domain = strtok(config_input_line," \t\r\n");
				debug(3,2)("%s\n",domain);
				if(reconfigure_in_thread)
				{
					safe_free(Config_r.current_domain);
				}
				else
				{
					safe_free(Config.current_domain);
				}
				if(!strcmp(domain+2,"common_begin"))
				{
                    debug(3,2)("%s\n",domain+2);
                    if(reconfigure_in_thread)
                        Config_r.current_domain = xstrdup("common_begin");
                    else
                        Config.current_domain = xstrdup("common_begin");
                }
                else if(!strcmp(domain+2,"common_end"))
                {
                    debug(3,2)("%s\n",domain+2);
                    if(reconfigure_in_thread)
                        Config_r.current_domain = xstrdup("common_end");
                    else
                        Config.current_domain = xstrdup("common_end");
                }
                else
                {
                    if(reconfigure_in_thread)
                        Config_r.current_domain = xstrdup(domain+2);
                    else
                        Config.current_domain = xstrdup(domain+2);
                    debug(3,2)("parseoneconfigfile : the domain is: %s\n",domain+2);
                    // Config.accesslist2[Config.current_domain_num]
                }
                continue;
            }
            if(strncmp(config_input_line,"#>",2) == 0)
            {
				if (--matched) {
					fprintf(stderr,"fatal: not correct end domain (%s)",config_input_line);
					// abort();
				}
				char* lasts;
				char *domain = strtok(config_input_line," \t\r\n");
				if(reconfigure_in_thread)
				{
					if(strncasecmp(domain+2,Config_r.current_domain,strlen(Config_r.current_domain)) == 0)
					{
						safe_free(Config_r.current_domain);
						Config_r.current_domain = xstrdup("common_end");
					}
					else
					{
						fprintf(stderr,"fatal: not correct end domain (%s)",Config_r.current_domain);
						abort();
					}
				}
				else
				{
					if(strncasecmp(domain+2,Config.current_domain,strlen(Config.current_domain)) == 0)
					{
						safe_free(Config.current_domain);
						Config.current_domain = xstrdup("common_end");
					}
					else
					{
						fprintf(stderr,"fatal: not correct end domain (%s)",Config.current_domain);
						abort();
					}
				}
				continue;
            }
#endif

            config_input_line_len = strlen(config_input_line);
            tmp_line = (char *) xrealloc(tmp_line, tmp_line_len + config_input_line_len + 1);
            strcpy(tmp_line + tmp_line_len, config_input_line);
            tmp_line_len += config_input_line_len;

            if (tmp_line[tmp_line_len - 1] == '\\')
            {
                debug(3, 5) ("parseoneconfigfile: tmp_line='%s'\n", tmp_line);
                tmp_line[--tmp_line_len] = '\0';
                continue;
            }
            debug(3, 5) ("processing: '%s'\n", tmp_line);

#ifdef  CC_FRAMEWORK
			/* get modues name and set global variable: cc_modules's enable*/
			char * tmp_line_exact = cc_removeTabAndBlank(tmp_line);
			if (!opt_parse_cfg_only && !opt_create_swap_dirs
					&& tmp_line_exact != NULL
					&& !strncmp(tmp_line_exact, CC_MOD_TAG, strlen(CC_MOD_TAG)) 
					&& NULL == strstr(tmp_line_exact, CC_SUB_MOD_TAG))
			{
				cc_load_modules(tmp_line_exact);
			}
#endif
            /* handle includes here */
            /*
               if (tmp_line_len >= 9 &&
               strncmp(tmp_line, "include", 7) == 0 &&
               xisspace(tmp_line[7]))
               {
               err_count += parsemanyconfigfiles(tmp_line + 8, depth + 1);
               }
               else */

            if(reconfigure_in_thread)
            {
                if(!parse_line_r(tmp_line))
                {
                    debug(3, 0) ("parseconfigfile: %s:%d unrecognized: '%s'\n",
                            cfg_filename,
                            config_lineno,
                            tmp_line);
                    err_count++;
                }
            }
            else if (!parse_line(tmp_line))
            {
                debug(3, 0) ("parseconfigfile: %s:%d unrecognized: '%s'\n",
                        cfg_filename,
                        config_lineno,
                        tmp_line);
                err_count++;
            }
            safe_free(tmp_line);
            tmp_line_len = 0;
        }
        else
        {
            /*
             * now parse the configure file over, 
             * we set the parse_over, and close the file pointer
             * and set the do_reconfigure
             */

            parse_over = 1;
            fclose(fp);
#if 0            
            defaults_if_none_r(0);
            cc_default_load_modules();
            do_reconfigure = 1;  
            mainReconfigure();
#endif
            
            eventAdd("parseOneConfigFilePost_defaults_if_none_r",parseOneConfigFilePost_defaults_if_none_r,NULL,g_b4DefaultIfNoneRWaitNsec,0);
            return;
        }
    }
    eventAdd("parse_one_part_config",parse_one_part_config,fp_pk,g_b4ParseOnePartCfgWaitNsec,0);
}

static int prepare_parseOneConfigFile(const char* file_name ,int depth)
{
	FILE *fp = NULL;
	const char *orig_cfg_filename = cfg_filename;
	int orig_config_lineno = config_lineno;
	char *token = NULL;

	debug(3, 1) ("Including Configuration File: %s (depth %d)\n", file_name, depth);
	if (depth > 16)
	{
		debug(0, 0) ("WARNING: can't include %s: includes are nested too deeply (>16)!\n", file_name);
		return 1;
	}
	if ((fp = fopen(file_name, "r")) == NULL)
		fatalf("Unable to open configuration file: %s: %s", file_name, xstrerror());
#ifdef _SQUID_WIN32_
	setmode(fileno(fp), O_TEXT);
#endif

	cfg_filename = file_name;

	if ((token = strrchr(cfg_filename, '/')))
		cfg_filename = token + 1;
#ifdef CC_FRAMEWORK
	Config_r.current_domain = xstrdup("common_end"); // the default domain is common_end
#endif
	CBDATA_INIT_TYPE(FilePacker);
	FilePacker*	fp_pk = cbdataAlloc(FilePacker);
	fp_pk->fp = fp;

	eventAdd("parse_one_part_config",parse_one_part_config,fp_pk,g_b4ParseOnePartCfgWaitNsec,0);
	return 0;
}

static int
parseManyConfigFiles(char *files, int depth)
{
	int error_count = 0;
	char *saveptr = NULL;
#if HAVE_GLOB
	char *path;
	glob_t globbuf;
	int i;
	memset(&globbuf, 0, sizeof(globbuf));
	for (path = strwordtok(files, &saveptr); path; path = strwordtok(NULL, &saveptr))
	{
		if (glob(path, globbuf.gl_pathc ? GLOB_APPEND : 0, NULL, &globbuf) != 0)
		{
			fatalf("Unable to find configuration file: %s: %s",
				   path, xstrerror());
		}
	}
	for (i = 0; i < globbuf.gl_pathc; i++)
	{
		error_count += parseOneConfigFile(globbuf.gl_pathv[i], depth);
	}
	globfree(&globbuf);
#else
	char *file = strwordtok(files, &saveptr);
	while (file != NULL)
	{
		error_count += parseOneConfigFile(file, depth);
		file = strwordtok(NULL, &saveptr);
	}
#endif /* HAVE_GLOB */
	return error_count;
}


static int
parseOneConfigFile(const char *file_name, int depth)
{
	FILE *fp = NULL;
	const char *orig_cfg_filename = cfg_filename;
	int orig_config_lineno = config_lineno;
	char *token = NULL;
	char *tmp_line = NULL;
	int tmp_line_len = 0;
	size_t config_input_line_len;
	int err_count = 0;
	static matched = 0;

	debug(3, 1) ("Including Configuration File: %s (depth %d)\n", file_name, depth);
	if (depth > 16)
	{
		debug(0, 0) ("WARNING: can't include %s: includes are nested too deeply (>16)!\n", file_name);
		return 1;
	}
	if ((fp = fopen(file_name, "r")) == NULL)
		fatalf("Unable to open configuration file: %s: %s",
			   file_name, xstrerror());
#ifdef _SQUID_WIN32_
	setmode(fileno(fp), O_TEXT);
#endif

	cfg_filename = file_name;

	if ((token = strrchr(cfg_filename, '/')))
		cfg_filename = token + 1;
	memset(config_input_line, '\0', BUFSIZ);
	config_lineno = 0;
#ifdef CC_FRAMEWORK
	Config.current_domain = xstrdup("common_end"); // the default domain is common_end
#endif
	while (fgets(config_input_line, BUFSIZ, fp))
	{
		// iconv_by_utf8("UTF-8", "GB2312");
		debug(3,2)("%s\n",config_input_line);
		config_lineno++;
		if ((token = strchr(config_input_line, '\n')))
			*token = '\0';
		if ((token = strchr(config_input_line, '\r')))
			*token = '\0';
#ifdef CC_FRAMEWORK
		if(config_input_line[0] == '#' && config_input_line[1]!='<' && config_input_line[1] != '>')
			continue;
#else
		if (config_input_line[0] == '#')
			continue;
#endif
		if (config_input_line[0] == '\0')
			continue;
#ifdef CC_FRAMEWORK
		if(strncmp(config_input_line,"#<",2) == 0)
		{
			if (++matched > 1) {
				fprintf(stderr,"fatal: not correct begin domain (%s)",config_input_line);
				// abort();
			}
			// transform config_input_line to lower characters
			debug(3,2)("we will transform %s to lower case\n",config_input_line);
			int index = 0;
			for (index=2; index<strlen(config_input_line); index++)
			{
				if (config_input_line[index]>='A' && config_input_line[index]<='Z')
					config_input_line[index] += 'a' - 'A';
			}
			debug(3,2)("after transform: %s\n",config_input_line);
			debug(3,2)("%s\n",config_input_line);
			char* domain = config_input_line;
			//char *domain = strtok(config_input_line," \t\r\n");
			debug(3,2)("%s\n",domain);
			safe_free(Config.current_domain);
			if(strncmp(domain+2,"common_begin",strlen("common_begin")) == 0 && strlen(domain) == strlen("common_begin"))
			{

				debug(3,2)("%s\n",domain+2);
				Config.current_domain = xstrdup("common_begin");
			}
			else if(strncmp(domain+2,"common_end",strlen("common_end"))==0 && strlen(domain) == strlen("common_end"))
			{
				debug(3,2)("%s\n",domain+2);
				Config.current_domain = xstrdup("common_end");
			}
			else
			{
				Config.current_domain = xstrdup(domain+2);
				debug(3,2)("parseOneConfigFile : the domain is: %s\n",domain+2);
//				Config.accesslist2[Config.current_domain_num]
			}
			continue;
		}
		if(strncmp(config_input_line,"#>",2) == 0)
		{
			if (--matched) {
				fprintf(stderr,"fatal: not correct end domain (%s)",config_input_line);
				// abort();
			}
			char *domain = strtok(config_input_line," \t\r\n");
			if(strncasecmp(domain+2,Config.current_domain,strlen(Config.current_domain)) == 0)
			{
				safe_free(Config.current_domain);
				Config.current_domain = xstrdup("common_end");
			}
			else
			{
				fprintf(stderr,"Fatal: not correct end domain (%s)",Config.current_domain);
				abort();
			}
			continue;
			
		}
#endif

		config_input_line_len = strlen(config_input_line);
		tmp_line = (char *) xrealloc(tmp_line, tmp_line_len + config_input_line_len + 1);
		strcpy(tmp_line + tmp_line_len, config_input_line);
		tmp_line_len += config_input_line_len;

		if (tmp_line[tmp_line_len - 1] == '\\')
		{
			debug(3, 5) ("parseOneConfigFile: tmp_line='%s'\n", tmp_line);
			tmp_line[--tmp_line_len] = '\0';
			continue;
		}
		debug(3, 5) ("Processing: '%s'\n", tmp_line);

#ifdef  CC_FRAMEWORK
		/* get modues name and set global variable: cc_modules's enable*/
		char * tmp_line_exact = cc_removeTabAndBlank(tmp_line);
		if (!opt_parse_cfg_only && !opt_create_swap_dirs
			&& tmp_line_exact != NULL
			&& !strncmp(tmp_line_exact, CC_MOD_TAG, strlen(CC_MOD_TAG)) 
			&& NULL == strstr(tmp_line_exact, CC_SUB_MOD_TAG ))
		{
			cc_load_modules(tmp_line_exact);
		}
#endif
		/* Handle includes here */
		if (tmp_line_len >= 9 &&
				strncmp(tmp_line, "include", 7) == 0 &&
				xisspace(tmp_line[7]))
		{
			err_count += parseManyConfigFiles(tmp_line + 8, depth + 1);
		}
		else if (!parse_line(tmp_line))
		{
			debug(3, 0) ("%s: %s:%d unrecognized: '%s'\n", __func__,
						 cfg_filename,
						 config_lineno,
						 tmp_line);
            err_count++;
#ifdef  CC_FRAMEWORK
            cc_framework_check_start("Not define");
#endif
		}
		safe_free(tmp_line);
		tmp_line_len = 0;
	}
#ifdef  CC_FRAMEWORK
	/*put the default module here*/
	cc_default_load_modules();
#endif

	fclose(fp);
	cfg_filename = orig_cfg_filename;
	config_lineno = orig_config_lineno;
	return err_count;
}

int
parseConfigFile(const char *file_name)
{
	int ret;
#ifdef CC_FRAMEWORK
    initializeSquidConfig(&Config);
#else
    configFreeMemory();
#endif
	default_all();
	ret = parseOneConfigFile(file_name, 0);

	/* adjust coss membuf again */
#ifdef CC_FRAMEWORK
	cc_call_hook_func_coss_membuf();
#endif

	defaults_if_none_r(1);
	configDoConfigure();
#ifdef CC_FRAMEWORK
    set_cc_effective_modules();
	cc_combineAclAccessWithCommon();
	debug(3,2)("parseConfigFile: before cc_combineModuleAclAccess\n");
	cc_combineModuleAclAccess();
	debug(3,2)("parseConfigFile: after cc_combineModuleAclAccess\n");
#endif

	if (opt_send_signal == -1)
		cachemgrRegister("config", "Current Squid Configuration", dump_config, 1, 1);
	return ret;
}


#ifdef CC_FRAMEWORK

typedef struct
{
    access_array *to_combine_tab[SQUID_CONFIG_ACL_BUCKET_NUM];
    int to_combine_slot;
    int to_free_slot;
}combine_accessList2_t;
CBDATA_TYPE(combine_accessList2_t);

static void __free_acl_access_array(access_array *aa)
{
	free_acl_access(&aa->http);
	free_acl_access(&aa->http2);
	free_acl_access(&aa->reply);
	free_acl_access(&aa->icp);
#if USE_HTCP
	free_acl_access(&aa->htcp);
#endif
#if USE_HTCP
	free_acl_access(&aa->htcp_clr);
#endif
	free_acl_access(&aa->miss);
#if USE_IDENT
	free_acl_access(&aa->identLookup);
#endif
	free_acl_access(&aa->auth_ip_shortcircuit);
#if FOLLOW_X_FORWARDED_FOR
	free_acl_access(&aa->followXFF);
#endif
	free_acl_access(&aa->log);
	free_acl_access(&aa->url_rewrite);
	free_acl_access(&aa->storeurl_rewrite);
	free_acl_access(&aa->location_rewrite);
	free_acl_access(&aa->noCache);
	free_acl_access(&aa->brokenPosts);
	free_acl_access(&aa->upgrade_http09);
	free_acl_access(&aa->vary_encoding);
#if SQUID_SNMP
	free_acl_access(&aa->snmp);
#endif
	free_acl_access(&aa->AlwaysDirect);
	free_acl_access(&aa->ASlists);
	free_acl_access(&aa->NeverDirect);
    free_acl_tos(&aa->outgoing_tos);
    free_acl_address(&aa->outgoing_address);
	free_refreshpattern(&aa->refresh);
	safe_free(aa->domain);
	safe_free(aa);        

	return;
}

void free_acl_access_array(access_array *aa)
{
    while(aa)
    {
        access_array *cur;
        cur = aa;
        aa = aa->next;
        __free_acl_access_array(cur);
    }
    return;
}

void cc_combineAclAccessWithCommonPostCleanDone_r(combine_accessList2_t *combine_accessList2)
{
    extern void mainReconfigure_done();
    cbdataFree(combine_accessList2);

    if(Config.onoff.debug_80_log)
    {
    debug(3,0)("%s: post clean end\n", __func__);
    }

    /*try to trigger mempool shrink ...*/
    memConfigure();
    //configDoConfigure();

    mainReconfigure_done();
    return;
}

void cc_combineAclAccessWithCommonPostCleanNext_r(combine_accessList2_t *combine_accessList2)
{
    int slot;

    //debug(3,0)("%s: post clean beg\n", __func__);

    for(slot = combine_accessList2->to_free_slot; 
    slot < combine_accessList2->to_free_slot + 100 && slot < SQUID_CONFIG_ACL_BUCKET_NUM; 
    slot ++)
    {
        access_array *tmp;
        
        tmp = combine_accessList2->to_combine_tab[ slot ];
        combine_accessList2->to_combine_tab[ slot ] = NULL;      

        if(NULL != tmp)
        {
            free_acl_access_array(tmp);
        }
    }    

    combine_accessList2->to_free_slot = slot;
    if(combine_accessList2->to_free_slot >= SQUID_CONFIG_ACL_BUCKET_NUM)
    {   
        eventAdd("cc_combineAclAccessWithCommonPostCleanDone_r", (EVH *)cc_combineAclAccessWithCommonPostCleanDone_r, combine_accessList2, g_b4AccessCleanWaitNsec, 0);		
        return;
    }

    eventAdd("cc_combineAclAccessWithCommonPostCleanNext_r", (EVH *)cc_combineAclAccessWithCommonPostCleanNext_r, combine_accessList2, g_b4AccessCleanWaitNsec, 0);		

    return;
}

void cc_combineAclAccessWithCommonPostCleanStart_r(combine_accessList2_t *combine_accessList2)
{

    if(Config.onoff.debug_80_log)
    {
    debug(3,0)("%s: post clean beg\n", __func__);
    }

    cc_combineAclAccessWithCommonPostCleanNext_r(combine_accessList2);

    return;
}

void cc_combineAclAccessWithCommonDone_r(combine_accessList2_t *combine_accessList2)
{
    int slot;

    for(slot = 0; slot < SQUID_CONFIG_ACL_BUCKET_NUM; slot ++)
    {
        access_array *tmp;

        /*xchg now*/        
        tmp = Config.accessList2[slot];
        Config.accessList2[slot] = combine_accessList2->to_combine_tab[ slot ];
        combine_accessList2->to_combine_tab[ slot ] = tmp;        
    }
    
	if (opt_send_signal == -1)
		cachemgrRegister("config", "Current Squid Configuration", dump_config, 1, 1);

    eventAdd("cc_combineAclAccessWithCommonPostCleanStart_r", (EVH *)cc_combineAclAccessWithCommonPostCleanStart_r, combine_accessList2, g_b4AccessCombineWaitNsec, 0);		
    return;		
}

void cc_combineAclAccessWithCommonNext_r(combine_accessList2_t *combine_accessList2)
{
	int slot,len_begin,len_end;
	access_array *cur,*next;
	access_array *common_begin, *common_end;
	common_begin = common_end = NULL;
	acl_access *cur_acl,*next_acl,*common_acl;

	/* set common_begin */
	slot = myHash("common_begin",SQUID_CONFIG_ACL_BUCKET_NUM);
	cur = next = Config.accessList2[slot];
	len_begin = strlen("common_begin");
	len_end = strlen("common_end");
	while(cur)
	{
		if(strncmp(cur->domain,"common_begin",len_begin) == 0)
		{
			common_begin = cur;
			break;

		}
		cur = cur->next;
	}

	/* set common_end  */
	slot = myHash("common_end",SQUID_CONFIG_ACL_BUCKET_NUM);
	cur = next = Config.accessList2[slot];
	while(cur)
	{
		if(strncmp(cur->domain,"common_end",len_end) == 0)
		{
			common_end = cur;
			break;

		}
		cur = cur->next;
	}

    slot = combine_accessList2->to_combine_slot;
    combine_accessList2->to_combine_slot ++;
	if(Config.onoff.debug_80_log)
	{
        debug(3,0)("%s: handle slot %d\n", __func__, slot);
	}

	if(slot < SQUID_CONFIG_ACL_BUCKET_NUM)
	{
	    assert(NULL == combine_accessList2->to_combine_tab[ slot ]);
	    access_array **tail = &(combine_accessList2->to_combine_tab[ slot ]);
		cur = next = Config.accessList2[slot];

        if(Config.onoff.debug_80_log)
        {
    		debug(3,0)("%s: combine slot %d beg\n", __func__, slot);
		}
		while(cur)	
		{
            access_array *des;

    		des = xcalloc(1,sizeof(access_array));
    		des->domain = xstrdup(cur->domain);

            /*copy cur to des*/
    		combineAccessArray(des,NULL,cur);

			/*every entry should link with the common*/

			if(strncmp(des->domain,"common_begin",len_begin) != 0 && strncmp(des->domain,"common_end",len_end)!= 0)
			{
			    if(Config.onoff.debug_80_log)
			    {
                    debug(3,0)("%s: combine to %s beg\n", __func__, des->domain);
                }
                
				combineAccessArray(des,common_end,common_begin);
				
				if(Config.onoff.debug_80_log)
				{
    				debug(3,0)("%s: combine to %s end\n", __func__, des->domain);
				}
			}

			(*tail) = des;
			tail    = &(des->next);
			
			cur = cur->next;
		}

		if(Config.onoff.debug_80_log)
		{
    		debug(3,0)("%s: combine slot %d end\n", __func__, slot);
		}

        /*interval is 10ms*/
	    eventAdd("cc_combineAclAccessWithCommonNext_r", (EVH *)cc_combineAclAccessWithCommonNext_r, combine_accessList2, g_b4AccessCombineWaitNsec, 0);

	    return;
	}

    if(SQUID_CONFIG_ACL_BUCKET_NUM == slot)
    {
        eventAdd("cc_combineAclAccessWithCommonDone_r", (EVH *)cc_combineAclAccessWithCommonDone_r, combine_accessList2, g_b4AccessCombineWaitNsec, 0);
	}

    return;
}

void cc_combineAclAccessWithCommonStart_r()
{
	int slot,len_begin,len_end;
	access_array *cur,*next;
	access_array *common_begin, *common_end;
	common_begin = common_end = NULL;
	acl_access *cur_acl,*next_acl,*common_acl;
    combine_accessList2_t *combine_accessList2;
    int idx;
    CBDATA_INIT_TYPE(combine_accessList2_t);
    combine_accessList2 = cbdataAlloc(combine_accessList2_t);
	for(idx = 0; idx < SQUID_CONFIG_ACL_BUCKET_NUM; idx ++)
	{
	    combine_accessList2->to_combine_tab[ idx ] = NULL;
	}	
	combine_accessList2->to_combine_slot = 0;
	combine_accessList2->to_free_slot    = 0;

	/* set common_begin */
	slot = myHash("common_begin",SQUID_CONFIG_ACL_BUCKET_NUM);
	cur = next = Config.accessList2[slot];
	len_begin = strlen("common_begin");
	len_end = strlen("common_end");
	while(cur)
	{
		if(strncmp(cur->domain,"common_begin",len_begin) == 0)
		{
			common_begin = cur;
			break;

		}
		cur = cur->next;
	}

	/* set common_end  */
	slot = myHash("common_end",SQUID_CONFIG_ACL_BUCKET_NUM);
	cur = next = Config.accessList2[slot];
	while(cur)
	{
		if(strncmp(cur->domain,"common_end",len_end) == 0)
		{
			common_end = cur;
			break;

		}
		cur = cur->next;
	}

	/* now set the common access_array as common_begin + common_end 
	 * */
	slot = myHash("common",SQUID_CONFIG_ACL_BUCKET_NUM);
	cur = next =Config.accessList2[slot];
	while(cur)
	{

		if(strncmp(cur->domain,"common",6) ==0 && strlen(cur->domain) == 6)
		{
			break;
		}
		cur = cur->next;
	}
	if(cur == NULL)
	{
		cur = xcalloc(1,sizeof(access_array));
		cur->domain = xstrdup("common");
	}
	cur->next = Config.accessList2[slot];
	Config.accessList2[slot] = cur;
	combineAccessArray(cur,common_end,common_begin);
    
    eventAdd("cc_combineAclAccessWithCommonNext_r", (EVH *)cc_combineAclAccessWithCommonNext_r, combine_accessList2, g_b4AccessCombineWaitNsec, 0);
	
    return;
}
void after_parseConfigFile_r(char* Configfile)
{
	int http_port_c = diffHttpPortConfig();
	int https_port_c = 0;
    if(Config.onoff.debug_80_log)
    {
    	debug(3,0)("%s: enter\n", __func__);
	}
	
#ifdef USE_SSL
	https_port_c= diffHttpsPortConfig();
#endif
	if (http_port_c || https_port_c)
		http_port_cfg_changed = 1;

	int i, j;
	if(Config.onoff.debug_80_log)
	{
    	debug(3,0)("%s: configFreeMemory beg\n", __func__);
	}
	// memcpy(&Config_r.cacheSwap, &Config.cacheSwap, sizeof(Config.cacheSwap));  // put this into parse_line_r
    // copy from Config_r to Config
	configFreeMemory();                                     // free Config
	if(Config.onoff.debug_80_log)
	{
    	debug(3,0)("%s: configFreeMemory end\n", __func__);
	}
	memcpy(&Config, &Config_r, sizeof(Config_r));

    update_maxobjsize();    // is this call necessary ???
	opt_forwarded_for = opt_forwarded_for_r;
	method_t method = 0;
	for (method = METHOD_EXT00; method < METHOD_ENUM_END; method++)
	{
		if (0 != strncmp("%EXT", RequestMethods_r[method].str, 4))
		{
			debug(248, 3)("RequestMethods_r[method].str = %s, RequestMethods[method].str = %s\n", 
                    RequestMethods_r[method].str, RequestMethods[method].str);
			urlExtMethodAdd(RequestMethods_r[method].str);
			char buf[32];
			snprintf(buf, sizeof(buf), "%%EXT%02d", method - METHOD_EXT00);
			safe_free(RequestMethods_r[method].str);
			RequestMethods_r[method].str = xstrdup(buf);
			RequestMethods_r[method].len = strlen(buf);
		}
	}
    if(Config.onoff.debug_80_log)
    {
        debug(3,0)("%s: cc_set_modules_configuring beg\n", __func__);
    }
    // copy from cc_modules_r to cc_modules
    cc_set_modules_configuring();   
    if(Config.onoff.debug_80_log)
    {
        debug(3,0)("%s: cc_set_modules_configuring end\n", __func__);
    }
    for (i=0;i<cc_modules_r.count;i++)
    {    
        cc_module *mod = cc_modules.items[i];
        cc_module *mod_r = cc_modules_r.items[i];

        mod->flags.init = mod_r->flags.init;  
        mod->flags.configed = mod_r->flags.configed;
        mod->flags.enable = mod_r->flags.enable;
        mod->flags.param = mod_r->flags.param;
        mod->flags.config_on = mod_r->flags.config_on;
        mod->flags.config_off = mod_r->flags.config_off;
        mod->flags.c_check_reply = mod_r->flags.c_check_reply;
        mod->lib = mod_r->lib;
		mod->cc_mod_param_call_back = mod_r->cc_mod_param_call_back;
        mod->has_acl_rules = mod_r->has_acl_rules;

        // 浅拷贝， 内存转移到cc_modules中的成员指针去了
        mod->acl_rules = mod_r->acl_rules;
        memcpy(&mod->mod_params, &mod_r->mod_params, sizeof(Array));
        for (j = 0; j < 100; j++) mod->domain_access[j] = mod_r->domain_access[j];
        // hook function pointer copy 
        memcpy(ADDR_OF_HOOK_FUNC(mod,hook_func_sys_init),ADDR_OF_HOOK_FUNC(mod_r,hook_func_sys_init),HPIDX_last* sizeof(void*));
    }  
    set_cc_effective_modules();

    if(Config.onoff.debug_80_log)
    {
        debug(3,0)("%s: cc_destroy_hook_to_module beg\n", __func__);
    }
    // copy from cc_hook_to_module_r to cc_hook_to_module
    cc_destroy_hook_to_module();
    if(Config.onoff.debug_80_log)
    {
        debug(3,0)("%s: cc_destroy_hook_to_module end\n", __func__);
    }
    for (i = 0; i < HPIDX_last; i++)
    {
        Array *a = &cc_hook_to_module_r[i];
        for (j = 0; j < a->count; j++)
        {
            arrayAppend(&cc_hook_to_module[i], a->items[j]);
            a->items[j] = NULL;
        }
    }

    if(Config.onoff.debug_80_log)
    {
        debug(3,0)("%s: configDoConfigure beg\n", __func__);
    }
	configDoConfigure();
	if(Config.onoff.debug_80_log)
	{
    	debug(3,0)("%s: configDoConfigure end\n", __func__);
	}

    if(Config.onoff.debug_80_log)
    {
    	debug(3,0)("%s: cc_combineModuleAclAccess beg\n", __func__);
	}
	
	cc_combineModuleAclAccess();
	
	if(Config.onoff.debug_80_log)
	{
    	debug(3,0)("%s: cc_combineModuleAclAccess end\n", __func__);
	}


    /*here must not make a event to run cc_combineAclAccessWithCommonStart_r */
    /*because we have to ensure 'common' domain is ready in Config*/
    cc_combineAclAccessWithCommonStart_r();

    return;
}

void after_parseConfigFile_r_0(char* Configfile)
{
	int http_port_c = diffHttpPortConfig();
	int https_port_c = 0;
#ifdef USE_SSL
	https_port_c= diffHttpsPortConfig();
#endif
	if (http_port_c || https_port_c)
		http_port_cfg_changed = 1;

	int i, j;
	// memcpy(&Config_r.cacheSwap, &Config.cacheSwap, sizeof(Config.cacheSwap));  // put this into parse_line_r
    // copy from Config_r to Config
	configFreeMemory();                                     // free Config
	memcpy(&Config, &Config_r, sizeof(Config_r));

    update_maxobjsize();    // is this call necessary ???
	opt_forwarded_for = opt_forwarded_for_r;
	method_t method = 0;
	for (method = METHOD_EXT00; method < METHOD_ENUM_END; method++)
	{
		if (0 != strncmp("%EXT", RequestMethods_r[method].str, 4))
		{
			debug(248, 3)("RequestMethods_r[method].str = %s, RequestMethods[method].str = %s\n", 
                    RequestMethods_r[method].str, RequestMethods[method].str);
			urlExtMethodAdd(RequestMethods_r[method].str);
			char buf[32];
			snprintf(buf, sizeof(buf), "%%EXT%02d", method - METHOD_EXT00);
			safe_free(RequestMethods_r[method].str);
			RequestMethods_r[method].str = xstrdup(buf);
			RequestMethods_r[method].len = strlen(buf);
		}
	}

    // copy from cc_modules_r to cc_modules
    cc_set_modules_configuring();   
    for (i=0;i<cc_modules_r.count;i++)
    {    
        cc_module *mod = cc_modules.items[i];
        cc_module *mod_r = cc_modules_r.items[i];

        mod->flags.init = mod_r->flags.init;  
        mod->flags.configed = mod_r->flags.configed;
        mod->flags.enable = mod_r->flags.enable;
        mod->flags.param = mod_r->flags.param;
        mod->flags.config_on = mod_r->flags.config_on;
        mod->flags.config_off = mod_r->flags.config_off;
        mod->flags.c_check_reply = mod_r->flags.c_check_reply;

        mod->lib = mod_r->lib;
		mod->cc_mod_param_call_back = mod_r->cc_mod_param_call_back;
        mod->has_acl_rules = mod_r->has_acl_rules;
        mod->acl_rules = mod_r->acl_rules;
        memcpy(&mod->mod_params, &mod_r->mod_params, sizeof(Array));
        for (j = 0; j < 100; j++) mod->domain_access[j] = mod_r->domain_access[j];
        // hook function pointer copy 
        memcpy(ADDR_OF_HOOK_FUNC(mod,hook_func_sys_init),ADDR_OF_HOOK_FUNC(mod_r,hook_func_sys_init),HPIDX_last* sizeof(void*));
    }  
    set_cc_effective_modules();

    // copy from cc_hook_to_module_r to cc_hook_to_module
    cc_destroy_hook_to_module();
    for (i = 0; i < HPIDX_last; i++)
    {
        Array *a = &cc_hook_to_module_r[i];
        for (j = 0; j < a->count; j++)
            arrayAppend(&cc_hook_to_module[i], a->items[j]);
    }

    debug(3,0)("%s: configDoConfigure beg\n", __func__);
	configDoConfigure();
	debug(3,0)("%s: configDoConfigure end\n", __func__);

	debug(3,0)("%s: cc_combineAclAccessWithCommon beg\n", __func__);
	cc_combineAclAccessWithCommon();
	debug(3,0)("%s: cc_combineAclAccessWithCommon end\n", __func__);

	debug(3,0)("%s: cc_combineModuleAclAccess beg\n", __func__);
	cc_combineModuleAclAccess();
	debug(3,0)("%s: cc_combineModuleAclAccess end\n", __func__);
	if (opt_send_signal == -1)
		cachemgrRegister("config", "Current Squid Configuration", dump_config, 1, 1);
}


int initializeSquidConfig(SquidConfig *config)
{
	config->Swap.maxSize = 0;
	config->Swap.highWaterMark = 0;
	config->Swap.lowWaterMark = 0;
	config->memMaxSize = 0;
	config->quickAbort.min = 0;
	config->quickAbort.max = 0;
	config->quickAbort.pct = 0;
	config->readAheadGap = 0;
	config->replPolicy = NULL;
	config->memPolicy = NULL;
	config->negativeTtl = 0;
	config->maxStale = 0;
	config->negativeDnsTtl = 0;
	config->positiveDnsTtl = 0;
	config->shutdownLifetime = 0;
	config->Timeout.read = 0;
	config->Timeout.lifetime = 0;
	config->Timeout.forward = 0;
	config->Timeout.connect = 0;
	config->Timeout.peer_connect = 0;
	config->Timeout.request = 0;
	config->Timeout.persistent_request = 0;
	config->Timeout.pconn = 0;
	config->Timeout.siteSelect = 0;
	config->Timeout.deadPeer = 0;
	config->Timeout.icp_query = 0;
	config->Timeout.icp_query_max = 0;
	config->Timeout.icp_query_min = 0;
	config->Timeout.mcast_icp_query = 0;
#if USE_IDENT
	config->Timeout.ident = 0;
#endif
#if !USE_DNSSERVERS
	config->Timeout.idns_retransmit = 0;
	config->Timeout.idns_query = 0;
#endif
	config->maxRequestHeaderSize = 0;
	config->maxRequestBodySize = 0;
	config->maxReplyHeaderSize = 0;
	config->ReplyBodySize.head = config->ReplyBodySize.tail = NULL;
	config->Port.icp = 0;
#if USE_HTCP
	config->Port.htcp = 0;
#endif
#if SQUID_SNMP
	config->Port.snmp = 0;
#endif
	config->Sockaddr.http = NULL;
#if USE_SSL
	config->Sockaddr.https = NULL;
#endif

#if SQUID_SNMP
	config->Snmp.configFile = NULL;
	config->Snmp.agentInfo = NULL;
#endif

#if USE_WCCP
	memset(&config->Wccp,0,sizeof(config->Wccp));
#endif

#if USE_WCCPv2
	memset(&config->Wccp2,0,sizeof(config->Wccp2));
#endif
		
	config->as_whois_server = NULL;
	memset(&config->Log,0,sizeof(config->Log));
	config->adminEmail = NULL;
	config->EmailFrom = NULL;
	config->EmailProgram = NULL;
	config->effectiveUser = NULL;
	config->visible_appname_string = NULL;
	config->effectiveGroup = NULL;
	memset(&config->Program,0,sizeof(config->Program));
#if USE_DNSSERVERS
	config->dnsChildren = 0;
#endif
	config->authenticateIpShortcircuitTTL =0;
	config->authenticateIpTTL = 0;
	config->authenticateTTL = 0;
	config->authenticateGCInterval = 0;
	config->appendDomain = NULL;
	config->appendDomainLen =0;
	config->debugOptions = NULL;
	config->pidFilename = 0 ;
	config->netdbFilename = 0;
	config->mimeTablePathname = 0;
	config->etcHostsPath = NULL;
	config->visibleHostname = NULL;
	config->uniqueHostname = NULL;
	config->hostnameAliases = NULL;
	config->errHtmlText = NULL;
	memset(&config->Announce,0,sizeof(config->Announce));
	memset(&config->Addrs,0,sizeof(config->Addrs));
	config->tcpRcvBufsz = 0;
	config->udpMaxHitObjsz = 0;
	config->hierarchy_stoplist = NULL;
	config->mcast_group_list = NULL;
	config->dns_testname_list = NULL;
	config->dns_nameservers = NULL;
	config->peers = NULL;
	config->npeers = 0;
	memset(&config->ipcache,0,sizeof(config->ipcache));
	memset(&config->fqdncache,0,sizeof(config->fqdncache));
	config->minDirectRtt = 0;
	config->minDirectHops = 0;
	config->passwd_list = NULL;
	memset(&config->Store,0,sizeof(config->Store));
	memset(&config->Netdb,0,sizeof(config->Netdb));
	memset(&config->onoff,0,sizeof(config->onoff));
	config->aclList = NULL;
	memset(&config->accessList2,0,sizeof(access_array*)*SQUID_CONFIG_ACL_BUCKET_NUM);
	memset(&config->accessList,0,sizeof(config->accessList));
	config->denyInfoList = NULL;
	memset(&config->authConfig,0,sizeof(config->authConfig));
	memset(&config->Ftp,0,sizeof(config->Ftp));
	config->Refresh = NULL;
	memset(&config->icons,0,sizeof(config->icons));
	config->errorDirectory = NULL;
	memset(&config->retry,0,sizeof(config->retry));
	config->MemPools.limit = 0;

#if DELAY_POOLS
	memset(&config->Delay,0,sizeof(config->Delay));
#endif
	config->max_open_disk_fds = 0;
	config->uri_whitespace = 0;
	config->rangeOffsetLimit = 0;

#if MULTICAST_MISS_STREAM
	memset(&config->mcast_miss,0,sizeof(config->mcast_miss));
#endif
	memset(config->header_access,0,sizeof(header_mangler)*HDR_ENUM_END);
	config->coredump_dir = NULL;
	config->chroot_dir = NULL;

#if USE_CACHE_DIGESTS
	memset(&config->digest,0,sizeof(config->digest));
#endif

#if USE_SSL
	memset(&config->SSL,0,sizeof(config->SSL));
#endif
	memset(&config->warnings,0,sizeof(config->warnings));
	config->store_dir_select_algorithm = NULL;
	config->sleep_after_fork = 0;
	config->minimum_expiry_time = 0;
	config->externalAclHelperList = NULL;
	config->zph_local = 0;
	config->zph_sibling =0;
	config->zph_parent = 0;
	config->zph_option = 0 ;
	config->errorMapList = NULL;
#if USE_SSL
	memset(&config->ssl_client,0,sizeof(config->ssl_client));
#endif
	config->refresh_stale_window = 0;
	config->umask = 0;
	config->max_filedescriptors = 0;
	config->accept_filter = NULL;
	config->incoming_rate = 0;
	config->current_domain = NULL;
	memset(&config->cacheSwap,0,sizeof(config->cacheSwap));
}

int parseConfigFile_r(const char *file_name)
{
    /*save event run time interval asap*/
    g_b4RecfgWaitNsec             = (0.0 + Config.b4RecfgWaitMsec) / 1000.0;
    g_b4DefaultLoadModuleWaitNsec = (0.0 + Config.b4DefaultLoadModuleWaitMsec) / 1000.0;
    g_b4DefaultIfNoneRWaitNsec    = (0.0 + Config.b4DefaultIfNoneRWaitMsec) / 1000.0;
    g_b4ParseOnePartCfgWaitNsec   = (0.0 + Config.b4ParseOnePartCfgWaitMsec) / 1000.0;
    g_b4AccessCleanWaitNsec       = (0.0 + Config.b4AccessCleanWaitMsec) / 1000.0;
    g_b4AccessCombineWaitNsec     = (0.0 + Config.b4AccessCombineWaitMsec) / 1000.0;
    g_b4AccessFreeWaitNsec        = (0.0 + Config.b4AccessFreeWaitMsec) / 1000.0;

    cc_set_modules_configuring_r();     // initialize cc_modules_r
    initializeSquidConfig(&Config_r);   // initialize Config_r
    cc_set_hook_to_module_r();         // initialize cc_hook_to_module_r
	default_all();
	return prepare_parseOneConfigFile(file_name,0);
}


#endif
static void
configDoConfigure(void)
{
	memset(&Config2, '\0', sizeof(SquidConfig2));
	/* init memory as early as possible */
	memConfigure();
	/* Sanity checks */
	if (Config.cacheSwap.swapDirs == NULL)
		fatal("No cache_dir's specified in config file");
	/* calculate Config.Swap.maxSize */
	storeDirConfigure();
	if (0 == Config.Swap.maxSize)
		/* people might want a zero-sized cache on purpose */
		(void) 0;
	else if (Config.Swap.maxSize < (Config.memMaxSize >> 10))
		debug(3, 0) ("WARNING cache_mem is larger than total disk cache space!\n");
	if (Config.Announce.period > 0)
	{
		Config.onoff.announce = 1;
	}
	else if (Config.Announce.period < 1)
	{
		Config.Announce.period = 86400 * 365;	/* one year */
		Config.onoff.announce = 0;
	}
	if (Config.onoff.httpd_suppress_version_string)
		visible_appname_string = (char *) appname_string;
	else
		visible_appname_string = (char *) full_appname_string;
#if USE_DNSSERVERS
	if (Config.dnsChildren < 1)
		fatal("No dnsservers allocated");
#endif
	if (Config.Program.url_rewrite.command)
	{
		if (Config.Program.url_rewrite.children < 1)
		{
			Config.Program.url_rewrite.children = 0;
			wordlistDestroy(&Config.Program.url_rewrite.command);
		}
	}
	if (Config.Program.location_rewrite.command)
	{
		if (Config.Program.location_rewrite.children < 1)
		{
			Config.Program.location_rewrite.children = 0;
			wordlistDestroy(&Config.Program.location_rewrite.command);
		}
	}
	if (Config.appendDomain)
		if (*Config.appendDomain != '.')
			fatal("append_domain must begin with a '.'");
	if (Config.errHtmlText == NULL)
		Config.errHtmlText = xstrdup(null_string);
	storeConfigure();
	snprintf(ThisCache, sizeof(ThisCache), "%s:%d (%s)",
			 uniqueHostname(),
			 getMyPort(),
			 visible_appname_string);
	/*
	 * the extra space is for loop detection in client_side.c -- we search
	 * for substrings in the Via header.
	 */
	snprintf(ThisCache2, sizeof(ThisCache), " %s:%d (%s)",
			 uniqueHostname(),
			 getMyPort(),
			 visible_appname_string);
	if (!Config.udpMaxHitObjsz || Config.udpMaxHitObjsz > SQUID_UDP_SO_SNDBUF)
		Config.udpMaxHitObjsz = SQUID_UDP_SO_SNDBUF;
	if (Config.appendDomain)
		Config.appendDomainLen = strlen(Config.appendDomain);
	else
		Config.appendDomainLen = 0;
	safe_free(debug_options)
	debug_options = xstrdup(Config.debugOptions);
	if (Config.retry.maxtries > 10)
		fatal("maximum_single_addr_tries cannot be larger than 10");
	if (Config.retry.maxtries < 1)
	{
		debug(3, 0) ("WARNING: resetting 'maximum_single_addr_tries to 1\n");
		Config.retry.maxtries = 1;
	}
	requirePathnameExists("MIME Config Table", Config.mimeTablePathname);
#if USE_DNSSERVERS
	requirePathnameExists("cache_dns_program", Config.Program.dnsserver);
#endif
#if USE_UNLINKD
	requirePathnameExists("unlinkd_program", Config.Program.unlinkd);
#endif
	requirePathnameExists("logfile_daemon", Config.Program.logfile_daemon);
	if (Config.Program.url_rewrite.command)
		requirePathnameExists("url_rewrite_program", Config.Program.url_rewrite.command->key);
	if (Config.Program.location_rewrite.command)
		requirePathnameExists("location_rewrite_program", Config.Program.location_rewrite.command->key);
	requirePathnameExists("Icon Directory", Config.icons.directory);
	requirePathnameExists("Error Directory", Config.errorDirectory);
	authenticateConfigure(&Config.authConfig);
	externalAclConfigure();
	refreshCheckConfigure();
#if HTTP_VIOLATIONS
	{
		const refresh_t *R;
		for (R = Config.Refresh; R; R = R->next)
		{
			if (!R->flags.override_expire)
				continue;
			debug(22, 1) ("WARNING: use of 'override-expire' in 'refresh_pattern' violates HTTP\n");
			break;
		}
		for (R = Config.Refresh; R; R = R->next)
		{
			if (!R->flags.override_lastmod)
				continue;
			debug(22, 1) ("WARNING: use of 'override-lastmod' in 'refresh_pattern' violates HTTP\n");
			break;
		}
		for (R = Config.Refresh; R; R = R->next)
		{
			if (R->stale_while_revalidate <= 0)
				continue;
			debug(22, 1) ("WARNING: use of 'stale-while-revalidate' in 'refresh_pattern' violates HTTP\n");
			break;
		}
	}
#endif
#if !HTTP_VIOLATIONS
	Config.onoff.via = 1;
#else
	if (!Config.onoff.via)
		debug(22, 1) ("WARNING: HTTP requires the use of Via\n");
#endif
	if (aclPurgeMethodInUse(Config.accessList.http))
		Config2.onoff.enable_purge = 1;
	if (geteuid() == 0)
	{
		if (NULL != Config.effectiveUser)
		{
			struct passwd *pwd = getpwnam(Config.effectiveUser);
			if (NULL == pwd)
				/*
				 * Andres Kroonmaa <andre@online.ee>:
				 * Some getpwnam() implementations (Solaris?) require
				 * an available FD < 256 for opening a FILE* to the
				 * passwd file.
				 * DW:
				 * This should be safe at startup, but might still fail
				 * during reconfigure.
				 */
				fatalf("getpwnam failed to find userid for effective user '%s'",
					   Config.effectiveUser);
			Config2.effectiveUserID = pwd->pw_uid;
			Config2.effectiveGroupID = pwd->pw_gid;
#if HAVE_PUTENV
			if (pwd->pw_dir && *pwd->pw_dir)
			{
				int len;
				char *env_str = xcalloc((len = strlen(pwd->pw_dir) + 6), 1);
				snprintf(env_str, len, "HOME=%s", pwd->pw_dir);
				putenv(env_str);
			}
#endif
		}
	}
	else
	{
		Config2.effectiveUserID = geteuid();
		Config2.effectiveGroupID = getegid();
	}
	if (NULL != Config.effectiveGroup)
	{
		struct group *grp = getgrnam(Config.effectiveGroup);
		if (NULL == grp)
			fatalf("getgrnam failed to find groupid for effective group '%s'",
				   Config.effectiveGroup);
		Config2.effectiveGroupID = grp->gr_gid;
	}
	if (0 == Config.onoff.client_db)
	{
		acl *a;
		for (a = Config.aclList; a; a = a->next)
		{
			if (ACL_MAXCONN != a->type)
				continue;
			debug(22, 0) ("WARNING: 'maxconn' ACL (%s) won't work with client_db disabled\n", a->name);
		}
	}
	if (Config.negativeDnsTtl <= 0)
	{
		debug(22, 0) ("WARNING: resetting negative_dns_ttl to 1 second\n");
		Config.negativeDnsTtl = 1;
	}
	if (Config.positiveDnsTtl < Config.negativeDnsTtl)
	{
		debug(22, 0) ("NOTICE: positive_dns_ttl must be larger than negative_dns_ttl. Resetting negative_dns_ttl to match\n");
		Config.positiveDnsTtl = Config.negativeDnsTtl;
	}
#if SIZEOF_SQUID_FILE_SZ <= 4
#if SIZEOF_SQUID_OFF_T <= 4
	if (Config.Store.maxObjectSize > 0x7FFF0000)
	{
		debug(22, 0) ("NOTICE: maximum_object_size limited to %d KB due to hardware limitations\n", 0x7FFF0000 / 1024);
		Config.Store.maxObjectSize = 0x7FFF0000;
	}
#elif SIZEOF_OFF_T <= 4
	if (Config.Store.maxObjectSize > 0xFFFF0000)
	{
		debug(22, 0) ("NOTICE: maximum_object_size limited to %d KB due to OS limitations\n", 0xFFFF0000 / 1024);
		Config.Store.maxObjectSize = 0xFFFF0000;
	}
#else
	if (Config.Store.maxObjectSize > 0xFFFF0000)
	{
		debug(22, 0) ("NOTICE: maximum_object_size limited to %d KB to keep compatibility with existing cache\n", 0xFFFF0000 / 1024);
		Config.Store.maxObjectSize = 0xFFFF0000;
	}
#endif
#endif
	if (Config.Store.maxInMemObjSize > 8 * 1024 * 1024)
		debug(22, 0) ("WARNING: Very large maximum_object_size_in_memory settings can have negative impact on performance\n");
#if USE_SSL
	Config.ssl_client.sslContext = sslCreateClientContext(Config.ssl_client.cert, Config.ssl_client.key, Config.ssl_client.version, Config.ssl_client.cipher, Config.ssl_client.options, Config.ssl_client.flags, Config.ssl_client.cafile, Config.ssl_client.capath, Config.ssl_client.crlfile);
#endif
}

/* Parse a time specification from the config file.  Store the
 * result in 'tptr', after converting it to 'units' */
static void
parseTimeLine(time_t * tptr, const char *units)
{
	char *token;
	double d;
	time_t m;
	time_t u;
	if ((u = parseTimeUnits(units)) == 0)
		self_destruct();
	if ((token = strtok(NULL, w_space)) == NULL)
		self_destruct();
	d = xatof(token);
	m = u;			/* default to 'units' if none specified */
	if (0 == d)
		(void) 0;
	else if ((token = strtok(NULL, w_space)) == NULL)
		debug(3, 0) ("WARNING: No units on '%s', assuming %f %s\n",
					 config_input_line, d, units);
	else if ((m = parseTimeUnits(token)) == 0)
		self_destruct();
	*tptr = m * d / u;
}

static int
parseTimeUnits(const char *unit)
{
	if (!strncasecmp(unit, T_SECOND_STR, strlen(T_SECOND_STR)))
		return 1;
	if (!strncasecmp(unit, T_MINUTE_STR, strlen(T_MINUTE_STR)))
		return 60;
	if (!strncasecmp(unit, T_HOUR_STR, strlen(T_HOUR_STR)))
		return 3600;
	if (!strncasecmp(unit, T_DAY_STR, strlen(T_DAY_STR)))
		return 86400;
	if (!strncasecmp(unit, T_WEEK_STR, strlen(T_WEEK_STR)))
		return 86400 * 7;
	if (!strncasecmp(unit, T_FORTNIGHT_STR, strlen(T_FORTNIGHT_STR)))
		return 86400 * 14;
	if (!strncasecmp(unit, T_MONTH_STR, strlen(T_MONTH_STR)))
		return 86400 * 30;
	if (!strncasecmp(unit, T_YEAR_STR, strlen(T_YEAR_STR)))
		return 86400 * 365.2522;
	if (!strncasecmp(unit, T_DECADE_STR, strlen(T_DECADE_STR)))
		return 86400 * 365.2522 * 10;
	debug(3, 1) ("parseTimeUnits: unknown time unit '%s'\n", unit);
	return 0;
}

static void
parseBytesLine(squid_off_t * bptr, const char *units)
{
	char *token;
	double d;
	squid_off_t m;
	squid_off_t u;
	if ((u = parseBytesUnits(units)) == 0)
		self_destruct();
	if ((token = strtok(NULL, w_space)) == NULL)
		self_destruct();
	if (strcmp(token, "none") == 0 || strcmp(token, "-1") == 0)
	{
		*bptr = (squid_off_t) - 1;
		return;
	}
	d = xatof(token);
	m = u;			/* default to 'units' if none specified */
	if (0.0 == d)
		(void) 0;
	else if ((token = strtok(NULL, w_space)) == NULL)
		debug(3, 0) ("WARNING: No units on '%s', assuming %f %s\n",
					 config_input_line, d, units);
	else if ((m = parseBytesUnits(token)) == 0)
		self_destruct();
	*bptr = m * d / u;
	if ((double) *bptr * 2 != m * d / u * 2)
		self_destruct();
}

static size_t
parseBytesUnits(const char *unit)
{
	if (!strncasecmp(unit, B_BYTES_STR, strlen(B_BYTES_STR)))
		return 1;
	if (!strncasecmp(unit, B_KBYTES_STR, strlen(B_KBYTES_STR)))
		return 1 << 10;
	if (!strncasecmp(unit, B_MBYTES_STR, strlen(B_MBYTES_STR)))
		return 1 << 20;
	if (!strncasecmp(unit, B_GBYTES_STR, strlen(B_GBYTES_STR)))
		return 1 << 30;
	debug(3, 1) ("parseBytesUnits: unknown bytes unit '%s'\n", unit);
	return 0;
}

/*****************************************************************************
 * Max
 *****************************************************************************/

static void
dump_acl(StoreEntry * entry, const char *name, acl * ae)
{
	while (ae != NULL)
	{
		debug(3, 3) ("dump_acl: %s %s\n", name, ae->name);
		if (strstr(ae->cfgline, " \""))
			storeAppendPrintf(entry, "%s\n", ae->cfgline);
		else
		{
			wordlist *w;
			wordlist *v;
			v = w = aclDumpGeneric(ae);
			while (v != NULL)
			{
				debug(3, 3) ("dump_acl: %s %s %s\n", name, ae->name, v->key);
				storeAppendPrintf(entry, "%s %s %s %s\n",
								  name,
								  ae->name,
								  aclTypeToStr(ae->type),
								  v->key);
				v = v->next;
			}
			wordlistDestroy(&w);
		}
		ae = ae->next;
	}
}

static void
parse_acl(acl ** ae)
{
	aclParseAclLine(ae);
}

static void
free_acl(acl ** ae)
{
	aclDestroyAcls(ae);
}

static void
dump_acl_list(StoreEntry * entry, acl_list * head)
{
	acl_list *l;
	for (l = head; l; l = l->next)
	{
		storeAppendPrintf(entry, " %s%s",
						  l->op ? null_string : "!",
						  l->acl->name);
	}
}

static void
dump_acl_access(StoreEntry * entry, const char *name, acl_access * head)
{
	acl_access *l;
	for (l = head; l; l = l->next)
	{
		storeAppendPrintf(entry, "%s %s",
						  name,
						  l->allow ? "Allow" : "Deny");
		dump_acl_list(entry, l->acl_list);
		storeAppendPrintf(entry, "\n");
	}
}

static void
parse_acl_access(acl_access ** head)
{
	aclParseAccessLine(head);
}

static void
free_acl_access(acl_access ** head)
{
	aclDestroyAccessList(head);
}

static void
dump_address(StoreEntry * entry, const char *name, struct in_addr addr)
{
	storeAppendPrintf(entry, "%s %s\n", name, inet_ntoa(addr));
}

static void
parse_address(struct in_addr *addr)
{
	const struct hostent *hp;
	char *token = strtok(NULL, w_space);

	if (token == NULL)
		self_destruct();
	if (safe_inet_addr(token, addr) == 1)
		(void) 0;
	else if ((hp = gethostbyname(token)))	/* dont use ipcache */
		*addr = inaddrFromHostent(hp);
	else
		self_destruct();
}

static void
free_address(struct in_addr *addr)
{
	memset(addr, '\0', sizeof(struct in_addr));
}

CBDATA_TYPE(acl_address);

static void
dump_acl_address(StoreEntry * entry, const char *name, acl_address * head)
{
	acl_address *l;
	for (l = head; l; l = l->next)
	{
		if (l->addr.s_addr != INADDR_ANY)
			storeAppendPrintf(entry, "%s %s", name, inet_ntoa(l->addr));
		else
			storeAppendPrintf(entry, "%s autoselect", name);
		dump_acl_list(entry, l->acl_list);
		storeAppendPrintf(entry, "\n");
	}
}

static void
freed_acl_address(void *data)
{
	acl_address *l = data;
	aclDestroyAclList(&l->acl_list);
}

static void
parse_acl_address(acl_address ** head)
{
	acl_address *l;
	acl_address **tail = head;	/* sane name below */
	CBDATA_INIT_TYPE_FREECB(acl_address, freed_acl_address);
	l = cbdataAlloc(acl_address);
	parse_address(&l->addr);
	aclParseAclList(&l->acl_list);
	while (*tail)
		tail = &(*tail)->next;
	*tail = l;
}

static void
free_acl_address(acl_address ** head)
{
	while (*head)
	{
		acl_address *l = *head;
		*head = l->next;
		cbdataFree(l);
	}
}

CBDATA_TYPE(acl_tos);

static void
dump_acl_tos(StoreEntry * entry, const char *name, acl_tos * head)
{
	acl_tos *l;
	for (l = head; l; l = l->next)
	{
		if (l->tos > 0)
			storeAppendPrintf(entry, "%s 0x%02X", name, l->tos);
		else
			storeAppendPrintf(entry, "%s none", name);
		dump_acl_list(entry, l->acl_list);
		storeAppendPrintf(entry, "\n");
	}
}

static void
freed_acl_tos(void *data)
{
	acl_tos *l = data;
	aclDestroyAclList(&l->acl_list);
}

static void
parse_acl_tos(acl_tos ** head)
{
	acl_tos *l;
	acl_tos **tail = head;	/* sane name below */
	int tos;
	char junk;
	char *token = strtok(NULL, w_space);
	if (!token)
		self_destruct();
	if (sscanf(token, "0x%x%c", &tos, &junk) != 1)
		self_destruct();
	if (tos < 0 || tos > 255)
		self_destruct();
	CBDATA_INIT_TYPE_FREECB(acl_tos, freed_acl_tos);
	l = cbdataAlloc(acl_tos);
	l->tos = tos;
	aclParseAclList(&l->acl_list);
	while (*tail)
		tail = &(*tail)->next;
	*tail = l;
}

static void
free_acl_tos(acl_tos ** head)
{
	while (*head)
	{
		acl_tos *l = *head;
		*head = l->next;
		l->next = NULL;
		cbdataFree(l);
	}
}

#if DELAY_POOLS

/* do nothing - free_delay_pool_count is the magic free function.
 * this is why delay_pool_count isn't just marked TYPE: ushort
 */
#define free_delay_pool_class(X)
#define free_delay_pool_access(X)
#define free_delay_pool_rates(X)
#define dump_delay_pool_class(X, Y, Z)
#define dump_delay_pool_access(X, Y, Z)
#define dump_delay_pool_rates(X, Y, Z)

static void
free_delay_pool_count(delayConfig * cfg)
{
	int i;

	if (!cfg->pools)
		return;
	for (i = 0; i < cfg->pools; i++)
	{
		if (cfg->class[i])
		{
			delayFreeDelayPool(i);
			safe_free(cfg->rates[i]);
		}
		aclDestroyAccessList(&cfg->access[i]);
	}
	delayFreeDelayData(cfg->pools);
	xfree(cfg->class);
	xfree(cfg->rates);
	xfree(cfg->access);
	memset(cfg, 0, sizeof(*cfg));
}

static void
dump_delay_pool_count(StoreEntry * entry, const char *name, delayConfig cfg)
{
	int i;
	LOCAL_ARRAY(char, nom, 32);

	if (!cfg.pools)
	{
		storeAppendPrintf(entry, "%s 0\n", name);
		return;
	}
	storeAppendPrintf(entry, "%s %d\n", name, cfg.pools);
	for (i = 0; i < cfg.pools; i++)
	{
		storeAppendPrintf(entry, "delay_class %d %d\n", i + 1, cfg.class[i]);
		snprintf(nom, 32, "delay_access %d", i + 1);
		dump_acl_access(entry, nom, cfg.access[i]);
		if (cfg.class[i] >= 1)
			storeAppendPrintf(entry, "delay_parameters %d %d/%d", i + 1,
							  cfg.rates[i]->aggregate.restore_bps,
							  cfg.rates[i]->aggregate.max_bytes);
		if (cfg.class[i] >= 3)
			storeAppendPrintf(entry, " %d/%d",
							  cfg.rates[i]->network.restore_bps,
							  cfg.rates[i]->network.max_bytes);
		if (cfg.class[i] >= 2)
			storeAppendPrintf(entry, " %d/%d",
							  cfg.rates[i]->individual.restore_bps,
							  cfg.rates[i]->individual.max_bytes);
		if (cfg.class[i] >= 1)
			storeAppendPrintf(entry, "\n");
	}
}

static void
parse_delay_pool_count(delayConfig * cfg)
{
	if (cfg->pools)
	{
		debug(3, 0) ("parse_delay_pool_count: multiple delay_pools lines, aborting all previous delay_pools config\n");
		free_delay_pool_count(cfg);
	}
	parse_ushort(&cfg->pools);
	if (cfg->pools)
	{
		delayInitDelayData(cfg->pools);
		cfg->class = xcalloc(cfg->pools, sizeof(u_char));
		cfg->rates = xcalloc(cfg->pools, sizeof(delaySpecSet *));
		cfg->access = xcalloc(cfg->pools, sizeof(acl_access *));
	}
}

static void
parse_delay_pool_class(delayConfig * cfg)
{
	ushort pool, class;

	parse_ushort(&pool);
	if (pool < 1 || pool > cfg->pools)
	{
		debug(3, 0) ("parse_delay_pool_class: Ignoring pool %d not in 1 .. %d\n", pool, cfg->pools);
		return;
	}
	parse_ushort(&class);
	if (class < 1 || class > 3)
	{
		debug(3, 0) ("parse_delay_pool_class: Ignoring pool %d class %d not in 1 .. 3\n", pool, class);
		return;
	}
	pool--;
	if (cfg->class[pool])
	{
		delayFreeDelayPool(pool);
		safe_free(cfg->rates[pool]);
	}
	/* Allocates a "delaySpecSet" just as large as needed for the class */
	cfg->rates[pool] = xmalloc(class * sizeof(delaySpec));
	cfg->class[pool] = class;
	cfg->rates[pool]->aggregate.restore_bps = cfg->rates[pool]->aggregate.max_bytes = -1;
	if (cfg->class[pool] >= 3)
		cfg->rates[pool]->network.restore_bps = cfg->rates[pool]->network.max_bytes = -1;
	if (cfg->class[pool] >= 2)
		cfg->rates[pool]->individual.restore_bps = cfg->rates[pool]->individual.max_bytes = -1;
	delayCreateDelayPool(pool, class);
}

static void
parse_delay_pool_rates(delayConfig * cfg)
{
	ushort pool, class;
	int i;
	delaySpec *ptr;
	char *token;

	parse_ushort(&pool);
	if (pool < 1 || pool > cfg->pools)
	{
		debug(3, 0) ("parse_delay_pool_rates: Ignoring pool %d not in 1 .. %d\n", pool, cfg->pools);
		return;
	}
	pool--;
	class = cfg->class[pool];
	if (class == 0)
	{
		debug(3, 0) ("parse_delay_pool_rates: Ignoring pool %d attempt to set rates with class not set\n", pool + 1);
		return;
	}
	ptr = (delaySpec *) cfg->rates[pool];
	/* read in "class" sets of restore,max pairs */
	while (class--)
	{
		token = strtok(NULL, "/");
		if (token == NULL)
			self_destruct();
		if (sscanf(token, "%d", &i) != 1)
			self_destruct();
		ptr->restore_bps = i;
		i = GetInteger();
		ptr->max_bytes = i;
		ptr++;
	}
	class = cfg->class[pool];
	/* if class is 3, swap around network and individual */
	if (class == 3)
	{
		delaySpec tmp;

		tmp = cfg->rates[pool]->individual;
		cfg->rates[pool]->individual = cfg->rates[pool]->network;
		cfg->rates[pool]->network = tmp;
	}
	/* initialize the delay pools */
	delayInitDelayPool(pool, class, cfg->rates[pool]);
}

static void
parse_delay_pool_access(delayConfig * cfg)
{
	ushort pool;

	parse_ushort(&pool);
	if (pool < 1 || pool > cfg->pools)
	{
		debug(3, 0) ("parse_delay_pool_access: Ignoring pool %d not in 1 .. %d\n", pool, cfg->pools);
		return;
	}
	aclParseAccessLine(&cfg->access[pool - 1]);
}
#endif

#ifdef HTTP_VIOLATIONS
static void
dump_http_header_access(StoreEntry * entry, const char *name, header_mangler header[])
{
	int i;
	header_mangler *other;
	for (i = 0; i < HDR_ENUM_END; i++)
	{
		if (header[i].access_list == NULL)
			continue;
		storeAppendPrintf(entry, "%s ", name);
		dump_acl_access(entry, httpHeaderNameById(i),
						header[i].access_list);
	}
	for (other = header[HDR_OTHER].next; other; other = other->next)
	{
		if (other->access_list == NULL)
			continue;
		storeAppendPrintf(entry, "%s ", name);
		dump_acl_access(entry, other->name,
						other->access_list);
	}
}

static void
parse_http_header_access(header_mangler header[])
{
	int id, i;
	char *t = NULL;
	if ((t = strtok(NULL, w_space)) == NULL)
	{
		debug(3, 0) ("%s line %d: %s\n",
					 cfg_filename, config_lineno, config_input_line);
		debug(3, 0) ("parse_http_header_access: missing header name.\n");
		return;
	}
	/* Now lookup index of header. */
	id = httpHeaderIdByNameDef(t, strlen(t));
	if (strcmp(t, "All") == 0)
		id = HDR_ENUM_END;
	else if (strcmp(t, "Other") == 0)
		id = HDR_OTHER;
	else if (id == -1)
	{
		header_mangler *hdr = header[HDR_OTHER].next;
		while (hdr && strcasecmp(hdr->name, t) != 0)
			hdr = hdr->next;
		if (!hdr)
		{
			hdr = xcalloc(1, sizeof *hdr);
			hdr->name = xstrdup(t);
			hdr->next = header[HDR_OTHER].next;
			header[HDR_OTHER].next = hdr;
		}
		parse_acl_access(&hdr->access_list);
		return;
	}
	if (id != HDR_ENUM_END)
	{
		parse_acl_access(&header[id].access_list);
	}
	else
	{
		char *next_string = t + strlen(t) - 1;
		*next_string = 'A';
		*(next_string + 1) = ' ';
		for (i = 0; i < HDR_ENUM_END; i++)
		{
			char *new_string = xstrdup(next_string);
			strtok(new_string, w_space);
			parse_acl_access(&header[i].access_list);
			safe_free(new_string);
		}
	}
}

static void
free_http_header_access(header_mangler header[])
{
	int i;
	header_mangler **hdrp;
	for (i = 0; i < HDR_ENUM_END; i++)
	{
		free_acl_access(&header[i].access_list);
	}
	hdrp = &header[HDR_OTHER].next;
	while (*hdrp)
	{
		header_mangler *hdr = *hdrp;
		free_acl_access(&hdr->access_list);
		if (!hdr->replacement)
		{
			*hdrp = hdr->next;
			safe_free(hdr->name);
			safe_free(hdr);
		}
		else
		{
			hdrp = &hdr->next;
		}
	}
}

static void
dump_http_header_replace(StoreEntry * entry, const char *name, header_mangler
						 header[])
{
	int i;
	header_mangler *other;
	for (i = 0; i < HDR_ENUM_END; i++)
	{
		if (NULL == header[i].replacement)
			continue;
		storeAppendPrintf(entry, "%s %s %s\n", name, httpHeaderNameById(i),
						  header[i].replacement);
	}
	for (other = header[HDR_OTHER].next; other; other = other->next)
	{
		if (other->replacement == NULL)
			continue;
		storeAppendPrintf(entry, "%s %s %s\n", name, other->name, other->replacement);
	}
}

static void
parse_http_header_replace(header_mangler header[])
{
	int id, i;
	char *t = NULL;
	if ((t = strtok(NULL, w_space)) == NULL)
	{
		debug(3, 0) ("%s line %d: %s\n",
					 cfg_filename, config_lineno, config_input_line);
		debug(3, 0) ("parse_http_header_replace: missing header name.\n");
		return;
	}
	/* Now lookup index of header. */
	id = httpHeaderIdByNameDef(t, strlen(t));
	if (strcmp(t, "All") == 0)
		id = HDR_ENUM_END;
	else if (strcmp(t, "Other") == 0)
		id = HDR_OTHER;
	else if (id == -1)
	{
		header_mangler *hdr = header[HDR_OTHER].next;
		while (hdr && strcasecmp(hdr->name, t) != 0)
			hdr = hdr->next;
		if (!hdr)
		{
			hdr = xcalloc(1, sizeof *hdr);
			hdr->name = xstrdup(t);
			hdr->next = header[HDR_OTHER].next;
			header[HDR_OTHER].next = hdr;
		}
		if (hdr->replacement != NULL)
			safe_free(hdr->replacement);
		hdr->replacement = xstrdup(t + strlen(t) + 1);
		return;
	}
	if (id != HDR_ENUM_END)
	{
		if (header[id].replacement != NULL)
			safe_free(header[id].replacement);
		header[id].replacement = xstrdup(t + strlen(t) + 1);
	}
	else
	{
		for (i = 0; i < HDR_ENUM_END; i++)
		{
			if (header[i].replacement != NULL)
				safe_free(header[i].replacement);
			header[i].replacement = xstrdup(t + strlen(t) + 1);
		}
	}
}

static void
free_http_header_replace(header_mangler header[])
{
	int i;
	header_mangler **hdrp;
	for (i = 0; i < HDR_ENUM_END; i++)
	{
		if (header[i].replacement != NULL)
			safe_free(header[i].replacement);
	}
	hdrp = &header[HDR_OTHER].next;
	while (*hdrp)
	{
		header_mangler *hdr = *hdrp;
		free_acl_access(&hdr->access_list);
		if (!hdr->access_list)
		{
			*hdrp = hdr->next;
			safe_free(hdr->name);
			safe_free(hdr);
		}
		else
		{
			hdrp = &hdr->next;
		}
	}
}
#endif

void
dump_cachedir_options(StoreEntry * entry, struct cache_dir_option *options, SwapDir * sd)
{
	struct cache_dir_option *option;
	if (!options)
		return;
	for (option = options; option->name; option++)
		if (option->dump)
			option->dump(entry, option->name, sd);
}

static void
dump_cachedir(StoreEntry * entry, const char *name, cacheSwap swap)
{
	SwapDir *s;
	int i;
	for (i = 0; i < swap.n_configured; i++)
	{
		s = swap.swapDirs + i;
		storeAppendPrintf(entry, "%s %s %s", name, s->type, s->path);
		if (s->dump)
			s->dump(entry, s);
		dump_cachedir_options(entry, common_cachedir_options, s);
		storeAppendPrintf(entry, "\n");
	}
}

static int
check_null_cachedir(cacheSwap swap)
{
	return swap.swapDirs == NULL;
}

static int
check_null_string(char *s)
{
	return s == NULL;
}

static void
allocate_new_authScheme(authConfig * cfg)
{
	if (cfg->schemes == NULL)
	{
		cfg->n_allocated = 4;
		cfg->schemes = xcalloc(cfg->n_allocated, sizeof(authScheme));
	}
	if (cfg->n_allocated == cfg->n_configured)
	{
		authScheme *tmp;
		cfg->n_allocated <<= 1;
		tmp = xcalloc(cfg->n_allocated, sizeof(authScheme));
		xmemcpy(tmp, cfg->schemes, cfg->n_configured * sizeof(authScheme));
		xfree(cfg->schemes);
		cfg->schemes = tmp;
	}
}

static void
parse_authparam(authConfig * config)
{
	char *type_str;
	char *param_str;
	authScheme *scheme = NULL;
	int type, i;

	if ((type_str = strtok(NULL, w_space)) == NULL)
		self_destruct();

	if ((param_str = strtok(NULL, w_space)) == NULL)
		self_destruct();

	if ((type = authenticateAuthSchemeId(type_str)) == -1)
	{
		debug(3, 0) ("Parsing Config File: Unknown authentication scheme '%s'.\n", type_str);
		return;
	}
	for (i = 0; i < config->n_configured; i++)
	{
		if (config->schemes[i].Id == type)
		{
			scheme = config->schemes + i;
		}
	}

	if (scheme == NULL)
	{
		allocate_new_authScheme(config);
		scheme = config->schemes + config->n_configured;
		config->n_configured++;
		scheme->Id = type;
		scheme->typestr = authscheme_list[type].typestr;
	}
	authscheme_list[type].parse(scheme, config->n_configured, param_str);
}

static void
free_authparam(authConfig * cfg)
{
	authScheme *scheme;
	int i;
	/* DON'T FREE THESE FOR RECONFIGURE */
	if (reconfiguring)
		return;
	for (i = 0; i < cfg->n_configured; i++)
	{
		scheme = cfg->schemes + i;
		authscheme_list[scheme->Id].freeconfig(scheme);
	}
	safe_free(cfg->schemes);
	cfg->schemes = NULL;
	cfg->n_allocated = 0;
	cfg->n_configured = 0;
}

static void
dump_authparam(StoreEntry * entry, const char *name, authConfig cfg)
{
	authScheme *scheme;
	int i;
	for (i = 0; i < cfg.n_configured; i++)
	{
		scheme = cfg.schemes + i;
		authscheme_list[scheme->Id].dump(entry, name, scheme);
	}
}

void
allocate_new_swapdir(cacheSwap * swap)
{
	if (swap->swapDirs == NULL)
	{
		swap->n_allocated = 4;
		swap->swapDirs = xcalloc(swap->n_allocated, sizeof(SwapDir));
	}
	if (swap->n_allocated == swap->n_configured)
	{
		SwapDir *tmp;
		swap->n_allocated <<= 1;
		tmp = xcalloc(swap->n_allocated, sizeof(SwapDir));
		xmemcpy(tmp, swap->swapDirs, swap->n_configured * sizeof(SwapDir));
		xfree(swap->swapDirs);
		swap->swapDirs = tmp;
	}
}

static int
find_fstype(char *type)
{
	int i;
	for (i = 0; storefs_list[i].typestr != NULL; i++)
	{
		if (strcasecmp(type, storefs_list[i].typestr) == 0)
		{
			return i;
		}
	}
	return (-1);
}

static void
parse_cachedir(cacheSwap * swap)
{
	char *type_str;
#ifdef CC_FRAMEWORK
	char *path_str2 = NULL;
#endif
	char *path_str;
	SwapDir *sd;
	int i;
	int fs;

	if ((type_str = strtok(NULL, w_space)) == NULL)
		self_destruct();

#ifdef CC_MULTI_CORE
	if(opt_squid_id > 0)
	{
		if ((path_str2 = strtok(NULL, w_space)) == NULL)
			self_destruct();
		size_t path_str_len = sizeof(char)*strlen(path_str2) + strlen("/squid/") +4 ;//4=\0 + 3 bit of id
		path_str = xcalloc(1, path_str_len);
		snprintf(path_str, path_str_len - 1, "%s%s%d/", path_str2, "/squid", opt_squid_id);
	}
	else
	{
		if ((path_str = strtok(NULL, w_space)) == NULL)
			self_destruct();

	}
	
#else
	if ((path_str = strtok(NULL, w_space)) == NULL)
		self_destruct();
#endif
	fs = find_fstype(type_str);
	if (fs < 0)
		self_destruct();

	/* reconfigure existing dir */
	for (i = 0; i < swap->n_configured; i++)
	{
		if ((strcasecmp(path_str, swap->swapDirs[i].path) == 0))
		{
			sd = swap->swapDirs + i;
			if (sd->type != storefs_list[fs].typestr)
			{
				debug(3, 0) ("ERROR: Can't change type of existing cache_dir %s %s to %s. Restart required\n", sd->type, sd->path, type_str);
#ifdef CC_FRAMEWORK
				if(path_str2)
				{
					safe_free(path_str);
				}
#endif
				return;
			}
			storefs_list[fs].reconfigurefunc(sd, i, path_str);
			update_maxobjsize();
#ifdef CC_FRAMEWORK
			if(path_str2)
			{
				safe_free(path_str);
			}
#endif
			return;
		}
	}

	/* new cache_dir */
	assert(swap->n_configured < 63);	/* 7 bits, signed */

	allocate_new_swapdir(swap);
	sd = swap->swapDirs + swap->n_configured;
	sd->type = storefs_list[fs].typestr;
	/* defaults in case fs implementation fails to set these */
	sd->min_objsize = 0;
	sd->max_objsize = -1;
	sd->fs.blksize = 1024;
	/* parse the FS parameters and options */
	storefs_list[fs].parsefunc(sd, swap->n_configured, path_str);
	swap->n_configured++;
	/* Update the max object size */
	update_maxobjsize();
#ifdef CC_FRAMEWORK
	if(path_str2)
	{
		safe_free(path_str);
	}
#endif
}

static void
parse_cachedir_option_readonly(SwapDir * sd, const char *option, const char *value, int reconfiguring)
{
	int read_only = 0;
	if (value)
		read_only = xatoi(value);
	else
		read_only = 1;
	sd->flags.read_only = read_only;
}

static void
dump_cachedir_option_readonly(StoreEntry * e, const char *option, SwapDir * sd)
{
	if (sd->flags.read_only)
		storeAppendPrintf(e, " %s", option);
}

static void
parse_cachedir_option_minsize(SwapDir * sd, const char *option, const char *value, int reconfiguring)
{
	squid_off_t size;

	if (!value)
		self_destruct();

	size = strto_off_t(value, NULL, 10);

	if (reconfiguring && sd->min_objsize != size)
		debug(3, 1) ("Cache dir '%s' min object size now %ld\n", sd->path, (long int) size);

	sd->min_objsize = size;
}

static void
dump_cachedir_option_minsize(StoreEntry * e, const char *option, SwapDir * sd)
{
	if (sd->min_objsize != 0)
		storeAppendPrintf(e, " %s=%ld", option, (long int) sd->min_objsize);
}

static void
parse_cachedir_option_maxsize(SwapDir * sd, const char *option, const char *value, int reconfiguring)
{
	squid_off_t size;

	if (!value)
		self_destruct();

	size = strto_off_t(value, NULL, 10);

	if (reconfiguring && sd->max_objsize != size)
		debug(3, 1) ("Cache dir '%s' max object size now %ld\n", sd->path, (long int) size);

	sd->max_objsize = size;
}

static void
dump_cachedir_option_maxsize(StoreEntry * e, const char *option, SwapDir * sd)
{
	if (sd->max_objsize != -1)
		storeAppendPrintf(e, " %s=%ld", option, (long int) sd->max_objsize);
}

void
parse_cachedir_options(SwapDir * sd, struct cache_dir_option *options, int reconfiguring)
{
	int old_read_only = sd->flags.read_only;
	char *name, *value;
	struct cache_dir_option *option, *op;

	while ((name = strtok(NULL, w_space)) != NULL)
	{
		value = strchr(name, '=');
		if (value)
			*value++ = '\0';	/* cut on = */
		option = NULL;
		if (options)
		{
			for (op = options; !option && op->name; op++)
			{
				if (strcmp(op->name, name) == 0)
				{
					option = op;
					break;
				}
			}
		}
		for (op = common_cachedir_options; !option && op->name; op++)
		{
			if (strcmp(op->name, name) == 0)
			{
				option = op;
				break;
			}
		}
		if (!option || !option->parse)
			self_destruct();
		option->parse(sd, name, value, reconfiguring);
	}
	/*
	 * Handle notifications about reconfigured single-options with no value
	 * where the removal of the option cannot be easily detected in the
	 * parsing...
	 */
	if (reconfiguring)
	{
		if (old_read_only != sd->flags.read_only)
		{
			debug(3, 1) ("Cache dir '%s' now %s\n",
						 sd->path, sd->flags.read_only ? "No-Store" : "Read-Write");
		}
	}
}

static void
free_cachedir(cacheSwap * swap)
{
	SwapDir *s;
	int i;
	/* DON'T FREE THESE FOR RECONFIGURE */
	if (reconfiguring)
		return;
	for (i = 0; i < swap->n_configured; i++)
	{
		s = swap->swapDirs + i;
		s->freefs(s);
		xfree(s->path);
	}
	safe_free(swap->swapDirs);
	swap->swapDirs = NULL;
	swap->n_allocated = 0;
	swap->n_configured = 0;
}

static const char *
peer_type_str(const peer_t type)
{
	switch (type)
	{
	case PEER_PARENT:
		return "parent";
		break;
	case PEER_SIBLING:
		return "sibling";
		break;
	case PEER_MULTICAST:
		return "multicast";
		break;
	default:
		return "unknown";
		break;
	}
}

static void
dump_peer(StoreEntry * entry, const char *name, peer * p)
{
	domain_ping *d;
	domain_type *t;
	LOCAL_ARRAY(char, xname, 128);
	while (p != NULL)
	{
		storeAppendPrintf(entry, "%s %s %s %d %d",
						  name,
						  p->host,
						  neighborTypeStr(p),
						  p->http_port,
						  p->icp.port);
		dump_peer_options(entry, p);
		for (d = p->peer_domain; d; d = d->next)
		{
			storeAppendPrintf(entry, "cache_peer_domain %s %s%s\n",
							  p->name,
							  d->do_ping ? null_string : "!",
							  d->domain);
		}
		if (p->access)
		{
			snprintf(xname, 128, "cache_peer_access %s", p->name);
			dump_acl_access(entry, xname, p->access);
		}
		for (t = p->typelist; t; t = t->next)
		{
			storeAppendPrintf(entry, "neighbor_type_domain %s %s %s\n",
							  p->name,
							  peer_type_str(t->type),
							  t->domain);
		}
		p = p->next;
	}
}

static u_short
GetService(const char *proto)
{
	struct servent *port = NULL;
	char *token = strtok(NULL, w_space);
	if (token == NULL)
	{
		self_destruct();
		return -1;		/* NEVER REACHED */
	}
	port = getservbyname(token, proto);
	if (port != NULL)
	{
		return ntohs((u_short) port->s_port);
	}
	return xatos(token);
}

static u_short
GetTcpService(void)
{
	return GetService("tcp");
}

static u_short
GetUdpService(void)
{
	return GetService("udp");
}

static void
parse_peer(peer ** head)
{
	char *token = NULL;
	peer *p;
	p = cbdataAlloc(peer);
	p->http_port = CACHE_HTTP_PORT;
	p->icp.port = CACHE_ICP_PORT;
	p->weight = 1;
	p->stats.logged_state = PEER_ALIVE;
	p->monitor.state = PEER_ALIVE;
	p->monitor.interval = 300;
	p->tcp_up = PEER_TCP_MAGIC_COUNT;
	if ((token = strtok(NULL, w_space)) == NULL)
		self_destruct();
	p->host = xstrdup(token);
	p->name = xstrdup(token);
	if ((token = strtok(NULL, w_space)) == NULL)
		self_destruct();
	p->type = parseNeighborType(token);
	if (p->type == PEER_MULTICAST)
	{
		p->options.no_digest = 1;
		p->options.no_netdb_exchange = 1;
	}
	p->http_port = GetTcpService();
	if (!p->http_port)
		self_destruct();
	p->icp.port = GetUdpService();
	p->connection_auth = -1;	/* auto */
	while ((token = strtok(NULL, w_space)))
	{
		if (!strcasecmp(token, "proxy-only"))
		{
			p->options.proxy_only = 1;
		}
		else if (!strcasecmp(token, "no-query"))
		{
			p->options.no_query = 1;
		}
		else if (!strcasecmp(token, "no-digest"))
		{
			p->options.no_digest = 1;
		}
		else if (!strcasecmp(token, "multicast-responder"))
		{
			p->options.mcast_responder = 1;
#if PEER_MULTICAST_SIBLINGS
		}
		else if (!strcasecmp(token, "multicast-siblings"))
		{
			p->options.mcast_siblings = 1;
#endif
		}
		else if (!strncasecmp(token, "weight=", 7))
		{
			p->weight = xatoi(token + 7);
		}
		else if (!strcasecmp(token, "closest-only"))
		{
			p->options.closest_only = 1;
		}
		else if (!strncasecmp(token, "ttl=", 4))
		{
			p->mcast.ttl = xatoi(token + 4);
			if (p->mcast.ttl < 0)
				p->mcast.ttl = 0;
			if (p->mcast.ttl > 128)
				p->mcast.ttl = 128;
		}
		else if (!strcasecmp(token, "default"))
		{
			p->options.default_parent = 1;
		}
		else if (!strcasecmp(token, "round-robin"))
		{
			p->options.roundrobin = 1;
		}
		else if (!strcasecmp(token, "userhash"))
		{
			p->options.userhash = 1;
		}
		else if (!strcasecmp(token, "sourcehash"))
		{
			p->options.sourcehash = 1;
#if USE_HTCP
		}
		else if (!strcasecmp(token, "htcp"))
		{
			p->options.htcp = 1;
		}
		else if (!strcasecmp(token, "htcp-oldsquid"))
		{
			p->options.htcp = 1;
			p->options.htcp_oldsquid = 1;
#endif
		}
		else if (!strcasecmp(token, "no-netdb-exchange"))
		{
			p->options.no_netdb_exchange = 1;
#if USE_CARP
		}
		else if (!strcasecmp(token, "carp"))
		{
			if (p->type != PEER_PARENT)
				fatalf("parse_peer: non-parent carp peer %s (%s:%d)\n", p->name, p->host, p->http_port);
			p->options.carp = 1;
#endif
#if DELAY_POOLS
		}
		else if (!strcasecmp(token, "no-delay"))
		{
			p->options.no_delay = 1;
#endif
		}
		else if (!strncasecmp(token, "login=", 6))
		{
			p->login = xstrdup(token + 6);
			rfc1738_unescape(p->login);
		}
		else if (!strncasecmp(token, "connect-timeout=", 16))
		{
			p->connect_timeout = xatoi(token + 16);
#if USE_CACHE_DIGESTS
		}
		else if (!strncasecmp(token, "digest-url=", 11))
		{
			p->digest_url = xstrdup(token + 11);
#endif
		}
		else if (!strcasecmp(token, "allow-miss"))
		{
			p->options.allow_miss = 1;
		}
		else if (!strncasecmp(token, "max-conn=", 9))
		{
			p->max_conn = xatoi(token + 9);
		}
		else if (!strcasecmp(token, "originserver"))
		{
			p->options.originserver = 1;
		}
		else if (!strncasecmp(token, "name=", 5))
		{
			safe_free(p->name);
			if (token[5])
				p->name = xstrdup(token + 5);
		}
		else if (!strncasecmp(token, "monitorurl=", 11))
		{
			char *url = token + 11;
			safe_free(p->monitor.url);
			if (*url == '/')
			{
				int size = strlen("http://") + strlen(p->host) + 16 + strlen(url);
				p->monitor.url = xmalloc(size);
				snprintf(p->monitor.url, size, "http://%s:%d%s", p->host, p->http_port, url);
			}
			else
			{
				p->monitor.url = xstrdup(url);
			}
		}
		else if (!strncasecmp(token, "monitorsize=", 12))
		{
			char *token2 = strchr(token + 12, ',');
			if (!token2)
				token2 = strchr(token + 12, '-');
			if (token2)
				*token2++ = '\0';
			p->monitor.min = xatoi(token + 12);
			p->monitor.max = token2 ? xatoi(token2) : -1;
		}
		else if (!strncasecmp(token, "monitorinterval=", 16))
		{
			p->monitor.interval = xatoi(token + 16);
		}
		else if (!strncasecmp(token, "monitortimeout=", 15))
		{
			p->monitor.timeout = xatoi(token + 15);
		}
		else if (!strncasecmp(token, "forceddomain=", 13))
		{
			safe_free(p->domain);
			if (token[13])
				p->domain = xstrdup(token + 13);
#if USE_SSL
		}
		else if (strcmp(token, "ssl") == 0)
		{
			p->use_ssl = 1;
		}
		else if (strncmp(token, "sslcert=", 8) == 0)
		{
			safe_free(p->sslcert);
			p->sslcert = xstrdup(token + 8);
		}
		else if (strncmp(token, "sslkey=", 7) == 0)
		{
			safe_free(p->sslkey);
			p->sslkey = xstrdup(token + 7);
		}
		else if (strncmp(token, "sslversion=", 11) == 0)
		{
			p->sslversion = xatoi(token + 11);
		}
		else if (strncmp(token, "ssloptions=", 11) == 0)
		{
			safe_free(p->ssloptions);
			p->ssloptions = xstrdup(token + 11);
		}
		else if (strncmp(token, "sslcipher=", 10) == 0)
		{
			safe_free(p->sslcipher);
			p->sslcipher = xstrdup(token + 10);
		}
		else if (strncmp(token, "sslcafile=", 10) == 0)
		{
			safe_free(p->sslcafile);
			p->sslcafile = xstrdup(token + 10);
		}
		else if (strncmp(token, "sslcapath=", 10) == 0)
		{
			safe_free(p->sslcapath);
			p->sslcapath = xstrdup(token + 10);
		}
		else if (strncmp(token, "sslcrlfile=", 11) == 0)
		{
			safe_free(p->sslcrlfile);
			p->sslcrlfile = xstrdup(token + 11);
		}
		else if (strncmp(token, "sslflags=", 9) == 0)
		{
			safe_free(p->sslflags);
			p->sslflags = xstrdup(token + 9);
		}
		else if (strncmp(token, "ssldomain=", 10) == 0)
		{
			safe_free(p->ssldomain);
			p->ssldomain = xstrdup(token + 10);
#endif
		}
		else if (strcmp(token, "front-end-https=off") == 0)
		{
			p->front_end_https = 0;
		}
		else if (strcmp(token, "front-end-https") == 0)
		{
			p->front_end_https = 1;
		}
		else if (strcmp(token, "front-end-https=on") == 0)
		{
			p->front_end_https = 1;
		}
		else if (strcmp(token, "front-end-https=auto") == 0)
		{
			p->front_end_https = -1;
		}
		else if (strcmp(token, "connection-auth=off") == 0)
		{
			p->connection_auth = 0;
		}
		else if (strcmp(token, "connection-auth") == 0)
		{
			p->connection_auth = 1;
		}
		else if (strcmp(token, "connection-auth=on") == 0)
		{
			p->connection_auth = 1;
		}
		else if (strcmp(token, "connection-auth=auto") == 0)
		{
			p->connection_auth = -1;
		}
		else if (strncmp(token, "idle=", 5) == 0)
		{
			p->idle = xatoi(token + 5);
		}
		else if (strcmp(token, "http11") == 0)
		{
			p->options.http11 = 1;
		}
		else
		{
			debug(3, 0) ("parse_peer: token='%s'\n", token);
			self_destruct();
		}
	}
	if (peerFindByName(p->name))
		fatalf("ERROR: cache_peer %s specified twice\n", p->name);
	if (p->weight < 1)
		p->weight = 1;
	p->icp.version = ICP_VERSION_CURRENT;
	p->test_fd = -1;
#if USE_CACHE_DIGESTS
	if (!p->options.no_digest)
	{
		p->digest = peerDigestCreate(p);
		cbdataLock(p->digest);	/* so we know when/if digest disappears */
	}
#endif
#if USE_SSL
	if (p->use_ssl)
	{
		p->sslContext = sslCreateClientContext(p->sslcert, p->sslkey, p->sslversion, p->sslcipher, p->ssloptions, p->sslflags, p->sslcafile, p->sslcapath, p->sslcrlfile);
	}
#endif
	while (*head != NULL)
		head = &(*head)->next;
	*head = p;
	Config.npeers++;
	peerClearRRStart();
}

static void
free_peer(peer ** P)
{
	peer *p;
	while ((p = *P) != NULL)
	{
		*P = p->next;
#if USE_CACHE_DIGESTS
		if (p->digest)
		{
			PeerDigest *pd = p->digest;
			p->digest = NULL;
			peerDigestNotePeerGone(pd);
			cbdataUnlock(pd);
		}
#endif
		cbdataFree(p);
	}
	Config.npeers = 0;
}

static void
dump_cachemgrpasswd(StoreEntry * entry, const char *name, cachemgr_passwd * list)
{
	wordlist *w;
	while (list != NULL)
	{
		if (strcmp(list->passwd, "none") && strcmp(list->passwd, "disable"))
			storeAppendPrintf(entry, "%s XXXXXXXXXX", name);
		else
			storeAppendPrintf(entry, "%s %s", name, list->passwd);
		for (w = list->actions; w != NULL; w = w->next)
		{
			storeAppendPrintf(entry, " %s", w->key);
		}
		storeAppendPrintf(entry, "\n");
		list = list->next;
	}
}

static void
parse_cachemgrpasswd(cachemgr_passwd ** head)
{
	char *passwd = NULL;
	wordlist *actions = NULL;
	cachemgr_passwd *p;
	cachemgr_passwd **P;
	parse_string(&passwd);
	parse_wordlist(&actions);
	p = xcalloc(1, sizeof(cachemgr_passwd));
	p->passwd = passwd;
	p->actions = actions;
	for (P = head; *P; P = &(*P)->next)
	{
		/*
		 * See if any of the actions from this line already have a
		 * password from previous lines.  The password checking
		 * routines in cache_manager.c take the the password from
		 * the first cachemgr_passwd struct that contains the
		 * requested action.  Thus, we should warn users who might
		 * think they can have two passwords for the same action.
		 */
		wordlist *w;
		wordlist *u;
		for (w = (*P)->actions; w; w = w->next)
		{
			for (u = actions; u; u = u->next)
			{
				if (strcmp(w->key, u->key))
					continue;
				debug(0, 0) ("WARNING: action '%s' (line %d) already has a password\n",
							 u->key, config_lineno);
			}
		}
	}
	*P = p;
}

static void
free_cachemgrpasswd(cachemgr_passwd ** head)
{
	cachemgr_passwd *p;
	while ((p = *head) != NULL)
	{
		*head = p->next;
		xfree(p->passwd);
		wordlistDestroy(&p->actions);
		xfree(p);
	}
}

static void
dump_denyinfo(StoreEntry * entry, const char *name, acl_deny_info_list * var)
{
	acl_name_list *a;
	while (var != NULL)
	{
		storeAppendPrintf(entry, "%s %s", name, var->err_page_name);
		for (a = var->acl_list; a != NULL; a = a->next)
			storeAppendPrintf(entry, " %s", a->name);
		storeAppendPrintf(entry, "\n");
		var = var->next;
	}
}

static void
parse_denyinfo(acl_deny_info_list ** var)
{
	aclParseDenyInfoLine(var);
}

void
free_denyinfo(acl_deny_info_list ** list)
{
	acl_deny_info_list *a = NULL;
	acl_deny_info_list *a_next = NULL;
	acl_name_list *l = NULL;
	acl_name_list *l_next = NULL;
	for (a = *list; a; a = a_next)
	{
		for (l = a->acl_list; l; l = l_next)
		{
			l_next = l->next;
			memFree(l, MEM_ACL_NAME_LIST);
			l = NULL;
		}
		a_next = a->next;
		memFree(a, MEM_ACL_DENY_INFO_LIST);
		a = NULL;
	}
	*list = NULL;
}

static void
parse_peer_access(void)
{
	char *host = NULL;
	peer *p;
	if (!(host = strtok(NULL, w_space)))
		self_destruct();
	if ((p = peerFindByName(host)) == NULL)
	{
		debug(15, 0) ("%s, line %d: No cache_peer '%s'\n",
					  cfg_filename, config_lineno, host);
		return;
	}
	aclParseAccessLine(&p->access);
}

static void
parse_hostdomain(void)
{
	char *host = NULL;
	char *domain = NULL;
	if (!(host = strtok(NULL, w_space)))
		self_destruct();
	while ((domain = strtok(NULL, list_sep)))
	{
		domain_ping *l = NULL;
		domain_ping **L = NULL;
		peer *p;
		if ((p = peerFindByName(host)) == NULL)
		{
			debug(15, 0) ("%s, line %d: No cache_peer '%s'\n",
						  cfg_filename, config_lineno, host);
			continue;
		}
		l = xcalloc(1, sizeof(domain_ping));
		l->do_ping = 1;
		if (*domain == '!')  	/* check for !.edu */
		{
			l->do_ping = 0;
			domain++;
		}
		l->domain = xstrdup(domain);
		for (L = &(p->peer_domain); *L; L = &((*L)->next));
		*L = l;
	}
}

static void
parse_hostdomaintype(void)
{
	char *host = NULL;
	char *type = NULL;
	char *domain = NULL;
	if (!(host = strtok(NULL, w_space)))
		self_destruct();
	if (!(type = strtok(NULL, w_space)))
		self_destruct();
	while ((domain = strtok(NULL, list_sep)))
	{
		domain_type *l = NULL;
		domain_type **L = NULL;
		peer *p;
		if ((p = peerFindByName(host)) == NULL)
		{
			debug(15, 0) ("%s, line %d: No cache_peer '%s'\n",
						  cfg_filename, config_lineno, host);
			return;
		}
		l = xcalloc(1, sizeof(domain_type));
		l->type = parseNeighborType(type);
		l->domain = xstrdup(domain);
		for (L = &(p->typelist); *L; L = &((*L)->next));
		*L = l;
	}
}

#if UNUSED_CODE
static void
dump_ushortlist(StoreEntry * entry, const char *name, ushortlist * u)
{
	while (u)
	{
		storeAppendPrintf(entry, "%s %d\n", name, (int) u->i);
		u = u->next;
	}
}

static int
check_null_ushortlist(ushortlist * u)
{
	return u == NULL;
}

static void
parse_ushortlist(ushortlist ** P)
{
	char *token;
	u_short i;
	ushortlist *u;
	ushortlist **U;
	while ((token = strtok(NULL, w_space)))
	{
		i = GetShort();
		u = xcalloc(1, sizeof(ushortlist));
		u->i = i;
		for (U = P; *U; U = &(*U)->next);
		*U = u;
	}
}

static void
free_ushortlist(ushortlist ** P)
{
	ushortlist *u;
	while ((u = *P) != NULL)
	{
		*P = u->next;
		xfree(u);
	}
}
#endif

static void
dump_int(StoreEntry * entry, const char *name, int var)
{
	storeAppendPrintf(entry, "%s %d\n", name, var);
}

void
parse_int(int *var)
{
	int i;
	i = GetInteger();
	*var = i;
}

static void
free_int(int *var)
{
	*var = 0;
}

static void
dump_onoff(StoreEntry * entry, const char *name, int var)
{
	storeAppendPrintf(entry, "%s %s\n", name, var ? "on" : "off");
}

void
parse_onoff(int *var)
{
	char *token = strtok(NULL, w_space);

	if (token == NULL)
		self_destruct();
	if (!strcasecmp(token, "on") || !strcasecmp(token, "enable"))
		*var = 1;
	else
		*var = 0;
}

#define free_onoff free_int

static void
dump_tristate(StoreEntry * entry, const char *name, int var)
{
	const char *state;
	if (var > 0)
		state = "on";
	else if (var < 0)
		state = "warn";
	else
		state = "off";
	storeAppendPrintf(entry, "%s %s\n", name, state);
}

static void
parse_tristate(int *var)
{
	char *token = strtok(NULL, w_space);

	if (token == NULL)
		self_destruct();
	if (!strcasecmp(token, "on") || !strcasecmp(token, "enable"))
		*var = 1;
	else if (!strcasecmp(token, "warn"))
		*var = -1;
	else
		*var = 0;
}

#define free_tristate free_int

static void
dump_refreshpattern(StoreEntry * entry, const char *name, refresh_t * head)
{
	while (head != NULL)
	{
		storeAppendPrintf(entry, "%s%s %s %d %d%% %d\n",
						  name,
						  head->flags.icase ? " -i" : null_string,
						  head->pattern,
						  (int) head->min / 60,
						  (int) (100.0 * head->pct + 0.5),
						  (int) head->max / 60);
#if HTTP_VIOLATIONS
		if (head->flags.override_expire)
			storeAppendPrintf(entry, " override-expire");
		if (head->flags.override_lastmod)
			storeAppendPrintf(entry, " override-lastmod");
		if (head->flags.reload_into_ims)
			storeAppendPrintf(entry, " reload-into-ims");
		if (head->flags.ignore_reload)
			storeAppendPrintf(entry, " ignore-reload");
		if (head->flags.ignore_no_cache)
			storeAppendPrintf(entry, " ignore-no-cache");
		if (head->flags.ignore_private)
			storeAppendPrintf(entry, " ignore-private");
		if (head->flags.ignore_auth)
			storeAppendPrintf(entry, " ignore-auth");
		if (head->stale_while_revalidate > 0)
			storeAppendPrintf(entry, " stale-while-revalidate=%d", head->stale_while_revalidate);
#endif
		if (head->flags.ignore_stale_while_revalidate)
			storeAppendPrintf(entry, " ignore-stale-while-revalidate");
		if (head->max_stale >= 0)
			storeAppendPrintf(entry, " max-stale=%d", head->max_stale);
		if (head->negative_ttl >= 0)
			storeAppendPrintf(entry, " negative-ttl=%d", head->negative_ttl);
		storeAppendPrintf(entry, "\n");
		head = head->next;
	}
}

static void
parse_refreshpattern(refresh_t ** head)
{
	char *token;
	char *pattern;
	time_t min = 0;
	double pct = 0.0;
	time_t max = 0;
#if HTTP_VIOLATIONS
	int override_expire = 0;
	int override_lastmod = 0;
	int reload_into_ims = 0;
	int ignore_reload = 0;
	int ignore_no_cache = 0;
	int ignore_private = 0;
	int ignore_auth = 0;
#endif
	int stale_while_revalidate = -1;
	int ignore_stale_while_revalidate = 0;
	int max_stale = -1;
	int negative_ttl = -1;
	int i;
	refresh_t *t;
	regex_t comp;
	int errcode;
	int flags = REG_EXTENDED | REG_NOSUB;
	if ((token = strtok(NULL, w_space)) == NULL)
		self_destruct();
	if (strcmp(token, "-i") == 0)
	{
		flags |= REG_ICASE;
		token = strtok(NULL, w_space);
	}
	else if (strcmp(token, "+i") == 0)
	{
		flags &= ~REG_ICASE;
		token = strtok(NULL, w_space);
	}
	if (token == NULL)
		self_destruct();
#ifdef CC_FRAMEWORK
	if ('@' == token[0])
		token[0] = '^';
#endif
	pattern = xstrdup(token);
	i = GetInteger();		/* token: min */
	min = (time_t) (i * 60);	/* convert minutes to seconds */
	//min = min(min,INT_MAX/2);
	min = (min > INT_MAX/2)?INT_MAX/2:min;
	i = GetInteger();		/* token: pct */
	pct = (double) i / 100.0;
	i = GetInteger();		/* token: max */
	max = (time_t) (i * 60);	/* convert minutes to seconds */
	max = (max > INT_MAX/2)?INT_MAX/2:max;
	/* Options */
	while ((token = strtok(NULL, w_space)) != NULL)
	{
#if HTTP_VIOLATIONS
		if (!strcmp(token, "override-expire"))
			override_expire = 1;
		else if (!strcmp(token, "override-lastmod"))
			override_lastmod = 1;
		else if (!strcmp(token, "ignore-no-cache"))
			ignore_no_cache = 1;
		else if (!strcmp(token, "ignore-private"))
			ignore_private = 1;
		else if (!strcmp(token, "ignore-auth"))
			ignore_auth = 1;
		else if (!strcmp(token, "reload-into-ims"))
		{
			reload_into_ims = 1;
			refresh_nocache_hack = 1;
			/* tell client_side.c that this is used */
		}
		else if (!strcmp(token, "ignore-reload"))
		{
			ignore_reload = 1;
			refresh_nocache_hack = 1;
			/* tell client_side.c that this is used */
		}
		else if (!strncmp(token, "stale-while-revalidate=", 23))
		{
			stale_while_revalidate = atoi(token + 23);
		}
		else
#endif
			if (!strncmp(token, "max-stale=", 10))
			{
				max_stale = atoi(token + 10);
			}
			else if (!strncmp(token, "negative-ttl=", 13))
			{
				negative_ttl = atoi(token + 13);
			}
			else if (!strcmp(token, "ignore-stale-while-revalidate"))
			{
				ignore_stale_while_revalidate = 1;
			}
			else
			{
				debug(22, 0) ("parse_refreshpattern: Unknown option '%s': %s\n",
							  pattern, token);
			}
	}
	if ((errcode = regcomp(&comp, pattern, flags)) != 0)
	{
		char errbuf[256];
		regerror(errcode, &comp, errbuf, sizeof errbuf);
		debug(22, 0) ("%s line %d: %s\n",
					  cfg_filename, config_lineno, config_input_line);
		debug(22, 0) ("parse_refreshpattern: Invalid regular expression '%s': %s\n",
					  pattern, errbuf);
        regfree(&comp);
		return;
	}

	pct = pct < 0.0 ? 0.0 : pct;
	max = max < 0 ? 0 : max;
	t = xcalloc(1, sizeof(refresh_t));
//#ifdef CC_FRAMEWROK	
	t->regex_flags = flags;
//#endif
	t->pattern = (char *) xstrdup(pattern);
	t->compiled_pattern = comp;
	t->min = min;
	t->pct = pct;
	t->max = max;
	if (flags & REG_ICASE)
		t->flags.icase = 1;
#if HTTP_VIOLATIONS
	if (override_expire)
		t->flags.override_expire = 1;
	if (override_lastmod)
		t->flags.override_lastmod = 1;
	if (reload_into_ims)
		t->flags.reload_into_ims = 1;
	if (ignore_reload)
		t->flags.ignore_reload = 1;
	if (ignore_no_cache)
		t->flags.ignore_no_cache = 1;
	if (ignore_private)
		t->flags.ignore_private = 1;
	if (ignore_auth)
		t->flags.ignore_auth = 1;
#endif
	t->flags.ignore_stale_while_revalidate = ignore_stale_while_revalidate;
	t->stale_while_revalidate = stale_while_revalidate;
	t->max_stale = max_stale;
	t->negative_ttl = negative_ttl;
	t->next = NULL;
#ifdef CC_FRAMEWORK
	int slot = 0, find = 0;
	char domain[512] = "";
	access_array *array = NULL;
	if (reconfigure_in_thread)
	{
		strncpy(domain, Config_r.current_domain, 512);
		slot = myHash(domain, SQUID_CONFIG_ACL_BUCKET_NUM);
		array = Config_r.accessList2[slot];
	}
	else
	{
		strncpy(domain, Config.current_domain, 512);
		slot = myHash(domain, SQUID_CONFIG_ACL_BUCKET_NUM);
		array = Config.accessList2[slot];
	}
	while(array)
	{
		if(!strcmp(array->domain, domain))
		{       
			find = 1;    
			refresh_t *new = xcalloc(1, sizeof(refresh_t));
			cc_copy_refresh(t, new);
			if (NULL == array->refresh)
			{
				array->refresh = new;
				break;
			}
			refresh_t *end = array->refresh;
			while (NULL != end->next)
			{
				end = end->next;
			}
			end->next = new;
			break;  
		}       
		array = array->next;
	}
	if(!find)
	{
		array = xcalloc(1, sizeof(access_array));
		array->domain = xstrdup(domain);
		refresh_t *new = xcalloc(1, sizeof(refresh_t));
		cc_copy_refresh(t, new);
		array->refresh = new;
		if (reconfigure_in_thread)
		{
			array->next = Config_r.accessList2[slot];
			Config_r.accessList2[slot] = array;
		}
		else
		{
			array->next = Config.accessList2[slot];
			Config.accessList2[slot] = array;
		}
	}
#endif
	while (*head)
		head = &(*head)->next;
	*head = t;
	safe_free(pattern);
}

#if UNUSED_CODE
	static int
check_null_refreshpattern(refresh_t * data)
{
	return data == NULL;
}
#endif

static void
free_refreshpattern(refresh_t ** head)
{
	refresh_t *t;
	while ((t = *head) != NULL)
	{
		*head = t->next;
		safe_free(t->pattern);
		regfree(&t->compiled_pattern);
		safe_free(t);
	}
}

static void
dump_string(StoreEntry * entry, const char *name, char *var)
{
	if (var != NULL)
		storeAppendPrintf(entry, "%s %s\n", name, var);
}

static void
dump_string_with_id(StoreEntry * entry, const char *name, char *var)
{
	if (var != NULL)
		storeAppendPrintf(entry, "%s %s\n", name, var);
}

static void
parse_string_with_id(char **var)
{
	char *token = strtok(NULL, w_space);
	safe_free(*var);
	if (token == NULL)
		self_destruct();

#ifdef CC_MULTI_CORE
	if(opt_squid_id < 0)
	{
		*var = xstrdup(token);
		return;
	}
	int len = strlen(token) + 5; //5 = \0 + . + 3bit of id
	*var = xcalloc(1, len);
	snprintf(*var, len - 1, "%s.%d", token, opt_squid_id);
#else
	*var = xstrdup(token);
#endif
}

static void
parse_string(char **var)
{
	char *token = strtok(NULL, w_space);
	safe_free(*var);
	if (token == NULL)
		self_destruct();
	*var = xstrdup(token);
}

static void
free_string_with_id(char **var)
{
	safe_free(*var);
}

static void
free_string(char **var)
{
	safe_free(*var);
}

void
parse_eol(char *volatile *var)
{
	unsigned char *token = (unsigned char *) strtok(NULL, null_string);
	safe_free(*var);
	if (token == NULL)
		self_destruct();
	while (*token && xisspace(*token))
		token++;
	if (!*token)
		self_destruct();
	*var = xstrdup((char *) token);
}

#define dump_eol dump_string
#define free_eol free_string


static void
dump_time_t(StoreEntry * entry, const char *name, time_t var)
{
	storeAppendPrintf(entry, "%s %d seconds\n", name, (int) var);
}

void
parse_time_t(time_t * var)
{
	parseTimeLine(var, T_SECOND_STR);
}

static void
free_time_t(time_t * var)
{
	*var = 0;
}

#if UNUSED_CODE
static void
dump_size_t(StoreEntry * entry, const char *name, squid_off_t var)
{
	storeAppendPrintf(entry, "%s %" PRINTF_OFF_T "\n", name, var);
}

#endif

static void
dump_b_size_t(StoreEntry * entry, const char *name, squid_off_t var)
{
	storeAppendPrintf(entry, "%s %" PRINTF_OFF_T " %s\n", name, var, B_BYTES_STR);
}

static void
dump_kb_size_t(StoreEntry * entry, const char *name, squid_off_t var)
{
	storeAppendPrintf(entry, "%s %" PRINTF_OFF_T " %s\n", name, var, B_KBYTES_STR);
}

static void
parse_b_size_t(squid_off_t * var)
{
	parseBytesLine(var, B_BYTES_STR);
}

CBDATA_TYPE(body_size);

static void
parse_body_size_t(dlink_list * bodylist)
{
	body_size *bs;
	CBDATA_INIT_TYPE(body_size);
	bs = cbdataAlloc(body_size);
	bs->maxsize = GetOffT();
	aclParseAccessLine(&bs->access_list);

	dlinkAddTail(bs, &bs->node, bodylist);
}

static void
dump_body_size_t(StoreEntry * entry, const char *name, dlink_list bodylist)
{
	body_size *bs;
	bs = (body_size *) bodylist.head;
	while (bs)
	{
		acl_list *l;
		acl_access *head = bs->access_list;
		while (head != NULL)
		{
			storeAppendPrintf(entry, "%s %" PRINTF_OFF_T " %s", name, bs->maxsize,
							  head->allow ? "Allow" : "Deny");
			for (l = head->acl_list; l != NULL; l = l->next)
			{
				storeAppendPrintf(entry, " %s%s",
								  l->op ? null_string : "!",
								  l->acl->name);
			}
			storeAppendPrintf(entry, "\n");
			head = head->next;
		}
		bs = (body_size *) bs->node.next;
	}
}

static void
free_body_size_t(dlink_list * bodylist)
{
	body_size *bs, *tempnode;
	bs = (body_size *) bodylist->head;
	while (bs)
	{
		bs->maxsize = 0;
		aclDestroyAccessList(&bs->access_list);
		tempnode = (body_size *) bs->node.next;
		dlinkDelete(&bs->node, bodylist);
		cbdataFree(bs);
		bs = tempnode;
	}
}

static int
check_null_body_size_t(dlink_list bodylist)
{
	return bodylist.head == NULL;
}


static void
parse_kb_size_t(squid_off_t * var)
{
	parseBytesLine(var, B_KBYTES_STR);
}

static void
free_size_t(squid_off_t * var)
{
	*var = 0;
}

#define free_b_size_t free_size_t
#define free_kb_size_t free_size_t
#define free_mb_size_t free_size_t
#define free_gb_size_t free_size_t

static void
dump_ushort(StoreEntry * entry, const char *name, u_short var)
{
	storeAppendPrintf(entry, "%s %d\n", name, var);
}

static void
free_ushort(u_short * u)
{
	*u = 0;
}

static void
parse_ushort(u_short * var)
{
	*var = GetShort();
}

static void
dump_wordlist(StoreEntry * entry, const char *name, const wordlist * list)
{
	while (list != NULL)
	{
		storeAppendPrintf(entry, "%s %s\n", name, list->key);
		list = list->next;
	}
}

void
parse_wordlist(wordlist ** list)
{
	char *token;
	char *t = strtok(NULL, "");
	while ((token = strwordtok(NULL, &t)))
		wordlistAdd(list, token);
}

static int
check_null_wordlist(wordlist * w)
{
	return w == NULL;
}

static int
check_null_acl_access(acl_access * a)
{
	return a == NULL;
}

#define free_wordlist wordlistDestroy

#define free_uri_whitespace free_int

static void
parse_uri_whitespace(int *var)
{
	char *token = strtok(NULL, w_space);
	if (token == NULL)
		self_destruct();
	if (!strcasecmp(token, "strip"))
		*var = URI_WHITESPACE_STRIP;
	else if (!strcasecmp(token, "deny"))
		*var = URI_WHITESPACE_DENY;
	else if (!strcasecmp(token, "allow"))
		*var = URI_WHITESPACE_ALLOW;
	else if (!strcasecmp(token, "encode"))
		*var = URI_WHITESPACE_ENCODE;
	else if (!strcasecmp(token, "chop"))
		*var = URI_WHITESPACE_CHOP;
	else
		self_destruct();
}


static void
dump_uri_whitespace(StoreEntry * entry, const char *name, int var)
{
	const char *s;
	if (var == URI_WHITESPACE_ALLOW)
		s = "allow";
	else if (var == URI_WHITESPACE_ENCODE)
		s = "encode";
	else if (var == URI_WHITESPACE_CHOP)
		s = "chop";
	else if (var == URI_WHITESPACE_DENY)
		s = "deny";
	else
		s = "strip";
	storeAppendPrintf(entry, "%s %s\n", name, s);
}

static void
free_removalpolicy(RemovalPolicySettings ** settings)
{
	if (!*settings)
		return;
	free_string(&(*settings)->type);
	free_wordlist(&(*settings)->args);
	safe_free(*settings);
}

static void
parse_removalpolicy(RemovalPolicySettings ** settings)
{
	if (*settings)
		free_removalpolicy(settings);
	*settings = xcalloc(1, sizeof(**settings));
	parse_string(&(*settings)->type);
	parse_wordlist(&(*settings)->args);
}

static void
dump_removalpolicy(StoreEntry * entry, const char *name, RemovalPolicySettings * settings)
{
	wordlist *args;
	storeAppendPrintf(entry, "%s %s", name, settings->type);
	args = settings->args;
	while (args)
	{
		storeAppendPrintf(entry, " %s", args->key);
		args = args->next;
	}
	storeAppendPrintf(entry, "\n");
}

static void
parse_errormap(errormap ** head)
{
	errormap *m = xcalloc(1, sizeof(*m));
	char *url = strtok(NULL, w_space);
	char *token;
	struct error_map_entry **tail = &m->map;
	if (!url)
		self_destruct();
	m->url = xstrdup(url);
	while ((token = strtok(NULL, w_space)))
	{
		struct error_map_entry *e = xcalloc(1, sizeof(*e));
		e->value = xstrdup(token);
		e->status = xatoi(token);
		if (!e->status)
			e->status = -errorPageId(token);
		if (!e->status)
			debug(15, 0) ("WARNING: Unknown errormap code: %s\n", token);
		*tail = e;
		tail = &e->next;
	}
	while (*head)
		head = &(*head)->next;
	*head = m;
}

static void
dump_errormap(StoreEntry * entry, const char *name, errormap * map)
{
	while (map)
	{
		struct error_map_entry *me;
		storeAppendPrintf(entry, "%s %s",
						  name, map->url);
		for (me = map->map; me; me = me->next)
			storeAppendPrintf(entry, " %s", me->value);
		storeAppendPrintf(entry, "\n");
		map = map->next;
	}
}

static void
free_errormap(errormap ** head)
{
	while (*head)
	{
		errormap *map = *head;
		*head = map->next;
		while (map->map)
		{
			struct error_map_entry *me = map->map;
			map->map = me->next;
			safe_free(me->value);
			safe_free(me);
		}
		safe_free(map->url);
		safe_free(map);
	}
}

#include "cf_parser.h"

peer_t
parseNeighborType(const char *s)
{
	if (!strcasecmp(s, "parent"))
		return PEER_PARENT;
	if (!strcasecmp(s, "neighbor"))
		return PEER_SIBLING;
	if (!strcasecmp(s, "neighbour"))
		return PEER_SIBLING;
	if (!strcasecmp(s, "sibling"))
		return PEER_SIBLING;
	if (!strcasecmp(s, "multicast"))
		return PEER_MULTICAST;
	debug(15, 0) ("WARNING: Unknown neighbor type: %s\n", s);
	return PEER_SIBLING;
}

#if USE_WCCPv2
static void
parse_sockaddr_in_list(sockaddr_in_list ** head)
{
	char *token;
	sockaddr_in_list *s;
	while ((token = strtok(NULL, w_space)))
	{
		s = xcalloc(1, sizeof(*s));
		if (!parse_sockaddr(token, &s->s))
			self_destruct();
		while (*head)
			head = &(*head)->next;
		*head = s;
	}
}

static void
dump_sockaddr_in_list(StoreEntry * e, const char *n, const sockaddr_in_list * s)
{
	while (s)
	{
		storeAppendPrintf(e, "%s %s:%d\n",
						  n,
						  inet_ntoa(s->s.sin_addr),
						  ntohs(s->s.sin_port));
		s = s->next;
	}
}

static void
free_sockaddr_in_list(sockaddr_in_list ** head)
{
	sockaddr_in_list *s;
	while ((s = *head) != NULL)
	{
		*head = s->next;
		xfree(s);
	}
}

#if UNUSED_CODE
static int
check_null_sockaddr_in_list(const sockaddr_in_list * s)
{
	return NULL == s;
}
#endif
#endif /* USE_WCCPv2 */

static void
parse_http_port_specification(http_port_list * s, char *token)
{
	char *host = NULL;
	const struct hostent *hp;
	unsigned short port = 0;
	char *t;
	s->name = xstrdup(token);
	if ((t = strchr(token, ':')))
	{
		/* host:port */
		host = token;
		*t = '\0';
		port = xatos(t + 1);
	}
	else
	{
		/* port */
		port = xatos(token);
	}
	if (port == 0)
		self_destruct();
#ifdef CC_MULTI_CORE
	if(opt_squid_id > 0)
	{
		port += opt_squid_id;
	}
#endif
	s->s.sin_port = htons(port);
	if (NULL == host)
		s->s.sin_addr = any_addr;
	else if (1 == safe_inet_addr(host, &s->s.sin_addr))
		(void) 0;
	else if ((hp = gethostbyname(host)))
	{
		/* dont use ipcache */
		s->s.sin_addr = inaddrFromHostent(hp);
		s->defaultsite = xstrdup(host);
	}
	else
		self_destruct();
}

static void
parse_http_port_option(http_port_list * s, char *token)
{
	if (strncmp(token, "defaultsite=", 12) == 0)
	{
		safe_free(s->defaultsite);
		s->defaultsite = xstrdup(token + 12);
		s->accel = 1;
	}
	else if (strncmp(token, "name=", 5) == 0)
	{
		safe_free(s->name);
		s->name = xstrdup(token + 5);
	}
	else if (strcmp(token, "transparent") == 0)
	{
		s->transparent = 1;
	}
	else if (strcmp(token, "vhost") == 0)
	{
		s->vhost = 1;
		s->accel = 1;
	}
	else if (strcmp(token, "vport") == 0)
	{
		s->vport = -1;
		s->accel = 1;
	}
	else if (strncmp(token, "vport=", 6) == 0)
	{
		s->vport = xatos(token + 6);
		s->accel = 1;
	}
	else if (strcmp(token, "accel") == 0)
	{
		s->accel = 1;
	}
	else if (strcmp(token, "no-connection-auth") == 0)
	{
		s->no_connection_auth = 1;
	}
	else if (strncmp(token, "urlgroup=", 9) == 0)
	{
		s->urlgroup = xstrdup(token + 9);
	}
	else if (strncmp(token, "protocol=", 9) == 0)
	{
		s->protocol = xstrdup(token + 9);
#if LINUX_TPROXY
	}
	else if (strcmp(token, "tproxy") == 0)
	{
		s->tproxy = 1;
		need_linux_tproxy = 1;
#endif
	}
	else if (strcmp(token, "act-as-origin") == 0)
	{
		s->act_as_origin = 1;
		s->accel = 1;
	}
	else if (strcmp(token, "allow-direct") == 0)
	{
		s->allow_direct = 1;
	}
	else if (strcmp(token, "http11") == 0)
	{
		s->http11 = 1;
	}
	else if (strcmp(token, "tcpkeepalive") == 0)
	{
		s->tcp_keepalive.enabled = 1;
	}
	else if (strncmp(token, "tcpkeepalive=", 13) == 0)
	{
		char *t = token + 13;
		s->tcp_keepalive.enabled = 1;
		s->tcp_keepalive.idle = atoi(t);
		t = strchr(t, ',');
		if (t)
		{
			t++;
			s->tcp_keepalive.interval = atoi(t);
			t = strchr(t, ',');
		}
		if (t)
		{
			t++;
			s->tcp_keepalive.timeout = atoi(t);
			t = strchr(t, ',');
		}
	}
	else
	{
		self_destruct();
	}
}

static void
verify_http_port_options(http_port_list * s)
{
	if (s->accel && s->transparent)
	{
		debug(28, 0) ("Can't be both a transparent proxy and web server accelerator on the same port\n");
		self_destruct();
	}
}

static void
free_generic_http_port_data(http_port_list * s)
{
	safe_free(s->name);
	safe_free(s->protocol);
	safe_free(s->defaultsite);
}

static void
cbdataFree_http_port(void *data)
{
	free_generic_http_port_data(data);
}


static void
parse_http_port_list(http_port_list ** head)
{
	CBDATA_TYPE(http_port_list);
	char *token;
	http_port_list *s;
	CBDATA_INIT_TYPE_FREECB(http_port_list, cbdataFree_http_port);
	token = strtok(NULL, w_space);
	if (!token)
		self_destruct();
	s = cbdataAlloc(http_port_list);
	s->protocol = xstrdup("http");
	parse_http_port_specification(s, token);
	/* parse options ... */
	while ((token = strtok(NULL, w_space)))
	{
		parse_http_port_option(s, token);
	}
	verify_http_port_options(s);
	while (*head)
		head = &(*head)->next;
	*head = s;
}

#ifdef CC_FRAMEWORK
static void
dump_generic_http_port_to_buf(char * cfg_line_buf, const http_port_list * s)
{
	LOCAL_ARRAY(char, buf, 4096);
	buf[0] = '\0';
	snprintf(cfg_line_buf, BUFSIZ, "%s %s:%d",
					  "http_port",
					  inet_ntoa(s->s.sin_addr),
					  ntohs(s->s.sin_port));
	if (s->transparent)
		strcat(cfg_line_buf, " transparent");
	if (s->accel)
		strcat(cfg_line_buf, " accel");
	if (s->defaultsite)
	{
		memset(buf, '\0', sizeof(buf));
		snprintf(buf, 4096, " defaultsite=%s", s->defaultsite);
		strcat(cfg_line_buf, buf);;
	}
	if (s->vhost)
		strcat(cfg_line_buf, " vhost");
	if (s->vport == -1)
		strcat(cfg_line_buf, " vport");
	else if (s->vport)
	{
		memset(buf, '\0', sizeof(buf));
		snprintf(buf, 4096, " vport=%d", s->vport);
		strcat(cfg_line_buf, buf);;
	}
	if (s->urlgroup)
	{
		memset(buf, '\0', sizeof(buf));
		snprintf(buf, 4096, " urlgroup=%s", s->urlgroup);
		strcat(cfg_line_buf, buf);;
	}
	if (s->protocol)
	{
		memset(buf, '\0', sizeof(buf));
		snprintf(buf, 4096, " protocol=%s", s->protocol);
		strcat(cfg_line_buf, buf);;
	}
	if (s->no_connection_auth)
		strcat(cfg_line_buf, " no-connection-auth");
#if LINUX_TPROXY
	if (s->tproxy)
		strcat(cfg_line_buf, " tproxy");
#endif
	if (s->http11)
		strcat(cfg_line_buf, " http11");
	if (s->tcp_keepalive.enabled)
	{
		if (s->tcp_keepalive.idle || s->tcp_keepalive.interval || s->tcp_keepalive.timeout)
		{
			memset(buf, '\0', sizeof(buf));
			snprintf(buf, 4096, " tcp_keepalive=%d,%d,%d", s->tcp_keepalive.idle, s->tcp_keepalive.interval, s->tcp_keepalive.timeout);
			strcat(cfg_line_buf, buf);;
		}
		else
		{
			strcat(cfg_line_buf, " tcp_keepalive");
		}
	}
}

#if USE_SSL
static void
dump_generic_https_port_to_buf(char * cfg_line_buf, const https_port_list * s)
{
	LOCAL_ARRAY(char, buf, 4096);
	buf[0] = '\0';

	dump_generic_http_port_to_buf(cfg_line_buf, &s->http);
	if (s->cert)
	{
		memset(buf, '\0', sizeof(buf));
		snprintf(buf, 4096, " cert=%s", s->cert);
		strcat(cfg_line_buf, buf);;
	}
	if (s->key)
	{
		memset(buf, '\0', sizeof(buf));
		snprintf(buf, 4096, " key=%s", s->key);
		strcat(cfg_line_buf, buf);
	}
	if (s->version)
	{
		memset(buf, '\0', sizeof(buf));
		snprintf(buf, 4096, " version=%d", s->version);
		strcat(cfg_line_buf, buf);
	}
	if (s->options)
	{
		memset(buf, '\0', sizeof(buf));
		snprintf(buf, 4096, " options=%s", s->options);
		strcat(cfg_line_buf, buf);
	}
	if (s->cipher)
	{
		memset(buf, '\0', sizeof(buf));
		snprintf(buf, 4096, " cipher=%s", s->cipher);
		strcat(cfg_line_buf, buf);
	}
	if (s->cafile)
	{
		memset(buf, '\0', sizeof(buf));
		snprintf(buf, 4096, " cafile=%s", s->cafile);
		strcat(cfg_line_buf, buf);
	}
	if (s->capath)
	{
		memset(buf, '\0', sizeof(buf));
		snprintf(buf, 4096, " capath=%s", s->capath);
		strcat(cfg_line_buf, buf);
	}
	if (s->crlfile)
	{
		memset(buf, '\0', sizeof(buf));
		snprintf(buf, 4096, " crlfile=%s", s->crlfile);
		strcat(cfg_line_buf, buf);
	}
	if (s->dhfile)
	{
		memset(buf, '\0', sizeof(buf));
		snprintf(buf, 4096, " dhparams=%s", s->dhfile);
		strcat(cfg_line_buf, buf);
	}
	if (s->sslflags)
	{
		memset(buf, '\0', sizeof(buf));
		snprintf(buf, 4096, " sslflags=%s", s->sslflags);
		strcat(cfg_line_buf, buf);
	}
}

static int 
diffHttpsPortConfig()
{
	char *http_port_lines[MAXHTTPPORTS];
	char *http_port_lines_r[MAXHTTPPORTS];
	int n = 0, m = 0, i = 0, j = 0;
	https_port_list * s;

	if (Config.Sockaddr.https == NULL && Config_r.Sockaddr.https == NULL)	
		return 0;
	if (Config.Sockaddr.https == NULL && Config_r.Sockaddr.https != NULL)	
		return 1;
	if (Config.Sockaddr.https != NULL && Config_r.Sockaddr.https == NULL)	
		return 1;
	
	s = Config.Sockaddr.https;
	while(s)
	{
		char *cfg_line_buf = xcalloc(BUFSIZ, sizeof(char));	
		dump_generic_https_port_to_buf(cfg_line_buf, s);
		http_port_lines[n] = cfg_line_buf;
		n++;
		s = (https_port_list *) s->http.next;
	}
	s = Config_r.Sockaddr.https;
	while(s)
	{
		char *cfg_line_buf = xcalloc(BUFSIZ, sizeof(char));	
		dump_generic_https_port_to_buf(cfg_line_buf, s);
		http_port_lines_r[m] = cfg_line_buf;
		m++;
		s = (https_port_list *) s->http.next;
	}
	
	if (m != n)
	{
		for (i = 0; i < n; i++)
		{
			safe_free(http_port_lines[i]);
		}
		for (j = 0; j < m; j++)
		{
			safe_free(http_port_lines_r[j]);
		}
		return 1;	
	}
	for (i = 0; i < m; i++)
	{
		for (j = 0; j < n; j++)
		{
			if (!strcmp(http_port_lines[i], http_port_lines_r[j]))
				break;
		}	
		if (j == n)
		{
			for (j = 0; j < n; j++)
			{
				safe_free(http_port_lines[j]);
				safe_free(http_port_lines_r[j]);
			}		
			return 1;
		}
	}	

	for (j = 0; j < n; j++)
	{
		safe_free(http_port_lines[j]);
		safe_free(http_port_lines_r[j]);
	}		
	return 0;
}
#endif

static int 
diffHttpPortConfig()
{
	char *http_port_lines[MAXHTTPPORTS];
	char *http_port_lines_r[MAXHTTPPORTS];
	int n = 0, m = 0, i = 0, j = 0;
	http_port_list * s;

	if (Config.Sockaddr.http ==  NULL && Config_r.Sockaddr.http == NULL)	
		return 0;
	if (Config.Sockaddr.http == NULL && Config_r.Sockaddr.http != NULL)	
		return 1;
	if (Config.Sockaddr.http != NULL && Config_r.Sockaddr.http == NULL)	
		return 1;
	
	for (s = Config.Sockaddr.http; s; s = s->next)
	{
		char *cfg_line_buf = xcalloc(BUFSIZ, sizeof(char));	
		dump_generic_http_port_to_buf(cfg_line_buf, s);
		http_port_lines[n] = cfg_line_buf;
		n++;
	}
	for (s = Config_r.Sockaddr.http; s; s = s->next)
	{
		char *cfg_line_buf = xcalloc(BUFSIZ, sizeof(char));	
		dump_generic_http_port_to_buf(cfg_line_buf, s);
		http_port_lines_r[m] = cfg_line_buf;
		m++;
	}
	
	if (m != n)
	{
		for (i = 0; i < n; i++)
		{
			safe_free(http_port_lines[i]);
		}
		for (j = 0; j < m; j++)
		{
			safe_free(http_port_lines_r[j]);
		}
		return 1;	
	}
	for (i = 0; i < m; i++)
	{
		for (j = 0; j < n; j++)
		{
			if (!strcmp(http_port_lines[i], http_port_lines_r[j]))
				break;
		}	
		if (j == n)
		{
			for (j = 0; j < n; j++)
			{
				safe_free(http_port_lines[j]);
				safe_free(http_port_lines_r[j]);
			}		
			return 1;
		}
	}	

	for (j = 0; j < n; j++)
	{
		safe_free(http_port_lines[j]);
		safe_free(http_port_lines_r[j]);
	}		
	return 0;
}

#endif

static void
dump_generic_http_port(StoreEntry * e, const char *n, const http_port_list * s)
{
	storeAppendPrintf(e, "%s %s:%d",
					  n,
					  inet_ntoa(s->s.sin_addr),
					  ntohs(s->s.sin_port));
	if (s->transparent)
		storeAppendPrintf(e, " transparent");
	if (s->accel)
		storeAppendPrintf(e, " accel");
	if (s->defaultsite)
		storeAppendPrintf(e, " defaultsite=%s", s->defaultsite);
	if (s->vhost)
		storeAppendPrintf(e, " vhost");
	if (s->vport == -1)
		storeAppendPrintf(e, " vport");
	else if (s->vport)
		storeAppendPrintf(e, " vport=%d", s->vport);
	if (s->urlgroup)
		storeAppendPrintf(e, " urlgroup=%s", s->urlgroup);
	if (s->protocol)
		storeAppendPrintf(e, " protocol=%s", s->protocol);
	if (s->no_connection_auth)
		storeAppendPrintf(e, " no-connection-auth");
#if LINUX_TPROXY
	if (s->tproxy)
		storeAppendPrintf(e, " tproxy");
#endif
	if (s->http11)
		storeAppendPrintf(e, " http11");
	if (s->tcp_keepalive.enabled)
	{
		if (s->tcp_keepalive.idle || s->tcp_keepalive.interval || s->tcp_keepalive.timeout)
		{
			storeAppendPrintf(e, " tcp_keepalive=%d,%d,%d", s->tcp_keepalive.idle, s->tcp_keepalive.interval, s->tcp_keepalive.timeout);
		}
		else
		{
			storeAppendPrintf(e, " tcp_keepalive");
		}
	}
}
static void
dump_http_port_list(StoreEntry * e, const char *n, const http_port_list * s)
{
	while (s)
	{
		dump_generic_http_port(e, n, s);
		storeAppendPrintf(e, "\n");
		s = s->next;
	}
}

static void
free_http_port_list(http_port_list ** head)
{
	http_port_list *s;
	while ((s = *head) != NULL)
	{
		*head = s->next;
		cbdataFree(s);
	}
}

#if UNUSED_CODE
static int
check_null_http_port_list(const http_port_list * s)
{
	return NULL == s;
}
#endif

#if USE_SSL
static void
cbdataFree_https_port(void *data)
{
	https_port_list *s = data;
	free_generic_http_port_data(&s->http);
	safe_free(s->cert);
	safe_free(s->key);
	safe_free(s->cipher);
	safe_free(s->options);
	safe_free(s->clientca);
	safe_free(s->cafile);
	safe_free(s->capath);
	safe_free(s->crlfile);
	safe_free(s->dhfile);
	safe_free(s->sslflags);
	safe_free(s->sslcontext);
	if (s->sslContext)
		SSL_CTX_free(s->sslContext);
	s->sslContext = NULL;
}

static void
parse_https_port_list(https_port_list ** head)
{
	CBDATA_TYPE(https_port_list);
	char *token;
	https_port_list *s;
	CBDATA_INIT_TYPE_FREECB(https_port_list, cbdataFree_https_port);
	token = strtok(NULL, w_space);
	if (!token)
		self_destruct();
	s = cbdataAlloc(https_port_list);
	s->http.protocol = xstrdup("https");
	parse_http_port_specification(&s->http, token);
	/* parse options ... */
	while ((token = strtok(NULL, w_space)))
	{
		if (strncmp(token, "cert=", 5) == 0)
		{
			safe_free(s->cert);
			s->cert = xstrdup(token + 5);
		}
		else if (strncmp(token, "key=", 4) == 0)
		{
			safe_free(s->key);
			s->key = xstrdup(token + 4);
		}
		else if (strncmp(token, "version=", 8) == 0)
		{
			s->version = xatoi(token + 8);
			if (s->version < 1 || s->version > 4)
				self_destruct();
		}
		else if (strncmp(token, "options=", 8) == 0)
		{
			safe_free(s->options);
			s->options = xstrdup(token + 8);
		}
		else if (strncmp(token, "cipher=", 7) == 0)
		{
			safe_free(s->cipher);
			s->cipher = xstrdup(token + 7);
		}
		else if (strncmp(token, "clientca=", 9) == 0)
		{
			safe_free(s->clientca);
			s->clientca = xstrdup(token + 9);
		}
		else if (strncmp(token, "cafile=", 7) == 0)
		{
			safe_free(s->cafile);
			s->cafile = xstrdup(token + 7);
		}
		else if (strncmp(token, "capath=", 7) == 0)
		{
			safe_free(s->capath);
			s->capath = xstrdup(token + 7);
		}
		else if (strncmp(token, "crlfile=", 8) == 0)
		{
			safe_free(s->crlfile);
			s->crlfile = xstrdup(token + 8);
		}
		else if (strncmp(token, "dhparams=", 9) == 0)
		{
			safe_free(s->dhfile);
			s->dhfile = xstrdup(token + 9);
		}
		else if (strncmp(token, "sslflags=", 9) == 0)
		{
			safe_free(s->sslflags);
			s->sslflags = xstrdup(token + 9);
		}
		else if (strncmp(token, "sslcontext=", 11) == 0)
		{
			safe_free(s->sslcontext);
			s->sslcontext = xstrdup(token + 11);
		}
		else
		{
			parse_http_port_option(&s->http, token);
		}
	}
	verify_http_port_options(&s->http);
	while (*head)
		head = (https_port_list **) (void *) (&(*head)->http.next);
	s->sslContext = sslCreateServerContext(s->cert, s->key, s->version, s->cipher, s->options, s->sslflags, s->clientca, s->cafile, s->capath, s->crlfile, s->dhfile, s->sslcontext);
#if WE_DONT_CARE_ABOUT_THIS_ERROR
	if (!s->sslContext)
		self_destruct();
#endif
	*head = s;
}

static void
dump_https_port_list(StoreEntry * e, const char *n, const https_port_list * s)
{
	while (s)
	{
		dump_generic_http_port(e, n, &s->http);
		if (s->cert)
			storeAppendPrintf(e, " cert=%s", s->cert);
		if (s->key)
			storeAppendPrintf(e, " key=%s", s->key);
		if (s->version)
			storeAppendPrintf(e, " version=%d", s->version);
		if (s->options)
			storeAppendPrintf(e, " options=%s", s->options);
		if (s->cipher)
			storeAppendPrintf(e, " cipher=%s", s->cipher);
		if (s->cafile)
			storeAppendPrintf(e, " cafile=%s", s->cafile);
		if (s->capath)
			storeAppendPrintf(e, " capath=%s", s->capath);
		if (s->crlfile)
			storeAppendPrintf(e, " crlfile=%s", s->crlfile);
		if (s->dhfile)
			storeAppendPrintf(e, " dhparams=%s", s->dhfile);
		if (s->sslflags)
			storeAppendPrintf(e, " sslflags=%s", s->sslflags);
		storeAppendPrintf(e, "\n");
		s = (https_port_list *) s->http.next;
	}
}

static void
free_https_port_list(https_port_list ** head)
{
	https_port_list *s;
	while ((s = *head) != NULL)
	{
		*head = (https_port_list *) s->http.next;
		cbdataFree(s);
	}
}

#if 0
static int
check_null_https_port_list(const https_port_list * s)
{
	return NULL == s;
}
#endif

#endif /* USE_SSL */

#ifdef CC_FRAMEWORK

static void cc_free_accessList(SquidConfig *config)     // added param config, by chenqi
{
	int i = 0;
	access_array *aa ,*bb;
	for(i = 0; i < SQUID_CONFIG_ACL_BUCKET_NUM; i++)
	{
		aa = config->accessList2[i];
		while(aa)
		{
            bb = aa->next;
			free_acl_access(&aa->http);
			free_acl_access(&aa->http2);
			free_acl_access(&aa->reply);
			free_acl_access(&aa->icp);
#if USE_HTCP
			free_acl_access(&aa->htcp);
#endif
#if USE_HTCP
			free_acl_access(&aa->htcp_clr);
#endif
			free_acl_access(&aa->miss);
#if USE_IDENT
			free_acl_access(&aa->identLookup);
#endif
			free_acl_access(&aa->auth_ip_shortcircuit);
#if FOLLOW_X_FORWARDED_FOR
			free_acl_access(&aa->followXFF);
#endif
			free_acl_access(&aa->log);
			free_acl_access(&aa->url_rewrite);
			free_acl_access(&aa->storeurl_rewrite);
			free_acl_access(&aa->location_rewrite);
			free_acl_access(&aa->noCache);
			free_acl_access(&aa->brokenPosts);
			free_acl_access(&aa->upgrade_http09);
			free_acl_access(&aa->vary_encoding);
#if SQUID_SNMP
			free_acl_access(&aa->snmp);
#endif
			free_acl_access(&aa->AlwaysDirect);
			free_acl_access(&aa->NeverDirect);
            free_acl_tos(&aa->outgoing_tos);
            free_acl_address(&aa->outgoing_address);
			free_refreshpattern(&aa->refresh);
			safe_free(aa->domain);
			safe_free(aa);
			aa = bb;
		}
	}
}

// added by chenqi
// this function used to free Config_r and cc_modules_r after reload
void configFreeMemory_r(void)      
{
    // free Config_r
    free_all_r();
    cc_free_accessList(&Config_r);       // free Config_r.accessList2
    safe_free(Config_r.current_domain);  // free Config_r.current_domain
#if USE_SSL
    if (Config_r.ssl_client.sslContext)
        SSL_CTX_free(Config_r.ssl_client.sslContext);
    Config_r.ssl_client.sslContext = NULL;
#endif
    //initializeSquidConfig(&Config_r); // initialize Config_r
}
// added end

#endif

#ifdef CC_FRAMEWORK
/*
void free_pan_domain_node(void *data)
{
	hash_link *hl = data;
	pan_domain_access *tmp = (pan_domain_access *) hl;
	access_array *cur = tmp->add;

	if (strcmp(cur->domain, "common"))
	{
		free_acl_access(&cur->http);
		free_acl_access(&cur->http2);
		free_acl_access(&cur->reply);
		free_acl_access(&cur->icp);
#if USE_HTCP
		free_acl_access(&cur->htcp);
#endif
#if USE_HTCP
		free_acl_access(&cur->htcp_clr);
#endif
		free_acl_access(&cur->miss);
#if USE_IDENT
		free_acl_access(&cur->identLookup);
#endif
		free_acl_access(&cur->auth_ip_shortcircuit);
#if FOLLOW_X_FORWARDED_FOR
		free_acl_access(&cur->followXFF);
#endif
		free_acl_access(&cur->log);
		free_acl_access(&cur->url_rewrite);
		free_acl_access(&cur->storeurl_rewrite);
		free_acl_access(&cur->location_rewrite);
		free_acl_access(&cur->noCache);
		free_acl_access(&cur->brokenPosts);
		free_acl_access(&cur->upgrade_http09);
		free_acl_access(&cur->vary_encoding);
#if SQUID_SNMP
		free_acl_access(&cur->snmp);
#endif
		free_acl_access(&cur->AlwaysDirect);
		free_acl_access(&cur->NeverDirect);
		safe_free(cur->domain);
		safe_free(cur);
	}

	xfree(tmp);
}
*/
#endif

#ifdef CC_FRAMEWORK
typedef struct
{
    access_array *to_free_tab[ SQUID_CONFIG_ACL_BUCKET_NUM] ;
    int           to_free_idx;
}free_accessList2_t;

CBDATA_TYPE(free_accessList2_t);
void cc_free_accessList_done(free_accessList2_t *accessList2_to_free)
{
    cbdataFree(accessList2_to_free);
    return;
}

void cc_free_accessList_next(free_accessList2_t *accessList2_to_free)
{
    int idx;

    for(idx = accessList2_to_free->to_free_idx; idx < SQUID_CONFIG_ACL_BUCKET_NUM; idx ++)
    {
        access_array *to_free_access_array;
        to_free_access_array = accessList2_to_free->to_free_tab[ idx ];
        accessList2_to_free->to_free_tab[ idx ] = NULL;
        if(NULL != to_free_access_array)
        {
            free_acl_access_array(to_free_access_array);
            
            /*note:cc_free_accessList_next handle one at once!*/
            idx ++;/*before break, idx move one step forward*/
            break; 
        }
    }

    if(Config.onoff.debug_80_log)
    {
        debug(3,0)("%s: free from %d to %d\n", __func__, accessList2_to_free->to_free_idx, idx);    
    }

    accessList2_to_free->to_free_idx = idx;

    if(accessList2_to_free->to_free_idx >= SQUID_CONFIG_ACL_BUCKET_NUM)
    {
        /*end*/
        cc_free_accessList_done(accessList2_to_free);
        return;
    }

    eventAdd("cc_free_accessList_next", (EVH *)cc_free_accessList_next, accessList2_to_free, g_b4AccessFreeWaitNsec, 0);

    return;
}

void cc_free_accessList_start(SquidConfig *config)
{
    int idx;
    free_accessList2_t *accessList2_to_free;
    CBDATA_INIT_TYPE(free_accessList2_t);
    accessList2_to_free = cbdataAlloc(free_accessList2_t);
    for(idx = 0; idx < SQUID_CONFIG_ACL_BUCKET_NUM; idx ++)
    {
        accessList2_to_free->to_free_tab[ idx ] = config->accessList2[ idx ];
        config->accessList2[ idx ] = NULL;
    }
    accessList2_to_free->to_free_idx = 0;


    eventAdd("cc_free_accessList_next", (EVH *)cc_free_accessList_next, accessList2_to_free, g_b4AccessFreeWaitNsec, 0);
    
    return;
}
#endif

void configFreeMemory(void)      // free Config and cc_modules
{
    // free Config
    if(Config.onoff.debug_80_log)
    {
        debug(3,0)("%s: free_all beg\n", __func__);    
    }
	free_all();
	if(Config.onoff.debug_80_log)
	{
    	debug(3,0)("%s: free_all end\n", __func__);    
	}

#ifndef CC_FRAMEWORK
    if(Config.onoff.debug_80_log)
    {
    	debug(3,0)("%s: cc_free_accessList beg\n", __func__);    
	}
	
	cc_free_accessList(&Config);     // free Config.accessList2
	
	if(Config.onoff.debug_80_log)
	{
    	debug(3,0)("%s: cc_free_accessList end\n", __func__);    
	}
#endif
#ifdef CC_FRAMEWORK
    cc_free_accessList_start(&Config);
#endif

    safe_free(Config.current_domain);// free Config.current_domain
#if USE_SSL
	if (Config.ssl_client.sslContext)
		SSL_CTX_free(Config.ssl_client.sslContext);
	Config.ssl_client.sslContext = NULL;
#endif
#ifdef CC_FRAMEWORK
    //initializeSquidConfig(&Config); // initialize Config
#endif
}

void
requirePathnameExists(const char *name, const char *path)
{
	struct stat sb;
	char pathbuf[BUFSIZ];
	assert(path != NULL);
	if (Config.chroot_dir && (geteuid() == 0))
	{
		snprintf(pathbuf, BUFSIZ, "%s/%s", Config.chroot_dir, path);
		path = pathbuf;
	}
	if (stat(path, &sb) < 0)
	{
		if ((opt_send_signal == -1 || opt_send_signal == SIGHUP) && !opt_parse_cfg_only)
			fatalf("%s %s: %s", name, path, xstrerror());
		else
			fprintf(stderr, "WARNING: %s %s: %s\n", name, path, xstrerror());
	}
}

char *
strtokFile(void)
{
	static int fromFile = 0;
	static FILE *wordFile = NULL;

	char *t, *fn;
	LOCAL_ARRAY(char, buf, 256);

strtok_again:
	if (!fromFile)
	{
		t = (strtok(NULL, w_space));
		if (!t || *t == '#')
		{
			return NULL;
		}
		else if (*t == '\"' || *t == '\'')
		{
			/* quote found, start reading from file */
			fn = ++t;
			while (*t && *t != '\"' && *t != '\'')
				t++;
			*t = '\0';
			if ((wordFile = fopen(fn, "r")) == NULL)
			{
				debug(28, 0) ("strtokFile: %s not found\n", fn);
				return (NULL);
			}
#ifdef _SQUID_WIN32_
			setmode(fileno(wordFile), O_TEXT);
#endif
			fromFile = 1;
		}
		else
		{
			return t;
		}
	}
	/* fromFile */
	if (fgets(buf, 256, wordFile) == NULL)
	{
		/* stop reading from file */
		fclose(wordFile);
		wordFile = NULL;
		fromFile = 0;
		goto strtok_again;
	}
	else
	{
		char *t2, *t3;
		t = buf;
		/* skip leading and trailing white space */
		t += strspn(buf, w_space);
		t2 = t + strcspn(t, w_space);
		t3 = t2 + strspn(t2, w_space);
		while (*t3 && *t3 != '#')
		{
			t2 = t3 + strcspn(t3, w_space);
			t3 = t2 + strspn(t2, w_space);
		}
		*t2 = '\0';
		/* skip comments */
		if (*t == '#')
			goto strtok_again;
		/* skip blank lines */
		if (!*t)
			goto strtok_again;
		return t;
	}
}

static void
parse_logformat(logformat ** logformat_definitions)
{
	logformat *nlf;
	char *name, *def;

	if ((name = strtok(NULL, w_space)) == NULL)
		self_destruct();
	if ((def = strtok(NULL, "\r\n")) == NULL)
		self_destruct();

	debug(3, 1) ("Logformat for '%s' is '%s'\n", name, def);

	nlf = xcalloc(1, sizeof(logformat));
	nlf->name = xstrdup(name);
	if (!accessLogParseLogFormat(&nlf->format, def))
		self_destruct();
	nlf->next = *logformat_definitions;
	*logformat_definitions = nlf;
}

static void
parse_access_log(customlog ** logs)
{
	const char *filename, *logdef_name;
	customlog *cl;
	logformat *lf;

	cl = xcalloc(1, sizeof(*cl));

	if ((filename = strtok(NULL, w_space)) == NULL)
		self_destruct();

	if (strcmp(filename, "none") == 0)
	{
		cl->type = CLF_NONE;
		goto done;
	}
	if ((logdef_name = strtok(NULL, w_space)) == NULL)
		logdef_name = "auto";

	debug(3, 9) ("Log definition name '%s' file '%s'\n", logdef_name, filename);
/*#ifdef CC_FRAMEWORK_CHANGE
	if(opt_squid_id < 0)
	{
		cl->filename = xstrdup(filename);
	}
	else
	{
		int len = strlen(filename) + 5; //5 = \0 + . + 3bit of id
		cl->filename = xcalloc(1, len);
		snprintf(cl->filename, len, "%s.%d", filename, opt_squid_id);
	}

#else
*/
	cl->filename = xstrdup(filename);
//#endif

	/* look for the definition pointer corresponding to this name */
	if(reconfigure_in_thread)
		lf=Config_r.Log.logformats;
	else
		lf = Config.Log.logformats;
	while (lf != NULL)
	{
		debug(3, 9) ("Comparing against '%s'\n", lf->name);
		if (strcmp(lf->name, logdef_name) == 0)
			break;
		lf = lf->next;
	}
	if (lf != NULL)
	{
		cl->type = CLF_CUSTOM;
		cl->logFormat = lf;
	}
	else if (strcmp(logdef_name, "auto") == 0)
	{
		cl->type = CLF_AUTO;
	}
	else if (strcmp(logdef_name, "squid") == 0)
	{
		cl->type = CLF_SQUID;
	}
	else if (strcmp(logdef_name, "common") == 0)
	{
		cl->type = CLF_COMMON;
	}
	else
	{
		debug(3, 0) ("Log format '%s' is not defined\n", logdef_name);
		self_destruct();
	}

done:
	aclParseAclList(&cl->aclList);

	while (*logs)
		logs = &(*logs)->next;
	*logs = cl;
}

static void
dump_logformat(StoreEntry * entry, const char *name, logformat * definitions)
{
	accessLogDumpLogFormat(entry, name, definitions);
}

static void
dump_access_log(StoreEntry * entry, const char *name, customlog * logs)
{
	customlog *log;
	for (log = logs; log; log = log->next)
	{
		storeAppendPrintf(entry, "%s ", name);
		switch (log->type)
		{
		case CLF_CUSTOM:
			storeAppendPrintf(entry, "%s %s", log->filename, log->logFormat->name);
			break;
		case CLF_NONE:
			storeAppendPrintf(entry, "none");
			break;
		case CLF_SQUID:
			storeAppendPrintf(entry, "%s squid", log->filename);
			break;
		case CLF_COMMON:
			storeAppendPrintf(entry, "%s squid", log->filename);
			break;
		case CLF_AUTO:
			if (log->aclList)
				storeAppendPrintf(entry, "%s auto", log->filename);
			else
				storeAppendPrintf(entry, "%s", log->filename);
			break;
		case CLF_UNKNOWN:
			break;
		}
		if (log->aclList)
			dump_acl_list(entry, log->aclList);
		storeAppendPrintf(entry, "\n");
	}
}

static void
free_logformat(logformat ** definitions)
{
	while (*definitions)
	{
		logformat *format = *definitions;
		*definitions = format->next;
		accessLogFreeLogFormat(&format->format);
		xfree(format);
	}
}

static void
free_access_log(customlog ** definitions)
{
	while (*definitions)
	{
		customlog *log = *definitions;
		*definitions = log->next;

		log->logFormat = NULL;
		log->type = CLF_UNKNOWN;
		if (log->aclList)
			aclDestroyAclList(&log->aclList);
		safe_free(log->filename);
		xfree(log);
	}
}

static void
parse_programline(wordlist ** line)
{
	if (*line)
		self_destruct();
	parse_wordlist(line);
}

static void
free_programline(wordlist ** line)
{
	free_wordlist(line);
}

static void
dump_programline(StoreEntry * entry, const char *name, const wordlist * line)
{
	dump_wordlist(entry, name, line);
}

static void
parse_zph_mode(enum zph_mode *mode)
{
	char *token = strtok(NULL, w_space);
	if (!token)
		self_destruct();
	if (strcmp(token, "off") == 0)
		*mode = ZPH_OFF;
	else if (strcmp(token, "tos") == 0)
		*mode = ZPH_TOS;
	else if (strcmp(token, "priority") == 0)
		*mode = ZPH_PRIORITY;
	else if (strcmp(token, "option") == 0)
		*mode = ZPH_OPTION;
	else
	{
		debug(3, 0) ("WARNING: unsupported zph_mode argument '%s'\n", token);
	}
}

static void
dump_zph_mode(StoreEntry * entry, const char *name, enum zph_mode mode)
{
	const char *modestr = "unknown";
	switch (mode)
	{
	case ZPH_OFF:
		modestr = "off";
		break;
	case ZPH_TOS:
		modestr = "tos";
		break;
	case ZPH_PRIORITY:
		modestr = "priority";
		break;
	case ZPH_OPTION:
		modestr = "option";
		break;
	}
	storeAppendPrintf(entry, "%s %s\n", name, modestr);
}

static void
free_zph_mode(enum zph_mode *mode)
{
	*mode = ZPH_OFF;
}
#ifdef CC_FRAMEWORK
static void
defaults_if_none_r(int first_time)
{
//	if (check_null_acl_access(Config_r.accessList.http)) {
	if(check_null_acl_access(cc_get_common_acl_access(first_time, "http"))){
		default_line("http_access deny all");
	}
	if(check_null_acl_access(cc_get_common_acl_access(first_time, "reply"))){
	//if (check_null_acl_access(Config_r.accessList.reply)) {
		default_line("http_reply_access allow all");
	}
	if(check_null_acl_access(cc_get_common_acl_access(first_time, "icp"))){
	//if (check_null_acl_access(Config_r.accessList.icp)) {
		default_line("icp_access deny all");
	}
#if USE_HTCP
	if(check_null_acl_access(cc_get_common_acl_access(first_time, "htcp"))){
	//if (check_null_acl_access(Config_r.accessList.htcp)) {
		default_line("htcp_access deny all");
	}
#endif
#if USE_HTCP
	if(check_null_acl_access(cc_get_common_acl_access(first_time, "htcp_clr_access"))){
	//if (check_null_acl_access(Config_r.accessList.htcp_clr)) {
		default_line("htcp_clr_access deny all");
	}
#endif
#if USE_IDENT
	if(check_null_acl_access(cc_get_common_acl_access(first_time, "ident_lookup_access"))){
	//if (check_null_acl_access(Config_r.accessList.identLookup)) {
		default_line("ident_lookup_access deny all");
	}
#endif
	if (first_time)
	{
		if (check_null_body_size_t(Config.ReplyBodySize)) {
			default_line("reply_body_max_size 0 allow all");
		}
	}
	else
	{
		if (check_null_body_size_t(Config_r.ReplyBodySize)) {
			default_line("reply_body_max_size 0 allow all");
		}
	}
#if FOLLOW_X_FORWARDED_FOR
	//if (check_null_acl_access(Config_r.accessList.followXFF)) {
	if(check_null_acl_access(cc_get_common_acl_access(first_time, "follow_x_forwarded_for"))){
		default_line("follow_x_forwarded_for deny all");
	}
#endif
	if (first_time)
	{
		if (check_null_cachedir(Config.cacheSwap)) {
			default_line("cache_dir ufs /usr/local/squid/var/cache 100 16 256");
		}
	}
	else
	{
		if (check_null_cachedir(Config_r.cacheSwap)) {
			default_line("cache_dir ufs /usr/local/squid/var/cache 100 16 256");
		}
	}


#if USE_WCCPv2
	if(first_time)
	{
		if (check_null_wccp2_service(Config.Wccp2.info)) {
			default_line("wccp2_service standard 0");
		}
	}
	else
	{
		if (check_null_wccp2_service(Config_r.Wccp2.info)) {
			default_line("wccp2_service standard 0");
		}
	}
#endif
#if SQUID_SNMP
	if(check_null_acl_access(cc_get_common_acl_access(first_time, "snmp_access"))){
		//	if (check_null_acl_access(Config_r.accessList.snmp)) {
		default_line("snmp_access deny all");
	}
#endif
	if(first_time)
	{
		if (check_null_wordlist(Config.dns_nameservers)) {
			default_line("dns_nameservers 127.0.0.1");
		}
		if (check_null_wordlist(Config.dns_testname_list)) {
			default_line("dns_testnames netscape.com internic.net nlanr.net microsoft.com");
		}
		if (check_null_string(Config.as_whois_server)) {
			default_line("as_whois_server whois.ra.net");
		}
		if (check_null_string(Config.coredump_dir)) {
			default_line("coredump_dir none");
		}
		if (check_null_acl_access(Config.accessList.url_rewrite)) {
			default_line("url_rewrite_access deny all");
		}       
		if (check_null_acl_access(Config.accessList.storeurl_rewrite)) {
			default_line("storeurl_access deny all");
		}       
		if (check_null_acl_access(Config.accessList.location_rewrite)) {
			default_line("location_rewrite_access deny all");
		}
	}
	else
	{
		if (check_null_wordlist(Config_r.dns_nameservers)) {
			default_line("dns_nameservers 127.0.0.1");
		}
		if (check_null_wordlist(Config_r.dns_testname_list)) {
			default_line("dns_testnames netscape.com internic.net nlanr.net microsoft.com");
		}
		if (check_null_string(Config_r.as_whois_server)) {
			default_line("as_whois_server whois.ra.net");
		}
		if (check_null_string(Config_r.coredump_dir)) {
			default_line("coredump_dir none");
		}
		if (check_null_acl_access(Config_r.accessList.url_rewrite)) {
			default_line("url_rewrite_access deny all");
		}       
		if (check_null_acl_access(Config_r.accessList.storeurl_rewrite)) {
			default_line("storeurl_access deny all");
		}       
		if (check_null_acl_access(Config_r.accessList.location_rewrite)) {
			default_line("location_rewrite_access deny all");
		}
	}
}

static void parse_extension_method_r()
{
	char *token; 
	char *t = strtok(NULL, "");
	while ((token = strwordtok(NULL, &t)))
	{       
		method_t method = 0;
		for (method++; method < METHOD_ENUM_END; method++)
		{
			if (0 == strcmp(token, RequestMethods_r[method].str))
			{
				debug(23, 2) ("Extension method '%s' already exists\n", token);
				return;
			}
			if (0 != strncmp("%EXT", RequestMethods_r[method].str, 4))
				continue;
			RequestMethods_r[method].str = xstrdup(token);
			RequestMethods_r[method].len = strlen(token);
			debug(23, 1) ("Extension method '%s' added, enum=%d\n", token, (int) method);
			return;
		}
		debug(23, 1) ("WARNING: Could not add new extension method '%s' due to lack of array space\n", token);
	}     
}

extern void cc_parse_acl_access_r(char* token);

typedef void (*cc_parser_t)(void *);
typedef void (*cc_nopara_parser_t)(void);

typedef struct 
{
    size_t          len;
    unsigned char  *data;
}cc_string_t;

#define cc_string(str)         { sizeof(str) - 1, (unsigned char *) str }
#define cc_null_string         { 0, NULL }
#define cc_str_set(str, text)  (str)->len = sizeof(text) - 1; (str)->data = (unsigned char *) text
#define cc_str_null(str)       (str)->len = 0; (str)->data = NULL

typedef struct
{
    cc_string_t    tag;
    cc_parser_t    parser;
    void          *args;
}cc_tag_parser_t;

static void parse_cache_dir(void *des_cacheswap)
{
    memcpy(des_cacheswap, &Config.cacheSwap, sizeof(Config.cacheSwap)); 
    return;
}

static void parse_nopara_caller(void *func)
{
   ((cc_nopara_parser_t)func)();
    return;
}

static const cc_tag_parser_t g_cc_tag_parser_tab[] = {
    {cc_string("auth_param"                                    ), (cc_parser_t)parse_authparam                 , (void *)&Config_r.authConfig},
    {cc_string("authenticate_cache_garbage_interval"           ), (cc_parser_t)parse_time_t                    , (void *)&Config_r.authenticateGCInterval},
    {cc_string("authenticate_ttl"                              ), (cc_parser_t)parse_time_t                    , (void *)&Config_r.authenticateTTL},
    {cc_string("authenticate_ip_ttl"                           ), (cc_parser_t)parse_time_t                    , (void *)&Config_r.authenticateIpTTL},
    {cc_string("authenticate_ip_shortcircuit_ttl"              ), (cc_parser_t)parse_time_t                    , (void *)&Config_r.authenticateIpShortcircuitTTL},
    {cc_string("external_acl_type"                             ), (cc_parser_t)parse_externalAclHelper         , (void *)&Config_r.externalAclHelperList},
    {cc_string("acl"                                           ), (cc_parser_t)parse_acl                       , (void *)&Config_r.aclList},
    {cc_string("http_access"                                   ), (cc_parser_t)cc_parse_acl_access_r           , (void *)"http_access"},
    {cc_string("http_access2"                                  ), (cc_parser_t)cc_parse_acl_access_r           , (void *)"http_access2"},
    {cc_string("http_reply_access"                             ), (cc_parser_t)cc_parse_acl_access_r           , (void *)"http_reply_access"},
    {cc_string("icp_access"                                    ), (cc_parser_t)cc_parse_acl_access_r           , (void *)"icp_access"},
#if USE_HTCP
    {cc_string("htcp_access"                                   ), (cc_parser_t)cc_parse_acl_access_r           , (void *)"htcp_access"},
    {cc_string("htcp_clr_access"                               ), (cc_parser_t)cc_parse_acl_access_r           , (void *)"htcp_clr_access"},
#endif
    {cc_string("miss_access"                                   ), (cc_parser_t)cc_parse_acl_access_r           , (void *)"miss_access"},
#if USE_IDENT
    {cc_string("ident_lookup_access"                           ), (cc_parser_t)cc_parse_acl_access_r           , (void *)"ident_lookup_access"},
#endif
    {cc_string("reply_body_max_size"                           ), (cc_parser_t)parse_body_size_t               , (void *)&Config_r.ReplyBodySize},
    {cc_string("authenticate_ip_shortcircuit_access"           ), (cc_parser_t)cc_parse_acl_access_r           , (void *)"authenticate_ip_shortcircuit_access"},
#if FOLLOW_X_FORWARDED_FOR
    {cc_string("follow_x_forwarded_for"                        ), (cc_parser_t)cc_parse_acl_access_r           , (void *)"follow_x_forwarded_for"},
    {cc_string("acl_uses_indirect_client"                      ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.acl_uses_indirect_client},
    {cc_string("delay_pool_uses_indirect_client"               ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.delay_pool_uses_indirect_client},
    {cc_string("log_uses_indirect_client"                      ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.log_uses_indirect_client},
#endif
#if USE_SSL
    {cc_string("ssl_unclean_shutdown"                          ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.SSL.unclean_shutdown},
    {cc_string("ssl_engine"                                    ), (cc_parser_t)parse_string                    , (void *)&Config_r.SSL.ssl_engine},
    {cc_string("sslproxy_client_certificate"                   ), (cc_parser_t)parse_string                    , (void *)&Config_r.ssl_client.cert},
    {cc_string("sslproxy_client_key"                           ), (cc_parser_t)parse_string                    , (void *)&Config_r.ssl_client.key},
    {cc_string("sslproxy_version"                              ), (cc_parser_t)parse_int                       , (void *)&Config_r.ssl_client.version},
    {cc_string("sslproxy_options"                              ), (cc_parser_t)parse_string                    , (void *)&Config_r.ssl_client.options},
    {cc_string("sslproxy_cipher"                               ), (cc_parser_t)parse_string                    , (void *)&Config_r.ssl_client.cipher},
    {cc_string("sslproxy_cafile"                               ), (cc_parser_t)parse_string                    , (void *)&Config_r.ssl_client.cafile},
    {cc_string("sslproxy_capath"                               ), (cc_parser_t)parse_string                    , (void *)&Config_r.ssl_client.capath},
    {cc_string("sslproxy_flags"                                ), (cc_parser_t)parse_string                    , (void *)&Config_r.ssl_client.flags},
    {cc_string("sslpassword_program"                           ), (cc_parser_t)parse_string                    , (void *)&Config_r.Program.ssl_password},
#endif
    {cc_string("http_port"                                     ), (cc_parser_t)parse_http_port_list            , (void *)&Config_r.Sockaddr.http},
    {cc_string("ascii_port"                                    ), (cc_parser_t)parse_http_port_list            , (void *)&Config_r.Sockaddr.http},
#if USE_SSL
    {cc_string("https_port"                                    ), (cc_parser_t)parse_https_port_list           , (void *)&Config_r.Sockaddr.https},
#endif
    {cc_string("tcp_outgoing_tos"                              ), (cc_parser_t)parse_acl_tos                   , (void *)&Config_r.accessList.outgoing_tos},
    {cc_string("tcp_outgoing_dscp"                             ), (cc_parser_t)parse_acl_tos                   , (void *)&Config_r.accessList.outgoing_tos},
    {cc_string("tcp_outgoing_ds"                               ), (cc_parser_t)parse_acl_tos                   , (void *)&Config_r.accessList.outgoing_tos},
    {cc_string("tcp_outgoing_address"                          ), (cc_parser_t)parse_acl_address               , (void *)&Config_r.accessList.outgoing_address},
    {cc_string("zph_mode"                                      ), (cc_parser_t)parse_zph_mode                  , (void *)&Config_r.zph_mode},
    {cc_string("zph_local"                                     ), (cc_parser_t)parse_int                       , (void *)&Config_r.zph_local},
    {cc_string("zph_sibling"                                   ), (cc_parser_t)parse_int                       , (void *)&Config_r.zph_sibling},
    {cc_string("zph_parent"                                    ), (cc_parser_t)parse_int                       , (void *)&Config_r.zph_parent},
    {cc_string("zph_option"                                    ), (cc_parser_t)parse_int                       , (void *)&Config_r.zph_option},
    {cc_string("cache_peer"                                    ), (cc_parser_t)parse_peer                      , (void *)&Config_r.peers},
    {cc_string("cache_peer_domain"                             ), (cc_parser_t)parse_nopara_caller             , (void *)parse_hostdomain},
    {cc_string("cache_host_domain"                             ), (cc_parser_t)parse_nopara_caller             , (void *)parse_hostdomain},
    {cc_string("cache_peer_access"                             ), (cc_parser_t)parse_nopara_caller             , (void *)parse_peer_access},
    {cc_string("neighbor_type_domain"                          ), (cc_parser_t)parse_nopara_caller             , (void *)parse_hostdomaintype},
    {cc_string("dead_peer_timeout"                             ), (cc_parser_t)parse_time_t                    , (void *)&Config_r.Timeout.deadPeer},
    {cc_string("hierarchy_stoplist"                            ), (cc_parser_t)parse_wordlist                  , (void *)&Config_r.hierarchy_stoplist},
    {cc_string("cache_mem"                                     ), (cc_parser_t)parse_b_size_t                  , (void *)&Config_r.memMaxSize},
    {cc_string("maximum_object_size_in_memory"                 ), (cc_parser_t)parse_b_size_t                  , (void *)&Config_r.Store.maxInMemObjSize},
    {cc_string("memory_replacement_policy"                     ), (cc_parser_t)parse_removalpolicy             , (void *)&Config_r.memPolicy},
    {cc_string("cache_replacement_policy"                      ), (cc_parser_t)parse_removalpolicy             , (void *)&Config_r.replPolicy},
    {cc_string("cache_dir"                                     ), (cc_parser_t)parse_cache_dir                 , (void *)&Config_r.cacheSwap},
    {cc_string("store_dir_select_algorithm"                    ), (cc_parser_t)parse_string                    , (void *)&Config_r.store_dir_select_algorithm},
    {cc_string("max_open_disk_fds"                             ), (cc_parser_t)parse_int                       , (void *)&Config_r.max_open_disk_fds},
    {cc_string("minimum_object_size"                           ), (cc_parser_t)parse_b_size_t                  , (void *)&Config_r.Store.minObjectSize},
    {cc_string("maximum_object_size"                           ), (cc_parser_t)parse_b_size_t                  , (void *)&Config_r.Store.maxObjectSize},
    {cc_string("cache_swap_low"                                ), (cc_parser_t)parse_int                       , (void *)&Config_r.Swap.lowWaterMark},
    {cc_string("cache_swap_high"                               ), (cc_parser_t)parse_int                       , (void *)&Config_r.Swap.highWaterMark},
#if HTTP_VIOLATIONS
    {cc_string("update_headers"                                ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.update_headers},
#endif
    {cc_string("logformat"                                     ), (cc_parser_t)parse_logformat                 , (void *)&Config_r.Log.logformats},
    {cc_string("access_log"                                    ), (cc_parser_t)parse_access_log                , (void *)&Config_r.Log.accesslogs},
    {cc_string("cache_access_log"                              ), (cc_parser_t)parse_access_log                , (void *)&Config_r.Log.accesslogs},
    {cc_string("log_access"                                    ), (cc_parser_t)cc_parse_acl_access_r           , (void *)"log_access"},
    {cc_string("logfile_daemon"                                ), (cc_parser_t)parse_string                    , (void *)&Config_r.Program.logfile_daemon},
    {cc_string("cache_log"                                     ), (cc_parser_t)parse_string_with_id            , (void *)&Config_r.Log.log},
    {cc_string("cache_store_log"                               ), (cc_parser_t)parse_string                    , (void *)&Config_r.Log.store},
    {cc_string("cache_swap_state"                              ), (cc_parser_t)parse_string                    , (void *)&Config_r.Log.swap},
    {cc_string("cache_swap_log"                                ), (cc_parser_t)parse_string                    , (void *)&Config_r.Log.swap},
    {cc_string("logfile_rotate"                                ), (cc_parser_t)parse_int                       , (void *)&Config_r.Log.rotateNumber},
    {cc_string("emulate_httpd_log"                             ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.common_log},
    {cc_string("log_ip_on_direct"                              ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.log_ip_on_direct},
    {cc_string("mime_table"                                    ), (cc_parser_t)parse_string                    , (void *)&Config_r.mimeTablePathname},
    {cc_string("log_mime_hdrs"                                 ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.log_mime_hdrs},
#if USE_USERAGENT_LOG
    {cc_string("useragent_log"                                 ), (cc_parser_t)parse_string                    , (void *)&Config_r.Log.useragent},
#endif
#if USE_REFERER_LOG
    {cc_string("referer_log"                                   ), (cc_parser_t)parse_string                    , (void *)&Config_r.Log.referer},
    {cc_string("referrer_log"                                  ), (cc_parser_t)parse_string                    , (void *)&Config_r.Log.referer},
#endif
    {cc_string("pid_filename"                                  ), (cc_parser_t)parse_string_with_id            , (void *)&Config_r.pidFilename},
    {cc_string("debug_options"                                 ), (cc_parser_t)parse_eol                       , (void *)&Config_r.debugOptions},
    {cc_string("log_fqdn"                                      ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.log_fqdn},
    {cc_string("client_netmask"                                ), (cc_parser_t)parse_address                   , (void *)&Config_r.Addrs.client_netmask},
#ifdef CC_FRAMEWORK
    {cc_string("debug_log_timestamp"                           ), (cc_parser_t)parse_int                       , (void *)&Config_r.debugLogTsSwitch},
    {cc_string("eventrun_max_msec"                             ), (cc_parser_t)parse_int                       , (void *)&Config_r.evenRunMaxMsec},
    {cc_string("b4recfg_wait_msec"                             ), (cc_parser_t)parse_int                       , (void *)&Config_r.b4RecfgWaitMsec},
    {cc_string("b4defaultloadmodule_wait_msec"                 ), (cc_parser_t)parse_int                       , (void *)&Config_r.b4DefaultLoadModuleWaitMsec},
    {cc_string("b4defaultifnoner_wait_msec"                    ), (cc_parser_t)parse_int                       , (void *)&Config_r.b4DefaultIfNoneRWaitMsec},
    {cc_string("b4parseonepartcfg_wait_msec"                   ), (cc_parser_t)parse_int                       , (void *)&Config_r.b4ParseOnePartCfgWaitMsec},
    {cc_string("b4accessclean_wait_msec"                       ), (cc_parser_t)parse_int                       , (void *)&Config_r.b4AccessCleanWaitMsec},
    {cc_string("b4accesscombine_wait_msec"                     ), (cc_parser_t)parse_int                       , (void *)&Config_r.b4AccessCombineWaitMsec},
    {cc_string("b4accessfree_wait_msec"                        ), (cc_parser_t)parse_int                       , (void *)&Config_r.b4AccessFreeWaitMsec},    
    {cc_string("debug_80_log"                                  ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.debug_80_log},
#endif
    
#if WIP_FWD_LOG
    {cc_string("forward_log"                                   ), (cc_parser_t)parse_string                    , (void *)&Config_r.Log.forward},
#endif
    {cc_string("strip_query_terms"                             ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.strip_query_terms},
    {cc_string("buffered_logs"                                 ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.buffered_logs},
    {cc_string("netdb_filename"                                ), (cc_parser_t)parse_string                    , (void *)&Config_r.netdbFilename},
    {cc_string("ftp_user"                                      ), (cc_parser_t)parse_string                    , (void *)&Config_r.Ftp.anon_user},
    {cc_string("ftp_list_width"                                ), (cc_parser_t)parse_int                       , (void *)&Config_r.Ftp.list_width},
    {cc_string("ftp_passive"                                   ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.Ftp.passive},
    {cc_string("ftp_sanitycheck"                               ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.Ftp.sanitycheck},
    {cc_string("ftp_telnet_protocol"                           ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.Ftp.telnet},
    {cc_string("diskd_program"                                 ), (cc_parser_t)parse_string                    , (void *)&Config_r.Program.diskd},
#if USE_UNLINKD
    {cc_string("unlinkd_program"                               ), (cc_parser_t)parse_string                    , (void *)&Config_r.Program.unlinkd},
#endif
#if USE_ICMP
    {cc_string("pinger_program"                                ), (cc_parser_t)parse_string                    , (void *)&Config_r.Program.pinger},
#endif
    {cc_string("storeurl_rewrite_program"                      ), (cc_parser_t)parse_programline               , (void *)&Config_r.Program.store_rewrite.command},
    {cc_string("storeurl_rewrite_children"                     ), (cc_parser_t)parse_int                       , (void *)&Config_r.Program.store_rewrite.children},
    {cc_string("storeurl_rewrite_concurrency"                  ), (cc_parser_t)parse_int                       , (void *)&Config_r.Program.store_rewrite.concurrency},
    {cc_string("url_rewrite_program"                           ), (cc_parser_t)parse_programline               , (void *)&Config_r.Program.url_rewrite.command},
    {cc_string("redirect_program"                              ), (cc_parser_t)parse_programline               , (void *)&Config_r.Program.url_rewrite.command},
    {cc_string("url_rewrite_children"                          ), (cc_parser_t)parse_int                       , (void *)&Config_r.Program.url_rewrite.children},
    {cc_string("redirect_children"                             ), (cc_parser_t)parse_int                       , (void *)&Config_r.Program.url_rewrite.children},
    {cc_string("url_rewrite_concurrency"                       ), (cc_parser_t)parse_int                       , (void *)&Config_r.Program.url_rewrite.concurrency},
    {cc_string("redirect_concurrency"                          ), (cc_parser_t)parse_int                       , (void *)&Config_r.Program.url_rewrite.concurrency},
    {cc_string("url_rewrite_host_header"                       ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.redir_rewrites_host},
    {cc_string("redirect_rewrites_host_header"                 ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.redir_rewrites_host},
    {cc_string("url_rewrite_access"                            ), (cc_parser_t)cc_parse_acl_access_r           , (void *)"url_rewrite_access"},
    {cc_string("redirector_access"                             ), (cc_parser_t)cc_parse_acl_access_r           , (void *)"url_rewrite_access"},
    {cc_string("storeurl_access"                               ), (cc_parser_t)cc_parse_acl_access_r           , (void *)"storeurl_access"},
    {cc_string("redirector_bypass"                             ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.redirector_bypass},
    {cc_string("location_rewrite_program"                      ), (cc_parser_t)parse_programline               , (void *)&Config_r.Program.location_rewrite.command},
    {cc_string("location_rewrite_children"                     ), (cc_parser_t)parse_int                       , (void *)&Config_r.Program.location_rewrite.children},
    {cc_string("location_rewrite_concurrency"                  ), (cc_parser_t)parse_int                       , (void *)&Config_r.Program.location_rewrite.concurrency},
    {cc_string("location_rewrite_access"                       ), (cc_parser_t)cc_parse_acl_access_r           , (void *)"location_rewrite_access"},
    {cc_string("cache"                                         ), (cc_parser_t)cc_parse_acl_access_r           , (void *)"cache"},
    {cc_string("no_cache"                                      ), (cc_parser_t)cc_parse_acl_access_r           , (void *)"cache"},
    {cc_string("max_stale"                                     ), (cc_parser_t)parse_time_t                    , (void *)&Config_r.maxStale},
    {cc_string("refresh_pattern"                               ), (cc_parser_t)parse_refreshpattern            , (void *)&Config_r.Refresh},
    {cc_string("quick_abort_min"                               ), (cc_parser_t)parse_kb_size_t                 , (void *)&Config_r.quickAbort.min},
    {cc_string("quick_abort_max"                               ), (cc_parser_t)parse_kb_size_t                 , (void *)&Config_r.quickAbort.max},
    {cc_string("quick_abort_pct"                               ), (cc_parser_t)parse_int                       , (void *)&Config_r.quickAbort.pct},
    {cc_string("read_ahead_gap"                                ), (cc_parser_t)parse_b_size_t                  , (void *)&Config_r.readAheadGap},
    {cc_string("negative_ttl"                                  ), (cc_parser_t)parse_time_t                    , (void *)&Config_r.negativeTtl},
    {cc_string("positive_dns_ttl"                              ), (cc_parser_t)parse_time_t                    , (void *)&Config_r.positiveDnsTtl},
    {cc_string("negative_dns_ttl"                              ), (cc_parser_t)parse_time_t                    , (void *)&Config_r.negativeDnsTtl},
    {cc_string("range_offset_limit"                            ), (cc_parser_t)parse_b_size_t                  , (void *)&Config_r.rangeOffsetLimit},
    {cc_string("minimum_expiry_time"                           ), (cc_parser_t)parse_time_t                    , (void *)&Config_r.minimum_expiry_time},
    {cc_string("store_avg_object_size"                         ), (cc_parser_t)parse_kb_size_t                 , (void *)&Config_r.Store.avgObjectSize},
    {cc_string("store_objects_per_bucket"                      ), (cc_parser_t)parse_int                       , (void *)&Config_r.Store.objectsPerBucket},
    {cc_string("request_header_max_size"                       ), (cc_parser_t)parse_b_size_t                  , (void *)&Config_r.maxRequestHeaderSize},
    {cc_string("reply_header_max_size"                         ), (cc_parser_t)parse_b_size_t                  , (void *)&Config_r.maxReplyHeaderSize},
    {cc_string("request_body_max_size"                         ), (cc_parser_t)parse_b_size_t                  , (void *)&Config_r.maxRequestBodySize},
    {cc_string("broken_posts"                                  ), (cc_parser_t)cc_parse_acl_access_r           , (void *)"broken_posts"},
    {cc_string("upgrade_http0.9"                               ), (cc_parser_t)cc_parse_acl_access_r           , (void *)"upgrade_http0.9"},
#if HTTP_VIOLATIONS
    {cc_string("via"                                           ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.via},
#endif
    {cc_string("cache_vary"                                    ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.cache_vary},
    {cc_string("broken_vary_encoding"                          ), (cc_parser_t)cc_parse_acl_access_r           , (void *)"broken_vary_encoding"},
    {cc_string("collapsed_forwarding"                          ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.collapsed_forwarding},
    {cc_string("refresh_stale_hit"                             ), (cc_parser_t)parse_time_t                    , (void *)&Config_r.refresh_stale_window},
    {cc_string("ie_refresh"                                    ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.ie_refresh},
    {cc_string("vary_ignore_expire"                            ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.vary_ignore_expire},
    {cc_string("extension_methods"                             ), (cc_parser_t)parse_extension_method_r        , (void *)&RequestMethods_r},
    {cc_string("request_entities"                              ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.request_entities},
#if HTTP_VIOLATIONS
    {cc_string("header_access"                                 ), (cc_parser_t)parse_http_header_access        , (void *)&Config_r.header_access[0]},
    {cc_string("header_replace"                                ), (cc_parser_t)parse_http_header_replace       , (void *)&Config_r.header_access[0]},
#endif
    {cc_string("relaxed_header_parser"                         ), (cc_parser_t)parse_tristate                  , (void *)&Config_r.onoff.relaxed_header_parser},
    {cc_string("server_http11"                                 ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.server_http11},
    {cc_string("ignore_expect_100"                             ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.ignore_expect_100},
    {cc_string("external_refresh_check"                        ), (cc_parser_t)parse_refreshCheckHelper        , (void *)&Config_r.Program.refresh_check},
    {cc_string("forward_timeout"                               ), (cc_parser_t)parse_time_t                    , (void *)&Config_r.Timeout.forward},
    {cc_string("connect_timeout"                               ), (cc_parser_t)parse_time_t                    , (void *)&Config_r.Timeout.connect},
    {cc_string("peer_connect_timeout"                          ), (cc_parser_t)parse_time_t                    , (void *)&Config_r.Timeout.peer_connect},
    {cc_string("read_timeout"                                  ), (cc_parser_t)parse_time_t                    , (void *)&Config_r.Timeout.read},
    {cc_string("request_timeout"                               ), (cc_parser_t)parse_time_t                    , (void *)&Config_r.Timeout.request},
    {cc_string("persistent_request_timeout"                    ), (cc_parser_t)parse_time_t                    , (void *)&Config_r.Timeout.persistent_request},
    {cc_string("client_lifetime"                               ), (cc_parser_t)parse_time_t                    , (void *)&Config_r.Timeout.lifetime},
    {cc_string("half_closed_clients"                           ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.half_closed_clients},
    {cc_string("pconn_timeout"                                 ), (cc_parser_t)parse_time_t                    , (void *)&Config_r.Timeout.pconn},
#if USE_IDENT
    {cc_string("ident_timeout"                                 ), (cc_parser_t)parse_time_t                    , (void *)&Config_r.Timeout.ident},
#endif
    {cc_string("shutdown_lifetime"                             ), (cc_parser_t)parse_time_t                    , (void *)&Config_r.shutdownLifetime},
    {cc_string("cache_mgr"                                     ), (cc_parser_t)parse_string                    , (void *)&Config_r.adminEmail},
    {cc_string("mail_from"                                     ), (cc_parser_t)parse_string                    , (void *)&Config_r.EmailFrom},
    {cc_string("mail_program"                                  ), (cc_parser_t)parse_eol                       , (void *)&Config_r.EmailProgram},
    {cc_string("cache_effective_user"                          ), (cc_parser_t)parse_string                    , (void *)&Config_r.effectiveUser},
    {cc_string("cache_effective_group"                         ), (cc_parser_t)parse_string                    , (void *)&Config_r.effectiveGroup},
    {cc_string("httpd_suppress_version_string"                 ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.httpd_suppress_version_string},
    {cc_string("visible_hostname"                              ), (cc_parser_t)parse_string_with_id            , (void *)&Config_r.visibleHostname},
    {cc_string("unique_hostname"                               ), (cc_parser_t)parse_string                    , (void *)&Config_r.uniqueHostname},
    {cc_string("hostname_aliases"                              ), (cc_parser_t)parse_wordlist                  , (void *)&Config_r.hostnameAliases},
    {cc_string("umask"                                         ), (cc_parser_t)parse_int                       , (void *)&Config_r.umask},
    {cc_string("announce_period"                               ), (cc_parser_t)parse_time_t                    , (void *)&Config_r.Announce.period},
    {cc_string("announce_host"                                 ), (cc_parser_t)parse_string                    , (void *)&Config_r.Announce.host},
    {cc_string("announce_file"                                 ), (cc_parser_t)parse_string                    , (void *)&Config_r.Announce.file},
    {cc_string("announce_port"                                 ), (cc_parser_t)parse_ushort                    , (void *)&Config_r.Announce.port},
    {cc_string("httpd_accel_no_pmtu_disc"                      ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.accel_no_pmtu_disc},
#if DELAY_POOLS
    {cc_string("delay_pools"                                   ), (cc_parser_t)parse_delay_pool_count          , (void *)&Config_r.Delay},
    {cc_string("delay_class"                                   ), (cc_parser_t)parse_delay_pool_class          , (void *)&Config_r.Delay},
    {cc_string("delay_access"                                  ), (cc_parser_t)parse_delay_pool_access         , (void *)&Config_r.Delay},
    {cc_string("delay_parameters"                              ), (cc_parser_t)parse_delay_pool_rates          , (void *)&Config_r.Delay},
    {cc_string("delay_initial_bucket_level"                    ), (cc_parser_t)parse_ushort                    , (void *)&Config_r.Delay.initial},
#endif
#if USE_WCCP
    {cc_string("wccp_router"                                   ), (cc_parser_t)parse_address                   , (void *)&Config_r.Wccp.router},
#endif
#if USE_WCCPv2
    {cc_string("wccp2_router"                                  ), (cc_parser_t)parse_sockaddr_in_list          , (void *)&Config_r.Wccp2.router},
#endif
#if USE_WCCP
    {cc_string("wccp_version"                                  ), (cc_parser_t)parse_int                       , (void *)&Config_r.Wccp.version},
#endif
#if USE_WCCPv2
    {cc_string("wccp2_rebuild_wait"                            ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.Wccp2.rebuildwait},
    {cc_string("wccp2_forwarding_method"                       ), (cc_parser_t)parse_int                       , (void *)&Config_r.Wccp2.forwarding_method},
    {cc_string("wccp2_return_method"                           ), (cc_parser_t)parse_int                       , (void *)&Config_r.Wccp2.return_method},
    {cc_string("wccp2_assignment_method"                       ), (cc_parser_t)parse_int                       , (void *)&Config_r.Wccp2.assignment_method},
    {cc_string("wccp2_service"                                 ), (cc_parser_t)parse_wccp2_service             , (void *)&Config_r.Wccp2.info},
    {cc_string("wccp2_service_info"                            ), (cc_parser_t)parse_wccp2_service_info        , (void *)&Config_r.Wccp2.info},
    {cc_string("wccp2_weight"                                  ), (cc_parser_t)parse_int                       , (void *)&Config_r.Wccp2.weight},
#endif
#if USE_WCCP
    {cc_string("wccp_address"                                  ), (cc_parser_t)parse_address                   , (void *)&Config_r.Wccp.address},
#endif
#if USE_WCCPv2
    {cc_string("wccp2_address"                                 ), (cc_parser_t)parse_address                   , (void *)&Config_r.Wccp2.address},
#endif
    {cc_string("client_persistent_connections"                 ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.client_pconns},
    {cc_string("server_persistent_connections"                 ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.server_pconns},
    {cc_string("persistent_connection_after_error"             ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.error_pconns},
    {cc_string("detect_broken_pconn"                           ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.detect_broken_server_pconns},
#if USE_CACHE_DIGESTS
    {cc_string("digest_generation"                             ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.digest_generation},
    {cc_string("digest_bits_per_entry"                         ), (cc_parser_t)parse_int                       , (void *)&Config_r.digest.bits_per_entry},
    {cc_string("digest_rebuild_period"                         ), (cc_parser_t)parse_time_t                    , (void *)&Config_r.digest.rebuild_period},
    {cc_string("digest_rewrite_period"                         ), (cc_parser_t)parse_time_t                    , (void *)&Config_r.digest.rewrite_period},
    {cc_string("digest_swapout_chunk_size"                     ), (cc_parser_t)parse_b_size_t                  , (void *)&Config_r.digest.swapout_chunk_size},
    {cc_string("digest_rebuild_chunk_percentage"               ), (cc_parser_t)parse_int                       , (void *)&Config_r.digest.rebuild_chunk_percentage},
#endif
#if SQUID_SNMP
    {cc_string("snmp_port"                                     ), (cc_parser_t)parse_ushort                    , (void *)&Config_r.Port.snmp},
    {cc_string("snmp_access"                                   ), (cc_parser_t)cc_parse_acl_access_r           , (void *)"snmp_access"},
    {cc_string("snmp_incoming_address"                         ), (cc_parser_t)parse_address                   , (void *)&Config_r.Addrs.snmp_incoming},
    {cc_string("snmp_outgoing_address"                         ), (cc_parser_t)parse_address                   , (void *)&Config_r.Addrs.snmp_outgoing},
#endif
    {cc_string("icp_port"                                      ), (cc_parser_t)parse_ushort                    , (void *)&Config_r.Port.icp},
    {cc_string("udp_port"                                      ), (cc_parser_t)parse_ushort                    , (void *)&Config_r.Port.icp},
#if USE_HTCP
    {cc_string("htcp_port"                                     ), (cc_parser_t)parse_ushort                    , (void *)&Config_r.Port.htcp},
#endif
    {cc_string("log_icp_queries"                               ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.log_udp},
    {cc_string("udp_incoming_address"                          ), (cc_parser_t)parse_address                   , (void *)&Config_r.Addrs.udp_incoming},
    {cc_string("udp_outgoing_address"                          ), (cc_parser_t)parse_address                   , (void *)&Config_r.Addrs.udp_outgoing},
    {cc_string("icp_hit_stale"                                 ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.icp_hit_stale},
    {cc_string("minimum_direct_hops"                           ), (cc_parser_t)parse_int                       , (void *)&Config_r.minDirectHops},
    {cc_string("minimum_direct_rtt"                            ), (cc_parser_t)parse_int                       , (void *)&Config_r.minDirectRtt},
    {cc_string("netdb_low"                                     ), (cc_parser_t)parse_int                       , (void *)&Config_r.Netdb.low},
    {cc_string("netdb_high"                                    ), (cc_parser_t)parse_int                       , (void *)&Config_r.Netdb.high},
    {cc_string("netdb_ping_period"                             ), (cc_parser_t)parse_time_t                    , (void *)&Config_r.Netdb.period},
    {cc_string("query_icmp"                                    ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.query_icmp},
    {cc_string("test_reachability"                             ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.test_reachability},
    {cc_string("icp_query_timeout"                             ), (cc_parser_t)parse_int                       , (void *)&Config_r.Timeout.icp_query},
    {cc_string("maximum_icp_query_timeout"                     ), (cc_parser_t)parse_int                       , (void *)&Config_r.Timeout.icp_query_max},
    {cc_string("minimum_icp_query_timeout"                     ), (cc_parser_t)parse_int                       , (void *)&Config_r.Timeout.icp_query_min},
    {cc_string("mcast_groups"                                  ), (cc_parser_t)parse_wordlist                  , (void *)&Config_r.mcast_group_list},
#if MULTICAST_MISS_STREAM
    {cc_string("mcast_miss_addr"                               ), (cc_parser_t)parse_address                   , (void *)&Config_r.mcast_miss.addr},
    {cc_string("mcast_miss_ttl"                                ), (cc_parser_t)parse_ushort                    , (void *)&Config_r.mcast_miss.ttl},
    {cc_string("mcast_miss_port"                               ), (cc_parser_t)parse_ushort                    , (void *)&Config_r.mcast_miss.port},
    {cc_string("mcast_miss_encode_key"                         ), (cc_parser_t)parse_string                    , (void *)&Config_r.mcast_miss.encode_key},
#endif
    {cc_string("mcast_icp_query_timeout"                       ), (cc_parser_t)parse_int                       , (void *)&Config_r.Timeout.mcast_icp_query},
    {cc_string("icon_directory"                                ), (cc_parser_t)parse_string                    , (void *)&Config_r.icons.directory},
    {cc_string("global_internal_static"                        ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.global_internal_static},
    {cc_string("short_icon_urls"                               ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.icons.use_short_names},
    {cc_string("error_directory"                               ), (cc_parser_t)parse_string                    , (void *)&Config_r.errorDirectory},
    {cc_string("error_map"                                     ), (cc_parser_t)parse_errormap                  , (void *)&Config_r.errorMapList},
    {cc_string("err_html_text"                                 ), (cc_parser_t)parse_eol                       , (void *)&Config_r.errHtmlText},
    {cc_string("deny_info"                                     ), (cc_parser_t)parse_denyinfo                  , (void *)&Config_r.denyInfoList},
    {cc_string("nonhierarchical_direct"                        ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.nonhierarchical_direct},
    {cc_string("prefer_direct"                                 ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.prefer_direct},
#if HTTP_VIOLATIONS
    {cc_string("ignore_ims_on_miss"                            ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.ignore_ims_on_miss},
#endif
    {cc_string("always_direct"                                 ), (cc_parser_t)cc_parse_acl_access_r           , (void *)"always_direct"},
    {cc_string("never_direct"                                  ), (cc_parser_t)cc_parse_acl_access_r           , (void *)"never_direct"},
    {cc_string("max_filedescriptors"                           ), (cc_parser_t)parse_int                       , (void *)&Config_r.max_filedescriptors},
    {cc_string("max_filedesc"                                  ), (cc_parser_t)parse_int                       , (void *)&Config_r.max_filedescriptors},
    {cc_string("accept_filter"                                 ), (cc_parser_t)parse_string                    , (void *)&Config_r.accept_filter},
    {cc_string("tcp_recv_bufsize"                              ), (cc_parser_t)parse_b_size_t                  , (void *)&Config_r.tcpRcvBufsz},
    {cc_string("incoming_rate"                                 ), (cc_parser_t)parse_int                       , (void *)&Config_r.incoming_rate},
    {cc_string("check_hostnames"                               ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.check_hostnames},
    {cc_string("allow_underscore"                              ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.allow_underscore},
#if USE_DNSSERVERS
    {cc_string("cache_dns_program"                             ), (cc_parser_t)parse_string                    , (void *)&Config_r.Program.dnsserver},
    {cc_string("dns_children"                                  ), (cc_parser_t)parse_int                       , (void *)&Config_r.dnsChildren},
#endif
#if !USE_DNSSERVERS
    {cc_string("dns_retransmit_interval"                       ), (cc_parser_t)parse_time_t                    , (void *)&Config_r.Timeout.idns_retransmit},
    {cc_string("dns_timeout"                                   ), (cc_parser_t)parse_time_t                    , (void *)&Config_r.Timeout.idns_query},
#endif
    {cc_string("dns_defnames"                                  ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.res_defnames},
    {cc_string("dns_nameservers"                               ), (cc_parser_t)parse_wordlist                  , (void *)&Config_r.dns_nameservers},
    {cc_string("hosts_file"                                    ), (cc_parser_t)parse_string                    , (void *)&Config_r.etcHostsPath},
    {cc_string("dns_testnames"                                 ), (cc_parser_t)parse_wordlist                  , (void *)&Config_r.dns_testname_list},
    {cc_string("append_domain"                                 ), (cc_parser_t)parse_string                    , (void *)&Config_r.appendDomain},
    {cc_string("ignore_unknown_nameservers"                    ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.ignore_unknown_nameservers},
    {cc_string("ipcache_size"                                  ), (cc_parser_t)parse_int                       , (void *)&Config_r.ipcache.size},
    {cc_string("ipcache_low"                                   ), (cc_parser_t)parse_int                       , (void *)&Config_r.ipcache.low},
    {cc_string("ipcache_high"                                  ), (cc_parser_t)parse_int                       , (void *)&Config_r.ipcache.high},
    {cc_string("fqdncache_size"                                ), (cc_parser_t)parse_int                       , (void *)&Config_r.fqdncache.size},
    {cc_string("memory_pools"                                  ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.mem_pools},
    {cc_string("memory_pools_limit"                            ), (cc_parser_t)parse_b_size_t                  , (void *)&Config_r.MemPools.limit},
    {cc_string("forwarded_for"                                 ), (cc_parser_t)parse_onoff                     , (void *)&opt_forwarded_for_r},
    {cc_string("cachemgr_passwd"                               ), (cc_parser_t)parse_cachemgrpasswd            , (void *)&Config_r.passwd_list},
    {cc_string("client_db"                                     ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.client_db},
#if HTTP_VIOLATIONS
    {cc_string("reload_into_ims"                               ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.reload_into_ims},
#endif
    {cc_string("maximum_single_addr_tries"                     ), (cc_parser_t)parse_int                       , (void *)&Config_r.retry.maxtries},
    {cc_string("retry_on_error"                                ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.retry.onerror},
    {cc_string("as_whois_server"                               ), (cc_parser_t)parse_string                    , (void *)&Config_r.as_whois_server},
    {cc_string("offline_mode"                                  ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.offline},
    {cc_string("uri_whitespace"                                ), (cc_parser_t)parse_uri_whitespace            , (void *)&Config_r.uri_whitespace},
    {cc_string("coredump_dir"                                  ), (cc_parser_t)parse_string                    , (void *)&Config_r.coredump_dir},
    {cc_string("chroot"                                        ), (cc_parser_t)parse_string                    , (void *)&Config_r.chroot_dir},
    {cc_string("balance_on_multiple_ip"                        ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.balance_on_multiple_ip},
    {cc_string("pipeline_prefetch"                             ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.pipeline_prefetch},
    {cc_string("high_response_time_warning"                    ), (cc_parser_t)parse_int                       , (void *)&Config_r.warnings.high_rptm},
    {cc_string("high_page_fault_warning"                       ), (cc_parser_t)parse_int                       , (void *)&Config_r.warnings.high_pf},
    {cc_string("high_memory_warning"                           ), (cc_parser_t)parse_b_size_t                  , (void *)&Config_r.warnings.high_memory},
    {cc_string("sleep_after_fork"                              ), (cc_parser_t)parse_int                       , (void *)&Config_r.sleep_after_fork},
    {cc_string("zero_buffers"                                  ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.zero_buffers},
    {cc_string("windows_ipaddrchangemonitor"                   ), (cc_parser_t)parse_onoff                     , (void *)&Config_r.onoff.WIN32_IpAddrChangeMonitor},
};

static int g_cc_tag_parser_tab_num = sizeof(g_cc_tag_parser_tab)/sizeof(g_cc_tag_parser_tab[0]);

static int
parse_line_r(char *buff)
{
	char	*token;
	size_t   token_len;
	int      cc_tag_parser_tab_idx;
	const cc_tag_parser_t *cc_tag_parser;
	
	debug(0,10)("parse_line_r: %s\n", buff);

	if ((token = strtok(buff, w_space)) == NULL)
	{
	    return (1);/*ignore*/
	}		

	token_len = strlen(token);
    for(cc_tag_parser_tab_idx = 0; cc_tag_parser_tab_idx < g_cc_tag_parser_tab_num; cc_tag_parser_tab_idx ++)
    {
        cc_tag_parser = &g_cc_tag_parser_tab[ cc_tag_parser_tab_idx ];
        
        if(token_len == cc_tag_parser->tag.len && 0 == strcmp(token, cc_tag_parser->tag.data))
        {
            cc_tag_parser->parser(cc_tag_parser->args);
            return (1);/*succ*/
        }
    }
    if (strstr(token, CC_MOD_TAG) && !strstr(token, CC_SUB_MOD_TAG))
    {
        return (1);/*succ*/
    }

    return (0);/*fail*/
}


static int
parse_line_r_orig(char *buff)
{
	int	result = 1;
	char	*token;
	debug(0,10)("parse_line_t: %s\n", buff);
	//char** lasts = xcalloc(1,sizeof(char*));      // delete this line by chenqi...
	if ((token = strtok(buff, w_space)) == NULL)
		(void) 0;	/* ignore empty lines */
	else if (!strcmp(token, "auth_param"))
		parse_authparam(&Config_r.authConfig);
	else if (!strcmp(token, "authenticate_cache_garbage_interval"))
		parse_time_t(&Config_r.authenticateGCInterval);
	else if (!strcmp(token, "authenticate_ttl"))
		parse_time_t(&Config_r.authenticateTTL);
	else if (!strcmp(token, "authenticate_ip_ttl"))
		parse_time_t(&Config_r.authenticateIpTTL);
	else if (!strcmp(token, "authenticate_ip_shortcircuit_ttl"))
		parse_time_t(&Config_r.authenticateIpShortcircuitTTL);
	else if (!strcmp(token, "external_acl_type"))
		parse_externalAclHelper(&Config_r.externalAclHelperList);
	else if (!strcmp(token, "acl"))
		parse_acl(&Config_r.aclList);
	else if (!strcmp(token, "http_access"))
		cc_parse_acl_access_r("http_access");
	else if (!strcmp(token, "http_access2"))
		cc_parse_acl_access_r("http_access2");
	else if (!strcmp(token, "http_reply_access"))
		cc_parse_acl_access_r("http_reply_access");
	else if (!strcmp(token, "icp_access"))
		cc_parse_acl_access_r("icp_access");
#if USE_HTCP
	else if (!strcmp(token, "htcp_access"))
		cc_parse_acl_access_r("htcp_access");
#endif
#if USE_HTCP
	else if (!strcmp(token, "htcp_clr_access"))
		cc_parse_acl_access_r("htcp_clr_access");
#endif
	else if (!strcmp(token, "miss_access"))
		cc_parse_acl_access_r("miss_access");
#if USE_IDENT
	else if (!strcmp(token, "ident_lookup_access"))
		cc_parse_acl_access_r("ident_lookup_access");
#endif
	else if (!strcmp(token, "reply_body_max_size"))
		parse_body_size_t(&Config_r.ReplyBodySize);
	else if (!strcmp(token, "authenticate_ip_shortcircuit_access"))
		cc_parse_acl_access_r("authenticate_ip_shortcircuit_access");
#if FOLLOW_X_FORWARDED_FOR
	else if (!strcmp(token, "follow_x_forwarded_for"))
		cc_parse_acl_access_r("follow_x_forwarded_for");
#endif
#if FOLLOW_X_FORWARDED_FOR
	else if (!strcmp(token, "acl_uses_indirect_client"))
		parse_onoff(&Config_r.onoff.acl_uses_indirect_client);
#endif
#if FOLLOW_X_FORWARDED_FOR
	else if (!strcmp(token, "delay_pool_uses_indirect_client"))
		parse_onoff(&Config_r.onoff.delay_pool_uses_indirect_client);
#endif
#if FOLLOW_X_FORWARDED_FOR
	else if (!strcmp(token, "log_uses_indirect_client"))
		parse_onoff(&Config_r.onoff.log_uses_indirect_client);
#endif
#if USE_SSL
	else if (!strcmp(token, "ssl_unclean_shutdown"))
		parse_onoff(&Config_r.SSL.unclean_shutdown);
#endif
#if USE_SSL
	else if (!strcmp(token, "ssl_engine"))
		parse_string(&Config_r.SSL.ssl_engine);
#endif
#if USE_SSL
	else if (!strcmp(token, "sslproxy_client_certificate"))
		parse_string(&Config_r.ssl_client.cert);
#endif
#if USE_SSL
	else if (!strcmp(token, "sslproxy_client_key"))
		parse_string(&Config_r.ssl_client.key);
#endif
#if USE_SSL
	else if (!strcmp(token, "sslproxy_version"))
		parse_int(&Config_r.ssl_client.version);
#endif
#if USE_SSL
	else if (!strcmp(token, "sslproxy_options"))
		parse_string(&Config_r.ssl_client.options);
#endif
#if USE_SSL
	else if (!strcmp(token, "sslproxy_cipher"))
		parse_string(&Config_r.ssl_client.cipher);
#endif
#if USE_SSL
	else if (!strcmp(token, "sslproxy_cafile"))
		parse_string(&Config_r.ssl_client.cafile);
#endif
#if USE_SSL
	else if (!strcmp(token, "sslproxy_capath"))
		parse_string(&Config_r.ssl_client.capath);
#endif
#if USE_SSL
	else if (!strcmp(token, "sslproxy_flags"))
		parse_string(&Config_r.ssl_client.flags);
#endif
#if USE_SSL
	else if (!strcmp(token, "sslpassword_program"))
		parse_string(&Config_r.Program.ssl_password);
#endif
	else if (!strcmp(token, "http_port"))
		parse_http_port_list(&Config_r.Sockaddr.http);
	else if (!strcmp(token, "ascii_port"))
		parse_http_port_list(&Config_r.Sockaddr.http);
#if USE_SSL
	else if (!strcmp(token, "https_port"))
		parse_https_port_list(&Config_r.Sockaddr.https);
#endif
	else if (!strcmp(token, "tcp_outgoing_tos"))
		parse_acl_tos(&Config_r.accessList.outgoing_tos);
	else if (!strcmp(token, "tcp_outgoing_dscp"))
		parse_acl_tos(&Config_r.accessList.outgoing_tos);
	else if (!strcmp(token, "tcp_outgoing_ds"))
		parse_acl_tos(&Config_r.accessList.outgoing_tos);
	else if (!strcmp(token, "tcp_outgoing_address"))
		parse_acl_address(&Config_r.accessList.outgoing_address);
	else if (!strcmp(token, "zph_mode"))
		parse_zph_mode(&Config_r.zph_mode);
	else if (!strcmp(token, "zph_local"))
		parse_int(&Config_r.zph_local);
	else if (!strcmp(token, "zph_sibling"))
		parse_int(&Config_r.zph_sibling);
	else if (!strcmp(token, "zph_parent"))
		parse_int(&Config_r.zph_parent);
	else if (!strcmp(token, "zph_option"))
		parse_int(&Config_r.zph_option);
	else if (!strcmp(token, "cache_peer"))
		parse_peer(&Config_r.peers);
	else if (!strcmp(token, "cache_peer_domain"))
		parse_hostdomain();
	else if (!strcmp(token, "cache_host_domain"))
		parse_hostdomain();
	else if (!strcmp(token, "cache_peer_access"))
		parse_peer_access();
	else if (!strcmp(token, "neighbor_type_domain"))
		parse_hostdomaintype();
	else if (!strcmp(token, "dead_peer_timeout"))
		parse_time_t(&Config_r.Timeout.deadPeer);
	else if (!strcmp(token, "hierarchy_stoplist"))
		parse_wordlist(&Config_r.hierarchy_stoplist);
	else if (!strcmp(token, "cache_mem"))
		parse_b_size_t(&Config_r.memMaxSize);
	else if (!strcmp(token, "maximum_object_size_in_memory"))
		parse_b_size_t(&Config_r.Store.maxInMemObjSize);
	else if (!strcmp(token, "memory_replacement_policy"))
		parse_removalpolicy(&Config_r.memPolicy);
	else if (!strcmp(token, "cache_replacement_policy"))
		parse_removalpolicy(&Config_r.replPolicy);
	else if (!strcmp(token, "cache_dir"))
		//parse_cachedir(&Config_r.cacheSwap);
        memcpy(&Config_r.cacheSwap, &Config.cacheSwap, sizeof(Config.cacheSwap)); 
	else if (!strcmp(token, "store_dir_select_algorithm"))
		parse_string(&Config_r.store_dir_select_algorithm);
	else if (!strcmp(token, "max_open_disk_fds"))
		parse_int(&Config_r.max_open_disk_fds);
	else if (!strcmp(token, "minimum_object_size"))
		parse_b_size_t(&Config_r.Store.minObjectSize);
	else if (!strcmp(token, "maximum_object_size"))
		parse_b_size_t(&Config_r.Store.maxObjectSize);
	else if (!strcmp(token, "cache_swap_low"))
		parse_int(&Config_r.Swap.lowWaterMark);
	else if (!strcmp(token, "cache_swap_high"))
		parse_int(&Config_r.Swap.highWaterMark);
#if HTTP_VIOLATIONS
	else if (!strcmp(token, "update_headers"))
		parse_onoff(&Config_r.onoff.update_headers);
#endif
	else if (!strcmp(token, "logformat"))
		parse_logformat(&Config_r.Log.logformats);
	else if (!strcmp(token, "access_log"))
		parse_access_log(&Config_r.Log.accesslogs);
	else if (!strcmp(token, "cache_access_log"))
		parse_access_log(&Config_r.Log.accesslogs);
	else if (!strcmp(token, "log_access"))
		cc_parse_acl_access_r("log_access");
	else if (!strcmp(token, "logfile_daemon"))
		parse_string(&Config_r.Program.logfile_daemon);
	else if (!strcmp(token, "cache_log"))
		parse_string_with_id(&Config_r.Log.log);
	else if (!strcmp(token, "cache_store_log"))
		parse_string(&Config_r.Log.store);
	else if (!strcmp(token, "cache_swap_state"))
		parse_string(&Config_r.Log.swap);
	else if (!strcmp(token, "cache_swap_log"))
		parse_string(&Config_r.Log.swap);
	else if (!strcmp(token, "logfile_rotate"))
		parse_int(&Config_r.Log.rotateNumber);
	else if (!strcmp(token, "emulate_httpd_log"))
		parse_onoff(&Config_r.onoff.common_log);
	else if (!strcmp(token, "log_ip_on_direct"))
		parse_onoff(&Config_r.onoff.log_ip_on_direct);
	else if (!strcmp(token, "mime_table"))
		parse_string(&Config_r.mimeTablePathname);
	else if (!strcmp(token, "log_mime_hdrs"))
		parse_onoff(&Config_r.onoff.log_mime_hdrs);
#if USE_USERAGENT_LOG
	else if (!strcmp(token, "useragent_log"))
		parse_string(&Config_r.Log.useragent);
#endif
#if USE_REFERER_LOG
	else if (!strcmp(token, "referer_log"))
		parse_string(&Config_r.Log.referer);
	else if (!strcmp(token, "referrer_log"))
		parse_string(&Config_r.Log.referer);
#endif
	else if (!strcmp(token, "pid_filename"))
		parse_string_with_id(&Config_r.pidFilename);
	else if (!strcmp(token, "debug_options"))
		parse_eol(&Config_r.debugOptions);
	else if (!strcmp(token, "log_fqdn"))
		parse_onoff(&Config_r.onoff.log_fqdn);
	else if (!strcmp(token, "client_netmask"))
		parse_address(&Config_r.Addrs.client_netmask);
#if WIP_FWD_LOG
	else if (!strcmp(token, "forward_log"))
		parse_string(&Config_r.Log.forward);
#endif
	else if (!strcmp(token, "strip_query_terms"))
		parse_onoff(&Config_r.onoff.strip_query_terms);
	else if (!strcmp(token, "buffered_logs"))
		parse_onoff(&Config_r.onoff.buffered_logs);
	else if (!strcmp(token, "netdb_filename"))
		parse_string(&Config_r.netdbFilename);
	else if (!strcmp(token, "ftp_user"))
		parse_string(&Config_r.Ftp.anon_user);
	else if (!strcmp(token, "ftp_list_width"))
		parse_int(&Config_r.Ftp.list_width);
	else if (!strcmp(token, "ftp_passive"))
		parse_onoff(&Config_r.Ftp.passive);
	else if (!strcmp(token, "ftp_sanitycheck"))
		parse_onoff(&Config_r.Ftp.sanitycheck);
	else if (!strcmp(token, "ftp_telnet_protocol"))
		parse_onoff(&Config_r.Ftp.telnet);
	else if (!strcmp(token, "diskd_program"))
		parse_string(&Config_r.Program.diskd);
#if USE_UNLINKD
	else if (!strcmp(token, "unlinkd_program"))
		parse_string(&Config_r.Program.unlinkd);
#endif
#if USE_ICMP
	else if (!strcmp(token, "pinger_program"))
		parse_string(&Config_r.Program.pinger);
#endif
	else if (!strcmp(token, "storeurl_rewrite_program"))
		parse_programline(&Config_r.Program.store_rewrite.command);
	else if (!strcmp(token, "storeurl_rewrite_children"))
		parse_int(&Config_r.Program.store_rewrite.children);
	else if (!strcmp(token, "storeurl_rewrite_concurrency"))
		parse_int(&Config_r.Program.store_rewrite.concurrency);
	else if (!strcmp(token, "url_rewrite_program"))
		parse_programline(&Config_r.Program.url_rewrite.command);
	else if (!strcmp(token, "redirect_program"))
		parse_programline(&Config_r.Program.url_rewrite.command);
	else if (!strcmp(token, "url_rewrite_children"))
		parse_int(&Config_r.Program.url_rewrite.children);
	else if (!strcmp(token, "redirect_children"))
		parse_int(&Config_r.Program.url_rewrite.children);
	else if (!strcmp(token, "url_rewrite_concurrency"))
		parse_int(&Config_r.Program.url_rewrite.concurrency);
	else if (!strcmp(token, "redirect_concurrency"))
		parse_int(&Config_r.Program.url_rewrite.concurrency);
	else if (!strcmp(token, "url_rewrite_host_header"))
		parse_onoff(&Config_r.onoff.redir_rewrites_host);
	else if (!strcmp(token, "redirect_rewrites_host_header"))
		parse_onoff(&Config_r.onoff.redir_rewrites_host);
	else if (!strcmp(token, "url_rewrite_access"))
		cc_parse_acl_access_r("url_rewrite_access");
	else if (!strcmp(token, "redirector_access"))
		cc_parse_acl_access_r("url_rewrite_access");
	else if (!strcmp(token, "storeurl_access"))
		cc_parse_acl_access_r("storeurl_access");
	else if (!strcmp(token, "redirector_bypass"))
		parse_onoff(&Config_r.onoff.redirector_bypass);
	else if (!strcmp(token, "location_rewrite_program"))
		parse_programline(&Config_r.Program.location_rewrite.command);
	else if (!strcmp(token, "location_rewrite_children"))
		parse_int(&Config_r.Program.location_rewrite.children);
	else if (!strcmp(token, "location_rewrite_concurrency"))
		parse_int(&Config_r.Program.location_rewrite.concurrency);
	else if (!strcmp(token, "location_rewrite_access"))
		cc_parse_acl_access_r("location_rewrite_access");
	else if (!strcmp(token, "cache"))
		cc_parse_acl_access_r("cache");
	else if (!strcmp(token, "no_cache"))
		cc_parse_acl_access_r("cache");
	else if (!strcmp(token, "max_stale"))
		parse_time_t(&Config_r.maxStale);
	else if (!strcmp(token, "refresh_pattern"))
		parse_refreshpattern(&Config_r.Refresh);
	else if (!strcmp(token, "quick_abort_min"))
		parse_kb_size_t(&Config_r.quickAbort.min);
	else if (!strcmp(token, "quick_abort_max"))
		parse_kb_size_t(&Config_r.quickAbort.max);
	else if (!strcmp(token, "quick_abort_pct"))
		parse_int(&Config_r.quickAbort.pct);
	else if (!strcmp(token, "read_ahead_gap"))
		parse_b_size_t(&Config_r.readAheadGap);
	else if (!strcmp(token, "negative_ttl"))
		parse_time_t(&Config_r.negativeTtl);
	else if (!strcmp(token, "positive_dns_ttl"))
		parse_time_t(&Config_r.positiveDnsTtl);
	else if (!strcmp(token, "negative_dns_ttl"))
		parse_time_t(&Config_r.negativeDnsTtl);
	else if (!strcmp(token, "range_offset_limit"))
		parse_b_size_t(&Config_r.rangeOffsetLimit);
	else if (!strcmp(token, "minimum_expiry_time"))
		parse_time_t(&Config_r.minimum_expiry_time);
	else if (!strcmp(token, "store_avg_object_size"))
		parse_kb_size_t(&Config_r.Store.avgObjectSize);
	else if (!strcmp(token, "store_objects_per_bucket"))
		parse_int(&Config_r.Store.objectsPerBucket);
	else if (!strcmp(token, "request_header_max_size"))
		parse_b_size_t(&Config_r.maxRequestHeaderSize);
	else if (!strcmp(token, "reply_header_max_size"))
		parse_b_size_t(&Config_r.maxReplyHeaderSize);
	else if (!strcmp(token, "request_body_max_size"))
		parse_b_size_t(&Config_r.maxRequestBodySize);
	else if (!strcmp(token, "broken_posts"))
		cc_parse_acl_access_r("broken_posts");
	else if (!strcmp(token, "upgrade_http0.9"))
		cc_parse_acl_access_r("upgrade_http0.9");
#if HTTP_VIOLATIONS
	else if (!strcmp(token, "via"))
		parse_onoff(&Config_r.onoff.via);
#endif
	else if (!strcmp(token, "cache_vary"))
		parse_onoff(&Config_r.onoff.cache_vary);
	else if (!strcmp(token, "broken_vary_encoding"))
		cc_parse_acl_access_r("broken_vary_encoding");
	else if (!strcmp(token, "collapsed_forwarding"))
		parse_onoff(&Config_r.onoff.collapsed_forwarding);
	else if (!strcmp(token, "refresh_stale_hit"))
		parse_time_t(&Config_r.refresh_stale_window);
	else if (!strcmp(token, "ie_refresh"))
		parse_onoff(&Config_r.onoff.ie_refresh);
	else if (!strcmp(token, "vary_ignore_expire"))
		parse_onoff(&Config_r.onoff.vary_ignore_expire);
	else if (!strcmp(token, "extension_methods"))
		parse_extension_method_r(&RequestMethods_r);
	else if (!strcmp(token, "request_entities"))
		parse_onoff(&Config_r.onoff.request_entities);
#if HTTP_VIOLATIONS
	else if (!strcmp(token, "header_access"))
		parse_http_header_access(&Config_r.header_access[0]);
#endif
#if HTTP_VIOLATIONS
	else if (!strcmp(token, "header_replace"))
		parse_http_header_replace(&Config_r.header_access[0]);
#endif
	else if (!strcmp(token, "relaxed_header_parser"))
		parse_tristate(&Config_r.onoff.relaxed_header_parser);
	else if (!strcmp(token, "server_http11"))
		parse_onoff(&Config_r.onoff.server_http11);
	else if (!strcmp(token, "ignore_expect_100"))
		parse_onoff(&Config_r.onoff.ignore_expect_100);
	else if (!strcmp(token, "external_refresh_check"))
		parse_refreshCheckHelper(&Config_r.Program.refresh_check);
	else if (!strcmp(token, "forward_timeout"))
		parse_time_t(&Config_r.Timeout.forward);
	else if (!strcmp(token, "connect_timeout"))
		parse_time_t(&Config_r.Timeout.connect);
	else if (!strcmp(token, "peer_connect_timeout"))
		parse_time_t(&Config_r.Timeout.peer_connect);
	else if (!strcmp(token, "read_timeout"))
		parse_time_t(&Config_r.Timeout.read);
	else if (!strcmp(token, "request_timeout"))
		parse_time_t(&Config_r.Timeout.request);
	else if (!strcmp(token, "persistent_request_timeout"))
		parse_time_t(&Config_r.Timeout.persistent_request);
	else if (!strcmp(token, "client_lifetime"))
		parse_time_t(&Config_r.Timeout.lifetime);
	else if (!strcmp(token, "half_closed_clients"))
		parse_onoff(&Config_r.onoff.half_closed_clients);
	else if (!strcmp(token, "pconn_timeout"))
		parse_time_t(&Config_r.Timeout.pconn);
#if USE_IDENT
	else if (!strcmp(token, "ident_timeout"))
		parse_time_t(&Config_r.Timeout.ident);
#endif
	else if (!strcmp(token, "shutdown_lifetime"))
		parse_time_t(&Config_r.shutdownLifetime);
	else if (!strcmp(token, "cache_mgr"))
		parse_string(&Config_r.adminEmail);
	else if (!strcmp(token, "mail_from"))
		parse_string(&Config_r.EmailFrom);
	else if (!strcmp(token, "mail_program"))
		parse_eol(&Config_r.EmailProgram);
	else if (!strcmp(token, "cache_effective_user"))
		parse_string(&Config_r.effectiveUser);
	else if (!strcmp(token, "cache_effective_group"))
		parse_string(&Config_r.effectiveGroup);
	else if (!strcmp(token, "httpd_suppress_version_string"))
		parse_onoff(&Config_r.onoff.httpd_suppress_version_string);
	else if (!strcmp(token, "visible_hostname"))
		parse_string_with_id(&Config_r.visibleHostname);
	else if (!strcmp(token, "unique_hostname"))
		parse_string(&Config_r.uniqueHostname);
	else if (!strcmp(token, "hostname_aliases"))
		parse_wordlist(&Config_r.hostnameAliases);
	else if (!strcmp(token, "umask"))
		parse_int(&Config_r.umask);
	else if (!strcmp(token, "announce_period"))
		parse_time_t(&Config_r.Announce.period);
	else if (!strcmp(token, "announce_host"))
		parse_string(&Config_r.Announce.host);
	else if (!strcmp(token, "announce_file"))
		parse_string(&Config_r.Announce.file);
	else if (!strcmp(token, "announce_port"))
		parse_ushort(&Config_r.Announce.port);
	else if (!strcmp(token, "httpd_accel_no_pmtu_disc"))
		parse_onoff(&Config_r.onoff.accel_no_pmtu_disc);
#if DELAY_POOLS
	else if (!strcmp(token, "delay_pools"))
		parse_delay_pool_count(&Config_r.Delay);
#endif
#if DELAY_POOLS
	else if (!strcmp(token, "delay_class"))
		parse_delay_pool_class(&Config_r.Delay);
#endif
#if DELAY_POOLS
	else if (!strcmp(token, "delay_access"))
		parse_delay_pool_access(&Config_r.Delay);
#endif
#if DELAY_POOLS
	else if (!strcmp(token, "delay_parameters"))
		parse_delay_pool_rates(&Config_r.Delay);
#endif
#if DELAY_POOLS
	else if (!strcmp(token, "delay_initial_bucket_level"))
		parse_ushort(&Config_r.Delay.initial);
#endif
#if USE_WCCP
	else if (!strcmp(token, "wccp_router"))
		parse_address(&Config_r.Wccp.router);
#endif
#if USE_WCCPv2
	else if (!strcmp(token, "wccp2_router"))
		parse_sockaddr_in_list(&Config_r.Wccp2.router);
#endif
#if USE_WCCP
	else if (!strcmp(token, "wccp_version"))
		parse_int(&Config_r.Wccp.version);
#endif
#if USE_WCCPv2
	else if (!strcmp(token, "wccp2_rebuild_wait"))
		parse_onoff(&Config_r.Wccp2.rebuildwait);
#endif
#if USE_WCCPv2
	else if (!strcmp(token, "wccp2_forwarding_method"))
		parse_int(&Config_r.Wccp2.forwarding_method);
#endif
#if USE_WCCPv2
	else if (!strcmp(token, "wccp2_return_method"))
		parse_int(&Config_r.Wccp2.return_method);
#endif
#if USE_WCCPv2
	else if (!strcmp(token, "wccp2_assignment_method"))
		parse_int(&Config_r.Wccp2.assignment_method);
#endif
#if USE_WCCPv2
	else if (!strcmp(token, "wccp2_service"))
		parse_wccp2_service(&Config_r.Wccp2.info);
#endif
#if USE_WCCPv2
	else if (!strcmp(token, "wccp2_service_info"))
		parse_wccp2_service_info(&Config_r.Wccp2.info);
#endif
#if USE_WCCPv2
	else if (!strcmp(token, "wccp2_weight"))
		parse_int(&Config_r.Wccp2.weight);
#endif
#if USE_WCCP
	else if (!strcmp(token, "wccp_address"))
		parse_address(&Config_r.Wccp.address);
#endif
#if USE_WCCPv2
	else if (!strcmp(token, "wccp2_address"))
		parse_address(&Config_r.Wccp2.address);
#endif
	else if (!strcmp(token, "client_persistent_connections"))
		parse_onoff(&Config_r.onoff.client_pconns);
	else if (!strcmp(token, "server_persistent_connections"))
		parse_onoff(&Config_r.onoff.server_pconns);
	else if (!strcmp(token, "persistent_connection_after_error"))
		parse_onoff(&Config_r.onoff.error_pconns);
	else if (!strcmp(token, "detect_broken_pconn"))
		parse_onoff(&Config_r.onoff.detect_broken_server_pconns);
#if USE_CACHE_DIGESTS
	else if (!strcmp(token, "digest_generation"))
		parse_onoff(&Config_r.onoff.digest_generation);
#endif
#if USE_CACHE_DIGESTS
	else if (!strcmp(token, "digest_bits_per_entry"))
		parse_int(&Config_r.digest.bits_per_entry);
#endif
#if USE_CACHE_DIGESTS
	else if (!strcmp(token, "digest_rebuild_period"))
		parse_time_t(&Config_r.digest.rebuild_period);
#endif
#if USE_CACHE_DIGESTS
	else if (!strcmp(token, "digest_rewrite_period"))
		parse_time_t(&Config_r.digest.rewrite_period);
#endif
#if USE_CACHE_DIGESTS
	else if (!strcmp(token, "digest_swapout_chunk_size"))
		parse_b_size_t(&Config_r.digest.swapout_chunk_size);
#endif
#if USE_CACHE_DIGESTS
	else if (!strcmp(token, "digest_rebuild_chunk_percentage"))
		parse_int(&Config_r.digest.rebuild_chunk_percentage);
#endif
#if SQUID_SNMP
	else if (!strcmp(token, "snmp_port"))
		parse_ushort(&Config_r.Port.snmp);
#endif
#if SQUID_SNMP
	else if (!strcmp(token, "snmp_access"))
		cc_parse_acl_access_r("snmp_access");
#endif
#if SQUID_SNMP
	else if (!strcmp(token, "snmp_incoming_address"))
		parse_address(&Config_r.Addrs.snmp_incoming);
#endif
#if SQUID_SNMP
	else if (!strcmp(token, "snmp_outgoing_address"))
		parse_address(&Config_r.Addrs.snmp_outgoing);
#endif
	else if (!strcmp(token, "icp_port"))
		parse_ushort(&Config_r.Port.icp);
	else if (!strcmp(token, "udp_port"))
		parse_ushort(&Config_r.Port.icp);
#if USE_HTCP
	else if (!strcmp(token, "htcp_port"))
		parse_ushort(&Config_r.Port.htcp);
#endif
	else if (!strcmp(token, "log_icp_queries"))
		parse_onoff(&Config_r.onoff.log_udp);
	else if (!strcmp(token, "udp_incoming_address"))
		parse_address(&Config_r.Addrs.udp_incoming);
	else if (!strcmp(token, "udp_outgoing_address"))
		parse_address(&Config_r.Addrs.udp_outgoing);
	else if (!strcmp(token, "icp_hit_stale"))
		parse_onoff(&Config_r.onoff.icp_hit_stale);
	else if (!strcmp(token, "minimum_direct_hops"))
		parse_int(&Config_r.minDirectHops);
	else if (!strcmp(token, "minimum_direct_rtt"))
		parse_int(&Config_r.minDirectRtt);
	else if (!strcmp(token, "netdb_low"))
		parse_int(&Config_r.Netdb.low);
	else if (!strcmp(token, "netdb_high"))
		parse_int(&Config_r.Netdb.high);
	else if (!strcmp(token, "netdb_ping_period"))
		parse_time_t(&Config_r.Netdb.period);
	else if (!strcmp(token, "query_icmp"))
		parse_onoff(&Config_r.onoff.query_icmp);
	else if (!strcmp(token, "test_reachability"))
		parse_onoff(&Config_r.onoff.test_reachability);
	else if (!strcmp(token, "icp_query_timeout"))
		parse_int(&Config_r.Timeout.icp_query);
	else if (!strcmp(token, "maximum_icp_query_timeout"))
		parse_int(&Config_r.Timeout.icp_query_max);
	else if (!strcmp(token, "minimum_icp_query_timeout"))
		parse_int(&Config_r.Timeout.icp_query_min);
	else if (!strcmp(token, "mcast_groups"))
		parse_wordlist(&Config_r.mcast_group_list);
#if MULTICAST_MISS_STREAM
	else if (!strcmp(token, "mcast_miss_addr"))
		parse_address(&Config_r.mcast_miss.addr);
#endif
#if MULTICAST_MISS_STREAM
	else if (!strcmp(token, "mcast_miss_ttl"))
		parse_ushort(&Config_r.mcast_miss.ttl);
#endif
#if MULTICAST_MISS_STREAM
	else if (!strcmp(token, "mcast_miss_port"))
		parse_ushort(&Config_r.mcast_miss.port);
#endif
#if MULTICAST_MISS_STREAM
	else if (!strcmp(token, "mcast_miss_encode_key"))
		parse_string(&Config_r.mcast_miss.encode_key);
#endif
	else if (!strcmp(token, "mcast_icp_query_timeout"))
		parse_int(&Config_r.Timeout.mcast_icp_query);
	else if (!strcmp(token, "icon_directory"))
		parse_string(&Config_r.icons.directory);
	else if (!strcmp(token, "global_internal_static"))
		parse_onoff(&Config_r.onoff.global_internal_static);
	else if (!strcmp(token, "short_icon_urls"))
		parse_onoff(&Config_r.icons.use_short_names);
	else if (!strcmp(token, "error_directory"))
		parse_string(&Config_r.errorDirectory);
	else if (!strcmp(token, "error_map"))
		parse_errormap(&Config_r.errorMapList);
	else if (!strcmp(token, "err_html_text"))
		parse_eol(&Config_r.errHtmlText);
	else if (!strcmp(token, "deny_info"))
		parse_denyinfo(&Config_r.denyInfoList);
	else if (!strcmp(token, "nonhierarchical_direct"))
		parse_onoff(&Config_r.onoff.nonhierarchical_direct);
	else if (!strcmp(token, "prefer_direct"))
		parse_onoff(&Config_r.onoff.prefer_direct);
#if HTTP_VIOLATIONS
	else if (!strcmp(token, "ignore_ims_on_miss"))
		parse_onoff(&Config_r.onoff.ignore_ims_on_miss);
#endif
	else if (!strcmp(token, "always_direct"))
		cc_parse_acl_access_r("always_direct");
	else if (!strcmp(token, "never_direct"))
		cc_parse_acl_access_r("never_direct");
	else if (!strcmp(token, "max_filedescriptors"))
		parse_int(&Config_r.max_filedescriptors);
	else if (!strcmp(token, "max_filedesc"))
		parse_int(&Config_r.max_filedescriptors);
	else if (!strcmp(token, "accept_filter"))
		parse_string(&Config_r.accept_filter);
	else if (!strcmp(token, "tcp_recv_bufsize"))
		parse_b_size_t(&Config_r.tcpRcvBufsz);
	else if (!strcmp(token, "incoming_rate"))
		parse_int(&Config_r.incoming_rate);
	else if (!strcmp(token, "check_hostnames"))
		parse_onoff(&Config_r.onoff.check_hostnames);
	else if (!strcmp(token, "allow_underscore"))
		parse_onoff(&Config_r.onoff.allow_underscore);
#if USE_DNSSERVERS
	else if (!strcmp(token, "cache_dns_program"))
		parse_string(&Config_r.Program.dnsserver);
#endif
#if USE_DNSSERVERS
	else if (!strcmp(token, "dns_children"))
		parse_int(&Config_r.dnsChildren);
#endif
#if !USE_DNSSERVERS
	else if (!strcmp(token, "dns_retransmit_interval"))
		parse_time_t(&Config_r.Timeout.idns_retransmit);
#endif
#if !USE_DNSSERVERS
	else if (!strcmp(token, "dns_timeout"))
		parse_time_t(&Config_r.Timeout.idns_query);
#endif
	else if (!strcmp(token, "dns_defnames"))
		parse_onoff(&Config_r.onoff.res_defnames);
	else if (!strcmp(token, "dns_nameservers"))
		parse_wordlist(&Config_r.dns_nameservers);
	else if (!strcmp(token, "hosts_file"))
		parse_string(&Config_r.etcHostsPath);
	else if (!strcmp(token, "dns_testnames"))
		parse_wordlist(&Config_r.dns_testname_list);
	else if (!strcmp(token, "append_domain"))
		parse_string(&Config_r.appendDomain);
	else if (!strcmp(token, "ignore_unknown_nameservers"))
		parse_onoff(&Config_r.onoff.ignore_unknown_nameservers);
	else if (!strcmp(token, "ipcache_size"))
		parse_int(&Config_r.ipcache.size);
	else if (!strcmp(token, "ipcache_low"))
		parse_int(&Config_r.ipcache.low);
	else if (!strcmp(token, "ipcache_high"))
		parse_int(&Config_r.ipcache.high);
	else if (!strcmp(token, "fqdncache_size"))
		parse_int(&Config_r.fqdncache.size);
	else if (!strcmp(token, "memory_pools"))
		parse_onoff(&Config_r.onoff.mem_pools);
	else if (!strcmp(token, "memory_pools_limit"))
		parse_b_size_t(&Config_r.MemPools.limit);
	else if (!strcmp(token, "forwarded_for"))
		parse_onoff(&opt_forwarded_for_r);
	else if (!strcmp(token, "cachemgr_passwd"))
		parse_cachemgrpasswd(&Config_r.passwd_list);
	else if (!strcmp(token, "client_db"))
		parse_onoff(&Config_r.onoff.client_db);
#if HTTP_VIOLATIONS
	else if (!strcmp(token, "reload_into_ims"))
		parse_onoff(&Config_r.onoff.reload_into_ims);
#endif
	else if (!strcmp(token, "maximum_single_addr_tries"))
		parse_int(&Config_r.retry.maxtries);
	else if (!strcmp(token, "retry_on_error"))
		parse_onoff(&Config_r.retry.onerror);
	else if (!strcmp(token, "as_whois_server"))
		parse_string(&Config_r.as_whois_server);
	else if (!strcmp(token, "offline_mode"))
		parse_onoff(&Config_r.onoff.offline);
	else if (!strcmp(token, "uri_whitespace"))
		parse_uri_whitespace(&Config_r.uri_whitespace);
	else if (!strcmp(token, "coredump_dir"))
		parse_string(&Config_r.coredump_dir);
	else if (!strcmp(token, "chroot"))
		parse_string(&Config_r.chroot_dir);
	else if (!strcmp(token, "balance_on_multiple_ip"))
		parse_onoff(&Config_r.onoff.balance_on_multiple_ip);
	else if (!strcmp(token, "pipeline_prefetch"))
		parse_onoff(&Config_r.onoff.pipeline_prefetch);
	else if (!strcmp(token, "high_response_time_warning"))
		parse_int(&Config_r.warnings.high_rptm);
	else if (!strcmp(token, "high_page_fault_warning"))
		parse_int(&Config_r.warnings.high_pf);
	else if (!strcmp(token, "high_memory_warning"))
		parse_b_size_t(&Config_r.warnings.high_memory);
	else if (!strcmp(token, "sleep_after_fork"))
		parse_int(&Config_r.sleep_after_fork);
	else if (!strcmp(token, "zero_buffers"))
		parse_onoff(&Config_r.onoff.zero_buffers);
	else if (!strcmp(token, "windows_ipaddrchangemonitor"))
		parse_onoff(&Config_r.onoff.WIN32_IpAddrChangeMonitor);
	else if (strstr(token, CC_MOD_TAG) && !strstr(token, CC_SUB_MOD_TAG))
		;//cc_parse_mod_acl_access(Config.accessList.cc_mod_acls);
	else
		result = 0; /* failure */
	return(result);
}

 void
free_all_r(void)
{
	free_authparam(&Config_r.authConfig);
	free_time_t(&Config_r.authenticateGCInterval);
	free_time_t(&Config_r.authenticateTTL);
	free_time_t(&Config_r.authenticateIpTTL);
	free_time_t(&Config_r.authenticateIpShortcircuitTTL);
	free_externalAclHelper(&Config_r.externalAclHelperList);
	free_acl(&Config_r.aclList);
//	free_acl(&Config.aclList);
	free_acl_access(&Config_r.accessList.http);
	free_acl_access(&Config_r.accessList.http2);
	free_acl_access(&Config_r.accessList.reply);
	free_acl_access(&Config_r.accessList.icp);
#if USE_HTCP
	free_acl_access(&Config_r.accessList.htcp);
#endif
#if USE_HTCP
	free_acl_access(&Config_r.accessList.htcp_clr);
#endif
	free_acl_access(&Config_r.accessList.miss);
#if USE_IDENT
	free_acl_access(&Config_r.accessList.identLookup);
#endif
	free_body_size_t(&Config_r.ReplyBodySize);
	free_acl_access(&Config_r.accessList.auth_ip_shortcircuit);
#if FOLLOW_X_FORWARDED_FOR
	free_acl_access(&Config_r.accessList.followXFF);
#endif
#if FOLLOW_X_FORWARDED_FOR
	free_onoff(&Config_r.onoff.acl_uses_indirect_client);
#endif
#if FOLLOW_X_FORWARDED_FOR
	free_onoff(&Config_r.onoff.delay_pool_uses_indirect_client);
#endif
#if FOLLOW_X_FORWARDED_FOR
	free_onoff(&Config_r.onoff.log_uses_indirect_client);
#endif
#if USE_SSL
	free_onoff(&Config_r.SSL.unclean_shutdown);
#endif
#if USE_SSL
	free_string(&Config_r.SSL.ssl_engine);
#endif
#if USE_SSL
	free_string(&Config_r.ssl_client.cert);
#endif
#if USE_SSL
	free_string(&Config_r.ssl_client.key);
#endif
#if USE_SSL
	free_int(&Config_r.ssl_client.version);
#endif
#if USE_SSL
	free_string(&Config_r.ssl_client.options);
#endif
#if USE_SSL
	free_string(&Config_r.ssl_client.cipher);
#endif
#if USE_SSL
	free_string(&Config_r.ssl_client.cafile);
#endif
#if USE_SSL
	free_string(&Config_r.ssl_client.capath);
#endif
#if USE_SSL
	free_string(&Config_r.ssl_client.flags);
#endif
#if USE_SSL
	free_string(&Config_r.Program.ssl_password);
#endif
	free_http_port_list(&Config_r.Sockaddr.http);
#if USE_SSL
	free_https_port_list(&Config_r.Sockaddr.https);
#endif
	free_acl_tos(&Config_r.accessList.outgoing_tos);
	free_acl_address(&Config_r.accessList.outgoing_address);
	free_zph_mode(&Config_r.zph_mode);
	free_int(&Config_r.zph_local);
	free_int(&Config_r.zph_sibling);
	free_int(&Config_r.zph_parent);
	free_int(&Config_r.zph_option);
	free_peer(&Config_r.peers);
	free_time_t(&Config_r.Timeout.deadPeer);
	free_wordlist(&Config_r.hierarchy_stoplist);
	free_b_size_t(&Config_r.memMaxSize);
	free_b_size_t(&Config_r.Store.maxInMemObjSize);
	free_removalpolicy(&Config_r.memPolicy);
	free_removalpolicy(&Config_r.replPolicy);
	free_cachedir(&Config_r.cacheSwap);
	free_string(&Config_r.store_dir_select_algorithm);
	free_int(&Config_r.max_open_disk_fds);
	free_b_size_t(&Config_r.Store.minObjectSize);
	free_b_size_t(&Config_r.Store.maxObjectSize);
	free_int(&Config_r.Swap.lowWaterMark);
	free_int(&Config_r.Swap.highWaterMark);
#if HTTP_VIOLATIONS
	free_onoff(&Config_r.onoff.update_headers);
#endif
	free_logformat(&Config_r.Log.logformats);
	free_access_log(&Config_r.Log.accesslogs);
	free_acl_access(&Config_r.accessList.log);
	free_string(&Config_r.Program.logfile_daemon);
	free_string(&Config_r.Log.log);
	free_string(&Config_r.Log.store);
	free_string(&Config_r.Log.swap);
	free_int(&Config_r.Log.rotateNumber);
	free_onoff(&Config_r.onoff.common_log);
	free_onoff(&Config_r.onoff.log_ip_on_direct);
	free_string(&Config_r.mimeTablePathname);
	free_onoff(&Config_r.onoff.log_mime_hdrs);
#if USE_USERAGENT_LOG
	free_string(&Config_r.Log.useragent);
#endif
#if USE_REFERER_LOG
	free_string(&Config_r.Log.referer);
#endif
	free_string(&Config_r.pidFilename);
	free_eol(&Config_r.debugOptions);
	free_onoff(&Config_r.onoff.log_fqdn);
	free_address(&Config_r.Addrs.client_netmask);
#if WIP_FWD_LOG
	free_string(&Config_r.Log.forward);
#endif
	free_onoff(&Config_r.onoff.strip_query_terms);
	free_onoff(&Config_r.onoff.buffered_logs);
	free_string(&Config_r.netdbFilename);
	free_string(&Config_r.Ftp.anon_user);
	free_int(&Config_r.Ftp.list_width);
	free_onoff(&Config_r.Ftp.passive);
	free_onoff(&Config_r.Ftp.sanitycheck);
	free_onoff(&Config_r.Ftp.telnet);
	free_string(&Config_r.Program.diskd);
#if USE_UNLINKD
	free_string(&Config_r.Program.unlinkd);
#endif
#if USE_ICMP
	free_string(&Config_r.Program.pinger);
#endif
	free_programline(&Config_r.Program.store_rewrite.command);
	free_int(&Config_r.Program.store_rewrite.children);
	free_int(&Config_r.Program.store_rewrite.concurrency);
	free_programline(&Config_r.Program.url_rewrite.command);
	free_int(&Config_r.Program.url_rewrite.children);
	free_int(&Config_r.Program.url_rewrite.concurrency);
	free_onoff(&Config_r.onoff.redir_rewrites_host);
	free_acl_access(&Config_r.accessList.url_rewrite);
	free_acl_access(&Config_r.accessList.storeurl_rewrite);
	free_onoff(&Config_r.onoff.redirector_bypass);
	free_programline(&Config_r.Program.location_rewrite.command);
	free_int(&Config_r.Program.location_rewrite.children);
	free_int(&Config_r.Program.location_rewrite.concurrency);
	free_acl_access(&Config_r.accessList.location_rewrite);
	free_acl_access(&Config_r.accessList.noCache);
	free_time_t(&Config_r.maxStale);
	free_refreshpattern(&Config_r.Refresh);
	free_kb_size_t(&Config_r.quickAbort.min);
	free_kb_size_t(&Config_r.quickAbort.max);
	free_int(&Config_r.quickAbort.pct);
	free_b_size_t(&Config_r.readAheadGap);
	free_time_t(&Config_r.negativeTtl);
	free_time_t(&Config_r.positiveDnsTtl);
	free_time_t(&Config_r.negativeDnsTtl);
	free_b_size_t(&Config_r.rangeOffsetLimit);
	free_time_t(&Config_r.minimum_expiry_time);
	free_kb_size_t(&Config_r.Store.avgObjectSize);
	free_int(&Config_r.Store.objectsPerBucket);
	free_b_size_t(&Config_r.maxRequestHeaderSize);
	free_b_size_t(&Config_r.maxReplyHeaderSize);
	free_b_size_t(&Config_r.maxRequestBodySize);
	free_acl_access(&Config_r.accessList.brokenPosts);
	free_acl_access(&Config_r.accessList.upgrade_http09);
#if HTTP_VIOLATIONS
	free_onoff(&Config_r.onoff.via);
#endif
	free_onoff(&Config_r.onoff.cache_vary);
	free_acl_access(&Config_r.accessList.vary_encoding);
	free_onoff(&Config_r.onoff.collapsed_forwarding);
	free_time_t(&Config_r.refresh_stale_window);
	free_onoff(&Config_r.onoff.ie_refresh);
	free_onoff(&Config_r.onoff.vary_ignore_expire);
	free_extension_method(&RequestMethods_r);
	free_onoff(&Config_r.onoff.request_entities);
#if HTTP_VIOLATIONS
	free_http_header_access(&Config_r.header_access[0]);
#endif
#if HTTP_VIOLATIONS
	free_http_header_replace(&Config_r.header_access[0]);
#endif
	free_tristate(&Config_r.onoff.relaxed_header_parser);
	free_onoff(&Config_r.onoff.server_http11);
	free_onoff(&Config_r.onoff.ignore_expect_100);
	free_refreshCheckHelper(&Config_r.Program.refresh_check);
	free_time_t(&Config_r.Timeout.forward);
	free_time_t(&Config_r.Timeout.connect);
	free_time_t(&Config_r.Timeout.peer_connect);
	free_time_t(&Config_r.Timeout.read);
	free_time_t(&Config_r.Timeout.request);
	free_time_t(&Config_r.Timeout.persistent_request);
	free_time_t(&Config_r.Timeout.lifetime);
	free_onoff(&Config_r.onoff.half_closed_clients);
	free_time_t(&Config_r.Timeout.pconn);
#if USE_IDENT
	free_time_t(&Config_r.Timeout.ident);
#endif
	free_time_t(&Config_r.shutdownLifetime);
	free_string(&Config_r.adminEmail);
	free_string(&Config_r.EmailFrom);
	free_eol(&Config_r.EmailProgram);
	free_string(&Config_r.effectiveUser);
	free_string(&Config_r.effectiveGroup);
	free_onoff(&Config_r.onoff.httpd_suppress_version_string);
	free_string(&Config_r.visibleHostname);
	free_string(&Config_r.uniqueHostname);
	free_wordlist(&Config_r.hostnameAliases);
	free_int(&Config_r.umask);
	free_time_t(&Config_r.Announce.period);
	free_string(&Config_r.Announce.host);
	free_string(&Config_r.Announce.file);
	free_ushort(&Config_r.Announce.port);
	free_onoff(&Config_r.onoff.accel_no_pmtu_disc);
#if DELAY_POOLS
	free_delay_pool_count(&Config_r.Delay);
#endif
#if DELAY_POOLS
	free_delay_pool_class(&Config_r.Delay);
#endif
#if DELAY_POOLS
	free_delay_pool_access(&Config_r.Delay);
#endif
#if DELAY_POOLS
	free_delay_pool_rates(&Config_r.Delay);
#endif
#if DELAY_POOLS
	free_ushort(&Config_r.Delay.initial);
#endif
#if USE_WCCP
	free_address(&Config_r.Wccp.router);
#endif
#if USE_WCCPv2
	free_sockaddr_in_list(&Config_r.Wccp2.router);
#endif
#if USE_WCCP
	free_int(&Config_r.Wccp.version);
#endif
#if USE_WCCPv2
	free_onoff(&Config_r.Wccp2.rebuildwait);
#endif
#if USE_WCCPv2
	free_int(&Config_r.Wccp2.forwarding_method);
#endif
#if USE_WCCPv2
	free_int(&Config_r.Wccp2.return_method);
#endif
#if USE_WCCPv2
	free_int(&Config_r.Wccp2.assignment_method);
#endif
#if USE_WCCPv2
	free_wccp2_service(&Config_r.Wccp2.info);
#endif
#if USE_WCCPv2
	free_wccp2_service_info(&Config_r.Wccp2.info);
#endif
#if USE_WCCPv2
	free_int(&Config_r.Wccp2.weight);
#endif
#if USE_WCCP
	free_address(&Config_r.Wccp.address);
#endif
#if USE_WCCPv2
	free_address(&Config_r.Wccp2.address);
#endif
	free_onoff(&Config_r.onoff.client_pconns);
	free_onoff(&Config_r.onoff.server_pconns);
	free_onoff(&Config_r.onoff.error_pconns);
	free_onoff(&Config_r.onoff.detect_broken_server_pconns);
#if USE_CACHE_DIGESTS
	free_onoff(&Config_r.onoff.digest_generation);
#endif
#if USE_CACHE_DIGESTS
	free_int(&Config_r.digest.bits_per_entry);
#endif
#if USE_CACHE_DIGESTS
	free_time_t(&Config_r.digest.rebuild_period);
#endif
#if USE_CACHE_DIGESTS
	free_time_t(&Config_r.digest.rewrite_period);
#endif
#if USE_CACHE_DIGESTS
	free_b_size_t(&Config_r.digest.swapout_chunk_size);
#endif
#if USE_CACHE_DIGESTS
	free_int(&Config_r.digest.rebuild_chunk_percentage);
#endif
#if SQUID_SNMP
	free_ushort(&Config_r.Port.snmp);
#endif
#if SQUID_SNMP
	free_acl_access(&Config_r.accessList.snmp);
#endif
#if SQUID_SNMP
	free_address(&Config_r.Addrs.snmp_incoming);
#endif
#if SQUID_SNMP
	free_address(&Config_r.Addrs.snmp_outgoing);
#endif
	free_ushort(&Config_r.Port.icp);
#if USE_HTCP
	free_ushort(&Config_r.Port.htcp);
#endif
	free_onoff(&Config_r.onoff.log_udp);
	free_address(&Config_r.Addrs.udp_incoming);
	free_address(&Config_r.Addrs.udp_outgoing);
	free_onoff(&Config_r.onoff.icp_hit_stale);
	free_int(&Config_r.minDirectHops);
	free_int(&Config_r.minDirectRtt);
	free_int(&Config_r.Netdb.low);
	free_int(&Config_r.Netdb.high);
	free_time_t(&Config_r.Netdb.period);
	free_onoff(&Config_r.onoff.query_icmp);
	free_onoff(&Config_r.onoff.test_reachability);
	free_int(&Config_r.Timeout.icp_query);
	free_int(&Config_r.Timeout.icp_query_max);
	free_int(&Config_r.Timeout.icp_query_min);
	free_wordlist(&Config_r.mcast_group_list);
#if MULTICAST_MISS_STREAM
	free_address(&Config_r.mcast_miss.addr);
#endif
#if MULTICAST_MISS_STREAM
	free_ushort(&Config_r.mcast_miss.ttl);
#endif
#if MULTICAST_MISS_STREAM
	free_ushort(&Config_r.mcast_miss.port);
#endif
#if MULTICAST_MISS_STREAM
	free_string(&Config_r.mcast_miss.encode_key);
#endif
	free_int(&Config_r.Timeout.mcast_icp_query);
	free_string(&Config_r.icons.directory);
	free_onoff(&Config_r.onoff.global_internal_static);
	free_onoff(&Config_r.icons.use_short_names);
	free_string(&Config_r.errorDirectory);
	free_errormap(&Config_r.errorMapList);
	free_eol(&Config_r.errHtmlText);
	free_denyinfo(&Config_r.denyInfoList);
	free_onoff(&Config_r.onoff.nonhierarchical_direct);
	free_onoff(&Config_r.onoff.prefer_direct);
#if HTTP_VIOLATIONS
	free_onoff(&Config_r.onoff.ignore_ims_on_miss);
#endif
	free_acl_access(&Config_r.accessList.AlwaysDirect);
	free_acl_access(&Config_r.accessList.NeverDirect);
	free_int(&Config_r.max_filedescriptors);
	free_string(&Config_r.accept_filter);
	free_b_size_t(&Config_r.tcpRcvBufsz);
	free_int(&Config_r.incoming_rate);
	free_onoff(&Config_r.onoff.check_hostnames);
	free_onoff(&Config_r.onoff.allow_underscore);
#if USE_DNSSERVERS
	free_string(&Config_r.Program.dnsserver);
#endif
#if USE_DNSSERVERS
	free_int(&Config_r.dnsChildren);
#endif
#if !USE_DNSSERVERS
	free_time_t(&Config_r.Timeout.idns_retransmit);
#endif
#if !USE_DNSSERVERS
	free_time_t(&Config_r.Timeout.idns_query);
#endif
	free_onoff(&Config_r.onoff.res_defnames);
	free_wordlist(&Config_r.dns_nameservers);
	free_string(&Config_r.etcHostsPath);
	free_wordlist(&Config_r.dns_testname_list);
	free_string(&Config_r.appendDomain);
	free_onoff(&Config_r.onoff.ignore_unknown_nameservers);
	free_int(&Config_r.ipcache.size);
	free_int(&Config_r.ipcache.low);
	free_int(&Config_r.ipcache.high);
	free_int(&Config_r.fqdncache.size);
	free_onoff(&Config_r.onoff.mem_pools);
	free_b_size_t(&Config_r.MemPools.limit);
	free_onoff(&opt_forwarded_for_r);
	free_cachemgrpasswd(&Config_r.passwd_list);
	free_onoff(&Config_r.onoff.client_db);
#if HTTP_VIOLATIONS
	free_onoff(&Config_r.onoff.reload_into_ims);
#endif
	free_int(&Config_r.retry.maxtries);
	free_onoff(&Config_r.retry.onerror);
	free_string(&Config_r.as_whois_server);
	free_onoff(&Config_r.onoff.offline);
	free_uri_whitespace(&Config_r.uri_whitespace);
	free_string(&Config_r.coredump_dir);
	free_string(&Config_r.chroot_dir);
	free_onoff(&Config_r.onoff.balance_on_multiple_ip);
	free_onoff(&Config_r.onoff.pipeline_prefetch);
	free_int(&Config_r.warnings.high_rptm);
	free_int(&Config_r.warnings.high_pf);
	free_b_size_t(&Config_r.warnings.high_memory);
	free_int(&Config_r.sleep_after_fork);
	free_onoff(&Config_r.onoff.zero_buffers);
	free_onoff(&Config_r.onoff.WIN32_IpAddrChangeMonitor);
}
#endif
