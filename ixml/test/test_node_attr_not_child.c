/*
 * Regression test: an element's attributes are not its children.
 *
 * Attributes have the element as parentNode, and ixmlNode_isParent() only
 * compared parentNode pointers. removeChild(), insertBefore() and
 * replaceChild() therefore treated an attribute as a child of its element:
 * removeChild(e, attr) succeeded, unlinked the attribute from the child
 * list it is not in and orphaned the element's other attributes.
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
	IXML_Element *el;
	IXML_Node *attr;
	IXML_Node *text;
	IXML_Node *removed = NULL;

	CHECK(ixmlDocument_createDocumentEx(&doc) == IXML_SUCCESS);
	el = ixmlDocument_createElement(doc, "E");
	CHECK(el);
	CHECK(ixmlNode_appendChild((IXML_Node *)doc, (IXML_Node *)el) ==
		IXML_SUCCESS);
	CHECK(ixmlElement_setAttribute(el, "a", "1") == IXML_SUCCESS);
	CHECK(ixmlElement_setAttribute(el, "b", "2") == IXML_SUCCESS);
	attr = (IXML_Node *)ixmlElement_getAttributeNode(el, "a");
	CHECK(attr);
	text = ixmlDocument_createTextNode(doc, "t");
	CHECK(text);

	/* removeChild on an attribute must be refused. */
	CHECK(ixmlNode_removeChild((IXML_Node *)el, attr, &removed) ==
		IXML_NOT_FOUND_ERR);
	CHECK(removed == NULL);
	/* insertBefore/replaceChild with an attribute as reference too. */
	CHECK(ixmlNode_insertBefore((IXML_Node *)el, text, attr) ==
		IXML_NOT_FOUND_ERR);
	CHECK(ixmlNode_replaceChild((IXML_Node *)el, text, attr, &removed) ==
		IXML_NOT_FOUND_ERR);
	CHECK(removed == NULL);

	/* Both attributes and the child list are untouched. */
	CHECK(ixmlElement_hasAttribute(el, "a"));
	CHECK(ixmlElement_hasAttribute(el, "b"));
	CHECK(ixmlNode_getFirstChild((IXML_Node *)el) == NULL);
	CHECK(text->parentNode == NULL);

	ixmlNode_free(text);
	ixmlDocument_free(doc);
	puts("test_node_attr_not_child: PASS");

	return 0;
}
