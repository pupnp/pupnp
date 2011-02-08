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

#ifndef UPNP_DEBUG_H
#define UPNP_DEBUG_H

/*!
 * \file
 */

#include "ThreadPool.h"
#include "upnpconfig.h"
#include "UpnpGlobal.h"		/* for UPNP_INLINE */

#include <stdio.h>

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
 *  purposes.  Packet level will display all the incoming and outgoing 
 *  packets that are flowing between the control point and the device. 
 *  Info Level displays the other important operational information 
 *  regarding the working of the library. If the user selects All, 
 *  then the library displays all the debugging information that it has.
 *    \li \c UPNP_CRITICAL [0]
 *    \li \c UPNP_PACKET [1]
 *    \li \c UPNP_INFO [2]
 *    \li \c UPNP_ALL [3]
 */
typedef enum Upnp_Module {
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
typedef enum Upnp_LogLevel_e {
	UPNP_CRITICAL,
	UPNP_PACKET,
	UPNP_INFO,
	UPNP_ALL
} Upnp_LogLevel;
/*@}*/

/*!
 * Default log level : see \c Upnp_LogLevel
 */
#define UPNP_DEFAULT_LOG_LEVEL	UPNP_ALL

/*!
 * \brief Initialize the log files.
 *
 * \return -1 if fails or UPNP_E_SUCCESS if succeeds.
 */
#ifdef DEBUG
int UpnpInitLog(void);
#else
static UPNP_INLINE int UpnpInitLog(void)
{
	return UPNP_E_SUCCESS;
}
#endif
/*!
 * \brief Set the log level (see \c Upnp_LogLevel).
 */
#ifdef DEBUG
void UpnpSetLogLevel(
	/*! [in] Log level. */
	Upnp_LogLevel log_level);
#else
static UPNP_INLINE void UpnpSetLogLevel(Upnp_LogLevel log_level)
{
	return;
	log_level = log_level;
}
#endif

/*!
 * \brief Closes the log files.
 */
#ifdef DEBUG
void UpnpCloseLog(void);
#else
static UPNP_INLINE void UpnpCloseLog(void)
{
}
#endif

/*!
 * \brief Set the name for error and information files, respectively.
 */
#ifdef DEBUG
void UpnpSetLogFileNames(
	/*! [in] Name of the error file. */
	const char *ErrFileName,
	/*! [in] Name of the information file. */
	const char *InfoFileName);
#else
static UPNP_INLINE void UpnpSetLogFileNames(const char *ErrFileName,
	const char *InfoFileName)
{
	return;
	ErrFileName = ErrFileName;
	InfoFileName = InfoFileName;
}
#endif

/*!
 * \brief Check if the module is turned on for debug and returns the file
 * descriptor corresponding to the debug level
 *
 * \return NULL if the module is turn off for debug otheriwse returns the
 *	right file descriptor.
 */
#ifdef DEBUG
FILE *UpnpGetDebugFile(
	/*! [in] The level of the debug logging. It will decide whether debug
	 * statement will go to standard output, or any of the log files. */
	Upnp_LogLevel level,
	/*! [in] debug will go in the name of this module. */
	Dbg_Module module);
#else
static UPNP_INLINE FILE *UpnpGetDebugFile(Upnp_LogLevel level, Dbg_Module module)
{
	return NULL;
	level = level;
	module = module;
}
#endif

/*!
 * \brief Returns true if debug output should be done in this module.
 *
 * \return Nonzero value if true, zero if false.
 */
#ifdef DEBUG
int DebugAtThisLevel(
	/*! [in] The level of the debug logging. It will decide whether debug
	 * statement will go to standard output, or any of the log files. */
	Upnp_LogLevel DLevel,
	/*! [in] Debug will go in the name of this module. */
	Dbg_Module Module);
#else
static UPNP_INLINE int DebugAtThisLevel(Upnp_LogLevel DLevel, Dbg_Module Module)
{
	return 0;
	DLevel = DLevel;
	Module = Module;
}
#endif

/*!
 * \brief Prints the debug statement either on the standard output or log file
 * along with the information from where this debug statement is coming.
 */
#ifdef DEBUG
void UpnpPrintf(
	/*! [in] The level of the debug logging. It will decide whether debug
	 * statement will go to standard output, or any of the log files. */
	Upnp_LogLevel DLevel,
	/*! [in] debug will go in the name of this module. */
	Dbg_Module Module,
	/*! [in] Name of the file from where debug statement is coming. */
	const char *DbgFileName,
	/*! [in] Line number of the file from where debug statement is coming. */
	int DbgLineNo,
	/*! [in] Printf like format specification. */
	const char *FmtStr,
	/*! [in] Printf like Variable number of arguments that will go in the
	 * debug statement. */
	...)
#if (__GNUC__ >= 3)
	/* This enables printf like format checking by the compiler. */
	__attribute__ ((format(__printf__, 5, 6)))
#endif
	;
#else /* DEBUG */
static UPNP_INLINE void UpnpPrintf(Upnp_LogLevel DLevel, Dbg_Module Module,
	const char *DbgFileName, int DbgLineNo, const char *FmtStr, ...)
{
	return;
	DLevel = DLevel;
	Module = Module;
	DbgFileName = DbgFileName;
	DbgLineNo = DbgLineNo;
	FmtStr = FmtStr;
}
#endif /* DEBUG */

/*!
 * \brief Writes the file name and file number from where debug statement is
 * coming to the log file.
 */
#ifdef DEBUG
void UpnpDisplayFileAndLine(
	/*! [in] File descriptor where line number and file name will be
	 * written. */
	FILE * fd,
	/*! [in] Name of the file. */
	const char *DbgFileName,
	/*! [in] Line number of the file. */
	int DbgLineNo);
#else
static UPNP_INLINE void UpnpDisplayFileAndLine(FILE *fd,
	const char *DbgFileName, int DbgLineNo)
{
	return;
	fd = fd;
	DbgFileName = DbgFileName;
	DbgLineNo = DbgLineNo;
}
#endif

/*!
 * \brief Writes the buffer in the file as per the requested banner
 */
#ifdef DEBUG
void UpnpDisplayBanner(
	/*! [in] file descriptor where the banner will be written. */
	FILE * fd,
	/*! [in] The buffer that will be written. */
	const char **lines,
	/*! [in] Size of the buffer. */
	size_t size,
	/*! [in] This parameter provides the width of the banner. */
	size_t starlength);
#else
static UPNP_INLINE void UpnpDisplayBanner(FILE *fd, const char **lines,
	size_t size, int starlength)
{
	return;
	fd = fd;
	lines = lines;
	size = size;
	starlength = starlength;
}
#endif

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /* UPNP_DEBUG_H */
