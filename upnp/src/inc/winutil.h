#ifndef WINUTIL_H
#define WINUTIL_H

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

/* C specific */
/* VC needs these in C++ mode too (do other compilers?) */
#if !defined(__cplusplus) || defined(UPNP_USE_MSVCPP)

	#ifdef _WIN32

		#ifndef S_ISREG
			#define S_ISREG(m) (((m)&S_IFMT) == S_IFREG)
		#endif

		#ifndef S_ISDIR
			#define S_ISDIR(m) (((m)&S_IFMT) == S_IFDIR)
		#endif

		#ifndef EADDRINUSE /* VS2010 has this defined */
			#define EADDRINUSE WSAEADDRINUSE
		#endif

		#define strcasecmp stricmp
		#define strncasecmp strnicmp
		#define strerror_r(a, b, c) (strerror_s((b), (c), (a)))

	#endif /* _WIN32 */

#endif /* !defined(__cplusplus) || defined(UPNP_USE_MSVCPP) */

#endif /* WINUTIL_H */
