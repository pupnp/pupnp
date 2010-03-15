

/*!
 * \file
 *
 * \brief UpnpDiscovery object implementation.
 *
 * \author Marcelo Roberto Jimenez
 */


#include "config.h"


#include "Discovery.h"


#include <stdlib.h> /* for calloc(), free() */
#include <string.h> /* for memset() */


struct SUpnpDiscovery
{
	int m_errCode;
	int m_expires;
	UpnpString *m_deviceID;
	UpnpString *m_deviceType;
	UpnpString *m_serviceType;
	UpnpString *m_serviceVer;
	UpnpString *m_location;
	UpnpString *m_os;
	UpnpString *m_date;
	UpnpString *m_ext;
	/* Variables should be declared with struct sockaddr_storage,
	 * but users must only see a struct sockaddr pointer */
	struct sockaddr_storage m_destAddr;
};


UpnpDiscovery *UpnpDiscovery_new()
{
	struct SUpnpDiscovery *p = calloc(1, sizeof (struct SUpnpDiscovery));

#if 0
	p->errCode = 0;
	p->m_expires = 0;
#endif
	p->m_deviceID = UpnpString_new();
	p->m_deviceType = UpnpString_new();
	p->m_serviceType = UpnpString_new();
	p->m_serviceVer = UpnpString_new();
	p->m_location = UpnpString_new();
	p->m_os = UpnpString_new();
	p->m_date = UpnpString_new();
	p->m_ext = UpnpString_new();
	memset(&p->m_destAddr, 0, sizeof(struct sockaddr_storage));

	return (UpnpDiscovery *)p;
}


void UpnpDiscovery_delete(UpnpDiscovery *p)
{
	struct SUpnpDiscovery *q = (struct SUpnpDiscovery *)p;

	q->m_errCode = 0;

	q->m_expires = 0;

	UpnpString_delete(q->m_deviceID);
	q->m_deviceID = NULL;

	UpnpString_delete(q->m_deviceType);
	q->m_deviceType = NULL;

	UpnpString_delete(q->m_serviceType);
	q->m_serviceType = NULL;

	UpnpString_delete(q->m_serviceVer);
	q->m_serviceVer = NULL;

	UpnpString_delete(q->m_location);
	q->m_location = NULL;

	UpnpString_delete(q->m_os);
	q->m_os = NULL;

	UpnpString_delete(q->m_date);
	q->m_date = NULL;

	UpnpString_delete(q->m_ext);
	q->m_ext = NULL;

	memset(&q->m_destAddr, 0, sizeof(struct sockaddr_storage));

	free(p);
}


UpnpDiscovery *UpnpDiscovery_dup(const UpnpDiscovery *p)
{
	UpnpDiscovery *q = UpnpDiscovery_new();
	
	UpnpDiscovery_assign(q, p);
	
	return q;
}


void UpnpDiscovery_assign(UpnpDiscovery *p, const UpnpDiscovery *q)
{
	if (p != q) {
		UpnpDiscovery_set_ErrCode(p, UpnpDiscovery_get_ErrCode(q));
		UpnpDiscovery_set_Expires(p, UpnpDiscovery_get_Expires(q));
		UpnpDiscovery_set_DeviceID(p, UpnpDiscovery_get_DeviceID(q));
		UpnpDiscovery_set_DeviceType(p, UpnpDiscovery_get_DeviceType(q));
		UpnpDiscovery_set_ServiceType(p, UpnpDiscovery_get_ServiceType(q));
		UpnpDiscovery_set_ServiceVer(p, UpnpDiscovery_get_ServiceVer(q));
		UpnpDiscovery_set_Location(p, UpnpDiscovery_get_Location(q));
		UpnpDiscovery_set_Os(p, UpnpDiscovery_get_Os(q));
		UpnpDiscovery_set_Date(p, UpnpDiscovery_get_Date(q));
		UpnpDiscovery_set_Ext(p, UpnpDiscovery_get_Ext(q));
		UpnpDiscovery_set_DestAddr(p, UpnpDiscovery_get_DestAddr(q));
	}
}


int UpnpDiscovery_get_ErrCode(const UpnpDiscovery *p)
{
	return ((struct SUpnpDiscovery *)p)->m_errCode;
}


void UpnpDiscovery_set_ErrCode(UpnpDiscovery *p, int n)
{
	((struct SUpnpDiscovery *)p)->m_errCode = n;
}


int UpnpDiscovery_get_Expires(const UpnpDiscovery *p)
{
	return ((struct SUpnpDiscovery *)p)->m_expires;
}


void UpnpDiscovery_set_Expires(UpnpDiscovery *p, int n)
{
	((struct SUpnpDiscovery *)p)->m_expires = n;
}


const UpnpString *UpnpDiscovery_get_DeviceID(const UpnpDiscovery *p)
{
	return ((struct SUpnpDiscovery *)p)->m_deviceID;
}

const char *UpnpDiscovery_get_DeviceID_cstr(const UpnpDiscovery *p)
{
	return UpnpString_get_String(UpnpDiscovery_get_DeviceID(p));
}


void UpnpDiscovery_set_DeviceID(UpnpDiscovery *p, const UpnpString *s)
{
	UpnpString_delete(((struct SUpnpDiscovery *)p)->m_deviceID);
	((struct SUpnpDiscovery *)p)->m_deviceID = UpnpString_dup(s);
}


void UpnpDiscovery_strcpy_DeviceID(UpnpDiscovery *p, const char *s)
{
	UpnpString_delete(((struct SUpnpDiscovery *)p)->m_deviceID);
	((struct SUpnpDiscovery *)p)->m_deviceID = UpnpString_new();
	UpnpString_set_String(((struct SUpnpDiscovery *)p)->m_deviceID, s);
}


const UpnpString *UpnpDiscovery_get_DeviceType(const UpnpDiscovery *p)
{
	return ((struct SUpnpDiscovery *)p)->m_deviceType;
}

const char *UpnpDiscovery_get_DeviceType_cstr(const UpnpDiscovery *p)
{
	return UpnpString_get_String(UpnpDiscovery_get_DeviceType(p));
}


void UpnpDiscovery_set_DeviceType(UpnpDiscovery *p, const UpnpString *s)
{
	UpnpString_delete(((struct SUpnpDiscovery *)p)->m_deviceType);
	((struct SUpnpDiscovery *)p)->m_deviceType = UpnpString_dup(s);
}


void UpnpDiscovery_strcpy_DeviceType(UpnpDiscovery *p, const char *s)
{
	UpnpString_delete(((struct SUpnpDiscovery *)p)->m_deviceID);
	((struct SUpnpDiscovery *)p)->m_deviceID = UpnpString_new();
	UpnpString_set_String(((struct SUpnpDiscovery *)p)->m_deviceID, s);
}


const UpnpString *UpnpDiscovery_get_ServiceType(const UpnpDiscovery *p)
{
	return ((struct SUpnpDiscovery *)p)->m_serviceType;
}

const char *UpnpDiscovery_get_ServiceType_cstr(const UpnpDiscovery *p)
{
	return UpnpString_get_String(UpnpDiscovery_get_ServiceType(p));
}


void UpnpDiscovery_set_ServiceType(UpnpDiscovery *p, const UpnpString *s)
{
	UpnpString_delete(((struct SUpnpDiscovery *)p)->m_serviceType);
	((struct SUpnpDiscovery *)p)->m_serviceType = UpnpString_dup(s);
}


void UpnpDiscovery_strcpy_ServiceType(UpnpDiscovery *p, const char *s)
{
	UpnpString_delete(((struct SUpnpDiscovery *)p)->m_serviceType);
	((struct SUpnpDiscovery *)p)->m_serviceType = UpnpString_new();
	UpnpString_set_String(((struct SUpnpDiscovery *)p)->m_serviceType, s);
}


const UpnpString *UpnpDiscovery_get_ServiceVer(const UpnpDiscovery *p)
{
	return ((struct SUpnpDiscovery *)p)->m_serviceVer;
}

const char *UpnpDiscovery_get_ServiceVer_cstr(const UpnpDiscovery *p)
{
	return UpnpString_get_String(UpnpDiscovery_get_ServiceVer(p));
}


void UpnpDiscovery_set_ServiceVer(UpnpDiscovery *p, const UpnpString *s)
{
	UpnpString_delete(((struct SUpnpDiscovery *)p)->m_serviceVer);
	((struct SUpnpDiscovery *)p)->m_serviceVer = UpnpString_dup(s);
}


void UpnpDiscovery_strcpy_ServiceVer(UpnpDiscovery *p, const char *s)
{
	UpnpString_delete(((struct SUpnpDiscovery *)p)->m_serviceVer);
	((struct SUpnpDiscovery *)p)->m_serviceVer = UpnpString_new();
	UpnpString_set_String(((struct SUpnpDiscovery *)p)->m_serviceVer, s);
}


const UpnpString *UpnpDiscovery_get_Location(const UpnpDiscovery *p)
{
	return ((struct SUpnpDiscovery *)p)->m_location;
}

const char *UpnpDiscovery_get_Location_cstr(const UpnpDiscovery *p)
{
	return UpnpString_get_String(UpnpDiscovery_get_Location(p));
}


void UpnpDiscovery_set_Location(UpnpDiscovery *p, const UpnpString *s)
{
	UpnpString_delete(((struct SUpnpDiscovery *)p)->m_location);
	((struct SUpnpDiscovery *)p)->m_location = UpnpString_dup(s);
}


void UpnpDiscovery_strcpy_Location(UpnpDiscovery *p, const char *s)
{
	UpnpString_delete(((struct SUpnpDiscovery *)p)->m_location);
	((struct SUpnpDiscovery *)p)->m_location = UpnpString_new();
	UpnpString_set_String(((struct SUpnpDiscovery *)p)->m_location, s);
}


void UpnpDiscovery_strncpy_Location(UpnpDiscovery *p, const char *s, int n)
{
	UpnpString_delete(((struct SUpnpDiscovery *)p)->m_location);
	((struct SUpnpDiscovery *)p)->m_location = UpnpString_new();
	UpnpString_set_StringN(((struct SUpnpDiscovery *)p)->m_location, s, n);
}


const UpnpString *UpnpDiscovery_get_Os(const UpnpDiscovery *p)
{
	return ((struct SUpnpDiscovery *)p)->m_os;
}

const char *UpnpDiscovery_get_Os_cstr(const UpnpDiscovery *p)
{
	return UpnpString_get_String(UpnpDiscovery_get_Os(p));
}


void UpnpDiscovery_set_Os(UpnpDiscovery *p, const UpnpString *s)
{
	UpnpString_delete(((struct SUpnpDiscovery *)p)->m_os);
	((struct SUpnpDiscovery *)p)->m_os = UpnpString_dup(s);
}


void UpnpDiscovery_strcpy_Os(UpnpDiscovery *p, const char *s)
{
	UpnpString_delete(((struct SUpnpDiscovery *)p)->m_os);
	((struct SUpnpDiscovery *)p)->m_os = UpnpString_new();
	UpnpString_set_String(((struct SUpnpDiscovery *)p)->m_os, s);
}


void UpnpDiscovery_strncpy_Os(UpnpDiscovery *p, const char *s, int n)
{
	UpnpString_delete(((struct SUpnpDiscovery *)p)->m_os);
	((struct SUpnpDiscovery *)p)->m_os = UpnpString_new();
	UpnpString_set_StringN(((struct SUpnpDiscovery *)p)->m_os, s, n);
}


const UpnpString *UpnpDiscovery_get_Date(const UpnpDiscovery *p)
{
	return ((struct SUpnpDiscovery *)p)->m_date;
}

const char *UpnpDiscovery_get_Date_cstr(const UpnpDiscovery *p)
{
	return UpnpString_get_String(UpnpDiscovery_get_Date(p));
}


void UpnpDiscovery_set_Date(UpnpDiscovery *p, const UpnpString *s)
{
	UpnpString_delete(((struct SUpnpDiscovery *)p)->m_date);
	((struct SUpnpDiscovery *)p)->m_date = UpnpString_dup(s);
}


void UpnpDiscovery_strcpy_Date(UpnpDiscovery *p, const char *s)
{
	UpnpString_delete(((struct SUpnpDiscovery *)p)->m_date);
	((struct SUpnpDiscovery *)p)->m_date = UpnpString_new();
	UpnpString_set_String(((struct SUpnpDiscovery *)p)->m_date, s);
}


const UpnpString *UpnpDiscovery_get_Ext(const UpnpDiscovery *p)
{
	return ((struct SUpnpDiscovery *)p)->m_ext;
}

const char *UpnpDiscovery_get_Ext_cstr(const UpnpDiscovery *p)
{
	return UpnpString_get_String(UpnpDiscovery_get_Ext(p));
}


void UpnpDiscovery_set_Ext(UpnpDiscovery *p, const UpnpString *s)
{
	UpnpString_delete(((struct SUpnpDiscovery *)p)->m_ext);
	((struct SUpnpDiscovery *)p)->m_ext = UpnpString_dup(s);
}


void UpnpDiscovery_strcpy_Ext(UpnpDiscovery *p, const char *s)
{
	UpnpString_delete(((struct SUpnpDiscovery *)p)->m_ext);
	((struct SUpnpDiscovery *)p)->m_ext = UpnpString_new();
	UpnpString_set_String(((struct SUpnpDiscovery *)p)->m_ext, s);
}


void UpnpDiscovery_strncpy_Ext(UpnpDiscovery *p, const char *s, int n)
{
	UpnpString_delete(((struct SUpnpDiscovery *)p)->m_ext);
	((struct SUpnpDiscovery *)p)->m_ext = UpnpString_new();
	UpnpString_set_StringN(((struct SUpnpDiscovery *)p)->m_ext, s, n);
}


const struct sockaddr *UpnpDiscovery_get_DestAddr(const UpnpDiscovery *p)
{
	return (struct sockaddr *)&((struct SUpnpDiscovery *)p)->m_destAddr;
}


void UpnpDiscovery_set_DestAddr(UpnpDiscovery *p, const struct sockaddr *sa)
{
	((struct SUpnpDiscovery *)p)->m_destAddr = *(struct sockaddr_storage *)sa;
}

