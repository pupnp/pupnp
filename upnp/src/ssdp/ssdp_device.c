/**************************************************************************
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
 **************************************************************************/


#include "config.h"


#ifdef INCLUDE_DEVICE_APIS
#if EXCLUDE_SSDP == 0


#include "httpparser.h"
#include "httpreadwrite.h"
#include "ssdplib.h"
#include "statcodes.h"
#include "ThreadPool.h"
#include "unixutil.h"
#include "upnpapi.h"
#include "UpnpInet.h"


#include <assert.h>
#include <stdio.h>
#include <string.h>


#define MSGTYPE_SHUTDOWN	0
#define MSGTYPE_ADVERTISEMENT	1
#define MSGTYPE_REPLY		2


/************************************************************************
* Function : advertiseAndReplyThread
*
* Parameters:
*	IN void *data: Structure containing the search request
*
* Description:
*	This function is a wrapper function to reply the search request
*	coming from the control point.
*
* Returns: void *
*	always return NULL
***************************************************************************/
void *
advertiseAndReplyThread( IN void *data )
{
    SsdpSearchReply *arg = ( SsdpSearchReply * ) data;

    AdvertiseAndReply( 0, arg->handle,
                       arg->event.RequestType,
                       (struct sockaddr*)&arg->dest_addr,
                       arg->event.DeviceType,
                       arg->event.UDN,
                       arg->event.ServiceType, arg->MaxAge );
    free( arg );

    return NULL;
}

/************************************************************************
* Function : ssdp_handle_device_request
*
* Parameters:
*	IN http_message_t *hmsg: SSDP search request from the control point
*	IN struct sockaddr *dest_addr: The address info of control point
*
* Description:
*	This function handles the search request. It do the sanity checks of
*	the request and then schedules a thread to send a random time reply (
*	random within maximum time given by the control point to reply).
*
* Returns: void *
*	1 if successful else appropriate error
***************************************************************************/
#ifdef INCLUDE_DEVICE_APIS
void ssdp_handle_device_request(
	IN http_message_t *hmsg,
	IN struct sockaddr *dest_addr)
{
#define MX_FUDGE_FACTOR 10

    int handle;
    struct Handle_Info *dev_info = NULL;
    memptr hdr_value;
    int mx;
    char save_char;
    SsdpEvent event;
    int ret_code;
    SsdpSearchReply *threadArg = NULL;
    ThreadPoolJob job;
    int replyTime;
    int maxAge;

    /* check man hdr. */
    if( httpmsg_find_hdr( hmsg, HDR_MAN, &hdr_value ) == NULL ||
        memptr_cmp( &hdr_value, "\"ssdp:discover\"" ) != 0 ) {
        /* bad or missing hdr. */
        return;
    }
    /* MX header. */
    if( httpmsg_find_hdr( hmsg, HDR_MX, &hdr_value ) == NULL ||
        ( mx = raw_to_int( &hdr_value, 10 ) ) < 0 ) {
        return;
    }
    /* ST header. */
    if( httpmsg_find_hdr( hmsg, HDR_ST, &hdr_value ) == NULL ) {
        return;
    }
    save_char = hdr_value.buf[hdr_value.length];
    hdr_value.buf[hdr_value.length] = '\0';
    ret_code = ssdp_request_type( hdr_value.buf, &event );
    /* restore. */
    hdr_value.buf[hdr_value.length] = save_char;
    if( ret_code == -1 ) {
        /* bad ST header. */
        return;
    }

    HandleLock();
    /* device info. */
    if( GetDeviceHandleInfo( dest_addr->sa_family, 
        &handle, &dev_info ) != HND_DEVICE ) {
        HandleUnlock();
        /* no info found. */
        return;
    }
    maxAge = dev_info->MaxAge;
    HandleUnlock();

    UpnpPrintf( UPNP_PACKET, API, __FILE__, __LINE__,
        "ssdp_handle_device_request with Cmd %d SEARCH\n",
        event.Cmd );
    UpnpPrintf( UPNP_PACKET, API, __FILE__, __LINE__,
        "MAX-AGE     =  %d\n", maxAge );
    UpnpPrintf( UPNP_PACKET, API, __FILE__, __LINE__,
        "MX     =  %d\n", event.Mx );
    UpnpPrintf( UPNP_PACKET, API, __FILE__, __LINE__,
        "DeviceType   =  %s\n", event.DeviceType );
    UpnpPrintf( UPNP_PACKET, API, __FILE__, __LINE__,
        "DeviceUuid   =  %s\n", event.UDN );
    UpnpPrintf( UPNP_PACKET, API, __FILE__, __LINE__,
        "ServiceType =  %s\n", event.ServiceType );

    threadArg =
        ( SsdpSearchReply * ) malloc( sizeof( SsdpSearchReply ) );

    if( threadArg == NULL ) {
        return;
    }
    threadArg->handle = handle;
    memcpy( &threadArg->dest_addr, dest_addr, sizeof(threadArg->dest_addr) );
    threadArg->event = event;
    threadArg->MaxAge = maxAge;

    TPJobInit( &job, advertiseAndReplyThread, threadArg );
    TPJobSetFreeFunction( &job, ( free_routine ) free );

    /* Subtract a percentage from the mx to allow for network and processing
     * delays (i.e. if search is for 30 seconds, respond
     * within 0 - 27 seconds). */
    if( mx >= 2 ) {
        mx -= MAXVAL( 1, mx / MX_FUDGE_FACTOR );
    }

    if( mx < 1 ) {
        mx = 1;
    }

    replyTime = rand() % mx;

    TimerThreadSchedule( &gTimerThread, replyTime, REL_SEC, &job,
                         SHORT_TERM, NULL );
}
#endif

/************************************************************************
* Function : NewRequestHandler
*
* Parameters:
*		IN struct sockaddr *DestAddr: Ip address, to send the reply.
*		IN int NumPacket: Number of packet to be sent.
*		IN char **RqPacket:Number of packet to be sent.
*
* Description:
*	This function works as a request handler which passes the HTTP
*	request string to multicast channel then
*
* Returns: void *
*	1 if successful else appropriate error
***************************************************************************/
static int
NewRequestHandler( IN struct sockaddr *DestAddr,
                   IN int NumPacket,
                   IN char **RqPacket )
{
    char errorBuffer[ERROR_BUFFER_LEN];
    SOCKET ReplySock;
    int socklen = sizeof( struct sockaddr_storage );
    int Index;
    unsigned long replyAddr = inet_addr( gIF_IPV4 );
    /* a/c to UPNP Spec */
    int ttl = 4;
    int hops = 1;
    char buf_ntop[64];
    int ret = UPNP_E_SUCCESS;

    ReplySock = socket( DestAddr->sa_family, SOCK_DGRAM, 0 );
    if ( ReplySock == -1 ) {
        strerror_r(errno, errorBuffer, ERROR_BUFFER_LEN);
        UpnpPrintf( UPNP_INFO, SSDP, __FILE__, __LINE__,
            "SSDP_LIB: New Request Handler:"
            "Error in socket(): %s\n", errorBuffer );

        return UPNP_E_OUTOF_SOCKET;
    }
 
    if( DestAddr->sa_family == AF_INET ) {
        inet_ntop(AF_INET, &((struct sockaddr_in*)DestAddr)->sin_addr, 
            buf_ntop, sizeof(buf_ntop));
        setsockopt( ReplySock, IPPROTO_IP, IP_MULTICAST_IF,
            (char *)&replyAddr, sizeof (replyAddr) );
        setsockopt( ReplySock, IPPROTO_IP, IP_MULTICAST_TTL,
            (char *)&ttl, sizeof (int) );
        socklen = sizeof(struct sockaddr_in);
    } else if( DestAddr->sa_family == AF_INET6 ) {
        inet_ntop(AF_INET6, &((struct sockaddr_in6*)DestAddr)->sin6_addr, 
            buf_ntop, sizeof(buf_ntop));
        setsockopt( ReplySock, IPPROTO_IPV6, IPV6_MULTICAST_IF,
            (char *)&gIF_INDEX, sizeof(gIF_INDEX) );
        setsockopt( ReplySock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS,
            (char *)&hops, sizeof(hops) );
    } else {
        UpnpPrintf( UPNP_CRITICAL, SSDP, __FILE__, __LINE__,
            "Invalid destination address specified." );
        ret = UPNP_E_NETWORK_ERROR;
        goto end_NewRequestHandler;
    }

    for( Index = 0; Index < NumPacket; Index++ ) {
        int rc;
        UpnpPrintf( UPNP_INFO, SSDP, __FILE__, __LINE__,
            ">>> SSDP SEND to %s >>>\n%s\n",
            buf_ntop, *( RqPacket + Index ) );
        rc = sendto( ReplySock, *( RqPacket + Index ),
                     strlen( *( RqPacket + Index ) ),
                     0, DestAddr, socklen );

        if (rc == -1) {
            strerror_r(errno, errorBuffer, ERROR_BUFFER_LEN);
            UpnpPrintf( UPNP_INFO, SSDP, __FILE__, __LINE__,
                        "SSDP_LIB: New Request Handler:"
                        "Error in socket(): %s\n", errorBuffer );
            ret = UPNP_E_SOCKET_WRITE;
            goto end_NewRequestHandler;
        }
    }

end_NewRequestHandler:
    shutdown( ReplySock, SD_BOTH );
    UpnpCloseSocket( ReplySock );

    return ret;
}

/**
 * return 1 if an inet6 @ has been found
 */
int extractIPv6address(char *url, char *address)
{
	int i = 0;
	int j = 0;
	int ret = 0;

	while (url[i] != '[' && url[i] != '\0') {
		i++;
	}
	if( url[i] == '\0') {
		goto exit_function;
	}

	/* bracket has been found, we deal with an IPv6 address */
	i++;
	while (url[i] != '\0' &&  url[i] != ']' ) {
		address[j] = url[i];
		i++;
		j++;
	}
	if (url[i] == '\0') {
		goto exit_function;
	}

	if (url[i] == ']') {
		address[j] = '\0';
		ret = 1;
	}

exit_function:
	return ret;
}


/**
 * return 1 if the Url contains an ULA or GUA IPv6 address
 * 0 otherwise
 */
int isUrlV6UlaGua(char *descdocUrl)
{
	char address[INET6_ADDRSTRLEN];
	struct in6_addr v6_addr;

	if (extractIPv6address(descdocUrl, address)) {
		inet_pton(AF_INET6, address, &v6_addr);
		return !IN6_IS_ADDR_LINKLOCAL(&v6_addr);
	}

	return 0;
}


/************************************************************************
* Function : CreateServiceRequestPacket
*
* Parameters:
*	IN int msg_type : type of the message ( Search Reply, Advertisement
*		or Shutdown )
*	IN char * nt : ssdp type
*	IN char * usn : unique service name ( go in the HTTP Header)
*	IN char * location :Location URL.
*	IN int  duration :Service duration in sec.
*	OUT char** packet :Output buffer filled with HTTP statement.
*	IN int AddressFamily: Address family of the HTTP request.
*
* Description:
*	This function creates a HTTP request packet.  Depending
*	on the input parameter it either creates a service advertisement
*	request or service shutdown request etc.
*
* Returns: void
*
***************************************************************************/
void CreateServicePacket(
	IN int msg_type,
	IN char *nt,
	IN char *usn,
	IN char *location,
	IN int duration,
	OUT char **packet,
	IN int AddressFamily)
{
	int ret_code;
	char *nts;
	membuffer buf;

	/* Notf == 0 means service shutdown,
	 * Notf == 1 means service advertisement,
	 * Notf == 2 means reply */
	membuffer_init(&buf);
	buf.size_inc = 30;
	*packet = NULL;

	if (msg_type == MSGTYPE_REPLY) {
		ret_code = http_MakeMessage(
			&buf, 1, 1,
			"R" "sdc" "D" "sc" "ssc" "ssc" "ssc" "S" "Xc" "ssc" "sscc",
			HTTP_OK,
			"CACHE-CONTROL: max-age=", duration,
			"EXT:",
			"LOCATION: ", location,
			"OPT: ", "\"http://schemas.upnp.org/upnp/1/0/\"; ns=01",
			"01-NLS: ", gUpnpSdkNLSuuid,
			X_USER_AGENT,
			"ST: ", nt,
			"USN: ", usn);
		if (ret_code != 0) {
			return;
		}
	} else if (msg_type == MSGTYPE_ADVERTISEMENT ||
		   msg_type == MSGTYPE_SHUTDOWN) {
		char *host = NULL;
		if (msg_type == MSGTYPE_ADVERTISEMENT) {
			nts = "ssdp:alive";
		} else {
      			/* shutdown */
			nts = "ssdp:byebye";
		}
		/* NOTE: The CACHE-CONTROL and LOCATION headers are not present in
		 * a shutdown msg, but are present here for MS WinMe interop. */
		if (AddressFamily == AF_INET) {
			host = SSDP_IP;
		} else {
			if (isUrlV6UlaGua(location)) {
				host = "[" SSDP_IPV6_SITELOCAL "]";
			} else {
				host = "[" SSDP_IPV6_LINKLOCAL "]";
			}
		}
		ret_code = http_MakeMessage(
			&buf, 1, 1,
			"Q" "sssdc" "sdc" "ssc" "ssc" "ssc" "ssc" "ssc" "S" "Xc" "sscc",
			HTTPMETHOD_NOTIFY, "*", (size_t)1,
			"HOST: ", host,
			":", SSDP_PORT,
			"CACHE-CONTROL: max-age=", duration,
			"LOCATION: ", location,
			"OPT: ", "\"http://schemas.upnp.org/upnp/1/0/\"; ns=01",
			"01-NLS: ", gUpnpSdkNLSuuid,
			"NT: ", nt,
			"NTS: ", nts,
			X_USER_AGENT,
			"USN: ", usn );
		if (ret_code != 0) {
			return;
		}
	} else {
		/* unknown msg */
		assert(0);
	}

	/* return msg */
	*packet = membuffer_detach(&buf);
	membuffer_destroy(&buf);

	return;
}


/************************************************************************
* Function : DeviceAdvertisement
*
* Parameters:
*	IN char * DevType : type of the device
*	IN int RootDev: flag to indicate if the device is root device
*	IN char * nt : ssdp type
*	IN char * usn : unique service name
*	IN char * location :Location URL.
*	IN int  duration :Service duration in sec.
*
* Description:
*	This function creates the device advertisement request based on 
*	the input parameter, and send it to the multicast channel.
*
* Returns: int
*	UPNP_E_SUCCESS if successful else appropriate error
***************************************************************************/
int
DeviceAdvertisement( IN char *DevType,
                     int RootDev,
                     char *Udn,
                     IN char *Location,
                     IN int Duration,
                     IN int AddressFamily)
{
    struct sockaddr_storage __ss;
    struct sockaddr_in* DestAddr4 = (struct sockaddr_in*)&__ss;
    struct sockaddr_in6* DestAddr6 = (struct sockaddr_in6*)&__ss;

    /* char Mil_Nt[LINE_SIZE] */
    char Mil_Usn[LINE_SIZE];
    char *msgs[3];
    int ret_code = UPNP_E_SUCCESS;

    UpnpPrintf( UPNP_INFO, SSDP, __FILE__, __LINE__,
        "In function DeviceAdvertisement\n" );

    memset( &__ss, 0, sizeof(__ss) );
    if( AddressFamily == AF_INET ) {
        DestAddr4->sin_family = AF_INET;
        inet_pton( AF_INET, SSDP_IP, &DestAddr4->sin_addr );
        DestAddr4->sin_port = htons( SSDP_PORT );
    } else if( AddressFamily == AF_INET6 ) {
        DestAddr6->sin6_family = AF_INET6;
	inet_pton(AF_INET6,
		(isUrlV6UlaGua(Location)) ? SSDP_IPV6_SITELOCAL : SSDP_IPV6_LINKLOCAL,
		&DestAddr6->sin6_addr );
        DestAddr6->sin6_port = htons( SSDP_PORT );
        DestAddr6->sin6_scope_id = gIF_INDEX;
    } else {
        UpnpPrintf( UPNP_CRITICAL, SSDP, __FILE__, __LINE__,
            "Invalid device address family.\n" );
    }

    msgs[0] = NULL;
    msgs[1] = NULL;
    msgs[2] = NULL;

    /* If deviceis a root device , here we need to send 3 advertisement
     * or reply */
    if( RootDev ) {
        sprintf( Mil_Usn, "%s::upnp:rootdevice", Udn );
        CreateServicePacket( MSGTYPE_ADVERTISEMENT, "upnp:rootdevice",
            Mil_Usn, Location, Duration, &msgs[0], AddressFamily );
    }
    /* both root and sub-devices need to send these two messages */
    CreateServicePacket( MSGTYPE_ADVERTISEMENT, Udn, Udn,
                         Location, Duration, &msgs[1], AddressFamily );
    sprintf( Mil_Usn, "%s::%s", Udn, DevType );
    CreateServicePacket( MSGTYPE_ADVERTISEMENT, DevType, Mil_Usn,
                         Location, Duration, &msgs[2], AddressFamily );
    /* check error */
    if( ( RootDev && msgs[0] == NULL ) ||
        msgs[1] == NULL || msgs[2] == NULL ) {
        free( msgs[0] );
        free( msgs[1] );
        free( msgs[2] );
        return UPNP_E_OUTOF_MEMORY;
    }
    /* send packets */
    if( RootDev ) {
        /* send 3 msg types */
        ret_code = NewRequestHandler( (struct sockaddr*)&__ss, 3, &msgs[0] );
    } else                      /* sub-device */
    {
        /* send 2 msg types */
        ret_code = NewRequestHandler( (struct sockaddr*)&__ss, 2, &msgs[1] );
    }

    /* free msgs */
    free( msgs[0] );
    free( msgs[1] );
    free( msgs[2] );

    return ret_code;
}

/************************************************************************
* Function : SendReply
*
* Parameters:
*	IN struct sockaddr * DestAddr:destination IP address.
*	IN char *DevType: Device type
*	IN int RootDev: 1 means root device 0 means embedded device.
*	IN char * Udn: Device UDN
*	IN char * Location: Location of Device description document.
*	IN int  Duration :Life time of this device.
*	IN int ByType:
*
* Description:
*	This function creates the reply packet based on the input parameter, 
*	and send it to the client addesss given in its input parameter DestAddr.
*
* Returns: int
*	UPNP_E_SUCCESS if successful else appropriate error
***************************************************************************/
int
SendReply( IN struct sockaddr *DestAddr,
           IN char *DevType,
           IN int RootDev,
           IN char *Udn,
           IN char *Location,
           IN int Duration,
           IN int ByType )
{
    int ret_code;
    char *msgs[2];
    int num_msgs;
    char Mil_Usn[LINE_SIZE];
    int i;

    msgs[0] = NULL;
    msgs[1] = NULL;

    if( RootDev ) {
        /* one msg for root device */
        num_msgs = 1;

        sprintf( Mil_Usn, "%s::upnp:rootdevice", Udn );
        CreateServicePacket( MSGTYPE_REPLY, "upnp:rootdevice",
            Mil_Usn, Location, Duration, &msgs[0], DestAddr->sa_family );
    } else {
        /* two msgs for embedded devices */
        num_msgs = 1;

        /*NK: FIX for extra response when someone searches by udn */
        if( !ByType ) {
            CreateServicePacket( MSGTYPE_REPLY, Udn, Udn, Location,
                Duration, &msgs[0], DestAddr->sa_family );
        } else {
            sprintf( Mil_Usn, "%s::%s", Udn, DevType );
            CreateServicePacket( MSGTYPE_REPLY, DevType, Mil_Usn,
                Location, Duration, &msgs[0], DestAddr->sa_family );
        }
    }

    /* check error */
    for( i = 0; i < num_msgs; i++ ) {
        if( msgs[i] == NULL ) {
            free( msgs[0] );
            return UPNP_E_OUTOF_MEMORY;
        }
    }

    /* send msgs */
    ret_code = NewRequestHandler( DestAddr, num_msgs, msgs );
    for( i = 0; i < num_msgs; i++ ) {
        if( msgs[i] != NULL )
            free( msgs[i] );
    }

    return ret_code;
}

/************************************************************************
* Function : DeviceReply
*
* Parameters:
*	IN struct sockaddr *DestAddr:destination IP address.
*	IN char *DevType: Device type
*	IN int RootDev: 1 means root device 0 means embedded device.
*	IN char * Udn: Device UDN
*	IN char * Location: Location of Device description document.
*	IN int  Duration :Life time of this device.
* Description:
*	This function creates the reply packet based on the input parameter, 
*	and send it to the client address given in its input parameter DestAddr.
*
* Returns: int
*	UPNP_E_SUCCESS if successful else appropriate error
***************************************************************************/
int
DeviceReply( IN struct sockaddr *DestAddr,
             IN char *DevType,
             IN int RootDev,
             IN char *Udn,
             IN char *Location,
             IN int Duration)
{
    char *szReq[3],
      Mil_Nt[LINE_SIZE],
      Mil_Usn[LINE_SIZE];
    int RetVal;

    szReq[0] = NULL;
    szReq[1] = NULL;
    szReq[2] = NULL;

    /* create 2 or 3 msgs */
    if( RootDev ) {
        /* 3 replies for root device */
        strcpy( Mil_Nt, "upnp:rootdevice" );
        sprintf( Mil_Usn, "%s::upnp:rootdevice", Udn );
        CreateServicePacket( MSGTYPE_REPLY, Mil_Nt, Mil_Usn,
            Location, Duration, &szReq[0], DestAddr->sa_family );
    }
    sprintf( Mil_Nt, "%s", Udn );
    sprintf( Mil_Usn, "%s", Udn );
    CreateServicePacket( MSGTYPE_REPLY, Mil_Nt, Mil_Usn,
        Location, Duration, &szReq[1], DestAddr->sa_family );
    sprintf( Mil_Nt, "%s", DevType );
    sprintf( Mil_Usn, "%s::%s", Udn, DevType );
    CreateServicePacket( MSGTYPE_REPLY, Mil_Nt, Mil_Usn,
        Location, Duration, &szReq[2], DestAddr->sa_family );
    /* check error */
    if( ( RootDev && szReq[0] == NULL ) ||
        szReq[1] == NULL || szReq[2] == NULL ) {
        free( szReq[0] );
        free( szReq[1] );
        free( szReq[2] );
        return UPNP_E_OUTOF_MEMORY;
    }
    /* send replies */
    if( RootDev ) {
        RetVal = NewRequestHandler( DestAddr, 3, szReq );
    } else {
        RetVal = NewRequestHandler( DestAddr, 2, &szReq[1] );
    }
    /* free */
    free( szReq[0] );
    free( szReq[1] );
    free( szReq[2] );

    return RetVal;
}

/************************************************************************
* Function : ServiceAdvertisement
*
* Parameters:
*	IN char * Udn: Device UDN
*	IN char *ServType: Service Type.
*	IN char * Location: Location of Device description document.
*	IN int  Duration :Life time of this device.
*	IN int AddressFamily: Device address family
* Description:
*	This function creates the advertisement packet based
*	on the input parameter, and send it to the multicast channel.
*
* Returns: int
*	UPNP_E_SUCCESS if successful else appropriate error
***************************************************************************/
int
ServiceAdvertisement( IN char *Udn,
                      IN char *ServType,
                      IN char *Location,
                      IN int Duration,
                      IN int AddressFamily)
{
    char Mil_Usn[LINE_SIZE];
    char *szReq[1];
    int RetVal = UPNP_E_SUCCESS;
    struct sockaddr_storage __ss;
    struct sockaddr_in* DestAddr4 = (struct sockaddr_in*)&__ss;
    struct sockaddr_in6* DestAddr6 = (struct sockaddr_in6*)&__ss;

    memset( &__ss, 0, sizeof(__ss) );
    if( AddressFamily == AF_INET ) {
        DestAddr4->sin_family = AF_INET;
        inet_pton( AF_INET, SSDP_IP, &DestAddr4->sin_addr );
        DestAddr4->sin_port = htons( SSDP_PORT );
    } else if( AddressFamily == AF_INET6 ) {
        DestAddr6->sin6_family = AF_INET6;
	inet_pton(AF_INET6,
		(isUrlV6UlaGua(Location)) ? SSDP_IPV6_SITELOCAL : SSDP_IPV6_LINKLOCAL,
		&DestAddr6->sin6_addr );
        DestAddr6->sin6_port = htons( SSDP_PORT );
        DestAddr6->sin6_scope_id = gIF_INDEX;
    } else {
        UpnpPrintf( UPNP_CRITICAL, SSDP, __FILE__, __LINE__,
            "Invalid device address family.\n" );
    }

    sprintf( Mil_Usn, "%s::%s", Udn, ServType );

    /* CreateServiceRequestPacket(1,szReq[0],Mil_Nt,Mil_Usn,
     * Server,Location,Duration); */
    CreateServicePacket( MSGTYPE_ADVERTISEMENT, ServType, Mil_Usn,
                         Location, Duration, &szReq[0], AddressFamily );
    if( szReq[0] == NULL ) {
        return UPNP_E_OUTOF_MEMORY;
    }

    RetVal = NewRequestHandler( (struct sockaddr*)&__ss, 1, szReq );

    free( szReq[0] );
    return RetVal;
}

/************************************************************************
* Function : ServiceReply
*
* Parameters:
*	IN struct sockaddr *DestAddr:
*	IN char * Udn: Device UDN
*	IN char *ServType: Service Type.
*	IN char * Location: Location of Device description document.
*	IN int  Duration :Life time of this device.
* Description:
*	This function creates the advertisement packet based 
*	on the input parameter, and send it to the multicast channel.
*
* Returns: int
*	UPNP_E_SUCCESS if successful else appropriate error
***************************************************************************/
int
ServiceReply( IN struct sockaddr *DestAddr,
              IN char *ServType,
              IN char *Udn,
              IN char *Location,
              IN int Duration )
{
    char Mil_Usn[LINE_SIZE];
    char *szReq[1];
    int RetVal;

    szReq[0] = NULL;

    sprintf( Mil_Usn, "%s::%s", Udn, ServType );

    CreateServicePacket( MSGTYPE_REPLY, ServType, Mil_Usn,
        Location, Duration, &szReq[0], DestAddr->sa_family );
    if( szReq[0] == NULL ) {
        return UPNP_E_OUTOF_MEMORY;
    }

    RetVal = NewRequestHandler( DestAddr, 1, szReq );

    free( szReq[0] );
    return RetVal;
}

/************************************************************************
* Function : ServiceShutdown
*
* Parameters:
*	IN char * Udn: Device UDN
*	IN char *ServType: Service Type.
*	IN char * Location: Location of Device description document.
*	IN int  Duration :Service duration in sec.
*	IN int AddressFamily: Device address family
* Description:
*	This function creates a HTTP service shutdown request packet 
*	and sent it to the multicast channel through RequestHandler.
*
* Returns: int
*	UPNP_E_SUCCESS if successful else appropriate error
***************************************************************************/
int
ServiceShutdown( IN char *Udn,
                 IN char *ServType,
                 IN char *Location,
                 IN int Duration,
                 IN int AddressFamily)
{
    char Mil_Usn[LINE_SIZE];
    char *szReq[1];
    struct sockaddr_storage __ss;
    struct sockaddr_in* DestAddr4 = (struct sockaddr_in*)&__ss;
    struct sockaddr_in6* DestAddr6 = (struct sockaddr_in6*)&__ss;
    int RetVal = UPNP_E_SUCCESS;

    memset( &__ss, 0, sizeof(__ss) );
    if( AddressFamily == AF_INET ) {
        DestAddr4->sin_family = AF_INET;
        inet_pton( AF_INET, SSDP_IP, &DestAddr4->sin_addr );
        DestAddr4->sin_port = htons( SSDP_PORT );
    } else if( AddressFamily == AF_INET6 ) {
        DestAddr6->sin6_family = AF_INET6;
	inet_pton(AF_INET6,
		(isUrlV6UlaGua(Location)) ? SSDP_IPV6_SITELOCAL : SSDP_IPV6_LINKLOCAL,
		&DestAddr6->sin6_addr );
        DestAddr6->sin6_port = htons( SSDP_PORT );
        DestAddr6->sin6_scope_id = gIF_INDEX;
    } else {
        UpnpPrintf( UPNP_CRITICAL, SSDP, __FILE__, __LINE__,
            "Invalid device address family.\n" );
    }

    /* sprintf(Mil_Nt,"%s",ServType); */
    sprintf( Mil_Usn, "%s::%s", Udn, ServType );
    /* CreateServiceRequestPacket(0,szReq[0],Mil_Nt,Mil_Usn,
     * Server,Location,Duration); */
    CreateServicePacket( MSGTYPE_SHUTDOWN, ServType, Mil_Usn,
                         Location, Duration, &szReq[0], AddressFamily );
    if( szReq[0] == NULL ) {
        return UPNP_E_OUTOF_MEMORY;
    }
    RetVal = NewRequestHandler( (struct sockaddr*)&__ss, 1, szReq );

    free( szReq[0] );
    return RetVal;
}

/************************************************************************
* Function : DeviceShutdown
*
* Parameters:
*	IN char *DevType: Device Type.
*	IN int RootDev:1 means root device.
*	IN char * Udn: Device UDN
*	IN char * Location: Location URL
*	IN int  Duration :Device duration in sec.
*	IN int AddressFamily: Device address family.
*
* Description:
*	This function creates a HTTP device shutdown request packet 
*	and sent it to the multicast channel through RequestHandler.
*
* Returns: int
*	UPNP_E_SUCCESS if successful else appropriate error
***************************************************************************/
int
DeviceShutdown( IN char *DevType,
                IN int RootDev,
                IN char *Udn,
                IN char *_Server,
                IN char *Location,
                IN int Duration, 
                IN int AddressFamily)
{
    struct sockaddr_storage __ss;
    struct sockaddr_in* DestAddr4 = (struct sockaddr_in*)&__ss;
    struct sockaddr_in6* DestAddr6 = (struct sockaddr_in6*)&__ss;
    char *msgs[3];
    char Mil_Usn[LINE_SIZE];
    int ret_code = UPNP_E_SUCCESS;

    msgs[0] = NULL;
    msgs[1] = NULL;
    msgs[2] = NULL;

    memset( &__ss, 0, sizeof(__ss) );
    if( AddressFamily == AF_INET ) {
        DestAddr4->sin_family = AF_INET;
        inet_pton( AF_INET, SSDP_IP, &DestAddr4->sin_addr );
        DestAddr4->sin_port = htons( SSDP_PORT );
    } else if( AddressFamily == AF_INET6 ) {
        DestAddr6->sin6_family = AF_INET6;
	inet_pton(AF_INET6,
		(isUrlV6UlaGua(Location)) ? SSDP_IPV6_SITELOCAL : SSDP_IPV6_LINKLOCAL,
		&DestAddr6->sin6_addr );
        DestAddr6->sin6_port = htons( SSDP_PORT );
        DestAddr6->sin6_scope_id = gIF_INDEX;
    } else {
        UpnpPrintf( UPNP_CRITICAL, SSDP, __FILE__, __LINE__,
            "Invalid device address family.\n" );
    }
    /* root device has one extra msg */
    if( RootDev ) {
        sprintf( Mil_Usn, "%s::upnp:rootdevice", Udn );
        CreateServicePacket( MSGTYPE_SHUTDOWN, "upnp:rootdevice",
            Mil_Usn, Location, Duration, &msgs[0], AddressFamily );
    }
    UpnpPrintf( UPNP_INFO, SSDP, __FILE__, __LINE__,
        "In function DeviceShutdown\n" );
    /* both root and sub-devices need to send these two messages */
    CreateServicePacket( MSGTYPE_SHUTDOWN, Udn, Udn,
                         Location, Duration, &msgs[1], AddressFamily );
    sprintf( Mil_Usn, "%s::%s", Udn, DevType );
    CreateServicePacket( MSGTYPE_SHUTDOWN, DevType, Mil_Usn,
                         Location, Duration, &msgs[2], AddressFamily );
    /* check error */
    if( ( RootDev && msgs[0] == NULL ) ||
        msgs[1] == NULL || msgs[2] == NULL ) {
        free( msgs[0] );
        free( msgs[1] );
        free( msgs[2] );
        return UPNP_E_OUTOF_MEMORY;
    }
    /* send packets */
    if( RootDev ) {
        /* send 3 msg types */
        ret_code = NewRequestHandler( (struct sockaddr*)&__ss, 3, &msgs[0] );
    } else {
	/* sub-device */
        /* send 2 msg types */
        ret_code = NewRequestHandler( (struct sockaddr*)&__ss, 2, &msgs[1] );
    }
    /* free msgs */
    free( msgs[0] );
    free( msgs[1] );
    free( msgs[2] );

    return ret_code;
}

#endif /* EXCLUDE_SSDP */
#endif /* INCLUDE_DEVICE_APIS */

