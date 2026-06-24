/* test_ithread_rwlock.c
 *
 * Verifies that ithread_rwlock_t provides true read-write lock semantics:
 * multiple threads can hold a read lock simultaneously.
 *
 * When ithread.h is processed before config.h, UPNP_USE_RWLOCK is undefined
 * (defaults to 0) and ithread_rwlock_t silently aliases to pthread_mutex_t.
 * That causes HandleLock/HandleReadLock in upnpapi.h to call pthread_mutex_*
 * on a GlobalHndRWLock that upnpapi.c initialized as a pthread_rwlock_t,
 * resulting in undefined behaviour and deadlock.
 *
 * The test holds a read lock on the main thread and spawns two reader threads.
 * A real rwlock allows both to proceed concurrently; a mutex would block them
 * until the main thread releases -- which it never does before the timeout.
 *
 * On platforms where UPNP_USE_RWLOCK=0 the mutex fallback is intentional and
 * this test is skipped (exit 77).  The test is also skipped on Windows because
 * clock_gettime/CLOCK_REALTIME are not available in the MSVC CRT.
 */

#include "config.h" /* IWYU pragma: keep - must be first; defines UPNP_USE_RWLOCK before ithread.h */

#include "ithread.h"

#include <stdio.h>

#if !UPNP_USE_RWLOCK || defined(_WIN32)
int main(void)
{
	/* UPNP_USE_RWLOCK=0: mutex fallback is intentional on this platform.
	 * _WIN32: clock_gettime/CLOCK_REALTIME not available in the MSVC CRT.
	 */
	return 77;
}
#else

	#include <pthread.h>
	#include <time.h>

static ithread_rwlock_t g_rwlock;
static int g_reader_count;
static pthread_mutex_t g_mutex;
static pthread_cond_t g_cond;

static void *reader_thread(void *arg)
{
	(void)arg;
	ithread_rwlock_rdlock(&g_rwlock);

	pthread_mutex_lock(&g_mutex);
	g_reader_count++;
	pthread_cond_signal(&g_cond);
	pthread_mutex_unlock(&g_mutex);

	ithread_rwlock_unlock(&g_rwlock);
	return NULL;
}

int main(void)
{
	pthread_t t1, t2;
	struct timespec deadline;
	int concurrent_count;
	int rc;

	ithread_rwlock_init(&g_rwlock, NULL);
	pthread_mutex_init(&g_mutex, NULL);
	pthread_cond_init(&g_cond, NULL);

	/* Hold a read lock so threads must share it with us (not wait for us).
	 */
	ithread_rwlock_rdlock(&g_rwlock);

	pthread_create(&t1, NULL, reader_thread, NULL);
	pthread_create(&t2, NULL, reader_thread, NULL);

	clock_gettime(CLOCK_REALTIME, &deadline);
	deadline.tv_sec += 5;

	pthread_mutex_lock(&g_mutex);
	while (g_reader_count < 2) {
		rc = pthread_cond_timedwait(&g_cond, &g_mutex, &deadline);
		if (rc != 0)
			break;
	}
	/* Capture the count while still holding g_mutex; after the unlock and
	 * join, threads that were blocked on g_rwlock will run and increment
	 * g_reader_count, making the final value unreliable for the message. */
	concurrent_count = g_reader_count;
	rc = (concurrent_count >= 2) ? 0 : 1;
	pthread_mutex_unlock(&g_mutex);

	ithread_rwlock_unlock(&g_rwlock);
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);

	ithread_rwlock_destroy(&g_rwlock);
	pthread_mutex_destroy(&g_mutex);
	pthread_cond_destroy(&g_cond);

	if (rc != 0) {
		printf("FAIL: ithread_rwlock_t does not allow concurrent "
		       "readers "
		       "(concurrent=%d, expected 2; "
		       "ithread_rwlock_t may be aliased to pthread_mutex_t)\n",
			concurrent_count);
		return 1;
	}
	puts("PASS: ithread_rwlock_t allows concurrent readers");
	return 0;
}

#endif /* UPNP_USE_RWLOCK */
