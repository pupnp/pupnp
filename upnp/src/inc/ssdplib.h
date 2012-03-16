#ifndef SSDPLIB_H
#define SSDPLIB_H 

/**************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation 
 * All rights reserved. 
 * Copyright (C) 2011-2012 France Telecom All rights reserved. 
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

/*!
 * \defgroup SSDPlib SSDP Library
 *
 * @{
 *
 * \file
 */

#include "httpparser.h"
#include "httpreadwrite.h"
#include "miniserver.h"
#include "UpnpInet.h"

#include <sys/types.h>
#include <signal.h>
#include <setjmp.h>
#include <errno.h>

#ifdef WIN32
#else /* WIN32 */
	#include <syslog.h>
	#ifndef __APPLE__
		#include <netinet/in_systm.h>
		#include <netinet/ip.h>
		#include <netinet/ip_icmp.h>
	#endif /* __APPLE__ */
	#include <sys/time.h>
#endif /* WIN32 */

/*! Enumeration to define all different types of ssdp searches */
typedef enum SsdpSearchType {
	/*! Unknown search command. */
	SSDP_SERROR = -1,
	SSDP_ALL,
	SSDP_ROOTDEVICE,
	SSDP_DEVICEUDN,
	SSDP_DEVICETYPE,
	SSDP_SERVICE
} SType;

#define BUFSIZE   (size_t)2500
#define SSDP_IP   "239.255.255.250"
#define SSDP_IPV6_LINKLOCAL "FF02::C"
#define SSDP_IPV6_SITELOCAL "FF05::C"
#define SSDP_PORT 1900
#define NUM_TRY 3
#define THREAD_LIMIT 50
#define COMMAND_LEN  300

/*! can be overwritten by configure CFLAGS argument. */
#ifndef X_USER_AGENT
	/*! @name X_USER_AGENT
	 *  The {\tt X_USER_AGENT} constant specifies the value of the X-User-Agent:
	 *  HTTP header. The value "redsonic" is needed for the DSM-320. See
	 *  https://sourceforge.net/forum/message.php?msg_id=3166856 for more
	 * information
	 */
	#define X_USER_AGENT "redsonic"
#endif

/*! Error codes. */
#define NO_ERROR_FOUND    0
#define E_REQUEST_INVALID  	-3
#define E_RES_EXPIRED		-4
#define E_MEM_ALLOC		-5
#define E_HTTP_SYNTEX		-6
#define E_SOCKET 		-7

#define RQST_TIMEOUT    20

/*! Structure to store the SSDP information */
typedef struct SsdpEventStruct {
	enum SsdpSearchType RequestType;
	int  ErrCode;
	int  MaxAge;
	int  Mx;
	char UDN[LINE_SIZE];
	char DeviceType[LINE_SIZE];
	/* NT or ST */
	char ServiceType[LINE_SIZE];
	char Location[LINE_SIZE];
	char HostAddr[LINE_SIZE];
	char Os[LINE_SIZE];
	char Ext[LINE_SIZE];
	char Date[LINE_SIZE];
	struct sockaddr *DestAddr;
	void * Cookie;
} SsdpEvent;

typedef void (* SsdpFunPtr)(SsdpEvent *);

typedef struct TData
{
	int Mx;
	void * Cookie;
	char * Data;
	struct sockaddr_storage DestAddr;
} ThreadData;

typedef struct ssdpsearchreply
{
	int MaxAge;
	UpnpDevice_Handle handle;
	struct sockaddr_storage dest_addr;
	SsdpEvent event;
} SsdpSearchReply;

typedef struct ssdpsearcharg
{
	int timeoutEventId;
	char * searchTarget;
	void *cookie;
	enum SsdpSearchType requestType;
} SsdpSearchArg;


typedef struct 
{
	http_parser_t parser;
	struct sockaddr_storage dest_addr;
} ssdp_thread_data;

/* globals */

#ifdef INCLUDE_CLIENT_APIS
	extern SOCKET gSsdpReqSocket4;
	#ifdef UPNP_ENABLE_IPV6
		extern SOCKET gSsdpReqSocket6;
	#endif /* UPNP_ENABLE_IPV6 */
#endif /* INCLUDE_CLIENT_APIS */
typedef int (*ParserFun)(char *, SsdpEvent *);

/*!
 * \name SSDP Server Functions
 *
 * @{
 */

/*!
 * \brief Sends SSDP advertisements, replies and shutdown messages.
 *
 * \return UPNP_E_SUCCESS if successful else appropriate error.
 */
int AdvertiseAndReply(
	/* [in] -1 = Send shutdown, 0 = send reply, 1 = Send Advertisement. */
	int AdFlag, 
	/* [in] Device handle. */
	UpnpDevice_Handle Hnd, 
	/* [in] Search type for sending replies. */
	enum SsdpSearchType SearchType, 
	/* [in] Destination address. */
	struct sockaddr *DestAddr,
	/* [in] Device type. */
	char *DeviceType, 
	/* [in] Device UDN. */
	char *DeviceUDN, 
	/* [in] Service type. */
	char *ServiceType,
	/* [in] Advertisement age. */
	int Exp);

/*!
 * \brief Fills the fields of the event structure like DeviceType, Device UDN
 * and Service Type.
 *
 * \return  0 if successful else -1.
 */
int unique_service_name(
	/* [in] Service Name string. */
	char *cmd,
	/* [out] The SSDP event structure partially filled by all the
	 * function. */
	SsdpEvent *Evt);

/*!
 * \brief This function figures out the type of the SSDP search in the in the
 * request.
 *
 * \return enum SsdpSearchType. Returns appropriate search type,
 * else returns SSDP_ERROR
 */
enum SsdpSearchType ssdp_request_type1(
	/* [in] command came in the ssdp request. */
	char *cmd);

/*!
 * \brief Starts filling the SSDP event structure based upon the
 * request received.
 *
 * \return 0 on success; -1 on error.
 */
int ssdp_request_type(
	/* [in] command came in the ssdp request. */
	char *cmd,
	/* [out] The event structure partially filled by this function. */
	SsdpEvent *Evt);

/*!
 * \brief This function reads the data from the ssdp socket.
 */
void readFromSSDPSocket(
	/* [in] SSDP socket. */
	SOCKET socket);

/*!
 * \brief Creates the IPv4 and IPv6 ssdp sockets required by the
 *  control point and device operation.
 *
 * \return UPNP_E_SUCCESS if successful else returns appropriate error.
 */
int get_ssdp_sockets(
	/* [out] Array of SSDP sockets. */
	MiniServerSockArray *out);

/* @} SSDP Server Functions */

/*!
 * \name SSDP Control Point Functions
 *
 * @{
 */

/*!
 * \brief This function handles the ssdp messages from the devices. These
 * messages includes the search replies, advertisement of device coming alive
 * and bye byes.
 */
void ssdp_handle_ctrlpt_msg(
	/* [in] SSDP message from the device. */
	http_message_t *hmsg, 
	/* [in] Address of the device. */
	struct sockaddr_storage *dest_addr,
	/* [in] timeout kept by the control point while sending search message.
	 * Only in search reply. */
	int timeout,
	/* [in] Cookie stored by the control point application. This cookie will
	 * be returned to the control point in the callback.
	 * Only in search reply. */
	void *cookie);

/*!
 * \brief Creates and send the search request for a specific URL.
 *
 * This function implements the search request of the discovery phase.
 * A M-SEARCH request is sent on the SSDP channel for both IPv4 and
 * IPv6 addresses. The search target(ST) is required and must be one of
 * the following:
 *     \li "ssdp:all" : Search for all devices and services.
 *     \li "ssdp:rootdevice" : Search for root devices only.
 *     \li "uuid:<device-uuid>" : Search for a particular device.
 *     \li "urn:schemas-upnp-org:device:<deviceType:v>"
 *     \li "urn:schemas-upnp-org:service:<serviceType:v>"
 *     \li "urn:<domain-name>:device:<deviceType:v>"
 *     \li "urn:<domain-name>:service:<serviceType:v>"
 *
 * \return 1 if successful else appropriate error.
 */
int SearchByTarget(
	/* [in] Number of seconds to wait, to collect all the responses. */
	int Mx,
	/* [in] Search target. */
	char *St,
	/* [in] Cookie provided by control point application. This cokie will
	 * be returned to application in the callback. */
	void *Cookie);

/* @} SSDP Control Point Functions */

/*!
 * \name SSDP Device Functions
 *
 * @{
 */

/*!
 * \brief Wrapper function to reply the search request coming from the
 * control point.
 *
 * \return always return NULL
 */
void *advertiseAndReplyThread(
	/* [in] Structure containing the search request. */
	void *data);

/*!
 * \brief Handles the search request. It does the sanity checks of the
 * request and then schedules a thread to send a random time reply
 * (random within maximum time given by the control point to reply).
 */
#ifdef INCLUDE_DEVICE_APIS
void ssdp_handle_device_request(
	/* [in] . */
	http_message_t *hmsg, 
	/* [in] . */
	struct sockaddr_storage *dest_addr);
#else /* INCLUDE_DEVICE_APIS */
static UPNP_INLINE void ssdp_handle_device_request(
	/* [in] . */
	http_message_t *hmsg, 
	/* [in] . */
	struct sockaddr_storage *dest_addr) {}
#endif /* INCLUDE_DEVICE_APIS */

/*!
 * \brief Creates the device advertisement request based on the input
 * parameter, and send it to the multicast channel.
 *
 * \return UPNP_E_SUCCESS if successful else appropriate error.
 */
int DeviceAdvertisement(
	/* [in] type of the device. */
	char *DevType,
	/* [in] flag to indicate if the device is root device. */
	int RootDev,
	/* [in] UDN. */
	char *Udn, 
	/* [in] Location URL. */
	char *Location,
	/* [in] Service duration in sec. */
	int Duration,
	/* [in] Device address family. */
	int AddressFamily,
	/* [in] PowerState as defined by UPnP Low Power. */
	int PowerState,
	/* [in] SleepPeriod as defined by UPnP Low Power. */
	int SleepPeriod,
	/* [in] RegistrationState as defined by UPnP Low Power. */
	int RegistrationState);

/*!
 * \brief Creates the reply packet based on the input parameter, and send it
 * to the client addesss given in its input parameter DestAddr.
 *
 * \return UPNP_E_SUCCESS if successful else appropriate error.
 */
int SendReply(
	/* [in] destination IP address. */
	struct sockaddr *DestAddr, 
	/* [in] Device type. */
	char *DevType, 
	/* [in] 1 means root device 0 means embedded device. */
	int RootDev, 
	/* [in] Device UDN. */
	char *Udn, 
	/* [in] Location of Device description document. */
	char *Location, 
	/* [in] Life time of this device. */
	int Duration, 
	/* [in] . */
	int ByType,
	/* [in] PowerState as defined by UPnP Low Power. */
	int PowerState,
	/* [in] SleepPeriod as defined by UPnP Low Power. */
	int SleepPeriod,
	/* [in] RegistrationState as defined by UPnP Low Power. */
	int RegistrationState);

/*!
 * \brief Creates the reply packet based on the input parameter, and send it
 * to the client address given in its input parameter DestAddr.
 *
 * \return UPNP_E_SUCCESS if successful else appropriate error.
 */
int DeviceReply(
	/* [in] destination IP address. */
	struct sockaddr *DestAddr, 
	/* [in] Device type. */
	char *DevType, 
	/* [in] 1 means root device 0 means embedded device. */
	int RootDev, 
	/* [in] Device UDN. */
	char *Udn, 
	/* [in] Location of Device description document. */
	char *Location, 
	/* [in] Life time of this device. */
	int Duration,
	/* [in] PowerState as defined by UPnP Low Power. */
	int PowerState,
	/* [in] SleepPeriod as defined by UPnP Low Power. */
	int SleepPeriod,
	/* [in] RegistrationState as defined by UPnP Low Power. */
	int RegistrationState);

/*!
 * \brief Creates the advertisement packet based on the input parameter,
 * and send it to the multicast channel.
 *
 * \return UPNP_E_SUCCESS if successful else appropriate error.
 */
int ServiceAdvertisement(
	/* [in] Device UDN. */
	char *Udn, 
	/* [in] Service Type. */
	char *ServType,
	/* [in] Location of Device description document. */
	char *Location,
	/* [in] Life time of this device. */
	int Duration,
	/* [in] Device address family. */
	int AddressFamily,
	/* [in] PowerState as defined by UPnP Low Power. */
	int PowerState,
	/* [in] SleepPeriod as defined by UPnP Low Power. */
	int SleepPeriod,
	/* [in] RegistrationState as defined by UPnP Low Power. */
	int RegistrationState);

/*!
 * \brief Creates the advertisement packet based on the input parameter,
 * and send it to the multicast channel.
 *
 * \return UPNP_E_SUCCESS if successful else appropriate error.
 */
int ServiceReply(
	/* [in] . */
	struct sockaddr *DestAddr,  
	/* [in] Service Type. */
	char *ServType, 
	/* [in] Device UDN. */
	char *Udn, 
	/* [in] Location of Device description document. */
	char *Location,
	/* [in] Life time of this device. */
	int Duration,
	/* [in] PowerState as defined by UPnP Low Power. */
	int PowerState,
	/* [in] SleepPeriod as defined by UPnP Low Power. */
	int SleepPeriod,
	/* [in] RegistrationState as defined by UPnP Low Power. */
	int RegistrationState);

/*!
 * \brief Creates a HTTP service shutdown request packet and sends it to the
 * multicast channel through RequestHandler.
 *
 * \return UPNP_E_SUCCESS if successful else appropriate error.
 */
int ServiceShutdown(
	/* [in] Device UDN. */
	char *Udn,
	/* [in] Service Type. */
	char *ServType,
	/* [in] Location of Device description document. */
	char *Location,
	/* [in] Service duration in sec. */
	int Duration,
	/* [in] Device address family. */
	int AddressFamily,
	/* [in] PowerState as defined by UPnP Low Power. */
	int PowerState,
	/* [in] SleepPeriod as defined by UPnP Low Power. */
	int SleepPeriod,
	/* [in] RegistrationState as defined by UPnP Low Power. */
	int RegistrationState);

/*!
 * \brief Creates a HTTP device shutdown request packet and send it to the
 * multicast channel through RequestHandler.
 *
 * \return UPNP_E_SUCCESS if successful else appropriate error.
 */
int DeviceShutdown(
	/* [in] Device Type. */
	char *DevType, 
	/* [in] 1 means root device. */
	int RootDev,
	/* [in] Device UDN. */
	char *Udn, 
	/* [in] Location URL. */
	char *Location, 
	/* [in] Device duration in sec. */
	int Duration,
	/* [in] Device address family. */
	int AddressFamily,
	/* [in] PowerState as defined by UPnP Low Power. */
	int PowerState,
	/* [in] SleepPeriod as defined by UPnP Low Power. */
	int SleepPeriod,
	/* [in] RegistrationState as defined by UPnP Low Power. */
	int RegistrationState);

/* @} SSDP Device Functions */

/* @} SSDPlib SSDP Library */

#endif /* SSDPLIB_H */
