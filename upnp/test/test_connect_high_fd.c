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
 * \brief Regression test for issue #423 (crash when socket fd >= FD_SETSIZE).
 *
 * Root cause: Check_Connect_And_Wait_Connection() in httpreadwrite.c called
 * FD_SET(sock, &fdSet) unconditionally.  When sock >= FD_SETSIZE (1024),
 * glibc's _FORTIFY_SOURCE turns FD_SET into __fdelt_chk() which abort()s.
 * Without _FORTIFY_SOURCE the fd_set bitmap is silently overflowed,
 * corrupting adjacent stack memory.
 *
 * Fix: replace select()/FD_SET() with poll() on non-Windows.  poll() uses
 * an array of struct pollfd and has no fd-number limit.
 *
 * Test:
 *   1. Raise RLIMIT_NOFILE above FD_SETSIZE and open dummy fds so the next
 *      socket() call returns fd >= FD_SETSIZE.
 *   2. Spin up a minimal HTTP/1.1 server on 127.0.0.1 in a background thread.
 *   3. Call UpnpDownloadXmlDoc() which calls http_RequestAndResponse() ->
 *      private_connect() -> Check_Connect_And_Wait_Connection().
 *      Old code: FD_SET(sock >= FD_SETSIZE) -> abort().
 *      Fixed code: poll() -> no limit, completes normally.
 */

#include "ixml.h"
#include "posix_overwrites.h" /* IWYU pragma: keep */
#include "upnp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32

	#include <arpa/inet.h>
	#include <fcntl.h>
	#include <netinet/in.h>
	#include <pthread.h>
	#include <sys/resource.h>
	#include <sys/select.h>
	#include <sys/socket.h>
	#include <sys/time.h>
	#include <unistd.h>

/* Minimal valid UPnP device description. */
static const char XML_BODY[] =
	"<?xml version=\"1.0\"?>"
	"<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
	"<specVersion><major>1</major><minor>0</minor></specVersion>"
	"<device>"
	"<deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType>"
	"<friendlyName>test423</friendlyName>"
	"<manufacturer>Test</manufacturer>"
	"<modelName>Test</modelName>"
	"<UDN>uuid:test423-0000-0000-0000-000000000000</UDN>"
	"</device>"
	"</root>";

struct server_state
{
	int listenfd;
	unsigned short port;
};

static void *http_server_thread(void *arg)
{
	struct server_state *srv = (struct server_state *)arg;
	char req_buf[2048];
	char resp_buf[1024];
	int resp_len;
	int cfd;
	struct timeval tv;

	tv.tv_sec = 10;
	tv.tv_usec = 0;

	cfd = accept(srv->listenfd, NULL, NULL);
	if (cfd < 0)
		return NULL;

	setsockopt(cfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	setsockopt(cfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

	recv(cfd, req_buf, sizeof(req_buf) - 1, 0);

	resp_len = snprintf(resp_buf,
		sizeof(resp_buf),
		"HTTP/1.1 200 OK\r\n"
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

static int setup_server(struct server_state *srv)
{
	struct sockaddr_in addr;
	socklen_t alen = sizeof(addr);
	struct timeval tv;
	int one = 1;

	tv.tv_sec = 10;
	tv.tv_usec = 0;

	srv->listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (srv->listenfd < 0) {
		perror("socket");
		return -1;
	}
	setsockopt(srv->listenfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	setsockopt(srv->listenfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = 0;

	if (bind(srv->listenfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		close(srv->listenfd);
		return -1;
	}
	if (listen(srv->listenfd, 4) < 0) {
		perror("listen");
		close(srv->listenfd);
		return -1;
	}
	if (getsockname(srv->listenfd, (struct sockaddr *)&addr, &alen) < 0) {
		perror("getsockname");
		close(srv->listenfd);
		return -1;
	}
	srv->port = ntohs(addr.sin_port);
	return 0;
}

int main(void)
{
	struct server_state srv;
	pthread_t srv_tid;
	struct rlimit rl;
	int *dummy_fds;
	int n_dummy = 0;
	char url[80];
	IXML_Document *doc = NULL;
	int rc;

	/* Step 1: set up listening server before opening dummy fds so it gets
	 * a small fd number. */
	if (setup_server(&srv) < 0)
		return EXIT_FAILURE;
	printf("HTTP server on 127.0.0.1:%u (listenfd=%d)\n",
		(unsigned)srv.port,
		srv.listenfd);

	/* Step 2: raise RLIMIT_NOFILE so we can open fds above FD_SETSIZE. */
	if (getrlimit(RLIMIT_NOFILE, &rl) < 0) {
		perror("getrlimit");
		close(srv.listenfd);
		return EXIT_FAILURE;
	}
	if (rl.rlim_cur < (rlim_t)(FD_SETSIZE + 64)) {
		rl.rlim_cur = (rlim_t)(FD_SETSIZE + 64);
		if (rl.rlim_cur > rl.rlim_max)
			rl.rlim_cur = rl.rlim_max;
		setrlimit(RLIMIT_NOFILE, &rl); /* best-effort */
	}

	/* Step 3: open dummy fds until the highest open fd >= FD_SETSIZE - 1,
	 * which ensures the next socket() call returns fd >= FD_SETSIZE. */
	dummy_fds = malloc((size_t)(FD_SETSIZE + 64) * sizeof(int));
	if (!dummy_fds) {
		perror("malloc");
		close(srv.listenfd);
		return EXIT_FAILURE;
	}

	while (n_dummy < FD_SETSIZE + 64) {
		int fd = open("/dev/null", O_RDONLY | O_CLOEXEC);
		if (fd < 0)
			break;
		dummy_fds[n_dummy++] = fd;
		if (fd >= FD_SETSIZE - 1)
			break;
	}

	if (n_dummy == 0 || dummy_fds[n_dummy - 1] < FD_SETSIZE - 1) {
		fprintf(stderr,
			"SKIP: could not push fd to FD_SETSIZE boundary "
			"(FD_SETSIZE=%d, highest dummy fd=%d). "
			"Try raising RLIMIT_NOFILE.\n",
			FD_SETSIZE,
			n_dummy > 0 ? dummy_fds[n_dummy - 1] : -1);
		for (int i = 0; i < n_dummy; i++)
			close(dummy_fds[i]);
		free(dummy_fds);
		close(srv.listenfd);
		return EXIT_SUCCESS;
	}

	printf("Opened %d dummy fds; highest=%d. "
	       "Next socket will be fd >= %d (FD_SETSIZE=%d).\n",
		n_dummy,
		dummy_fds[n_dummy - 1],
		FD_SETSIZE,
		FD_SETSIZE);

	/* Step 4: start the server thread. */
	pthread_create(&srv_tid, NULL, http_server_thread, &srv);

	/* Step 5: call UpnpDownloadXmlDoc().  Internally this calls
	 * private_connect() -> Check_Connect_And_Wait_Connection() with the
	 * newly created socket whose fd >= FD_SETSIZE.
	 *
	 * OLD code: FD_SET(sock, &fdSet) with sock >= FD_SETSIZE overflows the
	 *           fd_set bitmap; _FORTIFY_SOURCE calls abort() here.
	 * FIXED code: poll() is used instead, no fd-number limit. */
	snprintf(url,
		sizeof(url),
		"http://127.0.0.1:%u/test.xml",
		(unsigned)srv.port);
	printf("Calling UpnpDownloadXmlDoc(%s) ...\n", url);
	rc = UpnpDownloadXmlDoc(url, &doc);

	pthread_join(srv_tid, NULL);
	close(srv.listenfd);

	if (doc)
		ixmlDocument_free(doc);

	for (int i = 0; i < n_dummy; i++)
		close(dummy_fds[i]);
	free(dummy_fds);

	/* Any return code other than a crash is acceptable: the important
	 * thing is that Check_Connect_And_Wait_Connection did not abort(). */
	if (rc == UPNP_E_SUCCESS) {
		printf("UpnpDownloadXmlDoc: OK (XML document received)\n");
	} else {
		printf("UpnpDownloadXmlDoc returned %d "
		       "(non-fatal: connection path ran without FD_SET "
		       "crash)\n",
			rc);
	}

	printf("test_connect_high_fd: PASS\n");
	return EXIT_SUCCESS;
}

#else /* _WIN32 */

int main(void)
{
	/* Windows fd_set uses an array of SOCKET handles, not a bitmap,
	 * so FD_SETSIZE overflow does not apply here. */
	printf("test_connect_high_fd: SKIP (not applicable on Windows)\n");
	return EXIT_SUCCESS;
}

#endif /* _WIN32 */
