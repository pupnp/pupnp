

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


/*!
 * \brief Internal implementation of the class UpnpString.
 *
 * \internal
 */
struct SUpnpString
{
	/*! \brief Length of the string. */
	int m_length;
	/*! \brief Pointer to a dynamically allocated area that holds the NULL
	 * terminated string. */
	char *m_string;
};


UpnpString *UpnpString_new()
{
	// All bytes are zero, and so is the length of the string.
	struct SUpnpString *p = calloc(1, sizeof (struct SUpnpString));
	if (p == NULL) {
		goto error_handler1;
	}
#if 0
	p->m_length = 0;
#endif

	// This byte is zero, calloc does initialize it.
	p->m_string = calloc(1, 1);
	if (p->m_string == NULL) {
		goto error_handler2;
	}

	return (UpnpString *)p;

	//free(p->m_string);
error_handler2:
	free(p);
error_handler1:
	return NULL;
}


void UpnpString_delete(UpnpString *p)
{
	struct SUpnpString *q = (struct SUpnpString *)p;
		
	if (!q) return;

	q->m_length = 0;

	free(q->m_string);
	q->m_string = NULL;

	free(p);
}


UpnpString *UpnpString_dup(const UpnpString *p)
{
	struct SUpnpString *q = calloc(1, sizeof (struct SUpnpString));
	if (q == NULL) {
		goto error_handler1;
	}
	q->m_length = ((struct SUpnpString *)p)->m_length;
	q->m_string = strdup(((struct SUpnpString *)p)->m_string);
	if (q->m_string == NULL) {
		goto error_handler2;
	}

	return (UpnpString *)q;

	//free(q->m_string);
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


int UpnpString_get_Length(const UpnpString *p)
{
	return ((struct SUpnpString *)p)->m_length;
}


const char *UpnpString_get_String(const UpnpString *p)
{
	return ((struct SUpnpString *)p)->m_string;
}


void UpnpString_set_String(UpnpString *p, const char *s)
{
	free(((struct SUpnpString *)p)->m_string);
	((struct SUpnpString *)p)->m_length = strlen(s);
	((struct SUpnpString *)p)->m_string = strdup(s);
}


void UpnpString_set_StringN(UpnpString *p, const char *s, int n)
{
	free(((struct SUpnpString *)p)->m_string);
	((struct SUpnpString *)p)->m_length = n;
	((struct SUpnpString *)p)->m_string = (char *)malloc(n+1);
	strncpy(((struct SUpnpString *)p)->m_string, s, n);
	((struct SUpnpString *)p)->m_string[n] = 0;
}


void UpnpString_clear(UpnpString *p)
{
	((struct SUpnpString *)p)->m_length = 0;
	// No need to realloc now, will do later when needed
	((struct SUpnpString *)p)->m_string[0] = 0;
}

/* @} UpnpString */

