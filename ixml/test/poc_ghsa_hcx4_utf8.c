#include "ixml.h"

#include <stdlib.h>
#include <string.h>

static void try_xml(const char *xml)
{
	IXML_Document *doc = ixmlParseBuffer(xml);
	if (doc)
		ixmlDocument_free(doc);
}

int main(void);

int main(void)
{
	/* GHSA-hcx4-hh6g-5cpc: exercises Parser_UTF8ToInt and
	 * Parser_getNextToken at multi-byte UTF-8 boundaries.
	 * Running with AddressSanitizer on unfixed code will crash. */

	/* Minimal and truncated ASCII inputs */
	try_xml("<a/>");
	try_xml("<");
	try_xml("</");
	try_xml("<ab");

	/* '&' with 3 chars remaining before null — exercises Parser_getChar */
	{
		char buf[490];
		memset(buf, 'x', sizeof(buf));
		buf[0] = '<';
		buf[1] = 'r';
		buf[2] = '>';
		buf[489] = '\0';
		buf[486] = '&';
		try_xml(buf);
	}

	/* U+00E9 = 0xC3 0xA9 ("é"); 2-byte UTF-8 ending right before null */
	{
		char buf[8];
		buf[0] = '<';
		buf[1] = 'r';
		buf[2] = '>';
		buf[3] = '<';
		buf[4] = (char)0xC3;
		buf[5] = (char)0xA9;
		buf[6] = '/';
		buf[7] = '\0';
		try_xml(buf);
	}

	/* First byte of a 2-byte UTF-8 sequence right before null */
	{
		char buf[8];
		buf[0] = '<';
		buf[1] = (char)0xC3;
		buf[2] = (char)0xA9;
		buf[3] = '>';
		buf[4] = '<';
		buf[5] = '/';
		buf[6] = (char)0xC3;
		buf[7] = '\0';
		try_xml(buf);
	}

	/* 488-byte buffer with 2-byte UTF-8 'é' ending at the last two bytes */
	{
		size_t n = 488;
		char *buf = (char *)malloc(n + 1);
		memset(buf, 'a', n);
		buf[0] = '<';
		buf[1] = 'r';
		buf[2] = '>';
		buf[n - 2] = (char)0xC3;
		buf[n - 1] = (char)0xA9;
		buf[n] = '\0';
		try_xml(buf);
		free(buf);
	}

	return 0;
}
