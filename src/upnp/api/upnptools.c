/**************************************************************************
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
 **************************************************************************/


#include "config.h"


/*!
 * \file
 */


#if EXCLUDE_DOM == 0


#include "upnp.h"
#include "upnptools.h"


#include "uri.h"


#include <stdarg.h>
#include <stdio.h>


/*! Maximum action header buffer length. */
#define HEADER_LENGTH 2000

#ifdef WIN32
	#define snprintf _snprintf
#endif

/*!
 * \brief Structure to maintain a error code and string associated with the
 * error code.
 */
struct ErrorString {
	/*! Error code. */
	int rc;
	/*! Error description. */
	const char *rcError;
};


/*!
 * \brief Array of error structures.
 */
struct ErrorString ErrorMessages[] = {
	{UPNP_E_SUCCESS, "UPNP_E_SUCCESS"},
	{UPNP_E_INVALID_HANDLE, "UPNP_E_INVALID_HANDLE"},
	{UPNP_E_INVALID_PARAM, "UPNP_E_INVALID_PARAM"},
	{UPNP_E_OUTOF_HANDLE, "UPNP_E_OUTOF_HANDLE"},
	{UPNP_E_OUTOF_CONTEXT, "UPNP_E_OUTOF_CONTEXT"},
	{UPNP_E_OUTOF_MEMORY, "UPNP_E_OUTOF_MEMORY"},
	{UPNP_E_INIT, "UPNP_E_INIT"},
	{UPNP_E_BUFFER_TOO_SMALL, "UPNP_E_BUFFER_TOO_SMALL"},
	{UPNP_E_INVALID_DESC, "UPNP_E_INVALID_DESC"},
	{UPNP_E_INVALID_URL, "UPNP_E_INVALID_URL"},
	{UPNP_E_INVALID_SID, "UPNP_E_INVALID_SID"},
	{UPNP_E_INVALID_DEVICE, "UPNP_E_INVALID_DEVICE"},
	{UPNP_E_INVALID_SERVICE, "UPNP_E_INVALID_SERVICE"},
	{UPNP_E_BAD_RESPONSE, "UPNP_E_BAD_RESPONSE"},
	{UPNP_E_BAD_REQUEST, "UPNP_E_BAD_REQUEST"},
	{UPNP_E_INVALID_ACTION, "UPNP_E_INVALID_ACTION"},
	{UPNP_E_FINISH, "UPNP_E_FINISH"},
	{UPNP_E_INIT_FAILED, "UPNP_E_INIT_FAILED"},
	{UPNP_E_URL_TOO_BIG, "UPNP_E_URL_TOO_BIG"},
	{UPNP_E_BAD_HTTPMSG, "UPNP_E_BAD_HTTPMSG"},
	{UPNP_E_ALREADY_REGISTERED, "UPNP_E_ALREADY_REGISTERED"},
	{UPNP_E_INVALID_INTERFACE, "UPNP_E_INVALID_INTERFACE"},
	{UPNP_E_NETWORK_ERROR, "UPNP_E_NETWORK_ERROR"},
	{UPNP_E_SOCKET_WRITE, "UPNP_E_SOCKET_WRITE"},
	{UPNP_E_SOCKET_READ, "UPNP_E_SOCKET_READ"},
	{UPNP_E_SOCKET_BIND, "UPNP_E_SOCKET_BIND"},
	{UPNP_E_SOCKET_CONNECT, "UPNP_E_SOCKET_CONNECT"},
	{UPNP_E_OUTOF_SOCKET, "UPNP_E_OUTOF_SOCKET"},
	{UPNP_E_LISTEN, "UPNP_E_LISTEN"},
	{UPNP_E_TIMEDOUT, "UPNP_E_TIMEDOUT"},
	{UPNP_E_SOCKET_ERROR, "UPNP_E_SOCKET_ERROR"},
	{UPNP_E_FILE_WRITE_ERROR, "UPNP_E_FILE_WRITE_ERROR"},
	{UPNP_E_CANCELED, "UPNP_E_CANCELED"},
	{UPNP_E_EVENT_PROTOCOL, "UPNP_E_EVENT_PROTOCOL"},
	{UPNP_E_SUBSCRIBE_UNACCEPTED, "UPNP_E_SUBSCRIBE_UNACCEPTED"},
	{UPNP_E_UNSUBSCRIBE_UNACCEPTED, "UPNP_E_UNSUBSCRIBE_UNACCEPTED"},
	{UPNP_E_NOTIFY_UNACCEPTED, "UPNP_E_NOTIFY_UNACCEPTED"},
	{UPNP_E_INVALID_ARGUMENT, "UPNP_E_INVALID_ARGUMENT"},
	{UPNP_E_FILE_NOT_FOUND, "UPNP_E_FILE_NOT_FOUND"},
	{UPNP_E_FILE_READ_ERROR, "UPNP_E_FILE_READ_ERROR"},
	{UPNP_E_EXT_NOT_XML, "UPNP_E_EXT_NOT_XML"},
	{UPNP_E_NO_WEB_SERVER, "UPNP_E_NO_WEB_SERVER"},
	{UPNP_E_OUTOF_BOUNDS, "UPNP_E_OUTOF_BOUNDS"},
	{UPNP_E_NOT_FOUND, "UPNP_E_NOT_FOUND"},
	{UPNP_E_INTERNAL_ERROR, "UPNP_E_INTERNAL_ERROR"},
};

const char *UpnpGetErrorMessage(int rc)
{
	size_t i;

	for (i = 0; i < sizeof (ErrorMessages) / sizeof (ErrorMessages[0]); ++i) {
		if (rc == ErrorMessages[i].rc) {
			return ErrorMessages[i].rcError;
		}
	}

	return "Unknown error code";
}

/*!
 * \todo There is some unnecessary allocation and deallocation going on here
 * because of the way resolve_rel_url() was originally written and used. In the
 * future it would be nice to clean this up.
 */
int UpnpResolveURL(
	const char *BaseURL,
	const char *RelURL,
	char *AbsURL)
{
	int ret = UPNP_E_SUCCESS;
	char *tempRel = NULL;

	if (!RelURL) {
		ret = UPNP_E_INVALID_PARAM;
		goto ExitFunction;
	}
	tempRel = resolve_rel_url((char *)BaseURL, (char *)RelURL);
	if (tempRel) {
		strcpy(AbsURL, tempRel);
		free(tempRel);
	} else
		ret = UPNP_E_INVALID_URL;

ExitFunction:
	return ret;
}


int UpnpResolveURL2(
	const char *BaseURL,
	const char *RelURL,
	char **AbsURL)
{
	int ret = UPNP_E_SUCCESS;

	if (!RelURL) {
		ret = UPNP_E_INVALID_PARAM;
		goto ExitFunction;
	}
	*AbsURL = resolve_rel_url((char *)BaseURL, (char *)RelURL);
	if (!*AbsURL)
		ret = UPNP_E_INVALID_URL;

ExitFunction:
	return ret;
}


/*!
 * \brief Adds the argument in the action request or response.
 *
 * This function creates the action request or response if it is a first
 * argument, otherwise it will add the argument in the document.
 *
 * \returns UPNP_E_SUCCESS if successful, otherwise the appropriate error.
 */
static int addToAction(
	/*! [in] flag to tell if the ActionDoc is for response or request. */
	int response,
	/*! [in,out] Request or response document. */
	IXML_Document **ActionDoc,
	/*! [in] Name of the action request or response. */
	const char *ActionName,
	/*! [in] Service type. */
	const char *ServType,
	/*! [in] Name of the argument. */
	const char *ArgName,
	/*! [in] Value of the argument. */
	const char *ArgValue)
{
	char *ActBuff = NULL;
	IXML_Node *node = NULL;
	IXML_Element *Ele = NULL;
	IXML_Node *Txt = NULL;
	int rc = 0;

	if (ActionName == NULL || ServType == NULL) {
		return UPNP_E_INVALID_PARAM;
	}

	if (*ActionDoc == NULL) {
		ActBuff = (char *)malloc(HEADER_LENGTH);
		if (ActBuff == NULL) {
			return UPNP_E_OUTOF_MEMORY;
		}

		if (response) {
			rc = snprintf(ActBuff, HEADER_LENGTH,
				"<u:%sResponse xmlns:u=\"%s\">\r\n</u:%sResponse>",
				ActionName, ServType, ActionName);
		} else {
			rc = snprintf(ActBuff, HEADER_LENGTH,
				"<u:%s xmlns:u=\"%s\">\r\n</u:%s>",
				ActionName, ServType, ActionName);
		}
		if (rc < 0 || (unsigned int) rc >= HEADER_LENGTH) {
			free(ActBuff);
			return UPNP_E_OUTOF_MEMORY;
		}

		rc = ixmlParseBufferEx(ActBuff, ActionDoc);
		free(ActBuff);
		if (rc != IXML_SUCCESS) {
			if (rc == IXML_INSUFFICIENT_MEMORY) {
				return UPNP_E_OUTOF_MEMORY;
			} else {
				return UPNP_E_INVALID_DESC;
			}
		}
	}

	if (ArgName != NULL /*&& ArgValue != NULL */) {
		node = ixmlNode_getFirstChild((IXML_Node *)*ActionDoc);
		Ele = ixmlDocument_createElement(*ActionDoc, ArgName);
		if(ArgValue) {
			Txt = ixmlDocument_createTextNode(*ActionDoc, ArgValue);
			ixmlNode_appendChild((IXML_Node *)Ele, Txt);
		}
		ixmlNode_appendChild(node, (IXML_Node *)Ele);
	}

	return UPNP_E_SUCCESS;
}


/*!
 * \brief Creates the action request or response from the argument list.
 *
 * \return Action request or response document if successful, otherwise
 * 	returns NULL
 */
static IXML_Document *makeAction(
	/*! [in] flag to tell if the ActionDoc is for response or request. */
	int response,
	/*! [in] Name of the action request or response. */
	const char *ActionName,
	/*! [in] Service type. */
	const char *ServType,
	/*! [in] Number of arguments in the action request or response. */
	int NumArg,
	/*! [in] pointer to the first argument. */
	const char *Arg,
	/*! [in] Argument list. */
	va_list ArgList)
{
	const char *ArgName;
	const char *ArgValue;
	char *ActBuff;
	int Idx = 0;
	IXML_Document *ActionDoc;
	IXML_Node *node;
	IXML_Element *Ele;
	IXML_Node *Txt = NULL;
	int rc = 0;

	if (ActionName == NULL || ServType == NULL) {
		return NULL;
	}

	ActBuff = (char *)malloc(HEADER_LENGTH);
	if (ActBuff == NULL) {
		return NULL;
	}

	if (response) {
		rc = snprintf(ActBuff, HEADER_LENGTH,
			"<u:%sResponse xmlns:u=\"%s\">\r\n</u:%sResponse>",
			ActionName, ServType, ActionName);
	} else {
		rc = snprintf(ActBuff, HEADER_LENGTH,
			"<u:%s xmlns:u=\"%s\">\r\n</u:%s>",
			ActionName, ServType, ActionName);
	}
	if (rc < 0 || (unsigned int) rc >= HEADER_LENGTH ||
		ixmlParseBufferEx(ActBuff, &ActionDoc) != IXML_SUCCESS) {
		free(ActBuff);
		return NULL;
	}

	free(ActBuff);
	if(ActionDoc == NULL) {
		return NULL;
	}

	if (NumArg > 0) {
		/*va_start(ArgList, Arg); */
		ArgName = Arg;
		for ( ; ; ) {
			ArgValue = va_arg(ArgList, const char *);
			if (ArgName != NULL) {
				node = ixmlNode_getFirstChild((IXML_Node *)ActionDoc);
				Ele = ixmlDocument_createElement(ActionDoc, ArgName);
				if (ArgValue) {
					Txt = ixmlDocument_createTextNode(ActionDoc, ArgValue);
					ixmlNode_appendChild((IXML_Node *)Ele, Txt);
				}
				ixmlNode_appendChild(node, (IXML_Node *)Ele);
			}
			if (++Idx < NumArg) {
				ArgName = va_arg(ArgList, const char *);
			} else {
				break;
			}
		}
		/*va_end(ArgList); */
	}

	return ActionDoc;
}


IXML_Document *UpnpMakeAction(
	const char *ActionName,
	const char *ServType,
	int NumArg,
	const char *Arg,
	...)
{
	va_list ArgList;
	IXML_Document *out = NULL;

	va_start(ArgList, Arg);
	out = makeAction(0, ActionName, ServType, NumArg, Arg, ArgList);
	va_end(ArgList);

	return out;
}


IXML_Document *UpnpMakeActionResponse(
	const char *ActionName,
	const char *ServType,
	int NumArg,
	const char *Arg,
	...)
{
	va_list ArgList;
	IXML_Document *out = NULL;

	va_start(ArgList, Arg);
	out = makeAction(1, ActionName, ServType, NumArg, Arg, ArgList);
	va_end(ArgList);

	return out;
}


int UpnpAddToAction(
	IXML_Document **ActionDoc,
	const char *ActionName,
	const char *ServType,
	const char *ArgName,
	const char *ArgValue)
{
	return addToAction(0, ActionDoc, ActionName, ServType, ArgName, ArgValue);
}


int UpnpAddToActionResponse(
	IXML_Document **ActionResponse,
	const char *ActionName,
	const char *ServType,
	const char *ArgName,
	const char *ArgValue)
{
	return addToAction(1, ActionResponse, ActionName, ServType, ArgName, ArgValue);
}


IXML_Document *UpnpCreatePropertySet(
	int NumArg,
	const char *Arg,
	...)
{
	va_list ArgList;
	int Idx = 0;
	char BlankDoc[] =
		"<e:propertyset xmlns:e=\"urn:schemas-upnp-org:event-1-0\">"
		"</e:propertyset>";
	const char *ArgName,
	*ArgValue;
	IXML_Node *node;
	IXML_Element *Ele;
	IXML_Element *Ele1;
	IXML_Node *Txt;
	IXML_Document *PropSet;

	if(ixmlParseBufferEx(BlankDoc, &PropSet) != IXML_SUCCESS) {
		return NULL;
	}

	if (NumArg < 1) {
		return PropSet;
	}

	va_start(ArgList, Arg);
	ArgName = Arg;
	while (Idx++ != NumArg) {
		ArgValue = va_arg(ArgList, const char *);
		if (ArgName != NULL /*&& ArgValue != NULL */) {
			node = ixmlNode_getFirstChild((IXML_Node *)PropSet);
			Ele1 = ixmlDocument_createElement(PropSet, "e:property");
			Ele = ixmlDocument_createElement(PropSet, ArgName);
			if (ArgValue) {
				Txt = ixmlDocument_createTextNode(PropSet, ArgValue);
				ixmlNode_appendChild((IXML_Node *)Ele, Txt);
			}
			ixmlNode_appendChild((IXML_Node *)Ele1, (IXML_Node *)Ele);
			ixmlNode_appendChild(             node, (IXML_Node *)Ele1);
		}
		ArgName = va_arg(ArgList, const char *);
	}
	va_end(ArgList);

	return PropSet;
}


int UpnpAddToPropertySet(
	IXML_Document **PropSet,
	const char *ArgName,
	const char *ArgValue)
{
	char BlankDoc[] =
		"<e:propertyset xmlns:e=\"urn:schemas-upnp-org:event-1-0\">"
		"</e:propertyset>";
	IXML_Node *node;
	IXML_Element *Ele;
	IXML_Element *Ele1;
	IXML_Node *Txt;
	int rc;

	if (ArgName == NULL) {
		return UPNP_E_INVALID_PARAM;
	}

	if (*PropSet == NULL) {
		rc = ixmlParseBufferEx(BlankDoc, PropSet);
		if (rc != IXML_SUCCESS) {
			return UPNP_E_OUTOF_MEMORY;
		}
	}

	node = ixmlNode_getFirstChild((IXML_Node *)*PropSet);

	Ele1 = ixmlDocument_createElement(*PropSet, "e:property");
	Ele = ixmlDocument_createElement(*PropSet, ArgName);

	if (ArgValue) {
		Txt = ixmlDocument_createTextNode(*PropSet, ArgValue);
		ixmlNode_appendChild((IXML_Node *)Ele, Txt);
	}

	ixmlNode_appendChild((IXML_Node *)Ele1, (IXML_Node *)Ele);
	ixmlNode_appendChild(node, (IXML_Node *)Ele1);

	return UPNP_E_SUCCESS;
}


#endif /* EXCLUDE_DOM == 0 */

