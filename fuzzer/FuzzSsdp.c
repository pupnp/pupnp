#include "config.h"

#include "httpparser.h"
#include "ssdplib.h"
#include "upnpapi.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if EXCLUDE_SSDP == 0
/* Splits a discovery header the way ssdp_handle_ctrlpt_msg() does. That
 * handler only reaches the header parsing when a control point is
 * registered, which needs UpnpInit2() and real network interfaces, so the
 * same calls are made here directly. */
static void parse_target(http_message_t *msg, int header, int is_usn)
{
	memptr value;
	SsdpEvent event;
	char *cmd;

	if (httpmsg_find_hdr(msg, header, &value) == NULL) {
		return;
	}
	cmd = malloc(value.length + 1);
	if (!cmd) {
		return;
	}
	memcpy(cmd, value.buf, value.length);
	cmd[value.length] = '\0';

	if (is_usn) {
		memset(&event, 0, sizeof(event));
		unique_service_name(cmd, &event);
	} else {
		ssdp_request_type(cmd, &event);
	}
	free(cmd);
}
#endif

int LLVMFuzzerInitialize(int *argc, char ***argv)
{
	(void)argc;
	(void)argv;

	ithread_initialize_library();
	ithread_rwlock_init(&GlobalHndRWLock, NULL);
	return 0;
}

extern int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size)
{
#if EXCLUDE_SSDP == 0
	http_parser_t parser;
	parse_status_t status;
	struct sockaddr_storage dest;
	struct sockaddr_in *dest4 = (struct sockaddr_in *)&dest;
	memptr value;
	int expires;
	int as_response;

	/* SSDP datagrams are read into a BUFSIZE buffer. */
	if (Size < 1 || Size > BUFSIZE) {
		return 0;
	}

	/* One leading byte selects a datagram received on the multicast
	 * socket (NOTIFY, M-SEARCH) or a unicast M-SEARCH reply. */
	as_response = Data[0] & 1;
	Data += 1;
	Size -= 1;

	if (as_response) {
		parser_response_init(&parser, HTTPMETHOD_MSEARCH);
	} else {
		parser_request_init(&parser);
	}
	status = parser_append(&parser, (const char *)Data, Size);

	/* Same acceptance rule as start_event_handler(). */
	if (status != PARSE_SUCCESS &&
		!(status == PARSE_FAILURE &&
			parser.msg.method == HTTPMETHOD_NOTIFY &&
			parser.valid_ssdp_notify_hack)) {
		goto exit;
	}

	memset(&dest, 0, sizeof(dest));
	dest4->sin_family = AF_INET;
	dest4->sin_port = htons(1900);
	dest4->sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if (parser.msg.is_request && parser.msg.method == HTTPMETHOD_MSEARCH) {
		ssdp_handle_device_request(&parser.msg, &dest);
	}

	if (httpmsg_find_hdr(&parser.msg, HDR_CACHE_CONTROL, &value) != NULL) {
		matchstr(value.buf, value.length, "%imax-age = %d%0", &expires);
	}
	parse_target(&parser.msg, HDR_NT, 0);
	parse_target(&parser.msg, HDR_ST, 0);
	parse_target(&parser.msg, HDR_USN, 1);

exit:
	httpmsg_destroy(&parser.msg);
#else
	(void)Data;
	(void)Size;
#endif

	return 0;
}
