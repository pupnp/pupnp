/*******************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation 
 * All rights reserved. 
 * Copyright (C) 2011 France Telecom All rights reserved. 
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
 * \addtogroup UPnPAPI
 * 
 * @{
 * 
 * \file
 */

#include "config.h"

#include "upnpapi.h"

#include "httpreadwrite.h"
#include "membuffer.h"
#include "ssdplib.h"
#include "soaplib.h"
#include "sysdep.h"
#include "ThreadPool.h"
#include "UpnpStdInt.h"
#include "UpnpUniStd.h" /* for close() */
#include "uuid.h"

/* Needed for GENA */
#include "gena.h"
#include "miniserver.h"
#include "service_table.h"

#ifdef INTERNAL_WEB_SERVER
	#include "urlconfig.h"
	#include "VirtualDir.h"
	#include "webserver.h"
#endif /* INTERNAL_WEB_SERVER */

#include <sys/stat.h>

#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
	/* Do not include these files */
#else
	#include <sys/ioctl.h>
	#include <sys/param.h>
	#include <sys/types.h>
#endif

#ifndef IN6_IS_ADDR_GLOBAL
#define IN6_IS_ADDR_GLOBAL(a) \
		((((__const uint32_t *) (a))[0] & htonl(0x70000000)) \
		== htonl (0x20000000))
#endif /* IS ADDR GLOBAL */

#ifndef IN6_IS_ADDR_ULA
#define IN6_IS_ADDR_ULA(a) \
		((((__const uint32_t *) (a))[0] & htonl(0xfe000000)) \
		== htonl (0xfc000000))
#endif /* IS ADDR ULA */

/*! This structure is for virtual directory callbacks */
struct VirtualDirCallbacks virtualDirCallback;

/*! Pointer to the virtual directory list. */
virtualDirList *pVirtualDirList;

#ifdef INCLUDE_CLIENT_APIS
/*! Mutex to synchronize the subscription handling at the client side. */
ithread_mutex_t GlobalClientSubscribeMutex;
#endif /* INCLUDE_CLIENT_APIS */

/*! rwlock to synchronize handles (root device or control point handle). */
ithread_rwlock_t GlobalHndRWLock;


/*! Mutex to synchronize the uuid creation process. */
ithread_mutex_t gUUIDMutex;

/*! Initialization mutex. */
ithread_mutex_t gSDKInitMutex = PTHREAD_MUTEX_INITIALIZER;

/*! Global timer thread. */
TimerThread gTimerThread;

/*! Send thread pool. */
ThreadPool gSendThreadPool;

/*! Receive thread pool. */
ThreadPool gRecvThreadPool;

/*! Mini server thread pool. */
ThreadPool gMiniServerThreadPool;

/*! Flag to indicate the state of web server */
WebServerState bWebServerState = WEB_SERVER_DISABLED;

/*! Static buffer to contain interface name. (extern'ed in upnp.h) */
char gIF_NAME[LINE_SIZE] = { '\0' };

/*! Static buffer to contain interface IPv4 address. (extern'ed in upnp.h) */
char gIF_IPV4[22]/* INET_ADDRSTRLEN*/ = { '\0' };

/*! Static buffer to contain interface IPv6 address. (extern'ed in upnp.h) */
char gIF_IPV6[65]/* INET6_ADDRSTRLEN*/ = { '\0' };

/*! Static buffer to contain interface ULA or GUA IPv6 address. (extern'ed in upnp.h) */
char gIF_IPV6_ULA_GUA[INET6_ADDRSTRLEN] = { '\0' };

/*! Contains interface index. (extern'ed in upnp.h) */
unsigned gIF_INDEX = (unsigned)-1;

/*! local IPv4 port for the mini-server */
unsigned short LOCAL_PORT_V4;

/*! local IPv6 port for the mini-server */
unsigned short LOCAL_PORT_V6;

/*! UPnP device and control point handle table  */
#define NUM_HANDLE 200
static void *HandleTable[NUM_HANDLE];

/*! a local dir which serves as webserver root */
extern membuffer gDocumentRootDir;

/*! Maximum content-length (in bytes) that the SDK will process on an incoming
 * packet. Content-Length exceeding this size will be not processed and
 * error 413 (HTTP Error Code) will be returned to the remote end point. */
size_t g_maxContentLength = DEFAULT_SOAP_CONTENT_LENGTH;

/*! Global variable to denote the state of Upnp SDK == 0 if uninitialized,
 * == 1 if initialized. */
int UpnpSdkInit = 0;

/*! Global variable to denote the state of Upnp SDK client registration.
 * == 0 if unregistered, == 1 if registered. */
int UpnpSdkClientRegistered = 0;

/*! Global variable to denote the state of Upnp SDK IPv4 device registration.
 * == 0 if unregistered, == 1 if registered. */
int UpnpSdkDeviceRegisteredV4 = 0;

/*! Global variable to denote the state of Upnp SDK IPv6 device registration.
 * == 0 if unregistered, == 1 if registered. */
int UpnpSdkDeviceregisteredV6 = 0;

/*! Global variable used in discovery notifications. */
Upnp_SID gUpnpSdkNLSuuid;


/*!
 * \brief (Windows Only) Initializes the Windows Winsock library.
 *
 * \return UPNP_E_SUCCESS on success, UPNP_E_INIT_FAILED on failure.
 */
static int WinsockInit(void)
{
	int retVal = UPNP_E_SUCCESS;
#ifdef WIN32
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	wVersionRequested = MAKEWORD(2, 2);
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		/* Tell the user that we could not find a usable */
		/* WinSock DLL.                                  */
		retVal = UPNP_E_INIT_FAILED;
		goto exit_function;
	}
	/* Confirm that the WinSock DLL supports 2.2.
	 * Note that if the DLL supports versions greater
	 * than 2.2 in addition to 2.2, it will still return
	 * 2.2 in wVersion since that is the version we
	 * requested. */
	if (LOBYTE(wsaData.wVersion) != 2 ||
	    HIBYTE(wsaData.wVersion) != 2) {
		/* Tell the user that we could not find a usable
		 * WinSock DLL. */
		WSACleanup();
		retVal = UPNP_E_INIT_FAILED; 
		goto exit_function;
	}
	/* The WinSock DLL is acceptable. Proceed. */
exit_function:
#else
#endif
	return retVal;
}


/*!
 * \brief Initializes the global mutexes used by the UPnP SDK.
 *
 * \return UPNP_E_SUCCESS on success or UPNP_E_INIT_FAILED if a mutex could not
 * 	be initialized.
 */
static int UpnpInitMutexes(void)
{
#ifdef __CYGWIN__
	/* On Cygwin, pthread_mutex_init() fails without this memset. */
	/* TODO: Fix Cygwin so we don't need this memset(). */
	memset(&GlobalHndRWLock, 0, sizeof(GlobalHndRWLock));
#endif
	if (ithread_rwlock_init(&GlobalHndRWLock, NULL) != 0) {
		return UPNP_E_INIT_FAILED;
	}

	if (ithread_mutex_init(&gUUIDMutex, NULL) != 0) {
		return UPNP_E_INIT_FAILED;
	}
	/* initialize subscribe mutex. */
#ifdef INCLUDE_CLIENT_APIS
	if (ithread_mutex_init(&GlobalClientSubscribeMutex, NULL) != 0) {
		return UPNP_E_INIT_FAILED;
	}
#endif
	return UPNP_E_SUCCESS;
}


/*!
 * \brief Initializes the global threadm pools used by the UPnP SDK.
 *
 * \return UPNP_E_SUCCESS on success or UPNP_E_INIT_FAILED if a mutex could not
 * 	be initialized.
 */
static int UpnpInitThreadPools(void)
{
	int ret = UPNP_E_SUCCESS;
	ThreadPoolAttr attr;

	TPAttrInit(&attr);
	TPAttrSetMaxThreads(&attr, MAX_THREADS);
	TPAttrSetMinThreads(&attr, MIN_THREADS);
	TPAttrSetStackSize(&attr, THREAD_STACK_SIZE);
	TPAttrSetJobsPerThread(&attr, JOBS_PER_THREAD);
	TPAttrSetIdleTime(&attr, THREAD_IDLE_TIME);
	TPAttrSetMaxJobsTotal(&attr, MAX_JOBS_TOTAL);

	if (ThreadPoolInit(&gSendThreadPool, &attr) != UPNP_E_SUCCESS) {
		ret = UPNP_E_INIT_FAILED;
		goto exit_function;
	}

	if (ThreadPoolInit(&gRecvThreadPool, &attr) != UPNP_E_SUCCESS) {
		ret = UPNP_E_INIT_FAILED;
		goto exit_function;
	}

	if (ThreadPoolInit(&gMiniServerThreadPool, &attr) != UPNP_E_SUCCESS) {
		ret = UPNP_E_INIT_FAILED;
		goto exit_function;
	}

exit_function:
	if (ret != UPNP_E_SUCCESS) {
		UpnpSdkInit = 0;
		UpnpFinish();
	}

	return ret;
}


/*!
 * \brief Performs the initial steps in initializing the UPnP SDK.
 *
 *	\li Winsock library is initialized for the process (Windows specific).
 *	\li The logging (for debug messages) is initialized.
 *	\li Mutexes, Handle table and thread pools are allocated and initialized.
 *	\li Callback functions for SOAP and GENA are set, if they're enabled.
 *	\li The SDK timer thread is initialized.
 *
 * \return UPNP_E_SUCCESS on success.
 */
static int UpnpInitPreamble(void)
{
	int retVal = UPNP_E_SUCCESS;
	int i;
	uuid_upnp nls_uuid;

	retVal = WinsockInit();
	if (retVal != UPNP_E_SUCCESS) {
		return retVal;
	}

	/* needed by SSDP or other parts. */
	srand((unsigned int)time(NULL));

	/* Initialize debug output. */
	retVal = UpnpInitLog();
	if (retVal != UPNP_E_SUCCESS) {
		/* UpnpInitLog does not return a valid UPNP_E_*. */
		return UPNP_E_INIT_FAILED;
	}

	UpnpPrintf( UPNP_INFO, API, __FILE__, __LINE__, "Inside UpnpInitPreamble\n" );

	/* Initialize SDK global mutexes. */
	retVal = UpnpInitMutexes();
	if (retVal != UPNP_E_SUCCESS) {
		return retVal;
	}

	/* Create the NLS uuid. */
	uuid_create(&nls_uuid);
	uuid_unpack(&nls_uuid, gUpnpSdkNLSuuid);

	/* Initializes the handle list. */
	HandleLock();
	for (i = 0; i < NUM_HANDLE; ++i) {
		HandleTable[i] = NULL;
	}
	HandleUnlock();

	/* Initialize SDK global thread pools. */
	retVal = UpnpInitThreadPools();
	if (retVal != UPNP_E_SUCCESS) {
		return retVal;
	}

#if EXCLUDE_SOAP == 0
	SetSoapCallback(soap_device_callback);
#endif

#if EXCLUDE_GENA == 0
	SetGenaCallback(genaCallback);
#endif

	/* Initialize the SDK timer thread. */
	retVal = TimerThreadInit( &gTimerThread, &gSendThreadPool );
	if (retVal != UPNP_E_SUCCESS) {
		UpnpFinish();

		return retVal;
	}

	return UPNP_E_SUCCESS;
}


/*!
 * \brief Finishes initializing the UPnP SDK.
 *	\li The MiniServer is started, if enabled.
 *	\li The WebServer is started, if enabled.
 * 
 * \return UPNP_E_SUCCESS on success or  UPNP_E_INIT_FAILED if a mutex could not
 * 	be initialized.
 */
static int UpnpInitStartServers(
	/*! [in] Local Port to listen for incoming connections. */
	unsigned short DestPort)
{
	int retVal = 0;

	UpnpPrintf( UPNP_INFO, API, __FILE__, __LINE__,
		"Entering UpnpInitStartServers\n" );

#if EXCLUDE_MINISERVER == 0
	LOCAL_PORT_V4 = DestPort;
	LOCAL_PORT_V6 = DestPort;
	retVal = StartMiniServer(&LOCAL_PORT_V4, &LOCAL_PORT_V6);
	if (retVal != UPNP_E_SUCCESS) {
		UpnpPrintf(UPNP_CRITICAL, API, __FILE__, __LINE__,
			"Miniserver failed to start");
		UpnpFinish();
		return retVal;
	}
#endif

#if EXCLUDE_WEB_SERVER == 0
	membuffer_init(&gDocumentRootDir);
	retVal = UpnpEnableWebserver(WEB_SERVER_ENABLED);
	if (retVal != UPNP_E_SUCCESS) {
		UpnpFinish();
		return retVal;
	}
#endif

	UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__,
		"Exiting UpnpInitStartServers\n");

	return UPNP_E_SUCCESS;
}


int UpnpInit(const char *HostIP, unsigned short DestPort)
{
	int retVal = UPNP_E_SUCCESS;

	/* Initializes the ithread library */
	ithread_initialize_library();

	ithread_mutex_lock(&gSDKInitMutex);

	/* Check if we're already initialized. */
	if (UpnpSdkInit == 1) {
		retVal = UPNP_E_INIT;
		goto exit_function;
	}

	/* Perform initialization preamble. */
	retVal = UpnpInitPreamble();
	if (retVal != UPNP_E_SUCCESS) {
		goto exit_function;
	}

	UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__,
		"UpnpInit with HostIP=%s, DestPort=%d.\n", 
		HostIP ? HostIP : "", DestPort);

	/* Verify HostIP, if provided, or find it ourselves. */
	if (HostIP != NULL) {
		strncpy(gIF_IPV4, HostIP, sizeof(gIF_IPV4));
	} else {
		if( getlocalhostname( gIF_IPV4, sizeof(gIF_IPV4) ) != UPNP_E_SUCCESS ) {
			retVal = UPNP_E_INIT_FAILED;
			goto exit_function;
		}
	}

	/* Set the UpnpSdkInit flag to 1 to indicate we're successfully initialized. */
	UpnpSdkInit = 1;

	/* Finish initializing the SDK. */
	retVal = UpnpInitStartServers(DestPort);
	if (retVal != UPNP_E_SUCCESS) {
		UpnpSdkInit = 0;
		goto exit_function;
	}

	UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__,
		"Host Ip: %s Host Port: %d\n", gIF_IPV4,
		LOCAL_PORT_V4);

exit_function:
	ithread_mutex_unlock(&gSDKInitMutex);

	return retVal;
}

#ifdef UPNP_ENABLE_IPV6
int UpnpInit2(const char *IfName, unsigned short DestPort)
{
	int retVal;

	/* Initializes the ithread library */
	ithread_initialize_library();

	ithread_mutex_lock(&gSDKInitMutex);

	/* Check if we're already initialized. */
	if (UpnpSdkInit == 1) {
		retVal = UPNP_E_INIT;
		goto exit_function;
	}

	/* Perform initialization preamble. */
	retVal = UpnpInitPreamble();
	if (retVal != UPNP_E_SUCCESS) {
		goto exit_function;
	}

	UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__,
		"UpnpInit2 with IfName=%s, DestPort=%d.\n", 
		IfName ? IfName : "", DestPort);

	/* Retrieve interface information (Addresses, index, etc). */
	retVal = UpnpGetIfInfo( IfName );
	if (retVal != UPNP_E_SUCCESS) {
		goto exit_function;
	}

	/* Set the UpnpSdkInit flag to 1 to indicate we're successfully initialized. */
	UpnpSdkInit = 1;

	/* Finish initializing the SDK. */
	retVal = UpnpInitStartServers(DestPort);
	if (retVal != UPNP_E_SUCCESS) {
		UpnpSdkInit = 0;
		goto exit_function;
	}

exit_function:
	ithread_mutex_unlock(&gSDKInitMutex);

	return retVal;
}
#endif

#ifdef DEBUG
/*!
 * \brief Prints thread pool statistics.
 */
void PrintThreadPoolStats(
	/*! [in] The thread pool. */
	ThreadPool *tp,
	/*! [in] The file name that called this function, use the macro
	 * __FILE__. */
	const char *DbgFileName,
	/*! [in] The line number that the function was called, use the macro
	 * __LINE__. */
	int DbgLineNo,
	/*! [in] The message. */
	const char *msg)
{
	ThreadPoolStats stats;
	ThreadPoolGetStats(tp, &stats);
	UpnpPrintf(UPNP_INFO, API, DbgFileName, DbgLineNo,
		"%s\n"
		"High Jobs pending: %d\n"
		"Med Jobs Pending: %d\n"
		"Low Jobs Pending: %d\n"
		"Average wait in High Q in milliseconds: %lf\n"
		"Average wait in Med Q in milliseconds: %lf\n"
		"Average wait in Low Q in milliseconds: %lf\n"
		"Max Threads Used: %d\n"
		"Worker Threads: %d\n"
		"Persistent Threads: %d\n"
		"Idle Threads: %d\n"
		"Total Threads: %d\n"
		"Total Work Time: %lf\n"
		"Total Idle Time: %lf\n",
		msg,
		stats.currentJobsHQ,
		stats.currentJobsMQ,
		stats.currentJobsLQ,
		stats.avgWaitHQ,
		stats.avgWaitMQ,
		stats.avgWaitLQ,
		stats.maxThreads,
		stats.workerThreads,
		stats.persistentThreads,
		stats.idleThreads,
		stats.totalThreads,
		stats.totalWorkTime,
		stats.totalIdleTime);
}
#else
static UPNP_INLINE void PrintThreadPoolStats(ThreadPool *tp,
	const char *DbgFileName, int DbgLineNo, const char *msg)
{
	return;
	tp = tp;
	DbgFileName = DbgFileName;
	DbgLineNo = DbgLineNo;
	msg = msg;
}
#endif /* DEBUG */

int UpnpFinish(void)
{
#ifdef INCLUDE_DEVICE_APIS
	UpnpDevice_Handle device_handle;
#endif
#ifdef INCLUDE_CLIENT_APIS
	UpnpClient_Handle client_handle;
#endif
	struct Handle_Info *temp;

	if (UpnpSdkInit != 1)
		return UPNP_E_FINISH;
	UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__,
		   "Inside UpnpFinish: UpnpSdkInit is %d\n", UpnpSdkInit);
	if (UpnpSdkInit == 1)
		UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__,
			"UpnpFinish: UpnpSdkInit is ONE\n");
	PrintThreadPoolStats(&gSendThreadPool, __FILE__, __LINE__,
		"Send Thread Pool");
	PrintThreadPoolStats(&gRecvThreadPool, __FILE__, __LINE__,
		"Recv Thread Pool");
	PrintThreadPoolStats(&gMiniServerThreadPool, __FILE__, __LINE__,
		"MiniServer Thread Pool");
#ifdef INCLUDE_DEVICE_APIS
	if (GetDeviceHandleInfo(AF_INET, &device_handle, &temp) == HND_DEVICE)
		UpnpUnRegisterRootDevice(device_handle);
	if (GetDeviceHandleInfo(AF_INET6, &device_handle, &temp) == HND_DEVICE)
		UpnpUnRegisterRootDevice(device_handle);
#endif
#ifdef INCLUDE_CLIENT_APIS
	if (GetClientHandleInfo(&client_handle, &temp) == HND_CLIENT)
		UpnpUnRegisterClient(client_handle);
#endif
	TimerThreadShutdown(&gTimerThread);
	StopMiniServer();
#if EXCLUDE_WEB_SERVER == 0
	web_server_destroy();
#endif
	ThreadPoolShutdown(&gMiniServerThreadPool);
	PrintThreadPoolStats(&gMiniServerThreadPool, __FILE__, __LINE__,
		"MiniServer Thread Pool");
	ThreadPoolShutdown(&gRecvThreadPool);
	PrintThreadPoolStats(&gSendThreadPool, __FILE__, __LINE__,
		"Send Thread Pool");
	ThreadPoolShutdown(&gSendThreadPool);
	PrintThreadPoolStats(&gRecvThreadPool, __FILE__, __LINE__,
		"Recv Thread Pool");
#ifdef INCLUDE_CLIENT_APIS
	ithread_mutex_destroy(&GlobalClientSubscribeMutex);
#endif
	ithread_rwlock_destroy(&GlobalHndRWLock);
	ithread_mutex_destroy(&gUUIDMutex);
	/* remove all virtual dirs */
	UpnpRemoveAllVirtualDirs();
	/* Clean-up ithread library resources */
	ithread_cleanup_library();
	UpnpSdkInit = 0;
	UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__,
		"Exiting UpnpFinish: UpnpSdkInit is :%d:\n", UpnpSdkInit);
	UpnpCloseLog();

	return UPNP_E_SUCCESS;
}

unsigned short UpnpGetServerPort(void)
{
	if (UpnpSdkInit != 1)
		return 0;

	return LOCAL_PORT_V4;
}

#ifdef UPNP_ENABLE_IPV6
unsigned short UpnpGetServerPort6(void)
{
	if (UpnpSdkInit != 1)
		return 0;

	return LOCAL_PORT_V6;
}
#endif

char *UpnpGetServerIpAddress(void)
{
	if (UpnpSdkInit != 1)
		return NULL;

	return gIF_IPV4;
}

char *UpnpGetServerIp6Address(void)
{
	if (UpnpSdkInit != 1)
		return NULL;

	return gIF_IPV6;
}

char *UpnpGetServerUlaGuaIp6Address(void)
{
	if (UpnpSdkInit != 1)
		return NULL;

	return gIF_IPV6_ULA_GUA;
}

/*!
 * \brief Get a free handle.
 *
 * \return On success, an integer greater than zero or UPNP_E_OUTOF_HANDLE on
 * 	failure.
 */
static int GetFreeHandle()
{
	/* Handle 0 is not used as NULL translates to 0 when passed as a handle */
	int i = 1;

	while (i < NUM_HANDLE && HandleTable[i] != NULL)
		++i;
	if (i == NUM_HANDLE)
		return UPNP_E_OUTOF_HANDLE;
	else
		return i;
}

/*!
 * \brief Free handle.
 *
 * \return UPNP_E_SUCCESS if successful or UPNP_E_INVALID_HANDLE if not
 */
static int FreeHandle(
	/*! [in] Handle index. */
	int Upnp_Handle)
{
	int ret = UPNP_E_INVALID_HANDLE;

	UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__,
		"FreeHandle: entering, Handle is %d\n", Upnp_Handle);
	if (Upnp_Handle < 1 || Upnp_Handle >= NUM_HANDLE) {
		UpnpPrintf(UPNP_CRITICAL, API, __FILE__, __LINE__,
			"FreeHandle: Handle %d is out of range\n",
			Upnp_Handle);
	} else if (HandleTable[Upnp_Handle] == NULL) {
		UpnpPrintf(UPNP_CRITICAL, API, __FILE__, __LINE__,
			"FreeHandle: HandleTable[%d] is NULL\n",
			Upnp_Handle);
	} else {
		free( HandleTable[Upnp_Handle] );
		HandleTable[Upnp_Handle] = NULL;
		ret = UPNP_E_SUCCESS;
	}
	UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
		"FreeHandle: exiting, ret = %d.\n", ret);

	return ret;
}

#ifdef INCLUDE_DEVICE_APIS
int UpnpRegisterRootDevice(
	const char *DescUrl,
	Upnp_FunPtr Fun,
	const void *Cookie,
	UpnpDevice_Handle *Hnd)
{
	struct Handle_Info *HInfo = NULL;
	int retVal = 0;
	int hasServiceTable = 0;

	HandleLock();

	UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
		"Inside UpnpRegisterRootDevice\n");

	if (UpnpSdkInit != 1) {
		retVal = UPNP_E_FINISH;
		goto exit_function;
	}

	if (Hnd == NULL ||
	    Fun == NULL ||
	    DescUrl == NULL ||
	    strlen(DescUrl) == 0) {
		retVal = UPNP_E_INVALID_PARAM;
		goto exit_function;
	}

	if (UpnpSdkDeviceRegisteredV4 == 1) {
		retVal = UPNP_E_ALREADY_REGISTERED;
		goto exit_function;
	}

	*Hnd = GetFreeHandle();
	if (*Hnd == UPNP_E_OUTOF_HANDLE) {
		retVal = UPNP_E_OUTOF_MEMORY;
		goto exit_function;
	}

	HInfo = (struct Handle_Info *)malloc(sizeof (struct Handle_Info));
	if (HInfo == NULL) {
		retVal = UPNP_E_OUTOF_MEMORY;
		goto exit_function;
	}
	HandleTable[*Hnd] = HInfo;

	UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
		"Root device URL is %s\n", DescUrl );

	HInfo->aliasInstalled = 0;
	HInfo->HType = HND_DEVICE;
	strcpy(HInfo->DescURL, DescUrl);
	strcpy(HInfo->LowerDescURL, DescUrl);
	UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
		"Following Root Device URL will be used when answering to legacy CPs %s\n",
		HInfo->LowerDescURL);
	HInfo->Callback = Fun;
	HInfo->Cookie = (void *)Cookie;
	HInfo->MaxAge = DEFAULT_MAXAGE;
	HInfo->DeviceList = NULL;
	HInfo->ServiceList = NULL;
	HInfo->DescDocument = NULL;
#ifdef INCLUDE_CLIENT_APIS
	ListInit(&HInfo->SsdpSearchList, NULL, NULL);
	HInfo->ClientSubList = NULL;
#endif /* INCLUDE_CLIENT_APIS */
	HInfo->MaxSubscriptions = UPNP_INFINITE;
	HInfo->MaxSubscriptionTimeOut = UPNP_INFINITE;
	HInfo->DeviceAf = AF_INET;

	retVal = UpnpDownloadXmlDoc(HInfo->DescURL, &(HInfo->DescDocument));
	if (retVal != UPNP_E_SUCCESS) {
		UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
			"UpnpRegisterRootDevice: error downloading Document: %d\n",
			retVal);
#ifdef INCLUDE_CLIENT_APIS
		ListDestroy(&HInfo->SsdpSearchList, 0);
#endif /* INCLUDE_CLIENT_APIS */
		FreeHandle(*Hnd);
		goto exit_function;
	}
	UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
		"UpnpRegisterRootDevice: Valid Description\n"
		"UpnpRegisterRootDevice: DescURL : %s\n",
		HInfo->DescURL);

	HInfo->DeviceList =
		ixmlDocument_getElementsByTagName(HInfo->DescDocument, "device");
	if (!HInfo->DeviceList) {
#ifdef INCLUDE_CLIENT_APIS
		ListDestroy(&HInfo->SsdpSearchList, 0);
#endif /* INCLUDE_CLIENT_APIS */
		ixmlDocument_free(HInfo->DescDocument);
		FreeHandle(*Hnd);
		UpnpPrintf(UPNP_CRITICAL, API, __FILE__, __LINE__,
			"UpnpRegisterRootDevice: No devices found for RootDevice\n");
		retVal = UPNP_E_INVALID_DESC;
		goto exit_function;
	}

	HInfo->ServiceList = ixmlDocument_getElementsByTagName(
		HInfo->DescDocument, "serviceList");
	if (!HInfo->ServiceList) {
		UpnpPrintf(UPNP_CRITICAL, API, __FILE__, __LINE__,
			"UpnpRegisterRootDevice: No services found for RootDevice\n");
	}

	/*
	 * GENA SET UP
	 */
	UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
		"UpnpRegisterRootDevice: Gena Check\n");
	hasServiceTable = getServiceTable(
		(IXML_Node *)HInfo->DescDocument,
		&HInfo->ServiceTable,
		HInfo->DescURL);
	if (hasServiceTable) {
		UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
			"UpnpRegisterRootDevice: GENA Service Table\n"
			"Here are the known services:\n");
		printServiceTable( &HInfo->ServiceTable, UPNP_ALL, API );
	} else {
		UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
			"\nUpnpRegisterRootDevice: Empty service table\n");
	}

	UpnpSdkDeviceRegisteredV4 = 1;

	retVal = UPNP_E_SUCCESS;

exit_function:
	UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
		"Exiting RegisterRootDevice, return value == %d\n", retVal);
	HandleUnlock();

	return retVal;
}
#endif /* INCLUDE_DEVICE_APIS */


/*!
 * \brief Fills the sockadr_in with miniserver information.
 */
static int GetDescDocumentAndURL(
	/* [in] pointer to server address structure. */
	Upnp_DescType descriptionType,
	/* [in] . */
	char *description,
	/* [in] . */
	int config_baseURL,
	/* [in] . */
	int AddressFamily,
	/* [out] . */
	IXML_Document **xmlDoc,
	/* [out] . */
	char *descURL);


#ifdef INCLUDE_DEVICE_APIS
int UpnpRegisterRootDevice2(
	Upnp_DescType descriptionType,
	const char *description_const,
	size_t bufferLen,   /* ignored */
	int config_baseURL,
	Upnp_FunPtr Fun,
	const void *Cookie,
	UpnpDevice_Handle *Hnd)
{
	struct Handle_Info *HInfo = NULL;
	int retVal = 0;
	int hasServiceTable = 0;
	char *description = (char *)description_const;

	HandleLock();

	UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
		"Inside UpnpRegisterRootDevice2\n");

	if (UpnpSdkInit != 1) {
		retVal = UPNP_E_FINISH;
		goto exit_function;
	}

	if (Hnd == NULL || Fun == NULL) {
		retVal = UPNP_E_INVALID_PARAM;
		goto exit_function;
	}

	if (UpnpSdkDeviceRegisteredV4 == 1) {
		retVal = UPNP_E_ALREADY_REGISTERED;
		goto exit_function;
	}

	*Hnd = GetFreeHandle();
	if (*Hnd == UPNP_E_OUTOF_HANDLE) {
		retVal = UPNP_E_OUTOF_MEMORY;
		goto exit_function;
	}

	HInfo = (struct Handle_Info *)malloc(sizeof (struct Handle_Info));
	if (HInfo == NULL) {
		retVal = UPNP_E_OUTOF_MEMORY;
		goto exit_function;
	}
	HandleTable[*Hnd] = HInfo;

	/* prevent accidental removal of a non-existent alias */
	HInfo->aliasInstalled = 0;

	retVal = GetDescDocumentAndURL(
		descriptionType, description,
		config_baseURL, AF_INET, 
		&HInfo->DescDocument, HInfo->DescURL);
	if (retVal != UPNP_E_SUCCESS) {
		FreeHandle(*Hnd);
		goto exit_function;
	}

	strcpy(HInfo->LowerDescURL, HInfo->DescURL);
	UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
		"Following Root Device URL will be used when answering to legacy CPs %s\n",
		HInfo->LowerDescURL);
	HInfo->aliasInstalled = config_baseURL != 0;
	HInfo->HType = HND_DEVICE;
	HInfo->Callback = Fun;
	HInfo->Cookie = (void *)Cookie;
	HInfo->MaxAge = DEFAULT_MAXAGE;
	HInfo->DeviceList = NULL;
	HInfo->ServiceList = NULL;
#ifdef INCLUDE_CLIENT_APIS
	ListInit(&HInfo->SsdpSearchList, NULL, NULL);
	HInfo->ClientSubList = NULL;
#endif /* INCLUDE_CLIENT_APIS */
	HInfo->MaxSubscriptions = UPNP_INFINITE;
	HInfo->MaxSubscriptionTimeOut = UPNP_INFINITE;
	HInfo->DeviceAf = AF_INET;

	UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
		"UpnpRegisterRootDevice2: Valid Description\n"
		"UpnpRegisterRootDevice2: DescURL : %s\n",
		HInfo->DescURL);

	HInfo->DeviceList =
		ixmlDocument_getElementsByTagName( HInfo->DescDocument, "device" );
	if (!HInfo->DeviceList) {
#ifdef INCLUDE_CLIENT_APIS
		ListDestroy(&HInfo->SsdpSearchList, 0);
#endif /* INCLUDE_CLIENT_APIS */
		ixmlDocument_free(HInfo->DescDocument);
		FreeHandle(*Hnd);
		UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
			"UpnpRegisterRootDevice2: No devices found for RootDevice\n" );
		retVal = UPNP_E_INVALID_DESC;
		goto exit_function;
	}

	HInfo->ServiceList = ixmlDocument_getElementsByTagName(
		HInfo->DescDocument, "serviceList" );
	if (!HInfo->ServiceList) {
		UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
			"UpnpRegisterRootDevice2: No services found for RootDevice\n");
	}

	/*
	 * GENA SET UP
	 */
	UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
		"UpnpRegisterRootDevice2: Gena Check\n" );
	hasServiceTable = getServiceTable(
		(IXML_Node *)HInfo->DescDocument,
		&HInfo->ServiceTable,
		HInfo->DescURL);
	if (hasServiceTable) {
		UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
			"UpnpRegisterRootDevice2: GENA Service Table\n"
			"Here are the known services: \n");
		printServiceTable(&HInfo->ServiceTable, UPNP_ALL, API);
	} else {
		UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
			"\nUpnpRegisterRootDevice2: Empty service table\n");
	}

	UpnpSdkDeviceRegisteredV4 = 1;

	retVal = UPNP_E_SUCCESS;

exit_function:
	UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
		"Exiting RegisterRootDevice2, return value == %d\n", retVal);
	HandleUnlock();

	return retVal;
	bufferLen = bufferLen;
}
#endif /* INCLUDE_DEVICE_APIS */


#ifdef INCLUDE_DEVICE_APIS
int UpnpRegisterRootDevice3(
	const char *DescUrl,
	Upnp_FunPtr Fun,
	const void *Cookie,
	UpnpDevice_Handle *Hnd,
	const int AddressFamily)
{
	UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
		"Inside UpnpRegisterRootDevice3\n");
	return UpnpRegisterRootDevice4(DescUrl, Fun, Cookie, Hnd,
		AddressFamily, NULL);
}
#endif /* INCLUDE_DEVICE_APIS */


#ifdef INCLUDE_DEVICE_APIS
int UpnpRegisterRootDevice4(
	const char *DescUrl,
	Upnp_FunPtr Fun,
	const void *Cookie,
	UpnpDevice_Handle *Hnd,
	const int AddressFamily,
	const char *LowerDescUrl)
{
	struct Handle_Info *HInfo;
	int retVal = 0;
	int hasServiceTable = 0;
	int handler_index = 0;

	HandleLock();

	UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
		"Inside UpnpRegisterRootDevice4\n");
	if (UpnpSdkInit != 1) {
		retVal = UPNP_E_FINISH;
		goto exit_function;
	}
	if (Hnd == NULL ||
	    Fun == NULL ||
	    DescUrl == NULL ||
	    strlen(DescUrl) == 0 ||
	    (AddressFamily != AF_INET && AddressFamily != AF_INET6)) {
		retVal = UPNP_E_INVALID_PARAM;
		goto exit_function;
	}
	/* Test for already regsitered IPV4. */
	if (AddressFamily == AF_INET && UpnpSdkDeviceRegisteredV4 == 1) {
		retVal = UPNP_E_ALREADY_REGISTERED;
		goto exit_function;
	}
	/* Test for already registered IPV6. IPV6 devices might register on multiple
	 * IPv6 addresses (link local and GUA or ULA), so we must to check the
	 * description URL in the HandleTable. */
	while (handler_index < NUM_HANDLE && HandleTable[handler_index] != NULL) {
		if (strcmp(((struct Handle_Info *)HandleTable[handler_index])->DescURL, DescUrl)) {
			retVal = UPNP_E_ALREADY_REGISTERED;
			goto exit_function;
		}
		handler_index++;
	}
	*Hnd = GetFreeHandle();
	if (*Hnd == UPNP_E_OUTOF_HANDLE) {
		retVal = UPNP_E_OUTOF_MEMORY;
		goto exit_function;
	}
	HInfo = (struct Handle_Info *)malloc(sizeof (struct Handle_Info));
	if (HInfo == NULL) {
		retVal = UPNP_E_OUTOF_MEMORY;
		goto exit_function;
	}
	HandleTable[*Hnd] = HInfo;
	UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
		"Root device URL is %s\n", DescUrl);
	HInfo->aliasInstalled = 0;
	HInfo->HType = HND_DEVICE;
	strcpy(HInfo->DescURL, DescUrl);
	if (LowerDescUrl == NULL)
		strcpy(HInfo->LowerDescURL, DescUrl);
	else
		strcpy(HInfo->LowerDescURL, LowerDescUrl);
	UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
		"Following Root Device URL will be used when answering to legacy CPs %s\n",
		HInfo->LowerDescURL);
	HInfo->Callback = Fun;
	HInfo->Cookie = (void *)Cookie;
	HInfo->MaxAge = DEFAULT_MAXAGE;
	HInfo->DeviceList = NULL;
	HInfo->ServiceList = NULL;
	HInfo->DescDocument = NULL;
#ifdef INCLUDE_CLIENT_APIS
	ListInit(&HInfo->SsdpSearchList, NULL, NULL);
	HInfo->ClientSubList = NULL;
#endif /* INCLUDE_CLIENT_APIS */
	HInfo->MaxSubscriptions = UPNP_INFINITE;
	HInfo->MaxSubscriptionTimeOut = UPNP_INFINITE;
	HInfo->DeviceAf = AddressFamily;
	retVal = UpnpDownloadXmlDoc(HInfo->DescURL, &(HInfo->DescDocument));
	if (retVal != UPNP_E_SUCCESS) {
#ifdef INCLUDE_CLIENT_APIS
		ListDestroy(&HInfo->SsdpSearchList, 0);
#endif /* INCLUDE_CLIENT_APIS */
		FreeHandle(*Hnd);
		goto exit_function;
	}
	UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
		"UpnpRegisterRootDevice4: Valid Description\n"
		"UpnpRegisterRootDevice4: DescURL : %s\n",
		HInfo->DescURL);

	HInfo->DeviceList = ixmlDocument_getElementsByTagName(
		HInfo->DescDocument, "device");
	if (!HInfo->DeviceList) {
#ifdef INCLUDE_CLIENT_APIS
		ListDestroy(&HInfo->SsdpSearchList, 0);
#endif /* INCLUDE_CLIENT_APIS */
		ixmlDocument_free(HInfo->DescDocument);
		FreeHandle(*Hnd);
		UpnpPrintf(UPNP_CRITICAL, API, __FILE__, __LINE__,
			"UpnpRegisterRootDevice4: No devices found for RootDevice\n");
		retVal = UPNP_E_INVALID_DESC;
		goto exit_function;
	}

	HInfo->ServiceList = ixmlDocument_getElementsByTagName(
	HInfo->DescDocument, "serviceList" );
	if (!HInfo->ServiceList) {
		UpnpPrintf(UPNP_CRITICAL, API, __FILE__, __LINE__,
			"UpnpRegisterRootDevice4: No services found for RootDevice\n");
	}

	/*
	 * GENA SET UP
	 */
	UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
		"UpnpRegisterRootDevice4: Gena Check\n" );
	hasServiceTable = getServiceTable(
		(IXML_Node *)HInfo->DescDocument,
		&HInfo->ServiceTable,
		HInfo->DescURL);
	if (hasServiceTable) {
		UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
			"UpnpRegisterRootDevice4: GENA Service Table \n"
			"Here are the known services: \n" );
		printServiceTable(&HInfo->ServiceTable, UPNP_ALL, API);
	} else {
		UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
			"\nUpnpRegisterRootDevice4: Empty service table\n");
	}

	if (AddressFamily == AF_INET) {
		UpnpSdkDeviceRegisteredV4 = 1;
	} else {
		UpnpSdkDeviceregisteredV6 = 1;
	}

	retVal = UPNP_E_SUCCESS;

exit_function:
	UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
		"Exiting RegisterRootDevice4, return value == %d\n", retVal);
	HandleUnlock();

	return retVal;
}
#endif /* INCLUDE_DEVICE_APIS */


#ifdef INCLUDE_DEVICE_APIS
int UpnpUnRegisterRootDevice(UpnpDevice_Handle Hnd)
{
	UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__,
		   "Inside UpnpUnRegisterRootDevice\n");
	return UpnpUnRegisterRootDeviceLowPower(Hnd, -1, -1, -1);
}

int UpnpUnRegisterRootDeviceLowPower(UpnpDevice_Handle Hnd, int PowerState,
	int SleepPeriod, int RegistrationState)
{
	int retVal = 0;
	struct Handle_Info *HInfo = NULL;

	if (UpnpSdkInit != 1)
		return UPNP_E_FINISH;
	UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__,
		   "Inside UpnpUnRegisterRootDeviceLowPower\n");
#if EXCLUDE_GENA == 0
	if (genaUnregisterDevice(Hnd) != UPNP_E_SUCCESS)
		return UPNP_E_INVALID_HANDLE;
#endif

	HandleLock();
	if (GetHandleInfo(Hnd, &HInfo) == UPNP_E_INVALID_HANDLE) {
		HandleUnlock();
		return UPNP_E_INVALID_HANDLE;
	}
	HInfo->PowerState = PowerState;
	if( SleepPeriod < 0 )
		SleepPeriod = -1;
	HInfo->SleepPeriod = SleepPeriod;
	HInfo->RegistrationState = RegistrationState;
	HandleUnlock();

#if EXCLUDE_SSDP == 0
	retVal = AdvertiseAndReply(-1, Hnd, 0, (struct sockaddr *)NULL,
		(char *)NULL, (char *)NULL, (char *)NULL,
		HInfo->MaxAge);
#endif

	HandleLock();
	if (GetHandleInfo(Hnd, &HInfo) == UPNP_E_INVALID_HANDLE) {
		HandleUnlock();
		return UPNP_E_INVALID_HANDLE;
	}
	ixmlNodeList_free(HInfo->DeviceList);
	ixmlNodeList_free(HInfo->ServiceList);
	ixmlDocument_free(HInfo->DescDocument);
#ifdef INCLUDE_CLIENT_APIS
	ListDestroy(&HInfo->SsdpSearchList, 0);
#endif /* INCLUDE_CLIENT_APIS */
#ifdef INTERNAL_WEB_SERVER
	if (HInfo->aliasInstalled)
		web_server_set_alias(NULL, NULL, 0, 0);
#endif /* INTERNAL_WEB_SERVER */
	if (HInfo->DeviceAf == AF_INET)
		UpnpSdkDeviceRegisteredV4 = 0;
	else if (HInfo->DeviceAf == AF_INET6)
		UpnpSdkDeviceregisteredV6 = 0;
	FreeHandle(Hnd);
	HandleUnlock();

	UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__,
		   "Exiting UpnpUnRegisterRootDeviceLowPower\n");

	return retVal;
}
#endif /* INCLUDE_DEVICE_APIS */

#ifdef INCLUDE_CLIENT_APIS
int UpnpRegisterClient(Upnp_FunPtr Fun, const void *Cookie,
	UpnpClient_Handle *Hnd)
{
	struct Handle_Info *HInfo;

	if (UpnpSdkInit != 1)
		return UPNP_E_FINISH;
	UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
		   "Inside UpnpRegisterClient \n");
	if (Fun == NULL || Hnd == NULL)
		return UPNP_E_INVALID_PARAM;

	HandleLock();
	if (UpnpSdkClientRegistered) {
		HandleUnlock();
		return UPNP_E_ALREADY_REGISTERED;
	}
	if ((*Hnd = GetFreeHandle()) == UPNP_E_OUTOF_HANDLE) {
		HandleUnlock();
		return UPNP_E_OUTOF_MEMORY;
	}
	HInfo = (struct Handle_Info *)malloc(sizeof(struct Handle_Info));
	if (HInfo == NULL) {
		HandleUnlock();
		return UPNP_E_OUTOF_MEMORY;
	}
	HInfo->HType = HND_CLIENT;
	HInfo->Callback = Fun;
	HInfo->Cookie = (void *)Cookie;
	HInfo->ClientSubList = NULL;
	ListInit(&HInfo->SsdpSearchList, NULL, NULL);
#ifdef INCLUDE_DEVICE_APIS
	HInfo->MaxAge = 0;
	HInfo->MaxSubscriptions = UPNP_INFINITE;
	HInfo->MaxSubscriptionTimeOut = UPNP_INFINITE;
#endif
	HandleTable[*Hnd] = HInfo;
	UpnpSdkClientRegistered = 1;
	HandleUnlock();

	UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
		   "Exiting UpnpRegisterClient \n");

	return UPNP_E_SUCCESS;
}
#endif /* INCLUDE_CLIENT_APIS */

#ifdef INCLUDE_CLIENT_APIS
int UpnpUnRegisterClient(UpnpClient_Handle Hnd)
{
	struct Handle_Info *HInfo;
	ListNode *node = NULL;
	SsdpSearchArg *searchArg = NULL;

	if (UpnpSdkInit != 1)
		return UPNP_E_FINISH;
	UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
		   "Inside UpnpUnRegisterClient \n");

	HandleLock();
	if (!UpnpSdkClientRegistered) {
		HandleUnlock();
		return UPNP_E_INVALID_HANDLE;
	}
	HandleUnlock();

#if EXCLUDE_GENA == 0
	if (genaUnregisterClient(Hnd) != UPNP_E_SUCCESS)
		return UPNP_E_INVALID_HANDLE;
#endif
	HandleLock();
	if (GetHandleInfo(Hnd, &HInfo) == UPNP_E_INVALID_HANDLE) {
		HandleUnlock();
		return UPNP_E_INVALID_HANDLE;
	}
	/* clean up search list */
	node = ListHead(&HInfo->SsdpSearchList);
	while (node != NULL) {
		searchArg = (SsdpSearchArg *) node->item;
		if (searchArg) {
			free(searchArg->searchTarget);
			free(searchArg);
		}
		ListDelNode(&HInfo->SsdpSearchList, node, 0);
		node = ListHead(&HInfo->SsdpSearchList);
	}
	ListDestroy(&HInfo->SsdpSearchList, 0);
	FreeHandle(Hnd);
	UpnpSdkClientRegistered = 0;
	HandleUnlock();

	UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
		   "Exiting UpnpUnRegisterClient \n");

	return UPNP_E_SUCCESS;
}
#endif /* INCLUDE_CLIENT_APIS */

#ifdef INCLUDE_DEVICE_APIS
#ifdef INTERNAL_WEB_SERVER
/*!
 * \brief Determines alias for given name which is a file name or URL.
 *
 * \return UPNP_E_SUCCESS on success, nonzero on failure.
 */
static int GetNameForAlias(
	/*! [in] Name of the file. */
	char *name,
	/*! [out] Pointer to alias string. */
	char **alias)
{
    char *ext;
    char *al;

    ext = strrchr( name, '.' );
    if( ext == NULL || strcasecmp( ext, ".xml" ) != 0 ) {
        return UPNP_E_EXT_NOT_XML;
    }

    al = strrchr( name, '/' );
    if( al == NULL ) {
        *alias = name;
    } else {
        *alias = al;
    }

    return UPNP_E_SUCCESS;
}

/*!
 * \brief Fill the sockadr with IPv4 miniserver information.
 */
static void get_server_addr(
	/*! [out] pointer to server address structure. */
	struct sockaddr *serverAddr)
{
    struct sockaddr_in* sa4 = (struct sockaddr_in*)serverAddr;

    memset( serverAddr, 0, sizeof(struct sockaddr_storage) );

    sa4->sin_family = AF_INET;
    inet_pton( AF_INET, gIF_IPV4, &sa4->sin_addr );
    sa4->sin_port = htons( LOCAL_PORT_V4 );
}

/*!
 * \brief Fill the sockadr with IPv6 miniserver information.
 */
static void get_server_addr6(
	/*! [out] pointer to server address structure. */
	struct sockaddr *serverAddr)
{
    struct sockaddr_in6* sa6 = (struct sockaddr_in6*)serverAddr;

    memset( serverAddr, 0, sizeof(struct sockaddr_storage) );

    sa6->sin6_family = AF_INET6;
    inet_pton(AF_INET6, gIF_IPV6, &sa6->sin6_addr );
    sa6->sin6_port = htons( LOCAL_PORT_V6 );
}


static int GetDescDocumentAndURL(
	Upnp_DescType descriptionType,
	char *description,
	int config_baseURL,
	int AddressFamily,
	IXML_Document **xmlDoc,
	char *descURL)
{
	int retVal = 0;
	char *membuf = NULL;
	char aliasStr[LINE_SIZE];
	char *temp_str = NULL;
	FILE *fp = NULL;
	size_t fileLen;
	size_t num_read;
	time_t last_modified;
	struct stat file_info;
	struct sockaddr_storage serverAddr;
	int rc = UPNP_E_SUCCESS;

	if (description == NULL)
		return UPNP_E_INVALID_PARAM;
	/* non-URL description must have configuration specified */
	if (descriptionType != UPNPREG_URL_DESC && !config_baseURL)
		return UPNP_E_INVALID_PARAM;
	/* Get XML doc and last modified time */
	if (descriptionType == UPNPREG_URL_DESC) {
		retVal = UpnpDownloadXmlDoc(description, xmlDoc);
		if (retVal != UPNP_E_SUCCESS)
			return retVal;
		last_modified = time(NULL);
	} else if (descriptionType == UPNPREG_FILENAME_DESC) {
		retVal = stat(description, &file_info);
		if (retVal == -1)
			return UPNP_E_FILE_NOT_FOUND;
		fileLen = (size_t)file_info.st_size;
		last_modified = file_info.st_mtime;
		fp = fopen(description, "rb");
		if (fp == NULL)
			return UPNP_E_FILE_NOT_FOUND;
		membuf = (char *)malloc(fileLen + 1);
		if (membuf == NULL) {
			fclose(fp);
			return UPNP_E_OUTOF_MEMORY;
		}
		num_read = fread(membuf, 1, fileLen, fp);
		if (num_read != fileLen) {
			fclose(fp);
			free(membuf);
			return UPNP_E_FILE_READ_ERROR;
		}
		membuf[fileLen] = 0;
		fclose(fp);
		rc = ixmlParseBufferEx(membuf, xmlDoc);
		free(membuf);
	} else if (descriptionType == UPNPREG_BUF_DESC) {
		last_modified = time(NULL);
		rc = ixmlParseBufferEx(description, xmlDoc);
	} else {
		return UPNP_E_INVALID_PARAM;
	}

	if (rc != IXML_SUCCESS && descriptionType != UPNPREG_URL_DESC) {
		if (rc == IXML_INSUFFICIENT_MEMORY)
			return UPNP_E_OUTOF_MEMORY;
		else
			return UPNP_E_INVALID_DESC;
	}
	/* Determine alias */
	if (config_baseURL) {
		if (descriptionType == UPNPREG_BUF_DESC) {
			strcpy(aliasStr, "description.xml");
		} else {
			/* URL or filename */
			retVal = GetNameForAlias(description, &temp_str);
			if (retVal != UPNP_E_SUCCESS) {
				ixmlDocument_free(*xmlDoc);
				return retVal;
			}
			if (strlen(temp_str) > (LINE_SIZE - 1)) {
				ixmlDocument_free(*xmlDoc);
				free(temp_str);
				return UPNP_E_URL_TOO_BIG;
			}
			strcpy(aliasStr, temp_str);
		}
		if (AddressFamily == AF_INET) {
			get_server_addr((struct sockaddr *)&serverAddr);
		} else {
			get_server_addr6((struct sockaddr *)&serverAddr);
		}

		/* config */
		retVal =
		    configure_urlbase(*xmlDoc, (struct sockaddr *)&serverAddr,
				      aliasStr, last_modified, descURL);
		if (retVal != UPNP_E_SUCCESS) {
			ixmlDocument_free(*xmlDoc);
			return retVal;
		}
	} else {
		/* Manual */
		if (strlen(description) > (LINE_SIZE - 1)) {
			ixmlDocument_free(*xmlDoc);
			return UPNP_E_URL_TOO_BIG;
		}
		strcpy(descURL, description);
	}

	assert(*xmlDoc != NULL);

	return UPNP_E_SUCCESS;
}


#else /* INTERNAL_WEB_SERVER */ /* no web server */
static int GetDescDocumentAndURL(
	Upnp_DescType descriptionType,
	char *description,
	int config_baseURL,
	int AddressFamily,
	IXML_Document **xmlDoc,
	char *descURL)
{
	int retVal = 0;

	if (descriptionType != UPNPREG_URL_DESC || config_baseURL) {
		return UPNP_E_NO_WEB_SERVER;
	}

	if (description == NULL) {
		return UPNP_E_INVALID_PARAM;
	}

	if (strlen(description) > (LINE_SIZE - 1)) {
		return UPNP_E_URL_TOO_BIG;
	}
	strcpy(descURL, description);

	retVal = UpnpDownloadXmlDoc(description, xmlDoc);
	if (retVal != UPNP_E_SUCCESS) {
		return retVal;
	}

	return UPNP_E_SUCCESS;
}
#endif /* INTERNAL_WEB_SERVER */
#endif /* INCLUDE_DEVICE_APIS */


/*******************************************************************************
 *
 *                                  SSDP interface
 *
 ******************************************************************************/


#ifdef INCLUDE_DEVICE_APIS
#if EXCLUDE_SSDP == 0
int UpnpSendAdvertisement(UpnpDevice_Handle Hnd, int Exp)
{
    UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
        "Inside UpnpSendAdvertisement \n");
    return UpnpSendAdvertisementLowPower (Hnd, Exp, -1, -1, -1);
}

int UpnpSendAdvertisementLowPower(UpnpDevice_Handle Hnd, int Exp,
    int PowerState, int SleepPeriod, int RegistrationState)
{
    struct Handle_Info *SInfo = NULL;
    int retVal = 0,
     *ptrMx;
    upnp_timeout *adEvent;
    ThreadPoolJob job;

    if( UpnpSdkInit != 1 ) {
        return UPNP_E_FINISH;
    }

    UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
        "Inside UpnpSendAdvertisementLowPower \n" );

    HandleLock();
    if( GetHandleInfo( Hnd, &SInfo ) != HND_DEVICE ) {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    if( Exp < 1 )
        Exp = DEFAULT_MAXAGE;
    SInfo->MaxAge = Exp;
    SInfo->PowerState = PowerState;
    if( SleepPeriod < 0 )
        SleepPeriod = -1;
    SInfo->SleepPeriod = SleepPeriod;
    SInfo->RegistrationState = RegistrationState;
    HandleUnlock();
    retVal = AdvertiseAndReply( 1, Hnd, 0, ( struct sockaddr * )NULL,
                                ( char * )NULL, ( char * )NULL,
                                ( char * )NULL, Exp );

    if( retVal != UPNP_E_SUCCESS )
        return retVal;
    ptrMx = ( int * )malloc( sizeof( int ) );
    if( ptrMx == NULL )
        return UPNP_E_OUTOF_MEMORY;
    adEvent = ( upnp_timeout * ) malloc( sizeof( upnp_timeout ) );

    if( adEvent == NULL ) {
        free( ptrMx );
        return UPNP_E_OUTOF_MEMORY;
    }
    *ptrMx = Exp;
    adEvent->handle = Hnd;
    adEvent->Event = ptrMx;

    HandleLock();
    if( GetHandleInfo( Hnd, &SInfo ) != HND_DEVICE ) {
        HandleUnlock();
        free( adEvent );
        free( ptrMx );
        return UPNP_E_INVALID_HANDLE;
    }
#ifdef SSDP_PACKET_DISTRIBUTE
    TPJobInit( &job, ( start_routine ) AutoAdvertise, adEvent );
    TPJobSetFreeFunction( &job, ( free_routine ) free_upnp_timeout );
    TPJobSetPriority( &job, MED_PRIORITY );
    if( ( retVal = TimerThreadSchedule( &gTimerThread,
                                        ( ( Exp / 2 ) -
                                          ( AUTO_ADVERTISEMENT_TIME ) ),
                                        REL_SEC, &job, SHORT_TERM,
                                        &( adEvent->eventId ) ) )
        != UPNP_E_SUCCESS ) {
        HandleUnlock();
        free( adEvent );
        free( ptrMx );
        return retVal;
    }
#else
    TPJobInit( &job, ( start_routine ) AutoAdvertise, adEvent );
    TPJobSetFreeFunction( &job, ( free_routine ) free_upnp_timeout );
    TPJobSetPriority( &job, MED_PRIORITY );
    if( ( retVal = TimerThreadSchedule( &gTimerThread,
                                        Exp - AUTO_ADVERTISEMENT_TIME,
                                        REL_SEC, &job, SHORT_TERM,
                                        &( adEvent->eventId ) ) )
        != UPNP_E_SUCCESS ) {
        HandleUnlock();
        free( adEvent );
        free( ptrMx );
        return retVal;
    }
#endif

    HandleUnlock();
    UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
        "Exiting UpnpSendAdvertisementLowPower \n" );

    return retVal;

}
#endif /* EXCLUDE_SSDP == 0 */
#endif /* INCLUDE_DEVICE_APIS */


#if EXCLUDE_SSDP == 0
#ifdef INCLUDE_CLIENT_APIS


int UpnpSearchAsync(
	UpnpClient_Handle Hnd,
	int Mx,
	const char *Target_const,
	const void *Cookie_const )
{
    struct Handle_Info *SInfo = NULL;
    char *Target = ( char * )Target_const;

    if( UpnpSdkInit != 1 ) {
        return UPNP_E_FINISH;
    }

    UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
        "Inside UpnpSearchAsync\n" );

    HandleReadLock();
    if( GetHandleInfo( Hnd, &SInfo ) != HND_CLIENT ) {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    if( Mx < 1 )
        Mx = DEFAULT_MX;

    if( Target == NULL ) {
        HandleUnlock();
        return UPNP_E_INVALID_PARAM;
    }

    HandleUnlock();
    SearchByTarget( Mx, Target, ( void * )Cookie_const );

    UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
        "Exiting UpnpSearchAsync \n" );

    return UPNP_E_SUCCESS;

}
#endif /* INCLUDE_CLIENT_APIS */
#endif


/*******************************************************************************
 *
 *                                  GENA interface
 *
 ******************************************************************************/


#if EXCLUDE_GENA == 0
#ifdef INCLUDE_DEVICE_APIS
int UpnpSetMaxSubscriptions(UpnpDevice_Handle Hnd, int MaxSubscriptions)
{
    struct Handle_Info *SInfo = NULL;

    if( UpnpSdkInit != 1 ) {
        return UPNP_E_FINISH;
    }

    UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
        "Inside UpnpSetMaxSubscriptions \n" );

    HandleLock();
    if( ( ( MaxSubscriptions != UPNP_INFINITE )
          && ( MaxSubscriptions < 0 ) )
        || ( GetHandleInfo( Hnd, &SInfo ) != HND_DEVICE ) ) {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    SInfo->MaxSubscriptions = MaxSubscriptions;
    HandleUnlock();

    UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
        "Exiting UpnpSetMaxSubscriptions \n" );

    return UPNP_E_SUCCESS;

}
#endif /* INCLUDE_DEVICE_APIS */


#ifdef INCLUDE_DEVICE_APIS
int UpnpSetMaxSubscriptionTimeOut(UpnpDevice_Handle Hnd, int MaxSubscriptionTimeOut)
{
    struct Handle_Info *SInfo = NULL;

    if( UpnpSdkInit != 1 ) {
        return UPNP_E_FINISH;
    }

    UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
        "Inside UpnpSetMaxSubscriptionTimeOut\n" );

    HandleLock();

    if( ( ( MaxSubscriptionTimeOut != UPNP_INFINITE )
          && ( MaxSubscriptionTimeOut < 0 ) )
        || ( GetHandleInfo( Hnd, &SInfo ) != HND_DEVICE ) ) {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }

    SInfo->MaxSubscriptionTimeOut = MaxSubscriptionTimeOut;
    HandleUnlock();

    UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
        "Exiting UpnpSetMaxSubscriptionTimeOut\n");

    return UPNP_E_SUCCESS;

}
#endif /* INCLUDE_DEVICE_APIS */


#ifdef INCLUDE_CLIENT_APIS
int UpnpSubscribeAsync(
	UpnpClient_Handle Hnd,
	const char *EvtUrl_const,
	int TimeOut,
	Upnp_FunPtr Fun,
	const void *Cookie_const)
{
    struct Handle_Info *SInfo = NULL;
    struct UpnpNonblockParam *Param;
    char *EvtUrl = ( char * )EvtUrl_const;
    ThreadPoolJob job;

    if( UpnpSdkInit != 1 ) {
        return UPNP_E_FINISH;
    }

    UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
        "Inside UpnpSubscribeAsync\n");

    HandleReadLock();
    if( GetHandleInfo( Hnd, &SInfo ) != HND_CLIENT ) {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    if( EvtUrl == NULL ) {
        HandleUnlock();
        return UPNP_E_INVALID_PARAM;
    }
    if( TimeOut != UPNP_INFINITE && TimeOut < 1 ) {
        HandleUnlock();
        return UPNP_E_INVALID_PARAM;
    }
    if( Fun == NULL ) {
        HandleUnlock();
        return UPNP_E_INVALID_PARAM;
    }
    HandleUnlock();

    Param = (struct UpnpNonblockParam *)
        malloc(sizeof (struct UpnpNonblockParam));
    if( Param == NULL ) {
        return UPNP_E_OUTOF_MEMORY;
    }

    Param->FunName = SUBSCRIBE;
    Param->Handle = Hnd;
    strcpy( Param->Url, EvtUrl );
    Param->TimeOut = TimeOut;
    Param->Fun = Fun;
    Param->Cookie = (void *)Cookie_const;

    TPJobInit(&job, (start_routine)UpnpThreadDistribution, Param);
    TPJobSetFreeFunction(&job, (free_routine)free);
    TPJobSetPriority(&job, MED_PRIORITY);
    ThreadPoolAdd(&gSendThreadPool, &job, NULL);

    UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
        "Exiting UpnpSubscribeAsync\n");

    return UPNP_E_SUCCESS;

}
#endif /* INCLUDE_CLIENT_APIS */


#ifdef INCLUDE_CLIENT_APIS
int UpnpSubscribe(
	UpnpClient_Handle Hnd,
	const char *EvtUrl_const,
	int *TimeOut,
	Upnp_SID SubsId)
{
	int retVal;
	struct Handle_Info *SInfo = NULL;
	UpnpString *EvtUrl = UpnpString_new();
	UpnpString *SubsIdTmp = UpnpString_new();
	
	UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__, "Inside UpnpSubscribe\n");

	if (UpnpSdkInit != 1) {
		retVal = UPNP_E_FINISH;
		goto exit_function;
	}

	if (EvtUrl == NULL) {
		retVal = UPNP_E_OUTOF_MEMORY;
		goto exit_function;
	}
	if (EvtUrl_const == NULL) {
		retVal = UPNP_E_INVALID_PARAM;
		goto exit_function;
	}
	UpnpString_set_String(EvtUrl, EvtUrl_const);

	if (SubsIdTmp == NULL) {
		retVal = UPNP_E_OUTOF_MEMORY;
		goto exit_function;
	}
	if (SubsId == NULL) {
		retVal = UPNP_E_INVALID_PARAM;
		goto exit_function;
	}
	UpnpString_set_String(SubsIdTmp, SubsId);

	if (TimeOut == NULL) {
		retVal = UPNP_E_INVALID_PARAM;
		goto exit_function;
	}

	HandleReadLock();
	if (GetHandleInfo(Hnd, &SInfo) != HND_CLIENT) {
		HandleUnlock();
		retVal = UPNP_E_INVALID_HANDLE;
		goto exit_function;
	}
	HandleUnlock();

	retVal = genaSubscribe(Hnd, EvtUrl, TimeOut, SubsIdTmp);
	strcpy(SubsId, UpnpString_get_String(SubsIdTmp));

exit_function:
	UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
		"Exiting UpnpSubscribe, retVal=%d\n", retVal);

	UpnpString_delete(SubsIdTmp);
	UpnpString_delete(EvtUrl);

	return retVal;
}
#endif /* INCLUDE_CLIENT_APIS */


#ifdef INCLUDE_CLIENT_APIS
int UpnpUnSubscribe(UpnpClient_Handle Hnd, const Upnp_SID SubsId)
{
	struct Handle_Info *SInfo = NULL;
	int retVal;
	UpnpString *SubsIdTmp = UpnpString_new();

	UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__, "Inside UpnpUnSubscribe\n");

	if (UpnpSdkInit != 1) {
		retVal = UPNP_E_FINISH;
		goto exit_function;
	}

	if (SubsIdTmp == NULL) {
		retVal = UPNP_E_OUTOF_MEMORY;
		goto exit_function;
	}
	if (SubsId == NULL) {
		HandleUnlock();
		return UPNP_E_INVALID_PARAM;
	}
	UpnpString_set_String(SubsIdTmp, SubsId);

	HandleReadLock();
	if (GetHandleInfo(Hnd, &SInfo) != HND_CLIENT) {
		HandleUnlock();
		retVal = UPNP_E_INVALID_HANDLE;
		goto exit_function;
	}
	HandleUnlock();

	retVal = genaUnSubscribe(Hnd, SubsIdTmp);

exit_function:
	UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
		"Exiting UpnpUnSubscribe, retVal=%d\n", retVal);

	UpnpString_delete(SubsIdTmp);

	return retVal;
}
#endif /* INCLUDE_CLIENT_APIS */


#ifdef INCLUDE_CLIENT_APIS
int UpnpUnSubscribeAsync(
	UpnpClient_Handle Hnd,
	Upnp_SID SubsId,
	Upnp_FunPtr Fun,
	const void *Cookie_const)
{
	int retVal;
	ThreadPoolJob job;
	struct Handle_Info *SInfo = NULL;
	struct UpnpNonblockParam *Param;

	UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__, "Inside UpnpUnSubscribeAsync\n");

	if (UpnpSdkInit != 1) {
		retVal = UPNP_E_FINISH;
		goto exit_function;
	}

	if (SubsId == NULL) {
		retVal = UPNP_E_INVALID_PARAM;
		goto exit_function;
	}
	if (Fun == NULL) {
		retVal = UPNP_E_INVALID_PARAM;
		goto exit_function;
	}

	HandleReadLock();
	if (GetHandleInfo(Hnd, &SInfo) != HND_CLIENT) {
		HandleUnlock();
		retVal = UPNP_E_INVALID_HANDLE;
		goto exit_function;
	}
	HandleUnlock();

	Param = (struct UpnpNonblockParam *)malloc(sizeof(struct UpnpNonblockParam));
	if (Param == NULL) {
		retVal = UPNP_E_OUTOF_MEMORY;
		goto exit_function;
	}

	Param->FunName = UNSUBSCRIBE;
	Param->Handle = Hnd;
	strcpy( Param->SubsId, SubsId );
	Param->Fun = Fun;
	Param->Cookie = (void *)Cookie_const;
	TPJobInit( &job, ( start_routine ) UpnpThreadDistribution, Param );
	TPJobSetFreeFunction( &job, ( free_routine ) free );
	TPJobSetPriority( &job, MED_PRIORITY );
	ThreadPoolAdd( &gSendThreadPool, &job, NULL );

exit_function:
	UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__, "Exiting UpnpUnSubscribeAsync\n");

	return UPNP_E_SUCCESS;
}
#endif /* INCLUDE_CLIENT_APIS */


#ifdef INCLUDE_CLIENT_APIS
int UpnpRenewSubscription(
	UpnpClient_Handle Hnd,
	int *TimeOut,
	const Upnp_SID SubsId)
{
	struct Handle_Info *SInfo = NULL;
	int retVal;
	UpnpString *SubsIdTmp = UpnpString_new();

	UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__, "Inside UpnpRenewSubscription\n");

	if (UpnpSdkInit != 1) {
		return UPNP_E_FINISH;
	}

	if (SubsIdTmp == NULL) {
		retVal = UPNP_E_OUTOF_MEMORY;
		goto exit_function;
	}
	if (SubsId == NULL) {
		retVal = UPNP_E_INVALID_PARAM;
		goto exit_function;
	}
	UpnpString_set_String(SubsIdTmp, SubsId);

	if (TimeOut == NULL) {
		retVal = UPNP_E_INVALID_PARAM;
		goto exit_function;
	}

	HandleReadLock();
	if (GetHandleInfo(Hnd, &SInfo) != HND_CLIENT) {
		HandleUnlock();
		retVal = UPNP_E_INVALID_HANDLE;
		goto exit_function;
	}
	HandleUnlock();

	retVal = genaRenewSubscription(Hnd, SubsIdTmp, TimeOut);

exit_function:
	UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
		"Exiting UpnpRenewSubscription, retVal=%d\n", retVal);

	UpnpString_delete(SubsIdTmp);

	return retVal;
}
#endif /* INCLUDE_CLIENT_APIS */


#ifdef INCLUDE_CLIENT_APIS
int UpnpRenewSubscriptionAsync(
	UpnpClient_Handle Hnd,
	int TimeOut,
	Upnp_SID SubsId,
	Upnp_FunPtr Fun,
	const void *Cookie_const)
{
    ThreadPoolJob job;
    struct Handle_Info *SInfo = NULL;
    struct UpnpNonblockParam *Param;

    if( UpnpSdkInit != 1 ) {
        return UPNP_E_FINISH;
    }

    UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
        "Inside UpnpRenewSubscriptionAsync\n");
    HandleReadLock();
    if( GetHandleInfo( Hnd, &SInfo ) != HND_CLIENT ) {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    if( TimeOut != UPNP_INFINITE && TimeOut < 1 ) {
        HandleUnlock();
        return UPNP_E_INVALID_PARAM;
    }
    if( SubsId == NULL ) {
        HandleUnlock();
        return UPNP_E_INVALID_PARAM;
    }
    if( Fun == NULL ) {
        HandleUnlock();
        return UPNP_E_INVALID_PARAM;
    }
    HandleUnlock();

    Param =
        ( struct UpnpNonblockParam * )
        malloc( sizeof( struct UpnpNonblockParam ) );
    if( Param == NULL ) {
        return UPNP_E_OUTOF_MEMORY;
    }

    Param->FunName = RENEW;
    Param->Handle = Hnd;
    strcpy( Param->SubsId, SubsId );
    Param->Fun = Fun;
    Param->Cookie = ( void * )Cookie_const;
    Param->TimeOut = TimeOut;

    TPJobInit( &job, ( start_routine ) UpnpThreadDistribution, Param );
    TPJobSetFreeFunction( &job, ( free_routine ) free );
    TPJobSetPriority( &job, MED_PRIORITY );
    ThreadPoolAdd( &gSendThreadPool, &job, NULL );

    UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
        "Exiting UpnpRenewSubscriptionAsync\n");

    return UPNP_E_SUCCESS;
}
#endif /* INCLUDE_CLIENT_APIS */


#ifdef INCLUDE_DEVICE_APIS
int UpnpNotify(
	UpnpDevice_Handle Hnd,
	const char *DevID_const,
	const char *ServName_const,
	const char **VarName_const,
	const char **NewVal_const,
	int cVariables)
{
    struct Handle_Info *SInfo = NULL;
    int retVal;
    char *DevID = (char *)DevID_const;
    char *ServName = (char *)ServName_const;
    char **VarName = (char **)VarName_const;
    char **NewVal = (char **)NewVal_const;

    if( UpnpSdkInit != 1 ) {
        return UPNP_E_FINISH;
    }

    UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
        "Inside UpnpNotify\n");

    HandleReadLock();
    if( GetHandleInfo( Hnd, &SInfo ) != HND_DEVICE ) {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    if( DevID == NULL ) {
        HandleUnlock();
        return UPNP_E_INVALID_PARAM;
    }
    if( ServName == NULL ) {
        HandleUnlock();
        return UPNP_E_INVALID_PARAM;
    }
    if( VarName == NULL || NewVal == NULL || cVariables < 0 ) {
        HandleUnlock();
        return UPNP_E_INVALID_PARAM;
    }

    HandleUnlock();
    retVal =
        genaNotifyAll( Hnd, DevID, ServName, VarName, NewVal, cVariables );

    UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
        "Exiting UpnpNotify\n");

    return retVal;
}


int UpnpNotifyExt(
	UpnpDevice_Handle Hnd,
	const char *DevID_const,
	const char *ServName_const,
	IXML_Document *PropSet)
{
    struct Handle_Info *SInfo = NULL;
    int retVal;
    char *DevID = (char *)DevID_const;
    char *ServName = (char *)ServName_const;

    if( UpnpSdkInit != 1 ) {
        return UPNP_E_FINISH;
    }

    UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
        "Inside UpnpNotify \n" );

    HandleReadLock();
    if( GetHandleInfo( Hnd, &SInfo ) != HND_DEVICE ) {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    if( DevID == NULL ) {
        HandleUnlock();
        return UPNP_E_INVALID_PARAM;
    }
    if( ServName == NULL ) {
        HandleUnlock();
        return UPNP_E_INVALID_PARAM;
    }

    HandleUnlock();
    retVal = genaNotifyAllExt( Hnd, DevID, ServName, PropSet );

    UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
        "Exiting UpnpNotify \n" );

    return retVal;
}
#endif /* INCLUDE_DEVICE_APIS */


#ifdef INCLUDE_DEVICE_APIS
int UpnpAcceptSubscription(
	UpnpDevice_Handle Hnd,
	const char *DevID_const,
	const char *ServName_const,
	const char **VarName_const,
	const char **NewVal_const,
	int cVariables,
	const Upnp_SID SubsId)
{
	int ret = 0;
	int line = 0;
	struct Handle_Info *SInfo = NULL;
	char *DevID = (char *)DevID_const;
	char *ServName = (char *)ServName_const;
	char **VarName = (char **)VarName_const;
	char **NewVal = (char **)NewVal_const;

	UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
		"Inside UpnpAcceptSubscription\n");

	if (UpnpSdkInit != 1) {
		line = __LINE__;
		ret = UPNP_E_FINISH;
		goto exit_function;
	}

	HandleReadLock();

	if (GetHandleInfo(Hnd, &SInfo) != HND_DEVICE) {
		HandleUnlock();
		line = __LINE__;
		ret = UPNP_E_INVALID_HANDLE;
		goto exit_function;
	}
	if (DevID == NULL) {
		HandleUnlock();
		line = __LINE__;
		ret = UPNP_E_INVALID_PARAM;
		goto exit_function;
	}
	if (ServName == NULL) {
		HandleUnlock();
		line = __LINE__;
		ret = UPNP_E_INVALID_PARAM;
		goto exit_function;
	}
	if (SubsId == NULL) {
		HandleUnlock();
		line = __LINE__;
		ret = UPNP_E_INVALID_PARAM;
		goto exit_function;
	}
	/* Now accepts an empty state list, so the code below is commented out */
#if 0
	if (VarName == NULL || NewVal == NULL || cVariables < 0) {
		HandleUnlock();
		line = __LINE__;
		ret = UPNP_E_INVALID_PARAM;
		goto exit_function;
	}
#endif

	HandleUnlock();

	line = __LINE__;
	ret = genaInitNotify(
		Hnd, DevID, ServName, VarName, NewVal, cVariables, SubsId);

exit_function:
	UpnpPrintf(UPNP_ALL, API, __FILE__, line,
		"Exiting UpnpAcceptSubscription, ret = %d\n", ret);

	return ret;
}


int UpnpAcceptSubscriptionExt(
	UpnpDevice_Handle Hnd,
	const char *DevID_const,
	const char *ServName_const,
	IXML_Document *PropSet,
	Upnp_SID SubsId)
{
	int ret = 0;
	int line = 0;
	struct Handle_Info *SInfo = NULL;
	char *DevID = (char *)DevID_const;
	char *ServName = (char *)ServName_const;

	UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
		"Inside UpnpAcceptSubscription\n");

	if (UpnpSdkInit != 1) {
		line = __LINE__;
		ret = UPNP_E_FINISH;
		goto exit_function;
	}

	HandleReadLock();

	if (GetHandleInfo(Hnd, &SInfo) != HND_DEVICE) {
		HandleUnlock();
		line = __LINE__;
		ret = UPNP_E_INVALID_HANDLE;
		goto exit_function;
	}
	if (DevID == NULL) {
		HandleUnlock();
		line = __LINE__;
		ret = UPNP_E_INVALID_PARAM;
		goto exit_function;
	}
	if (ServName == NULL) {
		HandleUnlock();
		line = __LINE__;
		ret = UPNP_E_INVALID_PARAM;
		goto exit_function;
	}
	if (SubsId == NULL) {
		HandleUnlock();
		line = __LINE__;
		ret = UPNP_E_INVALID_PARAM;
		goto exit_function;
	}
	/* Now accepts an empty state list, so the code below is commented out */
#if 0
	if (PropSet == NULL) {
		HandleUnlock();
		line = __LINE__;
		ret = UPNP_E_INVALID_PARAM;
		goto exit_function;
	}
#endif

	HandleUnlock();

	line = __LINE__;
	ret = genaInitNotifyExt(Hnd, DevID, ServName, PropSet, SubsId);

exit_function:
	UpnpPrintf(UPNP_ALL, API, __FILE__, line,
		"Exiting UpnpAcceptSubscription, ret = %d.\n", ret);

	return ret;
}
#endif /* INCLUDE_DEVICE_APIS */
#endif /* EXCLUDE_GENA == 0 */


/*******************************************************************************
 *
 *                                  SOAP interface
 *
 ******************************************************************************/


#if EXCLUDE_SOAP == 0
#ifdef INCLUDE_CLIENT_APIS
int UpnpSendAction(
	UpnpClient_Handle Hnd,
	const char *ActionURL_const,
	const char *ServiceType_const,
	const char *DevUDN_const,
	IXML_Document *Action,
	IXML_Document **RespNodePtr)
{
    struct Handle_Info *SInfo = NULL;
    int retVal = 0;
    char *ActionURL = (char *)ActionURL_const;
    char *ServiceType = (char *)ServiceType_const;
    /* udn not used? */
    /*char *DevUDN = (char *)DevUDN_const;*/

    if( UpnpSdkInit != 1 ) {
        return UPNP_E_FINISH;
    }

    UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
        "Inside UpnpSendAction\n");
    if (DevUDN_const !=NULL) {
	UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
		"non NULL DevUDN is ignored\n");
    }
    DevUDN_const = NULL;

    HandleReadLock();
    if( GetHandleInfo( Hnd, &SInfo ) != HND_CLIENT ) {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    HandleUnlock();

    if( ActionURL == NULL ) {
        return UPNP_E_INVALID_PARAM;
    }

    if( ServiceType == NULL || Action == NULL || RespNodePtr == NULL
        || DevUDN_const != NULL ) {

        return UPNP_E_INVALID_PARAM;
    }

    retVal = SoapSendAction( ActionURL, ServiceType, Action, RespNodePtr );

    UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
	"Exiting UpnpSendAction\n");

    return retVal;
}


int UpnpSendActionEx(
	UpnpClient_Handle Hnd,
	const char *ActionURL_const,
	const char *ServiceType_const,
	const char *DevUDN_const,
	IXML_Document *Header,
	IXML_Document *Action,
	IXML_Document **RespNodePtr)
{
    struct Handle_Info *SInfo = NULL;
    int retVal = 0;
    char *ActionURL = (char *)ActionURL_const;
    char *ServiceType = (char *)ServiceType_const;
    /* udn not used? */
    /*char *DevUDN = (char *)DevUDN_const;*/

    if( UpnpSdkInit != 1 ) {
        return UPNP_E_FINISH;
    }

    UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
        "Inside UpnpSendActionEx\n");

    if( Header == NULL ) {
        retVal = UpnpSendAction( Hnd, ActionURL_const, ServiceType_const,
                                 DevUDN_const, Action, RespNodePtr );
        return retVal;
    }

    HandleReadLock();
    if( GetHandleInfo( Hnd, &SInfo ) != HND_CLIENT ) {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    HandleUnlock();

    if( ActionURL == NULL ) {
        return UPNP_E_INVALID_PARAM;
    }
    if( ServiceType == NULL || Action == NULL || RespNodePtr == NULL ) {
        return UPNP_E_INVALID_PARAM;
    }

    retVal = SoapSendActionEx( ActionURL, ServiceType, Header,
                               Action, RespNodePtr );

    UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
        "Exiting UpnpSendAction \n");

    return retVal;
}


int UpnpSendActionAsync(
	UpnpClient_Handle Hnd,
	const char *ActionURL_const,
	const char *ServiceType_const,
	const char *DevUDN_const,
	IXML_Document *Act,
	Upnp_FunPtr Fun,
	const void *Cookie_const)
{
    int rc;
    ThreadPoolJob job;
    struct Handle_Info *SInfo = NULL;
    struct UpnpNonblockParam *Param;
    DOMString tmpStr;
    char *ActionURL = (char *)ActionURL_const;
    char *ServiceType = (char *)ServiceType_const;
    /* udn not used? */
    /*char *DevUDN = (char *)DevUDN_const;*/

    if(UpnpSdkInit != 1) {
        return UPNP_E_FINISH;
    }

    UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
        "Inside UpnpSendActionAsync\n");

    HandleReadLock();
    if( GetHandleInfo( Hnd, &SInfo ) != HND_CLIENT ) {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    HandleUnlock();

    if( ActionURL == NULL ) {
        return UPNP_E_INVALID_PARAM;
    }
    if( ServiceType == NULL ||
        Act == NULL || Fun == NULL || DevUDN_const != NULL ) {
        return UPNP_E_INVALID_PARAM;
    }
    tmpStr = ixmlPrintNode( ( IXML_Node * ) Act );
    if( tmpStr == NULL ) {
        return UPNP_E_INVALID_ACTION;
    }

    Param =
        ( struct UpnpNonblockParam * )
        malloc( sizeof( struct UpnpNonblockParam ) );

    if( Param == NULL ) {
        return UPNP_E_OUTOF_MEMORY;
    }

    Param->FunName = ACTION;
    Param->Handle = Hnd;
    strcpy( Param->Url, ActionURL );
    strcpy( Param->ServiceType, ServiceType );

    rc = ixmlParseBufferEx( tmpStr, &( Param->Act ) );
    if( rc != IXML_SUCCESS ) {
        free( Param );
        ixmlFreeDOMString( tmpStr );
        if( rc == IXML_INSUFFICIENT_MEMORY ) {
            return UPNP_E_OUTOF_MEMORY;
        } else {
            return UPNP_E_INVALID_ACTION;
        }
    }
    ixmlFreeDOMString( tmpStr );
    Param->Cookie = ( void * )Cookie_const;
    Param->Fun = Fun;

    TPJobInit( &job, ( start_routine ) UpnpThreadDistribution, Param );
    TPJobSetFreeFunction( &job, ( free_routine ) free );

    TPJobSetPriority( &job, MED_PRIORITY );
    ThreadPoolAdd( &gSendThreadPool, &job, NULL );

    UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
        "Exiting UpnpSendActionAsync \n");

    return UPNP_E_SUCCESS;
}


int UpnpSendActionExAsync(
	UpnpClient_Handle Hnd,
	const char *ActionURL_const,
	const char *ServiceType_const,
	const char *DevUDN_const,
	IXML_Document *Header,
	IXML_Document *Act,
	Upnp_FunPtr Fun,
	const void *Cookie_const)
{
    struct Handle_Info *SInfo = NULL;
    struct UpnpNonblockParam *Param;
    DOMString tmpStr;
    DOMString headerStr = NULL;
    char *ActionURL = ( char * )ActionURL_const;
    char *ServiceType = ( char * )ServiceType_const;
    ThreadPoolJob job;
    int retVal = 0;

    if( UpnpSdkInit != 1 ) {
        return UPNP_E_FINISH;
    }

    UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
        "Inside UpnpSendActionExAsync\n");

    if( Header == NULL ) {
        retVal = UpnpSendActionAsync( Hnd, ActionURL_const,
                                      ServiceType_const, DevUDN_const, Act,
                                      Fun, Cookie_const );
        return retVal;
    }

    HandleReadLock();
    if( GetHandleInfo( Hnd, &SInfo ) != HND_CLIENT ) {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    HandleUnlock();

    if( ActionURL == NULL ) {
        return UPNP_E_INVALID_PARAM;
    }
    if( ServiceType == NULL || Act == NULL || Fun == NULL ) {
        return UPNP_E_INVALID_PARAM;
    }

    headerStr = ixmlPrintNode( ( IXML_Node * ) Header );

    tmpStr = ixmlPrintNode( ( IXML_Node * ) Act );
    if( tmpStr == NULL ) {
        return UPNP_E_INVALID_ACTION;
    }

    Param =
        ( struct UpnpNonblockParam * )
        malloc( sizeof( struct UpnpNonblockParam ) );
    if( Param == NULL ) {
        return UPNP_E_OUTOF_MEMORY;
    }

    Param->FunName = ACTION;
    Param->Handle = Hnd;
    strcpy( Param->Url, ActionURL );
    strcpy( Param->ServiceType, ServiceType );
    retVal = ixmlParseBufferEx( headerStr, &( Param->Header ) );
    if( retVal != IXML_SUCCESS ) {
        ixmlFreeDOMString( tmpStr );
        ixmlFreeDOMString( headerStr );
        if( retVal == IXML_INSUFFICIENT_MEMORY ) {
            return UPNP_E_OUTOF_MEMORY;
        } else {
            return UPNP_E_INVALID_ACTION;
        }
    }

    retVal = ixmlParseBufferEx( tmpStr, &( Param->Act ) );
    if( retVal != IXML_SUCCESS ) {
        ixmlFreeDOMString( tmpStr );
        ixmlFreeDOMString( headerStr );
        ixmlDocument_free( Param->Header );
        if( retVal == IXML_INSUFFICIENT_MEMORY ) {
            return UPNP_E_OUTOF_MEMORY;
        } else {
            return UPNP_E_INVALID_ACTION;
        }

    }

    ixmlFreeDOMString( tmpStr );
    ixmlFreeDOMString( headerStr );

    Param->Cookie = ( void * )Cookie_const;
    Param->Fun = Fun;

    TPJobInit( &job, ( start_routine ) UpnpThreadDistribution, Param );
    TPJobSetFreeFunction( &job, ( free_routine ) free );

    TPJobSetPriority( &job, MED_PRIORITY );
    ThreadPoolAdd( &gSendThreadPool, &job, NULL );

    UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
        "Exiting UpnpSendActionAsync\n");

    return UPNP_E_SUCCESS;
}


int UpnpGetServiceVarStatusAsync(
	UpnpClient_Handle Hnd,
	const char *ActionURL_const,
	const char *VarName_const,
	Upnp_FunPtr Fun,
	const void *Cookie_const)
{
    ThreadPoolJob job;
    struct Handle_Info *SInfo = NULL;
    struct UpnpNonblockParam *Param;
    char *ActionURL = (char *)ActionURL_const;
    char *VarName = (char *)VarName_const;

    if( UpnpSdkInit != 1 ) {
        return UPNP_E_FINISH;
    }

    UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
        "Inside UpnpGetServiceVarStatusAsync\n");

    HandleReadLock();
    if( GetHandleInfo( Hnd, &SInfo ) != HND_CLIENT ) {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    HandleUnlock();

    if( ActionURL == NULL ) {
        return UPNP_E_INVALID_PARAM;
    }
    if( VarName == NULL || Fun == NULL )
        return UPNP_E_INVALID_PARAM;

    Param =
        ( struct UpnpNonblockParam * )
        malloc( sizeof( struct UpnpNonblockParam ) );
    if( Param == NULL ) {
        return UPNP_E_OUTOF_MEMORY;
    }

    Param->FunName = STATUS;
    Param->Handle = Hnd;
    strcpy( Param->Url, ActionURL );
    strcpy( Param->VarName, VarName );
    Param->Fun = Fun;
    Param->Cookie = ( void * )Cookie_const;

    TPJobInit( &job, ( start_routine ) UpnpThreadDistribution, Param );
    TPJobSetFreeFunction( &job, ( free_routine ) free );

    TPJobSetPriority( &job, MED_PRIORITY );

    ThreadPoolAdd( &gSendThreadPool, &job, NULL );

    UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
        "Exiting UpnpGetServiceVarStatusAsync\n");

    return UPNP_E_SUCCESS;
}


int UpnpGetServiceVarStatus(
	UpnpClient_Handle Hnd,
	const char *ActionURL_const,
	const char *VarName_const,
	DOMString *StVar)
{
    struct Handle_Info *SInfo = NULL;
    int retVal = 0;
    char *StVarPtr;
    char *ActionURL = (char *)ActionURL_const;
    char *VarName = (char *)VarName_const;

    if(UpnpSdkInit != 1) {
        return UPNP_E_FINISH;
    }

    UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
        "Inside UpnpGetServiceVarStatus\n");

    HandleReadLock();
    if( GetHandleInfo( Hnd, &SInfo ) != HND_CLIENT ) {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }

    HandleUnlock();

    if( ActionURL == NULL ) {
        return UPNP_E_INVALID_PARAM;
    }
    if( VarName == NULL || StVar == NULL ) {
        return UPNP_E_INVALID_PARAM;
    }

    retVal = SoapGetServiceVarStatus( ActionURL, VarName, &StVarPtr );
    *StVar = StVarPtr;

    UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
        "Exiting UpnpGetServiceVarStatus \n");

    return retVal;
}
#endif /* INCLUDE_CLIENT_APIS */
#endif /* EXCLUDE_SOAP */


/*******************************************************************************
 *
 *                             Client API
 *
 ******************************************************************************/


int UpnpOpenHttpPost(
	const char *url,
	void **handle,
	const char *contentType,
	int contentLength,
	int timeout)
{
	return http_OpenHttpPost(
		url, handle, contentType, contentLength, timeout);
}


int UpnpWriteHttpPost(
	void *handle,
	char *buf,
	size_t *size,
	int timeout)
{
	return http_WriteHttpPost(handle, buf, size, timeout);
}


int UpnpCloseHttpPost(
	void *handle,
	int *httpStatus,
	int timeout)
{
	return http_CloseHttpPost(handle, httpStatus, timeout);
}


int UpnpOpenHttpGet(
	const char *url_str,
	void **Handle,
	char **contentType,
	int *contentLength,
	int *httpStatus,
	int timeout)
{
	return http_OpenHttpGet(
		url_str, Handle, contentType, contentLength, httpStatus, timeout);
}


int UpnpOpenHttpGetProxy(
	const char *url_str,
	const char *proxy_str,
	void **Handle,
	char **contentType,
	int *contentLength,
	int *httpStatus,
	int timeout)
{
	return http_OpenHttpGetProxy(
		url_str, proxy_str, Handle, contentType, contentLength,
		httpStatus, timeout);
}


int UpnpOpenHttpGetEx(
	const char *url_str,
	void **Handle,
	char **contentType,
	int *contentLength,
	int *httpStatus,
	int lowRange,
	int highRange,
	int timeout)
{
	return http_OpenHttpGetEx(
		url_str, Handle, contentType, contentLength, httpStatus,
		lowRange, highRange, timeout);
}


int UpnpCancelHttpGet(void *Handle)
{
	return http_CancelHttpGet(Handle);
}


int UpnpCloseHttpGet(void *Handle)
{
	return http_CloseHttpGet(Handle);
}


int UpnpReadHttpGet(void *Handle, char *buf, size_t *size, int timeout)
{
	return http_ReadHttpGet(Handle, buf, size, timeout);
}


int UpnpHttpGetProgress(void *Handle, size_t *length, size_t *total)
{
	return http_HttpGetProgress(Handle, length, total);
}


int UpnpDownloadUrlItem(const char *url, char **outBuf, char *contentType)
{
	int ret_code;
	size_t dummy;

	if (url == NULL || outBuf == NULL || contentType == NULL)
		return UPNP_E_INVALID_PARAM;
	ret_code = http_Download(url, HTTP_DEFAULT_TIMEOUT, outBuf, &dummy,
				 contentType);
	if (ret_code > 0)
		/* error reply was received */
		ret_code = UPNP_E_INVALID_URL;

	return ret_code;
}


int UpnpDownloadXmlDoc(const char *url, IXML_Document **xmlDoc)
{
	int ret_code;
	char *xml_buf;
	char content_type[LINE_SIZE];

	if (url == NULL || xmlDoc == NULL) {
		return UPNP_E_INVALID_PARAM;
	}

	ret_code = UpnpDownloadUrlItem(url, &xml_buf, content_type);
	if (ret_code != UPNP_E_SUCCESS) {
		UpnpPrintf(UPNP_CRITICAL, API, __FILE__, __LINE__,
			"Error downloading document, retCode: %d\n", ret_code);
		return ret_code;
	}

	if (strncasecmp(content_type, "text/xml", strlen("text/xml"))) {
		UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__, "Not text/xml\n");
		/* Linksys WRT54G router returns
		 * "CONTENT-TYPE: application/octet-stream".
		 * Let's be nice to Linksys and try to parse document anyway.
		 * If the data sended is not a xml file, ixmlParseBufferEx
		 * will fail and the function will return UPNP_E_INVALID_DESC too. */
#if 0
		free(xml_buf);
		return UPNP_E_INVALID_DESC;
#endif
	}

	ret_code = ixmlParseBufferEx(xml_buf, xmlDoc);
	free(xml_buf);
	if (ret_code != IXML_SUCCESS) {
		if (ret_code == IXML_INSUFFICIENT_MEMORY) {
			UpnpPrintf(UPNP_CRITICAL, API, __FILE__, __LINE__,
				"Out of memory, ixml error code: %d\n",
				ret_code);
			return UPNP_E_OUTOF_MEMORY;
		} else {
			UpnpPrintf(UPNP_CRITICAL, API, __FILE__, __LINE__,
				"Invalid Description, ixml error code: %d\n",
				ret_code);
			return UPNP_E_INVALID_DESC;
		}
	} else {
#ifdef DEBUG
		xml_buf = ixmlPrintNode((IXML_Node *)*xmlDoc);
		UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
			"Printing the Parsed xml document \n %s\n", xml_buf);
		UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
			"****************** END OF Parsed XML Doc *****************\n");
		ixmlFreeDOMString(xml_buf);
#endif
		UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
			"Exiting UpnpDownloadXmlDoc\n");

		return UPNP_E_SUCCESS;
	}
}


int UpnpGetIfInfo(const char *IfName)
{
#ifdef WIN32
	/* ---------------------------------------------------- */
	/* WIN32 implementation will use the IpHlpAPI library. */
	/* ---------------------------------------------------- */
	PIP_ADAPTER_ADDRESSES adapts = NULL;
	PIP_ADAPTER_ADDRESSES adapts_item;
	PIP_ADAPTER_UNICAST_ADDRESS uni_addr;
	SOCKADDR *ip_addr;
	struct in_addr v4_addr;
	struct in6_addr v6_addr;
	ULONG adapts_sz = 0;
	ULONG ret;
	int ifname_found = 0;
	int valid_addr_found = 0;

	/* Get Adapters addresses required size. */
	ret = GetAdaptersAddresses(AF_UNSPEC,
				   GAA_FLAG_SKIP_ANYCAST |
				   GAA_FLAG_SKIP_DNS_SERVER, NULL, adapts,
				   &adapts_sz);
	if (ret != ERROR_BUFFER_OVERFLOW) {
		UpnpPrintf(UPNP_CRITICAL, API, __FILE__, __LINE__,
			   "GetAdaptersAddresses failed to find list of adapters\n");
		return UPNP_E_INIT;
	}
	/* Allocate enough memory. */
	adapts = (PIP_ADAPTER_ADDRESSES) malloc(adapts_sz);
	if (adapts == NULL) {
		return UPNP_E_OUTOF_MEMORY;
	}
	/* Do the call that will actually return the info. */
	ret = GetAdaptersAddresses(AF_UNSPEC,
				   GAA_FLAG_SKIP_ANYCAST |
				   GAA_FLAG_SKIP_DNS_SERVER, NULL, adapts,
				   &adapts_sz);
	if (ret != ERROR_SUCCESS) {
		free(adapts);
		UpnpPrintf(UPNP_CRITICAL, API, __FILE__, __LINE__,
			   "GetAdaptersAddresses failed to find list of adapters\n");
		return UPNP_E_INIT;
	}
	/* Copy interface name, if it was provided. */
	if (IfName != NULL) {
		if (strlen(IfName) > sizeof(gIF_NAME))
			return UPNP_E_INVALID_INTERFACE;

		strncpy(gIF_NAME, IfName, sizeof(gIF_NAME));
		ifname_found = 1;
	}
	adapts_item = adapts;
	while (adapts_item != NULL) {
		if (adapts_item->Flags & IP_ADAPTER_NO_MULTICAST) {
			continue;
		}
		if (ifname_found == 0) {
			/* We have found a valid interface name. Keep it. */
#ifdef UPNP_USE_MSVCPP
			/*
			 * Partial fix for VC - friendly name is wchar string,
			 * but currently gIF_NAME is char string. For now try
			 * to convert it, which will work with many (but not
			 * all) adapters. A full fix would require a lot of
			 * big changes (gIF_NAME to wchar string?).
			 */
			wcstombs(gIF_NAME, adapts_item->FriendlyName,
				sizeof(gIF_NAME));
#else
			strncpy(gIF_NAME, adapts_item->FriendlyName,
				sizeof(gIF_NAME));
#endif
			ifname_found = 1;
		} else {
#ifdef UPNP_USE_MSVCPP
			/*
			 * Partial fix for VC - friendly name is wchar string,
			 * but currently gIF_NAME is char string. For now try
			 * to convert it, which will work with many (but not
			 * all) adapters. A full fix would require a lot of
			 * big changes (gIF_NAME to wchar string?).
			 */
			char tmpIfName[LINE_SIZE] = { 0 };
			wcstombs(tmpIfName, adapts_item->FriendlyName,
				sizeof(tmpIfName));
			if (strncmp
			    (gIF_NAME, tmpIfName,
			     sizeof(gIF_NAME)) != 0) {
				/* This is not the interface we're looking for. */
				continue;
			}
#else
			if (strncmp
			    (gIF_NAME, adapts_item->FriendlyName,
			     sizeof(gIF_NAME)) != 0) {
				/* This is not the interface we're looking for. */
				continue;
			}
#endif
		}
		/* Loop thru this adapter's unicast IP addresses. */
		uni_addr = adapts_item->FirstUnicastAddress;
		while (uni_addr) {
			ip_addr = uni_addr->Address.lpSockaddr;
			switch (ip_addr->sa_family) {
			case AF_INET:
				memcpy(&v4_addr,
				       &((struct sockaddr_in *)ip_addr)->
				       sin_addr, sizeof(v4_addr));
				valid_addr_found = 1;
				break;
			case AF_INET6:
				/* Only keep IPv6 link-local addresses. */
				if (IN6_IS_ADDR_LINKLOCAL
				    (&((struct sockaddr_in6 *)ip_addr)->
				     sin6_addr)) {
					memcpy(&v6_addr,
					       &((struct sockaddr_in6 *)
						 ip_addr)->sin6_addr,
					       sizeof(v6_addr));
					valid_addr_found = 1;
				}
				break;
			default:
				if (valid_addr_found == 0) {
					/* Address is not IPv4 or IPv6 and no valid address has  */
					/* yet been found for this interface. Discard interface name. */
					ifname_found = 0;
				}
				break;
			}
			/* Next address. */
			uni_addr = uni_addr->Next;
		}
		if (valid_addr_found == 1) {
			gIF_INDEX = adapts_item->IfIndex;
			break;
		}
		/* Next adapter. */
		adapts_item = adapts_item->Next;
	}
	/* Failed to find a valid interface, or valid address. */
	if (ifname_found == 0 || valid_addr_found == 0) {
		UpnpPrintf(UPNP_CRITICAL, API, __FILE__, __LINE__,
			   "Failed to find an adapter with valid IP addresses for use.\n");
		return UPNP_E_INVALID_INTERFACE;
	}
	inet_ntop(AF_INET, &v4_addr, gIF_IPV4, sizeof(gIF_IPV4));
	inet_ntop(AF_INET6, &v6_addr, gIF_IPV6, sizeof(gIF_IPV6));
#elif (defined(BSD) && BSD >= 199306) || defined(__FreeBSD_kernel__)
	struct ifaddrs *ifap, *ifa;
	struct in_addr v4_addr;
	struct in6_addr v6_addr;
	int ifname_found = 0;
	int valid_addr_found = 0;

	/* Copy interface name, if it was provided. */
	if (IfName != NULL) {
		if (strlen(IfName) > sizeof(gIF_NAME))
			return UPNP_E_INVALID_INTERFACE;

		strncpy(gIF_NAME, IfName, sizeof(gIF_NAME));
		ifname_found = 1;
	}
	/* Get system interface addresses. */
	if (getifaddrs(&ifap) != 0) {
		UpnpPrintf(UPNP_CRITICAL, API, __FILE__, __LINE__,
			   "getifaddrs failed to find list of addresses\n");
		return UPNP_E_INIT;
	}
	/* cycle through available interfaces and their addresses. */
	for (ifa = ifap; ifa != NULL; ifa = ifa->ifa_next) {
		/* Skip LOOPBACK interfaces, DOWN interfaces and interfaces that  */
		/* don't support MULTICAST. */
		if ((ifa->ifa_flags & IFF_LOOPBACK)
		    || (!(ifa->ifa_flags & IFF_UP))
		    || (!(ifa->ifa_flags & IFF_MULTICAST))) {
			continue;
		}
		if (ifname_found == 0) {
			/* We have found a valid interface name. Keep it. */
			strncpy(gIF_NAME, ifa->ifa_name, sizeof(gIF_NAME));
			ifname_found = 1;
		} else {
			if (strncmp(gIF_NAME, ifa->ifa_name, sizeof(gIF_NAME))
			    != 0) {
				/* This is not the interface we're looking for. */
				continue;
			}
		}
		/* Keep interface addresses for later. */
		switch (ifa->ifa_addr->sa_family) {
		case AF_INET:
			memcpy(&v4_addr,
			       &((struct sockaddr_in *)(ifa->ifa_addr))->
			       sin_addr, sizeof(v4_addr));
			valid_addr_found = 1;
			break;
		case AF_INET6:
			/* Only keep IPv6 link-local addresses. */
			if (IN6_IS_ADDR_LINKLOCAL
			    (&((struct sockaddr_in6 *)(ifa->ifa_addr))->
			     sin6_addr)) {
				memcpy(&v6_addr,
				       &((struct sockaddr_in6 *)(ifa->
								 ifa_addr))->
				       sin6_addr, sizeof(v6_addr));
				valid_addr_found = 1;
			}
			break;
		default:
			if (valid_addr_found == 0) {
				/* Address is not IPv4 or IPv6 and no valid address has  */
				/* yet been found for this interface. Discard interface name. */
				ifname_found = 0;
			}
			break;
		}
	}
	freeifaddrs(ifap);
	/* Failed to find a valid interface, or valid address. */
	if (ifname_found == 0 || valid_addr_found == 0) {
		UpnpPrintf(UPNP_CRITICAL, API, __FILE__, __LINE__,
			   "Failed to find an adapter with valid IP addresses for use.\n");
		return UPNP_E_INVALID_INTERFACE;
	}
	inet_ntop(AF_INET, &v4_addr, gIF_IPV4, sizeof(gIF_IPV4));
	inet_ntop(AF_INET6, &v6_addr, gIF_IPV6, sizeof(gIF_IPV6));
	gIF_INDEX = if_nametoindex(gIF_NAME);
#else
	char szBuffer[MAX_INTERFACES * sizeof(struct ifreq)];
	struct ifconf ifConf;
	struct ifreq ifReq;
	FILE *inet6_procfd;
	size_t i;
	int LocalSock;
	struct in6_addr v6_addr;
	unsigned if_idx;
	char addr6[8][5];
	char buf[65];		/* INET6_ADDRSTRLEN */
	int ifname_found = 0;
	int valid_addr_found = 0;

	/* Copy interface name, if it was provided. */
	if (IfName != NULL) {
		if (strlen(IfName) > sizeof(gIF_NAME))
			return UPNP_E_INVALID_INTERFACE;

		strncpy(gIF_NAME, IfName, sizeof(gIF_NAME));
		ifname_found = 1;
	}
	/* Create an unbound datagram socket to do the SIOCGIFADDR ioctl on.  */
	if ((LocalSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET) {
		UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
			   "Can't create addrlist socket\n");
		return UPNP_E_INIT;
	}
	/* Get the interface configuration information...  */
	ifConf.ifc_len = sizeof szBuffer;
	ifConf.ifc_ifcu.ifcu_buf = (caddr_t) szBuffer;

	if (ioctl(LocalSock, SIOCGIFCONF, &ifConf) < 0) {
		UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
			   "DiscoverInterfaces: SIOCGIFCONF returned error\n");
		return UPNP_E_INIT;
	}
	/* Cycle through the list of interfaces looking for IP addresses.  */
	for (i = 0; i < (size_t)ifConf.ifc_len;) {
		struct ifreq *pifReq =
		    (struct ifreq *)((caddr_t) ifConf.ifc_req + i);
		i += sizeof *pifReq;
		/* See if this is the sort of interface we want to deal with. */
		strcpy(ifReq.ifr_name, pifReq->ifr_name);
		if (ioctl(LocalSock, SIOCGIFFLAGS, &ifReq) < 0) {
			UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
				   "Can't get interface flags for %s:\n",
				   ifReq.ifr_name);
		}
		/* Skip LOOPBACK interfaces, DOWN interfaces and interfaces that  */
		/* don't support MULTICAST. */
		if ((ifReq.ifr_flags & IFF_LOOPBACK)
		    || (!(ifReq.ifr_flags & IFF_UP))
		    || (!(ifReq.ifr_flags & IFF_MULTICAST))) {
			continue;
		}
		if (ifname_found == 0) {
			/* We have found a valid interface name. Keep it. */
			strncpy(gIF_NAME, pifReq->ifr_name, sizeof(gIF_NAME));
			ifname_found = 1;
		} else {
			if (strncmp
			    (gIF_NAME, pifReq->ifr_name,
			     sizeof(gIF_NAME)) != 0) {
				/* This is not the interface we're looking for. */
				continue;
			}
		}
		/* Check address family. */
		if (pifReq->ifr_addr.sa_family == AF_INET) {
			/* Copy interface name, IPv4 address and interface index. */
			strncpy(gIF_NAME, pifReq->ifr_name, sizeof(gIF_NAME));
			inet_ntop(AF_INET,
				  &((struct sockaddr_in *)&pifReq->ifr_addr)->
				  sin_addr, gIF_IPV4, sizeof(gIF_IPV4));
			gIF_INDEX = if_nametoindex(gIF_NAME);
			valid_addr_found = 1;
			break;
		} else {
			/* Address is not IPv4 */
			ifname_found = 0;
		}
	}
	close(LocalSock);
	/* Failed to find a valid interface, or valid address. */
	if (ifname_found == 0 || valid_addr_found == 0) {
		UpnpPrintf(UPNP_CRITICAL, API, __FILE__, __LINE__,
			   "Failed to find an adapter with valid IP addresses for use.\n");

		return UPNP_E_INVALID_INTERFACE;
	}
	/* Try to get the IPv6 address for the same interface  */
	/* from "/proc/net/if_inet6", if possible. */
	inet6_procfd = fopen("/proc/net/if_inet6", "r");
	if (inet6_procfd) {
		while (fscanf(inet6_procfd,
			      "%4s%4s%4s%4s%4s%4s%4s%4s %02x %*02x %*02x %*02x %*20s\n",
			      addr6[0], addr6[1], addr6[2], addr6[3],
			      addr6[4], addr6[5], addr6[6], addr6[7],
			      &if_idx) != EOF) {
			/* Get same interface as IPv4 address retrieved. */
			if (gIF_INDEX == if_idx) {
				snprintf(buf, sizeof(buf),
					 "%s:%s:%s:%s:%s:%s:%s:%s", addr6[0],
					 addr6[1], addr6[2], addr6[3], addr6[4],
					 addr6[5], addr6[6], addr6[7]);
				/* Validate formed address and check for link-local. */
				if (inet_pton(AF_INET6, buf, &v6_addr) > 0) {
					if (IN6_IS_ADDR_ULA(&v6_addr)) {
						/* Got valid IPv6 ula. */
						strncpy(gIF_IPV6_ULA_GUA, buf,
							sizeof
							(gIF_IPV6_ULA_GUA));
					} else if (IN6_IS_ADDR_GLOBAL(&v6_addr)
						   && strlen(gIF_IPV6_ULA_GUA)
						   == 0) {
						/* got a GUA, should store it while no ULA is found */
						strncpy(gIF_IPV6_ULA_GUA, buf,
							sizeof
							(gIF_IPV6_ULA_GUA));
					} else
					    if (IN6_IS_ADDR_LINKLOCAL(&v6_addr)
						&& strlen(gIF_IPV6) == 0) {
						/* got a Link local IPv6 address. */
						strncpy(gIF_IPV6, buf,
							sizeof(gIF_IPV6));
					}
				}
			}
		}
		fclose(inet6_procfd);
	}
#endif
	UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__,
		   "Interface name=%s, index=%d, v4=%s, v6=%s, ULA or GUA v6=%s\n",
		   gIF_NAME, gIF_INDEX, gIF_IPV4, gIF_IPV6, gIF_IPV6_ULA_GUA);

	return UPNP_E_SUCCESS;
}


/*!
 * \brief Schedule async functions in threadpool.
 */
#ifdef INCLUDE_CLIENT_APIS
void UpnpThreadDistribution(struct UpnpNonblockParam *Param)
{
	/*int errCode = 0;*/

	UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
		"Inside UpnpThreadDistribution \n");

	switch (Param->FunName) {
#if EXCLUDE_GENA == 0
	case SUBSCRIBE: {
		struct Upnp_Event_Subscribe Evt;
		/* Cast away constness */
		/*UpnpString *Sid = (UpnpString *)UpnpEventSubscribe_get_SID(evt);*/
		UpnpString *Sid = UpnpString_new();
		UpnpString *Url = UpnpString_new();
		UpnpString_set_String(Url, Param->Url);
		Evt.ErrCode = genaSubscribe(
			Param->Handle,
			Url,
			(int *)&Param->TimeOut,
			Sid);
		strcpy(Evt.PublisherUrl, Param->Url);
		Evt.TimeOut = Param->TimeOut;
		strcpy((char *)Evt.Sid, UpnpString_get_String(Sid));
		Param->Fun(UPNP_EVENT_SUBSCRIBE_COMPLETE, &Evt, Param->Cookie);
		UpnpString_delete(Sid);
		UpnpString_delete(Url);
		free(Param);
		break;
	}
	case UNSUBSCRIBE: {
		struct Upnp_Event_Subscribe Evt;
		UpnpString *Sid = UpnpString_new();
		UpnpString_set_String(Sid, Param->SubsId);
		Evt.ErrCode = genaUnSubscribe(
			Param->Handle,
			Sid);
		strcpy((char *)Evt.Sid, UpnpString_get_String(Sid));
		strcpy(Evt.PublisherUrl, "");
		Evt.TimeOut = 0;
		Param->Fun(UPNP_EVENT_UNSUBSCRIBE_COMPLETE, &Evt, Param->Cookie);
		UpnpString_delete(Sid);
		free(Param);
		break;
	}
	case RENEW: {
		struct Upnp_Event_Subscribe Evt;
		UpnpString *Sid = UpnpString_new();
		UpnpString_set_String(Sid, Param->SubsId);
		Evt.ErrCode = genaRenewSubscription(
			Param->Handle,
			Sid,
			&Param->TimeOut);
		Evt.TimeOut = Param->TimeOut;
		strcpy((char *)Evt.Sid, UpnpString_get_String(Sid));
		Param->Fun(UPNP_EVENT_RENEWAL_COMPLETE, &Evt, Param->Cookie);
		UpnpString_delete(Sid);
		free(Param);
		break;
	}
#endif /* EXCLUDE_GENA == 0 */
#if EXCLUDE_SOAP == 0
	case ACTION: {
		struct Upnp_Action_Complete Evt;
		Evt.ActionResult = NULL;
		Evt.ErrCode = SoapSendAction(
			Param->Url,
			Param->ServiceType,
			Param->Act, &Evt.ActionResult);
		Evt.ActionRequest = Param->Act;
		strcpy(Evt.CtrlUrl, Param->Url);
		Param->Fun(UPNP_CONTROL_ACTION_COMPLETE, &Evt, Param->Cookie);
		ixmlDocument_free(Evt.ActionRequest);
		ixmlDocument_free(Evt.ActionResult);
		free(Param);
		break;
	}
	case STATUS: {
		struct Upnp_State_Var_Complete Evt;
		Evt.ErrCode = SoapGetServiceVarStatus(
			Param->Url,
			Param->VarName,
			&Evt.CurrentVal);
		strcpy(Evt.StateVarName, Param->VarName);
		strcpy(Evt.CtrlUrl, Param->Url);
		Param->Fun(UPNP_CONTROL_GET_VAR_COMPLETE, &Evt, Param->Cookie);
		free(Evt.CurrentVal);
		free(Param);
		break;
	}
#endif /* EXCLUDE_SOAP == 0 */
	default:
		break;
	}

	UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
		"Exiting UpnpThreadDistribution\n");
}
#endif /* INCLUDE_CLIENT_APIS */


/*!
 * \brief Get callback function ptr from a handle.
 *
 * \return Upnp_FunPtr
 */
Upnp_FunPtr GetCallBackFn(UpnpClient_Handle Hnd)
{
	return ((struct Handle_Info *)HandleTable[Hnd])->Callback;
}


/* Assumes at most one client */
Upnp_Handle_Type GetClientHandleInfo(
	UpnpClient_Handle *client_handle_out,
	struct Handle_Info **HndInfo)
{
	Upnp_Handle_Type ret = HND_CLIENT;
	UpnpClient_Handle client;

	if (GetHandleInfo(1, HndInfo) == HND_CLIENT) {
		client = 1;
	} else if (GetHandleInfo(2, HndInfo) == HND_CLIENT) {
		client = 2;
	} else {
		client = -1;
		ret = HND_INVALID;
	}

	*client_handle_out = client;
	return ret;
}


Upnp_Handle_Type GetDeviceHandleInfo(
	const int AddressFamily,
	UpnpDevice_Handle *device_handle_out,
	struct Handle_Info **HndInfo)
{
	/* Check if we've got a registered device of the address family specified. */
	if ((AddressFamily == AF_INET  && UpnpSdkDeviceRegisteredV4 == 0) ||
	    (AddressFamily == AF_INET6 && UpnpSdkDeviceregisteredV6 == 0)) {
		*device_handle_out = -1;
		return HND_INVALID;
	}

	/* Find it. */
	for (*device_handle_out=1; *device_handle_out < NUM_HANDLE; (*device_handle_out)++) {
		if (GetHandleInfo(*device_handle_out, HndInfo) == HND_DEVICE) {
			if ((*HndInfo)->DeviceAf == AddressFamily) {
				return HND_DEVICE;
			}
		}
	}

	*device_handle_out = -1;
	return HND_INVALID;
}


Upnp_Handle_Type GetHandleInfo(
	UpnpClient_Handle Hnd,
	struct Handle_Info **HndInfo)
{
	Upnp_Handle_Type ret = UPNP_E_INVALID_HANDLE;

	UpnpPrintf( UPNP_INFO, API, __FILE__, __LINE__,
		"GetHandleInfo: entering, Handle is %d\n", Hnd);

	if (Hnd < 1 || Hnd >= NUM_HANDLE) {
		UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__,
			"GetHandleInfo: Handle out of range\n");
	} else if (HandleTable[Hnd] == NULL) {
		UpnpPrintf(UPNP_CRITICAL, API, __FILE__, __LINE__,
			"GetHandleInfo: HandleTable[%d] is NULL\n",
			Hnd);
	} else if (HandleTable[Hnd] != NULL) {
		*HndInfo = (struct Handle_Info *)HandleTable[Hnd];
		ret = ((struct Handle_Info *)*HndInfo)->HType;
	}

	UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__, "GetHandleInfo: exiting\n");

	return ret;
}


int PrintHandleInfo(UpnpClient_Handle Hnd)
{
    struct Handle_Info * HndInfo;
    if (HandleTable[Hnd] != NULL) {
        HndInfo = HandleTable[Hnd];
            UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
                "Printing information for Handle_%d\n", Hnd);
            UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
                "HType_%d\n", HndInfo->HType);
#ifdef INCLUDE_DEVICE_APIS
                if(HndInfo->HType != HND_CLIENT)
                    UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
                        "DescURL_%s\n", HndInfo->DescURL);
#endif /* INCLUDE_DEVICE_APIS */
    } else {
        return UPNP_E_INVALID_HANDLE;
    }

    return UPNP_E_SUCCESS;
}


int getlocalhostname(char *out, size_t out_len)
{
	int ret = UPNP_E_SUCCESS;
	char tempstr[16];
	const char *p = NULL;

#ifdef WIN32
	struct hostent *h = NULL;
	struct sockaddr_in LocalAddr;

	gethostname(out, out_len);
	h = gethostbyname(out);
	if (h != NULL) {
		memcpy(&LocalAddr.sin_addr, h->h_addr_list[0], 4);
		p = inet_ntop(AF_INET, &LocalAddr.sin_addr, tempstr, sizeof(tempstr));
		if (p) {
			strncpy(out, p, out_len);
		} else {
			UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
				"getlocalhostname: inet_ntop returned error\n" );
			ret = UPNP_E_INIT;
		}
	} else {
		UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
			"getlocalhostname: gethostbyname returned error\n" );
		ret = UPNP_E_INIT;
	}

#elif (defined(BSD) && BSD >= 199306) || defined(__FreeBSD_kernel__)
	struct ifaddrs *ifap, *ifa;

	if (getifaddrs(&ifap) != 0) {
		UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
			"DiscoverInterfaces: getifaddrs() returned error\n");
		return UPNP_E_INIT;
	}

	/* cycle through available interfaces */
	for (ifa = ifap; ifa != NULL; ifa = ifa->ifa_next) {
		/* Skip loopback, point-to-point and down interfaces, 
		 * except don't skip down interfaces
		 * if we're trying to get a list of configurable interfaces. */
		if ((ifa->ifa_flags & IFF_LOOPBACK) ||
		    (!( ifa->ifa_flags & IFF_UP))) {
			continue;
		}
		if (ifa->ifa_addr->sa_family == AF_INET) {
			/* We don't want the loopback interface. */
			if (((struct sockaddr_in *)(ifa->ifa_addr))->sin_addr.s_addr ==
			    htonl(INADDR_LOOPBACK)) {
				continue;
			}
			p = inet_ntop(AF_INET,
				&((struct sockaddr_in *)(ifa->ifa_addr))->sin_addr,
				tempstr, sizeof(tempstr));
			if (p) {
				strncpy(out, p, out_len);
			} else {
				UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
					"getlocalhostname: inet_ntop returned error\n");
				ret = UPNP_E_INIT;
			}
			UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
				"Inside getlocalhostname: after strncpy %s\n", out);
			break;
		}
	}
	freeifaddrs(ifap);

	ret = ifa ? UPNP_E_SUCCESS : UPNP_E_INIT;
#else
	char szBuffer[MAX_INTERFACES * sizeof (struct ifreq)];
	struct ifconf ifConf;
	struct ifreq ifReq;
	int nResult;
	long unsigned int i;
	int LocalSock;
	struct sockaddr_in LocalAddr;
	int j = 0;

	/* purify */
	memset(&ifConf,  0, sizeof(ifConf));
	memset(&ifReq,   0, sizeof(ifReq));
	memset(szBuffer, 0, sizeof(szBuffer));

	/* Create an unbound datagram socket to do the SIOCGIFADDR ioctl on.  */
	LocalSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (LocalSock == INVALID_SOCKET) {
		UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
			"Can't create addrlist socket\n");
		return UPNP_E_INIT;
	}
	/* Get the interface configuration information... */
	ifConf.ifc_len = sizeof szBuffer;
	ifConf.ifc_ifcu.ifcu_buf = (caddr_t) szBuffer;
	nResult = ioctl(LocalSock, SIOCGIFCONF, &ifConf);
	if (nResult < 0) {
		UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
			"DiscoverInterfaces: SIOCGIFCONF returned error\n");

		return UPNP_E_INIT;
	}

	/* Cycle through the list of interfaces looking for IP addresses. */
	for (i = 0; i < (long unsigned int)ifConf.ifc_len && j < DEFAULT_INTERFACE; ) {
		struct ifreq *pifReq =
			(struct ifreq *)((caddr_t)ifConf.ifc_req + i);
		i += sizeof *pifReq;
		/* See if this is the sort of interface we want to deal with. */
		strcpy(ifReq.ifr_name, pifReq->ifr_name);
		if (ioctl(LocalSock, SIOCGIFFLAGS, &ifReq) < 0) {
			UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
				"Can't get interface flags for %s:\n",
				ifReq.ifr_name);
		}
		/* Skip loopback, point-to-point and down interfaces,
		 * except don't skip down interfaces
		 * if we're trying to get a list of configurable interfaces. */
		if ((ifReq.ifr_flags & IFF_LOOPBACK) ||
		    (!(ifReq.ifr_flags & IFF_UP))) {
			continue;
		}
		if (pifReq->ifr_addr.sa_family == AF_INET) {
			/* Get a pointer to the address...*/
			memcpy(&LocalAddr, &pifReq->ifr_addr,
				sizeof pifReq->ifr_addr);
			/* We don't want the loopback interface. */
			if (LocalAddr.sin_addr.s_addr ==
			    htonl(INADDR_LOOPBACK)) {
				continue;
			}
		}
		/* increment j if we found an address which is not loopback
		 * and is up */
		j++;
	}
	close(LocalSock);

	p = inet_ntop(AF_INET, &LocalAddr.sin_addr, tempstr, sizeof(tempstr));
	if (p) {
		strncpy(out, p, out_len);
	} else {
		UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
			"getlocalhostname: inet_ntop returned error\n" );
		ret = UPNP_E_INIT;
	}
	UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
		"Inside getlocalhostname: after strncpy %s\n", out);
#endif
	return ret;
}


#ifdef INCLUDE_DEVICE_APIS
#if EXCLUDE_SSDP == 0
void AutoAdvertise(void *input)
{
	upnp_timeout *event = (upnp_timeout *)input;

	UpnpSendAdvertisement(event->handle, *((int *)event->Event));
	free_upnp_timeout(event);
}
#endif /* EXCLUDE_SSDP == 0 */
#endif /* INCLUDE_DEVICE_APIS */


#ifdef INTERNAL_WEB_SERVER
int UpnpSetWebServerRootDir(const char *rootDir)
{
    if( UpnpSdkInit == 0 )
        return UPNP_E_FINISH;
    if( ( rootDir == NULL ) || ( strlen( rootDir ) == 0 ) ) {
        return UPNP_E_INVALID_PARAM;
    }

    membuffer_destroy( &gDocumentRootDir );

    return web_server_set_root_dir(rootDir);
}
#endif /* INTERNAL_WEB_SERVER */


int UpnpAddVirtualDir(const char *newDirName)
{
    virtualDirList *pNewVirtualDir;
    virtualDirList *pLast;
    virtualDirList *pCurVirtualDir;
    char dirName[NAME_SIZE];

    if( UpnpSdkInit != 1 ) {
        /* SDK is not initialized */
        return UPNP_E_FINISH;
    }

    if( ( newDirName == NULL ) || ( strlen( newDirName ) == 0 ) ) {
        return UPNP_E_INVALID_PARAM;
    }

    if( *newDirName != '/' ) {
        dirName[0] = '/';
        strcpy( dirName + 1, newDirName );
    } else {
        strcpy( dirName, newDirName );
    }

    pCurVirtualDir = pVirtualDirList;
    while( pCurVirtualDir != NULL ) {
        /* already has this entry */
        if( strcmp( pCurVirtualDir->dirName, dirName ) == 0 ) {
            return UPNP_E_SUCCESS;
        }

        pCurVirtualDir = pCurVirtualDir->next;
    }

    pNewVirtualDir =
        ( virtualDirList * ) malloc( sizeof( virtualDirList ) );
    if( pNewVirtualDir == NULL ) {
        return UPNP_E_OUTOF_MEMORY;
    }
    pNewVirtualDir->next = NULL;
    strcpy( pNewVirtualDir->dirName, dirName );
    *( pNewVirtualDir->dirName + strlen( dirName ) ) = 0;

    if( pVirtualDirList == NULL ) { /* first virtual dir */
        pVirtualDirList = pNewVirtualDir;
    } else {
        pLast = pVirtualDirList;
        while( pLast->next != NULL ) {
            pLast = pLast->next;
        }
        pLast->next = pNewVirtualDir;
    }

    return UPNP_E_SUCCESS;
}


int UpnpRemoveVirtualDir(const char *dirName)
{
    virtualDirList *pPrev;
    virtualDirList *pCur;
    int found = 0;

    if( UpnpSdkInit != 1 ) {
        return UPNP_E_FINISH;
    }

    if( dirName == NULL ) {
        return UPNP_E_INVALID_PARAM;
    }

    if( pVirtualDirList == NULL ) {
        return UPNP_E_INVALID_PARAM;
    }
    /* Handle the special case where the directory that we are */
    /* removing is the first and only one in the list. */
    if( ( pVirtualDirList->next == NULL ) &&
        ( strcmp( pVirtualDirList->dirName, dirName ) == 0 ) ) {
        free( pVirtualDirList );
        pVirtualDirList = NULL;
        return UPNP_E_SUCCESS;
    }

    pCur = pVirtualDirList;
    pPrev = pCur;

    while( pCur != NULL ) {
        if( strcmp( pCur->dirName, dirName ) == 0 ) {
            pPrev->next = pCur->next;
            free( pCur );
            found = 1;
            break;
        } else {
            pPrev = pCur;
            pCur = pCur->next;
        }
    }

    if( found == 1 )
        return UPNP_E_SUCCESS;
    else
        return UPNP_E_INVALID_PARAM;
}


void UpnpRemoveAllVirtualDirs(void)
{
    virtualDirList *pCur;
    virtualDirList *pNext;

    if( UpnpSdkInit != 1 ) {
        return;
    }

    pCur = pVirtualDirList;

    while( pCur != NULL ) {
        pNext = pCur->next;
        free( pCur );

        pCur = pNext;
    }

    pVirtualDirList = NULL;
}


int UpnpEnableWebserver(int enable)
{
    int retVal;

    if( UpnpSdkInit != 1 ) {
        return UPNP_E_FINISH;
    }

    switch ( enable ) {
#ifdef INTERNAL_WEB_SERVER
        case TRUE:
            if( ( retVal = web_server_init() ) != UPNP_E_SUCCESS ) {
                return retVal;
            }
            bWebServerState = WEB_SERVER_ENABLED;
            SetHTTPGetCallback( web_server_callback );
            break;

        case FALSE:
            web_server_destroy();
            bWebServerState = WEB_SERVER_DISABLED;
            SetHTTPGetCallback( NULL );
            break;
#endif /* INTERNAL_WEB_SERVER */
        default:
            return UPNP_E_INVALID_PARAM;
    }

    return UPNP_E_SUCCESS;
}


/*!
 * \brief Checks if the webserver is enabled or disabled. 
 *
 * \return 1, if webserver is enabled or 0, if webserver is disabled.
 */
int UpnpIsWebserverEnabled(void)
{
	if (UpnpSdkInit != 1) {
		return 0;
	}

	return bWebServerState == WEB_SERVER_ENABLED;
}


int UpnpVirtualDir_set_GetInfoCallback(VDCallback_GetInfo callback)
{
	int ret = UPNP_E_SUCCESS;
	if (!callback) {
	        ret = UPNP_E_INVALID_PARAM;
	} else {
		virtualDirCallback.get_info = callback;
	}

	return ret;
}


int UpnpVirtualDir_set_OpenCallback(VDCallback_Open callback)
{
	int ret = UPNP_E_SUCCESS;
	if (!callback) {
	        ret = UPNP_E_INVALID_PARAM;
	} else {
		virtualDirCallback.open = callback;
	}

	return ret;
}


int UpnpVirtualDir_set_ReadCallback(VDCallback_Read callback)
{
	int ret = UPNP_E_SUCCESS;
	if (!callback) {
	        ret = UPNP_E_INVALID_PARAM;
	} else {
		virtualDirCallback.read = callback;
	}

	return ret;
}


int UpnpVirtualDir_set_WriteCallback(VDCallback_Write callback)
{
	int ret = UPNP_E_SUCCESS;
	if (!callback) {
	        ret = UPNP_E_INVALID_PARAM;
	} else {
		virtualDirCallback.write = callback;
	}

	return ret;
}


int UpnpVirtualDir_set_SeekCallback(VDCallback_Seek callback)
{
	int ret = UPNP_E_SUCCESS;
	if (!callback) {
	        ret = UPNP_E_INVALID_PARAM;
	} else {
		virtualDirCallback.seek = callback;
	}

	return ret;
}


int UpnpVirtualDir_set_CloseCallback(VDCallback_Close callback)
{
	int ret = UPNP_E_SUCCESS;
	if (!callback) {
	        ret = UPNP_E_INVALID_PARAM;
	} else {
		virtualDirCallback.close = callback;
	}

	return ret;
}

int UpnpSetContentLength(UpnpClient_Handle Hnd, size_t contentLength)
{
	int errCode = UPNP_E_SUCCESS;
	struct Handle_Info *HInfo = NULL;

	do {
		if (UpnpSdkInit != 1) {
			errCode = UPNP_E_FINISH;
			break;
		}

		HandleLock();

		errCode = GetHandleInfo(Hnd, &HInfo);
		if (errCode != HND_DEVICE) {
			errCode = UPNP_E_INVALID_HANDLE;
			break;
		}
		if (contentLength > MAX_SOAP_CONTENT_LENGTH) {
			errCode = UPNP_E_OUTOF_BOUNDS;
			break;
		}
		g_maxContentLength = contentLength;
	} while (0);

	HandleUnlock();
	return errCode;
}

int UpnpSetMaxContentLength(size_t contentLength)
{
	int errCode = UPNP_E_SUCCESS;

	do {
		if (UpnpSdkInit != 1) {
			errCode = UPNP_E_FINISH;
			break;
		}
		g_maxContentLength = contentLength;
	} while(0);

	return errCode;
}

/* @} UPnPAPI */
