
/*!
 * \file
 *
 * \brief SSDPResultDataCallback.
 *
 * \author Marcelo Roberto Jimenez
 */

#include "config.h"

#include "SSDPResultData.h"
#include "SSDPResultDataCallback.h"

void SSDPResultData_Callback(const SSDPResultData *p)
{
	Upnp_FunPtr callback = SSDPResultData_get_CtrlptCallback(p);
	callback(UPNP_DISCOVERY_SEARCH_RESULT,
		SSDPResultData_get_Param(p),
		SSDPResultData_get_Cookie(p));
}
