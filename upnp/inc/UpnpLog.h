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

typedef struct s_UpnpLib UpnpLib;

#ifdef __cplusplus
extern "C" {
#endif

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
 *  regarding the working of the library. If the user selects All,
 *  then the library displays all the debugging information that it has.
 *    \li \c UPNP_CRITICAL [0]
 *    \li \c UPNP_ERROR [1]
 *    \li \c UPNP_INFO [2]
 *    \li \c UPNP_ALL [3]
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
        UPNP_CRITICAL,
        UPNP_ERROR,
        UPNP_INFO,
        UPNP_ALL
} Upnp_LogLevel;
/*@}*/

/* UPNP_PACKET probably resulted from a confusion between module and
   level and was only used by a few messages in ssdp_device.c (they
   have been moved to INFO). Kept for compatibility, don't use for new
   messages.
*/
#define UPNP_PACKET UPNP_ERROR

/*!
 * Default log level : see \c Upnp_LogLevel
 */
#define UPNP_DEFAULT_LOG_LEVEL UPNP_ALL

/*!
 * \brief Initialize the log files.
 *
 * \return -1 if fails or UPNP_E_SUCCESS if succeeds.
 */
int UpnpInitLog(
        /*! Library Handle */
        UpnpLib *p);

#if defined NDEBUG && !defined UPNP_DEBUG_C
#define UpnpInitLog UpnpInitLog_Inlined
static UPNP_INLINE int UpnpInitLog_Inlined(UpnpLib *p)
{
        (void)p;

        return 0;
}
#endif
/*!
 * \brief Set the log level (see \c Upnp_LogLevel).
 */
void UpnpSetLogLevel(
        /*! Library Handle */
        UpnpLib *p,
        /*! [in] Log level. */
        Upnp_LogLevel log_level);

#if defined NDEBUG && !defined UPNP_DEBUG_C
#define UpnpSetLogLevel UpnpSetLogLevel_Inlined
static UPNP_INLINE void UpnpSetLogLevel_Inlined(
        UpnpLib *p, Upnp_LogLevel log_level)
{
        (void)p;
        (void)log_level;
        return;
}
#endif

/*!
 * \brief Closes the log files.
 */
void UpnpCloseLog(
        /*! Library Handle */
        UpnpLib *p);

#if defined NDEBUG && !defined UPNP_DEBUG_C
#define UpnpCloseLog UpnpCloseLog_Inlined
static UPNP_INLINE void UpnpCloseLog_Inlined(UpnpLib *p) { (void)p; }
#endif

/*!
 * \brief Set the name for the log file. There used to be 2 separate files. The
 * second parameter has been kept for compatibility but is ignored.
 * Use a NULL file name for logging to stderr.
 */
void UpnpSetLogFileNames(
        /*! Library Handle */
        UpnpLib *p,
        /*! [in] Name of the log file. */
        const char *fileName,
        /*! [in] Ignored. */
        const char *Ignored);

#if defined NDEBUG && !defined UPNP_DEBUG_C
#define UpnpSetLogFileNames UpnpSetLogFileNames_Inlined
static UPNP_INLINE void UpnpSetLogFileNames_Inlined(
        UpnpLib *p, const char *ErrFileName, const char *ignored)
{
        (void)p;
        (void)ErrFileName;
        (void)ignored;
        return;
}
#endif

/*!
 * \brief Check if the module is turned on for debug and returns the file
 * descriptor corresponding to the debug level
 *
 * \return NULL if the module is turn off for debug otherwise returns the
 *	right FILE pointer.
 */
FILE *UpnpGetDebugFile(
        /*! Library Handle */
        UpnpLib *p,
        /*! [in] The level of the debug logging. It will decide whether debug
         * statement will go to standard output, or any of the log files. */
        Upnp_LogLevel level,
        /*! [in] debug will go in the name of this module. */
        Dbg_Module module);

#if defined NDEBUG && !defined UPNP_DEBUG_C
#define UpnpGetDebugFile UpnpGetDebugFile_Inlined
static UPNP_INLINE FILE *UpnpGetDebugFile_Inlined(
        UpnpLib *p, Upnp_LogLevel level, Dbg_Module module)
{
        (void)p;
        (void)level;
        (void)module;
        return NULL;
}
#endif

/*!
 * \brief Prints the debug statement either on the standard output or log file
 * along with the information from where this debug statement is coming.
 */
void UpnpPrintf(
        /*! Library Handle */
        UpnpLib *p,
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

#if defined NDEBUG && !defined UPNP_DEBUG_C
#define UpnpPrintf UpnpPrintf_Inlined
// static UPNP_INLINE void UpnpPrintf_Inlined(Upnp_LogLevel DLevel,
// 	Dbg_Module Module,
// 	const char *DbgFileName,
// 	int DbgLineNo,
// 	const char *FmtStr,
// 	...)
// #if (__GNUC__ >= 3)
// 	/* This enables printf like format checking by the compiler. */
// 	__attribute__((format(__printf__, 5, 6)))
// #endif
// 	;
static UPNP_INLINE void UpnpPrintf_Inlined(UpnpLib *p,
        Upnp_LogLevel DLevel,
        Dbg_Module Module,
        const char *DbgFileName,
        int DbgLineNo,
        const char *FmtStr,
        ...)
{
        (void)p;
        (void)DLevel;
        (void)Module;
        (void)DbgFileName;
        (void)DbgLineNo;
        (void)FmtStr;
        return;
}
#endif /* DEBUG */

#ifdef __cplusplus
}
#endif

#endif /* UPNPLOG_H */
