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
 * \brief Regression test for GHSA-4pw9-q67x-hfc3 (HTTP status 0 response).
 *
 * Root cause: parse_status_line() rejected only negative status codes, so
 * "HTTP/1.1 0 OK" was accepted.  http_Download() writes *document only for
 * status 200 and otherwise returns the peer's status code, which for 0 is
 * UPNP_E_SUCCESS.  UpnpDownloadUrlItem() then reported success with *outBuf
 * unwritten, and UpnpDownloadXmlDoc() parsed and freed its uninitialised
 * local buffer pointer.
 *
 * Test: a local HTTP server answers with "HTTP/1.1 0 OK" and an XML body.
 * UpnpDownloadUrlItem() must not return UPNP_E_SUCCESS.  *outBuf starts as
 * a sentinel, so the old behaviour is detected without depending on stack
 * contents.
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

static const char XML_BODY[] =
	"<?xml version=\"1.0\"?>"
	"<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
	"<specVersion><major>1</major><minor>0</minor></specVersion>"
	"</root>";

static int listenfd = -1;

static void *http_server_thread(void *arg)
{
	char req_buf[2048];
	char resp_buf[1024];
	int resp_len;
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

	resp_len = snprintf(resp_buf,
		sizeof(resp_buf),
		"HTTP/1.1 0 OK\r\n"
		"Content-Type: text/xml\r\n"
		"Content-Length: %zu\r\n"
		"Connection: close\r\n"
		"\r\n"
		"%s",
		strlen(XML_BODY),
		XML_BODY);

	send(cfd, resp_buf, (size_t)resp_len, 0);
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
	static char sentinel[] = "sentinel";
	pthread_t srv_tid;
	unsigned short port;
	char url[80];
	char content_type[LINE_SIZE];
	char *buf = sentinel;
	int rc;

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

	if (buf != sentinel)
		free(buf);

	if (rc == UPNP_E_SUCCESS) {
		fprintf(stderr,
			"FAIL: \"HTTP/1.1 0 OK\" reported as success, "
			"outBuf %s\n",
			buf == sentinel ? "left unwritten" : "written");
		return EXIT_FAILURE;
	}

	printf("UpnpDownloadUrlItem returned %d\n", rc);
	printf("poc_ghsa_4pw9_status_zero: PASS\n");
	return EXIT_SUCCESS;
}

#else /* _WIN32 */

int main(void)
{
	/* The defect is platform independent; the test server here uses POSIX
	 * sockets and threads. */
	printf("poc_ghsa_4pw9_status_zero: SKIP (not built for Windows)\n");
	return EXIT_SUCCESS;
}

#endif /* _WIN32 */
