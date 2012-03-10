/*******************************************************************************
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
 ******************************************************************************/


/*!
 * \file
 */


#include "ixmlparser.h"


#include <assert.h>
#include <string.h>


void ixmlNodeList_init(IXML_NodeList *nList)
{
	assert(nList != NULL);

	memset(nList, 0, sizeof (IXML_NodeList));
}


IXML_Node *ixmlNodeList_item(
	IXML_NodeList *nList,
	unsigned long index)
{
	IXML_NodeList *next;
	unsigned int i;

	/* if the list ptr is NULL */
	if (nList == NULL) {
		return NULL;
	}
	/* if index is more than list length */
	if (index > ixmlNodeList_length(nList) - 1lu) {
		return NULL;
	}

	next = nList;
	for (i = 0u; i < index && next != NULL; ++i) {
		next = next->next;
	}

	if (next == NULL) {
		return NULL;
	}

	return next->nodeItem;
}

int ixmlNodeList_addToNodeList(
	IXML_NodeList **nList,
	IXML_Node *add)
{
	IXML_NodeList *traverse = NULL;
	IXML_NodeList *p = NULL;
	IXML_NodeList *newListItem;

	assert(add != NULL);

	if (add == NULL) {
		return IXML_FAILED;
	}

	if (*nList == NULL) {
		/* nodelist is empty */
		*nList = (IXML_NodeList *)malloc(sizeof (IXML_NodeList));
		if (*nList == NULL) {
			return IXML_INSUFFICIENT_MEMORY;
		}

		ixmlNodeList_init(*nList);
	}

	if ((*nList)->nodeItem == NULL) {
		(*nList)->nodeItem = add;
	} else {
		traverse = *nList;
		while (traverse != NULL) {
			p = traverse;
			traverse = traverse->next;
		}

		newListItem = (IXML_NodeList *)malloc(sizeof (IXML_NodeList));
		if (newListItem == NULL) {
			return IXML_INSUFFICIENT_MEMORY;
		}
		p->next = newListItem;
		newListItem->nodeItem = add;
		newListItem->next = NULL;
	}

	return IXML_SUCCESS;
}


unsigned long ixmlNodeList_length(IXML_NodeList *nList)
{
	IXML_NodeList *list;
	unsigned long length = 0lu;

	list = nList;
	while (list != NULL) {
		++length;
		list = list->next;
	}

	return length;
}


void ixmlNodeList_free(IXML_NodeList *nList)
{
	IXML_NodeList *next;

	while (nList != NULL) {
		next = nList->next;
		free(nList);
		nList = next;
	}
}

