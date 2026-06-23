// regression: issue #195
// UpnpInit2 returns UPNP_E_SOCKET_BIND after a network interface change.
//
// Root cause: UpnpGetIfInfo() does not clear gIF_IPV6 before scanning.
// When the interface changes and no longer has a link-local IPv6 address,
// gIF_IPV6 retains the stale value from the previous initialization.
// get_miniserver_sockets() then tries to bind a TCP socket to that stale
// address, which fails with EADDRNOTAVAIL → UPNP_E_SOCKET_BIND.
//
// Fix: UpnpGetIfInfo() must clear gIF_IPV6 (and gIF_IPV6_ULA_GUA) before
// scanning so stale addresses do not persist across re-initializations.

#include "upnp.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <netinet/in.h>
#include <sys/socket.h>

// Internal headers — needed for gIF_IPV6 and UpnpGetIfInfo()
extern "C" {
#include "upnpapi.h"
}

// Test helper tools (CIfaddr4, CCaptureFd)
#include "tools/tools.cpp"

#include <arpa/inet.h>
#include <cstring>
#include <ifaddrs.h>
#include <net/if.h>

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgPointee;

// --- mock getifaddrs ---
class MockGetifaddrs
{
public:
	MOCK_METHOD(int, getifaddrs, (struct ifaddrs **));
};
MockGetifaddrs *ptrMockGetifaddrsObj = nullptr;
int getifaddrs(struct ifaddrs **ifap)
{
	return ptrMockGetifaddrsObj->getifaddrs(ifap);
}

// --- mock freeifaddrs ---
class MockFreeifaddrs
{
public:
	MOCK_METHOD(void, freeifaddrs, (struct ifaddrs *));
};
MockFreeifaddrs *ptrMockFreeifaddrObj = nullptr;
void freeifaddrs(struct ifaddrs *ifap)
{
	return ptrMockFreeifaddrObj->freeifaddrs(ifap);
}

// --- mock if_nametoindex ---
class MockIf_nametoindex
{
public:
	MOCK_METHOD(unsigned int, if_nametoindex, (const char *));
};
MockIf_nametoindex *ptrMockIf_nametoindexObj = nullptr;
unsigned int if_nametoindex(const char *ifname)
{
	return ptrMockIf_nametoindexObj->if_nametoindex(ifname);
}

class Issue195TestSuite : public ::testing::Test
{
protected:
	MockGetifaddrs mockGetifaddrsObj;
	MockFreeifaddrs mockFreeifaddrObj;
	MockIf_nametoindex mockIf_nametoindexObj;

	Issue195TestSuite()
	{
		ptrMockGetifaddrsObj = &mockGetifaddrsObj;
		ptrMockFreeifaddrObj = &mockFreeifaddrObj;
		ptrMockIf_nametoindexObj = &mockIf_nametoindexObj;
	}

	void SetUp() override
	{
		gIF_IPV6[0] = '\0';
		gIF_IPV6_PREFIX_LENGTH = 0;
		gIF_IPV6_ULA_GUA[0] = '\0';
		gIF_IPV6_ULA_GUA_PREFIX_LENGTH = 0;
	}

	void TearDown() override
	{
		gIF_IPV6[0] = '\0';
		gIF_IPV6_ULA_GUA[0] = '\0';
		ptrMockGetifaddrsObj = nullptr;
		ptrMockFreeifaddrObj = nullptr;
		ptrMockIf_nametoindexObj = nullptr;
	}
};

// regression: issue #195
// When UpnpGetIfInfo is called for an interface with no link-local IPv6,
// it must clear gIF_IPV6 so that a stale address from a previous init
// does not survive into the next StartMiniServer call.
TEST_F(Issue195TestSuite, GetIfInfo_clears_stale_IPv6_when_interface_has_none)
{
	char *github_action = std::getenv("GITHUB_ACTIONS");
	if (github_action) {
		GTEST_SKIP() << "due to issues with googlemock";
	}

	// Simulate the state after a previous initialization that had IPv6:
	// gIF_IPV6 holds a stale link-local address that no longer exists on
	// the interface after a network change.
	strncpy(gIF_IPV6, "fe80::dead:beef", INET6_ADDRSTRLEN - 1);
	gIF_IPV6[INET6_ADDRSTRLEN - 1] = '\0';

	// Mock getifaddrs to return an IPv4-only interface (simulates the
	// state after a network change where the IPv6 address was removed).
	struct ifaddrs *ifaddr = nullptr;
	CIfaddr4 ifaddr4Obj;
	ifaddr4Obj.set("eth0", "192.168.99.3/24");
	ifaddr = ifaddr4Obj.get();

	EXPECT_CALL(mockGetifaddrsObj, getifaddrs(_))
		.WillOnce(DoAll(SetArgPointee<0>(ifaddr), Return(0)));
	EXPECT_CALL(mockFreeifaddrObj, freeifaddrs(ifaddr)).Times(1);
	EXPECT_CALL(mockIf_nametoindexObj, if_nametoindex(_))
		.WillOnce(Return(2));

	int rc = UpnpGetIfInfo("eth0");
	EXPECT_EQ(rc, UPNP_E_SUCCESS);

	// Before fix: gIF_IPV6 == "fe80::dead:beef" (stale, not cleared)
	// After fix:  gIF_IPV6 == "" (cleared before scanning)
	EXPECT_STREQ(gIF_IPV6, "")
		<< "regression issue #195: UpnpGetIfInfo must clear gIF_IPV6 "
		   "when the scanned interface has no link-local IPv6. "
		   "A stale gIF_IPV6 causes get_miniserver_sockets to attempt "
		   "a bind() to the old address, returning UPNP_E_SOCKET_BIND "
		   "on every re-initialization after a network change.";
}

// regression: issue #247
// UpnpGetIfInfo must not modify gIF_NAME when it returns
// UPNP_E_INVALID_INTERFACE.
//
// Root cause: the Unix implementation writes IfName into gIF_NAME before the
// getifaddrs loop verifies the interface exists. When the lookup fails the
// function correctly returns UPNP_E_INVALID_INTERFACE, but gIF_NAME has
// already been overwritten with the invalid name.
//
// Fix: defer the write to gIF_NAME until after the interface is verified.

class Issue247TestSuite : public ::testing::Test
{
protected:
	MockGetifaddrs mockGetifaddrsObj;
	MockFreeifaddrs mockFreeifaddrObj;
	MockIf_nametoindex mockIf_nametoindexObj;

	Issue247TestSuite()
	{
		ptrMockGetifaddrsObj = &mockGetifaddrsObj;
		ptrMockFreeifaddrObj = &mockFreeifaddrObj;
		ptrMockIf_nametoindexObj = &mockIf_nametoindexObj;
	}

	void SetUp() override { gIF_NAME[0] = '\0'; }

	void TearDown() override
	{
		gIF_NAME[0] = '\0';
		ptrMockGetifaddrsObj = nullptr;
		ptrMockFreeifaddrObj = nullptr;
		ptrMockIf_nametoindexObj = nullptr;
	}
};

// regression: issue #247
TEST_F(Issue247TestSuite,
	GetIfInfo_does_not_corrupt_gIF_NAME_on_invalid_interface)
{
	char *github_action = std::getenv("GITHUB_ACTIONS");
	if (github_action) {
		GTEST_SKIP() << "due to issues with googlemock";
	}

	// System has one valid interface (eth0), but caller passes a typo
	// ("ethO")
	struct ifaddrs *ifaddr = nullptr;
	CIfaddr4 ifaddr4Obj;
	ifaddr4Obj.set("eth0", "192.168.77.48/22");
	ifaddr = ifaddr4Obj.get();

	EXPECT_CALL(mockGetifaddrsObj, getifaddrs(_))
		.WillOnce(DoAll(SetArgPointee<0>(ifaddr), Return(0)));
	EXPECT_CALL(mockFreeifaddrObj, freeifaddrs(ifaddr)).Times(1);
	EXPECT_CALL(mockIf_nametoindexObj, if_nametoindex(_)).Times(0);

	int rc = UpnpGetIfInfo("ethO"); // uppercase O, not zero
	EXPECT_EQ(rc, UPNP_E_INVALID_INTERFACE);

	// Before fix: gIF_NAME == "ethO" (written before validation)
	// After fix:  gIF_NAME == "" (not modified on failure)
	EXPECT_STREQ(gIF_NAME, "")
		<< "regression issue #247: UpnpGetIfInfo must not modify "
		   "gIF_NAME "
		   "when the interface lookup fails. The invalid name was "
		   "written "
		   "to gIF_NAME before validation, leaving the library in an "
		   "inconsistent state even though UPNP_E_INVALID_INTERFACE "
		   "was "
		   "returned.";
}

// regression: issue #598
// IN6_IS_ADDR_GLOBAL must correctly accept all 2000::/3 addresses.
//
// Root cause: the macro used bitmask 0x70000000 instead of 0xe0000000,
// checking bits 6,5,4 instead of the top 3 bits (7,6,5) of the first byte.
// This caused a000::/4 to be falsely classified as GUA and 3000::/4 to be
// incorrectly excluded from GUA.
//
// Fix: change the bitmask from 0x70000000 to 0xe0000000.

class Issue598TestSuite : public ::testing::Test
{
protected:
	MockGetifaddrs mockGetifaddrsObj;
	MockFreeifaddrs mockFreeifaddrObj;
	MockIf_nametoindex mockIf_nametoindexObj;

	/* Two-entry chain: link-local first, address-under-test second.
	 * gIF_IPV6_ULA_GUA is only written when the interface also has a
	 * link-local address (UpnpGetIfInfo gates the write on v6_addr being
	 * non-unspecified), so both entries are needed to exercise the GUA
	 * classification path on a realistic interface. */
	struct sockaddr_in6 addr6_ll{}, netmask6_ll{};
	struct ifaddrs ifaddr6_ll{};
	struct sockaddr_in6 addr6_ut{}, netmask6_ut{};
	struct ifaddrs ifaddr6_ut{};

	Issue598TestSuite()
	{
		ptrMockGetifaddrsObj = &mockGetifaddrsObj;
		ptrMockFreeifaddrObj = &mockFreeifaddrObj;
		ptrMockIf_nametoindexObj = &mockIf_nametoindexObj;
	}

	struct ifaddrs *makeIfaddr6Pair(
		const char *ifname, const char *addr_str)
	{
		memset(&addr6_ll, 0, sizeof(addr6_ll));
		memset(&netmask6_ll, 0, sizeof(netmask6_ll));
		memset(&ifaddr6_ll, 0, sizeof(ifaddr6_ll));
		addr6_ll.sin6_family = AF_INET6;
		inet_pton(AF_INET6, "fe80::1", &addr6_ll.sin6_addr);
		netmask6_ll.sin6_family = AF_INET6;
		memset(&netmask6_ll.sin6_addr, 0xff, 8);
		ifaddr6_ll.ifa_name = const_cast<char *>(ifname);
		ifaddr6_ll.ifa_flags = IFF_UP | IFF_MULTICAST;
		ifaddr6_ll.ifa_addr =
			reinterpret_cast<struct sockaddr *>(&addr6_ll);
		ifaddr6_ll.ifa_netmask =
			reinterpret_cast<struct sockaddr *>(&netmask6_ll);
		ifaddr6_ll.ifa_next = &ifaddr6_ut;

		memset(&addr6_ut, 0, sizeof(addr6_ut));
		memset(&netmask6_ut, 0, sizeof(netmask6_ut));
		memset(&ifaddr6_ut, 0, sizeof(ifaddr6_ut));
		addr6_ut.sin6_family = AF_INET6;
		inet_pton(AF_INET6, addr_str, &addr6_ut.sin6_addr);
		netmask6_ut.sin6_family = AF_INET6;
		memset(&netmask6_ut.sin6_addr, 0xff, 8);
		ifaddr6_ut.ifa_name = const_cast<char *>(ifname);
		ifaddr6_ut.ifa_flags = IFF_UP | IFF_MULTICAST;
		ifaddr6_ut.ifa_addr =
			reinterpret_cast<struct sockaddr *>(&addr6_ut);
		ifaddr6_ut.ifa_netmask =
			reinterpret_cast<struct sockaddr *>(&netmask6_ut);
		ifaddr6_ut.ifa_next = nullptr;

		return &ifaddr6_ll;
	}

	void SetUp() override
	{
		gIF_IPV6_ULA_GUA[0] = '\0';
		gIF_IPV6_ULA_GUA_PREFIX_LENGTH = 0;
		gIF_IPV6[0] = '\0';
	}

	void TearDown() override
	{
		gIF_IPV6_ULA_GUA[0] = '\0';
		gIF_IPV6[0] = '\0';
		ptrMockGetifaddrsObj = nullptr;
		ptrMockFreeifaddrObj = nullptr;
		ptrMockIf_nametoindexObj = nullptr;
	}
};

// regression: issue #598
// 3000::/4 is within 2000::/3 GUA space and must be stored in gIF_IPV6_ULA_GUA.
// Before fix: 0x30 & 0x70 == 0x30 != 0x20, so the GUA branch was never taken.
TEST_F(Issue598TestSuite, GetIfInfo_stores_3000_prefix_as_GUA)
{
	char *github_action = std::getenv("GITHUB_ACTIONS");
	if (github_action) {
		GTEST_SKIP() << "due to issues with googlemock";
	}

	struct ifaddrs *ifaddr = makeIfaddr6Pair("eth0", "3000::1");

	EXPECT_CALL(mockGetifaddrsObj, getifaddrs(_))
		.WillOnce(DoAll(SetArgPointee<0>(ifaddr), Return(0)));
	EXPECT_CALL(mockFreeifaddrObj, freeifaddrs(ifaddr)).Times(1);
	EXPECT_CALL(mockIf_nametoindexObj, if_nametoindex(_))
		.WillOnce(Return(2));

	int rc = UpnpGetIfInfo("eth0");
	EXPECT_EQ(rc, UPNP_E_SUCCESS);

	// Before fix: gIF_IPV6_ULA_GUA == "" (3000::/4 incorrectly excluded)
	// After fix:  gIF_IPV6_ULA_GUA == "3000::1"
	EXPECT_STREQ(gIF_IPV6_ULA_GUA, "3000::1")
		<< "regression issue #598: 3000::1 is in 2000::/3 GUA space "
		   "but "
		   "was excluded by bitmask 0x70000000 which checks the wrong "
		   "bits.";
}

// regression: issue #598
// a000::/4 is NOT in 2000::/3 GUA space and must not be stored as GUA.
// Before fix: 0xa0 & 0x70 == 0x20, so the address was a false positive.
TEST_F(Issue598TestSuite, GetIfInfo_does_not_store_a000_prefix_as_GUA)
{
	char *github_action = std::getenv("GITHUB_ACTIONS");
	if (github_action) {
		GTEST_SKIP() << "due to issues with googlemock";
	}

	struct ifaddrs *ifaddr = makeIfaddr6Pair("eth0", "a000::1");

	EXPECT_CALL(mockGetifaddrsObj, getifaddrs(_))
		.WillOnce(DoAll(SetArgPointee<0>(ifaddr), Return(0)));
	EXPECT_CALL(mockFreeifaddrObj, freeifaddrs(ifaddr)).Times(1);
	EXPECT_CALL(mockIf_nametoindexObj, if_nametoindex(_))
		.WillOnce(Return(2));

	int rc = UpnpGetIfInfo("eth0");
	EXPECT_EQ(rc, UPNP_E_SUCCESS);

	// Before fix: gIF_IPV6_ULA_GUA == "a000::1" (false positive)
	// After fix:  gIF_IPV6_ULA_GUA == "" (correctly not GUA)
	EXPECT_STREQ(gIF_IPV6_ULA_GUA, "")
		<< "regression issue #598: a000::1 is NOT in 2000::/3 GUA "
		   "space "
		   "but was falsely accepted by bitmask 0x70000000.";
}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
