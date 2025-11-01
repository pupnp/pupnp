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
#include <stdlib.h> /* for free() */
#include <string.h>

#include "posix_overwrites.h" // IWYU pragma: keep

void ixmlElement_init(IXML_Element *element)
{
	if (element) {
		memset(element, 0, sizeof(IXML_Element));
	}
}

const DOMString ixmlElement_getTagName(IXML_Element *element)
{
	if (element) {
		return element->tagName;
	} else {
		return NULL;
	}
}

int ixmlElement_setTagName(IXML_Element *element, const char *tagName)
{
	int rc = IXML_SUCCESS;

	assert(element && tagName);

	if (!element || !tagName) {
		return IXML_FAILED;
	}

	if (element->tagName) {
		free(element->tagName);
	}
	element->tagName = strdup(tagName);
	if (!element->tagName) {
		rc = IXML_INSUFFICIENT_MEMORY;
	}

	return rc;
}

const DOMString ixmlElement_getAttribute(
	IXML_Element *element, const DOMString name)
{
	IXML_Node *attrNode;

	if (!element || !name) {
		return NULL;
	}

	attrNode = element->n.firstAttr;
	while (attrNode) {
		if (strcmp(attrNode->nodeName, name) == 0) {
			return attrNode->nodeValue;
		} else {
			attrNode = attrNode->nextSibling;
		}
	}

	return NULL;
}

int ixmlElement_setAttribute(
	IXML_Element *element, const DOMString name, const DOMString value)
{
	IXML_Node *attrNode;
	IXML_Attr *newAttrNode;
	int errCode = IXML_SUCCESS;

	if (!element || !name || !value) {
		errCode = IXML_INVALID_PARAMETER;
		goto ErrorHandler;
	}

	if (Parser_isValidXmlName(name) == 0) {
		errCode = IXML_INVALID_CHARACTER_ERR;
		goto ErrorHandler;
	}

	attrNode = element->n.firstAttr;
	while (attrNode) {
		if (strcmp(attrNode->nodeName, name) == 0) {
			/* Found it */
			break;
		} else {
			attrNode = attrNode->nextSibling;
		}
	}

	if (!attrNode) {
		/* Add a new attribute */
		errCode = ixmlDocument_createAttributeEx(
			(IXML_Document *)element->n.ownerDocument,
			name,
			&newAttrNode);
		if (errCode != IXML_SUCCESS) {
			goto ErrorHandler;
		}

		attrNode = (IXML_Node *)newAttrNode;
		attrNode->nodeValue = strdup(value);
		if (!attrNode->nodeValue) {
			ixmlAttr_free(newAttrNode);
			errCode = IXML_INSUFFICIENT_MEMORY;
			goto ErrorHandler;
		}

		errCode = ixmlElement_setAttributeNode(
			element, newAttrNode, NULL);
		if (errCode != IXML_SUCCESS) {
			ixmlAttr_free(newAttrNode);
			goto ErrorHandler;
		}
	} else {
		if (attrNode->nodeValue) {
			/* Attribute name has a value already */
			free(attrNode->nodeValue);
		}
		attrNode->nodeValue = strdup(value);
		if (!attrNode->nodeValue) {
			errCode = IXML_INSUFFICIENT_MEMORY;
		}
	}

ErrorHandler:
	return errCode;
}

int ixmlElement_removeAttribute(IXML_Element *element, const DOMString name)
{
	IXML_Node *attrNode;

	if (!element || !name) {
		return IXML_INVALID_PARAMETER;
	}

	attrNode = element->n.firstAttr;
	while (attrNode) {
		if (strcmp(attrNode->nodeName, name) == 0) {
			/* Found it */
			break;
		} else {
			attrNode = attrNode->nextSibling;
		}
	}
	if (attrNode) {
		/* Has the attribute */
		if (attrNode->nodeValue) {
			free(attrNode->nodeValue);
			attrNode->nodeValue = NULL;
		}
	}

	return IXML_SUCCESS;
}

IXML_Attr *ixmlElement_getAttributeNode(
	IXML_Element *element, const DOMString name)
{
	IXML_Node *attrNode;

	if (!element || !name) {
		return NULL;
	}

	attrNode = element->n.firstAttr;
	while (attrNode) {
		if (strcmp(attrNode->nodeName, name) == 0) {
			/* found it */
			break;
		} else {
			attrNode = attrNode->nextSibling;
		}
	}

	return (IXML_Attr *)attrNode;
}

int ixmlElement_setAttributeNode(
	IXML_Element *element, IXML_Attr *newAttr, IXML_Attr **rtAttr)
{
	IXML_Node *attrNode = NULL;
	IXML_Node *node = NULL;
	IXML_Node *nextAttr = NULL;
	IXML_Node *prevAttr = NULL;
	IXML_Node *preSib = NULL;
	IXML_Node *nextSib = NULL;

	if (!element || !newAttr) {
		return IXML_INVALID_PARAMETER;
	}
	if (newAttr->n.ownerDocument != element->n.ownerDocument) {
		return IXML_WRONG_DOCUMENT_ERR;
	}
	if (newAttr->ownerElement) {
		return IXML_INUSE_ATTRIBUTE_ERR;
	}

	newAttr->ownerElement = element;
	node = (IXML_Node *)newAttr;
	attrNode = element->n.firstAttr;
	while (attrNode) {
		if (!strcmp(attrNode->nodeName, node->nodeName)) {
			/* Found it */
			break;
		} else {
			attrNode = attrNode->nextSibling;
		}
	}
	if (attrNode) {
		/* Already present, will replace by newAttr */
		preSib = attrNode->prevSibling;
		nextSib = attrNode->nextSibling;
		if (preSib) {
			preSib->nextSibling = node;
		}
		if (nextSib) {
			nextSib->prevSibling = node;
		}
		if (element->n.firstAttr == attrNode) {
			element->n.firstAttr = node;
		}
		if (rtAttr) {
			*rtAttr = (IXML_Attr *)attrNode;
		} else {
			ixmlAttr_free((IXML_Attr *)attrNode);
		}
	} else {
		/* Add this attribute */
		if (element->n.firstAttr) {
			prevAttr = element->n.firstAttr;
			nextAttr = prevAttr->nextSibling;
			while (nextAttr) {
				prevAttr = nextAttr;
				nextAttr = prevAttr->nextSibling;
			}
			prevAttr->nextSibling = node;
			node->prevSibling = prevAttr;
		} else {
			/* This is the first attribute node */
			element->n.firstAttr = node;
			node->prevSibling = NULL;
			node->nextSibling = NULL;
		}
		if (rtAttr) {
			*rtAttr = NULL;
		}
	}

	return IXML_SUCCESS;
}

/*!
 * \brief Find a attribute node whose contents are the same as the oldAttr.
 *
 * \return If found, the attribute node is returned, otherwise \b NULL is
 * returned.
 */
static IXML_Node *ixmlElement_findAttributeNode(
	/*! [in] The element to search for the attribute. */
	IXML_Element *element,
	/*! [in] The attribute node to match. */
	IXML_Attr *oldAttr)
{
	IXML_Node *attrNode;
	IXML_Node *oldAttrNode = (IXML_Node *)oldAttr;

	assert(element && oldAttr);

	attrNode = element->n.firstAttr;
	while (attrNode) {
		/* parentNode, prevSib, nextSib and ownerDocument doesn't matter
		 */
		if (ixmlNode_compare(attrNode, oldAttrNode) == 1) {
			/* Found it */
			break;
		} else {
			attrNode = attrNode->nextSibling;
		}
	}

	return attrNode;
}

int ixmlElement_removeAttributeNode(
	IXML_Element *element, IXML_Attr *oldAttr, IXML_Attr **rtAttr)
{
	IXML_Node *attrNode;
	IXML_Node *preSib;
	IXML_Node *nextSib;

	if (!element || !oldAttr) {
		return IXML_INVALID_PARAMETER;
	}

	attrNode = ixmlElement_findAttributeNode(element, oldAttr);
	if (attrNode) {
		/* Has the attribute */
		preSib = attrNode->prevSibling;
		nextSib = attrNode->nextSibling;
		if (preSib) {
			preSib->nextSibling = nextSib;
		}
		if (nextSib) {
			nextSib->prevSibling = preSib;
		}
		if (element->n.firstAttr == attrNode) {
			element->n.firstAttr = nextSib;
		}
		attrNode->parentNode = NULL;
		attrNode->prevSibling = NULL;
		attrNode->nextSibling = NULL;
		*rtAttr = (IXML_Attr *)attrNode;
		return IXML_SUCCESS;
	} else {
		return IXML_NOT_FOUND_ERR;
	}
}

IXML_NodeList *ixmlElement_getElementsByTagName(
	IXML_Element *element, const DOMString tagName)
{
	IXML_NodeList *returnNodeList = NULL;

	if (element && tagName) {
		ixmlNode_getElementsByTagName(
			(IXML_Node *)element, tagName, &returnNodeList);
	}
	return returnNodeList;
}

const DOMString ixmlElement_getAttributeNS(
	/* IN */ IXML_Element *element,
	/* IN */ const DOMString namespaceURI,
	/* IN */ const DOMString localName)
{
	IXML_Node *attrNode;

	if (!element || !namespaceURI || !localName) {
		return NULL;
	}

	attrNode = element->n.firstAttr;
	while (attrNode) {
		if (strcmp(attrNode->localName, localName) == 0 &&
			strcmp(attrNode->namespaceURI, namespaceURI) == 0) {
			/* Found it */
			return attrNode->nodeValue;
		} else {
			attrNode = attrNode->nextSibling;
		}
	}

	return NULL;
}

int ixmlElement_setAttributeNS(IXML_Element *element,
	const DOMString namespaceURI,
	const DOMString qualifiedName,
	const DOMString value)
{
	IXML_Node *attrNode = NULL;
	IXML_Node newAttrNode;
	IXML_Attr *newAttr;
	int rc;

	if (!element || !namespaceURI || !qualifiedName || !value) {
		return IXML_INVALID_PARAMETER;
	}
	if (Parser_isValidXmlName(qualifiedName) == 0) {
		return IXML_INVALID_CHARACTER_ERR;
	}
	ixmlNode_init(&newAttrNode);
	newAttrNode.nodeName = strdup(qualifiedName);
	if (!newAttrNode.nodeName) {
		return IXML_INSUFFICIENT_MEMORY;
	}
	rc = Parser_setNodePrefixAndLocalName(&newAttrNode);
	if (rc != IXML_SUCCESS) {
		Parser_freeNodeContent(&newAttrNode);
		return rc;
	}

	/* see DOM 2 spec page 59 */
	if ((newAttrNode.prefix && !namespaceURI) ||
		(newAttrNode.prefix && strcmp(newAttrNode.prefix, "xml") == 0 &&
			strcmp(namespaceURI,
				"http://www.w3.org/XML/1998/namespace") != 0) ||
		(strcmp(qualifiedName, "xmlns") == 0 &&
			strcmp(namespaceURI, "http://www.w3.org/2000/xmlns/") !=
				0)) {
		Parser_freeNodeContent(&newAttrNode);
		return IXML_NAMESPACE_ERR;
	}

	attrNode = element->n.firstAttr;
	while (attrNode) {
		if (strcmp(attrNode->localName, newAttrNode.localName) == 0 &&
			strcmp(attrNode->namespaceURI, namespaceURI) == 0) {
			/* Found it */
			break;
		} else {
			attrNode = attrNode->nextSibling;
		}
	}
	if (attrNode) {
		if (attrNode->prefix) {
			/* Remove the old prefix */
			free(attrNode->prefix);
		}
		/* replace it with the new prefix */
		if (newAttrNode.prefix) {
			attrNode->prefix = strdup(newAttrNode.prefix);
			if (!attrNode->prefix) {
				Parser_freeNodeContent(&newAttrNode);
				return IXML_INSUFFICIENT_MEMORY;
			}
		} else {
			attrNode->prefix = newAttrNode.prefix;
		}
		if (attrNode->nodeValue) {
			free(attrNode->nodeValue);
		}
		attrNode->nodeValue = strdup(value);
		if (!attrNode->nodeValue) {
			free(attrNode->prefix);
			Parser_freeNodeContent(&newAttrNode);
			return IXML_INSUFFICIENT_MEMORY;
		}
	} else {
		/* Add a new attribute */
		rc = ixmlDocument_createAttributeNSEx(
			(IXML_Document *)element->n.ownerDocument,
			namespaceURI,
			qualifiedName,
			&newAttr);
		if (rc != IXML_SUCCESS) {
			Parser_freeNodeContent(&newAttrNode);
			return rc;
		}
		newAttr->n.nodeValue = strdup(value);
		if (!newAttr->n.nodeValue) {
			ixmlAttr_free(newAttr);
			Parser_freeNodeContent(&newAttrNode);
			return IXML_INSUFFICIENT_MEMORY;
		}
		if (ixmlElement_setAttributeNodeNS(
			    element, newAttr, &newAttr) != IXML_SUCCESS) {
			ixmlAttr_free(newAttr);
			Parser_freeNodeContent(&newAttrNode);
			return IXML_FAILED;
		}
	}
	Parser_freeNodeContent(&newAttrNode);

	return IXML_SUCCESS;
}

int ixmlElement_removeAttributeNS(IXML_Element *element,
	const DOMString namespaceURI,
	const DOMString localName)
{
	IXML_Node *attrNode = NULL;

	if (!element || !namespaceURI || !localName) {
		return IXML_INVALID_PARAMETER;
	}

	attrNode = element->n.firstAttr;
	while (attrNode) {
		if (strcmp(attrNode->localName, localName) == 0 &&
			strcmp(attrNode->namespaceURI, namespaceURI) == 0) {
			/* Found it */
			break;
		} else {
			attrNode = attrNode->nextSibling;
		}
	}
	if (attrNode) {
		/* Has the attribute */
		if (attrNode->nodeValue) {
			free(attrNode->nodeValue);
			attrNode->nodeValue = NULL;
		}
	}

	return IXML_SUCCESS;
}

IXML_Attr *ixmlElement_getAttributeNodeNS(IXML_Element *element,
	const DOMString namespaceURI,
	const DOMString localName)
{
	IXML_Node *attrNode = NULL;

	if (!element || !namespaceURI || !localName) {
		return NULL;
	}

	attrNode = element->n.firstAttr;
	while (attrNode) {
		if (attrNode->localName && attrNode->namespaceURI &&
			strcmp(attrNode->localName, localName) == 0 &&
			strcmp(attrNode->namespaceURI, namespaceURI) == 0) {
			/* found it */
			break;
		}
		attrNode = attrNode->nextSibling;
	}

	return (IXML_Attr *)attrNode;
}

int ixmlElement_setAttributeNodeNS(
	/* IN */ IXML_Element *element,
	/* IN */ IXML_Attr *newAttr,
	/* OUT */ IXML_Attr **rtAttr)
{
	IXML_Node *node = NULL;
	IXML_Node *attrNode = NULL;

	if (!element || !newAttr) {
		return IXML_INVALID_PARAMETER;
	}
	if (newAttr->n.ownerDocument != element->n.ownerDocument) {
		return IXML_WRONG_DOCUMENT_ERR;
	}
	if (newAttr->ownerElement && newAttr->ownerElement != element) {
		return IXML_INUSE_ATTRIBUTE_ERR;
	}

	newAttr->ownerElement = element;
	node = (IXML_Node *)newAttr;
	attrNode = element->n.firstAttr;
	while (attrNode) {
		if (attrNode->localName && node->localName &&
			attrNode->namespaceURI && node->namespaceURI &&
			strcmp(attrNode->localName, node->localName) == 0 &&
			strcmp(attrNode->namespaceURI, node->namespaceURI) ==
				0) {
			/* Found it */
			break;
		}
		attrNode = attrNode->nextSibling;
	}
	if (attrNode) {
		/* already present, will replace by newAttr */
		IXML_Node *preSib = attrNode->prevSibling;
		IXML_Node *nextSib = attrNode->nextSibling;
		if (preSib) {
			preSib->nextSibling = node;
		}
		if (nextSib) {
			nextSib->prevSibling = node;
		}
		if (element->n.firstAttr == attrNode) {
			element->n.firstAttr = node;
		}
		*rtAttr = (IXML_Attr *)attrNode;

	} else {
		/* Add this attribute  */
		if (element->n.firstAttr) {
			/* Element has attribute already */
			IXML_Node *prevAttr = element->n.firstAttr;
			IXML_Node *nextAttr = prevAttr->nextSibling;
			while (nextAttr) {
				prevAttr = nextAttr;
				nextAttr = prevAttr->nextSibling;
			}
			prevAttr->nextSibling = node;
		} else {
			/* This is the first attribute node */
			element->n.firstAttr = node;
			node->prevSibling = NULL;
			node->nextSibling = NULL;
		}
		if (rtAttr) {
			*rtAttr = NULL;
		}
	}

	return IXML_SUCCESS;
}

IXML_NodeList *ixmlElement_getElementsByTagNameNS(IXML_Element *element,
	const DOMString namespaceURI,
	const DOMString localName)
{
	IXML_Node *node = (IXML_Node *)element;
	IXML_NodeList *nodeList = NULL;

	if (element && namespaceURI && localName) {
		ixmlNode_getElementsByTagNameNS(
			node, namespaceURI, localName, &nodeList);
	}

	return nodeList;
}

int ixmlElement_hasAttribute(IXML_Element *element, const DOMString name)
{
	IXML_Node *attrNode = NULL;

	if (!element || !name) {
		return 0;
	}

	attrNode = element->n.firstAttr;
	while (attrNode) {
		if (strcmp(attrNode->nodeName, name) == 0) {
			return 1;
		} else {
			attrNode = attrNode->nextSibling;
		}
	}

	return 0;
}

int ixmlElement_hasAttributeNS(IXML_Element *element,
	const DOMString namespaceURI,
	const DOMString localName)
{
	IXML_Node *attrNode = NULL;

	if (!element || !namespaceURI || !localName) {
		return 0;
	}

	attrNode = element->n.firstAttr;
	while (attrNode) {
		if (strcmp(attrNode->localName, localName) == 0 &&
			strcmp(attrNode->namespaceURI, namespaceURI) == 0) {
			return 1;
		} else {
			attrNode = attrNode->nextSibling;
		}
	}

	return 0;
}

void ixmlElement_free(IXML_Element *element)
{
	if (element) {
		ixmlNode_free((IXML_Node *)element);
	}
}
