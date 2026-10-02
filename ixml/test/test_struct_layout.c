/*
 * Regression test: the node structures as an application sees them.
 *
 * IXML_Node has a member, ctag, that only exists when the library is built
 * with script support (IXML_HAVE_SCRIPTSUPPORT, the default). The macro was
 * given to the library sources on the compiler command line and nothing
 * defined it for whoever included ixml.h, so an application saw IXML_Node
 * without ctag while the library used it with ctag. Every member that comes
 * after the embedded IXML_Node in IXML_Element and in IXML_Attr was then
 * read at the wrong offset.
 *
 * This file is compiled as an application is: it includes ixml.h and
 * defines nothing itself.
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

int main(void)
{
	IXML_Document *doc;
	IXML_Element *elem;
	IXML_Attr *attr;

	CHECK(ixmlDocument_createDocumentEx(&doc) == IXML_SUCCESS);
	elem = ixmlDocument_createElement(doc, "P");
	CHECK(elem);
	CHECK(ixmlNode_appendChild((IXML_Node *)doc, (IXML_Node *)elem) ==
		IXML_SUCCESS);
	CHECK(ixmlElement_setAttribute(elem, "a", "v") == IXML_SUCCESS);
	attr = ixmlElement_getAttributeNode(elem, "a");
	CHECK(attr);

	/* The members after the embedded IXML_Node, read directly, must be
	 * what the library stored there. */
	CHECK(elem->tagName == ixmlElement_getTagName(elem));
	CHECK(elem->tagName && !strcmp(elem->tagName, "P"));
	CHECK(attr->ownerElement == elem);

	ixmlDocument_free(doc);
	puts("test_struct_layout: PASS");

	return 0;
}
