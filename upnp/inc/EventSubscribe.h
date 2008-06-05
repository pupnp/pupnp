

#ifndef EVENTSUBSCRIBE_H
#define EVENTSUBSCRIBE_H


/*!
 * \file
 *
 * \brief UpnpEventSubscribe object declararion.
 *
 * \author Marcelo Roberto Jimenez
 *
 */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** Returned along with a \b UPNP_EVENT_SUBSCRIBE_COMPLETE or
 * \b UPNP_EVENT_UNSUBSCRIBE_COMPLETE callback.  */
typedef struct s_UpnpEventSubscribe UpnpEventSubscribe;


#include "UpnpGlobal.h" /* for EXPORT_SPEC */
#include "UpnpString.h"


/** Constructor */
EXPORT_SPEC UpnpEventSubscribe *UpnpEventSubscribe_new();

/** Destructor */
EXPORT_SPEC void UpnpEventSubscribe_delete(UpnpEventSubscribe *p);

/** Copy Constructor */
EXPORT_SPEC UpnpEventSubscribe *UpnpEventSubscribe_dup(const UpnpEventSubscribe *p);

/** Assignment operator */
EXPORT_SPEC void UpnpEventSubscribe_assign(UpnpEventSubscribe *q, const UpnpEventSubscribe *p);

/** The result of the operation. */
EXPORT_SPEC int UpnpEventSubscribe_get_ErrCode(const UpnpEventSubscribe *p);
EXPORT_SPEC void UpnpEventSubscribe_set_ErrCode(UpnpEventSubscribe *p, int n);

/** The actual subscription time (for subscriptions only). */
EXPORT_SPEC int UpnpEventSubscribe_get_TimeOut(const UpnpEventSubscribe *p);
EXPORT_SPEC void UpnpEventSubscribe_set_TimeOut(UpnpEventSubscribe *p, int n);

/** The SID for this subscription.  For subscriptions, this only
 *  contains a valid SID if the \b Upnp_EventSubscribe.result field
 *  contains a {\tt UPNP_E_SUCCESS} result code.  For unsubscriptions,
 *  this contains the SID from which the subscription is being
 *  unsubscribed.  */
EXPORT_SPEC const UpnpString *UpnpEventSubscribe_get_SID(const UpnpEventSubscribe *p);
EXPORT_SPEC void UpnpEventSubscribe_set_SID(UpnpEventSubscribe *p, const UpnpString *s);
EXPORT_SPEC void UpnpEventSubscribe_strcpy_SID(UpnpEventSubscribe *p, const char *s);


/** The event URL being subscribed to or removed from. */
EXPORT_SPEC const UpnpString *UpnpEventSubscribe_get_PublisherUrl(const UpnpEventSubscribe *p);
EXPORT_SPEC void UpnpEventSubscribe_set_PublisherUrl(UpnpEventSubscribe *p, const UpnpString *s);
EXPORT_SPEC void UpnpEventSubscribe_strcpy_PublisherUrl(UpnpEventSubscribe *p, const char *s);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* EVENTSUBSCRIBE_H */

