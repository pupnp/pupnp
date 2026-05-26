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

#include "gmock/gmock.h"
#include "gtest/gtest.h"

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
// UpnpGetIfInfo must not modify gIF_NAME when it returns UPNP_E_INVALID_INTERFACE.
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
TEST_F(Issue247TestSuite, GetIfInfo_does_not_corrupt_gIF_NAME_on_invalid_interface)
{
	char *github_action = std::getenv("GITHUB_ACTIONS");
	if (github_action) {
		GTEST_SKIP() << "due to issues with googlemock";
	}

	// System has one valid interface (eth0), but caller passes a typo ("ethO")
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
		<< "regression issue #247: UpnpGetIfInfo must not modify gIF_NAME "
		   "when the interface lookup fails. The invalid name was written "
		   "to gIF_NAME before validation, leaving the library in an "
		   "inconsistent state even though UPNP_E_INVALID_INTERFACE was "
		   "returned.";
}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
