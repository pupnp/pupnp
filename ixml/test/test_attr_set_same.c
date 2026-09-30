/*
 * Regression test: setting an attribute node that is already attached to
 * the element.
 *
 * ixmlElement_setAttributeNode() looks for an attribute with the same name
 * and replaces it. When newAttr itself was that attribute, the replace
 * code unlinked it from its own neighbours: the attributes after it were
 * cut off the list and, without a rtAttr, the still-linked attribute was
 * freed.
 */

#include "ixml.h"

#include <stdio.h>
#include <string.h>

#define NS "urn:test:ns"

#define CHECK(cond) \
	do { \
		if (!(cond)) { \
			printf("FAIL line %d: %s\n", __LINE__, #cond); \
			fflush(stdout); \
			return 1; \
		} \
	} while (0)

static int check_abc(IXML_Element *el)
{
	CHECK(!strcmp(ixmlElement_getAttribute(el, "a"), "1"));
	CHECK(!strcmp(ixmlElement_getAttribute(el, "b"), "2"));
	CHECK(!strcmp(ixmlElement_getAttribute(el, "c"), "3"));

	return 0;
}

/* Returns an element E with attributes a=1, b=2, c=3. */
static int build(IXML_Document **doc, IXML_Element **el)
{
	CHECK(ixmlDocument_createDocumentEx(doc) == IXML_SUCCESS);
	*el = ixmlDocument_createElement(*doc, "E");
	CHECK(*el);
	CHECK(ixmlElement_setAttribute(*el, "a", "1") == IXML_SUCCESS);
	CHECK(ixmlElement_setAttribute(*el, "b", "2") == IXML_SUCCESS);
	CHECK(ixmlElement_setAttribute(*el, "c", "3") == IXML_SUCCESS);

	return 0;
}

static int test_same_attr_with_rtattr(void)
{
	IXML_Document *doc;
	IXML_Element *el;
	IXML_Attr *b;
	IXML_Attr *old;

	if (build(&doc, &el))
		return 1;
	b = ixmlElement_getAttributeNode(el, "b");
	CHECK(b);
	old = b; /* non-NULL, must be overwritten */
	CHECK(ixmlElement_setAttributeNode(el, b, &old) == IXML_SUCCESS);
	CHECK(old == NULL);
	if (check_abc(el))
		return 1;
	CHECK(ixmlElement_getAttributeNode(el, "b") == b);
	ixmlElement_free(el);
	ixmlDocument_free(doc);

	return 0;
}

static int test_same_attr_without_rtattr(void)
{
	IXML_Document *doc;
	IXML_Element *el;
	IXML_Attr *a;

	if (build(&doc, &el))
		return 1;
	/* First attribute, and no rtAttr: it must not be freed. */
	a = ixmlElement_getAttributeNode(el, "a");
	CHECK(a);
	CHECK(ixmlElement_setAttributeNode(el, a, NULL) == IXML_SUCCESS);
	if (check_abc(el))
		return 1;
	CHECK(ixmlElement_getAttributeNode(el, "a") == a);
	ixmlElement_free(el);
	ixmlDocument_free(doc);

	return 0;
}

static int test_same_attr_ns(void)
{
	IXML_Document *doc;
	IXML_Element *el;
	IXML_Attr *p;
	IXML_Attr *q;
	IXML_Attr *old = NULL;

	CHECK(ixmlDocument_createDocumentEx(&doc) == IXML_SUCCESS);
	el = ixmlDocument_createElement(doc, "E");
	p = ixmlDocument_createAttributeNS(doc, NS, "p:x");
	q = ixmlDocument_createAttributeNS(doc, NS, "p:y");
	CHECK(el && p && q);
	CHECK(ixmlElement_setAttributeNodeNS(el, p, &old) == IXML_SUCCESS);
	CHECK(ixmlElement_setAttributeNodeNS(el, q, &old) == IXML_SUCCESS);
	CHECK(ixmlElement_setAttributeNodeNS(el, p, &old) == IXML_SUCCESS);
	CHECK(old == NULL);
	CHECK(ixmlElement_getAttributeNodeNS(el, NS, "x") == p);
	CHECK(ixmlElement_getAttributeNodeNS(el, NS, "y") == q);
	ixmlElement_free(el);
	ixmlDocument_free(doc);

	return 0;
}

int main(void)
{
	int err = 0;

	err |= test_same_attr_with_rtattr();
	err |= test_same_attr_without_rtattr();
	err |= test_same_attr_ns();
	if (!err)
		puts("test_attr_set_same: PASS");

	return err;
}
