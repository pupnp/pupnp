

#ifndef ACTIONREQUEST_H
#define ACTIONREQUEST_H


/*!
 * \file
 *
 * \brief UpnpActionRequest object declaration.
 *
 * \author Marcelo Roberto Jimenez
 *
 */


#include "ixml.h"       /* for IXML_Document */
#include "UpnpGlobal.h" /* for EXPORT_SPEC */
#include "UpnpInet.h"   /* for sockaddr, sockaddr_storage */
#include "UpnpString.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*! Returned as part of a \b UPNP_CONTROL_ACTION_COMPLETE callback.  */
typedef struct s_UpnpActionRequest UpnpActionRequest;


/*! Constructor */
EXPORT_SPEC UpnpActionRequest *UpnpActionRequest_new();

/*! Destructor */
EXPORT_SPEC void UpnpActionRequest_delete(UpnpActionRequest *p);

/*! Copy Constructor */
EXPORT_SPEC UpnpActionRequest *UpnpActionRequest_dup(const UpnpActionRequest *p);

/*! Assignment operator */
EXPORT_SPEC void UpnpActionRequest_assign(UpnpActionRequest *q, const UpnpActionRequest *p);

/*! The result of the operation */
EXPORT_SPEC int UpnpActionRequest_get_ErrCode(const UpnpActionRequest *p);
EXPORT_SPEC void UpnpActionRequest_set_ErrCode(UpnpActionRequest *p, int n);

/*! The socket number of the connection to the requestor */
EXPORT_SPEC int UpnpActionRequest_get_Socket(const UpnpActionRequest *p);
EXPORT_SPEC void UpnpActionRequest_set_Socket(UpnpActionRequest *p, int n);

/*! The error string in case of error */
EXPORT_SPEC const UpnpString *UpnpActionRequest_get_ErrStr(const UpnpActionRequest *p);
EXPORT_SPEC void UpnpActionRequest_set_ErrStr(UpnpActionRequest *p, const UpnpString *s);
EXPORT_SPEC void UpnpActionRequest_strcpy_ErrStr(UpnpActionRequest *p, const char *s);

/*! The Action Name */
EXPORT_SPEC const UpnpString *UpnpActionRequest_get_ActionName(const UpnpActionRequest *p);
EXPORT_SPEC void UpnpActionRequest_set_ActionName(UpnpActionRequest *p, const UpnpString *s);
EXPORT_SPEC void UpnpActionRequest_strcpy_ActionName(UpnpActionRequest *p, const char *s);

/*! The unique device ID */
EXPORT_SPEC const UpnpString *UpnpActionRequest_get_DevUDN(const UpnpActionRequest *p);
EXPORT_SPEC void UpnpActionRequest_set_DevUDN(UpnpActionRequest *p, const UpnpString *s);

/*! The service ID */
EXPORT_SPEC const UpnpString *UpnpActionRequest_get_ServiceID(const UpnpActionRequest *p);
EXPORT_SPEC void UpnpActionRequest_set_ServiceID(UpnpActionRequest *p, const UpnpString *s);

/*! The DOM document describing the action */
EXPORT_SPEC IXML_Document *UpnpActionRequest_get_ActionRequest(const UpnpActionRequest *p);
EXPORT_SPEC void UpnpActionRequest_set_ActionRequest(UpnpActionRequest *p, IXML_Document *d);

/*! The DOM document describing the result of the action */
EXPORT_SPEC IXML_Document *UpnpActionRequest_get_ActionResult(const UpnpActionRequest *p);
EXPORT_SPEC void UpnpActionRequest_set_ActionResult(UpnpActionRequest *p, IXML_Document *d);

/*! The DOM document containing the information from the SOAP header */
EXPORT_SPEC IXML_Document *UpnpActionRequest_get_SoapHeader(const UpnpActionRequest *p);
EXPORT_SPEC void UpnpActionRequest_set_SoapHeader(UpnpActionRequest *p, IXML_Document *d);

/*! IP address of the control point requesting this action */
EXPORT_SPEC struct sockaddr *UpnpActionRequest_get_CtrlPtIPAddr(const UpnpActionRequest *p);
EXPORT_SPEC void UpnpActionRequest_set_CtrlPtIPAddr(UpnpActionRequest *p, struct sockaddr *sa);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* ACTIONREQUEST_H */

