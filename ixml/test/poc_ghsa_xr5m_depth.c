/* Regression test for GHSA-xr5m-v53v-jfq9 (nesting depth).
 *
 * Parser_hasDefaultNamespace() in ixml/src/ixmlparser.c walks the whole
 * stack of open elements on every unprefixed start tag, looking for a default
 * namespace. When there is none, the walk reaches the bottom of the stack
 * every time, so n nested elements cost O(n^2) to parse. The fix must find
 * the default namespace in effect in constant time.
 *
 * The control is the same document with a default namespace declared on the
 * root. Every open element then carries that namespace, the walk stops at the
 * first frame, and the cost stays linear. */

#include "poc_ghsa_xr5m.h"

#define NESTING_DEPTH 16000

int main(void);

int main(void)
{
	size_t len = (size_t)NESTING_DEPTH * 7 + 32;
	char *plain = malloc(len + 1);
	char *defns = malloc(len + 1);
	char *p;
	int failed;

	if (!plain || !defns)
		return 1;

	/* <r><a><a>...</a></a></r> */
	p = xr5m_repeat(plain, "<r>", 1);
	p = xr5m_repeat(p, "<a>", NESTING_DEPTH);
	p = xr5m_repeat(p, "</a>", NESTING_DEPTH);
	p = xr5m_repeat(p, "</r>", 1);
	*p = '\0';

	/* <r xmlns="u"><a><a>...</a></a></r> */
	p = xr5m_repeat(defns, "<r xmlns=\"u\">", 1);
	p = xr5m_repeat(p, "<a>", NESTING_DEPTH);
	p = xr5m_repeat(p, "</a>", NESTING_DEPTH);
	p = xr5m_repeat(p, "</r>", 1);
	*p = '\0';

	failed = xr5m_check_ratio("deep, no namespace", plain, defns);

	free(plain);
	free(defns);
	return failed;
}
