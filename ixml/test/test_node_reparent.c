/*
 * Regression test: moving a node that already has a parent.
 *
 * ixmlNode_appendChild() and ixmlNode_insertBefore() only detached newChild
 * from its old parent when that parent was the same as the new one. A node
 * that belonged to a different parent ended up in two child lists, which
 * garbled the tree and made ixmlDocument_free() free it twice.
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

/* doc -> root -> { p1 -> { a, b, c }, p2 -> { x } } */
struct tree
{
	IXML_Document *doc;
	IXML_Node *root;
	IXML_Node *p1;
	IXML_Node *p2;
	IXML_Node *a;
	IXML_Node *b;
	IXML_Node *c;
	IXML_Node *x;
};

static IXML_Node *new_elem(IXML_Document *doc, const char *name)
{
	return (IXML_Node *)ixmlDocument_createElement(doc, name);
}

static int build(struct tree *t)
{
	CHECK(ixmlDocument_createDocumentEx(&t->doc) == IXML_SUCCESS);
	t->root = new_elem(t->doc, "root");
	t->p1 = new_elem(t->doc, "p1");
	t->p2 = new_elem(t->doc, "p2");
	t->a = new_elem(t->doc, "a");
	t->b = new_elem(t->doc, "b");
	t->c = new_elem(t->doc, "c");
	t->x = new_elem(t->doc, "x");
	CHECK(t->root && t->p1 && t->p2 && t->a && t->b && t->c && t->x);
	CHECK(ixmlNode_appendChild((IXML_Node *)t->doc, t->root) ==
		IXML_SUCCESS);
	CHECK(ixmlNode_appendChild(t->root, t->p1) == IXML_SUCCESS);
	CHECK(ixmlNode_appendChild(t->root, t->p2) == IXML_SUCCESS);
	CHECK(ixmlNode_appendChild(t->p1, t->a) == IXML_SUCCESS);
	CHECK(ixmlNode_appendChild(t->p1, t->b) == IXML_SUCCESS);
	CHECK(ixmlNode_appendChild(t->p1, t->c) == IXML_SUCCESS);
	CHECK(ixmlNode_appendChild(t->p2, t->x) == IXML_SUCCESS);

	return 0;
}

/* p1 must now hold exactly a, c, and b must be gone from it. */
static int check_p1_a_c(const struct tree *t)
{
	CHECK(t->p1->firstChild == t->a);
	CHECK(t->a->nextSibling == t->c);
	CHECK(t->c->nextSibling == NULL);
	CHECK(t->c->prevSibling == t->a);

	return 0;
}

static int test_append_child(void)
{
	struct tree t;

	if (build(&t))
		return 1;
	/* Move the middle child of p1 to the end of p2. */
	CHECK(ixmlNode_appendChild(t.p2, t.b) == IXML_SUCCESS);
	if (check_p1_a_c(&t))
		return 1;
	CHECK(t.b->parentNode == t.p2);
	CHECK(t.p2->firstChild == t.x);
	CHECK(t.x->nextSibling == t.b);
	CHECK(t.b->prevSibling == t.x);
	CHECK(t.b->nextSibling == NULL);
	ixmlDocument_free(t.doc);

	return 0;
}

static int test_insert_before(void)
{
	struct tree t;

	if (build(&t))
		return 1;
	/* Move the middle child of p1 in front of x in p2. */
	CHECK(ixmlNode_insertBefore(t.p2, t.b, t.x) == IXML_SUCCESS);
	if (check_p1_a_c(&t))
		return 1;
	CHECK(t.b->parentNode == t.p2);
	CHECK(t.p2->firstChild == t.b);
	CHECK(t.b->nextSibling == t.x);
	CHECK(t.x->prevSibling == t.b);
	CHECK(t.b->prevSibling == NULL);
	ixmlDocument_free(t.doc);

	return 0;
}

int main(void)
{
	int err = 0;

	err |= test_append_child();
	err |= test_insert_before();
	if (!err)
		puts("test_node_reparent: PASS");

	return err;
}
