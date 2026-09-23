/* Regression test for GHSA-xr5m-v53v-jfq9 (sibling lists).
 *
 * ixmlNode_appendChild() in ixml/src/node.c walks the whole child list to
 * find its end before every append, and the parser appends every node it
 * reads through it. A flat list of n siblings therefore costs O(n^2) to
 * parse: 16 000 empty elements in a 64 KB SOAP body take about 0.25 s, and
 * alternating text and elements about 2 s. The fix must append in constant
 * time.
 *
 * The control holds the same elements in groups of 64, so no child list is
 * longer than 64 and the walk cannot grow. */

#include "poc_ghsa_xr5m.h"

#define NUM_ELEMENTS 16000
#define GROUP_SIZE 64

int main(void);

int main(void)
{
	/* The largest document is the control: <g> and </g> per group. */
	size_t len = (size_t)NUM_ELEMENTS * 4 +
		     (size_t)(NUM_ELEMENTS / GROUP_SIZE) * 7 + 8;
	char *flat = malloc(len + 1);
	char *mixed = malloc(len + 1);
	char *grouped = malloc(len + 1);
	char *p;
	int i;
	int failed = 0;

	if (!flat || !mixed || !grouped)
		return 1;

	/* <r><a/><a/>...</r> */
	p = xr5m_repeat(flat, "<r>", 1);
	p = xr5m_repeat(p, "<a/>", NUM_ELEMENTS);
	p = xr5m_repeat(p, "</r>", 1);
	*p = '\0';

	/* <r>x<a/>x<a/>...</r>: the same node count as flat, half of them text
	 * nodes, which go through ixmlNode_appendChild() too. */
	p = xr5m_repeat(mixed, "<r>", 1);
	p = xr5m_repeat(p, "x<a/>", NUM_ELEMENTS / 2);
	p = xr5m_repeat(p, "</r>", 1);
	*p = '\0';

	/* <r><g><a/>...</g><g><a/>...</g>...</r> */
	p = xr5m_repeat(grouped, "<r>", 1);
	for (i = 0; i < NUM_ELEMENTS / GROUP_SIZE; i++) {
		p = xr5m_repeat(p, "<g>", 1);
		p = xr5m_repeat(p, "<a/>", GROUP_SIZE);
		p = xr5m_repeat(p, "</g>", 1);
	}
	p = xr5m_repeat(p, "</r>", 1);
	*p = '\0';

	failed |= xr5m_check_ratio("flat siblings", flat, grouped);
	failed |= xr5m_check_ratio("text/element siblings", mixed, grouped);

	free(flat);
	free(mixed);
	free(grouped);
	return failed;
}
