
#ifndef STATEVARCOMPLETE_H
#define STATEVARCOMPLETE_H


/*!
 * \file
 *
 * \brief UpnpStateVarComplete object declararion.
 *
 * Represents the reply for the current value of a state variable in an
 * asynchronous call.
 *
 * \author Marcelo Roberto Jimenez
 */

#define CLASS UpnpStateVarComplete

#define EXPAND_CLASS_MEMBERS(CLASS) \
	EXPAND_CLASS_MEMBER_INT(CLASS, ErrCode, int) \
	EXPAND_CLASS_MEMBER_STRING(CLASS, CtrlUrl) \
	EXPAND_CLASS_MEMBER_STRING(CLASS, StateVarName) \
	EXPAND_CLASS_MEMBER_DOMSTRING(CLASS, CurrentVal) \

#include "TemplateInclude.h"


#endif /* STATEVARCOMPLETE_H */

