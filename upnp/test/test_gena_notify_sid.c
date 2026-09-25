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
 * \brief Regression test for GHSA-ggw2-jjv9-h22c.
 *
 * The control point looked up the subscription for an incoming NOTIFY with
 * memcmp(ActualSID, sid, sid_len), where sid_len is the length of the SID
 * header. Nothing checked that sid_len equals the stored SID length, so:
 *
 *   - a SID longer than the stored one (the stored SID, a NUL byte, then
 *     padding) read past the end of the stored string;
 *   - an empty SID, or any prefix of the stored one, matched the
 *     subscription, so events were accepted without knowing the SID.
 *
 * A fake publisher grants the SID "uuid:ggw2-test". Then NOTIFY requests
 * are sent to the control point: the exact SID must be accepted (200), and
 * the empty, prefix and oversized SIDs must be rejected (412). The
 * over-read itself is only detected under AddressSanitizer.
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
	#include <unistd.h>

	#define TEST_SID "uuid:ggw2-test"

static const char k_response[] = "HTTP/1.1 200 OK\r\n"
				 "SID: " TEST_SID "\r\n"
				 "TIMEOUT: Second-1800\r\n"
				 "Content-Length: 0\r\n"
				 "\r\n";

static const char k_body[] =
	"<?xml version=\"1.0\"?>\r\n"
	"<e:propertyset xmlns:e=\"urn:schemas-upnp-org:event-1-0\">"
	"<e:property><Status>1</Status></e:property>"
	"</e:propertyset>\r\n";

static int g_listen_fd = -1;
static volatile int g_event_count = 0;

/* Answers every request (SUBSCRIBE, UNSUBSCRIBE) with k_response. */
static void *publisher_thread(void *arg)
{
	(void)arg;
	for (;;) {
		char buf[2048];
		size_t len = 0;
		int fd = accept(g_listen_fd, NULL, NULL);

		if (fd < 0)
			return NULL;
		/* Read up to the end of the request headers. */
		while (len < sizeof(buf) - 1) {
			ssize_t n =
				recv(fd, buf + len, sizeof(buf) - 1 - len, 0);

			if (n <= 0)
				break;
			len += (size_t)n;
			buf[len] = '\0';
			if (strstr(buf, "\r\n\r\n"))
				break;
		}
		send(fd, k_response, sizeof(k_response) - 1, 0);
		close(fd);
	}
}

static int client_callback(Upnp_EventType event_type, void *event, void *cookie)
{
	(void)event;
	(void)cookie;
	if (event_type == UPNP_EVENT_RECEIVED)
		__sync_fetch_and_add(&g_event_count, 1);
	return 0;
}

/*!
 * \brief Sends a NOTIFY carrying the given SID bytes (which may contain NUL)
 * to the control point's callback server.
 *
 * \return The HTTP status code of the reply, or -1 on error.
 */
static int send_notify(const char *sid, size_t sid_len)
{
	static char req[8192];
	char reply[256];
	size_t len = 0;
	ssize_t n;
	int status = -1;
	int fd;
	struct sockaddr_in addr;

	len += (size_t)snprintf(req + len,
		sizeof(req) - len,
		"NOTIFY / HTTP/1.1\r\n"
		"HOST: %s:%u\r\n"
		"CONTENT-TYPE: text/xml\r\n"
		"CONTENT-LENGTH: %u\r\n"
		"NT: upnp:event\r\n"
		"NTS: upnp:propchange\r\n"
		"SEQ: 1\r\n"
		"SID: ",
		UpnpGetServerIpAddress(),
		(unsigned)UpnpGetServerPort(),
		(unsigned)(sizeof(k_body) - 1));
	if (len + sid_len + 4 + sizeof(k_body) > sizeof(req))
		return -1;
	memcpy(req + len, sid, sid_len);
	len += sid_len;
	memcpy(req + len, "\r\n\r\n", 4);
	len += 4;
	memcpy(req + len, k_body, sizeof(k_body) - 1);
	len += sizeof(k_body) - 1;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
		return -1;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(UpnpGetServerPort());
	if (inet_pton(AF_INET, UpnpGetServerIpAddress(), &addr.sin_addr) != 1 ||
		connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0 ||
		send(fd, req, len, 0) != (ssize_t)len) {
		close(fd);
		return -1;
	}
	n = recv(fd, reply, sizeof(reply) - 1, 0);
	if (n > 0) {
		reply[n] = '\0';
		if (sscanf(reply, "HTTP/%*d.%*d %d", &status) != 1)
			status = -1;
	}
	close(fd);

	return status;
}

static int check(const char *name, const char *sid, size_t sid_len, int want)
{
	int status = send_notify(sid, sid_len);

	printf("%-10s SID (%4u bytes): HTTP %d, expected %d\n",
		name,
		(unsigned)sid_len,
		status,
		want);

	return status == want;
}
#endif /* !_WIN32 */

int main(void)
{
#ifdef _WIN32
	puts("SKIP: fake publisher uses POSIX threads, not supported on "
	     "Windows.");
	return EXIT_SUCCESS;
#else
	static char oversized[4096];
	int rc;
	int ok = 1;
	int timeout = 1800;
	UpnpClient_Handle handle = -1;
	Upnp_SID sid;
	struct sockaddr_in addr;
	socklen_t addr_len = sizeof(addr);
	pthread_t thread;
	char url[64];

	g_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	if (g_listen_fd < 0 ||
		bind(g_listen_fd, (struct sockaddr *)&addr, sizeof(addr)) !=
			0 ||
		listen(g_listen_fd, 16) != 0 ||
		getsockname(g_listen_fd, (struct sockaddr *)&addr, &addr_len) !=
			0) {
		puts("FAIL: could not set up the fake publisher");
		return EXIT_FAILURE;
	}
	snprintf(url,
		sizeof(url),
		"http://127.0.0.1:%hu/event",
		ntohs(addr.sin_port));
	if (pthread_create(&thread, NULL, publisher_thread, NULL) != 0) {
		puts("FAIL: could not start the fake publisher");
		return EXIT_FAILURE;
	}
	pthread_detach(thread);

	rc = UpnpInit2(NULL, 0);
	if (rc != UPNP_E_SUCCESS) {
		printf("FAIL: UpnpInit2 returned %d\n", rc);
		return EXIT_FAILURE;
	}
	rc = UpnpRegisterClient(client_callback, NULL, &handle);
	if (rc != UPNP_E_SUCCESS) {
		printf("FAIL: UpnpRegisterClient returned %d\n", rc);
		UpnpFinish();
		return EXIT_FAILURE;
	}
	rc = UpnpSubscribe(handle, url, &timeout, sid);
	if (rc != UPNP_E_SUCCESS) {
		printf("FAIL: UpnpSubscribe returned %d\n", rc);
		UpnpFinish();
		return EXIT_FAILURE;
	}

	/* The stored SID, its NUL terminator, then padding. */
	memset(oversized, 'A', sizeof(oversized));
	memcpy(oversized, TEST_SID, sizeof(TEST_SID));

	ok &= check("exact", TEST_SID, strlen(TEST_SID), 200);
	ok &= check("empty", "", 0, 412);
	ok &= check("prefix", "uuid:", 5, 412);
	ok &= check("oversized", oversized, sizeof(oversized), 412);
	printf("Events delivered to the callback: %d\n", g_event_count);
	if (g_event_count != 1) {
		puts("FAIL: only the exact SID may deliver an event");
		ok = 0;
	}

	UpnpUnSubscribe(handle, sid);
	UpnpUnRegisterClient(handle);
	UpnpFinish();
	close(g_listen_fd);

	if (!ok) {
		puts("test_gena_notify_sid: FAIL");
		return EXIT_FAILURE;
	}
	puts("test_gena_notify_sid: PASS");
	return EXIT_SUCCESS;
#endif /* !_WIN32 */
}
