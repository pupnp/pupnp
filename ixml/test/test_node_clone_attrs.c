/*
 * Regression test: cloning an element whose attributes were not all added
 * the same way.
 *
 * Attributes copied by ixmlNode_cloneNode() had a NULL parentNode, while
 * attributes added with ixmlElement_setAttribute() have the element as
 * parentNode. ixmlNode_cloneNode() walked the attribute list with the tree
 * walker, which uses the first attribute's parentNode as its fence, so an
 * element with both kinds of attributes made it run off the list and crash.
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

static int test_clone_of_clone_with_new_attr(void)
{
	IXML_Document *doc;
	IXML_Element *root;
	IXML_Element *e0;
	IXML_Node *clone1;
	IXML_Node *clone2;

	CHECK(ixmlDocument_createDocumentEx(&doc) == IXML_SUCCESS);
	root = ixmlDocument_createElement(doc, "root");
	e0 = ixmlDocument_createElement(doc, "e0");
	CHECK(root && e0);
	CHECK(ixmlNode_appendChild((IXML_Node *)doc, (IXML_Node *)root) ==
		IXML_SUCCESS);
	CHECK(ixmlElement_setAttribute(e0, "a", "1") == IXML_SUCCESS);

	/* A clone carries attribute "a" copied from e0. */
	clone1 = ixmlNode_cloneNode((IXML_Node *)e0, 1);
	CHECK(clone1);
	CHECK(ixmlNode_appendChild((IXML_Node *)root, clone1) == IXML_SUCCESS);
	/* Now the clone has a copied attribute and one set on it. */
	CHECK(ixmlElement_setAttribute((IXML_Element *)clone1, "b", "2") ==
		IXML_SUCCESS);

	clone2 = ixmlNode_cloneNode(clone1, 1);
	CHECK(clone2);
	CHECK(!strcmp(
		ixmlElement_getAttribute((IXML_Element *)clone2, "a"), "1"));
	CHECK(!strcmp(
		ixmlElement_getAttribute((IXML_Element *)clone2, "b"), "2"));
	ixmlNode_free(clone2);

	clone2 = ixmlNode_cloneNode(clone1, 0);
	CHECK(clone2);
	CHECK(!strcmp(
		ixmlElement_getAttribute((IXML_Element *)clone2, "a"), "1"));
	CHECK(!strcmp(
		ixmlElement_getAttribute((IXML_Element *)clone2, "b"), "2"));
	ixmlNode_free(clone2);

	ixmlElement_free(e0);
	ixmlDocument_free(doc);

	return 0;
}

int main(void)
{
	int err = 0;

	err |= test_clone_of_clone_with_new_attr();
	if (!err)
		puts("test_node_clone_attrs: PASS");

	return err;
}
