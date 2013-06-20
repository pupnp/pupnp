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


/*!
 * \file
 */


#include "ixmlparser.h"


#include <assert.h>
#include <stdlib.h> /* for free(), malloc() */
#include <string.h>


/*!
 * \brief Return the item number of a item in NamedNodeMap.
 */
static unsigned long ixmlNamedNodeMap_getItemNumber(
	/*! [in] The named node map to process. */
	IN IXML_NamedNodeMap *nnMap,
	/*! [in] The name of the item to find. */
	IN const char *name)
{
	IXML_Node *tempNode;
	unsigned long returnItemNo = 0lu;

	assert(nnMap != NULL && name != NULL);
	if (nnMap == NULL || name == NULL) {
		return (unsigned long)IXML_INVALID_ITEM_NUMBER;
	}

	tempNode = nnMap->nodeItem;
	while (tempNode != NULL) {
		if (strcmp(name, tempNode->nodeName) == 0) {
			return returnItemNo;
		}
		tempNode = tempNode->nextSibling;
		returnItemNo++;
	}

	return (unsigned long)IXML_INVALID_ITEM_NUMBER;
}


void ixmlNamedNodeMap_init(IXML_NamedNodeMap *nnMap)
{
	assert(nnMap != NULL);

	memset(nnMap, 0, sizeof (IXML_NamedNodeMap));
}


IXML_Node *ixmlNamedNodeMap_getNamedItem(
	IXML_NamedNodeMap *nnMap,
	const DOMString name)
{
	unsigned long index;

	if (nnMap == NULL || name == NULL) {
		return NULL;
	}

	index = ixmlNamedNodeMap_getItemNumber(nnMap, name);
	if (index == (unsigned long)IXML_INVALID_ITEM_NUMBER) {
		return NULL;
	} else {
		return ixmlNamedNodeMap_item(nnMap, index);
	}
}


IXML_Node *ixmlNamedNodeMap_item(
	IN IXML_NamedNodeMap *nnMap,
	IN unsigned long index )
{
	IXML_Node *tempNode;
	unsigned int i;

	if (nnMap == NULL) {
		return NULL;
	}

	if (index > ixmlNamedNodeMap_getLength(nnMap) - 1lu) {
		return NULL;
	}

	tempNode = nnMap->nodeItem;
	for (i = 0u; i < index && tempNode != NULL; ++i) {
		tempNode = tempNode->nextSibling;
	}

	return tempNode;
}


unsigned long ixmlNamedNodeMap_getLength(IXML_NamedNodeMap *nnMap)
{
	IXML_Node *tempNode;
	unsigned long length = 0lu;

	if (nnMap != NULL) {
		tempNode = nnMap->nodeItem;
		for (length = 0lu; tempNode != NULL; ++length) {
			tempNode = tempNode->nextSibling;
		}
	}

	return length;
}


void ixmlNamedNodeMap_free(IXML_NamedNodeMap *nnMap)
{
	IXML_NamedNodeMap *pNext;

	while (nnMap != NULL) {
		pNext = nnMap->next;
		free(nnMap);
		nnMap = pNext;
	}
}


int ixmlNamedNodeMap_addToNamedNodeMap(
	IXML_NamedNodeMap **nnMap,
	IXML_Node *add)
{
	IXML_NamedNodeMap *traverse = NULL;
	IXML_NamedNodeMap *p = NULL;
	IXML_NamedNodeMap *newItem = NULL;

	if(add == NULL) {
		return IXML_SUCCESS;
	}

	if (*nnMap == NULL) {
		/* nodelist is empty */
		*nnMap = (IXML_NamedNodeMap *)malloc(sizeof (IXML_NamedNodeMap));
		if (*nnMap == NULL) {
			return IXML_INSUFFICIENT_MEMORY;
		}
		ixmlNamedNodeMap_init(*nnMap);
	}
	if ((*nnMap)->nodeItem == NULL) {
		(*nnMap)->nodeItem = add;
	} else {
		traverse = *nnMap;
		p = traverse;
		while (traverse != NULL) {
			p = traverse;
			traverse = traverse->next;
		}
		newItem = (IXML_NamedNodeMap *)malloc(sizeof (IXML_NamedNodeMap));
		if (newItem == NULL) {
			return IXML_INSUFFICIENT_MEMORY;
		}
		p->next = newItem;
		newItem->nodeItem = add;
		newItem->next = NULL;
	}

	return IXML_SUCCESS;
}

