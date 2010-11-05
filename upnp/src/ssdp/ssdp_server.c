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

#if !defined(WIN32)
	#include <sys/param.h>
#endif

#include "config.h"


#if EXCLUDE_SSDP == 0

#include "membuffer.h"
#include "ssdplib.h"
#include <stdio.h>
#include "ThreadPool.h"
#include "miniserver.h"

#include "upnpapi.h"
#include "httpparser.h"
#include "httpreadwrite.h"

#define MAX_TIME_TOREAD  45

#ifdef INCLUDE_CLIENT_APIS
	SOCKET gSsdpReqSocket4 = INVALID_SOCKET;
	#ifdef UPNP_ENABLE_IPV6
		SOCKET gSsdpReqSocket6 = INVALID_SOCKET;
	#endif /* UPNP_ENABLE_IPV6 */
#endif /* INCLUDE_CLIENT_APIS */

void RequestHandler();
int create_ssdp_sock_v4( SOCKET* ssdpSock );
#ifdef UPNP_ENABLE_IPV6
int create_ssdp_sock_v6( SOCKET* ssdpSock );
int create_ssdp_sock_v6_ula_gua( SOCKET* ssdpSock );
#endif
#if INCLUDE_CLIENT_APIS
int create_ssdp_sock_reqv4( SOCKET* ssdpReqSock );
#ifdef UPNP_ENABLE_IPV6
int create_ssdp_sock_reqv6( SOCKET* ssdpReqSock );
#endif
#endif
Event ErrotEvt;

enum Listener { Idle, Stopping, Running };

struct SSDPSockArray {
	/* socket for incoming advertisments and search requests */
	SOCKET ssdpSock;
#ifdef INCLUDE_CLIENT_APIS
	/* socket for sending search requests and receiving search replies */
	int ssdpReqSock;
#endif /* INCLUDE_CLIENT_APIS */
};

#ifdef INCLUDE_DEVICE_APIS
#if EXCLUDE_SSDP == 0

/************************************************************************
 * Function : AdvertiseAndReply
 *
 * Parameters:
 *	IN int AdFlag:
 *		-1 = Send shutdown,
 *		 0 = send reply, 
 *		 1 = Send Advertisement
 *	IN UpnpDevice_Handle Hnd: Device handle
 *	IN enum SsdpSearchType SearchType:Search type for sending replies
 *	IN struct sockaddr *DestAddr:Destination address
 *	IN char *DeviceType:Device type
 *	IN char *DeviceUDN:Device UDN
 *	IN char *ServiceType:Service type
 *	IN int Exp:Advertisement age
 *
 * Description:
 *	This function sends SSDP advertisements, replies and shutdown messages.
 *
 * Returns: int
 *	UPNP_E_SUCCESS if successful else appropriate error
 ***************************************************************************/
static const char SERVICELIST_STR[] = "serviceList";

int AdvertiseAndReply(
	IN int AdFlag,
	IN UpnpDevice_Handle Hnd,
	IN enum SsdpSearchType SearchType,
	IN struct sockaddr *DestAddr,
	IN char *DeviceType,
	IN char *DeviceUDN,
	IN char *ServiceType,
	int Exp)
{
	int retVal = UPNP_E_SUCCESS;
	int i;
	int j;
	int defaultExp = DEFAULT_MAXAGE;
	struct Handle_Info *SInfo = NULL;
	char UDNstr[100];
	char devType[100];
	char servType[100];
	IXML_NodeList *nodeList = NULL;
	IXML_NodeList *tmpNodeList = NULL;
	IXML_Node *tmpNode = NULL;
	IXML_Node *tmpNode2 = NULL;
	IXML_Node *textNode = NULL;
	const DOMString tmpStr;
	char SERVER[200];
	const DOMString dbgStr;
	int NumCopy = 0;

	UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
		"Inside AdvertiseAndReply with AdFlag = %d\n", AdFlag);

	/* Use a read lock */
	HandleReadLock();
	if (GetHandleInfo(Hnd, &SInfo) != HND_DEVICE) {
		retVal = UPNP_E_INVALID_HANDLE;
		goto end_function;
	}

	defaultExp = SInfo->MaxAge;

	/* get server info */
	get_sdk_info(SERVER);

	/* parse the device list and send advertisements/replies */
	while (NumCopy == 0 || (AdFlag && NumCopy < NUM_SSDP_COPY)) {
		if (NumCopy != 0)
			imillisleep(SSDP_PAUSE);
		NumCopy++;

		for (i = 0;; i++) {
			UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
				"Entering new device list with i = %d\n\n", i);
			tmpNode = ixmlNodeList_item(SInfo->DeviceList, i);
			if (!tmpNode) {
				UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
					"Exiting new device list with i = %d\n\n", i);
				break;
			}
			dbgStr = ixmlNode_getNodeName(tmpNode);

			UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__,
				"Extracting device type once for %s\n", dbgStr);
			ixmlNodeList_free(nodeList);
			nodeList = ixmlElement_getElementsByTagName(
				(IXML_Element *)tmpNode, "deviceType");
			if (!nodeList) continue;

			UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
				"Extracting UDN for %s\n", dbgStr);
			dbgStr = ixmlNode_getNodeName(tmpNode);

			UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
				"Extracting device type\n");
			tmpNode2 = ixmlNodeList_item(nodeList, 0);
			if (!tmpNode2) continue;

			textNode = ixmlNode_getFirstChild(tmpNode2);
			if (!textNode) continue;

			UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
				"Extracting device type \n");
			tmpStr = ixmlNode_getNodeValue(textNode);
			if (!tmpStr) continue;

			strcpy(devType, tmpStr);
			UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
				"Extracting device type = %s\n", devType);
			if (!tmpNode) {
				UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
					"TempNode is NULL\n");
			}
			dbgStr = ixmlNode_getNodeName(tmpNode);

			UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
				"Extracting UDN for %s\n", dbgStr);
			ixmlNodeList_free(nodeList);
			nodeList = ixmlElement_getElementsByTagName(
				(IXML_Element *)tmpNode, "UDN");
			if (!nodeList) {
				UpnpPrintf(UPNP_CRITICAL, API, __FILE__,
					__LINE__, "UDN not found!\n");
				continue;
			}
			tmpNode2 = ixmlNodeList_item(nodeList, 0);
			if (!tmpNode2) {
				UpnpPrintf(UPNP_CRITICAL, API, __FILE__,
					__LINE__, "UDN not found!\n");
				continue;
			}
			textNode = ixmlNode_getFirstChild(tmpNode2);
			if (!textNode) {
				UpnpPrintf(UPNP_CRITICAL, API, __FILE__,
					__LINE__, "UDN not found!\n");
				continue;
			}
			tmpStr = ixmlNode_getNodeValue(textNode);
			if (!tmpStr) {
				UpnpPrintf(UPNP_CRITICAL, API, __FILE__, __LINE__,
					"UDN not found!\n");
				continue;
			}
			strcpy(UDNstr, tmpStr);
			UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__,
				"Sending UDNStr = %s \n", UDNstr);
			if (AdFlag) {
				/* send the device advertisement */
				if (AdFlag == 1) {
					DeviceAdvertisement(devType, i == 0,
                        UDNstr, SInfo->DescURL, Exp, SInfo->DeviceAf );
				} else {
		   			/* AdFlag == -1 */
					DeviceShutdown(devType, i == 0, UDNstr,
                        SERVER, SInfo->DescURL, Exp, SInfo->DeviceAf );
				}
			} else {
				switch (SearchType) {
				case SSDP_ALL:
					DeviceReply(DestAddr, devType, i == 0,
						UDNstr, SInfo->DescURL, defaultExp);
					break;
				case SSDP_ROOTDEVICE:
					if (i == 0) {
						SendReply(DestAddr, devType, 1,
							UDNstr, SInfo->DescURL, defaultExp, 0);
					}
					break;
				case SSDP_DEVICEUDN: {
					if (DeviceUDN && strlen(DeviceUDN) != 0) {
						if (strcasecmp(DeviceUDN, UDNstr)) {
							UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__,
								"DeviceUDN=%s and search UDN=%s DID NOT match\n",
								UDNstr, DeviceUDN);
							break;
						} else {
							UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__,
								"DeviceUDN=%s and search UDN=%s MATCH\n",
								UDNstr, DeviceUDN);
							SendReply(DestAddr, devType, 0,
								UDNstr, SInfo->DescURL,
								defaultExp, 0);
							break;
						}
					}
				}
				case SSDP_DEVICETYPE: {
					if (!strncasecmp(DeviceType, devType, strlen(DeviceType)-2)) {
						if (atoi(&DeviceType[strlen(DeviceType)-1]) <= atoi(&devType[strlen(devType)-1])) {
							/* the requested version is lower than the device version
							 * must reply with the lower version number */
							UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__,
								"DeviceType=%s and search devType=%s MATCH\n",
								devType, DeviceType);
							SendReply(DestAddr, DeviceType, 0, UDNstr, SInfo->DescURL, defaultExp, 1);
						} else {
							UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__,
								"DeviceType=%s and search devType=%s DID NOT MATCH\n",
								devType, DeviceType);
						}
					} else {
						UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__,
							"DeviceType=%s and search devType=%s DID NOT MATCH\n",
							devType, DeviceType);
					}
					break;
				}
				default:
					break;
				}
			}
			/* send service advertisements for services corresponding
			 * to the same device */
			UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__,
				"Sending service Advertisement\n");

			/* Correct service traversal such that each device's serviceList
			 * is directly traversed as a child of its parent device. This
			 * ensures that the service's alive message uses the UDN of
			 * the parent device. */
			tmpNode = ixmlNode_getFirstChild(tmpNode);
			while (tmpNode) {
				dbgStr = ixmlNode_getNodeName(tmpNode);
				if (!strncmp(dbgStr, SERVICELIST_STR, sizeof SERVICELIST_STR)) {
					break;
				}
				tmpNode = ixmlNode_getNextSibling(tmpNode);
			}
			ixmlNodeList_free(nodeList);
			if (!tmpNode) {
				nodeList = NULL;
				continue;
			}
			nodeList = ixmlElement_getElementsByTagName(
				(IXML_Element *)tmpNode, "service");
			if (!nodeList) {
				UpnpPrintf( UPNP_INFO, API, __FILE__, __LINE__,
					"Service not found 3\n" );
				continue;
			}
			for (j = 0;; j++) {
				tmpNode = ixmlNodeList_item(nodeList, j);
				if (!tmpNode) {
					break;
				}
				ixmlNodeList_free(tmpNodeList);
				tmpNodeList = ixmlElement_getElementsByTagName(
					(IXML_Element *)tmpNode, "serviceType");
				if (!tmpNodeList) {
					UpnpPrintf(UPNP_CRITICAL, API, __FILE__, __LINE__,
						"ServiceType not found \n");
					continue;
				}
				tmpNode2 = ixmlNodeList_item(tmpNodeList, 0);
				if (!tmpNode2) continue;
				textNode = ixmlNode_getFirstChild(tmpNode2);
				if (!textNode) continue;
				/* servType is of format Servicetype:ServiceVersion */
				tmpStr = ixmlNode_getNodeValue(textNode);
				if (!tmpStr) continue;
				strcpy(servType, tmpStr);
				UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__,
					"ServiceType = %s\n", servType);
				if (AdFlag) {
					if (AdFlag == 1) {
						ServiceAdvertisement(UDNstr, servType,
                                         SInfo->DescURL, Exp, SInfo->DeviceAf );
						} else {
						/* AdFlag == -1 */
						ServiceShutdown(UDNstr, servType,
                                         SInfo->DescURL, Exp, SInfo->DeviceAf );
					}
				} else {
					switch (SearchType) {
					case SSDP_ALL:
						ServiceReply(DestAddr, servType,
							UDNstr, SInfo->DescURL,
							defaultExp);
						break;
					case SSDP_SERVICE:
						if (ServiceType) {
							if (!strncasecmp(ServiceType, servType, strlen(ServiceType) - 2)) {
								if (atoi(&ServiceType[strlen(ServiceType)-1]) <= atoi(&servType[strlen(servType)-1])) {
									/* the requested version is lower than the service version
									 * must reply with the lower version number */
									UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__,
										"ServiceType=%s and search servType=%s MATCH\n",
										ServiceType, servType);
									SendReply(DestAddr, ServiceType, 0, UDNstr, SInfo->DescURL, defaultExp, 1);
								} else {
									UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__,
										"ServiceType=%s and search servType=%s DID NOT MATCH\n",
										ServiceType, servType);
								}
							} else {
								UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__,
									"ServiceType=%s and search servType=%s DID NOT MATCH\n",
									ServiceType, servType);
							}
						}
						break;
					default:
						break;
					}
				}
			}
			ixmlNodeList_free(tmpNodeList);
			tmpNodeList = NULL;
			ixmlNodeList_free(nodeList);
			nodeList = NULL;
		}
	}

end_function:
	ixmlNodeList_free(tmpNodeList);
	ixmlNodeList_free(nodeList);

	UpnpPrintf(UPNP_ALL, API, __FILE__, __LINE__,
		"Exiting AdvertiseAndReply.\n");

	HandleUnlock();

	return retVal;
}

#endif /* EXCLUDE_SSDP == 0 */
#endif /* INCLUDE_DEVICE_APIS */

/************************************************************************
 * Function : Make_Socket_NoBlocking
 *
 * Parameters:
 *	IN SOCKET sock: socket
 *
 * Description:
 *	This function makes socket non-blocking.
 *
 * Returns: int
 *	0 if successful else -1 
 ***************************************************************************/
int
Make_Socket_NoBlocking( SOCKET sock )
{
#ifdef WIN32
     u_long val=1;
     return ioctlsocket(sock, FIONBIO, &val);
#else
    int val;

    val = fcntl( sock, F_GETFL, 0 );
    if( fcntl( sock, F_SETFL, val | O_NONBLOCK ) == -1 ) {
        return -1;
    }
#endif
    return 0;
}

/************************************************************************
 * Function : unique_service_name
 *
 * Parameters:
 *	IN char *cmd: Service Name string
 *	OUT SsdpEvent *Evt: The SSDP event structure partially filled
 *		by all the function.
 *
 * Description:
 *	This function fills the fields of the event structure like DeviceType,
 *	Device UDN and Service Type
 *
 * Returns: int
 *	0 if successful else -1
 ***************************************************************************/
int unique_service_name(IN char *cmd, IN SsdpEvent *Evt)
{
    char TempBuf[COMMAND_LEN];
    char *TempPtr = NULL;
    char *Ptr = NULL;
    char *ptr1 = NULL;
    char *ptr2 = NULL;
    char *ptr3 = NULL;
    int CommandFound = 0;
    int length = 0;

    if( ( TempPtr = strstr( cmd, "uuid:schemas" ) ) != NULL ) {
        ptr1 = strstr( cmd, ":device" );
        if( ptr1 != NULL ) {
            ptr2 = strstr( ptr1 + 1, ":" );
        } else {
            return -1;
        }

        if( ptr2 != NULL ) {
            ptr3 = strstr( ptr2 + 1, ":" );
        } else {
            return -1;
        }

        if( ptr3 != NULL ) {
            sprintf( Evt->UDN, "uuid:%s", ptr3 + 1 );
        } else {
            return -1;
        }

        ptr1 = strstr( cmd, ":" );
        if( ptr1 != NULL ) {
            strncpy( TempBuf, ptr1, ptr3 - ptr1 );
            TempBuf[ptr3 - ptr1] = '\0';
            sprintf( Evt->DeviceType, "urn%s", TempBuf );
        } else {
            return -1;
        }
        return 0;
    }

    if( ( TempPtr = strstr( cmd, "uuid" ) ) != NULL ) {
        if( ( Ptr = strstr( cmd, "::" ) ) != NULL ) {
            strncpy( Evt->UDN, TempPtr, Ptr - TempPtr );
            Evt->UDN[Ptr - TempPtr] = '\0';
        } else {
            strcpy( Evt->UDN, TempPtr );
        }
        CommandFound = 1;
    }

    if( strstr( cmd, "urn:" ) != NULL
        && strstr( cmd, ":service:" ) != NULL ) {
        if( ( TempPtr = strstr( cmd, "urn" ) ) != NULL ) {
            strcpy( Evt->ServiceType, TempPtr );
            CommandFound = 1;
        }
    }

    if( strstr( cmd, "urn:" ) != NULL
        && strstr( cmd, ":device:" ) != NULL ) {
        if( ( TempPtr = strstr( cmd, "urn" ) ) != NULL ) {
            strcpy( Evt->DeviceType, TempPtr );
            CommandFound = 1;
        }
    }

    if( ( TempPtr = strstr( cmd, "::upnp:rootdevice" ) ) != NULL ) {
        /* Everything before "::upnp::rootdevice" is the UDN. */
        if( TempPtr != cmd ) {
            length = TempPtr - cmd;
            strncpy(Evt->UDN, cmd, length);
            Evt->UDN[length] = 0;
            CommandFound = 1;
        }
    }
   
    if( CommandFound == 0 ) {
        return -1;
    }

    return 0;
}

/************************************************************************
 * Function : ssdp_request_type1
 *
 * Parameters:
 *	IN char *cmd: command came in the ssdp request
 *
 * Description:
 *	This function figures out the type of the SSDP search in the
 *	in the request.
 *
 * Returns: enum SsdpSearchType
 *	return appropriate search type else returns SSDP_ERROR
 ***************************************************************************/
enum SsdpSearchType
ssdp_request_type1( IN char *cmd )
{
    if( strstr( cmd, ":all" ) != NULL ) {
        return SSDP_ALL;
    }
    if( strstr( cmd, ":rootdevice" ) != NULL ) {
        return SSDP_ROOTDEVICE;
    }
    if( strstr( cmd, "uuid:" ) != NULL ) {
        return SSDP_DEVICEUDN;
    }
    if( ( strstr( cmd, "urn:" ) != NULL )
        && ( strstr( cmd, ":device:" ) != NULL ) ) {
        return SSDP_DEVICETYPE;
    }
    if( ( strstr( cmd, "urn:" ) != NULL )
        && ( strstr( cmd, ":service:" ) != NULL ) ) {
        return SSDP_SERVICE;
    }
    return SSDP_SERROR;
}

/************************************************************************
 * Function : ssdp_request_type
 *
 * Parameters:
 *	IN char *cmd: command came in the ssdp request
 *	OUT SsdpEvent *Evt: The event structure partially filled by
 *		 this function.
 *
 * Description:
 *	This function starts filling the SSDP event structure based upon the 
 *	request received.
 *
 * Returns: int
 *	0 on success; -1 on error
 ***************************************************************************/
int
ssdp_request_type( IN char *cmd,
                   OUT SsdpEvent * Evt )
{
    // clear event
    memset( Evt, 0, sizeof( SsdpEvent ) );
    unique_service_name( cmd, Evt );
    Evt->ErrCode = NO_ERROR_FOUND;

    if( ( Evt->RequestType = ssdp_request_type1( cmd ) ) == SSDP_SERROR ) {
        Evt->ErrCode = E_HTTP_SYNTEX;
        return -1;
    }
    return 0;
}

/************************************************************************
 * Function : free_ssdp_event_handler_data
 *
 * Parameters:
 *	IN void *the_data: ssdp_thread_data structure. This structure contains
 *			SSDP request message.
 *
 * Description:
 *	This function frees the ssdp request
 *
 * Returns: VOID
 *
 ***************************************************************************/
static void
free_ssdp_event_handler_data( void *the_data )
{
    ssdp_thread_data *data = ( ssdp_thread_data * ) the_data;

    if( data != NULL ) {
        http_message_t *hmsg = &data->parser.msg;

        // free data
        httpmsg_destroy( hmsg );
        free( data );
    }
}

/************************************************************************
 * Function : valid_ssdp_msg
 *
 * Parameters:
 *	IN void *the_data: ssdp_thread_data structure. This structure contains
 *			SSDP request message.
 *
 * Description:
 *	This function do some quick checking of the ssdp msg
 *
 * Returns: xboolean
 *	returns TRUE if msg is valid else FALSE
 ***************************************************************************/
static UPNP_INLINE xboolean valid_ssdp_msg(IN http_message_t *hmsg)
{
    memptr hdr_value;

	/* check for valid methods - NOTIFY or M-SEARCH */
	if (hmsg->method != HTTPMETHOD_NOTIFY &&
	    hmsg->method != HTTPMETHOD_MSEARCH &&
	    hmsg->request_method != HTTPMETHOD_MSEARCH) {
		return FALSE;
	}
	if (hmsg->request_method != HTTPMETHOD_MSEARCH) {
		/* check PATH == "*" */
		if (hmsg->uri.type != RELATIVE ||
		    strncmp("*", hmsg->uri.pathquery.buff, hmsg->uri.pathquery.size) != 0) {
		    return FALSE;
		}
		/* check HOST header */
		if (httpmsg_find_hdr(hmsg, HDR_HOST, &hdr_value) == NULL ||
		    (memptr_cmp(&hdr_value, "239.255.255.250:1900") != 0 &&
		     memptr_cmp(&hdr_value, "[FF02::C]:1900") != 0 &&
		     memptr_cmp(&hdr_value, "[ff02::c]:1900") != 0 &&
		     memptr_cmp(&hdr_value, "[FF05::C]:1900") != 0 &&
		     memptr_cmp(&hdr_value, "[ff05::c]:1900") != 0)) {
			UpnpPrintf(UPNP_INFO, SSDP, __FILE__, __LINE__,
				"Invalid HOST header from SSDP message\n");

			return FALSE;
		}
	}

	/* passed quick check */
	return TRUE;
}

/************************************************************************
 * Function : start_event_handler
 *
 * Parameters:
 *	IN void *the_data: ssdp_thread_data structure. This structure contains
 *			SSDP request message.
 *
 * Description:
 *	This function parses the message and dispatches it to a handler
 *	which handles the ssdp request msg
 *
 * Returns: int
 *	0 if successful -1 if error
 ***************************************************************************/
static UPNP_INLINE int
start_event_handler( void *Data )
{

    http_parser_t *parser = NULL;
    parse_status_t status;
    ssdp_thread_data *data = ( ssdp_thread_data * ) Data;

    parser = &data->parser;

    status = parser_parse( parser );
    if( status == PARSE_FAILURE ) {
        if( parser->msg.method != HTTPMETHOD_NOTIFY ||
            !parser->valid_ssdp_notify_hack ) {
            UpnpPrintf( UPNP_INFO, SSDP, __FILE__, __LINE__,
                "SSDP recvd bad msg code = %d\n",
                status );
            // ignore bad msg, or not enuf mem
            goto error_handler;
        }
        // valid notify msg
    } else if( status != PARSE_SUCCESS ) {
        UpnpPrintf( UPNP_INFO, SSDP, __FILE__, __LINE__,
            "SSDP recvd bad msg code = %d\n", status );

        goto error_handler;
    }
    // check msg
    if( valid_ssdp_msg( &parser->msg ) != TRUE ) {
        goto error_handler;
    }
    return 0;                   //////// done; thread will free 'data'

  error_handler:
    free_ssdp_event_handler_data( data );
    return -1;
}

/************************************************************************
 * Function : ssdp_event_handler_thread
 *
 * Parameters:
 *	IN void *the_data: ssdp_thread_data structure. This structure contains
 *			SSDP request message.
 *
 * Description:
 *	This function is a thread that handles SSDP requests.
 *
 * Returns: void
 *
 ***************************************************************************/
static void ssdp_event_handler_thread(void *the_data)
{
	ssdp_thread_data *data = (ssdp_thread_data *)the_data;
	http_message_t *hmsg = &data->parser.msg;

	if (start_event_handler(the_data) != 0) {
		return;
	}
	/* send msg to device or ctrlpt */
	if (hmsg->method == HTTPMETHOD_NOTIFY ||
	    hmsg->request_method == HTTPMETHOD_MSEARCH) {
#ifdef INCLUDE_CLIENT_APIS
		ssdp_handle_ctrlpt_msg(hmsg, &data->dest_addr, FALSE, NULL);
#endif /* INCLUDE_CLIENT_APIS */
	} else {
		ssdp_handle_device_request(hmsg, &data->dest_addr);
	}

	/* free data */
	free_ssdp_event_handler_data(data);
}

/************************************************************************
 * Function : readFromSSDPSocket
 *
 * Parameters:
 *	IN SOCKET socket: SSDP socket
 *
 * Description:
 *	This function reads the data from the ssdp socket.
 *
 * Returns: void
 *
 ***************************************************************************/
void
readFromSSDPSocket( SOCKET socket )
{
    char *requestBuf = NULL;
    char staticBuf[BUFSIZE];
    struct sockaddr_storage __ss;
    ThreadPoolJob job;
    ssdp_thread_data *data = NULL;
    socklen_t socklen = sizeof( __ss );
    int byteReceived = 0;
    char ntop_buf[64];

    requestBuf = staticBuf;

    //in case memory
    //can't be allocated, still drain the 
    //socket using a static buffer

    data = ( ssdp_thread_data * )
        malloc( sizeof( ssdp_thread_data ) );

    if( data != NULL ) {
        //initialize parser

#ifdef INCLUDE_CLIENT_APIS
#ifdef UPNP_ENABLE_IPV6
        if( socket == gSsdpReqSocket4 || socket == gSsdpReqSocket6 ) {
            parser_response_init( &data->parser, HTTPMETHOD_MSEARCH );
        } else {
            parser_request_init( &data->parser );
        }
#else
        if( socket == gSsdpReqSocket4 ) {
            parser_response_init( &data->parser, HTTPMETHOD_MSEARCH );
        } else {
            parser_request_init( &data->parser );
        }

#endif
#else
        parser_request_init( &data->parser );
#endif

        //set size of parser buffer

        if( membuffer_set_size( &data->parser.msg.msg, BUFSIZE ) == 0 ) {
            //use this as the buffer for recv
            requestBuf = data->parser.msg.msg.buf;

        } else {
            free( data );
            data = NULL;
        }
    }
    byteReceived = recvfrom( socket, requestBuf,
                             BUFSIZE - 1, 0,
                             (struct sockaddr *)&__ss, &socklen );

    if( byteReceived > 0 ) {
        requestBuf[byteReceived] = '\0';

        if( __ss.ss_family == AF_INET )
            inet_ntop( AF_INET, &((struct sockaddr_in*)&__ss)->sin_addr, ntop_buf, sizeof(ntop_buf) );
#ifdef UPNP_ENABLE_IPV6
        else if( __ss.ss_family == AF_INET6 )
            inet_ntop( AF_INET6, &((struct sockaddr_in6*)&__ss)->sin6_addr, ntop_buf, sizeof(ntop_buf) );
#endif
        else
            strncpy( ntop_buf, "<Invalid address family>", sizeof(ntop_buf) );

        UpnpPrintf( UPNP_INFO, SSDP,
            __FILE__, __LINE__,
            "Start of received response ----------------------------------------------------\n"
            "%s\n"
            "End of received response ------------------------------------------------------\n"
            "From host %s\n",
            requestBuf,
            ntop_buf );
        UpnpPrintf( UPNP_PACKET, SSDP, __FILE__, __LINE__,
            "Start of received multicast packet --------------------------------------------\n"
            "%s\n"
            "End of received multicast packet ----------------------------------------------\n",
            requestBuf );
        //add thread pool job to handle request
        if( data != NULL ) {
            data->parser.msg.msg.length += byteReceived;
            // null-terminate
            data->parser.msg.msg.buf[byteReceived] = 0;
            memcpy( &data->dest_addr, &__ss, sizeof(__ss) );
            TPJobInit( &job, ( start_routine )
                       ssdp_event_handler_thread, data );
            TPJobSetFreeFunction( &job, free_ssdp_event_handler_data );
            TPJobSetPriority( &job, MED_PRIORITY );

            if( ThreadPoolAdd( &gRecvThreadPool, &job, NULL ) != 0 ) {
                free_ssdp_event_handler_data( data );
            }
        }
    } else {
        free_ssdp_event_handler_data( data );
    }
}


/************************************************************************
 * Function : get_ssdp_sockets								
 *
 * Parameters:
 *	OUT MiniServerSockArray *out: Array of SSDP sockets
 *
 * Description:
 *	This function creates the IPv4 and IPv6 ssdp sockets required by the
 *  control point and device operation.
 *
 * Returns: int
 *	return UPNP_E_SUCCESS if successful else returns appropriate error
 ***************************************************************************/
int get_ssdp_sockets(MiniServerSockArray *out)
{
	int retVal;

        out->ssdpReqSock4 = INVALID_SOCKET;
        out->ssdpReqSock6 = INVALID_SOCKET;

#if INCLUDE_CLIENT_APIS
	/* Create the IPv4 socket for SSDP REQUESTS */
	if(strlen(gIF_IPV4) > 0) {
		retVal = create_ssdp_sock_reqv4(&out->ssdpReqSock4);
		if(retVal != UPNP_E_SUCCESS) {
			return retVal;
		}
		/* For use by ssdp control point. */
		gSsdpReqSocket4 = out->ssdpReqSock4;
	} else {
		out->ssdpReqSock4 = INVALID_SOCKET;
	}

	/* Create the IPv6 socket for SSDP REQUESTS */
#ifdef UPNP_ENABLE_IPV6
	if (strlen(gIF_IPV6) > 0) {
		retVal = create_ssdp_sock_reqv6(&out->ssdpReqSock6);
		if (retVal != UPNP_E_SUCCESS) {
			shutdown(out->ssdpReqSock4, SD_BOTH);
			UpnpCloseSocket(out->ssdpReqSock4);
			return retVal;
		}
		/* For use by ssdp control point. */
		gSsdpReqSocket6 = out->ssdpReqSock6;
	} else {
		out->ssdpReqSock6 = INVALID_SOCKET;
	}
#endif  //IPv6


#endif /* INCLUDE_CLIENT_APIS */

	/* Create the IPv4 socket for SSDP */
	if (strlen(gIF_IPV4) > 0) {
		retVal = create_ssdp_sock_v4(&out->ssdpSock4);
		if (retVal != UPNP_E_SUCCESS) {
#ifdef INCLUDE_CLIENT_APIS
			shutdown(out->ssdpReqSock4, SD_BOTH);
			UpnpCloseSocket(out->ssdpReqSock4);
			shutdown(out->ssdpReqSock6, SD_BOTH);
			UpnpCloseSocket(out->ssdpReqSock6);
#endif
			return retVal;
		}
	} else {
		out->ssdpSock4 = INVALID_SOCKET;
	}

	/* Create the IPv6 socket for SSDP */
#ifdef UPNP_ENABLE_IPV6
	if (strlen(gIF_IPV6) > 0) {
		retVal = create_ssdp_sock_v6(&out->ssdpSock6);
		if (retVal != UPNP_E_SUCCESS) {
			shutdown(out->ssdpSock4, SD_BOTH);
			UpnpCloseSocket(out->ssdpSock4);
#ifdef INCLUDE_CLIENT_APIS
			shutdown(out->ssdpReqSock4, SD_BOTH);
			UpnpCloseSocket(out->ssdpReqSock4);
			shutdown(out->ssdpReqSock6, SD_BOTH);
			UpnpCloseSocket(out->ssdpReqSock6);
#endif
			return retVal;
		}
	} else {
		out->ssdpSock6 = INVALID_SOCKET;
	}

	if (strlen(gIF_IPV6_ULA_GUA) > 0) {
		retVal = create_ssdp_sock_v6_ula_gua(&out->ssdpSock6UlaGua);
		if (retVal != UPNP_E_SUCCESS) {
			shutdown(out->ssdpSock4, SD_BOTH);
			UpnpCloseSocket(out->ssdpSock4);
			shutdown(out->ssdpSock6, SD_BOTH);
			UpnpCloseSocket(out->ssdpSock6);
#ifdef INCLUDE_CLIENT_APIS
			shutdown(out->ssdpReqSock4, SD_BOTH);
			UpnpCloseSocket(out->ssdpReqSock4);
			shutdown(out->ssdpReqSock6, SD_BOTH);
			UpnpCloseSocket(out->ssdpReqSock6);
#endif
			return retVal;
		}
	} else {
		out->ssdpSock6UlaGua = INVALID_SOCKET;
	}
#endif //IPv6


	return UPNP_E_SUCCESS;
}


#if INCLUDE_CLIENT_APIS
/************************************************************************
 * Function : create_ssdp_sock_reqv4
 *
 * Parameters:
 *	IN SOCKET* ssdpReqSock: SSDP IPv4 request socket to be created
 *
 * Description:
 *	This function creates the SSDP IPv4 socket to be used by the control
 *  point.
 *
 * Returns:
 *  UPNP_E_SUCCESS on successful socket creation.
 ***************************************************************************/
int create_ssdp_sock_reqv4( SOCKET* ssdpReqSock )
{
    char errorBuffer[ERROR_BUFFER_LEN];
    u_char ttl = 4;

    *ssdpReqSock = socket( AF_INET, SOCK_DGRAM, 0 );
    if ( *ssdpReqSock == -1 ) {
        strerror_r(errno, errorBuffer, ERROR_BUFFER_LEN);
        UpnpPrintf( UPNP_CRITICAL, SSDP, __FILE__, __LINE__,
            "Error in socket(): %s\n", errorBuffer );
        return UPNP_E_OUTOF_SOCKET;
    }

    setsockopt( *ssdpReqSock, IPPROTO_IP, IP_MULTICAST_TTL,
        &ttl, sizeof (ttl) );

    // just do it, regardless if fails or not.
    Make_Socket_NoBlocking( *ssdpReqSock );

    return UPNP_E_SUCCESS;
}


/************************************************************************
 * Function : create_ssdp_sock_reqv6
 *
 * Parameters:
 *	IN SOCKET* ssdpReqSock: SSDP IPv6 request socket to be created
 *
 * Description:
 *	This function creates the SSDP IPv6 socket to be used by the control
 *  point.
 *
 * Returns: void
 *
 ***************************************************************************/
#ifdef UPNP_ENABLE_IPV6
int create_ssdp_sock_reqv6( SOCKET* ssdpReqSock )
{
    char errorBuffer[ERROR_BUFFER_LEN];
    char hops = 1;

    *ssdpReqSock = socket( AF_INET6, SOCK_DGRAM, 0 );
    if ( *ssdpReqSock == -1 ) {
        strerror_r(errno, errorBuffer, ERROR_BUFFER_LEN);
        UpnpPrintf( UPNP_CRITICAL, SSDP, __FILE__, __LINE__,
            "Error in socket(): %s\n", errorBuffer );
        return UPNP_E_OUTOF_SOCKET;
    }

    // MUST use scoping of IPv6 addresses to control the propagation os SSDP 
    // messages instead of relying on the Hop Limit (Equivalent to the TTL 
    // limit in IPv4).
    setsockopt( *ssdpReqSock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS,
        &hops, sizeof(hops) );

    // just do it, regardless if fails or not.
    Make_Socket_NoBlocking( *ssdpReqSock );

    return UPNP_E_SUCCESS;
}
#endif // IPv6

#endif /* INCLUDE_CLIENT_APIS */


/************************************************************************
 * Function : create_ssdp_sock_v4
 *
 * Parameters:
 *	IN SOCKET* ssdpSock: SSDP IPv4 socket to be created
 *
 * Description:
 *	This function ...
 *
 * Returns: void
 *
 ***************************************************************************/
int create_ssdp_sock_v4( SOCKET* ssdpSock )
{
    char errorBuffer[ERROR_BUFFER_LEN];
    int onOff;
    u_char ttl = 4;
    struct ip_mreq ssdpMcastAddr;
    struct sockaddr_storage __ss;
    struct sockaddr_in *ssdpAddr4 = (struct sockaddr_in *)&__ss;
    int ret = 0;
    struct in_addr addr;


    *ssdpSock = socket( AF_INET, SOCK_DGRAM, 0 );
    if ( *ssdpSock == -1 ) {
        strerror_r(errno, errorBuffer, ERROR_BUFFER_LEN);
        UpnpPrintf( UPNP_CRITICAL, SSDP, __FILE__, __LINE__,
            "Error in socket(): %s\n", errorBuffer );

        return UPNP_E_OUTOF_SOCKET;
    }

    onOff = 1;
    ret = setsockopt( *ssdpSock, SOL_SOCKET, SO_REUSEADDR,
        (char*)&onOff, sizeof(onOff) );
    if ( ret == -1) {
        strerror_r(errno, errorBuffer, ERROR_BUFFER_LEN);
        UpnpPrintf( UPNP_CRITICAL, SSDP, __FILE__, __LINE__,
            "Error in setsockopt() SO_REUSEADDR: %s\n", errorBuffer );
        shutdown( *ssdpSock, SD_BOTH );
        UpnpCloseSocket( *ssdpSock );

        return UPNP_E_SOCKET_ERROR;
    }
    
#if defined(BSD) || defined(__OSX__) || defined(__APPLE__)
    onOff = 1;
    ret = setsockopt( *ssdpSock, SOL_SOCKET, SO_REUSEPORT,
        (char *)&onOff, sizeof(onOff) );
    if ( ret == -1 ) {
        strerror_r(errno, errorBuffer, ERROR_BUFFER_LEN);
        UpnpPrintf( UPNP_CRITICAL, SSDP, __FILE__, __LINE__,
            "Error in setsockopt() SO_REUSEPORT: %s\n", errorBuffer );
        shutdown( *ssdpSock, SD_BOTH );
        UpnpCloseSocket( *ssdpSock );

        return UPNP_E_SOCKET_ERROR;
    }
#endif /* BSD */

    memset( &__ss, 0, sizeof( __ss ) );
    ssdpAddr4->sin_family = AF_INET;
    ssdpAddr4->sin_addr.s_addr = htonl( INADDR_ANY );
    ssdpAddr4->sin_port = htons( SSDP_PORT );
    ret = bind( *ssdpSock, (struct sockaddr *)ssdpAddr4, sizeof(*ssdpAddr4) );
    if ( ret == -1 ) {
        strerror_r(errno, errorBuffer, ERROR_BUFFER_LEN);
        UpnpPrintf( UPNP_CRITICAL, SSDP, __FILE__, __LINE__,
            "Error in bind(), addr=0x%08X, port=%d: %s\n",
            INADDR_ANY, SSDP_PORT, errorBuffer );
            shutdown( *ssdpSock, SD_BOTH );
        UpnpCloseSocket( *ssdpSock );

        return UPNP_E_SOCKET_BIND;
    }

    memset( (void *)&ssdpMcastAddr, 0, sizeof (struct ip_mreq) );
    ssdpMcastAddr.imr_interface.s_addr = inet_addr( gIF_IPV4 );
    ssdpMcastAddr.imr_multiaddr.s_addr = inet_addr( SSDP_IP );
    ret = setsockopt( *ssdpSock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
        (char *)&ssdpMcastAddr, sizeof(struct ip_mreq) );
    if ( ret == -1 ) {
        strerror_r(errno, errorBuffer, ERROR_BUFFER_LEN);
        UpnpPrintf( UPNP_CRITICAL, SSDP, __FILE__, __LINE__,
            "Error in setsockopt() IP_ADD_MEMBERSHIP (join multicast group): %s\n",
            errorBuffer );
        shutdown( *ssdpSock, SD_BOTH );
        UpnpCloseSocket( *ssdpSock );

        return UPNP_E_SOCKET_ERROR;
    }

    /* Set multicast interface. */
    memset( (void *)&addr, 0, sizeof (struct in_addr) );
    addr.s_addr = inet_addr(gIF_IPV4);
    ret = setsockopt(*ssdpSock, IPPROTO_IP, IP_MULTICAST_IF,
        (char *)&addr, sizeof addr);
    if ( ret == -1 ) {
        strerror_r(errno, errorBuffer, ERROR_BUFFER_LEN);
        UpnpPrintf( UPNP_INFO, SSDP, __FILE__, __LINE__,
            "Error in setsockopt() IP_MULTICAST_IF (set multicast interface): %s\n",
            errorBuffer );
        /* This is probably not a critical error, so let's continue. */
    }

    /* result is not checked becuase it will fail in WinMe and Win9x. */
    ret = setsockopt( *ssdpSock, IPPROTO_IP,
        IP_MULTICAST_TTL, &ttl, sizeof (ttl) );

    onOff = 1;
    ret = setsockopt( *ssdpSock, SOL_SOCKET, SO_BROADCAST,
        (char*)&onOff, sizeof(onOff) );
    if( ret == -1) {
        strerror_r(errno, errorBuffer, ERROR_BUFFER_LEN);
        UpnpPrintf( UPNP_CRITICAL, SSDP, __FILE__, __LINE__,
            "Error in setsockopt() SO_BROADCAST (set broadcast): %s\n",
            errorBuffer );
        shutdown( *ssdpSock, SD_BOTH );
        UpnpCloseSocket( *ssdpSock );

        return UPNP_E_NETWORK_ERROR;
    }

    return UPNP_E_SUCCESS;
}


/************************************************************************
 * Function : create_ssdp_sock_v6
 *
 * Parameters:
 *	IN SOCKET* ssdpSock: SSDP IPv6 socket to be created
 *
 * Description:
 *	This function ...
 *
 * Returns: void
 *
 ***************************************************************************/
#ifdef UPNP_ENABLE_IPV6
int create_ssdp_sock_v6( SOCKET* ssdpSock )
{
    char errorBuffer[ERROR_BUFFER_LEN];
    struct ipv6_mreq ssdpMcastAddr;
    struct sockaddr_storage __ss;
    struct sockaddr_in6 *ssdpAddr6 = (struct sockaddr_in6 *)&__ss;
    int onOff;
    int ret = 0;

    *ssdpSock = socket( AF_INET6, SOCK_DGRAM, 0 );
    if ( *ssdpSock == -1 ) {
        strerror_r(errno, errorBuffer, ERROR_BUFFER_LEN);
        UpnpPrintf( UPNP_CRITICAL, SSDP, __FILE__, __LINE__,
            "Error in socket(): %s\n", errorBuffer );

        return UPNP_E_OUTOF_SOCKET;
    }

    onOff = 1;
    ret = setsockopt( *ssdpSock, SOL_SOCKET, SO_REUSEADDR,
        (char*)&onOff, sizeof(onOff) );
    if ( ret == -1) {
        strerror_r(errno, errorBuffer, ERROR_BUFFER_LEN);
        UpnpPrintf( UPNP_CRITICAL, SSDP, __FILE__, __LINE__,
            "Error in setsockopt() SO_REUSEADDR: %s\n", errorBuffer );
        shutdown( *ssdpSock, SD_BOTH );
        UpnpCloseSocket( *ssdpSock );

        return UPNP_E_SOCKET_ERROR;
    }
    
#if defined(BSD) || defined(__OSX__) || defined(__APPLE__)
    onOff = 1;
    ret = setsockopt( *ssdpSock, SOL_SOCKET, SO_REUSEPORT,
        (char*)&onOff, sizeof (onOff) );
    if ( ret == -1 ) {
        strerror_r(errno, errorBuffer, ERROR_BUFFER_LEN);
        UpnpPrintf( UPNP_CRITICAL, SSDP, __FILE__, __LINE__,
            "Error in setsockopt() SO_REUSEPORT: %s\n", errorBuffer );
        shutdown( *ssdpSock, SD_BOTH );
        UpnpCloseSocket( *ssdpSock );

        return UPNP_E_SOCKET_ERROR;
    }
#endif /* BSD */

    memset( &__ss, 0, sizeof( __ss ) );
    ssdpAddr6->sin6_family = AF_INET6;
    ssdpAddr6->sin6_addr = in6addr_any;
    ssdpAddr6->sin6_scope_id = gIF_INDEX;
    ssdpAddr6->sin6_port = htons( SSDP_PORT );
    ret = bind( *ssdpSock, (struct sockaddr *)ssdpAddr6, sizeof(*ssdpAddr6) );
    if ( ret == -1 ) {
        strerror_r(errno, errorBuffer, ERROR_BUFFER_LEN);
        UpnpPrintf( UPNP_CRITICAL, SSDP, __FILE__, __LINE__,
            "Error in bind(), addr=0x%032lX, port=%d: %s\n",
            0lu, SSDP_PORT, errorBuffer );
        shutdown( *ssdpSock, SD_BOTH );
        UpnpCloseSocket( *ssdpSock );

        return UPNP_E_SOCKET_BIND;
    }

    memset( (void *)&ssdpMcastAddr, 0, sizeof(ssdpMcastAddr) );
    ssdpMcastAddr.ipv6mr_interface = gIF_INDEX;
    inet_pton( AF_INET6, SSDP_IPV6_LINKLOCAL, &ssdpMcastAddr.ipv6mr_multiaddr );
    ret = setsockopt( *ssdpSock, IPPROTO_IPV6, IPV6_JOIN_GROUP,
        (char *)&ssdpMcastAddr, sizeof(ssdpMcastAddr) );
    if ( ret == -1 ) {
        strerror_r(errno, errorBuffer, ERROR_BUFFER_LEN);
        UpnpPrintf( UPNP_CRITICAL, SSDP, __FILE__, __LINE__,
            "Error in setsockopt() IPV6_JOIN_GROUP (join multicast group): %s\n",
            errorBuffer );
        shutdown( *ssdpSock, SD_BOTH );
        UpnpCloseSocket( *ssdpSock );

        return UPNP_E_SOCKET_ERROR;
    }

    onOff = 1;
    ret = setsockopt( *ssdpSock, SOL_SOCKET, SO_BROADCAST,
        (char*)&onOff, sizeof(onOff) );
    if( ret == -1) {
        strerror_r(errno, errorBuffer, ERROR_BUFFER_LEN);
        UpnpPrintf( UPNP_CRITICAL, SSDP, __FILE__, __LINE__,
            "Error in setsockopt() SO_BROADCAST (set broadcast): %s\n",
            errorBuffer );
        shutdown( *ssdpSock, SD_BOTH );
        UpnpCloseSocket( *ssdpSock );

        return UPNP_E_NETWORK_ERROR;
    }

    return UPNP_E_SUCCESS;
}

#endif // IPv6

/************************************************************************
 * Function : create_ssdp_sock_v6_ula_gua
 *
 * Parameters:
 *	IN SOCKET* ssdpSock: SSDP IPv6 socket to be created
 *
 * Description:
 *	This function ...
 *
 * Returns: void
 *
 ***************************************************************************/
#ifdef UPNP_ENABLE_IPV6
int create_ssdp_sock_v6_ula_gua(SOCKET *ssdpSock)
{
	char errorBuffer[ERROR_BUFFER_LEN];
	struct ipv6_mreq ssdpMcastAddr;
	struct sockaddr_storage __ss;
	struct sockaddr_in6 *ssdpAddr6 = (struct sockaddr_in6 *)&__ss;
	int onOff;
	int ret = 0;

	*ssdpSock = socket(AF_INET6, SOCK_DGRAM, 0);
	if (*ssdpSock == -1) {
		strerror_r(errno, errorBuffer, ERROR_BUFFER_LEN);
		UpnpPrintf(UPNP_CRITICAL, SSDP, __FILE__, __LINE__,
				"Error in socket(): %s\n", errorBuffer);

		return UPNP_E_OUTOF_SOCKET;
	}

	onOff = 1;
	ret = setsockopt(*ssdpSock, SOL_SOCKET, SO_REUSEADDR,
		(char*)&onOff, sizeof(onOff) );
	if (ret == -1) {
		strerror_r(errno, errorBuffer, ERROR_BUFFER_LEN);
		UpnpPrintf(UPNP_CRITICAL, SSDP, __FILE__, __LINE__,
			"Error in setsockopt() SO_REUSEADDR: %s\n", errorBuffer);
		shutdown(*ssdpSock, SD_BOTH);
		UpnpCloseSocket(*ssdpSock);

		return UPNP_E_SOCKET_ERROR;
	}

#if defined(BSD) || defined(__OSX__) || defined(__APPLE__)
	onOff = 1;
	ret = setsockopt( *ssdpSock, SOL_SOCKET, SO_REUSEPORT,
			(char*)&onOff, sizeof (onOff) );
	if ( ret == -1 ) {
		strerror_r(errno, errorBuffer, ERROR_BUFFER_LEN);
		UpnpPrintf( UPNP_CRITICAL, SSDP, __FILE__, __LINE__,
				"Error in setsockopt() SO_REUSEPORT: %s\n", errorBuffer );
		shutdown( *ssdpSock, SD_BOTH );
		UpnpCloseSocket( *ssdpSock );

		return UPNP_E_SOCKET_ERROR;
	}
#endif /* BSD */

	memset( &__ss, 0, sizeof( __ss ) );
	ssdpAddr6->sin6_family = AF_INET6;
	ssdpAddr6->sin6_addr = in6addr_any;
	ssdpAddr6->sin6_scope_id = gIF_INDEX;
	ssdpAddr6->sin6_port = htons( SSDP_PORT );
	ret = bind( *ssdpSock, (struct sockaddr *)ssdpAddr6, sizeof(*ssdpAddr6) );
	if ( ret == -1 ) {
		strerror_r(errno, errorBuffer, ERROR_BUFFER_LEN);
		UpnpPrintf( UPNP_CRITICAL, SSDP, __FILE__, __LINE__,
				"Error in bind(), addr=0x%032lX, port=%d: %s\n",
				0lu, SSDP_PORT, errorBuffer );
		shutdown( *ssdpSock, SD_BOTH );
		UpnpCloseSocket( *ssdpSock );

		return UPNP_E_SOCKET_BIND;
	}

	memset( (void *)&ssdpMcastAddr, 0, sizeof(ssdpMcastAddr) );
	ssdpMcastAddr.ipv6mr_interface = gIF_INDEX;

	/* SITE LOCAL */
	inet_pton( AF_INET6, SSDP_IPV6_SITELOCAL, &ssdpMcastAddr.ipv6mr_multiaddr );
	ret = setsockopt( *ssdpSock, IPPROTO_IPV6, IPV6_JOIN_GROUP,
			(char *)&ssdpMcastAddr, sizeof(ssdpMcastAddr) );
	if ( ret == -1 ) {
		strerror_r(errno, errorBuffer, ERROR_BUFFER_LEN);
		UpnpPrintf( UPNP_CRITICAL, SSDP, __FILE__, __LINE__,
				"Error in setsockopt() IPV6_JOIN_GROUP (join multicast group): %s\n",
				errorBuffer );
		shutdown( *ssdpSock, SD_BOTH );
		UpnpCloseSocket( *ssdpSock );

		return UPNP_E_SOCKET_ERROR;
	}

	onOff = 1;
	ret = setsockopt( *ssdpSock, SOL_SOCKET, SO_BROADCAST,
			(char*)&onOff, sizeof(onOff) );
	if( ret == -1) {
		strerror_r(errno, errorBuffer, ERROR_BUFFER_LEN);
		UpnpPrintf( UPNP_CRITICAL, SSDP, __FILE__, __LINE__,
				"Error in setsockopt() SO_BROADCAST (set broadcast): %s\n",
				errorBuffer );
		shutdown( *ssdpSock, SD_BOTH );
		UpnpCloseSocket( *ssdpSock );

		return UPNP_E_NETWORK_ERROR;
	}

	return UPNP_E_SUCCESS;
}
#endif   //IPv6

#endif /* EXCLUDE_SSDP */

