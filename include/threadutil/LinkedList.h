/*******************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation 
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met: 
 *
 * * Redistributions of source code must retain the above copyright notice, 
 * this list of conditions and the following disclaimer. 
 * * Redistributions in binary form must reproduce the above copyright notice, 
 * this list of conditions and the following disclaimer in the documentation 
 * and/or other materials provided with the distribution. 
 * * Neither name of Intel Corporation nor the names of its contributors 
 * may be used to endorse or promote products derived from this software 
 * without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR 
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY 
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

#ifndef LINKED_LIST_H
#define LINKED_LIST_H

/*!
 * \file
 */

#include "FreeList.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EOUTOFMEM (-7 & 1<<29)

#define FREELISTSIZE 100
#define LIST_SUCCESS 1
#define LIST_FAIL 0

/*! Function for freeing list items. */
typedef void (*free_function)(void *arg);

/*! Function for comparing list items. Returns 1 if itemA==itemB */
typedef int (*cmp_routine)(void *itemA,void *itemB);

/*! Linked list node. Stores generic item and pointers to next and prev.
 * \internal
 */
typedef struct LISTNODE
{
	struct LISTNODE *prev;
	struct LISTNODE *next;
	void *item;
} ListNode;

/*!
 * Linked list (no protection).
 *
 * Because this is for internal use, parameters are NOT checked for validity.
 * The first item of the list is stored at node: head->next
 * The last item of the list is stored at node: tail->prev
 * If head->next=tail, then list is empty.
 * To iterate through the list:
 *
 *	LinkedList g;
 *	ListNode *temp = NULL;
 *	for (temp = ListHead(g);temp!=NULL;temp = ListNext(g,temp)) {
 *	}
 *
 * \internal
 */
typedef struct LINKEDLIST
{
	/*! head, first item is stored at: head->next */
	ListNode head;
	/*! tail, last item is stored at: tail->prev  */
	ListNode tail;
	/*! size of list */
	long size;
	/*! free list to use */
	FreeList freeNodeList;
	/*! free function to use */
	free_function free_func;
	/*! compare function to use */
	cmp_routine cmp_func;
} LinkedList;

/*!
 * \brief Initializes LinkedList. Must be called first and only once for List.
 * 
 *  \return
 *	\li \c 0 on success.
 *	\li \c EOUTOFMEM on failure.
 */
int ListInit(
	/*! Must be valid, non null, pointer to a linked list. */
	LinkedList *list,
	/*! Function used to compare items. (May be NULL). */
	cmp_routine cmp_func,
	/*! Function used to free items. (May be NULL). */
	free_function free_func);

/*!
 * \brief Adds a node to the head of the list. Node gets immediately after
 * list head.
 *
 *  Precondition:
 *      The list has been initialized.
 *
 * \return The pointer to the ListNode on success, NULL on failure.
 */
ListNode *ListAddHead(
	/*! Must be valid, non null, pointer to a linked list. */
	LinkedList *list,
	/*! Item to be added. */
	void *item);

/*!
 * \brief Adds a node to the tail of the list. Node gets added immediately
 * before list.tail.
 *
 * Precondition: The list has been initialized.
 *
 * \return The pointer to the ListNode on success, NULL on failure.
 */
ListNode *ListAddTail(
	/*! Must be valid, non null, pointer to a linked list. */
	LinkedList *list,
	/*! Item to be added. */
	void *item);

/*!
 * \brief Adds a node after the specified node. Node gets added immediately
 * after bnode.
 *
 *  Precondition: The list has been initialized.
 *
 * \return The pointer to the ListNode on success, NULL on failure.
 */
ListNode *ListAddAfter(
	/*! Must be valid, non null, pointer to a linked list. */
	LinkedList *list,
	/*! Item to be added. */
	void *item,
	/*! Node to add after. */
	ListNode *bnode);

/*!
 * \brief Adds a node before the specified node. Node gets added immediately
 * before anode.
 *
 * Precondition: The list has been initialized.
 *
 * \return The pointer to the ListNode on success, NULL on failure.
 */
ListNode *ListAddBefore(
	/*! Must be valid, non null, pointer to a linked list. */
	LinkedList *list,
	/*! Item to be added. */
	void *item,
	/*! Node to add in front of. */
	ListNode *anode);

/*!
 * \brief Removes a node from the list. The memory for the node is freed.
 *
 * Precondition: The list has been initialized.
 *
 * \return The pointer to the item stored in the node or NULL if the item
 * is freed.
 */
void *ListDelNode(
	/*! Must be valid, non null, pointer to a linked list. */
	LinkedList *list,
	/*! Node to delete. */
	ListNode *dnode,
	/*! if !0 then item is freed using free function. If 0 (or free
	 * function is NULL) then item is not freed. */
	int freeItem);

/*!
 * \brief Removes all memory associated with list nodes. Does not free
 * LinkedList *list.
 *
 * Precondition: The list has been initialized.
 *
 * \return 0 on success, EINVAL on failure.
 */
int ListDestroy(
	/*! Must be valid, non null, pointer to a linked list. */
	LinkedList *list,
	/*! if !0 then item is freed using free function. If 0 (or free
	 * function is NULL) then item is not freed. */
	int freeItem);

/*!
 * \brief Returns the head of the list.
 *    
 * Precondition: The list has been initialized.
 *
 * \return The head of the list. NULL if list is empty.
 */
ListNode *ListHead(
	/*! Must be valid, non null, pointer to a linked list. */
	LinkedList *list);

/*!
 * \brief Returns the tail of the list.
 *    
 * Precondition: The list has been initialized.
 *
 * \return The tail of the list. NULL if list is empty.
 */
ListNode *ListTail(
	/*! Must be valid, non null, pointer to a linked list. */
	LinkedList *list);

/*!
 * \brief Returns the next item in the list.
 *    
 * Precondition: The list has been initialized.
 *
 * \return The next item in the list. NULL if there are no more items in list.
 */
ListNode *ListNext(
	/*! Must be valid, non null, pointer to a linked list. */
	LinkedList *list,
	/*! Node from the list. */
	ListNode *node);

/*!
 * \brief Returns the previous item in the list.
 *    
 * Precondition: The list has been initialized.
 *
 * \return The previous item in the list. NULL if there are no more items in list.
 */
ListNode *ListPrev(
	/*! Must be valid, non null, pointer to a linked list. */
	LinkedList *list,
	/*! Node from the list. */
	ListNode *node);

/*!
 * \brief Finds the specified item in the list.
 *
 * Uses the compare function specified in ListInit. If compare function
 * is NULL then compares items as pointers.
 *
 * Precondition: The list has been initialized.
 *
 * \return The node containing the item. NULL if no node contains the item.
 */
ListNode* ListFind(
	/*! Must be valid, non null, pointer to a linked list. */
	LinkedList *list,
	/*! The node to start from, NULL if to start from beginning. */
	ListNode *start,
	/*! The item to search for. */
	void *item);

/*!
 * \brief Returns the size of the list.
 *
 * Precondition: The list has been initialized.
 *
 * \return The number of items in the list.
 */
long ListSize(
	/*! Must be valid, non null, pointer to a linked list. */
	LinkedList* list);

#ifdef __cplusplus
}
#endif

#endif /* LINKED_LIST_H */

