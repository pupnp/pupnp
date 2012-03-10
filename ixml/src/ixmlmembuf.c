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


/*!
 * \file
 */


#include "ixmlmembuf.h"
#include "ixml.h"


#include <assert.h>
#include <stdlib.h>
#include <string.h>


/*!
 * \brief Increases or decreases buffer capacity so that at least 'new_length'
 * bytes can be stored.
 *
 * On error, m's fields do not change.
 *
 * \return
 * 	\li UPNP_E_SUCCESS
 * 	\li UPNP_E_OUTOF_MEMORY
 */
static int ixml_membuf_set_size(
	/*! [in,out] The memory buffer. */
	INOUT ixml_membuf *m,
	/*! [in] The new lenght. */
	IN size_t new_length)
{
	size_t diff;
	size_t alloc_len;
	char *temp_buf;

	if (new_length >= m->length) {
		/* increase length */
		/* need more mem? */
		if (new_length <= m->capacity) {
			/* have enough mem; done */
			return 0;
		}

		diff = new_length - m->length;
		alloc_len = MAXVAL(m->size_inc, diff) + m->capacity;
	} else {
		/* decrease length */
		assert(new_length <= m->length);

		/* if diff is 0..m->size_inc, don't free */
		if ((m->capacity - new_length) <= m->size_inc) {
			return 0;
		}
		alloc_len = new_length + m->size_inc;
	}

	assert(alloc_len >= new_length);

	temp_buf = realloc(m->buf, alloc_len + (size_t)1);
	if (temp_buf == NULL) {
		/* try smaller size */
		alloc_len = new_length;
		temp_buf = realloc(m->buf, alloc_len + (size_t)1);
		if (temp_buf == NULL) {
			return IXML_INSUFFICIENT_MEMORY;
		}
	}
	/* save */
	m->buf = temp_buf;
	m->capacity = alloc_len;

	return 0;
}


void ixml_membuf_init(ixml_membuf *m)
{
	assert(m != NULL);

	m->size_inc = MEMBUF_DEF_SIZE_INC;
	m->buf = NULL;
	m->length = (size_t)0;
	m->capacity = (size_t)0;
}


void ixml_membuf_destroy(ixml_membuf *m)
{
	if (m == NULL) {
		return;
	}

	free(m->buf);
	ixml_membuf_init(m);
}


int ixml_membuf_assign(
	ixml_membuf *m,
	const void *buf,
	size_t buf_len)
{
	int return_code;

	assert(m != NULL);

	/* set value to null */
	if (buf == NULL) {
		ixml_membuf_destroy(m);
		return IXML_SUCCESS;
	}
	/* alloc mem */
	return_code = ixml_membuf_set_size(m, buf_len);
	if (return_code != 0) {
		return return_code;
	}

	/* copy */
	memcpy(m->buf, buf, buf_len);

	/* null-terminate */
	m->buf[buf_len] = 0;
	m->length = buf_len;

	return IXML_SUCCESS;
}


int ixml_membuf_assign_str(
	ixml_membuf *m,
	const char *c_str)
{
	return ixml_membuf_assign(m, c_str, strlen(c_str));
}


int ixml_membuf_append(
	INOUT ixml_membuf *m,
	IN const void *buf)
{
	assert(m != NULL);

	return ixml_membuf_insert(m, buf, (size_t)1, m->length);
}


int ixml_membuf_append_str(
	INOUT ixml_membuf *m,
	IN const char *c_str)
{
	return ixml_membuf_insert(m, c_str, strlen(c_str), m->length);
}


int ixml_membuf_insert(
	INOUT ixml_membuf *m,
	IN const void *buf,
	IN size_t buf_len,
	size_t index)
{
	int return_code = 0;

	assert(m != NULL);

	if (index > m->length) {
		return IXML_INDEX_SIZE_ERR;
	}

	if (buf == NULL || buf_len == (size_t)0) {
		return 0;
	}
	/* alloc mem */
	return_code = ixml_membuf_set_size(m, m->length + buf_len);
	if (return_code != 0) {
		return return_code;
	}
	/* insert data */
	/* move data to right of insertion point */
	memmove(m->buf + index + buf_len, m->buf + index, m->length - index);
	memcpy(m->buf + index, buf, buf_len);
	m->length += buf_len;
	/* Null terminate */
	m->buf[m->length] = 0;

	return 0;
}

