/**************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation 
 * All rights reserved. 
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

#include "FreeList.h"

#include <assert.h>
#include <stdlib.h>

int FreeListInit(FreeList *free_list, size_t elementSize, int maxFreeListLength)
{
	assert(free_list != NULL);

	if (free_list == NULL)
		return EINVAL;
	free_list->element_size = elementSize;
	free_list->maxFreeListLength = maxFreeListLength;
	free_list->head = NULL;
	free_list->freeListLength = 0;

	return 0;
}

void *FreeListAlloc(FreeList *free_list)
{
	FreeListNode *ret = NULL;

	assert(free_list != NULL);

	if (free_list == NULL)
		return NULL;

	if (free_list->head) {
		ret = free_list->head;
		free_list->head = free_list->head->next;
		free_list->freeListLength--;
	} else {
		ret = malloc(free_list->element_size);
	}

	return ret;
}

int FreeListFree(FreeList *free_list, void *element)
{
	FreeListNode *temp = NULL;

	assert(free_list != NULL);

	if (free_list == NULL)
		return EINVAL;
	if (element != NULL &&
	    free_list->freeListLength + 1 < free_list->maxFreeListLength) {
		free_list->freeListLength++;
		temp = (FreeListNode *)element;
		temp->next = free_list->head;
		free_list->head = temp;
	} else {
		free(element);
	}

	return 0;
}

int FreeListDestroy(FreeList *free_list)
{
	FreeListNode *temp = NULL;
	int i = 0;

	assert(free_list != NULL);

	if (!free_list)
		return EINVAL;
	while (free_list->head) {
		i++;
		temp = free_list->head->next;
		free(free_list->head);
		free_list->head = temp;
	}
	free_list->freeListLength = 0;

	return 0;
}

