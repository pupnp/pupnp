/**************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation
 * All rights reserved.
 * Copyright (C) 2012 France Telecom All rights reserved.
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

#include "config.h"

#if EXCLUDE_MINISERVER == 0

/*!
 * \file
 *
 * \brief Implements the functionality and utility functions
 * used by the Miniserver module.
 *
 * The miniserver is a central point for processing all network requests.
 * It is made of:
 *   - The SSDP sockets for discovery.
 *   - The HTTP listeners for description / control / eventing.
 *
 */

#include "miniserver.h"

#include "httpreadwrite.h"
#include "ithread.h"
#include "ssdplib.h"
#include "statcodes.h"
#include "ThreadPool.h"
#include "unixutil.h" /* for socklen_t, EAFNOSUPPORT */
#include "upnpapi.h"
#include "upnputil.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

/*! . */
#define APPLICATION_LISTENING_PORT 49152

struct mserv_request_t {
	/*! Connection handle. */
	SOCKET connfd;
	/*! . */
	struct sockaddr_storage foreign_sockaddr;
};

/*! . */
typedef enum {
	/*! . */
	MSERV_IDLE,
	/*! . */
	MSERV_RUNNING,
	/*! . */
	MSERV_STOPPING
} MiniServerState;

/*! . */
uint16_t miniStopSockPort;

/*!
 * module vars
 */
static MiniServerState gMServState = MSERV_IDLE;
#ifdef INTERNAL_WEB_SERVER
static MiniServerCallback gGetCallback = NULL;
static MiniServerCallback gSoapCallback = NULL;
static MiniServerCallback gGenaCallback = NULL;

void SetHTTPGetCallback(MiniServerCallback callback)
{
	gGetCallback = callback;
}

#ifdef INCLUDE_DEVICE_APIS
void SetSoapCallback(MiniServerCallback callback)
{
	gSoapCallback = callback;
}
#endif /* INCLUDE_DEVICE_APIS */

void SetGenaCallback(MiniServerCallback callback)
{
	gGenaCallback = callback;
}

/*!
 * \brief Based on the type pf message, appropriate callback is issued.
 *
 * \return 0 on Success or HTTP_INTERNAL_SERVER_ERROR if Callback is NULL.
 */
static int dispatch_request(
	/*! [in] Socket Information object. */
	IN SOCKINFO *info,
	/*! [in] HTTP parser object. */
	http_parser_t *hparser)
{
	MiniServerCallback callback;

	switch (hparser->msg.method) {
	/* Soap Call */
	case SOAPMETHOD_POST:
	case HTTPMETHOD_MPOST:
		callback = gSoapCallback;
		break;
	/* Gena Call */
	case HTTPMETHOD_NOTIFY:
	case HTTPMETHOD_SUBSCRIBE:
	case HTTPMETHOD_UNSUBSCRIBE:
		UpnpPrintf(UPNP_INFO, MSERV, __FILE__, __LINE__,
			"miniserver %d: got GENA msg\n", info->socket);
			callback = gGenaCallback;
		break;
	/* HTTP server call */
	case HTTPMETHOD_GET:
	case HTTPMETHOD_POST:
	case HTTPMETHOD_HEAD:
	case HTTPMETHOD_SIMPLEGET:
		callback = gGetCallback;
		break;
	default:
		callback = NULL;
	}
	if (callback == NULL) {
		return HTTP_INTERNAL_SERVER_ERROR;
	}
	callback(hparser, &hparser->msg, info);

	return 0;
}

/*!
 * \brief Send Error Message.
 */
static UPNP_INLINE void handle_error(
	/*! [in] Socket Information object. */
	SOCKINFO *info,
	/*! [in] HTTP Error Code. */
	int http_error_code,
	/*! [in] Major Version Number. */
	int major,
	/*! [in] Minor Version Number. */
	int minor)
{
	http_SendStatusResponse(info, http_error_code, major, minor);
}

/*!
 * \brief Free memory assigned for handling request and unitialize socket
 * functionality.
 */
static void free_handle_request_arg(
	/*! [in] Request Message to be freed. */
	void *args)
{
	struct mserv_request_t *request = (struct mserv_request_t *)args;

	sock_close(request->connfd);
	free(request);
}

/*!
 * \brief Receive the request and dispatch it for handling.
 */
static void handle_request(
	/*! [in] Request Message to be handled. */
	void *args)
{
	SOCKINFO info;
	int http_error_code;
	int ret_code;
	int major = 1;
	int minor = 1;
	http_parser_t parser;
	http_message_t *hmsg = NULL;
	int timeout = HTTP_DEFAULT_TIMEOUT;
	struct mserv_request_t *request = (struct mserv_request_t *)args;
	SOCKET connfd = request->connfd;

	UpnpPrintf( UPNP_INFO, MSERV, __FILE__, __LINE__,
		"miniserver %d: READING\n", connfd );
	/* parser_request_init( &parser ); */ /* LEAK_FIX_MK */
	hmsg = &parser.msg;
	ret_code = sock_init_with_ip(
		&info, connfd, (struct sockaddr *)&request->foreign_sockaddr);
	if (ret_code != UPNP_E_SUCCESS) {
		free(request);
		httpmsg_destroy(hmsg);
		return;
	}
	/* read */
	ret_code = http_RecvMessage(
		&info, &parser, HTTPMETHOD_UNKNOWN, &timeout, &http_error_code);
	if (ret_code != 0) {
		goto error_handler;
	}
	UpnpPrintf(UPNP_INFO, MSERV, __FILE__, __LINE__,
		"miniserver %d: PROCESSING...\n", connfd);
	/* dispatch */
	http_error_code = dispatch_request(&info, &parser);
	if (http_error_code != 0) {
		goto error_handler;
	}
	http_error_code = 0;

error_handler:
	if (http_error_code > 0) {
		if (hmsg) {
			major = hmsg->major_version;
			minor = hmsg->minor_version;
		}
		handle_error(&info, http_error_code, major, minor);
	}
	sock_destroy(&info, SD_BOTH);
	httpmsg_destroy(hmsg);
	free(request);

	UpnpPrintf(UPNP_INFO, MSERV, __FILE__, __LINE__,
		"miniserver %d: COMPLETE\n", connfd);
}

/*!
 * \brief Initilize the thread pool to handle a request, sets priority for the
 * job and adds the job to the thread pool.
 */
static UPNP_INLINE void schedule_request_job(
	/*! [in] Socket Descriptor on which connection is accepted. */
	SOCKET connfd,
	/*! [in] Clients Address information. */
	struct sockaddr *clientAddr)
{
	struct mserv_request_t *request;
	ThreadPoolJob job;

	memset(&job, 0, sizeof(job));

	request = (struct mserv_request_t *)malloc(
		sizeof (struct mserv_request_t));
	if (request == NULL) {
		UpnpPrintf( UPNP_INFO, MSERV, __FILE__, __LINE__,
			"mserv %d: out of memory\n", connfd);
		sock_close(connfd);
		return;
	}

	request->connfd = connfd;
	memcpy(&request->foreign_sockaddr, clientAddr,
		sizeof(request->foreign_sockaddr));
	TPJobInit(&job, (start_routine)handle_request, (void *)request);
	TPJobSetFreeFunction(&job, free_handle_request_arg);
	TPJobSetPriority(&job, MED_PRIORITY);
	if (ThreadPoolAdd(&gMiniServerThreadPool, &job, NULL) != 0) {
		UpnpPrintf(UPNP_INFO, MSERV, __FILE__, __LINE__,
			"mserv %d: cannot schedule request\n", connfd);
		free(request);
		sock_close(connfd);
		return;
	}
}
#endif

static UPNP_INLINE void fdset_if_valid(SOCKET sock, fd_set *set)
{
	if (sock != INVALID_SOCKET) {
		FD_SET(sock, set);
	}
}

static void web_server_accept(SOCKET lsock, fd_set *set)
{
#ifdef INTERNAL_WEB_SERVER
	SOCKET asock;
	socklen_t clientLen;
	struct sockaddr_storage clientAddr;
	char errorBuffer[ERROR_BUFFER_LEN];

	if (lsock != INVALID_SOCKET && FD_ISSET(lsock, set)) {
		clientLen = sizeof(clientAddr);
		asock = accept(lsock, (struct sockaddr *)&clientAddr,
			&clientLen);
		if (asock == INVALID_SOCKET) {
			strerror_r(errno, errorBuffer, ERROR_BUFFER_LEN);
			UpnpPrintf(UPNP_INFO, MSERV, __FILE__, __LINE__,
				"miniserver: Error in accept(): %s\n",
				errorBuffer);
		} else {
			schedule_request_job(asock,
				(struct sockaddr *)&clientAddr);
		}
	}
#endif /* INTERNAL_WEB_SERVER */
}

static void ssdp_read(SOCKET rsock, fd_set *set)
{
	if (rsock != INVALID_SOCKET && FD_ISSET(rsock, set)) {
		readFromSSDPSocket(rsock);
	}
}

static int receive_from_stopSock(SOCKET ssock, fd_set *set)
{
	ssize_t byteReceived;
	socklen_t clientLen;
	struct sockaddr_storage clientAddr;
	char requestBuf[256];
	char buf_ntop[INET6_ADDRSTRLEN];

	if (FD_ISSET(ssock, set)) {
		clientLen = sizeof(clientAddr);
		memset((char *)&clientAddr, 0, sizeof(clientAddr));
		byteReceived = recvfrom(ssock, requestBuf,
			(size_t)25, 0, (struct sockaddr *)&clientAddr, &clientLen);
		if (byteReceived > 0) {
			requestBuf[byteReceived] = '\0';
			inet_ntop(AF_INET,
				&((struct sockaddr_in*)&clientAddr)->sin_addr,
				buf_ntop, sizeof(buf_ntop));
			UpnpPrintf( UPNP_INFO, MSERV, __FILE__, __LINE__,
				"Received response: %s From host %s \n",
				requestBuf, buf_ntop );
			UpnpPrintf( UPNP_PACKET, MSERV, __FILE__, __LINE__,
				"Received multicast packet: \n %s\n",
				requestBuf);
			if (NULL != strstr(requestBuf, "ShutDown")) {
				return 1;
			}
		}
	}

	return 0;
}

/*!
 * \brief Run the miniserver.
 *
 * The MiniServer accepts a new request and schedules a thread to handle the
 * new request. Checks for socket state and invokes appropriate read and
 * shutdown actions for the Miniserver and SSDP sockets.
 */
static void RunMiniServer(
	/*! [in] Socket Array. */
	MiniServerSockArray *miniSock)
{
	char errorBuffer[ERROR_BUFFER_LEN];
	fd_set expSet;
	fd_set rdSet;
	SOCKET maxMiniSock;
	int ret = 0;
	int stopSock = 0;

	maxMiniSock = 0;
	maxMiniSock = max(maxMiniSock, miniSock->miniServerSock4);
	maxMiniSock = max(maxMiniSock, miniSock->miniServerSock6);
	maxMiniSock = max(maxMiniSock, miniSock->miniServerStopSock);
	maxMiniSock = max(maxMiniSock, miniSock->ssdpSock4);
	maxMiniSock = max(maxMiniSock, miniSock->ssdpSock6);
	maxMiniSock = max(maxMiniSock, miniSock->ssdpSock6UlaGua);
#ifdef INCLUDE_CLIENT_APIS
	maxMiniSock = max(maxMiniSock, miniSock->ssdpReqSock4);
	maxMiniSock = max(maxMiniSock, miniSock->ssdpReqSock6);
#endif /* INCLUDE_CLIENT_APIS */
	++maxMiniSock;

	gMServState = MSERV_RUNNING;
	while (!stopSock) {
		FD_ZERO(&rdSet);
		FD_ZERO(&expSet);
		/* FD_SET()'s */
		FD_SET(miniSock->miniServerStopSock, &expSet);
		FD_SET(miniSock->miniServerStopSock, &rdSet);
		fdset_if_valid(miniSock->miniServerSock4, &rdSet);
		fdset_if_valid(miniSock->miniServerSock6, &rdSet);
		fdset_if_valid(miniSock->ssdpSock4, &rdSet);
		fdset_if_valid(miniSock->ssdpSock6, &rdSet);
		fdset_if_valid(miniSock->ssdpSock6UlaGua, &rdSet);
#ifdef INCLUDE_CLIENT_APIS
		fdset_if_valid(miniSock->ssdpReqSock4, &rdSet);
		fdset_if_valid(miniSock->ssdpReqSock6, &rdSet);
#endif /* INCLUDE_CLIENT_APIS */
		/* select() */
		ret = select((int) maxMiniSock, &rdSet, NULL, &expSet, NULL);
		if (ret == SOCKET_ERROR && errno == EINTR) {
			continue;
		}
		if (ret == SOCKET_ERROR) {
			strerror_r(errno, errorBuffer, ERROR_BUFFER_LEN);
			UpnpPrintf(UPNP_CRITICAL, SSDP, __FILE__, __LINE__,
				"Error in select(): %s\n", errorBuffer);
			continue;
		} else {
			web_server_accept(miniSock->miniServerSock4, &rdSet);
			web_server_accept(miniSock->miniServerSock6, &rdSet);
#ifdef INCLUDE_CLIENT_APIS
			ssdp_read(miniSock->ssdpReqSock4, &rdSet);
			ssdp_read(miniSock->ssdpReqSock6, &rdSet);
#endif /* INCLUDE_CLIENT_APIS */
			ssdp_read(miniSock->ssdpSock4, &rdSet);
			ssdp_read(miniSock->ssdpSock6, &rdSet);
			ssdp_read(miniSock->ssdpSock6UlaGua, &rdSet);
			stopSock = receive_from_stopSock(
				miniSock->miniServerStopSock, &rdSet);
		}
	}
	/* Close all sockets. */
	sock_close(miniSock->miniServerSock4);
	sock_close(miniSock->miniServerSock6);
	sock_close(miniSock->miniServerStopSock);
	sock_close(miniSock->ssdpSock4);
	sock_close(miniSock->ssdpSock6);
	sock_close(miniSock->ssdpSock6UlaGua);
#ifdef INCLUDE_CLIENT_APIS
	sock_close(miniSock->ssdpReqSock4);
	sock_close(miniSock->ssdpReqSock6);
#endif /* INCLUDE_CLIENT_APIS */
	/* Free minisock. */
	free(miniSock);
	gMServState = MSERV_IDLE;

	return;
}

/*!
 * \brief Returns port to which socket, sockfd, is bound.
 *
 * \return -1 on error; check errno. 0 if successfull.
 */
static int get_port(
	/*! [in] Socket descriptor. */
	SOCKET sockfd,
	/*! [out] The port value if successful, otherwise, untouched. */
	uint16_t *port)
{
	struct sockaddr_storage sockinfo;
	socklen_t len;
	int code;

	len = sizeof(sockinfo);
	code = getsockname(sockfd, (struct sockaddr *)&sockinfo, &len);
	if (code == -1) {
		return -1;
	}
	if (sockinfo.ss_family == AF_INET) {
		*port = ntohs(((struct sockaddr_in*)&sockinfo)->sin_port);
	} else if(sockinfo.ss_family == AF_INET6) {
		*port = ntohs(((struct sockaddr_in6*)&sockinfo)->sin6_port);
	}
	UpnpPrintf(UPNP_INFO, MSERV, __FILE__, __LINE__,
		"sockfd = %d, .... port = %d\n", sockfd, (int)*port);

	return 0;
}

#ifdef INTERNAL_WEB_SERVER
/*!
 * \brief Creates a STREAM socket, binds to INADDR_ANY and listens for
 * incoming connecttions. Returns the actual port which the sockets
 * sub-system returned. 
 *
 * Also creates a DGRAM socket, binds to the loop back address and 
 * returns the port allocated by the socket sub-system.
 *
 * \return
 *	\li UPNP_E_OUTOF_SOCKET: Failed to create a socket.
 *	\li UPNP_E_SOCKET_BIND: Bind() failed.
 *	\li UPNP_E_LISTEN: Listen() failed.	
 *	\li UPNP_E_INTERNAL_ERROR: Port returned by the socket layer is < 0.
 *	\li UPNP_E_SUCCESS: Success.
 */
static int get_miniserver_sockets(
	/*! [in] Socket Array. */
	MiniServerSockArray *out,
	/*! [in] port on which the server is listening for incoming IPv4
	 * connections. */
	uint16_t listen_port4
#ifdef UPNP_ENABLE_IPV6
	,
	/*! [in] port on which the server is listening for incoming IPv6
	 * connections. */
	uint16_t listen_port6
#endif
	)
{
	char errorBuffer[ERROR_BUFFER_LEN];
	struct sockaddr_storage __ss_v4;
	struct sockaddr_in* serverAddr4 = (struct sockaddr_in*)&__ss_v4;
	SOCKET listenfd4;
	uint16_t actual_port4 = 0u;
#ifdef UPNP_ENABLE_IPV6
	struct sockaddr_storage __ss_v6;
	struct sockaddr_in6* serverAddr6 = (struct sockaddr_in6*)&__ss_v6;
	SOCKET listenfd6;
	uint16_t actual_port6 = 0u;
	int onOff;
#endif
	int ret_code;
	int reuseaddr_on = 0;
	int sockError = UPNP_E_SUCCESS;
	int errCode = 0;

	/* Create listen socket for IPv4/IPv6. An error here may indicate
	 * that we don't have an IPv4/IPv6 stack. */
	listenfd4 = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd4 == INVALID_SOCKET) {
		return UPNP_E_OUTOF_SOCKET;
	}
#ifdef UPNP_ENABLE_IPV6
	listenfd6 = socket(AF_INET6, SOCK_STREAM, 0);
	if (listenfd6 == INVALID_SOCKET) {
		sock_close(listenfd4);
		return UPNP_E_OUTOF_SOCKET;
	}
	onOff = 1;
	sockError = setsockopt(listenfd6, IPPROTO_IPV6, IPV6_V6ONLY,
			 (char *)&onOff, sizeof(onOff));
	if (sockError == SOCKET_ERROR) {
		sock_close(listenfd4);
		sock_close(listenfd6);
		return UPNP_E_SOCKET_BIND;
	}
#endif
	/* As per the IANA specifications for the use of ports by applications
	 * override the listen port passed in with the first available. */
	if (listen_port4 < APPLICATION_LISTENING_PORT) {
		listen_port4 = (uint16_t)APPLICATION_LISTENING_PORT;
	}
#ifdef UPNP_ENABLE_IPV6
	if (listen_port6 < APPLICATION_LISTENING_PORT) {
		listen_port6 = (uint16_t)APPLICATION_LISTENING_PORT;
	}
#endif
	memset(&__ss_v4, 0, sizeof (__ss_v4));
	serverAddr4->sin_family = (sa_family_t)AF_INET;
	serverAddr4->sin_addr.s_addr = htonl(INADDR_ANY);
#ifdef UPNP_ENABLE_IPV6
	memset(&__ss_v6, 0, sizeof (__ss_v6));
	serverAddr6->sin6_family = (sa_family_t)AF_INET6;
	serverAddr6->sin6_addr = in6addr_any;
#endif
	/* Getting away with implementation of re-using address:port and
	 * instead choosing to increment port numbers.
	 * Keeping the re-use address code as an optional behaviour that
	 * can be turned on if necessary.
	 * TURN ON the reuseaddr_on option to use the option. */
	if (reuseaddr_on) {
		/* THIS IS ALLOWS US TO BIND AGAIN IMMEDIATELY
		 * AFTER OUR SERVER HAS BEEN CLOSED
		 * THIS MAY CAUSE TCP TO BECOME LESS RELIABLE
		 * HOWEVER IT HAS BEEN SUGESTED FOR TCP SERVERS. */
		UpnpPrintf(UPNP_INFO, MSERV, __FILE__, __LINE__,
			"get_miniserver_sockets: resuseaddr is set.\n");
		if (listenfd4 != INVALID_SOCKET) {
			sockError = setsockopt(listenfd4, SOL_SOCKET,
				SO_REUSEADDR,
				(const char *)&reuseaddr_on, sizeof (int));
			if (sockError == SOCKET_ERROR) {
				sock_close(listenfd4);
#ifdef UPNP_ENABLE_IPV6
				sock_close(listenfd6);
#endif
				return UPNP_E_SOCKET_BIND;
			}
			serverAddr4->sin_port = htons(listen_port4);
			sockError = bind(listenfd4,
				(struct sockaddr *)&__ss_v4,
				sizeof (__ss_v4));
			if (sockError == SOCKET_ERROR) {
				strerror_r(errno, errorBuffer,
					ERROR_BUFFER_LEN);
				UpnpPrintf(UPNP_INFO, MSERV,
					__FILE__, __LINE__,
					"get_miniserver_sockets: "
					"Error in IPv4 bind(): %s\n", 
					errorBuffer);
				sock_close(listenfd4);
#ifdef UPNP_ENABLE_IPV6
				sock_close(listenfd6);
#endif
				/* Bind failed */
				return UPNP_E_SOCKET_BIND;
			}
		}
#ifdef UPNP_ENABLE_IPV6
		if (listenfd6 != INVALID_SOCKET) {
			sockError = setsockopt(listenfd6, SOL_SOCKET,
				SO_REUSEADDR,
			(const char *)&reuseaddr_on, sizeof (int));
			if (sockError == SOCKET_ERROR) {
				sock_close(listenfd4);
				sock_close(listenfd6);
				return UPNP_E_SOCKET_BIND;
			}
			serverAddr6->sin6_port = htons(listen_port6);
			sockError = bind(listenfd6,
				(struct sockaddr *)&__ss_v6,
				sizeof (__ss_v6));
			if (sockError == SOCKET_ERROR) {
				strerror_r(errno, errorBuffer,
					ERROR_BUFFER_LEN);
				UpnpPrintf(UPNP_INFO, MSERV,
					__FILE__, __LINE__,
					"get_miniserver_sockets: "
					"Error in IPv6 bind(): %s\n", 
					errorBuffer);
				sock_close(listenfd4);
				sock_close(listenfd6);
				/* Bind failed */
				return UPNP_E_SOCKET_BIND;
			}
		}
#endif  /* IPv6 */
	} else {
		if (listenfd4 != INVALID_SOCKET) {
			uint16_t orig_listen_port4 = listen_port4;
			do {
				serverAddr4->sin_port = htons(listen_port4++);
				sockError = bind(listenfd4,
					(struct sockaddr *)serverAddr4,
					sizeof(*serverAddr4));
				if (sockError == SOCKET_ERROR) {
#ifdef WIN32
					errCode = WSAGetLastError();
#else
					errCode = errno;
#endif
					if (errno == EADDRINUSE) {
						errCode = 1;
					}
				} else {
					errCode = 0;
				}
			} while (errCode != 0 &&
				 listen_port4 >= orig_listen_port4);
			if (sockError == SOCKET_ERROR) {
				strerror_r(errno, errorBuffer,
					ERROR_BUFFER_LEN);
				UpnpPrintf(UPNP_INFO, MSERV,
					__FILE__, __LINE__,
					"get_miniserver_sockets: "
					"Error in IPv4 bind(): %s\n",
					errorBuffer);
				sock_close(listenfd4);
#ifdef UPNP_ENABLE_IPV6
				sock_close(listenfd6);
#endif
				/* Bind failied. */
				return UPNP_E_SOCKET_BIND;
			}
		}
#ifdef UPNP_ENABLE_IPV6
		if (listenfd6 != INVALID_SOCKET) {
			uint16_t orig_listen_port6 = listen_port6;
			do {
				serverAddr6->sin6_port = htons(listen_port6++);
				sockError = bind(listenfd6,
					(struct sockaddr *)serverAddr6,
					sizeof(*serverAddr6));
				if (sockError == SOCKET_ERROR) {
#ifdef WIN32
					errCode = WSAGetLastError();
#else
					errCode = errno; 
#endif
					if (errno == EADDRINUSE) {
						errCode = 1;
					}
				} else {
					errCode = 0;
				}
			} while (errCode != 0 &&
				 listen_port6 >= orig_listen_port6);
			if (sockError == SOCKET_ERROR) {
				strerror_r(errno, errorBuffer,
					ERROR_BUFFER_LEN);
				UpnpPrintf(UPNP_INFO, MSERV,
					__FILE__, __LINE__,
					"get_miniserver_sockets: "
					"Error in IPv6 bind(): %s\n",
					errorBuffer);
				sock_close(listenfd4);
				sock_close(listenfd6);
				/* Bind failied. */
				return UPNP_E_SOCKET_BIND;
			}
		}
#endif
	}
	UpnpPrintf(UPNP_INFO, MSERV, __FILE__, __LINE__,
		"get_miniserver_sockets: bind successful\n");
	if (listenfd4 != INVALID_SOCKET) {
		ret_code = listen(listenfd4, SOMAXCONN);
		if (ret_code == SOCKET_ERROR) {
			strerror_r(errno, errorBuffer, ERROR_BUFFER_LEN);
			UpnpPrintf(UPNP_INFO, MSERV, __FILE__, __LINE__,
				"mserv start: Error in IPv4 listen(): %s\n",
				errorBuffer);
			sock_close(listenfd4);
#ifdef UPNP_ENABLE_IPV6
			sock_close(listenfd6);
#endif
			return UPNP_E_LISTEN;
		}
		ret_code = get_port(listenfd4, &actual_port4);
		if (ret_code < 0) {
			sock_close(listenfd4);
#ifdef UPNP_ENABLE_IPV6
			sock_close(listenfd6);
#endif
			return UPNP_E_INTERNAL_ERROR;
		}
		out->miniServerPort4 = actual_port4;
	}
#ifdef UPNP_ENABLE_IPV6
	if (listenfd6 != INVALID_SOCKET) {
		ret_code = listen(listenfd6, SOMAXCONN);
		if (ret_code == SOCKET_ERROR) {
			strerror_r(errno, errorBuffer, ERROR_BUFFER_LEN);
			UpnpPrintf(UPNP_INFO, MSERV, __FILE__, __LINE__,
				"mserv start: Error in IPv6 listen(): %s\n",
				errorBuffer);
			sock_close(listenfd4);
			sock_close(listenfd6);
			return UPNP_E_LISTEN;
		}
		ret_code = get_port(listenfd6, &actual_port6);
		if (ret_code < 0) {
			sock_close(listenfd4);
			sock_close(listenfd6);
			return UPNP_E_INTERNAL_ERROR;
		}
		out->miniServerPort6 = actual_port6;
	}
#endif
	out->miniServerSock4 = listenfd4;
#ifdef UPNP_ENABLE_IPV6
	out->miniServerSock6 = listenfd6;
#endif
	return UPNP_E_SUCCESS;
}
#endif /* INTERNAL_WEB_SERVER */

/*!
 * \brief Creates the miniserver STOP socket. This socket is created and 
 *  listened on to know when it is time to stop the Miniserver.
 *
 * \return 
 * \li \c UPNP_E_OUTOF_SOCKET: Failed to create a socket.
 * \li \c UPNP_E_SOCKET_BIND: Bind() failed.
 * \li \c UPNP_E_INTERNAL_ERROR: Port returned by the socket layer is < 0.
 * \li \c UPNP_E_SUCCESS: Success.
 */
static int get_miniserver_stopsock(
	/*! [in] Miniserver Socket Array. */
	MiniServerSockArray *out)
{
	char errorBuffer[ERROR_BUFFER_LEN];
	struct sockaddr_in stop_sockaddr;
	SOCKET miniServerStopSock = 0;
	int ret = 0;

	miniServerStopSock = socket(AF_INET, SOCK_DGRAM, 0);
	if (miniServerStopSock == INVALID_SOCKET) {
		strerror_r(errno, errorBuffer, ERROR_BUFFER_LEN);
		UpnpPrintf(UPNP_CRITICAL, MSERV, __FILE__, __LINE__,
			"Error in socket(): %s\n", errorBuffer);
		return UPNP_E_OUTOF_SOCKET;
	}
	/* Bind to local socket. */
	memset(&stop_sockaddr, 0, sizeof (stop_sockaddr));
	stop_sockaddr.sin_family = (sa_family_t)AF_INET;
	stop_sockaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	ret = bind(miniServerStopSock, (struct sockaddr *)&stop_sockaddr,
		sizeof(stop_sockaddr));
	if (ret == SOCKET_ERROR) {
		UpnpPrintf(UPNP_CRITICAL,
		MSERV, __FILE__, __LINE__,
			"Error in binding localhost!!!\n");
		sock_close(miniServerStopSock);
		return UPNP_E_SOCKET_BIND;
	}
	ret = get_port(miniServerStopSock, &miniStopSockPort);
	if (ret < 0) {
		sock_close(miniServerStopSock);
		return UPNP_E_INTERNAL_ERROR;
	}
	out->miniServerStopSock = miniServerStopSock;
	out->stopPort = miniStopSockPort;

	return UPNP_E_SUCCESS;
}

static UPNP_INLINE void InitMiniServerSockArray(MiniServerSockArray *miniSocket)
{
	miniSocket->miniServerSock4 = INVALID_SOCKET;
	miniSocket->miniServerSock6 = INVALID_SOCKET;
	miniSocket->miniServerStopSock = INVALID_SOCKET;
	miniSocket->ssdpSock4 = INVALID_SOCKET;
	miniSocket->ssdpSock6 = INVALID_SOCKET;
	miniSocket->ssdpSock6UlaGua = INVALID_SOCKET;
	miniSocket->stopPort = 0u;
	miniSocket->miniServerPort4 = 0u;
	miniSocket->miniServerPort6 = 0u;
#ifdef INCLUDE_CLIENT_APIS
	miniSocket->ssdpReqSock4 = INVALID_SOCKET;
	miniSocket->ssdpReqSock6 = INVALID_SOCKET;
#endif /* INCLUDE_CLIENT_APIS */
}

int StartMiniServer(
	/*! [in,out] Port on which the server listens for incoming IPv4
	 * connections. */
	uint16_t *listen_port4, 
	/*! [in,out] Port on which the server listens for incoming IPv6
	 * connections. */
	uint16_t *listen_port6)
{
	int ret_code;
	int count;
	int max_count = 10000;
	MiniServerSockArray *miniSocket;
	ThreadPoolJob job;

	memset(&job, 0, sizeof(job));

	switch (gMServState) {
	case MSERV_IDLE:
		break;
	default:
		/* miniserver running. */
		return UPNP_E_INTERNAL_ERROR;
	}
	miniSocket = (MiniServerSockArray *)malloc(
		sizeof (MiniServerSockArray));
	if (!miniSocket) {
		return UPNP_E_OUTOF_MEMORY;
	}
	InitMiniServerSockArray(miniSocket);
#ifdef INTERNAL_WEB_SERVER
	/* V4 and V6 http listeners. */
	ret_code = get_miniserver_sockets(
		miniSocket, *listen_port4
#ifdef UPNP_ENABLE_IPV6
		, *listen_port6
#endif
		);
	if (ret_code != UPNP_E_SUCCESS) {
		free(miniSocket);
		return ret_code;
	}
#endif
	/* Stop socket (To end miniserver processing). */
	ret_code = get_miniserver_stopsock(miniSocket);
	if (ret_code != UPNP_E_SUCCESS) {
		sock_close(miniSocket->miniServerSock4);
		sock_close(miniSocket->miniServerSock6);
		free(miniSocket);
		return ret_code;
	}
	/* SSDP socket for discovery/advertising. */
	ret_code = get_ssdp_sockets(miniSocket);
	if (ret_code != UPNP_E_SUCCESS) {
		sock_close(miniSocket->miniServerSock4);
		sock_close(miniSocket->miniServerSock6);
		sock_close(miniSocket->miniServerStopSock);
		free(miniSocket);
		return ret_code;
	}
	TPJobInit(&job, (start_routine)RunMiniServer, (void *)miniSocket);
	TPJobSetPriority(&job, MED_PRIORITY);
	TPJobSetFreeFunction(&job, (free_routine)free);
	ret_code = ThreadPoolAddPersistent(&gMiniServerThreadPool, &job, NULL);
	if (ret_code < 0) {
		sock_close(miniSocket->miniServerSock4);
		sock_close(miniSocket->miniServerSock6);
		sock_close(miniSocket->miniServerStopSock);
		sock_close(miniSocket->ssdpSock4);
		sock_close(miniSocket->ssdpSock6);
		sock_close(miniSocket->ssdpSock6UlaGua);
#ifdef INCLUDE_CLIENT_APIS
		sock_close(miniSocket->ssdpReqSock4);
		sock_close(miniSocket->ssdpReqSock6);
#endif /* INCLUDE_CLIENT_APIS */
		free(miniSocket);
		return UPNP_E_OUTOF_MEMORY;
	}
	/* Wait for miniserver to start. */
	count = 0;
	while (gMServState != (MiniServerState)MSERV_RUNNING && count < max_count) {
		/* 0.05s */
		usleep(50u * 1000u);
		count++;
	}
	if (count >= max_count) {
		/* Took it too long to start that thread. */
		sock_close(miniSocket->miniServerSock4);
		sock_close(miniSocket->miniServerSock6);
		sock_close(miniSocket->miniServerStopSock);
		sock_close(miniSocket->ssdpSock4);
		sock_close(miniSocket->ssdpSock6);
		sock_close(miniSocket->ssdpSock6UlaGua);
#ifdef INCLUDE_CLIENT_APIS
		sock_close(miniSocket->ssdpReqSock4);
		sock_close(miniSocket->ssdpReqSock6);
#endif /* INCLUDE_CLIENT_APIS */
		return UPNP_E_INTERNAL_ERROR;
	}
#ifdef INTERNAL_WEB_SERVER
	*listen_port4 = miniSocket->miniServerPort4;
	*listen_port6 = miniSocket->miniServerPort6;
#endif

	return UPNP_E_SUCCESS;
}

int StopMiniServer()
{
	char errorBuffer[ERROR_BUFFER_LEN];
	socklen_t socklen = sizeof (struct sockaddr_in);
	SOCKET sock;
	struct sockaddr_in ssdpAddr;
	char buf[256] = "ShutDown";
	size_t bufLen = strlen(buf);

	switch(gMServState) {
	case MSERV_RUNNING:
		gMServState = MSERV_STOPPING;
		break;
	default:
		return 0;
	}
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET) {
		strerror_r(errno, errorBuffer, ERROR_BUFFER_LEN);
		UpnpPrintf(UPNP_INFO, SSDP, __FILE__, __LINE__,
			"SSDP_SERVER: StopSSDPServer: Error in socket() %s\n",
			errorBuffer);
		return 0;
	}
	while(gMServState != (MiniServerState)MSERV_IDLE) {
		ssdpAddr.sin_family = (sa_family_t)AF_INET;
		ssdpAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
		ssdpAddr.sin_port = htons(miniStopSockPort);
		sendto(sock, buf, bufLen, 0,
			(struct sockaddr *)&ssdpAddr, socklen);
		usleep(1000u);
		if (gMServState == (MiniServerState)MSERV_IDLE) {
			break;
		}
		isleep(1u);
	}
	sock_close(sock);

	return 0;
}
#endif /* EXCLUDE_MINISERVER */
