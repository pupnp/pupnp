/*******************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation 
 * All rights reserved. 
 * Copyright (C) 2011-2012 France Telecom All rights reserved. 
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


#ifndef UPNPAPI_H
#define UPNPAPI_H


/*!
 * \file
 */


#include "client_table.h"
#include "upnp.h"
#include "VirtualDir.h"		/* for struct VirtualDirCallbacks */


#define MAX_INTERFACES 256

#define DEFAULT_INTERFACE 1

#define DEV_LIMIT 200

#define DEFAULT_MX 5

#define DEFAULT_MAXAGE 1800

#define DEFAULT_SOAP_CONTENT_LENGTH 16000
#define MAX_SOAP_CONTENT_LENGTH (size_t)32000

extern size_t g_maxContentLength;

/* 30-second timeout */
#define UPNP_TIMEOUT	30

typedef enum {HND_INVALID=-1,HND_CLIENT,HND_DEVICE} Upnp_Handle_Type;

/* Data to be stored in handle table for */
struct Handle_Info
{
	/*! . */
	Upnp_Handle_Type HType;
	/*! Callback function pointer. */
	Upnp_FunPtr  Callback;
	/*! . */
	char *Cookie;
	/*! 0 = not installed; otherwise installed. */
	int   aliasInstalled;

	/* Device Only */
#ifdef INCLUDE_DEVICE_APIS
	/*! URL for the use of SSDP. */
	char  DescURL[LINE_SIZE];
	/*! URL for the use of SSDP when answering to legacy CPs (CP searching
	 * for a v1 when the device is v2). */
	char  LowerDescURL[LINE_SIZE];
	/*! XML file path for device description. */
	char  DescXML[LINE_SIZE];
	/* Advertisement timeout */
	int MaxAge;
	/* Power State as defined by UPnP Low Power. */
	int PowerState;
	/* Sleep Period as defined by UPnP Low Power. */
	int SleepPeriod;
	/* Registration State as defined by UPnP Low Power. */
	int RegistrationState;
	/*! Description parsed in terms of DOM document. */
	IXML_Document *DescDocument;
	/*! List of devices in the description document. */
	IXML_NodeList *DeviceList;
	/*! List of services in the description document. */
	IXML_NodeList *ServiceList;
	/*! Table holding subscriptions and URL information. */
	service_table ServiceTable;
	/*! . */
	int MaxSubscriptions;
	/*! . */
	int MaxSubscriptionTimeOut;
	/*! Address family: AF_INET or AF_INET6. */
	int DeviceAf;
#endif

	/* Client only */
#ifdef INCLUDE_CLIENT_APIS
	/*! Client subscription list. */
	GenlibClientSubscription *ClientSubList;
	/*! Active SSDP searches. */
	LinkedList SsdpSearchList;
#endif
};

extern ithread_rwlock_t GlobalHndRWLock;

/*!
 * \brief Get handle information.
 *
 * \return HND_DEVICE, UPNP_E_INVALID_HANDLE
 */
Upnp_Handle_Type GetHandleInfo(
	/*! handle pointer (key for the client handle structure). */
	int Hnd,
	/*! handle structure passed by this function. */
	struct Handle_Info **HndInfo); 


#define HandleLock() HandleWriteLock()


#define HandleWriteLock()  \
	UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__, "Trying a write lock"); \
	ithread_rwlock_wrlock(&GlobalHndRWLock); \
	UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__, "Write lock acquired");


#define HandleReadLock()  \
	UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__, "Trying a read lock"); \
	ithread_rwlock_rdlock(&GlobalHndRWLock); \
	UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__, "Read lock acquired");


#define HandleUnlock() \
	UpnpPrintf(UPNP_INFO, API,__FILE__, __LINE__, "Trying Unlock"); \
	ithread_rwlock_unlock(&GlobalHndRWLock); \
	UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__, "Unlocked rwlock");


/*!
 * \brief Get client handle info.
 *
 * \note The logic around the use of this function should be revised.
 *
 * \return HND_CLIENT, HND_INVALID
 */
Upnp_Handle_Type GetClientHandleInfo(
	/*! [in] client handle pointer (key for the client handle structure). */
	int *client_handle_out, 
	/*! [out] Client handle structure passed by this function. */
	struct Handle_Info **HndInfo);
/*!
 * \brief Retrieves the device handle and information of the first device of
 * 	the address family spcified.
 *
 * \return HND_DEVICE or HND_INVALID
 */
Upnp_Handle_Type GetDeviceHandleInfo(
	/*! [in] Address family. */
	int AddressFamily,
	/*! [out] Device handle pointer. */
	int *device_handle_out, 
	/*! [out] Device handle structure passed by this function. */
	struct Handle_Info **HndInfo);


extern char gIF_NAME[LINE_SIZE];
extern char gIF_IPV4[INET_ADDRSTRLEN];
extern char gIF_IPV6[INET6_ADDRSTRLEN];

extern char gIF_IPV6_ULA_GUA[INET6_ADDRSTRLEN];

extern unsigned gIF_INDEX;


extern unsigned short LOCAL_PORT_V4;
extern unsigned short LOCAL_PORT_V6;


/*! NLS uuid. */
extern Upnp_SID gUpnpSdkNLSuuid;


extern TimerThread gTimerThread;
extern ThreadPool gRecvThreadPool;
extern ThreadPool gSendThreadPool;
extern ThreadPool gMiniServerThreadPool;


typedef enum {
	SUBSCRIBE,
	UNSUBSCRIBE,
	DK_NOTIFY,
	QUERY,
	ACTION,
	STATUS,
	DEVDESCRIPTION,
	SERVDESCRIPTION,
	MINI,
	RENEW
} UpnpFunName;


struct  UpnpNonblockParam 
{
	UpnpFunName FunName;
	int Handle;
	int TimeOut;
	char VarName[NAME_SIZE];
	char NewVal[NAME_SIZE];
	char DevType[NAME_SIZE];
	char DevId[NAME_SIZE];
	char ServiceType[NAME_SIZE];
	char ServiceVer[NAME_SIZE];
	char Url[NAME_SIZE];
	Upnp_SID SubsId;
	char *Cookie;
	Upnp_FunPtr Fun;
	IXML_Document *Header;
	IXML_Document *Act;
	struct DevDesc *Devdesc;
};


extern virtualDirList *pVirtualDirList;
extern struct VirtualDirCallbacks virtualDirCallback;


typedef enum {
	WEB_SERVER_DISABLED,
	WEB_SERVER_ENABLED
} WebServerState;


#define E_HTTP_SYNTAX -6


/*!
 * \brief Retrieve interface information and keep it in global variables.
 * If NULL, we'll find the first suitable interface for operation.
 *
 * The interface must fulfill these requirements:
 * \li Be UP.
 * \li Not be LOOPBACK.
 * \li Support MULTICAST.
 * \li Have a valid IPv4 or IPv6 address.
 *
 * We'll retrieve the following information from the interface:
 * \li gIF_NAME -> Interface name (by input or found).
 * \li gIF_IPV4 -> IPv4 address (if any).
 * \li gIF_IPV6 -> IPv6 address (if any).
 * \li gIF_IPV6_ULA_GUA -> ULA or GUA IPv6 address (if any)
 * \li gIF_INDEX -> Interface index number.
 *
 * \return UPNP_E_SUCCESS on success.
 */
int UpnpGetIfInfo(
	/*! [in] Interface name (can be NULL). */
	const char *IfName);


void UpnpThreadDistribution(struct UpnpNonblockParam * Param);


/*!
 * \brief This function is a timer thread scheduled by UpnpSendAdvertisement
 * to the send advetisement again.
 */
void AutoAdvertise(
	/*! [in] Information provided to the thread. */
	void *input); 


/*!
 * \brief Get local IP address.
 *
 * Gets the ip address for the DEFAULT_INTERFACE interface which is up and not
 * a loopback. Assumes at most MAX_INTERFACES interfaces
 *
 * \return UPNP_E_SUCCESS  if successful or UPNP_E_INIT.
 */
int getlocalhostname(
	/*! [out] IP address of the interface. */
	char *out,
	/*! [in] Length of the output buffer. */
	size_t out_len);


/*!
 * \brief Print handle info.
 *	
 * \return UPNP_E_SUCCESS if successful, otherwise returns appropriate error.
 */
int PrintHandleInfo(
	/*! [in] Handle index. */
	UpnpClient_Handle Hnd);


extern WebServerState bWebServerState;


#endif /* UPNPAPI_H */

