/**************************************************************************
 *
 * Copyright (c) 2018 Jean-Francois Dockes <jfd@recoll.org>
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
 * * Neither name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
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
 *************************************************************************/

#ifdef UPNP_HAVE_DEBUG
	#include "upnpdebug.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#ifdef UPNP_HAVE_DEBUG
	#include <string.h>
#endif

/* regression: issue #272 — UpnpInitLog() must not enable logging
 * when neither UpnpSetLogFileNames nor UpnpSetLogLevel has been
 * called.  Uses only UPNP_EXPORT_SPEC-exported functions so that
 * this block compiles against both the shared and static library. */
#ifndef NDEBUG
	#include "upnpdebug.h"
#endif

int main(void)
{
#ifndef NDEBUG
	UpnpInitLog();
	{
		FILE *fp = UpnpGetDebugFile(UPNP_CRITICAL, API);
		if (fp != NULL) {
			fprintf(stderr,
				"BUG issue #272: UpnpInitLog() enabled "
				"logging without user configuration\n");
			exit(1);
		}
	}
	UpnpCloseLog();
#endif /* !NDEBUG */

#ifdef UPNP_HAVE_DEBUG
	/* UpnpPrintf is an internal (hidden) symbol as of issue #365.
	 * This block uses only the public UpnpGetDebugFile() API so that the
	 * test links correctly whether UpnpPrintf is exported or not. */
	int i;
	FILE *logfp;

	/* Try a few random calls (let's crash it...) */
	UpnpCloseLog();
	UpnpCloseLog();
	/* Before init: any log-level query must return NULL. */
	logfp = UpnpGetDebugFile(UPNP_CRITICAL, API);
	if (logfp) {
		fprintf(stderr, "Log FP not NULL before init was called !\n");
		exit(1);
	}

	/* Let's really init. Request log to stderr */
	UpnpSetLogFileNames(NULL, NULL);
	UpnpSetLogLevel(UPNP_ERROR);
	UpnpInitLog();

	logfp = UpnpGetDebugFile(UPNP_CRITICAL, API);
	if (logfp)
		fprintf(logfp, "Hello LOG !\n");
	/* UPNP_INFO must be filtered when the active level is UPNP_ERROR. */
	if (UpnpGetDebugFile(UPNP_INFO, API)) {
		fprintf(stderr,
			"BUG: UPNP_INFO not filtered at UPNP_ERROR level\n");
		exit(1);
	}

	/* Let's try to a file */
	UpnpSetLogFileNames("libupnp_err.log", NULL);
	UpnpInitLog();
	logfp = UpnpGetDebugFile(UPNP_CRITICAL, API);
	if (logfp)
		fprintf(logfp, "Hello from the log file\n");

	/* Close and retry stuff */
	UpnpCloseLog();
	/* After close, log queries must return NULL again. */
	logfp = UpnpGetDebugFile(UPNP_CRITICAL, API);
	if (logfp) {
		fprintf(stderr, "BUG: log FP not NULL after UpnpCloseLog()\n");
		exit(1);
	}
	UpnpSetLogFileNames(NULL, NULL);
	UpnpInitLog();
	logfp = UpnpGetDebugFile(UPNP_CRITICAL, API);
	if (logfp)
		fprintf(logfp, "I'm back !\n");
	for (i = 0; i < 10000; i++) {
		UpnpInitLog();
		UpnpCloseLog();
	}
	UpnpCloseLog();
#else
	printf("DEBUG is not configured\n");
#endif

	exit(0);
}
