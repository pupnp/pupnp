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

#include "ixml.h"
#include "ixmlparser.h"

#include <stdlib.h> /* for free() */
#include <string.h>

#include "posix_overwrites.h" // IWYU pragma: keep

typedef struct s_extra_parameters
{
	const void *p1;
	const void *p2;
} Extra_Parameters_t;

typedef int (*Find_Condition_t)(
	const IXML_Node *n, const Extra_Parameters_t *p);

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

static int find_condition_name(const IXML_Node *n, const Extra_Parameters_t *p)
{
	const DOMString name;
	int ret;

	name = (const DOMString)p->p1;
	ret = n->nodeName && //
	      strcmp(n->nodeName, name) == 0;

	return ret;
}

static IXML_Node *get_attribute_node( //
	Find_Condition_t condition,
	IXML_Element *element,
	Extra_Parameters_t *p)
{
	IXML_Node *n = element->n.firstAttr;

	while (n) {
		if (condition(n, p)) {
			return n;
		}
		n = n->nextSibling;
	}

	return NULL;
}

const DOMString ixmlElement_getAttribute(
	IXML_Element *element, const DOMString name)
{
	IXML_Node *attrNode;
	Extra_Parameters_t p;

	if (!element || !name) {
		return NULL;
	}
	p.p1 = name;
	p.p2 = NULL;
	attrNode = get_attribute_node(find_condition_name, element, &p);
	if (attrNode) {
		return attrNode->nodeValue;
	}

	return NULL;
}

int ixmlElement_setAttribute( //
	IXML_Element *element,
	const DOMString name,
	const DOMString value)
{
	int error_code = IXML_SUCCESS;
	IXML_Node *attrNode = NULL;
	Extra_Parameters_t p;

	if (!element || !name || !value) {
		return IXML_INVALID_PARAMETER;
	}
	if (Parser_isValidXmlName(name) == 0) {
		return IXML_INVALID_CHARACTER_ERR;
	}
	p.p1 = name;
	p.p2 = NULL;
	attrNode = get_attribute_node(find_condition_name, element, &p);
	if (attrNode) {
		if (attrNode->nodeValue) {
			/* Attribute name has a value already */
			free(attrNode->nodeValue);
		}
		attrNode->nodeValue = strdup(value);
		if (!attrNode->nodeValue) {
			error_code = IXML_INSUFFICIENT_MEMORY;
			goto ErrorHandler;
		}
	} else {
		/* attrNode was not found */
		IXML_Attr *newAttr;

		/* Add a new attribute */
		error_code = ixmlDocument_createAttributeEx(
			(IXML_Document *)element->n.ownerDocument,
			name,
			&newAttr);
		if (error_code != IXML_SUCCESS) {
			goto ErrorHandler;
		}
		newAttr->n.nodeValue = strdup(value);
		if (!newAttr->n.nodeValue) {
			ixmlAttr_free(newAttr);
			error_code = IXML_INSUFFICIENT_MEMORY;
			goto ErrorHandler;
		}
		error_code = ixmlElement_setAttributeNode( //
			element,
			newAttr,
			NULL);
		if (error_code != IXML_SUCCESS) {
			ixmlAttr_free(newAttr);
			goto ErrorHandler;
		}
	}

ErrorHandler:
	return error_code;
}

int ixmlElement_removeAttribute(IXML_Element *element, const DOMString name)
{
	IXML_Node *attrNode;
	Extra_Parameters_t p;

	if (!element || !name) {
		return IXML_INVALID_PARAMETER;
	}
	p.p1 = name;
	p.p2 = NULL;
	attrNode = get_attribute_node(find_condition_name, element, &p);
	if (attrNode && attrNode->nodeValue) {
		/* Has the attribute */
		free(attrNode->nodeValue);
		attrNode->nodeValue = NULL;
	}

	return IXML_SUCCESS;
}

IXML_Attr *ixmlElement_getAttributeNode(
	IXML_Element *element, const DOMString name)
{
	Extra_Parameters_t p;

	if (!element || !name) {
		return NULL;
	}
	p.p1 = name;
	p.p2 = NULL;
	return (IXML_Attr *)get_attribute_node(
		find_condition_name, element, &p);
}

static int ixmlElement_setAttributeNode_common(
	/* IN */ Find_Condition_t find_condition,
	/* IN */ IXML_Element *element,
	/* IN */ IXML_Attr *newAttr,
	/* OUT */ IXML_Attr **rtAttr)
{
	IXML_Node *node = NULL;
	IXML_Node *attrNode = NULL;
	Extra_Parameters_t p;

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
	node = &newAttr->n;
	p.p1 = node;
	p.p2 = NULL;
	attrNode = get_attribute_node(find_condition, element, &p);
	if (attrNode) {
		/* Already present, will be replaced by newAttr */
		IXML_Node *prevSib = attrNode->prevSibling;
		IXML_Node *nextSib = attrNode->nextSibling;
		if (prevSib) {
			prevSib->nextSibling = node;
		}
		if (nextSib) {
			nextSib->prevSibling = node;
		}
		if (element->n.firstAttr == attrNode) {
			element->n.firstAttr = node;
		}
		node->parentNode = attrNode->parentNode;
		node->firstChild = attrNode->firstChild; // Should be NULL
		node->firstAttr = attrNode->firstAttr;	 // Should be NULL
		if (rtAttr) {
			attrNode->parentNode = NULL;
			*rtAttr = (IXML_Attr *)attrNode;
		} else {
			ixmlAttr_free((IXML_Attr *)attrNode);
		}
	} else {
		/* Add this attribute */
		if (element->n.firstAttr) {
			/* Element already has an attribute, go to the end */
			IXML_Node *prevAttr = element->n.firstAttr;
			IXML_Node *nextAttr = prevAttr->nextSibling;
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
			node->parentNode = &element->n;
		}
		if (rtAttr) {
			*rtAttr = NULL;
		}
	}

	return IXML_SUCCESS;
}

static int find_condition_node_name(
	const IXML_Node *n1, const Extra_Parameters_t *p)
{
	const IXML_Node *n2 = (const IXML_Node *)p->p1;
	int ret;

	ret = n1->nodeName && //
	      n2->nodeName && //
	      !strcmp(n1->nodeName, n2->nodeName);

	return ret;
}

int ixmlElement_setAttributeNode(
	/* IN */ IXML_Element *element,
	/* IN */ IXML_Attr *newAttr,
	/* OUT */ IXML_Attr **rtAttr)
{
	return ixmlElement_setAttributeNode_common(
		find_condition_node_name, element, newAttr, rtAttr);
}

static int find_condition_node(const IXML_Node *n1, const Extra_Parameters_t *p)
{
	const IXML_Node *n2 = (const IXML_Node *)p->p1;

	return ixmlNode_compare(n1, n2);
}

int ixmlElement_removeAttributeNode(
	IXML_Element *element, IXML_Attr *oldAttr, IXML_Attr **rtAttr)
{
	IXML_Node *attrNode;
	Extra_Parameters_t p;

	if (!element || !oldAttr) {
		return IXML_INVALID_PARAMETER;
	}
	p.p1 = oldAttr;
	p.p2 = NULL;
	attrNode = get_attribute_node(find_condition_node, element, &p);
	if (attrNode) {
		/* Has the attribute */
		IXML_Node *prevSib = attrNode->prevSibling;
		IXML_Node *nextSib = attrNode->nextSibling;
		if (prevSib) {
			prevSib->nextSibling = nextSib;
		}
		if (nextSib) {
			nextSib->prevSibling = prevSib;
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

static int find_condition_ns_ln(const IXML_Node *n, const Extra_Parameters_t *p)
{
	const DOMString namespaceURI = (const DOMString)p->p1;
	const DOMString localName = (const DOMString)p->p2;
	int ret;

	ret = n->localName &&			      //
	      n->namespaceURI &&		      //
	      strcmp(n->localName, localName) == 0 && //
	      strcmp(n->namespaceURI, namespaceURI) == 0;

	return ret;
}

const DOMString ixmlElement_getAttributeNS(
	/* IN */ IXML_Element *element,
	/* IN */ const DOMString namespaceURI,
	/* IN */ const DOMString localName)
{
	IXML_Node *n;
	Extra_Parameters_t p;

	if (!element || !namespaceURI || !localName) {
		return NULL;
	}
	p.p1 = namespaceURI;
	p.p2 = localName;
	n = get_attribute_node(find_condition_ns_ln, element, &p);
	if (n) {
		return n->nodeValue;
	}

	return NULL;
}

static int check_namespace_error(IXML_Node *n,
	const DOMString namespaceURI,
	const DOMString qualifiedName)
{
	/* see DOM 2 spec page 59 */
	/* clang-format off */
	return //
		(n->prefix &&
		 !namespaceURI)
		||
		(n->prefix &&
		 strcmp(n->prefix, "xml") == 0 &&
		 strcmp(namespaceURI, "http://www.w3.org/XML/1998/namespace") != 0)
		||
		(strcmp(qualifiedName, "xmlns") == 0 &&
		 strcmp(namespaceURI, "http://www.w3.org/2000/xmlns/") != 0);
	/* clang-format on */
}

int ixmlElement_setAttributeNS( //
	IXML_Element *element,
	const DOMString namespaceURI,
	const DOMString qualifiedName,
	const DOMString value)
{
	int error_code = IXML_SUCCESS;
	IXML_Node *attrNode = NULL;
	IXML_Node newAttrNode;
	int namespace_error;
	Extra_Parameters_t p;

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
	error_code = Parser_setNodePrefixAndLocalName(&newAttrNode);
	if (error_code != IXML_SUCCESS) {
		goto ErrorHandler;
	}
	namespace_error = check_namespace_error(
		&newAttrNode, namespaceURI, qualifiedName);
	if (namespace_error) {
		error_code = IXML_NAMESPACE_ERR;
		goto ErrorHandler;
	}
	p.p1 = namespaceURI;
	p.p2 = newAttrNode.localName;
	attrNode = get_attribute_node(find_condition_ns_ln, element, &p);
	if (attrNode) {
		if (attrNode->prefix) {
			/* Remove the old prefix */
			free(attrNode->prefix);
		}
		/* replace it with the new prefix */
		if (newAttrNode.prefix) {
			attrNode->prefix = strdup(newAttrNode.prefix);
			if (!attrNode->prefix) {
				error_code = IXML_INSUFFICIENT_MEMORY;
				goto ErrorHandler;
			}
		} else {
			attrNode->prefix = newAttrNode.prefix;
		}
		if (attrNode->nodeValue) {
			/* Attribute name has a value already */
			free(attrNode->nodeValue);
		}
		attrNode->nodeValue = strdup(value);
		if (!attrNode->nodeValue) {
			free(attrNode->prefix);
			error_code = IXML_INSUFFICIENT_MEMORY;
			goto ErrorHandler;
		}
	} else {
		/* attrNode was not found */
		IXML_Attr *newAttr;

		/* Add a new attribute */
		error_code = ixmlDocument_createAttributeNSEx(
			(IXML_Document *)element->n.ownerDocument,
			namespaceURI,
			qualifiedName,
			&newAttr);
		if (error_code != IXML_SUCCESS) {
			goto ErrorHandler;
		}
		newAttr->n.nodeValue = strdup(value);
		if (!newAttr->n.nodeValue) {
			ixmlAttr_free(newAttr);
			error_code = IXML_INSUFFICIENT_MEMORY;
			goto ErrorHandler;
		}
		error_code = ixmlElement_setAttributeNodeNS( //
			element,
			newAttr,
			NULL);
		if (error_code != IXML_SUCCESS) {
			ixmlAttr_free(newAttr);
			goto ErrorHandler;
		}
	}

ErrorHandler:
	Parser_freeNodeContent(&newAttrNode);

	return error_code;
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
		if (attrNode->localName &&
			strcmp(attrNode->localName, localName) == 0 &&
			strcmp(attrNode->namespaceURI, namespaceURI) == 0) {
			/* Found it */
			break;
		}
		attrNode = attrNode->nextSibling;
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
	Extra_Parameters_t p;

	if (!element || !namespaceURI || !localName) {
		return NULL;
	}
	p.p1 = namespaceURI;
	p.p2 = localName;
	return (IXML_Attr *)get_attribute_node(
		find_condition_ns_ln, element, &p);
}

int ixmlElement_setAttributeNodeNS(
	/* IN */ IXML_Element *element,
	/* IN */ IXML_Attr *newAttr,
	/* OUT */ IXML_Attr **rtAttr)
{
	return ixmlElement_setAttributeNode_common(
		find_condition_ns_ln, element, newAttr, rtAttr);
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
		if (attrNode->nodeName &&
			strcmp(attrNode->nodeName, name) == 0) {
			return 1;
		}
		attrNode = attrNode->nextSibling;
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
		if (attrNode->localName && attrNode->namespaceURI &&
			strcmp(attrNode->localName, localName) == 0 &&
			strcmp(attrNode->namespaceURI, namespaceURI) == 0) {
			return 1;
		}
		attrNode = attrNode->nextSibling;
	}

	return 0;
}

void ixmlElement_free(IXML_Element *element)
{
	if (element) {
		ixmlNode_free((IXML_Node *)element);
	}
}
