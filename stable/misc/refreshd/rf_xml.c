/*
 * $Id$
 * author : liwei(liwei@chinacache.com)
 * desc   : parse xml use libxml2
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include "refresh_header.h"
static xmlDocPtr doc;
static xmlNodePtr cur;

extern int enable_dir_purge;

static void urlRemoveHost(rf_url_list *url)
{
	size_t i, len;
	char *t1 = NULL;
	char *t2 = NULL;

	if (NULL == config.url_remove_host)
		return;

	t1 = strstr(url->buffer, "://");
	if (NULL == t1)
		return;
	t1 += 3;
	t2 = strchr(t1, '/');
	len = (NULL == t2) ? strlen(t1) : t2 - t1;
	if (0 == len)
		return;
	for (i = 0; i < config.url_remove_host_count; i++)
	{
		if (strlen(config.url_remove_host[i]) != len)
			continue;
		if (strncmp(config.url_remove_host[i], t1, len))
			continue;
		t2++;
		len = strlen(t2);
		memmove(url->buffer, t2, len);
		url->buffer[len] = '\0';
		url->len = len;
		return;
	}
}

int parse_url_list(xmlNodePtr cur,char *token,rf_cli_session *sess){
	xmlChar *key,*id;
	cur = cur->xmlChildrenNode;

	bool b_find = false;

	while (cur != NULL) {
		if ((!xmlStrcmp(cur->name, (const xmlChar *)token))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if(! (key && xmlStrlen(key))){
					xmlFree(key);
					goto NEXT;
			}

			id = xmlGetProp(cur,(const xmlChar *)"id");
			if(! (id && xmlStrlen(id))){
					xmlFree(id);
					xmlFree(key);
					goto NEXT;
			}

			b_find = true;

			rf_url_list * url = cc_malloc(sizeof(rf_url_list));
			url->buffer = cc_malloc(xmlStrlen(key) + 1);
			strcpy(url->buffer,(char *)key);

			/*
			//here we don't want to decode '&'
			char *c,*d;
			while((c = strstr(url->buffer,"%26")) != NULL){
			 *c++ = '&';
			 d = c + 2;
			 while((*d) != '\0') *c++ = *d++;
			 *c = '\0';
			 }
			 */

			url->len = xmlStrlen(key);
			url->id = strtoll((const char *)id, (char **)NULL, 10);

			xmlFree(key);
			xmlFree(id);

			/* Add Start: url remove host, by xin.yao, 2012-03-04 */
			urlRemoveHost(url);
			/* Add Ended: by xin.yao */

			cclog(4,"url: %s,id : %"PRINTF_UINT64_T, url->buffer,url->id);

			//add to url list
			if(sess->url_list == NULL){
				sess->url_list = url;
			}else{
				url->next = sess->url_list;
				sess->url_list = url;
			}

			sess->url_number++;
		}

NEXT:
		cur = cur->next;
	}

	return b_find ? RF_OK : RF_ERR_XML_PARSE;
}

int rf_parse_url(xmlNodePtr cur,rf_cli_session *sess){
	bool b_url = false;
	int ret = 0;

	while (cur != NULL) {
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"url_list"))){
			ret = parse_url_list(cur,"url",sess);
			if(ret != RF_OK){
					b_url = false;
					break;
			}
			else
					b_url = true;
		}
		cur = cur->next;
	}

	if(b_url){
			return RF_OK;
	}

	return RF_ERR_XML_PARSE;
}

int rf_parse_dir(xmlNodePtr cur,rf_cli_session *sess){
	if(sess->params == NULL){
		sess->params = cc_malloc(sizeof(rf_cli_dir_params));
	}

	rf_cli_dir_params *dir_params = (rf_cli_dir_params *)sess->params;

	bool b_action = false;
	bool b_dir = false;

	while (cur != NULL) {
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"action"))){
			xmlChar *key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			dir_params->action = strtol((const char *)key, (char **)NULL, 10);
			b_action = true;

			cclog(4,"action : %s", key);
			xmlFree(key);
		}

		if ((!xmlStrcmp(cur->name, (const xmlChar *)"report_address"))){
			xmlChar *key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if(dir_params->report_address == NULL){
				dir_params->report_address = strdup((const char *)key);
			}

			cclog(4,"report_address: %s", dir_params->report_address);
			xmlFree(key);
		}

		if ((!xmlStrcmp(cur->name, (const xmlChar *)"dir"))){
			xmlChar *key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			cclog(4,"dir : %s", key);

			if(xmlStrlen(key) > strlen("http://")){
				rf_url_list * url = cc_malloc(sizeof(rf_url_list));
				url->buffer = cc_malloc(xmlStrlen(key) + 1);
				strcpy(url->buffer,(char *)key);
				url->len = xmlStrlen(key);

				assert(sess->url_list == NULL);
				sess->url_list = url;
				b_dir = true;
			}
			else{
				cclog(1,"invalid url : %s",key);
				xmlFree(key);
				return RF_ERR_XML_INVALID_URL;
			}

			xmlFree(key);
		}

		cur = cur->next;
	}

	if(!(b_action && b_dir)){
		return RF_ERR_XML_PARSE;
	}

	return RF_OK;
}

int rf_xml_parse(rf_cli_session *sess,const char *buffer,int size){
	int ret = RF_OK;
	if(NULL == (doc = xmlParseMemory(buffer,size))){
		cclog(3,"Document not parsed successfully.[%s]", buffer);
		return RF_ERR_XML_PARSE;
	}

	if(NULL == (cur = xmlDocGetRootElement(doc))){
		cclog(1,"empty document.");
		ret = RF_ERR_XML_PARSE;
		goto OUT;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *) "method")) {
		cclog(1,"document of the wrong type, root node != method.");
		ret = RF_ERR_XML_PARSE;
		goto OUT;
	}

	xmlChar *method_name , *sessid;
	if(NULL == (method_name = xmlGetProp(cur,(const xmlChar *)"name"))){
		cclog(1,"get method name error!");
		ret = RF_ERR_XML_PARSE;
		goto OUT;
	}

	if(NULL == (sessid = xmlGetProp(cur,(const xmlChar *)"sessionid"))){
		cclog(1,"get sessionid error!");
		xmlFree(method_name);
		ret = RF_ERR_XML_PARSE;
		goto OUT;
	}

	cclog(4,"method name : %s,session id : %s",method_name,sessid);

	sess->method = action_atoi((char *)method_name);
	strcpy(sess->sessionid,(char *)sessid);

	xmlFree(method_name);
	xmlFree(sessid);

	cur = cur->xmlChildrenNode;
	switch (sess->method){
		case RF_FC_ACTION_URL_PURGE :
		case RF_FC_ACTION_URL_EXPIRE:
			ret = rf_parse_url(cur,sess);
			break;
		case RF_FC_ACTION_DIR_PURGE:
			if(!config.enable_dir_purge)
			{
				cclog(1,"Do not support dir purge!");
				ret = RF_ERR_XML_PARSE;
				break;
			}
		case RF_FC_ACTION_DIR_EXPIRE:
			ret = rf_parse_dir(cur,sess);
			break;
		default:
			cclog(1,"can not process method : %s",action_itoa(sess->method));
			break;
	};

OUT:
	xmlFreeDoc(doc);
	doc = NULL;
	return ret;
}
