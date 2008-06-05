

#ifndef DISCOVERY_H
#define DISCOVERY_H


/*!
 * \file
 *
 * \brief UpnpDiscovery object declararion.
 *
 * \author Marcelo Roberto Jimenez
 *
 */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** Returned in a \b UPNP_DISCOVERY_RESULT callback. */
typedef struct s_UpnpDiscovery UpnpDiscovery;


#include "UpnpGlobal.h" /* for EXPORT_SPEC */
#include "UpnpString.h"


#ifdef WIN32
	#include <ws2tcpip.h>
#else
	#include <netinet/in.h> /* for sockaddr, sockaddr_storage */
#endif


/** Constructor */
EXPORT_SPEC UpnpDiscovery *UpnpDiscovery_new();

/** Destructor */
EXPORT_SPEC void UpnpDiscovery_delete(UpnpDiscovery *p);

/** Copy Constructor */
EXPORT_SPEC UpnpDiscovery *UpnpDiscovery_dup(const UpnpDiscovery *p);

/** Assignment operator */
EXPORT_SPEC void UpnpDiscovery_assign(UpnpDiscovery *q, const UpnpDiscovery *p);

/** The result code of the \b UpnpSearchAsync call. */
EXPORT_SPEC int UpnpDiscovery_get_ErrCode(const UpnpDiscovery *p);
EXPORT_SPEC void UpnpDiscovery_set_ErrCode(UpnpDiscovery *p, int n);

/** The expiration time of the advertisement. */
EXPORT_SPEC int UpnpDiscovery_get_Expires(const UpnpDiscovery *p);
EXPORT_SPEC void UpnpDiscovery_set_Expires(UpnpDiscovery *p, int n);

/** The unique device identifier. */
EXPORT_SPEC const UpnpString *UpnpDiscovery_get_DeviceID(const UpnpDiscovery *p);
EXPORT_SPEC void UpnpDiscovery_set_DeviceID(UpnpDiscovery *p, const UpnpString *s);
EXPORT_SPEC void UpnpDiscovery_strcpy_DeviceID(UpnpDiscovery *p, const char *s);

/** The device type. */
EXPORT_SPEC const UpnpString *UpnpDiscovery_get_DeviceType(const UpnpDiscovery *p);
EXPORT_SPEC void UpnpDiscovery_set_DeviceType(UpnpDiscovery *p, const UpnpString *s);
EXPORT_SPEC void UpnpDiscovery_strcpy_DeviceType(UpnpDiscovery *p, const char *s);

/** The ServiceType. */
EXPORT_SPEC const UpnpString *UpnpDiscovery_get_ServiceType(const UpnpDiscovery *p);
EXPORT_SPEC void UpnpDiscovery_set_ServiceType(UpnpDiscovery *p, const UpnpString *s);
EXPORT_SPEC void UpnpDiscovery_strcpy_ServiceType(UpnpDiscovery *p, const char *s);

/** The service version. */
EXPORT_SPEC const UpnpString *UpnpDiscovery_get_ServiceVer(const UpnpDiscovery *p);
EXPORT_SPEC void UpnpDiscovery_set_ServiceVer(UpnpDiscovery *p, const UpnpString *s);
EXPORT_SPEC void UpnpDiscovery_strcpy_ServiceVer(UpnpDiscovery *p, const char *s);

/** The URL to the UPnP description document for the device. */
EXPORT_SPEC const UpnpString *UpnpDiscovery_get_Location(const UpnpDiscovery *p);
EXPORT_SPEC void UpnpDiscovery_set_Location(UpnpDiscovery *p, const UpnpString *s);
EXPORT_SPEC void UpnpDiscovery_strcpy_Location(UpnpDiscovery *p, const char *s);
EXPORT_SPEC void UpnpDiscovery_strncpy_Location(UpnpDiscovery *p, const char *s, int n);

/** The operating system the device is running. */
EXPORT_SPEC const UpnpString *UpnpDiscovery_get_Os(const UpnpDiscovery *p);
EXPORT_SPEC void UpnpDiscovery_set_Os(UpnpDiscovery *p, const UpnpString *s);
EXPORT_SPEC void UpnpDiscovery_strcpy_Os(UpnpDiscovery *p, const char *s);
EXPORT_SPEC void UpnpDiscovery_strncpy_Os(UpnpDiscovery *p, const char *s, int n);

/** Date when the response was generated. */
EXPORT_SPEC const UpnpString *UpnpDiscovery_get_Date(const UpnpDiscovery *p);
EXPORT_SPEC void UpnpDiscovery_set_Date(UpnpDiscovery *p, const UpnpString *s);
EXPORT_SPEC void UpnpDiscovery_strcpy_Date(UpnpDiscovery *p, const char *s);

/** Confirmation that the MAN header was understood by the device. */
EXPORT_SPEC const UpnpString *UpnpDiscovery_get_Ext(const UpnpDiscovery *p);
EXPORT_SPEC void UpnpDiscovery_set_Ext(UpnpDiscovery *p, const UpnpString *s);
EXPORT_SPEC void UpnpDiscovery_strcpy_Ext(UpnpDiscovery *p, const char *s);
EXPORT_SPEC void UpnpDiscovery_strncpy_Ext(UpnpDiscovery *p, const char *s, int n);

/** The host address of the device responding to the search. */
EXPORT_SPEC struct sockaddr *UpnpDiscovery_get_DestAddr(const UpnpDiscovery *p);
EXPORT_SPEC void UpnpDiscovery_set_DestAddr(UpnpDiscovery *p, struct sockaddr *sa);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* DISCOVERY_H */

