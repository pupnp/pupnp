/*******************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation 
 * All rights reserved. 
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


#include "config.h"


/*!
 * \file
 */


#include <sys/stat.h>


#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>


#ifdef WIN32
	/* Do not include these files */
#else
	#include <arpa/inet.h>
	#include <net/if.h>
	#include <netinet/in.h>
	#include <sys/ioctl.h>
	#include <sys/param.h>
	#include <sys/socket.h>
	#include <sys/types.h>
	#include <sys/utsname.h>


	#include <unistd.h>


	#if defined(_sun)
		#include <sys/sockio.h>
		#include <fcntl.h>
	#elif defined(BSD) && BSD >= 199306
		#include <ifaddrs.h>
	#endif
#endif


#include "upnpapi.h"
#include "httpreadwrite.h"
#include "membuffer.h"
#include "ssdplib.h"
#include "soaplib.h"
#include "ThreadPool.h"
#include "sysdep.h"
#include "uuid.h"


// Needed for GENA
#include "gena.h"
#include "miniserver.h"
#include "service_table.h"


#ifdef INTERNAL_WEB_SERVER
	#include "urlconfig.h"
	#include "VirtualDir.h"
	#include "webserver.h"
#endif // INTERNAL_WEB_SERVER


// This structure is for virtual directory callbacks
struct VirtualDirCallbacks virtualDirCallback;

//
virtualDirList *pVirtualDirList;

// Mutex to synchronize the subscription handling at the client side
#ifdef INCLUDE_CLIENT_APIS
ithread_mutex_t GlobalClientSubscribeMutex;
#endif /* INCLUDE_CLIENT_APIS */

// rwlock to synchronize handles (root device or control point handle)
ithread_rwlock_t GlobalHndRWLock;

// Mutex to synchronize the uuid creation process
ithread_mutex_t gUUIDMutex;

ithread_mutex_t gSDKInitMutex = PTHREAD_MUTEX_INITIALIZER;

TimerThread gTimerThread;

ThreadPool gSendThreadPool;
ThreadPool gRecvThreadPool;
#ifdef INTERNAL_WEB_SERVER
	ThreadPool gMiniServerThreadPool;
#endif

// Flag to indicate the state of web server
WebServerState bWebServerState = WEB_SERVER_DISABLED;

// Static buffer to contain interface name. (extern'ed in upnp.h)
char gIF_NAME[LINE_SIZE] = { '\0' };

// Static buffer to contain interface IPv4 address. (extern'ed in upnp.h)
char gIF_IPV4[22]/* INET_ADDRSTRLEN*/ = { '\0' };

// Static buffer to contain interface IPv6 address. (extern'ed in upnp.h)
char gIF_IPV6[65]/* INET6_ADDRSTRLEN*/ = { '\0' };

// Contains interface index. (extern'ed in upnp.h)
int  gIF_INDEX = -1;

// local IPv4 and IPv6 ports for the mini-server
unsigned short LOCAL_PORT_V4;
unsigned short LOCAL_PORT_V6;

// UPnP device and control point handle table 
void *HandleTable[NUM_HANDLE];

// a local dir which serves as webserver root
extern membuffer gDocumentRootDir;

// Maximum content-length that the SDK will process on an incoming packet. 
// Content-Length exceeding this size will be not processed and error 413 
// (HTTP Error Code) will be returned to the remote end point.
size_t g_maxContentLength = DEFAULT_SOAP_CONTENT_LENGTH; // in bytes

// Global variable to denote the state of Upnp SDK 
//    = 0 if uninitialized, = 1 if initialized.
int UpnpSdkInit = 0;

// Global variable to denote the state of Upnp SDK client registration.
// = 0 if unregistered, = 1 if registered.
int UpnpSdkClientRegistered = 0;

// Global variable to denote the state of Upnp SDK IPv4 device registration.
// = 0 if unregistered, = 1 if registered.
int UpnpSdkDeviceRegisteredV4 = 0;

// Global variable to denote the state of Upnp SDK IPv6 device registration.
// = 0 if unregistered, = 1 if registered.
int UpnpSdkDeviceregisteredV6 = 0;

// Global variable used in discovery notifications.
Upnp_SID gUpnpSdkNLSuuid;


// FIXME Put this declaration in the proper header file
static int GetDescDocumentAndURL(
	IN Upnp_DescType descriptionType,
	IN char *description,
	IN unsigned int bufferLen,
	IN int config_baseURL,
	IN int AddressFamily,
	OUT IXML_Document ** xmlDoc,
	OUT char descURL[LINE_SIZE] );


int UpnpInit(const char *HostIP, unsigned short DestPort)
{
	int retVal;

	ithread_mutex_lock( &gSDKInitMutex );

	// Check if we're already initialized.
	if( UpnpSdkInit == 1 ) {
		ithread_mutex_unlock( &gSDKInitMutex );
		return UPNP_E_INIT;
	}

	// Perform initialization preamble.
	retVal = UpnpInitPreamble();
	if( retVal != UPNP_E_SUCCESS ) {
		ithread_mutex_unlock( &gSDKInitMutex );
		return retVal;
	}

	UpnpPrintf( UPNP_INFO, API, __FILE__, __LINE__,
		"UpnpInit with HostIP=%s, DestPort=%d.\n", 
		HostIP ? HostIP : "", DestPort );

	// Verify HostIP, if provided, or find it ourselves.
	if( HostIP != NULL ) {
		strncpy( gIF_IPV4, HostIP, sizeof(gIF_IPV4) );
	} else {
		if( getlocalhostname( gIF_IPV4, sizeof(gIF_IPV4) ) != UPNP_E_SUCCESS ) {
			return UPNP_E_INIT_FAILED;
		}
	}

	// Set the UpnpSdkInit flag to 1 to indicate we're sucessfully initialized.
	UpnpSdkInit = 1;

	// Finish initializing the SDK.
	retVal = UpnpInitStartServers( DestPort );
	if( retVal != UPNP_E_SUCCESS ) {
		UpnpSdkInit = 0;
		ithread_mutex_unlock( &gSDKInitMutex );
		return retVal;
	}

	UpnpPrintf( UPNP_INFO, API, __FILE__, __LINE__,
		"Host Ip: %s Host Port: %d\n", gIF_IPV4,
		LOCAL_PORT_V4 );

	ithread_mutex_unlock( &gSDKInitMutex );

	return UPNP_E_SUCCESS;
}


int UpnpInit2(const char *IfName, unsigned short DestPort)
{
	int retVal;

	ithread_mutex_lock( &gSDKInitMutex );

	// Check if we're already initialized.
	if( UpnpSdkInit == 1 ) {
		ithread_mutex_unlock( &gSDKInitMutex );
		return UPNP_E_INIT;
	}

	// Perform initialization preamble.
	retVal = UpnpInitPreamble();
	if( retVal != UPNP_E_SUCCESS ) {
		ithread_mutex_unlock( &gSDKInitMutex );
		return retVal;
	}

	UpnpPrintf( UPNP_INFO, API, __FILE__, __LINE__,
		"UpnpInit2 with IfName=%s, DestPort=%d.\n", 
		IfName ? IfName : "", DestPort );

	// Retrieve interface information (Addresses, index, etc).
	retVal = UpnpGetIfInfo( IfName );
	if( retVal != UPNP_E_SUCCESS ) {
		ithread_mutex_unlock( &gSDKInitMutex );
		return retVal;
	}

	// Set the UpnpSdkInit flag to 1 to indicate we're sucessfully initialized.
	UpnpSdkInit = 1;

	// Finish initializing the SDK.
	retVal = UpnpInitStartServers( DestPort );
	if( retVal != UPNP_E_SUCCESS ) {
		UpnpSdkInit = 0;
		ithread_mutex_unlock( &gSDKInitMutex );
		return retVal;
	}

	ithread_mutex_unlock( &gSDKInitMutex );

	return UPNP_E_SUCCESS;
}


int UpnpFinish()
{
#ifdef INCLUDE_DEVICE_APIS
	UpnpDevice_Handle device_handle;
#endif
#ifdef INCLUDE_CLIENT_APIS
	UpnpClient_Handle client_handle;
#endif
	struct Handle_Info *temp;

#ifdef WIN32
	//	WSACleanup();
#endif

	if( UpnpSdkInit != 1 ) {
		return UPNP_E_FINISH;
	}

	UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__,
		"Inside UpnpFinish: UpnpSdkInit is %d\n", UpnpSdkInit);
	if (UpnpSdkInit == 1) {
		UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__,
		"UpnpFinish: UpnpSdkInit is ONE\n");
	}
	PrintThreadPoolStats(&gSendThreadPool, __FILE__, __LINE__, "Send Thread Pool");
	PrintThreadPoolStats(&gRecvThreadPool, __FILE__, __LINE__, "Recv Thread Pool");
#ifdef INTERNAL_WEB_SERVER
	PrintThreadPoolStats(&gMiniServerThreadPool, __FILE__, __LINE__, "MiniServer Thread Pool");
#endif

#ifdef INCLUDE_DEVICE_APIS
	if (GetDeviceHandleInfo(AF_INET, &device_handle, &temp) == HND_DEVICE ) {
		UpnpUnRegisterRootDevice(device_handle);
	}
	if (GetDeviceHandleInfo(AF_INET6, &device_handle, &temp) == HND_DEVICE ) {
		UpnpUnRegisterRootDevice(device_handle);
	}
#endif

#ifdef INCLUDE_CLIENT_APIS
	if (GetClientHandleInfo(&client_handle, &temp) == HND_CLIENT) {
		UpnpUnRegisterClient( client_handle );
	}
#endif

	TimerThreadShutdown(&gTimerThread);
	StopMiniServer();

#if EXCLUDE_WEB_SERVER == 0
	web_server_destroy();
#endif

#ifdef INTERNAL_WEB_SERVER
	ThreadPoolShutdown(&gMiniServerThreadPool);
	PrintThreadPoolStats(&gMiniServerThreadPool, __FILE__, __LINE__, "MiniServer Thread Pool");
#endif
	ThreadPoolShutdown(&gRecvThreadPool);
	PrintThreadPoolStats(&gSendThreadPool, __FILE__, __LINE__, "Send Thread Pool");
	ThreadPoolShutdown(&gSendThreadPool);
	PrintThreadPoolStats(&gRecvThreadPool, __FILE__, __LINE__, "Recv Thread Pool");

#ifdef INCLUDE_CLIENT_APIS
	ithread_mutex_destroy(&GlobalClientSubscribeMutex);
#endif
	ithread_rwlock_destroy(&GlobalHndRWLock);
	ithread_mutex_destroy(&gUUIDMutex);

	// remove all virtual dirs
	UpnpRemoveAllVirtualDirs();

	// allow static linking
#ifdef WIN32
#ifdef PTW32_STATIC_LIB
	pthread_win32_thread_detach_np();
#endif
#endif

	UpnpSdkInit = 0;
	UpnpPrintf( UPNP_INFO, API, __FILE__, __LINE__,
	"Exiting UpnpFinish : UpnpSdkInit is :%d:\n", UpnpSdkInit);
	UpnpCloseLog();

	return UPNP_E_SUCCESS;
}


unsigned short UpnpGetServerPort()
{
	if (UpnpSdkInit != 1) {
		return 0;
	}

	return LOCAL_PORT_V4;
}


unsigned short UpnpGetServerPort6()
{
	if (UpnpSdkInit != 1) {
		return 0;
	}

	return LOCAL_PORT_V6;
}


char *UpnpGetServerIpAddress()
{
	if (UpnpSdkInit != 1) {
		return NULL;
	}

	return gIF_IPV4;
}


char *UpnpGetServerIp6Address()
{
	if( UpnpSdkInit != 1 ) {
		return NULL;
	}

	return gIF_IPV6;
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
	HInfo->Callback = Fun;
	HInfo->Cookie = (void *)Cookie;
	HInfo->MaxAge = DEFAULT_MAXAGE;
	HInfo->DeviceList = NULL;
	HInfo->ServiceList = NULL;
	HInfo->DescDocument = NULL;
	CLIENTONLY( ListInit(&HInfo->SsdpSearchList, NULL, NULL); )
	CLIENTONLY( HInfo->ClientSubList = NULL; )
	HInfo->MaxSubscriptions = UPNP_INFINITE;
	HInfo->MaxSubscriptionTimeOut = UPNP_INFINITE;
	HInfo->DeviceAf = AF_INET;

	retVal = UpnpDownloadXmlDoc(HInfo->DescURL, &(HInfo->DescDocument));
	if (retVal != UPNP_E_SUCCESS) {
		UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
			"UpnpRegisterRootDevice: error downloading Document: %d\n",
			retVal);
		CLIENTONLY( ListDestroy(&HInfo->SsdpSearchList, 0); )
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
		CLIENTONLY( ListDestroy(&HInfo->SsdpSearchList, 0); )
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
			"\nUpnpRegisterRootDevice2: Empty service table\n");
	}

	UpnpSdkDeviceRegisteredV4 = 1;

	retVal = UPNP_E_SUCCESS;

exit_function:
	UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
		"Exiting RegisterRootDevice, return value == %d\n", retVal);
	HandleUnlock();

	return retVal;
}
#endif // INCLUDE_DEVICE_APIS


#ifdef INCLUDE_DEVICE_APIS
int UpnpRegisterRootDevice2(
	Upnp_DescType descriptionType,
	const char *description_const,
	size_t bufferLen,   // ignored unless descType == UPNPREG_BUF_DESC
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

	// prevent accidental removal of a non-existent alias
	HInfo->aliasInstalled = 0;

	retVal = GetDescDocumentAndURL(
		descriptionType, description, bufferLen,
		config_baseURL, AF_INET, 
		&HInfo->DescDocument, HInfo->DescURL);
	if (retVal != UPNP_E_SUCCESS) {
		FreeHandle(*Hnd);
		goto exit_function;
	}

	HInfo->aliasInstalled = config_baseURL != 0;
	HInfo->HType = HND_DEVICE;
	HInfo->Callback = Fun;
	HInfo->Cookie = (void *)Cookie;
	HInfo->MaxAge = DEFAULT_MAXAGE;
	HInfo->DeviceList = NULL;
	HInfo->ServiceList = NULL;
	CLIENTONLY( ListInit(&HInfo->SsdpSearchList, NULL, NULL); )
	CLIENTONLY( HInfo->ClientSubList = NULL; )
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
		CLIENTONLY( ListDestroy(&HInfo->SsdpSearchList, 0); )
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
}
#endif // INCLUDE_DEVICE_APIS


#ifdef INCLUDE_DEVICE_APIS
int UpnpRegisterRootDevice3(
	const char *DescUrl,
	Upnp_FunPtr Fun,
	const void *Cookie,
	UpnpDevice_Handle *Hnd,
	const int AddressFamily)
{
	struct Handle_Info *HInfo;
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
	    strlen(DescUrl) == 0 ||
	    (AddressFamily != AF_INET && AddressFamily != AF_INET6)) {
		retVal = UPNP_E_INVALID_PARAM;
		goto exit_function;
	}

	if ((AddressFamily == AF_INET  && UpnpSdkDeviceRegisteredV4 == 1) ||
	    (AddressFamily == AF_INET6 && UpnpSdkDeviceregisteredV6 == 1)) {
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
		"Root device URL is %s\n", DescUrl);

	HInfo->aliasInstalled = 0;
	HInfo->HType = HND_DEVICE;
	strcpy(HInfo->DescURL, DescUrl);
	HInfo->Callback = Fun;
	HInfo->Cookie = (void *)Cookie;
	HInfo->MaxAge = DEFAULT_MAXAGE;
	HInfo->DeviceList = NULL;
	HInfo->ServiceList = NULL;
	HInfo->DescDocument = NULL;
	CLIENTONLY( ListInit(&HInfo->SsdpSearchList, NULL, NULL); )
	CLIENTONLY( HInfo->ClientSubList = NULL; )
	HInfo->MaxSubscriptions = UPNP_INFINITE;
	HInfo->MaxSubscriptionTimeOut = UPNP_INFINITE;
	HInfo->DeviceAf = AddressFamily;

	retVal = UpnpDownloadXmlDoc(HInfo->DescURL, &(HInfo->DescDocument));
	if (retVal != UPNP_E_SUCCESS) {
		CLIENTONLY( ListDestroy(&HInfo->SsdpSearchList, 0); )
		FreeHandle(*Hnd);
		goto exit_function;
	}
	UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
		"UpnpRegisterRootDevice: Valid Description\n"
		"UpnpRegisterRootDevice: DescURL : %s\n",
		HInfo->DescURL);

	HInfo->DeviceList = ixmlDocument_getElementsByTagName(
		HInfo->DescDocument, "device");
	if (!HInfo->DeviceList) {
		CLIENTONLY( ListDestroy(&HInfo->SsdpSearchList, 0); )
		ixmlDocument_free(HInfo->DescDocument);
		FreeHandle(*Hnd);
		UpnpPrintf(UPNP_CRITICAL, API, __FILE__, __LINE__,
			"UpnpRegisterRootDevice: No devices found for RootDevice\n");
		retVal = UPNP_E_INVALID_DESC;
		goto exit_function;
	}

	HInfo->ServiceList = ixmlDocument_getElementsByTagName(
	HInfo->DescDocument, "serviceList" );
	if (!HInfo->ServiceList) {
		UpnpPrintf(UPNP_CRITICAL, API, __FILE__, __LINE__,
			"UpnpRegisterRootDevice: No services found for RootDevice\n");
	}

	/*
	 * GENA SET UP
	 */
	UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
		"UpnpRegisterRootDevice3: Gena Check\n" );
	hasServiceTable = getServiceTable(
		(IXML_Node *)HInfo->DescDocument,
		&HInfo->ServiceTable,
		HInfo->DescURL);
	if (hasServiceTable) {
		UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
			"UpnpRegisterRootDevice: GENA Service Table \n"
			"Here are the known services: \n" );
		printServiceTable(&HInfo->ServiceTable, UPNP_ALL, API);
	} else {
		UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
			"\nUpnpRegisterRootDevice3: Empty service table\n");
	}

	if (AddressFamily == AF_INET) {
		UpnpSdkDeviceRegisteredV4 = 1;
	} else {
		UpnpSdkDeviceregisteredV6 = 1;
	}

	retVal = UPNP_E_SUCCESS;

exit_function:
	UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
		"Exiting RegisterRootDevice3, return value == %d\n", retVal);
	HandleUnlock();

	return retVal;
}
#endif // INCLUDE_DEVICE_APIS


#ifdef INCLUDE_DEVICE_APIS
int UpnpUnRegisterRootDevice(UpnpDevice_Handle Hnd)
{
    int retVal = 0;
    struct Handle_Info *HInfo = NULL;

    // struct Handle_Info *info=NULL;

    if (UpnpSdkInit != 1) {
        return UPNP_E_FINISH;
    }

    UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__,
        "Inside UpnpUnRegisterRootDevice \n");
#if EXCLUDE_GENA == 0
    if( genaUnregisterDevice( Hnd ) != UPNP_E_SUCCESS )
        return UPNP_E_INVALID_HANDLE;
#endif

    HandleLock();
    if( GetHandleInfo( Hnd, &HInfo ) == UPNP_E_INVALID_HANDLE ) {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    HandleUnlock();

#if EXCLUDE_SSDP == 0
    retVal = AdvertiseAndReply(-1, Hnd, 0, (struct sockaddr *)NULL,
                                (char *)NULL, (char *)NULL,
                                (char *)NULL, HInfo->MaxAge);
#endif

    HandleLock();
    if( GetHandleInfo( Hnd, &HInfo ) == UPNP_E_INVALID_HANDLE ) {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    //info = (struct Handle_Info *) HandleTable[Hnd];
    ixmlNodeList_free( HInfo->DeviceList );
    ixmlNodeList_free( HInfo->ServiceList );
    ixmlDocument_free( HInfo->DescDocument );

    CLIENTONLY( ListDestroy( &HInfo->SsdpSearchList, 0 ); )

#ifdef INTERNAL_WEB_SERVER
    if( HInfo->aliasInstalled ) {
        web_server_set_alias( NULL, NULL, 0, 0 );
    }
#endif // INTERNAL_WEB_SERVER

    if( HInfo->DeviceAf == AF_INET ) {
        UpnpSdkDeviceRegisteredV4 = 0;
    } else if( HInfo->DeviceAf == AF_INET6 ) {
        UpnpSdkDeviceregisteredV6 = 0;
    }

    FreeHandle(Hnd);
    HandleUnlock();

    UpnpPrintf( UPNP_INFO, API, __FILE__, __LINE__,
        "Exiting UpnpUnRegisterRootDevice \n" );

    return retVal;

}
#endif // INCLUDE_DEVICE_APIS


#ifdef INCLUDE_CLIENT_APIS
int UpnpRegisterClient(
	Upnp_FunPtr Fun,
	const void *Cookie,
	UpnpClient_Handle *Hnd)
{
    struct Handle_Info *HInfo;

    if( UpnpSdkInit != 1 ) {
        return UPNP_E_FINISH;
    }
    UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
        "Inside UpnpRegisterClient \n" );
    if( Fun == NULL || Hnd == NULL ) {
        return UPNP_E_INVALID_PARAM;
    }

    HandleLock();

    if( UpnpSdkClientRegistered ) {
        HandleUnlock();
        return UPNP_E_ALREADY_REGISTERED;
    }
    if( ( *Hnd = GetFreeHandle() ) == UPNP_E_OUTOF_HANDLE ) {
        HandleUnlock();
        return UPNP_E_OUTOF_MEMORY;
    }
    HInfo = ( struct Handle_Info * )malloc( sizeof( struct Handle_Info ) );
    if( HInfo == NULL ) {
        HandleUnlock();
        return UPNP_E_OUTOF_MEMORY;
    }

    HInfo->HType = HND_CLIENT;
    HInfo->Callback = Fun;
    HInfo->Cookie = ( void * )Cookie;
    HInfo->ClientSubList = NULL;
    ListInit( &HInfo->SsdpSearchList, NULL, NULL );
#ifdef INCLUDE_DEVICE_APIS
    HInfo->MaxAge = 0;
    HInfo->MaxSubscriptions = UPNP_INFINITE;
    HInfo->MaxSubscriptionTimeOut = UPNP_INFINITE;
#endif

    HandleTable[*Hnd] = HInfo;
    UpnpSdkClientRegistered = 1;

    HandleUnlock();

    UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
        "Exiting UpnpRegisterClient \n" );

    return UPNP_E_SUCCESS;
}
#endif // INCLUDE_CLIENT_APIS


#ifdef INCLUDE_CLIENT_APIS
int UpnpUnRegisterClient(UpnpClient_Handle Hnd)
{
    struct Handle_Info *HInfo;
    ListNode *node = NULL;
    SsdpSearchArg *searchArg = NULL;

    if( UpnpSdkInit != 1 ) {
        return UPNP_E_FINISH;
    }

    UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
        "Inside UpnpUnRegisterClient \n" );
    HandleLock();
    if( !UpnpSdkClientRegistered ) {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    HandleUnlock();

#if EXCLUDE_GENA == 0
    if( genaUnregisterClient( Hnd ) != UPNP_E_SUCCESS )
        return UPNP_E_INVALID_HANDLE;
#endif
    HandleLock();
    if( GetHandleInfo( Hnd, &HInfo ) == UPNP_E_INVALID_HANDLE ) {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    //clean up search list
    node = ListHead( &HInfo->SsdpSearchList );
    while( node != NULL ) {
        searchArg = ( SsdpSearchArg * ) node->item;
        if( searchArg ) {
            free( searchArg->searchTarget );
            free( searchArg );
        }
        ListDelNode( &HInfo->SsdpSearchList, node, 0 );
        node = ListHead( &HInfo->SsdpSearchList );
    }

    ListDestroy( &HInfo->SsdpSearchList, 0 );
    FreeHandle(Hnd);
    UpnpSdkClientRegistered = 0;
    HandleUnlock();
    UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
        "Exiting UpnpUnRegisterClient \n" );
    return UPNP_E_SUCCESS;

}
#endif // INCLUDE_CLIENT_APIS


#ifdef INCLUDE_DEVICE_APIS
#ifdef INTERNAL_WEB_SERVER
/**************************************************************************
 * Function: GetNameForAlias
 *
 * Parameters:	
 *	IN char *name: name of the file
 *	OUT char** alias: pointer to alias string 
 *
 * Description:
 *	This function determines alias for given name which is a file name 
 *	or URL.
 *
 * Return Values:
 *	UPNP_E_SUCCESS on success, nonzero on failure.
 ***************************************************************************/
static int
GetNameForAlias( IN char *name,
                 OUT char **alias )
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

/**************************************************************************
 * Function: get_server_addr
 *
 * Parameters:	
 *	OUT struct sockaddr* serverAddr: pointer to server address
 *		structure 
 *
 * Description:
 *	This function fills the sockadr with IPv4 miniserver information.
 *
 * Return Values: VOID
 *      
 ***************************************************************************/
static void
get_server_addr( OUT struct sockaddr *serverAddr )
{
    struct sockaddr_in* sa4 = (struct sockaddr_in*)serverAddr;

    memset( serverAddr, 0, sizeof(struct sockaddr_storage) );

    sa4->sin_family = AF_INET;
    inet_pton( AF_INET, gIF_IPV4, &sa4->sin_addr );
    sa4->sin_port = htons( LOCAL_PORT_V4 );
}

/**************************************************************************
 * Function: get_server_addr6
 *
 * Parameters:	
 *	OUT struct sockaddr* serverAddr: pointer to server address
 *		structure 
 *
 * Description:
 *	This function fills the sockadr with IPv6 miniserver information.
 *
 * Return Values: VOID
 *      
 ***************************************************************************/
static void
get_server_addr6( OUT struct sockaddr *serverAddr )
{
    struct sockaddr_in6* sa6 = (struct sockaddr_in6*)serverAddr;

    memset( serverAddr, 0, sizeof(struct sockaddr_storage) );

    sa6->sin6_family = AF_INET6;
    inet_pton(AF_INET6, gIF_IPV6, &sa6->sin6_addr );
    sa6->sin6_port = htons( LOCAL_PORT_V6 );
}

/**************************************************************************
 * Function: GetDescDocumentAndURL ( In the case of device)
 *
 * Parameters:	
 *	IN Upnp_DescType descriptionType: pointer to server address
 *		structure 
 *	IN char* description:
 *	IN unsigned int bufferLen:
 *	IN int config_baseURL:
 *	IN int AddressFamily:
 *	OUT IXML_Document **xmlDoc:
 *	OUT char descURL[LINE_SIZE]:
 *
 * Description:
 *	This function fills the sockadr_in with miniserver information.
 *
 * Return Values: VOID
 *      
 ***************************************************************************/
static int
GetDescDocumentAndURL( IN Upnp_DescType descriptionType,
                       IN char *description,
                       IN unsigned int bufferLen,
                       IN int config_baseURL,
                       IN int AddressFamily,
                       OUT IXML_Document ** xmlDoc,
                       OUT char descURL[LINE_SIZE] )
{
    int retVal = 0;
    char *membuf = NULL;
    char aliasStr[LINE_SIZE];
    char *temp_str = NULL;
    FILE *fp = NULL;
    off_t fileLen;
    size_t num_read;
    time_t last_modified;
    struct stat file_info;
    struct sockaddr_storage serverAddr;
    int rc = UPNP_E_SUCCESS;

    if( description == NULL ) {
        return UPNP_E_INVALID_PARAM;
    }
    // non-URL description must have configuration specified
    if( descriptionType != UPNPREG_URL_DESC && ( !config_baseURL ) ) {
        return UPNP_E_INVALID_PARAM;
    }
    // get XML doc and last modified time
    if( descriptionType == UPNPREG_URL_DESC ) {
        if( ( retVal =
              UpnpDownloadXmlDoc( description,
                                  xmlDoc ) ) != UPNP_E_SUCCESS ) {
            return retVal;
        }
        last_modified = time( NULL );
    } else if( descriptionType == UPNPREG_FILENAME_DESC ) {
        retVal = stat( description, &file_info );
        if( retVal == -1 ) {
            return UPNP_E_FILE_NOT_FOUND;
        }
        fileLen = file_info.st_size;
        last_modified = file_info.st_mtime;

        if( ( fp = fopen( description, "rb" ) ) == NULL ) {
            return UPNP_E_FILE_NOT_FOUND;
        }

        if( ( membuf = ( char * )malloc( fileLen + 1 ) ) == NULL ) {
            fclose( fp );
            return UPNP_E_OUTOF_MEMORY;
        }

        num_read = fread( membuf, 1, fileLen, fp );
        if( num_read != fileLen ) {
            fclose( fp );
            free( membuf );
            return UPNP_E_FILE_READ_ERROR;
        }

        membuf[fileLen] = 0;
        fclose( fp );
        rc = ixmlParseBufferEx( membuf, xmlDoc );
        free( membuf );
    } else if( descriptionType == UPNPREG_BUF_DESC ) {
        last_modified = time( NULL );
        rc = ixmlParseBufferEx( description, xmlDoc );
    } else {
        return UPNP_E_INVALID_PARAM;
    }

    if( rc != IXML_SUCCESS && descriptionType != UPNPREG_URL_DESC ) {
        if( rc == IXML_INSUFFICIENT_MEMORY ) {
            return UPNP_E_OUTOF_MEMORY;
        } else {
            return UPNP_E_INVALID_DESC;
        }
    }
    // determine alias
    if( config_baseURL ) {
        if( descriptionType == UPNPREG_BUF_DESC ) {
            strcpy( aliasStr, "description.xml" );
        } else                  // URL or filename
        {
            retVal = GetNameForAlias( description, &temp_str );
            if( retVal != UPNP_E_SUCCESS ) {
                ixmlDocument_free( *xmlDoc );
                return retVal;
            }
            if( strlen( temp_str ) > ( LINE_SIZE - 1 ) ) {
                ixmlDocument_free( *xmlDoc );
                free( temp_str );
                return UPNP_E_URL_TOO_BIG;
            }
            strcpy( aliasStr, temp_str );
        }

        if(AddressFamily == AF_INET)
            get_server_addr( (struct sockaddr*)&serverAddr );
        else 
            get_server_addr6( (struct sockaddr*)&serverAddr );

        // config
        retVal = configure_urlbase( *xmlDoc, (struct sockaddr*)&serverAddr,
                                    aliasStr, last_modified, descURL );
        if( retVal != UPNP_E_SUCCESS ) {
            ixmlDocument_free( *xmlDoc );
            return retVal;
        }
    } else                      // manual
    {
        if( strlen( description ) > ( LINE_SIZE - 1 ) ) {
            ixmlDocument_free( *xmlDoc );
            return UPNP_E_URL_TOO_BIG;
        }
        strcpy( descURL, description );
    }

    assert( *xmlDoc != NULL );

    return UPNP_E_SUCCESS;
}

#else // no web server

/**************************************************************************
 * Function: GetDescDocumentAndURL ( In the case of control point)
 *
 *  Parameters:	
 *	IN Upnp_DescType descriptionType: pointer to server address
 *		structure 
 *	IN char* description:
 *	IN unsigned int bufferLen:
 *	IN int config_baseURL:
 *	IN int AddressFamily:
 *	OUT IXML_Document **xmlDoc:
 *	OUT char *descURL: 
 *
 * Description:
 *	This function fills the sockadr_in with miniserver information.
 *
 * Return Values: VOID
 *      
 ***************************************************************************/
static int
GetDescDocumentAndURL( IN Upnp_DescType descriptionType,
                       IN char *description,
                       IN unsigned int bufferLen,
                       IN int config_baseURL,
                       IN int AddressFamily,
                       OUT IXML_Document ** xmlDoc,
                       OUT char *descURL )
{
    int retVal;

    if( ( descriptionType != UPNPREG_URL_DESC ) || config_baseURL ) {
        return UPNP_E_NO_WEB_SERVER;
    }

    if( description == NULL ) {
        return UPNP_E_INVALID_PARAM;
    }

    if( strlen( description ) > ( LINE_SIZE - 1 ) ) {
        return UPNP_E_URL_TOO_BIG;
    }
    strcpy( descURL, description );

    if( ( retVal =
          UpnpDownloadXmlDoc( description, xmlDoc ) ) != UPNP_E_SUCCESS ) {
        return retVal;
    }

    return UPNP_E_SUCCESS;
}

#endif // INTERNAL_WEB_SERVER
#endif // INCLUDE_DEVICE_APIS


//-----------------------------------------------------------------------------
//
//                                   SSDP interface
//
//-----------------------------------------------------------------------------

#ifdef INCLUDE_DEVICE_APIS
#if EXCLUDE_SSDP == 0

/**************************************************************************
 * Function: UpnpSendAdvertisement 
 *
 * Parameters:	
 *	IN UpnpDevice_Handle Hnd: handle of the device instance
 *	IN int Exp : Timer for resending the advertisement
 *
 * Description:
 *	This function sends the device advertisement. It also schedules a
 *	job for the next advertisement after "Exp" time.
 *
 * Return Values: int
 *	UPNP_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
UpnpSendAdvertisement( IN UpnpDevice_Handle Hnd,
                       IN int Exp )
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
        "Inside UpnpSendAdvertisement \n" );

    HandleLock();
    if( GetHandleInfo( Hnd, &SInfo ) != HND_DEVICE ) {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    if( Exp < 1 )
        Exp = DEFAULT_MAXAGE;
    SInfo->MaxAge = Exp;
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
        "Exiting UpnpSendAdvertisement \n" );

    return retVal;

}  /****************** End of UpnpSendAdvertisement *********************/
#endif // INCLUDE_DEVICE_APIS
#endif
#if EXCLUDE_SSDP == 0
#ifdef INCLUDE_CLIENT_APIS

/**************************************************************************
 * Function: UpnpSearchAsync 
 *
 * Parameters:	
 *	IN UpnpClient_Handle Hnd: handle of the control point instance
 *	IN int Mx : Maximum time to wait for the search reply
 *	IN const char *Target_const: 
 *	IN const void *Cookie_const:
 *
 * Description:
 *	This function searches for the devices for the provided maximum time.
 *	It is a asynchronous function. It schedules a search job and returns. 
 *	client is notified about the search results after search timer.
 *
 * Return Values: int
 *	UPNP_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
UpnpSearchAsync( IN UpnpClient_Handle Hnd,
                 IN int Mx,
                 IN const char *Target_const,
                 IN const void *Cookie_const )
{
    struct Handle_Info *SInfo = NULL;
    char *Target = ( char * )Target_const;

    if( UpnpSdkInit != 1 ) {
        return UPNP_E_FINISH;
    }

    UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
        "Inside UpnpSearchAsync \n" );

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

    //HandleUnlock();

    UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
        "Exiting UpnpSearchAsync \n" );

    return UPNP_E_SUCCESS;

}  /****************** End of UpnpSearchAsync *********************/
#endif // INCLUDE_CLIENT_APIS
#endif
//-----------------------------------------------------------------------------
//
//                                   GENA interface 
//
//-----------------------------------------------------------------------------

#if EXCLUDE_GENA == 0
#ifdef INCLUDE_DEVICE_APIS

/**************************************************************************
 * Function: UpnpSetMaxSubscriptions 
 *
 * Parameters:	
 *	IN UpnpDevice_Handle Hnd: The handle of the device for which
 *		the maximum subscriptions is being set.
 *	IN int MaxSubscriptions: The maximum number of subscriptions to be
 *		allowed per service.
 *
 * Description:
 *	This function sets the maximum subscriptions of the control points
 * Return Values: int
 *	UPNP_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
UpnpSetMaxSubscriptions( IN UpnpDevice_Handle Hnd,
                         IN int MaxSubscriptions )
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

}  /***************** End of UpnpSetMaxSubscriptions ********************/
#endif // INCLUDE_DEVICE_APIS

#ifdef INCLUDE_DEVICE_APIS

/**************************************************************************
 * Function: UpnpSetMaxSubscriptionTimeOut 
 *
 * Parameters:	
 *	IN UpnpDevice_Handle Hnd: The handle of the device for which the
 *		maximum subscription time-out is being set.
 *	IN int MaxSubscriptionTimeOut:The maximum subscription time-out 
 *		to be accepted
 *
 * Description:
 *	This function sets the maximum subscription timer. Control points
 *	will require to send the subscription request before timeout.
 *
 * Return Values: int
 *	UPNP_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
UpnpSetMaxSubscriptionTimeOut( IN UpnpDevice_Handle Hnd,
                               IN int MaxSubscriptionTimeOut )
{
    struct Handle_Info *SInfo = NULL;

    if( UpnpSdkInit != 1 ) {
        return UPNP_E_FINISH;
    }

    UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
        "Inside UpnpSetMaxSubscriptionTimeOut \n" );

    HandleLock();

    if( ( ( MaxSubscriptionTimeOut != UPNP_INFINITE )
          && ( MaxSubscriptionTimeOut < 0 ) )
        || ( GetHandleInfo( Hnd, &SInfo ) != HND_DEVICE ) ) {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }

    SInfo->MaxSubscriptionTimeOut = MaxSubscriptionTimeOut;
    HandleUnlock();

    UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
        "Exiting UpnpSetMaxSubscriptionTimeOut \n" );

    return UPNP_E_SUCCESS;

}  /****************** End of UpnpSetMaxSubscriptionTimeOut ******************/
#endif // INCLUDE_DEVICE_APIS

#ifdef INCLUDE_CLIENT_APIS

/**************************************************************************
 * Function: UpnpSubscribeAsync 
 *
 * Parameters:	
 *	IN UpnpClient_Handle Hnd: The handle of the control point for which 
 *		the subscription request is to be sent.
 *	IN const char * EvtUrl_const: URL that control point wants to 
 *		subscribe
 *	IN int TimeOut: The requested subscription time.  Upon 
 *		return, it contains the actual subscription time 
 *		returned from the service
 *	IN Upnp_FunPtr Fun : callback function to tell result of the 
 *		subscription request
 *	IN const void * Cookie_const: cookie passed by client to give back 
 *		in the callback function.
 *
 * Description:
 *	This function performs the same operation as UpnpSubscribeAsync
 *	but returns immediately and calls the registered callback function 
 *	when the operation is complete.
 *
 * Return Values: int
 *	UPNP_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
UpnpSubscribeAsync( IN UpnpClient_Handle Hnd,
                    IN const char *EvtUrl_const,
                    IN int TimeOut,
                    IN Upnp_FunPtr Fun,
                    IN const void *Cookie_const )
{
    struct Handle_Info *SInfo = NULL;
    struct UpnpNonblockParam *Param;
    char *EvtUrl = ( char * )EvtUrl_const;
    ThreadPoolJob job;

    if( UpnpSdkInit != 1 ) {
        return UPNP_E_FINISH;
    }

    UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
        "Inside UpnpSubscribeAsync \n" );

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
    Param->Cookie = ( void * )Cookie_const;

    TPJobInit( &job, ( start_routine ) UpnpThreadDistribution, Param );
    TPJobSetFreeFunction( &job, ( free_routine ) free );
    TPJobSetPriority( &job, MED_PRIORITY );
    ThreadPoolAdd( &gSendThreadPool, &job, NULL );

    UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
        "Exiting UpnpSubscribeAsync \n" );

    return UPNP_E_SUCCESS;

}  /****************** End of UpnpSubscribeAsync *********************/
#endif // INCLUDE_CLIENT_APIS

#ifdef INCLUDE_CLIENT_APIS

/**************************************************************************
 * Function: UpnpSubscribe 
 *
 * Parameters:	
 *	IN UpnpClient_Handle Hnd: The handle of the control point.
 *	IN const char *PublisherUrl: The URL of the service to subscribe to.
 *	INOUT int *TimeOut: Pointer to a variable containing the requested 
 *		subscription time.  Upon return, it contains the
 *		actual subscription time returned from the service.
 *	OUT Upnp_SID SubsId: Pointer to a variable to receive the 
 *		subscription ID (SID). 
 *
 * Description:
 *	This function registers a control point to receive event
 *	notifications from another device.  This operation is synchronous
 *
 * Return Values: int
 *	UPNP_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int UpnpSubscribe(
	IN UpnpClient_Handle Hnd,
	IN const char *EvtUrl_const,
	INOUT int *TimeOut,
	OUT Upnp_SID SubsId)
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

/**************************************************************************
 * Function: UpnpUnSubscribe 
 *
 *  Parameters:	
 *	IN UpnpClient_Handle Hnd: The handle of the control point.
 *	IN const Upnp_SID SubsId: The ID returned when the control point 
 *		subscribed to the service.
 *
 * Description:
 *	This function removes the subscription of  a control point from a 
 *	service previously subscribed to using UpnpSubscribe or 
 *	UpnpSubscribeAsync. This is a synchronous call.
 *
 * Return Values: int
 *	UPNP_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int UpnpUnSubscribe(IN UpnpClient_Handle Hnd, IN const Upnp_SID SubsId)
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

/**************************************************************************
 * Function: UpnpUnSubscribeAsync 
 *
 *  Parameters:	
 *	IN UpnpClient_Handle Hnd: The handle of the subscribed control point. 
 *	IN Upnp_SID SubsId: The ID returned when the control point 
 *		subscribed to the service.
 *	IN Upnp_FunPtr Fun: Pointer to a callback function to be called
 *		when the operation is complete. 
 *	IN const void *Cookie:Pointer to user data to pass to the
 *		callback function when invoked.
 *
 *  Description:
 *      This function removes a subscription of a control point
 *  from a service previously subscribed to using UpnpSubscribe or
 *	UpnpSubscribeAsync,generating a callback when the operation is complete.
 *
 *  Return Values: int
 *      UPNP_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int UpnpUnSubscribeAsync(
	IN UpnpClient_Handle Hnd,
	IN Upnp_SID SubsId,
	IN Upnp_FunPtr Fun,
	IN const void *Cookie_const )
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

/**************************************************************************
 * Function: UpnpRenewSubscription 
 *
 * Parameters:	
 *	IN UpnpClient_Handle Hnd: The handle of the control point that 
 *		is renewing the subscription.
 *	INOUT int *TimeOut: Pointer to a variable containing the 
 *		requested subscription time.  Upon return, 
 *		it contains the actual renewal time. 
 *	IN const Upnp_SID SubsId: The ID for the subscription to renew. 
 *
 * Description:
 *	This function renews a subscription that is about to 
 *	expire.  This function is synchronous.
 *
 * Return Values: int
 *	UPNP_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int UpnpRenewSubscription(
	IN UpnpClient_Handle Hnd,
	INOUT int *TimeOut,
	IN const Upnp_SID SubsId )
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

/**************************************************************************
 * Function: UpnpRenewSubscriptionAsync 
 *
 * Parameters:	
 *	IN UpnpClient_Handle Hnd: The handle of the control point that 
 *		is renewing the subscription. 
 *	IN int TimeOut: The requested subscription time.  The 
 *		actual timeout value is returned when 
 *		the callback function is called. 
 *	IN Upnp_SID SubsId: The ID for the subscription to renew. 
 *	IN Upnp_FunPtr Fun: Pointer to a callback function to be 
 *		invoked when the renewal is complete. 
 *	IN const void *Cookie  : Pointer to user data passed 
 *		to the callback function when invoked.
 *
 * Description:
 *	This function renews a subscription that is about
 *	to expire, generating a callback when the operation is complete.
 *
 * Return Values: int
 *	UPNP_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
UpnpRenewSubscriptionAsync( IN UpnpClient_Handle Hnd,
                            INOUT int TimeOut,
                            IN Upnp_SID SubsId,
                            IN Upnp_FunPtr Fun,
                            IN const void *Cookie_const )
{
    ThreadPoolJob job;
    struct Handle_Info *SInfo = NULL;
    struct UpnpNonblockParam *Param;

    if( UpnpSdkInit != 1 ) {
        return UPNP_E_FINISH;
    }

    UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
        "Inside UpnpRenewSubscriptionAsync \n" );
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

    UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
        "Exiting UpnpRenewSubscriptionAsync \n" );

    return UPNP_E_SUCCESS;

}  /****************** End of UpnpRenewSubscriptionAsync *******************/
#endif // INCLUDE_CLIENT_APIS

#ifdef INCLUDE_DEVICE_APIS

/**************************************************************************
 * Function: UpnpNotify 
 *
 *  Parameters:	
 *	IN UpnpDevice_Handle: The handle to the device sending the event.
 *	IN const char *DevID: The device ID of the subdevice of the 
 *		service generating the event. 
 *	IN const char *ServID: The unique identifier of the service 
 *		generating the event. 
 *	IN const char **VarName: Pointer to an array of variables that 
 *		have changed.
 *	IN const char **NewVal: Pointer to an array of new values for 
 *		those variables. 
 *	IN int cVariables: The count of variables included in this 
 *		notification. 
 *
 * Description:
 *	This function sends out an event change notification to all
 *	control points subscribed to a particular service.  This function is
 *	synchronous and generates no callbacks.
 *
 * Return Values: int
 *	UPNP_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
UpnpNotify( IN UpnpDevice_Handle Hnd,
            IN const char *DevID_const,
            IN const char *ServName_const,
            IN const char **VarName_const,
            IN const char **NewVal_const,
            IN int cVariables )
{

    struct Handle_Info *SInfo = NULL;
    int retVal;
    char *DevID = ( char * )DevID_const;
    char *ServName = ( char * )ServName_const;
    char **VarName = ( char ** )VarName_const;
    char **NewVal = ( char ** )NewVal_const;

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
    if( VarName == NULL || NewVal == NULL || cVariables < 0 ) {
        HandleUnlock();
        return UPNP_E_INVALID_PARAM;
    }

    HandleUnlock();
    retVal =
        genaNotifyAll( Hnd, DevID, ServName, VarName, NewVal, cVariables );

    UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
        "Exiting UpnpNotify \n" );

    return retVal;

} /****************** End of UpnpNotify *********************/

/**************************************************************************
 * Function: UpnpNotifyExt 
 *
 * Parameters:	
 *	IN UpnpDevice_Handle: The handle to the device sending the 
 *		event.
 *	IN const char *DevID: The device ID of the subdevice of the 
 *		service generating the event.
 *	IN const char *ServID: The unique identifier of the service 
 *		generating the event. 
 *	IN IXML_Document *PropSet: The DOM document for the property set. 
 *		Property set documents must conform to the XML schema
 *		defined in section 4.3 of the Universal Plug and Play
 *		Device Architecture specification. 
 *
 * Description:
 *	This function is similar to UpnpNotify except that it takes
 *	a DOM document for the event rather than an array of strings. This 
 *	function is synchronous and generates no callbacks.
 *
 * Return Values: int
 *	UPNP_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
UpnpNotifyExt( IN UpnpDevice_Handle Hnd,
               IN const char *DevID_const,
               IN const char *ServName_const,
               IN IXML_Document * PropSet )
{

    struct Handle_Info *SInfo = NULL;
    int retVal;
    char *DevID = ( char * )DevID_const;
    char *ServName = ( char * )ServName_const;

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

}  /****************** End of UpnpNotify *********************/

#endif // INCLUDE_DEVICE_APIS

#ifdef INCLUDE_DEVICE_APIS

/**************************************************************************
 * Function: UpnpAcceptSubscription 
 *
 * Parameters:	
 *	IN UpnpDevice_Handle Hnd: The handle of the device. 
 *	IN const char *DevID: The device ID of the subdevice of the 
 *		service generating the event. 
 *	IN const char *ServID: The unique service identifier of the 
 *		service generating the event.
 *	IN const char **VarName: Pointer to an array of event variables.
 *	IN const char **NewVal: Pointer to an array of values for 
 *		the event variables.
 *	IN int cVariables: The number of event variables in VarName. 
 *	IN Upnp_SID SubsId: The subscription ID of the newly 
 *		registered control point. 
 *
 * Description:
 *	This function accepts a subscription request and sends
 *	out the current state of the eventable variables for a service.  
 *	The device application should call this function when it receives a 
 *	UPNP_EVENT_SUBSCRIPTION_REQUEST callback. This function is sychronous
 *	and generates no callbacks.
 *
 * Return Values: int
 *	UPNP_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int UpnpAcceptSubscription(
	IN UpnpDevice_Handle Hnd,
	IN const char *DevID_const,
	IN const char *ServName_const,
	IN const char **VarName_const,
	IN const char **NewVal_const,
	int cVariables,
	IN const Upnp_SID SubsId)
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
		goto ExitFunction;
	}

	HandleReadLock();

	if (GetHandleInfo(Hnd, &SInfo) != HND_DEVICE) {
		HandleUnlock();
		line = __LINE__;
		ret = UPNP_E_INVALID_HANDLE;
		goto ExitFunction;
	}
	if (DevID == NULL) {
		HandleUnlock();
		line = __LINE__;
		ret = UPNP_E_INVALID_PARAM;
		goto ExitFunction;
	}
	if (ServName == NULL) {
		HandleUnlock();
		line = __LINE__;
		ret = UPNP_E_INVALID_PARAM;
		goto ExitFunction;
	}
	if (SubsId == NULL) {
		HandleUnlock();
		line = __LINE__;
		ret = UPNP_E_INVALID_PARAM;
		goto ExitFunction;
	}
	/* Now accepts an empty state list, so the code below is commented out */
#if 0
	if (VarName == NULL || NewVal == NULL || cVariables < 0) {
		HandleUnlock();
		line = __LINE__;
		ret = UPNP_E_INVALID_PARAM;
		goto ExitFunction;
	}
#endif

	HandleUnlock();

	line = __LINE__;
	ret = genaInitNotify(
		Hnd, DevID, ServName, VarName, NewVal, cVariables, SubsId);

ExitFunction:
	UpnpPrintf(UPNP_ALL, API, __FILE__, line,
		"Exiting UpnpAcceptSubscription, ret = %d\n", ret);

	return ret;
}


/**************************************************************************
 * Function: UpnpAcceptSubscriptionExt 
 *
 * Parameters:	
 * 	IN UpnpDevice_Handle Hnd: The handle of the device. 
 * 	IN const char *DevID: The device ID of the subdevice of the 
 *		service generating the event. 
 *	IN const char *ServID: The unique service identifier of the service 
 *		generating the event. 
 *	IN IXML_Document *PropSet: The DOM document for the property set. 
 *		Property set documents must conform to the XML schema
 *		defined in section 4.3 of the Universal Plug and Play
 *		Device Architecture specification. 
 *	IN Upnp_SID SubsId: The subscription ID of the newly
 *		registered control point. 
 *
 * Description:
 *	This function is similar to UpnpAcceptSubscription except that it
 *	takes a DOM document for the variables to event rather than an array
 *	of strings. This function is sychronous and generates no callbacks.
 *
 * Return Values: int
 *	UPNP_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int UpnpAcceptSubscriptionExt(
	IN UpnpDevice_Handle Hnd,
	IN const char *DevID_const,
	IN const char *ServName_const,
	IN IXML_Document *PropSet,
	IN Upnp_SID SubsId)
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
		goto ExitFunction;
	}

	HandleReadLock();

	if (GetHandleInfo(Hnd, &SInfo) != HND_DEVICE) {
		HandleUnlock();
		line = __LINE__;
		ret = UPNP_E_INVALID_HANDLE;
		goto ExitFunction;
	}
	if (DevID == NULL) {
		HandleUnlock();
		line = __LINE__;
		ret = UPNP_E_INVALID_PARAM;
		goto ExitFunction;
	}
	if (ServName == NULL) {
		HandleUnlock();
		line = __LINE__;
		ret = UPNP_E_INVALID_PARAM;
		goto ExitFunction;
	}
	if (SubsId == NULL) {
		HandleUnlock();
		line = __LINE__;
		ret = UPNP_E_INVALID_PARAM;
		goto ExitFunction;
	}
	/* Now accepts an empty state list, so the code below is commented out */
#if 0
	if (PropSet == NULL) {
		HandleUnlock();
		line = __LINE__;
		ret = UPNP_E_INVALID_PARAM;
		goto ExitFunction;
	}
#endif

	HandleUnlock();

	line = __LINE__;
	ret = genaInitNotifyExt(Hnd, DevID, ServName, PropSet, SubsId);

ExitFunction:
	UpnpPrintf(UPNP_ALL, API, __FILE__, line,
		"Exiting UpnpAcceptSubscription, ret = %d.\n", ret);

	return ret;
}

#endif // INCLUDE_DEVICE_APIS
#endif // EXCLUDE_GENA == 0

//---------------------------------------------------------------------------
//
//                                   SOAP interface 
//
//---------------------------------------------------------------------------
#if EXCLUDE_SOAP == 0
#ifdef INCLUDE_CLIENT_APIS

/**************************************************************************
 * Function: UpnpSendAction 
 *
 * Parameters:	
 *	IN UpnpClient_Handle Hnd: The handle of the control point 
 *		sending the action. 
 *	IN const char *ActionURL: The action URL of the service. 
 *	IN const char *ServiceType: The type of the service. 
 *	IN const char *DevUDN: This parameter is ignored. 
 *	IN IXML_Document *Action: The DOM document for the action. 
 *	OUT IXML_Document **RespNode: The DOM document for the response 
 *		to the action.  The UPnP Library allocates this document
 *		and the caller needs to free it.  
 *  
 * Description:
 *	This function sends a message to change a state variable in a service.
 *	This is a synchronous call that does not return until the action is
 *	complete.
 * 
 *	Note that a positive return value indicates a SOAP-protocol error code.
 *	In this case,  the error description can be retrieved from RespNode.
 *	A negative return value indicates a UPnP Library error.
 *
 * Return Values: int
 *	UPNP_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
UpnpSendAction( IN UpnpClient_Handle Hnd,
                IN const char *ActionURL_const,
                IN const char *ServiceType_const,
                IN const char *DevUDN_const,
                IN IXML_Document * Action,
                OUT IXML_Document ** RespNodePtr )
{
    struct Handle_Info *SInfo = NULL;
    int retVal = 0;
    char *ActionURL = ( char * )ActionURL_const;
    char *ServiceType = ( char * )ServiceType_const;

    //char *DevUDN = (char *)DevUDN_const;  // udn not used?

    if( UpnpSdkInit != 1 ) {
        return UPNP_E_FINISH;
    }

    UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
        "Inside UpnpSendAction \n" );
    if(DevUDN_const !=NULL) {
	UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"non NULL DevUDN is ignored\n");
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

    UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
        "Exiting UpnpSendAction \n" );

    return retVal;

}  /****************** End of UpnpSendAction *********************/

/**************************************************************************
 * Function: UpnpSendActionEx 
 *
 * Parameters:	
 *	IN UpnpClient_Handle Hnd: The handle of the control point sending
 *		the action. 
 *	IN const char *ActionURL_const: The action URL of the service. 
 *	IN const char *ServiceType_const: The type of the service. 
 *	IN const char *DevUDN_const: This parameter is ignored. 
 *	IN IXML_Document *Header: The DOM document for the SOAP header. 
 *		This may be NULL if the header is not required. 
 *	IN IXML_Document *Action:   The DOM document for the action. 
 *	OUT IXML_Document **RespNodePtr: The DOM document for the response to
 *		the action.  The UPnP library allocates this document and the
 *		caller needs to free it.
 *  
 * Description:
 *	this function sends a message to change a state variable in a 
 *	service. This is a synchronous call that does not return until the 
 *	action is complete.
 *
 *	Note that a positive return value indicates a SOAP-protocol error code.
 *	In this case,  the error description can be retrieved from {\bf RespNode}.
 *	A negative return value indicates a UPnP Library error.
 *
 * Return Values: int
 *	UPNP_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
UpnpSendActionEx( IN UpnpClient_Handle Hnd,
                  IN const char *ActionURL_const,
                  IN const char *ServiceType_const,
                  IN const char *DevUDN_const,
                  IN IXML_Document * Header,
                  IN IXML_Document * Action,
                  OUT IXML_Document ** RespNodePtr )
{

    struct Handle_Info *SInfo = NULL;
    int retVal = 0;
    char *ActionURL = ( char * )ActionURL_const;
    char *ServiceType = ( char * )ServiceType_const;

    //char *DevUDN = (char *)DevUDN_const;  // udn not used?

    if( UpnpSdkInit != 1 ) {
        return UPNP_E_FINISH;
    }

    UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
        "Inside UpnpSendActionEx \n" );

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

    UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
        "Exiting UpnpSendAction \n" );

    return retVal;

}  /****************** End of UpnpSendActionEx *********************/

/**************************************************************************
 * Function: UpnpSendActionAsync 
 *
 *  Parameters:	
 *	IN UpnpClient_Handle Hnd: The handle of the control point 
 *		sending the action. 
 *	IN const char *ActionURL: The action URL of the service. 
 *	IN const char *ServiceType: The type of the service. 
 *	IN const char *DevUDN: This parameter is ignored. 
 *	IN IXML_Document *Action: The DOM document for the action to 
 *		perform on this device. 
 *	IN Upnp_FunPtr Fun: Pointer to a callback function to 
 *		be invoked when the operation completes
 *	IN const void *Cookie: Pointer to user data that to be 
 *		passed to the callback when invoked.
 *  
 * Description:
 *	this function sends a message to change a state variable
 *	in a service, generating a callback when the operation is complete.
 *	See UpnpSendAction for comments on positive return values. These 
 *	positive return values are sent in the event struct associated with the
 *	UPNP_CONTROL_ACTION_COMPLETE event.
 *
 * Return Values: int
 *	UPNP_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
UpnpSendActionAsync( IN UpnpClient_Handle Hnd,
                     IN const char *ActionURL_const,
                     IN const char *ServiceType_const,
                     IN const char *DevUDN_const,
                     IN IXML_Document * Act,
                     IN Upnp_FunPtr Fun,
                     IN const void *Cookie_const )
{
    ThreadPoolJob job;
    struct Handle_Info *SInfo = NULL;
    struct UpnpNonblockParam *Param;
    DOMString tmpStr;
    char *ActionURL = ( char * )ActionURL_const;
    char *ServiceType = ( char * )ServiceType_const;

    //char *DevUDN = (char *)DevUDN_const;
    int rc;

    if( UpnpSdkInit != 1 ) {
        return UPNP_E_FINISH;
    }

    UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
        "Inside UpnpSendActionAsync \n" );

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

    UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
        "Exiting UpnpSendActionAsync \n" );

    return UPNP_E_SUCCESS;

}  /****************** End of UpnpSendActionAsync *********************/

/*************************************************************************
 * Function: UpnpSendActionExAsync 
 *
 * Parameters:	
 *	IN UpnpClient_Handle Hnd: The handle of the control point 
 *		sending the action. 
 *	IN const char *ActionURL_const: The action URL of the service. 
 *	IN const char *ServiceType_const: The type of the service. 
 *	IN const char *DevUDN_const: This parameter is ignored. 
 *	IN IXML_Document *Header: The DOM document for the SOAP header. 
 *		This may be NULL if the header is not required. 
 *	IN IXML_Document *Act: The DOM document for the action to 
 *		perform on this device. 
 *	IN Upnp_FunPtr Fun: Pointer to a callback function to be invoked
 *		when the operation completes. 
 *	IN const void *Cookie_const: Pointer to user data that to be
 *		passed to the callback when invoked. 
 *
 * Description:
 *	this function sends sends a message to change a state variable
 *	in a service, generating a callback when the operation is complete.
 *	See UpnpSendAction for comments on positive return values. These 
 *	positive return values are sent in the event struct associated with 
 *	the UPNP_CONTROL_ACTION_COMPLETE event.
 *
 * Return Values: int
 *	UPNP_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
UpnpSendActionExAsync( IN UpnpClient_Handle Hnd,
                       IN const char *ActionURL_const,
                       IN const char *ServiceType_const,
                       IN const char *DevUDN_const,
                       IN IXML_Document * Header,
                       IN IXML_Document * Act,
                       IN Upnp_FunPtr Fun,
                       IN const void *Cookie_const )
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

    UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
        "Inside UpnpSendActionExAsync \n" );

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

    UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
        "Exiting UpnpSendActionAsync \n" );

    return UPNP_E_SUCCESS;

}  /****************** End of UpnpSendActionExAsync *********************/

/*************************************************************************
 * Function: UpnpGetServiceVarStatusAsync 
 *
 * Parameters:	
 *	IN UpnpClient_Handle Hnd: The handle of the control point. 
 *	IN const char *ActionURL: The URL of the service. 
 *	IN const char *VarName: The name of the variable to query. 
 *	IN Upnp_FunPtr Fun: Pointer to a callback function to 
 *		be invoked when the operation is complete. 
 *	IN const void *Cookie: Pointer to user data to pass to the 
 *		callback function when invoked. 
 *
 *  Description:
 *      this function queries the state of a variable of a 
 *  service, generating a callback when the operation is complete.
 *
 *  Return Values: int
 *      UPNP_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
UpnpGetServiceVarStatusAsync( IN UpnpClient_Handle Hnd,
                              IN const char *ActionURL_const,
                              IN const char *VarName_const,
                              IN Upnp_FunPtr Fun,
                              IN const void *Cookie_const )
{
    ThreadPoolJob job;
    struct Handle_Info *SInfo = NULL;
    struct UpnpNonblockParam *Param;
    char *ActionURL = ( char * )ActionURL_const;
    char *VarName = ( char * )VarName_const;

    if( UpnpSdkInit != 1 ) {
        return UPNP_E_FINISH;
    }

    UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
        "Inside UpnpGetServiceVarStatusAsync \n" );

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

    UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
        "Exiting UpnpGetServiceVarStatusAsync \n" );

    return UPNP_E_SUCCESS;

}  /****************** End of UpnpGetServiceVarStatusAsync ****************/

/**************************************************************************
 * Function: UpnpGetServiceVarStatus 
 *
 * Parameters:	
 *	IN UpnpClient_Handle Hnd: The handle of the control point.
 *	IN const char *ActionURL: The URL of the service. 
 *	IN const char *VarName: The name of the variable to query. 
 *	OUT DOMString *StVarVal: The pointer to store the value 
 *		for VarName. The UPnP Library allocates this string and
 *		the caller needs to free it.
 *  
 * Description:
 *	this function queries the state of a state variable of a service on
 *	another device.  This is a synchronous call. A positive return value
 *	indicates a SOAP error code, whereas a negative return code indicates
 *	a UPnP SDK error code.
 *
 * Return Values: int
 *	UPNP_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
UpnpGetServiceVarStatus( IN UpnpClient_Handle Hnd,
                         IN const char *ActionURL_const,
                         IN const char *VarName_const,
                         OUT DOMString * StVar )
{
    struct Handle_Info *SInfo = NULL;
    int retVal = 0;
    char *StVarPtr;
    char *ActionURL = ( char * )ActionURL_const;
    char *VarName = ( char * )VarName_const;

    if( UpnpSdkInit != 1 ) {
        return UPNP_E_FINISH;
    }

    UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
        "Inside UpnpGetServiceVarStatus \n" );

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

    UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
        "Exiting UpnpGetServiceVarStatus \n" );

    return retVal;

}
#endif // INCLUDE_CLIENT_APIS
#endif // EXCLUDE_SOAP


/******************************************************************************
 *
 *                             Client API's 
 *
 *****************************************************************************/


int UpnpOpenHttpPost(
	const char *url,
	void **handle,
	const char *contentType,
	int contentLength,
	int timeout)
{
    return http_OpenHttpPost( url, handle, contentType, contentLength,
                              timeout );
}


int UpnpWriteHttpPost(
	void *handle,
	char *buf,
	unsigned int *size,
	int timeout)
{
    return http_WriteHttpPost( handle, buf, size, timeout );
}


int UpnpCloseHttpPost(
	void *handle,
	int *httpStatus,
	int timeout)
{
    return http_CloseHttpPost( handle, httpStatus, timeout );
}


int UpnpOpenHttpGet(
	const char *url_str,
	void **Handle,
	char **contentType,
	int *contentLength,
	int *httpStatus,
	int timeout)
{
    return http_OpenHttpGet( url_str, Handle, contentType, contentLength,
                             httpStatus, timeout );
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
    return http_OpenHttpGetProxy( url_str, proxy_str, Handle, contentType, contentLength,
                             httpStatus, timeout );
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
    return http_OpenHttpGetEx( url_str,
                               Handle,
                               contentType,
                               contentLength,
                               httpStatus, lowRange, highRange, timeout );
}


int UpnpCancelHttpGet(void *Handle)
{
	return http_CancelHttpGet(Handle);
}


int UpnpCloseHttpGet(void *Handle)
{
	return http_CloseHttpGet(Handle);
}


int UpnpReadHttpGet(void *Handle, char *buf, unsigned int *size, int timeout)
{
	return http_ReadHttpGet(Handle, buf, size, timeout);
}


int UpnpHttpGetProgress(void *Handle, unsigned int *length, unsigned int *total)
{
	return http_HttpGetProgress(Handle, length, total);
}


int UpnpDownloadUrlItem(const char *url, char **outBuf, char *contentType)
{
    int ret_code;
    int dummy;

    if( url == NULL || outBuf == NULL || contentType == NULL ) {
        return UPNP_E_INVALID_PARAM;
    }

    ret_code = http_Download( url, HTTP_DEFAULT_TIMEOUT, outBuf, &dummy,
                              contentType );
    if( ret_code > 0 ) {
        // error reply was received
        ret_code = UPNP_E_INVALID_URL;
    }

    return ret_code;
}


/*!
 * \brief UpnpDownloadXmlDoc 
 *
 * \return UPNP_E_SUCCESS if successful otherwise the appropriate error code.
 */
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
		// Linksys WRT54G router returns 
		// "CONTENT-TYPE: application/octet-stream".
		// Let's be nice to Linksys and try to parse document anyway.
		// If the data sended is not a xml file, ixmlParseBufferEx
		// will fail and the function will return UPNP_E_INVALID_DESC too.
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

//----------------------------------------------------------------------------
//
//                UPNP-API  Internal function implementation
//
//----------------------------------------------------------------------------

#ifdef WIN32
int WinsockInit()
{
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	wVersionRequested = MAKEWORD( 2, 2 );

	err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 ) {
		/* Tell the user that we could not find a usable */
		/* WinSock DLL.                                  */
		return UPNP_E_INIT_FAILED;
	}

	/* Confirm that the WinSock DLL supports 2.2.*/
	/* Note that if the DLL supports versions greater    */
	/* than 2.2 in addition to 2.2, it will still return */
	/* 2.2 in wVersion since that is the version we      */
	/* requested.                                        */
	 
	if ( LOBYTE( wsaData.wVersion ) != 2 ||
			HIBYTE( wsaData.wVersion ) != 2 ) {
		/* Tell the user that we could not find a usable */
		/* WinSock DLL.                                  */
		WSACleanup( );
		return UPNP_E_INIT_FAILED; 
	}
    return UPNP_E_SUCCESS;
}
#endif


int UpnpInitPreamble()
{
    uuid_upnp nls_uuid;
    int retVal = 0;

#ifdef WIN32
    retVal = WinsockInit();
    if( retVal != UPNP_E_SUCCESS ){
        return retVal;
    }
	/* The WinSock DLL is acceptable. Proceed. */
#endif

    srand( (unsigned int)time( NULL ) );      // needed by SSDP or other parts

    // Initialize debug output.
    retVal = UpnpInitLog();
    if( retVal != UPNP_E_SUCCESS ) {
        // UpnpInitLog does not return a valid UPNP_E_*
        return UPNP_E_INIT_FAILED;
    }

    UpnpPrintf( UPNP_INFO, API, __FILE__, __LINE__, "Inside UpnpInitPreamble\n" );

   // Initialize SDK global mutexes.
    retVal = UpnpInitMutexes();
    if( retVal != UPNP_E_SUCCESS ) {
        return retVal;
    }

    // Create the NLS uuid.
    uuid_create(&nls_uuid);
    uuid_unpack(&nls_uuid, gUpnpSdkNLSuuid);

    // Init the handle list.
    HandleLock();
    InitHandleList();
    HandleUnlock();

    // Initialize SDK global thread pools.
    retVal = UpnpInitThreadPools();
    if( retVal != UPNP_E_SUCCESS ) {
        return retVal;
    }

#if EXCLUDE_SOAP == 0
    SetSoapCallback( soap_device_callback );
#endif

#if EXCLUDE_GENA == 0
    SetGenaCallback( genaCallback );
#endif

    // Initialize the SDK timer thread.
    retVal = TimerThreadInit( &gTimerThread, &gSendThreadPool );
    if( retVal != UPNP_E_SUCCESS ) {
        UpnpFinish();
        return retVal;
    }

    return UPNP_E_SUCCESS;
}


int UpnpInitMutexes()
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
    // initialize subscribe mutex
#ifdef INCLUDE_CLIENT_APIS
    if (ithread_mutex_init(&GlobalClientSubscribeMutex, NULL) != 0) {
        return UPNP_E_INIT_FAILED;
    }
#endif
    return UPNP_E_SUCCESS;
}


int UpnpInitThreadPools()
{
	int ret = UPNP_E_SUCCESS;
	ThreadPoolAttr attr;

	TPAttrInit(&attr);
	TPAttrSetMaxThreads(&attr, MAX_THREADS);
	TPAttrSetMinThreads(&attr, MIN_THREADS);
	TPAttrSetJobsPerThread(&attr, JOBS_PER_THREAD);
	TPAttrSetIdleTime(&attr, THREAD_IDLE_TIME);
	TPAttrSetMaxJobsTotal(&attr, MAX_JOBS_TOTAL);

	if (ThreadPoolInit(&gSendThreadPool, &attr) != UPNP_E_SUCCESS) {
		ret = UPNP_E_INIT_FAILED;
		goto ExitFunction;
	}

	if (ThreadPoolInit(&gRecvThreadPool, &attr) != UPNP_E_SUCCESS) {
		ret = UPNP_E_INIT_FAILED;
		goto ExitFunction;
	}

#ifdef INTERNAL_WEB_SERVER
	if (ThreadPoolInit(&gMiniServerThreadPool, &attr) != UPNP_E_SUCCESS) {
		ret = UPNP_E_INIT_FAILED;
		goto ExitFunction;
	}
#endif

ExitFunction:
	if (ret != UPNP_E_SUCCESS) {
		UpnpSdkInit = 0;
		UpnpFinish();
	}

	return ret;
}


int UpnpInitStartServers(unsigned short DestPort)
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


int UpnpGetIfInfo(const char *IfName)
{
#ifdef WIN32
    // ----------------------------------------------------
    // WIN32 implementation will use the IpHlpAPI library.
    // ----------------------------------------------------
    PIP_ADAPTER_ADDRESSES adapts = NULL;
    PIP_ADAPTER_ADDRESSES adapts_item;
    PIP_ADAPTER_UNICAST_ADDRESS uni_addr;
    SOCKADDR* ip_addr;
    struct in_addr v4_addr;
    struct in6_addr v6_addr;
    ULONG adapts_sz = 0;
    ULONG ret;
    int ifname_found = 0;
    int valid_addr_found = 0;

    // Get Adapters addresses required size.
    ret = GetAdaptersAddresses(AF_UNSPEC,
        GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_DNS_SERVER,
	NULL, adapts, &adapts_sz );
    if( ret != ERROR_BUFFER_OVERFLOW ) {
        UpnpPrintf( UPNP_CRITICAL, API, __FILE__, __LINE__,
            "GetAdaptersAddresses failed to find list of adapters\n" );
        return UPNP_E_INIT;
    }

    // Allocate enough memory.
    adapts = (PIP_ADAPTER_ADDRESSES)malloc( adapts_sz );
    if( adapts == NULL ) {
        return UPNP_E_OUTOF_MEMORY;
    }

    // Do the call that will actually return the info.
    ret = GetAdaptersAddresses( AF_UNSPEC,
	GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_DNS_SERVER,
	NULL, adapts, &adapts_sz );
    if( ret != ERROR_SUCCESS ) {
        free( adapts );
        UpnpPrintf( UPNP_CRITICAL, API, __FILE__, __LINE__,
            "GetAdaptersAddresses failed to find list of adapters\n" );
        return UPNP_E_INIT;
    }

    // Copy interface name, if it was provided.
    if( IfName != NULL ) {
        if( strlen(IfName) > sizeof(gIF_NAME) )
            return UPNP_E_INVALID_INTERFACE;

        strncpy( gIF_NAME, IfName, sizeof(gIF_NAME) );
        ifname_found = 1;
    }

    adapts_item = adapts;
    while( adapts_item != NULL ) {

        if( adapts_item->Flags & IP_ADAPTER_NO_MULTICAST ) {
            continue;
        }

        if( ifname_found == 0 ) {
            // We have found a valid interface name. Keep it.
            strncpy( gIF_NAME, adapts_item->FriendlyName, sizeof(gIF_NAME) );
            ifname_found = 1;
        } else {
            if( strncmp( gIF_NAME, adapts_item->FriendlyName, sizeof(gIF_NAME) ) != 0 ) {
                // This is not the interface we're looking for.
                continue;
            }
        }

        // Loop thru this adapter's unicast IP addresses.
        uni_addr = adapts_item->FirstUnicastAddress;
        while( uni_addr ) {
            ip_addr = uni_addr->Address.lpSockaddr;
            switch( ip_addr->sa_family ) {
            case AF_INET:
                memcpy( &v4_addr, &((struct sockaddr_in *)ip_addr)->sin_addr, sizeof(v4_addr) );
                valid_addr_found = 1;
                break;
            case AF_INET6:
                // Only keep IPv6 link-local addresses.
                if( IN6_IS_ADDR_LINKLOCAL(&((struct sockaddr_in6 *)ip_addr)->sin6_addr) ) {
                    memcpy( &v6_addr, &((struct sockaddr_in6 *)ip_addr)->sin6_addr, sizeof(v6_addr) );
                    valid_addr_found = 1;
                }
                break;
            default:
                if( valid_addr_found == 0 ) {
                    // Address is not IPv4 or IPv6 and no valid address has 
                    // yet been found for this interface. Discard interface name.
                    ifname_found = 0;
                }
                break;
            }

            // Next address.
            uni_addr = uni_addr->Next;
        }

        if( valid_addr_found == 1 ) {
            gIF_INDEX = adapts_item->IfIndex;
            break;
        }
        
        // Next adapter.
        adapts_item = adapts_item->Next;
    }

    // Failed to find a valid interface, or valid address.
    if( ifname_found == 0  || valid_addr_found == 0 ) {
        UpnpPrintf( UPNP_CRITICAL, API, __FILE__, __LINE__,
            "Failed to find an adapter with valid IP addresses for use.\n" );
        return UPNP_E_INVALID_INTERFACE;
    }

    inet_ntop(AF_INET, &v4_addr, gIF_IPV4, sizeof(gIF_IPV4));
    inet_ntop(AF_INET6, &v6_addr, gIF_IPV6, sizeof(gIF_IPV6));
#elif (defined(BSD) && BSD >= 199306)
    struct ifaddrs *ifap, *ifa;
    struct in_addr v4_addr;
    struct in6_addr v6_addr;
    int ifname_found = 0;
    int valid_addr_found = 0;

    // Copy interface name, if it was provided.
    if( IfName != NULL ) {
        if( strlen(IfName) > sizeof(gIF_NAME) )
            return UPNP_E_INVALID_INTERFACE;

        strncpy( gIF_NAME, IfName, sizeof(gIF_NAME) );
        ifname_found = 1;
    }

    // Get system interface addresses.
    if( getifaddrs(&ifap) != 0 ) {
        UpnpPrintf( UPNP_CRITICAL, API, __FILE__, __LINE__,
            "getifaddrs failed to find list of addresses\n" );
        return UPNP_E_INIT;
    }

    // cycle through available interfaces and their addresses.
    for (ifa = ifap; ifa != NULL; ifa = ifa->ifa_next)
    {
        // Skip LOOPBACK interfaces, DOWN interfaces and interfaces that 
        // don't support MULTICAST.
        if( ( ifa->ifa_flags & IFF_LOOPBACK )
            || ( !( ifa->ifa_flags & IFF_UP ) ) 
            || ( !( ifa->ifa_flags & IFF_MULTICAST ) ) ) {
            continue;
        }

        if( ifname_found == 0 ) {
            // We have found a valid interface name. Keep it.
            strncpy( gIF_NAME, ifa->ifa_name, sizeof(gIF_NAME) );
            ifname_found = 1;
        } else {
            if( strncmp( gIF_NAME, ifa->ifa_name, sizeof(gIF_NAME) ) != 0 ) {
                // This is not the interface we're looking for.
                continue;
            }
        }

        // Keep interface addresses for later.
        switch( ifa->ifa_addr->sa_family )
        {
        case AF_INET:
            memcpy( &v4_addr, &((struct sockaddr_in *)(ifa->ifa_addr))->sin_addr, sizeof(v4_addr) );
            valid_addr_found = 1;
            break;
        case AF_INET6:
            // Only keep IPv6 link-local addresses.
            if( IN6_IS_ADDR_LINKLOCAL(&((struct sockaddr_in6 *)(ifa->ifa_addr))->sin6_addr) ) {
                memcpy( &v6_addr, &((struct sockaddr_in6 *)(ifa->ifa_addr))->sin6_addr, sizeof(v6_addr) );
                valid_addr_found = 1;
            }
            break;
        default:
            if( valid_addr_found == 0 ) {
                // Address is not IPv4 or IPv6 and no valid address has 
                // yet been found for this interface. Discard interface name.
                ifname_found = 0;
            }
            break;
        }
    }
    freeifaddrs(ifap);

    // Failed to find a valid interface, or valid address.
    if( ifname_found == 0  || valid_addr_found == 0 ) {
        UpnpPrintf( UPNP_CRITICAL, API, __FILE__, __LINE__,
            "Failed to find an adapter with valid IP addresses for use.\n" );
        return UPNP_E_INVALID_INTERFACE;
    }

    inet_ntop(AF_INET, &v4_addr, gIF_IPV4, sizeof(gIF_IPV4));
    inet_ntop(AF_INET6, &v6_addr, gIF_IPV6, sizeof(gIF_IPV6));
    gIF_INDEX = if_nametoindex(gIF_NAME);
#else
    char szBuffer[MAX_INTERFACES * sizeof( struct ifreq )];
    struct ifconf ifConf;
    struct ifreq ifReq;
    FILE* inet6_procfd;
    int i;
    int LocalSock;
    struct in6_addr v6_addr;
    int if_idx;
    char addr6[8][5];
    char buf[65]; // INET6_ADDRSTRLEN
    int ifname_found = 0;
    int valid_addr_found = 0;

    // Copy interface name, if it was provided.
    if( IfName != NULL ) {
        if( strlen(IfName) > sizeof(gIF_NAME) )
            return UPNP_E_INVALID_INTERFACE;

        strncpy( gIF_NAME, IfName, sizeof(gIF_NAME) );
        ifname_found = 1;
    }

    // Create an unbound datagram socket to do the SIOCGIFADDR ioctl on. 
    if( ( LocalSock = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP ) ) < 0 ) {
        UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
            "Can't create addrlist socket\n" );
        return UPNP_E_INIT;
    }

    // Get the interface configuration information... 
    ifConf.ifc_len = sizeof szBuffer;
    ifConf.ifc_ifcu.ifcu_buf = ( caddr_t ) szBuffer;

    if( ioctl( LocalSock, SIOCGIFCONF, &ifConf ) < 0 ) {
        UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
            "DiscoverInterfaces: SIOCGIFCONF returned error\n" );
        return UPNP_E_INIT;
    }

    // Cycle through the list of interfaces looking for IP addresses. 
    for( i = 0; i < ifConf.ifc_len ; ) {
        struct ifreq *pifReq =
            ( struct ifreq * )( ( caddr_t ) ifConf.ifc_req + i );
        i += sizeof *pifReq;

        // See if this is the sort of interface we want to deal with.
        strcpy( ifReq.ifr_name, pifReq->ifr_name );
        if( ioctl( LocalSock, SIOCGIFFLAGS, &ifReq ) < 0 ) {
            UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
                "Can't get interface flags for %s:\n",
                ifReq.ifr_name );
        }

        // Skip LOOPBACK interfaces, DOWN interfaces and interfaces that 
        // don't support MULTICAST.
        if( ( ifReq.ifr_flags & IFF_LOOPBACK )
            || ( !( ifReq.ifr_flags & IFF_UP ) ) 
            || ( !( ifReq.ifr_flags & IFF_MULTICAST ) ) ) {
            continue;
        }

        if( ifname_found == 0 ) {
            // We have found a valid interface name. Keep it.
            strncpy( gIF_NAME, pifReq->ifr_name, sizeof(gIF_NAME) );
            ifname_found = 1;
        } else {
            if( strncmp( gIF_NAME, pifReq->ifr_name, sizeof(gIF_NAME) ) != 0 ) {
                // This is not the interface we're looking for.
                continue;
            }
        }

        // Check address family.
        if( pifReq->ifr_addr.sa_family == AF_INET ) {
            // Copy interface name, IPv4 address and interface index.
            strncpy( gIF_NAME, pifReq->ifr_name, sizeof(gIF_NAME) );
            inet_ntop(AF_INET, &((struct sockaddr_in*)&pifReq->ifr_addr)->sin_addr, gIF_IPV4, sizeof(gIF_IPV4));
            gIF_INDEX = if_nametoindex(gIF_NAME);

            valid_addr_found = 1;
            break;
        } else {
            // Address is not IPv4
            ifname_found = 0;
        }
    }
    close( LocalSock );

    // Failed to find a valid interface, or valid address.
    if( ifname_found == 0  || valid_addr_found == 0 ) {
        UpnpPrintf( UPNP_CRITICAL, API, __FILE__, __LINE__,
            "Failed to find an adapter with valid IP addresses for use.\n" );
        return UPNP_E_INVALID_INTERFACE;
    }
    
    // Try to get the IPv6 address for the same interface 
    // from "/proc/net/if_inet6", if possible.
    inet6_procfd = fopen( "/proc/net/if_inet6", "r" );
    if( inet6_procfd != NULL ) {
        while( fscanf(inet6_procfd,
                "%4s%4s%4s%4s%4s%4s%4s%4s %02x %*02x %*02x %*02x %*20s\n",
                addr6[0],addr6[1],addr6[2],addr6[3],
                addr6[4],addr6[5],addr6[6],addr6[7], &if_idx) != EOF) {
            // Get same interface as IPv4 address retrieved.
            if( gIF_INDEX == if_idx ) {
                snprintf(buf, sizeof(buf), "%s:%s:%s:%s:%s:%s:%s:%s",
                    addr6[0],addr6[1],addr6[2],addr6[3],
                    addr6[4],addr6[5],addr6[6],addr6[7]);
                // Validate formed address and check for link-local.
                if( inet_pton(AF_INET6, buf, &v6_addr) > 0 &&
                    IN6_IS_ADDR_LINKLOCAL(&v6_addr) ) {
                    // Got valid IPv6 link-local adddress.
                    strncpy( gIF_IPV6, buf, sizeof(gIF_IPV6) );
                    break;
                }
            }
        }
        fclose( inet6_procfd );
    }
#endif

    UpnpPrintf( UPNP_INFO, API, __FILE__, __LINE__, 
                "Interface name=%s, index=%d, v4=%s, v6=%s\n",
                gIF_NAME, gIF_INDEX, gIF_IPV4, gIF_IPV6 );

    return UPNP_E_SUCCESS;
}


/*!
 * \brief Schedule async functions in threadpool.
 */
#ifdef INCLUDE_CLIENT_APIS
void UpnpThreadDistribution(struct UpnpNonblockParam *Param)
{
	int errCode = 0;

	UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
		"Inside UpnpThreadDistribution \n" );

	switch ( Param->FunName ) {
#if EXCLUDE_GENA == 0
	case SUBSCRIBE: {
		UpnpEventSubscribe *evt = UpnpEventSubscribe_new();
		// cast away constness
		UpnpString *sid = (UpnpString *)UpnpEventSubscribe_get_SID(evt);
		UpnpEventSubscribe_strcpy_PublisherUrl(evt, Param->Url);
		errCode = genaSubscribe(
			Param->Handle,
			UpnpEventSubscribe_get_PublisherUrl(evt),
        	        (int *)&(Param->TimeOut),
	                sid);
		UpnpEventSubscribe_set_ErrCode(evt, errCode);
		UpnpEventSubscribe_set_TimeOut(evt, Param->TimeOut);
		Param->Fun(UPNP_EVENT_SUBSCRIBE_COMPLETE, evt, Param->Cookie);
	    	UpnpEventSubscribe_delete(evt);
		free(Param);
		break;
        }
        case UNSUBSCRIBE: {
		UpnpEventSubscribe *evt = UpnpEventSubscribe_new();
		UpnpEventSubscribe_strcpy_SID(evt, Param->SubsId);
		errCode = genaUnSubscribe(
			Param->Handle,
			UpnpEventSubscribe_get_SID(evt));
		UpnpEventSubscribe_set_ErrCode(evt, errCode);
		UpnpEventSubscribe_strcpy_PublisherUrl(evt, "");
		UpnpEventSubscribe_set_TimeOut(evt, 0);
		Param->Fun(UPNP_EVENT_UNSUBSCRIBE_COMPLETE, evt, Param->Cookie);
	    	UpnpEventSubscribe_delete(evt);
		free(Param);
		break;
        }
        case RENEW: {
		UpnpEventSubscribe *evt = UpnpEventSubscribe_new();
		UpnpEventSubscribe_strcpy_SID(evt, Param->SubsId);
		errCode = genaRenewSubscription(
			Param->Handle,
			UpnpEventSubscribe_get_SID(evt),
			&(Param->TimeOut));
		UpnpEventSubscribe_set_ErrCode(evt, errCode);
		UpnpEventSubscribe_set_TimeOut(evt, Param->TimeOut);
		Param->Fun(UPNP_EVENT_RENEWAL_COMPLETE, evt, Param->Cookie);
	    	UpnpEventSubscribe_delete(evt);
		free(Param);
		break;
        }
#endif // EXCLUDE_GENA == 0
#if EXCLUDE_SOAP == 0
        case ACTION: {
            UpnpActionComplete *Evt = UpnpActionComplete_new();
	    IXML_Document *actionResult = NULL;
	    int errCode = SoapSendAction(
                Param->Url, Param->ServiceType, Param->Act, &actionResult );
            UpnpActionComplete_set_ErrCode(Evt, errCode);
            UpnpActionComplete_set_ActionRequest(Evt, Param->Act);
            UpnpActionComplete_set_ActionResult(Evt, actionResult);
            UpnpActionComplete_strcpy_CtrlUrl(Evt, Param->Url );
            Param->Fun( UPNP_CONTROL_ACTION_COMPLETE, Evt, Param->Cookie );
            free(Param);
            UpnpActionComplete_delete(Evt);
            break;
        }
        case STATUS: {
                UpnpStateVarComplete *Evt = UpnpStateVarComplete_new();
		DOMString currentVal = NULL;
		int errCode = SoapGetServiceVarStatus(
                    Param->Url, Param->VarName, &currentVal );
                UpnpStateVarComplete_set_ErrCode(Evt, errCode);
                UpnpStateVarComplete_strcpy_CtrlUrl(Evt, Param->Url);
                UpnpStateVarComplete_strcpy_StateVarName(Evt, Param->VarName);
                UpnpStateVarComplete_set_CurrentVal(Evt, currentVal);
                Param->Fun( UPNP_CONTROL_GET_VAR_COMPLETE, Evt, Param->Cookie );
                free( Param );
                UpnpStateVarComplete_delete(Evt);
                break;
            }
#endif // EXCLUDE_SOAP == 0
        default:
            break;
    } // end of switch(Param->FunName)

    UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
        "Exiting UpnpThreadDistribution \n" );

}
#endif // INCLUDE_CLIENT_APIS


/*!
 * \brief Get callback function ptr from a handle.
 *
 * \return Upnp_FunPtr
 */
Upnp_FunPtr GetCallBackFn(UpnpClient_Handle Hnd)
{
	return ((struct Handle_Info *)HandleTable[Hnd])->Callback;
}


void InitHandleList()
{
	int i;

	for (i = 0; i < NUM_HANDLE; ++i) {
		HandleTable[i] = NULL;
	}
}


int GetFreeHandle()
{
	/* Handle 0 is not used as NULL translates to 0 when passed as a handle */
	int i = 1;

	while (i < NUM_HANDLE && HandleTable[i] != NULL) {
		++i;
	}

	if (i == NUM_HANDLE) {
		return UPNP_E_OUTOF_HANDLE;
	} else {
		return i;
	}
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
	// Check if we've got a registered device of the address family specified.
	if ((AddressFamily == AF_INET  && UpnpSdkDeviceRegisteredV4 == 0) ||
	    (AddressFamily == AF_INET6 && UpnpSdkDeviceregisteredV6 == 0)) {
		*device_handle_out = -1;
		return HND_INVALID;
	}

	// Find it.
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


int FreeHandle(int Upnp_Handle)
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
                    UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
                        "DescURL_%s\n", HndInfo->DescURL );
#endif
    } else {
        return UPNP_E_INVALID_HANDLE;
    }

    return UPNP_E_SUCCESS;
}


void printNodes(IXML_Node *tmpRoot, int depth)
{
    int i;
    IXML_NodeList *NodeList1;
    IXML_Node *ChildNode1;
    unsigned short NodeType;
    const DOMString NodeValue;
    const DOMString NodeName;
    NodeList1 = ixmlNode_getChildNodes(tmpRoot);
    for (i = 0; i < 100; ++i) {
        ChildNode1 = ixmlNodeList_item(NodeList1, i);
        if (ChildNode1 == NULL) {
            break;
        }
    
        printNodes(ChildNode1, depth+1);
        NodeType = ixmlNode_getNodeType(ChildNode1);
        NodeValue = ixmlNode_getNodeValue(ChildNode1);
        NodeName = ixmlNode_getNodeName(ChildNode1);
        UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
            "DEPTH-%2d-IXML_Node Type %d, "
            "IXML_Node Name: %s, IXML_Node Value: %s\n",
            depth, NodeType, NodeName, NodeValue);
    }
}


int getlocalhostname(char *out, const int out_len)
{
#ifdef WIN32
	struct hostent *h=NULL;
	struct sockaddr_in LocalAddr;

 		gethostname(out, out_len);
 		h=gethostbyname(out);
 		if (h!=NULL){
 			memcpy(&LocalAddr.sin_addr,h->h_addr_list[0],4);
 			strncpy(out, inet_ntoa(LocalAddr.sin_addr), out_len);
 		}
 		return UPNP_E_SUCCESS;
#elif (defined(BSD) && BSD >= 199306)
    struct ifaddrs *ifap, *ifa;

    if (getifaddrs(&ifap) != 0) {
        UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
            "DiscoverInterfaces: getifaddrs() returned error\n" );
        return UPNP_E_INIT;
    }

    // cycle through available interfaces
    for (ifa = ifap; ifa != NULL; ifa = ifa->ifa_next) {
        // Skip loopback, point-to-point and down interfaces, 
        // except don't skip down interfaces
        // if we're trying to get a list of configurable interfaces. 
        if( ( ifa->ifa_flags & IFF_LOOPBACK )
            || ( !( ifa->ifa_flags & IFF_UP ) ) ) {
            continue;
        }
        if( ifa->ifa_addr->sa_family == AF_INET ) {
            // We don't want the loopback interface. 
            if( ((struct sockaddr_in *)(ifa->ifa_addr))->sin_addr.s_addr ==
                htonl( INADDR_LOOPBACK ) ) {
                continue;
            }

            strncpy( out, inet_ntoa( ((struct sockaddr_in *)(ifa->ifa_addr))->
                sin_addr ), out_len );
            out[LINE_SIZE-1] = '\0';
            UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
                "Inside getlocalhostname : after strncpy %s\n",
                out );
            break;
        }
    }
    freeifaddrs(ifap);

    return ifa ? UPNP_E_SUCCESS : UPNP_E_INIT;
#else
    char szBuffer[MAX_INTERFACES * sizeof( struct ifreq )];
    struct ifconf ifConf;
    struct ifreq ifReq;
    int nResult;
    int i;
    int LocalSock;
    struct sockaddr_in LocalAddr;
    int j = 0;

    // Create an unbound datagram socket to do the SIOCGIFADDR ioctl on. 
    if( ( LocalSock = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP ) ) < 0 ) {
        UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
            "Can't create addrlist socket\n" );
        return UPNP_E_INIT;
    }
    // Get the interface configuration information... 
    ifConf.ifc_len = sizeof szBuffer;
    ifConf.ifc_ifcu.ifcu_buf = ( caddr_t ) szBuffer;
    nResult = ioctl( LocalSock, SIOCGIFCONF, &ifConf );

    if( nResult < 0 ) {
        UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
            "DiscoverInterfaces: SIOCGIFCONF returned error\n" );

        return UPNP_E_INIT;
    }
    // Cycle through the list of interfaces looking for IP addresses. 

    for( i = 0; ( ( i < ifConf.ifc_len ) && ( j < DEFAULT_INTERFACE ) ); ) {
        struct ifreq *pifReq =
            ( struct ifreq * )( ( caddr_t ) ifConf.ifc_req + i );
        i += sizeof *pifReq;

        // See if this is the sort of interface we want to deal with.
        strcpy( ifReq.ifr_name, pifReq->ifr_name );
        if( ioctl( LocalSock, SIOCGIFFLAGS, &ifReq ) < 0 ) {
            UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
                "Can't get interface flags for %s:\n",
                ifReq.ifr_name );
        }
        // Skip loopback, point-to-point and down interfaces, 
        // except don't skip down interfaces
        // if we're trying to get a list of configurable interfaces. 
        if( ( ifReq.ifr_flags & IFF_LOOPBACK )
            || ( !( ifReq.ifr_flags & IFF_UP ) ) ) {
            continue;
        }
        if( pifReq->ifr_addr.sa_family == AF_INET ) {
            // Get a pointer to the address...
            memcpy( &LocalAddr, &pifReq->ifr_addr,
                    sizeof pifReq->ifr_addr );

            // We don't want the loopback interface. 
            if( LocalAddr.sin_addr.s_addr == htonl( INADDR_LOOPBACK ) ) {
                continue;
            }

        }
        //increment j if we found an address which is not loopback
        //and is up
        j++;

    }
    close( LocalSock );

    strncpy( out, inet_ntoa( LocalAddr.sin_addr ), out_len );

    UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
        "Inside getlocalhostname : after strncpy %s\n",
        out );
    return UPNP_E_SUCCESS;
#endif
}


#ifdef INCLUDE_DEVICE_APIS
#if EXCLUDE_SSDP == 0
void AutoAdvertise(void *input)
{
	upnp_timeout *event = (upnp_timeout *)input;

	UpnpSendAdvertisement(event->handle, *((int *)event->Event));
	free_upnp_timeout(event);
}
#endif // EXCLUDE_SSDP == 0
#endif // INCLUDE_DEVICE_APIS


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
#endif // INTERNAL_WEB_SERVER


int UpnpAddVirtualDir(const char *newDirName)
{
    virtualDirList *pNewVirtualDir;
    virtualDirList *pLast;
    virtualDirList *pCurVirtualDir;
    char dirName[NAME_SIZE];

    if( UpnpSdkInit != 1 ) {
        // SDK is not initialized
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
        // already has this entry
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

    if( pVirtualDirList == NULL ) { // first virtual dir
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
    //
    // Handle the special case where the directory that we are
    // removing is the first and only one in the list.
    //

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


void UpnpRemoveAllVirtualDirs()
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
#endif
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
int UpnpIsWebserverEnabled()
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


int UpnpSetContentLength(
	UpnpClient_Handle Hnd,
	int contentLength)
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
	} while(0);

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

