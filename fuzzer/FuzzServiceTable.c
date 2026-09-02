#include "service_table.h"

#include "ixml.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size)
{
	IXML_Document *doc = NULL;
	service_table table;
	char *xml;

	if (Size < 1 || Size > 65536) {
		return 0;
	}

	xml = malloc(Size + 1);
	if (!xml) {
		return 0;
	}
	memcpy(xml, Data, Size);
	xml[Size] = '\0';

	/* A control point parses the device description document fetched from a
	 * device, then builds the service table from it. */
	if (ixmlParseBufferEx(xml, &doc) == IXML_SUCCESS && doc) {
		memset(&table, 0, sizeof(table));
		getServiceTable((IXML_Node *)doc, &table, "http://127.0.0.1/");
		freeServiceTable(&table);
		ixmlDocument_free(doc);
	}

	free(xml);

	return 0;
}
