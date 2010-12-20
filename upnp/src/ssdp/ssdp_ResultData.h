
#ifndef SSDP_RESULTDATA_H
#define SSDP_RESULTDATA_H

/*!
 * \addtogroup SSDPlib
 *
 * @{
 * 
 * \file
 *
 * \brief SSDPResultData object declararion.
 *
 * \author Marcelo Roberto Jimenez
 */

/******************************************************************************/

#ifdef TEMPLATE_GENERATE_SOURCE
#undef TEMPLATE_GENERATE_SOURCE

	#include "Discovery.h" /* for UpnpDiscovery */

#define TEMPLATE_GENERATE_SOURCE
#else /* TEMPLATE_GENERATE_SOURCE */

	#include "Discovery.h" /* for UpnpDiscovery */

#endif /* TEMPLATE_GENERATE_SOURCE */

/******************************************************************************/

#include "Callback.h" /* for Upnp_FunPtr */

#define CLASS SSDPResultData

#define EXPAND_CLASS_MEMBERS(CLASS) \
	EXPAND_CLASS_MEMBER_OBJECT(CLASS, Param, UpnpDiscovery) \
	EXPAND_CLASS_MEMBER_INT(CLASS, Cookie, void *) \
	EXPAND_CLASS_MEMBER_INT(CLASS, CtrlptCallback, Upnp_FunPtr) \

#include "TemplateInclude.h"

/*! */
void SSDPResultData_Callback(const SSDPResultData *p);

/* @} SSDPlib */

#endif /* SSDP_RESULTDATA_H */

