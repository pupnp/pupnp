/* test_threadpool_burst.c
 *
 * Regression test: the thread pool must grow to run a burst of jobs.
 *
 * AddWorker() only created a thread when the jobs-per-thread ratio was
 * reached or when every thread was already busy. A thread counts as busy
 * only once it has picked a job up, so in a burst of jobs the queued ones
 * were invisible to the check: the pool stayed at its minimum size with jobs
 * waiting behind blocked ones while the maximum was far away. The miniserver
 * hands every accepted connection to such a pool, so a few connections that
 * block were enough to starve everybody else.
 *
 * The test models the miniserver pool: min 2 and max 12 threads, one of them
 * persistent (the accept loop), and blocking jobs.
 *
 * ThreadPool.c is compiled into the test because its symbols have hidden
 * visibility in the shared library.
 */

#include "config.h" // IWYU pragma: keep - must be first; defines UPNP_USE_RWLOCK before ithread.h

#include "ThreadPool.h"
#include "ithread.h"

#include <stdio.h>
#include <stdlib.h>

#define MIN_POOL_THREADS 2
#define MAX_POOL_THREADS 12
/* One thread of the pool is the persistent one. */
#define MAX_WORKERS (MAX_POOL_THREADS - 1)
#define WAIT_MS 1500

static ithread_mutex_t gMutex;
static ithread_cond_t gCond;
static int gRelease;
static int gStarted;

static void blocking_job(void *arg)
{
	(void)arg;
	ithread_mutex_lock(&gMutex);
	gStarted++;
	while (!gRelease)
		ithread_cond_wait(&gCond, &gMutex);
	ithread_mutex_unlock(&gMutex);
}

static void persistent_job(void *arg)
{
	(void)arg;
	ithread_mutex_lock(&gMutex);
	while (!gRelease)
		ithread_cond_wait(&gCond, &gMutex);
	ithread_mutex_unlock(&gMutex);
}

/* Waits until at least 'want' jobs have started or the timeout expires, and
 * returns how many have started. */
static int wait_started(int want)
{
	int waited = 0;
	int started;

	for (;;) {
		ithread_mutex_lock(&gMutex);
		started = gStarted;
		ithread_mutex_unlock(&gMutex);
		if (started >= want || waited >= WAIT_MS)
			return started;
		imillisleep(10);
		waited += 10;
	}
}

static void release_all(void)
{
	ithread_mutex_lock(&gMutex);
	gRelease = 1;
	ithread_cond_broadcast(&gCond);
	ithread_mutex_unlock(&gMutex);
}

/* Submits 'njobs' blocking jobs in a burst to a pool that has the
 * persistent thread and checks how many run at the same time and, once they
 * are released, that all of them run. */
static int run_burst(int njobs, int expect_running)
{
	ThreadPool tp;
	ThreadPoolAttr attr;
	ThreadPoolJob job;
	int id;
	int i;
	int started;

	gRelease = 0;
	gStarted = 0;
	TPAttrInit(&attr);
	TPAttrSetMinThreads(&attr, MIN_POOL_THREADS);
	TPAttrSetMaxThreads(&attr, MAX_POOL_THREADS);
	TPAttrSetJobsPerThread(&attr, JOBS_PER_THREAD);
	if (ThreadPoolInit(&tp, &attr) != 0) {
		fprintf(stderr, "ThreadPoolInit failed\n");
		return 1;
	}
	TPJobInit(&job, (start_routine)persistent_job, NULL);
	if (ThreadPoolAddPersistent(&tp, &job, &id) != 0) {
		fprintf(stderr, "ThreadPoolAddPersistent failed\n");
		release_all();
		ThreadPoolShutdown(&tp);
		return 1;
	}
	for (i = 0; i < njobs; i++) {
		TPJobInit(&job, (start_routine)blocking_job, NULL);
		if (ThreadPoolAdd(&tp, &job, NULL) != 0) {
			fprintf(stderr, "ThreadPoolAdd failed at job %d\n", i);
			release_all();
			ThreadPoolShutdown(&tp);
			return 1;
		}
	}

	started = wait_started(expect_running);
	printf("%d jobs: %d running at once (expected %d)\n",
		njobs,
		started,
		expect_running);
	if (started != expect_running) {
		fprintf(stderr,
			"FAIL: the pool did not grow to run the burst\n");
		release_all();
		ThreadPoolShutdown(&tp);
		return 1;
	}

	/* The jobs that had to wait must run once the others finish. */
	release_all();
	started = wait_started(njobs);
	if (started != njobs) {
		fprintf(stderr,
			"FAIL: only %d of %d jobs ran after the release\n",
			started,
			njobs);
		ThreadPoolShutdown(&tp);
		return 1;
	}
	ThreadPoolShutdown(&tp);

	return 0;
}

int main(void)
{
	int err = 0;

	ithread_mutex_init(&gMutex, NULL);
	ithread_cond_init(&gCond, NULL);
	/* A burst that fits in the pool: every job must get its own thread. */
	err |= run_burst(8, 8);
	/* A burst that does not fit: the pool stops at its maximum. */
	err |= run_burst(20, MAX_WORKERS);
	ithread_cond_destroy(&gCond);
	ithread_mutex_destroy(&gMutex);
	if (!err)
		puts("test_threadpool_burst: PASS");

	return err ? EXIT_FAILURE : EXIT_SUCCESS;
}
