/*!*****************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation 
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met: 
 *
 * * Redistributions of source code must retain the above copyright notice, 
 * this list of conditions and the following disclaimer. 
 * * Redistributions in binary form must reproduce the above copyright notice, 
 * this list of conditions and the following disclaimer in the documentation 
 * and/or other materials provided with the distribution. 
 * * Neither name of Intel Corporation nor the names of its contributors 
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

#ifndef IXML_H
#define IXML_H


/*! \file */


/*! \defgroup XMLApi The XML API */
/*@{*/


#include <stdio.h>
#include <string.h>
#include <assert.h>


#ifdef WIN32
	#ifndef UPNP_STATIC_LIB
		#ifdef LIBUPNP_EXPORTS
			/* set up declspec for dll export to make functions visible to library users */
			#define EXPORT_SPEC __declspec(dllexport)
		#else
			#define EXPORT_SPEC __declspec(dllimport)
		#endif
	#else
		#define EXPORT_SPEC
	#endif
#else
	#define EXPORT_SPEC
#endif

typedef int BOOL;


#define DOMString   char *


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


/*! \name DOM Interfaces 
 * The Document Object Model consists of a set of objects and interfaces
 * for accessing and manipulating documents.  IXML does not implement all
 * the interfaces documented in the DOM2-Core recommendation but defines
 * a subset of the most useful interfaces.  A description of the supported
 * interfaces and methods is presented in this section.
 *
 * For a complete discussion on the object model, the object hierarchy,
 * etc., refer to section 1.1 of the DOM2-Core recommendation.
 */
/*@{*/

/*******************************************************************************
 *
 *   DOM node type
 *
 ******************************************************************************/
typedef enum
{
    eINVALID_NODE                   = 0,
    eELEMENT_NODE                   = 1,
    eATTRIBUTE_NODE                 = 2,
    eTEXT_NODE                      = 3,
    eCDATA_SECTION_NODE             = 4,
    eENTITY_REFERENCE_NODE          = 5,
    eENTITY_NODE                    = 6,                
    ePROCESSING_INSTRUCTION_NODE    = 7,
    eCOMMENT_NODE                   = 8,
    eDOCUMENT_NODE                  = 9,
    eDOCUMENT_TYPE_NODE             = 10,
    eDOCUMENT_FRAGMENT_NODE         = 11,
    eNOTATION_NODE                  = 12,

}   IXML_NODE_TYPE;

/*******************************************************************************
 *
 *   Error Codes
 *
 ******************************************************************************/
typedef enum 
{   /* see DOM spec */
    IXML_SUCCESS                        = 0,

    IXML_INDEX_SIZE_ERR                 = 1,
    IXML_DOMSTRING_SIZE_ERR             = 2,
    IXML_HIERARCHY_REQUEST_ERR          = 3,
    IXML_WRONG_DOCUMENT_ERR             = 4,
    IXML_INVALID_CHARACTER_ERR          = 5,
    IXML_NO_DATA_ALLOWED_ERR            = 6,
    IXML_NO_MODIFICATION_ALLOWED_ERR    = 7,
    IXML_NOT_FOUND_ERR                  = 8,
    IXML_NOT_SUPPORTED_ERR              = 9,
    IXML_INUSE_ATTRIBUTE_ERR            = 10,
    IXML_INVALID_STATE_ERR              = 11,
    IXML_SYNTAX_ERR                     = 12,
    IXML_INVALID_MODIFICATION_ERR       = 13,
    IXML_NAMESPACE_ERR                  = 14,
    IXML_INVALID_ACCESS_ERR             = 15,

    IXML_NO_SUCH_FILE                   = 101,
    IXML_INSUFFICIENT_MEMORY            = 102,
    IXML_FILE_DONE                      = 104,
    IXML_INVALID_PARAMETER              = 105,
    IXML_FAILED                         = 106,
    IXML_INVALID_ITEM_NUMBER            = 107,
} IXML_ERRORCODE;


#define DOCUMENTNODENAME    "#document"
#define TEXTNODENAME        "#text"
#define CDATANODENAME       "#cdata-section"

/*******************************************************************************
 *
 *   DOM data structures
 *
 ******************************************************************************/

typedef struct _IXML_Document *Docptr;

typedef struct _IXML_Node    *Nodeptr;
typedef struct _IXML_Node
{
    DOMString       nodeName;
    DOMString       nodeValue;
    IXML_NODE_TYPE  nodeType;
    DOMString       namespaceURI;
    DOMString       prefix;
    DOMString       localName;
    BOOL            readOnly;

    Nodeptr         parentNode;
    Nodeptr         firstChild;
    Nodeptr         prevSibling;
    Nodeptr         nextSibling;
    Nodeptr         firstAttr;
    Docptr          ownerDocument;

} IXML_Node;

typedef struct _IXML_Document
{
    IXML_Node    n;
} IXML_Document;

typedef struct _IXML_CDATASection
{
    IXML_Node    n;
} IXML_CDATASection;

typedef struct _IXML_Element
{
    IXML_Node   n;
    DOMString   tagName;

} IXML_Element;

typedef struct _IXML_ATTR
{
    IXML_Node   n;
    BOOL        specified;
    IXML_Element *ownerElement;
} IXML_Attr;

typedef struct _IXML_Text
{
    IXML_Node   n;
} IXML_Text;

typedef struct _IXML_NodeList
{
    IXML_Node    *nodeItem;
    struct  _IXML_NodeList *next;
} IXML_NodeList;


typedef struct _IXML_NamedNodeMap
{
    IXML_Node                 *nodeItem;
    struct _IXML_NamedNodeMap *next;
} IXML_NamedNodeMap;

/*@}*/ /* DOM Interfaces */


#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 *
 *   NODE interfaces
 *
 ******************************************************************************/

/*! \name Interface Node
 * The \b Node interface forms the primary datatype for all other DOM 
 * objects. Every other interface is derived from this interface, inheriting 
 * its functionality. For more information, refer to DOM2-Core page 34.
 */
/*@{*/

/*! \brief Returns the name of the \b Node, depending on what type of 
*  \b Node it is, in a read-only string. Refer to the table in the 
*  DOM2-Core for a description of the node names for various interfaces.
*
*  \return A constant \b DOMString of the node name.
*/
EXPORT_SPEC const DOMString ixmlNode_getNodeName(
	/*! Pointer to the node to retrieve the name. */
	IXML_Node *nodeptr);


/*! \brief Returns the value of the \b Node as a string. Note that this string
 *  is not a copy and modifying it will modify the value of the \b Node.
 *
 *  \return A \b DOMString of the \b Node value.
 */
EXPORT_SPEC const DOMString ixmlNode_getNodeValue(
	/*! Pointer to the \b Node to retrieve the value. */
	IXML_Node *nodeptr);


/*! \brief Assigns a new value to a \b Node.
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
	/*! The \b Node to which to assign a new value. */
	IXML_Node *nodeptr, 
	/*! The new value of the \b Node. */
	const char *newNodeValue);


/*! \brief Retrieves the type of a \b Node.
 *
 * The defined \b Node constants are:
 *    \li \c eATTRIBUTE_NODE 
 *    \li \c eCDATA_SECTION_NODE
 *    \li \c eCOMMENT_NODE
 *    \li \c eDOCUMENT_FRAGMENT_NODE
 *    \li \c eDOCUMENT_NODE
 *    \li \c eDOCUMENT_TYPE_NODE
 *    \li \c eELEMENT_NODE 
 *    \li \c eENTITY_NODE
 *    \li \c eENTITY_REFERENCE_NODE
 *    \li \c eNOTATION_NODE
 *    \li \c ePROCESSING_INSTRUCTION_NODE
 *    \li \c eTEXT_NODE
 *
 *  \return An integer representing the type of the \b Node.
 */
EXPORT_SPEC unsigned short ixmlNode_getNodeType(
	/*! The \b Node from which to retrieve the type. */
	IXML_Node *nodeptr);


/*! \brief Retrieves the parent \b Node for a \b Node.
 *
 * \return A pointer to the parent \b Node or \c NULL if the \b Node has no
 *	parent.
 */
EXPORT_SPEC IXML_Node *ixmlNode_getParentNode(
	/*! The \b Node from which to retrieve the parent. */ 
	IXML_Node *nodeptr);


/*! \brief Retrieves the list of children of a \b Node in a \b NodeList 
 * structure.
 *
 * If a \b Node has no children, \b ixmlNode_getChildNodes 
 * returns a \b NodeList structure that contains no \b Nodes.
 *
 * \return A \b NodeList of the children of the \b Node.
 */
EXPORT_SPEC IXML_NodeList *               
ixmlNode_getChildNodes(
	/*! The \b Node from which to retrieve the children. */
	IXML_Node *nodeptr);


/*! \brief Retrieves the first child \b Node of a \b Node.
 *
 * \return A pointer to the first child \b Node or \c NULL if the \b Node does
 * not have any children.
 */
EXPORT_SPEC IXML_Node *ixmlNode_getFirstChild(
	/*! The \b Node from which to retrieve the first child. */ 
	IXML_Node *nodeptr);


/*! \brief Retrieves the last child \b Node of a \b Node.
 *
 * \return A pointer to the last child \b Node or \c NULL if the \b Node does
 * not have any children.
 */
EXPORT_SPEC IXML_Node *ixmlNode_getLastChild(
	/*! The \b Node from which to retrieve the last child. */
	IXML_Node *nodeptr);


/*! \brief Retrieves the sibling \b Node immediately preceding this \b Node.
 *
 * \return A pointer to the previous sibling \b Node or \c NULL if no such
 * \b Node exists.
 */
EXPORT_SPEC IXML_Node *ixmlNode_getPreviousSibling(
	/*! The \b Node for which to retrieve the previous sibling. */
	IXML_Node *nodeptr);


/*! \brief Retrieves the sibling \b Node immediately following this \b Node.
 *
 *  \return A pointer to the next sibling \b Node or \c NULL if no such
 *  \b Node exists.
 */
EXPORT_SPEC IXML_Node *ixmlNode_getNextSibling(
	/*! The \b Node from which to retrieve the next sibling. */ 
	IXML_Node *nodeptr);


  /*! \brief Retrieves the attributes of a \b Node, if it is an \b Element node,
   *  in a \b NamedNodeMap structure.
   *
   *  \return A \b NamedNodeMap of the attributes or \c NULL.
   */

EXPORT_SPEC IXML_NamedNodeMap*           
ixmlNode_getAttributes(IXML_Node *nodeptr  
		         /*! The \b Node from which to retrieve the 
			     attributes. */ 
                   );

  /*! \brief Retrieves the document object associated with this \b Node.  This 
   *  owner document \b Node allows other \b Nodes to be created in the 
   *  context of this document.  Note that \b Document nodes do not have 
   *  an owner document.
   *
   *  \return [Document*] A pointer to the owning \b Document or 
   *                      \c NULL, if the \b Node does not have an owner.
   */

EXPORT_SPEC IXML_Document*               
ixmlNode_getOwnerDocument(IXML_Node *nodeptr  
		            /*! The \b Node from which to retrieve the 
			        owner document. */
                      );

  /*! \brief Retrieves the namespace URI for a \b Node as a \b DOMString.  Only
   *  \b Nodes of type \c eELEMENT_NODE or \c eATTRIBUTE_NODE can 
   *  have a namespace URI.  \b Nodes created through the \b Document 
   *  interface will only contain a namespace if created using 
   *  \b ixmlDocument_createElementNS.
   *
   *  \return A \b DOMString representing the URI of the namespace or \c NULL.
   */

EXPORT_SPEC const DOMString         
ixmlNode_getNamespaceURI(IXML_Node *nodeptr  
		           /*! The \b Node for which to retrieve the 
			       namespace. */
                     );

  /*! \brief Retrieves the namespace prefix, if present.  The prefix is the name
   *  used as an alias for the namespace URI for this element.  Only 
   *  \b Nodes of type \c eELEMENT_NODE or \c eATTRIBUTE_NODE can have 
   *  a prefix. \b Nodes created through the \b Document interface will 
   *  only contain a prefix if created using \b ixmlDocument_createElementNS.
   *
   *  \return A \b DOMString representing the namespace prefix or \c NULL.
   */

EXPORT_SPEC const DOMString               
ixmlNode_getPrefix(IXML_Node *nodeptr  
		     /*! The \b Node from which to retrieve the prefix. */
               );

  /*! \brief Retrieves the local name of a \b Node, if present.  The local name is
   *  the tag name without the namespace prefix.  Only \b Nodes of type
   *  \c eELEMENT_NODE or \c eATTRIBUTE_NODE can have a local name.
   *  \b Nodes created through the \b Document interface will only 
   *  contain a local name if created using \b ixmlDocument_createElementNS.
   *
   *  \return [const DOMString] A \b DOMString representing the local name 
   *                            of the \b Element or \c NULL.
   */

EXPORT_SPEC const DOMString         
ixmlNode_getLocalName(IXML_Node *nodeptr  
		        /*! The \b Node from which to retrieve the local 
			    name. */
                  );

  /*! \brief Inserts a new child \b Node before the existing child \b Node.  
   *  \b refChild can be \c NULL, which inserts \b newChild at the
   *  end of the list of children.  Note that the \b Node (or \b Nodes) 
   *  in \b newChild must already be owned by the owner document (or have no
   *  owner at all) of \b nodeptr for insertion.  If not, the \b Node 
   *  (or \b Nodes) must be imported into the document using 
   *  \b ixmlDocument_importNode.  If \b newChild is already in the tree,
   *  it is removed first.
   *
   *  \return [int] An integer representing one of the following:
   *      \li \c IXML_SUCCESS: The operation completed successfully.
   *      \li \c IXML_INVALID_PARAMETER: Either \b nodeptr or 
   *            \b newChild is \c NULL.
   *      \li \c IXML_HIERARCHY_REQUEST_ERR: The type of the \b Node 
   *            does not allow children of the type of \b newChild.
   *      \li \c IXML_WRONG_DOCUMENT_ERR: \b newChild has an owner 
   *            document that does not match the owner of \b nodeptr.
   *      \li \c IXML_NO_MODIFICATION_ALLOWED_ERR: \b nodeptr is 
   *            read-only or the parent of the \b Node being inserted is 
   *            read-only.
   *      \li \c IXML_NOT_FOUND_ERR: \b refChild is not a child of 
   *            \b nodeptr.
   */

EXPORT_SPEC int     
ixmlNode_insertBefore(IXML_Node *nodeptr,   
		        /*! The parent of the \b Node before which to 
			    insert the new child. */
                      IXML_Node* newChild,      
		        /*! The \b Node to insert into the tree. */
                      IXML_Node* refChild       
		        /*! The reference child where the new \b Node 
			    should be inserted. The new \b Node will
			    appear directly before the reference child. */
                  );

  /*! \brief Replaces an existing child \b Node with a new child \b Node in 
   *  the list of children of a \b Node. If \b newChild is already in 
   *  the tree, it will first be removed. \b returnNode will contain the 
   *  \b oldChild \b Node, appropriately removed from the tree (i.e. it 
   *  will no longer have an owner document).
   *
   *  \return [int] An integer representing one of the following:
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

EXPORT_SPEC int     
ixmlNode_replaceChild(IXML_Node *nodeptr,     
		        /*! The parent of the \b Node which contains the 
			    child to replace. */
                      IXML_Node* newChild,        
		        /*! The child with which to replace \b oldChild. */
                      IXML_Node* oldChild,        
		        /*! The child to replace with \b newChild. */
                      IXML_Node** returnNode      
		        /*! Pointer to a \b Node to place the removed \b oldChild \b Node. */
                  );

  /*! \brief Removes a child from the list of children of a \b Node.
   *  \b returnNode will contain the \b oldChild \b Node, 
   *  appropriately removed from the tree (i.e. it will no longer have an 
   *  owner document).
   *
   *  \return [int] An integer representing one of the following:
   *      \li \c IXML_SUCCESS: The operation completed successfully.
   *      \li \c IXML_INVALID_PARAMETER: Either \b nodeptr or 
   *            \b oldChild is \c NULL.
   *      \li \c IXML_NO_MODIFICATION_ALLOWED_ERR: \b nodeptr or its 
   *            parent is read-only.
   *      \li \c IXML_NOT_FOUND_ERR: \b oldChild is not among the 
   *            children of \b nodeptr.
   */

EXPORT_SPEC int     
ixmlNode_removeChild(IXML_Node *nodeptr,     
		       /*! The parent of the child to remove. */
                     IXML_Node* oldChild,  
		       /*! The child \b Node to remove. */
                     IXML_Node **returnNode
		       /*! Pointer to a \b Node to place the removed \b oldChild \b Node. */
                 );


  /*! \brief Appends a child \b Node to the list of children of a \b Node.  If
   *  \b newChild is already in the tree, it is removed first.
   *
   *  \return [int] An integer representing one of the following:
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
EXPORT_SPEC int     
ixmlNode_appendChild(IXML_Node *nodeptr,  
		       /*! The \b Node in which to append the new child. */
                     IXML_Node* newChild      
		       /*! The new child to append. */
                 );


  /*! \brief Queries whether or not a \b Node has children.
   *
   *  \return \c TRUE if the \b Node has one or more children otherwise \c FALSE.
   */
EXPORT_SPEC BOOL    
ixmlNode_hasChildNodes(IXML_Node *nodeptr  
		         /*! The \b Node to query for children. */
                   );

/*! \brief Clones a \b Node.  The new \b Node does not have a parent. The
 *  \b deep parameter controls whether the subtree of the \b Node is
 *  also cloned.  For details on cloning specific types of \b Nodes, 
 *  refer to the DOM2-Core recommendation.
 *
 *  \return A clone of \b nodeptr or \c NULL.
 */
EXPORT_SPEC IXML_Node*   
ixmlNode_cloneNode(IXML_Node *nodeptr,  
		     /*! The \b Node to clone.  */
                   BOOL deep
		     /*! \c TRUE to clone the subtree also or \c FALSE 
		         to clone only \b nodeptr. */
                  );


/*! \brief Queries whether this \b Node has attributes.  Note that only 
 *  \b Element nodes have attributes.
 *
 *  \return \c TRUE if the \b Node has attributes otherwise \c FALSE.
 */
EXPORT_SPEC BOOL    
ixmlNode_hasAttributes(
	/*! The \b Node to query for attributes. */
	IXML_Node *nodeptr);


/*! \brief Frees a \b Node and all \b Nodes in its subtree.
 */
EXPORT_SPEC void ixmlNode_free(
	/*! The \b Node to free. */
	IXML_Node *nodeptr);

/*@}*/ /* Node Interface */

/*================================================================
*
*   Attribute interfaces
*
*
*=================================================================*/

/*! \name Interface Attr
 * The \b Attr interface represents an attribute of an \b Element.
 * The document type definition (DTD) or schema usually dictate the
 * allowable attributes and values for a particular element.  For more 
 * information, refer to the <em>Interface Attr</em> section in the DOM2-Core.
 */
/*@{*/


  /*! Frees an \b Attr node.
   *
   *  \return [void] This function does not return a value.
   */

EXPORT_SPEC void    
ixmlAttr_free(IXML_Attr *attrNode  
		/*! The \b Attr node to free.  */
             );

/*@}*/ /* Interface Attr */


/*================================================================
*
*   CDATASection interfaces
*
*
*=================================================================*/

/*! \name Interface CDATASection
 * The \b CDATASection is used to escape blocks of text containing
 * characters that would otherwise be regarded as markup. CDATA sections
 * cannot be nested. Their primary purpose is for including material such
 * XML fragments, without needing to escape all the delimiters.  For more 
 * information, refer to the <em>Interface CDATASection</em> section in the
 * DOM2-Core.
 */
/*@{*/


  /*! Initializes a \b CDATASection node.
   *
   *  \return [void] This function does not return a value.
   */

EXPORT_SPEC void    
ixmlCDATASection_init(IXML_CDATASection *nodeptr  
		        /*! The \b CDATASection node to initialize.  */
                     );


  /*! Frees a \b CDATASection node.
   *
   *  \return [void] This function does not return a value.
   */

EXPORT_SPEC void    
ixmlCDATASection_free(IXML_CDATASection *nodeptr  
		        /*! The \b CDATASection node to free. */
                     );

/*@}*/ /* Interface CDATASection */


/*================================================================
*
*   Document interfaces
*
*
*=================================================================*/

/*! \name Interface Document
 * The \b Document interface represents the entire XML document.
 * In essence, it is the root of the document tree and provides the
 * primary interface to the elements of the document. For more information,
 * refer to the <em>Interface Document</em> section in the DOM2Core.
 */
/*@{*/

  /*! Initializes a \b Document node.
   *
   *  \return [void] This function does not return a value.
   */

EXPORT_SPEC void    
ixmlDocument_init(IXML_Document *nodeptr  
		    /*! The \b Document node to initialize.  */
                 );

  /*! Creates a new empty \b Document node.  The 
   *  \b ixmlDocument_createDocumentEx API differs from the
   *  \b ixmlDocument_createDocument API in that it returns an error code
   *  describing the reason for the failure rather than just \c NULL.
   *
   *  \return [int] An integer representing one of the following:
   *      \li \c IXML_SUCCESS: The operation completed successfully.
   *      \li \c IXML_INSUFFICIENT_MEMORY: Not enough free memory exists 
   *            to complete this operation.
   */

EXPORT_SPEC int ixmlDocument_createDocumentEx(IXML_Document** doc 
		                    /*! Pointer to a \b Document where the 
				        new object will be stored. */
		                  );


  /*! Creates a new empty \b Document node.
   *
   *  \return [Document*] A pointer to the new \b Document or \c NULL on 
   *                      failure.
   */

EXPORT_SPEC IXML_Document* ixmlDocument_createDocument();

  /*! Creates a new \b Element node with the given tag name.  The new
   *  \b Element node has a \c nodeName of \b tagName and
   *  the \c localName, \c prefix, and \c namespaceURI set 
   *  to \c NULL.  To create an \b Element with a namespace, 
   *  see \b ixmlDocument_createElementNS.
   *
   *  The \b ixmlDocument_createElementEx API differs from the \b
   *  ixmlDocument_createElement API in that it returns an error code
   *  describing the reason for failure rather than just \c NULL.
   *
   *  \return [int] An integer representing one of the following:
   *      \li \c IXML_SUCCESS: The operation completed successfully.
   *      \li \c IXML_INVALID_PARAMETER: Either \b doc or 
   *            \b tagName is \c NULL.
   *      \li \c IXML_INSUFFICIENT_MEMORY: Not enough free memory exists 
   *            to complete this operation.
   */

EXPORT_SPEC int
ixmlDocument_createElementEx(IXML_Document *doc,  
		               /*! The owner \b Document of the new node. */
                             const DOMString tagName,  
			       /*! The tag name of the new \b Element 
				   node. */
                             IXML_Element **rtElement
			       /*! Pointer to an \b Element where the new 
				   object will be stored. */
                            );

  /*! Creates a new \b Element node with the given tag name.  The new
   *  \b Element node has a \c nodeName of \b tagName and
   *  the \c localName, \c prefix, and \c namespaceURI set 
   *  to \c NULL.  To create an \b Element with a namespace, 
   *  see \b ixmlDocument_createElementNS.
   *
   *  \return [Document*] A pointer to the new \b Element or \c NULL on 
   *                      failure.
   */

EXPORT_SPEC IXML_Element*
ixmlDocument_createElement(IXML_Document *doc,  
		             /*! The owner \b Document of the new node. */
                           const DOMString tagName    
			     /*! The tag name of the new \b Element node. */
                           );


  /*! Creates a new \b Text node with the given data.  
   *  The \b ixmlDocument_createTextNodeEx API differs from the
   *  \b ixmlDocument_createTextNode API in that it returns an error code
   *  describing the reason for failure rather than just \c NULL.
   *
   *  \return [int] An integer representing one of the following:
   *      \li \c IXML_SUCCESS: The operation completed successfully.
   *      \li \c IXML_INVALID_PARAMETER: Either \b doc or \b data 
   *            is \c NULL.
   *      \li \c IXML_INSUFFICIENT_MEMORY: Not enough free memory exists 
   *            to complete this operation.
   */

EXPORT_SPEC int
ixmlDocument_createTextNodeEx(IXML_Document *doc,  
		                /*! The owner \b Document of the new node. */
                              const DOMString data,      
			        /*! The data to associate with the new \b Text node. */
                              IXML_Node** textNode 
			        /*! A pointer to a \b Node where the new 
				    object will be stored. */
                              );


  /*! Creates a new \b Text node with the given data.
   *
   *  \return [Node*] A pointer to the new \b Node or \c NULL on failure.
   */

EXPORT_SPEC IXML_Node*
ixmlDocument_createTextNode(IXML_Document *doc,  
		              /*! The owner \b Document of the new node. */
                            const DOMString data       
			      /*! The data to associate with the new \b Text 
			          node. */
                            );

  /*! Creates a new \b CDATASection node with given data.
   *
   *  The \b ixmlDocument_createCDATASectionEx API differs from the \b
   *  ixmlDocument_createCDATASection API in that it returns an error code
   *  describing the reason for failure rather than just \c NULL.
   *
   *  \return [int] An integer representing one of the following:
   *      \li \c IXML_SUCCESS: The operation completed successfully.
   *      \li \c IXML_INVALID_PARAMETER: Either \b doc or \b data 
   *            is \c NULL.
   *      \li \c IXML_INSUFFICIENT_MEMORY: Not enough free memory exists 
   *            to complete this operation.
   */

EXPORT_SPEC int
ixmlDocument_createCDATASectionEx(IXML_Document *doc,  
		                    /*! The owner \b Document of the new 
				        node. */
                                  const DOMString data,      
				    /*! The data to associate with the new 
				        \b CDATASection node. */
                                  IXML_CDATASection** cdNode   
				    /*! A pointer to a \b Node where the 
				        new object will be stored. */ 
                                 );


  /*! Creates a new \b CDATASection node with given data.
   *
   *  \return A pointer to the new \b CDATASection or \c NULL on failure.
   */
EXPORT_SPEC IXML_CDATASection*
ixmlDocument_createCDATASection(IXML_Document *doc,  
				  /*! The owner \b Document of the new node. */
                                const DOMString data  
				  /*! The data to associate with the new \b CDATASection node. */
                               );


  /*! Creates a new \b Attr node with the given name.  
   *
   *  \return [Attr*] A pointer to the new \b Attr or \c NULL on failure.
   */
EXPORT_SPEC IXML_Attr*
ixmlDocument_createAttribute(IXML_Document *doc,  
		               /*! The owner \b Document of the new node. */
                             const char *name      
			       /*! The name of the new attribute. */
                            );


  /*! Creates a new \b Attr node with the given name.  
   *
   *  The \b ixmlDocument_createAttributeEx API differs from the \b
   *  ixmlDocument_createAttribute API in that it returns an error code
   *  describing the reason for failure rather than just \c NULL.
   *
   *  \return An integer representing one of the following:
   *      \li \c IXML_SUCCESS: The operation completed successfully.
   *      \li \c IXML_INVALID_PARAMETER: Either \b doc or \b name 
   *            is \c NULL.
   *      \li \c IXML_INSUFFICIENT_MEMORY: Not enough free memory exists 
   *            to complete this operation.
   */

EXPORT_SPEC int
ixmlDocument_createAttributeEx(IXML_Document *doc,  
		                 /*! The owner \b Document of the new node. */
                               const char *name,      
			         /*! The name of the new attribute. */
                               IXML_Attr** attrNode
			         /*! A pointer to a \b Attr where the new object will be stored. */
                              );


  /*! Returns a \b NodeList of all \b Elements that match the given
   *  tag name in the order in which they were encountered in a preorder
   *  traversal of the \b Document tree.  
   *
   *  \return A pointer to a \b NodeList containing the matching items or \c NULL on an error.
   */

EXPORT_SPEC IXML_NodeList*
ixmlDocument_getElementsByTagName(IXML_Document *doc,     
		                    /*! The \b Document to search. */
                                  const DOMString tagName  
				    /*! The tag name to find. */
                                 );

/* introduced in DOM level 2 */

  /*! Creates a new \b Element node in the given qualified name and
   *  namespace URI.
   *
   *  The \b ixmlDocument_createElementNSEx API differs from the \b
   *  ixmlDocument_createElementNS API in that it returns an error code
   *  describing the reason for failure rather than just \c NULL.
   *
   *  \return [int] An integer representing one of the following:
   *      \li \c IXML_SUCCESS: The operation completed successfully.
   *      \li \c IXML_INVALID_PARAMETER: Either \b doc, 
   *            \b namespaceURI, or \b qualifiedName is \c NULL.
   *      \li \c IXML_INSUFFICIENT_MEMORY: Not enough free memory exists 
   *            to complete this operation.
   */

EXPORT_SPEC int
ixmlDocument_createElementNSEx(IXML_Document *doc,           
		                 /*! The owner \b Document of the new node. */
                               const DOMString namespaceURI,  
			         /*! The namespace URI for the new \b Element. */
                               const DOMString qualifiedName,  
			         /*! The qualified name of the new \b Element. */
                               IXML_Element** rtElement
			         /*! A pointer to an \b Element where the new object will be stored. */
                              );


  /*! Creates a new \b Element node in the given qualified name and
   *  namespace URI.
   *
   *  \return [Element*] A pointer to the new \b Element or \c NULL on 
   *                     failure.
   */

EXPORT_SPEC IXML_Element*
ixmlDocument_createElementNS(IXML_Document *doc,           
		               /*! The owner \b Document of the new node. */
                             const DOMString namespaceURI,  
			       /*! The namespace URI for the new \b Element. */
                             const DOMString qualifiedName  
			       /*! The qualified name of the new \b Element. */
                             );

  /*! Creates a new \b Attr node with the given qualified name and
   *  namespace URI.
   *
   *  The \b ixmlDocument_createAttributeNSEx API differs from the \b
   *  ixmlDocument_createAttributeNS API in that it returns an error code
   *  describing the reason for failure rather than just \c NULL.
   *
   *  \return [int] An integer representing one of the following:
   *      \li \c IXML_SUCCESS: The operation completed successfully.
   *      \li \c IXML_INVALID_PARAMETER: Either \b doc, 
   *            \b namespaceURI, or \b qualifiedName is \c NULL.
   *      \li \c IXML_INSUFFICIENT_MEMORY: Not enough free memory exists 
   *            to complete this operation.
   */

EXPORT_SPEC int
ixmlDocument_createAttributeNSEx(IXML_Document *doc,
		                   /*! The owner \b Document of the new 
				       \b Attr. */
                                 const DOMString namespaceURI, 
				   /*! The namespace URI for the attribute. */
                                 const DOMString qualifiedName, 
				   /*! The qualified name of the attribute. */
                                 IXML_Attr** attrNode
				   /*! A pointer to an \b Attr where the new object will be stored. */
                                );   

  /*! Creates a new \b Attr node with the given qualified name and
   *  namespace URI.
   *
   *  \return [Attr*] A pointer to the new \b Attr or \c NULL on failure.
   */
EXPORT_SPEC IXML_Attr*
ixmlDocument_createAttributeNS(IXML_Document *doc, 
		                 /*! The owner \b Document of the new \b Attr. */
                               const DOMString namespaceURI, 
			         /*! The namespace URI for the attribute. */
                               const DOMString qualifiedName 
			         /*! The qualified name of the attribute. */
                              );   


  /*! Returns a \b NodeList of \b Elements that match the given
   *  local name and namespace URI in the order they are encountered
   *  in a preorder traversal of the \b Document tree.  Either 
   *  \b namespaceURI or \b localName can be the special <tt>"*"</tt>
   *  character, which matches any namespace or any local name respectively.
   *
   *  \return A pointer to a \b NodeList containing the matching items or \c NULL on an error.
   */
EXPORT_SPEC IXML_NodeList*   
ixmlDocument_getElementsByTagNameNS(IXML_Document* doc,          
		                      /*! The \b Document to search. */
                                    const DOMString namespaceURI, 
				      /*! The namespace of the elements to find or <tt>"*"</tt> to match any namespace. */
                                    const DOMString localName     
				      /*! The local name of the elements to find or <tt>"*"</tt> to match any local name. */
                                    );


  /*! Returns the \b Element whose \c ID matches that given id.
   *
   *  \return A pointer to the matching \b Element or \c NULL on an error.
   */
EXPORT_SPEC IXML_Element*    
ixmlDocument_getElementById(IXML_Document* doc,         
		              /*! The owner \b Document of the \b Element. */
                            const DOMString tagName  
			      /*! The name of the \b Element.*/
                            );

  /*! Frees a \b Document object and all \b Nodes associated with it.  
   *  Any \b Nodes extracted via any other interface function, e.g. 
   *  \b ixmlDocument_GetElementById, become invalid after this call unless
   *  explicitly cloned.
   */
EXPORT_SPEC void        
ixmlDocument_free(IXML_Document* doc  
		    /*! The \b Document to free.  */
                 );

  /*! Imports a \b Node from another \b Document into this 
   *  \b Document.  The new \b Node does not a have parent node: it is a 
   *  clone of the original \b Node with the \c ownerDocument set to 
   *  \b doc.  The \b deep parameter controls whether all the children 
   *  of the \b Node are imported.  Refer to the DOM2-Core recommendation 
   *  for details on importing specific node types.
   *
   *  \return [int] An integer representing one of the following:
   *      \li \c IXML_SUCCESS: The operation completed successfully.
   *      \li \c IXML_INVALID_PARAMETER: Either \b doc or 
   *            \b importNode is not a valid pointer.
   *      \li \c IXML_NOT_SUPPORTED_ERR: \b importNode is a 
   *            \b Document, which cannot be imported.
   *      \li \c IXML_FAILED: The import operation failed because the 
   *            \b Node to be imported could not be cloned.
   */

EXPORT_SPEC int         
ixmlDocument_importNode(IXML_Document* doc,     
		          /*! The \b Document into which to import. */
                        IXML_Node* importNode,  
			  /*! The \b Node to import. */
                        BOOL deep,         
			  /*! \c TRUE to import all children of \b importNode or \c FALSE to import only the 
			      root node. */
                        IXML_Node** rtNode      
			  /*! A pointer to a new \b Node owned by \b doc. */
                       );
/*@}*/ /* Interface Document */

/*================================================================
*
*   Element interfaces
*
*
*=================================================================*/

/*! \name Interface Element
 * The \b Element interface represents an element in an XML document.
 * Only \b Elements are allowed to have attributes, which are stored in the
 * \c attributes member of a \b Node.  The \b Element interface
 * extends the \b Node interface and adds more operations to manipulate
 * attributes.
 */
/*! @{ */

  /*! Initializes a \b IXML_Element node.
   *
   *  \return [void] This function does not return a value.
   */

EXPORT_SPEC void ixmlElement_init(IXML_Element *element  
		        /*! The \b Element to initialize.*/
                     );


  /*! Returns the name of the tag as a constant string.
   *
   *  \return [const DOMString] A \b DOMString representing the name of the \b Element.
   */

EXPORT_SPEC const DOMString
ixmlElement_getTagName(IXML_Element* element  
		         /*! The \b Element from which to retrieve the name. */
                      );

  /*! Retrieves an attribute of an \b Element by name.  
   *
   *  \return A \b DOMString representing the value of the attribute.
   */

EXPORT_SPEC const DOMString   
ixmlElement_getAttribute(IXML_Element* element,  
		           /*! The \b Element from which to retrieve the attribute. */
                         const DOMString name     
			   /*! The name of the attribute to retrieve. */
                        );

  /*! Adds a new attribute to an \b Element.  If an attribute with the same
   *  name already exists, the attribute value will be updated with the
   *  new value in \b value.  
   *
   *  \return [int] An integer representing of the following:
   *      \li \c IXML_SUCCESS: The operation completed successfully.
   *      \li \c IXML_INVALID_PARAMETER: Either \b element, 
   *            \b name, or \b value is \c NULL.
   *      \li \c IXML_INVALID_CHARACTER_ERR: \b name contains an 
   *            illegal character.
   *      \li \c IXML_INSUFFICIENT_MEMORY: Not enough free memory exists 
   *            to complete the operation.
   */

EXPORT_SPEC int         
ixmlElement_setAttribute(IXML_Element* element,  
		           /*! The \b Element on which to set the attribute. */
                         const DOMString name,    
			   /*! The name of the attribute. */
                         const DOMString value
			   /*! The value of the attribute.  Note that this is 
			       a non-parsed string and any markup must be escaped. */
                        );

  /*! Removes an attribute by name.  
   *
   *  \return [int] An integer representing one of the following:
   *      \li \c IXML_SUCCESS: The operation completed successfully.
   *      \li \c IXML_INVALID_PARAMETER: Either \b element or 
   *            \b name is \c NULL.
   */

EXPORT_SPEC int         
ixmlElement_removeAttribute(IXML_Element* element,  
		              /*! The \b Element from which to remove the 
			          attribute. */
                            const DOMString name     
			      /*! The name of the attribute to remove.  */
                           );              

  /*! Retrieves an attribute node by name.  See 
   *  \b ixmlElement_getAttributeNodeNS to retrieve an attribute node using
   *  a qualified name or namespace URI.
   *
   *  \return A pointer to the attribute matching \b name or \c NULL on an error.
   */

EXPORT_SPEC IXML_Attr*       
ixmlElement_getAttributeNode(IXML_Element* element,  
		               /*! The \b Element from which to get the attribute node.  */
                             const DOMString name     
			       /*! The name of the attribute node to find. */
                            );

  /*! Adds a new attribute node to an \b Element.  If an attribute already
   *  exists with \b newAttr as a name, it will be replaced with the
   *  new one and the old one will be returned in \b rtAttr.
   *
   *  \return [int] An integer representing one of the following:
   *      \li \c IXML_SUCCESS: The operation completed successfully.
   *      \li \c IXML_INVALID_PARAMETER: Either \b element or 
   *            \b newAttr is \c NULL.
   *      \li \c IXML_WRONG_DOCUMENT_ERR: \b newAttr does not belong 
   *            to the same one as \b element.
   *      \li \c IXML_INUSE_ATTRIBUTE_ERR: \b newAttr is already 
   *            an attribute of another \b Element.
   */

EXPORT_SPEC int         
ixmlElement_setAttributeNode(IXML_Element* element,  
		               /*! The \b Element in which to add the new attribute. */
                             IXML_Attr* newAttr,     
			       /*! The new \b Attr to add. */
                             IXML_Attr** rtAttr      
			       /*! A pointer to an \b Attr where the old 
				   \b Attr will be stored.  This will have  
				   a \c NULL if no prior node existed. */
                            );

  /*! Removes the specified attribute node from an \b Element.  
   *
   *  \return [int] An integer representing one of the following:
   *      \li \c IXML_SUCCESS: The operation completed successfully.
   *      \li \c IXML_INVALID_PARAMETER: Either \b element or 
   *            \b oldAttr is \c NULL.
   *      \li \c IXML_NOT_FOUND_ERR: \b oldAttr is not among the list 
   *            attributes of \b element.
   */

EXPORT_SPEC int         
ixmlElement_removeAttributeNode(IXML_Element* element,  
		                  /*! The \b Element from which to remove 
				      the attribute. */
                                IXML_Attr* oldAttr,     
				  /*! The attribute to remove from the \b Element. */
                                IXML_Attr** rtAttr      
				  /*! A pointer to an attribute in which to 
				      place the removed attribute. */
                               );

  /*! Returns a \b NodeList of all \em descendant \b Elements with
   *  a given tag name, in the order in which they are encountered in a
   *  pre-order traversal of this \b Element tree.
   *
   *  \return [NodeList*] A \b NodeList of the matching \b Elements or 
   *                      \c NULL on an error.
   */

EXPORT_SPEC IXML_NodeList*   
ixmlElement_getElementsByTagName(IXML_Element* element,  
		                   /*! The \b Element from which to start the search. */
                                 const DOMString tagName
				   /*! The name of the tag for which to search. */
                                );

/* introduced in DOM 2 */

  /*! Retrieves an attribute value using the local name and namespace URI.
   *
   *  \return [DOMString] A \b DOMString representing the value of the 
   *                      matching attribute.
   */

EXPORT_SPEC const DOMString
ixmlElement_getAttributeNS(IXML_Element* element,       
		             /*! The \b Element from which to get the attribute value. */
                           const DOMString namespaceURI, 
			     /*! The namespace URI of the attribute. */
                           const DOMString localname     
			     /*! The local name of the attribute. */
                          );

  /*! Adds a new attribute to an \b Element using the local name and 
   *  namespace URI.  If another attribute matches the same local name and 
   *  namespace, the prefix is changed to be the prefix part of the 
   *  \c qualifiedName and the value is changed to \b value.
   *
   *  \return [int] An integer representing one of the following:
   *      \li \c IXML_SUCCESS: The operation completed successfully.
   *      \li \c IXML_INVALID_PARAMETER: Either \b element, 
   *            \b namespaceURI, \b qualifiedName, or \b value is 
   *            \c NULL.
   *      \li \c IXML_INVALID_CHARACTER_ERR: \b qualifiedName contains 
   *            an invalid character.
   *      \li \c IXML_NAMESPACE_ERR: Either the \b qualifiedName or 
   *            \b namespaceURI is malformed.  Refer to the DOM2-Core for 
   *            possible reasons.
   *      \li \c IXML_INSUFFICIENT_MEMORY: Not enough free memory exist 
   *            to complete the operation.
   *      \li \c IXML_FAILED: The operation could not be completed.
   */

EXPORT_SPEC int         
ixmlElement_setAttributeNS(IXML_Element* element,         
		             /*! The \b Element on which to set the attribute. */
                           const DOMString namespaceURI,   
		             /*! The namespace URI of the new attribute. */
                           const DOMString qualifiedName,  
			     /*! The qualified name of the attribute. */
                           const DOMString value 
			     /*! The new value for the attribute. */
                          );

  /*! Removes an attribute using the namespace URI and local name.
   *
   *  \return [int] An integer representing one of the following:
   *      \li \c IXML_SUCCESS: The operation completed successfully.
   *      \li \c IXML_INVALID_PARAMETER: Either \b element, 
   *            \b namespaceURI, or \b localName is \c NULL.
   */

EXPORT_SPEC int         
ixmlElement_removeAttributeNS(IXML_Element* element,        
		                /*! The \b Element from which to remove the the attribute. */
                              const DOMString namespaceURI,  
			        /*! The namespace URI of the attribute. */
                              const DOMString localName      
			        /*! The local name of the attribute.*/
                             );

  /*! Retrieves an \b Attr node by local name and namespace URI.
   *
   *  \return [Attr*] A pointer to an \b Attr or \c NULL on an error.
   */

EXPORT_SPEC IXML_Attr*       
ixmlElement_getAttributeNodeNS(IXML_Element* element,        
		                 /*! The \b Element from which to get the attribute. */
                               const DOMString namespaceURI,  
			         /*! The namespace URI of the attribute. */
                               const DOMString localName      
			         /*! The local name of the attribute. */
                              );

  /*! Adds a new attribute node.  If an attribute with the same local name
   *  and namespace URI already exists in the \b Element, the existing 
   *  attribute node is replaced with \b newAttr and the old returned in 
   *  \b rcAttr.
   *
   *  \return [int] An integer representing one of the following:
   *      \li \c IXML_SUCCESS: The operation completed successfully.
   *      \li \c IXML_INVALID_PARAMETER: Either \b element or 
   *            \b newAttr is \c NULL.
   *      \li \c IXML_WRONG_DOCUMENT_ERR: \b newAttr does not belong 
   *            to the same document as \b element.
   *      \li \c IXML_INUSE_ATTRIBUTE_ERR: \b newAttr already is an 
   *            attribute of another \b Element.
   */
EXPORT_SPEC int         
ixmlElement_setAttributeNodeNS(IXML_Element* element,  
		                 /*! The \b Element in which to add the attribute node. */
                               IXML_Attr*   newAttr,     
			         /*! The new \b Attr to add. */
                               IXML_Attr**  rcAttr      
			         /*! A pointer to the replaced \b Attr, if it exists. */
                              );


  /*! Returns a \b NodeList of all \em descendant \b Elements with a
   *  given tag name, in the order in which they are encountered in the
   *  pre-order traversal of the \b Element tree.
   *
   *  \return A \b NodeList of matching \b Elements or \c NULL on an error.
   */
EXPORT_SPEC IXML_NodeList*   
ixmlElement_getElementsByTagNameNS(IXML_Element* element,        
		                     /*! The \b Element from which to start the search. */
                                   const DOMString namespaceURI,
				     /*! The namespace URI of the \b Elements to find. */
                                   const DOMString localName      
				     /*! The local name of the \b Elements to find. */
                                  );

  /*! Queries whether the \b Element has an attribute with the given name
   *  or a default value.
   *
   *  \return [BOOL] \c TRUE if the \b Element has an attribute with 
   *                 this name or has a default value for that attribute, 
   *                 otherwise \c FALSE.
   */

EXPORT_SPEC BOOL ixmlElement_hasAttribute(IXML_Element* element, 
		           /*! The \b Element on which to check for an attribute. */
                         const DOMString name    
			   /*! The name of the attribute for which to check. */
                        );

  /*! Queries whether the \b Element has an attribute with the given
   *  local name and namespace URI or has a default value for that attribute.
   *
   *  \return [BOOL] \c TRUE if the \b Element has an attribute with 
   *                 the given namespace and local name or has a default 
   *                 value for that attribute, otherwise \c FALSE.
   */

EXPORT_SPEC BOOL        
ixmlElement_hasAttributeNS(IXML_Element* element,       
		             /*! The \b Element on which to check for the attribute. */
                           const DOMString namespaceURI, 
			     /*! The namespace URI of the attribute. */
                           const DOMString localName     
			     /*! The local name of the attribute. */
                          );

/*! \brief Frees the given \b Element and any subtree of the \b Element.
 */
EXPORT_SPEC void ixmlElement_free(
	/*! The \b Element to free. */
	IXML_Element *element);

/*@}*/ /* Interface Element */


/*================================================================
*
*   NamedNodeMap interfaces
*
*
*=================================================================*/

/*! \name Interface NamedNodeMap
 * A \b NamedNodeMap object represents a list of objects that can be
 * accessed by name.  A \b NamedNodeMap maintains the objects in 
 * no particular order.  The \b Node interface uses a \b NamedNodeMap
 * to maintain the attributes of a node.
 */
/*@{*/

  /*! Returns the number of items contained in this \b NamedNodeMap.
   *
   *  \return [unsigned long] The number of nodes in this map.
   */

EXPORT_SPEC unsigned long 
ixmlNamedNodeMap_getLength(IXML_NamedNodeMap *nnMap  
		             /*! The \b NamedNodeMap from which to retrieve 
			         the size. */
                          );

  /*! Retrieves a \b Node from the \b NamedNodeMap by name.
   *
   *  \return [Node*] A \b Node or \c NULL if there is an error.
   */

EXPORT_SPEC IXML_Node*   
ixmlNamedNodeMap_getNamedItem(IXML_NamedNodeMap *nnMap, 
		                /*! The \b NamedNodeMap to search. */
                              const DOMString name       
			        /*! The name of the \b Node to find. */
                             );

  /*! Adds a new \b Node to the \b NamedNodeMap using the \b Node 
   *  name attribute.
   *
   *  \return [Node*] The old \b Node if the new \b Node replaces it or 
   *                  \c NULL if the \b Node was not in the 
   *                  \b NamedNodeMap before.
   */

EXPORT_SPEC IXML_Node*   
ixmlNamedNodeMap_setNamedItem(IXML_NamedNodeMap *nnMap, 
		                /*! The \b NamedNodeMap in which to add the new \b Node. */
                              IXML_Node *arg            
			        /*! The new \b Node to add to the \b NamedNodeMap. */
                             );

  /*! Removes a \b Node from a \b NamedNodeMap specified by name.
   *
   *  \return [Node*] A pointer to the \b Node, if found, or \c NULL if 
   *                  it wasn't.
   */

EXPORT_SPEC IXML_Node*   
ixmlNamedNodeMap_removeNamedItem(IXML_NamedNodeMap *nnMap,  
		                   /*! The \b NamedNodeMap from which to remove the item. */
                                 const DOMString name        
				   /*! The name of the item to remove. */
                                );

  /*! Retrieves a \b Node from a \b NamedNodeMap specified by a
   *  numerical index.
   *
   *  \return A pointer to the \b Node, if found, or \c NULL if it wasn't.
   */

EXPORT_SPEC IXML_Node*   
ixmlNamedNodeMap_item(IXML_NamedNodeMap *nnMap, 
		        /*! The \b NamedNodeMap from which to remove the \b Node. */
                      unsigned long index  
		        /*! The index into the map to remove. */
                     );

/* introduced in DOM level 2 */

  /*! Retrieves a \b Node from a \b NamedNodeMap specified by
   *  namespace URI and local name.
   *
   *  \return A pointer to the \b Node, if found, or \c NULL if it wasn't
   */

EXPORT_SPEC IXML_Node*   
ixmlNamedNodeMap_getNamedItemNS(IXML_NamedNodeMap *nnMap,    
		                  /*! The \b NamedNodeMap from which to remove the \b Node. */
                                const DOMString *namespaceURI,
				  /*! The namespace URI of the \b Node to remove. */
                                const DOMString localName     
				  /*! The local name of the \b Node to remove. */
                               );

  /*! Adds a new \b Node to the \b NamedNodeMap using the \b Node 
   *  local name and namespace URI attributes.
   *
   *  \return [Node*] The old \b Node if the new \b Node replaces it or 
   *                  \c NULL if the \b Node was not in the 
   *                  \b NamedNodeMap before.
   */

EXPORT_SPEC IXML_Node*   
ixmlNamedNodeMap_setNamedItemNS(IXML_NamedNodeMap *nnMap, 
		                  /*! The \b NamedNodeMap in which to add the \b Node. */
                                IXML_Node *arg 
				  /*! The \b Node to add to the map. */
                               );

  /*! Removes a \b Node from a \b NamedNodeMap specified by 
   *  namespace URI and local name.
   *
   *  \return A pointer to the \b Node, if found, or \c NULL if it wasn't.
   */
EXPORT_SPEC IXML_Node*   
ixmlNamedNodeMap_removeNamedItemNS(IXML_NamedNodeMap *nnMap,    
		                     /*! The \b NamedNodeMap from which to remove the \b Node. */
                                   const DOMString namespaceURI, 
				     /*! The namespace URI of the \b Node to remove. */
                                   const DOMString localName     
				     /*! The local name of the \b Node to remove. */
                                  );

/*! \brief Frees a \b NamedNodeMap.
 *
 * The \b Nodes inside the map are not freed, just the \b NamedNodeMap object.
 */
EXPORT_SPEC void ixmlNamedNodeMap_free(
	/*! The \b NamedNodeMap to free. */
	IXML_NamedNodeMap *nnMap);

/*@}*/ /* Interface NodeMap */

/*================================================================
*
*   NodeList interfaces
*
*
*=================================================================*/

/*! \name Interface NodeList
 * The \b NodeList interface abstracts an ordered collection of
 * nodes.  Note that changes to the underlying nodes will change
 * the nodes contained in a \b NodeList.  The DOM2-Core refers to
 * this as being \em live.
 */
/*@{*/

  /*! Retrieves a \b Node from a \b NodeList specified by a 
   *  numerical index.
   *
   *  \return [Node*] A pointer to a \b Node or \c NULL if there was an 
   *                  error.
   */

EXPORT_SPEC IXML_Node*           
ixmlNodeList_item(IXML_NodeList *nList,     
		    /*! The \b NodeList from which to retrieve the \b Node. */
                  unsigned long index  
		    /*! The index into the \b NodeList to retrieve. */
                 );

  /*! Returns the number of \b Nodes in a \b NodeList.
   *
   *  \return [unsigned long] The number of \b Nodes in the \b NodeList.
   */

EXPORT_SPEC unsigned long   
ixmlNodeList_length(IXML_NodeList *nList  
		      /*! The \b NodeList for which to retrieve the 
		          number of \b Nodes. */
                   );

  /*! Frees a \b NodeList object.  Since the underlying \b Nodes are
   *  references, they are not freed using this operating.  This only
   *  frees the \b NodeList object.
   *
   *  \return [void] This function does not return a value.
   */

EXPORT_SPEC void            
ixmlNodeList_free(IXML_NodeList *nList  
		    /*! The \b NodeList to free.  */
                 );

/*@}*/ /* Interface NodeList */


/*******************************************************************************
 * 
 *   ixml interfaces
 *
 ******************************************************************************/
/*! \name IXML API
 * The IXML API contains utility functions that are not part of the standard
 * DOM interfaces.  They include functions to create a DOM structure from a
 * file or buffer, create an XML file from a DOM structure, and manipulate 
 * DOMString objects.
 */
/*@{*/


/*! \brief Renders a \b Node and all sub-elements into an XML document
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
 * \return [DOMString] A \b DOMString with the XML document representation 
 *                     of the DOM tree or \c NULL on an error.
 */
EXPORT_SPEC DOMString ixmlPrintDocument(IXML_Document *doc);


/*! \brief Renders a \b Node and all sub-elements into an XML text
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
 * \return [DOMString] A \b DOMString with the XML text representation 
 *                     of the DOM tree or \c NULL on an error.
 */
EXPORT_SPEC DOMString ixmlPrintNode(
	/*! The root of the \b Node tree to render to XML text. */
	IXML_Node *doc
);


/*! \brief Renders a \b Node and all sub-elements into an XML document
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
	/*! The root of the \b Node tree to render to XML text. */
	IXML_Document *doc);


/*! \brief Renders a \b Node and all sub-elements into an XML text
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
	/*! The root of the \b Node tree to render to XML text. */
	IXML_Node *doc);


/*! \brief Makes the XML parser more tolerant to malformed text.
 */
EXPORT_SPEC void ixmlRelaxParser(
	/*! If \b errorChar is 0 (default), the parser is strict about XML 
	 * encoding : invalid UTF-8 sequences or "&" entities are rejected, and 
	 * the parsing aborts.
	 * If \b errorChar is not 0, the parser is relaxed: invalid UTF-8 
	 * characters are replaced by the \b errorChar, and invalid "&" entities 
	 * are left untranslated. The parsing is then allowed to continue.
	 */
	char errorChar);


/*! \brief Parses an XML text buffer converting it into an IXML DOM representation.
 *
 * \return A \b Document if the buffer correctly parses or \c NULL on an error. 
 */
EXPORT_SPEC IXML_Document*
ixmlParseBuffer(
	/*! The buffer that contains the XML text to convert to a \b Document. */
	const char *buffer);


/*! \brief Parses an XML text buffer converting it into an IXML DOM representation.
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
	/*! The buffer that contains the XML text to convert to a \b Document. */
	const char *buffer,
	/*! A point to store the \b Document if file correctly parses or \b NULL on an error. */
	IXML_Document** doc);


/*! \brief Parses an XML text file converting it into an IXML DOM representation.
 *
 * \return A \b Document if the file correctly parses or \c NULL on an error.
 */
EXPORT_SPEC IXML_Document *ixmlLoadDocument(
	/*! The filename of the XML text to convert to a \b Document. */
	const char* xmlFile);


/*! \brief Parses an XML text file converting it into an IXML DOM representation.
 *
 *  The \b ixmlLoadDocumentEx API differs from the \b ixmlLoadDocument
 *  API in that it returns a an error code representing the actual failure
 *  rather than just \c NULL.
 *
 *  \return An integer representing one of the following:
 *      \li \c IXML_SUCCESS: The operation completed successfully.
 *      \li \c IXML_INVALID_PARAMETER: The \b xmlFile is not a valid 
 *            pointer.
 *      \li \c IXML_INSUFFICIENT_MEMORY: Not enough free memory exists 
 *            to complete this operation.
 */
EXPORT_SPEC int ixmlLoadDocumentEx(
	/*! The filename of the XML text to convert to a \b Document. */
	const char *xmlFile,
	/*! A pointer to the \b Document if file correctly parses or \b NULL
	 * on an error. */
	IXML_Document **doc);


/*! \brief Clones an existing \b DOMString.
 *
 * \return A new \b DOMString that is a duplicate of the original or \c NULL
 * if the operation could not be completed.
 */
EXPORT_SPEC DOMString ixmlCloneDOMString(
	/*! The source \b DOMString to clone. */
	const DOMString src);


/*! \brief Frees a \b DOMString.
 */
EXPORT_SPEC void ixmlFreeDOMString(
	/*! The \b DOMString to free. */
	DOMString buf);


/*@}*/ /* IXML API */

#ifdef __cplusplus
}
#endif

/*@}*/ /* The XML API */

#endif  /* IXML_H */

