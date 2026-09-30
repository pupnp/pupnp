/* test_virtual_dir.c
 *
 * Regression test: UpnpRemoveVirtualDir() must accept the same name that
 * UpnpAddVirtualDir() accepts.
 *
 * UpnpAddVirtualDir() prepends the missing leading '/' to the name it
 * stores, but UpnpRemoveVirtualDir() compared the name as given.
 * UpnpAddVirtualDir("vd") followed by UpnpRemoveVirtualDir("vd") therefore
 * failed with UPNP_E_INVALID_PARAM and left the directory registered.
 */

#include "upnp.h"

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

/* Adds add_name, removes remove_name and checks that the directory is gone:
 * adding it again must not report an old cookie. */
static int add_remove(const char *add_name, const char *remove_name)
{
	static int cookie_a;
	static int cookie_b;
	const void *old = &cookie_b;

	CHECK(UpnpAddVirtualDir(add_name, &cookie_a, &old) == UPNP_E_SUCCESS);
	CHECK(old == NULL);
	CHECK(UpnpRemoveVirtualDir(remove_name) == UPNP_E_SUCCESS);
	old = &cookie_b;
	CHECK(UpnpAddVirtualDir(add_name, &cookie_a, &old) == UPNP_E_SUCCESS);
	CHECK(old == NULL);
	CHECK(UpnpRemoveVirtualDir(remove_name) == UPNP_E_SUCCESS);

	return 0;
}

int main(void)
{
	int rc;

	rc = UpnpInit2(NULL, 0);
	if (rc != UPNP_E_SUCCESS) {
		fprintf(stderr,
			"UpnpInit2 failed (%d); skipping (no network?)\n",
			rc);
		return EXIT_SUCCESS;
	}

	/* Same spelling, with and without the leading slash. */
	if (add_remove("/with_slash", "/with_slash"))
		return EXIT_FAILURE;
	if (add_remove("no_slash", "no_slash"))
		return EXIT_FAILURE;
	/* Mixed spellings name the same directory. */
	if (add_remove("mixed_a", "/mixed_a"))
		return EXIT_FAILURE;
	if (add_remove("/mixed_b", "mixed_b"))
		return EXIT_FAILURE;

	/* Something that was never added is still an error. */
	CHECK(UpnpRemoveVirtualDir("never_added") == UPNP_E_INVALID_PARAM);
	CHECK(UpnpRemoveVirtualDir("/never_added") == UPNP_E_INVALID_PARAM);

	UpnpFinish();
	puts("test_virtual_dir: PASS");

	return EXIT_SUCCESS;
}
