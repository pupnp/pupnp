

#ifndef STATEVARREQUEST_H
#define STATEVARREQUEST_H


/*!
 * \file
 *
 * \brief UpnpStateVarRequest object declararion.
 *
 * \author Marcelo Roberto Jimenez
 *
 */


#include "ixml.h"       /* for DOMString */
#include "UpnpGlobal.h" /* for EXPORT_SPEC */
#include "UpnpInet.h"   /* for sockaddr, sockaddr_storage */
#include "UpnpString.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** Represents the request for current value of a state variable in a service
 *  state table.  */
typedef struct s_UpnpStateVarRequest UpnpStateVarRequest;


/** Constructor */
EXPORT_SPEC UpnpStateVarRequest *UpnpStateVarRequest_new();

/** Destructor */
EXPORT_SPEC void UpnpStateVarRequest_delete(UpnpStateVarRequest *p);

/** Copy Constructor */
EXPORT_SPEC UpnpStateVarRequest *UpnpStateVarRequest_dup(const UpnpStateVarRequest *p);

/** Assignment operator */
EXPORT_SPEC void UpnpStateVarRequest_assign(UpnpStateVarRequest *q, const UpnpStateVarRequest *p);

/** The result of the operation */
EXPORT_SPEC int UpnpStateVarRequest_get_ErrCode(const UpnpStateVarRequest *p);
EXPORT_SPEC void UpnpStateVarRequest_set_ErrCode(UpnpStateVarRequest *p, int n);

/** The socket number of the connection to the requestor */
EXPORT_SPEC int UpnpStateVarRequest_get_Socket(const UpnpStateVarRequest *p);
EXPORT_SPEC void UpnpStateVarRequest_set_Socket(UpnpStateVarRequest *p, int n);

/** The error string in case of error */
EXPORT_SPEC const UpnpString *UpnpStateVarRequest_get_ErrStr(const UpnpStateVarRequest *p);
EXPORT_SPEC void UpnpStateVarRequest_set_ErrStr(UpnpStateVarRequest *p, const UpnpString *s);
EXPORT_SPEC void UpnpStateVarRequest_strcpy_ErrStr(UpnpStateVarRequest *p, const char *s);

/** The unique device ID */
EXPORT_SPEC const UpnpString *UpnpStateVarRequest_get_DevUDN(const UpnpStateVarRequest *p);
EXPORT_SPEC void UpnpStateVarRequest_set_DevUDN(UpnpStateVarRequest *p, const UpnpString *s);

/** The service ID */
EXPORT_SPEC const UpnpString *UpnpStateVarRequest_get_ServiceID(const UpnpStateVarRequest *p);
EXPORT_SPEC void UpnpStateVarRequest_set_ServiceID(UpnpStateVarRequest *p, const UpnpString *s);

/** The name of the variable. */
EXPORT_SPEC const UpnpString *UpnpStateVarRequest_get_StateVarName(const UpnpStateVarRequest *p);
EXPORT_SPEC void UpnpStateVarRequest_set_StateVarName(UpnpStateVarRequest *p, const UpnpString *s);
EXPORT_SPEC void UpnpStateVarRequest_strcpy_StateVarName(UpnpStateVarRequest *p, const char *s);

/** IP address of sender requesting the state variable. */
EXPORT_SPEC struct sockaddr *UpnpStateVarRequest_get_CtrlPtIPAddr(const UpnpStateVarRequest *p);
EXPORT_SPEC void UpnpStateVarRequest_set_CtrlPtIPAddr(UpnpStateVarRequest *p, struct sockaddr *sa);

/** The current value of the variable. This needs to be allocated by 
 *  the caller.  When finished with it, the SDK frees this {\bf DOMString}. */
EXPORT_SPEC const DOMString UpnpStateVarRequest_get_CurrentVal(const UpnpStateVarRequest *p);
EXPORT_SPEC void UpnpStateVarRequest_set_CurrentVal(UpnpStateVarRequest *p, const DOMString s);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* STATEVARREQUEST_H */

