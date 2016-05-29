/*
 * =====================================================================================
 *
 *       Filename:  linkedlist.h
 *
 *    Description:  
 *
 *        Created:  03/29/2012 11:57:13 AM
 *       Compiler:  gcc
 *         Author:  Xu Jian (XJ), xujian@chinacache.com
 *        COMPANY:  ChinaCache
 *
 * =====================================================================================
 */
#ifndef _LINKED_LIST_H_DB66A843_B302_4E11_AA0D_F01105B80C9E_
#define _LINKED_LIST_H_DB66A843_B302_4E11_AA0D_F01105B80C9E_
typedef unsigned int (*LinkedListGetKey)( const void* );
typedef int (*LinkedListIsEqual)( const void*, const void* );
typedef int (*LinkedListIsNull)( const void* );
typedef int (*LinkedListDoFree)( void* );
typedef int (*LinkedListTraversalFunction)( void *local, void *param );
typedef struct _LinkedListConfigure
{
    LinkedListIsEqual IsEqual;
    LinkedListIsNull IsNull;
    LinkedListDoFree DoFree;
} LinkedListConfigure; /* - - - - - - end of struct LinkedListConfigure - - - - - - */
typedef struct _LinkedListNode
{
    void *data;
    struct _LinkedListNode *next;
} LinkedListNode; /* - - - - - - end of struct LinkedListNode - - - - - - */
typedef struct _LinkedList
{
    LinkedListConfigure conf;
    LinkedListNode *head;
} LinkedList; /* - - - - - - end of struct LinkedList - - - - - - */
int LinkedListDestroy( LinkedList *linked );
LinkedList* LinkedListCreate( const LinkedListConfigure *conf );
void* LinkedListTraversal( LinkedList *linked, LinkedListTraversalFunction Traversal, void *param );
void* LinkedListSearch( LinkedList *linked, const void *element );
void* LinkedListInsert( LinkedList *linked, void *element );
#endif /* _LINKED_LIST_H_DB66A843_B302_4E11_AA0D_F01105B80C9E_ */
