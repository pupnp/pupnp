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


#include "config.h"


/*!
 * \file
 */


#if EXCLUDE_GENA == 0
#ifdef INCLUDE_CLIENT_APIS


#include "gena.h"
#include "httpparser.h"
#include "httpreadwrite.h"
#include "parsetools.h"
#include "statcodes.h"
#include "sysdep.h"
#include "uuid.h"
#include "upnpapi.h"

#ifdef WIN32
	#define snprintf _snprintf
#endif

extern ithread_mutex_t GlobalClientSubscribeMutex;


/*!
 * \brief This is a thread function to send the renewal just before the
 * subscription times out.
 */
static void GenaAutoRenewSubscription(
	/*! [in] Thread data(upnp_timeout *) needed to send the renewal. */
	IN void *input)
{
	upnp_timeout *event = (upnp_timeout *) input;
        struct Upnp_Event_Subscribe *sub_struct = (struct Upnp_Event_Subscribe *)event->Event;
	void *cookie;
	Upnp_FunPtr callback_fun;
	struct Handle_Info *handle_info;
	int send_callback = 0;
	int eventType = 0;
	int timeout = 0;
	int errCode = 0;
	UpnpString *tmpSID = UpnpString_new();

	if (AUTO_RENEW_TIME == 0) {
		UpnpPrintf( UPNP_INFO, GENA, __FILE__, __LINE__, "GENA SUB EXPIRED");
		sub_struct->ErrCode = UPNP_E_SUCCESS;
		send_callback = 1;
		eventType = UPNP_EVENT_SUBSCRIPTION_EXPIRED;
	} else {
		UpnpPrintf(UPNP_INFO, GENA, __FILE__, __LINE__, "GENA AUTO RENEW");
		timeout = sub_struct->TimeOut;
		UpnpString_set_String(tmpSID, sub_struct->Sid);
		errCode = genaRenewSubscription(
			event->handle,
			tmpSID,
			&timeout);
		sub_struct->ErrCode = errCode;
		sub_struct->TimeOut = timeout;
		if (errCode != UPNP_E_SUCCESS &&
		    errCode != GENA_E_BAD_SID &&
		    errCode != GENA_E_BAD_HANDLE) {
			send_callback = 1;
			eventType = UPNP_EVENT_AUTORENEWAL_FAILED;
		}
	}

	if (send_callback) {
		HandleReadLock();
		if( GetHandleInfo( event->handle, &handle_info ) != HND_CLIENT ) {
			HandleUnlock();
			free_upnp_timeout(event);
			goto end_function;
		}
		UpnpPrintf(UPNP_INFO, GENA, __FILE__, __LINE__, "HANDLE IS VALID");

		/* make callback */
		callback_fun = handle_info->Callback;
		cookie = handle_info->Cookie;
		HandleUnlock();
		callback_fun(eventType, event->Event, cookie);
	}

	free_upnp_timeout(event);

end_function:
	UpnpString_delete(tmpSID);
	return;
}


/*!
 * \brief Schedules a job to renew the subscription just before time out.
 *
 * \return GENA_E_SUCCESS if successful, otherwise returns the appropriate
 * 	error code.
 */
static int ScheduleGenaAutoRenew(
	/*! [in] Handle that also contains the subscription list. */
	IN int client_handle,
	/*! [in] The time out value of the subscription. */
	IN int TimeOut,
	/*! [in] Subscription being renewed. */
	IN ClientSubscription *sub)
{
	struct Upnp_Event_Subscribe *RenewEventStruct = NULL;
	upnp_timeout *RenewEvent = NULL;
	int return_code = GENA_SUCCESS;
	ThreadPoolJob job;
	const UpnpString *tmpSID = UpnpClientSubscription_get_SID(sub);
	const UpnpString *tmpEventURL = UpnpClientSubscription_get_EventURL(sub);

	memset(&job, 0, sizeof(job));

	if (TimeOut == UPNP_INFINITE) {
		return_code = GENA_SUCCESS;
		goto end_function;
	}

	RenewEventStruct = (struct Upnp_Event_Subscribe *)malloc(sizeof (struct Upnp_Event_Subscribe));
	if (RenewEventStruct == NULL) {
		return_code = UPNP_E_OUTOF_MEMORY;
		goto end_function;
	}
	memset(RenewEventStruct, 0, sizeof(struct Upnp_Event_Subscribe));

	RenewEvent = (upnp_timeout *) malloc(sizeof(upnp_timeout));
	if (RenewEvent == NULL) {
		free(RenewEventStruct);
		return_code = UPNP_E_OUTOF_MEMORY;
		goto end_function;
	}
	memset(RenewEvent, 0, sizeof(upnp_timeout));

	/* schedule expire event */
	RenewEventStruct->ErrCode = UPNP_E_SUCCESS;
	RenewEventStruct->TimeOut = TimeOut;
	strncpy(RenewEventStruct->Sid, UpnpString_get_String(tmpSID),
		sizeof(RenewEventStruct->Sid) - 1);
	strncpy(RenewEventStruct->PublisherUrl,
		UpnpString_get_String(tmpEventURL), NAME_SIZE - 1);

	/* RenewEvent->EventType=UPNP_EVENT_SUBSCRIPTION_EXPIRE; */
	RenewEvent->handle = client_handle;
	RenewEvent->Event = RenewEventStruct;

	TPJobInit(&job, (start_routine) GenaAutoRenewSubscription, RenewEvent);
	TPJobSetFreeFunction(&job, (free_routine)free_upnp_timeout);
	TPJobSetPriority(&job, MED_PRIORITY);

	/* Schedule the job */
	return_code = TimerThreadSchedule(
		&gTimerThread,
		TimeOut - AUTO_RENEW_TIME,
		REL_SEC,
		&job, SHORT_TERM,
		&(RenewEvent->eventId));
	if (return_code != UPNP_E_SUCCESS) {
		free(RenewEvent);
		free(RenewEventStruct);
		goto end_function;
	}

	UpnpClientSubscription_set_RenewEventId(sub, RenewEvent->eventId);

	return_code = GENA_SUCCESS;

end_function:
	return return_code;
}


/*!
 * \brief Sends the UNSUBCRIBE gena request and recieves the response from the
 * 	device and returns it as a parameter.
 *
 * \returns 0 if successful, otherwise returns the appropriate error code.
 */
static int gena_unsubscribe(
	/*! [in] Event URL of the service. */
	IN const UpnpString *url,
	/*! [in] The subcription ID. */
	IN const UpnpString *sid,
	/*! [out] The UNSUBCRIBE response from the device. */
	OUT http_parser_t *response )
{
	int return_code;
	uri_type dest_url;
	membuffer request;

	/* parse url */
	return_code = http_FixStrUrl(
		UpnpString_get_String(url),
		UpnpString_get_Length(url),
		&dest_url);
	if (return_code != 0) {
		return return_code;
	}

	/* make request msg */
	membuffer_init(&request);
	request.size_inc = 30;
	return_code = http_MakeMessage(
		&request, 1, 1,
		"q" "ssc" "Uc",
		HTTPMETHOD_UNSUBSCRIBE, &dest_url,
		"SID: ", UpnpString_get_String(sid));

	/* Not able to make the message so destroy the existing buffer */
	if (return_code != 0) {
		membuffer_destroy(&request);

		return return_code;
	}

	/* send request and get reply */
	return_code = http_RequestAndResponse(
		&dest_url, request.buf, request.length,
		HTTPMETHOD_UNSUBSCRIBE, HTTP_DEFAULT_TIMEOUT, response);
	membuffer_destroy(&request);
	if (return_code != 0) {
		httpmsg_destroy(&response->msg);
	}

	if (return_code == 0 && response->msg.status_code != HTTP_OK) {
		return_code = UPNP_E_UNSUBSCRIBE_UNACCEPTED;
		httpmsg_destroy(&response->msg);
	}

	return return_code;
}


/*!
 * \brief Subscribes or renew subscription.
 *
 * \return 0 if successful, otherwise returns the appropriate error code.
 */
static int gena_subscribe(
	/*! [in] URL of service to subscribe. */
	IN const UpnpString *url,
	/*! [in,out] Subscription time desired (in secs). */
	INOUT int *timeout,
	/*! [in] for renewal, this contains a currently held subscription SID.
	 * For first time subscription, this must be NULL. */
	IN const UpnpString *renewal_sid,
	/*! [out] SID returned by the subscription or renew msg. */
	OUT UpnpString *sid)
{
	int return_code;
	int parse_ret = 0;
	int local_timeout = CP_MINIMUM_SUBSCRIPTION_TIME;
	memptr sid_hdr;
	memptr timeout_hdr;
	char timeout_str[25];
	membuffer request;
	uri_type dest_url;
	http_parser_t response;
	int rc = 0;

	UpnpString_clear(sid);

	/* request timeout to string */
	if (timeout == NULL) {
		timeout = &local_timeout;
	}
	if (*timeout < 0) {
		memset(timeout_str, 0, sizeof(timeout_str));
		strncpy(timeout_str, "infinite", sizeof(timeout_str) - 1);
	} else if(*timeout < CP_MINIMUM_SUBSCRIPTION_TIME) {
		rc = snprintf(timeout_str, sizeof(timeout_str),
			"%d", CP_MINIMUM_SUBSCRIPTION_TIME);
	} else {
		rc = snprintf(timeout_str, sizeof(timeout_str), "%d", *timeout);
	}
	if (rc < 0 || (unsigned int) rc >= sizeof(timeout_str))
		return UPNP_E_OUTOF_MEMORY;

	/* parse url */
	return_code = http_FixStrUrl(
		UpnpString_get_String(url),
		UpnpString_get_Length(url),
		&dest_url);
	if (return_code != 0) {
		return return_code;
	}

	/* make request msg */
	membuffer_init(&request);
	request.size_inc = 30;
	if (renewal_sid) {
		/* renew subscription */
		return_code = http_MakeMessage(
			&request, 1, 1,
			"q" "ssc" "sscc",
			HTTPMETHOD_SUBSCRIBE, &dest_url,
			"SID: ", UpnpString_get_String(renewal_sid),
			"TIMEOUT: Second-", timeout_str );
	} else {
		/* subscribe */
		if (dest_url.hostport.IPaddress.ss_family == AF_INET6) {
			struct sockaddr_in6* DestAddr6 = (struct sockaddr_in6*)&dest_url.hostport.IPaddress;
			return_code = http_MakeMessage(
				&request, 1, 1,
				"q" "sssdsc" "sc" "sscc",
				HTTPMETHOD_SUBSCRIBE, &dest_url,
				"CALLBACK: <http://[",
				(IN6_IS_ADDR_LINKLOCAL(&DestAddr6->sin6_addr) || strlen(gIF_IPV6_ULA_GUA) == 0) ?
					gIF_IPV6 : gIF_IPV6_ULA_GUA,
				"]:", LOCAL_PORT_V6, "/>",
				"NT: upnp:event",
				"TIMEOUT: Second-", timeout_str);
		} else {
			return_code = http_MakeMessage(
				&request, 1, 1,
				"q" "sssdsc" "sc" "sscc",
				HTTPMETHOD_SUBSCRIBE, &dest_url,
				"CALLBACK: <http://", gIF_IPV4, ":", LOCAL_PORT_V4, "/>",
				"NT: upnp:event",
				"TIMEOUT: Second-", timeout_str);
		}
	}
	if (return_code != 0) {
		return return_code;
	}

	/* send request and get reply */
	return_code = http_RequestAndResponse(&dest_url, request.buf,
		request.length,
		HTTPMETHOD_SUBSCRIBE,
		HTTP_DEFAULT_TIMEOUT,
		&response);
	membuffer_destroy(&request);

	if (return_code != 0) {
		httpmsg_destroy(&response.msg);

		return return_code;
	}
	if (response.msg.status_code != HTTP_OK) {
		httpmsg_destroy(&response.msg);

		return UPNP_E_SUBSCRIBE_UNACCEPTED;
	}

	/* get SID and TIMEOUT */
	if (httpmsg_find_hdr(&response.msg, HDR_SID, &sid_hdr) == NULL ||
	    sid_hdr.length == 0 ||
	    httpmsg_find_hdr( &response.msg, HDR_TIMEOUT, &timeout_hdr ) == NULL ||
	    timeout_hdr.length == 0 ) {
		httpmsg_destroy( &response.msg );

		return UPNP_E_BAD_RESPONSE;
	}

	/* save timeout */
	parse_ret = matchstr(timeout_hdr.buf, timeout_hdr.length, "%iSecond-%d%0", timeout);
	if (parse_ret == PARSE_OK) {
		/* nothing to do */
	} else if (memptr_cmp_nocase(&timeout_hdr, "Second-infinite") == 0) {
		*timeout = -1;
	} else {
		httpmsg_destroy( &response.msg );

		return UPNP_E_BAD_RESPONSE;
	}

	/* save SID */
	UpnpString_set_StringN(sid, sid_hdr.buf, sid_hdr.length);
	if (UpnpString_get_String(sid) == NULL) {
		httpmsg_destroy(&response.msg);

		return UPNP_E_OUTOF_MEMORY;
	}
	httpmsg_destroy(&response.msg);

	return UPNP_E_SUCCESS;
}


int genaUnregisterClient(UpnpClient_Handle client_handle)
{
	ClientSubscription *sub_copy = UpnpClientSubscription_new();
	int return_code = UPNP_E_SUCCESS;
	struct Handle_Info *handle_info = NULL;
	http_parser_t response;

	while (TRUE) {
		HandleLock();

		if (GetHandleInfo(client_handle, &handle_info) != HND_CLIENT) {
			HandleUnlock();
			return_code = GENA_E_BAD_HANDLE;
			goto exit_function;
		}
		if (handle_info->ClientSubList == NULL) {
			return_code = UPNP_E_SUCCESS;
			break;
		}
		UpnpClientSubscription_assign(sub_copy, handle_info->ClientSubList);
		RemoveClientSubClientSID(
			&handle_info->ClientSubList,
			UpnpClientSubscription_get_SID(sub_copy));

		HandleUnlock();

		return_code = gena_unsubscribe(
			UpnpClientSubscription_get_EventURL(sub_copy),
			UpnpClientSubscription_get_ActualSID(sub_copy),
			&response);
		if (return_code == 0) {
			httpmsg_destroy(&response.msg);
		}
		free_client_subscription(sub_copy);
	}

	freeClientSubList(handle_info->ClientSubList);
	HandleUnlock();

exit_function:
	UpnpClientSubscription_delete(sub_copy);
	return return_code;
}


#ifdef INCLUDE_CLIENT_APIS
int genaUnSubscribe(
	UpnpClient_Handle client_handle,
	const UpnpString *in_sid)
{
	ClientSubscription *sub = NULL;
	int return_code = GENA_SUCCESS;
	struct Handle_Info *handle_info;
	ClientSubscription *sub_copy = UpnpClientSubscription_new();
	http_parser_t response;

	/* validate handle and sid */
	HandleLock();
	if (GetHandleInfo(client_handle, &handle_info) != HND_CLIENT) {
		HandleUnlock();
		return_code = GENA_E_BAD_HANDLE;
		goto exit_function;
	}
	sub = GetClientSubClientSID(handle_info->ClientSubList, in_sid);
	if (sub == NULL) {
		HandleUnlock();
		return_code = GENA_E_BAD_SID;
		goto exit_function;
	}
	UpnpClientSubscription_assign(sub_copy, sub);
	HandleUnlock();

	return_code = gena_unsubscribe(
		UpnpClientSubscription_get_EventURL(sub_copy),
		UpnpClientSubscription_get_ActualSID(sub_copy),
		&response);
	if (return_code == 0) {
		httpmsg_destroy(&response.msg);
	}
	free_client_subscription(sub_copy);

	HandleLock();
	if (GetHandleInfo(client_handle, &handle_info) != HND_CLIENT) {
		HandleUnlock();
		return_code = GENA_E_BAD_HANDLE;
		goto exit_function;
	}
	RemoveClientSubClientSID(&handle_info->ClientSubList, in_sid);
	HandleUnlock();

exit_function:
	UpnpClientSubscription_delete(sub_copy);
	return return_code;
}
#endif /* INCLUDE_CLIENT_APIS */


#ifdef INCLUDE_CLIENT_APIS
int genaSubscribe(
	UpnpClient_Handle client_handle,
	const UpnpString *PublisherURL,
	int *TimeOut,
	UpnpString *out_sid)
{
	int return_code = GENA_SUCCESS;
	ClientSubscription *newSubscription = UpnpClientSubscription_new();
	uuid_upnp uid;
	Upnp_SID temp_sid;
	Upnp_SID temp_sid2;
	UpnpString *ActualSID = UpnpString_new();
	UpnpString *EventURL = UpnpString_new();
	struct Handle_Info *handle_info;
	int rc = 0;

	memset(temp_sid, 0, sizeof(temp_sid));
	memset(temp_sid2, 0, sizeof(temp_sid2));

	UpnpPrintf(UPNP_INFO, GENA, __FILE__, __LINE__, "GENA SUBSCRIBE BEGIN");

	UpnpString_clear(out_sid);

	HandleReadLock();
	/* validate handle */
	if (GetHandleInfo(client_handle, &handle_info) != HND_CLIENT) {
		return_code = GENA_E_BAD_HANDLE;
		SubscribeLock();
		goto error_handler;
	}
	HandleUnlock();

	/* subscribe */
	SubscribeLock();
	return_code = gena_subscribe(PublisherURL, TimeOut, NULL, ActualSID);
	HandleLock();
	if (return_code != UPNP_E_SUCCESS) {
		UpnpPrintf( UPNP_CRITICAL, GENA, __FILE__, __LINE__,
			"SUBSCRIBE FAILED in transfer error code: %d returned\n",
			return_code );
		goto error_handler;
	}

	if(GetHandleInfo(client_handle, &handle_info) != HND_CLIENT) {
		return_code = GENA_E_BAD_HANDLE;
		goto error_handler;
	}

	/* generate client SID */
	uuid_create(&uid );
	uuid_unpack(&uid, temp_sid);
	rc = snprintf(temp_sid2, sizeof(temp_sid2), "uuid:%s", temp_sid);
	if (rc < 0 || (unsigned int) rc >= sizeof(temp_sid2)) {
		return_code = UPNP_E_OUTOF_MEMORY;
		goto error_handler;
	}
	UpnpString_set_String(out_sid, temp_sid2);

	/* create event url */
	UpnpString_assign(EventURL, PublisherURL);

	/* fill subscription */
	if (newSubscription == NULL) {
		return_code = UPNP_E_OUTOF_MEMORY;
		goto error_handler;
	}
	UpnpClientSubscription_set_RenewEventId(newSubscription, -1);
	UpnpClientSubscription_set_SID(newSubscription, out_sid);
	UpnpClientSubscription_set_ActualSID(newSubscription, ActualSID);
	UpnpClientSubscription_set_EventURL(newSubscription, EventURL);
	UpnpClientSubscription_set_Next(newSubscription, handle_info->ClientSubList);
	handle_info->ClientSubList = newSubscription;

	/* schedule expiration event */
	return_code = ScheduleGenaAutoRenew(client_handle, *TimeOut, newSubscription);

error_handler:
	UpnpString_delete(ActualSID);
	UpnpString_delete(EventURL);
	if (return_code != UPNP_E_SUCCESS)
		UpnpClientSubscription_delete(newSubscription);
	HandleUnlock();
	SubscribeUnlock();

	return return_code;
}
#endif /* INCLUDE_CLIENT_APIS */


int genaRenewSubscription(
	UpnpClient_Handle client_handle,
	const UpnpString *in_sid,
	int *TimeOut)
{
	int return_code = GENA_SUCCESS;
	ClientSubscription *sub = NULL;
	ClientSubscription *sub_copy = UpnpClientSubscription_new();
	struct Handle_Info *handle_info;
	UpnpString *ActualSID = UpnpString_new();
	ThreadPoolJob tempJob;

	HandleLock();

	/* validate handle and sid */
	if (GetHandleInfo(client_handle, &handle_info) != HND_CLIENT) {
		HandleUnlock();

		return_code = GENA_E_BAD_HANDLE;
		goto exit_function;
	}

	sub = GetClientSubClientSID(handle_info->ClientSubList, in_sid);
	if (sub == NULL) {
		HandleUnlock();

		return_code = GENA_E_BAD_SID;
		goto exit_function;
	}

	/* remove old events */
	if (TimerThreadRemove(
		&gTimerThread,
		UpnpClientSubscription_get_RenewEventId(sub),
		&tempJob) == 0 ) {
		free_upnp_timeout((upnp_timeout *)tempJob.arg);
	}

	UpnpPrintf(UPNP_INFO, GENA, __FILE__, __LINE__, "REMOVED AUTO RENEW  EVENT");

	UpnpClientSubscription_set_RenewEventId(sub, -1);
	UpnpClientSubscription_assign(sub_copy, sub);

	HandleUnlock();

	return_code = gena_subscribe(
		UpnpClientSubscription_get_EventURL(sub_copy),
		TimeOut,
		UpnpClientSubscription_get_ActualSID(sub_copy),
		ActualSID);

	HandleLock();

	if (GetHandleInfo(client_handle, &handle_info) != HND_CLIENT) {
		HandleUnlock();
		return_code = GENA_E_BAD_HANDLE;
		goto exit_function;
	}

	/* we just called GetHandleInfo, so we don't check for return value */
	/*GetHandleInfo(client_handle, &handle_info); */
	if (return_code != UPNP_E_SUCCESS) {
		/* network failure (remove client sub) */
		RemoveClientSubClientSID(&handle_info->ClientSubList, in_sid);
		free_client_subscription(sub_copy);
		HandleUnlock();
		goto exit_function;
	}

	/* get subscription */
	sub = GetClientSubClientSID(handle_info->ClientSubList, in_sid);
	if (sub == NULL) {
		free_client_subscription(sub_copy);
		HandleUnlock();
		return_code = GENA_E_BAD_SID;
		goto exit_function;
	}

	/* store actual sid */
	UpnpClientSubscription_set_ActualSID(sub, ActualSID);

	/* start renew subscription timer */
	return_code = ScheduleGenaAutoRenew(client_handle, *TimeOut, sub);
	if (return_code != GENA_SUCCESS) {
		RemoveClientSubClientSID(
			&handle_info->ClientSubList,
			UpnpClientSubscription_get_SID(sub));
	}
	free_client_subscription(sub_copy);
	HandleUnlock();

exit_function:
	UpnpString_delete(ActualSID);
	UpnpClientSubscription_delete(sub_copy);
	return return_code;
}


void gena_process_notification_event(
	SOCKINFO *info,
	http_message_t *event)
{
	struct Upnp_Event event_struct;
	IXML_Document *ChangedVars = NULL;
	int eventKey;
	token sid;
	ClientSubscription *subscription = NULL;
	struct Handle_Info *handle_info;
	void *cookie;
	Upnp_FunPtr callback;
	UpnpClient_Handle client_handle;
	const UpnpString *tmpSID = NULL;

	memptr sid_hdr;
	memptr nt_hdr,
	nts_hdr;
	memptr seq_hdr;

	/* get SID */
	if (httpmsg_find_hdr(event, HDR_SID, &sid_hdr) == NULL) {
		error_respond(info, HTTP_PRECONDITION_FAILED, event);
		goto exit_function;
	}
	sid.buff = sid_hdr.buf;
	sid.size = sid_hdr.length;

	/* get event key */
	if (httpmsg_find_hdr(event, HDR_SEQ, &seq_hdr) == NULL ||
	    matchstr(seq_hdr.buf, seq_hdr.length, "%d%0", &eventKey) != PARSE_OK) {
		error_respond( info, HTTP_BAD_REQUEST, event );
		goto exit_function;
	}

	/* get NT and NTS headers */
	if (httpmsg_find_hdr(event, HDR_NT, &nt_hdr) == NULL ||
	    httpmsg_find_hdr(event, HDR_NTS, &nts_hdr) == NULL) {
		error_respond( info, HTTP_BAD_REQUEST, event );
		goto exit_function;
	}

	/* verify NT and NTS headers */
	if (memptr_cmp(&nt_hdr, "upnp:event") != 0 ||
	    memptr_cmp(&nts_hdr, "upnp:propchange") != 0) {
		error_respond(info, HTTP_PRECONDITION_FAILED, event);
		goto exit_function;
	}

	/* parse the content (should be XML) */
	if (!has_xml_content_type(event) ||
	    event->msg.length == 0 ||
	    ixmlParseBufferEx(event->entity.buf, &ChangedVars) != IXML_SUCCESS) {
		error_respond(info, HTTP_BAD_REQUEST, event);
		goto exit_function;
	}

	HandleLock();

	/* get client info */
	if (GetClientHandleInfo(&client_handle, &handle_info) != HND_CLIENT) {
		error_respond(info, HTTP_PRECONDITION_FAILED, event);
		HandleUnlock();
		goto exit_function;
	}

	/* get subscription based on SID */
	subscription = GetClientSubActualSID(handle_info->ClientSubList, &sid);
	if (subscription == NULL) {
		if (eventKey == 0) {
			/* wait until we've finished processing a subscription  */
			/*   (if we are in the middle) */
			/* this is to avoid mistakenly rejecting the first event if we  */
			/*   receive it before the subscription response */
			HandleUnlock();

			/* try and get Subscription Lock  */
			/*   (in case we are in the process of subscribing) */
			SubscribeLock();

			/* get HandleLock again */
			HandleLock();

			if (GetClientHandleInfo(&client_handle, &handle_info) != HND_CLIENT) {
				error_respond(info, HTTP_PRECONDITION_FAILED, event);
				SubscribeUnlock();
				HandleUnlock();
				goto exit_function;
			}

			subscription = GetClientSubActualSID(handle_info->ClientSubList, &sid);
			if (subscription == NULL) {
				error_respond( info, HTTP_PRECONDITION_FAILED, event );
				SubscribeUnlock();
				HandleUnlock();
				goto exit_function;
			}

			SubscribeUnlock();
		} else {
			error_respond( info, HTTP_PRECONDITION_FAILED, event );
			HandleUnlock();
			goto exit_function;
		}
	}

	/* success */
	error_respond(info, HTTP_OK, event);

	/* fill event struct */
	tmpSID = UpnpClientSubscription_get_SID(subscription);
	memset(event_struct.Sid, 0, sizeof(event_struct.Sid));
	strncpy(event_struct.Sid, UpnpString_get_String(tmpSID),
		sizeof(event_struct.Sid) - 1);
	event_struct.EventKey = eventKey;
	event_struct.ChangedVariables = ChangedVars;

	/* copy callback */
	callback = handle_info->Callback;
	cookie = handle_info->Cookie;

	HandleUnlock();

	/* make callback with event struct */
	/* In future, should find a way of mainting */
	/* that the handle is not unregistered in the middle of a */
	/* callback */
	callback(UPNP_EVENT_RECEIVED, &event_struct, cookie);

exit_function:
	ixmlDocument_free(ChangedVars);
}


#endif /* INCLUDE_CLIENT_APIS */
#endif /* EXCLUDE_GENA */

