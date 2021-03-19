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

#include <cctype> /* for std::tolower on Windows */

#ifdef _WIN32
#define strcasecmp stricmp
#else
/* Other systems have strncasecmp */
#endif

#ifndef UPNP_USE_MSVCPP
#ifdef UPNP_USE_BCBPP
static size_t strnlen(const char *s, size_t n) { return strnlen_s(s, n); }
#else
/* VC has strnlen which is already included but with
 * (potentially) different linkage */
/* strnlen() is a GNU extension. */
#if !HAVE_STRNLEN
static size_t strnlen(const char *s, size_t n)
{
        const char *p = (const char *)memchr(s, 0, n);
        return p ? p - s : n;
}
#endif /* !HAVE_STRNLEN */
#endif /* UPNP_USE_BCBPP */
#endif /* _WIN32 */

/* strndup() is a GNU extension. */
#if !HAVE_STRNDUP || defined(_WIN32)
static char *strndup(const char *__string, size_t __n)
{
        size_t strsize = strnlen(__string, __n) + 1;
        char *newstr = (char *)malloc(strsize);
        if (newstr == NULL) {
                return NULL;
        }
        strncpy(newstr, __string, strsize);
        newstr[strsize - 1] = 0;

        return newstr;
}
#endif /* HAVE_STRNDUP && !defined(_WIN32) */

/*!
 * \brief Internal implementation of the class UpnpString.
 *
 * \internal
 */
#if 0
struct s_UpnpString
{
        /*! \brief Length of the string. */
        size_t m_length;
        /*! \brief Pointer to a dynamically allocated area that holds the NULL
         * terminated string. */
        char *m_string;
};
#endif

UpnpString *UpnpString_new() { return new UpnpString(); }

void UpnpString_delete(UpnpString *p) { delete p; }

UpnpString *UpnpString_dup(const UpnpString *p) { return new UpnpString(*p); }

void UpnpString_assign(UpnpString *p, const UpnpString *q) { *p = *q; }

size_t UpnpString_get_Length(const UpnpString *p) { return p->length(); }

const char *UpnpString_get_String(const UpnpString *p) { return p->c_str(); }

int UpnpString_set_String(UpnpString *p, const char *s)
{
        p->assign(s);

        return true;
}

int UpnpString_set_StringN(UpnpString *p, const char *s, size_t n)
{
        p->assign(s, n);

        return true;
}

void UpnpString_clear(UpnpString *p) { p->clear(); }

int UpnpString_cmp(const UpnpString *p, const UpnpString *q)
{
        return p->compare(*q);
}

static bool icase(unsigned char a, unsigned char b)
{
        return std::tolower(a) == std::tolower(b);
}

int UpnpString_casecmp(const UpnpString *p, const UpnpString *q)
{
        if (p->length() == q->length()) {
                return std::equal(q->begin(), q->end(), p->begin(), icase);
        } else {
                return false;
        }
}

/* @} UpnpString */
