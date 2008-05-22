

#ifndef SSDP_RESULTDATA_H
#define SSDP_RESULTDATA_H


/** Structure to contain Discovery response */
typedef struct s_SSDPResultData SSDPResultData;


#include "Discovery.h" /* for UpnpDiscovery */
#include "upnp.h"      /* for Upnp_FunPtr */


/** Constructor */
SSDPResultData *SSDPResultData_new();

/** Destructor */
void SSDPResultData_delete(SSDPResultData *p);

/** Copy Constructor */
SSDPResultData *SSDPResultData_dup(const SSDPResultData *p);

/** Assignment operator */
void SSDPResultData_assign(SSDPResultData *q, const SSDPResultData *p);

/**  */
UpnpDiscovery *SSDPResultData_get_Param(const SSDPResultData *p);
void SSDPResultData_set_Param(SSDPResultData *p, const UpnpDiscovery *d);

/**  */
void *SSDPResultData_get_Cookie(const SSDPResultData *p);
void SSDPResultData_set_Cookie(SSDPResultData *p, void *c);

/**  */
Upnp_FunPtr SSDPResultData_get_CtrlptCallback(const SSDPResultData *p);
void SSDPResultData_set_CtrlptCallback(SSDPResultData *p, Upnp_FunPtr f);

/** */
void SSDPResultData_Callback(const SSDPResultData *p);

#endif /* SSDP_RESULTDATA_H */

