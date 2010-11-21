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

#ifndef GENLIB_NET_SOCK_H
#define GENLIB_NET_SOCK_H

/*!
 * \file
 */

#include "upnputil.h"

#ifdef WIN32
	/* Do not #include <netinet/in.h> */
#else
	#include <netinet/in.h>
#endif

/* The following are not defined under winsock.h */
#ifndef SD_RECEIVE
	#define SD_RECEIVE      0x00
	#define SD_SEND         0x01
	#define SD_BOTH         0x02
#endif

/*! */
typedef struct 
{
	/*! Handle/descriptor to a socket. */
	SOCKET socket;
	/*! The following two fields are filled only in incoming requests. */
	struct sockaddr_storage foreign_sockaddr;
} SOCKINFO;

#ifdef __cplusplus
#extern "C" {
#endif

/*!
 * \brief Assign the passed in socket descriptor to socket descriptor in the
 * SOCKINFO structure.
 *
 * \return Integer:
 * \li \c UPNP_E_SUCCESS	
 * \li \c UPNP_E_OUTOF_MEMORY
 * \li \c UPNP_E_SOCKET_ERROR
 */
int sock_init(
	/*! Socket Information Object. */
	OUT SOCKINFO *info,
	/*! Socket Descriptor. */
	IN SOCKET sockfd);

/*!
 * \brief Calls the sock_init function and assigns the passed in IP address
 * and port to the IP address and port in the SOCKINFO structure.
 *
 * \return Integer:
 * \li \c UPNP_E_SUCCESS	
 * \li \c UPNP_E_OUTOF_MEMORY
 * \li \c UPNP_E_SOCKET_ERROR
 */
int sock_init_with_ip(
	/*! Socket Information Object. */
	OUT SOCKINFO* info,
	/*! Socket Descriptor. */
	IN SOCKET sockfd, 
	/*! Remote socket address. */
        IN struct sockaddr *foreign_sockaddr);

/*!
 * \brief Reads data on socket in sockinfo.
 *
 * \return Integer:
 * \li \c numBytes - On Success, no of bytes received.
 * \li \c UPNP_E_TIMEDOUT - Timeout.
 * \li \c UPNP_E_SOCKET_ERROR - Error on socket calls.
 */
int sock_read(
	/*! Socket Information Object. */
	IN SOCKINFO *info,
	/*! Buffer to get data to. */
	OUT char* buffer,
	/*! Size of the buffer. */
	IN int bufsize,
	/*! timeout value. */
	INOUT int *timeoutSecs);

/*!
 * \brief Writes data on the socket in sockinfo.
 *
 * \return Integer:
 * \li \c numBytes - On Success, no of bytes received.
 * \li \c UPNP_E_TIMEDOUT - Timeout.
 * \li \c UPNP_E_SOCKET_ERROR - Error on socket calls.
 */
int sock_write(
	/*! Socket Information Object. */
	IN SOCKINFO *info,
	/*! Buffer to send data from. */
	IN const char *buffer,
	/*! Size of the buffer. */
	IN int bufsize,
	/*! timeout value. */
	INOUT int *timeoutSecs);

/*!
 * \brief Shutsdown the socket using the ShutdownMethod to indicate whether
 * sends and receives on the socket will be dis-allowed.
 *
 * After shutting down the socket, closesocket is called to release system
 * resources used by the socket calls.
 *
 * \return Integer:
 * \li \c UPNP_E_SOCKET_ERROR on failure.
 * \li \c UPNP_E_SUCCESS on success.
 */
int sock_destroy(
	/*! Socket Information Object. */
	INOUT SOCKINFO* info,
	/*! How to shutdown the socket. Used by sockets's shutdown(). */
	int ShutdownMethod);

/*!
 * \brief Closes the socket if it is different from -1.
 *
 * \return -1 if an error occurred or if the socket is -1.
 */
static UPNP_INLINE int sock_close(
	/*! Socket descriptor. */
	SOCKET sock)
{
	int ret = -1;

	if (sock != -1) {
		ret = UpnpCloseSocket(sock);
	}

	return ret;
}

#ifdef __cplusplus
}	/* #extern "C" */
#endif


#endif /* GENLIB_NET_SOCK_H */

