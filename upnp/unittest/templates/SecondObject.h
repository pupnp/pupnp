
#ifndef SECONDOBJECT_H
#define SECONDOBJECT_H

/*!
 * \file
 *
 * \brief Object used in unit testing of object templates.
 */

/******************************************************************************/

#ifdef TEMPLATE_GENERATE_SOURCE
#undef TEMPLATE_GENERATE_SOURCE

        /*#include "ThirdObject.h"*/

#define TEMPLATE_GENERATE_SOURCE
#else /* TEMPLATE_GENERATE_SOURCE */

        /*#include "ThirdObject.h"*/

#endif /* TEMPLATE_GENERATE_SOURCE */

/******************************************************************************/

#include "TheStruct.h"

#define CLASS UnitSecondObject

#define EXPAND_CLASS_MEMBERS(CLASS) \
        EXPAND_CLASS_MEMBER_INT(CLASS, TheInteger, int) \
        EXPAND_CLASS_MEMBER_INT(CLASS, TheUnsignedLong, unsigned long) \
        EXPAND_CLASS_MEMBER_INT(CLASS, TheCharPointer, char *) \
	EXPAND_CLASS_MEMBER_BUFFER(CLASS, TheBuffer, struct TheStruct) \
	EXPAND_CLASS_MEMBER_LIST(CLASS, TheList) \
	/* EXPAND_CLASS_MEMBER_OBJECT(CLASS, TheThirdObject, ThirdObject) */ \
        EXPAND_CLASS_MEMBER_STRING(CLASS, TheString) \
	EXPAND_CLASS_MEMBER_DOMSTRING(CLASS, TheDomString)

#include "TemplateInclude.h"

#endif /* SECONDOBJECT_H */

