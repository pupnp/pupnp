// SoapSendAction() and SoapSendActionEx() wrote the service type into the
// SOAPAction header as given. A control point usually takes it from the
// serviceType element of the description document a device served, where
// "&#13;&#10;" decodes to CR LF, so a device could add header lines to the
// request or end the header block early.

#include "upnp.h"
#include "gtest/gtest.h"
#include <cstring>
#include <string>
#include <thread>

extern "C" {
#include "soaplib.h"
}

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

class SoapActionHeaderTestSuite : public ::testing::Test
{
protected:
	int listen_fd_{-1};
	sockaddr_in addr_{};
	std::string request_;
	std::thread server_;
	IXML_Document *action_{nullptr};
	IXML_Document *header_{nullptr};

	void SetUp() override
	{
		listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
		ASSERT_GE(listen_fd_, 0);
		addr_.sin_family = AF_INET;
		addr_.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		ASSERT_EQ(bind(listen_fd_,
				  reinterpret_cast<sockaddr *>(&addr_),
				  sizeof(addr_)),
			0);
		ASSERT_EQ(listen(listen_fd_, 1), 0);
		socklen_t len = sizeof(addr_);
		ASSERT_EQ(getsockname(listen_fd_,
				  reinterpret_cast<sockaddr *>(&addr_),
				  &len),
			0);
		ASSERT_EQ(ixmlParseBufferEx("<u:Act xmlns:u=\"urn:x\"></u:Act>",
				  &action_),
			IXML_SUCCESS);
		ASSERT_EQ(ixmlParseBufferEx("<h:Hdr xmlns:h=\"urn:x\"></h:Hdr>",
				  &header_),
			IXML_SUCCESS);
		server_ = std::thread([this] {
			// Nothing connects when the request is refused.
			pollfd lpfd{listen_fd_, POLLIN, 0};
			if (poll(&lpfd, 1, 2000) <= 0)
				return;
			int fd = accept(listen_fd_, nullptr, nullptr);
			if (fd < 0)
				return;
			char buf[1024];
			pollfd pfd{fd, POLLIN, 0};
			while (request_.find("\r\n\r\n") == std::string::npos &&
				poll(&pfd, 1, 2000) > 0) {
				ssize_t n = read(fd, buf, sizeof(buf));
				if (n <= 0)
					break;
				request_.append(buf, (size_t)n);
			}
			static const char resp[] = "HTTP/1.1 404 Not Found\r\n"
						   "Content-Length: 0\r\n"
						   "\r\n";
			(void)write(fd, resp, sizeof(resp) - 1);
			close(fd);
		});
	}

	void TearDown() override
	{
		if (server_.joinable())
			server_.join();
		if (listen_fd_ >= 0)
			close(listen_fd_);
		ixmlDocument_free(action_);
		ixmlDocument_free(header_);
	}

	std::string Url()
	{
		return "http://127.0.0.1:" +
		       std::to_string(ntohs(addr_.sin_port)) + "/ctl";
	}

	void ExpectNoInjectedHeader()
	{
		server_.join();
		EXPECT_EQ(request_.find("X-Injected"), std::string::npos)
			<< request_;
	}
};

TEST_F(SoapActionHeaderTestSuite, SendActionRejectsCrlfInServiceType)
{
	std::string url = Url();
	char service_type[] = "urn:x:service:y:1\r\nX-Injected: 1\r\nZ: \"z";
	IXML_Document *resp = nullptr;
	EXPECT_EQ(SoapSendAction(&url[0], service_type, action_, &resp),
		UPNP_E_INVALID_PARAM);
	ixmlDocument_free(resp);
	ExpectNoInjectedHeader();
}

TEST_F(SoapActionHeaderTestSuite, SendActionExRejectsCrlfInServiceType)
{
	std::string url = Url();
	char service_type[] = "urn:x:service:y:1\r\nX-Injected: 1\r\nZ: \"z";
	IXML_Document *resp = nullptr;
	EXPECT_EQ(SoapSendActionEx(
			  &url[0], service_type, header_, action_, &resp),
		UPNP_E_INVALID_PARAM);
	ixmlDocument_free(resp);
	ExpectNoInjectedHeader();
}

TEST_F(SoapActionHeaderTestSuite, SendActionKeepsValidServiceType)
{
	std::string url = Url();
	char service_type[] = "urn:x:service:y:1";
	IXML_Document *resp = nullptr;
	(void)SoapSendAction(&url[0], service_type, action_, &resp);
	ixmlDocument_free(resp);
	server_.join();
	EXPECT_NE(
		request_.find("\r\nSOAPAction: \"urn:x:service:y:1#Act\"\r\n"),
		std::string::npos)
		<< request_;
}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
