

#ifndef EVENT_H
#define EVENT_H


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** Returned along with a \b UPNP_EVENT_RECEIVED callback.  */
typedef struct s_UpnpEvent UpnpEvent;


#include "ixml.h"       /* for IXML_Document */
#include "UpnpString.h"


/** Constructor */
UpnpEvent *UpnpEvent_new();

/** Destructor */
void UpnpEvent_delete(UpnpEvent *p);

/** Copy Constructor */
UpnpEvent *UpnpEvent_dup(const UpnpEvent *p);

/** Assignment operator */
void UpnpEvent_assign(UpnpEvent *q, const UpnpEvent *p);

/** The event sequence number. */
int UpnpEvent_get_EventKey(const UpnpEvent *p);
void UpnpEvent_set_EventKey(UpnpEvent *p, int n);

/** The DOM tree representing the changes generating the event. */
IXML_Document *UpnpEvent_get_ChangedVariables(const UpnpEvent *p);
void UpnpEvent_set_ChangedVariables(UpnpEvent *p, IXML_Document *d);

/** The subscription ID for this subscription. */
UpnpString *UpnpEvent_get_SID(const UpnpEvent *p);
void UpnpEvent_set_SID(UpnpEvent *p, const UpnpString *s);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* EVENT_H */

