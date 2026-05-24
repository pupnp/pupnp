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
 * \brief Regression test for issue #410: UpnpGetSsdpReqPort4/6().
 *
 * Verifies that after UpnpSearchAsync() the new accessors return a
 * non-zero ephemeral port for IPv4 (and, when IPv6 is available, for IPv6).
 */

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
	unsigned short port4;
	unsigned short port6;

	printf("Initializing UPnP stack ... ");
	fflush(stdout);
	rc = UpnpInit2(NULL, 0);
	if (rc != UPNP_E_SUCCESS) {
		printf("FAIL (UpnpInit2 returned %d)\n", rc);
		exit(EXIT_FAILURE);
	}
	printf("OK\n");

	printf("Registering control point ... ");
	fflush(stdout);
	rc = UpnpRegisterClient(discovery_callback, NULL, &handle);
	if (rc != UPNP_E_SUCCESS) {
		printf("FAIL (UpnpRegisterClient returned %d)\n", rc);
		UpnpFinish();
		exit(EXIT_FAILURE);
	}
	printf("OK\n");

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

	/* Wait for the OS to assign the ephemeral port on first sendto(). */
	imillisleep(500);

	port4 = UpnpGetSsdpReqPort4();
	printf("UpnpGetSsdpReqPort4() = %hu\n", port4);
	if (port4 == 0) {
		printf("FAIL (expected a non-zero ephemeral port for IPv4)\n");
		UpnpUnRegisterClient(handle);
		UpnpFinish();
		exit(EXIT_FAILURE);
	}

	port6 = UpnpGetSsdpReqPort6();
	printf("UpnpGetSsdpReqPort6() = %hu (0 if IPv6 not available)\n",
		port6);

	/* Wait for the search Mx timer to expire. */
	imillisleep(1000);

	UpnpUnRegisterClient(handle);
	UpnpFinish();

	printf("test_ssdp_req_port: PASS\n");
	exit(EXIT_SUCCESS);
}
