#include "gtest/gtest.h"
#include "gmock/gmock.h"

// for TestSuites needing headers linked against the static C library
extern "C" {
    #include "upnp.h"
    #include "upnpapi.h"
    #include "upnputil.h"
}

#define EXPECT_IN_RANGE(VAL, MIN, MAX) \
    EXPECT_GE((VAL), (MIN));           \
    EXPECT_LE((VAL), (MAX))

using ::testing::MatchesRegex;


// UpnpApi Testsuite for IP4
//--------------------------
class UpnpApiIPv4TestSuite: public ::testing::Test
{
    // Fixtures for this Testsuite
    protected:
    std::string interface = "ens2";
    unsigned short PORT = 51515;
};

TEST_F(UpnpApiIPv4TestSuite, UpnpGetIfInfo)
{
    EXPECT_EQ(UpnpGetIfInfo(interface.c_str()), UPNP_E_SUCCESS);
    EXPECT_STREQ(gIF_NAME, interface.c_str());
    EXPECT_THAT(gIF_IPV4, MatchesRegex("[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}"));
    EXPECT_STREQ(gIF_IPV6, "");
    EXPECT_STREQ(gIF_IPV6_ULA_GUA, "");
    EXPECT_IN_RANGE((int)gIF_INDEX, 1, 20);
}

TEST_F(UpnpApiIPv4TestSuite, UpnpInit2)
{
    int return_value = UpnpInit2(interface.c_str(), PORT);
    EXPECT_EQ(return_value, UPNP_E_SUCCESS);
}


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
