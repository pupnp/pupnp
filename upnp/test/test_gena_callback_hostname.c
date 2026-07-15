/* test_gena_callback_hostname.c
 *
 * Regression test for OSS-Fuzz issue 531780528.
 *
 * create_url_list() (called while handling an inbound GENA SUBSCRIBE
 * request's Callback header) used to call getaddrinfo() on any callback
 * host that wasn't a literal IP address. Since the Callback header is
 * subscriber-supplied, this let a remote peer block a worker thread on DNS
 * resolution -- a denial-of-service reachable without authentication
 * (CWE-400). It also made gena_validate_delivery_urls() (the CVE-2020-12695
 * "CallStranger" same-subnet check) vulnerable to DNS rebinding: the
 * resolved address used for the subnet check could differ from the address
 * actually used to deliver the NOTIFY later.
 *
 * Fix: create_url_list() now requires delivery/callback URLs to use a
 * literal IP address; hostnames are rejected the same way any other
 * malformed callback URL is, with no DNS lookup ever attempted.
 *
 * Test cases:
 *   A. Callback URL using a hostname ("localhost") instead of a literal IP
 *      -> must be rejected (HTTP 412), and must return promptly (no DNS
 *      blocking).
 *   B. Callback URL using the server's own literal IP (the existing,
 *      supported pattern) -> must still be accepted (HTTP 200), guarding
 *      against a regression in the CallStranger same-subnet check.
 */

#include "Callback.h"
#include "posix_overwrites.h" /* IWYU pragma: keep */
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

/* Path the registered service listens on for SUBSCRIBE. */
#define EVENT_URL_PATH "/event/callbackhost"

/* Minimal device description with a single evented service. */
static const char DEVICE_DESC[] =
	"<?xml version=\"1.0\"?>"
	"<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
	"<specVersion><major>1</major><minor>0</minor></specVersion>"
	"<device>"
	"<deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType>"
	"<friendlyName>callbackhost</friendlyName>"
	"<manufacturer>Test</manufacturer>"
	"<modelName>Test</modelName>"
	"<UDN>uuid:callbackhost-0000-0000-0000-000000000000</UDN>"
	"<serviceList><service>"
	"<serviceType>urn:schemas-upnp-org:service:Basic:1</serviceType>"
	"<serviceId>urn:upnp-org:serviceId:Basic</serviceId>"
	"<SCPDURL>/scpd.xml</SCPDURL>"
	"<controlURL>/control/callbackhost</controlURL>"
	"<eventSubURL>" EVENT_URL_PATH "</eventSubURL>"
	"</service></serviceList>"
	"</device>"
	"</root>";

static int device_callback(Upnp_EventType t, void *e, void *c)
{
	(void)t;
	(void)e;
	(void)c;
	return 0;
}

#ifndef _WIN32

/*
 * Send a SUBSCRIBE request with the given Callback URL (verbatim, already
 * including the surrounding "<...>"). Returns the HTTP status code, or -1
 * on socket/parse error. Also reports elapsed wall-clock time so a
 * regression back to synchronous DNS resolution shows up as a stall, not
 * just a status-code mismatch.
 */
static int send_subscribe(const char *server_ip,
	unsigned short server_port,
	const char *callback_url,
	double *elapsed_ms)
{
	int fd = -1;
	int status = -1;
	char req[512];
	int req_len;
	struct sockaddr_in addr;
	char resp[512];
	struct timeval tv, t0, t1;
	ssize_t n;

	req_len = snprintf(req,
		sizeof(req),
		"SUBSCRIBE " EVENT_URL_PATH " HTTP/1.1\r\n"
		"Host: %s:%u\r\n"
		"NT: upnp:event\r\n"
		"Timeout: Second-1800\r\n"
		"Connection: close\r\n"
		"Callback: %s\r\n\r\n",
		server_ip,
		(unsigned)server_port,
		callback_url);
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

	gettimeofday(&t0, NULL);
	if (send(fd, req, (size_t)req_len, 0) < 0) {
		perror("send");
		goto done;
	}

	n = recv(fd, resp, sizeof(resp) - 1, 0);
	gettimeofday(&t1, NULL);
	*elapsed_ms = (double)(t1.tv_sec - t0.tv_sec) * 1000.0 +
		      (double)(t1.tv_usec - t0.tv_usec) / 1000.0;
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

/* Test A: a hostname-based Callback URL must be rejected (HTTP 412) and
 * must not stall the request handler. */
static int test_hostname_callback_rejected(
	const char *server_ip, unsigned short server_port)
{
	int status;
	double elapsed_ms = 0.0;

	status = send_subscribe(
		server_ip, server_port, "<http://localhost:1/cb>", &elapsed_ms);

	printf("Test A: SUBSCRIBE with hostname Callback URL -> HTTP %d "
	       "(%.1f ms)\n",
		status,
		elapsed_ms);

	if (status != 412) {
		fprintf(stderr,
			"Test A FAIL: expected HTTP 412 (hostname callback "
			"URLs must be rejected), got %d.\n",
			status);
		return -1;
	}
	if (elapsed_ms > 2000.0) {
		fprintf(stderr,
			"Test A FAIL: request took %.1f ms -- looks like it "
			"blocked on DNS resolution instead of rejecting "
			"immediately.\n",
			elapsed_ms);
		return -1;
	}

	puts("Test A PASS: hostname Callback URL rejected promptly (HTTP "
	     "412).");
	return 0;
}

/* Test B: the existing, supported pattern -- a literal-IP Callback URL on
 * the same subnet as the device -- must still be accepted. */
static int test_literal_ip_callback_accepted(
	const char *server_ip, unsigned short server_port)
{
	int status;
	double elapsed_ms = 0.0;
	char callback_url[128];

	snprintf(callback_url,
		sizeof(callback_url),
		"<http://%s:1/cb>",
		server_ip);

	status = send_subscribe(
		server_ip, server_port, callback_url, &elapsed_ms);

	printf("Test B: SUBSCRIBE with literal-IP Callback URL -> HTTP %d "
	       "(%.1f ms)\n",
		status,
		elapsed_ms);

	if (status != 200) {
		fprintf(stderr,
			"Test B FAIL: expected HTTP 200 (literal-IP callback "
			"URLs must still work), got %d.\n",
			status);
		return -1;
	}

	puts("Test B PASS: literal-IP Callback URL still accepted (HTTP "
	     "200).");
	return 0;
}

#endif /* !_WIN32 */

int main(void)
{
#ifdef _WIN32
	puts("SKIP: raw POSIX socket test not supported on Windows.");
	return EXIT_SUCCESS;
#else
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

	if (test_hostname_callback_rejected(server_ip, server_port) != 0)
		result = 1;

	if (test_literal_ip_callback_accepted(server_ip, server_port) != 0)
		result = 1;

	UpnpUnRegisterRootDevice(handle);
	UpnpFinish();
	return result ? EXIT_FAILURE : EXIT_SUCCESS;
#endif /* !_WIN32 */
}
