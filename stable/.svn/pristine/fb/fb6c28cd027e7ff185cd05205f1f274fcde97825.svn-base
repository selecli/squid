#include "cc_framework_api.h"
#ifdef CC_FRAMEWORK

void * cc_mod_param_pool_alloc(void)
{
    void * obj = NULL;
    if (NULL == cc_mod_param_pool)
    {    
        cc_mod_param_pool = memPoolCreate("cc_mod_param", sizeof(cc_mod_param));
    }    
    return obj = memPoolAlloc(cc_mod_param_pool);
}

void * cc_private_data_pool_alloc()
{
	void * obj = NULL;
	if (NULL == cc_private_data_pool)
	{
		cc_private_data_pool = memPoolCreate("cc_private_data", Config.cc_all_mod_num*sizeof(cc_private_data));
	}
	return obj = memPoolAlloc(cc_private_data_pool);
}

int cc_register_mod_param(cc_module *mod, void *param, FREE_CALLBACK *call_back)
{
    assert(param && call_back); 
    if (reconfigure_in_thread)
        mod = (cc_module*)cc_modules_r.items[mod->slot];
    else 
        mod = (cc_module*)cc_modules.items[mod->slot];

    assert(mod && mod->flags.param);
    cc_mod_param *mod_param = (cc_mod_param*)cc_mod_param_pool_alloc();
    mod_param->count = 0;
    mod_param->param = param;
    mod_param->call_back = call_back;
    arrayAppend(&mod->mod_params, mod_param);
    debug(90,2)( "cc_register_mod_param: %s (mod->slot=%d), cc_mod_param->param === %p, cc_mod_param->call_back ====%p\n ",mod->name, mod->slot, param,call_back);
	mod->cc_mod_param_call_back = call_back;
    return 1;
}

void *cc_get_global_mod_param(cc_module *mod)
{
	if (0==mod->flags.param || 0==mod->mod_params.count)
		return NULL;

	cc_mod_param *mod_param = NULL;
	mod_param = mod->mod_params.items[0];
	return mod_param ? mod_param->param : NULL;
}

void *cc_get_mod_param(int fd, cc_module *mod)
{
	if (0==mod->flags.param || 0==fd_table[fd].matched_mod_params[mod->slot].count)
		return NULL;
	
	cc_mod_param *mod_param = NULL;
	mod_param = fd_table[fd].matched_mod_params[mod->slot].items[0];
    return mod_param ? mod_param->param: NULL;
}

void *cc_get_mod_param_by_FwdState(FwdState *fwd, cc_module *mod)
{
    if (0==mod->flags.param ||  NULL == fwd->old_fd_params.matched_mod_params)
        return NULL;
    if (0 ==  fwd->old_fd_params.matched_mod_params[mod->slot].count)
        return NULL;
    cc_mod_param *mod_param = NULL;
    mod_param = fwd->old_fd_params.matched_mod_params[mod->slot].items[0];
    return mod_param ? mod_param->param: NULL;
}

Array * cc_get_mod_param_array(int fd, cc_module *mod)
{
	Array * ret = &fd_table[fd].matched_mod_params[mod->slot];
	return ret;
}

int cc_get_mod_param_count(int fd, cc_module *mod)
{
	return fd_table[fd].matched_mod_params[mod->slot].count;
}

/*
 * data_t: where do pri_data hook =FDE_PRIVATE_DATA/REQUEST_PRIVATE_DATA/STORE_ENTRY_PRIVATE_DATA
 * tag: where do pri_data hook =/fde/clientHttpRequest/StoreEntry
 * call_back: which is handle pri_data freeing
 */
int cc_register_mod_private_data(mod_private_data_t data_t, void *tag, void *private_data, FREE_CALLBACK *call_back, cc_module *mod)
{
	switch (data_t)
	{
		case FWDSTATE_PRIVATE_DATA:
			{
				FwdState* fwdState = (FwdState*)tag;
				if( !fwdState->cc_fwdState_private_data ) {
					fwdState->cc_fwdState_private_data = (cc_private_data*)cc_private_data_pool_alloc();
				}
				fwdState->cc_fwdState_private_data[mod->slot].private_data = private_data;
				fwdState->cc_fwdState_private_data[mod->slot].call_back = call_back;
				break;
			}
	
		case FDE_PRIVATE_DATA:
			{
				//FIXME it does not work
				if (sizeof(*(int *)tag) == sizeof(int))
				{
					fde *F = &fd_table[*(int *)tag];
					cc_private_data *p_data = NULL;
					if (!F->cc_fde_private_data)
						F->cc_fde_private_data = (cc_private_data*) cc_private_data_pool_alloc();
					p_data = &F->cc_fde_private_data[mod->slot];					
					p_data->private_data = private_data;
					p_data->call_back = call_back;
				}
				else
					debug(90, 1) ("cc_register_mod_private_data: %s register FDE_PRIVATE_DATA error because of tag's or data_t's err!\n", mod->name);
				break;				
			}				
		case REQUEST_PRIVATE_DATA:
			{
				//FIXME it does not work
				if (sizeof(*(clientHttpRequest *)tag)==sizeof(clientHttpRequest))
				{	
					clientHttpRequest *httpReq = (clientHttpRequest *)tag;
					cc_private_data *p_data = NULL;
					if (!httpReq->cc_clientHttpRequest_private_data)
						httpReq->cc_clientHttpRequest_private_data = (cc_private_data*)cc_private_data_pool_alloc();
					p_data = &httpReq->cc_clientHttpRequest_private_data[mod->slot];
					p_data->private_data = private_data;
					p_data->call_back = call_back;
				}
				else
					debug(90, 1) ("cc_register_mod_private_data: %s register REQUEST_PRIVATE_DATA error because of tag's or data_t's err!\n", mod->name);
				break;

			}
		case REQUEST_T_PRIVATE_DATA:
			{
				request_t *request = (request_t *)tag;
				cc_private_data *p_data = NULL;
				if (!request->cc_request_private_data)
					request->cc_request_private_data = (cc_private_data*)cc_private_data_pool_alloc();
				p_data = &request->cc_request_private_data[mod->slot];
				p_data->private_data = private_data;
				debug(90,5)("p_data->private_data =  %p\n", p_data->private_data);
				p_data->call_back = call_back;
				break;

			}
		case STORE_ENTRY_PRIVATE_DATA:
			{
				//FIXME it does not work
				if (sizeof(*(StoreEntry *)tag) == sizeof(StoreEntry))
				{
					StoreEntry *entry = (StoreEntry *)tag;
					cc_private_data *p_data = NULL;
					if (!entry->cc_StoreEntry_private_data)
						entry->cc_StoreEntry_private_data = (cc_private_data*)cc_private_data_pool_alloc();
					p_data = &entry->cc_StoreEntry_private_data[mod->slot];
					p_data->private_data = private_data;
					p_data->call_back = call_back;
				}
				else
					debug(90, 1) ("cc_register_mod_private_data: %s register STORE_ENTRY_PRIVATE_DATA error because of tag's or data_t's err!\n", mod->name);
				break;				
			}
		case CONN_STATE_DATA:
			{
				if (sizeof(*(ConnStateData *)tag) == sizeof(ConnStateData))
				{
					ConnStateData *conn = (ConnStateData *)tag;
					cc_private_data *p_data = NULL;
					if (!conn->cc_connStateData_private_data)
						conn->cc_connStateData_private_data = (cc_private_data*)cc_private_data_pool_alloc();
					p_data = &conn->cc_connStateData_private_data[mod->slot];
					p_data->private_data = private_data;
					p_data->call_back = call_back;
				}
				else
					debug(90, 1) ("cc_register_mod_private_data: %s register CONN_STATE_DATA error because of tag's or data_t's err!\n", mod->name);
				break;				
			}
		case MEMOBJECT_PRIVATE_DATA:
			{
				MemObject *mem = (MemObject *)tag;
				cc_private_data *p_data = NULL;
				if(!mem->cc_MemObject_private_data)
				{
					mem->cc_MemObject_private_data = xcalloc(Config.cc_all_mod_num, sizeof(cc_private_data));
				}
				p_data = &mem->cc_MemObject_private_data[mod->slot];
				p_data->private_data = private_data;
				p_data->call_back = call_back;

				break;
			}
		default:
		;
	}
	return 1;
}


void *cc_get_mod_private_data(mod_private_data_t data_t, void *tag, cc_module *mod)
{
	cc_private_data *p_data = NULL;
	switch (data_t)
	{
		case FWDSTATE_PRIVATE_DATA:
			{
				FwdState* fwdState = (FwdState*)tag;

				if( !fwdState->cc_fwdState_private_data )
					return NULL;

				return fwdState->cc_fwdState_private_data[mod->slot].private_data;
			}
		case FDE_PRIVATE_DATA:
			{
					fde *F = &fd_table[*(int *)tag];
					if (F->cc_fde_private_data)
					{
						p_data = &F->cc_fde_private_data[mod->slot];
						if (p_data && p_data->private_data)
							return p_data->private_data;
					}
				
				break;
			}				
		case REQUEST_PRIVATE_DATA:
			{
				clientHttpRequest *httpReq = (clientHttpRequest *)tag;
				if (httpReq->cc_clientHttpRequest_private_data)
				{
					p_data = &httpReq->cc_clientHttpRequest_private_data[mod->slot];
					if (p_data && p_data->private_data)
						return p_data->private_data;
				}
				break;
			}


		case REQUEST_T_PRIVATE_DATA:
			{
				request_t *request = (request_t *)tag;
				if (request->cc_request_private_data)
				{
					p_data = &request->cc_request_private_data[mod->slot];
					debug(90,5)(" p_data = %p , p_data->private_data = %p\n",  p_data, p_data->private_data);
					if (p_data && p_data->private_data) {
						return p_data->private_data;
					}
				}

				break;
				
			}

		case STORE_ENTRY_PRIVATE_DATA:
			{
				if (sizeof(*(StoreEntry *)tag)==sizeof(StoreEntry))
				{
					StoreEntry *entry = (StoreEntry *)tag;
					if (entry->cc_StoreEntry_private_data)
					{
						p_data = &entry->cc_StoreEntry_private_data[mod->slot];
						if (p_data && p_data->private_data)
							return p_data->private_data;
					}
				}
				else
					goto err;
				break;
			}
		case CONN_STATE_DATA:
			{
				if (sizeof(*(ConnStateData *)tag)==sizeof(ConnStateData))
				{
					ConnStateData *conn = (ConnStateData *)tag;
					if (conn->cc_connStateData_private_data)
					{
						p_data = &conn->cc_connStateData_private_data[mod->slot];
						if (p_data && p_data->private_data)
							return p_data->private_data;
					}
				}
				else
					goto err;
			}
			break;
		case MEMOBJECT_PRIVATE_DATA:
			{
				MemObject *mem = (MemObject *)tag;
				if(mem->cc_MemObject_private_data)
				{
					p_data = &mem->cc_MemObject_private_data[mod->slot];
					if(p_data && p_data->private_data)
					{
						return p_data->private_data;
					}
				}
			}
		default:
			goto err;
	}
err:
	debug(90, 2) ("cc_get_mod_private_data: %s can't get private data\n", mod->name);
	return NULL;
}


void cc_register_hook_handler(HPIDX_t hook_index, int mod_slot, void ** mod_hook_ptr, void * handler)
{
    *mod_hook_ptr = handler;
    int * mod_slot_ptr = xcalloc(1, sizeof(int));
    *mod_slot_ptr = mod_slot;
    if (reconfigure_in_thread)
        arrayAppend(&cc_hook_to_module_r[hook_index], mod_slot_ptr);
    else
        arrayAppend(&cc_hook_to_module[hook_index], mod_slot_ptr);
}

/****************************FlexiCache Reload Check Configure Error Project******************************/


static FILE * gLogFile = NULL;
static int gErrorCount= 0;
static char *checkBuffer = NULL;
#define ERROR_LOG_DIR  "/data/proclog/log/squid/"
#define ERROE_LOG_FILE "/data/proclog/log/squid/squidConfError.log"
#define ERROE_FILE_INF "Process Name: [Line-No.  Error-Reason   Conf-Line]"
#define ERROR_LOG_SIZE 1024 

static const char * cc_framework_check_fetch_buf(void)
{
    if(NULL == checkBuffer)
        checkBuffer = xcalloc(ERROR_LOG_SIZE, sizeof(char));
    return checkBuffer;
}
static void cc_framework_check_open_done(const char *reason)
{
    cc_framework_check_fetch_buf();
    if(access(ERROR_LOG_DIR, F_OK))
    {   
        memset(checkBuffer, 0, ERROR_LOG_SIZE);
        snprintf(checkBuffer, ERROR_LOG_SIZE, "mkdir -p %s",ERROR_LOG_DIR);
        system(checkBuffer);
    }   
    memset(checkBuffer, 0, ERROR_LOG_SIZE);
    snprintf(checkBuffer, ERROR_LOG_SIZE,"touch %s", ERROE_LOG_FILE);
    system(checkBuffer);
    snprintf(checkBuffer, ERROR_LOG_SIZE, "chown squid:squid %s", ERROE_LOG_FILE);
    system(checkBuffer);
    memset(checkBuffer, 0, ERROR_LOG_SIZE);
    snprintf(checkBuffer, ERROR_LOG_SIZE, "chmod 644 %s", ERROE_LOG_FILE);
    system(checkBuffer);
    memset(checkBuffer, 0, ERROR_LOG_SIZE);

    gLogFile = fopen(ERROE_LOG_FILE, "w+");
    if(gLogFile)
    {
        memset(checkBuffer, 0, ERROR_LOG_SIZE);
        snprintf(checkBuffer, ERROR_LOG_SIZE, "Time: %s %s\n%s\n",accessLogTime(squid_curtime),reason, ERROE_FILE_INF);
        fprintf(gLogFile, "%s", checkBuffer);
        fflush(gLogFile);
    }
}

static FILE *cc_framework_check_open_start(const char *reason)
{
    if(NULL == gLogFile)
        cc_framework_check_open_done(reason);
    return gLogFile;
}   

void cc_framewrok_check_clean(void)
{
    cc_framework_check_fetch_buf();
    memset(checkBuffer, 0, ERROR_LOG_SIZE);
    snprintf(checkBuffer, ERROR_LOG_SIZE, "rm -f %s",ERROE_LOG_FILE);
    system(checkBuffer);
    safe_free(checkBuffer);
}

static void cc_framework_check_done(const char * buf)
{
    if(gLogFile)
    {
        fprintf(gLogFile, "%s", buf);
        fflush(gLogFile);
    }
}       

void cc_framework_write_start(const char *reason)
{
    gErrorCount++;
    cc_framework_check_fetch_buf();
    cc_framework_check_open_start("FC Check Configure");
    memset(checkBuffer, 0, ERROR_LOG_SIZE);
    snprintf(checkBuffer, ERROR_LOG_SIZE, "FC: [%-4d  \"%-20s\"\t\t\"%s\"]\n", config_lineno, reason, config_input_line);
    cc_framework_check_done(checkBuffer);
}

void cc_framework_check_start(const char *reason)
{
    if(0 == reconfigure_in_thread)
    {
        cc_framework_write_start(reason);
    }
}

void cc_framework_check_end(void)
{
    if(gLogFile)
    {   
        fprintf(gLogFile,"Total Count %d Error Lines\n\n", gErrorCount);
        fflush(gLogFile);
        fclose(gLogFile);
        gLogFile = NULL;
    }   
    if(cc_fc_reload_check)
        cc_fc_reload_check = 0;
    gErrorCount = 0;
    safe_free(checkBuffer);
}

void cc_framework_check_self_destruct(void)
{

    cc_framework_check_fetch_buf();
    cc_framework_check_open_start("FC Process DOWN");
    cc_framework_write_start("self Destruct Fatal");
    cc_framework_check_end();
}
#undef ERROR_LOG_DIR 
#undef ERROE_LOG_FILE 
#undef ERROR_LOG_SIZE 
#undef ERROE_FILE_INF 
#endif
