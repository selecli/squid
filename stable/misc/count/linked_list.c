/*
 * =====================================================================================
 *
 *       Filename:  linked_list.c
 *
 *    Description:  
 *
 *        Created:  12/14/2011 10:13:09 AM
 *       Compiler:  gcc
 *         Author:  XuJian
 *        Company:  China Cache
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "linked_list.h"
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  LinkedListCreate
 *  Description:  
 * =====================================================================================
 */
LinkedList*
LinkedListCreate( LinkedListNodeDataFree NodeDataFree )
{
    LinkedList *list;
    if( NodeDataFree == NULL )
        return NULL;
    list = (LinkedList *)malloc( sizeof(LinkedList) );
    if( list == NULL )
    {
        perror( "malloc failed." );
        return NULL;
    }
    memset( list, 0, sizeof(LinkedList) );
    list->NodeDataFree = NodeDataFree;
    return list;
} /* - - - - - - - end of function LinkedListCreate - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  LinkedListDestroy
 *  Description:  
 * =====================================================================================
 */
int
LinkedListDestroy( LinkedList *list )
{
    LinkedListNode *p, *begin;
    if( list->amount == 0 )
        return 0;
    begin = list->head;
    p = begin;
    do
    {
        LinkedListNode *next;
        list->NodeDataFree( p->data );
        next = p->next;
        p->next = NULL;
        p->prev = NULL;
        p = next;
    }while( p != begin );
    list->head = list->position = NULL;
    list->amount = 0;
    free( list );
    return 0;
} /* - - - - - - - end of function LinkedListDestroy - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  LinkedListData
 *  Description:  
 * =====================================================================================
 */
void*
LinkedListData( void* p )
{
    LinkedListNode *node;
    if( p == NULL )
        return NULL;
    node = (LinkedListNode*)p;
    return (void*)node->data;
}
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  LinkedListGet
 *  Description:  
 * =====================================================================================
 */
void*
LinkedListGet( LinkedList *list )
{
    void *ret;
    if( list->position == NULL )
        return NULL;
    ret = (void*)list->position;
    list->position = list->position->next;
    return ret;
} /* - - - - - - - end of function LinkedListGet - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  LinkedListGetData
 *  Description:  
 * =====================================================================================
 */
void*
LinkedListGetData( LinkedList *list )
{
    void *ret;
    if( list->position == NULL )
        return NULL;
    ret = (void*)list->position->data;
    list->position = list->position->next;
    return ret;
} /* - - - - - - - end of function LinkedListGetData - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  LinkedListIsEmpty
 *  Description:  
 * =====================================================================================
 */
int
LinkedListIsEmpty( LinkedList *list )
{
    return( list->amount == 0 );
} /* - - - - - - - end of function LinkedListIsEmpty - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  LinkedListDelete
 *  Description:  
 * =====================================================================================
 */
int
LinkedListDelete( LinkedList *list, void *p )
{
    LinkedListNode *node;
    node = (LinkedListNode*)p;
    list->NodeDataFree( node->data );
#if 1
    if( list->amount == 1 )
    {
        node->data = NULL;
        node->next = NULL;
        node->prev = NULL;
        list->position = NULL;
        list->head = NULL;
        list->amount = 0;
        return 0;
    }
    if( list->position == node )
        list->position = node->next;
    if( list->head == node )
        list->head = node->next;
    node->next->prev = node->prev;
    node->prev->next = node->next;
    node->next = NULL;
    node->prev = NULL;
    free( node );
    -- list->amount;
#else /* not 1 */
    if( list->amount == 1 )
    {
        node->data = NULL;
        node->next = NULL;
        list->position = NULL;
        list->head = NULL;
        list->amount = 0;
        return 0;
    }
    if( list->position == node->next )
        list->position = node;
    if( list->head == node->next )
        list->head = node;
    node->data = node->next->data;
    next = node->next->next;
    free( node->next );
    node->next = next;
    -- list->amount;
#endif /* not 1 */
    return 0;
} /* - - - - - - - end of function LinkedListDelete - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  LinkedListCycleInsert
 *  Description:  
 * =====================================================================================
 */
void*
LinkedListCycleInsert( LinkedList *list, void *data )
{
    LinkedListNode *node;
    node = (LinkedListNode *)malloc( sizeof(LinkedListNode) );
    if( node == NULL )
    {
        perror( "malloc failed" );
        return NULL;
    }
    node->data = data;
#if 1
    if( list->head == NULL )
    {
        node->next = node;
        node->prev = node;
        list->head = node;
        list->position = node;
    }
    else
    {
        LinkedListNode *head, *next;
        head = list->head;
        next = head->next;
        node->next = next;
        node->prev = head;
        head->next = node;
        next->prev = node;
    }
#else /* not 1 */
    if( list->head == NULL )
    {
        node->next = node;
        list->head = node;
        list->position = node;
    }
    else
    {
        node->next = list->head->next->next;
        list->head->next->next = node;
    }
#endif /* not 1 */
    ++list->amount;
    return (void*)node;
} /* - - - - - - - end of function LinkedListCycleInsert - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  LinkedListRewind
 *  Description:  
 * =====================================================================================
 */
void*
LinkedListRewind( LinkedList *list )
{
    void *ret;
    ret = list->position;
    list->position = list->head;
    return ret;
} /* - - - - - - - end of function LinkedListRewind - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  LinkedListMovePosition
 *  Description:  
 * =====================================================================================
 */
int
LinkedListMovePosition( LinkedList *list, void *p )
{
    list->position = p;
    return 0;
} /* - - - - - - - end of function LinkedListMovePosition - - - - - - - - - - - */
