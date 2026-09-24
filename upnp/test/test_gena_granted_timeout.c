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
 * \brief Regression test for GHSA-gvhr-pxx7-ch7x.
 *
 * The control point clamped the subscription timeout it requested to
 * CP_MINIMUM_SUBSCRIPTION_TIME, but accepted any timeout the publisher
 * granted. A granted TIMEOUT: Second-0 scheduled the auto-renewal
 * AUTO_RENEW_TIME seconds in the past, so it fired at once, got Second-0
 * again, and looped, sending SUBSCRIBE requests as fast as the network
 * allowed.
 *
 * A fake publisher answers every SUBSCRIBE with Second-0 and counts them.
 * After one UpnpSubscribe(), the granted timeout must be raised to
 * CP_MINIMUM_SUBSCRIPTION_TIME, and no renewal may arrive before
 * CP_MINIMUM_SUBSCRIPTION_TIME - AUTO_RENEW_TIME seconds.
 */

#include "Callback.h"
#include "config.h"
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

static const char k_response[] = "HTTP/1.1 200 OK\r\n"
				 "SID: uuid:gvhr-test\r\n"
				 "TIMEOUT: Second-0\r\n"
				 "Content-Length: 0\r\n"
				 "\r\n";

static int g_listen_fd = -1;
static volatile int g_subscribe_count = 0;

/* Answers every request with k_response, counting SUBSCRIBEs. */
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
		buf[len] = '\0';
		if (strncmp(buf, "SUBSCRIBE ", 10) == 0)
			__sync_fetch_and_add(&g_subscribe_count, 1);
		send(fd, k_response, sizeof(k_response) - 1, 0);
		close(fd);
	}
}

static int client_callback(Upnp_EventType event_type, void *event, void *cookie)
{
	(void)event_type;
	(void)event;
	(void)cookie;
	return 0;
}
#endif /* !_WIN32 */

int main(void)
{
#ifdef _WIN32
	puts("SKIP: fake publisher uses POSIX threads, not supported on "
	     "Windows.");
	return EXIT_SUCCESS;
#else
	int rc;
	int timeout = 1800;
	int count;
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
	/* The first renewal is due CP_MINIMUM_SUBSCRIPTION_TIME -
	 * AUTO_RENEW_TIME seconds after subscribing; wait less than that. */
	sleep(CP_MINIMUM_SUBSCRIPTION_TIME - AUTO_RENEW_TIME - 2);
	count = g_subscribe_count;
	printf("Granted timeout reported as %d\n", timeout);
	printf("SUBSCRIBE requests received: %d\n", count);
	/* On failure, skip UpnpFinish(): a renewal storm may be running. */
	if (timeout != CP_MINIMUM_SUBSCRIPTION_TIME) {
		printf("FAIL: expected the granted Second-0 to be raised to "
		       "%d\n",
			CP_MINIMUM_SUBSCRIPTION_TIME);
		return EXIT_FAILURE;
	}
	if (count != 1) {
		puts("FAIL: renewal fired before it was due");
		return EXIT_FAILURE;
	}

	UpnpUnRegisterClient(handle);
	UpnpFinish();
	close(g_listen_fd);

	puts("test_gena_granted_timeout: PASS");
	return EXIT_SUCCESS;
#endif /* !_WIN32 */
}
