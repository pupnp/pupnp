#define UPNP_DEBUG_C

#include "ithread.h"
#include "upnp.h"
#include "upnpdebug.h"
#include "uri.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* parse_hostport() in uri.c uses this global for IPv6 scope IDs. */
unsigned gIF_INDEX = (unsigned)-1;
ithread_rwlock_t GlobalHndRWLock;

void UpnpPrintf(Upnp_LogLevel DLevel,
	Dbg_Module Module,
	const char *DbgFileName,
	int DbgLineNo,
	const char *FmtStr,
	...)
{
	(void)DLevel;
	(void)Module;
	(void)DbgFileName;
	(void)DbgLineNo;
	(void)FmtStr;
}

struct test_case
{
	const char *uri;
	int expected;
	int line;
};

#define TEST_INVALID_PORT(URI_VALUE) \
	{.uri = URI_VALUE, .expected = UPNP_E_INVALID_URL, .line = __LINE__}

static int run_test(const struct test_case *tc)
{
	uri_type parsed;
	int ret = parse_uri(tc->uri, strlen(tc->uri), &parsed);

	if (ret == tc->expected) {
		return 0;
	}

	printf("%s:%d parse_uri('%s') returned %d, expected %d\n",
		__FILE__,
		tc->line,
		tc->uri,
		ret,
		tc->expected);
	return 1;
}

/*
 * Regression test for OSS-Fuzz issue 531780528: parse_uri()/parse_hostport()
 * used to call getaddrinfo() synchronously whenever a URI's host was not a
 * literal IP address. Since parse_uri() runs on untrusted text (e.g. an
 * incoming HTTP request line), this let a remote peer block a worker thread
 * on DNS resolution -- a denial-of-service reachable without authentication
 * (CWE-400). It also explains the OSS-Fuzz report itself: libFuzzer aborts
 * (SIGABRT) on a per-run timeout, and the reported "crash" frames were
 * simply wherever execution was stuck when the watchdog fired.
 *
 * parse_uri() must never resolve a hostname: hostport.IPaddress must stay
 * unresolved (AF_UNSPEC) immediately after parsing, and the parsed port
 * must still be recorded separately for later use by resolve_hostport().
 */
static int check_no_eager_resolution(void)
{
	uri_type url;
	const char *s = "http://localhost:1234/path";

	if (parse_uri(s, strlen(s), &url) != HTTP_SUCCESS) {
		printf("%s:%d parse_uri('%s') failed to parse a valid URL\n",
			__FILE__,
			__LINE__,
			s);
		return 1;
	}
	if (url.hostport.IPaddress.ss_family != (sa_family_t)AF_UNSPEC) {
		printf("%s:%d parse_uri('%s') resolved the hostname eagerly "
		       "(ss_family = %d, expected AF_UNSPEC) -- DNS "
		       "resolution must be deferred to resolve_hostport()\n",
			__FILE__,
			__LINE__,
			s,
			(int)url.hostport.IPaddress.ss_family);
		return 1;
	}
	if (url.hostport.port != 1234) {
		printf("%s:%d parse_uri('%s') hostport.port = %d, expected "
		       "1234\n",
			__FILE__,
			__LINE__,
			s,
			(int)url.hostport.port);
		return 1;
	}

	return 0;
}

/* A literal IP address is resolved directly (inet_pton, no DNS) during
 * parse_uri() itself, so resolve_hostport() must be a fast no-op on it. */
static int check_literal_ip_is_noop(void)
{
	uri_type url;
	const char *s = "http://192.0.2.1:80/path";
	struct sockaddr_in *sai4;

	if (parse_uri(s, strlen(s), &url) != HTTP_SUCCESS) {
		printf("%s:%d parse_uri('%s') failed to parse a valid URL\n",
			__FILE__,
			__LINE__,
			s);
		return 1;
	}
	if (url.hostport.IPaddress.ss_family != (sa_family_t)AF_INET) {
		printf("%s:%d parse_uri('%s') literal IP should already be "
		       "resolved (ss_family = %d)\n",
			__FILE__,
			__LINE__,
			s,
			(int)url.hostport.IPaddress.ss_family);
		return 1;
	}
	if (resolve_hostport(&url.hostport) != HTTP_SUCCESS) {
		printf("%s:%d resolve_hostport() failed on a literal IP\n",
			__FILE__,
			__LINE__);
		return 1;
	}
	sai4 = (struct sockaddr_in *)&url.hostport.IPaddress;
	if (ntohl(sai4->sin_addr.s_addr) != 0xC0000201u /* 192.0.2.1 */) {
		printf("%s:%d resolved address does not match literal IP\n",
			__FILE__,
			__LINE__);
		return 1;
	}

	return 0;
}

/* resolve_hostport() must actually perform the deferred DNS resolution when
 * called explicitly, and must apply the parsed port. */
static int check_resolve_hostport_localhost(void)
{
	uri_type url;
	const char *s = "http://localhost:4321/path";
	struct sockaddr_in *sai4;
	struct sockaddr_in6 *sai6;

	if (parse_uri(s, strlen(s), &url) != HTTP_SUCCESS) {
		printf("%s:%d parse_uri('%s') failed to parse a valid URL\n",
			__FILE__,
			__LINE__,
			s);
		return 1;
	}
	if (resolve_hostport(&url.hostport) != HTTP_SUCCESS) {
		printf("%s:%d resolve_hostport() failed to resolve "
		       "localhost\n",
			__FILE__,
			__LINE__);
		return 1;
	}
	if (url.hostport.IPaddress.ss_family == (sa_family_t)AF_INET) {
		sai4 = (struct sockaddr_in *)&url.hostport.IPaddress;
		if (ntohs(sai4->sin_port) != 4321) {
			printf("%s:%d resolved port = %d, expected 4321\n",
				__FILE__,
				__LINE__,
				(int)ntohs(sai4->sin_port));
			return 1;
		}
	} else if (url.hostport.IPaddress.ss_family == (sa_family_t)AF_INET6) {
		sai6 = (struct sockaddr_in6 *)&url.hostport.IPaddress;
		if (ntohs(sai6->sin6_port) != 4321) {
			printf("%s:%d resolved port = %d, expected 4321\n",
				__FILE__,
				__LINE__,
				(int)ntohs(sai6->sin6_port));
			return 1;
		}
	} else {
		printf("%s:%d unexpected ss_family %d after resolving "
		       "localhost\n",
			__FILE__,
			__LINE__,
			(int)url.hostport.IPaddress.ss_family);
		return 1;
	}

	return 0;
}

int main(void)
{
	int i;
	int failures = 0;
	static const struct test_case tests[] = {
		TEST_INVALID_PORT("http://127.0.0.1:65536/"),
		TEST_INVALID_PORT("http://127.0.0.1:65537/"),
		TEST_INVALID_PORT("http://127.0.0.1:131073/"),
		TEST_INVALID_PORT("http://127.0.0.1:-1/"),
		TEST_INVALID_PORT("http://127.0.0.1:-65535/"),
	};

	for (i = 0; i < (int)(sizeof(tests) / sizeof(tests[0])); i++) {
		failures += run_test(&tests[i]);
	}

	failures += check_no_eager_resolution();
	failures += check_literal_ip_is_noop();
	failures += check_resolve_hostport_localhost();

	return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
