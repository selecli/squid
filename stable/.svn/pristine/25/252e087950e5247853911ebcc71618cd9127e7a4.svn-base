/*
 * =====================================================================================
 *
 *       Filename:  hashtable.c
 *
 *    Description:  
 *
 *        Created:  03/29/2012 11:33:54 AM
 *       Compiler:  gcc
 *         Author:  Xu Jian (XJ), xujian@chinacache.com
 *        COMPANY:  ChinaCache
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "linkedlist.h"
#include "hashtable.h"
#include "dbg.h"
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  HashReport
 *  Description:
 * =====================================================================================
 */
    char*
HashReport( HashTable *hash )
{
    static char report[1024];
    char *p = report;
    p += sprintf( p, "hash %s report length: %d, smooth: %d, conflict: %d",
                  hash->name,
                  hash->conf.total,
                  hash->smooth_counter,
                  hash->conflict_counter );
    Debug( 30, "%s", report );
    return report;
} /* - - - - - - - end of function HashReport - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  HashDestroy
 *  Description:  Destroy data in hash.
 *                Need to call free on hash.
 * =====================================================================================
 */
    int
HashDestroy( HashTable *hash )
{
    int i;
    LinkedList **linked;
    assert( hash );
    HashReport( hash );
    linked = (LinkedList**)hash->data;
    for( i = hash->conf.total - 1; i >= 0; --i )
    {
        if( linked[i] != 0 )
        {
            LinkedListDestroy( linked[i] );
            free( linked[i] );
            linked[i] = NULL;
        }
    }
    free( hash->name );
    free( hash->data );
    memset( hash, 0, sizeof(HashTable) );
    return 0;
} /* - - - - - - - end of function HashDestroy - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  HashCreate
 *  Description:  Do malloc.
 *                Need to call HashDestory() and free() on returned value
 *                Copy but no directly using conf
 * =====================================================================================
 */
    HashTable*
HashCreate( const HashConfigure *conf )
{
    HashTable *hash;
    assert( conf );
    hash = (HashTable *)malloc( sizeof(HashTable) );
    if( hash == NULL )
        return NULL;
    memset( hash, 0, sizeof(HashTable) );
    hash->data = (void *)malloc( conf->total * sizeof(void*) );
    if( hash->data == NULL )
    {
        free( hash );
        return NULL;
    }
    memset( hash->data, 0, conf->total * sizeof(void*) );
    memcpy( &hash->conf, conf, sizeof(HashConfigure) );
    if( conf->name )
        hash->name = strdup( conf->name );
    else
        hash->name = strdup( "unknown" );
    return hash;
} /* - - - - - - - end of function HashCreate - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  HashTraversal
 *  Description:  Break while Traversal() returns non-zero
 * =====================================================================================
 */
    void*
HashTraversal( HashTable *hash, HashTraversalFunction Traversal, void *param )
{
    int i;
    void *ret = NULL;
    LinkedList **data;
    assert( hash );
    assert( Traversal );
    data = hash->data;
    for( i = hash->conf.total - 1; i >= 0; --i )
    {
        if( data[i] != NULL )
        {
            ret = LinkedListTraversal( (LinkedList*)data[i], Traversal, param );
            if( ret != 0 )
                break;
        }
    }
    return ret;
} /* - - - - - - - end of function HashTraversal - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  HashAt
 *  Description:  
 * =====================================================================================
 */
    static LinkedList**
HashAt( const HashTable *hash, HashKey key )
{
    LinkedList **linked;
    assert( hash );
    linked = (LinkedList**)hash->data;
    return linked + key % hash->conf.total;
} /* - - - - - - - end of function HashAt - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  HashSearch
 *  Description:  
 * =====================================================================================
 */
    void*
HashSearch( HashTable *hash, const void *element )
{
    HashKey key;
    LinkedList **linked;
    void *ret;
    assert( hash );
    assert( element );
    key = hash->conf.GetKey( element );
    linked = HashAt( hash, key );
    ret = LinkedListSearch( *linked, element );
    return ret;
} /* - - - - - - - end of function HashSearch - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  HashInsert
 *  Description:  May insert same element
 * =====================================================================================
 */
    void*
HashInsert( HashTable *hash, void *element )
{
    HashKey key;
    LinkedList **linked;
    void *ret;
    assert( hash );
    assert( element );
    key = hash->conf.GetKey( element );
    linked = HashAt( hash, key );
    if( *linked == NULL )
    {
        hash->smooth_counter++;
        LinkedListConfigure conf;
        conf.IsEqual = hash->conf.IsEqual;
        conf.IsNull = hash->conf.IsNull;
        conf.DoFree = hash->conf.DoFree;
        *linked = LinkedListCreate( &conf );
    }
    else
        hash->conflict_counter++;
    ret = LinkedListInsert( *linked, element );
    return ret;
} /* - - - - - - - end of function HashInsert - - - - - - - - - - - */
