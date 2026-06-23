/* test_gena_subscribe_dos.c
 *
 * Regression test for issue #435:
 * Flooding a UPnP device with SUBSCRIBE requests exhausts memory.
 *
 * The reporter's PoC (Python) sends 99999 SUBSCRIBE requests, each carrying a
 * ~65536-byte Callback header with a unique random suffix.  Memory grows
 * without bound for two compounding reasons:
 *
 *   1. Per-subscription allocation: create_url_list() calls
 *      malloc(callback_header_len + 1) with no upper bound.  A 65536-byte
 *      header wastes 64 KB per accepted subscription.
 *
 *   2. Unlimited subscriptions: MaxSubscriptions defaults to UPNP_INFINITE,
 *      so subscriptions accumulate for up to 1800 s each regardless of how
 *      many arrive.  Even with small callback URLs an attacker can saturate
 *      memory over time.
 *
 * Fixes applied:
 *   1. Callback headers longer than MAX_SUBSCRIPTION_CALLBACK_HEADER_SIZE are
 *      rejected with HTTP 412 Precondition Failed before any allocation.
 *   2. MaxSubscriptions now defaults to DEFAULT_MAX_SUBSCRIPTIONS instead of
 *      UPNP_INFINITE, capping the subscription list per service.
 *
 * Test cases:
 *   A. Oversized Callback header:
 *      Before fix 1 → HTTP 200 (accepted, 65536-byte alloc) → FAIL
 *      After  fix 1 → HTTP 412 (rejected before alloc)      → PASS
 *
 *   B. Subscription count limit (MaxSubscriptions = SMALL_LIMIT):
 *      Before fix 2 → all N+1 requests return HTTP 200      → FAIL
 *      After  fix 2 → requests 1..N return 200, N+1 → 500  → PASS
 *
 * The Callback URL always uses the server's own IP so that
 * gena_validate_delivery_urls() passes the subnet check.
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
#define EVENT_URL_PATH "/event/dos435"

/* Callback header size that matches the reporter's PoC. */
#define LARGE_CB_PATH_LEN 0x10000u /* 65536 bytes */

/* How many subscriptions to accept before capping in test B. */
#define SMALL_LIMIT 3

/* Minimal device description with a single evented service. */
static const char DEVICE_DESC[] =
	"<?xml version=\"1.0\"?>"
	"<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
	"<specVersion><major>1</major><minor>0</minor></specVersion>"
	"<device>"
	"<deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType>"
	"<friendlyName>dos435</friendlyName>"
	"<manufacturer>Test</manufacturer>"
	"<modelName>Test</modelName>"
	"<UDN>uuid:dos435-0000-0000-0000-000000000000</UDN>"
	"<serviceList><service>"
	"<serviceType>urn:schemas-upnp-org:service:Basic:1</serviceType>"
	"<serviceId>urn:upnp-org:serviceId:Basic</serviceId>"
	"<SCPDURL>/scpd.xml</SCPDURL>"
	"<controlURL>/control/dos435</controlURL>"
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
 * Send one SUBSCRIBE request.
 * cb_path is appended after "http://server_ip:1/" in the Callback header.
 * Returns the HTTP status code, or -1 on socket/parse error.
 */
static int send_subscribe(const char *server_ip,
	unsigned short server_port,
	const char *cb_path,
	size_t cb_path_len)
{
	int fd = -1;
	int status = -1;
	char *req = NULL;
	size_t cap, off;
	struct sockaddr_in addr;
	char resp[512];
	struct timeval tv;
	ssize_t n;

	const char *hdr_fmt = "SUBSCRIBE " EVENT_URL_PATH " HTTP/1.1\r\n"
			      "Host: %s:%u\r\n"
			      "NT: upnp:event\r\n"
			      "Timeout: Second-1800\r\n"
			      "Connection: close\r\n"
			      "Callback: <http://%s:1/";

	cap = 256 + cb_path_len + 8;
	req = malloc(cap);
	if (!req) {
		perror("malloc");
		return -1;
	}

	off = (size_t)snprintf(
		req, cap, hdr_fmt, server_ip, (unsigned)server_port, server_ip);
	memcpy(req + off, cb_path, cb_path_len);
	off += cb_path_len;
	off += (size_t)snprintf(req + off, cap - off, ">\r\n\r\n");

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
	if (send(fd, req, off, 0) < 0) {
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
	free(req);
	return status;
}

/*
 * Test A: Callback header larger than MAX_SUBSCRIPTION_CALLBACK_HEADER_SIZE
 * must be rejected with HTTP 412 without allocating storage.
 */
static int test_oversized_callback(
	const char *server_ip, unsigned short server_port)
{
	char *large_path;
	int status;

	large_path = malloc(LARGE_CB_PATH_LEN);
	if (!large_path) {
		perror("malloc");
		return -1;
	}
	memset(large_path, 'a', LARGE_CB_PATH_LEN);

	status = send_subscribe(
		server_ip, server_port, large_path, LARGE_CB_PATH_LEN);
	free(large_path);

	printf("Test A: SUBSCRIBE with %u-byte Callback path -> HTTP %d\n",
		LARGE_CB_PATH_LEN,
		status);

	if (status == 412) {
		puts("Test A PASS: oversized Callback rejected (HTTP 412).");
		return 0;
	}
	fprintf(stderr,
		"Test A FAIL: expected HTTP 412, got %d.\n"
		"  HTTP 200 means a %u-byte allocation was accepted (issue "
		"#435).\n",
		status,
		LARGE_CB_PATH_LEN);
	return -1;
}

/*
 * Test B: After MaxSubscriptions subscriptions are registered the next
 * SUBSCRIBE must be rejected with HTTP 500.
 * Uses UpnpSetMaxSubscriptions() to force a small limit so the test runs fast.
 */
static int test_subscription_count_limit(UpnpDevice_Handle handle,
	const char *server_ip,
	unsigned short server_port)
{
	int i, status;
	char path[32];

	if (UpnpSetMaxSubscriptions(handle, SMALL_LIMIT) != UPNP_E_SUCCESS) {
		fprintf(stderr,
			"Test B: UpnpSetMaxSubscriptions(%d) failed\n",
			SMALL_LIMIT);
		return -1;
	}

	/* Fill the subscription list up to the limit. */
	for (i = 1; i <= SMALL_LIMIT; i++) {
		snprintf(path, sizeof(path), "cb%d", i);
		status = send_subscribe(
			server_ip, server_port, path, strlen(path));
		printf("Test B: subscription %d/%d -> HTTP %d\n",
			i,
			SMALL_LIMIT,
			status);
		if (status != 200) {
			fprintf(stderr,
				"Test B FAIL: subscription %d should have been "
				"accepted (HTTP 200), got %d.\n",
				i,
				status);
			return -1;
		}
	}

	/* One more must be rejected. */
	snprintf(path, sizeof(path), "cb%d", SMALL_LIMIT + 1);
	status = send_subscribe(server_ip, server_port, path, strlen(path));
	printf("Test B: subscription %d (over limit) -> HTTP %d\n",
		SMALL_LIMIT + 1,
		status);

	if (status == 500) {
		puts("Test B PASS: subscription beyond limit rejected "
		     "(HTTP 500).");
		return 0;
	}
	fprintf(stderr,
		"Test B FAIL: expected HTTP 500 when MaxSubscriptions=%d is "
		"exceeded, got %d.\n"
		"  UPNP_INFINITE default means subscriptions accumulate "
		"without bound (issue #435).\n",
		SMALL_LIMIT,
		status);
	return -1;
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

	if (test_oversized_callback(server_ip, server_port) != 0)
		result = 1;

	if (test_subscription_count_limit(handle, server_ip, server_port) != 0)
		result = 1;

	UpnpUnRegisterRootDevice(handle);
	UpnpFinish();
	return result ? EXIT_FAILURE : EXIT_SUCCESS;
#endif /* !_WIN32 */
}
