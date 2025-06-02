/**************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation
 * All rights reserved.
 * Copyright (c) 2012 France Telecom All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * - Neither name of Intel Corporation nor the names of its contributors
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
 **************************************************************************/

#include "LinkedList.h"

#ifndef _WIN32
	/* Do not #include <sys/param.h> on Windows */
	#include <sys/param.h>
#endif

#if (defined(BSD) && BSD >= 199306) || defined(__APPLE__)
	#include <stdlib.h>
#else
	#include <malloc.h>
#endif

#include <assert.h>
#include <errno.h> // IWYU pragma: keep

static int freeListNode(ListNode *node, LinkedList *list)
{
	assert(list != 0);

	return FreeListFree(&list->freeNodeList, node);
}

/*!
 * \brief Dynamically creates a list node.
 *
 *  Parameters:
 *      void * item - the item to store
 *  Returns:
 *      The new node, 0 on failure.
 */
static ListNode *CreateListNode(
	/*! the item to store. */
	void *item,
	/*! The list to add it to. */
	LinkedList *list)
{
	ListNode *temp = 0;

	assert(list != 0);

	temp = (ListNode *)FreeListAlloc(&list->freeNodeList);
	if (temp) {
		temp->prev = 0;
		temp->next = 0;
		temp->item = item;
	}

	return temp;
}

int ListInit(LinkedList *list, cmp_routine cmp_func, free_function free_func)
{
	int retCode = 0;

	assert(list != 0);

	if (!list)
		return EINVAL;
	list->size = 0;
	list->cmp_func = cmp_func;
	list->free_func = free_func;
	retCode = FreeListInit(
		&list->freeNodeList, sizeof(ListNode), FREELISTSIZE);

	assert(retCode == 0);

	list->head.item = 0;
	list->head.next = &list->tail;
	list->head.prev = 0;
	list->tail.item = 0;
	list->tail.prev = &list->head;
	list->tail.next = 0;

	return retCode;
}

ListNode *ListAddHead(LinkedList *list, void *item)
{
	assert(list != 0);

	if (!list)
		return 0;

	return ListAddAfter(list, item, &list->head);
}

ListNode *ListAddTail(LinkedList *list, void *item)
{
	assert(list != 0);

	if (!list)
		return 0;

	return ListAddBefore(list, item, &list->tail);
}

ListNode *ListAddAfter(LinkedList *list, void *item, ListNode *bnode)
{
	ListNode *newNode = 0;

	assert(list != 0);

	if (!list || !bnode)
		return 0;
	newNode = CreateListNode(item, list);
	if (newNode) {
		ListNode *temp = bnode->next;

		bnode->next = newNode;
		newNode->prev = bnode;
		newNode->next = temp;
		temp->prev = newNode;
		list->size++;

		return newNode;
	}

	return 0;
}

ListNode *ListAddBefore(LinkedList *list, void *item, ListNode *anode)
{
	ListNode *newNode = 0;

	assert(list != 0);

	if (!list || !anode)
		return 0;
	newNode = CreateListNode(item, list);
	if (newNode) {
		ListNode *temp = anode->prev;

		anode->prev = newNode;
		newNode->next = anode;
		newNode->prev = temp;
		temp->next = newNode;
		list->size++;

		return newNode;
	}

	return 0;
}

void *ListDelNode(LinkedList *list, ListNode *dnode, int freeItem)
{
	void *temp;

	assert(list != 0);
	assert(dnode != &list->head);
	assert(dnode != &list->tail);

	if (!list || dnode == &list->head || dnode == &list->tail || !dnode)
		return 0;
	temp = dnode->item;
	dnode->prev->next = dnode->next;
	dnode->next->prev = dnode->prev;
	freeListNode(dnode, list);
	list->size--;
	if (freeItem && list->free_func) {
		list->free_func(temp);
		temp = 0;
	}

	return temp;
}

int ListDestroy(LinkedList *list, int freeItem)
{
	ListNode *dnode = 0;
	ListNode *temp = 0;

	if (!list)
		return EINVAL;

	for (dnode = list->head.next; dnode != &list->tail;) {
		temp = dnode->next;
		ListDelNode(list, dnode, freeItem);
		dnode = temp;
	}
	list->size = 0;
	FreeListDestroy(&list->freeNodeList);

	return 0;
}

ListNode *ListHead(LinkedList *list)
{
	assert(list != 0);

	if (!list)
		return 0;

	if (!list->size)
		return 0;
	else
		return list->head.next;
}

ListNode *ListTail(LinkedList *list)
{
	assert(list != 0);

	if (!list)
		return 0;

	if (!list->size)
		return 0;
	else
		return list->tail.prev;
}

ListNode *ListNext(LinkedList *list, ListNode *node)
{
	assert(list != 0);
	assert(node != 0);

	if (!list || !node)
		return 0;
	if (node->next == &list->tail)
		return 0;
	else
		return node->next;
}

ListNode *ListPrev(LinkedList *list, ListNode *node)
{
	assert(list != 0);
	assert(node != 0);

	if (!list || !node)
		return 0;

	if (node->prev == &list->head)
		return 0;
	else
		return node->prev;
}

ListNode *ListFind(LinkedList *list, ListNode *start, void *item)
{
	ListNode *finger = 0;

	if (!list)
		return 0;
	if (!start)
		start = &list->head;

	assert(start);

	finger = start->next;

	assert(finger);

	while (finger != &list->tail) {
		if (list->cmp_func) {
			if (list->cmp_func(item, finger->item))
				return finger;
		} else {
			if (item == finger->item)
				return finger;
		}
		finger = finger->next;
	}

	return 0;
}

long ListSize(LinkedList *list)
{
	assert(list != 0);

	if (!list)
		return EINVAL;

	return list->size;
}
