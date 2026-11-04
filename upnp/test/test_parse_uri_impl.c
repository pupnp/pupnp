/*
 * This wrapper intentionally builds uri.c into the test binary.
 *
 * Why this exists:
 * - parse_uri() is internal (declared in upnp/src/inc/uri.h), not part of
 *   the public Upnp* API that unit tests usually exercise via libupnp.
 * - Shared-library builds do not reliably export parse_uri(), so a test that
 *   only links with libupnp may not resolve this symbol.
 * - Compiling uri.c through a test-local translation unit avoids autotools
 *   object/dependency naming conflicts while keeping the library API surface
 *   unchanged.
 */

/*
 * Regression test for PR #666: the <ctype.h> functions take an int that must
 * be representable as unsigned char or equal EOF (C11 7.4). uri.c passed a
 * plain char, so a URL byte from 0x80 to 0xFF became a negative int where
 * char is signed. glibc tolerates that, so the wrappers below count every
 * out-of-range argument instead; test_parse_uri.c fails when the count is
 * not zero. <ctype.h> is included first so that its include guard keeps
 * uri.c from redefining the names.
 */
#include <ctype.h>
#include <limits.h>
#include <stdio.h>

int ctype_bad_args = 0;

static int ctype_arg(int c)
{
	if (c != EOF && (c < 0 || c > UCHAR_MAX)) {
		ctype_bad_args++;
		return EOF;
	}
	return c;
}

static int checked_isalnum(int c) { return isalnum(ctype_arg(c)); }
static int checked_isalpha(int c) { return isalpha(ctype_arg(c)); }
static int checked_isdigit(int c) { return isdigit(ctype_arg(c)); }
static int checked_isxdigit(int c) { return isxdigit(ctype_arg(c)); }

#undef isalnum
#undef isalpha
#undef isdigit
#undef isxdigit
#define isalnum checked_isalnum
#define isalpha checked_isalpha
#define isdigit checked_isdigit
#define isxdigit checked_isxdigit

#include "../src/genlib/net/uri/uri.c"
