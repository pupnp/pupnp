/*
 * Regression test: an empty CDATA section is valid XML.
 *
 * Parser_processCDSect() only accepted a CDATA section with at least one
 * character, so ixmlParseBufferEx() failed with IXML_SYNTAX_ERR for
 * documents such as <a><![CDATA[]]></a>.
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

/* Parses xml, prints the root element again and compares it to expected. */
static int check_parse(const char *xml, const char *expected)
{
	IXML_Document *doc = NULL;
	DOMString out;

	CHECK(ixmlParseBufferEx(xml, &doc) == IXML_SUCCESS);
	CHECK(doc);
	out = ixmlPrintNode(ixmlNode_getFirstChild((IXML_Node *)doc));
	CHECK(out);
	CHECK(!strcmp(out, expected));
	ixmlFreeDOMString(out);
	ixmlDocument_free(doc);

	return 0;
}

int main(void)
{
	if (check_parse("<a><![CDATA[]]></a>", "<a><![CDATA[]]></a>\r\n"))
		return 1;
	if (check_parse("<a>x<![CDATA[]]>y</a>", "<a>x<![CDATA[]]>y</a>\r\n"))
		return 1;
	if (check_parse("<a><b><![CDATA[]]></b><c/></a>",
		    "<a>\r\n<b><![CDATA[]]></b>\r\n<c></c>\r\n</a>\r\n"))
		return 1;
	if (check_parse("<a><![CDATA[v]]></a>", "<a><![CDATA[v]]></a>\r\n"))
		return 1;

	return 0;
}
