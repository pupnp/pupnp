/* FuzzDevice: requests to a registered device.
 *
 * FuzzWebServer runs the request dispatch with no device and no virtual
 * directory registered, so SOAP actions, QueryStateVariable, device-side
 * GENA SUBSCRIBE and the virtual directory callbacks are never reached. This
 * target starts the SDK on the loopback interface, registers a device with
 * one service and a virtual directory, and then feeds each input to the same
 * dispatch, through a socket pair, as one request. Nothing is read from the
 * network: the miniserver and SSDP threads stay idle.
 *
 * The application callback answers actions and state variable queries with
 * fixed values, and never accepts a subscription, so no NOTIFY is sent. The
 * subscriptions a SUBSCRIBE adds are dropped after each input. */

#include "config.h"

#include "httpparser.h"
#include "httpreadwrite.h"
#include "service_table.h"
#include "sock.h"
#include "statcodes.h"
#include "upnp.h"
#include "upnpapi.h"
#include "webserver.h"

#if defined(INCLUDE_DEVICE_APIS) && EXCLUDE_GENA == 0 && EXCLUDE_SOAP == 0 && \
	EXCLUDE_WEB_SERVER == 0
	#define FUZZ_DEVICE 1
	#include "gena.h"
	#include "soaplib.h"
#endif

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/* Keeps the whole input within the socket buffer, so the single write()
 * below never blocks. */
#define MAX_INPUT_SIZE 65536

#ifdef FUZZ_DEVICE

static const char description[] =
	"<?xml version=\"1.0\"?>\r\n"
	"<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
	"<specVersion><major>1</major><minor>0</minor></specVersion>"
	"<device>"
	"<deviceType>urn:schemas-upnp-org:device:fuzz:1</deviceType>"
	"<friendlyName>fuzz</friendlyName>"
	"<UDN>uuid:fuzz-0000-0000-0000-000000000002</UDN>"
	"<serviceList><service>"
	"<serviceType>urn:schemas-upnp-org:service:fuzz:1</serviceType>"
	"<serviceId>urn:upnp-org:serviceId:fuzz1</serviceId>"
	"<SCPDURL>/scpd.xml</SCPDURL>"
	"<controlURL>/control</controlURL>"
	"<eventSubURL>/event</eventSubURL>"
	"</service></serviceList>"
	"</device>"
	"</root>\r\n";

/* Content of every file in the virtual directory. */
static const char vd_content[] = "0123456789abcdefghijklmnopqrstuvwxyz"
				 "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789\n";

static UpnpDevice_Handle device_handle = -1;

struct vd_file
{
	off_t pos;
};

static int device_callback(Upnp_EventType type, void *event, void *cookie)
{
	(void)cookie;

	switch (type) {
	case UPNP_CONTROL_ACTION_REQUEST: {
		UpnpActionRequest *req = event;
		const char *name = UpnpActionRequest_get_ActionName_cstr(req);

		if (name && strcmp(name, "Fail") == 0) {
			UpnpActionRequest_set_ErrCode(req, 401);
			UpnpActionRequest_strcpy_ErrStr(req, "Invalid Action");
		} else {
			UpnpActionRequest_set_ErrCode(req, UPNP_E_SUCCESS);
			UpnpActionRequest_set_ActionResult(req,
				ixmlParseBuffer(
					"<u:Response xmlns:u=\"urn:schemas-"
					"upnp-org:service:fuzz:1\">"
					"<Result>ok</Result></u:Response>"));
		}
		break;
	}
	case UPNP_CONTROL_GET_VAR_REQUEST: {
		UpnpStateVarRequest *req = event;
		const char *name =
			UpnpStateVarRequest_get_StateVarName_cstr(req);

		if (name && strcmp(name, "Fail") == 0) {
			UpnpStateVarRequest_set_ErrCode(req, 404);
			UpnpStateVarRequest_strcpy_ErrStr(req, "Invalid Var");
		} else {
			UpnpStateVarRequest_set_ErrCode(req, UPNP_E_SUCCESS);
			/* The setter keeps a copy. */
			UpnpStateVarRequest_set_CurrentVal(req, "42");
		}
		break;
	}
	default:
		/* Subscriptions are not accepted, so no NOTIFY is sent. */
		break;
	}

	return 0;
}

static int vd_get_info(const char *filename,
	UpnpFileInfo *info,
	const void *cookie,
	const void **request_cookie)
{
	(void)cookie;
	(void)request_cookie;
	if (!filename || strstr(filename, "missing")) {
		return -1;
	}
	UpnpFileInfo_set_FileLength(info, (off_t)(sizeof(vd_content) - 1));
	UpnpFileInfo_set_LastModified(info, 0);
	UpnpFileInfo_set_IsDirectory(info, 0);
	UpnpFileInfo_set_IsReadable(info, 1);
	UpnpFileInfo_set_ContentType(info, "text/plain");

	return 0;
}

static UpnpWebFileHandle vd_open(const char *filename,
	enum UpnpOpenFileMode mode,
	const void *cookie,
	const void *request_cookie)
{
	struct vd_file *f;

	(void)filename;
	(void)mode;
	(void)cookie;
	(void)request_cookie;
	f = calloc(1, sizeof(*f));

	return f;
}

static int vd_read(UpnpWebFileHandle h,
	char *buf,
	size_t len,
	const void *cookie,
	const void *request_cookie)
{
	struct vd_file *f = h;
	size_t left;

	(void)cookie;
	(void)request_cookie;
	if (f->pos < 0 || (size_t)f->pos >= sizeof(vd_content) - 1) {
		return 0;
	}
	left = sizeof(vd_content) - 1 - (size_t)f->pos;
	if (len > left) {
		len = left;
	}
	memcpy(buf, vd_content + f->pos, len);
	f->pos += (off_t)len;

	return (int)len;
}

static int vd_write(UpnpWebFileHandle h,
	char *buf,
	size_t len,
	const void *cookie,
	const void *request_cookie)
{
	(void)h;
	(void)buf;
	(void)cookie;
	(void)request_cookie;

	return (int)len;
}

static int vd_seek(UpnpWebFileHandle h,
	off_t offset,
	int origin,
	const void *cookie,
	const void *request_cookie)
{
	struct vd_file *f = h;
	off_t base;

	(void)cookie;
	(void)request_cookie;
	switch (origin) {
	case SEEK_SET:
		base = 0;
		break;
	case SEEK_CUR:
		base = f->pos;
		break;
	case SEEK_END:
		base = (off_t)(sizeof(vd_content) - 1);
		break;
	default:
		return -1;
	}
	if (offset < -base) {
		return -1;
	}
	f->pos = base + offset;

	return 0;
}

static int vd_close(
	UpnpWebFileHandle h, const void *cookie, const void *request_cookie)
{
	(void)cookie;
	(void)request_cookie;
	free(h);

	return 0;
}

/* Name of an up loopback interface with an IPv4 address ("lo", "lo0"). */
static int find_loopback(char *name, size_t size)
{
	struct ifaddrs *ifap;
	struct ifaddrs *ifa;
	int found = 0;

	if (getifaddrs(&ifap) != 0) {
		return 0;
	}
	for (ifa = ifap; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET &&
			(ifa->ifa_flags & IFF_LOOPBACK) &&
			(ifa->ifa_flags & IFF_UP)) {
			snprintf(name, size, "%s", ifa->ifa_name);
			found = 1;
			break;
		}
	}
	freeifaddrs(ifap);

	return found;
}

/* Drops the subscriptions that the last input added. */
static void reset_subscriptions(void)
{
	struct Handle_Info *info;
	service_info *service;

	HandleLock(__FILE__, __LINE__);
	if (GetHandleInfo(device_handle, &info) == HND_DEVICE) {
		for (service = info->ServiceTable.serviceList; service;
			service = service->next) {
			freeSubscriptionList(service->subscriptionList);
			service->subscriptionList = NULL;
			service->TotalSubscriptions = 0;
		}
	}
	HandleUnlock(__FILE__, __LINE__);
}

/* Mirrors dispatch_request() in miniserver.c. */
static void dispatch(SOCKINFO *info, http_parser_t *parser)
{
	http_message_t *request = &parser->msg;

	/* The miniserver drops requests without a HOST header. */
	if (httpmsg_find_hdr(request, HDR_HOST, NULL) == NULL) {
		return;
	}

	switch (request->method) {
	case SOAPMETHOD_POST:
	case HTTPMETHOD_MPOST:
		soap_device_callback(parser, request, info);
		break;
	case HTTPMETHOD_NOTIFY:
	case HTTPMETHOD_SUBSCRIBE:
	case HTTPMETHOD_UNSUBSCRIBE:
		genaCallback(parser, request, info);
		break;
	case HTTPMETHOD_GET:
	case HTTPMETHOD_POST:
	case HTTPMETHOD_HEAD:
	case HTTPMETHOD_SIMPLEGET:
		web_server_callback(parser, request, info);
		break;
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
	char ifname[IF_NAMESIZE + 1];
	int rc;

	(void)argc;
	(void)argv;

	/* Without a device the target cannot reach what it is for: fail
	 * loudly rather than fuzz nothing. */
	if (!find_loopback(ifname, sizeof(ifname))) {
		fprintf(stderr, "FuzzDevice: no IPv4 loopback interface\n");
		abort();
	}
	rc = UpnpInit2(ifname, 0);
	if (rc != UPNP_E_SUCCESS) {
		fprintf(stderr, "FuzzDevice: UpnpInit2(%s) = %d\n", ifname, rc);
		abort();
	}
	if (UpnpVirtualDir_set_GetInfoCallback(vd_get_info) != UPNP_E_SUCCESS ||
		UpnpVirtualDir_set_OpenCallback(vd_open) != UPNP_E_SUCCESS ||
		UpnpVirtualDir_set_ReadCallback(vd_read) != UPNP_E_SUCCESS ||
		UpnpVirtualDir_set_WriteCallback(vd_write) != UPNP_E_SUCCESS ||
		UpnpVirtualDir_set_SeekCallback(vd_seek) != UPNP_E_SUCCESS ||
		UpnpVirtualDir_set_CloseCallback(vd_close) != UPNP_E_SUCCESS ||
		UpnpAddVirtualDir("/vd", NULL, NULL) != UPNP_E_SUCCESS) {
		fprintf(stderr, "FuzzDevice: virtual directory setup failed\n");
		abort();
	}
	rc = UpnpRegisterRootDevice2(UPNPREG_BUF_DESC,
		description,
		sizeof(description) - 1,
		1,
		device_callback,
		NULL,
		&device_handle);
	if (rc != UPNP_E_SUCCESS) {
		fprintf(stderr,
			"FuzzDevice: UpnpRegisterRootDevice2 = %d\n",
			rc);
		abort();
	}

	return 0;
}

#endif /* FUZZ_DEVICE */

extern int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size)
{
#ifdef FUZZ_DEVICE
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

	/* A local control point: GENA and SOAP pick the handle by the address
	 * family of the peer, and GENA checks callback URLs against it. */
	memset(&peer, 0, sizeof(peer));
	peer4->sin_family = AF_INET;
	peer4->sin_port = htons(49153);
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
	reset_subscriptions();
#else
	(void)Data;
	(void)Size;
#endif

	return 0;
}
