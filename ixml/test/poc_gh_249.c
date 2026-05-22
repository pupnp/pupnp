/* Regression test for GitHub issue #249 / CVE-2021-28302.
 *
 * Parsing XML with deeply nested unclosed tags builds a tree N levels deep.
 * The old recursive ixmlNode_free() overflowed the stack when freeing it.
 * The fix replaced the recursive implementation with an iterative one. */

#include "ixml.h"

#include <stdlib.h>
#include <string.h>

/* 10 000 levels is enough to crash the old recursive ixmlNode_free() on
 * devices with a small thread stack (e.g. 128 KB–512 KB), and completes in
 * well under a second with the iterative replacement. */
#define NESTING_DEPTH 10000

int main(void);

int main(void)
{
	const char prefix[] = "<root>";
	const char tag[] = "<a>";
	const char suffix[] = "</root>";
	size_t tag_len = sizeof(tag) - 1;
	size_t len = (sizeof(prefix) - 1) + tag_len * NESTING_DEPTH +
		     (sizeof(suffix) - 1);
	char *buf = malloc(len + 1);
	if (!buf)
		return 1;

	char *p = buf;
	memcpy(p, prefix, sizeof(prefix) - 1);
	p += sizeof(prefix) - 1;
	for (int i = 0; i < NESTING_DEPTH; i++) {
		memcpy(p, tag, tag_len);
		p += tag_len;
	}
	memcpy(p, suffix, sizeof(suffix) - 1);
	p += sizeof(suffix) - 1;
	*p = '\0';

	/* The parser may return an error for the malformed (unclosed) tags,
	 * but must not crash.  The crash was in ixmlDocument_free() after the
	 * parser built a partial tree and then called the (then-recursive)
	 * ixmlNode_free() to clean it up. */
	IXML_Document *doc = NULL;
	ixmlParseBufferEx(buf, &doc);
	free(buf);

	ixmlDocument_free(doc);

	return 0;
}
