// regression: issue #379
// gena_validate_delivery_urls() must accept cross-subnet delivery URLs when
// both the device and the subscriber are on RFC 1918 private addresses.
//
// Root cause: the CallStranger fix (PR #181) checks the delivery URL against
// the device's configured netmask. In a multicast-routed environment the
// device (192.168.10.34/24) and the subscriber (192.168.27.65/24) can be on
// different /24 subnets but still both within the 192.168/16 private range.
// The UPnP spec says "for private networks the delivery URL must adhere to
// the RFC 1918 ranges", not "must be on the same subnet".

#include "gtest/gtest.h"

extern "C" {
#include "sock.h"
#include "upnpapi.h"
#include "uri.h"
}

#include <arpa/inet.h>
#include <cstring>
#include <netinet/in.h>

// Forward-declare the internal function (not exported in gena.h)
extern "C" int gena_validate_delivery_urls(SOCKINFO *info, URL_list *url_list);

static SOCKINFO make_sockinfo_ipv4(const char *peer_ip)
{
	SOCKINFO info{};
	auto *sa =
		reinterpret_cast<struct sockaddr_in *>(&info.foreign_sockaddr);
	sa->sin_family = AF_INET;
	inet_pton(AF_INET, peer_ip, &sa->sin_addr);
	info.foreign_sockaddr.ss_family = AF_INET;
	return info;
}

static URL_list make_url_list_ipv4(const char *delivery_ip)
{
	URL_list list{};
	list.size = 1;
	list.parsedURLs = new uri_type[1]{};
	auto *sa = reinterpret_cast<struct sockaddr_in *>(
		&list.parsedURLs[0].hostport.IPaddress);
	sa->sin_family = AF_INET;
	inet_pton(AF_INET, delivery_ip, &sa->sin_addr);
	return list;
}

class GenaValidateDeliveryUrlsTest : public ::testing::Test
{
protected:
	void TearDown() override
	{
		delete[] url_list_.parsedURLs;
		url_list_.parsedURLs = nullptr;
	}

	SOCKINFO info_{};
	URL_list url_list_{};
};

// Regression guard: same /24 subnet — must still be accepted after the fix.
TEST_F(GenaValidateDeliveryUrlsTest, same_subnet_accepted)
{
	strncpy(gIF_IPV4, "192.168.10.34", sizeof(gIF_IPV4));
	strncpy(gIF_IPV4_NETMASK, "255.255.255.0", sizeof(gIF_IPV4_NETMASK));
	info_ = make_sockinfo_ipv4("192.168.10.1");
	url_list_ = make_url_list_ipv4("192.168.10.100");

	EXPECT_EQ(gena_validate_delivery_urls(&info_, &url_list_), 0);
}

// Core regression: cross-subnet but same RFC 1918 192.168/16 block.
TEST_F(GenaValidateDeliveryUrlsTest, cross_subnet_same_rfc1918_192_168_accepted)
{
	strncpy(gIF_IPV4, "192.168.10.34", sizeof(gIF_IPV4));
	strncpy(gIF_IPV4_NETMASK, "255.255.255.0", sizeof(gIF_IPV4_NETMASK));
	info_ = make_sockinfo_ipv4("192.168.10.1");
	url_list_ = make_url_list_ipv4("192.168.27.65");

	EXPECT_EQ(gena_validate_delivery_urls(&info_, &url_list_), 0);
}

// Cross-RFC1918-class: device on 10/8, delivery on 192.168/16 — both private.
TEST_F(GenaValidateDeliveryUrlsTest, cross_rfc1918_class_10_to_192_accepted)
{
	strncpy(gIF_IPV4, "10.0.1.1", sizeof(gIF_IPV4));
	strncpy(gIF_IPV4_NETMASK, "255.0.0.0", sizeof(gIF_IPV4_NETMASK));
	info_ = make_sockinfo_ipv4("10.0.0.1");
	url_list_ = make_url_list_ipv4("192.168.27.65");

	EXPECT_EQ(gena_validate_delivery_urls(&info_, &url_list_), 0);
}

// Regression guard: public delivery URL must always be rejected.
TEST_F(GenaValidateDeliveryUrlsTest, public_delivery_url_rejected)
{
	strncpy(gIF_IPV4, "192.168.10.34", sizeof(gIF_IPV4));
	strncpy(gIF_IPV4_NETMASK, "255.255.255.0", sizeof(gIF_IPV4_NETMASK));
	info_ = make_sockinfo_ipv4("192.168.10.1");
	url_list_ = make_url_list_ipv4("203.0.113.5");

	EXPECT_EQ(gena_validate_delivery_urls(&info_, &url_list_), -1);
}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
