

#ifndef ACTIONREQUEST_H
#define ACTIONREQUEST_H


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** Returned as part of a \b UPNP_CONTROL_ACTION_COMPLETE callback.  */
typedef struct s_UpnpActionRequest UpnpActionRequest;


#include "ixml.h"       // for IXML_Document
#include "UpnpString.h"


#include <netinet/in.h> // for in_addr


/** Constructor */
UpnpActionRequest *UpnpActionRequest_new();

/** Destructor */
void UpnpActionRequest_delete(UpnpActionRequest *p);

/** Copy Constructor */
UpnpActionRequest *UpnpActionRequest_dup(const UpnpActionRequest *p);

/** Assignment operator */
void UpnpActionRequest_assign(UpnpActionRequest *q, const UpnpActionRequest *p);

/** The result of the operation */
int UpnpActionRequest_get_ErrCode(const UpnpActionRequest *p);
void UpnpActionRequest_set_ErrCode(UpnpActionRequest *p, int n);

/** The socket number of the connection to the requestor */
int UpnpActionRequest_get_Socket(const UpnpActionRequest *p);
void UpnpActionRequest_set_Socket(UpnpActionRequest *p, int n);

/** The error string in case of error */
const UpnpString *UpnpActionRequest_get_ErrStr(const UpnpActionRequest *p);
void UpnpActionRequest_set_ErrStr(UpnpActionRequest *p, const UpnpString *s);
void UpnpActionRequest_strcpy_ErrStr(UpnpActionRequest *p, const char *s);

/** The Action Name */
const UpnpString *UpnpActionRequest_get_ActionName(const UpnpActionRequest *p);
void UpnpActionRequest_set_ActionName(UpnpActionRequest *p, const UpnpString *s);
void UpnpActionRequest_strcpy_ActionName(UpnpActionRequest *p, const char *s);

/** The unique device ID */
const UpnpString *UpnpActionRequest_get_DevUDN(const UpnpActionRequest *p);
void UpnpActionRequest_set_DevUDN(UpnpActionRequest *p, const UpnpString *s);

/** The service ID */
const UpnpString *UpnpActionRequest_get_ServiceID(const UpnpActionRequest *p);
void UpnpActionRequest_set_ServiceID(UpnpActionRequest *p, const UpnpString *s);

/** The DOM document describing the action */
IXML_Document *UpnpActionRequest_get_ActionRequest(const UpnpActionRequest *p);
void UpnpActionRequest_set_ActionRequest(UpnpActionRequest *p, IXML_Document *d);

/** The DOM document describing the result of the action */
IXML_Document *UpnpActionRequest_get_ActionResult(const UpnpActionRequest *p);
void UpnpActionRequest_set_ActionResult(UpnpActionRequest *p, IXML_Document *d);

/** IP address of the control point requesting this action */
struct sockaddr_storage *UpnpActionRequest_get_CtrlPtIPAddr(const UpnpActionRequest *p);
void UpnpActionRequest_set_CtrlPtIPAddr(UpnpActionRequest *p, struct sockaddr_storage *ia);

/** The DOM document containing the information from the SOAP header */
IXML_Document *UpnpActionRequest_get_SoapHeader(const UpnpActionRequest *p);
void UpnpActionRequest_set_SoapHeader(UpnpActionRequest *p, IXML_Document *d);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* ACTIONREQUEST_H */

