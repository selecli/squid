/*
 * =====================================================================================
 *
 *       Filename:  linkedlist.c
 *
 *    Description:  Single linked list.
 *
 *        Created:  03/29/2012 11:53:16 AM
 *       Compiler:  gcc
 *         Author:  Xu Jian (XJ), xujian@chinacache.com
 *        COMPANY:  ChinaCache
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "linkedlist.h"
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  LinkedListDestroy
 *  Description:  Destroy data in linked.
 *                Need to call free() on linked.
 * =====================================================================================
 */
    int
LinkedListDestroy( LinkedList *linked )
{
    LinkedListNode *node, *next;
    LinkedListDoFree DoFree;
    assert( linked );
    DoFree = linked->conf.DoFree;
    for( node = linked->head; node != NULL; node = next )
    {
        next = node->next;
        DoFree( node->data );
        node->data = NULL;
        node->next = NULL;
        free( node );
    }
    linked->head = NULL;
    return 0;
} /* - - - - - - - end of function LinkedListDestroy - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  LinkedListCreate
 *  Description:  Do malloc.
 *                Need to call free()
 * =====================================================================================
 */
    LinkedList*
LinkedListCreate( const LinkedListConfigure *conf )
{
    LinkedList *linked;
    assert( conf );
    linked = (LinkedList *)malloc( sizeof(LinkedList) );
    if( linked == NULL )
        return NULL;
    memcpy( &linked->conf, conf, sizeof(LinkedListConfigure) );
    linked->head = NULL;
    return linked;
} /* - - - - - - - end of function LinkedListCreate - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  LinkedListTraversal
 *  Description:  Break while Traversal() returns non-zero
 * =====================================================================================
 */
    void*
LinkedListTraversal( LinkedList *linked, LinkedListTraversalFunction Traversal, void *param )
{
    LinkedListNode *node;
    assert( Traversal );
    if( linked == NULL )
        return NULL;
    for( node = linked->head; node != NULL; node = node->next )
        if( Traversal( node->data, param ) != 0 )
            break;
    return node;
} /* - - - - - - - end of function LinkedListTraversal - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  LinkedListSearch
 *  Description:  
 * =====================================================================================
 */
    void*
LinkedListSearch( LinkedList *linked, const void *element )
{
    LinkedListNode *node;
    LinkedListIsEqual IsEqual;
    assert( element );
    if( linked == NULL )
        return NULL;
    IsEqual = linked->conf.IsEqual;
    for( node = linked->head; node != NULL; node = node->next )
        if( IsEqual( node->data, element ) )
            return (void*)node->data;
    return NULL;
} /* - - - - - - - end of function LinkedListSearch - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  LinkedListInsert
 *  Description:  Do malloc
 *                Need to call LinkedListDestroy().
 *                May insert same element.
 * =====================================================================================
 */
    void*
LinkedListInsert( LinkedList *linked, void *element )
{
    LinkedListNode *node;
    assert( linked );
    assert( element );
    node = (LinkedListNode *)malloc( sizeof(LinkedListNode ) );
    if( node == NULL )
        return NULL;
    node->data = element;
    node->next = linked->head;
    linked->head = node;
    return node;
} /* - - - - - - - end of function LinkedListInser - - - - - - - - - - - */
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  LinkedListAt
 *  Description:
 * =====================================================================================
 */
    void*
LinkedListAt( LinkedList *linked, int n )
{
    int count;
    LinkedListNode *node;
    assert( linked );
    assert( n >= 0 );
    count = 0;
    for( node = linked->head; node != NULL; node = node->next )
        if( ++count > n )
            return node->data;
    return 0;
} /* - - - - - - - end of function LinkedListAt - - - - - - - - - - - */
