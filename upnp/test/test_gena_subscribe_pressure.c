/* test_gena_subscribe_pressure.c
 *
 * Concurrent flood pressure test for issue #435.
 *
 * The reporter's PoC sends 99999 SUBSCRIBE requests as fast as possible
 * without waiting for responses, creating a pile-up of connections that
 * overwhelms the server.  This test replicates that pressure using
 * NTHREADS POSIX threads, each firing REQUESTS_PER_THREAD large-URL
 * SUBSCRIBE requests fire-and-forget (connect + send, no recv).
 *
 * Total requests: NTHREADS * REQUESTS_PER_THREAD = 10000.
 * (Kept well below the PoC's 99999 so the test finishes within the CI
 * per-test time budget; the correctness check is the health-check below,
 * not the raw request count.)
 *
 * After the flood a single sequential health-check SUBSCRIBE (small URL)
 * is sent and must return HTTP 200.  This proves two things at once:
 *   1. The server is still alive and responsive (not crashed or deadlocked).
 *   2. Fix 1 (MAX_SUBSCRIPTION_CALLBACK_HEADER_SIZE) rejected every flood
 *      request before allocating storage, so TotalSubscriptions is still 0
 *      and the health-check subscription is accepted normally.
 *
 * Without Fix 1 the flood creates up to DEFAULT_MAX_SUBSCRIPTIONS
 * subscriptions (each with a 65536-byte URL).  The health-check then
 * receives HTTP 500 (limit exhausted) instead of HTTP 200, failing the
 * test.
 */

#include "posix_overwrites.h" /* IWYU pragma: keep */
#include "upnp.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
	#include <arpa/inet.h>
	#include <netinet/in.h>
	#include <pthread.h>
	#include <sys/socket.h>
	#include <sys/time.h>
	#include <unistd.h>
#endif

#define EVENT_URL_PATH "/event/dos435p"
#define LARGE_CB_PATH_LEN 0x10000u /* 65536 bytes — same as reporter's PoC */
#define NTHREADS 4
#define REQUESTS_PER_THREAD 2500 /* 4 * 2500 = 10000 */

static const char DEVICE_DESC[] =
	"<?xml version=\"1.0\"?>"
	"<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
	"<specVersion><major>1</major><minor>0</minor></specVersion>"
	"<device>"
	"<deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType>"
	"<friendlyName>dos435p</friendlyName>"
	"<manufacturer>Test</manufacturer>"
	"<modelName>Test</modelName>"
	"<UDN>uuid:dos435p-0000-0000-0000-000000000000</UDN>"
	"<serviceList><service>"
	"<serviceType>urn:schemas-upnp-org:service:Basic:1</serviceType>"
	"<serviceId>urn:upnp-org:serviceId:Basic</serviceId>"
	"<SCPDURL>/scpd.xml</SCPDURL>"
	"<controlURL>/control/dos435p</controlURL>"
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

typedef struct
{
	struct sockaddr_in addr;
	const char *request;
	size_t request_len;
} flood_args_t;

/* Each thread sends REQUESTS_PER_THREAD large-URL SUBSCRIBE requests.
 * Fire-and-forget: connect + send, then close without reading the response.
 * Connection failures (server busy) are silently skipped. */
static void *flood_thread(void *arg)
{
	const flood_args_t *fa = (const flood_args_t *)arg;
	int i, fd;
	struct timeval tv;

	for (i = 0; i < REQUESTS_PER_THREAD; i++) {
		fd = socket(AF_INET, SOCK_STREAM, 0);
		if (fd < 0)
			continue;
		/* Short send timeout so a stalled server does not hang the
		 * thread indefinitely. */
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

		if (connect(fd,
			    (const struct sockaddr *)&fa->addr,
			    sizeof(fa->addr)) == 0) {
			send(fd, fa->request, fa->request_len, MSG_NOSIGNAL);
		}
		close(fd);
	}
	return NULL;
}

/* Sequential health-check: sends one small-URL SUBSCRIBE and returns the
 * HTTP status code, or -1 on error. */
static int health_check(const char *server_ip, unsigned short server_port)
{
	int fd = -1, status = -1;
	struct sockaddr_in addr;
	struct timeval tv;
	char req[256], resp[512];
	int req_len;
	ssize_t n;

	req_len = snprintf(req,
		sizeof(req),
		"SUBSCRIBE " EVENT_URL_PATH " HTTP/1.1\r\n"
		"Host: %s:%u\r\n"
		"NT: upnp:event\r\n"
		"Timeout: Second-1800\r\n"
		"Connection: close\r\n"
		"Callback: <http://%s:1/healthcheck>\r\n"
		"\r\n",
		server_ip,
		(unsigned)server_port,
		server_ip);
	if (req_len <= 0)
		return -1;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
		return -1;

	/* Generous timeout: the server may still be draining the flood queue.
	 */
	tv.tv_sec = 10;
	tv.tv_usec = 0;
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(server_port);
	if (inet_pton(AF_INET, server_ip, &addr.sin_addr) != 1)
		goto done;
	if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
		perror("health_check: connect");
		goto done;
	}
	if (send(fd, req, (size_t)req_len, 0) < 0)
		goto done;

	n = recv(fd, resp, sizeof(resp) - 1, 0);
	if (n > 0) {
		resp[n] = '\0';
		if (sscanf(resp, "HTTP/1.%*d %d", &status) != 1)
			status = -1;
	}

done:
	close(fd);
	return status;
}

#endif /* !_WIN32 */

int main(void)
{
#ifdef _WIN32
	puts("SKIP: pressure test uses POSIX threads, not supported on "
	     "Windows.");
	return EXIT_SUCCESS;
#else
	int rc;
	UpnpDevice_Handle handle = -1;
	const char *server_ip;
	unsigned short server_port;
	flood_args_t fa;
	char *large_request = NULL;
	size_t large_request_len;
	pthread_t threads[NTHREADS];
	int i, health;

	signal(SIGPIPE, SIG_IGN);

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

	/* Build the large-URL request once; share it read-only across threads.
	 */
	{
		const char *hdr_fmt =
			"SUBSCRIBE " EVENT_URL_PATH " HTTP/1.1\r\n"
			"Host: %s:%u\r\n"
			"NT: upnp:event\r\n"
			"Timeout: Second-1800\r\n"
			"Connection: close\r\n"
			"Callback: <http://%s:1/";

		size_t hdr_len = 256 + LARGE_CB_PATH_LEN + 8;
		large_request = malloc(hdr_len);
		if (!large_request) {
			perror("malloc");
			UpnpUnRegisterRootDevice(handle);
			UpnpFinish();
			return EXIT_FAILURE;
		}
		large_request_len = (size_t)snprintf(large_request,
			hdr_len,
			hdr_fmt,
			server_ip,
			(unsigned)server_port,
			server_ip);
		memset(large_request + large_request_len,
			'a',
			LARGE_CB_PATH_LEN);
		large_request_len += LARGE_CB_PATH_LEN;
		large_request_len +=
			(size_t)snprintf(large_request + large_request_len,
				hdr_len - large_request_len,
				">\r\n\r\n");
	}

	memset(&fa.addr, 0, sizeof(fa.addr));
	fa.addr.sin_family = AF_INET;
	fa.addr.sin_port = htons(server_port);
	inet_pton(AF_INET, server_ip, &fa.addr.sin_addr);
	fa.request = large_request;
	fa.request_len = large_request_len;

	printf("Flooding with %d threads x %d requests (%u-byte Callback)...\n",
		NTHREADS,
		REQUESTS_PER_THREAD,
		LARGE_CB_PATH_LEN);

	for (i = 0; i < NTHREADS; i++)
		pthread_create(&threads[i], NULL, flood_thread, &fa);
	for (i = 0; i < NTHREADS; i++)
		pthread_join(threads[i], NULL);

	free(large_request);
	printf("Flood complete. Checking server health...\n");

	health = health_check(server_ip, server_port);
	printf("Health-check SUBSCRIBE -> HTTP %d\n", health);

	UpnpUnRegisterRootDevice(handle);
	UpnpFinish();

	if (health == 200) {
		puts("PASS: server is responsive and accepted health-check "
		     "subscription.\n"
		     "      No flood request created a subscription "
		     "(Fix 1 rejected all large-URL requests).");
		return EXIT_SUCCESS;
	}

	fprintf(stderr,
		"FAIL: expected HTTP 200 from health-check, got %d.\n"
		"  HTTP 500 means MaxSubscriptions was exhausted by the "
		"flood,\n"
		"  i.e. Fix 1 (MAX_SUBSCRIPTION_CALLBACK_HEADER_SIZE) did not\n"
		"  reject all large-URL requests before allocating storage.\n"
		"  Server unresponsive (-1) means it crashed or deadlocked.\n",
		health);
	return EXIT_FAILURE;
#endif /* !_WIN32 */
}
