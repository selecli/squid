/*
 * =====================================================================================
 *
 *       Filename:  hash.c
 *
 *    Description:  
 *
 *        Created:  12/07/2011 01:55:07 PM
 *       Compiler:  gcc
 *         Author:  XuJian
 *        Company:  China Cache
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash.h"
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  HashAt
 *  Description:  
 * =====================================================================================
 */
inline void*
HashAt( const Hash *hash, int index )
{
    return( hash->data + index * hash->conf.element_size );
} /* - - - - - - - end of function HashAt - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  HashDestroy
 *  Description:  
 * =====================================================================================
 */
int
HashDestroy( Hash *hash )
{
    int i;
    void *element;
    HashIsNull IsNull;
    HashDoFree DoFree;
    IsNull = hash->conf.IsNull;
    DoFree = hash->conf.DoFree;
    for( i = hash->conf.total - 1; i >= 0; --i )
    {
        element = HashAt( hash, i );
        if( IsNull( element ) )
            continue;
        DoFree( element );
        element = NULL;
    }
    if( hash->data != NULL )
        free( hash->data );
    memset( hash, 0, sizeof(Hash) );
    free( hash );
    return 0;
} /* - - - - - - - end of function HashDestroy - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  HashCreate
 *  Description:  
 * =====================================================================================
 */
Hash*
HashCreate( const HashConfigure *conf )
{
    size_t size;
    Hash *hash;
    hash = (Hash *)malloc( sizeof(Hash) );
    if( hash == NULL )
    {
        perror( "malloc failed" );
        return NULL;
    }
    memset( hash, 0, sizeof(Hash) );
    size = conf->total * conf->element_size;
    hash->data = (void *)malloc( size );
    if( hash->data == NULL )
    {
        perror( "malloc failed" );
        free( hash );
        hash = NULL;
        return NULL;
    }
    memset( hash->data, 0, size );
    memcpy( &hash->conf, conf, sizeof(HashConfigure) );
    return hash;
} /* - - - - - - - end of function HashCreate - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  HashReport
 *  Description:  
 * =====================================================================================
 */
char*
HashReport( const Hash *hash )
{
    static char buf[64];
    sprintf( buf, "%u_total %u_used %u_same %u_conflict %u_nospace",
            hash->conf.total,
            hash->used_count,
            hash->same_count,
            hash->conflict_count,
            hash->nospace_count );
    return buf;
} /* - - - - - - - end of function HashReport - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  HashSearch
 *  Description:  
 * =====================================================================================
 */
void*
HashSearch( Hash *hash, void *element )
{
    int key;
    void *position;
    unsigned int max = hash->conf.total;
    int start = -1;
    HashGetKey GetKey = hash->conf.GetKey;
    HashIsEqual IsEqual = hash->conf.IsEqual;
    HashIsNull IsNull = hash->conf.IsNull;
    HashDoFree DoFree = hash->conf.DoFree;
    key = GetKey( element ) % max;
    while(1)
    {
        position = HashAt( hash, key );
        /* not existed */
        if(  IsNull( position )  )
        {
            ++hash->used_count;
            memcpy( position, element, hash->conf.element_size );
            return position;
        }
        /* found */
        else if( IsEqual( position, element ) )
        {
            ++hash->same_count;
            return position;
        }
        else
        {
            /* conflicted */
            ++hash->conflict_count;
            if( start == -1 )
                start = key;
            /* not existed && cannot store */
            else if( start == key )
            {
                ++hash->nospace_count;
                DoFree( position );
                return NULL;
            }
            key = (( key << 4 ) + key + 37 ) % max;
        }
    }
    return NULL;
} /* - - - - - - - end of function HashSearch - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  HashPrint
 *  Description:  
 * =====================================================================================
 */
int
HashPrint( Hash *hash )
{
    int i;
    void *element;
    HashIsNull IsNull;
    HashElementPrint ElementPrint;
    IsNull = hash->conf.IsNull;
    ElementPrint = hash->conf.ElementPrint;
    for( i = hash->conf.total - 1; i >= 0; --i )
    {
        element = HashAt( hash, i );
        if( IsNull( element ) )
            continue;
        ElementPrint( element );
    }
    return 0;
} /* - - - - - - - end of function HashPrint - - - - - - - - - - - */
