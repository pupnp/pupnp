
#ifndef DISCOVERY_H
#define DISCOVERY_H

/*!
 * \file
 *
 * \brief UpnpDiscovery object declararion.
 *
 * Returned in a \b UPNP_DISCOVERY_RESULT callback.
 *
 * \author Marcelo Roberto Jimenez
 */

#include "UpnpInet.h" /* for struct sockaddr_storage */

#define CLASS UpnpDiscovery

#define EXPAND_CLASS_MEMBERS(CLASS) \
	EXPAND_CLASS_MEMBER_INT(CLASS, ErrCode, int) \
	EXPAND_CLASS_MEMBER_INT(CLASS, Expires, int) \
	EXPAND_CLASS_MEMBER_STRING(CLASS, DeviceID) \
	EXPAND_CLASS_MEMBER_STRING(CLASS, DeviceType) \
	EXPAND_CLASS_MEMBER_STRING(CLASS, ServiceType) \
	EXPAND_CLASS_MEMBER_STRING(CLASS, ServiceVer) \
	EXPAND_CLASS_MEMBER_STRING(CLASS, Location) \
	EXPAND_CLASS_MEMBER_STRING(CLASS, Os) \
	EXPAND_CLASS_MEMBER_STRING(CLASS, Date) \
	EXPAND_CLASS_MEMBER_STRING(CLASS, Ext) \
	EXPAND_CLASS_MEMBER_BUFFER(CLASS, DestAddr, struct sockaddr_storage) \

#include "TemplateInclude.h"

#endif /* DISCOVERY_H */

