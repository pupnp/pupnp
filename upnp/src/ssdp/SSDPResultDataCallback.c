
/*!
 * \file
 *
 * \brief SSDPResultDataCallback.
 *
 * \author Marcelo Roberto Jimenez
 */

#include "config.h"

#include "SSDPResultDataCallback.h"

#include "SSDPResultData.h"

void SSDPResultData_Callback(UpnpLib *p, const SSDPResultData *rd)
{
        Upnp_FunPtr callback = SSDPResultData_get_CtrlptCallback(rd);
        callback(p,
                UPNP_DISCOVERY_SEARCH_RESULT,
                SSDPResultData_get_Param(rd),
                SSDPResultData_get_Cookie(rd));
}
