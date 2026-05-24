/* poc_gh_153.c
 *
 * Regression test for issue #153: UpnpUnRegisterRootDevice does not free
 * everything — use-after-free in the alias lifecycle under concurrent access.
 *
 * Sequence that triggers the bug:
 *   1. Install alias (refcount = 1)
 *   2. Simulate HTTP handler grabbing alias (refcount = 2)
 *   3. Uninstall alias via web_server_ut_set_alias(NULL,...) (refcount = 1)
 *   4. Handler releases its copy (refcount -> 0, buffers freed; but
 *      gAliasDoc.doc.buf is left as a dangling non-NULL pointer)
 *   5. UpnpFinish() -> web_server_destroy() -> alias_release(&gAliasDoc):
 *      is_valid_alias() returns true (dangling buf != NULL), then
 *      *gAliasDoc.ct is dereferenced -> use-after-free caught by ASan.
 *
 * regression: issue #153
 */

#include "upnp.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

/* regression: issue #153 — test hooks exported from libupnp */
extern void web_server_ut_grab_alias(void);
extern void web_server_ut_release_alias(void);
extern int web_server_ut_set_alias(
	const char *name, const char *content, size_t len);

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

	/* Step 1: install alias (refcount = 1) */
	rc = web_server_ut_set_alias("/desc.xml", "<root/>", 7);
	if (rc != UPNP_E_SUCCESS) {
		fprintf(stderr, "web_server_ut_set_alias failed (%d)\n", rc);
		UpnpFinish();
		return EXIT_FAILURE;
	}

	/* Step 2: simulate an HTTP handler grabbing the alias (refcount = 2) */
	web_server_ut_grab_alias();

	/* Step 3: uninstall alias, as UpnpUnRegisterRootDevice would (refcount
	 * = 1) */
	web_server_ut_set_alias(NULL, NULL, 0);

	/* Step 4: handler releases its local copy (refcount -> 0).
	 * BUG: gAliasDoc.doc.buf is left as a dangling non-NULL pointer. */
	web_server_ut_release_alias();

	/* Step 5: UpnpFinish() -> web_server_destroy() ->
	 * alias_release(&gAliasDoc). Without the fix, this dereferences the
	 * freed gAliasDoc.ct. */
	UpnpFinish();

	puts("PASS: no use-after-free detected.");
	return EXIT_SUCCESS;
}
