/*
 * Regression test: ixmlElement_removeAttributeNS() on an element that has
 * an attribute without a namespace URI.
 *
 * The parser stores a namespace declaration such as xmlns:p="urn:x" as an
 * attribute with a local name but a NULL namespaceURI. removeAttributeNS()
 * compared the namespace URI of every attribute with a local-name match
 * without checking for NULL, so it crashed on such a document.
 */

#include "ixml.h"

#include <stdio.h>

#define NS "urn:test:ns"

#define CHECK(cond) \
	do { \
		if (!(cond)) { \
			printf("FAIL line %d: %s\n", __LINE__, #cond); \
			fflush(stdout); \
			return 1; \
		} \
	} while (0)

static int test_parsed_xmlns_attr(void)
{
	IXML_Document *doc = NULL;
	IXML_Element *root;

	CHECK(ixmlParseBufferEx("<root xmlns:p=\"urn:x\"/>", &doc) ==
		IXML_SUCCESS);
	root = (IXML_Element *)ixmlNode_getFirstChild((IXML_Node *)doc);
	CHECK(root);

	/* The declaration has local name "p" but no namespace URI. */
	CHECK(ixmlElement_removeAttributeNS(root, "urn:x", "p") ==
		IXML_SUCCESS);
	CHECK(ixmlElement_removeAttributeNS(root,
		      "http://www.w3.org/2000/xmlns/",
		      "p") == IXML_SUCCESS);
	ixmlDocument_free(doc);

	return 0;
}

static int test_remove_still_works(void)
{
	IXML_Document *doc;
	IXML_Element *el;

	CHECK(ixmlDocument_createDocumentEx(&doc) == IXML_SUCCESS);
	el = ixmlDocument_createElement(doc, "E");
	CHECK(el);
	CHECK(ixmlElement_setAttributeNS(el, NS, "p:a", "1") == IXML_SUCCESS);
	CHECK(ixmlElement_getAttributeNS(el, NS, "a"));
	CHECK(ixmlElement_removeAttributeNS(el, NS, "a") == IXML_SUCCESS);
	CHECK(ixmlElement_getAttributeNS(el, NS, "a") == NULL);
	ixmlElement_free(el);
	ixmlDocument_free(doc);

	return 0;
}

int main(void)
{
	int err = 0;

	err |= test_parsed_xmlns_attr();
	err |= test_remove_still_works();
	if (!err)
		puts("test_attr_remove_ns: PASS");

	return err;
}
