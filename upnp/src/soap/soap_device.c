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

/*!
 * \file
 */

#include "config.h"

#ifdef INCLUDE_DEVICE_APIS
#if EXCLUDE_SOAP == 0

#include "ActionRequest.h"
#include "httpparser.h"
#include "httpreadwrite.h"
#include "parsetools.h"
#include "soaplib.h"
#include "ssdplib.h"
#include "statcodes.h"
#include "unixutil.h"
#include "upnpapi.h"

#include <assert.h>
#include <string.h>

#ifdef WIN32
	#define snprintf _snprintf
#endif

/*! timeout duration in secs for transmission/reception */
#define SOAP_TIMEOUT UPNP_TIMEOUT

#define SREQ_HDR_NOT_FOUND	 -1
#define SREQ_BAD_HDR_FORMAT	 -2
#define SREQ_NOT_EXTENDED	 -3

#define SOAP_INVALID_ACTION 401
#define SOAP_INVALID_ARGS	402
#define SOAP_OUT_OF_SYNC	403
#define SOAP_INVALID_VAR	404
#define SOAP_ACTION_FAILED	501
#define SOAP_MEMORY_OUT		603


static const char *SOAP_BODY = "Body";
static const char *SOAP_URN = "http:/""/schemas.xmlsoap.org/soap/envelope/";
static const char *QUERY_STATE_VAR_URN = "urn:schemas-upnp-org:control-1-0";

static const char *Soap_Invalid_Action = "Invalid Action";
/*static const char* Soap_Invalid_Args = "Invalid Args"; */
static const char *Soap_Action_Failed = "Action Failed";
static const char *Soap_Invalid_Var = "Invalid Var";
static const char *Soap_Memory_out = "Out of Memory";

typedef struct soap_devserv_t {
	char dev_udn[NAME_SIZE];
	char service_type[NAME_SIZE];
	char service_id[NAME_SIZE];
	memptr action_name;
	Upnp_FunPtr callback;
	void *cookie;
}soap_devserv_t;


/*!
 * \brief Sends SOAP error response.
 */
static void send_error_response(
	/*! [in] Socket info. */
	IN SOCKINFO *info,
	/*! [in] Error code. */
	IN int error_code,
	/*! [in] Error message. */
	IN const char *err_msg,
	/*! [in] HTTP request. */
	IN http_message_t *hmsg)
{
	off_t content_length;
	int timeout_secs = SOAP_TIMEOUT;
	int major;
	int minor;
	const char *start_body =
/*		"<?xml version=\"1.0\"?>\n" required?? */
		"<s:Envelope "
		"xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
		"s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\n"
		"<s:Body>\n"
		"<s:Fault>\n"
		"<faultcode>s:Client</faultcode>\n"
		"<faultstring>UPnPError</faultstring>\n"
		"<detail>\n"
		"<UPnPError xmlns=\"urn:schemas-upnp-org:control-1-0\">\n"
		"<errorCode>";
	const char *mid_body =
		"</errorCode>\n"
		"<errorDescription>";
	const char *end_body =
		"</errorDescription>\n"
		"</UPnPError>\n"
		"</detail>\n"
		"</s:Fault>\n"
		"</s:Body>\n"
		"</s:Envelope>\n";
	char err_code_str[30];
	membuffer headers;

	memset(err_code_str, 0, sizeof(err_code_str));
	snprintf(err_code_str, sizeof(err_code_str), "%d", error_code);
	/* calc body len */
	content_length = (off_t) (strlen(start_body) + strlen(err_code_str) +
				  strlen(mid_body) + strlen(err_msg) +
				  strlen(end_body));
	http_CalcResponseVersion(hmsg->major_version, hmsg->minor_version,
				 &major, &minor);
	/* make headers */
	membuffer_init(&headers);
	if (http_MakeMessage(&headers, major, minor,
			"RNsDsSXcc" "sssss",
			500,
			content_length,
			ContentTypeHeader,
			"EXT:\r\n",
			X_USER_AGENT,
			start_body, err_code_str, mid_body, err_msg,
			end_body) != 0) {
		membuffer_destroy(&headers);
		/* out of mem */
		return;
	}
	/* send err msg */
	http_SendMessage(info, &timeout_secs, "b",
		headers.buf, headers.length);
	membuffer_destroy(&headers);
}

/*!
 * \brief Sends response of get var status.
 */
static UPNP_INLINE void send_var_query_response(
	/*! [in] Socket info. */
	SOCKINFO *info,
	/*! [in] Value of the state variable. */
	const char *var_value,
	/*! [in] HTTP request. */
	http_message_t *hmsg)
{
	off_t content_length;
	int timeout_secs = SOAP_TIMEOUT;
	int major;
	int minor;
	const char *start_body =
		"<s:Envelope "
		"xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
		"s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\n"
		"<s:Body>\n"
		"<u:QueryStateVariableResponse "
		"xmlns:u=\"urn:schemas-upnp-org:control-1-0\">\n" "<return>";
	const char *end_body =
		"</return>\n"
		"</u:QueryStateVariableResponse>\n" "</s:Body>\n" "</s:Envelope>\n";
	membuffer response;

	http_CalcResponseVersion(hmsg->major_version, hmsg->minor_version,
				 &major, &minor);
	content_length = (off_t) (strlen(start_body) + strlen(var_value) +
				  strlen(end_body));
	/* make headers */
	membuffer_init(&response);
	if (http_MakeMessage(&response, major, minor,
			"RNsDsSXcc" "sss",
			HTTP_OK,
			content_length,
			ContentTypeHeader,
			"EXT:\r\n",
			X_USER_AGENT,
			start_body, var_value, end_body) != 0) {
		membuffer_destroy(&response);
		/* out of mem */
		return;
	}
	/* send msg */
	http_SendMessage(info, &timeout_secs, "b",
		response.buf, response.length);
	membuffer_destroy(&response);
}


/*!
 * \brief Sends the SOAP action response.
 */
static UPNP_INLINE void send_action_response(
	/*! [in] Socket info. */
	SOCKINFO *info,
	/*! [in] The response document. */
	IXML_Document *action_resp,
	/*! [in] Action request document. */
	http_message_t *request)
{
	char *xml_response = NULL;
	membuffer headers;
	int major, minor;
	int err_code;
	off_t content_length;
	int ret_code;
	int timeout_secs = SOAP_TIMEOUT;
	static const char *start_body =
		/*"<?xml version=\"1.0\"?>" required?? */
		"<s:Envelope xmlns:s=\"http://schemas.xmlsoap."
		"org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap."
		"org/soap/encoding/\"><s:Body>\n";
	static const char *end_body = "</s:Body> </s:Envelope>";

	/* init */
	http_CalcResponseVersion(request->major_version, request->minor_version,
		&major, &minor);
	membuffer_init(&headers);
	err_code = UPNP_E_OUTOF_MEMORY;	/* one error only */
	/* get xml */
	xml_response = ixmlPrintNode((IXML_Node *) action_resp);
	if (!xml_response)
		goto error_handler;
	content_length = (off_t)(strlen(start_body) + strlen(xml_response) +
		strlen(end_body));
	/* make headers */
	if (http_MakeMessage(&headers, major, minor,
			"RNsDsSXcc",
			HTTP_OK,	/* status code */
			content_length,
			ContentTypeHeader,
			"EXT:\r\n", X_USER_AGENT) != 0) {
		goto error_handler;
	}
	/* send whole msg */
	ret_code = http_SendMessage(
		info, &timeout_secs, "bbbb",
		headers.buf, headers.length,
		start_body, strlen(start_body),
		xml_response, strlen(xml_response),
		end_body, strlen(end_body));
	if (ret_code != 0) {
		UpnpPrintf(UPNP_INFO, SOAP, __FILE__, __LINE__,
			   "Failed to send response: err code = %d\n",
			   ret_code);
	}
	err_code = 0;

error_handler:
	ixmlFreeDOMString(xml_response);
	membuffer_destroy(&headers);
	if (err_code != 0) {
		/* only one type of error to worry about - out of mem */
		send_error_response(info, SOAP_ACTION_FAILED, "Out of memory",
			request);
	}
}


/*!
 * \brief Handles the SOAP requests to querry the state variables.
 * This functionality has been deprecated in the UPnP V1.0 architecture.
 */
static UPNP_INLINE void handle_query_variable(
		/*! [in] Socket info. */
		SOCKINFO *info,
		/*! [in] HTTP Request. */
		http_message_t *request,
		/*! [in] SOAP device/service information. */
		soap_devserv_t *soap_info,
		/*! [in] Node containing variable name. */
		IXML_Node *req_node)
{
	UpnpStateVarRequest *variable = UpnpStateVarRequest_new();
	const char *err_str;
	int err_code;
	const DOMString var_name;

	/* set default error */
	err_code = SOAP_INVALID_VAR;
	err_str = Soap_Invalid_Var;

	if (variable == NULL) {
		err_code = SOAP_MEMORY_OUT;
		err_str = Soap_Memory_out;
		goto error_handler;
	}

	UpnpStateVarRequest_set_ErrCode(variable, UPNP_E_SUCCESS);
	UpnpStateVarRequest_strcpy_DevUDN(variable, soap_info->dev_udn);
	UpnpStateVarRequest_strcpy_ServiceID(variable, soap_info->service_id);
	var_name = ixmlNode_getNodeValue(req_node);
	UpnpStateVarRequest_strcpy_StateVarName(variable, var_name);
	UpnpStateVarRequest_set_CtrlPtIPAddr(variable, &info->foreign_sockaddr);

	/* send event */
	soap_info->callback(UPNP_CONTROL_GET_VAR_REQUEST, variable,
			soap_info->cookie);
	UpnpPrintf(UPNP_INFO, SOAP, __FILE__, __LINE__,
		"Return from callback for var request\n");
	/* validate, and handle result */
	if (UpnpStateVarRequest_get_CurrentVal(variable) == NULL)
		goto error_handler;
	if (UpnpStateVarRequest_get_ErrCode(variable) != UPNP_E_SUCCESS) {
		if (UpnpString_get_Length(UpnpStateVarRequest_get_ErrStr(variable)) > 0) {
			err_code = UpnpStateVarRequest_get_ErrCode(variable);
			err_str = UpnpStateVarRequest_get_ErrStr_cstr(variable);
		}
		goto error_handler;
	}
	/* send response */
	send_var_query_response(info, UpnpStateVarRequest_get_CurrentVal(variable), request);
	err_code = 0;

	/* error handling and cleanup */
error_handler:
	UpnpStateVarRequest_delete(variable);
	if (err_code != 0)
		send_error_response(info, err_code, err_str, request);
}

/*!
 * \brief Handles the SOAP action request.
 */
static void handle_invoke_action(
		/*! [in] Socket info. */
		SOCKINFO *info,
		/*! [in] HTTP Request. */
		http_message_t *request,
		/*! [in] SOAP device/service information. */
		soap_devserv_t *soap_info,
		/*! [in] Node containing the SOAP action request. */
		IXML_Node *req_node)
{
	char save_char;
	UpnpActionRequest *action = UpnpActionRequest_new();
	IXML_Document *actionRequestDoc = NULL;
	IXML_Document *actionResultDoc = NULL;
	int err_code;
	const char *err_str;
	memptr action_name;
	DOMString act_node = NULL;

	/* null-terminate */
	action_name = soap_info->action_name;
	save_char = action_name.buf[action_name.length];
	action_name.buf[action_name.length] = '\0';
	/* get action node */
	act_node = ixmlPrintNode(req_node);
	if (!act_node) {
		err_code = SOAP_MEMORY_OUT;
		err_str = Soap_Memory_out;
		goto error_handler;
	}
	err_code = ixmlParseBufferEx(act_node, &actionRequestDoc);
	if (err_code != IXML_SUCCESS) {
		if (IXML_INSUFFICIENT_MEMORY == err_code) {
			err_code = SOAP_MEMORY_OUT;
			err_str = Soap_Memory_out;
		} else {
			err_code = SOAP_INVALID_ACTION;
			err_str = Soap_Invalid_Action;
		}
		goto error_handler;
	}
	UpnpActionRequest_set_ErrCode(action, UPNP_E_SUCCESS);
	UpnpActionRequest_strcpy_ActionName(action, action_name.buf);
	UpnpActionRequest_strcpy_DevUDN(action, soap_info->dev_udn);
	UpnpActionRequest_strcpy_ServiceID(action, soap_info->service_id);
	UpnpActionRequest_set_ActionRequest(action, actionRequestDoc);
	UpnpActionRequest_set_ActionResult(action, NULL);
	UpnpActionRequest_set_CtrlPtIPAddr(action, &info->foreign_sockaddr);

	UpnpPrintf(UPNP_INFO, SOAP, __FILE__, __LINE__, "Calling Callback\n");
	soap_info->callback(UPNP_CONTROL_ACTION_REQUEST, action, soap_info->cookie);
	err_code = UpnpActionRequest_get_ErrCode(action);
	if (err_code != UPNP_E_SUCCESS) {
		err_str = UpnpActionRequest_get_ErrStr_cstr(action);
		if (strlen(err_str) <= 0) {
			err_code = SOAP_ACTION_FAILED;
			err_str = Soap_Action_Failed;
		}
		goto error_handler;
	}
	/* validate, and handle action error */
	actionResultDoc = UpnpActionRequest_get_ActionResult(action);
	if (actionResultDoc == NULL) {
		err_code = SOAP_ACTION_FAILED;
		err_str = Soap_Action_Failed;
		goto error_handler;
	}
	/* send response */
	send_action_response(info, actionResultDoc, request);
	err_code = 0;

	/* error handling and cleanup */
error_handler:
	ixmlDocument_free(actionResultDoc);
	ixmlDocument_free(actionRequestDoc);
	ixmlFreeDOMString(act_node);
	/* restore */
	action_name.buf[action_name.length] = save_char;
	if (err_code != 0)
		send_error_response(info, err_code, err_str, request);
	UpnpActionRequest_delete(action);
}

/*!
 * \brief Retrieve SOAP device/service information associated
 * with request-URI, which includes the callback function to hand-over
 * the request to the device application.
 *
 * \return 0 if OK, -1 on error.
 */
static int get_dev_service(
		/*! [in] HTTP request. */
		http_message_t *request,
		/*! [in] Address family: AF_INET or AF_INET6. */
		int AddressFamily,
		/*! [out] SOAP device/service information. */
		soap_devserv_t *soap_info)
{
	struct Handle_Info *device_info;
	int device_hnd;
	service_info *serv_info;
	char save_char;
	/* error by default */
	int ret_code = -1;
	const char *control_url;

	/* null-terminate pathquery of url */
	control_url = request->uri.pathquery.buff;
	save_char = control_url[request->uri.pathquery.size];
	((char *)control_url)[request->uri.pathquery.size] = '\0';

	HandleReadLock();

	if (GetDeviceHandleInfo(AddressFamily, &device_hnd,
				&device_info) != HND_DEVICE)
		goto error_handler;
	serv_info = FindServiceControlURLPath(
		&device_info->ServiceTable, control_url);
	if (!serv_info)
		goto error_handler;

	namecopy(soap_info->dev_udn, serv_info->UDN);
	namecopy(soap_info->service_type, serv_info->serviceType);
	namecopy(soap_info->service_id, serv_info->serviceId);
	soap_info->callback = device_info->Callback;
	soap_info->cookie = device_info->Cookie;
	ret_code = 0;

 error_handler:
	/* restore */
	((char *)control_url)[request->uri.pathquery.size] = save_char;
	HandleUnlock();
	return ret_code;
}

/*!
 * \brief Get the SOAPACTION header value for M-POST request.
 *
 * \return UPNP_E_SUCCESS if OK, error number on failure.
 */
static int get_mpost_acton_hdrval(
		/*! [in] HTTP request. */
		http_message_t *request,
		/*! [out] Buffer to get the header value */
		memptr *val)
{
	http_header_t *hdr;
	memptr ns_value, dummy_quote, value;
	membuffer soap_action_name;

	assert(HTTPMETHOD_MPOST == request->method);
	hdr = httpmsg_find_hdr(request, HDR_MAN, &value);
	if (NULL == hdr)
		return SREQ_NOT_EXTENDED;
	if (matchstr(value.buf, value.length, "%q%i ; ns = %s",
		     &dummy_quote, &ns_value) != PARSE_OK)
		return SREQ_NOT_EXTENDED;
	/* create soapaction name header */
	membuffer_init(&soap_action_name);
	if (membuffer_assign(&soap_action_name,
		ns_value.buf, ns_value.length) == UPNP_E_OUTOF_MEMORY ||
	    membuffer_append_str(&soap_action_name,
		"-SOAPACTION") == UPNP_E_OUTOF_MEMORY) {
		membuffer_destroy(&soap_action_name);
		return UPNP_E_OUTOF_MEMORY;
	}
	hdr = httpmsg_find_hdr_str(request, soap_action_name.buf);
	membuffer_destroy(&soap_action_name);
	if (NULL == hdr)
		return SREQ_HDR_NOT_FOUND;
	val->buf = hdr->value.buf;
	val->length = hdr->value.length;
	return UPNP_E_SUCCESS;
}

/*!
 * \brief Check the header validity, and get the action name
 * and the version of the service that the CP wants to use.
 *
 * \return UPNP_E_SUCCESS if OK, error number on failure.
 */
static int check_soapaction_hdr(
		/*! [in] HTTP request. */
		http_message_t *request,
		/*! [in, out] SOAP device/service information. */
		soap_devserv_t *soap_info)
{
	memptr value;
	char save_char;
	char *hash_pos = NULL;
	char *col_pos1, *col_pos2, *serv_type;
	int ret_code;

	/* find SOAPACTION header */
	if (SOAPMETHOD_POST == request->method) {
		if (!httpmsg_find_hdr(request, HDR_SOAPACTION, &value))
			return SREQ_HDR_NOT_FOUND;
	} else {
		ret_code = get_mpost_acton_hdrval(request, &value);
		if (ret_code != UPNP_E_SUCCESS) {
			return ret_code;
		}
	}

	/* error by default */
	ret_code = SREQ_BAD_HDR_FORMAT;
	/* get action name*/
	save_char = value.buf[value.length];
	value.buf[value.length] = '\0';
	hash_pos = strchr(value.buf, '#');
	if (NULL == hash_pos) {
		goto error_handler;
	}
	*hash_pos = '\0';
	if (matchstr(hash_pos+1,
			value.length - (size_t)(hash_pos+1 - value.buf),
			"%s", &soap_info->action_name) != PARSE_OK) {
		goto error_handler;
	}

	/* check service type */
	if (value.buf[0] != '\"') {
		goto error_handler;
	}
	serv_type = &value.buf[1];
	col_pos1 = strrchr(serv_type, ':');
	if (NULL == col_pos1) {
		goto error_handler;
	}
	col_pos2 = strrchr(soap_info->service_type, ':');
	/* XXX: this should be checked when service list is generated */
	assert(col_pos2 != NULL);
	if (col_pos2-soap_info->service_type == col_pos1-serv_type &&
		strncmp(soap_info->service_type, serv_type, col_pos1-serv_type) == 0) {
		/* for action invocation, update the version information */
		namecopy(soap_info->service_type, serv_type);
	} else if (strcmp(serv_type, QUERY_STATE_VAR_URN) == 0 &&
			memptr_cmp(&soap_info->action_name, "QueryStateVariable") == 0) {
		/* query variable */
		soap_info->action_name.buf = NULL;
		soap_info->action_name.length = 0;
	} else {
		goto error_handler;
	}
	ret_code = UPNP_E_SUCCESS;

error_handler:
	if (hash_pos != NULL) {
		*hash_pos = '#';
	}
	value.buf[value.length] = save_char;
	return ret_code;
}


/*!
 * \brief Check validity of the SOAP request per UPnP specification.
 *
 * \return 0 if OK, -1 on failure.
 */
static int check_soap_request(
		/*! [in] SOAP device/service information. */
		soap_devserv_t *soap_info,
		/*! [in] Document containing the SOAP action request. */
		IN IXML_Document *xml_doc,
		/*! [out] Node containing the SOAP action request/variable name. */
		IXML_Node **req_node)
{
	IXML_Node *envp_node = NULL;
	IXML_Node *body_node = NULL;
	IXML_Node *action_node = NULL;
	const DOMString local_name = NULL;
	const DOMString ns_uri = NULL;
	int ret_val = -1;

	/* Got the Envelop node here */
	envp_node = ixmlNode_getFirstChild((IXML_Node *) xml_doc);
	if (NULL == envp_node) {
		goto error_handler;
	}
	ns_uri = ixmlNode_getNamespaceURI(envp_node);
	if (NULL == ns_uri || strcmp(ns_uri, SOAP_URN) != 0) {
		goto error_handler;
	}
	/* Got Body here */
	body_node = ixmlNode_getFirstChild(envp_node);
	if (NULL == body_node) {
		goto error_handler;
	}
	local_name = ixmlNode_getLocalName(body_node);
	if (NULL == local_name || strcmp(local_name, SOAP_BODY) != 0) {
		goto error_handler;
	}
	/* Got action node here */
	action_node = ixmlNode_getFirstChild(body_node);
	if (NULL == action_node) {
		goto error_handler;
	}
	/* check local name and namespace of action node */
	ns_uri = ixmlNode_getNamespaceURI(action_node);
	if (NULL == ns_uri) {
		goto error_handler;
	}
	local_name = ixmlNode_getLocalName(action_node);
	if (NULL == local_name) {
		goto error_handler;
	}
	if (NULL == soap_info->action_name.buf) {
		IXML_Node *varname_node = NULL;
		IXML_Node *nametxt_node = NULL;
		if (strcmp(ns_uri, QUERY_STATE_VAR_URN) != 0 ||
			strcmp(local_name, "QueryStateVariable") != 0) {
			goto error_handler;
		}
		varname_node = ixmlNode_getFirstChild(action_node);
		if(NULL == varname_node) {
			goto error_handler;
		}
		local_name = ixmlNode_getLocalName(varname_node);
		if (strcmp(local_name, "varName") != 0) {
			goto error_handler;
		}
		nametxt_node = ixmlNode_getFirstChild(varname_node);
		if (NULL == nametxt_node ||
			ixmlNode_getNodeType(nametxt_node) != eTEXT_NODE) {
			goto error_handler;
		}
		*req_node = nametxt_node;
	} else {
		/* check service type against SOAPACTION header */
		if (strcmp(soap_info->service_type, ns_uri) != 0 ||
			memptr_cmp(&soap_info->action_name, local_name) != 0) {
			goto error_handler;
		}
		*req_node = action_node;
	}
	/* success */
	ret_val = 0;

error_handler:
	return ret_val;
}


/*!
 * \brief This is a callback called by minisever after receiving the request
 * from the control point. After HTTP processing, it calls handle_soap_request
 * to start SOAP processing.
 */
void soap_device_callback(
	/*! [in] Parsed request received by the device. */
	http_parser_t *parser,
	/*! [in] HTTP request. */
	http_message_t *request,
	/*! [in,out] Socket info. */
	SOCKINFO *info)
{
	int err_code;
	IXML_Document *xml_doc = NULL;
	soap_devserv_t *soap_info = NULL;
	IXML_Node *req_node = NULL;

	/* get device/service identified by the request-URI */
	soap_info = malloc(sizeof(soap_devserv_t));
	if (NULL == soap_info) {
		err_code = HTTP_INTERNAL_SERVER_ERROR;
		goto error_handler;
	}
	if (get_dev_service(request,
			info->foreign_sockaddr.ss_family, soap_info) < 0) {
		err_code = HTTP_NOT_FOUND;
		goto error_handler;
	}
	/* validate: content-type == text/xml */
	if (!has_xml_content_type(request)) {
		err_code = HTTP_UNSUPPORTED_MEDIA_TYPE;
		goto error_handler;
	}
	/* check SOAPACTION HTTP header */
	err_code = check_soapaction_hdr(request, soap_info);
	if (err_code != UPNP_E_SUCCESS) {
		switch (err_code) {
		case SREQ_NOT_EXTENDED:
			err_code = HTTP_NOT_EXTENDED;
			break;
		case UPNP_E_OUTOF_MEMORY:
			err_code = HTTP_INTERNAL_SERVER_ERROR;
			break;
		default:
			err_code = HTTP_BAD_REQUEST;
			break;
		}
		goto error_handler;
	}
	/* parse XML */
	err_code = ixmlParseBufferEx(request->entity.buf, &xml_doc);
	if (err_code != IXML_SUCCESS) {
		if (IXML_INSUFFICIENT_MEMORY == err_code)
			err_code = HTTP_INTERNAL_SERVER_ERROR;
		else
			err_code = HTTP_BAD_REQUEST;
		goto error_handler;
	}
	/* check SOAP body */
	if (check_soap_request(soap_info, xml_doc, &req_node) < 0)
	{
		err_code = HTTP_BAD_REQUEST;
		goto error_handler;
	}
	/* process SOAP request */
	if (NULL == soap_info->action_name.buf)
		/* query var */
		handle_query_variable(info, request, soap_info, req_node);
	else
		/* invoke action */
		handle_invoke_action(info, request, soap_info, req_node);

	err_code = HTTP_OK;

error_handler:
	ixmlDocument_free(xml_doc);
	free(soap_info);
	if (err_code != HTTP_OK) {
		http_SendStatusResponse(info, err_code, request->major_version,
				request->minor_version);
	}
	return;
	parser = parser;
}

#endif /* EXCLUDE_SOAP */

#endif /* INCLUDE_DEVICE_APIS */

