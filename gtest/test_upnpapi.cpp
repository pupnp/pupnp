// Mock network interfaces
// Author: 2021-03-06 - Ingo Höft <Ingo@Hoeft-online.de>

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "./tools/ifaddrs.cpp"

// for TestSuites needing headers linked against the static C library
extern "C" {
    #include "upnp.h"
    #include "upnpapi.h"
}

using ::testing::_;
using ::testing::Return;
using ::testing::DoAll;
using ::testing::SetArgPointee;
using ::testing::MatchesRegex;


// --- mock getifaddrs -------------------------------------
class getifaddrsInterface {
public:
    virtual ~getifaddrsInterface() {}
    virtual int getifaddrs(struct ifaddrs**) = 0;
};

class GetifaddrsMock : public getifaddrsInterface {
public:
    virtual ~GetifaddrsMock() {}
    MOCK_METHOD(int, getifaddrs, (struct ifaddrs**), (override));
};

// Pointer Will be initialized later in the test body
// For further information look at https://stackoverflow.com/a/66498073/5014688
getifaddrsInterface* ptrGetifaddrsMockObj = nullptr;
int getifaddrs(struct ifaddrs** ifap) {
    return ptrGetifaddrsMockObj->getifaddrs(ifap);
}

// --- mock freeifaddrs ------------------------------------
class FreeifaddrsInterface {
public:
    virtual ~FreeifaddrsInterface() {}
    virtual void freeifaddrs(struct ifaddrs*) = 0;
};

class FreeifaddrsMock : public FreeifaddrsInterface {
public:
    virtual ~FreeifaddrsMock() {}
    MOCK_METHOD(void, freeifaddrs, (struct ifaddrs*), (override));
};

FreeifaddrsInterface* ptrFreeifaddrsMockObj = nullptr;
void freeifaddrs(struct ifaddrs* ifap) {
    return ptrFreeifaddrsMockObj->freeifaddrs(ifap);
}

// --- mock if_nametoindex ---------------------------------
class If_nametoindexInterface {
public:
    virtual ~If_nametoindexInterface() {}
    virtual unsigned int if_nametoindex(const char*) = 0;
};

class If_nametoindexMock : public If_nametoindexInterface {
public:
    virtual ~If_nametoindexMock() {}
    MOCK_METHOD(unsigned int, if_nametoindex, (const char*), (override));
};

If_nametoindexInterface* ptrIf_nametoindexMockObj = nullptr;
unsigned int if_nametoindex(const char* ifname) {
    return ptrIf_nametoindexMockObj->if_nametoindex(ifname);
}


// UpnpApi Testsuite for IP4
//--------------------------
class UpnpApiIPv4TestSuite: public ::testing::Test
{
protected:
    // Fixtures for this Testsuite
    unsigned short PORT = 51515;
};


TEST_F(UpnpApiIPv4TestSuite, UpnpGetIfInfo)
{
    struct ifaddrs* ifaddr = nullptr;

    Ifaddr4 ifaddr4;
    ifaddr = ifaddr4.get();

    GetifaddrsMock getifaddrsMockObj;
    ptrGetifaddrsMockObj = &getifaddrsMockObj;

    FreeifaddrsMock freeifaddrsMockObj;
    ptrFreeifaddrsMockObj = &freeifaddrsMockObj;

    If_nametoindexMock if_nametoindexMockObj;
    ptrIf_nametoindexMockObj = &if_nametoindexMockObj;

    EXPECT_CALL(getifaddrsMockObj, getifaddrs(_))
        .WillOnce(DoAll(SetArgPointee<0>(ifaddr), Return(0)));
    EXPECT_CALL(freeifaddrsMockObj, freeifaddrs(ifaddr))
        .Times(1);
    EXPECT_CALL(if_nametoindexMockObj, if_nametoindex(_))
        .WillOnce(Return(1));

    EXPECT_EQ(UpnpGetIfInfo(ifaddr->ifa_name), UPNP_E_SUCCESS);
    EXPECT_STREQ(gIF_NAME, ifaddr->ifa_name);
    EXPECT_THAT(gIF_IPV4, MatchesRegex("[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}"));
    EXPECT_STREQ(gIF_IPV6, "");
    EXPECT_STREQ(gIF_IPV6_ULA_GUA, "");
    EXPECT_EQ(gIF_INDEX, (const unsigned int)1);
}

TEST_F(UpnpApiIPv4TestSuite, UpnpInit2)
{
    //GTEST_SKIP();
    struct ifaddrs* ifaddr = nullptr;

    Ifaddr4 ifaddr4;
    ifaddr = ifaddr4.get();

    GetifaddrsMock getifaddrsMockObj;
    ptrGetifaddrsMockObj = &getifaddrsMockObj;

    FreeifaddrsMock freeifaddrsMockObj;
    ptrFreeifaddrsMockObj = &freeifaddrsMockObj;

    If_nametoindexMock if_nametoindexMockObj;
    ptrIf_nametoindexMockObj = &if_nametoindexMockObj;

    EXPECT_CALL(getifaddrsMockObj, getifaddrs(_))
        .WillOnce(DoAll(SetArgPointee<0>(ifaddr), Return(0)));
    EXPECT_CALL(freeifaddrsMockObj, freeifaddrs(ifaddr))
        .Times(1);
    EXPECT_CALL(if_nametoindexMockObj, if_nametoindex(_))
        .Times(1);

    int return_value = UpnpInit2(ifaddr->ifa_name, PORT);
    EXPECT_EQ(return_value, UPNP_E_SOCKET_BIND);
}


/* first tests with IP6 not working anymore with mocking, will be improved next
// UpnpApi Testsuite for IP6
//--------------------------
class UpnpApiIPv6TestSuite: public ::testing::Test
{
    // Fixtures for this Testsuite
    protected:
    std::string interface = "ens1";
    unsigned short PORT = 51515;
};

TEST_F(UpnpApiIPv6TestSuite, UpnpGetIfInfo)
{
    EXPECT_EQ(UpnpGetIfInfo(interface.c_str()), UPNP_E_SUCCESS);
    EXPECT_STREQ(gIF_NAME, interface.c_str());
    EXPECT_STREQ(gIF_IPV4, "");
    //strncpy(gIF_IPV6, "fe80::5054:ff:fe40:50f6", 24); //testing the regex
    EXPECT_THAT(gIF_IPV6, MatchesRegex("([A-Fa-f0-9]{1,4}::?){1,7}[A-Fa-f0-9]{1,4}"));
    EXPECT_THAT(gIF_IPV6_ULA_GUA, MatchesRegex("([A-Fa-f0-9]{1,4}::?){1,7}[A-Fa-f0-9]{1,4}"));
    EXPECT_IN_RANGE((int)gIF_INDEX, 1, 20);
}

TEST_F(UpnpApiIPv6TestSuite, UpnpInit2)
{
    //GTEST_SKIP();
    int return_value = UpnpInit2(interface.c_str(), PORT);
    EXPECT_EQ(return_value, UPNP_E_SUCCESS);
}
*/

// UpnpApi common Testsuite
//-------------------------
TEST(UpnpApiTestSuite, GetHandleInfo)
{
    Handle_Info **HndInfo = 0;
    EXPECT_EQ(GetHandleInfo(0, HndInfo), HND_INVALID);
    EXPECT_EQ(GetHandleInfo(1, HndInfo), HND_INVALID);
    EXPECT_EQ(GetHandleInfo(NUM_HANDLE, HndInfo), HND_INVALID);
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}