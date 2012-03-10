/*!
 * \addtogroup UpnpString
 *
 * Due to its heavy use, this class is coded for efficiency, not for beauty.
 * Do not use this as example to other classes. Please take a look at any
 * other one.
 *
 * \todo Always alloc a minimum size like 64 bytes or so and when shrinking
 * do not perform a new memory allocation.
 *
 * @{
 *
 * \file
 *
 * \brief UpnpString object implementation.
 */

#include "config.h"

#include "UpnpString.h"

#include <stdlib.h> /* for calloc(), free() */
#include <string.h> /* for strlen(), strdup() */

#ifdef WIN32
	#define strcasecmp stricmp
#else
	/* Other systems have strncasecmp */
#endif

#ifndef UPNP_USE_MSVCPP
	/* VC has strnlen which is already included but with (potentially) different linkage */
	/* strnlen() is a GNU extension. */
	#if HAVE_STRNLEN
		extern size_t strnlen(const char *s, size_t maxlen);
	#else /* HAVE_STRNLEN */
		static size_t strnlen(const char *s, size_t n)
		{
			const char *p = (const char *)memchr(s, 0, n);
			return p ? p - s : n;
		}
	#endif /* HAVE_STRNLEN */
#endif /* WIN32 */

/* strndup() is a GNU extension. */
#if HAVE_STRNDUP && !defined(WIN32)
	extern char *strndup(__const char *__string, size_t __n);
#else /* HAVE_STRNDUP && !defined(WIN32) */
	static char *strndup(const char *__string, size_t __n)
	{
		size_t strsize = strnlen(__string, __n);
		char *newstr = (char *)malloc(strsize + 1);
		if (newstr == NULL)
			return NULL;

		strncpy(newstr, __string, strsize);
		newstr[strsize] = 0;

		return newstr;
	}
#endif /* HAVE_STRNDUP && !defined(WIN32) */

/*!
 * \brief Internal implementation of the class UpnpString.
 *
 * \internal
 */
struct SUpnpString
{
	/*! \brief Length of the string. */
	size_t m_length;
	/*! \brief Pointer to a dynamically allocated area that holds the NULL
	 * terminated string. */
	char *m_string;
};

UpnpString *UpnpString_new()
{
	/* All bytes are zero, and so is the length of the string. */
	struct SUpnpString *p = calloc((size_t)1, sizeof (struct SUpnpString));
	if (p == NULL) {
		goto error_handler1;
	}
#if 0
	p->m_length = 0;
#endif

	/* This byte is zero, calloc does initialize it. */
	p->m_string = calloc((size_t)1, (size_t)1);
	if (p->m_string == NULL) {
		goto error_handler2;
	}

	return (UpnpString *)p;

	/*free(p->m_string); */
error_handler2:
	free(p);
error_handler1:
	return NULL;
}

void UpnpString_delete(UpnpString *p)
{
	struct SUpnpString *q = (struct SUpnpString *)p;
		
	if (!q) return;

	q->m_length = (size_t)0;

	free(q->m_string);
	q->m_string = NULL;

	free(p);
}

UpnpString *UpnpString_dup(const UpnpString *p)
{
	struct SUpnpString *q = calloc((size_t)1, sizeof (struct SUpnpString));
	if (q == NULL) {
		goto error_handler1;
	}
	q->m_length = ((struct SUpnpString *)p)->m_length;
	q->m_string = strdup(((struct SUpnpString *)p)->m_string);
	if (q->m_string == NULL) {
		goto error_handler2;
	}

	return (UpnpString *)q;

	/*free(q->m_string); */
error_handler2:
	free(q);
error_handler1:
	return NULL;
}

void UpnpString_assign(UpnpString *p, const UpnpString *q)
{
	if (p != q) {
		UpnpString_set_String(p, UpnpString_get_String(q));
	}
}

size_t UpnpString_get_Length(const UpnpString *p)
{
	return ((struct SUpnpString *)p)->m_length;
}

void UpnpString_set_Length(UpnpString *p, size_t n)
{
	if (((struct SUpnpString *)p)->m_length > n) {
		((struct SUpnpString *)p)->m_length = n;
		/* No need to realloc now, will do later when needed. */
		((struct SUpnpString *)p)->m_string[n] = 0;
	}
}

const char *UpnpString_get_String(const UpnpString *p)
{
	return ((struct SUpnpString *)p)->m_string;
}

int UpnpString_set_String(UpnpString *p, const char *s)
{
	char *q = strdup(s);
	if (!q) goto error_handler1;
	free(((struct SUpnpString *)p)->m_string);
	((struct SUpnpString *)p)->m_length = strlen(q);
	((struct SUpnpString *)p)->m_string = q;

error_handler1:
	return q != NULL;
}

int UpnpString_set_StringN(UpnpString *p, const char *s, size_t n)
{
	char *q = strndup(s, n);
	if (!q) goto error_handler1;
	free(((struct SUpnpString *)p)->m_string);
	((struct SUpnpString *)p)->m_length = strlen(q);
	((struct SUpnpString *)p)->m_string = q;

error_handler1:
	return q != NULL;
}

void UpnpString_clear(UpnpString *p)
{
	((struct SUpnpString *)p)->m_length = (size_t)0;
	/* No need to realloc now, will do later when needed. */
	((struct SUpnpString *)p)->m_string[0] = 0;
}

int UpnpString_cmp(UpnpString *p, UpnpString *q)
{
	const char *cp = UpnpString_get_String(p);
	const char *cq = UpnpString_get_String(q);

	return strcmp(cp, cq);
}

int UpnpString_casecmp(UpnpString *p, UpnpString *q)
{
	const char *cp = UpnpString_get_String(p);
	const char *cq = UpnpString_get_String(q);

	return strcasecmp(cp, cq);
}

/* @} UpnpString */
