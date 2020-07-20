
#include "upnp.h"
#include "upnptools.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct test
{
	const char *base;
	const char *rel;
	const char *expect;
	int line;
	int error;
};

/*
#define TEST(BaseURL, RelURL, expect, ...) \
	{ \
		BaseURL, RelURL, expect, __LINE__, ##__VA_ARGS__ \
	}
*/

#define TEST(BaseURL, RelURL, expectURL) \
	{ \
		.base = BaseURL, .rel = RelURL, .expect = expectURL, \
		.line = __LINE__ \
	}

#define TEST_ERROR(BaseURL, RelURL, expectURL, error_code) \
	{ \
		.base = BaseURL, .rel = RelURL, .expect = expectURL, \
		.line = __LINE__, .error = error_code \
	}

static int result(const struct test *test)
{
	char *absurl = NULL;
	int ret = 0;

	ret = UpnpResolveURL2(test->base, test->rel, &absurl);
	if (ret == test->error &&
		(test->expect == NULL || strcmp(test->expect, absurl) == 0)) {
		ret = 0;
	} else {
		printf("%s:%d:  '%s' | '%s' -> '%s' != '%s' (%d)\n",
			__FILE__,
			test->line,
			test->base,
			test->rel,
			absurl,
			test->expect,
			ret);
		ret = 1;
	}
	free(absurl);
	return ret;
}

/* The URLs must be resolvale! */
static const char ABS_URL1[] = "http://pupnp.sourceforge.net/path1/";
static const char ABS_URL2[] = "http://pupnp.sourceforge.net/path1/path1";
// static const char ABS_URL3[] = "http://localhost/path1/";
// static const char ABS_URL4[] = "http://127.0.0.1/path1/";
// static const char ABS_URL5[] = "http://127.0.0.1:6544/path1/";
// static const char ABS_URL6[] = "http://[::1]:6544/path1/";

static const char REL_URL1[] = "//localhost/path2";
static const char REL_URL2[] = "/path3";
static const char REL_URL3[] = "path4";
static const char REL_URL4[] = "../path5";
// static const char REL_URL5[] = "?query1";
static const char REL_URL6[] = "#frag1";

static const char ABS_RFC[] = "http://localhost/b/c/d;p?q";
/*
s,\<a\>,localhost,
s,//g\>,//127.0.0.1,
*/

static const struct test RFC3986[] = {
	/* Errors */
	TEST_ERROR(NULL, NULL, NULL, UPNP_E_INVALID_PARAM),
	TEST_ERROR(ABS_URL1, NULL, NULL, UPNP_E_INVALID_PARAM),
	TEST_ERROR("foo", "bar", NULL, UPNP_E_INVALID_URL),
	/* Custom */
	TEST(NULL, ABS_URL1, ABS_URL1),
	TEST(ABS_URL1, ABS_URL2, ABS_URL2),
	TEST(ABS_URL1, "", ABS_URL1),
	TEST(ABS_URL1, REL_URL1, "http://localhost/path2"),
	TEST(ABS_URL2, REL_URL1, "http://localhost/path2"),
	TEST(ABS_URL1, REL_URL2, "http://pupnp.sourceforge.net/path3"),
	TEST(ABS_URL2, REL_URL2, "http://pupnp.sourceforge.net/path3"),
	TEST(ABS_URL1, REL_URL3, "http://pupnp.sourceforge.net/path1/path4"),
	TEST(ABS_URL2, REL_URL3, "http://pupnp.sourceforge.net/path1/path4"),
	TEST(ABS_URL1, REL_URL4, "http://pupnp.sourceforge.net/path5"),
	TEST(ABS_URL2, REL_URL4, "http://pupnp.sourceforge.net/path5"),
	TEST(ABS_URL1, REL_URL6, "http://pupnp.sourceforge.net/path1/#frag1"),
	TEST(ABS_URL2,
		REL_URL6,
		"http://pupnp.sourceforge.net/path1/path1#frag1"),
	TEST("http://127.0.0.1:6544/getDeviceDesc",
		"CDS_Event",
		"http://127.0.0.1:6544/CDS_Event"),
	/* <http://tools.ietf.org/html/rfc3986#section-5.4.1> Normal Examples */
	TEST(ABS_RFC, "g:h", "g:h"),
	TEST(ABS_RFC, "g", "http://localhost/b/c/g"),
	TEST(ABS_RFC, "./g", "http://localhost/b/c/g"),
	TEST(ABS_RFC, "g/", "http://localhost/b/c/g/"),
	TEST(ABS_RFC, "/g", "http://localhost/g"),
	TEST(ABS_RFC, "//127.0.0.1", "http://127.0.0.1"),
	TEST(ABS_RFC, "?y", "http://localhost/b/c/d;p?y"),
	TEST(ABS_RFC, "g?y", "http://localhost/b/c/g?y"),
	TEST(ABS_RFC, "#s", "http://localhost/b/c/d;p?q#s"),
	TEST(ABS_RFC, "g#s", "http://localhost/b/c/g#s"),
	TEST(ABS_RFC, "g?y#s", "http://localhost/b/c/g?y#s"),
	TEST(ABS_RFC, ";x", "http://localhost/b/c/;x"),
	TEST(ABS_RFC, "g;x", "http://localhost/b/c/g;x"),
	TEST(ABS_RFC, "g;x?y#s", "http://localhost/b/c/g;x?y#s"),
	TEST(ABS_RFC, "", "http://localhost/b/c/d;p?q"),
	TEST(ABS_RFC, ".", "http://localhost/b/c/"),
	TEST(ABS_RFC, "./", "http://localhost/b/c/"),
	TEST(ABS_RFC, "..", "http://localhost/b/"),
	TEST(ABS_RFC, "../", "http://localhost/b/"),
	TEST(ABS_RFC, "../g", "http://localhost/b/g"),
	TEST(ABS_RFC, "../..", "http://localhost/"),
	TEST(ABS_RFC, "../../", "http://localhost/"),
	TEST(ABS_RFC, "../../g", "http://localhost/g"),
	/* <http://tools.ietf.org/html/rfc3986#section-5.4.2> Abnormal Examples
	 */
	TEST(ABS_RFC, "../../../g", "http://localhost/g"),
	TEST(ABS_RFC, "../../../../g", "http://localhost/g"),
	TEST(ABS_RFC, "/./g", "http://localhost/g"),
	TEST(ABS_RFC, "/../g", "http://localhost/g"),
	TEST(ABS_RFC, "g.", "http://localhost/b/c/g."),
	TEST(ABS_RFC, ".g", "http://localhost/b/c/.g"),
	TEST(ABS_RFC, "g..", "http://localhost/b/c/g.."),
	TEST(ABS_RFC, "..g", "http://localhost/b/c/..g"),
	TEST(ABS_RFC, "./../g", "http://localhost/b/g"),
	TEST(ABS_RFC, "./g/.", "http://localhost/b/c/g/"),
	TEST(ABS_RFC, "g/./h", "http://localhost/b/c/g/h"),
	TEST(ABS_RFC, "g/../h", "http://localhost/b/c/h"),
	TEST(ABS_RFC, "g;x=1/./y", "http://localhost/b/c/g;x=1/y"),
	TEST(ABS_RFC, "g;x=1/../y", "http://localhost/b/c/y"),
	TEST(ABS_RFC, "g?y/./x", "http://localhost/b/c/g?y/./x"),
	TEST(ABS_RFC, "g?y/../x", "http://localhost/b/c/g?y/../x"),
	TEST(ABS_RFC, "g#s/./x", "http://localhost/b/c/g#s/./x"),
	TEST(ABS_RFC, "g#s/../x", "http://localhost/b/c/g#s/../x"),
	TEST(ABS_RFC, "http:g", "http:g"),
};
#define ARRAY_SIZE(a) (sizeof(a) / sizeof *(a))

int main()
{
	int i, ret = 0;

	for (i = 0; i < (int)ARRAY_SIZE(RFC3986); i++)
		ret += result(&RFC3986[i]);

	exit(ret ? EXIT_FAILURE : EXIT_SUCCESS);
}

/*
 gcc -o url-test -g url-test.c -I ixml/inc -I upnp/inc upnp/.libs/libupnp.a
 -L ixml/.libs -lixml -lpthread
 */
