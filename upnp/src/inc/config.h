/**************************************************************************
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
 **************************************************************************/

#ifndef INTERNAL_CONFIG_H
#define INTERNAL_CONFIG_H 


#include "autoconfig.h"


/*!
 *  \name Compile time configuration options
 *
 *  The Linux SDK for UPnP Devices contains some compile-time parameters 
 *  that effect the behavior of the SDK.  All configuration options are 
 *  located in {\tt src/inc/config.h}.
 *
 * @{
 */


/*!
 *  \name THREAD_IDLE_TIME
 *
 *  The {\tt THREAD_IDLE_TIME} constant determines when a thread will be
 *  removed from the thread pool and returned to the operating system. When 
 *  a thread in the thread pool has been idle for this number of milliseconds
 *  the thread will be released from the thread pool.  The default value is
 *  5000 milliseconds (5 seconds).
 *
 * @{
 */
#define THREAD_IDLE_TIME 5000
/* @} */


/*!
 * \name JOBS_PER_THREAD
 *
 * The {\tt JOBS_PER_THREAD} constant determines when a new thread will be
 * allocated to the thread pool inside the  SDK. The thread pool will
 * try and maintain this jobs/thread ratio. When the jobs/thread ratio 
 * becomes greater than this, then a new thread (up to the max) will be 
 * allocated to the thread pool.  The default ratio is 10 jobs/thread.
 *
 * @{
 */
#define JOBS_PER_THREAD 10
/* @} */


/*!
 * \name MIN_THREADS
 *
 * The {\tt MIN_THREADS} constant defines the minimum number of threads the
 * thread pool inside the SDK will create.  The thread pool will
 * always have this number of threads. These threads are used
 * for both callbacks into applications built on top of the SDK and also
 * for making connections to other control points and devices. This number
 * includes persistent threads.  The default value is two threads.
 *
 * @{
 */
#define MIN_THREADS 2 
/* @} */


/*!
 * \name MAX_THREADS
 *
 * The {\tt MAX_THREADS} constant defines the maximum number of threads the
 * thread pool inside the SDK will create.  These threads are used
 * for both callbacks into applications built on top of the library and also 
 * for making connections to other control points and devices.  It is not 
 * recommended that this value be below 10, since the threads are 
 * necessary for correct operation.  This value can be increased for greater
 * performance in operation at the expense of greater memory overhead.  The 
 * default value is 12.
 *
 * @{
 */
#define MAX_THREADS 12 
/* @} */


/*!
 * \name THREAD_STACK_SIZE
 *
 * The {\tt THREAD_STACK_SIZE} constant defines the minimum stack size (in
 * bytes) allocated for the stack of each thread the thread pool inside the
 * SDK will create.  These threads are used for both callbacks into
 * applications built on top of the library and also for making connections
 * to other control points and devices.  This value will not be used if it
 * is lower than ITHREAD_STACK_MIN or greater than a system-imposed limit.
 * This value can be used to lower memory overhead in embedded systems.
 * The default value is 0 (so it is not used by default).
 *
 * @{
 */
#define THREAD_STACK_SIZE (size_t)0
/* @} */


/*! \name MAX_JOBS_TOTAL
 *
 *  The {\tt MAX_JOBS_TOTAL} constant determines the maximum number of jobs
 *  that can be queued. If this limit is reached further jobs will be thrown
 *  to avoid memory exhaustion. The default value 100.
 *  (Added by Axis.)
 *
 * @{
 */
#define MAX_JOBS_TOTAL 100
/* @} */


/*!
 * \name DEFAULT_SOAP_CONTENT_LENGTH
 *
 * SOAP messages will read at most {\tt DEFAULT_SOAP_CONTENT_LENGTH} bytes.  
 * This prevents devices that have a misbehaving web server to send 
 * a large amount of data to the control point causing it to crash.  
 * This can be adjusted dynamically with {\tt UpnpSetMaxContentLength}.
 *
 * @{
 */
#define DEFAULT_SOAP_CONTENT_LENGTH 16000
/* @} */


/*!
 * \name NUM_SSDP_COPY
 *
 * This configuration parameter determines how many copies of each SSDP 
 * advertisement and search packets will be sent. By default it will send two 
 * copies of every packet.  
 *
 * @{
 */
#define NUM_SSDP_COPY  2
/* @} */


/*!
 * \name SSDP_PAUSE
 *
 * This configuration parameter determines the pause between identical SSDP 
 * advertisement and search packets. The pause is measured in milliseconds
 * and defaults to 100.
 *
 * @{
 */
#define SSDP_PAUSE  100u
/* @} */

/*!
 * \name WEB_SERVER_BUF_SIZE
 * 
 * This configuration parameter sets the maximum buffer size for the 
 * webserver. The default value is 1MB.
 *
 * @{
 */
#define WEB_SERVER_BUF_SIZE  (size_t)(1024*1024)
/* @} */

/*!
 * \name WEB_SERVER_CONTENT_LANGUAGE
 *
 * This configuration parameter sets the value of the Content-Language
 * header for the webserver. Thanks to this parameter, the use can advertize
 * the language used by the device in the description (friendlyName) and
 * presentation steps of UPnP. The default value is empty string so no
 * Content-Language header is added.
 *
 * @{
 */
#define WEB_SERVER_CONTENT_LANGUAGE ""
/* @} */

/*!
 * \name AUTO_RENEW_TIME
 *
 * The {\tt AUTO_RENEW_TIME} is the time, in seconds, before a subscription
 * expires that the SDK automatically resubscribes.  The default 
 * value is 10 seconds.  Setting this value too low can result in the 
 * subscription renewal not making it to the device in time, causing the 
 * subscription to timeout. In order to avoid continually resubscribing
 * the minimum subscription time is five seconds more than the auto renew
 * time.
 *
 * @{
 */
#define AUTO_RENEW_TIME 10
/* @} */

/*!
 * \name CP_MINIMUM_SUBSCRIPTION_TIME
 *
 * The {\tt CP_MINIMUM_SUBSCRIPTION_TIME} is the minimum subscription time
 * allowed for a control point using the SDK. Subscribing for less than
 * this time automatically results in a subscription for this amount.  The 
 * default value is 5 seconds more than the {\tt AUTO_RENEW_TIME}, or 15
 * seconds.
 *
 * @{
 */
#define CP_MINIMUM_SUBSCRIPTION_TIME (AUTO_RENEW_TIME + 5)
/* @} */


/*!
 * \name MAX_SEARCH_TIME
 *
 * The {\tt MAX_SEARCH_TIME} is the maximum time
 * allowed for an SSDP search by a control point. Searching for greater than
 * this time automatically results in a search for this amount.  The default 
 * value is 80 seconds.
 *
 * @{
 */
#define MAX_SEARCH_TIME 80
/* @} */


/*!
 * \name MIN_SEARCH_TIME
 *
 * The {\tt MIN_SEARCH_TIME} is the minimumm time
 * allowed for an SSDP search by a control point. Searching for less than
 * this time automatically results in a search for this amount.  The default 
 * value is 2 seconds.
 *
 * @{
 */
#define MIN_SEARCH_TIME 2
/* @} */


/*!
 * \name AUTO_ADVERTISEMENT_TIME
 *
 * The {\tt AUTO_ADVERTISEMENT_TIME} is the time, in seconds, before an
 * device advertisements expires before a renewed advertisement is sent.
 * The default time is 30 seconds.
 *
 * @{
 */
#define AUTO_ADVERTISEMENT_TIME 30
/* @} */


/*!
 * \name SSDP_PACKET_DISTRIBUTE
 *
 * The {\tt SSDP_PACKET_DISTRIBUTE} enables the SSDP packets to be sent
 * at an interval equal to half of the expiration time of SSDP packets
 * minus the AUTO_ADVERTISEMENT_TIME. This is used to increase
 * the probability of SSDP packets reaching to control points.
 * It is recommended that this flag be turned on for embedded wireless 
 * devices.
 *
 * @{
 */
#define SSDP_PACKET_DISTRIBUTE 1
/* @} */


/*!
 * \name GENA_NOTIFICATION_SENDING_TIMEOUT
 *
 * The {\tt GENA_NOTIFICATION_SENDING_TIMEOUT} specifies the number of seconds
 * to wait for sending GENA notifications to the Control Point.
 *
 * This timeout will be used to know how many seconds GENA notification threads
 * will wait to write on the socket to send the notification. By putting a
 * lower value than HTTP_DEFAULT_TIMEOUT, the thread will not wait too long and
 * will return quickly if writing is impossible. This is very useful as some
 * Control Points disconnect from the network without unsubscribing as a result
 * if HTTP_DEFAULT_TIMEOUT is used, all the GENA threads will be blocked to send
 * notifications to those disconnected Control Points until the subscription
 * expires.
 *
 * @{
 */
#define GENA_NOTIFICATION_SENDING_TIMEOUT HTTP_DEFAULT_TIMEOUT
/* @} */


/*!
 * \name GENA_NOTIFICATION_ANSWERING_TIMEOUT
 *
 * The {\tt GENA_NOTIFICATION_ANSWERING_TIMEOUT} specifies the number of seconds
 * to wait for receiving the answer to a GENA notification from the Control
 * Point.
 *
 * This timeout will be used to know how many seconds GENA notification threads
 * will wait on the socket to read for an answer from the CP. By putting a
 * lower value than HTTP_DEFAULT_TIMEOUT, the thread will not wait too long and
 * will return quickly if there is no answer from the CP. This is very useful as
 * some Control Points disconnect from the network without unsubscribing and if
 * HTTP_DEFAULT_TIMEOUT is used, all the GENA threads will be blocked to wait
 * for an answer from those disconnected Control Points until the subscription
 * expires. However, it should be noted that UDA specifies a value of 30s for
 * waiting the CP's answer.
 *
 * @{
 */
#define GENA_NOTIFICATION_ANSWERING_TIMEOUT HTTP_DEFAULT_TIMEOUT
/* @} */


/*!
 * \name Module Exclusion
 *
 * Depending on the requirements, the user can selectively discard any of 
 * the major modules like SOAP, GENA, SSDP or the Internal web server. By 
 * default everything is included inside the SDK.  By setting any of
 * the values below to 0, that component will not be included in the final
 * SDK.
 * \begin{itemize}
 *   \item {\tt EXCLUDE_SOAP[0,1]}
 *   \item {\tt EXCLUDE_GENA[0,1]}
 *   \item {\tt EXCLUDE_SSDP[0,1]}
 *   \item {\tt EXCLUDE_DOM [0,1]}
 *   \item {\tt EXCLUDE_MINISERVER[0,1]}
 *   \item {\tt EXCLUDE_WEB_SERVER[0,1]}
 *   \item {\tt EXCLUDE_JNI[0,1]}
 * \end{itemize}
 *
 * @{
 */
#define EXCLUDE_SSDP 0
#define EXCLUDE_SOAP 0
#define EXCLUDE_GENA 0
#define EXCLUDE_DOM  0
#define EXCLUDE_MINISERVER 0
#define EXCLUDE_WEB_SERVER 0
#ifdef USE_JNI
#	define EXCLUDE_JNI 0
#else
#	define EXCLUDE_JNI 1
#endif
/* @} */

    
/*!
 * \name DEBUG_TARGET
 *
 * The user has the option to redirect the library output debug messages 
 * to either the screen or to a log file.  All the output messages with 
 * debug level 0 will go to {\tt upnp.err} and messages with debug level 
 * greater than zero will be redirected to {\tt upnp.out}.
 *
 * @{
 */
#define DEBUG_TARGET		1   
/* @} */


/*!
 * \name Other debugging features
 *
 * The UPnP SDK contains other features to aid in debugging:
 * see <upnp/inc/upnpdebug.h>
 */

#define DEBUG_ALL		1
#define DEBUG_SSDP		0
#define DEBUG_SOAP		0
#define DEBUG_GENA		0
#define DEBUG_TPOOL		0
#define DEBUG_MSERV		0
#define DEBUG_DOM		0
#define DEBUG_HTTP		0
#define DEBUG_API		0


/*
 * @} Compile time configuration options
 */


/***************************************************************************
 * Do not change, Internal purpose only!!!
 ***************************************************************************/ 

/*!
 * @{
 */


/*
 * Set additional defines based on requested configuration 
 */


/* configure --enable-client */
#if UPNP_HAVE_CLIENT
#	define INCLUDE_CLIENT_APIS	1
#endif


/* configure --enable-device */
#if UPNP_HAVE_DEVICE
#	define INCLUDE_DEVICE_APIS	1
#endif


/* configure --enable-webserver */
#if UPNP_HAVE_WEBSERVER
#	define INTERNAL_WEB_SERVER	1
#endif

/* configure --enable-ssdp */
#undef EXCLUDE_SSDP
#if UPNP_HAVE_SSDP
#       define EXCLUDE_SSDP 0
#else
#       define EXCLUDE_SSDP 1
#endif

/* configure --enable-soap */
#undef EXCLUDE_SOAP
#if UPNP_HAVE_SOAP
#	define EXCLUDE_SOAP 0
#else
#	define EXCLUDE_SOAP 1
#endif

/* configure --enable-gena */
#undef EXCLUDE_GENA
#if UPNP_HAVE_GENA
#       define EXCLUDE_GENA 0
#else
#       define EXCLUDE_GENA 1
#endif

#undef  EXCLUDE_WEB_SERVER
#undef  EXCLUDE_MINISERVER
#ifdef  INTERNAL_WEB_SERVER
#	define EXCLUDE_WEB_SERVER 0
#	define EXCLUDE_MINISERVER 0
#else
#	define EXCLUDE_WEB_SERVER 1
#	define EXCLUDE_MINISERVER 1
#endif


#if EXCLUDE_SSDP == 1 && EXCLUDE_GENA == 1 && EXCLUDE_SOAP == 1 && EXCLUDE_WEB_SERVER == 1
#	undef  EXCLUDE_MINISERVER
#	define EXCLUDE_MINISERVER 1
#	if INTERNAL_WEB_SERVER
#		error "conflicting settings: use configure --disable-webserver"
#	endif
#endif


#if EXCLUDE_SSDP == 0 || EXCLUDE_GENA == 0 || EXCLUDE_SOAP == 0 || EXCLUDE_WEB_SERVER == 0
#	undef  EXCLUDE_MINISERVER
#	define EXCLUDE_MINISERVER 0
#	if EXCLUDE_WEB_SERVER == 0 && !defined INTERNAL_WEB_SERVER
#		error "conflicting settings : use configure --enable-webserver"
#	endif
#endif


/*
 * @}
 */

#endif /* INTERNAL_CONFIG_H */

