#include "preloader.h"

static xmlDocPtr doc;
static xmlNodePtr cur;

int xml_parse_url_list(xmlNodePtr cur, char * tag, struct session_st * session)
{
	bool b_find;
	xmlChar * id;
	xmlChar * key;
    struct task_url_st * task_url;

	b_find = 0;
	cur = cur->xmlChildrenNode;

	while (NULL != cur)
	{
		if (0 == xmlStrcmp(cur->name, (const xmlChar *)tag))
		{
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);

			if(NULL == key || 0 == xmlStrlen(key))
			{
				xmlFree(key);
				goto NEXT;
			}

			id = xmlGetProp(cur,(const xmlChar *)"id");

			if(NULL == id || 0 == xmlStrlen(id))
			{
				xmlFree(id);
				xmlFree(key);
				goto NEXT;
			}

			b_find = 1;
            task_url = task_url_create((char *)id, (char *)key);
            if (NULL == task_url)
            {
                LogDebug(4, "XML parsed tag <url> error: id(%s); url(%s).", (char *)id, (char *)key);
            }
            else
            {
                list_insert_tail(session->taskurl_list, task_url);
                LogDebug(4, "XML parsed tag <url> succeed: id(%s); url(%s).", (char *)id, (char *)key);
            }
			xmlFree(key);
			xmlFree(id);
		}

NEXT:
		cur = cur->next;
	}

	return (b_find ? 0 : -1);
}

int XML_ParseUrl(xmlNodePtr cur, struct session_st *session)
{
	bool b_url = false;
	int ret = 0;

	while (cur != NULL)
	{
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"url_list")))
		{
			ret = xml_parse_url_list(cur, "url", session);

			if(ret < 0)
			{
				b_url = false;
				break;
			}
			else
			{
				b_url = true;
			}
		}
		cur = cur->next;
	}

	return b_url ? 0 : -1;
}

static int xml_parse_action_type(char * action_str)
{
	char * str;
	char * delim;
	char * token1;
	char * token2;
	char * saveptr;
	int action;
	int ret_action;

	delim = " ,\t";
	action = ACTION_NONE;
	ret_action = ACTION_NONE;

	str = xstrdup(action_str);

	token1 = strtok_r(str, delim, &saveptr);

	if (NULL == token1)
	{
		safe_process(free, str);
		return ACTION_NONE;
	}

	token2 = strtok_r(NULL, delim, &saveptr);

	if (NULL == token2)
	{
		if (0 == strcmp(token1, "preload"))
		{
			action = ACTION_PRELOAD;
		}
		else if (0 == strcmp(token1, "refresh"))
		{
			action = ACTION_REFRESH;
		}
		else if (0 == strcmp(token1, "remove"))
		{
			action = ACTION_REMOVE;
		}
		else
		{
			action = ACTION_NONE;
		}
	}
	else
	{
		if (strcmp(token1, "refersh") && strcmp(token2, "preload"))
		{
			action = ACTION_NONE;
		}
		else
		{
			action = ACTION_REFRESH_PRELOAD;
		}
	}

	safe_process(free, str);

	switch (action)
	{
		case ACTION_REFRESH:
		case ACTION_PRELOAD:
		case ACTION_REFRESH_PRELOAD:
		case ACTION_REMOVE:
			ret_action = action;
			break;

		case ACTION_NONE:
		default:
			ret_action = ACTION_NONE;
			break;
	}

	return ret_action;
}

int xml_parse_check_type(char * check_type_str)
{
	if (0 == strcasecmp(check_type_str, "md5"))
	{
		return CHECK_TYPE_MD5;
	}

	if (0 == strcasecmp(check_type_str, "sha1"))
	{
		return CHECK_TYPE_SHA1;
	}

	return CHECK_TYPE_BASIC;
}

int xml_parse_tags(xmlNodePtr cur, struct session_st * session)
{
	char * endptr;
	char * p_port;
	char * addr_str;
	unsigned int port;
	xmlChar * key;
	xmlChar * need_report;

	while (NULL != cur)
	{
		if (0 == xmlStrcmp(cur->name, (const xmlChar *)"action"))
		{
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			session->action = xml_parse_action_type((char *)key);

			LogDebug(4, "XML parsed tag <action>: %s.", (char *)key);

			xmlFree(key);

			if (ACTION_NONE == session->action)
			{
				/* action must be a valid value. */
				return -1;
			}
		}
		else if (0 == xmlStrcmp(cur->name, (const xmlChar *)"is_override"))
		{
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			session->is_override = (unsigned int)strtol((char *)key, NULL, 10);

			LogDebug(4, "XML parsed tag <is_override>: %s.", (char *)key);

			xmlFree(key);
		}
		else if (0 == xmlStrcmp(cur->name, (const xmlChar *)"check_type"))
		{
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			session->check_type = xml_parse_check_type((char *)key);

			LogDebug(4, "XML parsed tag <check_type>: %s.", (char *)key);

			xmlFree(key);
		}
		else if (0 == xmlStrcmp(cur->name, (const xmlChar *)"priority"))
		{
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			session->priority = (unsigned int)strtol((char *)key, NULL, 10);

			LogDebug(4, "XML parsed tag <priority>: %s.", (char *)key);

			xmlFree(key);
		}
		else if (0 == xmlStrcmp(cur->name, (const xmlChar *)"nest_track_level"))
		{
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			session->nest_track_level = (unsigned int)strtol((char *)key, NULL, 10);

			LogDebug(4, "XML parsed tag <nest_track_level>: %s.", (char *)key);

			xmlFree(key);
		}
		else if (0 == xmlStrcmp(cur->name, (const xmlChar *)"limit_rate"))
		{
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);

			session->limit_rate = (unsigned int)strtol((char *)key, &endptr, 10);

            session->limit_rate *= 1024 / 8;

			LogDebug(4, "XML parsed tag <limit_rate>: %s.", (char *)key);

			xmlFree(key);
		}
		else if (0 == xmlStrcmp(cur->name, (const xmlChar *)"preload_address"))
		{
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);

			addr_str = xmalloc(xmlStrlen(key) + 1);
			strcpy(addr_str, (char *)key);

			p_port = strchr(addr_str, ':');

			if (NULL == p_port)
			{
				port = 80;	
                LogError(1, "XML parsed tag <preload_address> error: lack the port, default use 80.");
			}
			else
			{
				*p_port = '\0';
				p_port++;

				if ('\0' == *p_port)
				{
					port = 80;	
                    LogError(1, "XML parsed tag <preload_address> error: lack the port, use default 80.");
				}
				else
				{
					port = (unsigned int)strtol(p_port, NULL, 10);

					if (port < 0 || port > 65535)
					{
						port = 80;
                        LogError(1, "XML parsed tag <preload_address> error: port(%u) beyonds the limit, use default 80.", port);
					}
				}
			}

			session->preload_addr.sin_family = AF_INET;
			session->preload_addr.sin_port = htons(port);
			if (inet_pton(AF_INET, addr_str, (void *)&session->preload_addr.sin_addr) < 0)
			{
				safe_process(free, addr_str);
                LogError(1, "XML parsed tag <preload_address> error for \"%s\": %s.", (char *)key, xerror());
				return -1;
			}
			safe_process(free, addr_str);
			LogDebug(4, "XML parsed tag <preload_address>: %s.", (char *)key);
			xmlFree(key);
		}
		else if (0 == xmlStrcmp(cur->name, (const xmlChar *)"report_address"))
		{
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			need_report = xmlGetProp(cur, (const xmlChar *)"need");
			if(NULL == need_report)
			{
				xmlFree(key);
                LogError(1, "XML parsed tag <report_address> error: lack the key 'need'.");
				return -1;
			}

			if (0 == strcmp((char *)need_report, "yes"))
			{
				session->need_report = 1;

				addr_str = xmalloc(xmlStrlen(key) + 1);
				strcpy(addr_str, (char *)key);

				p_port = strchr(addr_str, ':');

				if (NULL == p_port)
				{
					port = 80;	
                    LogError(1, "XML parsed tag <report_address> error: lack the port, use default 80.");
				}
				else
				{
					*p_port = '\0';
					p_port++;

					if ('\0' == *p_port)
					{
						port = 80;	
                        LogError(1, "XML parsed tag <report_address> error: lack the  port, use default 80.");
					}
					else
					{
						port = (unsigned int)strtol(p_port, NULL, 10);

						if (port < 0 || port > 65535)
						{
							port = 80;
                            LogError(1, "XML parsed tag <report_address> error: port(%u) beyonds the limit, use default 80.", port);
						}
					}
				}

				session->report_addr.sin_family = AF_INET;

				session->report_addr.sin_port = htons(port);

				if (inet_pton(AF_INET, addr_str, (void *)&session->report_addr.sin_addr) < 0)
				{
					safe_process(free, addr_str);
					xmlFree(key);
                    xmlFree(need_report);
                    LogError(1, "XML parsed tag <report_address> error for \"%s\": %s.", (char *)key, xerror());
					return -1;
				}

				safe_process(free, addr_str);
			}

			LogDebug(4, "XML parsed tag <report_address>: %s.", (char *)key);

			xmlFree(key);
            xmlFree(need_report);
		}
		else if (0 == xmlStrcmp(cur->name, (const xmlChar *)"url_list"))
		{
			if (xml_parse_url_list(cur, "url", session) < 0)
			{
				return -1;
			}
		}

		cur = cur->next;
	}

	return 0;
}

void * xml_parse(const char * buffer, const size_t size)
{
	xmlChar * session_id;
	struct session_st * session;

    LogDebug(5, "xml parsing data: \n%s", buffer);

	doc = xmlParseMemory(buffer, size);

	if(NULL == doc)
	{
        LogError(1, "XML parsed document failed: %s", buffer);
		return NULL;
	}

	cur = xmlDocGetRootElement(doc);

	if(NULL == cur)
	{
		xmlFreeDoc(doc);
        LogError(1, "XML parsed document error: it's empty.");
		return NULL;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *) "preload_task"))
	{
		xmlFreeDoc(doc);
        LogError(1, "XML parsed tag <preload_task> error: root node must be \"preload_task\".");
		return NULL;
	}

	session_id = xmlGetProp(cur, (const xmlChar *)"sessionid");

	if(NULL == session_id)
	{
		xmlFreeDoc(doc);
        LogError(1, "XML parsed tag <preload_task> error: lack the key 'sessionid'.");
		return NULL;
	}

    session = xcalloc(1, g_tsize.session_st);
    session->taskurl_list = list_create();
	session->start_time = time(NULL);
	session->id = xstrdup((char *)session_id);
	pthread_mutex_init(&session->mutex, NULL);

	LogDebug(1, "XML parsed tag <preload_task>: (sessionid) %s.", session_id);

	xmlFree(session_id);

	cur = cur->xmlChildrenNode;

	if (xml_parse_tags(cur, session) < 0)
	{
        session_free(session);
		xmlFreeDoc(doc);
		return NULL;
	}

	xmlFreeDoc(doc);

	return session;
}
