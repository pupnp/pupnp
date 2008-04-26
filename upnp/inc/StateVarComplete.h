

#ifndef STATEVARCOMPLETE_H
#define STATEVARCOMPLETE_H


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** Represents the reply for the current value of a state variable in an
    asynchronous call. */
typedef struct {} UpnpStateVarComplete;


#include "String.h"     // for UpnpString


#include "ixml.h"       // for DOMString


/** Constructor */
UpnpStateVarComplete *UpnpStateVarComplete_new();

/** Destructor */
void UpnpStateVarComplete_delete(UpnpStateVarComplete *p);

/** Copy Constructor */
UpnpStateVarComplete *UpnpStateVarComplete_dup(const UpnpStateVarComplete *p);

/** Assignment operator */
void UpnpStateVarComplete_assign(UpnpStateVarComplete *q, const UpnpStateVarComplete *p);

/** The result of the operation */
int UpnpStateVarComplete_get_ErrCode(const UpnpStateVarComplete *p);
void UpnpStateVarComplete_set_ErrCode(UpnpStateVarComplete *p, int n);

/** The control URL for the service. */
const UpnpString *UpnpStateVarComplete_get_CtrlUrl(const UpnpStateVarComplete *p);
void UpnpStateVarComplete_set_CtrlUrl(UpnpStateVarComplete *p, const UpnpString *s);
void UpnpStateVarComplete_strcpy_CtrlUrl(UpnpStateVarComplete *p, const char *s);

/** The name of the variable. */
const UpnpString *UpnpStateVarComplete_get_StateVarName(const UpnpStateVarComplete *p);
void UpnpStateVarComplete_set_StateVarName(UpnpStateVarComplete *p, const UpnpString *s);
void UpnpStateVarComplete_strcpy_StateVarName(UpnpStateVarComplete *p, const char *s);

/** The current value of the variable. This needs to be allocated by 
 *  the caller.  When finished with it, the SDK frees this {\bf DOMString}. */
const DOMString UpnpStateVarComplete_get_CurrentVal(const UpnpStateVarComplete *p);
void UpnpStateVarComplete_set_CurrentVal(UpnpStateVarComplete *p, const DOMString s);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* STATEVARCOMPLETE_H */

