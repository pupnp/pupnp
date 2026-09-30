/*
 * Regression test: ixmlElement_removeAttributeNode().
 *
 * The function unlinked the attribute but did not clear its ownerElement,
 * so a removed attribute could never be attached to another element
 * (IXML_INUSE_ATTRIBUTE_ERR) and kept a dangling pointer to the old owner.
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
	IXML_Element *e1;
	IXML_Element *e2;
	IXML_Attr *a;
	IXML_Attr *removed = NULL;

	CHECK(ixmlDocument_createDocumentEx(&doc) == IXML_SUCCESS);
	e1 = ixmlDocument_createElement(doc, "E1");
	e2 = ixmlDocument_createElement(doc, "E2");
	CHECK(e1 && e2);
	CHECK(ixmlElement_setAttribute(e1, "a", "1") == IXML_SUCCESS);
	CHECK(ixmlElement_setAttribute(e1, "b", "2") == IXML_SUCCESS);
	a = ixmlElement_getAttributeNode(e1, "a");
	CHECK(a);

	CHECK(ixmlElement_removeAttributeNode(e1, a, &removed) == IXML_SUCCESS);
	CHECK(removed == a);
	CHECK(!ixmlElement_hasAttribute(e1, "a"));
	CHECK(ixmlElement_hasAttribute(e1, "b"));

	/* The removed attribute can now be attached to another element. */
	CHECK(ixmlElement_setAttributeNode(e2, a, NULL) == IXML_SUCCESS);
	CHECK(!strcmp(ixmlElement_getAttribute(e2, "a"), "1"));

	ixmlElement_free(e1);
	ixmlElement_free(e2);
	ixmlDocument_free(doc);
	puts("test_attr_remove_node: PASS");

	return 0;
}
