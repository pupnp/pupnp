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

/*!
 * \file
 */

#include "ixml.h"
#include "ixmldebug.h"
#include "ixmlmembuf.h"
#include "ixmlparser.h"

#include <stdlib.h> /* for free() */
#include <string.h>

#include "posix_overwrites.h" // IWYU pragma: keep

/*!
 * \brief Appends a string to a buffer, substituting some characters by escape
 * sequences.
 */
static void copy_with_escape(
	/*! [in,out] The input/output buffer. */
	ixml_membuf *buf,
	/*! [in] The string to copy from. */
	const char *p)
{
	size_t i;
	size_t plen;

	if (!p) {
		return;
	}
	plen = strlen(p);
	for (i = (size_t)0; i < plen; ++i) {
		switch (p[i]) {
		case '<':
			ixml_membuf_append_str(buf, "&lt;");
			break;
		case '>':
			ixml_membuf_append_str(buf, "&gt;");
			break;
		case '&':
			ixml_membuf_append_str(buf, "&amp;");
			break;
		case '\'':
			ixml_membuf_append_str(buf, "&apos;");
			break;
		case '\"':
			ixml_membuf_append_str(buf, "&quot;");
			break;
		default:
			ixml_membuf_append(buf, &p[i]);
			break;
		}
	}
}

/*! \brief Emit the closing tag for an element node. */
static void ixmlPrintCloseTag(IXML_Node *node, ixml_membuf *buf)
{
	IXML_Node *sib = node->nextSibling;

	ixml_membuf_append_str(buf, "</");
	ixml_membuf_append_str(buf, ixmlNode_getNodeName(node));
	if (sib && ixmlNode_getNodeType(sib) == eTEXT_NODE)
		ixml_membuf_append_str(buf, ">");
	else
		ixml_membuf_append_str(buf, ">\r\n");
}

/*!
 * \brief Iterative function to print all the nodes in a tree.
 * Internal to parser only.
 */
static void ixmlPrintDomTreeRecursive(
	/*! [in] \todo documentation. */
	IXML_Node *nodeptr,
	/*! [in] \todo documentation. */
	ixml_membuf *buf)
{
	IXML_Node *node;
	IXML_Node *fence;
	IXML_Node *next_child;
	int ascending;

	if (!nodeptr)
		return;

	/* Attribute chains traverse nextSibling only — handle iteratively. */
	if (nodeptr->nodeType == eATTRIBUTE_NODE) {
		IXML_Node *attr = nodeptr;
		while (attr) {
			ixml_membuf_append_str(buf, ixmlNode_getNodeName(attr));
			ixml_membuf_append_str(buf, "=\"");
			copy_with_escape(buf, ixmlNode_getNodeValue(attr));
			ixml_membuf_append_str(buf, "\"");
			if (attr->nextSibling)
				ixml_membuf_append_str(buf, " ");
			attr = attr->nextSibling;
		}
		return;
	}

	fence = nodeptr->parentNode;
	node = nodeptr;
	ascending = 0;

	while (node && node != fence) {
		if (ascending) {
			/* Returning from a child subtree: emit closing tag. */
			if (ixmlNode_getNodeType(node) == eELEMENT_NODE)
				ixmlPrintCloseTag(node, buf);
			if (node->nextSibling) {
				node = node->nextSibling;
				ascending = 0;
			} else {
				node = node->parentNode;
			}
			continue;
		}

		/* First visit (descending). */
		next_child = NULL;

		switch (ixmlNode_getNodeType(node)) {
		case eTEXT_NODE:
			copy_with_escape(buf, ixmlNode_getNodeValue(node));
			break;

		case eCDATA_SECTION_NODE:
			ixml_membuf_append_str(buf, "<![CDATA[");
			ixml_membuf_append_str(
				buf, ixmlNode_getNodeValue(node));
			ixml_membuf_append_str(buf, "]]>");
			break;

		case ePROCESSING_INSTRUCTION_NODE:
			ixml_membuf_append_str(buf, "<?");
			ixml_membuf_append_str(buf, ixmlNode_getNodeName(node));
			ixml_membuf_append_str(buf, " ");
			copy_with_escape(buf, ixmlNode_getNodeValue(node));
			ixml_membuf_append_str(buf, "?>\n");
			break;

		case eDOCUMENT_NODE:
			next_child = node->firstChild;
			break;

		case eELEMENT_NODE: {
			IXML_Node *child;
			IXML_Node *attr;

			ixml_membuf_append_str(buf, "<");
			ixml_membuf_append_str(buf, ixmlNode_getNodeName(node));
			if (node->firstAttr) {
				ixml_membuf_append_str(buf, " ");
				for (attr = node->firstAttr; attr;
					attr = attr->nextSibling) {
					ixml_membuf_append_str(buf,
						ixmlNode_getNodeName(attr));
					ixml_membuf_append_str(buf, "=\"");
					copy_with_escape(buf,
						ixmlNode_getNodeValue(attr));
					ixml_membuf_append_str(buf, "\"");
					if (attr->nextSibling)
						ixml_membuf_append_str(
							buf, " ");
				}
			}
			child = ixmlNode_getFirstChild(node);
			if (child &&
				ixmlNode_getNodeType(child) == eELEMENT_NODE)
				ixml_membuf_append_str(buf, ">\r\n");
			else
				ixml_membuf_append_str(buf, ">");
			if (child) {
				next_child = child;
			} else {
				/* Leaf element: close tag immediately. */
				ixmlPrintCloseTag(node, buf);
			}
			break;
		}

		default:
			IxmlPrintf(__FILE__,
				__LINE__,
				"ixmlPrintDomTreeRecursive",
				"Warning, unknown node type %d\n",
				(int)ixmlNode_getNodeType(node));
			break;
		}

		if (next_child) {
			node = next_child;
		} else if (node->nextSibling) {
			node = node->nextSibling;
		} else {
			node = node->parentNode;
			ascending = 1;
		}
	}
}

/*!
 * \brief Print a DOM tree.
 *
 * Element, and Attribute nodes are handled differently. We don't want to print
 * the Element and Attribute nodes' sibling.
 */
static void ixmlPrintDomTree(
	/*! [in] \todo documentation. */
	IXML_Node *nodeptr,
	/*! [in] \todo documentation. */
	ixml_membuf *buf)
{
	const char *nodeName = NULL;
	const char *nodeValue = NULL;
	IXML_Node *child = NULL;

	if (!nodeptr || !buf) {
		return;
	}

	nodeName = (const char *)ixmlNode_getNodeName(nodeptr);
	nodeValue = ixmlNode_getNodeValue(nodeptr);
	switch (ixmlNode_getNodeType(nodeptr)) {
	case eTEXT_NODE:
	case eCDATA_SECTION_NODE:
	case ePROCESSING_INSTRUCTION_NODE:
	case eDOCUMENT_NODE:
		ixmlPrintDomTreeRecursive(nodeptr, buf);
		break;

	case eATTRIBUTE_NODE:
		ixml_membuf_append_str(buf, nodeName);
		ixml_membuf_append_str(buf, "=\"");
		copy_with_escape(buf, nodeValue);
		ixml_membuf_append_str(buf, "\"");
		break;

	case eELEMENT_NODE:
		ixml_membuf_append_str(buf, "<");
		ixml_membuf_append_str(buf, nodeName);
		if (nodeptr->firstAttr) {
			ixml_membuf_append_str(buf, " ");
			ixmlPrintDomTreeRecursive(nodeptr->firstAttr, buf);
		}
		child = ixmlNode_getFirstChild(nodeptr);
		if (child && ixmlNode_getNodeType(child) == eELEMENT_NODE) {
			ixml_membuf_append_str(buf, ">\r\n");
		} else {
			ixml_membuf_append_str(buf, ">");
		}

		/* output the children */
		ixmlPrintDomTreeRecursive(ixmlNode_getFirstChild(nodeptr), buf);

		/* Done with children. Output the end tag. */
		ixml_membuf_append_str(buf, "</");
		ixml_membuf_append_str(buf, nodeName);
		ixml_membuf_append_str(buf, ">\r\n");
		break;

	default:
		IxmlPrintf(__FILE__,
			__LINE__,
			"ixmlPrintDomTree",
			"Warning, unknown node type %d\n",
			(int)ixmlNode_getNodeType(nodeptr));
		break;
	}
}

/*!
 * \brief Converts a DOM tree into a text string.
 *
 * Element, and Attribute nodes are handled differently. We don't want to print
 * the Element and Attribute nodes' sibling.
 */
static void ixmlDomTreetoString(
	/*! [in] \todo documentation. */
	IXML_Node *nodeptr,
	/*! [in] \todo documentation. */
	ixml_membuf *buf)
{
	const char *nodeName = NULL;
	const char *nodeValue = NULL;
	IXML_Node *child = NULL;

	if (!nodeptr || !buf) {
		return;
	}

	nodeName = (const char *)ixmlNode_getNodeName(nodeptr);
	nodeValue = ixmlNode_getNodeValue(nodeptr);

	switch (ixmlNode_getNodeType(nodeptr)) {
	case eTEXT_NODE:
	case eCDATA_SECTION_NODE:
	case ePROCESSING_INSTRUCTION_NODE:
	case eDOCUMENT_NODE:
		ixmlPrintDomTreeRecursive(nodeptr, buf);
		break;

	case eATTRIBUTE_NODE:
		ixml_membuf_append_str(buf, nodeName);
		ixml_membuf_append_str(buf, "=\"");
		copy_with_escape(buf, nodeValue);
		ixml_membuf_append_str(buf, "\"");
		break;

	case eELEMENT_NODE:
		ixml_membuf_append_str(buf, "<");
		ixml_membuf_append_str(buf, nodeName);
		if (nodeptr->firstAttr) {
			ixml_membuf_append_str(buf, " ");
			ixmlPrintDomTreeRecursive(nodeptr->firstAttr, buf);
		}
		child = ixmlNode_getFirstChild(nodeptr);
		if (child && ixmlNode_getNodeType(child) == eELEMENT_NODE) {
			ixml_membuf_append_str(buf, ">");
		} else {
			ixml_membuf_append_str(buf, ">");
		}

		/* output the children */
		ixmlPrintDomTreeRecursive(ixmlNode_getFirstChild(nodeptr), buf);

		/* Done with children. Output the end tag. */
		ixml_membuf_append_str(buf, "</");
		ixml_membuf_append_str(buf, nodeName);
		ixml_membuf_append_str(buf, ">");
		break;

	default:
		IxmlPrintf(__FILE__,
			__LINE__,
			"ixmlPrintDomTreeRecursive",
			"Warning, unknown node type %d\n",
			(int)ixmlNode_getNodeType(nodeptr));
		break;
	}
}

int ixmlLoadDocumentEx(const char *xmlFile, IXML_Document **doc)
{
	if (!xmlFile || !doc) {
		return IXML_INVALID_PARAMETER;
	}

	return Parser_LoadDocument(doc, xmlFile, 1);
}

IXML_Document *ixmlLoadDocument(const char *xmlFile)
{
	IXML_Document *doc = NULL;

	ixmlLoadDocumentEx(xmlFile, &doc);

	return doc;
}

DOMString ixmlPrintDocument(IXML_Document *doc)
{
	IXML_Node *rootNode = (IXML_Node *)doc;
	ixml_membuf memBuf;
	ixml_membuf *buf = &memBuf;

	if (!rootNode) {
		return NULL;
	}

	ixml_membuf_init(buf);
	ixml_membuf_append_str(buf, "<?xml version=\"1.0\"?>\r\n");
	ixmlPrintDomTree(rootNode, buf);

	return buf->buf;
}

DOMString ixmlPrintNode(IXML_Node *node)
{
	ixml_membuf memBuf;
	ixml_membuf *buf = &memBuf;

	if (!node) {
		return NULL;
	}

	ixml_membuf_init(buf);
	ixmlPrintDomTree(node, buf);

	return buf->buf;
}

DOMString ixmlDocumenttoString(IXML_Document *doc)
{
	IXML_Node *rootNode = (IXML_Node *)doc;
	ixml_membuf memBuf;
	ixml_membuf *buf = &memBuf;

	if (!rootNode) {
		return NULL;
	}

	ixml_membuf_init(buf);
	ixml_membuf_append_str(buf, "<?xml version=\"1.0\"?>\r\n");
	ixmlDomTreetoString(rootNode, buf);

	return buf->buf;
}

DOMString ixmlNodetoString(IXML_Node *node)
{
	ixml_membuf memBuf;
	ixml_membuf *buf = &memBuf;

	if (!node) {
		return NULL;
	}

	ixml_membuf_init(buf);
	ixmlDomTreetoString(node, buf);

	return buf->buf;
}

void ixmlRelaxParser(char errorChar) { Parser_setErrorChar(errorChar); }

#ifdef IXML_HAVE_SCRIPTSUPPORT
void ixmlSetBeforeFree(IXML_BeforeFreeNode_t hndlr)
{
	Parser_setBeforeFree(hndlr);
}
#endif

int ixmlParseBufferEx(const char *buffer, IXML_Document **retDoc)
{
	if (!buffer || !retDoc) {
		return IXML_INVALID_PARAMETER;
	}
	if (buffer[0] == '\0') {
		return IXML_INVALID_PARAMETER;
	}

	return Parser_LoadDocument(retDoc, buffer, 0);
}

IXML_Document *ixmlParseBuffer(const char *buffer)
{
	IXML_Document *doc = NULL;

	ixmlParseBufferEx(buffer, &doc);

	return doc;
}

DOMString ixmlCloneDOMString(const DOMString src)
{
	if (!src) {
		return NULL;
	}

	return strdup(src);
}

void ixmlFreeDOMString(DOMString buf)
{
	if (buf) {
		free(buf);
	}
}
