/*******************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation 
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met: 
 *
 * - Redistributions of source code must retain the above copyright notice, 
 * this list of conditions and the following disclaimer. 
 * - Redistributions in binary form must reproduce the above copyright notice, 
 * this list of conditions and the following disclaimer in the documentation 
 * and/or other materials provided with the distribution. 
 * - Neither name of Intel Corporation nor the names of its contributors 
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
 ******************************************************************************/

/*!
 * \file
 */

#include "config.h"

#include "ithread.h"
#include "ixml.h"
#include "upnp.h"
#include "upnpdebug.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef DEBUG

/*! Mutex to synchronize all the log file opeartions in the debug mode */
static ithread_mutex_t GlobalDebugMutex;

/*! Global log level */
static Upnp_LogLevel g_log_level = UPNP_DEFAULT_LOG_LEVEL;

/*! File handle for the error log file */
static FILE *ErrFileHnd = NULL;

/*! File handle for the information log file */
static FILE *InfoFileHnd = NULL;

/*! Name of the error file */
static const char *errFileName = "libupnp_err.log";

/*! Name of the info file */
static const char *infoFileName = "libupnp_info.log";

int UpnpInitLog(void)
{
	ithread_mutex_init(&GlobalDebugMutex, NULL);
	if (DEBUG_TARGET == 1) {
		if ((ErrFileHnd = fopen(errFileName, "a")) == NULL) {
			fprintf(stderr, "Failed to open errFileName (%s): %s\n", errFileName, strerror(errno));
			ErrFileHnd = stderr;
		}
		if ((InfoFileHnd = fopen(infoFileName, "a")) == NULL) {
			fprintf(stderr, "Failed to open infoFileName (%s): %s\n", infoFileName, strerror(errno));
			InfoFileHnd = stdout;
		}
	}
	return UPNP_E_SUCCESS;
}

void UpnpSetLogLevel(Upnp_LogLevel log_level)
{
	g_log_level = log_level;
}

void UpnpCloseLog(void)
{
	if (DEBUG_TARGET == 1) {
		fflush(ErrFileHnd);
		fflush(InfoFileHnd);
		fclose(ErrFileHnd);
		fclose(InfoFileHnd);
	}
	ithread_mutex_destroy(&GlobalDebugMutex);
}

void UpnpSetLogFileNames(const char *ErrFileName, const char *InfoFileName)
{
	if (ErrFileName) {
		errFileName = ErrFileName;
	}
	if (InfoFileName) {
		infoFileName = InfoFileName;
	}
}

int DebugAtThisLevel(Upnp_LogLevel DLevel, Dbg_Module Module)
{
	int ret = DLevel <= g_log_level;
	ret &=
	    DEBUG_ALL ||
	    (Module == SSDP && DEBUG_SSDP) ||
	    (Module == SOAP && DEBUG_SOAP) ||
	    (Module == GENA && DEBUG_GENA) ||
	    (Module == TPOOL && DEBUG_TPOOL) ||
	    (Module == MSERV && DEBUG_MSERV) ||
	    (Module == DOM && DEBUG_DOM) || (Module == HTTP && DEBUG_HTTP);

	return ret;
	Module = Module; /* VC complains about this being unreferenced */
}

void UpnpPrintf(Upnp_LogLevel DLevel,
		Dbg_Module Module,
		const char *DbgFileName, int DbgLineNo, const char *FmtStr, ...)
{
	va_list ArgList;

	if (!DebugAtThisLevel(DLevel, Module))
		return;
	ithread_mutex_lock(&GlobalDebugMutex);

	va_start(ArgList, FmtStr);
	if (DEBUG_TARGET) {
		if (DbgFileName)
			UpnpDisplayFileAndLine(stdout, DbgFileName, DbgLineNo);
		vfprintf(stdout, FmtStr, ArgList);
		fflush(stdout);
	} else if (DLevel == 0) {
		if (DbgFileName)
			UpnpDisplayFileAndLine(ErrFileHnd, DbgFileName,
					       DbgLineNo);
		vfprintf(ErrFileHnd, FmtStr, ArgList);
		fflush(ErrFileHnd);
	} else {
		if (DbgFileName)
			UpnpDisplayFileAndLine(InfoFileHnd, DbgFileName,
					       DbgLineNo);
		vfprintf(InfoFileHnd, FmtStr, ArgList);
		fflush(InfoFileHnd);
	}
	va_end(ArgList);
	ithread_mutex_unlock(&GlobalDebugMutex);
}

FILE *GetDebugFile(Upnp_LogLevel DLevel, Dbg_Module Module)
{
	FILE *ret;

	if (!DebugAtThisLevel(DLevel, Module))
		ret = NULL;
	if (DEBUG_TARGET)
		ret = stdout;
	else if (DLevel == 0)
		ret = ErrFileHnd;
	else
		ret = InfoFileHnd;

	return ret;
}

void UpnpDisplayFileAndLine(FILE *fd, const char *DbgFileName, int DbgLineNo)
{
	char timebuf[26];
	time_t now = time(NULL);
	struct tm *timeinfo;
	timeinfo = localtime(&now);
	strftime(timebuf, 26, "%Y-%m-%d %H:%M:%S", timeinfo);

	fprintf(fd, "%s UPNP: Thread:0x%lX [%s:%d]: ", timebuf,
#ifdef _WIN32
		(unsigned long int)ithread_self().p
#else
		(unsigned long int)ithread_self()
#endif
	, DbgFileName, DbgLineNo);
	fflush(fd);
}

#endif /* DEBUG */
