

#ifndef STATEVARCOMPLETE_H
#define STATEVARCOMPLETE_H


/*!
 * \file
 *
 * \brief UpnpStateVarComplete object declararion.
 *
 * \author Marcelo Roberto Jimenez
 *
 */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** Represents the reply for the current value of a state variable in an
    asynchronous call. */
typedef struct s_UpnpStateVarComplete UpnpStateVarComplete;


#include "UpnpGlobal.h" /* for EXPORT_SPEC */
#include "UpnpString.h"


#include "ixml.h"       /* for DOMString */


/** Constructor */
EXPORT_SPEC UpnpStateVarComplete *UpnpStateVarComplete_new();

/** Destructor */
EXPORT_SPEC void UpnpStateVarComplete_delete(UpnpStateVarComplete *p);

/** Copy Constructor */
EXPORT_SPEC UpnpStateVarComplete *UpnpStateVarComplete_dup(const UpnpStateVarComplete *p);

/** Assignment operator */
EXPORT_SPEC void UpnpStateVarComplete_assign(UpnpStateVarComplete *q, const UpnpStateVarComplete *p);

/** The result of the operation */
EXPORT_SPEC int UpnpStateVarComplete_get_ErrCode(const UpnpStateVarComplete *p);
EXPORT_SPEC void UpnpStateVarComplete_set_ErrCode(UpnpStateVarComplete *p, int n);

/** The control URL for the service. */
EXPORT_SPEC const UpnpString *UpnpStateVarComplete_get_CtrlUrl(const UpnpStateVarComplete *p);
EXPORT_SPEC void UpnpStateVarComplete_set_CtrlUrl(UpnpStateVarComplete *p, const UpnpString *s);
EXPORT_SPEC void UpnpStateVarComplete_strcpy_CtrlUrl(UpnpStateVarComplete *p, const char *s);

/** The name of the variable. */
EXPORT_SPEC const UpnpString *UpnpStateVarComplete_get_StateVarName(const UpnpStateVarComplete *p);
EXPORT_SPEC void UpnpStateVarComplete_set_StateVarName(UpnpStateVarComplete *p, const UpnpString *s);
EXPORT_SPEC void UpnpStateVarComplete_strcpy_StateVarName(UpnpStateVarComplete *p, const char *s);

/** The current value of the variable. This needs to be allocated by 
 *  the caller.  When finished with it, the SDK frees this {\bf DOMString}. */
EXPORT_SPEC const DOMString UpnpStateVarComplete_get_CurrentVal(const UpnpStateVarComplete *p);
EXPORT_SPEC void UpnpStateVarComplete_set_CurrentVal(UpnpStateVarComplete *p, const DOMString s);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* STATEVARCOMPLETE_H */

