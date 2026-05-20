#include "ixml.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Build a buffer of `total` bytes (plus null terminator) with prefix,
   padding, and a specific tail pattern before the null. */
static int try_buf(const char *prefix, size_t total, const char *tail)
{
	size_t plen = strlen(prefix);
	size_t tlen = strlen(tail);

	if (plen + tlen > total)
		return 0;

	char *buf = (char *)calloc(total + 1, 1);
	if (!buf)
		return 1;

	memcpy(buf, prefix, plen);
	memset(buf + plen, 'x', total - plen - tlen);
	memcpy(buf + total - tlen, tail, tlen);
	buf[total] = '\0';

	IXML_Document *doc = ixmlParseBuffer(buf);
	if (doc)
		ixmlDocument_free(doc);
	free(buf);
	return 0;
}

int main(void);

int main(void)
{
	int err = 0;

	/* GHSA-hcx4-hh6g-5cpc: 1-byte heap-buffer-overflow READ triggered by
	 * strchr(DEC_NUMBERS/HEX_NUMBERS, '\0') returning non-NULL, causing
	 * while loops in Parser_getChar to advance past the null terminator.
	 * Fixed by adding '*pnum &&' guard in the ESC_HEX and ESC_DEC loops,
	 * and by fixing Parser_skipWhiteSpaces to stop at null.
	 *
	 * These inputs exercise the fix by placing truncated XML entity
	 * references and trailing whitespace exactly at the buffer boundary.
	 * Running with AddressSanitizer on unfixed code will crash; the fixed
	 * code must return without crashing (any parse result is acceptable).
	 */

	const char *prefix = "<root>";

	/* Decimal entity cut off before closing semicolon at buffer end */
	const char *dec_tails[] = {
		"&#1", "&#12", "&#123", "&#1234", "&#12345", NULL};

	/* Hex entity cut off before closing semicolon at buffer end */
	const char *hex_tails[] = {
		"&#x1", "&#xA", "&#xAB", "&#xABC", "&#xABCD", NULL};

	/* Named entities and structural tokens at buffer end */
	const char *named_tails[] = {"</root>",
		"&#x0;",
		"&#0;",
		"&quot;",
		"&amp;",
		"&lt;",
		"&gt;",
		"&apos;",
		"/>",
		"</",
		NULL};

	/* Trailing whitespace — exercises Parser_skipWhiteSpaces */
	const char *ws_tails[] = {" ", "  ", "\t", "\n", " \t\n", NULL};

	/* Test around the boundary sizes mentioned in the original reports
	 * (issues #425, #446 mentioned ~490-byte buffers). */
	for (size_t total = 486; total <= 494; total++) {
		int i;
		for (i = 0; dec_tails[i]; i++) {
			err |= try_buf(prefix, total, dec_tails[i]);
		}
		for (i = 0; hex_tails[i]; i++) {
			err |= try_buf(prefix, total, hex_tails[i]);
		}
		for (i = 0; named_tails[i]; i++) {
			err |= try_buf(prefix, total, named_tails[i]);
		}
		for (i = 0; ws_tails[i]; i++) {
			err |= try_buf(prefix, total, ws_tails[i]);
		}
	}

	/* Minimal cases — small buffers with entity at the very end */
	const char *cases[] = {"<r>&#1",
		"<r>&#12",
		"<r>&#123",
		"<r>&#x1",
		"<r>&#xA",
		"<r>&#xAB",
		"<r> ",
		"<r>\t",
		"<r>\n",
		NULL};
	for (int i = 0; cases[i]; i++) {
		IXML_Document *doc = ixmlParseBuffer(cases[i]);
		if (doc)
			ixmlDocument_free(doc);
	}

	return err;
}
