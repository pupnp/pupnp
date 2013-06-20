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

#ifndef FREE_LIST_H
#define FREE_LIST_H

/*!
 * \file
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "ithread.h"

#include <errno.h>

/*!
 * Free list node. points to next free item.
 * Memory for node is borrowed from allocated items.
 * \internal
 */
typedef struct FREELISTNODE
{
	struct FREELISTNODE *next;
} FreeListNode;

/*!
 * Stores head and size of free list, as well as mutex for protection.
 * \internal
 */
typedef struct FREELIST
{
	FreeListNode *head;
	size_t element_size;
 	int maxFreeListLength;
	int freeListLength;
} FreeList;

/*!
 * \brief Initializes Free List.
 *
 * Must be called first and only once for FreeList.
 *
 * \return:
 *	\li \c 0 on success.
 *	\li \c EINVAL on failure.
 */
int FreeListInit(
	/*! Must be valid, non null, pointer to a linked list. */
	FreeList *free_list,
	/*! Size of elements to store in free list. */
	size_t elementSize,
	/*! Max size that the free list can grow to before returning
	 * memory to O.S. */
	int maxFreeListLength);

/*!
 * \brief Allocates chunk of set size.
 *
 * If a free item is available in the list, returnes the stored item,
 * otherwise calls the O.S. to allocate memory.
 *
 * \return Non NULL on success. NULL on failure.
 */
void *FreeListAlloc(
	/*! Must be valid, non null, pointer to a linked list. */
	FreeList *free_list);

/*!
 * \brief Returns an item to the Free List.
 *
 * If the free list is smaller than the max size then adds the item to the
 * free list, otherwise returns the item to the O.S.
 *
 * \return:
 *	\li \c 0 on success.
 *	\li \c EINVAL on failure.
 */
int FreeListFree(
	/*! Must be valid, non null, pointer to a free list. */
	FreeList *free_list,
	/*! Must be a pointer allocated by FreeListAlloc. */
	void *element);

/*!
 * \brief Releases the resources stored with the free list.
 *
 * \return:
 *	\li \c 0 on success.
 *	\li \c EINVAL on failure.
 */
int FreeListDestroy(
	/*! Must be valid, non null, pointer to a linked list. */
	FreeList *free_list);

#ifdef __cplusplus
}
#endif

#endif /* FREE_LIST_H */

