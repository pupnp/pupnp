#ifndef ITHREAD_H
#define ITHREAD_H

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

/*!
 * \file
 */

#if !defined(WIN32)
	#include <sys/param.h>
#endif

#include "UpnpGlobal.h" /* For UPNP_INLINE, EXPORT_SPEC */
#include "UpnpUniStd.h" /* for close() */

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>

#if defined(BSD) && !defined(__GNU__)
	#define PTHREAD_MUTEX_RECURSIVE_NP PTHREAD_MUTEX_RECURSIVE
#endif


#if defined(PTHREAD_MUTEX_RECURSIVE) || defined(__DragonFly__)
	/* This system has SuS2-compliant mutex attributes.
	 * E.g. on Cygwin, where we don't have the old nonportable (NP) symbols
	 */
	#define ITHREAD_MUTEX_FAST_NP       PTHREAD_MUTEX_NORMAL
	#define ITHREAD_MUTEX_RECURSIVE_NP  PTHREAD_MUTEX_RECURSIVE
	#define ITHREAD_MUTEX_ERRORCHECK_NP PTHREAD_MUTEX_ERRORCHECK
#else /* PTHREAD_MUTEX_RECURSIVE */
	#define ITHREAD_MUTEX_FAST_NP       PTHREAD_MUTEX_FAST_NP
	#define ITHREAD_MUTEX_RECURSIVE_NP  PTHREAD_MUTEX_RECURSIVE_NP
	#define ITHREAD_MUTEX_ERRORCHECK_NP PTHREAD_MUTEX_ERRORCHECK_NP
#endif /* PTHREAD_MUTEX_RECURSIVE */


#define ITHREAD_PROCESS_PRIVATE PTHREAD_PROCESS_PRIVATE
#define ITHREAD_PROCESS_SHARED  PTHREAD_PROCESS_SHARED


#define ITHREAD_CANCELED PTHREAD_CANCELED


#define ITHREAD_STACK_MIN PTHREAD_STACK_MIN
#define ITHREAD_CREATE_DETACHED PTHREAD_CREATE_DETACHED
#define ITHREAD_CREATE_JOINABLE PTHREAD_CREATE_JOINABLE

/***************************************************************************
 * Name: ithread_t
 *
 *  Description:
 *      Thread handle.
 *      typedef to pthread_t.
 *      Internal Use Only.
 ***************************************************************************/
typedef pthread_t ithread_t;

  
/****************************************************************************
 * Name: ithread_attr_t
 *
 *  Description:
 *      Thread attribute.
 *      typedef to pthread_attr_t
 *      Internal Use Only
 ***************************************************************************/
typedef pthread_attr_t ithread_attr_t;	


/****************************************************************************
 * Name: start_routine
 *
 *  Description:
 *      Thread start routine 
 *      Internal Use Only.
 ***************************************************************************/
typedef void *(*start_routine)(void *arg);

  
/****************************************************************************
 * Name: ithread_cond_t
 *
 *  Description:
 *      condition variable.
 *      typedef to pthread_cond_t
 *      Internal Use Only.
 ***************************************************************************/
typedef pthread_cond_t ithread_cond_t;


/****************************************************************************
 * Name: ithread_mutexattr_t
 *
 *  Description:
 *      Mutex attribute.
 *      typedef to pthread_mutexattr_t
 *      Internal Use Only
 ***************************************************************************/
typedef pthread_mutexattr_t ithread_mutexattr_t;	


/****************************************************************************
 * Name: ithread_mutex_t
 *
 *  Description:
 *      Mutex.
 *      typedef to pthread_mutex_t
 *      Internal Use Only.
 ***************************************************************************/
typedef pthread_mutex_t ithread_mutex_t;


/****************************************************************************
 * Name: ithread_condattr_t
 *
 *  Description:
 *      Condition attribute.
 *      typedef to pthread_condattr_t
 *      NOT USED
 *      Internal Use Only
 ***************************************************************************/
typedef pthread_condattr_t ithread_condattr_t;	


/****************************************************************************
 * Name: ithread_rwlockattr_t
 *
 *  Description:
 *      Mutex attribute.
 *      typedef to pthread_rwlockattr_t
 *      Internal Use Only
 ***************************************************************************/
#if UPNP_USE_RWLOCK
typedef pthread_rwlockattr_t ithread_rwlockattr_t;	
#endif /* UPNP_USE_RWLOCK */


/****************************************************************************
 * Name: ithread_rwlock_t
 *
 *  Description:
 *      Condition attribute.
 *      typedef to pthread_rwlock_t
 *      Internal Use Only
 ***************************************************************************/
#if UPNP_USE_RWLOCK
	typedef pthread_rwlock_t ithread_rwlock_t;
#else
	/* Read-write locks aren't available: use mutex instead. */
	typedef ithread_mutex_t ithread_rwlock_t;
#endif /* UPNP_USE_RWLOCK */


/****************************************************************************
 * Function: ithread_initialize_library
 *
 *  Description:
 *      Initializes the library. Does nothing in all implementations, except
 *      when statically linked for WIN32.
 *  Parameters:
 *      none.
 *  Returns:
 *      0 on success, Nonzero on failure.
 ***************************************************************************/
static UPNP_INLINE int ithread_initialize_library(void) {
	int ret = 0;

	return ret;
}


/****************************************************************************
 * Function: ithread_cleanup_library
 *
 *  Description:
 *      Clean up library resources. Does nothing in all implementations, except
 *      when statically linked for WIN32.
 *  Parameters:
 *      none.
 *  Returns:
 *      0 on success, Nonzero on failure.
 ***************************************************************************/
static UPNP_INLINE int ithread_cleanup_library(void) {
	int ret = 0;

	return ret;
}


/****************************************************************************
 * Function: ithread_initialize_thread
 *
 *  Description:
 *      Initializes the thread. Does nothing in all implementations, except
 *      when statically linked for WIN32.
 *  Parameters:
 *      none.
 *  Returns:
 *      0 on success, Nonzero on failure.
 ***************************************************************************/
static UPNP_INLINE int ithread_initialize_thread(void) {
	int ret = 0;

#if defined(WIN32) && defined(PTW32_STATIC_LIB)
	ret = !pthread_win32_thread_attach_np();
#endif

	return ret;
}


/****************************************************************************
 * Function: ithread_cleanup_thread
 *
 *  Description:
 *      Clean up thread resources. Does nothing in all implementations, except
 *      when statically linked for WIN32.
 *  Parameters:
 *      none.
 *  Returns:
 *      0 on success, Nonzero on failure.
 ***************************************************************************/
static UPNP_INLINE int ithread_cleanup_thread(void) {
	int ret = 0;

#if defined(WIN32) && defined(PTW32_STATIC_LIB)
	ret = !pthread_win32_thread_detach_np();
#endif

	return ret;
}


/****************************************************************************
 * Function: ithread_mutexattr_init
 *
 *  Description:
 *      Initializes a mutex attribute variable.
 *      Used to set the type of the mutex.
 *  Parameters:
 *      ithread_mutexattr_init * attr (must be valid non NULL pointer to 
 *                                     pthread_mutexattr_t)
 *  Returns:
 *      0 on success, Nonzero on failure.
 *      Always returns 0.
 *      See man page for pthread_mutexattr_init
 ***************************************************************************/
#define ithread_mutexattr_init pthread_mutexattr_init


/****************************************************************************
 * Function: ithread_mutexattr_destroy
 *
 *  Description:
 *      Releases any resources held by the mutex attribute.
 *      Currently there are no resources associated with the attribute
 *  Parameters:
 *      ithread_mutexattr_t * attr (must be valid non NULL pointer to 
 *                                  pthread_mutexattr_t)
 *  Returns:
 *      0 on success, Nonzero on failure.
 *      Always returns 0.
 *      See man page for pthread_mutexattr_destroy
 ***************************************************************************/
#define ithread_mutexattr_destroy pthread_mutexattr_destroy
  
  
/****************************************************************************
 * Function: ithread_mutexattr_setkind_np
 *
 *  Description:
 *      Sets the mutex type in the attribute.
 *      Valid types are: ITHREAD_MUTEX_FAST_NP 
 *                       ITHREAD_MUTEX_RECURSIVE_NP 
 *                       ITHREAD_MUTEX_ERRORCHECK_NP
 *
 *  Parameters:
 *      ithread_mutexattr_t * attr (must be valid non NULL pointer to 
 *                                   ithread_mutexattr_t)
 *      int kind (one of ITHREAD_MUTEX_FAST_NP or ITHREAD_MUTEX_RECURSIVE_NP
 *                or ITHREAD_MUTEX_ERRORCHECK_NP)
 *  Returns:
 *      0 on success. Nonzero on failure.
 *      Returns EINVAL if the kind is not supported.
 *      See man page for pthread_mutexattr_setkind_np
 *****************************************************************************/
#if defined(PTHREAD_MUTEX_RECURSIVE) || defined(__DragonFly__)
	#define ithread_mutexattr_setkind_np pthread_mutexattr_settype
#else
	#define ithread_mutexattr_setkind_np pthread_mutexattr_setkind_np
#endif /* UPNP_USE_RWLOCK */

/****************************************************************************
 * Function: ithread_mutexattr_getkind_np
 *
 *  Description:
 *      Gets the mutex type in the attribute.
 *      Valid types are: ITHREAD_MUTEX_FAST_NP 
 *                       ITHREAD_MUTEX_RECURSIVE_NP 
 *                       ITHREAD_MUTEX_ERRORCHECK_NP
 *
 *  Parameters:
 *      ithread_mutexattr_t * attr (must be valid non NULL pointer to 
 *                                   pthread_mutexattr_t)
 *      int *kind (one of ITHREAD_MUTEX_FAST_NP or ITHREAD_MUTEX_RECURSIVE_NP
 *                or ITHREAD_MUTEX_ERRORCHECK_NP)
 *  Returns:
 *      0 on success. Nonzero on failure.
 *      Always returns 0.
 *      See man page for pthread_mutexattr_getkind_np
 *****************************************************************************/
#if defined(PTHREAD_MUTEX_RECURSIVE) || defined(__DragonFly__)
	#define ithread_mutexattr_getkind_np pthread_mutexattr_gettype
#else
	#define ithread_mutexattr_getkind_np pthread_mutexattr_getkind_np
#endif /* UPNP_USE_RWLOCK */

  
/****************************************************************************
 * Function: ithread_mutex_init
 *
 *  Description:
 *      Initializes mutex.
 *      Must be called before use.
 *      
 *  Parameters:
 *      ithread_mutex_t * mutex (must be valid non NULL pointer to pthread_mutex_t)
 *      const ithread_mutexattr_t * mutex_attr 
 *  Returns:
 *      0 on success, Nonzero on failure.
 *      Always returns 0.
 *      See man page for pthread_mutex_init
 *****************************************************************************/
#define ithread_mutex_init pthread_mutex_init


/****************************************************************************
 * Function: ithread_mutex_lock
 *
 *  Description:
 *      Locks mutex.
 *  Parameters:
 *      ithread_mutex_t * mutex (must be valid non NULL pointer to pthread_mutex_t)
 *      mutex must be initialized.
 *      
 *  Returns:
 *      0 on success, Nonzero on failure.
 *      Always returns 0.
 *      See man page for pthread_mutex_lock
 *****************************************************************************/
#define ithread_mutex_lock pthread_mutex_lock
  

/****************************************************************************
 * Function: ithread_mutex_unlock
 *
 *  Description:
 *      Unlocks mutex.
 *
 *  Parameters:
 *      ithread_mutex_t * mutex (must be valid non NULL pointer to pthread_mutex_t)
 *      mutex must be initialized.
 *      
 *  Returns:
 *      0 on success, Nonzero on failure.
 *      Always returns 0.
 *      See man page for pthread_mutex_unlock
 *****************************************************************************/
#define ithread_mutex_unlock pthread_mutex_unlock


/****************************************************************************
 * Function: ithread_mutex_destroy
 *
 *  Description:
 *      Releases any resources held by the mutex. 
 *		Mutex can no longer be used after this call.
 *		Mutex is only destroyed when there are no longer any threads waiting on it. 
 *		Mutex cannot be destroyed if it is locked.
 *  Parameters:
 *      ithread_mutex_t * mutex (must be valid non NULL pointer to pthread_mutex_t)
 *      mutex must be initialized.
 *  Returns:
 *      0 on success. Nonzero on failure.
 *      Always returns 0.
 *      See man page for pthread_mutex_destroy
 *****************************************************************************/
#define ithread_mutex_destroy pthread_mutex_destroy


/****************************************************************************
 * Function: ithread_rwlockattr_init
 *
 *  Description:
 *      Initializes a rwlock attribute variable to default values.
 *  Parameters:
 *      const ithread_rwlockattr_init *attr (must be valid non NULL pointer to 
 *                                           pthread_rwlockattr_t)
 *  Returns:
 *      0 on success, Nonzero on failure.
 *      Always returns 0.
 *      See man page for pthread_rwlockattr_init
 ***************************************************************************/
#if UPNP_USE_RWLOCK
	#define ithread_rwlockattr_init pthread_rwlockattr_init
#endif /* UPNP_USE_RWLOCK */


/****************************************************************************
 * Function: ithread_rwlockattr_destroy
 *
 *  Description:
 *      Releases any resources held by the rwlock attribute.
 *  Parameters:
 *      ithread_rwlockattr_t *attr (must be valid non NULL pointer to 
 *                                  pthread_rwlockattr_t)
 *  Returns:
 *      0 on success, Nonzero on failure.
 *      Always returns 0.
 *      See man page for pthread_rwlockattr_destroy
 ***************************************************************************/
#if UPNP_USE_RWLOCK
	#define ithread_rwlockattr_destroy pthread_rwlockattr_destroy
#endif /* UPNP_USE_RWLOCK */
  
  
/****************************************************************************
 * Function: ithread_rwlockatttr_setpshared
 *
 *  Description:
 *      Sets the rwlock type in the attribute.
 *      Valid types are: ITHREAD_PROCESS_PRIVATE 
 *                       ITHREAD_PROCESS_SHARED
 *
 *  Parameters:
 *      ithread_rwlockattr_t * attr (must be valid non NULL pointer to 
 *                                   ithread_rwlockattr_t)
 *      int kind (one of ITHREAD_PROCESS_PRIVATE or ITHREAD_PROCESS_SHARED)
 *
 *  Returns:
 *      0 on success. Nonzero on failure.
 *      Returns EINVAL if the kind is not supported.
 *      See man page for pthread_rwlockattr_setkind_np
 *****************************************************************************/
#if UPNP_USE_RWLOCK
	#define ithread_rwlockatttr_setpshared pthread_rwlockatttr_setpshared
#endif /* UPNP_USE_RWLOCK */


/****************************************************************************
 * Function: ithread_rwlockatttr_getpshared
 *
 *  Description:
 *      Gets the rwlock type in the attribute.
 *      Valid types are: ITHREAD_PROCESS_PRIVATE 
 *                       ITHREAD_PROCESS_SHARED 
 *
 *  Parameters:
 *      ithread_rwlockattr_t * attr (must be valid non NULL pointer to 
 *                                   pthread_rwlockattr_t)
 *      int *kind (one of ITHREAD_PROCESS_PRIVATE or ITHREAD_PROCESS_SHARED)
 *
 *  Returns:
 *      0 on success. Nonzero on failure.
 *      Always returns 0.
 *      See man page for pthread_rwlockatttr_getpshared
 *****************************************************************************/
#if UPNP_USE_RWLOCK
	#define ithread_rwlockatttr_getpshared pthread_rwlockatttr_getpshared
#endif /* UPNP_USE_RWLOCK */

  
/****************************************************************************
 * Function: ithread_rwlock_init
 *
 *  Description:
 *      Initializes rwlock.
 *      Must be called before use.
 *      
 *  Parameters:
 *      ithread_rwlock_t *rwlock (must be valid non NULL pointer to pthread_rwlock_t)
 *      const ithread_rwlockattr_t *rwlock_attr 
 *  Returns:
 *      0 on success, Nonzero on failure.
 *      Always returns 0.
 *      See man page for pthread_rwlock_init
 *****************************************************************************/
#if UPNP_USE_RWLOCK
	#define ithread_rwlock_init pthread_rwlock_init
#else
	/* Read-write locks aren't available: use mutex instead. */
	#define ithread_rwlock_init ithread_mutex_init
#endif

/****************************************************************************
 * Function: ithread_rwlock_rdlock
 *
 *  Description:
 *      Locks rwlock for reading.
 *  Parameters:
 *      ithread_rwlock_t *rwlock (must be valid non NULL pointer to pthread_rwlock_t)
 *      rwlock must be initialized.
 *      
 *  Returns:
 *      0 on success, Nonzero on failure.
 *      Always returns 0.
 *      See man page for pthread_rwlock_rdlock
 *****************************************************************************/
#if UPNP_USE_RWLOCK
	#define ithread_rwlock_rdlock pthread_rwlock_rdlock
#else
	/* Read-write locks aren't available: use mutex instead. */
	#define ithread_rwlock_rdlock ithread_mutex_lock
#endif /* UPNP_USE_RWLOCK */

/****************************************************************************
 * Function: ithread_rwlock_wrlock
 *
 *  Description:
 *      Locks rwlock for writting.
 *  Parameters:
 *      ithread_rwlock_t *rwlock (must be valid non NULL pointer to pthread_rwlock_t)
 *      rwlock must be initialized.
 *      
 *  Returns:
 *      0 on success, Nonzero on failure.
 *      Always returns 0.
 *      See man page for pthread_rwlock_wrlock
 *****************************************************************************/
#if UPNP_USE_RWLOCK
	#define ithread_rwlock_wrlock pthread_rwlock_wrlock
#else
	/* Read-write locks aren't available: use mutex instead. */
	#define ithread_rwlock_wrlock ithread_mutex_lock
#endif /* UPNP_USE_RWLOCK */


/****************************************************************************
 * Function: ithread_rwlock_unlock
 *
 *  Description:
 *      Unlocks rwlock.
 *
 *  Parameters:
 *      ithread_rwlock_t *rwlock (must be valid non NULL pointer to pthread_rwlock_t)
 *      rwlock must be initialized.
 *      
 *  Returns:
 *      0 on success, Nonzero on failure.
 *      Always returns 0.
 *      See man page for pthread_rwlock_unlock
 *****************************************************************************/
#if UPNP_USE_RWLOCK
	#define ithread_rwlock_unlock pthread_rwlock_unlock
#else
	/* Read-write locks aren't available: use mutex instead. */
	#define ithread_rwlock_unlock ithread_mutex_unlock
#endif /* UPNP_USE_RWLOCK */


/****************************************************************************
 * Function: ithread_rwlock_destroy
 *
 *  Description:
 *      Releases any resources held by the rwlock. 
 *		rwlock can no longer be used after this call.
 *		rwlock is only destroyed when there are no longer any threads waiting on it. 
 *		rwlock cannot be destroyed if it is locked.
 *  Parameters:
 *      ithread_rwlock_t *rwlock (must be valid non NULL pointer to pthread_rwlock_t)
 *      rwlock must be initialized.
 *  Returns:
 *      0 on success. Nonzero on failure.
 *      Always returns 0.
 *      See man page for pthread_rwlock_destroy
 *****************************************************************************/
#if UPNP_USE_RWLOCK
	#define ithread_rwlock_destroy pthread_rwlock_destroy
#else
	/* Read-write locks aren't available: use mutex instead. */
	#define ithread_rwlock_destroy ithread_mutex_destroy
#endif /* UPNP_USE_RWLOCK */


/****************************************************************************
 * Function: ithread_cond_init
 *
 *  Description:
 *      Initializes condition variable.
 *      Must be called before use.
 *  Parameters:
 *      ithread_cond_t *cond (must be valid non NULL pointer to pthread_cond_t)
 *      const ithread_condattr_t *cond_attr (ignored)
 *  Returns:
 *      0 on success, Nonzero on failure.
 *      See man page for pthread_cond_init
 *****************************************************************************/
#define ithread_cond_init pthread_cond_init


/****************************************************************************
 * Function: ithread_cond_signal
 *
 *  Description:
 *      Wakes up exactly one thread waiting on condition.
 *      Associated mutex MUST be locked by thread before entering this call.
 *  Parameters:
 *      ithread_cond_t *cond (must be valid non NULL pointer to 
 *      ithread_cond_t)
 *      cond must be initialized
 *  Returns:
 *      0 on success, Nonzero on failure.
 *      See man page for pthread_cond_signal
 *****************************************************************************/
#define ithread_cond_signal pthread_cond_signal


/****************************************************************************
 * Function: ithread_cond_broadcast
 *
 *  Description:
 *      Wakes up all threads waiting on condition.
 *      Associated mutex MUST be locked by thread before entering this call.
 *  Parameters:
 *      ithread_cond_t *cond (must be valid non NULL pointer to 
 *      ithread_cond_t)
 *      cond must be initialized
 *  Returns:
 *      0 on success, Nonzero on failure.
 *      See man page for pthread_cond_broadcast
 *****************************************************************************/
#define ithread_cond_broadcast pthread_cond_broadcast
  

/****************************************************************************
 * Function: ithread_cond_wait
 *
 *  Description:
 *      Atomically releases mutex and waits on condition.
 *      Associated mutex MUST be locked by thread before entering this call.
 *      Mutex is reacquired when call returns.
 *  Parameters:
 *      ithread_cond_t *cond (must be valid non NULL pointer to 
 *      ithread_cond_t)
 *      cond must be initialized
 *      ithread_mutex_t *mutex (must be valid non NULL pointer to 
 *      ithread_mutex_t)
 *      Mutex must be locked.
 *  Returns:
 *      0 on success, Nonzero on failure.
 *      See man page for pthread_cond_wait
 *****************************************************************************/
#define ithread_cond_wait pthread_cond_wait
  

  /****************************************************************************
   * Function: pthread_cond_timedwait
   *
   *	Description:      
   *		Atomically releases the associated mutex and waits on the
   *	condition.
   *		If the condition is not signaled in the specified time than the
   *	call times out and returns.
   *		Associated mutex MUST be locked by thread before entering this call.
   *		Mutex is reacquired when call returns.
   *  Parameters:
   *      ithread_cond_t *cond (must be valid non NULL pointer to ithread_cond_t)
   *      	cond must be initialized
   *      ithread_mutex_t *mutex (must be valid non NULL pointer to ithread_mutex_t)
   *      	Mutex must be locked.
   *      const struct timespec *abstime (absolute time, measured from Jan 1, 1970)
   *  Returns:
   *      0 on success. ETIMEDOUT on timeout. Nonzero on failure.
   *      See man page for pthread_cond_timedwait
   ***************************************************************************/
 
#define ithread_cond_timedwait pthread_cond_timedwait
  

  /****************************************************************************
   * Function: ithread_cond_destroy
   *
   *  Description:
   *      Releases any resources held by the condition variable. 
   *		Condition variable can no longer be used after this call.	
   *  Parameters:
   *      ithread_cond_t *cond (must be valid non NULL pointer to 
   *      ithread_cond_t)
   *      cond must be initialized.
   *  Returns:
   *      0 on success. Nonzero on failure.
   *      See man page for pthread_cond_destroy
   ***************************************************************************/
#define ithread_cond_destroy pthread_cond_destroy

  /****************************************************************************
   * Function: ithread_attr_init
   *
   *  Description:
   *      Initialises thread attribute object.
   *  Parameters:
   *      ithread_attr_t *attr (must be valid non NULL pointer to
   *      ithread_attr_t)
   *  Returns:
   *      0 on success. Nonzero on failure.
   *      See man page for pthread_attr_init
   ***************************************************************************/
#define ithread_attr_init pthread_attr_init

  /****************************************************************************
   * Function: ithread_attr_destroy
   *
   *  Description:
   *      Destroys thread attribute object.
   *  Parameters:
   *      ithread_attr_t *attr (must be valid non NULL pointer to
   *      ithread_attr_t)
   *  Returns:
   *      0 on success. Nonzero on failure.
   *      See man page for pthread_attr_destroy
   ***************************************************************************/
#define ithread_attr_destroy pthread_attr_destroy

  /****************************************************************************
   * Function: ithread_attr_setstacksize
   *
   *  Description:
   *      Sets stack size of a thread attribute object.
   *  Parameters:
   *      ithread_attr_t *attr (must be valid non NULL pointer to
   *      ithread_attr_t)
   *      size_t stacksize (value of stacksize must be greater than
   *      ITHREAD_STACK_MIN and lower than system-imposed limits
   *  Returns:
   *      0 on success. Nonzero on failure.
   *      See man page for pthread_attr_setstacksize
   ***************************************************************************/
#define ithread_attr_setstacksize pthread_attr_setstacksize

  /****************************************************************************
   * Function: ithread_attr_setdetachstate
   *
   *  Description:
   *      Sets detach state of a thread attribute object.
   *  Parameters:
   *      ithread_attr_t *attr (must be valid non NULL pointer to
   *      ithread_attr_t)
   *      int detachstate (value of detachstate must be ITHREAD_CREATE_DETACHED
   *      or ITHREAD_CREATE_JOINABLE)
   *  Returns:
   *      0 on success. Nonzero on failure.
   *      See man page for pthread_attr_setdetachstate
   ***************************************************************************/
#define ithread_attr_setdetachstate pthread_attr_setdetachstate

  /****************************************************************************
   * Function: ithread_create
   *
   *  Description:
   *		Creates a thread with the given start routine
   *      and argument.
   *  Parameters:
   *      ithread_t * thread (must be valid non NULL pointer to pthread_t)
   *      ithread_attr_t *attr
   *      void * (start_routine) (void *arg) (start routine)
   *      void * arg - argument.
   *  Returns:
   *      0 on success. Nonzero on failure.
   *	    Returns EAGAIN if a new thread can not be created.
   *      Returns EINVAL if there is a problem with the arguments.
   *      See man page fore pthread_create
   ***************************************************************************/
#define ithread_create pthread_create


  /****************************************************************************
   * Function: ithread_cancel
   *
   *  Description:
   *		Cancels a thread.
   *  Parameters:
   *      ithread_t * thread (must be valid non NULL pointer to ithread_t)
   *  Returns:
   *      0 on success. Nonzero on failure.
   *      See man page for pthread_cancel
   ***************************************************************************/
#define ithread_cancel pthread_cancel
  

  /****************************************************************************
   * Function: ithread_exit
   *
   *  Description:
   *		Returns a return code from a thread.
   *      Implicitly called when the start routine returns.
   *  Parameters:
   *      void  * return_code return code to return
   *      See man page for pthread_exit
   ***************************************************************************/
#define ithread_exit pthread_exit


/****************************************************************************
   * Function: ithread_get_current_thread_id
   *
   *  Description:
   *		Returns the handle of the currently running thread.
   *  Returns:
   *		The handle of the currently running thread.
   *              See man page for pthread_self
   ***************************************************************************/
#define ithread_get_current_thread_id pthread_self


  /****************************************************************************
   * Function: ithread_self
   *
   *  Description:
   *		Returns the handle of the currently running thread.
   *  Returns:
   *		The handle of the currently running thread.
   *              See man page for pthread_self
   ***************************************************************************/
#define ithread_self pthread_self


  /****************************************************************************
   * Function: ithread_detach
   *
   *  Description:
   *		Makes a thread's resources reclaimed immediately 
   *            after it finishes
   *            execution.  
   *  Returns:
   *		0 on success, Nonzero on failure.
   *      See man page for pthread_detach
   ***************************************************************************/
#define ithread_detach pthread_detach  


  /****************************************************************************
   * Function: ithread_join
   *
   *  Description:
   *		Suspends the currently running thread until the 
   * specified thread
   *      has finished. 
   *      Returns the return code of the thread, or ITHREAD_CANCELED 
   *      if the thread has been canceled.
   *  Parameters:
   *      ithread_t *thread (valid non null thread identifier)
   *      void ** return (space for return code) 
   *  Returns:
   *		0 on success, Nonzero on failure.
   *     See man page for pthread_join
   ***************************************************************************/
#define ithread_join pthread_join
  

/****************************************************************************
 * Function: isleep
 *
 *  Description:
 *		Suspends the currently running thread for the specified number 
 *      of seconds
 *      Always returns 0.
 *  Parameters:
 *      unsigned int seconds - number of seconds to sleep.
 *  Returns:
 *		0 on success, Nonzero on failure.
 *              See man page for sleep (man 3 sleep)
 *****************************************************************************/
#ifdef WIN32
	#define isleep(x) Sleep((x)*1000)
#else
	#define isleep sleep
#endif


/****************************************************************************
 * Function: isleep
 *
 *  Description:
 *		Suspends the currently running thread for the specified number 
 *      of milliseconds
 *      Always returns 0.
 *  Parameters:
 *      unsigned int milliseconds - number of milliseconds to sleep.
 *  Returns:
 *		0 on success, Nonzero on failure.
 *              See man page for sleep (man 3 sleep)
 *****************************************************************************/
#ifdef WIN32
	#define imillisleep Sleep
#else
	#define imillisleep(x) usleep(1000*x)
#endif


#if !defined(PTHREAD_MUTEX_RECURSIVE) && !defined(__DragonFly__) && !defined(UPNP_USE_MSVCPP)
/* !defined(UPNP_USE_MSVCPP) should probably also have pthreads version check - but it's not clear if that is possible */
/* NK: Added for satisfying the gcc compiler */
EXPORT_SPEC int pthread_mutexattr_setkind_np(pthread_mutexattr_t *attr, int kind);
#endif


#ifdef __cplusplus
}
#endif


#endif /* ITHREAD_H */

