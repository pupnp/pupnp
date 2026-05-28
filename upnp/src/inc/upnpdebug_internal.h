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

#ifndef UPNP_DEBUG_INTERNAL_H
#define UPNP_DEBUG_INTERNAL_H

/*!
 * \file
 * Internal declaration of UpnpPrintf.  Include this header (not upnpdebug.h)
 * from library-internal translation units that call UpnpPrintf directly.
 * External callers must use UpnpGetDebugFile() from the public upnpdebug.h.
 */

#include "upnpdebug.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * \brief Prints the debug statement either on the standard output or log file
 * along with the information from where this debug statement is coming.
 *
 * This function is internal to libupnp and is not part of the public API.
 */
void UpnpPrintf(
	/*! [in] The level of the debug logging. */
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
	/*! [in] Printf like Variable number of arguments. */
	...)
#if (__GNUC__ >= 3)
	__attribute__((format(__printf__, 5, 6)))
#endif
	;

#ifdef __cplusplus
}
#endif

#endif /* UPNP_DEBUG_INTERNAL_H */
