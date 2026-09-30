/*
 * Regression tests: ixmlElement_removeAttributeNode().
 *
 * - The function unlinked the attribute but did not clear its ownerElement,
 *   so a removed attribute could never be attached to another element
 *   (IXML_INUSE_ATTRIBUTE_ERR) and kept a dangling pointer to the old owner.
 * - It stored the removed attribute through rtAttr without checking it, so
 *   a NULL rtAttr crashed.
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

static int test_move_removed_attr(void)
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

	return 0;
}

static int test_null_rtattr(void)
{
	IXML_Document *doc;
	IXML_Element *e;
	IXML_Attr *a;

	CHECK(ixmlDocument_createDocumentEx(&doc) == IXML_SUCCESS);
	e = ixmlDocument_createElement(doc, "E");
	CHECK(e);
	CHECK(ixmlElement_setAttribute(e, "a", "1") == IXML_SUCCESS);
	a = ixmlElement_getAttributeNode(e, "a");
	CHECK(a);

	/* The caller gets no way to own the attribute: refuse, and leave the
	 * attribute where it is. */
	CHECK(ixmlElement_removeAttributeNode(e, a, NULL) ==
		IXML_INVALID_PARAMETER);
	CHECK(ixmlElement_getAttributeNode(e, "a") == a);
	CHECK(!strcmp(ixmlElement_getAttribute(e, "a"), "1"));

	ixmlElement_free(e);
	ixmlDocument_free(doc);

	return 0;
}

int main(void)
{
	int err = 0;

	err |= test_move_removed_attr();
	err |= test_null_rtattr();
	if (!err)
		puts("test_attr_remove_node: PASS");

	return err;
}
