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
 * \brief Regression test for GHSA-9mqx-m85r-46wh (NUL in a GENA CALLBACK).
 *
 * Root cause: create_url_list() kept the CALLBACK header value by length, an
 * embedded NUL included, and parsed the URLs after the NUL. When the
 * subscription was later copied for a notification, copy_URL_list() sized the
 * copy with strlen(), so the parsed URL tokens pointed past the allocation
 * and the NOTIFY sender read out of bounds.
 *
 * Test: a SUBSCRIBE whose CALLBACK is "<\0<http://ip:1/cb>" must be rejected
 * with HTTP 412, while the same request without the NUL is accepted.
 */

#include "posix_overwrites.h" /* IWYU pragma: keep */
#include "upnp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
	#include <arpa/inet.h>
	#include <netinet/in.h>
	#include <sys/socket.h>
	#include <sys/time.h>
	#include <unistd.h>
#endif

#define EVENT_URL_PATH "/event/callbacknul"

static const char DEVICE_DESC[] =
	"<?xml version=\"1.0\"?>"
	"<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
	"<specVersion><major>1</major><minor>0</minor></specVersion>"
	"<device>"
	"<deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType>"
	"<friendlyName>callbacknul</friendlyName>"
	"<manufacturer>Test</manufacturer>"
	"<modelName>Test</modelName>"
	"<UDN>uuid:callbacknul-0000-0000-0000-000000000000</UDN>"
	"<serviceList><service>"
	"<serviceType>urn:schemas-upnp-org:service:Basic:1</serviceType>"
	"<serviceId>urn:upnp-org:serviceId:Basic</serviceId>"
	"<SCPDURL>/scpd.xml</SCPDURL>"
	"<controlURL>/control/callbacknul</controlURL>"
	"<eventSubURL>" EVENT_URL_PATH "</eventSubURL>"
	"</service></serviceList>"
	"</device>"
	"</root>";

static int device_callback(Upnp_EventType t, void *e, void *c)
{
	(void)t;
	(void)e;
	(void)c;
	return 0;
}

#ifndef _WIN32

/*
 * Send a SUBSCRIBE whose Callback value is "<" followed, if with_nul is set,
 * by a NUL byte, and then "<http://server_ip:1/cb>". Returns the HTTP status
 * code, or -1 on error.
 */
static int send_subscribe(
	const char *server_ip, unsigned short server_port, int with_nul)
{
	int fd = -1;
	int status = -1;
	char req[512];
	int head_len;
	int tail_len;
	size_t req_len;
	struct sockaddr_in addr;
	char resp[512];
	struct timeval tv;
	ssize_t n;

	head_len = snprintf(req,
		sizeof(req),
		"SUBSCRIBE " EVENT_URL_PATH " HTTP/1.1\r\n"
		"Host: %s:%u\r\n"
		"NT: upnp:event\r\n"
		"Timeout: Second-1800\r\n"
		"Connection: close\r\n"
		"Callback: <",
		server_ip,
		(unsigned)server_port);
	if (head_len < 0 || (size_t)head_len >= sizeof(req) - 1)
		return -1;
	req_len = (size_t)head_len;
	if (with_nul)
		req[req_len++] = '\0';
	tail_len = snprintf(req + req_len,
		sizeof(req) - req_len,
		"<http://%s:1/cb>\r\n\r\n",
		server_ip);
	if (tail_len < 0 || (size_t)tail_len >= sizeof(req) - req_len)
		return -1;
	req_len += (size_t)tail_len;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		perror("socket");
		goto done;
	}
	tv.tv_sec = 5;
	tv.tv_usec = 0;
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(server_port);
	if (inet_pton(AF_INET, server_ip, &addr.sin_addr) != 1) {
		fprintf(stderr, "inet_pton(%s) failed\n", server_ip);
		goto done;
	}
	if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
		perror("connect");
		goto done;
	}
	if (send(fd, req, req_len, 0) < 0) {
		perror("send");
		goto done;
	}
	n = recv(fd, resp, sizeof(resp) - 1, 0);
	if (n > 0) {
		resp[n] = '\0';
		if (sscanf(resp, "HTTP/1.%*d %d", &status) != 1)
			status = -1;
	}

done:
	if (fd >= 0)
		close(fd);
	return status;
}

#endif /* !_WIN32 */

int main(void)
{
#ifdef _WIN32
	puts("SKIP: raw POSIX socket test not supported on Windows.");
	return EXIT_SUCCESS;
#else
	int rc;
	UpnpDevice_Handle handle = -1;
	const char *server_ip;
	unsigned short server_port;
	int status;
	int result = 0;

	rc = UpnpInit2(NULL, 0);
	if (rc != UPNP_E_SUCCESS) {
		fprintf(stderr,
			"UpnpInit2 failed (%d); skipping (no network?)\n",
			rc);
		return EXIT_SUCCESS;
	}

	server_ip = UpnpGetServerIpAddress();
	server_port = UpnpGetServerPort();
	if (!server_ip || !server_port) {
		fprintf(stderr, "Could not determine server address\n");
		UpnpFinish();
		return EXIT_FAILURE;
	}
	printf("Server: %s:%u\n", server_ip, server_port);

	rc = UpnpRegisterRootDevice2(UPNPREG_BUF_DESC,
		DEVICE_DESC,
		sizeof(DEVICE_DESC) - 1,
		1,
		device_callback,
		NULL,
		&handle);
	if (rc != UPNP_E_SUCCESS) {
		fprintf(stderr, "UpnpRegisterRootDevice2 failed: %d\n", rc);
		UpnpFinish();
		return EXIT_FAILURE;
	}

	status = send_subscribe(server_ip, server_port, 1);
	printf("SUBSCRIBE with a NUL in Callback -> HTTP %d\n", status);
	if (status != 412) {
		fprintf(stderr, "FAIL: expected HTTP 412, got %d\n", status);
		result = 1;
	}

	status = send_subscribe(server_ip, server_port, 0);
	printf("SUBSCRIBE without the NUL -> HTTP %d\n", status);
	if (status != 200) {
		fprintf(stderr, "FAIL: expected HTTP 200, got %d\n", status);
		result = 1;
	}

	UpnpUnRegisterRootDevice(handle);
	UpnpFinish();
	if (result == 0)
		puts("poc_ghsa_9mqx_callback_nul: PASS");
	return result ? EXIT_FAILURE : EXIT_SUCCESS;
#endif /* !_WIN32 */
}
