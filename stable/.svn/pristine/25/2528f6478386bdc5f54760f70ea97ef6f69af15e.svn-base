#ifndef MOD_CUSTOMIZED_SERVER_SIDE_ERROR_PAGE
#define MOD_CUSTOMIZED_SERVER_SIDE_ERROR_PAGE
#define MAX_LOCATION_BYTES 256
#include <time.h>
/*
typedef struct _customized_server_side_error_page{
	int TriggerTimeSec;
	int ResponseStatus;
	time_t error_page_expires;
	
}customized_server_side_error_page;
*/

typedef struct _mod_config
{
	int TriggerTimeSec;  //time  seconds of connecting timeout
	int ResponseStatus; // responseStaus returned to client
	//int OptionalHttpStatusPattern[60];  //Optional response Status when enterring module
	time_t error_page_expires; //time error_page expires
	int OfflineTimeToLive;
//	int is_url;
	char location[MAX_LOCATION_BYTES];
	char OptionalHttpStatusPattern[256];
	MemBuf customized_error_text;
}mod_config;

typedef struct _error_page_private_data
{
	time_t TimeToTrigger;
//	time_t error_page_expires;
	int OfflineTimeToLive;
	int ResponseStatus;
	int recusive_time;
	int old_status; //backup the original status of server
	char OptionalHttpStatusPattern[256];
}error_page_private_data;
#endif
