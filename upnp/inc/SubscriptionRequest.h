

#ifndef SUBSCRIPTIONREQUEST_H
#define SUBSCRIPTIONREQUEST_H


/*!
 * \file
 *
 * \brief UpnpSubscriptionRequest object declararion.
 *
 * Returned along with a \b UPNP_EVENT_SUBSCRIPTION_REQUEST callback.
 *
 * \author Marcelo Roberto Jimenez
 */

#define CLASS UpnpSubscriptionRequest

#define EXPAND_CLASS_MEMBERS(CLASS) \
	EXPAND_CLASS_MEMBER_STRING(CLASS, ServiceId) \
	EXPAND_CLASS_MEMBER_STRING(CLASS, UDN) \
	EXPAND_CLASS_MEMBER_STRING(CLASS, SID) \

#include "TemplateInclude.h"


#endif /* SUBSCRIPTIONREQUEST_H */

