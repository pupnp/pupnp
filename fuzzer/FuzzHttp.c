#include "httpparser.h"
#include <stddef.h>
#include <stdint.h>

extern int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size)
{
	http_parser_t parser;
	int as_response;

	if (Size < 1) {
		return 0;
	}

	/* One leading byte selects request vs response parsing. */
	as_response = Data[0] & 1;
	Data += 1;
	Size -= 1;

	if (as_response) {
		parser_response_init(&parser, HTTPMETHOD_GET);
	} else {
		parser_request_init(&parser);
	}

	parser_append(&parser, (const char *)Data, Size);

	httpmsg_destroy(&parser.msg);
	return 0;
}
