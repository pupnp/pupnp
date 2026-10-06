/*
 * Regression test: ixmlElement_setAttributeNS() running out of memory while
 * it sets an attribute that the element already has.
 *
 * When the copy of the new value failed, the function freed the prefix of
 * the attribute and left the pointer in place. The attribute stayed in the
 * element with a dangling prefix, which was read and freed again later.
 *
 * The test replaces strdup() to make that one copy fail, so it is built only
 * where a definition in the executable is the one the library calls.
 */

#include "ixml.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NS "urn:test:ns"
#define FAIL_VALUE "strdup-of-this-value-fails"

#define CHECK(cond) \
	do { \
		if (!(cond)) { \
			printf("FAIL line %d: %s\n", __LINE__, #cond); \
			fflush(stdout); \
			return 1; \
		} \
	} while (0)

static int strdup_failures;

char *strdup(const char *s)
{
	size_t size = strlen(s) + 1;
	char *copy;

	if (!strcmp(s, FAIL_VALUE)) {
		strdup_failures++;
		return NULL;
	}
	copy = malloc(size);
	if (copy) {
		memcpy(copy, s, size);
	}

	return copy;
}

static int test_set_attribute_ns_value_oom(void)
{
	IXML_Document *doc;
	IXML_Element *el;
	IXML_Attr *attr;
	const char *value;
	const char *prefix;

	CHECK(ixmlDocument_createDocumentEx(&doc) == IXML_SUCCESS);
	el = ixmlDocument_createElement(doc, "E");
	CHECK(el);
	CHECK(ixmlElement_setAttributeNS(el, NS, "p:a", "1") == IXML_SUCCESS);
	CHECK(ixmlElement_setAttributeNS(el, NS, "q:a", FAIL_VALUE) ==
		IXML_INSUFFICIENT_MEMORY);
	CHECK(strdup_failures == 1);
	/* The attribute is as it was before the failed call. */
	value = ixmlElement_getAttributeNS(el, NS, "a");
	CHECK(value && !strcmp(value, "1"));
	attr = ixmlElement_getAttributeNodeNS(el, NS, "a");
	CHECK(attr);
	prefix = ixmlNode_getPrefix((IXML_Node *)attr);
	CHECK(prefix && !strcmp(prefix, "p"));
	ixmlElement_free(el);
	ixmlDocument_free(doc);

	return 0;
}

int main(void)
{
	if (test_set_attribute_ns_value_oom()) {
		return 1;
	}
	printf("PASS\n");

	return 0;
}
