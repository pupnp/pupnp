

#ifndef UPNPINET_H
#define UPNPINET_H


/*!
 * \file
 *
 * \brief Provides a platform independent way to include TCP/IP types and functions.
 */


#ifdef WIN32
	#include <winsock2.h>
	#include <Ws2tcpip.h>
#else
	#include <sys/param.h>
	#if (defined(BSD) && BSD >= 199306) || defined (__FreeBSD_kernel__)
		#include <ifaddrs.h>
		/* Do not move or remove the include below for "sys/socket"!
		 * Will break FreeBSD builds. */
		#include <sys/socket.h>
	#endif
	#include <netinet/in.h>
#endif


#endif /* UPNPINET_H */

