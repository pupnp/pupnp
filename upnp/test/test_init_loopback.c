/* test_init_loopback.c
 *
 * UpnpInit2() must accept a loopback interface that the caller names
 * explicitly. UpnpGetIfInfo() skipped every IFF_LOOPBACK interface, even a
 * named one, and returned UPNP_E_INVALID_INTERFACE. Auto-selection
 * (IfName == NULL) still skips loopback interfaces.
 *
 * The loopback is "lo" on Linux and "lo0" on the BSDs, macOS and illumos,
 * so the name is taken from getifaddrs().
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

/* Copies the name and IPv4 address of an up loopback interface. */
static int find_loopback(char *name, size_t name_size, char *ip, size_t ip_size)
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
		inet_ntop(AF_INET,
			&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr,
			ip,
			(socklen_t)ip_size);
		found = 1;
		break;
	}
	freeifaddrs(ifap);

	return found;
}

int main(void)
{
	char name[IF_NAMESIZE + 1];
	char ip[INET_ADDRSTRLEN];
	const char *server_ip;
	int rc;
	int err = 0;

	if (!find_loopback(name, sizeof name, ip, sizeof ip)) {
		puts("SKIP: no IPv4 loopback interface is up");
		return EXIT_SUCCESS;
	}
	rc = UpnpInit2(name, 0);
	server_ip = UpnpGetServerIpAddress();
	printf("UpnpInit2(\"%s\") -> %d, server address %s:%u\n",
		name,
		rc,
		server_ip ? server_ip : "(none)",
		(unsigned)UpnpGetServerPort());
	if (rc != UPNP_E_SUCCESS) {
		fprintf(stderr, "  FAIL: expected UPNP_E_SUCCESS\n");
		err = 1;
	} else if (!server_ip || strcmp(server_ip, ip) != 0) {
		fprintf(stderr, "  FAIL: expected server address %s\n", ip);
		err = 1;
	}
	UpnpFinish();
	if (!err)
		puts("test_init_loopback: PASS");

	return err ? EXIT_FAILURE : EXIT_SUCCESS;
}

#else /* _WIN32 */

int main(void)
{
	puts("SKIP: test uses getifaddrs() (not available on Windows).");
	return EXIT_SUCCESS;
}

#endif /* _WIN32 */
