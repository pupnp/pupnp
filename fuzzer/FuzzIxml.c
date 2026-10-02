#include "ixml.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define kMinInputLength 10
#define kMaxInputLength 5120

int CheckXML(char *filename)
{

	int rc;
	DOMString s;
	IXML_Document *doc = NULL;

	rc = ixmlLoadDocumentEx(filename, &doc);
	if (rc != IXML_SUCCESS) {
		return rc;
	}

	s = ixmlPrintDocument(doc);
	if (s == NULL || s[0] == '\0') {
		ixmlDocument_free(doc);
		return 1;
	}

	ixmlFreeDOMString(s);
	ixmlDocument_free(doc);

	return 0;
}

extern int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size)
{
	char filename[256];
	FILE *fp;

	if (Size < kMinInputLength || Size > kMaxInputLength) {
		return 0;
	}

	snprintf(filename, sizeof(filename), "/tmp/libfuzzer.%d", getpid());
	fp = fopen(filename, "wb");
	if (!fp) {
		return 0;
	}

	fwrite(Data, Size, 1, fp);
	fclose(fp);

	/* libFuzzer only accepts 0 and -1, and leaves an input out of the
	 * corpus on anything but 0: a document that fails to parse must
	 * still return 0. */
	(void)CheckXML(filename);
	unlink(filename);
	return 0;
}
