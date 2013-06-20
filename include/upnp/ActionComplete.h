
#ifndef ACTIONCOMPLETE_H
#define ACTIONCOMPLETE_H


/*!
 * \file
 *
 * \brief UpnpActionComplete object declaration.
 *
 * \author Marcelo Roberto Jimenez
 */

#define CLASS UpnpActionComplete

#define EXPAND_CLASS_MEMBERS(CLASS) \
	EXPAND_CLASS_MEMBER_INT(CLASS, ErrCode, int) \
	EXPAND_CLASS_MEMBER_STRING(CLASS, CtrlUrl) \
	EXPAND_CLASS_MEMBER_INT(CLASS, ActionRequest, IXML_Document *) \
	EXPAND_CLASS_MEMBER_INT(CLASS, ActionResult, IXML_Document *) \

#include "TemplateInclude.h"


#endif /* ACTIONCOMPLETE_H */

