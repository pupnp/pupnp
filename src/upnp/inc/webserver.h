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

#ifndef GENLIB_NET_HTTP_WEBSERVER_H
#define GENLIB_NET_HTTP_WEBSERVER_H

#include <time.h>
#include "sock.h"
#include "httpparser.h"

#ifdef __cplusplus
extern "C" {
#endif


struct SendInstruction
{
	int IsVirtualFile;
	int IsChunkActive;
	int IsRangeActive;
	int IsTrailers;
	char RangeHeader[200];
	char AcceptLanguageHeader[200];
	off_t RangeOffset;
	/*! Read from local source and send on the network. */
	off_t ReadSendSize;
	/*! Recv from the network and write into local file. */
	long RecvWriteSize;
	/* Later few more member could be added depending
	 * on the requirement.*/
};


/*!
 * \brief Initilialize the different documents. Initialize the memory
 * for root directory for web server. Call to initialize global XML
 * document. Sets bWebServerState to WEB_SERVER_ENABLED.
 *
 * \note alias_content is not freed here
 *
 * \return
 * \li \c 0 - OK
 * \li \c UPNP_E_OUTOF_MEMORY
 */
int web_server_init(void);


/*!
 * \brief Release memory allocated for the global web server root
 * directory and the global XML document. Resets the flag bWebServerState
 * to WEB_SERVER_DISABLED.
 */
void web_server_destroy(void);


/*!
 * \brief Replaces current alias with the given alias. To remove the current
 * alias, set alias_name to NULL.
 *
 * \note alias_content is not freed here
 *
 * \return
 * \li \c 0 - OK
 * \li \c UPNP_E_OUTOF_MEMORY
 */
int web_server_set_alias(
	/*! [in] Webserver name of alias; created by caller and freed by caller
	 * (doesn't even have to be malloc()d. */
	const char* alias_name,
	/*! [in] The xml doc; this is allocated by the caller; and freed by
	 * the web server. */
	const char* alias_content,
	/*! [in] Length of alias body in bytes. */
	size_t alias_content_length,
	/*! [in] Time when the contents of alias were last changed (local time). */
	time_t last_modified);


/*!
 * \brief Assign the path specfied by the input const char* root_dir parameter
 * to the global Document root directory. Also check for path names ending
 * in '/'.
 *
 * \return Integer.
 */
int web_server_set_root_dir(
	/*! [in] String having the root directory for the document. */
	const char* root_dir);


/*!
 * \brief Main entry point into web server; Handles HTTP GET and HEAD
 * requests.
 */
void web_server_callback(
	/*! [in] . */
	http_parser_t *parser,
	/*! [in] . */
	http_message_t *req,
	/*! [in,out] . */
	SOCKINFO *info);


#ifdef __cplusplus
} /* extern C */
#endif


#endif /* GENLIB_NET_HTTP_WEBSERVER_H */

