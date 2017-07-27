#ifndef EXTRAHEADERS_H
#define EXTRAHEADERS_H


/*!
 * \file
 *
 * \brief ExtraHeaders object declararion.
 *
 * \author Marcelo Roberto Jimenez
 */

#define CLASS ExtraHeaders

#define EXPAND_CLASS_MEMBERS(CLASS) \
	EXPAND_CLASS_MEMBER_LIST(CLASS, node) \
	EXPAND_CLASS_MEMBER_STRING(CLASS, name) \
        EXPAND_CLASS_MEMBER_STRING(CLASS, value) \
        EXPAND_CLASS_MEMBER_DOMSTRING(CLASS, resp) \

#include "TemplateInclude.h"


#endif /* EXTRAHEADERS_H */
