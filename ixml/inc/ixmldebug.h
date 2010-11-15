

#ifndef IXMLDEBUG_H
#define IXMLDEBUG_H


#include "UpnpGlobal.h"
#include "ixml.h"


/*!
 * \file
 *
 * \brief Auxiliar routines to aid debugging.
 */


/*!
 * \brief Prints the debug statement either on the standard output or log file
 * along with the information from where this debug statement is coming.
 */ 
#ifdef DEBUG
void IxmlPrintf(
	/*! [in] The file name, usually __FILE__. */
	const char *DbgFileName,
	/*! [in] The line number, usually __LINE__ or a variable that got the
	 * __LINE__ at the appropriate place. */
	int DbgLineNo,
	/*! [in] The function name. */
	const char *FunctionName,
	/*! [in] Printf like format specification. */
	const char* FmtStr,
	/*! [in] Printf like Variable number of arguments that will go in the debug
	 * statement. */
	...)
#if (__GNUC__ >= 3)
	/* This enables printf like format checking by the compiler */
	__attribute__((format (__printf__, 4, 5)))
#endif
;
#else /* DEBUG */
static UPNP_INLINE void IxmlPrintf(
	const char *FmtStr,
	...)
{
	FmtStr = FmtStr;
}
#endif /* DEBUG */


/*!
 * \brief Print the node names and values of a XML tree.
 */
#ifdef DEBUG
void printNodes(
	/*! [in] The root of the tree to print. */
	IXML_Node *tmpRoot,
	/*! [in] The depth to print. */
	int depth);
#else
static UPNP_INLINE void printNodes(
	IXML_Node *tmpRoot,
	int depth)
{
	tmpRoot = tmpRoot;
	depth = depth;
}
#endif


#endif /* IXMLDEBUG_H */

