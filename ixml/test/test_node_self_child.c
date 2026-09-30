/*
 * Regression test: a node cannot become a child of itself.
 *
 * ixmlNode_isAncestor() is strict, so appendChild(e, e), insertBefore(e, e,
 * ref) and replaceChild(e, e, old) were not rejected. appendChild(e, e)
 * returned success and made e its own parent and first child, a cycle that
 * ixmlNode_free() and the printing code loop on.
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
	IXML_Node *e;
	IXML_Node *child;
	IXML_Node *removed = NULL;

	CHECK(ixmlDocument_createDocumentEx(&doc) == IXML_SUCCESS);
	e = (IXML_Node *)ixmlDocument_createElement(doc, "E");
	child = (IXML_Node *)ixmlDocument_createElement(doc, "C");
	CHECK(e && child);
	CHECK(ixmlNode_appendChild((IXML_Node *)doc, e) == IXML_SUCCESS);
	CHECK(ixmlNode_appendChild(e, child) == IXML_SUCCESS);

	CHECK(ixmlNode_appendChild(e, e) == IXML_HIERARCHY_REQUEST_ERR);
	CHECK(ixmlNode_insertBefore(e, e, child) == IXML_HIERARCHY_REQUEST_ERR);
	CHECK(ixmlNode_replaceChild(e, e, child, &removed) ==
		IXML_HIERARCHY_REQUEST_ERR);
	CHECK(removed == NULL);

	/* The tree is unchanged. */
	CHECK(e->parentNode == (IXML_Node *)doc);
	CHECK(e->firstChild == child);
	CHECK(child->parentNode == e);
	CHECK(child->nextSibling == NULL);
	CHECK(e->nextSibling == NULL);
	CHECK(e->prevSibling == NULL);

	ixmlDocument_free(doc);
	puts("test_node_self_child: PASS");

	return 0;
}
