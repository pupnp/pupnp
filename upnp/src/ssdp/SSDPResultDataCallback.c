/*!
 * \file
 *
 * \brief SSDPResultDataCallback.
 *
 * \author Marcelo Roberto Jimenez
 */

#include "SSDPResultDataCallback.h"

#include "Callback.h"
#include "SSDPResultData.h"

void SSDPResultData_Callback(SSDPResultData *p)
{
	Upnp_FunPtr callback = SSDPResultData_get_CtrlptCallback(p);
	callback(UPNP_DISCOVERY_SEARCH_RESULT,
		SSDPResultData_get_Param(p),
		SSDPResultData_get_Cookie(p));
}
