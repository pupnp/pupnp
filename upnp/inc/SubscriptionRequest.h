

#ifndef SUBSCRIPTIONREQUEST_H
#define SUBSCRIPTIONREQUEST_H


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** Returned along with a \b UPNP_EVENT_SUBSCRIPTION_REQUEST callback. */
typedef struct {} UpnpSubscriptionRequest;


#include "UpnpString.h"


/** Constructor */
UpnpSubscriptionRequest *UpnpSubscriptionRequest_new();

/** Destructor */
void UpnpSubscriptionRequest_delete(UpnpSubscriptionRequest *p);

/** Copy Constructor */
UpnpSubscriptionRequest *UpnpSubscriptionRequest_dup(const UpnpSubscriptionRequest *p);

/** Assignment operator */
void UpnpSubscriptionRequest_assign(UpnpSubscriptionRequest *q, const UpnpSubscriptionRequest *p);

/** The identifier for the service being subscribed to. */
const UpnpString *UpnpSubscriptionRequest_get_ServiceId(const UpnpSubscriptionRequest *p);
void UpnpSubscriptionRequest_set_ServiceId(UpnpSubscriptionRequest *p, const UpnpString *s);
void UpnpSubscriptionRequest_strcpy_ServiceId(UpnpSubscriptionRequest *p, const char *s);

/** Universal device name. */
const UpnpString *UpnpSubscriptionRequest_get_UDN(const UpnpSubscriptionRequest *p);
void UpnpSubscriptionRequest_set_UDN(UpnpSubscriptionRequest *p, const UpnpString *s);
void UpnpSubscriptionRequest_strcpy_UDN(UpnpSubscriptionRequest *p, const char *s);

/** The assigned subscription ID for this subscription. */
const UpnpString *UpnpSubscriptionRequest_get_SID(const UpnpSubscriptionRequest *p);
void UpnpSubscriptionRequest_set_SID(UpnpSubscriptionRequest *p, const UpnpString *s);
void UpnpSubscriptionRequest_strcpy_SID(UpnpSubscriptionRequest *p, const char *s);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* SUBSCRIPTIONREQUEST_H */

