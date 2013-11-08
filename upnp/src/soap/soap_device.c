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

#include "httpparser.h"
#include "httpreadwrite.h"
#include "parsetools.h"
#include "soaplib.h"
#include "ssdplib.h"
#include "statcodes.h"
#include "unixutil.h"
#include "upnpapi.h"

#ifdef WIN32
	#define snprintf _snprintf
#endif

/*! timeout duration in secs for transmission/reception */
#define SOAP_TIMEOUT UPNP_TIMEOUT

#define SREQ_HDR_NOT_FOUND	 -1
#define SREQ_BAD_HDR_FORMAT	 -2

#define SOAP_INVALID_ACTION 401
#define SOAP_INVALID_ARGS	402
#define SOAP_OUT_OF_SYNC	403
#define SOAP_INVALID_VAR	404
#define SOAP_ACTION_FAILED	501

static const char *SOAP_BODY = "Body";
static const char *SOAP_URN = "http:/""/schemas.xmlsoap.org/soap/envelope/";
static const char *QUERY_STATE_VAR_URN = "urn:schemas-upnp-org:control-1-0";

static const char *Soap_Invalid_Action = "Invalid Action";
/*static const char* Soap_Invalid_Args = "Invalid Args"; */
static const char *Soap_Action_Failed = "Action Failed";
static const char *Soap_Invalid_Var = "Invalid Var";

/*!
 * \brief This function retrives the name of the SOAP action.
 *
 * \return 0 if successful else returns appropriate error.
 */
static UPNP_INLINE int get_request_type(
	/*! [in] HTTP request. */
	http_message_t *request,
	/*! [out] SOAP action name. */
	memptr *action_name)
{
	memptr value;
	memptr ns_value, dummy_quote;
	http_header_t *hdr;
	char save_char;
	char *s;
	membuffer soap_action_name;
	size_t n;

	/* find soapaction header */
	if (request->method == SOAPMETHOD_POST) {
		if (!httpmsg_find_hdr(request, HDR_SOAPACTION, &value))
			return SREQ_HDR_NOT_FOUND;
	} else {
		/* M-POST */
		/* get NS value from MAN header */
		hdr = httpmsg_find_hdr(request, HDR_MAN, &value);
		if (hdr == NULL)
			return SREQ_HDR_NOT_FOUND;
		if (matchstr(value.buf, value.length, "%q%i ; ns = %s",
			     &dummy_quote, &ns_value) != 0)
			return SREQ_BAD_HDR_FORMAT;
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
		if (!hdr)
			return SREQ_HDR_NOT_FOUND;
		value.buf = hdr->value.buf;
		value.length = hdr->value.length;
	}
	/* determine type */
	save_char = value.buf[value.length];
	value.buf[value.length] = '\0';
	s = strchr(value.buf, '#');
	if (s == NULL) {
		value.buf[value.length] = save_char;
		return SREQ_BAD_HDR_FORMAT;
	}
	/* move to value */
	s++;
	n = value.length - (size_t)(s - value.buf);
	if (matchstr(s, n, "%s", action_name) != PARSE_OK) {
		value.buf[value.length] = save_char;
		return SREQ_BAD_HDR_FORMAT;
	}
	/* action name or variable ? */
	if (memptr_cmp(action_name, "QueryStateVariable") == 0) {
		/* query variable */
		action_name->buf = NULL;
		action_name->length = 0;
	}
	/* restore */
	value.buf[value.length] = save_char;

	return 0;
}

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
 * \brief Separates the action node from the root DOM node.
 *
 * \return 0 if successful, or -1 if fails.
 */
static UPNP_INLINE int get_action_node(
	/*! [in] The root DOM node. */
	IXML_Document *TempDoc,
	/*! [in] IXML_Node name to be searched. */
	char *NodeName,
	/*! [out] Response/Output node. */
	IXML_Document **RespNode)
{
	IXML_Node *EnvpNode = NULL;
	IXML_Node *BodyNode = NULL;
	IXML_Node *ActNode = NULL;
	DOMString ActNodeName = NULL;
	const DOMString nodeName;
	int ret_code = -1;	/* error, by default */
	IXML_NodeList *nl = NULL;

	UpnpPrintf(UPNP_INFO, SOAP, __FILE__, __LINE__,
		   "get_action_node(): node name =%s\n ", NodeName);
	*RespNode = NULL;
	/* Got the Envelope node here */
	EnvpNode = ixmlNode_getFirstChild((IXML_Node *) TempDoc);
	if (!EnvpNode)
		goto error_handler;
	nl = ixmlElement_getElementsByTagNameNS((IXML_Element *)EnvpNode,
						"*", "Body");
	if (!nl)
		goto error_handler;
	BodyNode = ixmlNodeList_item(nl, 0);
	if (!BodyNode)
		goto error_handler;
	/* Got action node here */
	ActNode = ixmlNode_getFirstChild(BodyNode);
	if (!ActNode)
		goto error_handler;
	/* Test whether this is the action node */
	nodeName = ixmlNode_getNodeName(ActNode);
	if (!nodeName)
		goto error_handler;
	if (!strstr(nodeName, NodeName))
		goto error_handler;
	else {
		ActNodeName = ixmlPrintNode(ActNode);
		if (!ActNodeName)
			goto error_handler;
		ret_code = ixmlParseBufferEx(ActNodeName, RespNode);
		if (ret_code != IXML_SUCCESS) {
			ret_code = -1;
			goto error_handler;
		}
	}
	/* success */
	ret_code = 0;

error_handler:
	ixmlFreeDOMString(ActNodeName);
	if (nl)
		ixmlNodeList_free(nl);
	return ret_code;
}

/*!
 * \brief Checks the soap body xml came in the SOAP request.
 *
 * \return UPNP_E_SUCCESS if successful else returns appropriate error.
 */
static int check_soap_body(
	/* [in] soap body xml document. */
	IN IXML_Document *doc,
	/* [in] URN. */
	IN const char *urn,
	/* [in] Name of the requested action. */
	IN const char *actionName)
{
	IXML_NodeList *nl = NULL;
	IXML_Node *bodyNode = NULL;
	IXML_Node *actionNode = NULL;
	const DOMString ns = NULL;
	const DOMString name = NULL;
	int ret_code = UPNP_E_INVALID_ACTION;

	nl = ixmlDocument_getElementsByTagNameNS(doc, SOAP_URN, SOAP_BODY);
	if (nl) {
		bodyNode = ixmlNodeList_item(nl, 0);
		if (bodyNode) {
			actionNode = ixmlNode_getFirstChild(bodyNode);
			if (actionNode) {
				ns = ixmlNode_getNamespaceURI(actionNode);
				name = ixmlNode_getLocalName(actionNode);
				/* Don't check version number, to accept a
				 * request comming on a v1 service when
				 * publishing a v2 service */
				if (name &&
				    ns &&
				    !strcmp(actionName, name) &&
				    !strncmp(urn, ns, strlen (urn) - 2))
					ret_code = UPNP_E_SUCCESS;
			}
		}
		ixmlNodeList_free(nl);
	}
	return ret_code;
}

/*!
 * \brief Checks the HTTP header of the SOAP request coming from the
 * control point.
 *
 * \return UPNP_E_SUCCESS if successful else returns appropriate error.
 */
static int check_soap_action_header(
	/*! [in] HTTP request. */
	http_message_t * request,
	/*! [in] URN. */
	const char *urn,
	/*! [out] Name of the SOAP action. */
	char **actionName)
{
	memptr header_name;
	http_header_t *soap_action_header = NULL;
	char *ns_compare = NULL;
	size_t tempSize = 0;
	int ret_code = UPNP_E_SUCCESS;
	char *temp_header_value = NULL;
	char *temp = NULL;
	char *temp2 = NULL;

	/* check soap action header */
	soap_action_header = httpmsg_find_hdr(request, HDR_SOAPACTION,
					      &header_name);
	if (!soap_action_header) {
		ret_code = UPNP_E_INVALID_ACTION;
		return ret_code;
	}
	if (soap_action_header->value.length <= 0) {
		ret_code = UPNP_E_INVALID_ACTION;
		return ret_code;
	}
	temp_header_value = malloc(soap_action_header->value.length + 1);
	if (!temp_header_value) {
		ret_code = UPNP_E_OUTOF_MEMORY;
		return ret_code;
	}
	strncpy(temp_header_value, soap_action_header->value.buf,
		soap_action_header->value.length);
	temp_header_value[soap_action_header->value.length] = 0;
	temp = strchr(temp_header_value, '#');
	if (!temp) {
		free(temp_header_value);
		ret_code = UPNP_E_INVALID_ACTION;
		return ret_code;
	}

	(*temp) = 0;		/* temp make string */

	/* check to see if it is Query State Variable or
	 * Service Action */
	tempSize = strlen(urn) + 2;
	ns_compare = malloc(tempSize);
	if (!ns_compare) {
		ret_code = UPNP_E_OUTOF_MEMORY;
		free(temp_header_value);
		return ret_code;
	}
	snprintf(ns_compare, tempSize, "\"%s", urn);
	/* Don't check version number, to accept a request comming on a v1
	 * service  when publishing a v2 service */
	if (strncmp(temp_header_value, ns_compare, strlen(ns_compare) - 2))
		ret_code = UPNP_E_INVALID_ACTION;
	else {
		ret_code = UPNP_E_SUCCESS;
		temp++;
		temp2 = strchr(temp, '\"');
		/* remove ending " if present */
		if (temp2)
			(*temp2) = 0;
		if (*temp)
			(*actionName) = strdup(temp);
		if (!*actionName)
			ret_code = UPNP_E_OUTOF_MEMORY;
	}

	free(temp_header_value);
	free(ns_compare);
	return ret_code;
}

/*!
 * \brief Retrives all the information needed to process the incoming SOAP
 * request. It finds the device and service info and also the callback
 * function to hand-over the request to the device application.
 *
 * \return UPNP_E_SUCCESS if successful else returns appropriate error.
 */
static int get_device_info(
	/*! [in] HTTP request. */
	http_message_t *request,
	/*! [in] flag for a querry. */
	int isQuery,
	/*! [in] Action request document. */
	IXML_Document *actionDoc,
	/*! [in] . */
	int AddressFamily,
	/*! [out] Device UDN string. */
	OUT char device_udn[LINE_SIZE],
	/*! [out] Service ID string. */
	char service_id[LINE_SIZE],
	/*! [out] callback function of the device application. */
	Upnp_FunPtr *callback,
	/*! [out] cookie stored by device application. */
	void **cookie)
{
	struct Handle_Info *device_info;
	int device_hnd;
	service_info *serv_info;
	char save_char;
	/* error by default */
	int ret_code = -1;
	const char *control_url;
	char *actionName = NULL;

	/* null-terminate pathquery of url */
	control_url = request->uri.pathquery.buff;
	save_char = control_url[request->uri.pathquery.size];
	((char *)control_url)[request->uri.pathquery.size] = '\0';

	HandleLock();

	if (GetDeviceHandleInfo(AddressFamily, &device_hnd,
				&device_info) != HND_DEVICE)
		goto error_handler;
	serv_info = FindServiceControlURLPath(
		&device_info->ServiceTable, control_url);
	if (!serv_info)
		goto error_handler;
	if (isQuery) {
		ret_code = check_soap_action_header(request,
			QUERY_STATE_VAR_URN, &actionName);
		if (ret_code != UPNP_E_SUCCESS &&
		    ret_code != UPNP_E_OUTOF_MEMORY) {
			ret_code = UPNP_E_INVALID_ACTION;
			goto error_handler;
		}
		/* check soap body */
		ret_code = check_soap_body(actionDoc, QUERY_STATE_VAR_URN,
			actionName);
		free(actionName);
		if (ret_code != UPNP_E_SUCCESS)
			goto error_handler;
	} else {
		ret_code = check_soap_action_header(request,
			serv_info->serviceType, &actionName);
		if (ret_code != UPNP_E_SUCCESS &&
		    ret_code != UPNP_E_OUTOF_MEMORY) {
			ret_code = UPNP_E_INVALID_SERVICE;
			goto error_handler;
		}
		/* check soap body */
		ret_code = check_soap_body(actionDoc, serv_info->serviceType,
			actionName);
		free(actionName);
		if (ret_code != UPNP_E_SUCCESS) {
			ret_code = UPNP_E_INVALID_SERVICE;
			goto error_handler;
		}
	}
	namecopy(service_id, serv_info->serviceId);
	namecopy(device_udn, serv_info->UDN);
	*callback = device_info->Callback;
	*cookie = device_info->Cookie;
	ret_code = 0;

 error_handler:
	/* restore */
	((char *)control_url)[request->uri.pathquery.size] = save_char;
	HandleUnlock();
	return ret_code;
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
 * \brief Finds the name of the state variable asked in the SOAP request.
 *
 * \return 0 if successful else returns -1.
 */
static UPNP_INLINE int get_var_name(
	/*! [in] Document containing variable request. */
	IXML_Document *TempDoc,
	/*! [out] Name of the state varible. */
	char *VarName)
{
	IXML_Node *EnvpNode = NULL;
	IXML_Node *BodyNode = NULL;
	IXML_Node *StNode = NULL;
	IXML_Node *VarNameNode = NULL;
	IXML_Node *VarNode = NULL;
	const DOMString StNodeName = NULL;
	const DOMString Temp = NULL;
	int ret_val = -1;

	/* Got the Envelop node here */
	EnvpNode = ixmlNode_getFirstChild((IXML_Node *) TempDoc);
	if (EnvpNode == NULL)
		goto error_handler;
	/* Got Body here */
	BodyNode = ixmlNode_getFirstChild(EnvpNode);
	if (BodyNode == NULL)
		goto error_handler;
	/* Got action node here */
	StNode = ixmlNode_getFirstChild(BodyNode);
	if (StNode == NULL)
		goto error_handler;
	/* Test whether this is the action node */
	StNodeName = ixmlNode_getNodeName(StNode);
	if (StNodeName == NULL ||
	    strstr(StNodeName, "QueryStateVariable") == NULL)
		goto error_handler;
	VarNameNode = ixmlNode_getFirstChild(StNode);
	if (VarNameNode == NULL)
		goto error_handler;
	VarNode = ixmlNode_getFirstChild(VarNameNode);
	Temp = ixmlNode_getNodeValue(VarNode);
	linecopy(VarName, Temp);
	UpnpPrintf(UPNP_INFO, SOAP, __FILE__, __LINE__,
		   "Received query for variable  name %s\n", VarName);

	/* success */
	ret_val = 0;

error_handler:
	return ret_val;
}

/*!
 * \brief Handles the SOAP requests to querry the state variables.
 * This functionality has been deprecated in the UPnP V1.0 architecture.
 */
static UPNP_INLINE void handle_query_variable(
	/*! [in] Socket info. */
	SOCKINFO *info,
	/*! [in] HTTP request. */
	http_message_t *request,
	/*! [in] Document containing the variable request SOAP message. */
	IXML_Document *xml_doc)
{
	Upnp_FunPtr soap_event_callback;
	void *cookie;
	char var_name[LINE_SIZE];
	struct Upnp_State_Var_Request variable;
	const char *err_str;
	int err_code;

	/* get var name */
	if (get_var_name(xml_doc, var_name) != 0) {
		send_error_response(info, SOAP_INVALID_VAR,
				    Soap_Invalid_Var, request);
		return;
	}
	/* get info for event */
	err_code = get_device_info(request, 1, xml_doc,
				   info->foreign_sockaddr.ss_family,
				   variable.DevUDN,
				   variable.ServiceID,
				   &soap_event_callback, &cookie);
	if (err_code != 0) {
		send_error_response(info, SOAP_INVALID_VAR,
				    Soap_Invalid_Var, request);
		return;
	}
	linecopy(variable.ErrStr, "");
	variable.ErrCode = UPNP_E_SUCCESS;
	namecopy(variable.StateVarName, var_name);
	variable.CurrentVal = NULL;
	variable.CtrlPtIPAddr = info->foreign_sockaddr;
	/* send event */
	soap_event_callback(UPNP_CONTROL_GET_VAR_REQUEST, &variable, cookie);
	UpnpPrintf(UPNP_INFO, SOAP, __FILE__, __LINE__,
		   "Return from callback for var request\n");
	/* validate, and handle result */
	if (variable.CurrentVal == NULL) {
		send_error_response(info, SOAP_INVALID_VAR, Soap_Invalid_Var,
				    request);
		return;
	}
	if (variable.ErrCode != UPNP_E_SUCCESS) {
		if (strlen(variable.ErrStr) == 0) {
			err_code = SOAP_INVALID_VAR;
			err_str = Soap_Invalid_Var;
		} else {
			err_code = variable.ErrCode;
			err_str = variable.ErrStr;
		}
		send_error_response(info, err_code, err_str, request);
		ixmlFreeDOMString(variable.CurrentVal);
		return;
	}
	/* send response */
	send_var_query_response(info, variable.CurrentVal, request);
	ixmlFreeDOMString(variable.CurrentVal);
}

/*!
 * \brief Handles the SOAP action request. It checks the integrity of the SOAP
 * action request and gives the call back to the device application.
 */
static void handle_invoke_action(
	/*! [in] Socket info. */
	IN SOCKINFO *info,
	/*! [in] HTTP Request. */
	IN http_message_t *request,
	/*! [in] Name of the SOAP Action. */
	IN memptr action_name,
	/*! [in] Document containing the SOAP action request. */
	IN IXML_Document *xml_doc)
{
	char save_char;
	IXML_Document *resp_node = NULL;
	struct Upnp_Action_Request action;
	Upnp_FunPtr soap_event_callback;
	void *cookie = NULL;
	int err_code;
	const char *err_str;

	action.ActionResult = NULL;
	/* null-terminate */
	save_char = action_name.buf[action_name.length];
	action_name.buf[action_name.length] = '\0';
	/* set default error */
	err_code = SOAP_INVALID_ACTION;
	err_str = Soap_Invalid_Action;
	/* get action node */
	if (get_action_node(xml_doc, action_name.buf, &resp_node) == -1)
		goto error_handler;
	/* get device info for action event */
	err_code = get_device_info(request,
				   0,
				   xml_doc,
				   info->foreign_sockaddr.ss_family,
				   action.DevUDN,
				   action.ServiceID,
				   &soap_event_callback, &cookie);

	if (err_code != UPNP_E_SUCCESS)
		goto error_handler;
	namecopy(action.ActionName, action_name.buf);
	linecopy(action.ErrStr, "");
	action.ActionRequest = resp_node;
	action.ActionResult = NULL;
	action.ErrCode = UPNP_E_SUCCESS;
	action.CtrlPtIPAddr = info->foreign_sockaddr;
	UpnpPrintf(UPNP_INFO, SOAP, __FILE__, __LINE__, "Calling Callback\n");
	soap_event_callback(UPNP_CONTROL_ACTION_REQUEST, &action, cookie);
	if (action.ErrCode != UPNP_E_SUCCESS) {
		if (strlen(action.ErrStr) <= 0) {
			err_code = SOAP_ACTION_FAILED;
			err_str = Soap_Action_Failed;
		} else {
			err_code = action.ErrCode;
			err_str = action.ErrStr;
		}
		goto error_handler;
	}
	/* validate, and handle action error */
	if (action.ActionResult == NULL) {
		err_code = SOAP_ACTION_FAILED;
		err_str = Soap_Action_Failed;
		goto error_handler;
	}
	/* send response */
	send_action_response(info, action.ActionResult, request);
	err_code = 0;

	/* error handling and cleanup */
error_handler:
	ixmlDocument_free(action.ActionResult);
	ixmlDocument_free(resp_node);
	/* restore */
	action_name.buf[action_name.length] = save_char;
	if (err_code != 0)
		send_error_response(info, err_code, err_str, request);
}

/*!
 * \brief This is a callback called by minisever after receiving the request
 * from the control point. This function will start processing the request.
 * It calls handle_invoke_action to handle the SOAP action.
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
	const char *err_str;
	memptr action_name;
	IXML_Document *xml_doc = NULL;

	/* set default error */
	err_code = SOAP_INVALID_ACTION;
	err_str = Soap_Invalid_Action;

	/* validate: content-type == text/xml */
	if (!has_xml_content_type(request))
		goto error_handler;
	/* type of request */
	if (get_request_type(request, &action_name) != 0)
		goto error_handler;
	/* parse XML */
	err_code = ixmlParseBufferEx(request->entity.buf, &xml_doc);
	if (err_code != IXML_SUCCESS) {
		if (err_code == IXML_INSUFFICIENT_MEMORY)
			err_code = UPNP_E_OUTOF_MEMORY;
		else
			err_code = SOAP_ACTION_FAILED;
		err_str = "XML error";
		goto error_handler;
	}
	if (action_name.length == 0)
		/* query var */
		handle_query_variable(info, request, xml_doc);
	else
		/* invoke action */
		handle_invoke_action(info, request, action_name, xml_doc);
	/* no error */
	err_code = 0;

 error_handler:
	ixmlDocument_free(xml_doc);
	if (err_code != 0)
		send_error_response(info, err_code, err_str, request);
	return;
	parser = parser;
}

#endif /* EXCLUDE_SOAP */

#endif /* INCLUDE_DEVICE_APIS */

