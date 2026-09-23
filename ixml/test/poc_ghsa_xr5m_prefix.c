/* Regression test for GHSA-xr5m-v53v-jfq9 (namespace prefixes).
 *
 * Parser_ElementPrefixDefined() in ixml/src/ixmlparser.c resolves the prefix
 * of every prefixed start tag by walking the whole stack of open elements,
 * and every namespace declaration on each of them, until it finds the
 * prefix. n prefixed elements declared d levels up therefore cost
 * O(n * d) to parse. The fix must skip the elements that declare nothing,
 * and caps the namespace declarations in scope at 256 so that the remaining
 * walk is bounded; documents above the cap are rejected with IXML_SYNTAX_ERR.
 *
 * The timing control holds the same elements at the same depth, with the
 * prefix declared on the direct parent, where the walk stops at once. */

#include "poc_ghsa_xr5m.h"

#define NESTING_DEPTH 16000
#define NUM_ELEMENTS 16000
#define MAX_NAMESPACES 256

int main(void);

/* Parse n nested elements, each declaring a new prefix, around one child, and
 * return the result code. */
static int parse_with_namespaces(int n)
{
	/* "<a xmlns:p" + up to 5 digits + "=\"u\">" + "</a>" per level. */
	char *buf = malloc((size_t)n * 26 + 16);
	char *p = buf;
	IXML_Document *doc = NULL;
	int rc;
	int i;

	if (!buf)
		return IXML_INSUFFICIENT_MEMORY;
	for (i = 0; i < n; i++)
		p += sprintf(p, "<a xmlns:p%d=\"u\">", i);
	p = xr5m_repeat(p, "<b/>", 1);
	p = xr5m_repeat(p, "</a>", n);
	*p = '\0';

	rc = ixmlParseBufferEx(buf, &doc);
	ixmlDocument_free(doc);
	free(buf);
	return rc;
}

int main(void)
{
	size_t len = (size_t)NESTING_DEPTH * 7 + (size_t)NUM_ELEMENTS * 6 + 32;
	char *far = malloc(len + 1);
	char *near = malloc(len + 1);
	char *p;
	int failed = 0;
	int rc;

	if (!far || !near)
		return 1;

	/* <r xmlns:p="u"><a>...<a><p:x/>...</a>...</a></r> */
	p = xr5m_repeat(far, "<r xmlns:p=\"u\">", 1);
	p = xr5m_repeat(p, "<a>", NESTING_DEPTH);
	p = xr5m_repeat(p, "<p:x/>", NUM_ELEMENTS);
	p = xr5m_repeat(p, "</a>", NESTING_DEPTH);
	p = xr5m_repeat(p, "</r>", 1);
	*p = '\0';

	/* <r><a>...<a xmlns:p="u"><p:x/>...</a>...</a></r> */
	p = xr5m_repeat(near, "<r>", 1);
	p = xr5m_repeat(p, "<a>", NESTING_DEPTH - 1);
	p = xr5m_repeat(p, "<a xmlns:p=\"u\">", 1);
	p = xr5m_repeat(p, "<p:x/>", NUM_ELEMENTS);
	p = xr5m_repeat(p, "</a>", NESTING_DEPTH);
	p = xr5m_repeat(p, "</r>", 1);
	*p = '\0';

	failed |= xr5m_check_ratio("prefix declared far up", far, near);
	free(far);
	free(near);

	rc = parse_with_namespaces(MAX_NAMESPACES);
	printf("%d namespaces in scope: rc=%d (expected %d)\n",
		MAX_NAMESPACES,
		rc,
		IXML_SUCCESS);
	failed |= rc != IXML_SUCCESS;

	rc = parse_with_namespaces(MAX_NAMESPACES + 1);
	printf("%d namespaces in scope: rc=%d (expected %d)\n",
		MAX_NAMESPACES + 1,
		rc,
		IXML_SYNTAX_ERR);
	failed |= rc != IXML_SYNTAX_ERR;

	return failed;
}
