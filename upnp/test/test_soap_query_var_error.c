/* test_soap_query_var_error.c
 *
 * Regression test: the SOAP fault for a QueryStateVariable request must carry
 * the error description that the device callback set.
 *
 * handle_query_variable() took the error description from the
 * UpnpStateVarRequest of the callback and deleted the request before
 * send_error_response() wrote the description into the fault, so the fault
 * was made from freed memory: AddressSanitizer reports a heap-use-after-free,
 * and without it the client gets whatever the allocator left in the freed
 * block.
 *
 * The device callback answers the variable "Denied" with a value, error code
 * 606 and a description, and the variable "Status" with a value only. The
 * first must give a 500 with that code and that description, the second a 200
 * with the value.
 *
 * The stack runs on the loopback interface, so no network is needed.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32

	#include "upnp.h"

	#include <arpa/inet.h>
	#include <ifaddrs.h>
	#include <net/if.h>
	#include <netinet/in.h>
	#include <sys/socket.h>
	#include <sys/time.h>
	#include <unistd.h>

	#ifndef MSG_NOSIGNAL
		#define MSG_NOSIGNAL 0
	#endif

	#define CONTROL_URL_PATH "/control/qve"
	#define VAR_VALUE "42"
	#define ERR_CODE 606
	#define ERR_DESC "Action not authorized for this control point"

static const char DEVICE_DESC[] =
	"<?xml version=\"1.0\"?>"
	"<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
	"<specVersion><major>1</major><minor>0</minor></specVersion>"
	"<device>"
	"<deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType>"
	"<friendlyName>qve</friendlyName>"
	"<manufacturer>Test</manufacturer>"
	"<modelName>Test</modelName>"
	"<UDN>uuid:qve00000-0000-0000-0000-000000000000</UDN>"
	"<serviceList><service>"
	"<serviceType>urn:schemas-upnp-org:service:Basic:1</serviceType>"
	"<serviceId>urn:upnp-org:serviceId:Basic</serviceId>"
	"<SCPDURL>/scpd.xml</SCPDURL>"
	"<controlURL>" CONTROL_URL_PATH "</controlURL>"
	"<eventSubURL>/event/qve</eventSubURL>"
	"</service></serviceList>"
	"</device>"
	"</root>";

static int device_callback(Upnp_EventType t, void *e, void *c)
{
	UpnpStateVarRequest *req = e;
	const char *name;

	(void)c;
	if (t != UPNP_CONTROL_GET_VAR_REQUEST)
		return 0;
	name = UpnpStateVarRequest_get_StateVarName_cstr(req);
	UpnpStateVarRequest_set_CurrentVal(req, VAR_VALUE);
	if (name && strcmp(name, "Denied") == 0) {
		UpnpStateVarRequest_set_ErrCode(req, ERR_CODE);
		UpnpStateVarRequest_strcpy_ErrStr(req, ERR_DESC);
	}

	return 0;
}

/* Copies the name of an up IPv4 loopback interface. */
static int find_loopback(char *name, size_t name_size)
{
	struct ifaddrs *ifap;
	struct ifaddrs *ifa;
	int found = 0;

	if (getifaddrs(&ifap) != 0)
		return 0;
	for (ifa = ifap; ifa != NULL; ifa = ifa->ifa_next) {
		if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET ||
			!(ifa->ifa_flags & IFF_LOOPBACK) ||
			!(ifa->ifa_flags & IFF_UP))
			continue;
		snprintf(name, name_size, "%s", ifa->ifa_name);
		found = 1;
		break;
	}
	freeifaddrs(ifap);

	return found;
}

/* Sends a QueryStateVariable request for var and reads the whole answer into
 * resp. Returns the status code, or -1. */
static int query(const char *ip,
	unsigned short port,
	const char *var,
	char *resp,
	size_t resp_size)
{
	int sock;
	struct sockaddr_in addr;
	struct timeval tv;
	char body[512];
	char req[1024];
	ssize_t n;
	size_t total = 0;
	int status = -1;

	snprintf(body,
		sizeof body,
		"<?xml version=\"1.0\"?>"
		"<s:Envelope "
		"xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
		"s:encodingStyle="
		"\"http://schemas.xmlsoap.org/soap/encoding/\">"
		"<s:Body>"
		"<u:QueryStateVariable "
		"xmlns:u=\"urn:schemas-upnp-org:control-1-0\">"
		"<u:varName>%s</u:varName>"
		"</u:QueryStateVariable>"
		"</s:Body>"
		"</s:Envelope>",
		var);
	/* The miniserver only accepts a numeric ip:port Host header. */
	snprintf(req,
		sizeof req,
		"POST " CONTROL_URL_PATH " HTTP/1.1\r\n"
		"Host: %s:%u\r\n"
		"Content-Type: text/xml; charset=\"utf-8\"\r\n"
		"SOAPACTION: "
		"\"urn:schemas-upnp-org:control-1-0#QueryStateVariable\"\r\n"
		"Content-Length: %u\r\n"
		"Connection: close\r\n\r\n"
		"%s",
		ip,
		port,
		(unsigned)strlen(body),
		body);

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
		return -1;
	tv.tv_sec = 5;
	tv.tv_usec = 0;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
	memset(&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	inet_pton(AF_INET, ip, &addr.sin_addr);
	if (connect(sock, (struct sockaddr *)&addr, sizeof addr) != 0) {
		close(sock);
		return -1;
	}
	send(sock, req, strlen(req), MSG_NOSIGNAL);
	while (total < resp_size - 1 &&
		(n = recv(sock, resp + total, resp_size - 1 - total, 0)) > 0) {
		total += (size_t)n;
	}
	resp[total] = '\0';
	close(sock);
	if (total > 0)
		sscanf(resp, "HTTP/%*d.%*d %d", &status);

	return status;
}

int main(void)
{
	char name[64];
	char resp[2048];
	const char *ip;
	unsigned short port;
	UpnpDevice_Handle handle = -1;
	int status;
	int rc;
	int err = 0;

	if (!find_loopback(name, sizeof name)) {
		puts("SKIP: no IPv4 loopback interface is up");
		return EXIT_SUCCESS;
	}
	rc = UpnpInit2(name, 0);
	if (rc != UPNP_E_SUCCESS) {
		fprintf(stderr, "UpnpInit2(\"%s\") failed (%d)\n", name, rc);
		return EXIT_FAILURE;
	}
	ip = UpnpGetServerIpAddress();
	port = UpnpGetServerPort();
	rc = UpnpRegisterRootDevice2(UPNPREG_BUF_DESC,
		DEVICE_DESC,
		sizeof DEVICE_DESC - 1,
		1,
		device_callback,
		NULL,
		&handle);
	if (rc != UPNP_E_SUCCESS) {
		fprintf(stderr, "UpnpRegisterRootDevice2 failed (%d)\n", rc);
		UpnpFinish();
		return EXIT_FAILURE;
	}

	status = query(ip, port, "Status", resp, sizeof resp);
	if (status != 200 ||
		strstr(resp, "<return>" VAR_VALUE "</return>") == NULL) {
		fprintf(stderr,
			"Status: expected 200 and the value, got:\n%s\n",
			resp);
		err = 1;
	}

	status = query(ip, port, "Denied", resp, sizeof resp);
	if (status != 500 ||
		strstr(resp, "<errorCode>606</errorCode>") == NULL ||
		strstr(resp,
			"<errorDescription>" ERR_DESC
			"</errorDescription>") == NULL) {
		fprintf(stderr,
			"Denied: expected 500 with the error code and "
			"description of the callback, got:\n%s\n",
			resp);
		err = 1;
	}

	UpnpUnRegisterRootDevice(handle);
	UpnpFinish();
	if (!err)
		puts("PASS");

	return err ? EXIT_FAILURE : EXIT_SUCCESS;
}

#else /* _WIN32 */

int main(void)
{
	puts("SKIP: test uses POSIX sockets (not available on Windows).");
	return EXIT_SUCCESS;
}

#endif /* _WIN32 */
