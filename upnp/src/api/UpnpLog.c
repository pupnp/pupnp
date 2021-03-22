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
#define UPNP_DEBUG_C
#include "config.h"

#include "UpnpLog.h"

#include "UpnpLib.h"
#include "ithread.h"
#include "ixml.h"
#include "upnp.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* This is called from UpnpInit2(). So the user must call setLoggLogFileName()
 * before. This can be called again, for example to rotate the log
 * file, and we try to avoid multiple calls to the mutex init, with a
 * risk of race, probably not a problem, and not worth fixing. */
int UpnpInitLog(UpnpLib *p)
{
        const char *fname;
        FILE *fp = 0;

        if (!UpnpLib_get_gLogInitWasCalled(p)) {
                ithread_mutex_init(UpnpLib_getnc_gLogMutex(p), NULL);
                UpnpLib_set_gLogInitWasCalled(p, 1);
        }
        /* If the user did not ask for logging do nothing */
        if (!UpnpLib_get_gSetLogWasCalled(p)) {
                return UPNP_E_SUCCESS;
        }
        if (UpnpLib_get_gLogFp(p)) {
                if (!UpnpLib_get_gLogIsStderr(p)) {
                        fclose(UpnpLib_get_gLogFp(p));
                        UpnpLib_set_gLogFp(p, 0);
                }
        }
        UpnpLib_set_gLogIsStderr(p, 0);
        fname = UpnpLib_get_gLogFileName(p);
        if (fname) {
                fp = fopen(fname, "a");
                UpnpLib_set_gLogFp(p, fp);
                if (!fp) {
                        fprintf(stderr,
                                "Failed to open gLogFileName (%s): %s\n",
                                UpnpLib_get_gLogFileName(p),
                                strerror(errno));
                }
        }
        if (!fp) {
                UpnpLib_set_gLogFp(p, stderr);
                UpnpLib_set_gLogIsStderr(p, 1);
        }
        return UPNP_E_SUCCESS;
}

void UpnpSetLogLevel(UpnpLib *p, Upnp_LogLevel log_level)
{
        UpnpLib_set_gLogLevel(p, log_level);
        UpnpLib_set_gSetLogWasCalled(p, 1);
}

void UpnpCloseLog(UpnpLib *p)
{
        FILE *fp;
        if (!UpnpLib_get_gLogInitWasCalled(p)) {
                return;
        }

        /* Calling lock() assumes that someone called UpnpInitLog(), but
         * this is reasonable as it is called from UpnpInit2(). We risk a
         * crash if we do this without a lock.*/
        ithread_mutex_lock(UpnpLib_getnc_gLogMutex(p));
        fp = UpnpLib_get_gLogFp(p);
        if (fp && !UpnpLib_get_gLogIsStderr(p)) {
                fclose(fp);
        }
        UpnpLib_set_gLogFp(p, 0);
        UpnpLib_set_gLogIsStderr(p, 0);
        UpnpLib_set_gLogInitWasCalled(p, 0);
        ithread_mutex_unlock(UpnpLib_getnc_gLogMutex(p));
        ithread_mutex_destroy(UpnpLib_getnc_gLogMutex(p));
}

void UpnpSetLogFileNames(
        UpnpLib *p, const char *newgLogFileName, const char *ignored)
{
        (void)ignored;

        if (UpnpLib_get_gLogFileName(p)) {
                free(UpnpLib_get_gLogFileName(p));
                UpnpLib_set_gLogFileName(p, 0);
        }
        if (newgLogFileName && *newgLogFileName) {
                UpnpLib_set_gLogFileName(p, strdup(newgLogFileName));
        }
        UpnpLib_set_gSetLogWasCalled(p, 1);

        return;
}

static int DebugAtThisLevel(UpnpLib *p, Upnp_LogLevel DLevel, Dbg_Module Module)
{
        (void)Module;

        return (DLevel <= UpnpLib_get_gLogLevel(p)) &&
                (DEBUG_ALL || (Module == SSDP && DEBUG_SSDP) ||
                        (Module == SOAP && DEBUG_SOAP) ||
                        (Module == GENA && DEBUG_GENA) ||
                        (Module == TPOOL && DEBUG_TPOOL) ||
                        (Module == MSERV && DEBUG_MSERV) ||
                        (Module == DOM && DEBUG_DOM) ||
                        (Module == HTTP && DEBUG_HTTP));
}

static void UpnpDisplayFileAndLine(FILE *fp,
        const char *DbggLogFileName,
        int DbgLineNo,
        Upnp_LogLevel DLevel,
        Dbg_Module Module)
{
        char timebuf[26];
        time_t now = time(NULL);
        struct tm *timeinfo;
        const char *smod;
#if 0
	char *slev;
	/* Code kept around in case, but I think it's actually more convenient
	   to display a numeric level */
	switch (DLevel) {
	case UPNP_CRITICAL: slev="CRI";break;
	case UPNP_ERROR: slev="ERR";break;
	case UPNP_INFO: slev="INF";break;
	case UPNP_ALL: slev="ALL";break;
	default: slev="UNK";break;
	}
#else
        char slev[25];
        snprintf(slev, 25, "%d", DLevel);
#endif

        switch (Module) {
        case SSDP:
                smod = "SSDP";
                break;
        case SOAP:
                smod = "SOAP";
                break;
        case GENA:
                smod = "GENA";
                break;
        case TPOOL:
                smod = "TPOL";
                break;
        case MSERV:
                smod = "MSER";
                break;
        case DOM:
                smod = "DOM_";
                break;
        case API:
                smod = "API_";
                break;
        case HTTP:
                smod = "HTTP";
                break;
        default:
                smod = "UNKN";
                break;
        }

        timeinfo = localtime(&now);
        strftime(timebuf, 26, "%Y-%m-%d %H:%M:%S", timeinfo);

        fprintf(fp,
                "%s UPNP-%s-%s: Thread:0x%lX [%s:%d]: ",
                timebuf,
                smod,
                slev,
#ifdef __PTW32_DLLPORT
                (unsigned long int)ithread_self().p
#else
                (unsigned long int)ithread_self()
#endif
                ,
                DbggLogFileName,
                DbgLineNo);
        fflush(fp);
}

void UpnpPrintf(UpnpLib *p,
        Upnp_LogLevel DLevel,
        Dbg_Module Module,
        const char *DbggLogFileName,
        int DbgLineNo,
        const char *FmtStr,
        ...)
{
        FILE *fp;

        /*fprintf(stderr, "UpnpPrintf: fp %p level %d glev %d mod %d DEBUG_ALL
          %d\n", fp, DLevel, gLogLevel, Module, DEBUG_ALL);*/
        va_list ArgList;

        if (!UpnpLib_get_gLogInitWasCalled(p)) {
                return;
        }

        if (!DebugAtThisLevel(p, DLevel, Module))
                return;

        fp = UpnpLib_get_gLogFp(p);
        if (!fp) {
                return;
        }

        ithread_mutex_lock(UpnpLib_getnc_gLogMutex(p));
        va_start(ArgList, FmtStr);
        if (DbggLogFileName) {
                UpnpDisplayFileAndLine(
                        fp, DbggLogFileName, DbgLineNo, DLevel, Module);
                vfprintf(fp, FmtStr, ArgList);
                fflush(fp);
        }
        va_end(ArgList);
        ithread_mutex_unlock(UpnpLib_getnc_gLogMutex(p));
}

/* No locking here, the app should be careful about not calling
   closelog from a separate thread... */
FILE *UpnpGetDebugFile(UpnpLib *p, Upnp_LogLevel DLevel, Dbg_Module Module)
{
        if (!DebugAtThisLevel(p, DLevel, Module)) {
                return NULL;
        } else {
                return UpnpLib_get_gLogFp(p);
        }
}
