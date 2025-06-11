#ifndef UPNPINET_H
#define UPNPINET_H

/*!
 * \addtogroup Sock
 *
 * @{
 *
 * \file
 *
 * \brief Provides a platform independent way to include TCP/IP types and
 * functions.
 */

#include "UpnpUniStd.h" /* for close() */ // IWYU pragma: keep

#ifdef _WIN32
	#include <stdarg.h>
	#include <winsock2.h>
	#include <ws2tcpip.h>

	#define UpnpCloseSocket closesocket

	#if (_WIN32_WINNT < 0x0600)
typedef short sa_family_t;
	#else
typedef ADDRESS_FAMILY sa_family_t;
	#endif

	#define OPTION_VALUE_CAST const char *

#else /* _WIN32 */
	#include <sys/param.h>
	#if defined(__sun)
		#include <fcntl.h>
		#include <sys/sockio.h>
	#elif (defined(BSD) && BSD >= 199306) || defined(__FreeBSD_kernel__)
		#include <ifaddrs.h>
		/* Do not move or remove the include below for "sys/socket"!
		 * Will break FreeBSD builds. */
		#include <sys/socket.h>
	#endif
	#include <arpa/inet.h> /* for inet_pton() */
	#include <net/if.h>
	#include <netinet/in.h>

/*! This typedef makes the code slightly more WIN32 tolerant.
 * On WIN32 systems, SOCKET is unsigned and is not a file
 * descriptor. */
typedef int SOCKET;

	/*! INVALID_SOCKET is unsigned on win32. */
	#define INVALID_SOCKET (-1)

	/*! select() returns SOCKET_ERROR on win32. */
	#define SOCKET_ERROR (-1)

	/*! Alias to close() to make code more WIN32 tolerant. */
	#define UpnpCloseSocket close

	/*!
	 * Winsock declares setsockopt() like this:
	 *
	 * int setsockopt(
	 *	[in] SOCKET     s,
	 *	[in] int        level,
	 *	[in] int        optname,
	 *	[in] const char *optval,
	 *	[in] int        optlen
	 * );
	 *
	 * While POSIX declares it like this:
	 *
	 * #include <sys/socket.h>
	 *
	 * int setsockopt(
	 *	int socket,
	 *	int level,
	 *	int option_name,
	 *	const void *option_value,
	 *	socklen_t option_len
	 * );
	 *
	 * They diverge on the declaration of option_value, which causes
	 * troubles on Windows compilation. The following define addresses
	 * this issue.
	 */
	#define OPTION_VALUE_CAST const void *

#endif /* _WIN32 */

/* @} Sock */

#endif /* UPNPINET_H */
