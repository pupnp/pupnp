/* test_ssdp_mx_cap.c
 *
 * Regression test: the MX header of an M-SEARCH request must be capped.
 *
 * ssdp_handle_device_request() schedules the reply after a random delay in
 * [0, MX) seconds and keeps the reply job (with a copy of the request
 * destination) queued until then. MX was only checked for being
 * non-negative, so an unauthenticated peer could send a stream of
 * M-SEARCH packets with MX close to INT_MAX and pin memory in the timer
 * queue for years (CWE-770). UDA 1.1 says a device should treat MX > 5 as 5.
 *
 * Test cases:
 *   A. MX: 1          -> reply within the timeout (baseline).
 *   B. MX: 2000000000 -> reply within the timeout, because MX is capped.
 *
 * Pre-fix: B times out (the reply is scheduled up to ~57 years away).
 * Post-fix: B is answered within a few seconds.
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

#define REPLY_TIMEOUT_SEC 6

static const char DEVICE_DESC[] =
	"<?xml version=\"1.0\"?>"
	"<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
	"<specVersion><major>1</major><minor>0</minor></specVersion>"
	"<device>"
	"<deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType>"
	"<friendlyName>mxcap</friendlyName>"
	"<manufacturer>Test</manufacturer>"
	"<modelName>Test</modelName>"
	"<UDN>uuid:mxcap-0000-0000-0000-000000000000</UDN>"
	"</device>"
	"</root>";

#ifndef _WIN32

static int device_callback(Upnp_EventType t, void *e, void *c)
{
	(void)t;
	(void)e;
	(void)c;
	return 0;
}

/* Send a unicast M-SEARCH with the given MX to the SSDP port of server_ip.
 * Returns 1 if a reply mentioning our UDN arrives within the timeout, 0 if
 * not, -1 on socket error. */
static int search_replied(const char *server_ip, const char *mx)
{
	int fd;
	int found = 0;
	char req[256];
	char resp[2048];
	struct sockaddr_in addr;
	struct timeval tv;
	ssize_t n;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0)
		return -1;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(1900);
	if (inet_pton(AF_INET, server_ip, &addr.sin_addr) != 1) {
		close(fd);
		return -1;
	}
	tv.tv_sec = REPLY_TIMEOUT_SEC;
	tv.tv_usec = 0;
	if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0) {
		close(fd);
		return -1;
	}
	snprintf(req,
		sizeof(req),
		"M-SEARCH * HTTP/1.1\r\n"
		"HOST: 239.255.255.250:1900\r\n"
		"MAN: \"ssdp:discover\"\r\n"
		"MX: %s\r\n"
		"ST: upnp:rootdevice\r\n\r\n",
		mx);
	if (sendto(fd,
		    req,
		    strlen(req),
		    0,
		    (struct sockaddr *)&addr,
		    sizeof(addr)) < 0) {
		close(fd);
		return -1;
	}
	while ((n = recv(fd, resp, sizeof(resp) - 1, 0)) > 0) {
		resp[n] = '\0';
		if (strstr(resp, "uuid:mxcap-")) {
			found = 1;
			break;
		}
	}
	close(fd);

	return found;
}

int main(void)
{
	int rc;
	int result = 0;
	int r;
	UpnpDevice_Handle handle = -1;
	const char *server_ip;

	rc = UpnpInit2(NULL, 0);
	if (rc != UPNP_E_SUCCESS) {
		fprintf(stderr,
			"UpnpInit2 failed (%d); skipping (no network?)\n",
			rc);
		return EXIT_SUCCESS;
	}
	server_ip = UpnpGetServerIpAddress();
	if (!server_ip) {
		fprintf(stderr, "Could not determine server address\n");
		UpnpFinish();
		return EXIT_FAILURE;
	}
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

	r = search_replied(server_ip, "1");
	if (r != 1) {
		fprintf(stderr,
			"Baseline (MX: 1) got no reply (%d); skipping "
			"(environment?)\n",
			r);
		UpnpUnRegisterRootDevice(handle);
		UpnpFinish();
		return EXIT_SUCCESS;
	}
	printf("MX: 1 -> replied\n");

	r = search_replied(server_ip, "2000000000");
	printf("MX: 2000000000 -> %s\n", r == 1 ? "replied" : "NO REPLY");
	if (r != 1)
		result = 1;

	UpnpUnRegisterRootDevice(handle);
	UpnpFinish();
	return result ? EXIT_FAILURE : EXIT_SUCCESS;
}

#else /* _WIN32 */

int main(void)
{
	/* Raw POSIX sockets; not exercised on Windows. */
	(void)DEVICE_DESC;
	return EXIT_SUCCESS;
}

#endif /* !_WIN32 */
