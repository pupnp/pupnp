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


#include "ixmldebug.h"
#include "ixmlmembuf.h"
#include "ixmlparser.h"


#include <stdlib.h> /* for free() */
#include <string.h>


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

	if (p == NULL)
		return;
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


/*!
 * \brief Recursive function to print all the node in a tree.
 * Internal to parser only.
 */
static void ixmlPrintDomTreeRecursive(
	/*! [in] \todo documentation. */
	IXML_Node *nodeptr,
	/*! [in] \todo documentation. */
	ixml_membuf *buf)
{
	const char *nodeName = NULL;
	const char *nodeValue = NULL;
	IXML_Node *child = NULL,
	*sibling = NULL;

	if (nodeptr != NULL) {
		nodeName = (const char *)ixmlNode_getNodeName(nodeptr);
		nodeValue = ixmlNode_getNodeValue(nodeptr);
		
		switch (ixmlNode_getNodeType(nodeptr)) {
		case eTEXT_NODE:
			copy_with_escape(buf, nodeValue);
			break;

		case eCDATA_SECTION_NODE:
			ixml_membuf_append_str(buf, "<![CDATA[");
			ixml_membuf_append_str(buf, nodeValue);
			ixml_membuf_append_str(buf, "]]>");
			break;

		case ePROCESSING_INSTRUCTION_NODE:
			ixml_membuf_append_str(buf, "<?");
			ixml_membuf_append_str(buf, nodeName);
			ixml_membuf_append_str(buf, " ");
			copy_with_escape(buf, nodeValue);
			ixml_membuf_append_str(buf, "?>\n");
			break;

		case eDOCUMENT_NODE:
			ixmlPrintDomTreeRecursive(
				ixmlNode_getFirstChild(nodeptr), buf);
			break;

		case eATTRIBUTE_NODE:
			ixml_membuf_append_str(buf, nodeName);
			ixml_membuf_append_str(buf, "=\"");
			copy_with_escape(buf, nodeValue);
			ixml_membuf_append_str(buf, "\"");
			if (nodeptr->nextSibling != NULL) {
				ixml_membuf_append_str(buf, " ");
				ixmlPrintDomTreeRecursive(nodeptr->nextSibling, buf);
			}
			break;

		case eELEMENT_NODE:
			ixml_membuf_append_str(buf, "<");
			ixml_membuf_append_str(buf, nodeName);
			if (nodeptr->firstAttr != NULL) {
				ixml_membuf_append_str(buf, " ");
				ixmlPrintDomTreeRecursive(nodeptr->firstAttr, buf);
			}
			child = ixmlNode_getFirstChild(nodeptr);
			if (child != NULL &&
			    ixmlNode_getNodeType(child) == eELEMENT_NODE) {
				ixml_membuf_append_str(buf, ">\r\n");
			} else {
				ixml_membuf_append_str(buf, ">");
			}
			/* output the children */
			ixmlPrintDomTreeRecursive(
				ixmlNode_getFirstChild(nodeptr), buf);

			/* Done with children.  Output the end tag. */
			ixml_membuf_append_str(buf, "</");
			ixml_membuf_append_str(buf, nodeName);

			sibling = ixmlNode_getNextSibling(nodeptr);
			if (sibling != NULL &&
			    ixmlNode_getNodeType(sibling) == eTEXT_NODE) {
				ixml_membuf_append_str( buf, ">" );
			} else {
				ixml_membuf_append_str( buf, ">\r\n" );
			}
			ixmlPrintDomTreeRecursive(
				ixmlNode_getNextSibling(nodeptr), buf);
			break;

		default:
			IxmlPrintf(__FILE__, __LINE__, "ixmlPrintDomTreeRecursive",
				"Warning, unknown node type %d\n",
				(int)ixmlNode_getNodeType(nodeptr));
			break;
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

	if (nodeptr == NULL || buf == NULL) {
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
		if (nodeptr->firstAttr != NULL) {
			ixml_membuf_append_str(buf, " ");
			ixmlPrintDomTreeRecursive(nodeptr->firstAttr, buf);
		}
		child = ixmlNode_getFirstChild(nodeptr);
		if (child != NULL &&
		    ixmlNode_getNodeType(child) == eELEMENT_NODE) {
			ixml_membuf_append_str(buf, ">\r\n");
		} else {
			ixml_membuf_append_str(buf, ">");
		}

		/* output the children */
		ixmlPrintDomTreeRecursive(
			ixmlNode_getFirstChild(nodeptr), buf);

		/* Done with children. Output the end tag. */
		ixml_membuf_append_str(buf, "</");
		ixml_membuf_append_str(buf, nodeName);
		ixml_membuf_append_str(buf, ">\r\n");
		break;

	default:
		IxmlPrintf(__FILE__, __LINE__, "ixmlPrintDomTree",
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

	if (nodeptr == NULL || buf == NULL) {
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
		copy_with_escape(buf, nodeValue );
		ixml_membuf_append_str(buf, "\"");
		break;

	case eELEMENT_NODE:
		ixml_membuf_append_str(buf, "<");
		ixml_membuf_append_str(buf, nodeName);
		if (nodeptr->firstAttr != NULL) {
			ixml_membuf_append_str(buf, " ");
			ixmlPrintDomTreeRecursive(nodeptr->firstAttr, buf);
		}
		child = ixmlNode_getFirstChild(nodeptr);
		if (child != NULL &&
		    ixmlNode_getNodeType(child) == eELEMENT_NODE) {
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
		IxmlPrintf(__FILE__, __LINE__, "ixmlPrintDomTreeRecursive",
			"Warning, unknown node type %d\n",
			(int)ixmlNode_getNodeType(nodeptr));
		break;
	}
}


int ixmlLoadDocumentEx(const char *xmlFile, IXML_Document **doc)
{
	if (xmlFile == NULL || doc == NULL) {
		return IXML_INVALID_PARAMETER;
	}

	return Parser_LoadDocument(doc, xmlFile, TRUE);
}


IXML_Document *ixmlLoadDocument(const char *xmlFile)
{
	IXML_Document *doc = NULL;

	ixmlLoadDocumentEx(xmlFile, &doc);

	return doc;
}


DOMString ixmlPrintDocument(IXML_Document *doc)
{
	IXML_Node* rootNode = (IXML_Node *)doc;
	ixml_membuf memBuf;
	ixml_membuf *buf = &memBuf;

	if(rootNode == NULL) {
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

	if (node == NULL) {
		return NULL;
	}

	ixml_membuf_init(buf);
	ixmlPrintDomTree(node, buf);

	return buf->buf;
}


DOMString ixmlDocumenttoString(IXML_Document *doc)
{
	IXML_Node* rootNode = (IXML_Node *)doc;
	ixml_membuf memBuf;
	ixml_membuf *buf = &memBuf;

	if(rootNode == NULL) {
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

	if (node == NULL) {
		return NULL;
	}

	ixml_membuf_init(buf);
	ixmlDomTreetoString(node, buf);

	return buf->buf;
}


void ixmlRelaxParser(char errorChar)
{
	Parser_setErrorChar(errorChar);
}

#ifdef IXML_HAVE_SCRIPTSUPPORT
void ixmlSetBeforeFree(IXML_BeforeFreeNode_t hndlr)
{
	Parser_setBeforeFree(hndlr);
}
#endif


int ixmlParseBufferEx(const char *buffer, IXML_Document **retDoc)
{
	if (buffer == NULL || retDoc == NULL) {
		return IXML_INVALID_PARAMETER;
	}

	if (buffer[0] == '\0') {
		return IXML_INVALID_PARAMETER;
	}

	return Parser_LoadDocument(retDoc, buffer, FALSE);
}


IXML_Document *ixmlParseBuffer(const char *buffer)
{
	IXML_Document *doc = NULL;

	ixmlParseBufferEx(buffer, &doc);

	return doc;
}


DOMString ixmlCloneDOMString(const DOMString src)
{
	if (src == NULL) {
		return NULL;
	}

	return strdup(src);
}


void ixmlFreeDOMString(DOMString buf)
{
	if (buf != NULL) {
		free(buf);
	}
}

