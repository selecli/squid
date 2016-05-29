#include "cc_framework_api.h"
#include <stdbool.h>

#ifdef CC_FRAMEWORK

static cc_module *mod = NULL;
static uint32_t file_total = 0;

typedef enum {
	ST_KB_2,
	ST_KB_4,
	ST_KB_8,
	ST_KB_16,
	ST_KB_32,
	ST_KB_64,
	ST_MB_1,
	ST_MB_5,
	ST_MB_20,
	ST_MB_50,
	ST_MB_100,
	ST_GB_1,
	ST_GB_2,
	ST_GB_4,
	ST_GB_UNLIMITED
}size_flag;

typedef struct _st_form_t
{
	size_flag file_flag;
	unsigned long long int file_size;
	uint32_t file_count;
	char s[16];
} st_form;

#define GB_UNLIMITED 1<<31
st_form st_array[] = {
	{ST_KB_2, 2, 0, "0-2KB"},
	{ST_KB_4, 4, 0, "2-4KB"},
	{ST_KB_8, 8, 0, "4-8KB"},
	{ST_KB_16, 16, 0, "8-16KB"},
	{ST_KB_32, 32, 0, "16-32KB"},
	{ST_KB_64, 64, 0, "32-64KB"},
	{ST_MB_1, 1024, 0, "64KB-1MB"},
	{ST_MB_5, 6120, 0, "1-5MB"},
	{ST_MB_20, 24480, 0, "5-20MB"},
	{ST_MB_50, 61200, 0, "20-50MB"},
	{ST_MB_100, 122400, 0, "50-100MB"},
	{ST_GB_1, 1048576, 0, "100MB-1GB"},
	{ST_GB_2, 2097152, 0, "1-2GB"},
	{ST_GB_4, 4194304, 0, "2-4GB"},
	{ST_GB_UNLIMITED, GB_UNLIMITED, 0, "4GB-UNLIMITED"}
};

static size_flag st_transfer_kb(unsigned long long int file_size);
static OBJH fileSizeStat;
static void fileSizeStat(StoreEntry * sentry)
{
	debug(93 ,2)("fileSizeStat !!");
	size_flag loc_flag; 
	uint32_t total; 
	if(file_total == 0)
		total = 1;
	else 
		total = file_total;
	storeAppendPrintf(sentry, "Start Time:\t%s\n",
			mkrfc1123(squid_start.tv_sec));
	storeAppendPrintf(sentry, "Current Time:\t%s\n",
			mkrfc1123(current_time.tv_sec));
	storeAppendPrintf(sentry, "There are %d objects on the disks\n", n_disk_objects);
	storeAppendPrintf(sentry, "From nearly run, there are %d new additional objects,"
			" about %.1f%% in all objects\n", file_total, (float)file_total*100/n_disk_objects);
	for(loc_flag = ST_KB_2;loc_flag <= ST_GB_UNLIMITED; loc_flag++) 
		storeAppendPrintf(sentry, "%16s:count	%d		percent	%.1f%%\n", st_array[loc_flag].s, \
				st_array[loc_flag].file_count, (float)st_array[loc_flag].file_count*100/total);
}

static void st_module_dest()
{
	
}

/*
static void st_sub_count(unsigned long long int file_size)
{
	size_flag loc_flag;
	loc_flag = st_transfer_kb(file_size/1024);
	if(st_array[loc_flag].file_count)
		st_array[loc_flag].file_count--;
	if(file_total)
		file_total--;
}
*/

static int st_module_init()
{
	debug(93 ,2)("register file_size_stat to squid !!");
	cachemgrRegister("file_size_stat", "file size statistic", fileSizeStat, 0, 1);
	return 1;
}

static size_flag st_transfer_kb(unsigned long long int file_size)
{
	size_flag loc;
	for(loc = ST_KB_2;loc <= ST_GB_UNLIMITED; loc ++) {
		if( file_size < st_array[loc].file_size)
			break;
	}
	return loc;
}
static void  st_add_count(unsigned long long int file_size,StoreEntry *e)
{
	size_flag  loc_flag;
	debug(93 ,2)("(mod_statistic) --> st_add_count: %llu\n", file_size);
	loc_flag = st_transfer_kb(file_size/1024);
	st_array[loc_flag].file_count++;
	file_total++;

	/*
	int *flag = xmalloc(sizeof(int));
	*flag = 1 << loc_flag;
	cc_register_mod_private_data(STORE_ENTRY_PRIVATE_DATA,e,flag,xfree,mod);
	*/

}
/****************** start module ****************************/

static int hook_cleanup(cc_module *module)
{
	debug(93, 2)("(cc_mod_statistic)  --> hook_cleanup: cleanup\n");
	st_module_dest();

	return 1;
}

/*
static int hook_store_unlink(StoreEntry * e)
{
	if(store_dirs_rebuilding)
		return 1;

	if(EBIT_TEST(e->flags, KEY_PRIVATE)){
		debug(93,3)("cc_mod_statistic -> hook_store_unlink : %s is private !\n",storeKeyText(e->hash.key));
		return 1;
	}

	assert(e != NULL);
	if(e->mem_obj){
		debug(93, 2)("(cc_mod_statistic)  -->   hook_store_unlink, %lld e->swqp_file_sz\n", e->mem_obj->object_sz);
		st_sub_count(e->mem_obj->object_sz);
	}

	return 1;
}
*/


static int hook_init(cc_module *module)
{
	debug(93, 2)("(cc_mod_statistic)  -->   hook_init:init\n");

	st_module_init();
	return 1;
}

static int hook_http_read_reply(int fd,HttpStateData *data)
{
	debug(93, 2)("(cc_mod_statistic)  -->   hook_http_read_reply: read reply\n");
	MemObject *mem = data->entry->mem_obj;

	if(mem == NULL)
		return 1;

	unsigned long long int file_size = mem->reply->content_length;

	if(file_size < 0)
		return 1;

	debug(93, 3)("(cc_mod_statistic)  --> hook_http_read_reply:   %llu, %p\n", file_size, data);
	st_add_count(file_size,data->entry);

	return 1;
}

/*
static int hook_store_read_head(store_client *sc){
	debug(93,2)("(cc_mod_statistic) --> hook_store_read_head:  \n");
	StoreEntry *e = sc->entry;
	MemObject *mem = e->mem_obj;

	if(mem == NULL)
		return 1;

	debug(93,2)("(cc_mod_statistic) -->hook_store_read_head : read from store, object_size : %lld\n",mem->object_sz);
	st_add_count(mem->object_sz);
	return 1;
}
*/

/* module init */
int mod_register(cc_module *module)
{
	debug(93, 2)("(cc_mod_statistic)  -->   init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");
	/* necessary functions*/
	//module->hook_func_sys_cleanup = hook_cleanup;
	cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_cleanup),
			hook_cleanup);
	//module->hook_func_sys_init = hook_init;
	cc_register_hook_handler(HPIDX_hook_func_sys_init,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_init),
			hook_init);

	//module->hook_func_http_repl_read_end = hook_http_read_reply;
	cc_register_hook_handler(HPIDX_hook_func_http_repl_read_end,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_repl_read_end),
			hook_http_read_reply);

	//mod = module;
	//mod = (cc_module*)cc_modules.items[module->slot];
	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	return 1;
}
#endif
