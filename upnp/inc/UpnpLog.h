#ifndef UPNPLOG_H
#define UPNPLOG_H
/*******************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation
 * Copyright (c) 2006 Rémi Turboult <r3mi@users.sourceforge.net>
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

#include "upnpconfig.h"

#include "UpnpGlobal.h" /* for UPNP_INLINE */

#include <stdio.h>
#include <string.h>

typedef struct s_UpnpLib UpnpLib;

/*! \name Other debugging features
 *
 * The UPnP SDK contains other features to aid in debugging.
 */
/*@{*/

/*! \name Upnp_LogLevel
 *  The user has the option to select 4 different types of debugging levels,
 *  see \c UpnpSetLogLevel.
 *  The critical level will show only those messages
 *  which can halt the normal processing of the library, like memory
 *  allocation errors. The remaining three levels are just for debugging
 *  purposes. Error will show recoverable errors.
 *  Info Level displays the other important operational information
 *  regarding the working of the library. If the user selects Debug,
 *    \li \c UPNP_LOG_LEVEL_ERROR [-1]
 *    \li \c UPNP_CRITICAL [0]
 *    \li \c UPNP_ERROR [1]
 *    \li \c UPNP_INFO [2]
 *    \li \c UPNP_DEBUG [3]
 *    \li \c UPNP_NONE [4]
 */
typedef enum Upnp_Module
{
        SSDP,
        SOAP,
        GENA,
        TPOOL,
        MSERV,
        DOM,
        API,
        HTTP
} Dbg_Module;

/*@{*/
typedef enum Upnp_LogLevel_e
{
        UPNP_LOG_LEVEL_ERROR = -1,
        UPNP_CRITICAL,
        UPNP_ERROR,
        UPNP_INFO,
        UPNP_DEBUG,
        UPNP_NONE
} Upnp_LogLevel;
/*@}*/

/* UPNP_PACKET probably resulted from a confusion between module and
   level and was only used by a few messages in ssdp_device.c (they
   have been moved to INFO). Kept for compatibility, don't use for new
   messages.
*/
#define UPNP_PACKET UPNP_ERROR

/* Backwards compat to help any apps that happened to use this logging before
 * it was promoted
 */
#define UPNP_ALL UPNP_DEBUG

/*!
 * Default log level : see \c Upnp_LogLevel
 */
#define UPNP_DEFAULT_LOG_LEVEL UPNP_DEBUG

/*!
 * \brief Log callback function prototype.
 */
typedef void (*LogCallback)(
        /*! [in] Level of the log. */
        Upnp_LogLevel level,
        Dbg_Module module,
        const char *sourceFile,
        const int *sourceLine,
        const char *log);

#ifdef __cplusplus

#include "ithread.h"

#include <string>

class UpnpLog
{
public:
        UpnpLog();
        ~UpnpLog();

        int InitLog();
        void CloseLog();
        void Printf(Upnp_LogLevel DLevel,
                Dbg_Module Module,
                const char *file,
                int DbgLineNo,
                const char *FmtStr,
                ...)
#if (__GNUC__ >= 3)
                __attribute__((format(__printf__, 6, 7)))
#endif
                ;

        Upnp_LogLevel logLevel() const { return m_logLevel; }
        void setLogLevel(Upnp_LogLevel n) { m_logLevel = n; }

        int logIsStderr() const { return m_logIsStderr; }
        void setLogIsStderr(int n) { m_logIsStderr = n; }

        const std::string &logFileName() const { return m_logFileName; }
        void setLogFileName(const char *s)
        {
                if (!s) {
                        m_logFileName.clear();
                } else {
                        m_logFileName = s;
                }
        }

        LogCallback logCallback() const { return m_logCallback; }
        void setLogCallback(LogCallback p) { m_logCallback = p; }

private:
        ithread_mutex_t m_logMutex;
        Upnp_LogLevel m_logLevel;
        FILE *m_logFp;
        int m_logIsStderr;
        std::string m_logFileName;
        LogCallback m_logCallback;

        void logMutexLock() { ithread_mutex_lock(&m_logMutex); }
        void logMutexUnlock() { ithread_mutex_unlock(&m_logMutex); }

        FILE *logFp() const { return m_logFp; }
        void setLogFp(FILE *fp) { m_logFp = fp; }

        int DebugAtThisLevel(Upnp_LogLevel DLevel, Dbg_Module Module);
        void SetLogConfigFromEnvironment();
        void DisplayFileAndLine(const char *file,
                int line,
                Upnp_LogLevel DLevel,
                Dbg_Module Module);
};

extern "C" {

#else /* __cplusplus */

typedef struct s_UpnpLog UpnpLog;

#endif /* __cplusplus */

/*!
 * \brief Creates an instance of an UpnpLog object.
 */
EXPORT_SPEC UpnpLog *UpnpLog_new();

/*!
 * \brief Destroys an instance of an UpnpLog object.
 */
EXPORT_SPEC void UpnpLog_delete(UpnpLog *p);

/*!
 * \brief Converts a Upnp_LogLevel into a string.
 */
EXPORT_SPEC const char *UpnpLogLevelToStr(Upnp_LogLevel level);

/*!
 * \brief Converts a string to a Upnp_LogLevel.
 */
EXPORT_SPEC Upnp_LogLevel UpnpLogLevelFromStr(char *level);

/*!
 * \brief Initialize the log files.
 *
 * \return -1 if fails or UPNP_E_SUCCESS if succeeds.
 */
EXPORT_SPEC int UpnpInitLog(
        /*! Library Handle */
        UpnpLog *p);

/*!
 * \brief Set the log level (see \c Upnp_LogLevel).
 */
EXPORT_SPEC void UpnpSetLogLevel(
        /*! Library Handle */
        UpnpLog *p,
        /*! [in] Log level. */
        Upnp_LogLevel log_level);

/*!
 * \brief Closes the log files.
 */
EXPORT_SPEC void UpnpCloseLog(
        /*! Library Handle */
        UpnpLog *p);

/*!
 * \brief Set the name for the log file. There used to be 2 separate files. The
 * second parameter has been kept for compatibility but is ignored.
 * Use a NULL file name for logging to stderr.
 */
EXPORT_SPEC void UpnpSetLogFileName(
        /*! Library Handle */
        UpnpLog *p,
        /*! [in] Name of the log file. */
        const char *fileName);

/*!
 * \brief Prints the debug statement either on the standard output or log file
 * along with the information from where this debug statement is coming.
 */
EXPORT_SPEC void UpnpPrintf(
        /*! Library Handle */
        UpnpLog *p,
        /*! [in] The level of the debug logging. It will decide whether debug
         * statement will go to standard output, or any of the log files. */
        Upnp_LogLevel DLevel,
        /*! [in] debug will go in the name of this module. */
        Dbg_Module Module,
        /*! [in] Name of the file from where debug statement is coming. */
        const char *DbgFileName,
        /*! [in] Line number of the file from where debug statement is coming.
         */
        int DbgLineNo,
        /*! [in] Printf like format specification. */
        const char *FmtStr,
        /*! [in] Printf like Variable number of arguments that will go in the
         * debug statement. */
        ...)
#if (__GNUC__ >= 3)
        /* This enables printf like format checking by the compiler. */
        __attribute__((format(__printf__, 6, 7)))
#endif
        ;

/*!
 * \brief Set the logging callback.
 */
EXPORT_SPEC void UpnpSetLogCallback(
        /*! Library Handle */
        UpnpLog *p,
        /*! [in] Callback to be called for each log produced by the library. */
        LogCallback callback);

#ifdef __cplusplus
}
#endif

#endif /* UPNPLOG_H */
