/* test_statcodes_race.c
 *
 * Regression test: http_get_code_text() must be safe to call from several
 * threads at the same time.
 *
 * The status code tables are filled in lazily by the first call to
 * http_get_code_text(). The initialisation was not synchronised, so worker
 * threads that built their first HTTP message at the same time (for example
 * two SSDP replies) wrote the tables concurrently: a data race that
 * ThreadSanitizer reports. The result is the same either way, so the test
 * only fails under ThreadSanitizer; it also checks the returned strings.
 *
 * statcodes.c is compiled into this test, because the function has hidden
 * visibility in the shared library.
 */

#include "config.h" // IWYU pragma: keep - must be first; defines UPNP_USE_RWLOCK before ithread.h

#include "ithread.h"
#include "statcodes.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_THREADS 8

static ithread_mutex_t gStartMutex;
static ithread_cond_t gStartCond;
static int gStart;

static void *worker(void *arg)
{
	int *bad = (int *)arg;
	const char *ok;
	const char *not_found;
	const char *error;

	/* Wait for the go signal, so all threads reach the first call
	 * together. */
	ithread_mutex_lock(&gStartMutex);
	while (!gStart)
		ithread_cond_wait(&gStartCond, &gStartMutex);
	ithread_mutex_unlock(&gStartMutex);

	ok = http_get_code_text(200);
	not_found = http_get_code_text(404);
	error = http_get_code_text(500);
	if (!ok || strcmp(ok, "OK") != 0 || !not_found ||
		strcmp(not_found, "Not Found") != 0 || !error ||
		strcmp(error, "Internal Server Error") != 0)
		*bad = 1;

	return NULL;
}

int main(void)
{
	ithread_t threads[NUM_THREADS];
	int bad[NUM_THREADS];
	int i;
	int failed = 0;

	ithread_mutex_init(&gStartMutex, NULL);
	ithread_cond_init(&gStartCond, NULL);
	memset(bad, 0, sizeof(bad));
	for (i = 0; i < NUM_THREADS; i++) {
		if (ithread_create(&threads[i], NULL, worker, &bad[i]) != 0) {
			fprintf(stderr, "ithread_create failed\n");
			return EXIT_FAILURE;
		}
	}
	ithread_mutex_lock(&gStartMutex);
	gStart = 1;
	ithread_cond_broadcast(&gStartCond);
	ithread_mutex_unlock(&gStartMutex);
	for (i = 0; i < NUM_THREADS; i++) {
		ithread_join(threads[i], NULL);
		failed |= bad[i];
	}
	ithread_cond_destroy(&gStartCond);
	ithread_mutex_destroy(&gStartMutex);
	if (failed) {
		fprintf(stderr, "FAIL: wrong status text\n");
		return EXIT_FAILURE;
	}
	puts("test_statcodes_race: PASS");

	return EXIT_SUCCESS;
}
