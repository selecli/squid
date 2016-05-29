/**************************************************************************
 *   Copyright (C) 2013
 *
 *   File ID:
 *   File name: mod_range.c
 *   Summary:
 *   Description: 
 *      1.Use to limit AUFS object count;
 *      2.Use to limit COSS object count; 
 *   Configure: Match  ACL  or on
 *   Version: V1.0
 *   Author:Wen.Si 
 *   Date: 2009-12-03 Create
 -------------------------------------------------------------------------
 *   Version: V2.0
 *   Author: Yandong.Nian
 *   Date: 2013-10-31
 *   LastModified: 2013-10-18/Yandong.Nian;
 **************************************************************************/
#include "cc_framework_api.h"
#include "../../squid/src/fs/coss/store_coss.h"
#ifdef CC_FRAMEWORK

static cc_module* mod = NULL;

struct LimitInfo {
	long coss;
	long aufs;
	long all;
};

static int free_func(void * data)
{
	if(data)
		safe_free(data);
	return 0;
}

static int hook_func_sys_parse_param(char *cfg_line)
{

    char *t = NULL, *token = strtok(cfg_line, w_space);
	if(!token || strcmp(token, "mod_limit_obj_count"))
	{
		debug(111, 0)("mod_limit_obj_count error module name!\n");
		return -1;
	}

    long all_l = 0, aufs_l = 0, coss_l = 0;
	while((token = strtok(NULL, w_space)))
	{
		if(!strcmp(token, "aufs") && (t = strtok(NULL, w_space)))
		{
            aufs_l = atol(t);
            continue;
		}
		if(!strcmp(token, "coss") && (t = strtok(NULL, w_space)))
		{
            coss_l = atol(t);
            continue;
		}
		if(!strcmp(token, "all") && (t = strtok(NULL, w_space)))
        {
            all_l = atol(t);
            continue;
        }
        break;
	}
	
    if(all_l <= 0 && aufs_l <= 0 && coss_l <= 0)
    {
        debug(111, 0)("mod_limit_obj_count error limit number! %s %s\n", token, t);    
        return -1;
    }
    struct LimitInfo* limit = xmalloc(sizeof(struct LimitInfo));
    limit->all  = all_l  > 0 ? all_l : 0;
    limit->aufs = aufs_l > 0 ? aufs_l: 0;
    limit->coss = coss_l > 0 ? coss_l: 0;
    debug(111,2)("mod_limit_obj_count aufs:%ld  coss:%ld all:%ld\n", limit->aufs, limit->coss, limit->all);
	cc_register_mod_param(mod, limit, free_func);
	return 0;
}


static void storeCossRemoveObjFromCacheDir(void *data)
{
    int i = 0;
    SwapDir* sd = NULL; 
    for(i = 0; i < Config.cacheSwap.n_configured; i++)
    {
        sd = &Config.cacheSwap.swapDirs[i];
        if(sd->snapshot >= sd->cur_obj_cnt)
          continue;
        if(strcmp(sd->type, "coss"))
          continue;
        CossInfo *cs = (CossInfo *) sd->fsdata;
        if(cs->rmark < 0)
        {
            cs->rmark = cs->curstripe;
        }
        StoreEntry* e = NULL;
        int count = (sd->cur_obj_cnt - sd->snapshot) > 512? 512: sd->cur_obj_cnt - sd->snapshot;
        count = count <  70 ? 70 : count;
        int s_count = count, loop = 0;
        CossStripe * cstripe = NULL;
        debug(111, 6)("mod_limit_obj_count Coss cur_obj_cnt %d cs->count %d d_count %d\n", sd->cur_obj_cnt, cs->count, count);
        //sd->cur_obj_cnt = cs->count;

        while(count > 0 && loop <= 2000 && sd->cur_obj_cnt > sd->snapshot)
        {
            loop++;
            cs->rmark = (cs->rmark +1)% cs->numstripes;
            cstripe = &cs->stripes[cs->rmark];
            dlink_node *m = cstripe->objlist.head;
            while (m != NULL) 
            {    
                e = m->data;
                m = m->next;
                if (e && !storeEntryLocked(e))
                {
                    if (e->swap_filen > -1 && e->swap_status == SWAPOUT_DONE && EBIT_TEST(e->flags, ENTRY_VALIDATED))
                    {
                        EBIT_CLR(e->flags, ENTRY_VALIDATED);
                        storeDirUpdateSwapSize(&Config.cacheSwap.swapDirs[e->swap_dirn], e->swap_file_sz, -1);
                    }
                    count--;
                    if(e->swap_filen > -1)
                        storeCossUnlink(sd, e);
                    storeRelease(e);
                }
            } 
        }
        debug(111, 5)("mod_limit_obj_count Coss %d delete %d\n", cs->rmark, s_count -count);
    }
}



static void divideAndShareObjectCount(int share_count, int i_tp)
{
    int i = 0;
    SwapDir* sd = NULL;
    share_count = share_count > 0 ? share_count : 1;
    for (i = 0; i < Config.cacheSwap.n_configured; i++)
    {
        sd = &Config.cacheSwap.swapDirs[i];
        if(i_tp == 2 && strcmp(sd->type, "aufs"))
          continue;
        if(i_tp == 3 && strcmp(sd->type, "coss"))
          continue;
        if(share_count < sd->cur_obj_cnt)
          sd->snapshot = sd->cur_obj_cnt * 0.9;   //remove 10 precent
        if(share_count >= sd->cur_obj_cnt)
          sd->snapshot =  sd->cur_obj_cnt * 0.95; //remove 5 precent
        debug(111 ,5)("mod_limit_obj_count path %s cur_obj_count %d snap %d\n", sd->path, sd->cur_obj_cnt,sd->snapshot);
    }
}


static int hook_limit_object_judgement(void)
{
    if(store_dirs_rebuilding)
    {
        debug(111 , 8)("mod_limit_obj_count store_dirs_rebuilding\n");
        return 0;
    }
    struct LimitInfo* limit = (struct LimitInfo*)cc_get_global_mod_param(mod);
    if(!limit || (limit->all <= 0 && limit->coss <=  0 && limit->aufs <= 0))
      return 0;
    SwapDir* sd = NULL; 
    int i = 0, dirCount = 0, ret = 0;
    int all_t = Config.cacheSwap.n_configured, aufs_t = 0, coss_t = 0;
    long allTotal = 0, aufsTotal = 0, cossTatal = 0;

    for (i = 0; i < Config.cacheSwap.n_configured; i++)
    {
        sd = &Config.cacheSwap.swapDirs[i];
        debug(111 , 7)("mod_limit_obj_count[%s], cur [%d]\n", sd->path, sd->cur_obj_cnt);
        allTotal += (sd->cur_obj_cnt);
        if((limit->aufs > 0 || limit->all > 0) && !strcmp(sd->type, "aufs"))
        {
            aufsTotal += sd->cur_obj_cnt;
            aufs_t += 1;
        }
        else if((limit->coss > 0 || limit->all > 0) && !strcmp(sd->type, "coss"))
        {
            cossTatal += sd->cur_obj_cnt;
            coss_t += 1;
            dirCount = 1;
        }
    }
    debug(111 , 5)("mod_limit_obj_count All Cache_dir Object Count %ld n_disk_objects %d\n", allTotal, n_disk_objects);
    debug(111 , 8)("mod_limit_obj_count Aufs %ld Limit %ld| Coss %ld limit %ld|All %ld Limit %ld\n", aufsTotal, limit->aufs, cossTatal, limit->coss, allTotal, limit->all);

    if(limit->all > 0)
    {
        if(allTotal <= limit->all || all_t <= 0)
        {
            debug(111 , 7)("mod_limit_obj_count %d All Count %ld  <= Config %ld\n",all_t, allTotal, limit->all);
            return 0;
        }
        divideAndShareObjectCount(allTotal/all_t, 0);
        if(dirCount == 1)
            eventAdd("storeCossRemoveObjFromCacheDir", storeCossRemoveObjFromCacheDir, NULL, 1.0, 0);
        return 1;
    }

    if(limit->aufs > 0)
    {
        if(aufsTotal > limit->aufs && 0 < aufs_t)
            divideAndShareObjectCount(aufsTotal/aufs_t, 2);
        ret += 2;
        if(aufsTotal <= limit->aufs || aufs_t <= 0)
        {
            ret -= 2;
            debug(111 , 7)("mod_limit_obj_count %d AUFS %ld <= Config %ld\n", aufs_t, aufsTotal, limit->aufs);
        }
    }
    if(limit->coss > 0)
    {
        if(cossTatal > limit->coss && 0 < coss_t )
        {
            divideAndShareObjectCount(cossTatal/coss_t, 3);
            eventAdd("storeCossRemoveObjFromCacheDir", storeCossRemoveObjFromCacheDir, NULL, 1.0, 0);
        }
        ret += 3;
        if(cossTatal <= limit->coss || coss_t <= 0)
        {
            ret -= 3;
            debug(111 , 7)("mod_limit_obj_count %d Coss %ld <= Config %ld\n", coss_t, cossTatal, limit->coss);
        }
    }
    return ret;
}	

int mod_register(cc_module *module)
{
    debug(111, 1)("(mod_limit_obj_count) ->  init: init module\n");

    strcpy(module->version, "7.0.R.16488.i686");

    cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
                module->slot, 
                (void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
                hook_func_sys_parse_param);

    cc_register_hook_handler(HPIDX_hook_func_limit_obj,
                module->slot, 
                (void **)ADDR_OF_HOOK_FUNC(module, hook_func_limit_obj),
                hook_limit_object_judgement);

    if(reconfigure_in_thread)
      mod = (cc_module*)cc_modules.items[module->slot];
    else
      mod = module;
    return 0;
}
#endif
