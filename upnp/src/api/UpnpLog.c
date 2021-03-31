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

#if linux
#include <sys/syscall.h>
#endif

const char *UpnpLogLevelToStr(Upnp_LogLevel level)
{
        switch (level) {
        case UPNP_CRITICAL:
                return "CRIT";
        case UPNP_ERROR:
                return "ERRO";
        case UPNP_INFO:
                return "INFO";
        case UPNP_DEBUG:
                return "DEBG";
        default:
                return "UNKN";
        }
}

Upnp_LogLevel UpnpLogLevelFromStr(char *level)
{
        if (strcmp(level, "CRIT") == 0) {
                return UPNP_CRITICAL;
        }
        if (strcmp(level, "ERRO") == 0) {
                return UPNP_ERROR;
        }
        if (strcmp(level, "INFO") == 0) {
                return UPNP_INFO;
        }
        if (strcmp(level, "DEBUG") == 0) {
                return UPNP_DEBUG;
        }
        if (strcmp(level, "NONE") == 0) {
                return UPNP_NONE;
        }
        return -1;
}

static void UpnpSetLogConfigFromEnvironment(UpnpLib *p)
{
        char *logFile = getenv("UPNP_LOG_FILE");
        if (logFile) {
                UpnpSetLogFileName(p, strdup(logFile));
        }

        char *logLevel = getenv("UPNP_LOG_LEVEL");
        if (logLevel) {
                Upnp_LogLevel mapped = UpnpLogLevelFromStr(logLevel);
                if (mapped != UPNP_LOG_LEVEL_ERROR) {
                        UpnpSetLogLevel(p, mapped);
                }
        }
}

/* This routine is called from UpnpInit2().
 * It can be called again, for example to rotate the log file, and we try to
 * avoid multiple calls to the mutex init, with a risk of race, probably not a
 * problem, and not worth fixing. */
int UpnpInitLog(UpnpLib *p)
{
        const char *fname;
        FILE *fp = 0;

        if (!UpnpLib_get_LogInitWasCalled(p)) {
                ithread_mutex_init(UpnpLib_getnc_LogMutex(p), NULL);
                UpnpLib_set_LogInitWasCalled(p, 1);
        }

        /* Env vars override other config */
        UpnpSetLogConfigFromEnvironment(p);

        /* If the user did not ask for logging do nothing */
        if (!UpnpLib_get_SetLogWasCalled(p)) {
                return UPNP_E_SUCCESS;
        }

        if (UpnpLib_get_LogFp(p)) {
                if (!UpnpLib_get_LogIsStderr(p)) {
                        fclose(UpnpLib_get_LogFp(p));
                        UpnpLib_set_LogFp(p, 0);
                }
        }
        UpnpLib_set_LogIsStderr(p, 0);
        fname = UpnpLib_get_LogFileName(p);
        if (fname) {
                fp = fopen(fname, "a");
                UpnpLib_set_LogFp(p, fp);
                if (!fp) {
                        fprintf(stderr,
                                "Failed to open gLogFileName (%s): %s\n",
                                UpnpLib_get_LogFileName(p),
                                strerror(errno));
                }
        }
        if (!fp) {
                UpnpLib_set_LogFp(p, stderr);
                UpnpLib_set_LogIsStderr(p, 1);
        }
        return UPNP_E_SUCCESS;
}

void UpnpSetLogLevel(UpnpLib *p, Upnp_LogLevel log_level)
{
        UpnpLib_set_LogLevel(p, log_level);
        UpnpLib_set_SetLogWasCalled(p, 1);
}

void UpnpCloseLog(UpnpLib *p)
{
        FILE *fp;
        if (!UpnpLib_get_LogInitWasCalled(p)) {
                return;
        }

        /* Calling lock() assumes that someone called UpnpInitLog(), but
         * this is reasonable as it is called from UpnpInit2(). We risk a
         * crash if we do this without a lock.*/
        ithread_mutex_lock(UpnpLib_getnc_LogMutex(p));
        fp = UpnpLib_get_LogFp(p);
        if (fp && !UpnpLib_get_LogIsStderr(p)) {
                fclose(fp);
        }
        UpnpLib_set_LogFp(p, 0);
        UpnpLib_set_LogIsStderr(p, 0);
        UpnpLib_set_LogInitWasCalled(p, 0);
        ithread_mutex_unlock(UpnpLib_getnc_LogMutex(p));
        ithread_mutex_destroy(UpnpLib_getnc_LogMutex(p));
}

void UpnpSetLogFileName(UpnpLib *p, const char *newgLogFileName)
{
        if (UpnpLib_get_LogFileName(p)) {
                free(UpnpLib_get_LogFileName(p));
                UpnpLib_set_LogFileName(p, 0);
        }
        if (newgLogFileName && *newgLogFileName) {
                UpnpLib_set_LogFileName(p, strdup(newgLogFileName));
        }
        UpnpLib_set_SetLogWasCalled(p, 1);
}

static int DebugAtThisLevel(UpnpLib *p, Upnp_LogLevel DLevel, Dbg_Module Module)
{
        (void)Module;

        return (DLevel <= UpnpLib_get_LogLevel(p)) &&
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
                smod = "DOM ";
                break;
        case API:
                smod = "API ";
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
                "%s %s UPNP:%s [0x%016lX][%s:%d] ",
                timebuf,
                UpnpLogLevelToStr(DLevel),
                smod,
#ifdef __PTW32_DLLPORT
                (unsigned long int)ithread_self().p
#else
#if linux
                (unsigned long int)syscall(SYS_gettid)
#else
                (unsigned long int)ithread_self()
#endif
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
        va_list ArgList;

        /*fprintf(stderr, "UpnpPrintf: fp %p level %d glev %d mod %d DEBUG_ALL
          %d\n", fp, DLevel, gLogLevel, Module, DEBUG_ALL);*/
        if (!UpnpLib_get_LogInitWasCalled(p)) {
                return;
        }

        va_start(ArgList, FmtStr);
        ithread_mutex_lock(UpnpLib_getnc_LogMutex(p));

        if (UpnpLib_get_LogCallback(p)) {
                char *buffer;
                int size;
#ifndef _WIN32
                size = vsnprintf(NULL, 0, FmtStr, ArgList) + 1;
#else
                size = _vscprintf(FmtStr, ArgList) + 1;
#endif
                buffer = (char *)malloc(size);
                if (!buffer) {
                        goto exit_function;
                }
                vsnprintf(buffer, size, FmtStr, ArgList);
                UpnpLib_get_LogCallback(p)(
                        DLevel, Module, DbggLogFileName, &DbgLineNo, buffer);
                free(buffer);
        }

        if (!DebugAtThisLevel(p, DLevel, Module))
                goto exit_function;

        fp = UpnpLib_get_LogFp(p);
        if (!fp) {
                goto exit_function;
        }

        if (DbggLogFileName) {
                UpnpDisplayFileAndLine(
                        fp, DbggLogFileName, DbgLineNo, DLevel, Module);
                vfprintf(fp, FmtStr, ArgList);
                fflush(fp);
        }

exit_function:
        ithread_mutex_unlock(UpnpLib_getnc_LogMutex(p));
        va_end(ArgList);
}

/* No locking here, the app should be careful about not calling
   closelog from a separate thread... */
FILE *UpnpGetDebugFile(UpnpLib *p, Upnp_LogLevel DLevel, Dbg_Module Module)
{
        if (!DebugAtThisLevel(p, DLevel, Module)) {
                return NULL;
        } else {
                return UpnpLib_get_LogFp(p);
        }
}

int UpnpSetLogCallback(UpnpLib *p, LogCallback callback)
{
        UpnpLib_set_LogCallback(p, callback);
}