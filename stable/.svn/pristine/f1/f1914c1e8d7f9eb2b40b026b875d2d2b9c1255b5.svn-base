/*
 * =====================================================================================
 *
 *       Filename:  hashtable.h
 *
 *    Description:  
 *
 *        Created:  03/29/2012 11:36:51 AM
 *       Compiler:  gcc
 *         Author:  Xu Jian (XJ), xujian@chinacache.com
 *        COMPANY:  ChinaCache
 *
 * =====================================================================================
 */
#ifndef _HASH_TABLE_H_082E5810_D4EB_4A51_99A6_7F5367DA8A5A_
#define _HASH_TABLE_H_082E5810_D4EB_4A51_99A6_7F5367DA8A5A_ 
typedef unsigned int HashKey; /* must be basic data type */
typedef HashKey (*HashGetKey)( const void* );
typedef int (*HashIsEqual)( const void*, const void* );
typedef int (*HashIsNull)( const void* );
typedef int (*HashDoFree)( void* );
typedef int (*HashTraversalFunction)( void *local, void *param ); /* must be same as LinkedListTraversalFunction */
typedef struct _HashConfigure
{
    unsigned int total;
    char *name;
    HashGetKey GetKey;
    HashIsEqual IsEqual;
    HashIsNull IsNull;                          /* considering to delete this */
    HashDoFree  DoFree;
} HashConfigure; /*  - - - - - - end of struct HashConfigure - - -     - - - */
typedef struct _HashTable
{
    HashConfigure conf;
    unsigned int conflict_counter;
    unsigned int smooth_counter;
    void *data;
    char *name;
} HashTable; /* - - - - - - end of struct HashTable - - - - - - */
int HashDestroy( HashTable *hash );
HashTable* HashCreate( const HashConfigure *conf );
void* HashTraversal( HashTable *hash, HashTraversalFunction Traversal, void *param );
void* HashSearch( HashTable *hash, const void *element );
void* HashInsert( HashTable *hash, void *element );
char* HashReport( HashTable *hash );
#endif /* _HASH_TABLE_H_082E5810_D4EB_4A51_99A6_7F5367DA8A5A_ */
