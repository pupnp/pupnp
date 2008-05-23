

#ifndef ACTIONCOMPLETE_H
#define ACTIONCOMPLETE_H


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef struct s_UpnpActionComplete UpnpActionComplete;


#include "UpnpString.h"


#include "ixml.h"       /* for IXML_Document */


/** Constructor */
UpnpActionComplete *UpnpActionComplete_new();

/** Destructor */
void UpnpActionComplete_delete(UpnpActionComplete *p);

/** Copy Constructor */
UpnpActionComplete *UpnpActionComplete_dup(const UpnpActionComplete *p);

/** Assignment operator */
void UpnpActionComplete_assign(UpnpActionComplete *q, const UpnpActionComplete *p);

/** The result of the operation */
int UpnpActionComplete_get_ErrCode(const UpnpActionComplete *p);
void UpnpActionComplete_set_ErrCode(UpnpActionComplete *p, int n);

/** The control URL for service. */
const UpnpString *UpnpActionComplete_get_CtrlUrl(const UpnpActionComplete *p);
void UpnpActionComplete_set_CtrlUrl(UpnpActionComplete *p, const UpnpString *s);
void UpnpActionComplete_strcpy_CtrlUrl(UpnpActionComplete *p, const char *s);

/** The DOM document describing the action */
IXML_Document *UpnpActionComplete_get_ActionRequest(const UpnpActionComplete *p);
void UpnpActionComplete_set_ActionRequest(UpnpActionComplete *p, IXML_Document *d);

/** The DOM document describing the result of the action */
IXML_Document *UpnpActionComplete_get_ActionResult(const UpnpActionComplete *p);
void UpnpActionComplete_set_ActionResult(UpnpActionComplete *p, IXML_Document *d);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* ACTIONCOMPLETE_H */

