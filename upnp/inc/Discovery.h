

#ifndef DISCOVERY_H
#define DISCOVERY_H


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** Returned in a {\bf UPNP_DISCOVERY_RESULT} callback. */
typedef struct {} UpnpDiscovery;


#include "String.h"     /* for UpnpString */


#include <netinet/in.h> /* for sockaddr_in */


/** Constructor */
UpnpDiscovery *UpnpDiscovery_new();

/** Destructor */
void UpnpDiscovery_delete(UpnpDiscovery *p);

/** Copy Constructor */
UpnpDiscovery *UpnpDiscovery_dup(const UpnpDiscovery *p);

/** Assignment operator */
void UpnpDiscovery_assign(UpnpDiscovery *q, const UpnpDiscovery *p);

/** The result code of the {\bf UpnpSearchAsync} call. */
int UpnpDiscovery_get_ErrCode(const UpnpDiscovery *p);
void UpnpDiscovery_set_ErrCode(UpnpDiscovery *p, int n);

/** The expiration time of the advertisement. */
int UpnpDiscovery_get_Expires(const UpnpDiscovery *p);
void UpnpDiscovery_set_Expires(UpnpDiscovery *p, int n);

/** The unique device identifier. */
const UpnpString *UpnpDiscovery_get_DeviceID(const UpnpDiscovery *p);
void UpnpDiscovery_set_DeviceID(UpnpDiscovery *p, const UpnpString *s);
void UpnpDiscovery_strcpy_DeviceID(UpnpDiscovery *p, const char *s);

/** The device type. */
const UpnpString *UpnpDiscovery_get_DeviceType(const UpnpDiscovery *p);
void UpnpDiscovery_set_DeviceType(UpnpDiscovery *p, const UpnpString *s);
void UpnpDiscovery_strcpy_DeviceType(UpnpDiscovery *p, const char *s);

/** The ServiceType. */
const UpnpString *UpnpDiscovery_get_ServiceType(const UpnpDiscovery *p);
void UpnpDiscovery_set_ServiceType(UpnpDiscovery *p, const UpnpString *s);
void UpnpDiscovery_strcpy_ServiceType(UpnpDiscovery *p, const char *s);

/** The service version. */
const UpnpString *UpnpDiscovery_get_ServiceVer(const UpnpDiscovery *p);
void UpnpDiscovery_set_ServiceVer(UpnpDiscovery *p, const UpnpString *s);
void UpnpDiscovery_strcpy_ServiceVer(UpnpDiscovery *p, const char *s);

/** The URL to the UPnP description document for the device. */
const UpnpString *UpnpDiscovery_get_Location(const UpnpDiscovery *p);
void UpnpDiscovery_set_Location(UpnpDiscovery *p, const UpnpString *s);
void UpnpDiscovery_strcpy_Location(UpnpDiscovery *p, const char *s);
void UpnpDiscovery_strncpy_Location(UpnpDiscovery *p, const char *s, int n);

/** The operating system the device is running. */
const UpnpString *UpnpDiscovery_get_Os(const UpnpDiscovery *p);
void UpnpDiscovery_set_Os(UpnpDiscovery *p, const UpnpString *s);
void UpnpDiscovery_strcpy_Os(UpnpDiscovery *p, const char *s);
void UpnpDiscovery_strncpy_Os(UpnpDiscovery *p, const char *s, int n);

/** Date when the response was generated. */
const UpnpString *UpnpDiscovery_get_Date(const UpnpDiscovery *p);
void UpnpDiscovery_set_Date(UpnpDiscovery *p, const UpnpString *s);
void UpnpDiscovery_strcpy_Date(UpnpDiscovery *p, const char *s);

/** Confirmation that the MAN header was understood by the device. */
const UpnpString *UpnpDiscovery_get_Ext(const UpnpDiscovery *p);
void UpnpDiscovery_set_Ext(UpnpDiscovery *p, const UpnpString *s);
void UpnpDiscovery_strcpy_Ext(UpnpDiscovery *p, const char *s);
void UpnpDiscovery_strncpy_Ext(UpnpDiscovery *p, const char *s, int n);

/** The host address of the device responding to the search. */
struct sockaddr *UpnpDiscovery_get_DestAddr(const UpnpDiscovery *p);
void UpnpDiscovery_set_DestAddr(UpnpDiscovery *p, struct sockaddr *sa);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* DISCOVERY_H */

