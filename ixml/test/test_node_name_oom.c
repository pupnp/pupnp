/*
 * Regression test: creating an element or a namespaced node running out of
 * memory while it copies the name.
 *
 * ixmlDocument_createElementEx() freed the tag name when the copy of the
 * node name failed and then called ixmlElement_free(), which frees the tag
 * name again. ixmlNode_setNodeName() freed the node name when the prefix or
 * the local name could not be set and left the pointer in place, so the
 * callers freed it again with the node. The parser creates every element
 * with ixmlDocument_createElementEx().
 *
 * The test replaces strdup() to make the copies of one name fail, so it is
 * built only where a definition in the executable is the one the library
 * calls.
 */

#include "ixml.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NS "urn:test:ns"
#define FAIL_NAME "oom"

#define CHECK(cond) \
	do { \
		if (!(cond)) { \
			printf("FAIL line %d: %s\n", __LINE__, #cond); \
			fflush(stdout); \
			return 1; \
		} \
	} while (0)

/* Copies of FAIL_NAME counted so far, and which one fails (0: none). */
static int fail_name_copies;
static int fail_name_copy;

char *strdup(const char *s)
{
	size_t size = strlen(s) + 1;
	char *copy;

	if (!strcmp(s, FAIL_NAME) && ++fail_name_copies == fail_name_copy) {
		return NULL;
	}
	copy = malloc(size);
	if (copy) {
		memcpy(copy, s, size);
	}

	return copy;
}

static void fail_copy(int n)
{
	fail_name_copies = 0;
	fail_name_copy = n;
}

static int test_create_element_node_name_oom(void)
{
	IXML_Document *doc;
	IXML_Element *el = NULL;

	CHECK(ixmlDocument_createDocumentEx(&doc) == IXML_SUCCESS);
	/* The first copy is the tag name, the second one the node name. */
	fail_copy(2);
	CHECK(ixmlDocument_createElementEx(doc, FAIL_NAME, &el) ==
		IXML_INSUFFICIENT_MEMORY);
	CHECK(fail_name_copies == 2);
	CHECK(el == NULL);
	fail_copy(0);
	ixmlDocument_free(doc);

	return 0;
}

static int test_create_element_ns_local_name_oom(void)
{
	IXML_Document *doc;
	IXML_Element *el = NULL;

	CHECK(ixmlDocument_createDocumentEx(&doc) == IXML_SUCCESS);
	/* The only copy of the bare local name. */
	fail_copy(1);
	CHECK(ixmlDocument_createElementNSEx(doc, NS, "p:" FAIL_NAME, &el) ==
		IXML_INSUFFICIENT_MEMORY);
	CHECK(fail_name_copies == 1);
	CHECK(el == NULL);
	fail_copy(0);
	ixmlDocument_free(doc);

	return 0;
}

static int test_create_attribute_ns_local_name_oom(void)
{
	IXML_Document *doc;
	IXML_Attr *attr = NULL;

	CHECK(ixmlDocument_createDocumentEx(&doc) == IXML_SUCCESS);
	fail_copy(1);
	CHECK(ixmlDocument_createAttributeNSEx(
		      doc, NS, "p:" FAIL_NAME, &attr) ==
		IXML_INSUFFICIENT_MEMORY);
	CHECK(fail_name_copies == 1);
	CHECK(attr == NULL);
	fail_copy(0);
	ixmlDocument_free(doc);

	return 0;
}

static int test_parse_element_name_oom(void)
{
	IXML_Document *doc;
	int n;
	int rc;

	/* Fail each copy of the element name in turn. */
	for (n = 1;; n++) {
		doc = NULL;
		fail_copy(n);
		rc = ixmlParseBufferEx("<r><" FAIL_NAME "/></r>", &doc);
		if (fail_name_copies < n) {
			/* No copy failed: the parse is complete. */
			CHECK(rc == IXML_SUCCESS);
			ixmlDocument_free(doc);
			break;
		}
		CHECK(rc != IXML_SUCCESS);
		CHECK(doc == NULL);
	}
	fail_copy(0);
	CHECK(n > 2);

	return 0;
}

int main(void)
{
	if (test_create_element_node_name_oom() ||
		test_create_element_ns_local_name_oom() ||
		test_create_attribute_ns_local_name_oom() ||
		test_parse_element_name_oom()) {
		return 1;
	}
	printf("PASS\n");

	return 0;
}
