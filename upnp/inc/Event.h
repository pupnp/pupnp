

#ifndef EVENT_H
#define EVENT_H


/*!
 * \file
 *
 * \brief UpnpEvent object declararion.
 *
 * \author Marcelo Roberto Jimenez
 */


/*! Returned along with a \b UPNP_EVENT_RECEIVED callback.  */
typedef struct s_UpnpEvent UpnpEvent;


#include "ixml.h"       /* for IXML_Document */
#include "UpnpGlobal.h" /* for EXPORT_SPEC */
#include "UpnpString.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*! Constructor */
EXPORT_SPEC UpnpEvent *UpnpEvent_new();

/*! Destructor */
EXPORT_SPEC void UpnpEvent_delete(UpnpEvent *p);

/*! Copy Constructor */
EXPORT_SPEC UpnpEvent *UpnpEvent_dup(const UpnpEvent *p);

/*! Assignment operator */
EXPORT_SPEC void UpnpEvent_assign(UpnpEvent *p, const UpnpEvent *q);

/*! The event sequence number. */
EXPORT_SPEC int UpnpEvent_get_EventKey(const UpnpEvent *p);
EXPORT_SPEC void UpnpEvent_set_EventKey(UpnpEvent *p, int n);

/*! The DOM tree representing the changes generating the event. */
EXPORT_SPEC IXML_Document *UpnpEvent_get_ChangedVariables(const UpnpEvent *p);
EXPORT_SPEC void UpnpEvent_set_ChangedVariables(UpnpEvent *p, IXML_Document *d);

/*! The subscription ID for this subscription. */
EXPORT_SPEC const UpnpString *UpnpEvent_get_SID(const UpnpEvent *p);
EXPORT_SPEC const char *UpnpEvent_get_SID_cstr(const UpnpEvent *p);
EXPORT_SPEC void UpnpEvent_set_SID(UpnpEvent *p, const UpnpString *s);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* EVENT_H */

