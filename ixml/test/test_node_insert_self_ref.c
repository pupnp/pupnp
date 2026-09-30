/*
 * Regression test: using a child as its own reference node.
 *
 * insertBefore(p, x, x) removed x from p and then linked it in front of
 * itself: x ended up detached with prevSibling and nextSibling pointing at
 * x. replaceChild(p, x, x) went through the same path and then removed x.
 * The DOM defines both as no-ops that leave x where it is.
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

struct tree
{
	IXML_Document *doc;
	IXML_Node *p;
	IXML_Node *a;
	IXML_Node *x;
	IXML_Node *b;
};

/* doc -> p -> { a, x, b } */
static int build(struct tree *t)
{
	CHECK(ixmlDocument_createDocumentEx(&t->doc) == IXML_SUCCESS);
	t->p = (IXML_Node *)ixmlDocument_createElement(t->doc, "p");
	t->a = (IXML_Node *)ixmlDocument_createElement(t->doc, "a");
	t->x = (IXML_Node *)ixmlDocument_createElement(t->doc, "x");
	t->b = (IXML_Node *)ixmlDocument_createElement(t->doc, "b");
	CHECK(t->p && t->a && t->x && t->b);
	CHECK(ixmlNode_appendChild((IXML_Node *)t->doc, t->p) == IXML_SUCCESS);
	CHECK(ixmlNode_appendChild(t->p, t->a) == IXML_SUCCESS);
	CHECK(ixmlNode_appendChild(t->p, t->x) == IXML_SUCCESS);
	CHECK(ixmlNode_appendChild(t->p, t->b) == IXML_SUCCESS);

	return 0;
}

/* p must still be { a, x, b }. */
static int check_unchanged(const struct tree *t)
{
	CHECK(t->p->firstChild == t->a);
	CHECK(t->a->nextSibling == t->x);
	CHECK(t->x->prevSibling == t->a);
	CHECK(t->x->nextSibling == t->b);
	CHECK(t->b->prevSibling == t->x);
	CHECK(t->b->nextSibling == NULL);
	CHECK(t->x->parentNode == t->p);

	return 0;
}

static int test_insert_before_self(void)
{
	struct tree t;

	if (build(&t))
		return 1;
	CHECK(ixmlNode_insertBefore(t.p, t.x, t.x) == IXML_SUCCESS);
	if (check_unchanged(&t))
		return 1;
	/* Same with the first child, which also owns p->firstChild. */
	CHECK(ixmlNode_insertBefore(t.p, t.a, t.a) == IXML_SUCCESS);
	if (check_unchanged(&t))
		return 1;
	ixmlDocument_free(t.doc);

	return 0;
}

static int test_replace_child_self(void)
{
	struct tree t;
	IXML_Node *ret = NULL;

	if (build(&t))
		return 1;
	CHECK(ixmlNode_replaceChild(t.p, t.x, t.x, &ret) == IXML_SUCCESS);
	CHECK(ret == t.x);
	if (check_unchanged(&t))
		return 1;
	/* Without a return node the child must not be freed either. */
	CHECK(ixmlNode_replaceChild(t.p, t.x, t.x, NULL) == IXML_SUCCESS);
	if (check_unchanged(&t))
		return 1;
	ixmlDocument_free(t.doc);

	return 0;
}

int main(void)
{
	int err = 0;

	err |= test_insert_before_self();
	err |= test_replace_child_self();
	if (!err)
		puts("test_node_insert_self_ref: PASS");

	return err;
}
