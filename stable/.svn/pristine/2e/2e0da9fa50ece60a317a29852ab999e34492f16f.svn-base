/*
 * =====================================================================================
 *
 *       Filename:  framework.c
 *
 *    Description:  for detect.
 *                  This framework will parse date file for modules.
 *
 *        Created:  03/15/2012 12:03:46 PM
 *       Compiler:  gcc
 *         Author:  Xu Jian (XJ), xujian@chinacache.com
 *        Company:  ChinaCache
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <dlfcn.h>
#include <assert.h>
#include "dbg.h"
#include "framework.h"
unsigned int release_count = 0;
static const char kMODULE_DIR[] = "/usr/local/detectorig/module/";
static const char kENTRANCE_FUNCTION[] = "RegisterModule";
static unsigned int link_data_read_count_max = 1;
enum{
    kMODULE_MAX_NUMBER = 64,
    kMAX_PATH_LENGTH = 256,
    kHOOK_MAX_TYPE = 32,
    kHOOK_MAX_NUMBER = 32
};
//char module_directory[256];
typedef struct _ModuleList
{
    int count;
    void *table[kMODULE_MAX_NUMBER];
} ModuleList; /* - - - - - - end of struct ModuleList - - - - - - */
ModuleList module_list = {0};
typedef struct _HookFunctionList
{
    int count[kHOOK_MAX_TYPE];
    HookFunction table[kHOOK_MAX_TYPE][kHOOK_MAX_NUMBER];
} HookFunctionList; /* - - - - - - end of struct HookFunctionList - - - - - - */
HookFunctionList hook_table = {{0},{{0}}};
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  GetLinkDataReadCountMax
 *  Description:
 * =====================================================================================
 */
    unsigned int
GetLinkDataReadCountMax( void )
{
    return link_data_read_count_max;
} /* - - - - - - - end of function GetLinkDataReadCountMax - - - - - - - - - - - */
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  SetLinkDataReadCountMax
 *  Description:
 * =====================================================================================
 */
    int
SetLinkDataReadCountMax( unsigned int count )
{
    if( link_data_read_count_max < count )
    {
        link_data_read_count_max = count;
        Debug( 40, "link.data read count: %d.", count );
    }
    return 0;
} /* - - - - - - - end of function SetLinkDataReadCountMax - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  LoadModule
 *  Description:  do some dlopen, need to do dlclose in so other function
 * =====================================================================================
 */
    int
LoadModule( void )
{
    int ReleaseModule( void );
    DIR *dir;
    struct dirent *dirent;
    static char module_file[1024];
    char *ptr_file;
    dir = opendir( kMODULE_DIR );
    if( dir == NULL )
    {
        Debug( 1, "Cannot open module directory." );
        return 1;
    }
    ptr_file = module_file;
    ptr_file += sprintf( ptr_file, "%s", kMODULE_DIR );
    while(( dirent = readdir( dir )) != NULL )
    {
        void *handle;
        int (*function)(void);
        if( module_list.count == kMODULE_MAX_NUMBER )
        {
            Debug( 20, "Cannot add module any more, we already have %d modules.", module_list.count );
            break;
        }
        if( strncmp( dirent->d_name, "mod_", 4 ) != 0 ||
            strncmp( rindex( dirent->d_name, '.' ), ".so", 3 ) != 0 )
        {
            Debug( 30, "Skip %s, in module directory.", dirent->d_name );
            continue;
        }
        sprintf( ptr_file, "%s", dirent->d_name );
        handle = dlopen( module_file, RTLD_LAZY );
        if( handle == NULL )
            continue;
        Debug( 30, "Find a module: %s", module_file );
        module_list.table[module_list.count++] = handle;
        function = dlsym( handle, kENTRANCE_FUNCTION );
        if( function != NULL )
        {
            Debug( 30, "Find a entrace function." );
            if( function() != 0 )
            {
                Debug( 30, "Failed to load module in %s.", module_file );
                ReleaseModule();
                return 1;
            }
        }
    }
    closedir( dir );
    return 0;
} /* - - - - - - - end of function LoadModule - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ReleaseModule
 *  Description:  
 * =====================================================================================
 */
    int
ReleaseModule( void )
{
    int i;
    for( i = module_list.count - 1; i >= 0; --i )
        dlclose( module_list.table[i] );
    return 0;
} /* - - - - - - - end of function ReleaseModule - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  AddHookFunction
 *  Description:  
 * =====================================================================================
 */
    int
AddHookFunction( HookType type, HookFunction Function )
{
    assert( Function );
    if( hook_table.count[type] == kHOOK_MAX_NUMBER )
    {
        Debug( 1, "Cannot add more function to hook point." );
        return 1;
    }
    else
    {
        hook_table.table[type][hook_table.count[type]++] = Function;
        return 0;
    }
} /* - - - - - - - end of function AddHookFunction - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  CallHookFunction
 *  Description:  
 * =====================================================================================
 */
    int
CallHookFunction( HookType type, const void *data )
{
    int i;
    int ret = 0;
    for( i = hook_table.count[type] - 1; i >= 0; --i )
    {
        HookFunction Function;
        Function = hook_table.table[type][i];
        if( Function( data ) != 0 )
        {
            Debug( 20, "Error in hook function (%d).", (int)type );
            ret = 1;
        }
    }
    return ret;
} /* - - - - - - - end of function CallHookFunction - - - - - - - - - - - */
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  ReleaseCpu
 * =====================================================================================
 */
    int
ReleaseCpu(void)
{
    extern unsigned int release_count;
    if( ++release_count == 1<<7 )
    {
        usleep(1);
        Debug( 60, "Release CPU." );
        release_count = 0;
    }
    return 0;
} /* - - - - - - - end of function ReleaseCpu - - - - - - - - - - - */
