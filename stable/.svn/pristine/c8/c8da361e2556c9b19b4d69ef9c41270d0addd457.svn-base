/**************************************************************************
 *   Copyright (C) 2013
 *
 *   File ID:
 *   File name: mod_select_disk.c
 *   Summary:
 *   Description: 
 *      5.Use to Select Disk Dir; 
 *   Configure: Match  ACL
 *   Version: V1.0
 *   Author: Yandong.Nian
 *   Date: 2014-08-28
 *   LastModified: 2014-08-28/Yandong.Nian;
 **************************************************************************/

#include "cc_framework_api.h"
#ifdef CC_FRAMEWORK
#define MAX_CACHE_DIE 256
#define SELECT_LEAST_LOAD  0 
#define SELECT_ROUND_ROBIN 1

#define PMOD_NAME "mod_select_disk" 

/***********************************Moudle Interface Start****************************************/
static cc_module* mod = NULL;
static long long g_SelectDiskConfig_Count = 0;
static long long g_SelectDiskMemPd_Count = 0;
static uint8_t   g_CheckReply_Flags = 0;
char g_CacheDirForbidDisk[MAX_CACHE_DIE];
int g_SelectDiskMethod = SELECT_LEAST_LOAD;


typedef struct __cache_select_t
{
    uint8_t  ch_p;          //whether check reply
    uint16_t dirn;          //current position of selecting dirn
    Array dir;              //match dirn Array
}cache_select_t;


typedef struct _SD_MemPd_t
{
    int fd;
}SD_MemPd_t;

static void *
swdaeSwapDirParamAlloc(void)
{
    cache_select_t *swap = xcalloc(1, sizeof(cache_select_t));
    if(swap) g_SelectDiskConfig_Count++;
    return swap;
}   

static int 
swdaeSwapDirParamFree(void *data)
{
    cache_select_t *swap = data;
    if (swap)
    {
        g_SelectDiskConfig_Count--;
        safe_free(swap);
    }
    swap = data = NULL;
    debug(172, 2)("%s %s %lld\n", PMOD_NAME, __func__, g_SelectDiskConfig_Count);
    return 0;
}

static void *
p_Mod_AllocMemPd(void)
{
    SD_MemPd_t *ssPd = xmalloc(sizeof(SD_MemPd_t));
    if (ssPd)
    {
        ssPd->fd = -1;
        g_SelectDiskMemPd_Count++;
    }
    return ssPd;
}
static int 
p_Mod_FreeMemPd(void *data)
{
    SD_MemPd_t *ssPd = data;
    if (ssPd)
    {
        safe_free(data);
        g_SelectDiskMemPd_Count--;
    }
    ssPd = data = NULL;
    debug(172, 2)("%s %s %lld\n", PMOD_NAME, __func__, g_SelectDiskMemPd_Count);
    return 0;
}

static acl *
g_Mod_FindByName(const char *name)
{
    acl *a = Config.aclList;
    if(reconfigure_in_thread) a = Config_r.aclList;
        
    for ( ; a; a = a->next)
    {        
        if (!strcasecmp(a->name, name)) return a;
    }
    return NULL;
}

static inline int 
g_Mod_MatchAclTypeByName(const char *t, squid_acl tp1, squid_acl tp2)
{
    if ('!' == *t) t++;
    acl *a = (t ? g_Mod_FindByName(t) : NULL);
    return (a && (tp1 == a->type || tp2 == a->type)) ? 1: 0;
}

static int 
g_Mod_SetCheckFlagsStart(char *p_line)
{
    int check_reply = 0;
    char *t = strtok(p_line, w_space);
    while(0 == check_reply && (t = strtok(NULL, w_space)))
    {   
        check_reply = g_Mod_MatchAclTypeByName(t, ACL_REP_HEADER, ACL_REP_MIME_TYPE);
    } 
    return check_reply;
}

static int 
swdeaSwapDirMatchCacheDir(char *t, cache_select_t *p, char *cache_dir)
{
    int i = 0;
    int i_t_count = 0;
    int i_s_count = Config.cacheSwap.n_configured; 
    SwapDir *gswap = Config.cacheSwap.swapDirs; 
    if(reconfigure_in_thread)
    {
        i_s_count = Config_r.cacheSwap.n_configured;
        gswap = Config_r.cacheSwap.swapDirs; 
    }
    for(i = 0; i < i_s_count; i++)
    {
        if(0 == strncmp(gswap[i].path, t, strlen(t)))
        {
            if (gswap[i].flags.read_only)
            {
                debug(172, 1)("%s %s match cache_dir Read_only %s %s\n",
                        PMOD_NAME, __func__, gswap[i].path, t);
            }
            else if (0 == cache_dir[i])
            {
                debug(172, 2)("%s %s match swap[%d] %s %s\n", 
                        PMOD_NAME, __func__, i, gswap[i].path, t);

                arrayAppend(&p->dir, (void *)i);
                p->dir.items[p->dir.count-1] = (void *)i;
                i_t_count++;
                cache_dir[i] = 1;
            }
            else
            {
                debug(172, 1)("%s %s already matchd %s %s\n", PMOD_NAME, __func__, gswap[i].path, t);
            }
        }
        else
        {
            debug(172, 1)("%s %s not match cache_dir\n", PMOD_NAME, __func__);
        }
    }
    return i_t_count;
}

/***********************************Moudle Interface End ****************************************/
/***********************************Moudle Core Start****************************************/
static int
swdaeSwapDirSwapDirSize(int swapdir, squid_off_t objsize)
{
    squid_off_t s_max_objsize = Config.cacheSwap.swapDirs[swapdir].max_objsize;
    squid_off_t s_min_objsize = Config.cacheSwap.swapDirs[swapdir].min_objsize;

    if (objsize == -1)
        return s_max_objsize == -1;
    if (s_min_objsize <= objsize && (s_max_objsize > objsize || -1 == s_max_objsize))
        return 1;
    return 0;
}


static int 
swdaeSwapDirLeastLoad(cache_select_t *cache, const StoreEntry *e) 
{
    squid_off_t objsize;
    int most_free = 0, cur_free;
    squid_off_t least_objsize = -1; 
    int least_load = INT_MAX;
    int load;
    int dirn = 0;
    int dirn_no = -1; 
    int i;
    SwapDir *sd;

    /* Calculate the object size */
    objsize = objectLen(e);
    if (objsize != -1) 
        objsize += e->mem_obj->swap_hdr_sz;

    
    for (i = 0; i < cache->dir.count; i++)
    {   
        cache->dirn = cache->dirn % cache->dir.count;
        dirn = (int)cache->dir.items[cache->dirn];
        cache->dirn++;
        if (0 > dirn || dirn >= Config.cacheSwap.n_configured)
            continue;
        sd = &Config.cacheSwap.swapDirs[dirn];
        sd->flags.selected = 0;
        if (sd->flags.read_only)
        {
            debug(172, 5)("%s %s %s read_only\n", PMOD_NAME, __func__,sd->path);
            continue;
        }
        if (sd->cur_size > sd->max_size)
        {
            debug(172, 5)("%s %s %s cur_size > sd->max_size\n", PMOD_NAME, __func__, sd->path);
            continue;
        }
        if (!swdaeSwapDirSwapDirSize(dirn, objsize))
        {
            debug(172,5)("%s %s objsize isn't Suitable %s %s\n", PMOD_NAME, __func__, sd->path, storeUrl(e));
            continue;
        }
        if (sd->checkobj(sd, e) == 0)
        {
            debug(172,5)("%s %s %s checkobj(sd,e) == 0\n",PMOD_NAME, __func__, sd->path);
            continue;
        }
        load = sd->checkload(sd, ST_OP_CREATE);
        if (load < 0 || load > 2000)
        {
            debug(172, 5)("%s %s %s Load too high %s\n", PMOD_NAME, __func__, sd->path, storeUrl(e));
            continue;
        }
        if (load > least_load)
            continue;
        cur_free = sd->max_size - sd->cur_size;
        /* If the load is equal, then look in more details */
        if (load == least_load)
        {
            /* closest max_objsize fit */
            if (least_objsize != -1)
                if (sd->max_objsize > least_objsize || sd->max_objsize == -1)
                    continue;
            /* most free */
            if (cur_free < most_free)
                continue;
        }
        least_load = load;
        least_objsize = sd->max_objsize;
        most_free = cur_free;
        dirn_no = dirn;
    }
    if (dirn_no >= 0)
    {
        sd = &Config.cacheSwap.swapDirs[dirn_no];
        debug(172,5)("%s %s Select [%s] store %s\n", PMOD_NAME, __func__, sd->path, storeUrl(e));
        Config.cacheSwap.swapDirs[dirn_no].flags.selected = 1;
    }else
    {
        debug(172,5)("%s %s Select Null store %s\n",PMOD_NAME, __func__, storeUrl(e));
    }
    return dirn_no;
}


static int 
swdaeSwapDirRoundRobin(cache_select_t *cache, const StoreEntry * e)
{
    static int dirn = 0;
    int i;  
    int load;
    SwapDir *sd;
    squid_off_t objsize = objectLen(e);
    for (i = 0; i < cache->dir.count; i++)
    {
        cache->dirn = cache->dirn % cache->dir.count;
        dirn = (int)cache->dir.items[cache->dirn];
        cache->dirn++;
        if (0 > dirn || dirn >= Config.cacheSwap.n_configured)
            continue;
        sd = &Config.cacheSwap.swapDirs[dirn];
        if (sd->flags.read_only)
        {
            debug(172, 5)("%s %s: %s read_only\n", PMOD_NAME, __func__, sd->path);
            continue;
        }
        if (sd->cur_size > sd->max_size)
        {
            debug(172, 5)("%s %s: %scur_size > sd->max_size\n", PMOD_NAME, __func__, sd->path);
            continue;
        }

        if (!swdaeSwapDirSwapDirSize(dirn, objsize))
        {
            debug(172,5)("%s %s objsize isn't Suitable %s %s\n",PMOD_NAME, __func__, sd->path, storeUrl(e));
            continue;
        }

        if (sd->checkobj(sd, e) == 0)
        {
            debug(172,5)("%s %s checkobj(sd,e) == 0\n", PMOD_NAME, sd->path);
            continue;
        }
        load = sd->checkload(sd, ST_OP_CREATE);
        if (load < 0 || load > 2000)
        {
            debug(172, 5)("%s %s Load too high %s\n", PMOD_NAME, sd->path, storeUrl(e));
            continue;
        }
        debug(172,3)("%s %s select [%s] store %s\n", PMOD_NAME, __func__, sd->path, storeUrl(e));
        return dirn;
    }
    debug(172, 7)("%s %s nomatch %s\n", PMOD_NAME, __func__, storeUrl(e));
    return -1;
}

static inline int 
swdaeSwapDirSelectDiskStart(int m_Flag, cache_select_t *cache, const StoreEntry *e)
{
    int dirn = -1;
    if (-1 == dirn && SELECT_LEAST_LOAD ==  m_Flag && cache && e)
        dirn = swdaeSwapDirLeastLoad(cache, e); 
    if (-1 == dirn && SELECT_ROUND_ROBIN == m_Flag && cache && e)
        dirn = swdaeSwapDirRoundRobin(cache, e); 
    return dirn;
}

static int 
swdaeSwapDirAclNBCheck(aclCheck_t *ch, const StoreEntry *e)
{
    int pos = 0;
    int dirn = -1;
    int answer = 0;
    acl_access *A = mod->acl_rules;
    cc_mod_param *mod_param = NULL;
    cache_select_t *cache = NULL;
    while (0 > dirn && A && pos < mod->mod_params.count)
    {
        if ((mod_param = mod->mod_params.items[pos]) && (cache = mod_param->param))
        {
            answer = aclMatchAclList(A->acl_list, ch);
            if (answer > 0 && A->allow == ACCESS_ALLOWED)
            {
                dirn = swdaeSwapDirSelectDiskStart(g_SelectDiskMethod, cache, e);
            }
        }
        pos++;
        A = A->next;
    }
    debug(172, 8)("%s %s reply_check %d %s\n", PMOD_NAME,__func__, dirn, storeUrl(e));
    return dirn < 0 ? -1: dirn;
}

/***********************************Moudle Core End****************************************/


/***********************************Moudle Hook Start****************************************/
static int hookSwdaeSwapDirAclCheck(const StoreEntry *e, char **forbid, int *ret) 
{
    int dirn = *ret;
    if (forbid)
        *forbid = g_CacheDirForbidDisk;
    if(NULL ==e || NULL == e->mem_obj)
    {   
        return dirn;
    }       
    if(0 == mod->flags.param || 0 == mod->mod_params.count)
    {   
        return dirn;
    }  

    if (dirn >= 0 && dirn < Config.cacheSwap.n_configured)
    {
        debug(172, 5)("%s %s areadly selected %d %s\n", PMOD_NAME,__func__, dirn, storeUrl(e));
        return dirn;
    }

    SD_MemPd_t *ssPd = cc_get_mod_private_data(MEMOBJECT_PRIVATE_DATA, e->mem_obj , mod);
    if (ssPd && ssPd->fd >= 0 && fd_table[ssPd->fd].cc_run_state)
    {
        cache_select_t *cache = cc_get_mod_param(ssPd->fd, mod);
        dirn = swdaeSwapDirSelectDiskStart(g_SelectDiskMethod, cache, e);
        ssPd = dirn >= 0 ? ssPd : NULL;
    }

    if (NULL == ssPd && g_CheckReply_Flags)
    {
        aclCheck_t ch; 
        memset(&ch, 0, sizeof(ch));
        ch.request = e->mem_obj->request;
        ch.reply = storeEntryReply((StoreEntry *)e);
        aclChecklistCacheInit(&ch);
        dirn = swdaeSwapDirAclNBCheck(&ch, e);
        aclCheckCleanup(&ch);
    }
    *ret = dirn;
    return 0;
}

static int 
hookSwdaeSwapRegiestMemPd(HttpStateData *hs, char *buf, ssize_t len)
{
    StoreEntry *e = hs ? hs->entry : NULL;
    if (0 == g_CheckReply_Flags && e->mem_obj && e && EBIT_TEST(e->flags, ENTRY_CACHABLE))
    {
        SD_MemPd_t *ssPd = cc_get_mod_private_data(MEMOBJECT_PRIVATE_DATA, e->mem_obj , mod);
        if (NULL == ssPd) ssPd = p_Mod_AllocMemPd();
        ssPd->fd = hs->fd;
        cc_register_mod_private_data(MEMOBJECT_PRIVATE_DATA, e->mem_obj, ssPd, p_Mod_FreeMemPd, mod);
        debug(172,5)("%s %s %s %s\n", PMOD_NAME,__func__, ssPd ? "cache":"no-cache", storeUrl(e));
    }
    return 0;
}

static int hookSwdaeTryParseParam(char *cfg_line)
{
    int i_t_count = 0;
    char *t = NULL;
    char *allow = strstr(cfg_line, " allow ");
    char *deny = strstr(cfg_line, " deny ");
    char cache_dir[MAX_CACHE_DIE];
    
    if (NULL == allow || (deny && allow > deny))
    {
        debug(172,1)("%s %s ParseFail allow=%p deny=%p\n", PMOD_NAME,__func__,allow, deny);
        return -1;
    }
    *allow = '\0';
    allow++;

    t = strtok(cfg_line, w_space);
    if(NULL == t || strcmp(t, PMOD_NAME))
    {
        debug(172,1)("%s %s ParseFail name=%s\n", PMOD_NAME,__func__, t);
        return -1;
    }
    memset(cache_dir, 0, MAX_CACHE_DIE);
    cache_select_t *pConfig = swdaeSwapDirParamAlloc();
    while(pConfig && (t = strtok(NULL, w_space)))
    {
        i_t_count += swdeaSwapDirMatchCacheDir(t, pConfig, cache_dir);
    }

    if(i_t_count <= 0)
    {
        debug(172,1)("%s %s ParseFail i_t_count=%d\n", PMOD_NAME,__func__, i_t_count);
        swdaeSwapDirParamFree(pConfig);
        pConfig = NULL;
        return -1;
    }
    pConfig->ch_p = g_Mod_SetCheckFlagsStart(allow);
    debug(172,2)("%s %s %d check_reply=%d\n", PMOD_NAME,__func__, i_t_count, pConfig->ch_p);
    cc_register_mod_param(mod, pConfig, swdaeSwapDirParamFree);
    return 0;
}


static int hookSwdaeBeforeSysInit(void)
{
    cache_select_t *cache = NULL;
    g_CheckReply_Flags = 0;
    cc_mod_param *mod_param = NULL;

    int pcount = 0;
    int all_count = 0;

    int i = 0;
    int k = 0;
    int dirn = 0;
    memset(g_CacheDirForbidDisk, 0, MAX_CACHE_DIE); 

    for (i = 0; i < mod->mod_params.count; i++)
    {
        if ((mod_param = mod->mod_params.items[i]) && (cache = mod_param->param))
        {
            pcount = 0;
            for (k = 0; k < cache->dir.count; k++)
            {
                dirn = (int)cache->dir.items[k];
                if (dirn >= 0 && dirn < Config.cacheSwap.n_configured)
                {
                    pcount++;
                    all_count++;
                    g_CacheDirForbidDisk[dirn] = 1;
                    debug(172,1)("%s %s private dir %s\n",
                            PMOD_NAME, __func__, storeSwapDir(dirn));
                }
            }
            if (0 == pcount)
            {
                cache->dir.count = 0;
            }
            if (cache->dir.count > 0 && cache->ch_p)
            {
                g_CheckReply_Flags = 1;
            }
        }
    }
    debug(172,1)("%s %s g_CheckReply_Flags = %d\n", PMOD_NAME, __func__, g_CheckReply_Flags);
    g_SelectDiskMethod = SELECT_LEAST_LOAD;
    char *s_method = Config.store_dir_select_algorithm;
    if (s_method &&  0 == strcasecmp(s_method, "round-robin"))
    {
        g_SelectDiskMethod = SELECT_ROUND_ROBIN;
    }
    debug(172,1)("%s %s %s %d\n", PMOD_NAME, __func__, s_method, g_SelectDiskMethod);
    return 0;
}
/***********************************Moudle Hook End ****************************************/


/***********************************Moudle regest Start****************************************/
int mod_register(cc_module *module)
{
    debug(172, 1)("%s->  init: init module\n", PMOD_NAME);
    strcpy(module->version, "7.0.R.16488.i686");

    cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
            module->slot,
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
            hookSwdaeTryParseParam);

    cc_register_hook_handler(HPIDX_hook_func_before_sys_init,
            module->slot,
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_before_sys_init),
            hookSwdaeBeforeSysInit);

    cc_register_hook_handler(HPIDX_hook_func_buf_recv_from_s_header,
            module->slot,
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_buf_recv_from_s_header),
            hookSwdaeSwapRegiestMemPd);

    cc_register_hook_handler(HPIDX_hook_func_private_assign_dir,
            module->slot,
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_assign_dir),
            hookSwdaeSwapDirAclCheck);

    if(reconfigure_in_thread)
        mod = (cc_module*)cc_modules.items[module->slot];
    else
        mod = module;
    return 0;
}
/***********************************Moudle regest End****************************************/
#undef MAX_CACHE_DIE 
#undef PMOD_NAME
#undef SELECT_LEAST_LOAD 
#undef SELECT_ROUND_ROBIN 
#endif
