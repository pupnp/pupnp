/* Regression test for GHSA-xr5m-v53v-jfq9 (attribute lists).
 *
 * Every attribute the parser reads costs three walks of the element's
 * attribute list: the duplicate check in isDuplicateAttribute()
 * (ixml/src/ixmlparser.c), the duplicate check in get_attribute_node() and
 * the walk to the end of the list in ixmlElement_setAttributeNode_common()
 * (ixml/src/element.c). n attributes on one element therefore cost O(n^2):
 * 7 000 of them in a 64 KB SOAP body take about 0.17 s.
 *
 * The duplicate checks are required for well-formedness, so the fix caps the
 * number of attributes per element at 256 and rejects documents above it with
 * IXML_SYNTAX_ERR. This test checks both sides of the cap. */

#include "ixml.h"

#include <stdio.h>
#include <stdlib.h>

#define MAX_ATTRIBUTES 256

int main(void);

/* Parse <r a0="" a1="" ... a(n-1)=""/> and return the result code. */
static int parse_with_attributes(int n)
{
	/* " a" + up to 5 digits + "=\"\"" per attribute. */
	char *buf = malloc((size_t)n * 10 + 8);
	char *p = buf;
	IXML_Document *doc = NULL;
	int rc;
	int i;

	if (!buf)
		return IXML_INSUFFICIENT_MEMORY;
	p += sprintf(p, "<r");
	for (i = 0; i < n; i++)
		p += sprintf(p, " a%d=\"\"", i);
	sprintf(p, "/>");

	rc = ixmlParseBufferEx(buf, &doc);
	ixmlDocument_free(doc);
	free(buf);
	return rc;
}

int main(void)
{
	int failed = 0;
	int rc;

	rc = parse_with_attributes(MAX_ATTRIBUTES);
	printf("%d attributes: rc=%d (expected %d)\n",
		MAX_ATTRIBUTES,
		rc,
		IXML_SUCCESS);
	failed |= rc != IXML_SUCCESS;

	rc = parse_with_attributes(MAX_ATTRIBUTES + 1);
	printf("%d attributes: rc=%d (expected %d)\n",
		MAX_ATTRIBUTES + 1,
		rc,
		IXML_SYNTAX_ERR);
	failed |= rc != IXML_SYNTAX_ERR;

	/* The reporter's shape: as many attributes as fit in 64 KB. */
	rc = parse_with_attributes(7000);
	printf("7000 attributes: rc=%d (expected %d)\n", rc, IXML_SYNTAX_ERR);
	failed |= rc != IXML_SYNTAX_ERR;

	return failed;
}
