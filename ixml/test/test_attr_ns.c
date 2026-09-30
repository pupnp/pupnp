/*
 * Regression test: adding a second namespaced attribute to an element.
 *
 * ixmlElement_setAttributeNS() and ixmlElement_setAttributeNodeNS() used a
 * lookup condition that expects a namespace URI and a local name, but was
 * handed an attribute node. As soon as the element already had one
 * namespaced attribute, adding another one dereferenced a NULL local name.
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

static int test_set_attribute_ns(void)
{
	IXML_Document *doc;
	IXML_Element *el;

	CHECK(ixmlDocument_createDocumentEx(&doc) == IXML_SUCCESS);
	el = ixmlDocument_createElement(doc, "E");
	CHECK(el);
	CHECK(ixmlElement_setAttributeNS(el, NS, "p:a", "1") == IXML_SUCCESS);
	CHECK(ixmlElement_setAttributeNS(el, NS, "p:b", "2") == IXML_SUCCESS);
	CHECK(ixmlElement_setAttributeNS(el, NS, "p:a", "3") == IXML_SUCCESS);
	CHECK(!strcmp(ixmlElement_getAttributeNS(el, NS, "a"), "3"));
	CHECK(!strcmp(ixmlElement_getAttributeNS(el, NS, "b"), "2"));
	ixmlElement_free(el);
	ixmlDocument_free(doc);

	return 0;
}

static int test_set_attribute_node_ns(void)
{
	IXML_Document *doc;
	IXML_Element *el;
	IXML_Attr *a1;
	IXML_Attr *a2;
	IXML_Attr *a3;
	IXML_Attr *old = NULL;

	CHECK(ixmlDocument_createDocumentEx(&doc) == IXML_SUCCESS);
	el = ixmlDocument_createElement(doc, "E");
	CHECK(el);
	a1 = ixmlDocument_createAttributeNS(doc, NS, "p:a");
	a2 = ixmlDocument_createAttributeNS(doc, NS, "p:b");
	a3 = ixmlDocument_createAttributeNS(doc, NS, "q:a");
	CHECK(a1 && a2 && a3);
	CHECK(ixmlElement_setAttributeNodeNS(el, a1, &old) == IXML_SUCCESS);
	CHECK(old == NULL);
	CHECK(ixmlElement_setAttributeNodeNS(el, a2, &old) == IXML_SUCCESS);
	CHECK(old == NULL);
	/* Same namespace and local name as a1: replaces it. */
	CHECK(ixmlElement_setAttributeNodeNS(el, a3, &old) == IXML_SUCCESS);
	CHECK(old == a1);
	ixmlAttr_free(old);
	CHECK(ixmlElement_getAttributeNodeNS(el, NS, "a") == a3);
	CHECK(ixmlElement_getAttributeNodeNS(el, NS, "b") == a2);
	ixmlElement_free(el);
	ixmlDocument_free(doc);

	return 0;
}

int main(void)
{
	int err = 0;

	err |= test_set_attribute_ns();
	err |= test_set_attribute_node_ns();
	if (!err)
		puts("test_attr_ns: PASS");

	return err;
}
