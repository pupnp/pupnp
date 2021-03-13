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

/*!
 * \addtogroup SSDPlib
 *
 * @{
 *
 * \file
 */

#include "config.h"

#ifdef INCLUDE_CLIENT_APIS
#if EXCLUDE_SSDP == 0

#include "SSDPResultData.h"
#include "SSDPResultDataCallback.h"
#include "ThreadPool.h"
#include "UpnpInet.h"
#include "httpparser.h"
#include "httpreadwrite.h"
#include "ssdplib.h"
#include "statcodes.h"
#include "unixutil.h"
#include "upnpapi.h"
#include "winutil.h"

#include <stdio.h>

#ifdef _WIN32
#include <string.h>
#if defined(_MSC_VER) && _MSC_VER < 1900
#define snprintf _snprintf
#endif
#endif /* _WIN32 */

/*!
 * \brief Sends a callback to the control point application with a SEARCH
 * result.
 */
static void send_search_result(
        /* [in] Search reply from the device. */
        void *data)
{
        SSDPResultData *temp = (SSDPResultData *)data;

        SSDPResultData_Callback(temp);
        SSDPResultData_delete(temp);
}

static UPNP_INLINE int max_int(int a, int b)
{
        if (a > b) {
                return a;
        } else {
                return b;
        }
}

static UPNP_INLINE size_t min_size_t(size_t a, size_t b)
{
        if (a < b) {
                return a;
        } else {
                return b;
        }
}

void ssdp_handle_ctrlpt_msg(
        http_message_t *hmsg, struct sockaddr_storage *dest_addr, int timeout)
{
        int handle;
        int handle_start;
        struct Handle_Info *ctrlpt_info = NULL;
        memptr hdr_value;
        /* byebye or alive */
        int is_byebye;
        UpnpDiscovery *param = UpnpDiscovery_new();
        int expires;
        int ret;
        SsdpEvent event;
        int nt_found;
        int usn_found;
        int st_found;
        char save_char;
        Upnp_EventType event_type;
        Upnp_FunPtr ctrlpt_callback;
        void *ctrlpt_cookie;
        ListNode *node = NULL;
        SsdpSearchArg *searchArg = NULL;
        int matched = 0;
        SSDPResultData *threadData = NULL;
        ThreadPoolJob job;

        /* we are assuming that there can be only one client supported at a time
         */
        HandleReadLock();

        if (GetClientHandleInfo(&handle_start, &ctrlpt_info) != HND_CLIENT) {
                HandleUnlock();
                goto end_ssdp_handle_ctrlpt_msg;
        }
        HandleUnlock();
        /* search timeout */
        if (timeout) {
                for (handle = handle_start; handle < NUM_HANDLE; handle++) {
                        HandleLock();

                        /* get client info */
                        if (GetHandleInfo(handle, &ctrlpt_info) != HND_CLIENT) {
                                HandleUnlock();
                                continue;
                        }
                        /* copy */
                        ctrlpt_callback = ctrlpt_info->Callback;
                        ctrlpt_cookie = ctrlpt_info->Cookie;
                        HandleUnlock();

                        ctrlpt_callback(UPNP_DISCOVERY_SEARCH_TIMEOUT,
                                NULL,
                                ctrlpt_cookie);
                }
                goto end_ssdp_handle_ctrlpt_msg;
        }

        UpnpDiscovery_set_ErrCode(param, UPNP_E_SUCCESS);
        /* MAX-AGE, assume error */
        expires = -1;
        UpnpDiscovery_set_Expires(param, expires);
        if (httpmsg_find_hdr(hmsg, HDR_CACHE_CONTROL, &hdr_value) != NULL) {
                ret = matchstr(hdr_value.buf,
                        hdr_value.length,
                        "%imax-age = %d%0",
                        &expires);
                UpnpDiscovery_set_Expires(param, expires);
                if (ret != PARSE_OK)
                        goto end_ssdp_handle_ctrlpt_msg;
        }
        /* DATE */
        if (httpmsg_find_hdr(hmsg, HDR_DATE, &hdr_value) != NULL) {
                UpnpDiscovery_strcpy_Date(param, hdr_value.buf);
        }
        /* dest addr */
        UpnpDiscovery_set_DestAddr(param, dest_addr);
        /* EXT */
        if (httpmsg_find_hdr(hmsg, HDR_EXT, &hdr_value) != NULL) {
                UpnpDiscovery_strncpy_Ext(
                        param, hdr_value.buf, hdr_value.length);
        }
        /* LOCATION */
        if (httpmsg_find_hdr(hmsg, HDR_LOCATION, &hdr_value) != NULL) {
                UpnpDiscovery_strncpy_Location(
                        param, hdr_value.buf, hdr_value.length);
        }
        /* SERVER / USER-AGENT */
        if (httpmsg_find_hdr(hmsg, HDR_SERVER, &hdr_value) != NULL ||
                httpmsg_find_hdr(hmsg, HDR_USER_AGENT, &hdr_value) != NULL) {
                UpnpDiscovery_strncpy_Os(
                        param, hdr_value.buf, hdr_value.length);
        }
        /* clear everything */
        event.UDN[0] = '\0';
        event.DeviceType[0] = '\0';
        event.ServiceType[0] = '\0';
        nt_found = 0;
        if (httpmsg_find_hdr(hmsg, HDR_NT, &hdr_value) != NULL) {
                save_char = hdr_value.buf[hdr_value.length];
                hdr_value.buf[hdr_value.length] = '\0';
                nt_found = (ssdp_request_type(hdr_value.buf, &event) == 0);
                hdr_value.buf[hdr_value.length] = save_char;
        }
        usn_found = 0;
        if (httpmsg_find_hdr(hmsg, HDR_USN, &hdr_value) != NULL) {
                save_char = hdr_value.buf[hdr_value.length];
                hdr_value.buf[hdr_value.length] = '\0';
                usn_found = (unique_service_name(hdr_value.buf, &event) == 0);
                hdr_value.buf[hdr_value.length] = save_char;
        }
        if (nt_found || usn_found) {
                UpnpDiscovery_strcpy_DeviceID(param, event.UDN);
                UpnpDiscovery_strcpy_DeviceType(param, event.DeviceType);
                UpnpDiscovery_strcpy_ServiceType(param, event.ServiceType);
        }
        /* ADVERT. OR BYEBYE */
        if (hmsg->is_request) {
                /* use NTS hdr to determine advert., or byebye */
                if (httpmsg_find_hdr(hmsg, HDR_NTS, &hdr_value) == NULL) {
                        /* error; NTS header not found */
                        goto end_ssdp_handle_ctrlpt_msg;
                }
                if (memptr_cmp(&hdr_value, "ssdp:alive") == 0) {
                        is_byebye = 0;
                } else if (memptr_cmp(&hdr_value, "ssdp:byebye") == 0) {
                        is_byebye = 1;
                } else {
                        /* bad value */
                        goto end_ssdp_handle_ctrlpt_msg;
                }
                if (is_byebye) {
                        /* check device byebye */
                        if (!nt_found || !usn_found) {
                                /* bad byebye */
                                goto end_ssdp_handle_ctrlpt_msg;
                        }
                        event_type = UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE;
                } else {
                        /* check advertisement.
                         * Expires is valid if positive. This is for testing
                         * only. Expires should be greater than 1800 (30 mins)
                         */
                        if (!nt_found || !usn_found ||
                                UpnpString_get_Length(
                                        UpnpDiscovery_get_Location(param)) ==
                                        0 ||
                                UpnpDiscovery_get_Expires(param) <= 0) {
                                /* bad advertisement */
                                goto end_ssdp_handle_ctrlpt_msg;
                        }
                        event_type = UPNP_DISCOVERY_ADVERTISEMENT_ALIVE;
                }
                /* call callback */
                for (handle = handle_start; handle < NUM_HANDLE; handle++) {
                        HandleLock();

                        /* get client info */
                        if (GetHandleInfo(handle, &ctrlpt_info) != HND_CLIENT) {
                                HandleUnlock();
                                continue;
                        }
                        /* copy */
                        ctrlpt_callback = ctrlpt_info->Callback;
                        ctrlpt_cookie = ctrlpt_info->Cookie;
                        HandleUnlock();

                        ctrlpt_callback(event_type, param, ctrlpt_cookie);
                }
        } else {
                /* reply (to a SEARCH) */
                /* only checking to see if there is a valid ST header */
                st_found = 0;
                if (httpmsg_find_hdr(hmsg, HDR_ST, &hdr_value) != NULL) {
                        save_char = hdr_value.buf[hdr_value.length];
                        hdr_value.buf[hdr_value.length] = '\0';
                        st_found =
                                ssdp_request_type(hdr_value.buf, &event) == 0;
                        hdr_value.buf[hdr_value.length] = save_char;
                }
                if (hmsg->status_code != HTTP_OK ||
                        UpnpDiscovery_get_Expires(param) <= 0 ||
                        UpnpString_get_Length(
                                UpnpDiscovery_get_Location(param)) == 0 ||
                        !usn_found || !st_found) {
                        /* bad reply */
                        goto end_ssdp_handle_ctrlpt_msg;
                }
                /* check each current search */
                for (handle = handle_start; handle < NUM_HANDLE; handle++) {
                        HandleLock();

                        /* get client info */
                        if (GetHandleInfo(handle, &ctrlpt_info) != HND_CLIENT) {
                                HandleUnlock();
                                continue;
                        }
                        /* copy */
                        ctrlpt_callback = ctrlpt_info->Callback;
                        /* TODO: Check, clang reports:
                         * "value stores to 'ctrlpt_cookie' is never read". */
                        ctrlpt_cookie = ctrlpt_info->Cookie;

                        node = ListHead(&ctrlpt_info->SsdpSearchList);
                        /* temporary add null termination */
                        /*save_char = hdr_value.buf[ hdr_value.length ]; */
                        /*hdr_value.buf[ hdr_value.length ] = '\0'; */
                        while (node != NULL) {
                                searchArg = node->item;
                                /* check for match of ST header and search
                                 * target */
                                switch (searchArg->requestType) {
                                case SSDP_ALL:
                                        matched = 1;
                                        break;
                                case SSDP_ROOTDEVICE:
                                        matched = (event.RequestType ==
                                                SSDP_ROOTDEVICE);
                                        break;
                                case SSDP_DEVICEUDN:
                                        matched = !strncmp(
                                                searchArg->searchTarget,
                                                hdr_value.buf,
                                                hdr_value.length);
                                        break;
                                case SSDP_DEVICETYPE: {
                                        size_t m = min_size_t(hdr_value.length,
                                                strlen(searchArg->searchTarget));
                                        matched = !strncmp(
                                                searchArg->searchTarget,
                                                hdr_value.buf,
                                                m);
                                        break;
                                }
                                case SSDP_SERVICE: {
                                        size_t m = min_size_t(hdr_value.length,
                                                strlen(searchArg->searchTarget));
                                        matched = !strncmp(
                                                searchArg->searchTarget,
                                                hdr_value.buf,
                                                m);
                                        break;
                                }
                                default:
                                        matched = 0;
                                        break;
                                }
                                if (matched) {
                                        /* schedule call back */
                                        threadData = SSDPResultData_new();
                                        if (threadData != NULL) {
                                                SSDPResultData_set_Param(
                                                        threadData, param);
                                                SSDPResultData_set_Cookie(
                                                        threadData,
                                                        searchArg->cookie);
                                                SSDPResultData_set_CtrlptCallback(
                                                        threadData,
                                                        ctrlpt_callback);
                                                memset(&job, 0, sizeof(job));

                                                TPJobInit(&job,
                                                        (start_routine)
                                                                send_search_result,
                                                        threadData);
                                                TPJobSetPriority(
                                                        &job, MED_PRIORITY);
                                                TPJobSetFreeFunction(&job,
                                                        (free_routine)free);
                                                if (ThreadPoolAdd(
                                                            &gRecvThreadPool,
                                                            &job,
                                                            NULL) != 0) {
                                                        SSDPResultData_delete(
                                                                threadData);
                                                }
                                        }
                                }
                                node = ListNext(
                                        &ctrlpt_info->SsdpSearchList, node);
                        }

                        HandleUnlock();
                        /*ctrlpt_callback( UPNP_DISCOVERY_SEARCH_RESULT, param,
                         * ctrlpt_cookie ); */
                }
        }

end_ssdp_handle_ctrlpt_msg:
        UpnpDiscovery_delete(param);
}

#define COPY_OR_RETURN_IF_TOO_SMALL(len, buf) \
        do { \
                sz = (len) + 1; \
                if (current_buf + sz > last_in_buf) { \
                        return UPNP_E_BUFFER_TOO_SMALL; \
                } \
                memcpy(current_buf, buf, sz); \
                current_buf += sz; \
        } while (0)

/*!
 * \brief Creates a HTTP search request packet depending on the input
 * parameter.
 */
static int CreateClientRequestPacket(
        /*! [in,out] Output string in HTTP format. */
        char *RqstBuf,
        /*! [in] RqstBuf size. */
        size_t RqstBufSize,
        /*! [in] Search Target. */
        int Mx,
        /*! [in] Number of seconds to wait to collect all the responses. */
        char *SearchTarget,
        /*! [in] search address family. */
        int AddressFamily,
        /*! Is ULA_GUA call? */
        int ula_gua)
{
        int rc;
        char *const last_in_buf = RqstBuf + RqstBufSize - 1;
        char *current_buf = RqstBuf;
        size_t sz;
        char TempBuf[COMMAND_LEN];
        const char *command = "M-SEARCH * HTTP/1.1\r\n";
        const char *man = "MAN: \"ssdp:discover\"\r\n";

        COPY_OR_RETURN_IF_TOO_SMALL(strlen(command), command);

        switch (AddressFamily) {
        case AF_INET:
                rc = snprintf(TempBuf,
                        sizeof TempBuf,
                        "HOST: %s:%d\r\n",
                        SSDP_IP,
                        SSDP_PORT);
                break;
        case AF_INET6:
                if (ula_gua) {
                        rc = snprintf(TempBuf,
                                sizeof TempBuf,
                                "HOST: [%s]:%d\r\n",
                                SSDP_IPV6_SITELOCAL,
                                SSDP_PORT);
                } else {
                        rc = snprintf(TempBuf,
                                sizeof TempBuf,
                                "HOST: [%s]:%d\r\n",
                                SSDP_IPV6_LINKLOCAL,
                                SSDP_PORT);
                }
                break;
        default:
                return UPNP_E_INVALID_ARGUMENT;
        }
        /* From the snprintf man page:
         * The  functions  snprintf()  and  vsnprintf() do not write more than
         * size bytes (including the terminating null byte ('\0')).  If the
         * output was truncated due to this limit, then the return value is the
         * number of characters (excluding the terminating null  byte)  which
         * would have been written  to  the  final  string  if enough space had
         * been available. Thus, a return value of size or more means that the
         * output was truncated. */
        if (rc < 0 || rc >= (int)sizeof TempBuf) {
                return UPNP_E_INTERNAL_ERROR;
        }
        COPY_OR_RETURN_IF_TOO_SMALL((size_t)rc, TempBuf);
        COPY_OR_RETURN_IF_TOO_SMALL(strlen(man), man);
        if (Mx > 0) {
                rc = snprintf(TempBuf, sizeof TempBuf, "MX: %d\r\n", Mx);
                if (rc < 0 || rc >= (int)sizeof TempBuf) {
                        return UPNP_E_INTERNAL_ERROR;
                }
                COPY_OR_RETURN_IF_TOO_SMALL((size_t)rc, TempBuf);
        }
        if (SearchTarget) {
                rc = snprintf(
                        TempBuf, sizeof TempBuf, "ST: %s\r\n", SearchTarget);
                if (rc < 0 || rc >= (int)sizeof TempBuf) {
                        return UPNP_E_INTERNAL_ERROR;
                }
                COPY_OR_RETURN_IF_TOO_SMALL((size_t)rc, TempBuf);
        }
        COPY_OR_RETURN_IF_TOO_SMALL(2, "\r\n");

        return UPNP_E_SUCCESS;
}

/*!
 * \brief
 */
static void searchExpired(
        /* [in] . */
        void *arg)
{
        int id = ((SsdpSearchExpArg *)arg)->timeoutEventId;
        int handle = ((SsdpSearchExpArg *)arg)->handle;
        struct Handle_Info *ctrlpt_info = NULL;

        /* remove search Target from list and call client back */
        ListNode *node = NULL;
        SsdpSearchArg *item;
        Upnp_FunPtr ctrlpt_callback;
        void *cookie = NULL;
        int found = 0;

        HandleLock();

        /* remove search target from search list */
        if (GetHandleInfo(handle, &ctrlpt_info) != HND_CLIENT) {
                free(arg);
                HandleUnlock();
                return;
        }
        ctrlpt_callback = ctrlpt_info->Callback;
        node = ListHead(&ctrlpt_info->SsdpSearchList);
        while (node != NULL) {
                item = (SsdpSearchArg *)node->item;
                if (item->timeoutEventId == id) {
                        free(item->searchTarget);
                        cookie = item->cookie;
                        found = 1;
                        item->searchTarget = NULL;
                        free(item);
                        ListDelNode(&ctrlpt_info->SsdpSearchList, node, 0);
                        break;
                }
                node = ListNext(&ctrlpt_info->SsdpSearchList, node);
        }
        HandleUnlock();

        if (found)
                ctrlpt_callback(UPNP_DISCOVERY_SEARCH_TIMEOUT, NULL, cookie);

        free(arg);
}

int SearchByTarget(int Hnd, int Mx, char *St, void *Cookie)
{
        char errorBuffer[256];
        int *id = NULL;
        int ret = 0;
        char ReqBufv4[BUFSIZE];
#ifdef UPNP_ENABLE_IPV6
        char ReqBufv6[BUFSIZE];
        char ReqBufv6UlaGua[BUFSIZE];
#endif
        struct sockaddr_storage __ss_v4;
#ifdef UPNP_ENABLE_IPV6
        struct sockaddr_storage __ss_v6;
#endif
        struct sockaddr_in *destAddr4 = (struct sockaddr_in *)&__ss_v4;
#ifdef UPNP_ENABLE_IPV6
        struct sockaddr_in6 *destAddr6 = (struct sockaddr_in6 *)&__ss_v6;
#endif
        fd_set wrSet;
        SsdpSearchArg *newArg = NULL;
        SsdpSearchExpArg *expArg = NULL;
        int timeTillRead = 0;
        struct Handle_Info *ctrlpt_info = NULL;
        enum SsdpSearchType requestType;
        struct in_addr addrv4;
        SOCKET max_fd = 0;
        int retVal;

        /*ThreadData *ThData; */
        ThreadPoolJob job;

        if (strlen(gIF_IPV4) > (size_t)0 &&
                !inet_pton(AF_INET, gIF_IPV4, &addrv4)) {
                return UPNP_E_INVALID_PARAM;
        }

        memset(&job, 0, sizeof(job));

        requestType = ssdp_request_type1(St);
        if (requestType == SSDP_SERROR)
                return UPNP_E_INVALID_PARAM;
        UpnpPrintf(
                UPNP_INFO, SSDP, __FILE__, __LINE__, "Inside SearchByTarget\n");
        timeTillRead = Mx;
        if (timeTillRead < MIN_SEARCH_TIME)
                timeTillRead = MIN_SEARCH_TIME;
        else if (timeTillRead > MAX_SEARCH_TIME)
                timeTillRead = MAX_SEARCH_TIME;
        retVal = CreateClientRequestPacket(
                ReqBufv4, sizeof(ReqBufv4), timeTillRead, St, AF_INET, 0);
        if (retVal != UPNP_E_SUCCESS)
                return retVal;
#ifdef UPNP_ENABLE_IPV6
        retVal = CreateClientRequestPacket(
                ReqBufv6, sizeof(ReqBufv6), timeTillRead, St, AF_INET6, 0);
        if (retVal != UPNP_E_SUCCESS)
                return retVal;
        retVal = CreateClientRequestPacket(ReqBufv6UlaGua,
                sizeof(ReqBufv6UlaGua),
                timeTillRead,
                St,
                AF_INET6,
                1);
        if (retVal != UPNP_E_SUCCESS)
                return retVal;
#endif

        memset(&__ss_v4, 0, sizeof(__ss_v4));
        destAddr4->sin_family = (sa_family_t)AF_INET;
        inet_pton(AF_INET, SSDP_IP, &destAddr4->sin_addr);
        destAddr4->sin_port = htons(SSDP_PORT);

#ifdef UPNP_ENABLE_IPV6
        memset(&__ss_v6, 0, sizeof(__ss_v6));
        destAddr6->sin6_family = (sa_family_t)AF_INET6;
        inet_pton(AF_INET6, SSDP_IPV6_SITELOCAL, &destAddr6->sin6_addr);
        destAddr6->sin6_port = htons(SSDP_PORT);
        destAddr6->sin6_scope_id = gIF_INDEX;
#endif

        /* add search criteria to list */
        HandleLock();
        if (GetHandleInfo(Hnd, &ctrlpt_info) != HND_CLIENT) {
                HandleUnlock();
                return UPNP_E_INTERNAL_ERROR;
        }
        newArg = (SsdpSearchArg *)malloc(sizeof(SsdpSearchArg));
        newArg->searchTarget = strdup(St);
        newArg->cookie = Cookie;
        newArg->requestType = requestType;

        expArg = (SsdpSearchExpArg *)malloc(sizeof(SsdpSearchExpArg));
        expArg->handle = Hnd;
        id = (int *)&(expArg->timeoutEventId);
        TPJobInit(&job, (start_routine)searchExpired, expArg);
        TPJobSetPriority(&job, MED_PRIORITY);
        TPJobSetFreeFunction(&job, (free_routine)free);
        /* Schedule a timeout event to remove search Arg */
        TimerThreadSchedule(
                &gTimerThread, timeTillRead, REL_SEC, &job, SHORT_TERM, id);
        newArg->timeoutEventId = *id;
        ListAddTail(&ctrlpt_info->SsdpSearchList, newArg);
        HandleUnlock();
        /* End of lock */

        FD_ZERO(&wrSet);
        if (gSsdpReqSocket4 != INVALID_SOCKET) {
                setsockopt(gSsdpReqSocket4,
                        IPPROTO_IP,
                        IP_MULTICAST_IF,
                        (char *)&addrv4,
                        sizeof(addrv4));
                FD_SET(gSsdpReqSocket4, &wrSet);
                max_fd = max_int(max_fd, gSsdpReqSocket4);
        }
#ifdef UPNP_ENABLE_IPV6
        if (gSsdpReqSocket6 != INVALID_SOCKET) {
                setsockopt(gSsdpReqSocket6,
                        IPPROTO_IPV6,
                        IPV6_MULTICAST_IF,
                        (char *)&gIF_INDEX,
                        sizeof(gIF_INDEX));
                FD_SET(gSsdpReqSocket6, &wrSet);
                max_fd = max_int(max_fd, gSsdpReqSocket6);
        }
#endif
        ret = select(max_fd + 1, NULL, &wrSet, NULL, NULL);
        if (ret == -1) {
                strerror_r(errno, errorBuffer, sizeof errorBuffer);
                UpnpPrintf(UPNP_INFO,
                        SSDP,
                        __FILE__,
                        __LINE__,
                        "SSDP_LIB: Error in select(): %s\n",
                        errorBuffer);
                UpnpCloseSocket(gSsdpReqSocket4);
#ifdef UPNP_ENABLE_IPV6
                UpnpCloseSocket(gSsdpReqSocket6);
#endif
                return UPNP_E_INTERNAL_ERROR;
        }
#ifdef UPNP_ENABLE_IPV6
        if (gSsdpReqSocket6 != INVALID_SOCKET &&
                FD_ISSET(gSsdpReqSocket6, &wrSet)) {
                int NumCopy = 0;

                while (NumCopy < NUM_SSDP_COPY) {
                        UpnpPrintf(UPNP_INFO,
                                SSDP,
                                __FILE__,
                                __LINE__,
                                ">>> SSDP SEND M-SEARCH >>>\n%s\n",
                                ReqBufv6UlaGua);
                        sendto(gSsdpReqSocket6,
                                ReqBufv6UlaGua,
                                strlen(ReqBufv6UlaGua),
                                0,
                                (struct sockaddr *)&__ss_v6,
                                sizeof(struct sockaddr_in6));
                        NumCopy++;
                        imillisleep(SSDP_PAUSE);
                }
                NumCopy = 0;
                inet_pton(AF_INET6, SSDP_IPV6_LINKLOCAL, &destAddr6->sin6_addr);
                while (NumCopy < NUM_SSDP_COPY) {
                        UpnpPrintf(UPNP_INFO,
                                SSDP,
                                __FILE__,
                                __LINE__,
                                ">>> SSDP SEND M-SEARCH >>>\n%s\n",
                                ReqBufv6);
                        sendto(gSsdpReqSocket6,
                                ReqBufv6,
                                strlen(ReqBufv6),
                                0,
                                (struct sockaddr *)&__ss_v6,
                                sizeof(struct sockaddr_in6));
                        NumCopy++;
                        imillisleep(SSDP_PAUSE);
                }
        }
#endif /* IPv6 */
        if (gSsdpReqSocket4 != INVALID_SOCKET &&
                FD_ISSET(gSsdpReqSocket4, &wrSet)) {
                int NumCopy = 0;
                while (NumCopy < NUM_SSDP_COPY) {
                        UpnpPrintf(UPNP_INFO,
                                SSDP,
                                __FILE__,
                                __LINE__,
                                ">>> SSDP SEND M-SEARCH >>>\n%s\n",
                                ReqBufv4);
                        sendto(gSsdpReqSocket4,
                                ReqBufv4,
                                strlen(ReqBufv4),
                                0,
                                (struct sockaddr *)&__ss_v4,
                                sizeof(struct sockaddr_in));
                        NumCopy++;
                        imillisleep(SSDP_PAUSE);
                }
        }

        return 1;
}
#endif /* EXCLUDE_SSDP */
#endif /* INCLUDE_CLIENT_APIS */

/* @} SSDPlib */
