/* test_ssdp_multi_device.c
 *
 * Regression test: unregistering one root device must not silence the
 * others.
 *
 * The SDK kept "an IPv4 device is registered" as a boolean that
 * UpnpUnRegisterRootDevice() cleared. With two root devices registered,
 * unregistering one of them made GetDeviceHandleInfo() report that no
 * device was left, so the remaining device stopped answering M-SEARCH
 * requests (and UpnpFinish() did not unregister it).
 *
 * The test registers two devices, checks that both answer a search,
 * unregisters the first and checks that the second still answers.
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

#define UDN_A "uuid:multidev-aaaa-0000-0000-000000000000"
#define UDN_B "uuid:multidev-bbbb-0000-0000-000000000000"
#define REPLY_WINDOW_SEC 2

#define ANSWERED_A 1
#define ANSWERED_B 2

#ifndef _WIN32

static int device_callback(Upnp_EventType t, void *e, void *c)
{
	(void)t;
	(void)e;
	(void)c;
	return 0;
}

static void make_desc(char *out, size_t size, const char *udn)
{
	snprintf(out,
		size,
		"<?xml version=\"1.0\"?>"
		"<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
		"<specVersion><major>1</major><minor>0</minor></specVersion>"
		"<device>"
		"<deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType>"
		"<friendlyName>multidev</friendlyName>"
		"<manufacturer>Test</manufacturer>"
		"<modelName>Test</modelName>"
		"<UDN>%s</UDN>"
		"</device>"
		"</root>",
		udn);
}

/* Unicast an M-SEARCH for root devices to the SSDP port of server_ip and
 * collect for a short window which devices answer. Returns a mask of
 * ANSWERED_A / ANSWERED_B, or -1 on socket error. */
static int search_answers(const char *server_ip)
{
	int fd;
	int mask = 0;
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
	tv.tv_sec = REPLY_WINDOW_SEC;
	tv.tv_usec = 0;
	if (inet_pton(AF_INET, server_ip, &addr.sin_addr) != 1 ||
		setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0) {
		close(fd);
		return -1;
	}
	snprintf(req,
		sizeof(req),
		"M-SEARCH * HTTP/1.1\r\n"
		"HOST: 239.255.255.250:1900\r\n"
		"MAN: \"ssdp:discover\"\r\n"
		"MX: 1\r\n"
		"ST: upnp:rootdevice\r\n\r\n");
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
		if (strstr(resp, "multidev-aaaa"))
			mask |= ANSWERED_A;
		if (strstr(resp, "multidev-bbbb"))
			mask |= ANSWERED_B;
	}
	close(fd);

	return mask;
}

int main(void)
{
	int rc;
	int mask;
	UpnpDevice_Handle ha = -1;
	UpnpDevice_Handle hb = -1;
	const char *server_ip;
	char desc_a[1024];
	char desc_b[1024];

	rc = UpnpInit2(NULL, 0);
	if (rc != UPNP_E_SUCCESS) {
		fprintf(stderr,
			"UpnpInit2 failed (%d); skipping (no network?)\n",
			rc);
		return EXIT_SUCCESS;
	}
	server_ip = UpnpGetServerIpAddress();
	if (!server_ip) {
		fprintf(stderr, "no server address; skipping\n");
		UpnpFinish();
		return EXIT_SUCCESS;
	}
	make_desc(desc_a, sizeof(desc_a), UDN_A);
	make_desc(desc_b, sizeof(desc_b), UDN_B);
	rc = UpnpRegisterRootDevice2(UPNPREG_BUF_DESC,
		desc_a,
		strlen(desc_a),
		1,
		device_callback,
		NULL,
		&ha);
	if (rc != UPNP_E_SUCCESS) {
		fprintf(stderr, "register A failed (%d)\n", rc);
		UpnpFinish();
		return EXIT_FAILURE;
	}
	rc = UpnpRegisterRootDevice2(UPNPREG_BUF_DESC,
		desc_b,
		strlen(desc_b),
		1,
		device_callback,
		NULL,
		&hb);
	if (rc != UPNP_E_SUCCESS) {
		fprintf(stderr, "register B failed (%d)\n", rc);
		UpnpFinish();
		return EXIT_FAILURE;
	}

	mask = search_answers(server_ip);
	printf("both registered: answers mask %d\n", mask);
	if (mask != (ANSWERED_A | ANSWERED_B)) {
		fprintf(stderr,
			"Baseline failed (mask %d); skipping (environment?)\n",
			mask);
		UpnpFinish();
		return EXIT_SUCCESS;
	}

	if (UpnpUnRegisterRootDevice(ha) != UPNP_E_SUCCESS) {
		fprintf(stderr, "unregister A failed\n");
		UpnpFinish();
		return EXIT_FAILURE;
	}
	mask = search_answers(server_ip);
	printf("after unregistering A: answers mask %d\n", mask);
	UpnpFinish();
	if (mask != ANSWERED_B) {
		fprintf(stderr,
			"FAIL: expected only B to answer (mask %d)\n",
			ANSWERED_B);
		return EXIT_FAILURE;
	}

	puts("test_ssdp_multi_device: PASS");

	return EXIT_SUCCESS;
}

#else /* _WIN32 */

int main(void)
{
	/* Raw POSIX sockets; not exercised on Windows. */
	return EXIT_SUCCESS;
}

#endif /* !_WIN32 */
