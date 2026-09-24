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

#include "upnp.h"
#include "gtest/gtest.h"
#include <cstddef>
#include <cstring>
#include <string>

extern "C" {
#include "httpparser.h"
#include "httpreadwrite.h"
#include "sock.h"
#include "statcodes.h"
#include "upnpapi.h"
}

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
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

// regression: GHSA-gcj7-j9f7-q84c
// Transfer-Encoding: chunked bypasses g_maxContentLength: content_length is
// never set for chunked transfers, so the existing limit checks are skipped
// while the decoded entity accumulates past the configured maximum.
class GhsaGcj7TestSuite : public ::testing::Test
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

// A chunked response body larger than g_maxContentLength must be rejected.
TEST_F(GhsaGcj7TestSuite, ChunkedBodyExceedingLimitIsRejected)
{
	static const char hdr[] = "HTTP/1.1 200 OK\r\n"
				  "Transfer-Encoding: chunked\r\n"
				  "\r\n";

	char chunk_data[2048];
	memset(chunk_data, 'X', sizeof(chunk_data));

	write(sv[1], hdr, sizeof(hdr) - 1);
	write(sv[1], "800\r\n", 5); /* 0x800 = 2048 bytes */
	write(sv[1], chunk_data, sizeof(chunk_data));
	write(sv[1], "\r\n", 2);
	write(sv[1], "0\r\n\r\n", 5); /* terminator */
	close(sv[1]);
	sv[1] = -1;

	SOCKINFO info{};
	info.socket = sv[0];

	http_parser_t parser{};
	int timeout = 5;
	int http_err = 0;
	int ret = http_RecvMessage(
		&info, &parser, HTTPMETHOD_GET, &timeout, &http_err);

	httpmsg_destroy(&parser.msg);

	EXPECT_EQ(ret, UPNP_E_OUTOF_BOUNDS);
	EXPECT_EQ(http_err, HTTP_REQ_ENTITY_TOO_LARGE);
}

// A chunked response body exactly at the limit must be accepted.
TEST_F(GhsaGcj7TestSuite, ChunkedBodyAtExactLimitIsAccepted)
{
	static const char hdr[] = "HTTP/1.1 200 OK\r\n"
				  "Transfer-Encoding: chunked\r\n"
				  "\r\n";

	char chunk_data[1024];
	memset(chunk_data, 'X', sizeof(chunk_data));

	write(sv[1], hdr, sizeof(hdr) - 1);
	write(sv[1], "400\r\n", 5); /* 0x400 = 1024 bytes */
	write(sv[1], chunk_data, sizeof(chunk_data));
	write(sv[1], "\r\n", 2);
	write(sv[1], "0\r\n\r\n", 5); /* terminator */
	close(sv[1]);
	sv[1] = -1;

	SOCKINFO info{};
	info.socket = sv[0];

	http_parser_t parser{};
	int timeout = 5;
	int http_err = 0;
	int ret = http_RecvMessage(
		&info, &parser, HTTPMETHOD_GET, &timeout, &http_err);

	httpmsg_destroy(&parser.msg);

	EXPECT_EQ(ret, UPNP_E_SUCCESS);
}

// regression: GHSA-g6c2-x4r9-352g
// raw_to_int() casts strtol() (long, 64-bit on LP64) to int (32-bit),
// silently truncating Content-Length values above INT_MAX.
// Content-Length: 4294967301 (2^32+5) is stored as content_length=5,
// causing the parser to accept a 5-byte body as if the declared length
// were valid.
class GhsaG6c2TestSuite : public ::testing::Test
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

// Content-Length: 4294967301 (2^32+5) must be rejected, not silently
// truncated to 5 and accepted with a 5-byte body.
TEST_F(GhsaG6c2TestSuite, TruncatedContentLengthIsRejected)
{
	/* 4294967301 = 2^32 + 5; (int) cast truncates to 5 */
	static const char hdr[] = "NOTIFY /upnp/event HTTP/1.1\r\n"
				  "Host: 127.0.0.1:49152\r\n"
				  "Content-Length: 4294967301\r\n"
				  "\r\n";

	write(sv[1], hdr, sizeof(hdr) - 1);
	write(sv[1], "HELLO", 5);
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

// A request with a legitimate small Content-Length must still be accepted.
TEST_F(GhsaG6c2TestSuite, ValidSmallContentLengthIsAccepted)
{
	static const char hdr[] = "NOTIFY /upnp/event HTTP/1.1\r\n"
				  "Host: 127.0.0.1:49152\r\n"
				  "Content-Length: 5\r\n"
				  "\r\n";

	write(sv[1], hdr, sizeof(hdr) - 1);
	write(sv[1], "HELLO", 5);
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

// regression: GHSA-hg5x-73vf-vfm2
// Two related bugs in chunked Transfer-Encoding parsing:
//
// 1. A hex chunk-size token is parsed by match_int() into a 64-bit `long`
//    (via strtol) but only checked for `num < 0`, then narrowed to `int`
//    with `*value = (int)num;` — silently wrapping values above INT_MAX
//    instead of rejecting them. Worse, the call site
//    (parser_parse_chunky_entity, "%x%L%c") passes `&parser->chunk_size`,
//    which is `size_t`, into a vararg slot read back as `int *`
//    (httpparser.c's match(), case 'x'/'d') — a type-confused write that
//    only ever touches the low 4 bytes of the 8-byte field. A declared
//    chunk size of "100000000" (2^32) wraps to 0 and is misread as the
//    chunked-entity terminator, letting an attacker-declared multi-
//    gigabyte chunk be silently accepted as an empty, already-complete
//    entity.
//
// 2. Even with a syntactically valid (in-range) chunk size, the declared
//    size is never compared against g_maxContentLength before the parser
//    buffers the chunk's bytes: parser->msg.entity.length is only
//    incremented in parser_parse_chunky_body() once the *entire* declared
//    chunk has already been received into parser->msg.msg (see
//    httpparser.c:1693), so the existing g_maxContentLength check in
//    http_RecvMessage() (which reads entity.length) never fires while a
//    single oversized chunk is still streaming in.
class GhsaHg5xTestSuite : public ::testing::Test
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

// A chunk-size line declaring 2^32 ("100000000") must not be silently
// truncated to 0 and treated as an already-complete, empty entity.
TEST_F(GhsaHg5xTestSuite, OverflowingChunkSizeIsRejected)
{
	static const char hdr[] = "HTTP/1.1 200 OK\r\n"
				  "Transfer-Encoding: chunked\r\n"
				  "\r\n";

	write(sv[1], hdr, sizeof(hdr) - 1);
	/* 0x100000000 = 2^32; (int) truncation wraps this to 0, which is
	 * the chunked-entity terminator, followed by an empty trailer
	 * header block. */
	write(sv[1], "100000000\r\n", 11);
	write(sv[1], "\r\n", 2);
	close(sv[1]);
	sv[1] = -1;

	SOCKINFO info{};
	info.socket = sv[0];

	http_parser_t parser{};
	int timeout = 5;
	int http_err = 0;
	int ret = http_RecvMessage(
		&info, &parser, HTTPMETHOD_GET, &timeout, &http_err);

	httpmsg_destroy(&parser.msg);

	EXPECT_NE(ret, UPNP_E_SUCCESS);
}

// A single declared chunk larger than g_maxContentLength must be rejected
// as soon as its buffered bytes exceed the limit, without waiting for the
// full declared chunk size to be received.
TEST_F(GhsaHg5xTestSuite, OversizedSingleChunkIsRejectedBeforeBuffering)
{
	static const char hdr[] = "HTTP/1.1 200 OK\r\n"
				  "Transfer-Encoding: chunked\r\n"
				  "\r\n";

	char chunk_data[2048];
	memset(chunk_data, 'X', sizeof(chunk_data));

	write(sv[1], hdr, sizeof(hdr) - 1);
	/* Declare an 8 KB chunk (0x2000), but only ever send 2 KB of it —
	 * more than g_maxContentLength (1 KB), but less than the declared
	 * chunk size, so the chunk never completes and entity.length is
	 * never updated. */
	write(sv[1], "2000\r\n", 6);
	write(sv[1], chunk_data, sizeof(chunk_data));
	close(sv[1]);
	sv[1] = -1;

	SOCKINFO info{};
	info.socket = sv[0];

	http_parser_t parser{};
	int timeout = 5;
	int http_err = 0;
	int ret = http_RecvMessage(
		&info, &parser, HTTPMETHOD_GET, &timeout, &http_err);

	httpmsg_destroy(&parser.msg);

	EXPECT_EQ(ret, UPNP_E_OUTOF_BOUNDS);
	EXPECT_EQ(http_err, HTTP_REQ_ENTITY_TOO_LARGE);
}

// A chunk size within range and under the limit must still be accepted.
TEST_F(GhsaHg5xTestSuite, ValidChunkSizeIsAccepted)
{
	static const char hdr[] = "HTTP/1.1 200 OK\r\n"
				  "Transfer-Encoding: chunked\r\n"
				  "\r\n";

	char chunk_data[512];
	memset(chunk_data, 'X', sizeof(chunk_data));

	write(sv[1], hdr, sizeof(hdr) - 1);
	write(sv[1], "200\r\n", 5); /* 0x200 = 512 bytes */
	write(sv[1], chunk_data, sizeof(chunk_data));
	write(sv[1], "\r\n", 2);
	write(sv[1], "0\r\n\r\n", 5); /* terminator */
	close(sv[1]);
	sv[1] = -1;

	SOCKINFO info{};
	info.socket = sv[0];

	http_parser_t parser{};
	int timeout = 5;
	int http_err = 0;
	int ret = http_RecvMessage(
		&info, &parser, HTTPMETHOD_GET, &timeout, &http_err);

	httpmsg_destroy(&parser.msg);

	EXPECT_EQ(ret, UPNP_E_SUCCESS);
}

// regression: GHSA-f86h-hm8r-cqxr
// The HTTP header block was accepted with no size limit at all:
// g_maxContentLength constrains only the entity, so a single unauthenticated
// request could carry an unbounded number of headers, or one unbounded header
// value, and drive memory and CPU exhaustion before any handler ran. Header
// insertion also scans the existing list for a duplicate name, so cost was
// quadratic in header count.
//
// Fixed by bounding the header block in parser_append(), gated on the parser
// still reading the request/response line or the header fields.
class GhsaF86hTestSuite : public ::testing::Test
{
protected:
	int sv[2]{-1, -1};
	size_t saved_header_limit_{};
	size_t saved_content_limit_{};

	void SetUp() override
	{
		saved_header_limit_ = g_maxHeaderSize;
		saved_content_limit_ = g_maxContentLength;
		g_maxHeaderSize = 1024; /* 1 KB header block */
		ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, sv), 0);
	}

	void TearDown() override
	{
		g_maxHeaderSize = saved_header_limit_;
		g_maxContentLength = saved_content_limit_;
		if (sv[0] >= 0)
			close(sv[0]);
		if (sv[1] >= 0)
			close(sv[1]);
	}
};

// Many small headers whose combined size exceeds the limit must be rejected.
// Before the fix this was accepted, and the per-header duplicate scan made the
// cost quadratic in the number of headers.
TEST_F(GhsaF86hTestSuite, RejectsHeaderBlockExceedingLimitByCount)
{
	std::string req = "GET /desc.xml HTTP/1.1\r\n"
			  "HOST: 127.0.0.1:49152\r\n";
	/* ~14 bytes each, comfortably past the 1 KB limit */
	for (int i = 0; i < 200; i++)
		req += "X-Pad-" + std::to_string(i) + ": v\r\n";
	req += "\r\n";
	ASSERT_GT(req.size(), g_maxHeaderSize);

	write(sv[1], req.data(), req.size());
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
	EXPECT_EQ(http_err, HTTP_REQ_HEADER_FIELDS_TOO_LARGE);
}

// A single header line longer than the limit must be rejected while it is
// still arriving. This is the case a per-header counter inside the
// parser_parse_headers() loop cannot catch: until the line terminates, match()
// returns PARSE_INCOMPLETE and the loop body never runs, so the limit has to be
// enforced in parser_append(). The terminating CRLF is deliberately never
// sent — rejection must happen on size alone, not at end of message.
TEST_F(GhsaF86hTestSuite, RejectsSingleOversizedHeaderLine)
{
	std::string req = "GET /desc.xml HTTP/1.1\r\n"
			  "HOST: 127.0.0.1:49152\r\n"
			  "X-Big: ";
	req.append(4096, 'A'); /* no CRLF, no end of headers */

	write(sv[1], req.data(), req.size());
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
	EXPECT_EQ(http_err, HTTP_REQ_HEADER_FIELDS_TOO_LARGE);
}

// The same parser handles responses, so a control point talking to a malicious
// server must be protected too.
TEST_F(GhsaF86hTestSuite, RejectsOversizedHeaderBlockOnResponse)
{
	std::string resp = "HTTP/1.1 200 OK\r\n";
	for (int i = 0; i < 200; i++)
		resp += "X-Pad-" + std::to_string(i) + ": v\r\n";
	resp += "\r\n";
	ASSERT_GT(resp.size(), g_maxHeaderSize);

	write(sv[1], resp.data(), resp.size());
	close(sv[1]);
	sv[1] = -1;

	SOCKINFO info{};
	info.socket = sv[0];

	http_parser_t parser{};
	int timeout = 5;
	int http_err = 0;
	int ret = http_RecvMessage(
		&info, &parser, HTTPMETHOD_GET, &timeout, &http_err);

	httpmsg_destroy(&parser.msg);

	EXPECT_EQ(ret, UPNP_E_OUTOF_BOUNDS);
	EXPECT_EQ(http_err, HTTP_REQ_HEADER_FIELDS_TOO_LARGE);
}

// A header block under the limit must still be accepted.
TEST_F(GhsaF86hTestSuite, AcceptsHeaderBlockUnderLimit)
{
	static const char req[] = "GET /desc.xml HTTP/1.1\r\n"
				  "HOST: 127.0.0.1:49152\r\n"
				  "USER-AGENT: test/1.0\r\n"
				  "\r\n";

	write(sv[1], req, sizeof(req) - 1);
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

// The default limit must be generous enough for real UPnP traffic: a SUBSCRIBE
// with a long multi-URL CALLBACK header is the worst realistic case.
TEST_F(GhsaF86hTestSuite, AcceptsTypicalUpnpRequestAtDefaultLimit)
{
	g_maxHeaderSize = DEFAULT_MAX_HEADER_SIZE;

	std::string req = "SUBSCRIBE /upnp/event/service HTTP/1.1\r\n"
			  "HOST: 192.168.1.1:49152\r\n"
			  "USER-AGENT: OS/1.0 UPnP/2.0 product/1.0\r\n"
			  "CALLBACK: ";
	for (int i = 0; i < 20; i++)
		req += "<http://192.168.1.100:8080/event/callback/" +
		       std::to_string(i) + "> ";
	req += "\r\n"
	       "NT: upnp:event\r\n"
	       "TIMEOUT: Second-1800\r\n"
	       "\r\n";
	ASSERT_LT(req.size(), g_maxHeaderSize);

	write(sv[1], req.data(), req.size());
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

// The header limit must not leak into the entity. A chunked body larger than
// g_maxHeaderSize is entity data, not header data, and is governed by
// g_maxContentLength — the check is gated on parser position precisely so that
// chunked trailer headers, parsed at POS_ENTITY, are not caught by it.
TEST_F(GhsaF86hTestSuite, DoesNotApplyHeaderLimitToEntity)
{
	g_maxContentLength = 65536; /* well above the body below */

	static const char hdr[] = "HTTP/1.1 200 OK\r\n"
				  "Transfer-Encoding: chunked\r\n"
				  "\r\n";
	char chunk_data[4096]; /* 4x the 1 KB header limit */
	memset(chunk_data, 'X', sizeof(chunk_data));

	write(sv[1], hdr, sizeof(hdr) - 1);
	write(sv[1], "1000\r\n", 6); /* 0x1000 = 4096 bytes */
	write(sv[1], chunk_data, sizeof(chunk_data));
	write(sv[1], "\r\n", 2);
	write(sv[1], "0\r\n\r\n", 5); /* terminator */
	close(sv[1]);
	sv[1] = -1;

	SOCKINFO info{};
	info.socket = sv[0];

	http_parser_t parser{};
	int timeout = 5;
	int http_err = 0;
	int ret = http_RecvMessage(
		&info, &parser, HTTPMETHOD_GET, &timeout, &http_err);

	httpmsg_destroy(&parser.msg);

	EXPECT_EQ(ret, UPNP_E_SUCCESS);
}

// Setting the limit to 0 disables the check, matching UpnpSetMaxContentLength.
TEST_F(GhsaF86hTestSuite, ZeroLimitDisablesTheCheck)
{
	g_maxHeaderSize = 0;

	std::string req = "GET /desc.xml HTTP/1.1\r\n"
			  "HOST: 127.0.0.1:49152\r\n";
	for (int i = 0; i < 200; i++)
		req += "X-Pad-" + std::to_string(i) + ": v\r\n";
	req += "\r\n";

	write(sv[1], req.data(), req.size());
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

// 431 must render a reason phrase: the 4xx table previously stopped at 417,
// so reaching index 31 required extending it.
TEST_F(GhsaF86hTestSuite, StatusCode431HasReasonPhrase)
{
	const char *text = http_get_code_text(HTTP_REQ_HEADER_FIELDS_TOO_LARGE);

	ASSERT_NE(text, nullptr);
	EXPECT_STREQ(text, "Request Header Fields Too Large");
}

// regression: GHSA-q4rx-xfwj-m2gp
// The streaming HTTP GET client (UpnpOpenHttpGet() and friends) reads the
// response line and headers in ReadResponseLineAndHeaders(), which appended to
// the message buffer directly instead of going through parser_append(), so the
// GHSA-f86h header-block limit never applied. A malicious server streaming
// header lines without the terminating blank line grew the client's memory
// without bound, and the timeout never fired while data kept arriving.
//
// The server writes the whole oversized response and keeps the connection
// open. The fixed client stops reading once the limit is exceeded, leaving the
// rest unread in its socket; the unfixed client drains everything and then
// times out.
class GhsaQ4rxTestSuite : public ::testing::Test
{
protected:
	int listen_fd_{-1};
	int server_fd_{-1};
	void *handle_{};
	size_t saved_header_limit_{};

	void SetUp() override
	{
		saved_header_limit_ = g_maxHeaderSize;
		g_maxHeaderSize = 1024; /* 1 KB header block */

		listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
		ASSERT_GE(listen_fd_, 0);
		sockaddr_in addr{};
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		ASSERT_EQ(bind(listen_fd_,
				  reinterpret_cast<sockaddr *>(&addr),
				  sizeof(addr)),
			0);
		ASSERT_EQ(listen(listen_fd_, 1), 0);
		socklen_t len = sizeof(addr);
		ASSERT_EQ(getsockname(listen_fd_,
				  reinterpret_cast<sockaddr *>(&addr),
				  &len),
			0);

		std::string url = "http://127.0.0.1:" +
				  std::to_string(ntohs(addr.sin_port)) + "/x";
		ASSERT_EQ(http_OpenHttpConnection(url.c_str(), &handle_, 5),
			UPNP_E_SUCCESS);
		server_fd_ = accept(listen_fd_, nullptr, nullptr);
		ASSERT_GE(server_fd_, 0);
		/* Sends the GET and initializes the response parser. */
		ASSERT_EQ(http_MakeHttpRequest(UPNP_HTTPMETHOD_GET,
				  url.c_str(),
				  handle_,
				  nullptr,
				  nullptr,
				  0,
				  5),
			UPNP_E_SUCCESS);
	}

	void TearDown() override
	{
		g_maxHeaderSize = saved_header_limit_;
		if (handle_)
			http_CloseHttpConnection(handle_);
		if (server_fd_ >= 0)
			close(server_fd_);
		if (listen_fd_ >= 0)
			close(listen_fd_);
	}

	// The connection handle is private to httpreadwrite.c, but its first
	// member is the SOCKINFO, so the handle pointer converts to it.
	int ClientSocket() const
	{
		return static_cast<SOCKINFO *>(handle_)->socket;
	}

	int Unread() const
	{
		int n = 0;
		ioctl(ClientSocket(), FIONREAD, &n);
		return n;
	}

	// Send the response and wait until all of it sits in the client's
	// receive buffer, so the unread count afterwards is exact.
	void Send(const std::string &resp)
	{
		ASSERT_EQ(write(server_fd_, resp.data(), resp.size()),
			(ssize_t)resp.size());
		for (int i = 0; i < 500 && Unread() < (int)resp.size(); i++)
			usleep(1000);
		ASSERT_EQ(Unread(), (int)resp.size());
	}
};

// Endless header lines after a valid status line must be rejected once the
// header block exceeds the limit (second read loop).
TEST_F(GhsaQ4rxTestSuite, RejectsOversizedResponseHeaderBlock)
{
	std::string resp = "HTTP/1.1 200 OK\r\n";
	while (resp.size() < 16 * 1024)
		resp += "A: b\r\n"; /* never terminated by a blank line */
	Send(resp);

	int ret = http_GetHttpResponse(
		handle_, nullptr, nullptr, nullptr, nullptr, 1);

	EXPECT_EQ(ret, UPNP_E_BAD_RESPONSE);
	EXPECT_GT(Unread(), 0);
}

// A status line that never ends must be rejected once it exceeds the limit
// (first read loop).
TEST_F(GhsaQ4rxTestSuite, RejectsOversizedResponseLine)
{
	std::string resp = "HTTP/1.1 200 ";
	resp.append(16 * 1024, 'A'); /* no CRLF */
	Send(resp);

	int ret = http_GetHttpResponse(
		handle_, nullptr, nullptr, nullptr, nullptr, 1);

	EXPECT_EQ(ret, UPNP_E_BAD_RESPONSE);
	EXPECT_GT(Unread(), 0);
}

// A normal response under the limit must still be accepted.
TEST_F(GhsaQ4rxTestSuite, AcceptsResponseUnderLimit)
{
	Send("HTTP/1.1 200 OK\r\n"
	     "Content-Type: text/xml\r\n"
	     "Content-Length: 0\r\n"
	     "\r\n");

	int status = 0;
	int ret = http_GetHttpResponse(
		handle_, nullptr, nullptr, nullptr, &status, 1);

	EXPECT_EQ(ret, UPNP_E_SUCCESS);
	EXPECT_EQ(status, HTTP_OK);
}

// regression: GHSA-v5wv-qg6r-f87r
// Chunked framing escaped both size limits. Trailer headers are parsed at
// POS_ENTITY, so the g_maxHeaderSize check skipped them, and g_maxContentLength
// only counts chunk data, so trailer bytes were never charged. A chunk-size
// line that is never terminated kept match() at PARSE_INCOMPLETE, so the
// GHSA-hg5x single-chunk check never ran either. In both cases every received
// byte stayed buffered, growing memory without bound.
//
// Fixed by bounding the trailer block and the chunk-size line by
// g_maxHeaderSize inside the chunked parser, which also covers callers that
// parse the entity without going through parser_append().
using GhsaV5wvTestSuite = GhsaF86hTestSuite;

static const char k_chunked_resp_hdr[] = "HTTP/1.1 200 OK\r\n"
					 "Transfer-Encoding: chunked\r\n"
					 "\r\n"
					 "5\r\nhello\r\n";

// Endless trailer lines after the last chunk must be rejected once the trailer
// block exceeds the limit. The blank line that ends the trailers is never sent.
TEST_F(GhsaV5wvTestSuite, RejectsOversizedTrailerBlock)
{
	std::string resp = k_chunked_resp_hdr;
	resp += "0\r\n";
	for (int i = 0; i < 200; i++)
		resp += "X-Trailer-" + std::to_string(i) + ": v\r\n";
	ASSERT_GT(resp.size(), g_maxHeaderSize);

	write(sv[1], resp.data(), resp.size());
	close(sv[1]);
	sv[1] = -1;

	SOCKINFO info{};
	info.socket = sv[0];

	http_parser_t parser{};
	int timeout = 5;
	int http_err = 0;
	int ret = http_RecvMessage(
		&info, &parser, HTTPMETHOD_GET, &timeout, &http_err);

	httpmsg_destroy(&parser.msg);

	EXPECT_EQ(ret, UPNP_E_OUTOF_BOUNDS);
	EXPECT_EQ(http_err, HTTP_REQ_HEADER_FIELDS_TOO_LARGE);
}

// A chunk-size line that never ends (here through an endless chunk extension)
// must be rejected once it exceeds the limit.
TEST_F(GhsaV5wvTestSuite, RejectsUnterminatedChunkSizeLine)
{
	std::string resp = k_chunked_resp_hdr;
	resp += "5;ext=";
	resp.append(4096, 'a'); /* no CRLF */

	write(sv[1], resp.data(), resp.size());
	close(sv[1]);
	sv[1] = -1;

	SOCKINFO info{};
	info.socket = sv[0];

	http_parser_t parser{};
	int timeout = 5;
	int http_err = 0;
	int ret = http_RecvMessage(
		&info, &parser, HTTPMETHOD_GET, &timeout, &http_err);

	httpmsg_destroy(&parser.msg);

	EXPECT_EQ(ret, UPNP_E_OUTOF_BOUNDS);
	EXPECT_EQ(http_err, HTTP_REQ_HEADER_FIELDS_TOO_LARGE);
}

// Trailers under the limit must still be accepted.
TEST_F(GhsaV5wvTestSuite, AcceptsTrailersUnderLimit)
{
	std::string resp = k_chunked_resp_hdr;
	resp += "0\r\n"
		"X-Checksum: abc\r\n"
		"X-Other: def\r\n"
		"\r\n";

	write(sv[1], resp.data(), resp.size());
	close(sv[1]);
	sv[1] = -1;

	SOCKINFO info{};
	info.socket = sv[0];

	http_parser_t parser{};
	int timeout = 5;
	int http_err = 0;
	int ret = http_RecvMessage(
		&info, &parser, HTTPMETHOD_GET, &timeout, &http_err);

	EXPECT_EQ(ret, UPNP_E_SUCCESS);
	EXPECT_EQ(parser.msg.entity.length, 5u);

	httpmsg_destroy(&parser.msg);
}

// The streaming body reader parses the entity without parser_append(), so the
// limit must be enforced inside the chunked parser for it to be covered too.
using GhsaV5wvStreamTestSuite = GhsaQ4rxTestSuite;

TEST_F(GhsaV5wvStreamTestSuite, RejectsOversizedTrailerBlockWhenStreaming)
{
	/* The status line and headers are read 2 KB at a time, and the
	 * header check counts the whole buffer, so the limit must exceed one
	 * read for the headers to get through. */
	g_maxHeaderSize = 4096;

	std::string resp = k_chunked_resp_hdr;
	resp += "0\r\n";
	while (resp.size() < 16 * 1024)
		resp += "A: b\r\n"; /* never terminated by a blank line */
	Send(resp);

	ASSERT_EQ(http_GetHttpResponse(
			  handle_, nullptr, nullptr, nullptr, nullptr, 1),
		UPNP_E_SUCCESS);
	char buf[64];
	size_t size = sizeof(buf);
	int ret = http_ReadHttpResponse(handle_, buf, &size, 1);

	EXPECT_EQ(ret, UPNP_E_BAD_RESPONSE);
	EXPECT_GT(Unread(), 0);
}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
