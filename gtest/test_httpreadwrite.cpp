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

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
