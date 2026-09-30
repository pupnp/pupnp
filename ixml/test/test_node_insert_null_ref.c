/*
 * Regression test: insertBefore() with a NULL reference node.
 *
 * The DOM defines insertBefore(p, newChild, NULL) as appendChild(p,
 * newChild). ixmlNode_insertBefore() checked that refChild was a child of p
 * before looking at whether it was NULL: the check asserted in a Debug
 * build and returned IXML_NOT_FOUND_ERR otherwise, so the append branch was
 * never reached.
 */

#include "ixml.h"

#include <stdio.h>

#define CHECK(cond) \
	do { \
		if (!(cond)) { \
			printf("FAIL line %d: %s\n", __LINE__, #cond); \
			fflush(stdout); \
			return 1; \
		} \
	} while (0)

int main(void)
{
	IXML_Document *doc;
	IXML_Node *p;
	IXML_Node *a;
	IXML_Node *y;

	CHECK(ixmlDocument_createDocumentEx(&doc) == IXML_SUCCESS);
	p = (IXML_Node *)ixmlDocument_createElement(doc, "p");
	a = (IXML_Node *)ixmlDocument_createElement(doc, "a");
	y = (IXML_Node *)ixmlDocument_createElement(doc, "y");
	CHECK(p && a && y);
	CHECK(ixmlNode_appendChild((IXML_Node *)doc, p) == IXML_SUCCESS);

	/* Into an empty parent, then after an existing child. */
	CHECK(ixmlNode_insertBefore(p, a, NULL) == IXML_SUCCESS);
	CHECK(p->firstChild == a);
	CHECK(a->parentNode == p);
	CHECK(ixmlNode_insertBefore(p, y, NULL) == IXML_SUCCESS);
	CHECK(p->firstChild == a);
	CHECK(a->nextSibling == y);
	CHECK(y->prevSibling == a);
	CHECK(y->nextSibling == NULL);
	CHECK(y->parentNode == p);

	ixmlDocument_free(doc);
	puts("test_node_insert_null_ref: PASS");

	return 0;
}
