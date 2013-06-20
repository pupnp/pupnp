

#ifndef CLIENTSUBSCRIPTION_H
#define CLIENTSUBSCRIPTION_H


/*!
 * \file
 *
 * \brief GenlibClientSubscription object declararion.
 *
 * \author Marcelo Roberto Jimenez
 */

#define CLASS GenlibClientSubscription

#define EXPAND_CLASS_MEMBERS(CLASS) \
	EXPAND_CLASS_MEMBER_INT(CLASS, RenewEventId, int) \
	EXPAND_CLASS_MEMBER_STRING(CLASS, SID) \
	EXPAND_CLASS_MEMBER_STRING(CLASS, ActualSID) \
	EXPAND_CLASS_MEMBER_STRING(CLASS, EventURL) \
	EXPAND_CLASS_MEMBER_INT(CLASS, Next, GenlibClientSubscription *) \

#include "TemplateInclude.h"


#endif /* CLIENTSUBSCRIPTION_H */

