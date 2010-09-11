
#ifndef EVENTSUBSCRIBE_H
#define EVENTSUBSCRIBE_H


/*!
 * \file
 *
 * \brief UpnpEventSubscribe object declararion.
 *
 * Returned along with a \b UPNP_EVENT_SUBSCRIBE_COMPLETE or
 * \b UPNP_EVENT_UNSUBSCRIBE_COMPLETE callback.
 *
 * \author Marcelo Roberto Jimenez
 */

#define CLASS UpnpEventSubscribe

#define EXPAND_CLASS_MEMBERS(CLASS) \
	EXPAND_CLASS_MEMBER_INT(CLASS, ErrCode, int) \
	EXPAND_CLASS_MEMBER_INT(CLASS, TimeOut, int) \
	EXPAND_CLASS_MEMBER_STRING(CLASS, SID) \
	EXPAND_CLASS_MEMBER_STRING(CLASS, PublisherUrl) \

#include "TemplateInclude.h"


#endif /* EVENTSUBSCRIBE_H */

