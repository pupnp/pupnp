/*******************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation 
 * Copyright (c) 2006 Rémi Turboult <r3mi@users.sourceforge.net>
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
 ******************************************************************************/

#ifndef UPNP_DEBUG_H
#define UPNP_DEBUG_H 

#include "upnp.h"
#include "upnpconfig.h"

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif


/** @name Other debugging features
          The UPnP SDK contains other features to aid in debugging.
 */

/*! @{ */

/** @name Upnp_LogLevel
 *  The user has the option to select 4 different types of debugging levels,
 *  see {\tt UpnpSetLogLevel}. 
 *  The critical level will show only those messages 
 *  which can halt the normal processing of the library, like memory 
 *  allocation errors. The remaining three levels are just for debugging 
 *  purposes.  Packet level will display all the incoming and outgoing 
 *  packets that are flowing between the control point and the device. 
 *  Info Level displays the other important operational information 
 *  regarding the working of the library. If the user selects All, 
 *  then the library displays all the debugging information that it has.
 *  \begin{itemize}
 *    \item {\tt UPNP_CRITICAL [0]}
 *    \item {\tt UPNP_PACKET [1]}
 *    \item {\tt UPNP_INFO [2]}
 *    \item {\tt UPNP_ALL [3]}
 *  \end{itemize}
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

/*! @{ */
typedef enum Upnp_LogLevel_e {
	UPNP_CRITICAL,
	UPNP_PACKET,
	UPNP_INFO,
	UPNP_ALL
} Upnp_LogLevel;
/*! @} */


/**
 * Default log level : see {\tt Upnp_LogLevel}
 */
#define UPNP_DEFAULT_LOG_LEVEL	UPNP_ALL



/***************************************************************************
 * Function : UpnpInitLog
 *
 * Parameters:	void
 *
 * Description:
 *	This functions initializes the log files
 *
 * Returns: int
 *	-1 : If fails
 *	UPNP_E_SUCCESS : if success
 ***************************************************************************/
#ifdef DEBUG
int UpnpInitLog();
#else
static UPNP_INLINE int UpnpInitLog() { return UPNP_E_SUCCESS; }
#endif


/***************************************************************************
 * Function : UpnpSetLogLevel
 *				
 * Parameters: Upnp_LogLevel log_level
 *
 * Description:							
 *	This functions set the log level (see {\tt Upnp_LogLevel}
 * Returns: void
 ***************************************************************************/
#ifdef DEBUG
void UpnpSetLogLevel(Upnp_LogLevel log_level);
#else
static UPNP_INLINE void UpnpSetLogLevel(Upnp_LogLevel log_level) {}
#endif


/***************************************************************************
 * Function : UpnpCloseLog						
 *								
 * Parameters:	void					
 *								
 * Description:							
 *	This functions closes the log files
 * Returns: void
 ***************************************************************************/
#ifdef DEBUG
void UpnpCloseLog();
#else
static UPNP_INLINE void UpnpCloseLog() {}
#endif


/***************************************************************************
 * Function : UpnpSetLogFileNames		
 *							
 * Parameters:						
 *	IN const char* ErrFileName: name of the error file
 *	IN const char *InfoFileName: name of the information file
 *	IN int size: Size of the buffer
 *	IN int starLength: This parameter provides the width of the banner
 *								
 * Description:							
 *	This functions takes the buffer and writes the buffer in the file as 
 *	per the requested banner	
 * Returns: void
 ***************************************************************************/
#ifdef DEBUG
void UpnpSetLogFileNames(
	const char *ErrFileName,
	const char *InfoFileName);
#else
static UPNP_INLINE void UpnpSetLogFileNames(
	const char *ErrFileName,
	const char *InfoFileName) {}
#endif


/***************************************************************************
 * Function : UpnpGetDebugFile		
 *						
 * Parameters:					
 *	IN Upnp_LogLevel DLevel: The level of the debug logging. It will decide 
 *		whether debug statement will go to standard output, 
 *		or any of the log files.
 *	IN Dbg_Module Module: debug will go in the name of this module
 *								
 * Description:
 *	This function checks if the module is turned on for debug 
 *	and returns the file descriptor corresponding to the debug level
 * Returns: FILE *
 *	NULL : if the module is turn off for debug 
 *	else returns the right file descriptor
 ***************************************************************************/
#ifdef DEBUG
FILE *UpnpGetDebugFile(Upnp_LogLevel level, Dbg_Module module);
#else
static UPNP_INLINE FILE *UpnpGetDebugFile(Upnp_LogLevel level, Dbg_Module module)
{
	return NULL;
}
#endif


/***************************************************************************
 * Function : DebugAtThisLevel					
 *									
 * Parameters:			
 *	IN Upnp_LogLevel DLevel: The level of the debug logging. It will decide 
 *		whether debug statement will go to standard output, 
 *		or any of the log files.
 *	IN Dbg_Module Module: debug will go in the name of this module
 *					
 * Description:					
 *	This functions returns true if debug output should be done in this
 *	module.
 *
 * Returns: int
 ***************************************************************************/
#ifdef DEBUG
int DebugAtThisLevel(
	IN Upnp_LogLevel DLevel,
	IN Dbg_Module Module);
#else
static UPNP_INLINE int DebugAtThisLevel(
	IN Upnp_LogLevel DLevel,
	IN Dbg_Module Module) { return 0; }
#endif


/***************************************************************************
 * Function : UpnpPrintf				
 *									
 * Parameters:								
 *	IN Upnp_LogLevel DLevel: The level of the debug logging. It will decide 
 *		whether debug statement will go to standard output, 
 *		or any of the log files.
 *	IN Dbg_Module Module: debug will go in the name of this module
 *	IN char *DbgFileName: Name of the file from where debug statement is
 *							coming
 *	IN int DbgLineNo : Line number of the file from where debug statement 
 *				is coming
 *	IN char * FmtStr, ...: Variable number of arguments that will go 
 *				in the debug statement
 *					
 * Description:							
 *	This functions prints the debug statement either on the startdard 
 *	output or log file along with the information from where this 
 *	debug statement is coming
 * Returns: void
 ***************************************************************************/ 
#ifdef DEBUG
void UpnpPrintf(
	Upnp_LogLevel DLevel,
	Dbg_Module Module,
	const char* DbgFileName,
	int DbgLineNo,
	const char* FmtStr,
	...)
#if (__GNUC__ >= 3)
	/* This enables printf like format checking by the compiler */
	__attribute__((format (__printf__, 5, 6)))
#endif
;
#else /* DEBUG */
static UPNP_INLINE void UpnpPrintf(
	Upnp_LogLevel DLevel,
	Dbg_Module Module,
	const char* DbgFileName,
	int DbgLineNo,
	const char* FmtStr,
	...) {}
#endif /* DEBUG */


/***************************************************************************
 * Function : UpnpDisplayBanner				
 *							
 * Parameters:							
 *	IN FILE *fd: file descriptor where the banner will be written
 *	IN char **lines: The buffer that will be written
 *	IN int size: Size of the buffer
 *	IN int starLength: This parameter provides the width of the banner
 *									
 * Description:							
 *	This functions takes the buffer and writes the buffer in the file as 
 *	per the requested banner			
 * Returns: void
 ***************************************************************************/
#ifdef DEBUG
void UpnpDisplayBanner(
	FILE *fd,
	const char **lines,
	size_t size,
	int starlength);
#else
static UPNP_INLINE void UpnpDisplayBanner(
	FILE *fd,
	const char **lines,
	size_t size,
	int starlength) {}
#endif


/***************************************************************************
 * Function : UpnpDisplayFileAndLine				
 *								
 * Parameters:							
 *	IN FILE *fd: File descriptor where line number and file name will be 
 *			written 
 *	IN char *DbgFileName: Name of the file  
 *	IN int DbgLineNo : Line number of the file
 *								
 * Description:
 *	This function writes the file name and file number from where
 *		debug statement is coming to the log file
 * Returns: void
 ***************************************************************************/
#ifdef DEBUG
void UpnpDisplayFileAndLine(
	FILE *fd,
	const char *DbgFileName,
	int DbgLineNo);
#else
static UPNP_INLINE void UpnpDisplayFileAndLine(
	FILE *fd,
	const char *DbgFileName,
	int DbgLineNo) {}
#endif

/*! @} */

#ifdef __cplusplus
}
#endif

#endif /* UPNP_DEBUG_H */

