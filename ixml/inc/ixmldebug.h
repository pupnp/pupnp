

#ifndef IXMLDEBUG_H
#define IXMLDEBUG_H


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
	/*! [in] Printf like format specification. */
	const char* FmtStr,
	/*! [in] Printf like Variable number of arguments that will go in the debug
	 * statement. */
	...)
#if (__GNUC__ >= 3)
	/* This enables printf like format checking by the compiler */
	__attribute__((format (__printf__, 1, 2)))
#endif
;
#else /* DEBUG */
static UPNP_INLINE void IxmlPrintf(
	const char* FmtStr,
	...) {}
#endif /* DEBUG */


#endif /* IXMLDEBUG_H */

