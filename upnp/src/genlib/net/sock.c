/**************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation 
 * All rights reserved. 
 * Copyright (c) 2012 France Telecom All rights reserved. 
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

/*!
 * \addtogroup Sock
 * 
 * @{
 * 
 * \file
 *
 * \brief Implements the sockets functionality.
 */

#include "config.h"

#include "sock.h"

#include "unixutil.h"	/* for socklen_t, EAFNOSUPPORT */
#include "upnp.h"
#include "UpnpStdInt.h" /* for ssize_t */

#include "upnpdebug.h"
#include "upnputil.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>	/* for F_GETFL, F_SETFL, O_NONBLOCK */
#include <time.h>
#include <string.h>

#ifdef UPNP_ENABLE_OPEN_SSL
#include <openssl/ssl.h>
#endif

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#ifdef UPNP_ENABLE_OPEN_SSL
/* OpenSSL context defined in upnpapi.c */
extern SSL_CTX *gSslCtx;
#endif

int sock_init(SOCKINFO *info, SOCKET sockfd)
{
	assert(info);

	memset(info, 0, sizeof(SOCKINFO));
	info->socket = sockfd;

	return UPNP_E_SUCCESS;
}

int sock_init_with_ip(SOCKINFO *info, SOCKET sockfd,
	struct sockaddr *foreign_sockaddr)
{
	int ret;

	ret = sock_init(info, sockfd);
	if (ret != UPNP_E_SUCCESS) {
		return ret;
	}

	memcpy(&info->foreign_sockaddr, foreign_sockaddr,
	       sizeof(info->foreign_sockaddr));

	return UPNP_E_SUCCESS;
}

#ifdef UPNP_ENABLE_OPEN_SSL
int sock_ssl_connect(SOCKINFO *info)
{
	int status = 0;
	info->ssl = SSL_new(gSslCtx);
	if (!info->ssl) {
		return UPNP_E_SOCKET_ERROR;
	}
	status = SSL_set_fd(info->ssl, info->socket);
	if (status == 1) {
		status = SSL_connect(info->ssl);
	}
	if (status == 1) {
		return UPNP_E_SUCCESS;
	}
	return UPNP_E_SOCKET_ERROR;
}
#endif

int sock_destroy(SOCKINFO *info, int ShutdownMethod)
{
	int ret = UPNP_E_SUCCESS;
	char errorBuffer[ERROR_BUFFER_LEN];

	if (info->socket != INVALID_SOCKET) {
#ifdef UPNP_ENABLE_OPEN_SSL
		if (info->ssl) {
			SSL_shutdown(info->ssl);
			SSL_free(info->ssl);
			info->ssl = NULL;
		}
#endif
		if (shutdown(info->socket, ShutdownMethod) == -1) {
			strerror_r(errno, errorBuffer, ERROR_BUFFER_LEN);
			UpnpPrintf(UPNP_INFO, HTTP, __FILE__, __LINE__,
				   "Error in shutdown: %s\n", errorBuffer);
		}
		if (sock_close(info->socket) == -1) {
			ret = UPNP_E_SOCKET_ERROR;
		}
		info->socket = INVALID_SOCKET;
	}

	return ret;
}

/*!
 * \brief Receives or sends data. Also returns the time taken to receive or
 * send data.
 *
 * \return
 *	\li \c numBytes - On Success, no of bytes received or sent or
 *	\li \c UPNP_E_TIMEDOUT - Timeout
 *	\li \c UPNP_E_SOCKET_ERROR - Error on socket calls
 */
static int sock_read_write(
	/*! [in] Socket Information Object. */
	SOCKINFO *info,
	/*! [out] Buffer to get data to or send data from. */
	char *buffer,
	/*! [in] Size of the buffer. */
	size_t bufsize,
	/*! [in] timeout value. */
	int *timeoutSecs,
	/*! [in] Boolean value specifying read or write option. */
	int bRead)
{
	int retCode;
	fd_set readSet;
	fd_set writeSet;
	struct timeval timeout;
	long numBytes;
	time_t start_time = time(NULL);
	SOCKET sockfd = info->socket;
	long bytes_sent = 0;
	size_t byte_left = (size_t)0;
	ssize_t num_written;

	FD_ZERO(&readSet);
	FD_ZERO(&writeSet);
	if (bRead)
		FD_SET(sockfd, &readSet);
	else
		FD_SET(sockfd, &writeSet);
	timeout.tv_sec = *timeoutSecs;
	timeout.tv_usec = 0;
	while (TRUE) {
		if (*timeoutSecs < 0)
			retCode = select(sockfd + 1, &readSet, &writeSet,
				NULL, NULL);
		else
			retCode = select(sockfd + 1, &readSet, &writeSet,
				NULL, &timeout);
		if (retCode == 0)
			return UPNP_E_TIMEDOUT;
		if (retCode == -1) {
			if (errno == EINTR)
				continue;
			return UPNP_E_SOCKET_ERROR;
		} else
			/* read or write. */
			break;
	}
#ifdef SO_NOSIGPIPE
	{
		int old;
		int set = 1;
		socklen_t olen = sizeof(old);
		getsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, &old, &olen);
		setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, &set, sizeof(set));
#endif
		if (bRead) {
#ifdef UPNP_ENABLE_OPEN_SSL
            if (info->ssl) {
                numBytes = (long)SSL_read(info->ssl,
                                          buffer, (size_t)bufsize);
            }
            else {
#endif
			/* read data. */
			numBytes = (long)recv(sockfd, buffer, bufsize, MSG_NOSIGNAL);
#ifdef UPNP_ENABLE_OPEN_SSL
            }
#endif
		} else {
			byte_left = bufsize;
			bytes_sent = 0;
			while (byte_left != (size_t)0) {
#ifdef UPNP_ENABLE_OPEN_SSL
                if (info->ssl) {
                    num_written = SSL_write(info->ssl,
                                            buffer + bytes_sent, byte_left);
                }
                else {
#endif
				/* write data. */
				num_written = send(sockfd,
					buffer + bytes_sent, byte_left,
					MSG_DONTROUTE | MSG_NOSIGNAL);
#ifdef UPNP_ENABLE_OPEN_SSL
                }
#endif
				if (num_written == -1) {
#ifdef SO_NOSIGPIPE
					setsockopt(sockfd, SOL_SOCKET,
						SO_NOSIGPIPE, &old, olen);
#endif
					return (int)num_written;
				}
				byte_left -= (size_t)num_written;
				bytes_sent += num_written;
			}
			numBytes = bytes_sent;
		}
#ifdef SO_NOSIGPIPE
		setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, &old, olen);
	}
#endif
	if (numBytes < 0)
		return UPNP_E_SOCKET_ERROR;
	/* subtract time used for reading/writing. */
	if (*timeoutSecs != 0)
		*timeoutSecs -= (int)(time(NULL) - start_time);

	return (int)numBytes;
}

int sock_read(SOCKINFO *info, char *buffer, size_t bufsize, int *timeoutSecs)
{
	return sock_read_write(info, buffer, bufsize, timeoutSecs, TRUE);
}

int sock_write(SOCKINFO *info, const char *buffer, size_t bufsize, int *timeoutSecs)
{
	/* Consciently removing constness. */
	return sock_read_write(info, (char *)buffer, bufsize, timeoutSecs, FALSE);
}

int sock_make_blocking(SOCKET sock)
{
#ifdef WIN32
	u_long val = 0;
	return ioctlsocket(sock, FIONBIO, &val);
#else
	int val;

	val = fcntl(sock, F_GETFL, 0);
	if (fcntl(sock, F_SETFL, val & ~O_NONBLOCK) == -1) {
		return -1;
	}
#endif
	return 0;
}


int sock_make_no_blocking(SOCKET sock)
{
#ifdef WIN32
	u_long val = 1;
	return ioctlsocket(sock, FIONBIO, &val);
#else /* WIN32 */
	int val;

	val = fcntl(sock, F_GETFL, 0);
	if (fcntl(sock, F_SETFL, val | O_NONBLOCK) == -1) {
		return -1;
	}
#endif /* WIN32 */
	return 0;
}

/* @} Sock */
