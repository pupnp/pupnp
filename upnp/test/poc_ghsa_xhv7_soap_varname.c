/* poc_ghsa_xhv7_soap_varname.c
 *
 * Regression test for GHSA-7mx2-6v7x-xhv7: unauthenticated remote crash in
 * SOAP QueryStateVariable handling.
 *
 * check_soap_request() in soap_device.c takes the first child of the
 * <QueryStateVariable> action node and passes its local name straight to
 * strcmp(). ixmlNode_getLocalName() returns NULL for text and CDATA nodes,
 * so a request whose action node starts with a text or CDATA node instead
 * of a <varName> element made the worker thread dereference NULL and took
 * the whole process down (CWE-476). The request is handled inside the
 * library, before the device callback runs, and needs no authentication.
 *
 * Test cases:
 *   A. Valid <varName> element -> HTTP 200 (baseline, also run last to
 *      prove the server survived).
 *   B. Text node where <varName> is expected -> HTTP 400.
 *   C. CDATA node where <varName> is expected -> HTTP 400.
 *
 * Pre-fix: the process crashes (SIGSEGV) on case B.
 * Post-fix: B and C are rejected with 400 and the server keeps running.
 *
 * regression: GHSA-7mx2-6v7x-xhv7
 */

#include "upnp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
	#include <arpa/inet.h>
	#include <netinet/in.h>
	#include <sys/socket.h>
	#include <sys/time.h>
	#include <unistd.h>
#endif

#define CONTROL_URL_PATH "/control/xhv7"

/* Minimal device description with a single service. */
static const char DEVICE_DESC[] =
	"<?xml version=\"1.0\"?>"
	"<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
	"<specVersion><major>1</major><minor>0</minor></specVersion>"
	"<device>"
	"<deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType>"
	"<friendlyName>xhv7</friendlyName>"
	"<manufacturer>Test</manufacturer>"
	"<modelName>Test</modelName>"
	"<UDN>uuid:xhv7-0000-0000-0000-000000000000</UDN>"
	"<serviceList><service>"
	"<serviceType>urn:schemas-upnp-org:service:Basic:1</serviceType>"
	"<serviceId>urn:upnp-org:serviceId:Basic</serviceId>"
	"<SCPDURL>/scpd.xml</SCPDURL>"
	"<controlURL>" CONTROL_URL_PATH "</controlURL>"
	"<eventSubURL>/event/xhv7</eventSubURL>"
	"</service></serviceList>"
	"</device>"
	"</root>";

#define SOAP_PREFIX \
	"<?xml version=\"1.0\"?>" \
	"<s:Envelope " \
	"xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" " \
	"s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">" \
	"<s:Body>" \
	"<u:QueryStateVariable " \
	"xmlns:u=\"urn:schemas-upnp-org:control-1-0\">"

#define SOAP_SUFFIX \
	"</u:QueryStateVariable>" \
	"</s:Body>" \
	"</s:Envelope>"

static int device_callback(Upnp_EventType t, void *e, void *c)
{
	(void)c;
	if (t == UPNP_CONTROL_GET_VAR_REQUEST) {
		UpnpStateVarRequest_set_CurrentVal(
			(UpnpStateVarRequest *)e, "42");
	}
	return 0;
}

#ifndef _WIN32

/* POST a QueryStateVariable SOAP body to the control URL. Returns the HTTP
 * status code, or -1 on socket/parse error. */
static int send_query(
	const char *server_ip, unsigned short server_port, const char *body)
{
	int fd = -1;
	int status = -1;
	char req[2048];
	int req_len;
	struct sockaddr_in addr;
	char resp[512];
	struct timeval tv;
	ssize_t n;

	req_len = snprintf(req,
		sizeof(req),
		"POST " CONTROL_URL_PATH " HTTP/1.1\r\n"
		"Host: %s:%u\r\n"
		"Content-Type: text/xml; charset=\"utf-8\"\r\n"
		"SOAPACTION: "
		"\"urn:schemas-upnp-org:control-1-0#QueryStateVariable\"\r\n"
		"Content-Length: %u\r\n"
		"Connection: close\r\n\r\n"
		"%s",
		server_ip,
		(unsigned)server_port,
		(unsigned)strlen(body),
		body);
	if (req_len < 0 || (size_t)req_len >= sizeof(req)) {
		fprintf(stderr, "request too large\n");
		return -1;
	}

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		perror("socket");
		goto done;
	}

	tv.tv_sec = 5;
	tv.tv_usec = 0;
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(server_port);
	if (inet_pton(AF_INET, server_ip, &addr.sin_addr) != 1) {
		fprintf(stderr, "inet_pton(%s) failed\n", server_ip);
		goto done;
	}
	if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
		perror("connect");
		goto done;
	}

	if (send(fd, req, (size_t)req_len, 0) < 0) {
		perror("send");
		goto done;
	}

	n = recv(fd, resp, sizeof(resp) - 1, 0);
	if (n > 0) {
		resp[n] = '\0';
		if (sscanf(resp, "HTTP/1.%*d %d", &status) != 1)
			status = -1;
	}

done:
	if (fd >= 0)
		close(fd);
	return status;
}

static int check_query(const char *name,
	const char *server_ip,
	unsigned short server_port,
	const char *body,
	int expected)
{
	int status = send_query(server_ip, server_port, body);

	printf("%s -> HTTP %d\n", name, status);
	if (status != expected) {
		fprintf(stderr,
			"%s FAIL: expected HTTP %d, got %d.\n",
			name,
			expected,
			status);
		return -1;
	}
	printf("%s PASS\n", name);
	return 0;
}

#endif /* !_WIN32 */

int main(void)
{
#ifdef _WIN32
	puts("SKIP: raw POSIX socket test not supported on Windows.");
	return EXIT_SUCCESS;
#else
	static const char valid_body[] =
		SOAP_PREFIX "<u:varName>Status</u:varName>" SOAP_SUFFIX;
	static const char text_body[] = SOAP_PREFIX "Status" SOAP_SUFFIX;
	static const char cdata_body[] =
		SOAP_PREFIX "<![CDATA[Status]]>" SOAP_SUFFIX;
	int rc;
	UpnpDevice_Handle handle = -1;
	const char *server_ip;
	unsigned short server_port;
	int result = 0;

	rc = UpnpInit2(NULL, 0);
	if (rc != UPNP_E_SUCCESS) {
		fprintf(stderr,
			"UpnpInit2 failed (%d); skipping (no network?)\n",
			rc);
		return EXIT_SUCCESS;
	}

	server_ip = UpnpGetServerIpAddress();
	server_port = UpnpGetServerPort();
	if (!server_ip || !server_port) {
		fprintf(stderr, "Could not determine server address\n");
		UpnpFinish();
		return EXIT_FAILURE;
	}
	printf("Server: %s:%u\n", server_ip, server_port);

	rc = UpnpRegisterRootDevice2(UPNPREG_BUF_DESC,
		DEVICE_DESC,
		sizeof(DEVICE_DESC) - 1,
		1,
		device_callback,
		NULL,
		&handle);
	if (rc != UPNP_E_SUCCESS) {
		fprintf(stderr, "UpnpRegisterRootDevice2 failed: %d\n", rc);
		UpnpFinish();
		return EXIT_FAILURE;
	}

	if (check_query("Test A (valid varName, baseline)",
		    server_ip,
		    server_port,
		    valid_body,
		    200) != 0) {
		fprintf(stderr, "Baseline failed; skipping (environment?)\n");
		UpnpUnRegisterRootDevice(handle);
		UpnpFinish();
		return EXIT_SUCCESS;
	}
	if (check_query("Test B (text node)",
		    server_ip,
		    server_port,
		    text_body,
		    400) != 0)
		result = 1;
	if (check_query("Test C (CDATA node)",
		    server_ip,
		    server_port,
		    cdata_body,
		    400) != 0)
		result = 1;
	if (check_query("Test A (valid varName, server still alive)",
		    server_ip,
		    server_port,
		    valid_body,
		    200) != 0)
		result = 1;

	UpnpUnRegisterRootDevice(handle);
	UpnpFinish();
	return result ? EXIT_FAILURE : EXIT_SUCCESS;
#endif /* !_WIN32 */
}
