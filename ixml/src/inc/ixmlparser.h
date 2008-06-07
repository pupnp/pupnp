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


#ifndef _IXMLPARSER_H
#define _IXMLPARSER_H


/*!
 * \file
 */


#include "ixml.h"
#include "ixmlmembuf.h"


/* Parser definitions */
#define QUOT        "&quot;"
#define LT          "&lt;"
#define GT          "&gt;"
#define APOS        "&apos;"
#define AMP         "&amp;"
#define ESC_HEX     "&#x"
#define ESC_DEC     "&#"


typedef struct _IXML_NamespaceURI 
{
	char *nsURI;
	char *prefix;
	struct _IXML_NamespaceURI *nextNsURI;
} IXML_NamespaceURI;


typedef struct _IXML_ElementStack
{
	char *element;
	char *prefix;
	char *namespaceUri;
	IXML_NamespaceURI *pNsURI;
	struct _IXML_ElementStack *nextElement;
} IXML_ElementStack;


typedef enum
{
	eELEMENT,
	eATTRIBUTE,
	eCONTENT,
} PARSER_STATE;


typedef struct _Parser
{
	/*! Data buffer. */
	char *dataBuffer;
	/*! Pointer to the token parsed. */
	char *curPtr;
	/*! Saves for backup. */
	char *savePtr;
	ixml_membuf lastElem;
	ixml_membuf tokenBuf;    
	IXML_Node *pNeedPrefixNode;
	IXML_ElementStack *pCurElement;
	IXML_Node *currentNodePtr;
	PARSER_STATE state;
	BOOL bHasTopLevel;
} Parser;


/*!
 * \brief Check to see whether name is a valid xml name.
 */
BOOL Parser_isValidXmlName(
	/*! [in] The string to be checked. */
	const DOMString name);


/*!
 * \brief Sets the error character.
 *
 * If 'c' is 0 (default), the parser is strict about XML encoding:
 * invalid UTF-8 sequences or "&" entities are rejected, and the parsing 
 * aborts.
 *
 * If 'c' is not 0, the parser is relaxed: invalid UTF-8 characters
 * are replaced by this character, and invalid "&" entities are left
 * untranslated. The parsing is then allowed to continue.
 */
void Parser_setErrorChar(
	/*! [in] The character to become the error character. */
	char c);


/*!
 * \brief Fees a node contents.
 */
void Parser_freeNodeContent(
	/*! [in] The Node to process. */
	IXML_Node *IXML_Nodeptr);

int Parser_LoadDocument(IXML_Document **retDoc, const char * xmlFile, BOOL file);

int Parser_setNodePrefixAndLocalName(IXML_Node *newIXML_NodeIXML_Attr);


void ixmlAttr_free(IXML_Attr *attrNode);
void ixmlAttr_init(IXML_Attr *attrNode);

/*!
 * \brief Set the given element's tagName.
 *
 * \return One of the following:
 * 	\li \b IXML_SUCCESS, if successfull.
 * 	\li \b IXML_FAILED, if element of tagname is \b NULL.
 * 	\li \b IXML_INSUFFICIENT_MEMORY, if there is no memory to allocate the
 * 		buffer for the element's tagname.
 */
int ixmlElement_setTagName(
	/*! [in] The element to change the tagname. */
	IXML_Element *element,
	/*! [in] The new tagName for the element. */
	const char *tagName);


/*!
 * \brief Initializes a NamedNodeMap object.
 */
void ixmlNamedNodeMap_init(
	/*! [in] The named node map to process. */
	IXML_NamedNodeMap *nnMap);


/*!
 * \brief Add a node to a NamedNodeMap.
 *
 * \return IXML_SUCCESS or failure.
 */
int ixmlNamedNodeMap_addToNamedNodeMap(
	/* [in] The named node map. */
	IXML_NamedNodeMap **nnMap,
	/* [in] The node to add. */
	IXML_Node *add);

/*!
 * \brief Add a node to nodelist.
 */
int ixmlNodeList_addToNodeList(
	/*! [in] The pointer to the nodelist. */
	IXML_NodeList **nList,
	/*! [in] The node to add. */
	IXML_Node *add);


void    ixmlNode_init(IXML_Node *IXML_Nodeptr);
BOOL    ixmlNode_compare(IXML_Node *srcIXML_Node, IXML_Node *destIXML_Node);

void    ixmlNode_getElementsByTagName(IXML_Node *n, const char *tagname, IXML_NodeList **list);
void    ixmlNode_getElementsByTagNameNS(IXML_Node *IXML_Node, const char *namespaceURI,
                const char *localName, IXML_NodeList **list);

int     ixmlNode_setNodeProperties(IXML_Node* node, IXML_Node *src);
int     ixmlNode_setNodeName( IXML_Node* node, const DOMString qualifiedName);

void    ixmlNodeList_init(IXML_NodeList *nList);


#endif  // _IXMLPARSER_H

