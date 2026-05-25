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
 * \brief Regression test for issue #227: SSDP server must bind to the
 * selected interface address, not to INADDR_ANY (0.0.0.0).
 *
 * Strategy (Linux only):
 *   1. Call UpnpInit2() and note the assigned IPv4 server address via
 *      UpnpGetServerIpAddress().
 *   2. Enumerate this process's open socket file-descriptors via
 *      /proc/self/fd to collect their inodes.
 *   3. Scan /proc/net/udp for any entry whose inode belongs to this
 *      process and whose port is 1900 (SSDP_PORT).
 *   4. FAIL if any such entry shows a local address of 00000000
 *      (INADDR_ANY); PASS otherwise.
 *
 * Skipped on non-Linux platforms where /proc/net/udp is absent.
 */

#include "upnp.h"

#include <stdio.h>
#include <stdlib.h>

#ifndef _WIN32
	#include <arpa/inet.h>
	#include <dirent.h>
	#include <string.h>
	#include <sys/stat.h>
	#include <unistd.h>
#endif

#ifndef _WIN32

	#define MAX_SOCKETS 256

/* regression: issue #227 */

static int collect_own_socket_inodes(unsigned long *inodes, int max)
{
	DIR *dir = opendir("/proc/self/fd");
	if (!dir)
		return 0;

	int n = 0;
	struct dirent *ent;
	while ((ent = readdir(dir)) != NULL && n < max) {
		char path[64];
		snprintf(path, sizeof(path), "/proc/self/fd/%s", ent->d_name);
		struct stat st;
		if (stat(path, &st) == 0 && S_ISSOCK(st.st_mode))
			inodes[n++] = (unsigned long)st.st_ino;
	}
	closedir(dir);
	return n;
}

static int inode_in_list(
	unsigned long inode, const unsigned long *list, int list_len)
{
	for (int i = 0; i < list_len; i++)
		if (list[i] == inode)
			return 1;
	return 0;
}

/*
 * Returns  0 : checked OK (no INADDR_ANY SSDP socket belonging to us)
 * Returns -1 : FAIL (found at least one INADDR_ANY SSDP socket of ours)
 * Returns  1 : SKIP (no matching socket found or /proc not available)
 */
static int check_ssdp_v4_bound_to_specific(void)
{
	unsigned long our_inodes[MAX_SOCKETS];
	int n_inodes = collect_own_socket_inodes(our_inodes, MAX_SOCKETS);
	if (n_inodes == 0) {
		printf("SKIP (/proc/self/fd not available or no sockets "
		       "open)\n");
		return 1;
	}

	FILE *f = fopen("/proc/net/udp", "r");
	if (!f) {
		printf("SKIP (/proc/net/udp not available on this platform)\n");
		return 1;
	}

	char line[256];
	fgets(line, sizeof(line), f); /* skip header */

	int found = 0;
	int result = 1; /* default: skip (no matching socket seen) */
	while (fgets(line, sizeof(line), f)) {
		char local_addr[9] = {0};
		unsigned local_port = 0;
		unsigned long inode = 0;
		char sl[8];
		/*
		 * /proc/net/udp columns:
		 * sl  local_addr:port  rem_addr:port  st  tx:rx  tr:tm
		 *   retrnsmt  uid  timeout  inode  ...
		 */
		char rem[20], st_s[4], txrx[20], trtm[20];
		unsigned retrnsmt, uid, timeout;
		if (sscanf(line,
			    " %7s %8[^:]:%4X %19s %3s %19s %19s %u %u %u %lu",
			    sl,
			    local_addr,
			    &local_port,
			    rem,
			    st_s,
			    txrx,
			    trtm,
			    &retrnsmt,
			    &uid,
			    &timeout,
			    &inode) < 11)
			continue;

		if (local_port != 0x076C) /* 1900 decimal */
			continue;

		if (!inode_in_list(inode, our_inodes, n_inodes))
			continue;

		found = 1;
		if (strcmp(local_addr, "00000000") == 0) {
			printf("FAIL: our SSDP IPv4 socket (inode %lu) is "
			       "bound "
			       "to INADDR_ANY; should be bound to %s\n",
				inode,
				UpnpGetServerIpAddress());
			fclose(f);
			return -1;
		}
		printf("OK: our SSDP IPv4 socket (inode %lu) is bound to "
		       "%s (not INADDR_ANY)\n",
			inode,
			local_addr);
		result = 0;
	}
	fclose(f);

	if (!found)
		printf("SKIP (no SSDP socket belonging to this process found "
		       "in /proc/net/udp)\n");

	return result;
}

int main(void)
{
	int rc = UpnpInit2(NULL, 0);
	if (rc != UPNP_E_SUCCESS) {
		printf("SKIP (UpnpInit2 returned %d — no suitable interface)\n",
			rc);
		exit(77); /* automake SKIP exit code */
	}

	printf("Server IPv4 address: %s\n", UpnpGetServerIpAddress());

	int result = check_ssdp_v4_bound_to_specific();

	UpnpFinish();

	if (result < 0)
		exit(EXIT_FAILURE);

	printf("test_ssdp_bind_specific: PASS\n");
	exit(EXIT_SUCCESS);
}

#else /* _WIN32 */

int main(void)
{
	printf("test_ssdp_bind_specific: SKIP (Linux /proc interface not "
	       "available on Windows)\n");
	exit(77);
}

#endif /* _WIN32 */
