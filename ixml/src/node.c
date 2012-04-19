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
#include <stdlib.h> /* for free(), malloc() */
#include <string.h>


void ixmlNode_init(IXML_Node *nodeptr)
{
	assert(nodeptr != NULL);

	memset(nodeptr, 0, sizeof (IXML_Node));
}


void ixmlCDATASection_init(IXML_CDATASection *nodeptr)
{
	memset(nodeptr, 0, sizeof (IXML_CDATASection));
}


void ixmlCDATASection_free(IXML_CDATASection *nodeptr)
{
	if (nodeptr != NULL) {
		ixmlNode_free((IXML_Node *)nodeptr);
	}
}


/*!
 * \brief Frees a node content.
 */
static void ixmlNode_freeSingleNode(
	/*! [in] The node to free. */
	IXML_Node *nodeptr)
{
	IXML_Element *element = NULL;

	if (nodeptr != NULL) {
		if (nodeptr->nodeName != NULL) {
			free(nodeptr->nodeName);
		}
		if (nodeptr->nodeValue != NULL) {
			free(nodeptr->nodeValue);
		}
		if (nodeptr->namespaceURI != NULL) {
			free(nodeptr->namespaceURI);
		}
		if (nodeptr->prefix != NULL) {
			free(nodeptr->prefix);
		}
		if (nodeptr->localName != NULL) {
			free(nodeptr->localName);
		}
		switch (nodeptr->nodeType ) {
		case eELEMENT_NODE:
			element = (IXML_Element *)nodeptr;
			free(element->tagName);
			break;
		default:
			break;
		}
		free(nodeptr);
	}
}


void ixmlNode_free(IXML_Node *nodeptr)
{
	if (nodeptr != NULL) {
		ixmlNode_free(nodeptr->firstChild);
		ixmlNode_free(nodeptr->nextSibling);
		ixmlNode_free(nodeptr->firstAttr);
		ixmlNode_freeSingleNode(nodeptr);
	}
}


const DOMString ixmlNode_getNodeName(IXML_Node *nodeptr)
{
	if(nodeptr != NULL) {
		return nodeptr->nodeName;
	}

	return NULL;
}


const DOMString ixmlNode_getLocalName(IXML_Node *nodeptr)
{
	if (nodeptr != NULL) {
		return nodeptr->localName;
	}

	return NULL;
}


/*!
 * \brief Sets the namespace URI of the node.
 */
static int ixmlNode_setNamespaceURI(
	/*! [in] The \b Node on which to operate. */
	IXML_Node *nodeptr,
	/*! [in] The name space string to set. */
	const char *namespaceURI)
{
	if (nodeptr == NULL) {
		return IXML_INVALID_PARAMETER;
	}

	if (nodeptr->namespaceURI != NULL) {
		free(nodeptr->namespaceURI);
		nodeptr->namespaceURI = NULL;
	}

	if (namespaceURI != NULL) {
		nodeptr->namespaceURI = strdup(namespaceURI);
		if (nodeptr->namespaceURI == NULL) {
			return IXML_INSUFFICIENT_MEMORY;
		}
	}

	return IXML_SUCCESS;
}


/*
 * \brief Set the prefix of the node.
 */
static int ixmlNode_setPrefix(
	/*! [in] The \b Node on which to operate. */
	IXML_Node *nodeptr,
	/*! [in] The prefix string to set. */
	const char *prefix)
{
	if (nodeptr == NULL) {
		return IXML_INVALID_PARAMETER;
	}

	if (nodeptr->prefix != NULL) {
		free(nodeptr->prefix);
		nodeptr->prefix = NULL;
	}

	if (prefix != NULL) {
		nodeptr->prefix = strdup(prefix);
		if(nodeptr->prefix == NULL) {
			return IXML_INSUFFICIENT_MEMORY;
		}
	}

	return IXML_SUCCESS;
}


/*!
 * \brief Set the localName of the node.
 *
 * \return IXML_SUCCESS or failure.
 */
static int ixmlNode_setLocalName(
	/*! [in] The pointer to the node. */
	IXML_Node *nodeptr,
	/*! [in] The local name to set. */
	const char *localName)
{
	assert(nodeptr != NULL);

	if (nodeptr->localName != NULL) {
		free(nodeptr->localName);
		nodeptr->localName = NULL;
	}

	if (localName != NULL) {
		nodeptr->localName = strdup(localName);
		if (nodeptr->localName == NULL) {
			return IXML_INSUFFICIENT_MEMORY;
		}
	}

	return IXML_SUCCESS;
}


const DOMString ixmlNode_getNamespaceURI(IXML_Node *nodeptr)
{
	DOMString retNamespaceURI = NULL;

	if (nodeptr != NULL) {
		retNamespaceURI = nodeptr->namespaceURI;
	}

	return retNamespaceURI;
}


const DOMString ixmlNode_getPrefix(IXML_Node *nodeptr)
{
	const DOMString prefix = NULL;

	if (nodeptr != NULL) {
		prefix = nodeptr->prefix;
	}

	return prefix;
}


const DOMString ixmlNode_getNodeValue(IXML_Node *nodeptr)
{
	if ( nodeptr != NULL ) {
		return nodeptr->nodeValue;
	}

	return NULL;
}


int ixmlNode_setNodeValue(IXML_Node *nodeptr, const char *newNodeValue)
{
	int rc = IXML_SUCCESS;

	if (nodeptr == NULL) {
		return IXML_INVALID_PARAMETER;
	}

	if (nodeptr->nodeValue != NULL) {
		free(nodeptr->nodeValue);
		nodeptr->nodeValue = NULL;
	}

	if (newNodeValue != NULL) {
		nodeptr->nodeValue = strdup(newNodeValue);
		if (nodeptr->nodeValue == NULL) {
			return IXML_INSUFFICIENT_MEMORY;
		}
	}

	return rc;
}


unsigned short ixmlNode_getNodeType(IXML_Node *nodeptr)
{
	if (nodeptr != NULL) {
		return nodeptr->nodeType;
	} else {
		return (unsigned short)eINVALID_NODE;
	}
}


IXML_Node *ixmlNode_getParentNode(IXML_Node *nodeptr)
{
	if (nodeptr != NULL) {
		return nodeptr->parentNode;
	} else {
		return NULL;
	}
}


IXML_Node *ixmlNode_getFirstChild(IXML_Node *nodeptr)
{
	if (nodeptr != NULL) {
		return nodeptr->firstChild;
	} else {
		return NULL;
	}
}


IXML_Node *ixmlNode_getLastChild(IXML_Node *nodeptr)
{
	IXML_Node *prev;
	IXML_Node *next;

	if (nodeptr != NULL) {
		prev = nodeptr;
		next = nodeptr->firstChild;
		while (next != NULL) {
			prev = next;
			next = next->nextSibling;
		}
		return prev;
	} else {
		return NULL;
	}
}


IXML_Node *ixmlNode_getPreviousSibling(IXML_Node *nodeptr)
{
	if (nodeptr != NULL) {
		return nodeptr->prevSibling;
	} else {
		return NULL;
	}
}


IXML_Node *ixmlNode_getNextSibling(IXML_Node *nodeptr)
{
	if (nodeptr != NULL) {
		return nodeptr->nextSibling;
	} else {
		return NULL;
	}
}


IXML_Document *ixmlNode_getOwnerDocument(IXML_Node *nodeptr)
{
	if (nodeptr != NULL) {
		return (IXML_Document *)nodeptr->ownerDocument;
	} else {
		return NULL;
	}
}

/*!
 * \brief Check if ancestorNode is ancestor of toFind.
 *
 * \return TRUE or FALSE.
 */
static BOOL ixmlNode_isAncestor(
	/*! [in] The candidate to ancestor \b Node. */
	IXML_Node *ancestorNode,
	/*! [in] The \b Node to check for an ancestor. */
	IXML_Node *toFind)
{
	BOOL found = FALSE;

	if (ancestorNode != NULL && toFind != NULL) {
		if (toFind->parentNode == ancestorNode) {
			return TRUE;
		} else {
			found = ixmlNode_isAncestor(
				ancestorNode->firstChild, toFind);
			if (found == FALSE) {
				found = ixmlNode_isAncestor(
					ancestorNode->nextSibling, toFind);
			}
		}
	}

	return found;
}

/*!
 * \brief Check whether toFind is a children of nodeptr.
 *
 * \return TRUE or FALSE.
 */
static BOOL ixmlNode_isParent(
	/*! [in] The candidate to parent \b Node. */
	IXML_Node *nodeptr,
	/*! [in] The \b Node to check for his parent. */
	IXML_Node *toFind)
{
	BOOL found = FALSE;

	assert(nodeptr != NULL && toFind != NULL);

	if (nodeptr != NULL && toFind != NULL)
		found = toFind->parentNode == nodeptr;

	return found;
}

/*!
 * \brief Check to see whether nodeptr allows children of type newChild.    
 *
 * \return
 * 	\li TRUE, if nodeptr can have newChild as children.
 * 	\li FALSE, if nodeptr cannot have newChild as children.
 */
static BOOL ixmlNode_allowChildren(
	/*! [in] The \b Node to check. */
	IXML_Node *nodeptr,
	/*! [in] The child \b Node to check. */
	IXML_Node *newChild)
{
	assert(nodeptr != NULL && newChild != NULL);

	switch (nodeptr->nodeType) {
	case eATTRIBUTE_NODE:
	case eTEXT_NODE:
	case eCDATA_SECTION_NODE:
		return FALSE;

	case eELEMENT_NODE:
		switch (newChild->nodeType) {
		case eATTRIBUTE_NODE:
		case eDOCUMENT_NODE:
			return FALSE;
		default:
			break;
		}
	break;

	case eDOCUMENT_NODE:
		switch (newChild->nodeType) {
		case eELEMENT_NODE:
			break;
		default:
			return FALSE;
		}

	default:
		break;
	}

	return TRUE;
}


/*!
 * \brief Compare two nodes to see whether they are the same node.
 * Parent, sibling and children node are ignored.
 *
 * \return
 * 	\li TRUE, the two nodes are the same.
 * 	\li FALSE, the two nodes are not the same.
 */
BOOL ixmlNode_compare(
	/*! [in] The first \b Node. */
	IXML_Node *srcNode,
	/*! [in] The second \b Node. */
 	IXML_Node *destNode)
{
	assert(srcNode != NULL && destNode != NULL);

	return 
		srcNode == destNode ||
	(strcmp(srcNode->nodeName, destNode->nodeName) == 0 &&
	 strcmp(srcNode->nodeValue, destNode->nodeValue) == 0 &&
	 srcNode->nodeType == destNode->nodeType &&
	 strcmp(srcNode->namespaceURI, destNode->namespaceURI) == 0 &&
	 strcmp(srcNode->prefix, destNode->prefix) == 0 &&
	 strcmp(srcNode->localName, destNode->localName) == 0);
}


int ixmlNode_insertBefore(
	IXML_Node *nodeptr,
	IXML_Node *newChild,
	IXML_Node *refChild)
{
	int ret = IXML_SUCCESS;

	if (nodeptr == NULL || newChild == NULL) {
		return IXML_INVALID_PARAMETER;
	}
	/* whether nodeptr allow children of the type of newChild */
	if (ixmlNode_allowChildren(nodeptr, newChild) == FALSE) {
		return IXML_HIERARCHY_REQUEST_ERR;
	}
	/* or if newChild is one of nodeptr's ancestors */
	if (ixmlNode_isAncestor(newChild, nodeptr) == TRUE) {
		return IXML_HIERARCHY_REQUEST_ERR;
	}
	/* if newChild was created from a different document */
	if (nodeptr->ownerDocument != newChild->ownerDocument) {
		return IXML_WRONG_DOCUMENT_ERR;
	}
	/* if refChild is not a child of nodeptr */
	if (ixmlNode_isParent(nodeptr, refChild) == FALSE) {
		return IXML_NOT_FOUND_ERR;
	}

	if (refChild != NULL) {
		if (ixmlNode_isParent(nodeptr, newChild) == TRUE) {
			ixmlNode_removeChild(nodeptr, newChild, &newChild);
			newChild->nextSibling = NULL;
			newChild->prevSibling = NULL;
		}
		newChild->nextSibling = refChild;
		if (refChild->prevSibling != NULL) {
			refChild->prevSibling->nextSibling = newChild;
			newChild->prevSibling = refChild->prevSibling;
		}
		refChild->prevSibling = newChild;
		if (newChild->prevSibling == NULL) {
			nodeptr->firstChild = newChild;
		}
		newChild->parentNode = nodeptr;
	} else {
		ret = ixmlNode_appendChild( nodeptr, newChild );
	}

	return ret;
}


int ixmlNode_replaceChild(
	IXML_Node *nodeptr,
	IXML_Node *newChild,
	IXML_Node *oldChild,
	IXML_Node **returnNode)
{
	int ret = IXML_SUCCESS;

	if (nodeptr == NULL || newChild == NULL || oldChild == NULL) {
		return IXML_INVALID_PARAMETER;
	}
	/* if nodetype of nodeptr does not allow children of the type of newChild
	 * needs to add later or if newChild is one of nodeptr's ancestors */
	if (ixmlNode_isAncestor(newChild, nodeptr) == TRUE) {
		return IXML_HIERARCHY_REQUEST_ERR;
	}

	if (ixmlNode_allowChildren(nodeptr, newChild) == FALSE) {
		return IXML_HIERARCHY_REQUEST_ERR;
	}
	/* if newChild was created from a different document */
	if (nodeptr->ownerDocument != newChild->ownerDocument) {
		return IXML_WRONG_DOCUMENT_ERR;
	}
	/* if refChild is not a child of nodeptr */
	if (ixmlNode_isParent(nodeptr, oldChild) != TRUE) {
		return IXML_NOT_FOUND_ERR;
	}

	ret = ixmlNode_insertBefore(nodeptr, newChild, oldChild);
	if (ret != IXML_SUCCESS) {
		return ret;
	}

	ret = ixmlNode_removeChild(nodeptr, oldChild, returnNode);
	return ret;
}


int ixmlNode_removeChild(
	IXML_Node *nodeptr,
	IXML_Node *oldChild,
	IXML_Node **returnNode)
{
	if (!nodeptr || !oldChild)
		return IXML_INVALID_PARAMETER;
	if (!ixmlNode_isParent(nodeptr, oldChild))
		return IXML_NOT_FOUND_ERR;
	if (oldChild->prevSibling)
		oldChild->prevSibling->nextSibling = oldChild->nextSibling;
	if (nodeptr->firstChild == oldChild)
		nodeptr->firstChild = oldChild->nextSibling;
	if (oldChild->nextSibling)
		oldChild->nextSibling->prevSibling = oldChild->prevSibling;
	oldChild->nextSibling = NULL;
	oldChild->prevSibling = NULL;
	oldChild->parentNode = NULL;
	if (returnNode)
		*returnNode = oldChild;
	else
		ixmlNode_free(oldChild);

	return IXML_SUCCESS;
}


int ixmlNode_appendChild(IXML_Node *nodeptr, IXML_Node *newChild)
{
	IXML_Node *prev = NULL;
	IXML_Node *next = NULL;

	if (nodeptr == NULL || newChild == NULL) {
		return IXML_INVALID_PARAMETER;
	}
	/* if newChild was created from a different document */
	if (newChild->ownerDocument != NULL &&
	    nodeptr->ownerDocument != newChild->ownerDocument) {
		return IXML_WRONG_DOCUMENT_ERR;
	}
	/* if newChild is an ancestor of nodeptr */
	if (ixmlNode_isAncestor(newChild, nodeptr) == TRUE) {
		return IXML_HIERARCHY_REQUEST_ERR;
	}
	/* if nodeptr does not allow to have newChild as children */
	if (ixmlNode_allowChildren(nodeptr, newChild) == FALSE) {
		return IXML_HIERARCHY_REQUEST_ERR;
	}

	if (ixmlNode_isParent(nodeptr, newChild) == TRUE ) {
		ixmlNode_removeChild(nodeptr, newChild, &newChild);
	}
	/* set the parent node pointer */
	newChild->parentNode = nodeptr;
	newChild->ownerDocument = nodeptr->ownerDocument;

	/* if the first child */
	if (nodeptr->firstChild == NULL) {
		nodeptr->firstChild = newChild;
	} else {
		prev = nodeptr->firstChild;
		next = prev->nextSibling;
		while (next != NULL) {
			prev = next;
			next = prev->nextSibling;
		}
		prev->nextSibling = newChild;
		newChild->prevSibling = prev;
	}

	return IXML_SUCCESS;
}

/*!
 * \brief Returns a clone of nodeptr.
 *
 * \return A cloned node of nodeptr.
 */
static IXML_Node *ixmlNode_cloneTextNode(
	/*! [in] The \b Node to clone. */
	IXML_Node *nodeptr)
{
	IXML_Node *newNode = NULL;
	int rc;

	assert(nodeptr != NULL);

	newNode = (IXML_Node *)malloc(sizeof (IXML_Node));
	if (newNode == NULL) {
		return NULL;
	} else {
		ixmlNode_init(newNode);
		rc = ixmlNode_setNodeName(newNode, nodeptr->nodeName);
		if (rc != IXML_SUCCESS) {
			ixmlNode_free(newNode);
			return NULL;
		}
		rc = ixmlNode_setNodeValue(newNode, nodeptr->nodeValue);
		if (rc != IXML_SUCCESS) {
			ixmlNode_free(newNode);
			return NULL;
		}
		newNode->nodeType = eTEXT_NODE;
	}

	return newNode;
}

/*!
 * \brief Return a clone of CDATASection node.
 *
 * \return A clone of CDATASection node.
 */
static IXML_CDATASection *ixmlNode_cloneCDATASect(
	/*! [in] The \b Node to clone. */
	IXML_CDATASection *nodeptr)
{
	IXML_CDATASection *newCDATA = NULL;
	IXML_Node *newNode;
	IXML_Node *srcNode;
	int rc;

	assert(nodeptr != NULL);
	newCDATA = (IXML_CDATASection *)malloc(sizeof (IXML_CDATASection));
	if (newCDATA != NULL) {
		newNode = (IXML_Node *)newCDATA;
		ixmlCDATASection_init(newCDATA);
		srcNode = (IXML_Node *)nodeptr;
		rc = ixmlNode_setNodeName(newNode, srcNode->nodeName);
		if (rc != IXML_SUCCESS) {
			ixmlCDATASection_free(newCDATA);
			return NULL;
		}
		rc = ixmlNode_setNodeValue(newNode, srcNode->nodeValue);
		if (rc != IXML_SUCCESS) {
			ixmlCDATASection_free(newCDATA);
			return NULL;
		}
		newNode->nodeType = eCDATA_SECTION_NODE;
	}

	return newCDATA;
}


/*!
 * \brief Returns a clone of element node.
 *
 * \return A clone of element node.
 */
static IXML_Element *ixmlNode_cloneElement(
	/*! [in] The \b Node to clone. */
	IXML_Element *nodeptr)
{
	IXML_Element *newElement;
	IXML_Node *elementNode;
	IXML_Node *srcNode;
	int rc;

	assert(nodeptr != NULL);

	newElement = (IXML_Element *)malloc(sizeof (IXML_Element));
	if (newElement == NULL) {
		return NULL;
	}

	ixmlElement_init(newElement);
	rc = ixmlElement_setTagName(newElement, nodeptr->tagName);
	if (rc != IXML_SUCCESS) {
		ixmlElement_free(newElement);
		return NULL;
	}

	elementNode = (IXML_Node *)newElement;
	srcNode = (IXML_Node *)nodeptr;
	rc = ixmlNode_setNodeName(elementNode, srcNode->nodeName);
	if (rc != IXML_SUCCESS) {
		ixmlElement_free(newElement);
		return NULL;
	}

	rc = ixmlNode_setNodeValue(elementNode, srcNode->nodeValue);
	if (rc != IXML_SUCCESS) {
		ixmlElement_free(newElement);
		return NULL;
	}

	rc = ixmlNode_setNamespaceURI(elementNode, srcNode->namespaceURI);
	if (rc != IXML_SUCCESS) {
		ixmlElement_free(newElement);
		return NULL;
	}

	rc = ixmlNode_setPrefix(elementNode, srcNode->prefix);
	if (rc != IXML_SUCCESS) {
		ixmlElement_free(newElement);
		return NULL;
	}

	rc = ixmlNode_setLocalName(elementNode, srcNode->localName);
	if (rc != IXML_SUCCESS) {
		ixmlElement_free(newElement);
		return NULL;
	}

	elementNode->nodeType = eELEMENT_NODE;

	return newElement;
}


/*!
 * \brief Returns a new document node.
 *
 * Currently, the IXML_Document struct is just a node, so this function
 * just mallocs the IXML_Document, sets the node type and name.
 *
 * \return A new document node.
 */
static IXML_Document *ixmlNode_newDoc(void)
{
	IXML_Document *newDoc;
	IXML_Node *docNode;
	int rc;

	newDoc = (IXML_Document *)malloc(sizeof (IXML_Document));
	if (!newDoc)
		return NULL;
	ixmlDocument_init(newDoc);
	docNode = (IXML_Node *)newDoc;
	rc = ixmlNode_setNodeName(docNode, DOCUMENTNODENAME);
	if (rc != IXML_SUCCESS) {
		ixmlDocument_free(newDoc);
		return NULL;
	}
	newDoc->n.nodeType = eDOCUMENT_NODE;

	return newDoc;
}

/*!
 * \brief Returns a clone of an attribute node.
 *
 * \return A clone of an attribute node.
 */
static IXML_Attr *ixmlNode_cloneAttr(
	/*! [in] The \b Node to clone. */
	IXML_Attr *nodeptr)
{
	IXML_Attr *newAttr;
	IXML_Node *attrNode;
	IXML_Node *srcNode;
	int rc;

	assert(nodeptr != NULL);

	newAttr = (IXML_Attr *)malloc(sizeof (IXML_Attr));
	if (newAttr == NULL) {
		return NULL;
	}

	ixmlAttr_init(newAttr);
	attrNode = (IXML_Node *)newAttr;
	srcNode = (IXML_Node *)nodeptr;

	rc = ixmlNode_setNodeName(attrNode, srcNode->nodeName);
	if (rc != IXML_SUCCESS) {
		ixmlAttr_free(newAttr);
		return NULL;
	}

	rc = ixmlNode_setNodeValue(attrNode, srcNode->nodeValue);
	if (rc != IXML_SUCCESS) {
		ixmlAttr_free(newAttr);
		return NULL;
	}

	/* Check to see whether we need to split prefix and localname for attribute */
	rc = ixmlNode_setNamespaceURI(attrNode, srcNode->namespaceURI);
	if (rc != IXML_SUCCESS) {
		ixmlAttr_free(newAttr);
		return NULL;
	}

	rc = ixmlNode_setPrefix(attrNode, srcNode->prefix);
	if (rc != IXML_SUCCESS) {
		ixmlAttr_free(newAttr);
		return NULL;
	}

	rc = ixmlNode_setLocalName(attrNode, srcNode->localName);
	if (rc != IXML_SUCCESS) {
		ixmlAttr_free(newAttr);
		return NULL;
	}

	attrNode->nodeType = eATTRIBUTE_NODE;

	return newAttr;
}

/*!
 * \brief Return a clone of attribute node, with specified field set to TRUE.
 *
 * \return A clone of attribute node, with specified field set to TRUE.
 */
static IXML_Attr *ixmlNode_cloneAttrDirect(
	/*! [in] The \b Node to clone. */
	IXML_Attr *nodeptr)
{
	IXML_Attr *newAttr;

	assert(nodeptr != NULL);

	newAttr = ixmlNode_cloneAttr(nodeptr);
	if (newAttr != NULL) {
		newAttr->specified = TRUE;
	}

	return newAttr;
}


/*!
 * \brief Sets siblings nodes parent to be the same as this node's.
 */
static void ixmlNode_setSiblingNodesParent(
	/*! [in] The node to operate on. */
	IXML_Node *nodeptr)
{
	IXML_Node *parentNode = nodeptr->parentNode;
	IXML_Node *nextptr = nodeptr->nextSibling;

	while (nextptr != NULL) {
		nextptr->parentNode = parentNode;
		nextptr = nextptr->nextSibling;
	}
}


/*!
 * \brief Recursive function that clones a node tree of nodeptr.
 *
 * \returns The cloned node/tree.
 */
static IXML_Node *ixmlNode_cloneNodeTreeRecursive(
	/*! [in] Node tree to clone. */
	IXML_Node *nodeptr,
	/*! [in] TRUE if you want to clone the tree. */
	BOOL deep)
{
	IXML_Node *newNode = NULL;
	IXML_Element *newElement = NULL;
	IXML_Attr *newAttr = NULL;
	IXML_CDATASection *newCDATA = NULL;
	IXML_Document *newDoc = NULL;
	IXML_Node *nextSib = NULL;

	if (nodeptr != NULL) {
		switch (nodeptr->nodeType) {
		case eELEMENT_NODE:
			newElement = ixmlNode_cloneElement((IXML_Element *)nodeptr);
			if (newElement == NULL)
				return NULL;
			newElement->n.firstAttr = ixmlNode_cloneNodeTreeRecursive(
				nodeptr->firstAttr, deep);
			if (deep) {
				newElement->n.firstChild =
					ixmlNode_cloneNodeTreeRecursive(nodeptr->firstChild, deep);
				if (newElement->n.firstChild != NULL) {
					newElement->n.firstChild->parentNode = (IXML_Node *)newElement;
					ixmlNode_setSiblingNodesParent(newElement->n.firstChild);
				}
				nextSib = ixmlNode_cloneNodeTreeRecursive(nodeptr->nextSibling, deep);
				newElement->n.nextSibling = nextSib;
				if (nextSib != NULL) {
					nextSib->prevSibling = (IXML_Node *)newElement;
				}
			}
			newNode = (IXML_Node *)newElement;
			break;

		case eATTRIBUTE_NODE:
			newAttr = ixmlNode_cloneAttr((IXML_Attr *)nodeptr);
			if (newAttr == NULL)
				return NULL;
			nextSib = ixmlNode_cloneNodeTreeRecursive(nodeptr->nextSibling, deep);
			newAttr->n.nextSibling = nextSib;
			if (nextSib != NULL) {
				nextSib->prevSibling = (IXML_Node *)newAttr;
			}
			newNode = (IXML_Node *)newAttr;
			break;

		case eTEXT_NODE:
			newNode = ixmlNode_cloneTextNode(nodeptr);
			break;

		case eCDATA_SECTION_NODE:
			newCDATA = ixmlNode_cloneCDATASect((IXML_CDATASection *)nodeptr);
			newNode = (IXML_Node *)newCDATA;
			break;

		case eDOCUMENT_NODE:
			newDoc = ixmlNode_newDoc();
			if (newDoc == NULL)
				return NULL;
			newNode = (IXML_Node *)newDoc;
			if (deep) {
				newNode->firstChild = ixmlNode_cloneNodeTreeRecursive(
					nodeptr->firstChild, deep);
				if (newNode->firstChild != NULL) {
					newNode->firstChild->parentNode = newNode;
				}
			}
			break;

		case eINVALID_NODE:
		case eENTITY_REFERENCE_NODE:
		case eENTITY_NODE:
		case ePROCESSING_INSTRUCTION_NODE:
		case eCOMMENT_NODE:
		case eDOCUMENT_TYPE_NODE:
		case eDOCUMENT_FRAGMENT_NODE:
		case eNOTATION_NODE:
			break;
		}
	}

	return newNode;
}


/*!
 * \brief Function that clones a node tree of nodeptr.
 *
 * \returns The cloned node/tree.
 */
static IXML_Node *ixmlNode_cloneNodeTree(
	/*! [in] Node tree to clone. */
	IXML_Node *nodeptr,
	/*! [in] TRUE if you want to clone the tree. */
	BOOL deep)
{
	IXML_Node *newNode = NULL;
	IXML_Element *newElement;
	IXML_Node *childNode;

	assert(nodeptr != NULL);

	switch (nodeptr->nodeType) {
	case eELEMENT_NODE:
		newElement = ixmlNode_cloneElement((IXML_Element *)nodeptr);
		if (newElement == NULL)
			return NULL;
		newElement->n.firstAttr = ixmlNode_cloneNodeTreeRecursive(nodeptr->firstAttr, deep);
		if (deep) {
			newElement->n.firstChild = ixmlNode_cloneNodeTreeRecursive(
				nodeptr->firstChild, deep);
			childNode = newElement->n.firstChild;
			while (childNode != NULL) {
				childNode->parentNode = (IXML_Node *)newElement;
				childNode = childNode->nextSibling;
			}
			newElement->n.nextSibling = NULL;
		}
		newNode = ( IXML_Node * ) newElement;
		break;

	case eATTRIBUTE_NODE:
	case eTEXT_NODE:
	case eCDATA_SECTION_NODE:
	case eDOCUMENT_NODE:
		newNode = ixmlNode_cloneNodeTreeRecursive(nodeptr, deep);
		break;

	case eINVALID_NODE:
	case eENTITY_REFERENCE_NODE:
	case eENTITY_NODE:
	case ePROCESSING_INSTRUCTION_NODE:
	case eCOMMENT_NODE:
	case eDOCUMENT_TYPE_NODE:
	case eDOCUMENT_FRAGMENT_NODE:
	case eNOTATION_NODE:
#if 0
		/* create a new node here? */
		newNode = (IXML_Node *)malloc(sizeof(IXML_Node));
		if (newNode == NULL) {
			return NULL;
		}
#endif
		break;
	}

	/* by spec, the duplicate node has no parent */
	if (newNode != NULL)
		newNode->parentNode = NULL;

	return newNode;
}



IXML_Node *ixmlNode_cloneNode(IXML_Node *nodeptr, BOOL deep)
{
	IXML_Node *newNode;
	IXML_Attr *newAttrNode;

	if (nodeptr == NULL) {
		return NULL;
	}

	switch (nodeptr->nodeType) {
	case eATTRIBUTE_NODE:
		newAttrNode = ixmlNode_cloneAttrDirect((IXML_Attr *)nodeptr);
		return (IXML_Node *)newAttrNode;
		break;

	default:
		newNode = ixmlNode_cloneNodeTree(nodeptr, deep);
		return newNode;
		break;
	}
}


IXML_NodeList *ixmlNode_getChildNodes(IXML_Node *nodeptr)
{
	IXML_Node *tempNode;
	IXML_NodeList *newNodeList;
	int rc;

	if (nodeptr == NULL) {
		return NULL;
	}

	newNodeList = (IXML_NodeList *)malloc(sizeof(IXML_NodeList));
	if (newNodeList == NULL) {
		return NULL;
	}

	ixmlNodeList_init(newNodeList);
	tempNode = nodeptr->firstChild;
	while (tempNode != NULL) {
		rc = ixmlNodeList_addToNodeList(&newNodeList, tempNode);
		if (rc != IXML_SUCCESS) {
			ixmlNodeList_free(newNodeList);
			return NULL;
		}

		tempNode = tempNode->nextSibling;
	}

	return newNodeList;
}


IXML_NamedNodeMap *ixmlNode_getAttributes(IXML_Node *nodeptr)
{
	IXML_NamedNodeMap *returnNamedNodeMap = NULL;
	IXML_Node *tempNode;
	int rc;

	if(nodeptr == NULL) {
		return NULL;
	}

	switch(nodeptr->nodeType) {
	case eELEMENT_NODE:
		returnNamedNodeMap = (IXML_NamedNodeMap *)malloc(sizeof(IXML_NamedNodeMap));
		if(returnNamedNodeMap == NULL) {
			return NULL;
		}

		ixmlNamedNodeMap_init(returnNamedNodeMap);
		tempNode = nodeptr->firstAttr;
		while( tempNode != NULL ) {
			rc = ixmlNamedNodeMap_addToNamedNodeMap(&returnNamedNodeMap, tempNode);
			if(rc != IXML_SUCCESS) {
				ixmlNamedNodeMap_free(returnNamedNodeMap);
				return NULL;
			}

			tempNode = tempNode->nextSibling;
		}
		return returnNamedNodeMap;
	default:
		/* if not an ELEMENT_NODE */
		return NULL;
	}
}


BOOL ixmlNode_hasChildNodes(IXML_Node *nodeptr)
{
	if (nodeptr == NULL) {
		return FALSE;
	}

	return nodeptr->firstChild != NULL;
}


BOOL ixmlNode_hasAttributes(IXML_Node *nodeptr)
{
	if (nodeptr != NULL) {
		switch (nodeptr->nodeType) {
		case eELEMENT_NODE:
			if (nodeptr->firstAttr != NULL)
				return TRUE;
			break;
		default:
			break;
		}
	}

	return FALSE;
}


/*!
 * \brief Recursively traverse the whole tree, search for element with the
 * given tagname.
 */
static void ixmlNode_getElementsByTagNameRecursive(
	/*! [in] The \b Node tree. */
	IXML_Node *n,
	/*! [in] The tag name to match. */
	const char *tagname,
	/*! [out] The output \b NodeList. */
	IXML_NodeList **list)
{
	const char *name;

	if (n != NULL) {
		if (ixmlNode_getNodeType(n) == eELEMENT_NODE) {
			name = ixmlNode_getNodeName(n);
			if (strcmp(tagname, name) == 0 || strcmp(tagname, "*") == 0) {
				ixmlNodeList_addToNodeList(list, n);
			}
		}
		ixmlNode_getElementsByTagNameRecursive(ixmlNode_getFirstChild(n), tagname, list);
		ixmlNode_getElementsByTagNameRecursive(ixmlNode_getNextSibling(n), tagname, list);
	}
}


void ixmlNode_getElementsByTagName(
	IXML_Node *n,
	const char *tagname,
	IXML_NodeList **list)
{
	const char *name;

	assert(n != NULL && tagname != NULL);

	if (ixmlNode_getNodeType(n) == eELEMENT_NODE) {
		name = ixmlNode_getNodeName(n);
		if (strcmp(tagname, name) == 0 || strcmp(tagname, "*") == 0) {
			ixmlNodeList_addToNodeList(list, n);
		}
	}
	ixmlNode_getElementsByTagNameRecursive(ixmlNode_getFirstChild(n), tagname, list);
}


/*!
 * \brief 
 */
static void ixmlNode_getElementsByTagNameNSRecursive(
	/*! [in] . */
	IXML_Node *n,
	/*! [in] . */
	const char *namespaceURI,
	/*! [in] . */
	const char *localName,
	/*! [out] . */
	IXML_NodeList **list)
{
	const DOMString nsURI;
	const DOMString name;

	if (n != NULL) {
		if (ixmlNode_getNodeType(n) == eELEMENT_NODE) {
			name = ixmlNode_getLocalName(n);
			nsURI = ixmlNode_getNamespaceURI(n);

			if (name != NULL && nsURI != NULL &&
			    (strcmp(namespaceURI, nsURI) == 0 ||
			     strcmp(namespaceURI, "*") == 0 ) &&
			    (strcmp(name, localName) == 0 ||
			     strcmp(localName, "*") == 0)) {
				ixmlNodeList_addToNodeList(list, n);
			}
		}
		ixmlNode_getElementsByTagNameNSRecursive(
			ixmlNode_getFirstChild(n), namespaceURI, localName, list);
		ixmlNode_getElementsByTagNameNSRecursive(
			ixmlNode_getNextSibling(n), namespaceURI, localName, list);
	}
}


void ixmlNode_getElementsByTagNameNS(
	IXML_Node *n,
	const char *namespaceURI,
	const char *localName,
	IXML_NodeList **list)
{
	const DOMString nsURI;
	const DOMString name;

	assert(n != NULL && namespaceURI != NULL && localName != NULL);

	if (ixmlNode_getNodeType(n) == eELEMENT_NODE) {
		name = ixmlNode_getLocalName(n);
		nsURI = ixmlNode_getNamespaceURI(n);
		if (name != NULL && nsURI != NULL &&
		    (strcmp(namespaceURI, nsURI) == 0 ||
		     strcmp(namespaceURI, "*") == 0) &&
		    (strcmp(name, localName) == 0 ||
		     strcmp(localName, "*") == 0)) {
			ixmlNodeList_addToNodeList(list, n);
		}
	}

	ixmlNode_getElementsByTagNameNSRecursive(
		ixmlNode_getFirstChild(n), namespaceURI, localName, list);
}


int ixmlNode_setNodeName(
	IXML_Node *node,
	const DOMString qualifiedName)
{
	int rc = IXML_SUCCESS;

	assert( node != NULL );

	if (node->nodeName != NULL) {
		free(node->nodeName);
		node->nodeName = NULL;
	}

	if (qualifiedName != NULL) {
		/* set the name part */
		node->nodeName = strdup(qualifiedName);
		if (node->nodeName == NULL) {
			return IXML_INSUFFICIENT_MEMORY;
		}

		rc = Parser_setNodePrefixAndLocalName(node);
		if (rc != IXML_SUCCESS) {
			free(node->nodeName);
		}
	}

	return rc;
}


int ixmlNode_setNodeProperties(
	IXML_Node *destNode,
	IXML_Node *src)
{
	int rc;

	assert(destNode != NULL && src != NULL);
	if(destNode == NULL || src == NULL) {
		return IXML_INVALID_PARAMETER;
	}

	rc = ixmlNode_setNodeValue(destNode, src->nodeValue);
	if(rc != IXML_SUCCESS) {
		goto ErrorHandler;
	}

	rc = ixmlNode_setLocalName(destNode, src->localName);
	if(rc != IXML_SUCCESS) {
		goto ErrorHandler;
	}

	rc = ixmlNode_setPrefix(destNode, src->prefix);
	if(rc != IXML_SUCCESS) {
		goto ErrorHandler;
	}
	/* set nodetype */
	destNode->nodeType = src->nodeType;

	return IXML_SUCCESS;

ErrorHandler:
	if(destNode->nodeName != NULL) {
		free(destNode->nodeName);
		destNode->nodeName = NULL;
	}
	if(destNode->nodeValue != NULL) {
		free(destNode->nodeValue);
		destNode->nodeValue = NULL;
	}
	if(destNode->localName != NULL) {
		free(destNode->localName);
		destNode->localName = NULL;
	}

	return IXML_INSUFFICIENT_MEMORY;
}

