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

/*!
 * \addtogroup UpnpSamples
 *
 * @{
 *
 * \name Device Sample Module
 *
 * @{
 *
 * \file
 */

#include "tv_device.h"

#include <assert.h>

#define DEFAULT_WEB_DIR "./web"

#define DESC_URL_SIZE 200

/*! Global arrays for storing Tv Control Service variable names, values,
 * and defaults. */
const char *tvc_varname[] = { "Power", "Channel", "Volume" };

char tvc_varval[TV_CONTROL_VARCOUNT][TV_MAX_VAL_LEN];
const char *tvc_varval_def[] = { "1", "1", "5" };

/*! Global arrays for storing Tv Picture Service variable names, values,
 * and defaults. */
const char *tvp_varname[] = { "Color", "Tint", "Contrast", "Brightness" };

char tvp_varval[TV_PICTURE_VARCOUNT][TV_MAX_VAL_LEN];
const char *tvp_varval_def[] = { "5", "5", "5", "5" };

/*! The amount of time (in seconds) before advertisements will expire. */
int default_advr_expire = 100;

/*! Global structure for storing the state table for this device. */
struct TvService tv_service_table[2];

/*! Device handle supplied by UPnP SDK. */
UpnpDevice_Handle device_handle = -1;

/*! Mutex for protecting the global state table data
 * in a multi-threaded, asynchronous environment.
 * All functions should lock this mutex before reading
 * or writing the state table data. */
ithread_mutex_t TVDevMutex;

/*! Color constants */
#define MAX_COLOR 10
#define MIN_COLOR 1

/*! Brightness constants */
#define MAX_BRIGHTNESS 10
#define MIN_BRIGHTNESS 1

/*! Power constants */
#define POWER_ON 1
#define POWER_OFF 0

/*! Tint constants */
#define MAX_TINT 10
#define MIN_TINT 1

/*! Volume constants */
#define MAX_VOLUME 10
#define MIN_VOLUME 1

/*! Contrast constants */
#define MAX_CONTRAST 10
#define MIN_CONTRAST 1

/*! Channel constants */
#define MAX_CHANNEL 100
#define MIN_CHANNEL 1

/*!
 * \brief Initializes the service table for the specified service.
 */
static int SetServiceTable(
	/*! [in] one of TV_SERVICE_CONTROL or, TV_SERVICE_PICTURE. */
	int serviceType,
	/*! [in] UDN of device containing service. */
	const char *UDN,
	/*! [in] serviceId of service. */
	const char *serviceId,
	/*! [in] service type (as specified in Description Document) . */
	const char *serviceTypeS,
	/*! [in,out] service containing table to be set. */
	struct TvService *out)
{
	int i = 0;

	strcpy(out->UDN, UDN);
	strcpy(out->ServiceId, serviceId);
	strcpy(out->ServiceType, serviceTypeS);

	switch (serviceType) {
	case TV_SERVICE_CONTROL:
		out->VariableCount = TV_CONTROL_VARCOUNT;
		for (i = 0;
		     i < tv_service_table[TV_SERVICE_CONTROL].VariableCount;
		     i++) {
			tv_service_table[TV_SERVICE_CONTROL].VariableName[i]
			    = tvc_varname[i];
			tv_service_table[TV_SERVICE_CONTROL].VariableStrVal[i]
			    = tvc_varval[i];
			strcpy(tv_service_table[TV_SERVICE_CONTROL].
				VariableStrVal[i], tvc_varval_def[i]);
		}
		break;
	case TV_SERVICE_PICTURE:
		out->VariableCount = TV_PICTURE_VARCOUNT;
		for (i = 0;
		     i < tv_service_table[TV_SERVICE_PICTURE].VariableCount;
		     i++) {
			tv_service_table[TV_SERVICE_PICTURE].VariableName[i] =
			    tvp_varname[i];
			tv_service_table[TV_SERVICE_PICTURE].VariableStrVal[i] =
			    tvp_varval[i];
			strcpy(tv_service_table[TV_SERVICE_PICTURE].
				VariableStrVal[i], tvp_varval_def[i]);
		}
		break;
	default:
		assert(0);
	}

	return SetActionTable(serviceType, out);
}

int SetActionTable(int serviceType, struct TvService *out)
{
	if (serviceType == TV_SERVICE_CONTROL) {
		out->ActionNames[0] = "PowerOn";
		out->actions[0] = TvDevicePowerOn;
		out->ActionNames[1] = "PowerOff";
		out->actions[1] = TvDevicePowerOff;
		out->ActionNames[2] = "SetChannel";
		out->actions[2] = TvDeviceSetChannel;
		out->ActionNames[3] = "IncreaseChannel";
		out->actions[3] = TvDeviceIncreaseChannel;
		out->ActionNames[4] = "DecreaseChannel";
		out->actions[4] = TvDeviceDecreaseChannel;
		out->ActionNames[5] = "SetVolume";
		out->actions[5] = TvDeviceSetVolume;
		out->ActionNames[6] = "IncreaseVolume";
		out->actions[6] = TvDeviceIncreaseVolume;
		out->ActionNames[7] = "DecreaseVolume";
		out->actions[7] = TvDeviceDecreaseVolume;
		out->ActionNames[8] = NULL;
		return 1;
	} else if (serviceType == TV_SERVICE_PICTURE) {
		out->ActionNames[0] = "SetColor";
		out->ActionNames[1] = "IncreaseColor";
		out->ActionNames[2] = "DecreaseColor";
		out->actions[0] = TvDeviceSetColor;
		out->actions[1] = TvDeviceIncreaseColor;
		out->actions[2] = TvDeviceDecreaseColor;
		out->ActionNames[3] = "SetTint";
		out->ActionNames[4] = "IncreaseTint";
		out->ActionNames[5] = "DecreaseTint";
		out->actions[3] = TvDeviceSetTint;
		out->actions[4] = TvDeviceIncreaseTint;
		out->actions[5] = TvDeviceDecreaseTint;

		out->ActionNames[6] = "SetBrightness";
		out->ActionNames[7] = "IncreaseBrightness";
		out->ActionNames[8] = "DecreaseBrightness";
		out->actions[6] = TvDeviceSetBrightness;
		out->actions[7] = TvDeviceIncreaseBrightness;
		out->actions[8] = TvDeviceDecreaseBrightness;

		out->ActionNames[9] = "SetContrast";
		out->ActionNames[10] = "IncreaseContrast";
		out->ActionNames[11] = "DecreaseContrast";

		out->actions[9] = TvDeviceSetContrast;
		out->actions[10] = TvDeviceIncreaseContrast;
		out->actions[11] = TvDeviceDecreaseContrast;
		return 1;
	}

	return 0;
}

int TvDeviceStateTableInit(char *DescDocURL)
{
	IXML_Document *DescDoc = NULL;
	int ret = UPNP_E_SUCCESS;
	char *servid_ctrl = NULL;
	char *evnturl_ctrl = NULL;
	char *ctrlurl_ctrl = NULL;
	char *servid_pict = NULL;
	char *evnturl_pict = NULL;
	char *ctrlurl_pict = NULL;
	char *udn = NULL;

	/*Download description document */
	if (UpnpDownloadXmlDoc(DescDocURL, &DescDoc) != UPNP_E_SUCCESS) {
		SampleUtil_Print("TvDeviceStateTableInit -- Error Parsing %s\n",
				 DescDocURL);
		ret = UPNP_E_INVALID_DESC;
		goto error_handler;
	}
	udn = SampleUtil_GetFirstDocumentItem(DescDoc, "UDN");
	/* Find the Tv Control Service identifiers */
	if (!SampleUtil_FindAndParseService(DescDoc, DescDocURL,
					    TvServiceType[TV_SERVICE_CONTROL],
					    &servid_ctrl, &evnturl_ctrl,
					    &ctrlurl_ctrl)) {
		SampleUtil_Print("TvDeviceStateTableInit -- Error: Could not find Service: %s\n",
				 TvServiceType[TV_SERVICE_CONTROL]);
		ret = UPNP_E_INVALID_DESC;
		goto error_handler;
	}
	/* set control service table */
	SetServiceTable(TV_SERVICE_CONTROL, udn, servid_ctrl,
			TvServiceType[TV_SERVICE_CONTROL],
			&tv_service_table[TV_SERVICE_CONTROL]);

	/* Find the Tv Picture Service identifiers */
	if (!SampleUtil_FindAndParseService(DescDoc, DescDocURL,
					    TvServiceType[TV_SERVICE_PICTURE],
					    &servid_pict, &evnturl_pict,
					    &ctrlurl_pict)) {
		SampleUtil_Print("TvDeviceStateTableInit -- Error: Could not find Service: %s\n",
				 TvServiceType[TV_SERVICE_PICTURE]);
		ret = UPNP_E_INVALID_DESC;
		goto error_handler;
	}
	/* set picture service table */
	SetServiceTable(TV_SERVICE_PICTURE, udn, servid_pict,
			TvServiceType[TV_SERVICE_PICTURE],
			&tv_service_table[TV_SERVICE_PICTURE]);

error_handler:
	/* clean up */
	if (udn)
		free(udn);
	if (servid_ctrl)
		free(servid_ctrl);
	if (evnturl_ctrl)
		free(evnturl_ctrl);
	if (ctrlurl_ctrl)
		free(ctrlurl_ctrl);
	if (servid_pict)
		free(servid_pict);
	if (evnturl_pict)
		free(evnturl_pict);
	if (ctrlurl_pict)
		free(ctrlurl_pict);
	if (DescDoc)
		ixmlDocument_free(DescDoc);

	return (ret);
}

int TvDeviceHandleSubscriptionRequest(const UpnpSubscriptionRequest *sr_event)
{
	unsigned int i = 0;
	int cmp1 = 0;
	int cmp2 = 0;
	const char *l_serviceId = NULL;
	const char *l_udn = NULL;
	const char *l_sid = NULL;

	/* lock state mutex */
	ithread_mutex_lock(&TVDevMutex);

	l_serviceId = UpnpString_get_String(UpnpSubscriptionRequest_get_ServiceId(sr_event));
	l_udn = UpnpSubscriptionRequest_get_UDN_cstr(sr_event);
	l_sid = UpnpSubscriptionRequest_get_SID_cstr(sr_event);
	for (i = 0; i < TV_SERVICE_SERVCOUNT; ++i) {
		cmp1 = strcmp(l_udn, tv_service_table[i].UDN);
		cmp2 = strcmp(l_serviceId, tv_service_table[i].ServiceId);
		if (cmp1 == 0 && cmp2 == 0) {
#if 0
			PropSet = NULL;

			for (j = 0; j < tv_service_table[i].VariableCount; ++j) {
				/* add each variable to the property set */
				/* for initial state dump */
				UpnpAddToPropertySet(&PropSet,
						     tv_service_table[i].
						     VariableName[j],
						     tv_service_table[i].
						     VariableStrVal[j]);
			}

			/* dump initial state  */
			UpnpAcceptSubscriptionExt(device_handle,
						  l_udn,
						  l_serviceId, PropSet, l_sid);
			/* free document */
			Document_free(PropSet);
#endif
			UpnpAcceptSubscription(device_handle,
					       l_udn,
					       l_serviceId,
					       (const char **)
					       tv_service_table[i].VariableName,
					       (const char **)
					       tv_service_table
					       [i].VariableStrVal,
					       tv_service_table[i].
					       VariableCount, l_sid);
		}
	}

	ithread_mutex_unlock(&TVDevMutex);

	return 1;
}

int TvDeviceHandleGetVarRequest(UpnpStateVarRequest *cgv_event)
{
	unsigned int i = 0;
	int j = 0;
	int getvar_succeeded = 0;

	UpnpStateVarRequest_set_CurrentVal(cgv_event, NULL);

	ithread_mutex_lock(&TVDevMutex);

	for (i = 0; i < TV_SERVICE_SERVCOUNT; i++) {
		/* check udn and service id */
		const char *devUDN =
			UpnpString_get_String(UpnpStateVarRequest_get_DevUDN(cgv_event));
		const char *serviceID =
			UpnpString_get_String(UpnpStateVarRequest_get_ServiceID(cgv_event));
		if (strcmp(devUDN, tv_service_table[i].UDN) == 0 &&
		    strcmp(serviceID, tv_service_table[i].ServiceId) == 0) {
			/* check variable name */
			for (j = 0; j < tv_service_table[i].VariableCount; j++) {
				const char *stateVarName = UpnpString_get_String(
					UpnpStateVarRequest_get_StateVarName(cgv_event));
				if (strcmp(stateVarName,
					   tv_service_table[i].VariableName[j]) == 0) {
					getvar_succeeded = 1;
					UpnpStateVarRequest_set_CurrentVal(cgv_event,
						tv_service_table[i].VariableStrVal[j]);
					break;
				}
			}
		}
	}
	if (getvar_succeeded) {
		UpnpStateVarRequest_set_ErrCode(cgv_event, UPNP_E_SUCCESS);
	} else {
		SampleUtil_Print("Error in UPNP_CONTROL_GET_VAR_REQUEST callback:\n"
			"   Unknown variable name = %s\n",
			UpnpString_get_String(UpnpStateVarRequest_get_StateVarName(cgv_event)));
		UpnpStateVarRequest_set_ErrCode(cgv_event, 404);
		UpnpStateVarRequest_strcpy_ErrStr(cgv_event, "Invalid Variable");
	}

	ithread_mutex_unlock(&TVDevMutex);

	return UpnpStateVarRequest_get_ErrCode(cgv_event) == UPNP_E_SUCCESS;
}

int TvDeviceHandleActionRequest(UpnpActionRequest *ca_event)
{
	/* Defaults if action not found. */
	int action_found = 0;
	int i = 0;
	int service = -1;
	int retCode = 0;
	const char *errorString = NULL;
	const char *devUDN = NULL;
	const char *serviceID = NULL;
	const char *actionName = NULL;
	IXML_Document *actionResult = NULL;

	UpnpActionRequest_set_ErrCode(ca_event, 0);
	UpnpActionRequest_set_ActionResult(ca_event, NULL);

	devUDN     = UpnpString_get_String(UpnpActionRequest_get_DevUDN(    ca_event));
	serviceID  = UpnpString_get_String(UpnpActionRequest_get_ServiceID( ca_event));
	actionName = UpnpString_get_String(UpnpActionRequest_get_ActionName(ca_event));
	if (strcmp(devUDN,    tv_service_table[TV_SERVICE_CONTROL].UDN) == 0 &&
	    strcmp(serviceID, tv_service_table[TV_SERVICE_CONTROL].ServiceId) == 0) {
		/* Request for action in the TvDevice Control Service. */
		service = TV_SERVICE_CONTROL;
	} else if (strcmp(devUDN,       tv_service_table[TV_SERVICE_PICTURE].UDN) == 0 &&
		   strcmp(serviceID, tv_service_table[TV_SERVICE_PICTURE].ServiceId) == 0) {
		/* Request for action in the TvDevice Picture Service. */
		service = TV_SERVICE_PICTURE;
	}
	/* Find and call appropriate procedure based on action name.
	 * Each action name has an associated procedure stored in the
	 * service table. These are set at initialization. */
	for (i = 0;
	     i < TV_MAXACTIONS && tv_service_table[service].ActionNames[i] != NULL;
	     i++) {
		if (!strcmp(actionName, tv_service_table[service].ActionNames[i])) {
			if (!strcmp(tv_service_table[TV_SERVICE_CONTROL].
				    VariableStrVal[TV_CONTROL_POWER], "1") ||
			    !strcmp(actionName, "PowerOn")) {
				retCode = tv_service_table[service].actions[i](
					UpnpActionRequest_get_ActionRequest(ca_event),
					&actionResult,
					&errorString);
				UpnpActionRequest_set_ActionResult(ca_event, actionResult);
			} else {
				errorString = "Power is Off";
				retCode = UPNP_E_INTERNAL_ERROR;
			}
			action_found = 1;
			break;
		}
	}

	if (!action_found) {
		UpnpActionRequest_set_ActionResult(ca_event, NULL);
		UpnpActionRequest_strcpy_ErrStr(ca_event, "Invalid Action");
		UpnpActionRequest_set_ErrCode(ca_event, 401);
	} else {
		if (retCode == UPNP_E_SUCCESS) {
			UpnpActionRequest_set_ErrCode(ca_event, UPNP_E_SUCCESS);
		} else {
			/* copy the error string */
			UpnpActionRequest_strcpy_ErrStr(ca_event, errorString);
			switch (retCode) {
			case UPNP_E_INVALID_PARAM:
				UpnpActionRequest_set_ErrCode(ca_event, 402);
				break;
			case UPNP_E_INTERNAL_ERROR:
			default:
				UpnpActionRequest_set_ErrCode(ca_event, 501);
				break;
			}
		}
	}

	return UpnpActionRequest_get_ErrCode(ca_event);
}

int TvDeviceSetServiceTableVar(unsigned int service, int variable, char *value)
{
	/* IXML_Document  *PropSet= NULL; */
	if (service >= TV_SERVICE_SERVCOUNT ||
	    variable >= tv_service_table[service].VariableCount ||
	    strlen(value) >= TV_MAX_VAL_LEN)
		return (0);

	ithread_mutex_lock(&TVDevMutex);

	strcpy(tv_service_table[service].VariableStrVal[variable], value);
#if 0
	/* Using utility api */
	PropSet = UpnpCreatePropertySet(1,
		tv_service_table[service].VariableName[variable],
		tv_service_table[service].VariableStrVal[variable]);
	UpnpNotifyExt(device_handle, tv_service_table[service].UDN,
		tv_service_table[service].ServiceId, PropSet);
	/* Free created property set */
	Document_free(PropSet);
#endif
	UpnpNotify(device_handle,
		tv_service_table[service].UDN,
		tv_service_table[service].ServiceId,
		(const char **)&tv_service_table[service].VariableName[variable],
		(const char **)&tv_service_table[service].VariableStrVal[variable], 1);

	ithread_mutex_unlock(&TVDevMutex);

	return 1;
}

/*!
 * \brief Turn the power on/off, update the TvDevice control service
 * state table, and notify all subscribed control points of the
 * updated state.
 */
static int TvDeviceSetPower(
	/*! [in] If 1, turn power on. If 0, turn power off. */
	int on)
{
	char value[TV_MAX_VAL_LEN];
	int ret = 0;

	if (on != POWER_ON && on != POWER_OFF) {
		SampleUtil_Print("error: can't set power to value %d\n", on);
		return 0;
	}

	/* Vendor-specific code to turn the power on/off goes here. */

	sprintf(value, "%d", on);
	ret = TvDeviceSetServiceTableVar(TV_SERVICE_CONTROL, TV_CONTROL_POWER,
					 value);

	return ret;
}

int TvDevicePowerOn(IXML_Document * in,IXML_Document **out,
	const char **errorString)
{
	(*out) = NULL;
	(*errorString) = NULL;

	if (TvDeviceSetPower(POWER_ON)) {
		/* create a response */
		if (UpnpAddToActionResponse(out, "PowerOn",
					    TvServiceType[TV_SERVICE_CONTROL],
					    "Power", "1") != UPNP_E_SUCCESS) {
			(*out) = NULL;
			(*errorString) = "Internal Error";
			return UPNP_E_INTERNAL_ERROR;
		}
		return UPNP_E_SUCCESS;
	} else {
		(*errorString) = "Internal Error";
		return UPNP_E_INTERNAL_ERROR;
	}
	in = in;
}

int TvDevicePowerOff(IXML_Document *in, IXML_Document **out,
	const char **errorString)
{
	(*out) = NULL;
	(*errorString) = NULL;
	if (TvDeviceSetPower(POWER_OFF)) {
		/*create a response */

		if (UpnpAddToActionResponse(out, "PowerOff",
					    TvServiceType[TV_SERVICE_CONTROL],
					    "Power", "0") != UPNP_E_SUCCESS) {
			(*out) = NULL;
			(*errorString) = "Internal Error";
			return UPNP_E_INTERNAL_ERROR;
		}
		return UPNP_E_SUCCESS;
	}
	(*errorString) = "Internal Error";
	return UPNP_E_INTERNAL_ERROR;
	in = in;
}

int TvDeviceSetChannel(IXML_Document * in, IXML_Document ** out,
		       const char **errorString)
{
	char *value = NULL;
	int channel = 0;

	(*out) = NULL;
	(*errorString) = NULL;
	if (!(value = SampleUtil_GetFirstDocumentItem(in, "Channel"))) {
		(*errorString) = "Invalid Channel";
		return UPNP_E_INVALID_PARAM;
	}
	channel = atoi(value);
	if (channel < MIN_CHANNEL || channel > MAX_CHANNEL) {
		free(value);
		SampleUtil_Print("error: can't change to channel %d\n",
				 channel);
		(*errorString) = "Invalid Channel";
		return UPNP_E_INVALID_PARAM;
	}
	/* Vendor-specific code to set the channel goes here. */
	if (TvDeviceSetServiceTableVar(TV_SERVICE_CONTROL,
				       TV_CONTROL_CHANNEL, value)) {
		if (UpnpAddToActionResponse(out, "SetChannel",
					    TvServiceType[TV_SERVICE_CONTROL],
					    "NewChannel",
					    value) != UPNP_E_SUCCESS) {
			(*out) = NULL;
			(*errorString) = "Internal Error";
			free(value);
			return UPNP_E_INTERNAL_ERROR;
		}
		free(value);
		return UPNP_E_SUCCESS;
	} else {
		free(value);
		(*errorString) = "Internal Error";
		return UPNP_E_INTERNAL_ERROR;
	}
}

int IncrementChannel(int incr, IN IXML_Document * in, IXML_Document ** out,
		     const char **errorString)
{
	int curchannel;
	int newchannel;
	const char *actionName = NULL;
	char value[TV_MAX_VAL_LEN];

	if (incr > 0) {
		actionName = "IncreaseChannel";
	} else {
		actionName = "DecreaseChannel";
	}

	ithread_mutex_lock(&TVDevMutex);
	curchannel =
	    atoi(tv_service_table[TV_SERVICE_CONTROL].VariableStrVal
		 [TV_CONTROL_CHANNEL]);
	ithread_mutex_unlock(&TVDevMutex);

	newchannel = curchannel + incr;

	if (newchannel < MIN_CHANNEL || newchannel > MAX_CHANNEL) {
		SampleUtil_Print("error: can't change to channel %d\n",
				 newchannel);
		(*errorString) = "Invalid Channel";
		return UPNP_E_INVALID_PARAM;
	}
	/* Vendor-specific code to set the channel goes here. */
	sprintf(value, "%d", newchannel);
	if (TvDeviceSetServiceTableVar(TV_SERVICE_CONTROL,
				       TV_CONTROL_CHANNEL, value)) {
		if (UpnpAddToActionResponse(out, actionName,
					    TvServiceType[TV_SERVICE_CONTROL],
					    "Channel", value) != UPNP_E_SUCCESS)
		{
			(*out) = NULL;
			(*errorString) = "Internal Error";
			return UPNP_E_INTERNAL_ERROR;
		}
		return UPNP_E_SUCCESS;
	} else {
		(*errorString) = "Internal Error";
		return UPNP_E_INTERNAL_ERROR;
	}
	in = in;
}

int TvDeviceDecreaseChannel(IXML_Document * in, IXML_Document ** out,
			    const char **errorString)
{
	return IncrementChannel(-1, in, out, errorString);
}

int TvDeviceIncreaseChannel(IXML_Document * in, IXML_Document ** out,
			    const char **errorString)
{
	return IncrementChannel(1, in, out, errorString);
}

int TvDeviceSetVolume(IXML_Document * in, IXML_Document ** out,
		      const char **errorString)
{
	char *value = NULL;
	int volume = 0;

	(*out) = NULL;
	(*errorString) = NULL;
	if (!(value = SampleUtil_GetFirstDocumentItem(in, "Volume"))) {
		(*errorString) = "Invalid Volume";
		return UPNP_E_INVALID_PARAM;
	}
	volume = atoi(value);
	if (volume < MIN_VOLUME || volume > MAX_VOLUME) {
		SampleUtil_Print("error: can't change to volume %d\n", volume);
		(*errorString) = "Invalid Volume";
		return UPNP_E_INVALID_PARAM;
	}
	/* Vendor-specific code to set the volume goes here. */
	if (TvDeviceSetServiceTableVar(TV_SERVICE_CONTROL,
				       TV_CONTROL_VOLUME, value)) {
		if (UpnpAddToActionResponse(out, "SetVolume",
					    TvServiceType[TV_SERVICE_CONTROL],
					    "NewVolume",
					    value) != UPNP_E_SUCCESS) {
			(*out) = NULL;
			(*errorString) = "Internal Error";
			free(value);
			return UPNP_E_INTERNAL_ERROR;
		}
		free(value);
		return UPNP_E_SUCCESS;
	} else {
		free(value);
		(*errorString) = "Internal Error";
		return UPNP_E_INTERNAL_ERROR;
	}
}

/*!
 * \brief Increment the volume. Read the current volume from the state table,
 * add the increment, and then change the volume.
 */
static int IncrementVolume(
	/*! [in] The increment by which to change the volume. */
	int incr,
	/*! [in] Action request document. */
	IXML_Document * in,
	/*! [out] Action result document. */
	IXML_Document ** out,
	/*! [out] Error string in case action was unsuccessful. */
	const char **errorString)
{
	int curvolume;
	int newvolume;
	const char *actionName = NULL;
	char value[TV_MAX_VAL_LEN];

	if (incr > 0) {
		actionName = "IncreaseVolume";
	} else {
		actionName = "DecreaseVolume";
	}

	ithread_mutex_lock(&TVDevMutex);
	curvolume =
	    atoi(tv_service_table[TV_SERVICE_CONTROL].VariableStrVal
		 [TV_CONTROL_VOLUME]);
	ithread_mutex_unlock(&TVDevMutex);

	newvolume = curvolume + incr;
	if (newvolume < MIN_VOLUME || newvolume > MAX_VOLUME) {
		SampleUtil_Print("error: can't change to volume %d\n",
				 newvolume);
		(*errorString) = "Invalid Volume";
		return UPNP_E_INVALID_PARAM;
	}
	/* Vendor-specific code to set the volume goes here. */
	sprintf(value, "%d", newvolume);
	if (TvDeviceSetServiceTableVar(TV_SERVICE_CONTROL,
				       TV_CONTROL_VOLUME, value)) {
		if (UpnpAddToActionResponse(out, actionName,
					    TvServiceType[TV_SERVICE_CONTROL],
					    "Volume",
					    value) != UPNP_E_SUCCESS) {
			(*out) = NULL;
			(*errorString) = "Internal Error";
			return UPNP_E_INTERNAL_ERROR;
		}
		return UPNP_E_SUCCESS;
	} else {
		(*errorString) = "Internal Error";
		return UPNP_E_INTERNAL_ERROR;
	}
	in = in;
}

int TvDeviceIncreaseVolume(IXML_Document * in, IXML_Document ** out,
			   const char **errorString)
{
	return IncrementVolume(1, in, out, errorString);
}

int TvDeviceDecreaseVolume(IXML_Document * in, IXML_Document ** out,
			   const char **errorString)
{
	return IncrementVolume(-1, in, out, errorString);
}

int TvDeviceSetColor(IXML_Document * in, IXML_Document ** out,
		     const char **errorString)
{
	char *value = NULL;
	int color = 0;

	(*out) = NULL;
	(*errorString) = NULL;
	if (!(value = SampleUtil_GetFirstDocumentItem(in, "Color"))) {
		(*errorString) = "Invalid Color";
		return UPNP_E_INVALID_PARAM;
	}
	color = atoi(value);
	if (color < MIN_COLOR || color > MAX_COLOR) {
		SampleUtil_Print("error: can't change to color %d\n", color);
		(*errorString) = "Invalid Color";
		return UPNP_E_INVALID_PARAM;
	}
	/* Vendor-specific code to set the volume goes here. */
	if (TvDeviceSetServiceTableVar(TV_SERVICE_PICTURE,
				       TV_PICTURE_COLOR, value)) {
		if (UpnpAddToActionResponse(out, "SetColor",
					    TvServiceType[TV_SERVICE_PICTURE],
					    "NewColor",
					    value) != UPNP_E_SUCCESS) {
			(*out) = NULL;
			(*errorString) = "Internal Error";
			free(value);
			return UPNP_E_INTERNAL_ERROR;
		}
		free(value);
		return UPNP_E_SUCCESS;
	} else {
		free(value);
		(*errorString) = "Internal Error";
		return UPNP_E_INTERNAL_ERROR;
	}
}

/*!
 * \brief Increment the color. Read the current color from the state
 * table, add the increment, and then change the color.
 */
static int IncrementColor(
	/*! [in] The increment by which to change the volume. */
	int incr,
	/*! [in] Action request document. */
	IXML_Document * in,
	/*! [out] Action result document. */
	IXML_Document ** out,
	/*! [out] Error string in case action was unsuccessful. */
	const char **errorString)
{
	int curcolor;
	int newcolor;
	const char *actionName;
	char value[TV_MAX_VAL_LEN];

	if (incr > 0) {
		actionName = "IncreaseColor";
	} else {
		actionName = "DecreaseColor";
	}

	ithread_mutex_lock(&TVDevMutex);
	curcolor =
	    atoi(tv_service_table[TV_SERVICE_PICTURE].VariableStrVal
		 [TV_PICTURE_COLOR]);
	ithread_mutex_unlock(&TVDevMutex);

	newcolor = curcolor + incr;
	if (newcolor < MIN_COLOR || newcolor > MAX_COLOR) {
		SampleUtil_Print("error: can't change to color %d\n", newcolor);
		(*errorString) = "Invalid Color";
		return UPNP_E_INVALID_PARAM;
	}
	/* Vendor-specific code to set the volume goes here. */
	sprintf(value, "%d", newcolor);
	if (TvDeviceSetServiceTableVar(TV_SERVICE_PICTURE,
				       TV_PICTURE_COLOR, value)) {
		if (UpnpAddToActionResponse(out, actionName,
					    TvServiceType[TV_SERVICE_PICTURE],
					    "Color", value) != UPNP_E_SUCCESS) {
			(*out) = NULL;
			(*errorString) = "Internal Error";
			return UPNP_E_INTERNAL_ERROR;
		}
		return UPNP_E_SUCCESS;
	} else {
		(*errorString) = "Internal Error";
		return UPNP_E_INTERNAL_ERROR;
	}
	in = in;
}

int TvDeviceDecreaseColor(IXML_Document * in, IXML_Document ** out,
			  const char **errorString)
{
	return IncrementColor(-1, in, out, errorString);
}

int TvDeviceIncreaseColor(IXML_Document * in, IXML_Document ** out,
			  const char **errorString)
{
	return IncrementColor(1, in, out, errorString);
}

int TvDeviceSetTint(IXML_Document * in, IXML_Document ** out,
		    const char **errorString)
{
	char *value = NULL;
	int tint = -1;

	(*out) = NULL;
	(*errorString) = NULL;
	if (!(value = SampleUtil_GetFirstDocumentItem(in, "Tint"))) {
		(*errorString) = "Invalid Tint";
		return UPNP_E_INVALID_PARAM;
	}
	tint = atoi(value);
	if (tint < MIN_TINT || tint > MAX_TINT) {
		SampleUtil_Print("error: can't change to tint %d\n", tint);
		(*errorString) = "Invalid Tint";
		return UPNP_E_INVALID_PARAM;
	}
	/* Vendor-specific code to set the volume goes here. */
	if (TvDeviceSetServiceTableVar(TV_SERVICE_PICTURE,
				       TV_PICTURE_TINT, value)) {
		if (UpnpAddToActionResponse(out, "SetTint",
					    TvServiceType[TV_SERVICE_PICTURE],
					    "NewTint", value) != UPNP_E_SUCCESS)
		{
			(*out) = NULL;
			(*errorString) = "Internal Error";
			free(value);
			return UPNP_E_INTERNAL_ERROR;
		}
		free(value);
		return UPNP_E_SUCCESS;
	} else {
		free(value);
		(*errorString) = "Internal Error";
		return UPNP_E_INTERNAL_ERROR;
	}
}

/******************************************************************************
 * IncrementTint
 *
 * Description: 
 *       Increment the tint.  Read the current tint from the state
 *       table, add the increment, and then change the tint.
 *
 * Parameters:
 *   incr -- The increment by which to change the tint.
 *   
 *    IXML_Document * in -  action request document
 *    IXML_Document **out - action result document
 *    char **errorString - errorString (in case action was unsuccessful)
 *****************************************************************************/
int IncrementTint(IN int incr, IN IXML_Document *in, OUT IXML_Document **out,
		  OUT const char **errorString)
{
	int curtint;
	int newtint;
	const char *actionName = NULL;
	char value[TV_MAX_VAL_LEN];

	if (incr > 0) {
		actionName = "IncreaseTint";
	} else {
		actionName = "DecreaseTint";
	}

	ithread_mutex_lock(&TVDevMutex);
	curtint =
	    atoi(tv_service_table[TV_SERVICE_PICTURE].VariableStrVal
		 [TV_PICTURE_TINT]);
	ithread_mutex_unlock(&TVDevMutex);

	newtint = curtint + incr;
	if (newtint < MIN_TINT || newtint > MAX_TINT) {
		SampleUtil_Print("error: can't change to tint %d\n", newtint);
		(*errorString) = "Invalid Tint";
		return UPNP_E_INVALID_PARAM;
	}
	/* Vendor-specific code to set the volume goes here. */
	sprintf(value, "%d", newtint);
	if (TvDeviceSetServiceTableVar(TV_SERVICE_PICTURE,
				       TV_PICTURE_TINT, value)) {
		if (UpnpAddToActionResponse(out, actionName,
					    TvServiceType[TV_SERVICE_PICTURE],
					    "Tint", value) != UPNP_E_SUCCESS) {
			(*out) = NULL;
			(*errorString) = "Internal Error";
			return UPNP_E_INTERNAL_ERROR;
		}
		return UPNP_E_SUCCESS;
	} else {
		(*errorString) = "Internal Error";
		return UPNP_E_INTERNAL_ERROR;
	}
	in = in;
}

/******************************************************************************
 * TvDeviceIncreaseTint
 *
 * Description: 
 *       Increase tint.
 *
 * Parameters:
 *   
 *    IXML_Document * in -  action request document
 *    IXML_Document **out - action result document
 *    char **errorString - errorString (in case action was unsuccessful)
 *
 *****************************************************************************/
int TvDeviceIncreaseTint(IN IXML_Document *in, OUT IXML_Document **out,
	OUT const char **errorString)
{
	return IncrementTint(1, in, out, errorString);
}

/******************************************************************************
 * TvDeviceDecreaseTint
 *
 * Description: 
 *       Decrease tint.
 *
 * Parameters:
 *  
 *    IXML_Document * in -  action request document
 *    IXML_Document **out - action result document
 *    char **errorString - errorString (in case action was unsuccessful)
 *
 *****************************************************************************/
int TvDeviceDecreaseTint(IN IXML_Document *in, OUT IXML_Document **out,
	OUT const char **errorString)
{
	return IncrementTint(-1, in, out, errorString);
}

/*****************************************************************************
 * TvDeviceSetContrast
 *
 * Description: 
 *       Change the contrast, update the TvDevice picture service
 *       state table, and notify all subscribed control points of the
 *       updated state.
 *
 * Parameters:
 *   
 *    IXML_Document * in -  action request document
 *    IXML_Document **out - action result document
 *    char **errorString - errorString (in case action was unsuccessful)
 *
 ****************************************************************************/
int TvDeviceSetContrast(IN IXML_Document *in, OUT IXML_Document **out,
	OUT const char **errorString)
{
	char *value = NULL;
	int contrast = -1;

	(*out) = NULL;
	(*errorString) = NULL;

	if (!(value = SampleUtil_GetFirstDocumentItem(in, "Contrast"))) {
		(*errorString) = "Invalid Contrast";
		return UPNP_E_INVALID_PARAM;
	}
	contrast = atoi(value);
	if (contrast < MIN_CONTRAST || contrast > MAX_CONTRAST) {
		SampleUtil_Print("error: can't change to contrast %d\n",
				 contrast);
		(*errorString) = "Invalid Contrast";
		return UPNP_E_INVALID_PARAM;
	}
	/* Vendor-specific code to set the volume goes here. */
	if (TvDeviceSetServiceTableVar(TV_SERVICE_PICTURE,
				       TV_PICTURE_CONTRAST, value)) {
		if (UpnpAddToActionResponse(out, "SetContrast",
					    TvServiceType[TV_SERVICE_PICTURE],
					    "NewContrast",
					    value) != UPNP_E_SUCCESS) {
			(*out) = NULL;
			(*errorString) = "Internal Error";
			free(value);
			return UPNP_E_INTERNAL_ERROR;
		}
		free(value);
		return UPNP_E_SUCCESS;
	} else {
		free(value);
		(*errorString) = "Internal Error";
		return UPNP_E_INTERNAL_ERROR;
	}

}

/*!
 * \brief Increment the contrast.  Read the current contrast from the state
 * table, add the increment, and then change the contrast.
 */
static int IncrementContrast(
	/*! [in] The increment by which to change the volume. */
	int incr,
	/*! [in] Action request document. */
	IXML_Document * in,
	/*! [out] Action result document. */
	IXML_Document ** out,
	/*! [out] Error string in case action was unsuccessful. */
	const char **errorString)
{
	int curcontrast;
	int newcontrast;
	const char *actionName = NULL;
	char value[TV_MAX_VAL_LEN];

	if (incr > 0) {
		actionName = "IncreaseContrast";
	} else {
		actionName = "DecreaseContrast";
	}

	ithread_mutex_lock(&TVDevMutex);
	curcontrast =
	    atoi(tv_service_table[TV_SERVICE_PICTURE].VariableStrVal
		 [TV_PICTURE_CONTRAST]);
	ithread_mutex_unlock(&TVDevMutex);

	newcontrast = curcontrast + incr;
	if (newcontrast < MIN_CONTRAST || newcontrast > MAX_CONTRAST) {
		SampleUtil_Print("error: can't change to contrast %d\n",
				 newcontrast);
		(*errorString) = "Invalid Contrast";
		return UPNP_E_INVALID_PARAM;
	}
	/* Vendor-specific code to set the channel goes here. */
	sprintf(value, "%d", newcontrast);
	if (TvDeviceSetServiceTableVar(TV_SERVICE_PICTURE,
				       TV_PICTURE_CONTRAST, value)) {
		if (UpnpAddToActionResponse(out, actionName,
					    TvServiceType[TV_SERVICE_PICTURE],
					    "Contrast",
					    value) != UPNP_E_SUCCESS) {
			(*out) = NULL;
			(*errorString) = "Internal Error";
			return UPNP_E_INTERNAL_ERROR;
		}
		return UPNP_E_SUCCESS;
	} else {
		(*errorString) = "Internal Error";
		return UPNP_E_INTERNAL_ERROR;
	}
	in = in;
}

int TvDeviceIncreaseContrast(IXML_Document * in, IXML_Document ** out,
			     const char **errorString)
{
	return IncrementContrast(1, in, out, errorString);
}

int TvDeviceDecreaseContrast(IXML_Document * in, IXML_Document ** out,
			     const char **errorString)
{
	return IncrementContrast(-1, in, out, errorString);
}

int TvDeviceSetBrightness(IXML_Document * in, IXML_Document ** out,
			  const char **errorString)
{
	char *value = NULL;
	int brightness = -1;

	(*out) = NULL;
	(*errorString) = NULL;
	if (!(value = SampleUtil_GetFirstDocumentItem(in, "Brightness"))) {
		(*errorString) = "Invalid Brightness";
		return UPNP_E_INVALID_PARAM;
	}
	brightness = atoi(value);
	if (brightness < MIN_BRIGHTNESS || brightness > MAX_BRIGHTNESS) {
		SampleUtil_Print("error: can't change to brightness %d\n",
				 brightness);
		(*errorString) = "Invalid Brightness";
		return UPNP_E_INVALID_PARAM;
	}
	/* Vendor-specific code to set the volume goes here. */
	if (TvDeviceSetServiceTableVar(TV_SERVICE_PICTURE,
				       TV_PICTURE_BRIGHTNESS, value)) {
		if (UpnpAddToActionResponse(out, "SetBrightness",
					    TvServiceType[TV_SERVICE_PICTURE],
					    "NewBrightness",
					    value) != UPNP_E_SUCCESS) {
			(*out) = NULL;
			(*errorString) = "Internal Error";
			free(value);
			return UPNP_E_INTERNAL_ERROR;
		}
		free(value);
		return UPNP_E_SUCCESS;
	} else {
		free(value);
		(*errorString) = "Internal Error";
		return UPNP_E_INTERNAL_ERROR;
	}
}

/*!
 * \brief Increment the brightness. Read the current brightness from the state
 * table, add the increment, and then change the brightness.
 */
static int IncrementBrightness(
	/*! [in] The increment by which to change the brightness. */
	int incr,
	/*! [in] action request document. */
	IXML_Document * in,
	/*! [out] action result document. */
	IXML_Document ** out,
	/*! [out] errorString (in case action was unsuccessful). */
	const char **errorString)
{
	int curbrightness;
	int newbrightness;
	const char *actionName = NULL;
	char value[TV_MAX_VAL_LEN];

	if (incr > 0) {
		actionName = "IncreaseBrightness";
	} else {
		actionName = "DecreaseBrightness";
	}

	ithread_mutex_lock(&TVDevMutex);
	curbrightness =
	    atoi(tv_service_table[TV_SERVICE_PICTURE].VariableStrVal
		 [TV_PICTURE_BRIGHTNESS]);
	ithread_mutex_unlock(&TVDevMutex);

	newbrightness = curbrightness + incr;
	if (newbrightness < MIN_BRIGHTNESS || newbrightness > MAX_BRIGHTNESS) {
		SampleUtil_Print("error: can't change to brightness %d\n",
				 newbrightness);
		(*errorString) = "Invalid Brightness";
		return UPNP_E_INVALID_PARAM;
	}
	/* Vendor-specific code to set the channel goes here. */
	sprintf(value, "%d", newbrightness);
	if (TvDeviceSetServiceTableVar(TV_SERVICE_PICTURE,
				       TV_PICTURE_BRIGHTNESS, value)) {
		if (UpnpAddToActionResponse(out, actionName,
					    TvServiceType[TV_SERVICE_PICTURE],
					    "Brightness",
					    value) != UPNP_E_SUCCESS) {
			(*out) = NULL;
			(*errorString) = "Internal Error";
			return UPNP_E_INTERNAL_ERROR;
		}
		return UPNP_E_SUCCESS;
	} else {
		(*errorString) = "Internal Error";
		return UPNP_E_INTERNAL_ERROR;
	}
	in = in;
}

int TvDeviceIncreaseBrightness(IXML_Document * in, IXML_Document ** out,
			       const char **errorString)
{
	return IncrementBrightness(1, in, out, errorString);
}

int TvDeviceDecreaseBrightness(IXML_Document * in, IXML_Document ** out,
			       const char **errorString)
{
	return IncrementBrightness(-1, in, out, errorString);
}

int TvDeviceCallbackEventHandler(Upnp_EventType EventType, const void *Event, void *Cookie)
{
	switch (EventType) {
	case UPNP_EVENT_SUBSCRIPTION_REQUEST:
		TvDeviceHandleSubscriptionRequest((UpnpSubscriptionRequest *)Event);
		break;
	case UPNP_CONTROL_GET_VAR_REQUEST:
		TvDeviceHandleGetVarRequest((UpnpStateVarRequest *)Event);
		break;
	case UPNP_CONTROL_ACTION_REQUEST:
		TvDeviceHandleActionRequest((UpnpActionRequest *)Event);
		break;
		/* ignore these cases, since this is not a control point */
	case UPNP_DISCOVERY_ADVERTISEMENT_ALIVE:
	case UPNP_DISCOVERY_SEARCH_RESULT:
	case UPNP_DISCOVERY_SEARCH_TIMEOUT:
	case UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE:
	case UPNP_CONTROL_ACTION_COMPLETE:
	case UPNP_CONTROL_GET_VAR_COMPLETE:
	case UPNP_EVENT_RECEIVED:
	case UPNP_EVENT_RENEWAL_COMPLETE:
	case UPNP_EVENT_SUBSCRIBE_COMPLETE:
	case UPNP_EVENT_UNSUBSCRIBE_COMPLETE:
		break;
	default:
		SampleUtil_Print
		    ("Error in TvDeviceCallbackEventHandler: unknown event type %d\n",
		     EventType);
	}
	/* Print a summary of the event received */
	SampleUtil_PrintEvent(EventType, Event);

	return 0;
	Cookie = Cookie;
}

int TvDeviceStart(char *ip_address, unsigned short port,
		  const char *desc_doc_name, const char *web_dir_path,
		  print_string pfun, int combo)
{
	int ret = UPNP_E_SUCCESS;
	char desc_doc_url[DESC_URL_SIZE];

	ithread_mutex_init(&TVDevMutex, NULL);

	SampleUtil_Initialize(pfun);
	SampleUtil_Print("Initializing UPnP Sdk with\n"
			 "\tipaddress = %s port = %u\n",
			 ip_address ? ip_address : "{NULL}", port);
	ret = UpnpInit(ip_address, port);
	if (ret != UPNP_E_SUCCESS) {
		SampleUtil_Print("Error with UpnpInit -- %d\n", ret);
		UpnpFinish();

		return ret;
	}
	ip_address = UpnpGetServerIpAddress();
	port = UpnpGetServerPort();
	SampleUtil_Print("UPnP Initialized\n"
			 "\tipaddress = %s port = %u\n",
			 ip_address ? ip_address : "{NULL}", port);
	if (!desc_doc_name) {
		if (combo) {
			desc_doc_name = "tvcombodesc.xml";
		} else {
			desc_doc_name = "tvdevicedesc.xml";
		}
	}
	if (!web_dir_path) {
		web_dir_path = DEFAULT_WEB_DIR;
	}
	snprintf(desc_doc_url, DESC_URL_SIZE, "http://%s:%d/%s", ip_address,
		 port, desc_doc_name);
	SampleUtil_Print("Specifying the webserver root directory -- %s\n",
			 web_dir_path);
	ret = UpnpSetWebServerRootDir(web_dir_path);
	if (ret != UPNP_E_SUCCESS) {
		SampleUtil_Print
		    ("Error specifying webserver root directory -- %s: %d\n",
		     web_dir_path, ret);
		UpnpFinish();

		return ret;
	}
	SampleUtil_Print("Registering the RootDevice\n"
			 "\t with desc_doc_url: %s\n", desc_doc_url);
	ret = UpnpRegisterRootDevice(desc_doc_url, TvDeviceCallbackEventHandler,
				     &device_handle, &device_handle);
	if (ret != UPNP_E_SUCCESS) {
		SampleUtil_Print("Error registering the rootdevice : %d\n",
				 ret);
		UpnpFinish();

		return ret;
	} else {
		SampleUtil_Print("RootDevice Registered\n"
				 "Initializing State Table\n");
		TvDeviceStateTableInit(desc_doc_url);
		SampleUtil_Print("State Table Initialized\n");
		ret = UpnpSendAdvertisement(device_handle, default_advr_expire);
		if (ret != UPNP_E_SUCCESS) {
			SampleUtil_Print("Error sending advertisements : %d\n",
					 ret);
			UpnpFinish();

			return ret;
		}
		SampleUtil_Print("Advertisements Sent\n");
	}

	return UPNP_E_SUCCESS;
}

int TvDeviceStop(void)
{
	UpnpUnRegisterRootDevice(device_handle);
	UpnpFinish();
	SampleUtil_Finish();
	ithread_mutex_destroy(&TVDevMutex);

	return UPNP_E_SUCCESS;
}

void *TvDeviceCommandLoop(void *args)
{
	int stoploop = 0;
	char cmdline[100];
	char cmd[100];

	while (!stoploop) {
		sprintf(cmdline, " ");
		sprintf(cmd, " ");
		SampleUtil_Print("\n>> ");
		/* Get a command line */
		fgets(cmdline, 100, stdin);
		sscanf(cmdline, "%s", cmd);
		if (strcasecmp(cmd, "exit") == 0) {
			SampleUtil_Print("Shutting down...\n");
			TvDeviceStop();
			exit(0);
		} else {
			SampleUtil_Print("\n   Unknown command: %s\n\n", cmd);
			SampleUtil_Print("   Valid Commands:\n"
					 "     Exit\n\n");
		}
	}

	return NULL;
	args = args;
}

int device_main(int argc, char *argv[])
{
	unsigned int portTemp = 0;
	char *ip_address = NULL;
	char *desc_doc_name = NULL;
	char *web_dir_path = NULL;
	unsigned short port = 0;
	int i = 0;

	SampleUtil_Initialize(linux_print);
	/* Parse options */
	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-ip") == 0) {
			ip_address = argv[++i];
		} else if (strcmp(argv[i], "-port") == 0) {
			sscanf(argv[++i], "%u", &portTemp);
		} else if (strcmp(argv[i], "-desc") == 0) {
			desc_doc_name = argv[++i];
		} else if (strcmp(argv[i], "-webdir") == 0) {
			web_dir_path = argv[++i];
		} else if (strcmp(argv[i], "-help") == 0) {
			SampleUtil_Print("Usage: %s -ip ipaddress -port port"
					 " -desc desc_doc_name -webdir web_dir_path"
					 " -help (this message)\n", argv[0]);
			SampleUtil_Print
			    ("\tipaddress:     IP address of the device"
			     " (must match desc. doc)\n"
			     "\t\te.g.: 192.168.0.4\n"
			     "\tport:          Port number to use for"
			     " receiving UPnP messages (must match desc. doc)\n"
			     "\t\te.g.: 5431\n"
			     "\tdesc_doc_name: name of device description document\n"
			     "\t\te.g.: tvdevicedesc.xml\n"
			     "\tweb_dir_path: Filesystem path where web files"
			     " related to the device are stored\n"
			     "\t\te.g.: /upnp/sample/tvdevice/web\n");
			return 1;
		}
	}
	port = (unsigned short)portTemp;
	return TvDeviceStart(ip_address, port, desc_doc_name, web_dir_path,
			     linux_print, 0);
}

/*! @} Device Sample Module */

/*! @} UpnpSamples */
