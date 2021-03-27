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
#include <locale>

UpnpString *UpnpString_new() { return new UpnpString(); }

void UpnpString_delete(UpnpString *p) { delete p; }

UpnpString *UpnpString_dup(const UpnpString *p) { return new UpnpString(*p); }

void UpnpString_assign(UpnpString *p, const UpnpString *q) { *p = *q; }

size_t UpnpString_get_Length(const UpnpString *p) { return p->length(); }

const char *UpnpString_get_String(const UpnpString *p) { return p->c_str(); }

int UpnpString_set_String(UpnpString *p, const char *s)
{
        if (s) {
                p->assign(s);
        } else {
                p->assign("");
        }

        return true;
}

int UpnpString_set_StringN(UpnpString *p, const char *s, size_t n)
{
        if (s) {
                p->assign(s, n);
        } else {
                p->assign("", n);
        }

        return true;
}

void UpnpString_clear(UpnpString *p) { p->clear(); }

int UpnpString_cmp(const UpnpString *p, const UpnpString *q)
{
        return p->compare(*q);
}

int UpnpString_casecmp(const UpnpString *p, const UpnpString *q)
{
        const std::locale loc = std::locale();
        const char *a = UpnpString_get_String(p);
        const char *b = UpnpString_get_String(q);
        while (*a || *b) {
                char c = std::toupper(*a++, loc);
                char d = std::toupper(*b++, loc);
                if (c < d) {
                        return -1;
                }
                if (c > d) {
                        return 1;
                }
        }
        return 0;
}

/* @} UpnpString */
