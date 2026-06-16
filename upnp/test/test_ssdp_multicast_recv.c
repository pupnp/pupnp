/* test_ssdp_multicast_recv.c
 *
 * Regression test for the PR #579 regression (issue #227 follow-up).
 *
 * PR #579 bound the SSDP server sockets to the specific interface unicast
 * address (gIF_IPV4) instead of INADDR_ANY.  On Linux the kernel delivers
 * incoming multicast datagrams only to sockets bound to INADDR_ANY (or to
 * the multicast group address itself), NOT to sockets bound to a unicast
 * address -- even when IP_ADD_MEMBERSHIP has been called.  As a result the
 * SSDP server became deaf to all incoming M-SEARCH queries, breaking UPnP
 * device discovery (reported by KarlStraussberger / Gerbera).
 *
 * Strategy (Linux only):
 *   1. UpnpInit2() + UpnpRegisterRootDevice2() -- start a minimal UPnP device
 *      with a known UDN.
 *   2. Create a UDP socket, enable IP_MULTICAST_LOOP on it, set IP_MULTICAST_IF
 *      to the library's interface, send an SSDP M-SEARCH to
 * 239.255.255.250:1900.
 *   3. Collect responses for RECV_TIMEOUT_S seconds; accept only a response
 * that contains our specific UDN (proving the library's device replied, not
 * some other UPnP device on the LAN).
 *   4. FAIL if no matching response arrives.
 *
 * Currently FAILS: SSDP socket bound to unicast -> multicast M-SEARCH not
 * received. After fix:       SSDP socket bound to INADDR_ANY -> M-SEARCH
 * received -> response.
 *
 * regression: issue #227
 */

#include "upnp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef _WIN32
	#include <arpa/inet.h>
	#include <netinet/in.h>
	#include <sys/socket.h>
	#include <sys/time.h>
	#include <unistd.h>
#endif

#define SSDP_MCAST "239.255.255.250"
#define SSDP_PORT 1900
#define RECV_TIMEOUT_S 3

#define OUR_UDN "uuid:ssdp-recv-0000-0000-0000-000000000001"

/* Minimal device description -- no services needed for M-SEARCH response. */
static const char DEVICE_DESC[] =
	"<?xml version=\"1.0\"?>"
	"<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
	"<specVersion><major>1</major><minor>0</minor></specVersion>"
	"<device>"
	"<deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType>"
	"<friendlyName>ssdp-recv-test</friendlyName>"
	"<manufacturer>Test</manufacturer>"
	"<modelName>Test</modelName>"
	"<UDN>" OUR_UDN "</UDN>"
	"</device>"
	"</root>";

static int device_cb(Upnp_EventType t, void *e, void *c)
{
	(void)t;
	(void)e;
	(void)c;
	return 0;
}

#ifdef __linux__

static int run_test(void)
{
	int result = EXIT_FAILURE;
	int registered = 0;
	UpnpDevice_Handle handle;
	int fd = -1;

	int rc = UpnpInit2(NULL, 0);
	if (rc != UPNP_E_SUCCESS) {
		printf("SKIP: UpnpInit2 returned %d -- no suitable interface\n",
			rc);
		return 77;
	}

	const char *iface_ip = UpnpGetServerIpAddress();
	if (!iface_ip || iface_ip[0] == '\0') {
		printf("SKIP: no server IP available\n");
		goto done;
	}
	printf("Library interface: %s\n", iface_ip);

	rc = UpnpRegisterRootDevice2(UPNPREG_BUF_DESC,
		DEVICE_DESC,
		sizeof(DEVICE_DESC) - 1,
		1,
		device_cb,
		NULL,
		&handle);
	if (rc != UPNP_E_SUCCESS) {
		printf("FAIL: UpnpRegisterRootDevice2 returned %d\n", rc);
		goto done;
	}
	registered = 1;

	/* Let the library's SSDP threads settle before we send. */
	usleep(200000);

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		perror("socket");
		goto done;
	}

	/* Bind to an ephemeral port so the library's response comes back to us.
	 */
	struct sockaddr_in local;
	memset(&local, 0, sizeof(local));
	local.sin_family = AF_INET;
	local.sin_addr.s_addr = htonl(INADDR_ANY);
	local.sin_port = 0;
	if (bind(fd, (struct sockaddr *)&local, sizeof(local)) < 0) {
		perror("bind");
		goto done;
	}

	/* Send multicast via the same interface the library is using. */
	struct in_addr iface;
	inet_pton(AF_INET, iface_ip, &iface);
	if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, &iface, sizeof(iface)) <
		0) {
		perror("IP_MULTICAST_IF");
		goto done;
	}

	/* Enable loopback so the multicast packet is delivered to the library
	 * on this same host. */
	unsigned char loop = 1;
	if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) <
		0) {
		perror("IP_MULTICAST_LOOP");
		goto done;
	}

	/* Send an SSDP M-SEARCH to the multicast group. */
	const char *msearch = "M-SEARCH * HTTP/1.1\r\n"
			      "HOST: 239.255.255.250:1900\r\n"
			      "MAN: \"ssdp:discover\"\r\n"
			      "MX: 1\r\n"
			      "ST: ssdp:all\r\n"
			      "\r\n";

	struct sockaddr_in mcast;
	memset(&mcast, 0, sizeof(mcast));
	mcast.sin_family = AF_INET;
	mcast.sin_port = htons(SSDP_PORT);
	inet_pton(AF_INET, SSDP_MCAST, &mcast.sin_addr);

	ssize_t sent = sendto(fd,
		msearch,
		strlen(msearch),
		0,
		(struct sockaddr *)&mcast,
		sizeof(mcast));
	if (sent < 0) {
		perror("sendto M-SEARCH");
		goto done;
	}
	printf("Sent M-SEARCH (%zd bytes) to %s:%d\n",
		sent,
		SSDP_MCAST,
		SSDP_PORT);

	/*
	 * Collect responses until we see one containing our UDN or the deadline
	 * passes.  We may receive responses from other UPnP devices on the LAN
	 * before our library responds; only a response carrying OUR_UDN proves
	 * that the library's SSDP socket received the multicast M-SEARCH.
	 */
	char buf[4096];
	struct sockaddr_in from;
	socklen_t fromlen;
	time_t deadline = time(NULL) + RECV_TIMEOUT_S;

	while (time(NULL) < deadline) {
		long remaining = deadline - time(NULL);
		if (remaining <= 0)
			break;
		struct timeval tv = {remaining, 0};
		setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

		fromlen = sizeof(from);
		ssize_t n = recvfrom(fd,
			buf,
			sizeof(buf) - 1,
			0,
			(struct sockaddr *)&from,
			&fromlen);
		if (n < 0)
			break; /* timeout */

		buf[n] = '\0';
		char peer[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &from.sin_addr, peer, sizeof(peer));

		if (strstr(buf, OUR_UDN)) {
			printf("OK: received response from %s:%d (%zd bytes) "
			       "containing our UDN\n",
				peer,
				ntohs(from.sin_port),
				n);
			result = EXIT_SUCCESS;
			break;
		}

		printf("(ignoring response from %s -- not our device)\n", peer);
	}

	if (result != EXIT_SUCCESS) {
		printf("FAIL: no SSDP response from our device within %d s.\n"
		       "The SSDP server socket is not receiving multicast\n"
		       "M-SEARCH queries.  This confirms the PR #579 "
		       "regression:\n"
		       "binding the SSDP socket to a unicast address prevents\n"
		       "delivery of multicast datagrams on Linux.\n",
			RECV_TIMEOUT_S);
	}

done:
	if (registered)
		UpnpUnRegisterRootDevice(handle);
	UpnpFinish();
	if (fd >= 0)
		close(fd);
	return result;
}

int main(void)
{
	int r = run_test();
	if (r == 77) {
		printf("test_ssdp_multicast_recv: SKIP\n");
		exit(77);
	}
	if (r != EXIT_SUCCESS) {
		printf("test_ssdp_multicast_recv: FAIL\n");
		exit(EXIT_FAILURE);
	}
	printf("test_ssdp_multicast_recv: PASS\n");
	exit(EXIT_SUCCESS);
}

#else /* __linux__ */

int main(void)
{
	printf("test_ssdp_multicast_recv: SKIP (Linux only)\n");
	exit(77);
}

#endif /* __linux__ */
