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

#include "TimerThread.h"

#include <assert.h>

/*!
 * \brief Deallocates a dynamically allocated TimerEvent.
 */
static void FreeTimerEvent(
	/*! [in] Valid timer thread pointer. */
	TimerThread *timer,
	/*! [in] Must be allocated with CreateTimerEvent*/
	TimerEvent *event)
{
	assert(timer != NULL);

	FreeListFree(&timer->freeEvents, event);
}

/*!
 * \brief Implements timer thread.
 *
 * Waits for next event to occur and schedules associated job into threadpool.
 */
static void *TimerThreadWorker(
	/*! [in] arg is cast to (TimerThread *). */
	void *arg)
{
    TimerThread *timer = ( TimerThread * ) arg;
    ListNode *head = NULL;
    TimerEvent *nextEvent = NULL;
    time_t currentTime = 0;
    time_t nextEventTime = 0;
    struct timespec timeToWait;
    int tempId;

    assert( timer != NULL );

    ithread_mutex_lock( &timer->mutex );
    while (1) {
        /* mutex should always be locked at top of loop */
	/* Check for shutdown. */
        if (timer->shutdown) {
            timer->shutdown = 0;
            ithread_cond_signal( &timer->condition );
            ithread_mutex_unlock( &timer->mutex );
            return NULL;
        }
        nextEvent = NULL;
        /* Get the next event if possible. */
        if (timer->eventQ.size > 0) {
            head = ListHead( &timer->eventQ );
            if (head == NULL) {
                ithread_mutex_unlock( &timer->mutex );
                return NULL;
            }
            nextEvent = ( TimerEvent * ) head->item;
            nextEventTime = nextEvent->eventTime;
        }
        currentTime = time(NULL);
        /* If time has elapsed, schedule job. */
        if (nextEvent && currentTime >= nextEventTime) {
            if( nextEvent->persistent ) {
                if (ThreadPoolAddPersistent( timer->tp, &nextEvent->job, &tempId ) != 0) {
			if (nextEvent->job.arg != NULL && nextEvent->job.free_func != NULL) {
				nextEvent->job.free_func(nextEvent->job.arg);
			}
		}
            } else {
                if (ThreadPoolAdd( timer->tp, &nextEvent->job, &tempId ) != 0) {
			if (nextEvent->job.arg != NULL && nextEvent->job.free_func != NULL) {
				nextEvent->job.free_func(nextEvent->job.arg);
			}
		}
            }
            ListDelNode( &timer->eventQ, head, 0 );
            FreeTimerEvent( timer, nextEvent );
            continue;
        }
        if (nextEvent) {
            timeToWait.tv_nsec = 0;
            timeToWait.tv_sec = (long)nextEvent->eventTime;
            ithread_cond_timedwait( &timer->condition, &timer->mutex,
                                    &timeToWait );
        } else {
            ithread_cond_wait( &timer->condition, &timer->mutex );
        }
    }
}


/*!
 * \brief Calculates the appropriate timeout in absolute seconds
 * since Jan 1, 1970.
 *
 * \return 
 */
static int CalculateEventTime(
	/*! [in] Timeout. */
	time_t *timeout,
	/*! [in] Timeout type. */
	TimeoutType type)
{
    time_t now;

    assert( timeout != NULL );

    switch (type) {
    case ABS_SEC:
        return 0;
    default: /* REL_SEC) */
        time(&now);
        ( *timeout ) += now;
        return 0;
    }

    return -1;
}

/*!
 * \brief Creates a Timer Event. (Dynamically allocated).
 *
 * \return (TimerEvent *) on success, NULL on failure.
 */
static TimerEvent *CreateTimerEvent(
	/*! [in] Valid timer thread pointer. */
	TimerThread *timer,
	/*! [in] . */
	ThreadPoolJob *job,
	/*! [in] . */
	Duration persistent,
	/*! [in] The absoule time of the event in seconds from Jan, 1970. */
	time_t eventTime,
	/*! [in] Id of job. */
	int id)
{
    TimerEvent *temp = NULL;

    assert( timer != NULL );
    assert( job != NULL );

    temp = ( TimerEvent * ) FreeListAlloc( &timer->freeEvents );
    if( temp == NULL )
        return temp;
    temp->job = ( *job );
    temp->persistent = persistent;
    temp->eventTime = eventTime;
    temp->id = id;

    return temp;
}


int TimerThreadInit(TimerThread *timer, ThreadPool *tp)
{

    int rc = 0;

    ThreadPoolJob timerThreadWorker;

    assert( timer != NULL );
    assert( tp != NULL );

    if( ( timer == NULL ) || ( tp == NULL ) ) {
        return EINVAL;
    }

    rc += ithread_mutex_init( &timer->mutex, NULL );

    assert( rc == 0 );

    rc += ithread_mutex_lock( &timer->mutex );
    assert( rc == 0 );

    rc += ithread_cond_init( &timer->condition, NULL );
    assert( rc == 0 );

    rc += FreeListInit( &timer->freeEvents, sizeof( TimerEvent ), 100 );
    assert( rc == 0 );

    timer->shutdown = 0;
    timer->tp = tp;
    timer->lastEventId = 0;
    rc += ListInit( &timer->eventQ, NULL, NULL );

    assert( rc == 0 );

    if( rc != 0 ) {
        rc = EAGAIN;
    } else {

        TPJobInit( &timerThreadWorker, TimerThreadWorker, timer );
        TPJobSetPriority( &timerThreadWorker, HIGH_PRIORITY );

        rc = ThreadPoolAddPersistent( tp, &timerThreadWorker, NULL );
    }

    ithread_mutex_unlock( &timer->mutex );

    if( rc != 0 ) {
        ithread_cond_destroy( &timer->condition );
        ithread_mutex_destroy( &timer->mutex );
        FreeListDestroy( &timer->freeEvents );
        ListDestroy( &timer->eventQ, 0 );
    }

    return rc;
}

int TimerThreadSchedule(
	TimerThread *timer,
	time_t timeout,
	TimeoutType type,
	ThreadPoolJob *job,
	Duration duration,
	int *id)
{
    int rc = EOUTOFMEM;
    int found = 0;
    int tempId = 0;

    ListNode *tempNode = NULL;
    TimerEvent *temp = NULL;
    TimerEvent *newEvent = NULL;

    assert( timer != NULL );
    assert( job != NULL );

    if( ( timer == NULL ) || ( job == NULL ) ) {
        return EINVAL;
    }

    CalculateEventTime( &timeout, type );
    ithread_mutex_lock( &timer->mutex );

    if( id == NULL )
        id = &tempId;

    ( *id ) = INVALID_EVENT_ID;

    newEvent = CreateTimerEvent( timer, job, duration, timeout,
                                 timer->lastEventId );

    if( newEvent == NULL ) {
        ithread_mutex_unlock( &timer->mutex );
        return rc;
    }

    tempNode = ListHead( &timer->eventQ );
    /* add job to Q. Q is ordered by eventTime with the head of the Q being
     * the next event. */
    while( tempNode != NULL ) {
        temp = ( TimerEvent * ) tempNode->item;
        if( temp->eventTime >= timeout ) {
            if (ListAddBefore( &timer->eventQ, newEvent, tempNode))
                rc = 0;
            found = 1;
            break;
        }
        tempNode = ListNext( &timer->eventQ, tempNode );
    }
    /* add to the end of Q. */
    if (!found) {
        if( ListAddTail( &timer->eventQ, newEvent ) != NULL )
            rc = 0;
    }
    /* signal change in Q. */
    if( rc == 0 ) {
        ithread_cond_signal( &timer->condition );
    } else {
        FreeTimerEvent( timer, newEvent );
    }
    ( *id ) = timer->lastEventId++;
    ithread_mutex_unlock( &timer->mutex );

    return rc;
}

int TimerThreadRemove(
	TimerThread *timer,
	int id,
	ThreadPoolJob *out)
{
    int rc = INVALID_EVENT_ID;
    ListNode *tempNode = NULL;
    TimerEvent *temp = NULL;

    assert( timer != NULL );

    if( timer == NULL ) {
        return EINVAL;
    }

    ithread_mutex_lock( &timer->mutex );

    tempNode = ListHead( &timer->eventQ );

    while( tempNode != NULL ) {
        temp = ( TimerEvent * ) tempNode->item;
        if( temp->id == id )
        {

            ListDelNode( &timer->eventQ, tempNode, 0 );
            if( out != NULL )
                ( *out ) = temp->job;
            FreeTimerEvent( timer, temp );
            rc = 0;
            break;
        }
        tempNode = ListNext( &timer->eventQ, tempNode );
    }

    ithread_mutex_unlock( &timer->mutex );
    return rc;
}

int TimerThreadShutdown(TimerThread *timer)
{
    ListNode *tempNode2 = NULL;
    ListNode *tempNode = NULL;

    assert( timer != NULL );

    if( timer == NULL ) {
        return EINVAL;
    }

    ithread_mutex_lock( &timer->mutex );

    timer->shutdown = 1;
    tempNode = ListHead( &timer->eventQ );

    /* Delete nodes in Q. Call registered free function on argument. */
    while( tempNode != NULL ) {
        TimerEvent *temp = ( TimerEvent * ) tempNode->item;

        tempNode2 = ListNext( &timer->eventQ, tempNode );
        ListDelNode( &timer->eventQ, tempNode, 0 );
        if( temp->job.free_func ) {
            temp->job.free_func( temp->job.arg );
        }
        FreeTimerEvent( timer, temp );
        tempNode = tempNode2;
    }

    ListDestroy( &timer->eventQ, 0 );
    FreeListDestroy( &timer->freeEvents );

    ithread_cond_broadcast( &timer->condition );

    while (timer->shutdown) {
	/* wait for timer thread to shutdown. */
        ithread_cond_wait( &timer->condition, &timer->mutex );
    }
    ithread_mutex_unlock(&timer->mutex);

    /* destroy condition. */
    while(ithread_cond_destroy(&timer->condition) != 0) {
    }
    /* destroy mutex. */
    while (ithread_mutex_destroy(&timer->mutex) != 0) {
    }

    return 0;
}

