

#ifndef STATEVARREQUEST_H
#define STATEVARREQUEST_H


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** Represents the request for current value of a state variable in a service
 *  state table.  */
typedef struct {} UpnpStateVarRequest;


#include "String.h"     // for UpnpString


#include "ixml.h"       // for DOMString


#include <netinet/in.h> // for in_addr


/** Constructor */
UpnpStateVarRequest *UpnpStateVarRequest_new();

/** Destructor */
void UpnpStateVarRequest_delete(UpnpStateVarRequest *p);

/** Copy Constructor */
UpnpStateVarRequest *UpnpStateVarRequest_dup(const UpnpStateVarRequest *p);

/** Assignment operator */
void UpnpStateVarRequest_assign(UpnpStateVarRequest *q, const UpnpStateVarRequest *p);

/** The result of the operation */
int UpnpStateVarRequest_get_ErrCode(const UpnpStateVarRequest *p);
void UpnpStateVarRequest_set_ErrCode(UpnpStateVarRequest *p, int n);

/** The socket number of the connection to the requestor */
int UpnpStateVarRequest_get_Socket(const UpnpStateVarRequest *p);
void UpnpStateVarRequest_set_Socket(UpnpStateVarRequest *p, int n);

/** The error string in case of error */
const UpnpString *UpnpStateVarRequest_get_ErrStr(const UpnpStateVarRequest *p);
void UpnpStateVarRequest_set_ErrStr(UpnpStateVarRequest *p, const UpnpString *s);
void UpnpStateVarRequest_strcpy_ErrStr(UpnpStateVarRequest *p, const char *s);

/** The unique device ID */
const UpnpString *UpnpStateVarRequest_get_DevUDN(const UpnpStateVarRequest *p);
void UpnpStateVarRequest_set_DevUDN(UpnpStateVarRequest *p, const UpnpString *s);

/** The service ID */
const UpnpString *UpnpStateVarRequest_get_ServiceID(const UpnpStateVarRequest *p);
void UpnpStateVarRequest_set_ServiceID(UpnpStateVarRequest *p, const UpnpString *s);

/** The name of the variable. */
const UpnpString *UpnpStateVarRequest_get_StateVarName(const UpnpStateVarRequest *p);
void UpnpStateVarRequest_set_StateVarName(UpnpStateVarRequest *p, const UpnpString *s);
void UpnpStateVarRequest_strcpy_StateVarName(UpnpStateVarRequest *p, const char *s);

/** IP address of sender requesting the state variable. */
struct sockaddr_storage *UpnpStateVarRequest_get_CtrlPtIPAddr(const UpnpStateVarRequest *p);
void UpnpStateVarRequest_set_CtrlPtIPAddr(UpnpStateVarRequest *p, struct sockaddr_storage *ia);

/** The current value of the variable. This needs to be allocated by 
 *  the caller.  When finished with it, the SDK frees this {\bf DOMString}. */
const DOMString UpnpStateVarRequest_get_CurrentVal(const UpnpStateVarRequest *p);
void UpnpStateVarRequest_set_CurrentVal(UpnpStateVarRequest *p, const DOMString s);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* STATEVARREQUEST_H */

