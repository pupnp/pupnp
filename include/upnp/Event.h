
#ifndef EVENT_H
#define EVENT_H


/*!
 * \file
 *
 * \brief UpnpEvent object declararion.
 *
 * Returned along with a \b UPNP_EVENT_RECEIVED callback.
 *
 * \author Marcelo Roberto Jimenez
 */

#define CLASS UpnpEvent

#define EXPAND_CLASS_MEMBERS(CLASS) \
	EXPAND_CLASS_MEMBER_INT(CLASS, EventKey, int) \
	EXPAND_CLASS_MEMBER_INT(CLASS, ChangedVariables, IXML_Document *) \
	EXPAND_CLASS_MEMBER_STRING(CLASS, SID) \

#include "TemplateInclude.h"


#endif /* EVENT_H */

