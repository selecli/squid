/*
 * =====================================================================================
 *
 *       Filename:  hash.h
 *
 *    Description:  
 *
 *        Created:  12/07/2011 01:51:41 PM
 *       Compiler:  gcc
 *         Author:  XuJian
 *        Company:  China Cache
 *
 * =====================================================================================
 */
#ifndef _HASH_H_4EDEFEF2_
#define _HASH_H_4EDEFEF2_ 
typedef unsigned int (*HashGetKey)( const void* );
typedef int (*HashIsEqual)( const void*, const void* );
typedef int (*HashIsNull)( const void* );
typedef int (*HashDoFree)( void* );
typedef int (*HashElementPrint)( void* );
typedef struct _HashConfigure {
    unsigned int total;
    size_t element_size;
    HashGetKey GetKey;
    HashIsEqual IsEqual;
    HashIsNull IsNull;
    HashDoFree  DoFree;
    HashElementPrint ElementPrint;
} HashConfigure; /*  - - - - - - end of struct HashConfigure - - -     - - - */
typedef struct _Hash {
    HashConfigure conf;
    unsigned int used_count;
    unsigned int same_count;
    unsigned int conflict_count;
    unsigned int nospace_count;
    void *data;
} Hash; /*  - - - - - - end of struct Hash - - - - - - */
int HashDestroy( Hash *hash );
Hash* HashCreate( const HashConfigure *conf );
char* HashReport( const Hash *hash );
void* HashSearch( Hash *hash, void *element );
#endif /* _HASH_H_4EDEFEF2_ */
