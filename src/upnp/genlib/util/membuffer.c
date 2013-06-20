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

/*
 * \file
 *
 * \brief This file contains functions that operate on memory and buffers,
 * allocation, re-allocation, and modification of the memory
 */

#include "config.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "membuffer.h"
#include "upnp.h"
#include "unixutil.h"

char *str_alloc(const char *str, size_t str_len)
{
	char *s;

	s = (char *)malloc(str_len + (size_t)1);
	if (s == NULL) {
		return NULL;	/* no mem */
	}

	memcpy(s, str, str_len);
	s[str_len] = '\0';

	return s;
}

int memptr_cmp(memptr * m, const char *s)
{
	int cmp;

	cmp = strncmp(m->buf, s, m->length);

	if (cmp == 0 && m->length < strlen(s)) {
		/* both strings equal for 'm->length' chars */
		/*  if m is shorter than s, then s is greater */
		return -1;
	}

	return cmp;
}

int memptr_cmp_nocase(memptr * m, const char *s)
{
	int cmp;

	cmp = strncasecmp(m->buf, s, m->length);
	if (cmp == 0 && m->length < strlen(s)) {
		/* both strings equal for 'm->length' chars */
		/*  if m is shorter than s, then s is greater */
		return -1;
	}

	return cmp;
}

/*!
 * \brief Initialize the buffer.
 */
static UPNP_INLINE void membuffer_initialize(
	/*! [in,out] Buffer to be initialized. */
	membuffer *m)
{
	m->buf = NULL;
	m->length = (size_t)0;
	m->capacity = (size_t)0;
}

int membuffer_set_size(membuffer *m, size_t new_length)
{
	size_t diff;
	size_t alloc_len;
	char *temp_buf;

	if (new_length >= m->length) {	/* increase length */
		/* need more mem? */
		if (new_length <= m->capacity) {
			return 0;	/* have enough mem; done */
		}

		diff = new_length - m->length;
		alloc_len = MAXVAL(m->size_inc, diff) + m->capacity;
	} else {		/* decrease length */

		assert(new_length <= m->length);

		/* if diff is 0..m->size_inc, don't free */
		if ((m->capacity - new_length) <= m->size_inc) {
			return 0;
		}

		alloc_len = new_length + m->size_inc;
	}

	assert(alloc_len >= new_length);

	temp_buf = realloc(m->buf, alloc_len + (size_t)1);	/*LEAK_FIX_MK */

	/*temp_buf = Realloc( m->buf,m->length, alloc_len + 1 );LEAK_FIX_MK */

	if (temp_buf == NULL) {
		/* try smaller size */
		alloc_len = new_length;
		temp_buf = realloc(m->buf, alloc_len + (size_t)1);	/*LEAK_FIX_MK */
		/*temp_buf = Realloc( m->buf,m->length, alloc_len + 1 );LEAK_FIX_MK */

		if (temp_buf == NULL) {
			return UPNP_E_OUTOF_MEMORY;
		}
	}
	/* save */
	m->buf = temp_buf;
	m->capacity = alloc_len;
	return 0;
}

void membuffer_init(membuffer *m)
{
	assert(m != NULL);

	m->size_inc = MEMBUF_DEF_SIZE_INC;
	membuffer_initialize(m);
}

void membuffer_destroy(membuffer *m)
{
	if (m == NULL) {
		return;
	}

	free(m->buf);
	membuffer_init(m);
}

int membuffer_assign(membuffer *m, const void *buf, size_t buf_len)
{
	int return_code;

	assert(m != NULL);

	/* set value to null */
	if (buf == NULL) {
		membuffer_destroy(m);
		return 0;
	}
	/* alloc mem */
	return_code = membuffer_set_size(m, buf_len);
	if (return_code != 0)
		return return_code;
	/* copy */
	if (buf_len) {
		memcpy(m->buf, buf, buf_len);
		m->buf[buf_len] = 0;	/* null-terminate */
	}
	m->length = buf_len;

	return 0;
}

int membuffer_assign_str(membuffer *m, const char *c_str)
{
	return membuffer_assign(m, c_str, strlen(c_str));
}

int membuffer_append(membuffer *m, const void *buf, size_t buf_len)
{
	assert(m != NULL);

	return membuffer_insert(m, buf, buf_len, m->length);
}

int membuffer_append_str(membuffer *m, const char *c_str)
{
	return membuffer_insert(m, c_str, strlen(c_str), m->length);
}

int membuffer_insert(membuffer * m, const void *buf, size_t buf_len,
		     size_t index)
{
	int return_code;

	assert(m != NULL);

	if (index > m->length)
		return UPNP_E_OUTOF_BOUNDS;
	if (!buf || !buf_len) {
		return 0;
	}
	/* alloc mem */
	return_code = membuffer_set_size(m, m->length + buf_len);
	if (return_code) {
		return return_code;
	}
	/* insert data */
	/* move data to right of insertion point */
	memmove(m->buf + index + buf_len, m->buf + index, m->length - index);
	memcpy(m->buf + index, buf, buf_len);
	m->length += buf_len;
	/* null-terminate */
	m->buf[m->length] = 0;

	return 0;
}

void membuffer_delete(membuffer * m, size_t index, size_t num_bytes)
{
	int return_value;
	size_t new_length;
	size_t copy_len;

	assert(m != NULL);

	if (!m || !m->length)
		return;
	/* shrink count if it goes beyond buffer */
	if (index + num_bytes > m->length) {
		num_bytes = m->length - index;
		/* every thing at and after index purged */
		copy_len = (size_t)0;
	} else {
		/* calc num bytes after deleted string */
		copy_len = m->length - (index + num_bytes);
	}
	memmove(m->buf + index, m->buf + index + num_bytes, copy_len);
	new_length = m->length - num_bytes;
	/* trim buffer */
	return_value = membuffer_set_size(m, new_length);
	/* shrinking should always work */
	assert(return_value == 0);
	if (return_value != 0)
		return;

	/* don't modify until buffer is set */
	m->length = new_length;
	m->buf[new_length] = 0;
}

char *membuffer_detach(membuffer *m)
{
	char *buf;

	assert(m != NULL);

	buf = m->buf;

	/* free all */
	membuffer_initialize(m);

	return buf;
}

void membuffer_attach(membuffer *m, char *new_buf, size_t buf_len)
{
	assert(m != NULL);

	membuffer_destroy(m);
	m->buf = new_buf;
	m->length = buf_len;
	m->capacity = buf_len;
}
