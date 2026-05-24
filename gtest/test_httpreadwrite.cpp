// regression: issue #394
// http_RecvMessage() buffers the entire request body before checking
// g_maxContentLength, so a large POST body exhausts RAM before the 413
// rejection fires.
//
// Root cause: the Content-Length check in http_RecvMessage() only runs after
// parser_append() returns PARSE_SUCCESS (body fully buffered). But
// parser->content_length is populated from the Content-Length header as soon
// as entity parsing begins — well before the full body is received.
//
// Fix: add an early check in the PARSE_INCOMPLETE branch of http_RecvMessage()
// so that an oversized Content-Length is rejected immediately after the headers
// are parsed, without buffering any body bytes.

#include "gtest/gtest.h"

extern "C" {
#include "httpparser.h"
#include "httpreadwrite.h"
#include "sock.h"
#include "statcodes.h"
#include "upnpapi.h"
}

#include <sys/socket.h>
#include <unistd.h>

// SOAP POST request advertising a 2 MB body — no body bytes follow.
static const char k_soap_hdr_2mb[] =
	"POST /upnp/control/service HTTP/1.1\r\n"
	"HOST: 192.168.1.1:49152\r\n"
	"Content-Type: text/xml; charset=\"utf-8\"\r\n"
	"Content-Length: 2097152\r\n"
	"SOAPAction: \"urn:schemas-upnp-org:service:AVTransport:1#Play\"\r\n"
	"\r\n";

class Issue394TestSuite : public ::testing::Test
{
protected:
	int sv[2]{-1, -1};
	size_t saved_limit_{};

	void SetUp() override
	{
		saved_limit_ = g_maxContentLength;
		g_maxContentLength = 1024; /* 1 KB limit */
		ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, sv), 0);
	}

	void TearDown() override
	{
		g_maxContentLength = saved_limit_;
		if (sv[0] >= 0)
			close(sv[0]);
		if (sv[1] >= 0)
			close(sv[1]);
	}
};

// regression: issue #394
// When Content-Length exceeds g_maxContentLength the library must reject the
// request as soon as the header is parsed — without reading the body.
// The write end is closed immediately after the headers so that any attempt
// to buffer the body would hit EOF instead of blocking, making the
// distinction between "rejected early" and "waited for body then rejected"
// observable via the returned error code.
TEST_F(Issue394TestSuite, RejectsOversizedBodyBeforeBuffering)
{
	write(sv[1], k_soap_hdr_2mb, sizeof(k_soap_hdr_2mb) - 1);
	close(sv[1]);
	sv[1] = -1;

	SOCKINFO info{};
	info.socket = sv[0];

	http_parser_t parser{};
	int timeout = 5;
	int http_err = 0;
	int ret = http_RecvMessage(
		&info, &parser, HTTPMETHOD_UNKNOWN, &timeout, &http_err);

	httpmsg_destroy(&parser.msg);

	EXPECT_EQ(ret, UPNP_E_OUTOF_BOUNDS);
	EXPECT_EQ(http_err, HTTP_REQ_ENTITY_TOO_LARGE);
}

// A body exactly at the limit must be accepted.
TEST_F(Issue394TestSuite, AcceptsBodyAtExactLimit)
{
	static const char hdr[] = "POST /upnp/control HTTP/1.1\r\n"
				  "HOST: 127.0.0.1:49152\r\n"
				  "Content-Type: text/xml\r\n"
				  "Content-Length: 1024\r\n"
				  "SOAPAction: \"urn:test#Action\"\r\n"
				  "\r\n";
	char body[1024];
	memset(body, 'X', sizeof(body));

	write(sv[1], hdr, sizeof(hdr) - 1);
	write(sv[1], body, sizeof(body));
	close(sv[1]);
	sv[1] = -1;

	SOCKINFO info{};
	info.socket = sv[0];

	http_parser_t parser{};
	int timeout = 5;
	int http_err = 0;
	int ret = http_RecvMessage(
		&info, &parser, HTTPMETHOD_UNKNOWN, &timeout, &http_err);

	httpmsg_destroy(&parser.msg);

	EXPECT_EQ(ret, UPNP_E_SUCCESS);
}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
