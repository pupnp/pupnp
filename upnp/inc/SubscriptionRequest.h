

#ifndef SUBSCRIPTIONREQUEST_H
#define SUBSCRIPTIONREQUEST_H


/*!
 * \file
 *
 * \brief UpnpSubscriptionRequest object declararion.
 *
 * \author Marcelo Roberto Jimenez
 *
 */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** Returned along with a \b UPNP_EVENT_SUBSCRIPTION_REQUEST callback. */
typedef struct s_UpnpSubscriptionRequest UpnpSubscriptionRequest;


#include "UpnpGlobal.h" /* for EXPORT_SPEC */
#include "UpnpString.h"


/** Constructor */
EXPORT_SPEC UpnpSubscriptionRequest *UpnpSubscriptionRequest_new();

/** Destructor */
EXPORT_SPEC void UpnpSubscriptionRequest_delete(UpnpSubscriptionRequest *p);

/** Copy Constructor */
EXPORT_SPEC UpnpSubscriptionRequest *UpnpSubscriptionRequest_dup(const UpnpSubscriptionRequest *p);

/** Assignment operator */
EXPORT_SPEC void UpnpSubscriptionRequest_assign(UpnpSubscriptionRequest *q, const UpnpSubscriptionRequest *p);

/** The identifier for the service being subscribed to. */
EXPORT_SPEC const UpnpString *UpnpSubscriptionRequest_get_ServiceId(const UpnpSubscriptionRequest *p);
EXPORT_SPEC void UpnpSubscriptionRequest_set_ServiceId(UpnpSubscriptionRequest *p, const UpnpString *s);
EXPORT_SPEC void UpnpSubscriptionRequest_strcpy_ServiceId(UpnpSubscriptionRequest *p, const char *s);

/** Universal device name. */
EXPORT_SPEC const UpnpString *UpnpSubscriptionRequest_get_UDN(const UpnpSubscriptionRequest *p);
EXPORT_SPEC void UpnpSubscriptionRequest_set_UDN(UpnpSubscriptionRequest *p, const UpnpString *s);
EXPORT_SPEC void UpnpSubscriptionRequest_strcpy_UDN(UpnpSubscriptionRequest *p, const char *s);

/** The assigned subscription ID for this subscription. */
EXPORT_SPEC const UpnpString *UpnpSubscriptionRequest_get_SID(const UpnpSubscriptionRequest *p);
EXPORT_SPEC void UpnpSubscriptionRequest_set_SID(UpnpSubscriptionRequest *p, const UpnpString *s);
EXPORT_SPEC void UpnpSubscriptionRequest_strcpy_SID(UpnpSubscriptionRequest *p, const char *s);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* SUBSCRIPTIONREQUEST_H */

