// regression: Windows UpnpGetIfInfo() must not format an address family
// that was not found on the interface.
//
// Reported downstream in amule-org/amule#242 and amule-org/amule#301:
// on Windows, disabling IPv6 on the network adapter made aMule's UPnP
// port mapping fail completely, not just IPv6 discovery.
//
// Root cause: the _WIN32 branch of UpnpGetIfInfo() (upnp/src/api/upnpapi.c)
// declared v6_addr uninitialized and called inet_ntop() into gIF_IPV6
// unconditionally, even when no IPv6 address had been found for the
// selected adapter. That formatted uninitialized stack bytes into a
// syntactically valid but bogus address, which get_ssdp_sockets()
// (upnp/src/ssdp/ssdp_server.c) then took as "IPv6 is available here" via
// strlen(gIF_IPV6) > 0, tried and failed to join the SSDP multicast group,
// and aborted SSDP setup for both IPv4 and IPv6 — killing discovery
// entirely.
//
// Fix: UpnpSetIfAddrStrings() only formats an address family into its
// output buffer when the caller reports that family as found.

#include "gtest/gtest.h"

extern "C" {
#include "upnpapi.h"
}

#include <arpa/inet.h>
#include <cstring>
#include <netinet/in.h>

TEST(UpnpSetIfAddrStringsTestSuite, v4_only_leaves_v6_buffer_untouched)
{
	struct in_addr v4_addr;
	struct in_addr v4_netmask;
	struct in6_addr v6_addr; // deliberately never populated
	std::memset(&v4_addr, 0, sizeof(v4_addr));
	std::memset(&v4_netmask, 0, sizeof(v4_netmask));
	inet_pton(AF_INET, "192.168.1.42", &v4_addr);
	inet_pton(AF_INET, "255.255.255.0", &v4_netmask);
	// Poison v6_addr the way uninitialized stack memory would look,
	// instead of leaving it zeroed like the rest of the test doubles.
	std::memset(&v6_addr, 0xAA, sizeof(v6_addr));

	char out_v4[INET_ADDRSTRLEN] = {0};
	char out_v4_netmask[INET_ADDRSTRLEN] = {0};
	char out_v6[INET6_ADDRSTRLEN] = "sentinel";

	UpnpSetIfAddrStrings(1,
		&v4_addr,
		&v4_netmask,
		out_v4,
		sizeof(out_v4),
		out_v4_netmask,
		sizeof(out_v4_netmask),
		0,
		&v6_addr,
		out_v6,
		sizeof(out_v6));

	EXPECT_STREQ(out_v4, "192.168.1.42");
	EXPECT_STREQ(out_v4_netmask, "255.255.255.0");
	EXPECT_STREQ(out_v6, "sentinel")
		<< "v6 output buffer must be left untouched when no IPv6 "
		   "address was found, not filled with a formatted garbage "
		   "address";
}

TEST(UpnpSetIfAddrStringsTestSuite, v6_only_leaves_v4_buffers_untouched)
{
	struct in_addr v4_addr; // deliberately never populated
	struct in_addr v4_netmask; // deliberately never populated
	struct in6_addr v6_addr;
	std::memset(&v4_addr, 0xAA, sizeof(v4_addr));
	std::memset(&v4_netmask, 0xAA, sizeof(v4_netmask));
	std::memset(&v6_addr, 0, sizeof(v6_addr));
	inet_pton(AF_INET6, "fe80::1", &v6_addr);

	char out_v4[INET_ADDRSTRLEN] = "sentinel";
	char out_v4_netmask[INET_ADDRSTRLEN] = "sentinel";
	char out_v6[INET6_ADDRSTRLEN] = {0};

	UpnpSetIfAddrStrings(0,
		&v4_addr,
		&v4_netmask,
		out_v4,
		sizeof(out_v4),
		out_v4_netmask,
		sizeof(out_v4_netmask),
		1,
		&v6_addr,
		out_v6,
		sizeof(out_v6));

	EXPECT_STREQ(out_v4, "sentinel");
	EXPECT_STREQ(out_v4_netmask, "sentinel");
	EXPECT_STREQ(out_v6, "fe80::1");
}

TEST(UpnpSetIfAddrStringsTestSuite, both_found_formats_both)
{
	struct in_addr v4_addr;
	struct in_addr v4_netmask;
	struct in6_addr v6_addr;
	inet_pton(AF_INET, "10.0.0.5", &v4_addr);
	inet_pton(AF_INET, "255.0.0.0", &v4_netmask);
	inet_pton(AF_INET6, "fe80::5", &v6_addr);

	char out_v4[INET_ADDRSTRLEN] = {0};
	char out_v4_netmask[INET_ADDRSTRLEN] = {0};
	char out_v6[INET6_ADDRSTRLEN] = {0};

	UpnpSetIfAddrStrings(1,
		&v4_addr,
		&v4_netmask,
		out_v4,
		sizeof(out_v4),
		out_v4_netmask,
		sizeof(out_v4_netmask),
		1,
		&v6_addr,
		out_v6,
		sizeof(out_v6));

	EXPECT_STREQ(out_v4, "10.0.0.5");
	EXPECT_STREQ(out_v4_netmask, "255.0.0.0");
	EXPECT_STREQ(out_v6, "fe80::5");
}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
