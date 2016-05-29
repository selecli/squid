#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK
#define COOKIE_SIZE 50
#define MAX_NAME_SIZE 100
#define MAX_HEADER_SIZE 1024
static cc_module* mod = NULL;
/*
 * when the first time access,we must get param from uri,not from Cookie
 * 0: something wrong happens
 * 1: everything OK
 * */
static int get_param_from_uri(char *uri,char *primary_name,char *sub_key_name,char *file_name)
{
	char *pstr = strstr(uri,"://");
	if (NULL == pstr)
	{
		return 0;
	}
	pstr +=3 ;
	if (NULL == pstr)
	{
		return 0;
	}


	char *pos = pstr;
	char *pos_bak = pstr;
	int i = 0;
	pos = strchr(pos,'/');
//	while(pos = strchr(pos,'/'))
	while(pos)
	{
		if(2 == i)
		{
			strncpy(primary_name,pos_bak,pos - pos_bak);
			*(primary_name+ (pos - pos_bak)) = 0;
			debug(139,5)("primary_name = %s\n",primary_name);
		}
		if(3 == i)
		{
			strncpy(sub_key_name,pos_bak,pos - pos_bak);
			*(sub_key_name+ (pos - pos_bak)) = 0;
			debug(139,5)("sub_key_name = %s\n",sub_key_name);

		}
		pos ++;
		pos_bak = pos;
		i ++;
		debug(139,5)("pos = %s\n",pos);
		pos = strchr(pos,'/');

	}
	if(i <= 2)
	{
		debug(139,3)("not found primary key name\n");
		return 0;

	}
	if(3 == i)
	{
		debug(139,3)("not found sub key name\n");
		return 0;

	}

	char *psub_value =strchr(pos_bak,'.');
	if(NULL == psub_value)
	{
		return 0;
	}
	strncpy(file_name,pos_bak,psub_value-pos_bak);
	*(file_name+ (psub_value-pos_bak)) = 0;

	debug(139,5)("file_name = %s\n",file_name);

	return 1;

}
/*
 * change a string to its upper mod
 * no return value
 * */
/*
static void  to_upper(char str[]) 
{ 
    int count=0; 
    while( str[count] !='\0' ) 
    { 
        if( str[count]>=97 && str[count] <= 122 ) str[count]=str[count]-32; 
         count++; 
    } 

} 
*/
/****************************************************************
 *when has cookie header,each time we increase the count num,the 
 *rest remains the same
 ******************************************************************
 *return value:
 *0:something wrong happens
 *1:everything was OK
 * */
/*
static int deal_with_cookie(char* str)
{
	char *pstr = strchr(str,'=');
	if(NULL == pstr) {
		return 0;
	}
	char *start = pstr+1;
	if( NULL == start) {
		return 0;
	}
	char *end = strchr(start,')');
	if( NULL == end) {
		return 0;
	}

	char value[10];
	strncpy(value,start,end-start);
	*(value+(end-start))=0;
	debug(139,5)("value =%s \t num is: %d\n",value,atoi(value));

	int num = atoi(value);
	char stable_part[COOKIE_SIZE];
	if(num < 12)
		num ++; 
	else
		num = 12;
	strncpy(stable_part,str,pstr-str);
	*(stable_part+(pstr-str))='\0';
	snprintf(str,100,"%s=%d)",stable_part,num);
	debug(139,5)("str =[%s] \n",str);

	return 1;

}
*/
/*if the origindomain doesn't need cookie,this part is not neccessary 
 *
static int update_cookie_direction_0(clientHttpRequest *http)
{
	assert(http);
	assert(http->request);

	const char head_name_request[20] = "Cookie";
	int len = strlen(head_name_request);
	int flag = 0;
	HttpHeaderPos pos = HttpHeaderInitPos;

	request_t* request = http->request;

	char *uri = http->uri;
	char cookie_value[100]="0";
	
	char cookie_primary_key_name[20];
	char cookie_sub_key_name[20];
	int i = 0;

	debug(139,3)("in update_cookie_direction_0 receive uri %s\n",uri);

	HttpHeaderEntry e;
	HttpHeaderEntry *myheader;

	stringInit(&e.name, head_name_request);
	i = httpHeaderIdByNameDef(head_name_request, len);
	e.id = i;
	if (-1 == i)
		e.id = HDR_OTHER; 
	while ((myheader = httpHeaderGetEntry(&request->header, &pos))) 
	{
		if (strCaseCmp(myheader->name, head_name_request) == 0)
		{
			debug(139, 3)("%s is myheader->value,%s is name\n",myheader->value.buf,myheader->name.buf);
			flag = 1;
		}
	}

	debug(139,3)("flag=%d\n",flag);
	if(!flag)
	{
		if(0 == get_param_from_uri(uri,cookie_primary_key_name,cookie_sub_key_name))
		{
			return 1;
		}
		to_upper(cookie_primary_key_name);
		snprintf(cookie_value,100,"%s:(FM%s=0)",cookie_primary_key_name,cookie_sub_key_name);
		debug(139,3)("cookie value is %s\n",cookie_value);
		debug(139,3)("cookie_primary_key_name is %s\n",cookie_primary_key_name);
		debug(139,3)("cookie_sub_key_name is %s\n",cookie_sub_key_name);
		stringInit(&e.value, cookie_value);
		httpHeaderAddEntry(&request->header, httpHeaderEntryClone(&e));
	}
	return 1;
}
*/
static int update_cookie_direction_3(clientHttpRequest *http)
{
	assert(http);
	assert(http->request);

//	const char head_name_request[20] = "Cookie";
	const char head_name_reply[20] = "Set-Cookie";
//	int len = strlen(head_name_reply);
//	int flag = 0;
//	HttpHeaderPos pos = HttpHeaderInitPos;

//	request_t* request = http->request;
	HttpReply* reply = http->reply; 

	char *uri = http->uri;
//	char cookie_value[MAX_NAME_SIZE] = "0";
//	char cookie_value_from_url[MAX_NAME_SIZE] = "0 ";
	char cookie_primary_key_name[MAX_NAME_SIZE] = "0";
	char cookie_sub_key_name[MAX_NAME_SIZE] = "0";
	char cookie_file_name[MAX_NAME_SIZE] = "0";

	debug(139,3)("in update_cookie_direction_3 receive uri %s\n",uri);

	HttpHeaderEntry e;
//	HttpHeaderEntry *myheader;

	stringInit(&e.name, head_name_reply);
//	i = httpHeaderIdByNameDef(head_name_reply, len);
//	e.id = i;
	e.id = HDR_SET_COOKIE;
//	if (-1 == i)
//		e.id = HDR_OTHER; 
	/*
	while ((myheader = httpHeaderGetEntry(&request->header, &pos))) 
	{
		if (strCaseCmp(myheader->name, head_name_request) == 0)
		{
			debug(139, 3)("%s is myheader->value,%s is name\n",myheader->value.buf,myheader->name.buf);
			flag = 1;
			strcpy(cookie_value,myheader->value.buf);
			debug(139,5)("cookie_value =%s\n",cookie_value);
			if(0 == deal_with_cookie(cookie_value))
			{
				debug(139,3)("Cookie format is wrong\n");
				return 0;
			}
			if(0 == get_param_from_uri(uri,cookie_primary_key_name,cookie_sub_key_name))
			{
				debug(139,3)("get_param_from_uri error\n");
				return 0;
			}
			to_upper(cookie_primary_key_name);
			snprintf(cookie_value_from_url,100,"%s:(FM%s=",cookie_primary_key_name,cookie_sub_key_name);
			debug(139,5)("cookie_value_from_url is %s\n",cookie_value_from_url);
			if(strncmp(cookie_value,cookie_value_from_url,strlen(cookie_value_from_url)))
			{
				debug(139,3)("get_param_from_uri cookie_value_from_url doesn't match cookie_value\n");
				return 0;
			}

		}
	}
	*/

	if(reply->sline.status >= HTTP_OK && reply->sline.status < HTTP_BAD_REQUEST )
	{
		if(0 == get_param_from_uri(uri,cookie_primary_key_name,cookie_sub_key_name,cookie_file_name))
		{
			debug(139,3)("get_param_from_uri error\n");
			stringClean(&e.name);
			return 0;
		}
		struct tm *ptr;  
		ptr = gmtime(&reply->expires);  

		debug(139,3)("expires = %s\n", asctime(ptr));

		char tmpbuf[128];
		char cookie_value[MAX_HEADER_SIZE];

		strftime( tmpbuf, 128, "%a, %d %b %G %T GMT \n", ptr); 
		debug(139,3)("expires = %s\n", tmpbuf);
		snprintf(cookie_value,MAX_HEADER_SIZE,"DTAG=CAM=%s&LOC=%s&CRT=%s; expires=%s; path=/",cookie_primary_key_name,cookie_sub_key_name,cookie_file_name,tmpbuf);
		
		debug(139,5)("cookie value is %s\n",cookie_value);
		stringInit(&e.value, cookie_value);
		httpHeaderAddEntry(&reply->header, httpHeaderEntryClone(&e));
		stringClean(&e.value);
	}
	else
	{
		;//do nothing
	}
	stringClean(&e.name);


	return 1;
}

static int func_sys_init()
{

	debug(139,3)("mod_cookie-->func_sys_init: enter\n");
	return 0;
}
// module init 
int mod_register(cc_module *module)
{
	debug(139, 1)("(mod_cookie) ->  init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");

	cc_register_hook_handler(HPIDX_hook_func_sys_init,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_init),
			func_sys_init);

	cc_register_hook_handler(HPIDX_hook_func_http_repl_send_start,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_repl_send_start),
			update_cookie_direction_3);

	//mod = module;
	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
//	mod = (cc_module*)cc_modules.items[module->slot];
	
	return 1;
}

#endif
