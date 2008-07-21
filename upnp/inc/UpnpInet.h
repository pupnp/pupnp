

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
	#include <netinet/in.h>
#endif


#endif /* UPNPINET_H */

