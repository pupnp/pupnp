
#ifndef ACTIONREQUEST_H
#define ACTIONREQUEST_H


/*!
 * \file
 *
 * \brief UpnpActionRequest object declaration.
 *
 * Returned as part of a \b UPNP_CONTROL_ACTION_COMPLETE callback.
 *
 * \author Marcelo Roberto Jimenez
 */

#include "UpnpInet.h" /* for struct sockaddr_storage */

#define CLASS UpnpActionRequest

#define EXPAND_CLASS_MEMBERS(CLASS) \
	EXPAND_CLASS_MEMBER_INT(CLASS, ErrCode, int) \
	EXPAND_CLASS_MEMBER_INT(CLASS, Socket, int) \
	EXPAND_CLASS_MEMBER_STRING(CLASS, ErrStr) \
	EXPAND_CLASS_MEMBER_STRING(CLASS, ActionName) \
	EXPAND_CLASS_MEMBER_STRING(CLASS, DevUDN) \
	EXPAND_CLASS_MEMBER_STRING(CLASS, ServiceID) \
	EXPAND_CLASS_MEMBER_INT(CLASS, ActionRequest, IXML_Document *) \
	EXPAND_CLASS_MEMBER_INT(CLASS, ActionResult, IXML_Document *) \
	EXPAND_CLASS_MEMBER_INT(CLASS, SoapHeader, IXML_Document *) \
	EXPAND_CLASS_MEMBER_BUFFER(CLASS, CtrlPtIPAddr, struct sockaddr_storage) \

#include "TemplateInclude.h"


#endif /* ACTIONREQUEST_H */

