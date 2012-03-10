/**************************************************************************
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
 **************************************************************************/


#ifndef IXML_MEMBUF_H
#define IXML_MEMBUF_H


/*!
 * \file
 */


#include "ixml.h"


#include <stdlib.h> /* for size_t */


#define MINVAL(a, b) ( (a) < (b) ? (a) : (b) )
#define MAXVAL(a, b) ( (a) > (b) ? (a) : (b) )


#define MEMBUF_DEF_SIZE_INC 20u


/*!
 * \brief The ixml_membuf type.
 */
typedef struct
{
	char *buf;	
	size_t length;
	size_t capacity;
	size_t size_inc;
} ixml_membuf;


/*!
 * \brief ixml_membuf initialization routine.
 */
void ixml_membuf_init(
	/*! [in,out] The memory buffer to initializa. */
	ixml_membuf *m);


/*!
 * \brief ixml_membuf clearing routine.
 *
 * The internal buffer is deleted and ixml_membuf_init() is called in the end
 * to reinitialize the buffer.
 */
void ixml_membuf_destroy(
	/*! [in,out] The memory buffer to clear. */
	ixml_membuf *m);


/*!
 * \brief Copies the contents o a buffer to the designated ixml_membuf.
 *
 * The previous contents of the ixml_membuf are destroyed.
 *
 * \return IXML_SUCCESS if successfull, or the error code returned
 * by ixml_membuf_set_size().
 *
 * \sa ixml_membuf_assign_str().
 */
int ixml_membuf_assign(
	/*! [in,out] The memory buffer on which to operate. */
	ixml_membuf *m,
	/*! [in] The input buffer to copy from. */
	const void *buf, 
	/*! [in] The number of bytes to copy from the input buffer. */
	size_t buf_len);

/*!
 * \brief Copies a \b NULL terminated string to the ixml_buffer.
 *
 * This is a convenience function that internally uses ixml_membuf_assign().
 *
 * \return The return value of ixml_membuf_assign().
 *
 * \sa ixml_membuf_assign().
 */
int ixml_membuf_assign_str(
	/*! [in,out] The memory buffer on which to operate. */
	ixml_membuf *m,
	/*! [in] The input string to copy from. */
	const char *c_str);

/*!
 * \brief Appends one byte to the designated ixml_membuffer.
 *
 * This is a convenience function that internally uses ixml_membuf_insert().
 *
 * \return The return value of ixml_membuf_insert().
 *
 * \sa ixml_membuf_insert()
 */
int ixml_membuf_append(
	/*! [in,out] The memory buffer on which to operate. */
	ixml_membuf *m,
	/*! [in] The pointer to the byte to append. */
	const void *buf);

/*!
 * \brief Appends the contents of a \b NULL terminated string to the designated
 * ixml_membuf.
 *
 * This is a convenience function that internally uses ixml_membuf_insert().
 *
 * \return The return value of ixml_membuf_insert().
 *
 * \sa ixml_membuf_insert().
 */
int ixml_membuf_append_str(
	/*! [in,out] The memory buffer on which to operate. */
	ixml_membuf *m,
	/*! [in] The input string to copy from. */
	const char *c_str);

/*!
 * \brief 
 *
 * \return
 * 	\li 0 if successfull.
 * 	\li IXML_INDEX_SIZE_ERR if the index parameter is out of range.
 * 	\li Or the return code of ixml_membuf_set_size()
 *
 * \sa ixml_membuf_set_size()
 */
int ixml_membuf_insert(
	/*! [in,out] The memory buffer on which to operate. */
	ixml_membuf *m,
	/*! [in] The pointer to the input buffer. */
	const void *buf, 
	/*! [in] The buffer length. */
	size_t buf_len,
	/*! [in] The point of insertion relative to the beggining of the
	 * ixml_membuf internal buffer. */
	size_t index);


#endif /* IXML_MEMBUF_H */

