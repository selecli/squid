/*
 * =====================================================================================
 *
 *       Filename:  linked_list.h
 *
 *    Description:  
 *
 *        Created:  12/14/2011 10:14:55 AM
 *       Compiler:  gcc
 *         Author:  XuJian
 *        Company:  China Cache
 *
 * =====================================================================================
 */
#ifndef _LINK_LIST_H_4EE81C08_
#define _LINK_LIST_H_4EE81C08_ 
typedef int (*LinkedListNodeDataFree)( void* );
typedef struct _LinkedListNode {
    void *data;
    struct _LinkedListNode *next, *prev;
} LinkedListNode; /* - - - - - - end of struct LinkedListNode - - - - - - */
typedef struct _LinkedList {
    int amount;
    LinkedListNode *position;
    LinkedListNode *head;
    LinkedListNodeDataFree NodeDataFree;
} LinkedList; /* - - - - - - end of struct LinkedList - - - - - - */
LinkedList* LinkedListCreate( LinkedListNodeDataFree NodeDataFree );
int LinkedListDestroy( LinkedList *list );
void* LinkedListData( void* p );
void* LinkedListGet( LinkedList *list );
void* LinkedListGetData( LinkedList *list );
int LinkedListIsEmpty( LinkedList *list );
int LinkedListDelete( LinkedList *list, void *p );
void* LinkedListCycleInsert( LinkedList *list, void *data );
void* LinkedListRewind( LinkedList *list );
int LinkedListMovePosition( LinkedList *list, void *p );
#endif /* _LINK_LIST_H_4EE81C08_ */
