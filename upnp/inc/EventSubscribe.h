

#ifndef EVENTSUBSCRIBE_H
#define EVENTSUBSCRIBE_H


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** Returned along with a {\bf UPNP_EVENT_SUBSCRIBE_COMPLETE} or
 * {\bf UPNP_EVENT_UNSUBSCRIBE_COMPLETE} callback.  */
typedef struct {} EventSubscribe;


#include "String.h"     // for UpnpString


/** Constructor */
EventSubscribe *UpnpEventSubscribe_new();

/** Destructor */
void UpnpEventSubscribe_delete(EventSubscribe *p);

/** Copy Constructor */
EventSubscribe *UpnpEventSubscribe_dup(const EventSubscribe *p);

/** Assignment operator */
void UpnpEventSubscribe_assign(EventSubscribe *q, const EventSubscribe *p);

/** The result of the operation. */
int UpnpEventSubscribe_get_ErrCode(const EventSubscribe *p);
void UpnpEventSubscribe_set_ErrCode(EventSubscribe *p, int n);

/** The actual subscription time (for subscriptions only). */
int UpnpEventSubscribe_get_TimeOut(const EventSubscribe *p);
void UpnpEventSubscribe_set_TimeOut(EventSubscribe *p, int n);

/** The SID for this subscription.  For subscriptions, this only
 *  contains a valid SID if the {\bf Upnp_EventSubscribe.result} field
 *  contains a {\tt UPNP_E_SUCCESS} result code.  For unsubscriptions,
 *  this contains the SID from which the subscription is being
 *  unsubscribed.  */
const UpnpString *UpnpEventSubscribe_get_SID(const EventSubscribe *p);
void UpnpEventSubscribe_set_SID(EventSubscribe *p, const UpnpString *s);
void UpnpEventSubscribe_strcpy_SID(EventSubscribe *p, const char *s);


/** The event URL being subscribed to or removed from. */
const UpnpString *UpnpEventSubscribe_get_PublisherUrl(const EventSubscribe *p);
void UpnpEventSubscribe_set_PublisherUrl(EventSubscribe *p, const UpnpString *s);
void UpnpEventSubscribe_strcpy_PublisherUrl(EventSubscribe *p, const char *s);



#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* EVENTSUBSCRIBE_H */

