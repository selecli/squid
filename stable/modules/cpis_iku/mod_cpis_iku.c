/**************************************************************************
 *   Copyright (C) 2013
 *
 *   File ID:
 *   File name: mod_cpis_iku.c
 *   Summary:
 *   Description: 
 *      1. Use to switch youku private Range by url;  Dongshu.Zhou Creat; Yandong.Nian Modify;
 *      2. Use to switch letv  private Range by url;  Yandong.Nian Creat;
 *   Configure: Match  ACL
 ___________________________________________________________________________
 *   Version: V1.0, Create mod_cpis_iku
 *   Author:  Dongshu.Zhou
 *   Date:    2010-11-12
 ___________________________________________________________________________
 *   Version: V2.0, Modify mod_cpis_iku 
 *   Author:  Yandong.Nian
 *   Date:    2013-10-17
 **************************************************************************/
#include "cc_framework_api.h"
#ifdef CC_FRAMEWORK
static cc_module *mod = NULL;

typedef enum _channel
{
    NONE,
    IQIYI,
    YOUKU,
    LETVS,
    LETV
}TYPE;

typedef struct _mod_config
{
    TYPE channel;
	String ns;
	String nk;
}mod_conf_param;

static int free_configure_param(void *data)
{
    mod_conf_param *param = data;
    if(param)
    {
        stringClean(&param->ns);
        stringClean(&param->nk);
        param->channel = NONE;
        safe_free(param);
    }
	return 0;
}

static int hook_func_sys_parse_param(char *param_line)
{
    char *ns = NULL, *nk = NULL, *t= strtok(param_line, w_space);
    if(!t || strcmp(t, "mod_cpis_iku") || !(t = strtok(NULL, w_space)))
    {   
        debug(141, 5) ("mod_cpis_iku parse param fail %s ns=%s nk=%s\n", t, ns, nk);
        return -1; 
    }   
    TYPE channel = NONE; 
    if(!strcmp(t , "IQIYI")) channel = IQIYI;
    if(!strcmp(t , "LETVS")) channel = LETVS;
    if(!strcmp(t , "LETV"))  channel = LETV; 
    if(!strcmp(t , "YOUKU")) channel = YOUKU;

    if(!(ns = strtok(NULL, w_space)) || !(nk = strtok(NULL, w_space)))
    {   
        debug(141, 5) ("mod_cpis_iku parse param fail %s ns=%s nk=%s\n", t, ns, nk);    
        return -1; 
    }   
    debug(141, 5) ("mod_cpis_iku %s %s parse ok!\n", ns, nk);    
    mod_conf_param *param = xcalloc(1, sizeof(mod_conf_param));
    stringInit(&param->ns, ns);
    stringInit(&param->nk, nk);
    param->channel = channel;
    cc_register_mod_param(mod, param, free_configure_param);
    return 0;
}



static void drop_startend(const char* key, char* url)
{
    char *s = NULL, *e = NULL;
    if(url && key && (s = strstr(url, key)))
    {
        if(!(e = strstr(s, "&")))
          *(s-1) = 0;
        else
        {
            memmove(s, e+1, strlen(e+1));
            *(s+strlen(e+1)) = 0;
        }
        int len = strlen(url);
        if(len > 1 && url[len -1] == '?')
          url[len -1] = '\0';
    }
}

static int addPrivateRangeDone(clientHttpRequest *http, char *sBigen, char *sEnd, TYPE channel)
{
    if(http && sBigen)
    {
        char ns_value[128];
        long offset = atol(sBigen);
        long length = -1;
       if(sEnd)
         length = atol(sEnd);
       memset(ns_value, 0, 128);
        if((YOUKU == channel || IQIYI == channel) && length != -1)
          snprintf(ns_value, 127, "bytes=%ld-%ld", offset, offset + length -1);
        else if(length != -1)
          snprintf(ns_value, 127, "bytes=%ld-", offset);
        else
          snprintf(ns_value, 127, "bytes=%ld-%ld", offset, length);
        debug(141, 3)("mod_cpis_iku: Add Private Range %s : %ld-%ld\n", ns_value, offset, length);
        httpHeaderAddEntry(&http->request->header, httpHeaderEntryCreate(HDR_RANGE, "Range", ns_value));  
        return 1;
    }
    return 0;
}

static int dropUrlPrivateRangeParam(clientHttpRequest *http, const char *sBigen, const char *sEnd)
{
    if(http && sBigen && sEnd)
    {
        drop_startend(sBigen, http->uri);
        drop_startend(sEnd, http->uri);
        drop_startend(sBigen, http->request->urlpath.buf);
        drop_startend(sEnd, http->request->urlpath.buf);
        http->request->urlpath.len=strlen(http->request->urlpath.buf);
        safe_free(http->request->canonical);  
        return 1;
    }
    return 0;
}
static int urlParamSwitchToPrivateRangeYouku(clientHttpRequest *http, char *tmp, mod_conf_param *param)
{
    debug(141, 8)("mod_cpis_iku: switch Range Start iiii %s\n" ,http->uri);
    if(http && tmp && param && param->ns.buf && param->channel == YOUKU)
    {
        int isEnd = 0;
        char *sEnd = NULL, *sBigen = NULL;
        if(param && (sBigen = strstr(tmp, param->ns.buf)) && (sBigen[-1] == '?' || sBigen[-1] == '&'))
        {   
            debug(141, 5)("mod_cpis_iku: switch Range Start %s\n" ,http->uri);
            isEnd = strcspn(sBigen, "&");
            if((isEnd - param->ns.len > 0  && (sEnd = memchr(sBigen, '_', isEnd))))
            {   
                if(sEnd[1] != '1' && addPrivateRangeDone(http, sBigen + param->ns.len, sEnd + 2, param->channel))
                {   
                    debug(141, 3)("mod_cpis_iku: YOUKU switch to Private Range\n");
                }   
                dropUrlPrivateRangeParam(http, param->ns.buf, param->nk.buf);
                debug(141, 5)("mod_cpis_iku: YOUKU switch Range End %s\n" ,http->uri);
                return 1;
            }   
        }
    }
    return 0;
}

static int 
urlParamSwitchToPrivateRangeIQiyi(clientHttpRequest *http, char *tmp, mod_conf_param *param)
{
    if(http && tmp && param && param->ns.buf)
    {   
        char *sEnd = NULL, *sBigen = strstr(tmp, param->ns.buf);
        if(sBigen && sBigen[-1] == '&')
        {   
            sBigen += param->ns.len;
            if((sEnd = strstr(sBigen, param->nk.buf)) && sEnd[-1] == '&')
              sEnd += param->nk.len;
            else
              sEnd = NULL;
            if(addPrivateRangeDone(http, sBigen, sEnd, param->channel))
            {   
                debug(141, 3)("mod_cpis_iku: switch to Private Range\n");
            }   
            dropUrlPrivateRangeParam(http, param->ns.buf, param->nk.buf);
            debug(141, 5)("mod_cpis_iku: switch Range End %s\n" ,http->uri);
            return 1;
        }   
    }
    return 0;
}

static int urlParamSwitchToPrivateRange(clientHttpRequest *http, char *tmp, mod_conf_param *param)
{
    if(http && tmp && param && param->ns.buf &&  param->nk.buf)
    {
        char *sEnd = NULL, *sBigen = strstr(tmp, param->ns.buf);
        if(sBigen)
        {
            sBigen += param->ns.len;
            if((sEnd = strstr(sBigen, param->nk.buf))&& addPrivateRangeDone(http, sBigen, sEnd + param->nk.len, param->channel))
            {
                debug(141, 3)("mod_cpis_iku: switch to Private Range\n");
            }
            dropUrlPrivateRangeParam(http, param->ns.buf, param->nk.buf);
            debug(141, 5)("mod_cpis_iku: switch Range End %s\n" ,http->uri);
            return 1;
        }
    }
    return 0;
}

static int hook_switch_private_range(clientHttpRequest *http)
{
    char *tmp = NULL;
    if(!http || !http->uri || !http->request || NULL == (tmp = strchr(http->uri, '?')))
    {
        debug(141, 3)("http = NULL, return \n");
        return 0;
    }
    mod_conf_param *param= cc_get_mod_param(http->conn->fd, mod);
    if(param && param->channel == YOUKU && param->ns.buf)
      //&& (strstr(http->uri,".flv") || strstr(http->uri,".f4v") || strstr(http->uri,".mp4")))
    {
        urlParamSwitchToPrivateRangeYouku(http, tmp, param); 
        return 0;
    }
    if(param && param->ns.buf && param->nk.buf && param->channel == LETV)
    {
        return 0;
    }
    if(param && param->ns.buf && param->nk.buf && param->channel == IQIYI)
    {
        urlParamSwitchToPrivateRangeIQiyi(http, tmp, param);
        return 0;
    }
    if(param && param->ns.buf && param->nk.buf && param->channel != YOUKU)
    {
        urlParamSwitchToPrivateRange(http, tmp, param);
        return 0;
    }
    return 0;
}

static int hook_disable_sendfile(store_client *sc, int fd)
{
    debug(141, 9)("mod_cpis_iku: set disable sendfile %s\n", storeLookupUrl(sc->entry));
    sc->is_zcopyable = 0;
    return 1;
}

static int hook_set_http_status(clientHttpRequest *http)
{
    if(http && http->reply && (http->reply->sline.status == HTTP_PARTIAL_CONTENT || http->reply->sline.status == HTTP_OK))
    {
        http->reply->sline.status = HTTP_OK;
        debug(141, 7)("mod_cpis_iku: modify response status 206 to 200\n");
        mod_conf_param *param= cc_get_mod_param(http->conn->fd, mod);
        HttpHeaderEntry *e = httpHeaderFindEntry(&http->reply->header, HDR_CONTENT_RANGE);
        if(e && param && (param->channel == LETV || param->channel == LETVS))
        {
            httpHeaderDelByName(&http->reply->header, "LetvCR");
            httpHeaderAddEntry(&http->reply->header, httpHeaderEntryCreate(HDR_OTHER, "LetvCR", strBuf(e->value)));
            httpHeaderDelById(&http->reply->header, HDR_CONTENT_RANGE);
            debug(141, 7)("mod_cpis_iku: LETV modify Content-Range to letcCR\n");
        }
    }
    return 0;
}

int mod_register(cc_module *module)
{
    debug(141, 1)("(mod_cpis_iku) ->init module\n");
    strcpy(module->version, "5.5.R.6030.i686");

    cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
                module->slot, 
                (void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
                hook_func_sys_parse_param);

    cc_register_hook_handler(HPIDX_hook_func_client_set_zcopyable,
                module->slot, 
                (void **)ADDR_OF_HOOK_FUNC(module, hook_func_client_set_zcopyable),
                hook_disable_sendfile);

    cc_register_hook_handler(HPIDX_hook_func_http_req_parsed,
                module->slot, 
                (void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_parsed),
                hook_switch_private_range);

    cc_register_hook_handler(HPIDX_hook_func_http_repl_send_start,
                module->slot, 
                (void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_repl_send_start),
                hook_set_http_status);

    if(reconfigure_in_thread)
      mod = (cc_module*)cc_modules.items[module->slot];
    else 
      mod = module;
    return 0;
}
#endif
