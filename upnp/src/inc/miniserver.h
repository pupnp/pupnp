#ifndef MINISERVER_H
#define MINISERVER_H

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

/*!
 * \file
 */

#include "UpnpStdInt.h" /* IWYU pragma: keep */

#include "httpparser.h"
#include "sock.h"

extern SOCKET gMiniServerStopSock;

typedef struct MServerSockArray
{
	/*! IPv4 socket for listening for miniserver requests. */
	SOCKET miniServerSock4;
	/*! IPv6 LLA Socket for listening for miniserver requests. */
	SOCKET miniServerSock6;
	/*! IPv6 ULA or GUA Socket for listening for miniserver requests. */
	SOCKET miniServerSock6UlaGua;
	/*! Socket for stopping miniserver */
	SOCKET miniServerStopSock;
	/*! IPv4 SSDP Socket for incoming advertisements and search requests. */
	SOCKET ssdpSock4;
	/*! IPv6 LLA SSDP Socket for incoming advertisements and search
	 * requests.
	 */
	SOCKET ssdpSock6;
	/*! IPv6 ULA or GUA SSDP Socket for incoming advertisements and search
	 * requests. */
	SOCKET ssdpSock6UlaGua;
	/* ! . */
	uint16_t stopPort;
	/* ! . */
	uint16_t miniServerPort4;
	/* ! . */
	uint16_t miniServerPort6;
	/* ! . */
	uint16_t miniServerPort6UlaGua;
#ifdef INCLUDE_CLIENT_APIS
	/*! IPv4 SSDP socket for sending search requests and receiving search
	 * replies */
	SOCKET ssdpReqSock4;
	/*! IPv6 SSDP socket for sending search requests and receiving search
	 * replies */
	SOCKET ssdpReqSock6;
#endif /* INCLUDE_CLIENT_APIS */
} MiniServerSockArray;

/*! . */
typedef void (*MiniServerCallback)(
	/* ! [in] . */
	http_parser_t *parser,
	/* ! [in] . */
	http_message_t *request,
	/* ! [in] . */
	SOCKINFO *info);

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * \brief Set HTTP Get Callback.
 */
void SetHTTPGetCallback(
	/*! [in] HTTP Callback to be invoked . */
	MiniServerCallback callback);

/*!
 * \brief Set SOAP Callback.
 */
#ifdef INCLUDE_DEVICE_APIS
void SetSoapCallback(
	/*! [in] SOAP Callback to be invoked . */
	MiniServerCallback callback);
#else  /* INCLUDE_DEVICE_APIS */
static UPNP_INLINE void SetSoapCallback(MiniServerCallback callback)
{
	(void)callback;
}
#endif /* INCLUDE_DEVICE_APIS */
/*!
 * \brief Set GENA Callback.
 */
void SetGenaCallback(
	/*! [in] GENA Callback to be invoked. */
	MiniServerCallback callback);

/*!
 * \brief Start the MiniServer's SSDP and loopback control sockets.
 *
 * HTTP listeners are enabled separately by SetMiniServerListeners().
 * \return UPNP_E_SUCCESS, or a socket/thread-pool error.
 */
int StartMiniServer(void);

/*!
 * \brief Stop and Shutdown the MiniServer and free socket resources.
 *
 * \return Always returns 0.
 */
int StopMiniServer(void);

/* Release connection-list storage after all MiniServer workers have joined. */
void CleanupMiniServer(void);

/* Change HTTP listeners without restarting SSDP. */
int SetMiniServerListeners(int enable, uint16_t port);

/*!
 * \brief Shutdown all active socket connections to interrupt blocking
 * operations.
 */
void shutdown_all_active_connections(void);

#ifdef __cplusplus
} /* extern C */
#endif

#endif /* MINISERVER_H */
