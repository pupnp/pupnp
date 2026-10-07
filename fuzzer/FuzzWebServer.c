#include "config.h"

#include "httpparser.h"
#include "httpreadwrite.h"
#include "sock.h"
#include "statcodes.h"
#include "upnpapi.h"
#include "webserver.h"

#if EXCLUDE_GENA == 0
	#include "gena.h"
#endif
#if defined(INCLUDE_DEVICE_APIS) && EXCLUDE_SOAP == 0
	#include "soaplib.h"
#endif

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/* Keeps the whole input within the socket buffer, so the single write()
 * below never blocks. */
#define MAX_INPUT_SIZE 65536

static const char description[] =
	"<?xml version=\"1.0\"?>\r\n"
	"<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
	"<specVersion><major>1</major><minor>0</minor></specVersion>"
	"<device>"
	"<deviceType>urn:schemas-upnp-org:device:tvdevice:1</deviceType>"
	"<friendlyName>fuzz</friendlyName>"
	"<UDN>uuid:fuzz-0000-0000-0000-000000000001</UDN>"
	"</device>"
	"</root>\r\n";

/* Mirrors dispatch_request() in miniserver.c. */
static void dispatch(SOCKINFO *info, http_parser_t *parser)
{
	http_message_t *request = &parser->msg;

	/* The miniserver drops requests without a HOST header. */
	if (httpmsg_find_hdr(request, HDR_HOST, NULL) == NULL) {
		return;
	}

	switch (request->method) {
#if defined(INCLUDE_DEVICE_APIS) && EXCLUDE_SOAP == 0
	case SOAPMETHOD_POST:
	case HTTPMETHOD_MPOST:
		soap_device_callback(parser, request, info);
		break;
#endif
#if EXCLUDE_GENA == 0
	case HTTPMETHOD_NOTIFY:
	case HTTPMETHOD_SUBSCRIBE:
	case HTTPMETHOD_UNSUBSCRIBE:
		genaCallback(parser, request, info);
		break;
#endif
#if EXCLUDE_WEB_SERVER == 0
	case HTTPMETHOD_GET:
	case HTTPMETHOD_POST:
	case HTTPMETHOD_HEAD:
	case HTTPMETHOD_SIMPLEGET:
		web_server_callback(parser, request, info);
		break;
#endif
	default:
		http_SendStatusResponse(info,
			HTTP_INTERNAL_SERVER_ERROR,
			request->major_version,
			request->minor_version);
		break;
	}
}

int LLVMFuzzerInitialize(int *argc, char ***argv)
{
	(void)argc;
	(void)argv;

	ithread_initialize_library();
	ithread_rwlock_init(&GlobalHndRWLock, NULL);
#if EXCLUDE_WEB_SERVER == 0
	if (web_server_init() == UPNP_E_SUCCESS) {
		/* The web server takes ownership of the alias content. */
		char *doc = malloc(sizeof(description) - 1);

		if (doc) {
			memcpy(doc, description, sizeof(description) - 1);
			if (web_server_set_alias("description.xml",
				    doc,
				    sizeof(description) - 1,
				    0) != UPNP_E_SUCCESS) {
				free(doc);
			}
		}
	}
#endif

	return 0;
}

extern int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size)
{
	int sv[2];
	SOCKINFO info;
	struct sockaddr_storage peer;
	struct sockaddr_in *peer4 = (struct sockaddr_in *)&peer;
	http_parser_t parser;
	int timeout = 1;
	int http_error_code = 0;

	if (Size < 1 || Size > MAX_INPUT_SIZE) {
		return 0;
	}
	parser_request_init(&parser);
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0) {
		return 0;
	}
	if (write(sv[1], Data, Size) != (ssize_t)Size) {
		close(sv[0]);
		close(sv[1]);
		return 0;
	}
	/* The end of file lets messages without a length complete. Responses
	 * are left unread in sv[1] and dropped when it is closed. */
	shutdown(sv[1], SHUT_WR);

	/* GENA and SOAP pick the handle by the address family of the peer. */
	memset(&peer, 0, sizeof(peer));
	peer4->sin_family = AF_INET;
	peer4->sin_port = htons(49152);
	peer4->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	sock_init_with_ip(&info, sv[0], (struct sockaddr *)&peer);

	/* Same sequence as handle_request() in miniserver.c. */
	if (http_RecvMessage(&info,
		    &parser,
		    HTTPMETHOD_UNKNOWN,
		    &timeout,
		    &http_error_code) == 0) {
		dispatch(&info, &parser);
	} else if (http_error_code > 0) {
		http_SendStatusResponse(&info,
			http_error_code,
			parser.msg.major_version,
			parser.msg.minor_version);
	}

	httpmsg_destroy(&parser.msg);
	sock_destroy(&info, SD_BOTH);
	close(sv[1]);

	return 0;
}
