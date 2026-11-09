/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**************************************************************************
 *
 * Copyright (c) 2026 The pupnp contributors
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
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
 * \brief Regression test for GHSA-7vj7-3hjc-p8q9 (chunked body without a
 * terminating NUL).
 *
 * Root cause: http_Download() copied the entity plus the byte after it,
 * assuming that byte was a NUL, and shrank the buffer to entity length + 1.
 * After a chunked body followed by an extra CRLF the byte after the entity
 * is '\r', so the document handed to the caller was not NUL terminated and
 * UpnpDownloadXmlDoc() read past the allocation in strdup().
 *
 * Test: a local HTTP server sends a chunked XML body followed by an extra
 * CRLF.  UpnpDownloadUrlItem() must return the body followed by a NUL.  The
 * byte checked is the last one of the allocation, so the check is exact
 * without a sanitizer.
 */

#include "posix_overwrites.h" /* IWYU pragma: keep */
#include "upnp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32

	#include <arpa/inet.h>
	#include <netinet/in.h>
	#include <pthread.h>
	#include <sys/socket.h>
	#include <sys/time.h>
	#include <unistd.h>

static const char XML_BODY[] = "<root/>";

static const char RESPONSE[] = "HTTP/1.1 200 OK\r\n"
			       "Content-Type: text/xml\r\n"
			       "Transfer-Encoding: chunked\r\n"
			       "Connection: close\r\n"
			       "\r\n"
			       "7\r\n"
			       "<root/>\r\n"
			       "0\r\n"
			       "\r\n"
			       "\r\n";

static int listenfd = -1;

static void *http_server_thread(void *arg)
{
	char req_buf[2048];
	int cfd;
	struct timeval tv;

	(void)arg;
	tv.tv_sec = 10;
	tv.tv_usec = 0;

	cfd = accept(listenfd, NULL, NULL);
	if (cfd < 0)
		return NULL;

	setsockopt(cfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	setsockopt(cfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

	recv(cfd, req_buf, sizeof(req_buf) - 1, 0);
	send(cfd, RESPONSE, sizeof(RESPONSE) - 1, 0);
	close(cfd);
	return NULL;
}

static int setup_server(unsigned short *port)
{
	struct sockaddr_in addr;
	socklen_t alen = sizeof(addr);
	struct timeval tv;
	int one = 1;

	tv.tv_sec = 10;
	tv.tv_usec = 0;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd < 0) {
		perror("socket");
		return -1;
	}
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	setsockopt(listenfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = 0;

	if (bind(listenfd, (struct sockaddr *)&addr, sizeof(addr)) < 0 ||
		listen(listenfd, 4) < 0 ||
		getsockname(listenfd, (struct sockaddr *)&addr, &alen) < 0) {
		perror("bind/listen/getsockname");
		close(listenfd);
		return -1;
	}
	*port = ntohs(addr.sin_port);
	return 0;
}

int main(void)
{
	const size_t len = sizeof(XML_BODY) - 1;
	pthread_t srv_tid;
	unsigned short port;
	char url[80];
	char content_type[LINE_SIZE];
	char *buf = NULL;
	int rc;
	int ok;

	if (setup_server(&port) < 0)
		return EXIT_FAILURE;
	pthread_create(&srv_tid, NULL, http_server_thread, NULL);

	snprintf(url,
		sizeof(url),
		"http://127.0.0.1:%u/desc.xml",
		(unsigned)port);
	rc = UpnpDownloadUrlItem(url, &buf, content_type);

	pthread_join(srv_tid, NULL);
	close(listenfd);

	if (rc != UPNP_E_SUCCESS || buf == NULL) {
		fprintf(stderr,
			"FAIL: UpnpDownloadUrlItem returned %d, buf %p\n",
			rc,
			(void *)buf);
		free(buf);
		return EXIT_FAILURE;
	}
	/* buf holds len + 1 bytes: the body and its terminator. */
	ok = memcmp(buf, XML_BODY, len) == 0 && buf[len] == '\0';
	if (!ok)
		fprintf(stderr,
			"FAIL: document not NUL terminated "
			"(byte after the body is 0x%02x)\n",
			(unsigned char)buf[len]);
	free(buf);
	if (!ok)
		return EXIT_FAILURE;

	printf("poc_ghsa_7vj7_chunked_nul: PASS\n");
	return EXIT_SUCCESS;
}

#else /* _WIN32 */

int main(void)
{
	/* The defect is platform independent; the test server here uses POSIX
	 * sockets and threads. */
	printf("poc_ghsa_7vj7_chunked_nul: SKIP (not built for Windows)\n");
	return EXIT_SUCCESS;
}

#endif /* _WIN32 */
