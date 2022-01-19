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
#include "autoconfig.h"

#include "UpnpLog.h"
#include "upnp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void test_callback(Upnp_LogLevel level,
	Dbg_Module module,
	const char *sourceFile,
	const int *sourceLine,
	const char *log)
{
	(void)module;
	(void)sourceFile;
	(void)sourceLine;

	fprintf(stderr,
		"Hello, from the test callback. Level is: %s, Message is: "
		"'%s'\n",
		UpnpLogLevelToStr(level),
		log);
}

int main()
{
	int i;
	UpnpLog *l = UpnpLog_new();

	if (!l) {
		UpnpPrintf(0,
			UPNP_CRITICAL,
			API,
			__FILE__,
			__LINE__,
			"Insuficient memory!\n");
		exit(1);
	}
	/* Try a few random calls (let's crash it...) */
	UpnpCloseLog(l);
	UpnpCloseLog(l);
	UpnpPrintf(l,
		UPNP_CRITICAL,
		API,
		__FILE__,
		__LINE__,
		"This should not be printed!\n");

	/* Let's really init. Request log to stderr */
	UpnpSetLogFileName(l, NULL);
	UpnpSetLogLevel(l, UPNP_ERROR);
	UpnpInitLog(l);
	UpnpPrintf(l, UPNP_CRITICAL, API, __FILE__, __LINE__, "Hello LOG!\n");
	UpnpPrintf(l,
		UPNP_INFO,
		API,
		__FILE__,
		__LINE__,
		"This should not be here!\n");

	/* Let's try to a file */
	UpnpSetLogFileName(l, "libupnp_err.log");
	UpnpInitLog(l);
	UpnpPrintf(l,
		UPNP_CRITICAL,
		API,
		__FILE__,
		__LINE__,
		"Hello from the log file\n");

	/* Close and retry stuff */
	UpnpCloseLog(l);
	UpnpPrintf(l, UPNP_INFO, API, __FILE__, __LINE__, "Not here either!\n");
	UpnpSetLogFileName(l, NULL);
	UpnpInitLog(l);
	UpnpPrintf(l, UPNP_CRITICAL, API, __FILE__, __LINE__, "I'm back!\n");
	for (i = 0; i < 10000; i++) {
		UpnpInitLog(l);
		UpnpCloseLog(l);
	}
	UpnpCloseLog(l);

	/* Callback */
	UpnpInitLog(l);
	UpnpSetLogCallback(l, test_callback);
	UpnpPrintf(l,
		UPNP_CRITICAL,
		API,
		__FILE__,
		__LINE__,
		"And we'd better not risk another frontal assault, that "
		"rabbit's dynamite.\n");
	UpnpPrintf(l,
		UPNP_CRITICAL,
		API,
		__FILE__,
		__LINE__,
		"Would it help to confuse it if we run away more?\n");
	UpnpSetLogCallback(l, NULL);
	UpnpPrintf(l,
		UPNP_CRITICAL,
		API,
		__FILE__,
		__LINE__,
		"Oh, shut up and go and change your armor.\n");
	UpnpSetLogCallback(l, test_callback);
	UpnpPrintf(l,
		UPNP_CRITICAL,
		API,
		__FILE__,
		__LINE__,
		"Let us taunt it! It may become so cross that it will make a "
		"mistake.\n");

	exit(0);
}
