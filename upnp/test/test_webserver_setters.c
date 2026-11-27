/* test_webserver_setters.c
 *
 * Regression test: UpnpSetWebServerRootDir() and UpnpSetWebServerCorsString()
 * are declared in upnp.h in every build, so they must exist in every build.
 *
 * Both were only defined with the internal web server, so a program that
 * calls either of them did not link against a library built with
 * -DUPNP_ENABLE_WEBSERVER=OFF. Without the web server they now return
 * UPNP_E_NO_WEB_SERVER, like UpnpEnableWebserver().
 */

#include "upnp.h"
#include "upnpconfig.h"

#include <stdio.h>
#include <stdlib.h>

#define CHECK(cond) \
	do { \
		if (!(cond)) { \
			printf("FAIL line %d: %s\n", __LINE__, #cond); \
			fflush(stdout); \
			UpnpFinish(); \
			return EXIT_FAILURE; \
		} \
	} while (0)

int main(void)
{
#ifdef UPNP_HAVE_WEBSERVER
	const int expected = UPNP_E_SUCCESS;
#else
	const int expected = UPNP_E_NO_WEB_SERVER;
#endif

	/* Before UpnpInit2() both report that the SDK is not initialised. */
	CHECK(UpnpSetWebServerRootDir(".") == UPNP_E_FINISH);
	CHECK(UpnpSetWebServerCorsString("*") == UPNP_E_FINISH);

	CHECK(UpnpInit2(NULL, 0) == UPNP_E_SUCCESS);
	CHECK(UpnpSetWebServerRootDir(".") == expected);
	CHECK(UpnpSetWebServerCorsString("*") == expected);
	UpnpFinish();

	printf("test_webserver_setters: PASS\n");

	return EXIT_SUCCESS;
}
