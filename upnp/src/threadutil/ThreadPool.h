/*******************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation 
 * All rights reserved. 
 * Copyright (c) 2012 France Telecom All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met: 
 *
 * * Redistributions of source code must retain the above copyright notice, 
 * this list of conditions and the following disclaimer. 
 * * Redistributions in binary form must reproduce the above copyright notice, 
 * this list of conditions and the following disclaimer in the documentation 
 * and/or other materials provided with the distribution. 
 * * Neither name of Intel Corporation nor the names of its contributors 
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

#ifndef THREADPOOL_H
#define THREADPOOL_H

/*!
 * \file
 */

#include "FreeList.h"
#include "ithread.h"
#include "LinkedList.h"
#include "UpnpInet.h"
#include "UpnpGlobal.h" /* for UPNP_INLINE, EXPORT_SPEC */

#include <errno.h>

#ifdef WIN32
	#include <time.h>
	struct timezone
	{
		int  tz_minuteswest; /* minutes W of Greenwich */
		int  tz_dsttime;     /* type of dst correction */
	};
	int gettimeofday(struct timeval *tv, struct timezone *tz);
#else /* WIN32 */
	#include <sys/param.h>
	#include <sys/time.h> /* for gettimeofday() */
	#if defined(__OSX__) || defined(__APPLE__) || defined(__NetBSD__)
		#include <sys/resource.h>	/* for setpriority() */
	#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*! Size of job free list */
#define JOBFREELISTSIZE 100

#define INFINITE_THREADS -1

#define EMAXTHREADS (-8 & 1<<29)

/*! Invalid Policy */
#define INVALID_POLICY (-9 & 1<<29)

/*! Invalid JOB Id */
#define INVALID_JOB_ID (-2 & 1<<29)

typedef enum duration {
	SHORT_TERM,
	PERSISTENT
} Duration;

typedef enum priority {
	LOW_PRIORITY,
	MED_PRIORITY,
	HIGH_PRIORITY
} ThreadPriority;

/*! default priority used by TPJobInit */
#define DEFAULT_PRIORITY MED_PRIORITY

/*! default minimum used by TPAttrInit */
#define DEFAULT_MIN_THREADS 1

/*! default max used by TPAttrInit */
#define DEFAULT_MAX_THREADS 10

/*! default stack size used by TPAttrInit */
#define DEFAULT_STACK_SIZE 0u

/*! default jobs per thread used by TPAttrInit */
#define DEFAULT_JOBS_PER_THREAD 10

/*! default starvation time used by TPAttrInit */
#define DEFAULT_STARVATION_TIME	500

/*! default idle time used by TPAttrInit */
#define DEFAULT_IDLE_TIME 10 * 1000

/*! default free routine used TPJobInit */
#define DEFAULT_FREE_ROUTINE NULL

/*! default max jobs used TPAttrInit */
#define DEFAULT_MAX_JOBS_TOTAL 100

/*!
 * \brief Statistics.
 *
 * Always include stats because code change is minimal.
 */
#define STATS 1

#ifdef _DEBUG
	#define DEBUG 1
#endif

typedef int PolicyType;

#define DEFAULT_POLICY SCHED_OTHER

/*! Function for freeing a thread argument. */
typedef void (*free_routine)(void *arg);


/*! Attributes for thread pool. Used to set and change parameters of thread
 * pool. */
typedef struct THREADPOOLATTR
{
	/*! ThreadPool will always maintain at least this many threads. */
	int minThreads;
	/*! ThreadPool will never have more than this number of threads. */
	int maxThreads;
	/*! This is the minimum stack size allocated for each thread. */
	size_t stackSize;
	/*! This is the maximum time a thread will
	 * remain idle before dying (in milliseconds). */
	int maxIdleTime;
	/*! Jobs per thread to maintain. */
	int jobsPerThread;
	/*! Maximum number of jobs that can be queued totally. */
	int maxJobsTotal;
	/*! the time a low priority or med priority job waits before getting
	 * bumped up a priority (in milliseconds). */
	int starvationTime;
	/*! scheduling policy to use. */
	PolicyType schedPolicy;
} ThreadPoolAttr;

/*! Internal ThreadPool Job. */
typedef struct THREADPOOLJOB
{
	start_routine func;
	void *arg;
	free_routine free_func;
	struct timeval requestTime;
	ThreadPriority priority;
	int jobId;
} ThreadPoolJob;

/*! Structure to hold statistics. */
typedef struct TPOOLSTATS
{
	double totalTimeHQ;
	int totalJobsHQ;
	double avgWaitHQ;
	double totalTimeMQ;
	int totalJobsMQ;
	double avgWaitMQ;
	double totalTimeLQ;
	int totalJobsLQ;
	double avgWaitLQ;
	double totalWorkTime;
	double totalIdleTime;
	int workerThreads;
	int idleThreads;
	int persistentThreads;
	int totalThreads;
	int maxThreads;
	int currentJobsHQ;
	int currentJobsLQ;
	int currentJobsMQ;
} ThreadPoolStats;

/*!
 * \brief A thread pool similar to the thread pool in the UPnP SDK.
 *
 * Allows jobs to be scheduled for running by threads in a 
 * thread pool. The thread pool is initialized with a 
 * minimum and maximum thread number as well as a max idle time
 * and a jobs per thread ratio. If a worker thread waits the whole
 * max idle time without receiving a job and the thread pool
 * currently has more threads running than the minimum
 * then the worker thread will exit. If when 
 * scheduling a job the current job to thread ratio
 * becomes greater than the set ratio and the thread pool currently has
 * less than the maximum threads then a new thread will
 * be created.
 */
typedef struct THREADPOOL
{
	/*! Mutex to protect job qs. */
	ithread_mutex_t mutex;
	/*! Condition variable to signal Q. */
	ithread_cond_t condition;
	/*! Condition variable for start and stop. */
	ithread_cond_t start_and_shutdown;
	/*! ids for jobs */
	int lastJobId;
	/*! whether or not we are shutting down */
	int shutdown;
	/*! total number of threads */
	int totalThreads;
	/*! flag that's set when waiting for a new worker thread to start */
	int pendingWorkerThreadStart;
	/*! number of threads that are currently executing jobs */
	int busyThreads;
	/*! number of persistent threads */
	int persistentThreads;
	/*! free list of jobs */
	FreeList jobFreeList;
	/*! low priority job Q */
	LinkedList lowJobQ;
	/*! med priority job Q */
	LinkedList medJobQ;
	/*! high priority job Q */
	LinkedList highJobQ;
	/*! persistent job */
	ThreadPoolJob *persistentJob;
	/*! thread pool attributes */
	ThreadPoolAttr attr;
	/*! statistics */
	ThreadPoolStats stats;
} ThreadPool;

/*!
 * \brief Initializes and starts ThreadPool. Must be called first and
 * only once for ThreadPool.
 *
 * \return
 * \li \c 0 on success.
 * \li \c EAGAIN if not enough system resources to create minimum threads.
 * \li \c INVALID_POLICY if schedPolicy can't be set.
 * \li \c EMAXTHREADS if minimum threads is greater than maximum threads.
 */
int ThreadPoolInit(
	/*! Must be valid, non null, pointer to ThreadPool. */
	ThreadPool *tp,
	/*! Can be null. if not null then attr contains the following fields:
	 * \li \c minWorkerThreads - minimum number of worker threads thread
	 * pool will never have less than this number of threads.
	 * \li \c maxWorkerThreads - maximum number of worker threads thread
	 * pool will never have more than this number of threads.
	 * \li \c maxIdleTime - maximum time that a worker thread will spend
	 * idle. If a worker is idle longer than this time and there are more
	 * than the min number of workers running, then the worker thread
	 * exits.
	 * \li \c jobsPerThread - ratio of jobs to thread to try and maintain
	 * if a job is scheduled and the number of jobs per thread is greater
	 * than this number,and if less than the maximum number of workers are
	 * running then a new thread is started to help out with efficiency.
	 * \li \c schedPolicy - scheduling policy to try and set (OS dependent).
	 */
	ThreadPoolAttr *attr);

/*!
 * \brief Adds a persistent job to the thread pool.
 *
 * Job will be run as soon as possible. Call will block until job is scheduled.
 * 
 * \return
 *	\li \c 0 on success.
 *	\li \c EOUTOFMEM not enough memory to add job.
 *	\li \c EMAXTHREADS not enough threads to add persistent job.
 */
int ThreadPoolAddPersistent(
	/*! Valid thread pool pointer. */
	ThreadPool*tp,
	/*! Valid thread pool job. */
	ThreadPoolJob *job,
	/*! . */
	int *jobId);

/*!
 * \brief Gets the current set of attributes associated with the thread pool.
 *
 * \return
 * 	\li \c 0 on success, nonzero on failure.
 */
int ThreadPoolGetAttr(
	/*! valid thread pool pointer. */
	ThreadPool *tp,
	/*! non null pointer to store attributes. */
	ThreadPoolAttr *out);

/*!
 * \brief Sets the attributes for the thread pool.
 * Only affects future calculations.
 *
 * \return
 * 	\li \c 0 on success, nonzero on failure.
 * 	\li \c INVALID_POLICY if policy can not be set.
 */
int ThreadPoolSetAttr(
	/*! valid thread pool pointer. */
	ThreadPool *tp,
	/*! pointer to attributes, null sets attributes to default. */
	ThreadPoolAttr *attr);

/*!
 * \brief Adds a job to the thread pool. Job will be run as soon as possible.
 *
 * \return
 * 	\li \c 0 on success, nonzero on failure.
 * 	\li \c EOUTOFMEM if not enough memory to add job.
 */
int ThreadPoolAdd(
	/*! valid thread pool pointer. */
	ThreadPool*tp,
	/*! . */
	ThreadPoolJob *job,
	/*! id of job. */
	int *jobId);

/*!
 * \brief Removes a job from the thread pool. Can only remove jobs which
 * are not currently running.
 *
 * \return
 * 	\li \c 0 on success, nonzero on failure.
 * 	\li \c INVALID_JOB_ID if job not found. 
 */
int ThreadPoolRemove(
	/*! valid thread pool pointer. */
	ThreadPool *tp,
	/*! id of job. */
	int jobId,
	/*! space for removed job. */
	ThreadPoolJob *out);

/*!
 * \brief Shuts the thread pool down. Waits for all threads to finish.
 * May block indefinitely if jobs do not exit.
 *
 * \return 0 on success, nonzero on failure
 */
int ThreadPoolShutdown(
	/*! must be valid tp. */
	ThreadPool *tp);

/*!
 * \brief Initializes thread pool job. Sets the priority to default defined
 * in ThreadPool.h. Sets the free_routine to default defined in ThreadPool.h.
 *
 * \return Always returns 0.
 */
int TPJobInit(
	/*! must be valid thread pool attributes. */
	ThreadPoolJob *job,
	/*! function to run, must be valid. */
	start_routine func,
	/*! argument to pass to function. */
	void *arg);

/*!
 * \brief Sets the max threads for the thread pool attributes.
 *
 * \return Always returns 0.
 */
int TPJobSetPriority(
	/*! must be valid thread pool attributes. */
	ThreadPoolJob *job,
	/*! value to set. */
	ThreadPriority priority);

/*!
 * \brief Sets the max threads for the thread pool attributes.
 *
 * \return Always returns 0.
 */
int TPJobSetFreeFunction(
	/*! must be valid thread pool attributes. */
	ThreadPoolJob *job,
	/*! value to set. */
	free_routine func);

/*!
 * \brief Initializes thread pool attributes. Sets values to defaults defined
 * in ThreadPool.h.
 *
 * \return Always returns 0.
 */
int TPAttrInit(
	/*! must be valid thread pool attributes. */
	ThreadPoolAttr *attr);

/*!
 * \brief Sets the max threads for the thread pool attributes.
 *
 * \return Always returns 0.
 */
int TPAttrSetMaxThreads(
	/*! must be valid thread pool attributes. */
	ThreadPoolAttr *attr,
	/*! value to set. */
	int maxThreads);

/*!
 * \brief Sets the min threads for the thread pool attributes.
 *
 * \return Always returns 0.
 */
int TPAttrSetMinThreads(
	/*! must be valid thread pool attributes. */
	ThreadPoolAttr *attr,
	/*! value to set. */
	int minThreads);

/*!
 * \brief Sets the stack size for the thread pool attributes.
 *
 * \return Always returns 0.
 */
int TPAttrSetStackSize(
	/*! must be valid thread pool attributes. */
	ThreadPoolAttr *attr,
	/*! value to set. */
	size_t stackSize);

/*!
 * \brief Sets the idle time for the thread pool attributes.
 *
 * \return Always returns 0.
 */
int TPAttrSetIdleTime(
	/*! must be valid thread pool attributes. */
	ThreadPoolAttr *attr,
	/*! . */
	int idleTime);

/*!
 * \brief Sets the jobs per thread ratio
 *
 * \return Always returns 0.
 */
int TPAttrSetJobsPerThread(
	/*! must be valid thread pool attributes. */
	ThreadPoolAttr *attr,
	/*! number of jobs per thread to maintain. */
	int jobsPerThread);

/*!
 * \brief Sets the starvation time for the thread pool attributes.
 *
 * \return Always returns 0.
 */
int TPAttrSetStarvationTime(
	/*! must be valid thread pool attributes. */
	ThreadPoolAttr *attr,
	/*! milliseconds. */
	int starvationTime);

/*!
 * \brief Sets the scheduling policy for the thread pool attributes.
 *
 * \return Always returns 0.
 */
int TPAttrSetSchedPolicy(
	/*! must be valid thread pool attributes. */
	ThreadPoolAttr *attr,
	/*! must be a valid policy type. */
	PolicyType schedPolicy);

/*!
 * \brief Sets the maximum number jobs that can be qeued totally.
 *
 * \return Always returns 0.
 */
int TPAttrSetMaxJobsTotal(
	/*! must be valid thread pool attributes. */
	ThreadPoolAttr *attr,
	/*! maximum number of jobs. */
	int maxJobsTotal);

/*!
 * \brief Returns various statistics about the thread pool.
 *
 * Only valid if STATS has been defined.
 *
 * \return Always returns 0.
 */
#ifdef STATS
	EXPORT_SPEC int ThreadPoolGetStats(
		/*! Valid initialized threadpool. */
		ThreadPool *tp,
		/*! Valid stats, out parameter. */
		ThreadPoolStats *stats);
#else
	static UPNP_INLINE int ThreadPoolGetStats(
		/*! Valid initialized threadpool. */
		ThreadPool *tp,
		/*! Valid stats, out parameter. */
		ThreadPoolStats *stats) {}
#endif

/*!
 * \brief
 */
#ifdef STATS
	EXPORT_SPEC void ThreadPoolPrintStats(
		/*! . */
		ThreadPoolStats *stats);
#else
	static UPNP_INLINE void ThreadPoolPrintStats(
		/*! . */
		ThreadPoolStats *stats) {}
#endif

#ifdef __cplusplus
}
#endif

#endif /* THREADPOOL_H */

