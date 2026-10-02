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

/* The attribute list of el must be exactly first, second. */
static int check_two(IXML_Element *el, IXML_Attr *first, IXML_Attr *second)
{
	CHECK(el->n.firstAttr == &first->n);
	CHECK(first->n.prevSibling == NULL);
	CHECK(first->n.nextSibling == &second->n);
	CHECK(second->n.prevSibling == &first->n);
	CHECK(second->n.nextSibling == NULL);

	return 0;
}

/*
 * An attribute without a namespace is not found by the namespace search of
 * ixmlElement_setAttributeNodeNS(), so setting it a second time appended it
 * to the list again and left a cycle in it.
 */
static int test_same_attr_ns_without_namespace(void)
{
	IXML_Document *doc;
	IXML_Element *el;
	IXML_Attr *a;
	IXML_Attr *b;
	IXML_Attr *old = NULL;

	CHECK(ixmlDocument_createDocumentEx(&doc) == IXML_SUCCESS);
	el = ixmlDocument_createElement(doc, "E");
	a = ixmlDocument_createAttribute(doc, "a");
	b = ixmlDocument_createAttribute(doc, "b");
	CHECK(el && a && b);
	CHECK(ixmlElement_setAttributeNodeNS(el, a, &old) == IXML_SUCCESS);
	CHECK(ixmlElement_setAttributeNodeNS(el, b, &old) == IXML_SUCCESS);
	old = a; /* non-NULL, must be overwritten */
	CHECK(ixmlElement_setAttributeNodeNS(el, a, &old) == IXML_SUCCESS);
	CHECK(old == NULL);
	if (check_two(el, a, b))
		return 1;
	ixmlElement_free(el);
	ixmlDocument_free(doc);

	return 0;
}

/*
 * The name search of ixmlElement_setAttributeNode() can find another
 * attribute of the element that has the same name as newAttr. That one was
 * replaced by newAttr while newAttr was still linked, which made newAttr its
 * own sibling.
 */
static int test_same_attr_other_with_same_name(void)
{
	IXML_Document *doc;
	IXML_Element *el;
	IXML_Attr *first;
	IXML_Attr *second;
	IXML_Attr *old = NULL;

	CHECK(ixmlDocument_createDocumentEx(&doc) == IXML_SUCCESS);
	el = ixmlDocument_createElement(doc, "E");
	CHECK(el);
	CHECK(ixmlElement_setAttributeNS(el, NS, "a", "1") == IXML_SUCCESS);
	first = ixmlElement_getAttributeNodeNS(el, NS, "a");
	second = ixmlDocument_createAttribute(doc, "a");
	CHECK(first && second);
	CHECK(ixmlElement_setAttributeNodeNS(el, second, &old) == IXML_SUCCESS);
	CHECK(old == NULL);
	old = second; /* non-NULL, must be overwritten */
	CHECK(ixmlElement_setAttributeNode(el, second, &old) == IXML_SUCCESS);
	CHECK(old == NULL);
	if (check_two(el, first, second))
		return 1;
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
	err |= test_same_attr_ns_without_namespace();
	err |= test_same_attr_other_with_same_name();
	if (!err)
		puts("test_attr_set_same: PASS");

	return err;
}
