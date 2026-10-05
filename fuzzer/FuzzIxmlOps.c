/* Sequence-driven fuzzer for the ixml DOM mutation API.
 *
 * The input is a list of 4-byte operations over a small pool of nodes that all
 * belong to one document. After every operation the child and attribute lists
 * of the document tree and of every pool node are verified (no cycles,
 * parent/prev/next consistency), so corruption is reported at the offending
 * call. Nodes that end up detached (created but never attached, or returned by
 * remove/replace) are freed at the end, once per floating root.
 *
 * Only members of IXML_Node are read directly. They all come before the
 * optional ctag member, so the layout does not depend on
 * IXML_HAVE_SCRIPTSUPPORT.
 */
#include "ixml.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define POOL 48
#define MAX_OPS 160
#define WALK_LIMIT 4096
/* Nodes that cloneNode may create in one run. Every other operation creates at
 * most one node, so no tree ever gets near WALK_LIMIT without a cycle. */
#define CLONE_BUDGET 2048
#define NUM_OPS 20
#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

static IXML_Node *pool[POOL];
static int npool;
static IXML_Document *doc;
static int cur_op;
static int clone_budget;

static const char *g_names[] = {
	"a", "b", "c", "d", "p:e", "xmlns:p", "x", "q:r"};
static const char *g_values[] = {"1", "2", "", "v w", "<&>"};
static const char *g_ns[] = {"urn:x",
	"urn:y",
	"http://www.w3.org/2000/xmlns/",
	"http://www.w3.org/XML/1998/namespace"};

static void die(const char *why)
{
	fprintf(stderr, "INVARIANT VIOLATION: %s (after op %d)\n", why, cur_op);
	abort();
}

/* Track a detached node. When the pool is full the node is freed instead. */
static void reg(IXML_Node *n)
{
	int i;

	if (!n) {
		return;
	}
	for (i = 0; i < npool; i++) {
		if (pool[i] == n) {
			return;
		}
	}
	if (npool < POOL) {
		pool[npool++] = n;
	} else {
		ixmlNode_free(n);
	}
}

static IXML_Node *pick(uint8_t idx)
{
	if (npool == 0) {
		return NULL;
	}
	return pool[idx % npool];
}

static IXML_Node *pick_type(uint8_t idx, int type)
{
	int i;

	for (i = 0; i < npool; i++) {
		IXML_Node *n = pool[(idx + i) % npool];

		if (ixmlNode_getNodeType(n) == type) {
			return n;
		}
	}
	return NULL;
}

static void check_attrs(IXML_Node *e)
{
	IXML_Node *a;
	IXML_Node *prev = NULL;
	int steps = 0;

	for (a = e->firstAttr; a; a = a->nextSibling) {
		if (++steps > WALK_LIMIT) {
			die("attribute chain too long (cycle?)");
		}
		if (e->nodeType != eELEMENT_NODE) {
			die("attribute chain on a non-element");
		}
		if (a->nodeType != eATTRIBUTE_NODE) {
			die("non-attribute in attribute chain");
		}
		if (a->prevSibling != prev) {
			die("attr->prevSibling mismatch");
		}
		if (a->parentNode != e) {
			die("attr->parentNode mismatch");
		}
		prev = a;
	}
}

static void check_children(IXML_Node *parent)
{
	IXML_Node *c;
	IXML_Node *prev = NULL;
	int steps = 0;

	check_attrs(parent);
	for (c = parent->firstChild; c; c = c->nextSibling) {
		if (++steps > WALK_LIMIT) {
			die("child list too long (cycle?)");
		}
		if (c->parentNode != parent) {
			die("child->parentNode mismatch");
		}
		if (c->prevSibling != prev) {
			die("child->prevSibling mismatch");
		}
		prev = c;
	}
}

/* Walk the whole tree below the document. */
static void check_tree(void)
{
	IXML_Node *stack[WALK_LIMIT];
	int sp = 0;
	int visited = 0;

	stack[sp++] = (IXML_Node *)doc;
	while (sp > 0) {
		IXML_Node *n = stack[--sp];
		IXML_Node *c;

		if (++visited > WALK_LIMIT) {
			die("tree too large (cycle?)");
		}
		check_children(n);
		for (c = n->firstChild; c; c = c->nextSibling) {
			if (sp >= WALK_LIMIT - 1) {
				die("stack overflow (cycle?)");
			}
			stack[sp++] = c;
		}
	}
}

/* Does the subtree of root (children and attributes) contain target? */
static int subtree_contains(IXML_Node *root, IXML_Node *target)
{
	IXML_Node *stack[WALK_LIMIT];
	int sp = 0;
	int visited = 0;

	stack[sp++] = root;
	while (sp > 0) {
		IXML_Node *n = stack[--sp];
		IXML_Node *c;

		if (n == target) {
			return 1;
		}
		if (++visited > WALK_LIMIT) {
			die("subtree too large (cycle?)");
		}
		for (c = n->firstChild; c; c = c->nextSibling) {
			if (sp >= WALK_LIMIT - 1) {
				die("stack overflow (cycle?)");
			}
			stack[sp++] = c;
		}
		for (c = n->firstAttr; c; c = c->nextSibling) {
			if (sp >= WALK_LIMIT - 1) {
				die("stack overflow (cycle?)");
			}
			stack[sp++] = c;
		}
	}
	return 0;
}

/* Number of nodes in the subtree of n (children and attributes). */
static int subtree_size(IXML_Node *n)
{
	IXML_Node *c;
	int total = 1;

	for (c = n->firstChild; c; c = c->nextSibling) {
		total += subtree_size(c);
	}
	for (c = n->firstAttr; c; c = c->nextSibling) {
		total += subtree_size(c);
	}
	return total;
}

static void do_op(const uint8_t *op)
{
	uint8_t code = op[0] % NUM_OPS;
	uint8_t a = op[1];
	uint8_t b = op[2];
	uint8_t c = op[3];
	const char *name = g_names[c % ARRAY_LEN(g_names)];
	const char *val = g_values[b % ARRAY_LEN(g_values)];
	const char *ns = g_ns[a % ARRAY_LEN(g_ns)];
	const char *local = (c & 1) ? "e" : "a";
	IXML_Node *x;
	IXML_Node *y;
	IXML_Node *z;
	IXML_Node *ret = NULL;
	IXML_Attr *aret = NULL;

	switch (code) {
	case 0: {
		IXML_Element *e = NULL;

		if (npool < POOL && ixmlDocument_createElementEx(
					    doc, name, &e) == IXML_SUCCESS) {
			reg((IXML_Node *)e);
		}
		break;
	}
	case 1: {
		IXML_Node *t = NULL;

		if (npool < POOL && ixmlDocument_createTextNodeEx(
					    doc, val, &t) == IXML_SUCCESS) {
			reg(t);
		}
		break;
	}
	case 2: {
		IXML_Attr *at = NULL;
		int rc;

		if (npool >= POOL) {
			break;
		}
		if (b & 0x80) {
			rc = ixmlDocument_createAttributeNSEx(
				doc, ns, name, &at);
		} else {
			rc = ixmlDocument_createAttributeEx(doc, name, &at);
		}
		if (rc == IXML_SUCCESS && at) {
			ixmlNode_setNodeValue((IXML_Node *)at, val);
			reg((IXML_Node *)at);
		}
		break;
	}
	case 3: {
		IXML_CDATASection *cd = NULL;

		if (npool < POOL && ixmlDocument_createCDATASectionEx(
					    doc, val, &cd) == IXML_SUCCESS) {
			reg((IXML_Node *)cd);
		}
		break;
	}
	case 4: /* appendChild */
		x = (b & 1) ? (IXML_Node *)doc : pick(a);
		y = pick(c);
		if (x && y) {
			ixmlNode_appendChild(x, y);
		}
		break;
	case 5: /* insertBefore */
		x = (b & 1) ? (IXML_Node *)doc : pick(a);
		y = pick(c);
		z = (b & 2) ? NULL : pick(b >> 2);
		if (x && y) {
			ixmlNode_insertBefore(x, y, z);
		}
		break;
	case 6: /* replaceChild */
		x = (b & 1) ? (IXML_Node *)doc : pick(a);
		y = pick(c);
		z = pick(b >> 1);
		if (x && y && z &&
			ixmlNode_replaceChild(x, y, z, &ret) == IXML_SUCCESS) {
			reg(ret);
		}
		break;
	case 7: /* removeChild */
		x = (b & 1) ? (IXML_Node *)doc : pick(a);
		y = pick(c);
		if (x && y &&
			ixmlNode_removeChild(x, y, &ret) == IXML_SUCCESS) {
			reg(ret);
		}
		break;
	case 8: /* cloneNode */
		x = pick(a);
		if (x && npool < POOL) {
			int size;

			ret = ixmlNode_cloneNode(x, b & 1);
			if (!ret) {
				break;
			}
			/* Deep clones double a tree, so a few of them would
			 * pass WALK_LIMIT with no cycle at all. */
			size = subtree_size(ret);
			if (size > clone_budget) {
				ixmlNode_free(ret);
				break;
			}
			clone_budget -= size;
			reg(ret);
		}
		break;
	case 9: /* setAttribute */
		x = pick_type(a, eELEMENT_NODE);
		if (x) {
			ixmlElement_setAttribute((IXML_Element *)x, name, val);
		}
		break;
	case 10: /* removeAttribute */
		x = pick_type(a, eELEMENT_NODE);
		if (x) {
			ixmlElement_removeAttribute((IXML_Element *)x, name);
		}
		break;
	case 11: /* setAttributeNode */
		x = pick_type(a, eELEMENT_NODE);
		y = pick_type(c, eATTRIBUTE_NODE);
		if (x && y &&
			ixmlElement_setAttributeNode((IXML_Element *)x,
				(IXML_Attr *)y,
				&aret) == IXML_SUCCESS) {
			reg((IXML_Node *)aret);
		}
		break;
	case 12: /* removeAttributeNode */
		x = pick_type(a, eELEMENT_NODE);
		y = pick_type(c, eATTRIBUTE_NODE);
		if (x && y &&
			ixmlElement_removeAttributeNode((IXML_Element *)x,
				(IXML_Attr *)y,
				&aret) == IXML_SUCCESS) {
			reg((IXML_Node *)aret);
		}
		break;
	case 13: /* setAttributeNS */
		x = pick_type(a, eELEMENT_NODE);
		if (x) {
			ixmlElement_setAttributeNS(
				(IXML_Element *)x, ns, name, val);
		}
		break;
	case 14: /* removeAttributeNS */
		x = pick_type(a, eELEMENT_NODE);
		if (x) {
			ixmlElement_removeAttributeNS(
				(IXML_Element *)x, ns, local);
		}
		break;
	case 15: /* getAttributeNS / hasAttributeNS / getAttributeNodeNS */
		x = pick_type(a, eELEMENT_NODE);
		if (x) {
			(void)ixmlElement_getAttributeNS(
				(IXML_Element *)x, ns, local);
			(void)ixmlElement_hasAttributeNS(
				(IXML_Element *)x, ns, local);
			(void)ixmlElement_getAttributeNodeNS(
				(IXML_Element *)x, ns, local);
		}
		break;
	case 16: /* setAttributeNodeNS */
		x = pick_type(a, eELEMENT_NODE);
		y = pick_type(c, eATTRIBUTE_NODE);
		if (x && y &&
			ixmlElement_setAttributeNodeNS((IXML_Element *)x,
				(IXML_Attr *)y,
				&aret) == IXML_SUCCESS) {
			reg((IXML_Node *)aret);
		}
		break;
	case 17: { /* serialise */
		DOMString s;

		x = (b & 1) ? (IXML_Node *)doc : pick(a);
		if (x) {
			s = ixmlPrintNode(x);
			ixmlFreeDOMString(s);
			s = ixmlNodetoString(x);
			ixmlFreeDOMString(s);
		}
		break;
	}
	case 18: { /* setNodeValue / query lists */
		IXML_NodeList *l;

		x = pick(a);
		if (x) {
			ixmlNode_setNodeValue(x, val);
		}
		l = ixmlDocument_getElementsByTagName(doc, name);
		ixmlNodeList_free(l);
		if (x && ixmlNode_getNodeType(x) == eELEMENT_NODE) {
			l = ixmlElement_getElementsByTagName(
				(IXML_Element *)x, name);
			ixmlNodeList_free(l);
		}
		break;
	}
	case 19: { /* attribute map access */
		IXML_NamedNodeMap *m;
		unsigned long i;
		unsigned long n;

		x = pick_type(a, eELEMENT_NODE);
		if (x) {
			m = ixmlNode_getAttributes(x);
			n = ixmlNamedNodeMap_getLength(m);
			for (i = 0; i < n; i++) {
				(void)ixmlNamedNodeMap_item(m, i);
			}
			(void)ixmlNamedNodeMap_getNamedItem(m, name);
			ixmlNamedNodeMap_free(m);
		}
		break;
	}
	}
}

extern int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size)
{
	size_t i;
	int j;
	IXML_Node *roots[POOL];
	int owned[POOL];
	int nroots = 0;

	if (Size < 4) {
		return 0;
	}
	if (Size > 4 * MAX_OPS) {
		Size = 4 * MAX_OPS;
	}

	npool = 0;
	clone_budget = CLONE_BUDGET;
	doc = NULL;
	if (ixmlDocument_createDocumentEx(&doc) != IXML_SUCCESS || !doc) {
		return 0;
	}

	for (i = 0; i + 4 <= Size; i += 4) {
		cur_op = (int)(i / 4);
		do_op(Data + i);
		check_tree();
		for (j = 0; j < npool; j++) {
			check_children(pool[j]);
		}
	}

	/* Ownership by reachability: nodes reachable from the document are
	 * freed with it; among the rest, free only the maximal roots. */
	for (j = 0; j < npool; j++) {
		owned[j] = subtree_contains((IXML_Node *)doc, pool[j]);
	}
	for (j = 0; j < npool; j++) {
		int k;
		int nested = 0;

		if (owned[j]) {
			continue;
		}
		for (k = 0; k < npool && !nested; k++) {
			if (k == j || owned[k]) {
				continue;
			}
			if (subtree_contains(pool[k], pool[j]) &&
				!subtree_contains(pool[j], pool[k])) {
				nested = 1;
			}
		}
		if (!nested) {
			roots[nroots++] = pool[j];
		}
	}
	ixmlDocument_free(doc);
	for (j = 0; j < nroots; j++) {
		ixmlNode_free(roots[j]);
	}

	return 0;
}
