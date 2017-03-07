/*******************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation 
 * All rights reserved. 
 * Copyright (c) 2012 France Telecom All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met: 
 *
 * - Redistributions of source code must retain the above copyright notice, 
 * this list of conditions and the following disclaimer. 
 * - Redistributions in binary form must reproduce the above copyright notice, 
 * this list of conditions and the following disclaimer in the documentation 
 * and/or other materials provided with the distribution. 
 * - Neither name of Intel Corporation nor the names of its contributors 
 * may be used to endorse or promote products derived from this software 
 * without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR 
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY 
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

/*!
 * \file
 */

#if !defined(WIN32)
	#include <sys/param.h>
#endif

#include "ThreadPool.h"

#include "FreeList.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>	/* for memset()*/

/*!
 * \brief Returns the difference in milliseconds between two timeval structures.
 *
 * \internal
 *
 * \return The difference in milliseconds, time1-time2.
 */
static long DiffMillis(
	/*! . */
	struct timeval *time1,
	/*! . */
	struct timeval *time2)
{
	double temp = 0.0;

	temp = (double)time1->tv_sec - (double)time2->tv_sec;
	/* convert to milliseconds */
	temp *= 1000.0;

	/* convert microseconds to milliseconds and add to temp */
	/* implicit flooring of unsigned long data type */
	temp += ((double)time1->tv_usec - (double)time2->tv_usec) / 1000.0;

	return (long)temp;
}

#ifdef STATS
/*!
 * \brief Initializes the statistics structure.
 *
 * \internal
 */
static void StatsInit(
	/*! Must be valid non null stats structure. */
	ThreadPoolStats *stats)
{
	stats->totalIdleTime = 0.0;
	stats->totalJobsHQ = 0;
	stats->totalJobsLQ = 0;
	stats->totalJobsMQ = 0;
	stats->totalTimeHQ = 0.0;
	stats->totalTimeMQ = 0.0;
	stats->totalTimeLQ = 0.0;
	stats->totalWorkTime = 0.0;
	stats->totalIdleTime = 0.0;
	stats->avgWaitHQ = 0.0;
	stats->avgWaitMQ = 0.0;
	stats->avgWaitLQ = 0.0;
	stats->workerThreads = 0;
	stats->idleThreads = 0;
	stats->persistentThreads = 0;
	stats->maxThreads = 0;
	stats->totalThreads = 0;
}

/*!
 * \brief 
 *
 * \internal
 */
static void StatsAccountLQ(
	/*! . */
	ThreadPool *tp,
	/*! . */
	long diffTime)
{
	tp->stats.totalJobsLQ++;
	tp->stats.totalTimeLQ += (double)diffTime;
}

/*!
 * \brief 
 *
 * \internal
 */
static void StatsAccountMQ(
	/*! . */
	ThreadPool *tp,
	/*! . */
	long diffTime)
{
	tp->stats.totalJobsMQ++;
	tp->stats.totalTimeMQ += (double)diffTime;
}

/*!
 * \brief 
 *
 * \internal
 */
static void StatsAccountHQ(
	/*! . */
	ThreadPool *tp,
	/*! . */
	long diffTime)
{
	tp->stats.totalJobsHQ++;
	tp->stats.totalTimeHQ += (double)diffTime;
}

/*!
 * \brief Calculates the time the job has been waiting at the specified
 * priority.
 *
 * Adds to the totalTime and totalJobs kept in the thread pool statistics
 * structure.
 *
 * \internal
 */
static void CalcWaitTime(
	/*! . */
	ThreadPool *tp,
	/*! . */
	ThreadPriority p,
	/*! . */
	ThreadPoolJob *job)
{
	struct timeval now;
	long diff;

	assert(tp != NULL);
	assert(job != NULL);

	gettimeofday(&now, NULL);
	diff = DiffMillis(&now, &job->requestTime);
	switch (p) {
	case LOW_PRIORITY:
		StatsAccountLQ(tp, diff);
		break;
	case MED_PRIORITY:
		StatsAccountMQ(tp, diff);
		break;
	case HIGH_PRIORITY:
		StatsAccountHQ(tp, diff);
		break;
	default:
		assert(0);
	}
}

/*!
 * \brief 
 *
 * \internal
 */
static time_t StatsTime(
	/*! . */
	time_t *t)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	if (t)
		*t = tv.tv_sec;

	return tv.tv_sec;
}
#else /* STATS */
static UPNP_INLINE void StatsInit(ThreadPoolStats *stats) {}
static UPNP_INLINE void StatsAccountLQ(ThreadPool *tp, long diffTime) {}
static UPNP_INLINE void StatsAccountMQ(ThreadPool *tp, long diffTime) {}
static UPNP_INLINE void StatsAccountHQ(ThreadPool *tp, long diffTime) {}
static UPNP_INLINE void CalcWaitTime(ThreadPool *tp, ThreadPriority p, ThreadPoolJob *job) {}
static UPNP_INLINE time_t StatsTime(time_t *t) { return 0; }
#endif /* STATS */

/*!
 * \brief Compares thread pool jobs.
 *
 * \internal
 */
static int CmpThreadPoolJob(
	void *jobA,
	void *jobB)
{
	ThreadPoolJob *a = (ThreadPoolJob *)jobA;
	ThreadPoolJob *b = (ThreadPoolJob *)jobB;

	return a->jobId == b->jobId;
}

/*!
 * \brief Deallocates a dynamically allocated ThreadPoolJob.
 *
 * \internal
 */
static void FreeThreadPoolJob(
	/*! . */
	ThreadPool *tp,
	/*! Must be allocated with CreateThreadPoolJob. */
	ThreadPoolJob *tpj)
{
	FreeListFree(&tp->jobFreeList, tpj);
}

/*!
 * \brief Sets the scheduling policy of the current process.
 *
 * \internal
 * 
 * \return
 * 	\li \c 0 on success.
 *      \li \c result of GetLastError() on failure.
 *
 */
static int SetPolicyType(
	/*! . */
	PolicyType in)
{
	int retVal = 0;
#ifdef __CYGWIN__
	/* TODO not currently working... */
	retVal = 0;
#elif defined(__OSX__) || defined(__APPLE__) || defined(__NetBSD__)
	setpriority(PRIO_PROCESS, 0, 0);
	retVal = 0;
#elif defined(WIN32)
	retVal = sched_setscheduler(0, in);
#elif defined(_POSIX_PRIORITY_SCHEDULING) && _POSIX_PRIORITY_SCHEDULING > 0
	struct sched_param current;
	int sched_result;

	memset(&current, 0, sizeof(current));
	sched_getparam(0, &current);
	current.sched_priority = sched_get_priority_min(DEFAULT_POLICY);
	sched_result = sched_setscheduler(0, in, &current);
	retVal = (sched_result != -1 || errno == EPERM) ? 0 : errno;
#else
	retVal = 0;
#endif
	return retVal;
}

/*!
 * \brief Sets the priority of the currently running thread.
 *
 * \internal
 * 
 * \return
 *	\li \c 0 on success.
 *      \li \c EINVAL invalid priority or the result of GerLastError.
 */
static int SetPriority(
	/*! . */
	ThreadPriority priority)
{
#if defined(_POSIX_PRIORITY_SCHEDULING) && _POSIX_PRIORITY_SCHEDULING > 0
	int retVal = 0;
	int currentPolicy;
	int minPriority = 0;
	int maxPriority = 0;
	int actPriority = 0;
	int midPriority = 0;
	struct sched_param newPriority;
	int sched_result;

	pthread_getschedparam(ithread_self(), &currentPolicy, &newPriority);
	minPriority = sched_get_priority_min(currentPolicy);
	maxPriority = sched_get_priority_max(currentPolicy);
	midPriority = (maxPriority - minPriority) / 2;
	switch (priority) {
	case LOW_PRIORITY:
		actPriority = minPriority;
		break;
	case MED_PRIORITY:
		actPriority = midPriority;
		break;
	case HIGH_PRIORITY:
		actPriority = maxPriority;
		break;
	default:
		retVal = EINVAL;
		goto exit_function;
	};

	newPriority.sched_priority = actPriority;

	sched_result = pthread_setschedparam(ithread_self(), currentPolicy, &newPriority);
	retVal = (sched_result == 0 || errno == EPERM) ? 0 : sched_result;
exit_function:
	return retVal;
#else
	return 0;
	priority = priority;
#endif
}

/*!
 * \brief Determines whether any jobs need to be bumped to a higher priority Q
 * and bumps them.
 *
 * tp->mutex must be locked.
 *
 * \internal
 * 
 * \return
 */
static void BumpPriority(
	/*! . */
	ThreadPool *tp)
{
	int done = 0;
	struct timeval now;
	long diffTime = 0;
	ThreadPoolJob *tempJob = NULL;

	gettimeofday(&now, NULL);	
	while (!done) {
		if (tp->medJobQ.size) {
			tempJob = (ThreadPoolJob *)tp->medJobQ.head.next->item;
			diffTime = DiffMillis(&now, &tempJob->requestTime);
			if (diffTime >= tp->attr.starvationTime) {
				/* If job has waited longer than the starvation time
				* bump priority (add to higher priority Q) */
				StatsAccountMQ(tp, diffTime);
				ListDelNode(&tp->medJobQ, tp->medJobQ.head.next, 0);
				ListAddTail(&tp->highJobQ, tempJob);
				continue;
			}
		}
		if (tp->lowJobQ.size) {
			tempJob = (ThreadPoolJob *)tp->lowJobQ.head.next->item;
			diffTime = DiffMillis(&now, &tempJob->requestTime);
			if (diffTime >= tp->attr.maxIdleTime) {
				/* If job has waited longer than the starvation time
				 * bump priority (add to higher priority Q) */
				StatsAccountLQ(tp, diffTime);
				ListDelNode(&tp->lowJobQ, tp->lowJobQ.head.next, 0);
				ListAddTail(&tp->medJobQ, tempJob);
				continue;
			}
		}
		done = 1;
	}
}

/*!
 * \brief Sets the fields of the passed in timespec to be relMillis
 * milliseconds in the future.
 *
 * \internal
 */
static void SetRelTimeout(
	/*! . */
	struct timespec *time,
	/*! milliseconds in the future. */
	int relMillis)
{
	struct timeval now;
	int sec = relMillis / 1000;
	int milliSeconds = relMillis % 1000;

	gettimeofday(&now, NULL);
	time->tv_sec = now.tv_sec + sec;
	time->tv_nsec = (now.tv_usec / 1000 + milliSeconds) * 1000000;
}

/*!
 * \brief Sets seed for random number generator. Each thread sets the seed
 * random number generator.
 *
 * \internal
 */
static void SetSeed(void)
{
	struct timeval t;
  
	gettimeofday(&t, NULL);
#if defined(WIN32)
	srand((unsigned int)t.tv_usec + (unsigned int)ithread_get_current_thread_id().p);
#elif defined(BSD) || defined(__OSX__) || defined(__APPLE__) || defined(__FreeBSD_kernel__)
	srand((unsigned int)t.tv_usec + (unsigned int)ithread_get_current_thread_id());
#elif defined(__linux__) || defined(__sun) || defined(__CYGWIN__) || defined(__GLIBC__)
	srand((unsigned int)t.tv_usec + (unsigned int)ithread_get_current_thread_id());
#else
	{
		volatile union {
			volatile pthread_t tid;
			volatile unsigned i;
		} idu;

		idu.tid = ithread_get_current_thread_id();
		srand((unsigned int)t.millitm + idu.i);
	}
#endif
}

/*!
 * \brief Implements a thread pool worker. Worker waits for a job to become
 * available. Worker picks up persistent jobs first, high priority,
 * med priority, then low priority.
 *
 * If worker remains idle for more than specified max, the worker is released.
 *
 * \internal
 */
static void *WorkerThread(
	/*! arg -> is cast to (ThreadPool *). */
	void *arg)
{
	time_t start = 0;

	ThreadPoolJob *job = NULL;
	ListNode *head = NULL;

	struct timespec timeout;
	int retCode = 0;
	int persistent = -1;
	ThreadPool *tp = (ThreadPool *) arg;

	ithread_initialize_thread();

	/* Increment total thread count */
	ithread_mutex_lock(&tp->mutex);
	tp->totalThreads++;
	tp->pendingWorkerThreadStart = 0;
	ithread_cond_broadcast(&tp->start_and_shutdown);
	ithread_mutex_unlock(&tp->mutex);

	SetSeed();
	StatsTime(&start);
	while (1) {
		ithread_mutex_lock(&tp->mutex);
		if (job) {
			tp->busyThreads--;
			FreeThreadPoolJob(tp, job);
			job = NULL;
		}
		retCode = 0;
		tp->stats.idleThreads++;
		tp->stats.totalWorkTime += (double)StatsTime(NULL) - (double)start;
		StatsTime(&start);
		if (persistent == 0) {
			tp->stats.workerThreads--;
		} else if (persistent == 1) {
			/* Persistent thread becomes a regular thread */
			tp->persistentThreads--;
		}

		/* Check for a job or shutdown */
		while (tp->lowJobQ.size  == 0 &&
		       tp->medJobQ.size  == 0 &&
		       tp->highJobQ.size == 0 &&
		       !tp->persistentJob && !tp->shutdown) {
			/* If wait timed out and we currently have more than the
			 * min threads, or if we have more than the max threads
			 * (only possible if the attributes have been reset)
			 * let this thread die. */
			if ((retCode == ETIMEDOUT &&
			    tp->totalThreads > tp->attr.minThreads) ||
			    (tp->attr.maxThreads != -1 &&
			     tp->totalThreads > tp->attr.maxThreads)) {
				tp->stats.idleThreads--;
				goto exit_function;
			}
			SetRelTimeout(&timeout, tp->attr.maxIdleTime);

			/* wait for a job up to the specified max time */
			retCode = ithread_cond_timedwait(
				&tp->condition, &tp->mutex, &timeout);
		}
		tp->stats.idleThreads--;
		/* idle time */
		tp->stats.totalIdleTime += (double)StatsTime(NULL) - (double)start;
		/* work time */
		StatsTime(&start);
		/* bump priority of starved jobs */
		BumpPriority(tp);
		/* if shutdown then stop */
		if (tp->shutdown) {
			goto exit_function;
		} else {
			/* Pick up persistent job if available */
			if (tp->persistentJob) {
				job = tp->persistentJob;
				tp->persistentJob = NULL;
				tp->persistentThreads++;
				persistent = 1;
				ithread_cond_broadcast(&tp->start_and_shutdown);
			} else {
				tp->stats.workerThreads++;
				persistent = 0;
				/* Pick the highest priority job */
				if (tp->highJobQ.size > 0) {
					head = ListHead(&tp->highJobQ);
					if (head == NULL) {
						tp->stats.workerThreads--;
						goto exit_function;
					}
					job = (ThreadPoolJob *) head->item;
					CalcWaitTime(tp, HIGH_PRIORITY, job);
					ListDelNode(&tp->highJobQ, head, 0);
				} else if (tp->medJobQ.size > 0) {
					head = ListHead(&tp->medJobQ);
					if (head == NULL) {
						tp->stats.workerThreads--;
						goto exit_function;
					}
					job = (ThreadPoolJob *) head->item;
					CalcWaitTime(tp, MED_PRIORITY, job);
					ListDelNode(&tp->medJobQ, head, 0);
				} else if (tp->lowJobQ.size > 0) {
					head = ListHead(&tp->lowJobQ);
					if (head == NULL) {
						tp->stats.workerThreads--;
						goto exit_function;
					}
					job = (ThreadPoolJob *) head->item;
					CalcWaitTime(tp, LOW_PRIORITY, job);
					ListDelNode(&tp->lowJobQ, head, 0);
				} else {
					/* Should never get here */
					tp->stats.workerThreads--;
					goto exit_function;
				}
			}
		}

		tp->busyThreads++;
		ithread_mutex_unlock(&tp->mutex);

		/* In the future can log info */
		if (SetPriority(job->priority) != 0) {
		} else {
		}
		/* run the job */
		job->func(job->arg);
		/* return to Normal */
		SetPriority(DEFAULT_PRIORITY);
	}

exit_function:
	tp->totalThreads--;
	ithread_cond_broadcast(&tp->start_and_shutdown);
	ithread_mutex_unlock(&tp->mutex);
	ithread_cleanup_thread();

	return NULL;
}

/*!
 * \brief Creates a Thread Pool Job. (Dynamically allocated)
 *
 * \internal
 *
 * \return ThreadPoolJob *on success, NULL on failure.
 */
static ThreadPoolJob *CreateThreadPoolJob(
	/*! job is copied. */
	ThreadPoolJob *job,
	/*! id of job. */
	int id,
	/*! . */
	ThreadPool *tp)
{
	ThreadPoolJob *newJob = NULL;

	newJob = (ThreadPoolJob *)FreeListAlloc(&tp->jobFreeList);
	if (newJob) {
		*newJob = *job;
		newJob->jobId = id;
		gettimeofday(&newJob->requestTime, NULL);
	}

	return newJob;
}

/*!
 * \brief Creates a worker thread, if the thread pool does not already have
 * max threads.
 *
 * \remark The ThreadPool object mutex must be locked prior to calling this
 * function.
 *
 * \internal
 *
 * \return
 *	\li \c 0 on success, < 0 on failure.
 *	\li \c EMAXTHREADS if already max threads reached.
 *	\li \c EAGAIN if system can not create thread.
 */
static int CreateWorker(
	/*! A pointer to the ThreadPool object. */
	ThreadPool *tp)
{
	ithread_t temp;
	int rc = 0;
	ithread_attr_t attr;

	/* if a new worker is the process of starting, wait until it fully starts */
	while (tp->pendingWorkerThreadStart) {
		ithread_cond_wait(&tp->start_and_shutdown, &tp->mutex);
	}

	if (tp->attr.maxThreads != INFINITE_THREADS &&
	    tp->totalThreads + 1 > tp->attr.maxThreads) {
		return EMAXTHREADS;
	}
	ithread_attr_init(&attr);
	ithread_attr_setstacksize(&attr, tp->attr.stackSize);
	ithread_attr_setdetachstate(&attr, ITHREAD_CREATE_DETACHED);
	rc = ithread_create(&temp, &attr, WorkerThread, tp);
	ithread_attr_destroy(&attr);
	if (rc == 0) {
		rc = ithread_detach(temp);
		/* ithread_detach will return EINVAL if thread has been
		 successfully detached by ithread_create */
		if (rc == EINVAL)
			rc = 0;
		tp->pendingWorkerThreadStart = 1;
		/* wait until the new worker thread starts */
		while (tp->pendingWorkerThreadStart) {
			ithread_cond_wait(&tp->start_and_shutdown, &tp->mutex);
		}
	}
	if (tp->stats.maxThreads < tp->totalThreads) {
		tp->stats.maxThreads = tp->totalThreads;
	}

	return rc;
}

/*!
 * \brief Determines whether or not a thread should be added based on the
 * jobsPerThread ratio. Adds a thread if appropriate.
 *
 * \remark The ThreadPool object mutex must be locked prior to calling this
 * function.
 *
 * \internal
 */
static void AddWorker(
	/*! A pointer to the ThreadPool object. */
	ThreadPool *tp)
{
	long jobs = 0;
	int threads = 0;

	jobs = tp->highJobQ.size + tp->lowJobQ.size + tp->medJobQ.size;
	threads = tp->totalThreads - tp->persistentThreads;
	while (threads == 0 ||
	       (jobs / threads) >= tp->attr.jobsPerThread ||
	       (tp->totalThreads == tp->busyThreads) ) {
		if (CreateWorker(tp) != 0) {
			return;
		}
		threads++;
	}
}

int ThreadPoolInit(ThreadPool *tp, ThreadPoolAttr *attr)
{
	int retCode = 0;
	int i = 0;

	if (!tp) {
		return EINVAL;
	}

	retCode += ithread_mutex_init(&tp->mutex, NULL);
	retCode += ithread_mutex_lock(&tp->mutex);

	retCode += ithread_cond_init(&tp->condition, NULL);
	retCode += ithread_cond_init(&tp->start_and_shutdown, NULL);
	if (retCode) {
		ithread_mutex_unlock(&tp->mutex);
		ithread_mutex_destroy(&tp->mutex);
		ithread_cond_destroy(&tp->condition);
		ithread_cond_destroy(&tp->start_and_shutdown);
		return EAGAIN;
	}
	if (attr) {
		tp->attr = *attr;
	} else {
		TPAttrInit(&tp->attr);
	}
	if (SetPolicyType(tp->attr.schedPolicy) != 0) {
		ithread_mutex_unlock(&tp->mutex);
		ithread_mutex_destroy(&tp->mutex);
		ithread_cond_destroy(&tp->condition);
		ithread_cond_destroy(&tp->start_and_shutdown);

		return INVALID_POLICY;
	}
	retCode += FreeListInit(
		&tp->jobFreeList, sizeof(ThreadPoolJob), JOBFREELISTSIZE);
	StatsInit(&tp->stats);
	retCode += ListInit(&tp->highJobQ, CmpThreadPoolJob, NULL);
	retCode += ListInit(&tp->medJobQ, CmpThreadPoolJob, NULL);
	retCode += ListInit(&tp->lowJobQ, CmpThreadPoolJob, NULL);
	if (retCode) {
		retCode = EAGAIN;
	} else {
		tp->persistentJob = NULL;
		tp->lastJobId = 0;
		tp->shutdown = 0;
		tp->totalThreads = 0;
		tp->busyThreads = 0;
		tp->persistentThreads = 0;
		tp->pendingWorkerThreadStart = 0;
		for (i = 0; i < tp->attr.minThreads; ++i) {
			retCode = CreateWorker(tp);
			if (retCode) {
				break;
			}
		}
	}

	ithread_mutex_unlock(&tp->mutex);

	if (retCode) {
		/* clean up if the min threads could not be created */
		ThreadPoolShutdown(tp);
	}

	return retCode;
}

int ThreadPoolAddPersistent(ThreadPool *tp, ThreadPoolJob *job, int *jobId)
{
	int ret = 0;
	int tempId = -1;
	ThreadPoolJob *temp = NULL;

	if (!tp || !job) {
		return EINVAL;
	}
	if (!jobId) {
		jobId = &tempId;
	}
	*jobId = INVALID_JOB_ID;

	ithread_mutex_lock(&tp->mutex);

	/* Create A worker if less than max threads running */
	if (tp->totalThreads < tp->attr.maxThreads) {
		CreateWorker(tp);
	} else {
		/* if there is more than one worker thread
		 * available then schedule job, otherwise fail */
		if (tp->totalThreads - tp->persistentThreads - 1 == 0) {
			ret = EMAXTHREADS;
			goto exit_function;
		}
	}
	temp = CreateThreadPoolJob(job, tp->lastJobId, tp);
	if (!temp) {
		ret = EOUTOFMEM;
		goto exit_function;
	}
	tp->persistentJob = temp;

	/* Notify a waiting thread */
	ithread_cond_signal(&tp->condition);

	/* wait until long job has been picked up */
	while (tp->persistentJob)
		ithread_cond_wait(&tp->start_and_shutdown, &tp->mutex);
	*jobId = tp->lastJobId++;

exit_function:
	ithread_mutex_unlock(&tp->mutex);

	return ret;
}

int ThreadPoolAdd(ThreadPool *tp, ThreadPoolJob *job, int *jobId)
{
	int rc = EOUTOFMEM;
	int tempId = -1;
	long totalJobs;
	ThreadPoolJob *temp = NULL;

	if (!tp || !job)
		return EINVAL;

	ithread_mutex_lock(&tp->mutex);

	totalJobs = tp->highJobQ.size + tp->lowJobQ.size + tp->medJobQ.size;
	if (totalJobs >= tp->attr.maxJobsTotal) {
		fprintf(stderr, "total jobs = %ld, too many jobs", totalJobs);
		goto exit_function;
	}
	if (!jobId)
		jobId = &tempId;
	*jobId = INVALID_JOB_ID;
	temp = CreateThreadPoolJob(job, tp->lastJobId, tp);
	if (!temp)
		goto exit_function;
	switch (job->priority) {
	case HIGH_PRIORITY:
		if (ListAddTail(&tp->highJobQ, temp))
			rc = 0;
		break;
	case MED_PRIORITY:
		if (ListAddTail(&tp->medJobQ, temp))
			rc = 0;
		break;
	default:
		if (ListAddTail(&tp->lowJobQ, temp))
			rc = 0;
	}
	/* AddWorker if appropriate */
	AddWorker(tp);
	/* Notify a waiting thread */
	if (rc == 0)
		ithread_cond_signal(&tp->condition);
	else
		FreeThreadPoolJob(tp, temp);
	*jobId = tp->lastJobId++;

exit_function:
	ithread_mutex_unlock(&tp->mutex);

	return rc;
}

int ThreadPoolRemove(ThreadPool *tp, int jobId, ThreadPoolJob *out)
{
	int ret = INVALID_JOB_ID;
	ThreadPoolJob *temp = NULL;
	ListNode *tempNode = NULL;
	ThreadPoolJob dummy;

	if (!tp)
		return EINVAL;
	if (!out)
		out = &dummy;
	dummy.jobId = jobId;

	ithread_mutex_lock(&tp->mutex);

	tempNode = ListFind(&tp->highJobQ, NULL, &dummy);
	if (tempNode) {
		temp = (ThreadPoolJob *)tempNode->item;
		*out = *temp;
		ListDelNode(&tp->highJobQ, tempNode, 0);
		FreeThreadPoolJob(tp, temp);
		ret = 0;
		goto exit_function;
	}

	tempNode = ListFind(&tp->medJobQ, NULL, &dummy);
	if (tempNode) {
		temp = (ThreadPoolJob *)tempNode->item;
		*out = *temp;
		ListDelNode(&tp->medJobQ, tempNode, 0);
		FreeThreadPoolJob(tp, temp);
		ret = 0;
		goto exit_function;
	}
	tempNode = ListFind(&tp->lowJobQ, NULL, &dummy);
	if (tempNode) {
		temp = (ThreadPoolJob *)tempNode->item;
		*out = *temp;
		ListDelNode(&tp->lowJobQ, tempNode, 0);
		FreeThreadPoolJob(tp, temp);
		ret = 0;
		goto exit_function;
	}
	if (tp->persistentJob && tp->persistentJob->jobId == jobId) {
		*out = *tp->persistentJob;
		FreeThreadPoolJob(tp, tp->persistentJob);
		tp->persistentJob = NULL;
		ret = 0;
		goto exit_function;
	}

exit_function:
	ithread_mutex_unlock(&tp->mutex);

	return ret;
}

int ThreadPoolGetAttr(ThreadPool *tp, ThreadPoolAttr *out)
{
	if (!tp || !out)
		return EINVAL;
	if (!tp->shutdown)
		ithread_mutex_lock(&tp->mutex);
	*out = tp->attr;
	if (!tp->shutdown)
		ithread_mutex_unlock(&tp->mutex);

	return 0;
}

int ThreadPoolSetAttr(ThreadPool *tp, ThreadPoolAttr *attr)
{
	int retCode = 0;
	ThreadPoolAttr temp;
	int i = 0;

	if (!tp)
		return EINVAL;

	ithread_mutex_lock(&tp->mutex);

	if (attr)
		temp = *attr;
	else
		TPAttrInit(&temp);
	if (SetPolicyType(temp.schedPolicy) != 0) {
		ithread_mutex_unlock(&tp->mutex);
		return INVALID_POLICY;
	}
	tp->attr = temp;
	/* add threads */
	if (tp->totalThreads < tp->attr.minThreads) {
		for (i = tp->totalThreads; i < tp->attr.minThreads; i++) {
			retCode = CreateWorker(tp);
			if (retCode != 0) {
				break;
			}
		}
	}
	/* signal changes */
	ithread_cond_signal(&tp->condition); 

	ithread_mutex_unlock(&tp->mutex);

	if (retCode != 0)
		/* clean up if the min threads could not be created */
		ThreadPoolShutdown(tp);

	return retCode;
}

int ThreadPoolShutdown(ThreadPool *tp)
{
	ListNode *head = NULL;
	ThreadPoolJob *temp = NULL;

	if (!tp)
		return EINVAL;
	ithread_mutex_lock(&tp->mutex);
	/* clean up high priority jobs */
	while (tp->highJobQ.size) {
		head = ListHead(&tp->highJobQ);
		if (head == NULL) {
			ithread_mutex_unlock(&tp->mutex);
			return EINVAL;
		}
		temp = (ThreadPoolJob *)head->item;
		if (temp->free_func)
			temp->free_func(temp->arg);
		FreeThreadPoolJob(tp, temp);
		ListDelNode(&tp->highJobQ, head, 0);
	}
	ListDestroy(&tp->highJobQ, 0);
	/* clean up med priority jobs */
	while (tp->medJobQ.size) {
		head = ListHead(&tp->medJobQ);
		if (head == NULL) {
			ithread_mutex_unlock(&tp->mutex);
			return EINVAL;
		}
		temp = (ThreadPoolJob *)head->item;
		if (temp->free_func)
			temp->free_func(temp->arg);
		FreeThreadPoolJob(tp, temp);
		ListDelNode(&tp->medJobQ, head, 0);
	}
	ListDestroy(&tp->medJobQ, 0);
	/* clean up low priority jobs */
	while (tp->lowJobQ.size) {
		head = ListHead(&tp->lowJobQ);
		if (head == NULL) {
			ithread_mutex_unlock(&tp->mutex);
			return EINVAL;
		}
		temp = (ThreadPoolJob *)head->item;
		if (temp->free_func)
			temp->free_func(temp->arg);
		FreeThreadPoolJob(tp, temp);
		ListDelNode(&tp->lowJobQ, head, 0);
	}
	ListDestroy(&tp->lowJobQ, 0);
	/* clean up long term job */
	if (tp->persistentJob) {
		temp = tp->persistentJob;
		if (temp->free_func)
			temp->free_func(temp->arg);
		FreeThreadPoolJob(tp, temp);
		tp->persistentJob = NULL;
	}
	/* signal shutdown */
	tp->shutdown = 1;
	ithread_cond_broadcast(&tp->condition);
	/* wait for all threads to finish */
	while (tp->totalThreads > 0)
		ithread_cond_wait(&tp->start_and_shutdown, &tp->mutex);
	/* destroy condition */
	while (ithread_cond_destroy(&tp->condition) != 0) {}
	while (ithread_cond_destroy(&tp->start_and_shutdown) != 0) {}
	FreeListDestroy(&tp->jobFreeList);

	ithread_mutex_unlock(&tp->mutex);

	/* destroy mutex */
	while (ithread_mutex_destroy(&tp->mutex) != 0) {}

	return 0;
}

int TPAttrInit(ThreadPoolAttr *attr)
{
	if (!attr)
		return EINVAL;
	attr->jobsPerThread  = DEFAULT_JOBS_PER_THREAD;
	attr->maxIdleTime    = DEFAULT_IDLE_TIME;
	attr->maxThreads     = DEFAULT_MAX_THREADS;
	attr->minThreads     = DEFAULT_MIN_THREADS;
	attr->stackSize      = DEFAULT_STACK_SIZE;
	attr->schedPolicy    = DEFAULT_POLICY;
	attr->starvationTime = DEFAULT_STARVATION_TIME;
	attr->maxJobsTotal   = DEFAULT_MAX_JOBS_TOTAL;

	return 0;
}

int TPJobInit(ThreadPoolJob *job, start_routine func, void *arg)
{
	if (!job || !func)
		return EINVAL;
	job->func = func;
	job->arg = arg;
	job->priority = DEFAULT_PRIORITY;
	job->free_func = DEFAULT_FREE_ROUTINE;

	return 0;
}

int TPJobSetPriority(ThreadPoolJob *job, ThreadPriority priority)
{
	if (!job)
		return EINVAL;
	switch (priority) {
	case LOW_PRIORITY:
	case MED_PRIORITY:
	case HIGH_PRIORITY:
		job->priority = priority;
		return 0;
	default:
		return EINVAL;
	}
}

int TPJobSetFreeFunction(ThreadPoolJob *job, free_routine func)
{
	if(!job)
		return EINVAL;
	job->free_func = func;

	return 0;
}

int TPAttrSetMaxThreads(ThreadPoolAttr *attr, int maxThreads)
{
	if (!attr)
		return EINVAL;
	attr->maxThreads = maxThreads;

	return 0;
}

int TPAttrSetMinThreads(ThreadPoolAttr *attr, int minThreads)
{
	if (!attr)
		return EINVAL;
	attr->minThreads = minThreads;

	return 0;
}

int TPAttrSetStackSize(ThreadPoolAttr *attr, size_t stackSize)
{
        if (!attr)
                return EINVAL;
        attr->stackSize = stackSize;

        return 0;
}

int TPAttrSetIdleTime(ThreadPoolAttr *attr, int idleTime)
{
	if (!attr)
		return EINVAL;
	attr->maxIdleTime = idleTime;

	return 0;
}

int TPAttrSetJobsPerThread(ThreadPoolAttr *attr, int jobsPerThread)
{
	if (!attr)
		return EINVAL;
	attr->jobsPerThread = jobsPerThread;

	return 0;
}

int TPAttrSetStarvationTime(ThreadPoolAttr *attr, int starvationTime)
{
	if (!attr)
		return EINVAL;
	attr->starvationTime = starvationTime;

	return 0;
}

int TPAttrSetSchedPolicy(ThreadPoolAttr *attr, PolicyType schedPolicy)
{
	if (!attr)
		return EINVAL;
	attr->schedPolicy = schedPolicy;

	return 0;
}

int TPAttrSetMaxJobsTotal(ThreadPoolAttr *attr, int maxJobsTotal)
{
	if (!attr)
		return EINVAL;
	attr->maxJobsTotal = maxJobsTotal;

	return 0;
}

#ifdef STATS
void ThreadPoolPrintStats(ThreadPoolStats *stats)
{
	if (!stats)
		return;
	/* some OSses time_t length may depending on platform, promote it to long for safety */
	printf("ThreadPoolStats at Time: %ld\n", (long)StatsTime(NULL));
	printf("High Jobs pending: %d\n", stats->currentJobsHQ);
	printf("Med Jobs Pending: %d\n", stats->currentJobsMQ);
	printf("Low Jobs Pending: %d\n", stats->currentJobsLQ);
	printf("Average Wait in High Priority Q in milliseconds: %f\n", stats->avgWaitHQ);
	printf("Average Wait in Med Priority Q in milliseconds: %f\n", stats->avgWaitMQ);
	printf("Averate Wait in Low Priority Q in milliseconds: %f\n", stats->avgWaitLQ);
	printf("Max Threads Active: %d\n", stats->maxThreads);
	printf("Current Worker Threads: %d\n", stats->workerThreads);
	printf("Current Persistent Threads: %d\n", stats->persistentThreads);
	printf("Current Idle Threads: %d\n", stats->idleThreads);
	printf("Total Threads : %d\n", stats->totalThreads);
	printf("Total Time spent Working in seconds: %f\n", stats->totalWorkTime);
	printf("Total Time spent Idle in seconds : %f\n", stats->totalIdleTime);
}

int ThreadPoolGetStats(ThreadPool *tp, ThreadPoolStats *stats)
{
	if (tp == NULL || stats == NULL)
		return EINVAL;
	/* if not shutdown then acquire mutex */
	if (!tp->shutdown)
		ithread_mutex_lock(&tp->mutex);

	*stats = tp->stats;
	if (stats->totalJobsHQ > 0)
		stats->avgWaitHQ = stats->totalTimeHQ / (double)stats->totalJobsHQ;
	else
		stats->avgWaitHQ = 0.0;
	if (stats->totalJobsMQ > 0)
		stats->avgWaitMQ = stats->totalTimeMQ / (double)stats->totalJobsMQ;
	else
		stats->avgWaitMQ = 0.0;
	if (stats->totalJobsLQ > 0)
		stats->avgWaitLQ = stats->totalTimeLQ / (double)stats->totalJobsLQ;
	else
		stats->avgWaitLQ = 0.0;
	stats->totalThreads = tp->totalThreads;
	stats->persistentThreads = tp->persistentThreads;
	stats->currentJobsHQ = (int)ListSize(&tp->highJobQ);
	stats->currentJobsLQ = (int)ListSize(&tp->lowJobQ);
	stats->currentJobsMQ = (int)ListSize(&tp->medJobQ);

	/* if not shutdown then release mutex */
	if (!tp->shutdown)
		ithread_mutex_unlock(&tp->mutex);

	return 0;
}
#endif /* STATS */

#ifdef WIN32
	#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
		#define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
	#else
		#define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
	#endif

	int gettimeofday(struct timeval *tv, struct timezone *tz)
	{
		FILETIME ft;
		unsigned __int64 tmpres = 0;
		static int tzflag;

		if (tv) {
			GetSystemTimeAsFileTime(&ft);

			tmpres |= ft.dwHighDateTime;
			tmpres <<= 32;
			tmpres |= ft.dwLowDateTime;

			/*converting file time to unix epoch*/
			tmpres /= 10;  /*convert into microseconds*/
			tmpres -= DELTA_EPOCH_IN_MICROSECS; 
			tv->tv_sec = (long)(tmpres / 1000000UL);
			tv->tv_usec = (long)(tmpres % 1000000UL);
		}
		if (tz) {
			if (!tzflag) {
				_tzset();
				tzflag++;
			}
			tz->tz_minuteswest = _timezone / 60;
			tz->tz_dsttime = _daylight;
		}

		return 0;
	}
#endif /* WIN32 */
