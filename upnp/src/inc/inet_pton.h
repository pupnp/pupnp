#ifndef INET_PTON
#define INET_PTON

#ifdef WIN32

#ifdef IPV6_
#define INET_IPV6
#endif

#include "unixutil.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/*!
 * \file
 *
 * \author: Paul Vixie, 1996.
 *
 * \brief Network support routines missing in WIN32.
 *
 * \warning Don't even consider trying to compile this on a system where
 * sizeof(int) < 4.  sizeof(int) 4 is fine; all the world's not a VAX.
 *
 */

/*!
 * \brief convert a network format address to presentation format.
 *
 * \return
 *	pointer to presentation format address (`dst'), or NULL (see errno).
 */
extern const char *inet_ntop(int af, const void *src, char *dst,
			     socklen_t size);

/*!
 * \brief convert from presentation format (which usually means ASCII printable)
 * to network format (which is usually some kind of binary format).
 *
 * \return
 *	\li 1 if the address was valid for the specified address family
 *	\li 0 if the address wasn't valid (`dst' is untouched in this case)
 *	\li -1 if some other error occurred (`dst' is untouched in this case, too)
 */
extern int inet_pton(int af, const char *src, void *dst);

#endif /* WIN32 */

#endif /* INET_PTON */
