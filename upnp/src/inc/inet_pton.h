

#ifndef INET_PTON
#define INET_PTON


#ifdef WIN32


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
 * \brief format an IPv4 address
 *
 * \return `dst' (as a const)
 *
 * \note
 *	\li (1) uses no statics
 *	\li (2) takes a u_char* not an in_addr as input
 *
 */
extern const char *inet_ntop4(const u_char src, char *dst, socklen_t size);


/*!
 * \brief convert IPv6 binary address into presentation (printable) format
 */
#ifdef INET_IPV6
extern const char *inet_ntop6(const u_char *src, char *dst, socklen_t size);
#endif /* INET_IPV6 */


/*!
 * \brief like inet_aton() but without all the hexadecimal and shorthand.
 *
 * \return 1 if `src' is a valid dotted quad, else 0.
 *
 * \note does not touch `dst' unless it's returning 1.
 */
extern inet_pton4(const char *src,u_char *dst);


/*!
 * \brief convert presentation level address to network order binary form.
 *
 * \return 1 if `src' is a valid [RFC1884 2.2] address, else 0.
 *
 * \note
 *	\li (1) does not touch `dst' unless it's returning 1.
 *	\li (2) :: in a full address is silently ignored.
 */
#ifdef INET_IPV6
extern int inet_pton6(const char *src, u_char *dst);
#endif /* INET_IPV6 */


/*!
 * \brief convert a network format address to presentation format.
 *
 * \return
 *	pointer to presentation format address (`dst'), or NULL (see errno).
 */
extern const char *inet_ntop(int af,const void *src,char *dst,socklen_t size);


/*!
 * \brief convert from presentation format (which usually means ASCII printable)
 * to network format (which is usually some kind of binary format).
 *
 * \return
 *	\li 1 if the address was valid for the specified address family
 *	\li 0 if the address wasn't valid (`dst' is untouched in this case)
 *	\li -1 if some other error occurred (`dst' is untouched in this case, too)
 */
extern int inet_pton(int af,const char *src,void *dst);


#endif /* WIN32 */


#endif /* INET_PTON */

