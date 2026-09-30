/*
 * Regression test: cloning a text or CDATA node.
 *
 * ixmlNode_cloneNode() handed text and CDATA nodes to the tree walker, which
 * keeps going through the nextSibling chain. The clone of a text node that
 * had following siblings came back with clones of all of them attached.
 */

#include "ixml.h"

#include <stdio.h>
#include <string.h>

#define CHECK(cond) \
	do { \
		if (!(cond)) { \
			printf("FAIL line %d: %s\n", __LINE__, #cond); \
			fflush(stdout); \
			return 1; \
		} \
	} while (0)

static int check_single_clone(IXML_Node *src, int deep, const char *value)
{
	IXML_Node *clone = ixmlNode_cloneNode(src, deep);

	CHECK(clone);
	CHECK(clone->nodeType == src->nodeType);
	CHECK(!strcmp(ixmlNode_getNodeValue(clone), value));
	CHECK(clone->parentNode == NULL);
	CHECK(clone->prevSibling == NULL);
	CHECK(clone->nextSibling == NULL);
	ixmlNode_free(clone);

	return 0;
}

int main(void)
{
	IXML_Document *doc;
	IXML_Element *parent;
	IXML_Node *t1;
	IXML_Node *t2;
	IXML_Node *cd;
	IXML_Node *t3;

	CHECK(ixmlDocument_createDocumentEx(&doc) == IXML_SUCCESS);
	parent = ixmlDocument_createElement(doc, "P");
	CHECK(parent);
	CHECK(ixmlNode_appendChild((IXML_Node *)doc, (IXML_Node *)parent) ==
		IXML_SUCCESS);
	/* P -> { "one", "two", <![CDATA[cdata]]>, "three" } */
	t1 = ixmlDocument_createTextNode(doc, "one");
	t2 = ixmlDocument_createTextNode(doc, "two");
	cd = (IXML_Node *)ixmlDocument_createCDATASection(doc, "cdata");
	t3 = ixmlDocument_createTextNode(doc, "three");
	CHECK(t1 && t2 && cd && t3);
	CHECK(ixmlNode_appendChild((IXML_Node *)parent, t1) == IXML_SUCCESS);
	CHECK(ixmlNode_appendChild((IXML_Node *)parent, t2) == IXML_SUCCESS);
	CHECK(ixmlNode_appendChild((IXML_Node *)parent, cd) == IXML_SUCCESS);
	CHECK(ixmlNode_appendChild((IXML_Node *)parent, t3) == IXML_SUCCESS);

	/* First, middle and last text nodes, shallow and deep. */
	if (check_single_clone(t1, 0, "one") ||
		check_single_clone(t1, 1, "one") ||
		check_single_clone(t2, 0, "two") ||
		check_single_clone(t3, 1, "three"))
		return 1;
	/* A CDATA section with siblings on both sides. */
	if (check_single_clone(cd, 0, "cdata") ||
		check_single_clone(cd, 1, "cdata"))
		return 1;

	ixmlDocument_free(doc);
	puts("test_node_clone_text: PASS");

	return 0;
}
