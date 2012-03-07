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


#ifndef IXML_H
#define IXML_H


/*!
 * \file
 *
 * \defgroup XMLAPI XML API
 *
 * @{
 */


#include "UpnpGlobal.h" /* For EXPORT_SPEC */

/* Define BOOL. */
#ifndef __OBJC__ 
	typedef int BOOL;
#else
	/* For Objective C compilers, include objc.h which defines BOOL. */
	#include <objc/objc.h>
#endif

/*!
 * \brief The type of DOM strings.
 */
#define DOMString char *
/*typedef char *DOMString;*/


#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef INOUT
#define INOUT
#endif


/*!
 * \name DOM Interfaces
 *
 * The Document Object Model consists of a set of objects and interfaces
 * for accessing and manipulating documents.  IXML does not implement all
 * the interfaces documented in the DOM2-Core recommendation but defines
 * a subset of the most useful interfaces.  A description of the supported
 * interfaces and methods is presented in this section.
 *
 * For a complete discussion on the object model, the object hierarchy,
 * etc., refer to section 1.1 of the DOM2-Core recommendation.
 *
 * @{
 */


/*!
 * \brief The type of the DOM node
 */
typedef enum {
	eINVALID_NODE                = 0,
	eELEMENT_NODE                = 1,
	eATTRIBUTE_NODE              = 2,
	eTEXT_NODE                   = 3,
	eCDATA_SECTION_NODE          = 4,
	eENTITY_REFERENCE_NODE       = 5,
	eENTITY_NODE                 = 6,                
	ePROCESSING_INSTRUCTION_NODE = 7,
	eCOMMENT_NODE                = 8,
	eDOCUMENT_NODE               = 9,
	eDOCUMENT_TYPE_NODE          = 10,
	eDOCUMENT_FRAGMENT_NODE      = 11,
	eNOTATION_NODE               = 12,
}   IXML_NODE_TYPE;


/*!
 * \brief Error codes returned by the XML API, see the DOM spec
 */
typedef enum {
	IXML_SUCCESS                     = 0,

	IXML_INDEX_SIZE_ERR              = 1,
	IXML_DOMSTRING_SIZE_ERR          = 2,
	IXML_HIERARCHY_REQUEST_ERR       = 3,
	IXML_WRONG_DOCUMENT_ERR          = 4,
	IXML_INVALID_CHARACTER_ERR       = 5,
	IXML_NO_DATA_ALLOWED_ERR         = 6,
	IXML_NO_MODIFICATION_ALLOWED_ERR = 7,
	IXML_NOT_FOUND_ERR               = 8,
	IXML_NOT_SUPPORTED_ERR           = 9,
	IXML_INUSE_ATTRIBUTE_ERR         = 10,
	IXML_INVALID_STATE_ERR           = 11,
	IXML_SYNTAX_ERR                  = 12,
	IXML_INVALID_MODIFICATION_ERR    = 13,
	IXML_NAMESPACE_ERR               = 14,
	IXML_INVALID_ACCESS_ERR          = 15,

	IXML_NO_SUCH_FILE                = 101,
	IXML_INSUFFICIENT_MEMORY         = 102,
	IXML_FILE_DONE                   = 104,
	IXML_INVALID_PARAMETER           = 105,
	IXML_FAILED                      = 106,
	IXML_INVALID_ITEM_NUMBER         = 107,
} IXML_ERRORCODE;


#define DOCUMENTNODENAME    "#document"
#define TEXTNODENAME        "#text"
#define CDATANODENAME       "#cdata-section"


typedef struct _IXML_Document *Docptr;


typedef struct _IXML_Node     *Nodeptr;


/*!
 * \brief Data structure common to all types of nodes.
 */
typedef struct _IXML_Node
{
	DOMString         nodeName;
	DOMString         nodeValue;
	IXML_NODE_TYPE    nodeType;
	DOMString         namespaceURI;
	DOMString         prefix;
	DOMString         localName;
	BOOL              readOnly;
                          
	Nodeptr           parentNode;
	Nodeptr           firstChild;
	Nodeptr           prevSibling;
	Nodeptr           nextSibling;
	Nodeptr           firstAttr;
	Docptr            ownerDocument;
} IXML_Node;


/*!
 * \brief Data structure representing the DOM Document.
 */
typedef struct _IXML_Document
{
	IXML_Node n;
} IXML_Document;


/*!
 * \brief Data structure representing a CDATA section node.
 */
typedef struct _IXML_CDATASection
{
	IXML_Node n;
} IXML_CDATASection;


/*!
 * \brief Data structure representing an Element node.
 */
typedef struct _IXML_Element
{
	IXML_Node n;
	DOMString tagName;
} IXML_Element;


/*!
 * \brief Data structure representing an Attribute node.
 */
typedef struct _IXML_ATTR
{
	IXML_Node n;
	BOOL specified;
	IXML_Element *ownerElement;
} IXML_Attr;


/*!
 * \brief Data structure representing a Text node.
 */
typedef struct _IXML_Text
{
	IXML_Node n;
} IXML_Text;


/*!
 * \brief Data structure representing a list of nodes.
 */
typedef struct _IXML_NodeList
{
	IXML_Node *nodeItem;
	struct  _IXML_NodeList *next;
} IXML_NodeList;


/*!
 * \brief Data structure representing a list of named nodes.
 */
typedef struct _IXML_NamedNodeMap
{
	IXML_Node *nodeItem;
	struct _IXML_NamedNodeMap *next;
} IXML_NamedNodeMap;

/* @} DOM Interfaces */



#ifdef __cplusplus
extern "C" {
#endif


/*!
 * \name Interface Node
 *
 * The \b Node interface forms the primary datatype for all other DOM 
 * objects. Every other interface is derived from this interface, inheriting 
 * its functionality. For more information, refer to DOM2-Core page 34.
 *
 * @{
 */

/*!
 * \brief Returns the name of the \b Node, depending on what type of 
 * \b Node it is, in a read-only string.
 *
 * Refer to the table in the 
 * DOM2-Core for a description of the node names for various interfaces.
 *
 * \return A constant \b DOMString of the node name.
 */
EXPORT_SPEC const DOMString ixmlNode_getNodeName(
	/*! [in] Pointer to the node to retrieve the name. */
	IXML_Node *nodeptr);


/*!
 * \brief Returns the value of the \b Node as a string.
 *
 * Note that this string is not a copy and modifying it will modify the value
 * of the \b Node.
 *
 *  \return A \b DOMString of the \b Node value.
 */
EXPORT_SPEC const DOMString ixmlNode_getNodeValue(
	/*! [in] Pointer to the \b Node to retrieve the value. */
	IXML_Node *nodeptr);


/*!
 * \brief Assigns a new value to a \b Node.
 *
 * The \b newNodeValue string is duplicated and stored in the \b Node so that
 * the original does not have to persist past this call.
 *
 *  \return An integer representing one of the following:
 *      \li \c IXML_SUCCESS: The operation completed successfully.
 *      \li \c IXML_INVALID_PARAMETER: The <b>Node *</b> is not a valid pointer.
 *      \li \c IXML_INSUFFICIENT_MEMORY: Not enough free memory exists to
 *      	complete this operation.
 */
EXPORT_SPEC int ixmlNode_setNodeValue(
	/*! [in] The \b Node to which to assign a new value. */
	IXML_Node *nodeptr, 
	/*! [in] The new value of the \b Node. */
	const char *newNodeValue);


/*!
 * \brief Retrieves the type of a \b Node.
 *
 *  \return An enum IXML_NODE_TYPE representing the type of the \b Node.
 */
EXPORT_SPEC unsigned short ixmlNode_getNodeType(
	/*! [in] The \b Node from which to retrieve the type. */
	IXML_Node *nodeptr);


/*!
 * \brief Retrieves the parent \b Node for a \b Node.
 *
 * \return A pointer to the parent \b Node or \c NULL if the \b Node has no
 *	parent.
 */
EXPORT_SPEC IXML_Node *ixmlNode_getParentNode(
	/*! [in] The \b Node from which to retrieve the parent. */ 
	IXML_Node *nodeptr);


/*!
 * \brief Retrieves the list of children of a \b Node in a \b NodeList 
 * structure.
 *
 * If a \b Node has no children, \b ixmlNode_getChildNodes 
 * returns a \b NodeList structure that contains no \b Nodes.
 *
 * \return A \b NodeList of the children of the \b Node.
 */
EXPORT_SPEC IXML_NodeList *ixmlNode_getChildNodes(
	/*! [in] The \b Node from which to retrieve the children. */
	IXML_Node *nodeptr);


/*!
 * \brief Retrieves the first child \b Node of a \b Node.
 *
 * \return A pointer to the first child \b Node or \c NULL if the \b Node does
 * not have any children.
 */
EXPORT_SPEC IXML_Node *ixmlNode_getFirstChild(
	/*! [in] The \b Node from which to retrieve the first child. */ 
	IXML_Node *nodeptr);


/*!
 * \brief Retrieves the last child \b Node of a \b Node.
 *
 * \return A pointer to the last child \b Node or \c NULL if the \b Node does
 * not have any children.
 */
EXPORT_SPEC IXML_Node *ixmlNode_getLastChild(
	/*! [in] The \b Node from which to retrieve the last child. */
	IXML_Node *nodeptr);


/*!
 * \brief Retrieves the sibling \b Node immediately preceding this \b Node.
 *
 * \return A pointer to the previous sibling \b Node or \c NULL if no such
 * \b Node exists.
 */
EXPORT_SPEC IXML_Node *ixmlNode_getPreviousSibling(
	/*! [in] The \b Node for which to retrieve the previous sibling. */
	IXML_Node *nodeptr);


/*!
 * \brief Retrieves the sibling \b Node immediately following this \b Node.
 *
 *  \return A pointer to the next sibling \b Node or \c NULL if no such
 *  \b Node exists.
 */
EXPORT_SPEC IXML_Node *ixmlNode_getNextSibling(
	/*! [in] The \b Node from which to retrieve the next sibling. */ 
	IXML_Node *nodeptr);


/*!
 * \brief Retrieves the attributes of a \b Node, if it is an \b Element node,
 *  in a \b NamedNodeMap structure.
 *
 *  \return A \b NamedNodeMap of the attributes or \c NULL.
 */
EXPORT_SPEC IXML_NamedNodeMap *ixmlNode_getAttributes(
	/*! [in] The \b Node from which to retrieve the attributes. */ 
	IXML_Node *nodeptr);


/*!
 * \brief Retrieves the document object associated with this \b Node.
 *
 * This owner document \b Node allows other \b Nodes to be created in the
 * context of this document.  Note that \b Document nodes do not have an
 * owner document.
 *
 * \return A pointer to the owning \b Document or \c NULL, if the \b Node
 * does not have an owner.
 */
EXPORT_SPEC IXML_Document *ixmlNode_getOwnerDocument(
	/*! [in] The \b Node from which to retrieve the owner document. */
	IXML_Node *nodeptr);


/*!
 * \brief Retrieves the namespace URI for a \b Node as a \b DOMString.
 *
 * Only \b Nodes of type \c eELEMENT_NODE or \c eATTRIBUTE_NODE can have a
 * namespace URI.  \b Nodes created through the \b Document interface will
 * only contain a namespace if created using \b ixmlDocument_createElementNS.
 *
 * \return A \b DOMString representing the URI of the namespace or \c NULL.
 */
EXPORT_SPEC const DOMString ixmlNode_getNamespaceURI(
	/*! [in] The \b Node for which to retrieve the namespace. */
	IXML_Node *nodeptr);


/*!
 * \brief Retrieves the namespace prefix, if present.
 *
 * The prefix is the name used as an alias for the namespace URI for this
 * element.  Only \b Nodes of type \c eELEMENT_NODE or \c eATTRIBUTE_NODE can
 * have a prefix. \b Nodes created through the \b Document interface will only
 * contain a prefix if created using \b ixmlDocument_createElementNS.
 *
 *  \return A \b DOMString representing the namespace prefix or \c NULL.
 */
EXPORT_SPEC const DOMString               
ixmlNode_getPrefix(
	/*! The \b Node from which to retrieve the prefix. */
	IXML_Node *nodeptr);


/*!
 * \brief Retrieves the local name of a \b Node, if present.
 *
 * The local name is the tag name without the namespace prefix. Only \b Nodes
 * of type \c eELEMENT_NODE or \c eATTRIBUTE_NODE can have a local name.
 * \b Nodes created through the \b Document interface will only contain a local
 * name if created using \b ixmlDocument_createElementNS.
 *
 *  \return A \b DOMString representing the local name of the \b Element or
 *  	\c NULL.
 */
EXPORT_SPEC const DOMString ixmlNode_getLocalName(
	/*! [in] The \b Node from which to retrieve the local name. */
	IXML_Node *nodeptr);

/*! 
 * \brief Inserts a new child \b Node before the existing child \b Node.
 *
 * \b refChild can be \c NULL, which inserts \b newChild at the
 * end of the list of children.  Note that the \b Node (or \b Nodes) 
 * in \b newChild must already be owned by the owner document (or have no
 * owner at all) of \b nodeptr for insertion.  If not, the \b Node 
 * (or \b Nodes) must be imported into the document using 
 * \b ixmlDocument_importNode.  If \b newChild is already in the tree,
 * it is removed first.
 *
 * \return An integer representing one of the following:
 *     \li \c IXML_SUCCESS: The operation completed successfully.
 *     \li \c IXML_INVALID_PARAMETER: Either \b nodeptr or 
 *           \b newChild is \c NULL.
 *     \li \c IXML_HIERARCHY_REQUEST_ERR: The type of the \b Node 
 *           does not allow children of the type of \b newChild.
 *     \li \c IXML_WRONG_DOCUMENT_ERR: \b newChild has an owner 
 *           document that does not match the owner of \b nodeptr.
 *     \li \c IXML_NO_MODIFICATION_ALLOWED_ERR: \b nodeptr is 
 *           read-only or the parent of the \b Node being inserted is 
 *           read-only.
 *     \li \c IXML_NOT_FOUND_ERR: \b refChild is not a child of 
 *           \b nodeptr.
 */
EXPORT_SPEC int ixmlNode_insertBefore(
	/*! [in] The parent of the \b Node before which to insert the new child. */
	IXML_Node *nodeptr,   
	/*! [in] The \b Node to insert into the tree. */
	IXML_Node * newChild,
	/*! [in] The reference child where the new \b Node should be inserted.
	 * The new \b Node will appear directly before the reference child. */
	IXML_Node * refChild);


/*!
 * \brief Replaces an existing child \b Node with a new child \b Node in the
 * list of children of a \b Node.
 *
 * If \b newChild is already in the tree, it will first be removed.
 * \b returnNode will contain the \b oldChild \b Node, appropriately removed
 * from the tree (i.e. it will no longer have an owner document).
 *
 * \return An integer representing one of the following:
 *      \li \c IXML_SUCCESS: The operation completed successfully.
 *      \li \c IXML_INVALID_PARAMTER: Either \b nodeptr, \b newChild,
 *      	or \b oldChild is \c NULL.
 *      \li \c IXML_HIERARCHY_REQUEST_ERR: The \b newChild is not 
 *            a type of \b Node that can be inserted into this tree or 
 *            \b newChild is an ancestor of \b nodePtr.
 *      \li \c IXML_WRONG_DOCUMENT_ERR: \b newChild was created from 
 *            a different document than \b nodeptr.
 *      \li \c IXML_NO_MODIFICATION_ALLOWED_ERR: \b nodeptr or 
 *            its parent is read-only.
 *      \li \c IXML_NOT_FOUND_ERR: \b oldChild is not a child of 
 *            \b nodeptr.
 */
EXPORT_SPEC int ixmlNode_replaceChild(
	/*! [in] The parent of the \b Node which contains the child to replace. */
	IXML_Node *nodeptr,
	/*! [in] The child with which to replace \b oldChild. */
	IXML_Node *newChild,        
	/*! [in] The child to replace with \b newChild. */
	IXML_Node *oldChild,        
	/*! [out] Pointer to a \b Node to place the removed \b oldChild \b Node. */
	IXML_Node **returnNode);


/*!
 * \brief Removes a child from the list of children of a \b Node.
 *
 * \b returnNode will contain the \b oldChild \b Node, 
 * appropriately removed from the tree (i.e. it will no longer have an 
 * owner document).
 *
 * \return An integer representing one of the following:
 *     \li \c IXML_SUCCESS: The operation completed successfully.
 *     \li \c IXML_INVALID_PARAMETER: Either \b nodeptr or 
 *           \b oldChild is \c NULL.
 *     \li \c IXML_NO_MODIFICATION_ALLOWED_ERR: \b nodeptr or its 
 *           parent is read-only.
 *     \li \c IXML_NOT_FOUND_ERR: \b oldChild is not among the 
 *           children of \b nodeptr.
 */
EXPORT_SPEC int ixmlNode_removeChild(
	/*! [in] The parent of the child to remove. */
	IXML_Node *nodeptr,
	/*! [in] The child \b Node to remove. */
	IXML_Node *oldChild,
	/*! [out] Pointer to a \b Node to place the removed \b oldChild \b Node. */
	IXML_Node **returnNode);


/*!
 * \brief Appends a child \b Node to the list of children of a \b Node.
 *
 * If \b newChild is already in the tree, it is removed first.
 *
 * \return An integer representing one of the following:
 *      \li \c IXML_SUCCESS: The operation completed successfully.
 *      \li \c IXML_INVALID_PARAMETER: Either \b nodeptr or 
 *            \b newChild is \c NULL.
 *      \li \c IXML_HIERARCHY_REQUEST_ERR: \b newChild is of a type 
 *            that cannot be added as a child of \b nodeptr or 
 *            \b newChild is an ancestor of \b nodeptr.
 *      \li \c IXML_WRONG_DOCUMENT_ERR: \b newChild was created from 
 *            a different document than \b nodeptr.
 *      \li \c IXML_NO_MODIFICATION_ALLOWED_ERR: \b nodeptr is a 
 *            read-only \b Node.
 */
EXPORT_SPEC int ixmlNode_appendChild(
	/*! [in] The \b Node in which to append the new child. */
	IXML_Node *nodeptr,
	/*! [in] The new child to append. */
	IXML_Node * newChild);


/*!
 * \brief Queries whether or not a \b Node has children.
 *
 * \return \c TRUE if the \b Node has one or more children otherwise \c FALSE.
 */
EXPORT_SPEC BOOL ixmlNode_hasChildNodes(
	/*! [in] The \b Node to query for children. */
	IXML_Node *nodeptr);


/*!
 * \brief Clones a \b Node.
 *
 * The new \b Node does not have a parent. The \b deep parameter controls
 * whether the subtree of the \b Node is also cloned.
 *
 * For details on cloning specific types of \b Nodes, refer to the
 * DOM2-Core recommendation.
 *
 * \return A clone of \b nodeptr or \c NULL.
 */
EXPORT_SPEC IXML_Node *ixmlNode_cloneNode(
	/*! [in] The \b Node to clone.  */
	IXML_Node *nodeptr,
	/*! [in] \c TRUE to clone the subtree also or \c FALSE to clone only
	 * \b nodeptr. */
	BOOL deep);


/*!
 * \brief Queries whether this \b Node has attributes.
 *
 * Note that only \b Element nodes have attributes.
 *
 * \return \c TRUE if the \b Node has attributes otherwise \c FALSE.
 */
EXPORT_SPEC BOOL ixmlNode_hasAttributes(
	/*! [in] The \b Node to query for attributes. */
	IXML_Node *nodeptr);


/*!
 * \brief Frees a \b Node and all \b Nodes in its subtree.
 */
EXPORT_SPEC void ixmlNode_free(
	/*! [in] The \b Node tree to free. */
	IXML_Node *nodeptr);

/* @} Interface Node */



/*!
 * \name Interface Attr
 *
 * The \b Attr interface represents an attribute of an \b Element. The document
 * type definition (DTD) or schema usually dictate the allowable attributes and
 * values for a particular element.
 *
 * For more information, refer to the <em>Interface Attr</em> section in the
 * DOM2-Core.
 *
 * @{
 */


/*!
 * \brief Frees an \b Attr node.
 */
EXPORT_SPEC void ixmlAttr_free(
	/*! The \b Attr node to free. */
	IXML_Attr *attrNode);


/* @} Interface Attr */



/*!
 * \name Interface CDATASection
 *
 * The \b CDATASection is used to escape blocks of text containing
 * characters that would otherwise be regarded as markup. CDATA sections
 * cannot be nested. Their primary purpose is for including material such
 * XML fragments, without needing to escape all the delimiters.
 *
 * For more information, refer to the <em>Interface CDATASection</em> section
 * in the DOM2-Core.
 *
 * @{
 */


/*!
 * \brief Initializes a \b CDATASection node.
 */
EXPORT_SPEC void ixmlCDATASection_init(
	/*! [in] The <b>CDATA Section Node</b> to iniatialize. */
	IXML_CDATASection *nodeptr);


/*!
 * \brief Frees a \b CDATASection node.
 */
EXPORT_SPEC void ixmlCDATASection_free(
	/*! The \b CDATASection node to free. */
	IXML_CDATASection *nodeptr);


/* @} Interface CDATASection */



/*!
 * \name Interface Document
 *
 * The \b Document interface represents the entire XML document. In essence, it
 * is the root of the document tree and provides the primary interface to the
 * elements of the document.
 *
 * For more information, refer to the <em>Interface Document</em> section in
 * the DOM2Core.
 *
 * @{
 */


/*!
 * \brief Initializes a \b Document node.
 */
EXPORT_SPEC void ixmlDocument_init(
	/*! [in] The \b Document node to initialize.  */
	IXML_Document *nodeptr);


/*!
 * \brief Creates a new empty \b Document node.
 *
 * The \b ixmlDocument_createDocumentEx API differs from the
 * \b ixmlDocument_createDocument API in that it returns an error code
 * describing the reason for the failure rather than just \c NULL.
 *
 * \return An integer representing one of the following:
 *      \li \c IXML_SUCCESS: The operation completed successfully.
 *      \li \c IXML_INSUFFICIENT_MEMORY: Not enough free memory exists 
 *            to complete this operation.
 */
EXPORT_SPEC int ixmlDocument_createDocumentEx(
	/*! [out] Pointer to a \b Document where the new object will be stored. */
	IXML_Document **doc);


/*!
 * \brief Creates a new empty \b Document node.
 *
 * \return A pointer to the new \b Document object with the nodeName set to
 *  	"#document" or \c NULL on failure.
 */
EXPORT_SPEC IXML_Document *ixmlDocument_createDocument(void);


/*!
 * \brief Creates a new \b Element node with the given tag name.
 *
 * The new \b Element node has a \c nodeName of \b tagName and the
 * \c localName, \c prefix, and \c namespaceURI set to \c NULL.  To create an
 * \b Element with a namespace, see \b ixmlDocument_createElementNS.
 *
 * The \b ixmlDocument_createElementEx API differs from the \b
 * ixmlDocument_createElement API in that it returns an error code
 * describing the reason for failure rather than just \c NULL.
 *
 * \return An integer representing one of the following:
 *     \li \c IXML_SUCCESS: The operation completed successfully.
 *     \li \c IXML_INVALID_PARAMETER: Either \b doc or 
 *           \b tagName is \c NULL.
 *     \li \c IXML_INSUFFICIENT_MEMORY: Not enough free memory exists 
 *           to complete this operation.
 */
EXPORT_SPEC int ixmlDocument_createElementEx(
	/*! [in] The owner \b Document of the new node. */
	IXML_Document *doc,
	/*! [in] The tag name of the new \b Element node. */
	const DOMString tagName,  
	/*! [out] Pointer to an \b Element where the new object will be stored. */
	IXML_Element **rtElement);


/*!
 * \brief Creates a new \b Element node with the given tag name.
 *
 * The new \b Element node has a \c nodeName of \b tagName and the
 * \c localName, \c prefix, and \c namespaceURI set to \c NULL. To create an
 * \b Element with a namespace, see \b ixmlDocument_createElementNS.
 *
 *  \return A pointer to the new \b Element object with  the node name set to
 *  tagName, and localName, prefix and namespaceURI set to \c NULL, or \c NULL
 *  on failure.
 */
EXPORT_SPEC IXML_Element *ixmlDocument_createElement(
	/*! [in] The owner \b Document of the new node. */
	IXML_Document *doc,
	/*! [in] The tag name of the new \b Element node (case-sensitive). */
	const DOMString tagName);


/*!
 * \brief Creates a new \b Text node with the given data.
 * 
 * The \b ixmlDocument_createTextNodeEx() API differs from the
 * \b ixmlDocument_createTextNode API in that it returns an error code
 * describing the reason for failure rather than just \c NULL.
 *
 * \return An integer representing one of the following:
 *     \li \c IXML_SUCCESS: The operation completed successfully.
 *     \li \c IXML_INVALID_PARAMETER: Either \b doc or \b data 
 *           is \c NULL.
 *     \li \c IXML_INSUFFICIENT_MEMORY: Not enough free memory exists 
 *           to complete this operation.
 */
EXPORT_SPEC int ixmlDocument_createTextNodeEx(
	/*! [in] The owner \b Document of the new node. */
	IXML_Document *doc,
	/*! [in] The data to associate with the new \b Text node.
	 * It is stored in nodeValue field.*/
	const DOMString data,      
	/*! [out] A pointer to a \b Node where the new object will be stored. */
	IXML_Node **textNode);


/*!
 * \brief Creates a new \b Text node with the given data.
 *
 * \return A pointer to the new \b Node or \c NULL on failure.
 */
EXPORT_SPEC IXML_Node *ixmlDocument_createTextNode(
	/*! [in] The owner \b Document of the new node. */
	IXML_Document *doc,
	/*! [in] The data to associate with the new \b Text node. It is stored in
	 * the nodeValue field. */
	const DOMString data);


/*!
 * \brief Creates a new \b CDATASection node with given data.
 *
 * The \b ixmlDocument_createCDATASectionEx API differs from the \b
 * ixmlDocument_createCDATASection API in that it returns an error code
 * describing the reason for failure rather than just \c NULL.
 *
 * \return An integer representing one of the following:
 *     \li \c IXML_SUCCESS: The operation completed successfully.
 *     \li \c IXML_INVALID_PARAMETER: Either \b doc or \b data 
 *           is \c NULL.
 *     \li \c IXML_INSUFFICIENT_MEMORY: Not enough free memory exists 
 *           to complete this operation.
 */
EXPORT_SPEC int ixmlDocument_createCDATASectionEx(
	/*! [in] The owner \b Document of the new node. */
	IXML_Document *doc,
	/*! [in] The data to associate with the new \b CDATASection node. */
	const DOMString data,      
	/*! [out] A pointer to a \b Node where the new object will be stored. */ 
	IXML_CDATASection** cdNode);


/*!
 * \brief Creates a new \b CDATASection node with given data.
 *
 * \return A pointer to the new \b CDATASection or \c NULL on failure.
 */
EXPORT_SPEC IXML_CDATASection *ixmlDocument_createCDATASection(
	/*! [in] The owner \b Document of the new node. */
	IXML_Document *doc,
	/*! [in] The data to associate with the new \b CDATASection node. */
	const DOMString data);


/*!
 * \brief Creates a new \b Attr node with the given name.  
 *
 * \return A pointer to the new \b Attr object with the nodeName attribute
 * set to the given name, and the localName, prefix and namespaceURI set
 * to NULL or \c NULL on failure.
 *
 * The value of the attribute is the empty string.
 */
EXPORT_SPEC IXML_Attr *ixmlDocument_createAttribute(
	/*! [in] The owner \b Document of the new node. */
	IXML_Document *doc,  
	/*! [in] The name of the new attribute. */
	const char *name);


/*!
 * \brief Creates a new \b Attr node with the given name.  
 *
 * The \b ixmlDocument_createAttributeEx API differs from the \b
 * ixmlDocument_createAttribute API in that it returns an error code
 * describing the reason for failure rather than just \c NULL.
 *
 * \return An integer representing one of the following:
 *     \li \c IXML_SUCCESS: The operation completed successfully.
 *     \li \c IXML_INVALID_PARAMETER: Either \b doc or \b name 
 *           is \c NULL.
 *     \li \c IXML_INSUFFICIENT_MEMORY: Not enough free memory exists 
 *           to complete this operation.
 */
EXPORT_SPEC int ixmlDocument_createAttributeEx(
	/*! [in] The owner \b Document of the new node. */
	IXML_Document *doc,
	/*! [in] The name of the new attribute. */
	const char *name,      
	/*! [out] A pointer to a \b Attr where the new object will be stored. */
	IXML_Attr **attrNode);


/*!
 * \brief Returns a \b NodeList of all \b Elements that match the given
 * tag name in the order in which they were encountered in a preorder
 * traversal of the \b Document tree.  
 *
 * \return A pointer to a \b NodeList containing the matching items or \c NULL
 * on an error.
 */
EXPORT_SPEC IXML_NodeList *ixmlDocument_getElementsByTagName(
	/*! [in] The \b Document to search. */
	IXML_Document *doc,
	/*! [in] The tag name to find. The special value "*" matches all tags.*/
	const DOMString tagName);


/*
 * introduced in DOM level 2
 */


/*!
 * \brief Creates a new \b Element node in the given qualified name and
 * namespace URI.
 *
 * The \b ixmlDocument_createElementNSEx API differs from the \b
 * ixmlDocument_createElementNS API in that it returns an error code
 * describing the reason for failure rather than just \c NULL.
 *
 * \return An integer representing one of the following:
 *     \li \c IXML_SUCCESS: The operation completed successfully.
 *     \li \c IXML_INVALID_PARAMETER: Either \b doc, 
 *           \b namespaceURI, or \b qualifiedName is \c NULL.
 *     \li \c IXML_INSUFFICIENT_MEMORY: Not enough free memory exists 
 *           to complete this operation.
 */
EXPORT_SPEC int ixmlDocument_createElementNSEx(
	/*! [in] The owner \b Document of the new node. */
	IXML_Document *doc,
	/*! [in] The namespace URI for the new \b Element. */
	const DOMString namespaceURI,
	/*! [in] The qualified name of the new \b Element. */
	const DOMString qualifiedName,
	/*! [out] A pointer to an \b Element where the new object will be stored. */
	IXML_Element **rtElement);


/*!
 * \brief Creates a new \b Element node in the given qualified name and
 * namespace URI.
 *
 * \return A pointer to the new \b Element object with tagName qualifiedName,
 * prefix and localName extraced from qualfiedName, nodeName of qualfiedName,
 * namespaceURI of namespaceURI or \c NULL on failure.
 */
EXPORT_SPEC IXML_Element *ixmlDocument_createElementNS(
	/*! [in] The owner \b Document of the new node. */
	IXML_Document *doc,
	/*! [in] The namespace URI for the new \b Element. */
	const DOMString namespaceURI,  
	/*! [in] The qualified name of the new \b Element. */
	const DOMString qualifiedName);


/*!
 * \brief Creates a new \b Attr node with the given qualified name and
 * namespace URI.
 *
 * The \b ixmlDocument_createAttributeNSEx API differs from the \b
 * ixmlDocument_createAttributeNS API in that it returns an error code
 * describing the reason for failure rather than just \c NULL.
 *
 * \return An integer representing one of the following:
 *     \li \c IXML_SUCCESS: The operation completed successfully.
 *     \li \c IXML_INVALID_PARAMETER: Either \b doc, 
 *           \b namespaceURI, or \b qualifiedName is \c NULL.
 *     \li \c IXML_INSUFFICIENT_MEMORY: Not enough free memory exists 
 *           to complete this operation.
 */
EXPORT_SPEC int ixmlDocument_createAttributeNSEx(
	/*! [in] The owner \b Document of the new \b Attr. */
	IXML_Document *doc,
	/*! [in] The namespace URI for the attribute. */
	const DOMString namespaceURI, 
	/*! [in] The qualified name of the attribute. */
	const DOMString qualifiedName, 
	/*! [out] A pointer to an \b Attr where the new object will be stored. */
	IXML_Attr **attrNode);


/*!
 * \brief Creates a new \b Attribute node with the given qualified name and
 * namespace URI.
 *
 * \return A pointer to the new \b Attr node with the given namespaceURI and
 * 	qualifiedName. The prefix and localname are extracted from
 * 	the qualifiedName. The node value is empty. Or \c NULL on failure.
 */
EXPORT_SPEC IXML_Attr *ixmlDocument_createAttributeNS(
	/*! [in] The owner \b Document of the new \b Attribute. */
	IXML_Document *doc,
	/*! [in] The namespace URI for the attribute. */
	const DOMString namespaceURI, 
	/*! [in] The qualified name of the attribute. */
	const DOMString qualifiedName);   


/*!
 * \brief Returns a \b NodeList of \b Elements that match the given
 * local name and namespace URI in the order they are encountered
 * in a preorder traversal of the \b Document tree.
 *
 * Either \b namespaceURI or \b localName can be the special <tt>"*"</tt>
 * character, which matches any namespace or any local name respectively.
 *
 * \return A pointer to a \b NodeList containing the matching items or \c NULL
 * on an error.
 */
EXPORT_SPEC IXML_NodeList *ixmlDocument_getElementsByTagNameNS(
	/*! [in] The \b Document to search. */
	IXML_Document *doc,
	/*! [in] The namespace of the elements to find or <tt>"*"</tt> to match any
	 * namespace. */
	const DOMString namespaceURI, 
	/*! [in] The local name of the elements to find or <tt>"*"</tt> to match any
	 * local name. */
	const DOMString localName);


/*!
 * \brief Returns the \b Element whose \c ID matches that given id.
 *
 * \return A pointer to the matching \b Element or \c NULL on an error.
 */
EXPORT_SPEC IXML_Element *ixmlDocument_getElementById(
	/*! [in] The owner \b Document of the \b Element. */
	IXML_Document *doc,
	/*! [in] The name of the \b Element.*/
	const DOMString tagName);


/*!
 * \brief Frees a \b Document object and all \b Nodes associated with it.
 *
 * Any \b Nodes extracted via any other interface function, e.g. 
 * \b ixmlDocument_GetElementById, become invalid after this call unless
 * explicitly cloned.
 */
EXPORT_SPEC void ixmlDocument_free(
	/*! [in] The \b Document to free. */
	IXML_Document *doc);


/*!
 * \brief Imports a \b Node from another \b Document into this \b Document.
 *
 * The returned new \b Node does not a have parent node (parentNode is null):
 * it is a clone of the original \b Node with the \c ownerDocument set to
 * \b doc. The source node is not altered or removed from the original 
 * document.
 *
 * For all nodes, importing a node creates a node object owned by the
 * importing document, with attribute values identical to the source
 * node's nodeName and nodeType, plus the attributes related to namespaces
 * (prefix, localName, and namespaceURI).
 *
 * As in the cloneNode operation on a node, the source node is not altered.
 * 
 * The \b deep parameter controls whether all the children of the \b Node are
 * imported.
 *
 * Refer to the DOM2-Core recommendation for details on importing specific
 * node types.
 *
 * \return An integer representing one of the following:
 *     \li \c IXML_SUCCESS: The operation completed successfully.
 *     \li \c IXML_INVALID_PARAMETER: Either \b doc or 
 *           \b importNode is not a valid pointer.
 *     \li \c IXML_NOT_SUPPORTED_ERR: \b importNode is a 
 *           \b Document, which cannot be imported.
 *     \li \c IXML_FAILED: The import operation failed because the 
 *           \b Node to be imported could not be cloned.
 */
EXPORT_SPEC int ixmlDocument_importNode(
	/*! [in] The \b Document into which to import. */
	IXML_Document *doc,
	/*! [in] The \b Node to import. */
	IXML_Node * importNode,  
	/*! [in] \c TRUE to import all children of \b importNode or \c FALSE to
	 * import only the root node. */
	BOOL deep,         
	/*! [out] A pointer to a new \b Node owned by \b doc. */
	IXML_Node **rtNode);


/* @} Interface Document */




/*!
 * \name Interface Element
 *
 * The \b Element interface represents an element in an XML document.
 * Only \b Elements are allowed to have attributes, which are stored in the
 * \c attributes member of a \b Node.  The \b Element interface
 * extends the \b Node interface and adds more operations to manipulate
 * attributes.
 *
 * @{
 */


/*!
 * \brief Initializes a \b IXML_Element node.
 */
EXPORT_SPEC void ixmlElement_init(
	/*! [in] The \b Element to initialize.*/
	IXML_Element *element);


/*!
 * \brief Returns the name of the tag as a constant string.
 *
 * \return The name of the \b Element.
 */
EXPORT_SPEC const DOMString ixmlElement_getTagName(
	/*! [in] The \b Element from which to retrieve the name. */
	IXML_Element *element);


/*!
 * \brief Retrieves an attribute of an \b Element by name.  
 *
 * \return The value of the attribute, or \b NULL if that attribute
*       does not have a specified value.
 */
EXPORT_SPEC const DOMString ixmlElement_getAttribute(
	/*! [in] The \b Element from which to retrieve the attribute. */
	IXML_Element* element,  
	/*! [in] The name of the attribute to retrieve. */
	const DOMString name);


/*!
 * \brief Adds a new attribute to an \b Element.
 *
 * If an attribute with the same name already exists in the element, the
 * attribute value will be updated with the new value parameter. Otherwise,
 * a new attribute is inserted into the element.
 *
 * \return An integer representing of the following:
 *     \li \c IXML_SUCCESS: The operation completed successfully.
 *     \li \c IXML_INVALID_PARAMETER: Either \b element, 
 *           \b name, or \b value is \c NULL.
 *     \li \c IXML_INVALID_CHARACTER_ERR: \b name contains an 
 *           illegal character.
 *     \li \c IXML_INSUFFICIENT_MEMORY: Not enough free memory exists 
 *           to complete the operation.
 */
EXPORT_SPEC int ixmlElement_setAttribute(
	/*! [in] The \b Element on which to set the attribute. */
	IXML_Element *element,
	/*! [in] The name of the attribute. */
	const DOMString name,    
	/*! [in] The value of the attribute. Note that this is a non-parsed string
	 * and any markup must be escaped. */
	const DOMString value);


/*!
 * \brief Removes an attribute value by name. The attribute node is not removed.
 *
 * \return An integer representing one of the following:
 *     \li \c IXML_SUCCESS: The operation completed successfully.
 *     \li \c IXML_INVALID_PARAMETER: Either \b element or 
 *           \b name is \c NULL.
 */
EXPORT_SPEC int ixmlElement_removeAttribute(
	/*! [in] The \b Element from which to remove the attribute. */
	IXML_Element *element,
	/*! [in] The name of the attribute to remove.  */
	const DOMString name);              


/*!
 * \brief Retrieves an attribute node by name.
 * See \b ixmlElement_getAttributeNodeNS to retrieve an attribute node using
 * a qualified name or namespace URI.
 *
 * \return A pointer to the attribute matching \b name or \c NULL on if there
 * 	is no such attribute.
 */
EXPORT_SPEC IXML_Attr *ixmlElement_getAttributeNode(
	/*! [in] The \b Element from which to get the attribute node.  */
	IXML_Element *element,
	/*! [in] The name of the attribute node to find. */
	const DOMString name);


/*!
 * \brief Adds a new attribute node to an \b Element.
 *
 * If an attribute already exists with \b newAttr as a name, it will be
 * replaced with the new one and the old one will be returned in \b rtAttr.
 *
 * \return If successfull, the replaced attribute node is returned in rtAttr,
 * otherwise, \b NULL is returned in this pointer. The function return value
 * is an integer representing one of the following:
 *     \li \c IXML_SUCCESS: The operation completed successfully.
 *     \li \c IXML_INVALID_PARAMETER: Either \b element or 
 *           \b newAttr is \c NULL.
 *     \li \c IXML_WRONG_DOCUMENT_ERR: \b newAttr does not belong 
 *           to the same one as \b element.
 *     \li \c IXML_INUSE_ATTRIBUTE_ERR: \b newAttr is already 
 *           an attribute of another \b Element.
 */
EXPORT_SPEC int ixmlElement_setAttributeNode(
	/*! [in] The \b Element in which to add the new attribute. */
	IXML_Element *element,
	/*! [in] The new \b Attr to add. */
	IXML_Attr* newAttr,     
	/*! [out] A pointer to an \b Attr where the old \b Attr will be stored.
	 * This will have a \c NULL if no prior node existed. */
	IXML_Attr** rtAttr);


/*!
 * \brief Removes the specified attribute node from an \b Element.  
 *
 * \return An integer representing one of the following:
 *     \li \c IXML_SUCCESS: The operation completed successfully.
 *     \li \c IXML_INVALID_PARAMETER: Either \b element or 
 *           \b oldAttr is \c NULL.
 *     \li \c IXML_NOT_FOUND_ERR: \b oldAttr is not among the list 
 *           attributes of \b element.
 */
EXPORT_SPEC int ixmlElement_removeAttributeNode(
	/*! [in] The \b Element from which to remove the attribute. */
	IXML_Element *element,
	/*! [in] The attribute to remove from the \b Element. */
	IXML_Attr* oldAttr,     
	/*! [out] A pointer to an attribute in which to place the removed attribute. */
	IXML_Attr** rtAttr);


/*!
 * \brief Returns a \b NodeList of all \em descendant \b Elements with
 * a given tag name, in the order in which they are encountered in a
 * pre-order traversal of this \b Element tree.
 *
 * \return A \b NodeList of the matching \b Elements or \c NULL on an error.
 */
EXPORT_SPEC IXML_NodeList *ixmlElement_getElementsByTagName(
	/*! [in] The \b Element from which to start the search. */
	IXML_Element *element,
	/*! [in] The name of the tag for which to search. */
	const DOMString tagName);


/*
 * Introduced in DOM 2
 */


/*!
 * \brief Retrieves an attribute value using the local name and namespace URI.
 *
 * \return A \b DOMString representing the value of the matching attribute, or
 * \b NULL if that attribute does not have the specified value.
 */
EXPORT_SPEC const DOMString ixmlElement_getAttributeNS(
	/*! [in] The \b Element from which to get the attribute value. */
	IXML_Element *element,
	/*! [in] The namespace URI of the attribute. */
	const DOMString namespaceURI, 
	/*! [in] The local name of the attribute. */
	const DOMString localname);


/*!
 * \brief Adds a new attribute to an \b Element using the local name and 
 * namespace URI.
 *
 * If another attribute matches the same local name and namespace, the prefix
 * is changed to be the prefix part of the \c qualifiedName and the value is
 * changed to \b value.
 *
 * \return An integer representing one of the following:
 *     \li \c IXML_SUCCESS: The operation completed successfully.
 *     \li \c IXML_INVALID_PARAMETER: Either \b element, 
 *           \b namespaceURI, \b qualifiedName, or \b value is 
 *           \c NULL.
 *     \li \c IXML_INVALID_CHARACTER_ERR: \b qualifiedName contains 
 *           an invalid character.
 *     \li \c IXML_NAMESPACE_ERR: Either the \b qualifiedName or 
 *           \b namespaceURI is malformed.  Refer to the DOM2-Core for 
 *           possible reasons.
 *     \li \c IXML_INSUFFICIENT_MEMORY: Not enough free memory exist 
 *           to complete the operation.
 *     \li \c IXML_FAILED: The operation could not be completed.
 */
EXPORT_SPEC int ixmlElement_setAttributeNS(
	/*! [in] The \b Element on which to set the attribute. */
	IXML_Element *element,
	/*! [in] The namespace URI of the new attribute. */
	const DOMString namespaceURI,   
	/*! [in] The qualified name of the attribute. */
	const DOMString qualifiedName,  
	/*! [in] The new value for the attribute. */
	const DOMString value);


/*!
 * \brief Removes an attribute using the namespace URI and local name.
 *
 * The replacing attribute has the same namespace URI and local name, as well
 * as the original prefix.
 * 
 * \return An integer representing one of the following:
 *     \li \c IXML_SUCCESS: The operation completed successfully.
 *     \li \c IXML_INVALID_PARAMETER: Either \b element, 
 *           \b namespaceURI, or \b localName is \c NULL.
 */
EXPORT_SPEC int ixmlElement_removeAttributeNS(
	/*! [in] The \b Element from which to remove the the attribute. */
	IXML_Element *element,
	/*! [in] The namespace URI of the attribute. */
	const DOMString namespaceURI,  
	/*! [in] The local name of the attribute.*/
	const DOMString localName);


/*!
 * \brief Retrieves an \b Attr node by local name and namespace URI.
 *
 * \return A pointer to an \b Attribute node  with the specified attribute
 * 	local name and namespace URI or \c NULL if there is no such attribute.
 */
EXPORT_SPEC IXML_Attr *ixmlElement_getAttributeNodeNS(
	/*! [in] The \b Element from which to get the attribute. */
	IXML_Element *element,
	/*! [in] The namespace URI of the attribute. */
	const DOMString namespaceURI,  
	/*! [in] The local name of the attribute. */
	const DOMString localName);


/*!
 * \brief Adds a new attribute node to the element node specified.
 *
 * If an attribute with the same local name and namespace URI already exists in
 * the \b Element, the existing attribute node is replaced with \b newAttr and
 * the old returned in \b rcAttr.
 *
 * \return The output parameter rcAttr receives the replaced attribute node if
 * the newAttr attribute replaces an existing attribute with the same local name
 * and namespace, otherwise rcAttr receives \b NULL.
 *
 * The function return value is an integer representing one of the following:
 *     \li \c IXML_SUCCESS: The operation completed successfully.
 *     \li \c IXML_INVALID_PARAMETER: Either \b element or 
 *           \b newAttr is \c NULL.
 *     \li \c IXML_WRONG_DOCUMENT_ERR: \b newAttr does not belong 
 *           to the same document as \b element.
 *     \li \c IXML_INUSE_ATTRIBUTE_ERR: \b newAttr already is an 
 *           attribute of another \b Element.
 */
EXPORT_SPEC int ixmlElement_setAttributeNodeNS(
	/*! [in] The \b Element in which to add the attribute node. */
	IXML_Element *element,
	/*! [in] The new \b Attr to add. */
	IXML_Attr *newAttr,     
	/*! [out] A pointer to the replaced \b Attr, if it exists. */
	IXML_Attr **rcAttr);


/*!
 * \brief Returns a \b NodeList of all \em descendant \b Elements with a
 * given local name and namespace in the order in which they are encountered in
 * the pre-order traversal of the \b Element tree.
 *
 * \return A \b NodeList of matching \b Elements or \c NULL on an error.
 */
EXPORT_SPEC IXML_NodeList *ixmlElement_getElementsByTagNameNS(
	/*! [in] The \b Element from which to start the search. */
	IXML_Element *element,
	/*! [in] The namespace URI of the \b Elements to find.  The special value
	 * "*" matches all namespaces. */
	const DOMString namespaceURI,
	/*! [in] The local name of the \b Elements to find. The special value "*"
	 * matches all local names. */
	const DOMString localName);


/*!
 * \brief Queries whether the \b Element has an attribute with the given name
 * or a default value.
 *
 * \return \c TRUE if the \b Element has an attribute with this name or has a
 * default value for that attribute, otherwise \c FALSE.
 */
EXPORT_SPEC BOOL ixmlElement_hasAttribute(
	/*! [in] The \b Element on which to check for an attribute. */
	IXML_Element *element,
	/*! [in] The name of the attribute for which to check. */
	const DOMString name);


/*!
 * \brief Queries whether the \b Element has an attribute with the given
 * local name and namespace URI or has a default value for that attribute.
 *
 * \return \c TRUE if the \b Element has an attribute with the given namespace
 * and local name or has a default value for that attribute, otherwise \c FALSE.
 */
EXPORT_SPEC BOOL ixmlElement_hasAttributeNS(
	/*! [in] The \b Element on which to check for the attribute. */
	IXML_Element *element,
	/*! [in] The namespace URI of the attribute. */
	const DOMString namespaceURI, 
	/*! [in] The local name of the attribute. */
	const DOMString localName);


/*!
 * \brief Frees the given \b Element and any subtree of the \b Element.
 */
EXPORT_SPEC void ixmlElement_free(
	/*! [in] The \b Element to free. */
	IXML_Element *element);


/* @} Interface Element */



/*!
 * \name Interface NamedNodeMap
 *
 * A \b NamedNodeMap object represents a list of objects that can be
 * accessed by name.  A \b NamedNodeMap maintains the objects in 
 * no particular order.  The \b Node interface uses a \b NamedNodeMap
 * to maintain the attributes of a node.
 *
 * @{
 */


/*!
 * \brief Returns the number of items contained in this \b NamedNodeMap.
 *
 * \return The number of nodes in this map.
 */
EXPORT_SPEC unsigned long ixmlNamedNodeMap_getLength(
	/*! [in] The \b NamedNodeMap from which to retrieve the size. */
	IXML_NamedNodeMap *nnMap);


/*!
 * \brief Retrieves a \b Node from the \b NamedNodeMap by name.
 *
 * \return A Node with the specified nodeName, or \b NULL if it
 * 	does not identify any node in this map.
 */
EXPORT_SPEC IXML_Node *ixmlNamedNodeMap_getNamedItem(
	/*! [in] The \b NamedNodeMap to search. */
	IXML_NamedNodeMap *nnMap,
	/*! [in] The name of the \b Node to find. */
	const DOMString name);


/*!
 * \brief Adds a new \b Node to the \b NamedNodeMap using the \b Node name
 * attribute.
 *
 * \return The old \b Node if the new \b Node replaces it or \c NULL if the
 * \b Node was not in the \b NamedNodeMap before.
 */
EXPORT_SPEC IXML_Node *ixmlNamedNodeMap_setNamedItem(
	/*! The \b NamedNodeMap in which to add the new \b Node. */
	IXML_NamedNodeMap *nnMap,
	/*! The new \b Node to add to the \b NamedNodeMap. */
	IXML_Node *arg);


/*!
 * \brief Removes a \b Node from a \b NamedNodeMap specified by name.
 *
 * \return A pointer to the \b Node, if found, or \c NULL if it wasn't.
 */
EXPORT_SPEC IXML_Node *ixmlNamedNodeMap_removeNamedItem(
	/*! The \b NamedNodeMap from which to remove the item. */
	IXML_NamedNodeMap *nnMap,
	/*! The name of the item to remove. */
	const DOMString name);


/*!
 * \brief Retrieves the indexth item in the map. If index is greater than or
 * equal to the number of nodes in this map, this returns \b NULL.
 *
 * \return The node at the indexth position in the map, or \b NULL if that is
 * 	not a valid index.
 */
EXPORT_SPEC IXML_Node *ixmlNamedNodeMap_item(
	/*! [in] The \b NamedNodeMap from which to remove the \b Node. */
	IXML_NamedNodeMap *nnMap,
	/*! [in] The index into the map to remove. */
	unsigned long index);


/*
 * Introduced in DOM level 2
 */


/*!
 * \brief Retrieves a \b Node from a \b NamedNodeMap specified by namespace
 * URI and local name.
 *
 * \return A pointer to the \b Node, if found, or \c NULL if it wasn't
 */
EXPORT_SPEC IXML_Node *ixmlNamedNodeMap_getNamedItemNS(
	/*! The \b NamedNodeMap from which to remove the \b Node. */
	IXML_NamedNodeMap *nnMap,
	/*! The namespace URI of the \b Node to remove. */
	const DOMString *namespaceURI,
	/*! The local name of the \b Node to remove. */
	const DOMString localName);


/*!
 * \brief Adds a new \b Node to the \b NamedNodeMap using the \b Node 
 * local name and namespace URI attributes.
 *
 * \return The old \b Node if the new \b Node replaces it or \c NULL if the
 * \b Node was not in the \b NamedNodeMap before.
 */
EXPORT_SPEC IXML_Node *ixmlNamedNodeMap_setNamedItemNS(
	/*! The \b NamedNodeMap in which to add the \b Node. */
	IXML_NamedNodeMap *nnMap,
	/*! The \b Node to add to the map. */
	IXML_Node *arg);


/*!
 * \brief Removes a \b Node from a \b NamedNodeMap specified by 
 * namespace URI and local name.
 *
 * \return A pointer to the \b Node, if found, or \c NULL if it wasn't.
 */
EXPORT_SPEC IXML_Node *ixmlNamedNodeMap_removeNamedItemNS(
	/*! The \b NamedNodeMap from which to remove the \b Node. */
	IXML_NamedNodeMap *nnMap,
	/*! The namespace URI of the \b Node to remove. */
	const DOMString namespaceURI, 
	/*! The local name of the \b Node to remove. */
	const DOMString localName);


/*! \brief Frees a \b NamedNodeMap.
 *
 * The \b Nodes inside the map are not freed, just the \b NamedNodeMap object.
 */
EXPORT_SPEC void ixmlNamedNodeMap_free(
	/*! [in] The \b NamedNodeMap to free. */
	IXML_NamedNodeMap *nnMap);


/* @} Interface NodeMap */



/*!
 * \name Interface NodeList
 *
 * The \b NodeList interface abstracts an ordered collection of
 * nodes.  Note that changes to the underlying nodes will change
 * the nodes contained in a \b NodeList.  The DOM2-Core refers to
 * this as being \em live.
 *
 * @{
 */


/*!
 * \brief Retrieves a \b Node from a \b NodeList specified by a 
 * numerical index.
 *
 * \return A pointer to a \b Node or \c NULL if there was an error.
 */
EXPORT_SPEC IXML_Node *ixmlNodeList_item(
	/*! [in] The \b NodeList from which to retrieve the \b Node. */
	IXML_NodeList *nList,
	/*! [in] The index into the \b NodeList to retrieve. */
	unsigned long index);


/*!
 * \brief Returns the number of \b Nodes in a \b NodeList.
 *
 * \return The number of \b Nodes in the \b NodeList.
 */
EXPORT_SPEC unsigned long ixmlNodeList_length(
	/*! [in] The \b NodeList for which to retrieve the number of \b Nodes. */
	IXML_NodeList *nList);


/*!
 * \brief Frees a \b NodeList object.
 *
 * Since the underlying \b Nodes are references, they are not freed using this
 * operation. This only frees the \b NodeList object.
 */
EXPORT_SPEC void ixmlNodeList_free(
	/*! [in] The \b NodeList to free.  */
	IXML_NodeList *nList);


/* @} Interface NodeList */



/*!
 * \name IXML API
 *
 * The IXML API contains utility functions that are not part of the standard
 * DOM interfaces. They include functions to create a DOM structure from a
 * file or buffer, create an XML file from a DOM structure, and manipulate
 * DOMString objects.
 *
 * @{
 */


/*!
 * \brief Renders a \b Node and all sub-elements into an XML document
 * representation.
 *
 * The caller is required to free the \b DOMString
 * returned from this function using \b ixmlFreeDOMString when it
 * is no longer required.
 *
 * Note that this function can be used for any \b Node-derived
 * interface.  The difference between \b ixmlPrintDocument and
 * \b ixmlPrintNode is \b ixmlPrintDocument includes the XML prolog
 * while \b ixmlPrintNode only produces XML elements. An XML
 * document is not well formed unless it includes the prolog
 * and at least one element.
 *
 * This function  introduces lots of white space to print the
 * \b DOMString in readable  format.
 *
 * \return A \b DOMString with the XML document representation 
 *                     of the DOM tree or \c NULL on an error.
 */
EXPORT_SPEC DOMString ixmlPrintDocument(
	/*! [in] The document node to print. */
	IXML_Document *doc);


/*!
 * \brief Renders a \b Node and all sub-elements into an XML text
 * representation.
 *
 * The caller is required to free the \b DOMString
 * returned from this function using \b ixmlFreeDOMString when it
 * is no longer required.
 *
 * Note that this function can be used for any \b Node-derived
 * interface.  A similar \b ixmlPrintDocument function is defined
 * to avoid casting when printing whole documents. This function
 * introduces lots of white space to print the \b DOMString in readable
 * format.
 *
 * \return A \b DOMString with the XML text representation of the DOM tree or
 * \c NULL on an error.
 */
EXPORT_SPEC DOMString ixmlPrintNode(
	/*! [in] The root of the \b Node tree to render to XML text. */
	IXML_Node *doc
);


/*!
 * \brief Renders a \b Node and all sub-elements into an XML document
 * representation.
 *
 * The caller is required to free the \b DOMString
 * returned from this function using \b ixmlFreeDOMString when it
 * is no longer required.
 *
 * Note that this function can be used for any \b Node-derived
 * interface.  The difference between \b ixmlDocumenttoString and
 * \b ixmlNodetoString is \b ixmlDocumenttoString includes the XML
 * prolog while \b ixmlNodetoString only produces XML elements. An XML
 * document is not well formed unless it includes the prolog
 * and at least one element.
 *
 * \return A \b DOMString with the XML text representation of the DOM tree or
 * \c NULL on an error.
 */
EXPORT_SPEC DOMString ixmlDocumenttoString(
	/*! [in] The root of the \b Node tree to render to XML text. */
	IXML_Document *doc);


/*!
 * \brief Renders a \b Node and all sub-elements into an XML text
 * representation.  The caller is required to free the \b DOMString
 * returned from this function using \b ixmlFreeDOMString when it
 * is no longer required.
 *
 * Note that this function can be used for any \b Node-derived
 * interface.  The difference between \b ixmlNodetoString and
 * \b ixmlDocumenttoString is \b ixmlNodetoString does not include
 * the XML prolog, it only produces XML elements.
 *
 * \return A \b DOMString with the XML text representation of the DOM tree or
 * \c NULL on an error.
 */
EXPORT_SPEC DOMString ixmlNodetoString(
	/*! [in] The root of the \b Node tree to render to XML text. */
	IXML_Node *doc);


/*!
 * \brief Makes the XML parser more tolerant to malformed text.
 */
EXPORT_SPEC void ixmlRelaxParser(
	/*! [in] If \b errorChar is 0 (default), the parser is strict about XML 
	 * encoding : invalid UTF-8 sequences or "&" entities are rejected, and 
	 * the parsing aborts.
	 *
	 * If \b errorChar is not 0, the parser is relaxed: invalid UTF-8 
	 * characters are replaced by the \b errorChar, and invalid "&" entities 
	 * are left untranslated. The parsing is then allowed to continue.
	 */
	char errorChar);


/*!
 * \brief Parses an XML text buffer converting it into an IXML DOM representation.
 *
 * \return A \b Document if the buffer correctly parses or \c NULL on an error. 
 */
EXPORT_SPEC IXML_Document *ixmlParseBuffer(
	/*! [in] The buffer that contains the XML text to convert to a \b Document. */
	const char *buffer);


/*!
 * \brief Parses an XML text buffer converting it into an IXML DOM representation.
 *
 * The \b ixmlParseBufferEx API differs from the \b ixmlParseBuffer
 * API in that it returns an error code representing the actual failure
 * rather than just \c NULL.
 *
 * \return An integer representing one of the following:
 *     \li \c IXML_SUCCESS: The operation completed successfully.
 *     \li \c IXML_INVALID_PARAMETER: The \b buffer is not a valid 
 *           pointer.
 *     \li \c IXML_INSUFFICIENT_MEMORY: Not enough free memory exists 
 *           to complete this operation.
 */
EXPORT_SPEC int ixmlParseBufferEx(
	/*! [in] The buffer that contains the XML text to convert to a \b Document. */
	const char *buffer,
	/*! [out] A point to store the \b Document if file correctly parses or \b NULL on an error. */
	IXML_Document** doc);


/*!
 * \brief Parses an XML text file converting it into an IXML DOM representation.
 *
 * \return A \b Document if the file correctly parses or \c NULL on an error.
 */
EXPORT_SPEC IXML_Document *ixmlLoadDocument(
	/*! [in] The filename of the XML text to convert to a \b Document. */
	const char* xmlFile);


/*!
 * \brief Parses an XML text file converting it into an IXML DOM representation.
 *
 * The \b ixmlLoadDocumentEx API differs from the \b ixmlLoadDocument
 * API in that it returns a an error code representing the actual failure
 * rather than just \c NULL.
 *
 * \return An integer representing one of the following:
 *     \li \c IXML_SUCCESS: The operation completed successfully.
 *     \li \c IXML_INVALID_PARAMETER: The \b xmlFile is not a valid 
 *           pointer.
 *     \li \c IXML_INSUFFICIENT_MEMORY: Not enough free memory exists 
 *           to complete this operation.
 */
EXPORT_SPEC int ixmlLoadDocumentEx(
	/*! [in] The filename of the XML text to convert to a \b Document. */
	const char *xmlFile,
	/*! [out] A pointer to the \b Document if file correctly parses or \b NULL
	 * on an error. */
	IXML_Document **doc);


/*!
 * \brief Clones an existing \b DOMString.
 *
 * \return A new \b DOMString that is a duplicate of the original or \c NULL
 * if the operation could not be completed.
 */
EXPORT_SPEC DOMString ixmlCloneDOMString(
	/*! [in] The source \b DOMString to clone. */
	const DOMString src);


/*!
 * \brief Frees a \b DOMString.
 */
EXPORT_SPEC void ixmlFreeDOMString(
	/*! [in] The \b DOMString to free. */
	DOMString buf);


/* @} IXML API */


#ifdef __cplusplus
}
#endif


/* @} XMLAPI XML API */


#endif  /* IXML_H */

