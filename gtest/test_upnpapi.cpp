// Mock network interfaces
// For further information look at https://stackoverflow.com/a/66498073/5014688
// Author: 2021-03-06 - Ingo Höft <Ingo@Hoeft-online.de>

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "upnpapi.h"
#include "upnptools.h"
#include "tools/tools.cpp"
#include "UpnpLib.h"

using ::testing::_;
using ::testing::Return;
using ::testing::DoAll;
using ::testing::SetArgPointee;
using ::testing::AtLeast;


// --- mock getifaddrs -------------------------------------
class MockGetifaddrs {
public:
    MOCK_METHOD(int, getifaddrs, (struct ifaddrs**));
};

MockGetifaddrs* ptrMockGetifaddrsObj = nullptr;
int getifaddrs(struct ifaddrs** ifap) {
    return ptrMockGetifaddrsObj->getifaddrs(ifap);
}

// --- mock freeifaddrs ------------------------------------
class MockFreeifaddrs {
public:
    MOCK_METHOD(void, freeifaddrs, (struct ifaddrs*));
};

MockFreeifaddrs* ptrMockFreeifaddrObj = nullptr;
void freeifaddrs(struct ifaddrs* ifap) {
    return ptrMockFreeifaddrObj->freeifaddrs(ifap);
}

// --- mock bind -------------------------------------------
class MockBind {
public:
    MOCK_METHOD(int, bind, (int, const struct sockaddr*, socklen_t));
};

MockBind* ptrMockBindObj = nullptr;
int bind(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
    return ptrMockBindObj->bind(sockfd, addr, addrlen);
}

// --- mock if_nametoindex ---------------------------------
class MockIf_nametoindex {
public:
    MOCK_METHOD(unsigned int, if_nametoindex, (const char*));
};

MockIf_nametoindex* ptrMockIf_nametoindexObj = nullptr;
unsigned int if_nametoindex(const char* ifname) {
    return ptrMockIf_nametoindexObj->if_nametoindex(ifname);
}

// --- mock listen -----------------------------------------
class MockListen {
public:
    MOCK_METHOD(int, listen, (int, int));
};

MockListen* ptrMockListenObj = nullptr;
int listen(int sockfd, int backlog) {
    return ptrMockListenObj->listen(sockfd, backlog);
}

// --- mock select -----------------------------------------
class MockSelect {
public:
    MOCK_METHOD(int, select, (int nfds, fd_set* readfds, fd_set* writefds,
                              fd_set *exceptfds, struct timeval* timeout));
};

MockSelect* ptrMockSelectObj = nullptr;
int select(int nfds, fd_set* readfds, fd_set* writefds,
           fd_set* exceptfds, struct timeval* timeout) {
    return ptrMockSelectObj->select(nfds, readfds, writefds,
                                    exceptfds, timeout);
}

// --- mock accept -----------------------------------------
class MockAccept {
public:
    MOCK_METHOD(int, accept, (int sockfd, struct sockaddr* addr,
                              socklen_t* addrlen));
};

MockAccept* ptrMockAcceptObj = nullptr;
int accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen) {
    return ptrMockAcceptObj->accept(sockfd, addr, addrlen);
}

// --- mock getsockname ------------------------------------
// If needed


// --- mock setsockopt--------------------------------------
class MockSetsockopt {
public:
    MOCK_METHOD(int, setsockopt, (int sockfd, int level, int optname,
                                  const void* optval, socklen_t optlen));
};

MockSetsockopt* ptrMockSetsockoptObj = nullptr;
int setsockopt(int sockfd, int level, int optname, const void* optval,
               socklen_t optlen) {
    return ptrMockSetsockoptObj->setsockopt(sockfd, level, optname,
                                            optval, optlen);
}


// UpnpApi Testsuite for IP4
//==========================
class UpnpApiIPv4TestSuite: public ::testing::Test
// Fixtures for this Testsuite
{
protected:
    unsigned short PORT = 51515;

    // Instantiate the mock objects.
    // The global pointer to them are set in the constructor below.
    MockGetifaddrs mockGetifaddrsObj;
    MockFreeifaddrs mockFreeifaddrsObj;
    MockBind mockBindObj;
    MockIf_nametoindex mockIf_nametoindexObj;
    MockListen mockListenObj;
    MockSelect mockSelectObj;
    MockAccept mockAcceptObj;
    MockSetsockopt mockSetsockoptObj;

    UpnpApiIPv4TestSuite()
    {
        // set the global pointer to the mock objects
        ptrMockGetifaddrsObj = &mockGetifaddrsObj;
        ptrMockFreeifaddrObj = &mockFreeifaddrsObj;
        ptrMockBindObj = &mockBindObj;
        ptrMockIf_nametoindexObj = &mockIf_nametoindexObj;
        ptrMockListenObj = &mockListenObj;
        ptrMockSelectObj = &mockSelectObj;
        ptrMockAcceptObj = &mockAcceptObj;
        ptrMockSetsockoptObj = &mockSetsockoptObj;
    }
};


TEST_F(UpnpApiIPv4TestSuite, UpnpGetIfInfo_called_with_valid_interface)
{
    // provide a network interface
    struct ifaddrs* ifaddr = nullptr;
    CIfaddr4 ifaddr4Obj;
    ifaddr4Obj.set("if0v4", "192.168.99.3/11");
    ifaddr = ifaddr4Obj.get();

    EXPECT_CALL(mockGetifaddrsObj, getifaddrs(_))
        .WillOnce(DoAll(SetArgPointee<0>(ifaddr), Return(0)));
    EXPECT_CALL(mockFreeifaddrsObj, freeifaddrs(ifaddr))
        .Times(1);
    EXPECT_CALL(mockIf_nametoindexObj, if_nametoindex(_))
        .WillOnce(Return(2));

    UpnpLib* p = UpnpLib_new();

    EXPECT_STREQ(UpnpGetErrorMessage(
                  UpnpGetIfInfo(p, "if0v4")),
                  "UPNP_E_SUCCESS");

    // gIF_NAME mocked with getifaddrs above
    EXPECT_EQ(*UpnpLib_get_gIF_NAME(p), *&"if0v4");
    // gIF_IPV4 mocked with getifaddrs above
    EXPECT_EQ(*UpnpLib_get_gIF_IPV4(p), *&"192.168.99.3");
    //EXPECT_THAT(gIF_IPV4, MatchesRegex("[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}"));
    EXPECT_EQ(*UpnpLib_get_gIF_IPV4_NETMASK(p), *&"255.224.0.0");
    EXPECT_EQ(*UpnpLib_get_gIF_IPV6(p), *&"");
    EXPECT_EQ(UpnpLib_get_gIF_IPV6_PREFIX_LENGTH(p), (unsigned)0);
    EXPECT_EQ(*UpnpLib_get_gIF_IPV6_ULA_GUA(p), *&"");
    EXPECT_EQ(UpnpLib_get_gIF_IPV6_ULA_GUA_PREFIX_LENGTH(p), (unsigned)0);
    // index mocked with if_nametoindex above
    EXPECT_EQ(UpnpLib_get_gIF_INDEX(p), (unsigned)2);
    EXPECT_EQ(UpnpLib_get_LOCAL_PORT_V4(p), (unsigned short)0);
    EXPECT_EQ(UpnpLib_get_LOCAL_PORT_V6(p), (unsigned short)0);
    EXPECT_EQ(UpnpLib_get_LOCAL_PORT_V6_ULA_GUA(p), (unsigned short)0);

    UpnpLib_delete(p);
}


TEST_F(UpnpApiIPv4TestSuite, UpnpGetIfInfo_called_with_unknown_interface)
{
    GTEST_SKIP() << "due to failed github sanity check because of issue #247.\n"
                 << "Comment GTEST_SKIP() in the TestSuite to enable this test.";

    // provide a network interface
    struct ifaddrs* ifaddr = nullptr;
    CIfaddr4 ifaddr4Obj;
    ifaddr4Obj.set("eth0", "192.168.77.48/22");
    ifaddr = ifaddr4Obj.get();

    EXPECT_CALL(mockGetifaddrsObj, getifaddrs(_))
        .WillOnce(DoAll(SetArgPointee<0>(ifaddr), Return(0)));
    EXPECT_CALL(mockFreeifaddrsObj, freeifaddrs(ifaddr))
        .Times(1);
    EXPECT_CALL(mockIf_nametoindexObj, if_nametoindex(_))
        .Times(0);

    UpnpLib* p = UpnpLib_new();

    EXPECT_STREQ(UpnpGetErrorMessage(
                  UpnpGetIfInfo(p, "ethO")),
                  "UPNP_E_INVALID_INTERFACE");

    // gIF_NAME mocked with getifaddrs above
    EXPECT_EQ(*UpnpLib_get_gIF_NAME(p), *&"")
        << "ATTENTION! There is a wrong upper case 'O', not zero in \"ethO\"\n";
    // gIF_IPV4 mocked with getifaddrs above
    EXPECT_EQ(*UpnpLib_get_gIF_IPV4(p), *&"");
    //EXPECT_THAT(gIF_IPV4, MatchesRegex("[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}"));
    EXPECT_EQ(*UpnpLib_get_gIF_IPV4_NETMASK(p), *&"");
    EXPECT_EQ(*UpnpLib_get_gIF_IPV6(p), *&"");
    EXPECT_EQ(UpnpLib_get_gIF_IPV6_PREFIX_LENGTH(p), (unsigned)0);
    EXPECT_EQ(*UpnpLib_get_gIF_IPV6_ULA_GUA(p), *&"");
    EXPECT_EQ(UpnpLib_get_gIF_IPV6_ULA_GUA_PREFIX_LENGTH(p), (unsigned)0);
    // index mocked with if_nametoindex above
    EXPECT_EQ(UpnpLib_get_gIF_INDEX(p), (unsigned)4294967295)
                          << "    Which is: (unsigned)-1";
    EXPECT_EQ(UpnpLib_get_LOCAL_PORT_V4(p), (unsigned short)0);
    EXPECT_EQ(UpnpLib_get_LOCAL_PORT_V6(p), (unsigned short)0);
    EXPECT_EQ(UpnpLib_get_LOCAL_PORT_V6_ULA_GUA(p), (unsigned short)0);

    UpnpLib_delete(p);
}


TEST_F(UpnpApiIPv4TestSuite, initialize_default_UpnpInit2)
{
    GTEST_SKIP() << "due to failed github sanity check because of issue #272.\n"
                 << "Comment GTEST_SKIP() in the TestSuite to enable this test.";

    // provide a network interface
    struct ifaddrs* ifaddr = nullptr;
    CIfaddr4 ifaddr4Obj;
    ifaddr4Obj.set("if0v4", "192.168.99.3/20");
    ifaddr = ifaddr4Obj.get();

    // expect calls to system functions (which are mocked)
    EXPECT_CALL(mockGetifaddrsObj, getifaddrs(_))
        .WillOnce(DoAll(SetArgPointee<0>(ifaddr), Return(0)));
    EXPECT_CALL(mockFreeifaddrsObj, freeifaddrs(ifaddr))
        .Times(1); EXPECT_CALL(mockBindObj, bind(_,_,_))
        .Times(5);
    EXPECT_CALL(mockIf_nametoindexObj, if_nametoindex(_))
        .Times(1);
    EXPECT_CALL(mockListenObj, listen(_,_))
        .Times(3);
    EXPECT_CALL(mockSelectObj, select(_,_,_,_,_))
//        .Times(7);
        .Times(AtLeast(1));
    EXPECT_CALL(mockAcceptObj, accept(_,_,_))
        .Times(3);
    EXPECT_CALL(mockSetsockoptObj, setsockopt(_,_,_,_,_))
        .Times(11);

    UpnpLib* libStruct = nullptr;

    // Initialize capturing of the stderr output
    CCaptureFd captFdObj;
    captFdObj.capture(2);   // 1 = stdout, 2 = stderr

    EXPECT_STREQ(UpnpGetErrorMessage(
                  UpnpInit2(&libStruct, NULL, 0, NULL)),
                  "UPNP_E_SUCCESS");

    EXPECT_FALSE(captFdObj.print(std::cerr))
        << "Output to stderr is true. There should not be any output to stderr.\n";

    UpnpLib_delete(libStruct);
}


// UpnpApi common Testsuite
//-------------------------
TEST(UpnpApiTestSuite, get_handle_info)
{
    UpnpLib* p = UpnpLib_new();

    Handle_Info **HndInfo = 0;
    EXPECT_EQ(GetHandleInfo(p, 0, HndInfo), HND_INVALID);
    EXPECT_EQ(GetHandleInfo(p, 1, HndInfo), HND_INVALID);

    UpnpLib_delete(p);
}

TEST(UpnpApiTestSuite, get_error_message)
{
    EXPECT_STREQ(UpnpGetErrorMessage(0), "UPNP_E_SUCCESS");
    EXPECT_STREQ(UpnpGetErrorMessage(-121), "UPNP_E_INVALID_INTERFACE");
    EXPECT_STREQ(UpnpGetErrorMessage(1), "Unknown error code");
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
