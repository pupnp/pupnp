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
 * \brief Regression test for PR #519 (poll() vs select() socket readiness).
 *
 * On Windows x64 with MinGW, WSAPoll() expects WSAPOLLFD (SOCKET fd, 64-bit)
 * but struct pollfd has an int fd (32-bit), causing UpnpInit2() to hang.
 * This test verifies that:
 *   1. UpnpInit2() / UpnpFinish() complete within the test timeout.
 *   2. UpnpSearchAsync() completes without hanging, exercising the
 *      poll() (non-Windows) / select() (Windows) path in SearchByTarget().
 */

#include "Callback.h"
#include "upnp.h"

#include <stdio.h>
#include <stdlib.h>

#include "ithread.h" /* for imillisleep */

#include "posix_overwrites.h" /* IWYU pragma: keep */

static int discovery_callback(
	Upnp_EventType event_type, void *event, void *cookie)
{
	(void)event_type;
	(void)event;
	(void)cookie;
	return 0;
}

int main(void)
{
	int rc;
	UpnpClient_Handle handle = -1;

	/*
	 * Step 1: UpnpInit2() starts the miniserver thread, which calls
	 * RunMiniServer().  On Windows this used to hang because WSAPoll()
	 * was called with struct pollfd (int fd) instead of WSAPOLLFD
	 * (SOCKET fd).  With the fix, select() is used on Windows.
	 */
	printf("Initializing UPnP stack ... ");
	fflush(stdout);
	rc = UpnpInit2(NULL, 0);
	if (rc != UPNP_E_SUCCESS) {
		printf("FAIL (UpnpInit2 returned %d)\n", rc);
		exit(EXIT_FAILURE);
	}
	printf("OK\n");

	/*
	 * Step 2: Register as a control point client so we can issue
	 * a discovery search.
	 */
	printf("Registering control point ... ");
	fflush(stdout);
	rc = UpnpRegisterClient(discovery_callback, NULL, &handle);
	if (rc != UPNP_E_SUCCESS) {
		printf("FAIL (UpnpRegisterClient returned %d)\n", rc);
		UpnpFinish();
		exit(EXIT_FAILURE);
	}
	printf("OK\n");

	/*
	 * Step 3: Issue an SSDP M-SEARCH.  SearchByTarget() uses
	 * poll() (non-Windows) or select() (Windows) to wait until the
	 * multicast socket is writable before sending.  With the old broken
	 * WSAPoll() path this call could hang or return an error.
	 * Mx=1 means the search expires after 1 second.
	 */
	printf("Sending SSDP search (Mx=1s) ... ");
	fflush(stdout);
	rc = UpnpSearchAsync(handle, 1, "ssdp:all", NULL);
	if (rc != UPNP_E_SUCCESS) {
		printf("FAIL (UpnpSearchAsync returned %d)\n", rc);
		UpnpUnRegisterClient(handle);
		UpnpFinish();
		exit(EXIT_FAILURE);
	}
	printf("OK\n");

	/* Wait for the search Mx timer to expire. */
	imillisleep(1500);

	UpnpUnRegisterClient(handle);
	UpnpFinish();

	printf("test_sock_io: PASS\n");
	exit(EXIT_SUCCESS);
}
