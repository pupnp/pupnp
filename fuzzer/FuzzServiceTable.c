#include "service_table.h"

/* "ixml.h" is not included here on purpose: service_table.h already includes
 * it, together with the other headers this target needs (config.h, upnp.h,
 * LinkedList.h). */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size)
{
/* service_table, getServiceTable() and freeServiceTable() are only declared
 * when the device APIs and GENA are compiled in. Without this guard the
 * target fails to build on a client-only configuration, for instance
 * cmake -DFUZZER=ON -DUPNP_ENABLE_DEVICE_API=OFF. */
#if defined(INCLUDE_DEVICE_APIS) && EXCLUDE_GENA == 0
	IXML_Document *doc = NULL;
	service_table table;
	char *xml;

	/* The upper bound is a harness-side safety net only. The input size is
	 * meant to be driven by the runner, through libFuzzer's -max_len flag:
	 * on the command line, as in
	 *     ./FuzzServiceTable corpus/ -max_len=65536
	 * or, under OSS-Fuzz, through a FuzzServiceTable.options file holding
	 *     [libfuzzer]
	 *     max_len = 65536
	 * Neither is set today, so libFuzzer's own default of 4096 bytes
	 * applies and the test below never actually fires. */
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
#else
	(void)Data;
	(void)Size;
#endif

	return 0;
}
