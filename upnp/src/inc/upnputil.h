#ifndef UTIL_H
#define UTIL_H

/*******************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation
 * All rights reserved.
 * Copyright (c) 2012 France Telecom All rights reserved.
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

#include "upnp.h"
#include <errno.h>

/* usually used to specify direction of parameters in functions */
#ifndef IN
	#define IN
#endif

#ifndef OUT
	#define OUT
#endif

#ifndef INOUT
	#define INOUT
#endif


#define GEMD_OUT_OF_MEMORY -1
#define EVENT_TIMEDOUT -2
#define EVENT_TERMINATE	-3

/*! boolean type in C. */
#ifndef TRUE
	#define TRUE 1
#endif
#ifndef FALSE
	#define FALSE 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * \brief Copy no of bytes spcified by the LINE_SIZE constant, from the
 * source buffer. Null terminate the destination buffer.
 */
void linecopy(
	/*! [out] output buffer. */
	char dest[LINE_SIZE],
	/*! [in] input buffer. */
	const char *src);

/*!
 * \brief Copy no of bytes spcified by the NAME_SIZE constant, from the
 * source buffer. Null terminate the destination buffer
 */
void namecopy(
	/*! [out] output buffer. */
	char dest[NAME_SIZE],
	/*! [in] input buffer. */
	const char *src);

/*!
 * \brief Determine if the srclen passed in paramter is less than the
 * permitted LINE_SIZE. If it is use the passed parameter, if not
 * use the permitted LINE_SIZE as the length parameter.
 *
 * Copy no of bytes spcified by the LINE_SIZE constant, from the source
 * buffer. Null terminate the destination buffer.
 */
void linecopylen(
	/*! [out] output buffer. */
	char dest[LINE_SIZE],
	/*! [in] input buffer. */
	const char *src,
	/*! [in] bytes to be copied. */
	size_t srclen);

#ifdef __cplusplus
}
#endif

/* Size of the errorBuffer variable, passed to the strerror_r() function */
#define ERROR_BUFFER_LEN (size_t)256

/* C specific */
/* VC needs these in C++ mode too (do other compilers?) */
#if !defined(__cplusplus) || defined(UPNP_USE_MSVCPP)
	#ifdef WIN32
		#ifndef S_ISREG
			#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
		#endif
		#ifndef S_ISDIR
			#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
		#endif
		#ifndef EADDRINUSE		/* VS2010 has this defined */
			#define EADDRINUSE		WSAEADDRINUSE
		#endif
		#define strcasecmp		stricmp
		#define strncasecmp		strnicmp
		#define sleep(a)		Sleep((a)*1000)
		#define usleep(a)		Sleep((a)/1000)
		#define strerror_r(a,b,c)	(strerror_s((b),(c),(a)))
	#else
		#define max(a, b)   (((a)>(b))? (a):(b))
		#define min(a, b)   (((a)<(b))? (a):(b))
	#endif /* WIN32 */
#endif /* !defined(__cplusplus) || defined(UPNP_USE_MSVCPP) */

#endif /* UTIL_H */

