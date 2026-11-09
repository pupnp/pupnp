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
 * \brief Regression test for GHSA-wcf8-pjfj-w9wv (alias replaced while a
 * response is in flight).
 *
 * Root cause: web_server_set_alias() always released gAliasDoc first.  After
 * web_server_set_alias(NULL) had already dropped the global reference while
 * an HTTP handler still held another one, gAliasDoc kept pointing at the old
 * alias, so installing a new alias released the old one a second time.  The
 * old XML was freed while the handler was still sending it, and the
 * handler's own release then read the freed refcount and could free the
 * buffers again.
 *
 * Sequence (web_server_ut_* hooks stand in for the HTTP handler):
 *   1. install alias A                    (A refcount 1)
 *   2. handler grabs A                    (A refcount 2)
 *   3. unregister: set alias NULL         (A refcount 1)
 *   4. register again: install alias B    (bug: A refcount 0, A freed)
 *   5. handler releases its copy of A     (bug: use after free of A)
 *   6. UpnpFinish()
 * Detected by ASan (the CI sanitizer jobs).
 */

#include "upnp.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

extern void web_server_ut_grab_alias(void);
extern void web_server_ut_release_alias(void);
extern int web_server_ut_set_alias(
	const char *name, const char *content, size_t len);

int main(void)
{
	int rc;

	rc = UpnpInit2(NULL, 0);
	if (rc != UPNP_E_SUCCESS) {
		fprintf(stderr,
			"UpnpInit2 failed (%d); skipping (no network?)\n",
			rc);
		return EXIT_SUCCESS;
	}

	rc = web_server_ut_set_alias("/first.xml", "<first/>", 8);
	if (rc != UPNP_E_SUCCESS) {
		fprintf(stderr, "install first alias failed (%d)\n", rc);
		UpnpFinish();
		return EXIT_FAILURE;
	}
	web_server_ut_grab_alias();
	web_server_ut_set_alias(NULL, NULL, 0);
	rc = web_server_ut_set_alias("/second.xml", "<second/>", 9);
	if (rc != UPNP_E_SUCCESS) {
		fprintf(stderr, "install second alias failed (%d)\n", rc);
		web_server_ut_release_alias();
		UpnpFinish();
		return EXIT_FAILURE;
	}
	web_server_ut_release_alias();
	UpnpFinish();

	printf("poc_ghsa_wcf8_alias_replace: PASS\n");
	return EXIT_SUCCESS;
}
