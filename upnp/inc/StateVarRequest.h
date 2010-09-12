
#ifndef STATEVARREQUEST_H
#define STATEVARREQUEST_H


/*!
 * \file
 *
 * \brief UpnpStateVarRequest object declararion.
 *
 * Represents the request for current value of a state variable in a service
 * state table.
 *
 * \author Marcelo Roberto Jimenez
 */

#include "UpnpInet.h" /* for struct sockaddr_storage */

#define CLASS UpnpStateVarRequest

#define EXPAND_CLASS_MEMBERS(CLASS) \
	EXPAND_CLASS_MEMBER_INT(CLASS, ErrCode, int) \
	EXPAND_CLASS_MEMBER_INT(CLASS, Socket, int) \
	EXPAND_CLASS_MEMBER_STRING(CLASS, ErrStr) \
	EXPAND_CLASS_MEMBER_STRING(CLASS, DevUDN) \
	EXPAND_CLASS_MEMBER_STRING(CLASS, ServiceID) \
	EXPAND_CLASS_MEMBER_STRING(CLASS, StateVarName) \
	EXPAND_CLASS_MEMBER_BUFFER(CLASS, CtrlPtIPAddr, struct sockaddr_storage) \
	EXPAND_CLASS_MEMBER_DOMSTRING(CLASS, CurrentVal) \

#include "TemplateInclude.h"


#endif /* STATEVARREQUEST_H */

