#include "cc_framework_api.h"

static int needRealUnlink=0;
extern int squidaio_get_queue_len(void);
#ifdef CC_FRAMEWORK
#define CC_SUGGESTION_NUM 8000
//#define CC_UNLINK_SIZE 2000
/* struct suggestNode{ signed int suggest; SwapDir *sd; }; static len=0; static struct suggestNode suggestArray[CC_SUGGESTION_NUM]; */

static int overloading=0;
struct _SwapDirUnlinkData
{
	//SwapDir *sd;
	char sd_path[256];
	signed int suggestArray[CC_SUGGESTION_NUM];
	int len;
	int top;
};
typedef struct _SwapDirUnlinkData SwapDirUnlinkData;

/* struct UnlinkData { char sd_path[256]; signed int *unlinkFileN; int size; int len; int top; }; */


/* struct UnlinkDataArray { }; */
//static struct UnlinkData* ud; //UnlinkData for real unlink
static Array sdudp; /*swapDirUnlinkDataPointer*/
static MemPool * SwapDirUnlinkDataPool = NULL;
static cc_module* mod = NULL;

static void * SwapDirUnlinkDataPoolAlloc(void)
{
	void * obj = NULL;
	if (NULL == SwapDirUnlinkDataPool)
	{
		SwapDirUnlinkDataPool = memPoolCreate("mod_do_not_unlink other_data SwapDirUnlinkData", sizeof(SwapDirUnlinkData));
	}
	return obj = memPoolAlloc(SwapDirUnlinkDataPool);
}

static int is_disk_overloading()
{
	overloading=1;
	return 0;
}

int freeDiskErrorFlag(void * data)
{
	int *free_data = data;
	if (free_data)
		xfree(free_data);
	return 0;
}

static void setDiskErrorFlag(StoreEntry * e, int error_flag)
{
	if (e->swap_dirn < 0 || e->swap_filen < 0)
		return;
	SwapDir *SD = &Config.cacheSwap.swapDirs[e->swap_dirn];
	if (strcmp(SD->type, "aufs"))
		return;
	int *settedFlag = cc_get_mod_private_data(STORE_ENTRY_PRIVATE_DATA, e, mod);
	if (settedFlag)
		return;
	int *flag = xcalloc(1, sizeof(int));
	*flag = 0;
	if (error_flag)
		*flag = 1;
	else if(e->swap_filen >=0 && e->swap_dirn >= 0)
	{
		sfileno f = e->swap_filen;
		char *path = storeAufsDirFullPath(SD, f, NULL);     
		uid_t owner = geteuid();
		gid_t group = getegid();
		struct stat buf;
		if (!stat(path, &buf))
		{
			if (owner != buf.st_uid || group != buf.st_gid)
				*flag = 1;
		}
	}
	cc_register_mod_private_data(STORE_ENTRY_PRIVATE_DATA, e, flag, freeDiskErrorFlag, mod);
}

static int getDiskErrorFlag(StoreEntry * e)
{
	int *flag = cc_get_mod_private_data(STORE_ENTRY_PRIVATE_DATA, e, mod);
	if (flag && *flag)
		return 1;
	return 0;
}

static int needUnlink()
{
	int i;
	/* for(i=0;i<Config.cacheSwap.n_configured;i++) { */
	/* if(strstr(full_path,sdudp[i].sd_path)) { if(sdudp[i].len>=CC_SUGGESTION_NUM) { debug(130,1)("mod_do_not_unlink: need real unlink operation\n"); return 1; } return 0; } */

	//}
	i = needRealUnlink;
	if(i)
	{
		debug(130,2)("need realUnlink\n");
	}
	needRealUnlink = 0;
	return i;

}
static int put_suggestion(SwapDir *SD, sfileno fn)
{
	//assert(SD)
	
	int i;
	for(i=0;i<Config.cacheSwap.n_configured;i++)
	{
		if(!store_dirs_rebuilding && !strcmp(SD->path,((SwapDirUnlinkData*)sdudp.items[i])->sd_path))
		{
			if(((SwapDirUnlinkData*)sdudp.items[i])->len >=CC_SUGGESTION_NUM)
			{

				needRealUnlink = 1; //set needRealUnlink, when queue the request to the aio queue, the value will be reset
				debug(130,2)("put suggest num %d to fileN pool fail, because the stack is full, \n",fn);
				//if(ud[i].len==0 || (ud[i].len %CC_UNLINK_NUM )!=0)
				/* if(ud[i].len<ud[i].size) { ud[i].len++; } else { ud[i].unlinkFileN = xrealloc(ud[i].unlinkFileN,ud[i].size+CC_UNLINK_SIZE); ud[i].size +=CC_UNLINK_SIZE; int n ; for(n = ud[i].len;n<ud[i].size;n++) { ud[i].unlinkFileN[n] = -1; } ud[i].len++; } debug(130,3)("put_suggestion: the size of unlinkFileN is: %d,\t the len of the unlinkFileN is: %d\n ",ud[i].size,ud[i].len); ud[i].unlinkFileN[top++] = fn; */
				return -1;
			}
			//int j;
			debug(130,3)("put suggest num is: %d \n",fn);
			((SwapDirUnlinkData*)sdudp.items[i])->suggestArray[((SwapDirUnlinkData*)sdudp.items[i])->top++] = fn;
			((SwapDirUnlinkData*)sdudp.items[i])->len++;
			return 0;
			/*
			for(j=0;j<CC_SUGGESTION_NUM;j++)
			{
				if(sdudp[i].suggestArray[j]== -1)
				{
					debug(130,5)("put suggest num is: %d\n",fn);
					sdudp[i].suggestArray[j]= fn;
					sdudp[i].len++;
					return 0;
				}


			}
			*/

		}
	}
		return -1;

	}

static int get_suggestion(SwapDir* SD)
{
	//debug(130,3)("get_suggestion enter\n");
	int i;
	for(i=0;i<Config.cacheSwap.n_configured;i++)
	{
		debug(130,6)("get_suggestion: SD->path is: %s\n",SD->path);
		if(!strcmp(((SwapDirUnlinkData*)sdudp.items[i])->sd_path,SD->path))
		{
			debug(130,6)("get_suggestion: the ((SwapDirUnlinkData*)sdudp.items[i])->sd_path is: %s, the ((SwapDirUnlinkData*)sdudp.items[i])->len is: %d, and the size is: %d \n",((SwapDirUnlinkData*)sdudp.items[i])->sd_path, ((SwapDirUnlinkData*)sdudp.items[i])->len, CC_SUGGESTION_NUM);
			if(((SwapDirUnlinkData*)sdudp.items[i])->len<=0)
				return -1;
			int load = SD->checkload(SD, ST_OP_CREATE);
			if (load < 0 || load > 1000)
			{
			debug(130,2)("get_suggestion: sd->path %s, load is %d too heavy\n",((SwapDirUnlinkData*)sdudp.items[i])->sd_path, load);
				return -1;
			}
			if(overloading==1)
			{
				overloading=0;
			debug(130,2)("get_suggestion: sd->path %s, disk load is too heavy\n",((SwapDirUnlinkData*)sdudp.items[i])->sd_path);
				return -1;
			}
			/*
			int queue_len = squidaio_get_queue_len();
			if(queue_len > 50)
			{
				debug(130,1)("get_suggestion: the squidaio_queue_len is too large: %d\n",squidaio_get_queue_len());
				return -1;
			}
			*/
			debug(130,4)("get_suggestion: the squidaio_queue_len is: %d\n",squidaio_get_queue_len());
			((SwapDirUnlinkData*)sdudp.items[i])->len--;
			int fn = ((SwapDirUnlinkData*)sdudp.items[i])->suggestArray[((SwapDirUnlinkData*)sdudp.items[i])->top -1];
			((SwapDirUnlinkData*)sdudp.items[i])->suggestArray[--((SwapDirUnlinkData*)sdudp.items[i])->top] = -1;

			debug(130,3)("get suggest num is: %d\n",fn);
			return fn;
			/*
			   int j;
			   for(j = 0;j<CC_SUGGESTION_NUM;j++)
			   {
			   if(sdudp[i].suggestArray[j] !=-1)
			   {
			   debug(130,5)("get suggest num is: %d\n",sdudp[i].suggestArray[j]);
			   int fn = sdudp[i].suggestArray[j];
			   sdudp[i].suggestArray[j]=-1;
			   sdudp[i].len--;
			   return fn;

			   }
			   }
			   debug(130,9)("get_suggestion: no suggestArray[j] is not -1\n");
			   if(sdudp[i].len<=0)
			   return -1;
			   */
		}
	}
	return -1;
}

static int func_sys_init()
{
	debug(130,1)("mod_do_not_unlink: func_sys_init enter\n");
	int i;
  	arrayInit(&sdudp);

	for(i = 0 ;i <Config.cacheSwap.n_configured; i++)
	{
		SwapDirUnlinkData * sdud = SwapDirUnlinkDataPoolAlloc();
		//swapDirUnlinkDataPointer[i].sd_path = Config.cacheSwap.swapDirs[i].path	;
		strncpy(sdud->sd_path,Config.cacheSwap.swapDirs[i].path,strlen(Config.cacheSwap.swapDirs[i].path));
		
		//strncpy(ud[i].sd_path,Config.cacheSwap.swapDirs[i].path,strlen(Config.cacheSwap.swapDirs[i].path));
		int j;	
		for(j = 0 ;j<CC_SUGGESTION_NUM; j++)
		{
			sdud->suggestArray[j]=-1;
			//suggestArray[j].sd = NULL;
		}

		sdud->len = 0 ;
		sdud->top=0;

		/*
		for(j = 0;j<CC_UNLINK_SIZE;j++)
		{
			ud[i].unlinkFileN[j]=-1;
		}
		ud[i].size = CC_UNLINK_SIZE;
		ud[i].len = 0;
		ud[i].top = 0;

		*/
		arrayAppend(&sdudp, sdud);
	}
	return 0;
}

static int cleanup(cc_module* mod)
{
	int i;
	for(i=0; i<sdudp.count; i++)
	{
		if(sdudp.items[i])
			memPoolFree(SwapDirUnlinkDataPool, sdudp.items[i]);
	}
	arrayClean(&sdudp);
	if(NULL != SwapDirUnlinkDataPool)
		memPoolDestroy(SwapDirUnlinkDataPool);
	return 0;
}

int mod_register(cc_module *module)
{
	strcpy(module->version, "7.0.R.16488.i686");
	//module->hook_func_sys_init = func_sys_init;
	cc_register_hook_handler(HPIDX_hook_func_sys_init,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_init),
			func_sys_init);
	//module->hook_func_private_get_suggest = get_suggestion;
	cc_register_hook_handler(HPIDX_hook_func_private_get_suggest,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_get_suggest),
			get_suggestion);
	//module->hook_func_private_put_suggest = put_suggestion;
	cc_register_hook_handler(HPIDX_hook_func_private_put_suggest,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_put_suggest),
			put_suggestion);
	//module->hook_func_private_setDiskErrorFlag = setDiskErrorFlag;
	cc_register_hook_handler(HPIDX_hook_func_private_setDiskErrorFlag,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_setDiskErrorFlag),
			setDiskErrorFlag);
	//module->hook_func_private_getDiskErrorFlag = getDiskErrorFlag;
	cc_register_hook_handler(HPIDX_hook_func_private_getDiskErrorFlag,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_getDiskErrorFlag),
			getDiskErrorFlag);
	//module->hook_func_private_needUnlink = needUnlink;
	cc_register_hook_handler(HPIDX_hook_func_private_needUnlink,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_needUnlink),
			needUnlink);
	//module->hook_func_sys_cleanup = cleanup;
	cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_cleanup),
			cleanup);
	//module->hook_func_private_is_disk_overloading = is_disk_overloading;
	cc_register_hook_handler(HPIDX_hook_func_private_is_disk_overloading,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_is_disk_overloading),
			is_disk_overloading);


	//mod = module;
	//mod = (cc_module*)cc_modules.items[module->slot];
	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	return 0;
}
#endif
