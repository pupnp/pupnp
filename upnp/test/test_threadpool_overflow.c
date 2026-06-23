/* test_threadpool_overflow.c
 *
 * Regression test for issue #178: thread pool overflow detection and
 * backtrace diagnostic.
 *
 * Uses ThreadPool directly (no full UPnP stack) with maxThreads=0 so
 * the queue fills deterministically with no worker-thread races.
 *
 * Test A: jobs beyond maxJobsTotal are dropped and the droppedJobs counter
 *         is incremented.
 *
 * Test B (POSIX + HAVE_BACKTRACE only): on the first overflow, stderr
 *         contains the "too many jobs" message followed by at least one
 *         backtrace frame line.
 */

#include "ThreadPool.h"
#include "config.h"
#include "ithread.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#ifdef HAVE_BACKTRACE
	#include <unistd.h>
#endif

/* Small limit so the test runs fast. */
#define MAX_JOBS 5

static void noop_job(void *arg) { (void)arg; }

/* Test A: overflow detection and droppedJobs counter. */
static int test_overflow_detection(void)
{
	ThreadPool tp;
	ThreadPoolAttr attr;
	ThreadPoolJob job;
	int i, rc;

	TPAttrInit(&attr);
	TPAttrSetMaxJobsTotal(&attr, MAX_JOBS);
	TPAttrSetMinThreads(&attr, 0);
	TPAttrSetMaxThreads(&attr, 0);

	if (ThreadPoolInit(&tp, &attr) != 0) {
		fprintf(stderr, "test A: ThreadPoolInit failed\n");
		return -1;
	}

	/* Fill the queue exactly to maxJobsTotal — all must succeed. */
	for (i = 0; i < MAX_JOBS; i++) {
		TPJobInit(&job, (start_routine)noop_job, NULL);
		rc = ThreadPoolAdd(&tp, &job, NULL);
		if (rc != 0) {
			fprintf(stderr,
				"test A: job %d rejected unexpectedly "
				"(rc=%d)\n",
				i,
				rc);
			ThreadPoolShutdown(&tp);
			return -1;
		}
	}

	/* One more must be dropped. */
	TPJobInit(&job, (start_routine)noop_job, NULL);
	rc = ThreadPoolAdd(&tp, &job, NULL);
	if (rc == 0) {
		fprintf(stderr,
			"test A: job %d accepted but overflow was expected\n",
			MAX_JOBS);
		ThreadPoolShutdown(&tp);
		return -1;
	}

	if (tp.stats.droppedJobs != 1) {
		fprintf(stderr,
			"test A: expected droppedJobs=1, got %ld\n",
			tp.stats.droppedJobs);
		ThreadPoolShutdown(&tp);
		return -1;
	}

	ThreadPoolShutdown(&tp);
	printf("test A PASS: overflow detected, droppedJobs=1\n");
	return 0;
}

#ifdef HAVE_BACKTRACE
/* Test B: backtrace lines appear on stderr on the first overflow. */
static int test_backtrace_output(void)
{
	ThreadPool tp;
	ThreadPoolAttr attr;
	ThreadPoolJob job;
	int i, pipefd[2], saved_fd;
	char buf[4096];
	ssize_t n;
	int linecount;

	TPAttrInit(&attr);
	TPAttrSetMaxJobsTotal(&attr, MAX_JOBS);
	TPAttrSetMinThreads(&attr, 0);
	TPAttrSetMaxThreads(&attr, 0);

	if (ThreadPoolInit(&tp, &attr) != 0) {
		fprintf(stderr, "test B: ThreadPoolInit failed\n");
		return -1;
	}

	/* Fill queue to maxJobsTotal. */
	for (i = 0; i < MAX_JOBS; i++) {
		TPJobInit(&job, (start_routine)noop_job, NULL);
		ThreadPoolAdd(&tp, &job, NULL);
	}

	/* Redirect stderr to a pipe before triggering the first overflow. */
	if (pipe(pipefd) != 0) {
		fprintf(stderr, "test B: pipe() failed\n");
		ThreadPoolShutdown(&tp);
		return -1;
	}
	fflush(stderr);
	saved_fd = dup(STDERR_FILENO);
	dup2(pipefd[1], STDERR_FILENO);
	close(pipefd[1]);

	/* Trigger the overflow: prints "too many jobs" message + backtrace. */
	TPJobInit(&job, (start_routine)noop_job, NULL);
	ThreadPoolAdd(&tp, &job, NULL);

	/* Restore stderr and collect what was written. */
	fflush(stderr);
	dup2(saved_fd, STDERR_FILENO);
	close(saved_fd);
	n = read(pipefd[0], buf, sizeof(buf) - 1);
	buf[n > 0 ? (size_t)n : 0] = '\0';
	close(pipefd[0]);

	ThreadPoolShutdown(&tp);

	/* Count newline-terminated lines. */
	linecount = 0;
	for (i = 0; buf[i]; i++)
		if (buf[i] == '\n')
			linecount++;

	/*
	 * Expect at least 2 lines: the "too many jobs" message and at least
	 * one backtrace frame from backtrace_symbols_fd().
	 */
	if (linecount < 2) {
		fprintf(stderr,
			"test B: expected >=2 lines on first overflow"
			" (got %d):\n%s\n",
			linecount,
			buf);
		return -1;
	}

	printf("test B PASS: backtrace produced %d lines\n", linecount);
	return 0;
}
#endif /* HAVE_BACKTRACE */

int main(void)
{
	if (test_overflow_detection() != 0)
		return EXIT_FAILURE;

#ifdef HAVE_BACKTRACE
	if (test_backtrace_output() != 0)
		return EXIT_FAILURE;
#endif

	return EXIT_SUCCESS;
}
