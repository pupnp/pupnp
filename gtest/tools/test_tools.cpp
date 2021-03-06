#include "gtest/gtest.h"
#include "ifaddrs.cpp"

TEST(ToolsTestSuite, test_initializing_interface_addresses)
{
    struct ifaddrs* ifaddr = nullptr;
    struct sockaddr_in* ifa_addr_in = nullptr;

    Ifaddr4 ifaddr4;
    ifaddr = ifaddr4.get();

    EXPECT_EQ(ifaddr->ifa_next, nullptr);
    EXPECT_STREQ(ifaddr->ifa_name, "lo");
    EXPECT_EQ(ifaddr->ifa_flags, (const unsigned int)0);
    ifa_addr_in = (sockaddr_in*)ifaddr->ifa_addr;
    EXPECT_EQ(ifa_addr_in->sin_family, AF_INET);
    EXPECT_EQ(ifa_addr_in->sin_addr.s_addr, (const unsigned int)16777343);
    ifa_addr_in = (sockaddr_in*)ifaddr->ifa_netmask;
    EXPECT_EQ(ifa_addr_in->sin_family, AF_INET);
    EXPECT_EQ(ifa_addr_in->sin_addr.s_addr, (const unsigned int)255);
    ifa_addr_in = (sockaddr_in*)ifaddr->ifa_broadaddr;
    EXPECT_EQ(ifa_addr_in->sin_family, 0);
    EXPECT_EQ(ifa_addr_in->sin_addr.s_addr, (const unsigned int)0);
    EXPECT_EQ(ifaddr->ifa_data, nullptr);
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
