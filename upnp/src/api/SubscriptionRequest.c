

/*!
 * \file
 *
 * \brief UpnpSubscriptionRequest object implementation.
 *
 * \author Marcelo Roberto Jimenez
 */


#include "config.h"


#include "SubscriptionRequest.h"


#include <stdlib.h> /* for calloc(), free() */
#include <string.h> /* for memset(), strlen(), strdup() */


struct SUpnpSubscriptionRequest
{
	UpnpString *m_serviceId; 
	UpnpString *m_UDN;       
	UpnpString *m_SID;
};


UpnpSubscriptionRequest *UpnpSubscriptionRequest_new()
{
	struct SUpnpSubscriptionRequest *p = calloc(1, sizeof (struct SUpnpSubscriptionRequest));

	p->m_serviceId = UpnpString_new();
	p->m_UDN = UpnpString_new();
	p->m_SID = UpnpString_new();

	return (UpnpSubscriptionRequest *)p;
}


void UpnpSubscriptionRequest_delete(UpnpSubscriptionRequest *p)
{
	struct SUpnpSubscriptionRequest *q = (struct SUpnpSubscriptionRequest *)p;

	if (!q) return;

	UpnpString_delete(q->m_serviceId);
	q->m_serviceId = NULL;

	UpnpString_delete(q->m_UDN);
	q->m_UDN = NULL;

	UpnpString_delete(q->m_SID);
	q->m_SID = NULL;

	free(p);
}


UpnpSubscriptionRequest *UpnpSubscriptionRequest_dup(const UpnpSubscriptionRequest *p)
{
	UpnpSubscriptionRequest *q = UpnpSubscriptionRequest_new();
	
	UpnpSubscriptionRequest_assign(q, p);
	
	return q;
}


void UpnpSubscriptionRequest_assign(UpnpSubscriptionRequest *p, const UpnpSubscriptionRequest *q)
{
	if (p != q) {
		UpnpSubscriptionRequest_set_ServiceId(p, UpnpSubscriptionRequest_get_ServiceId(q));
		UpnpSubscriptionRequest_set_UDN(p, UpnpSubscriptionRequest_get_UDN(q));
		UpnpSubscriptionRequest_set_SID(p, UpnpSubscriptionRequest_get_SID(q));
	}
}


const UpnpString *UpnpSubscriptionRequest_get_ServiceId(const UpnpSubscriptionRequest *p)
{
	return ((struct SUpnpSubscriptionRequest *)p)->m_serviceId;
}

const char *UpnpSubscriptionRequest_get_ServiceId_cstr(const UpnpSubscriptionRequest *p)
{
	return UpnpString_get_String(UpnpSubscriptionRequest_get_ServiceId(p));
}


void UpnpSubscriptionRequest_set_ServiceId(UpnpSubscriptionRequest *p, const UpnpString *s)
{
	UpnpString_delete(((struct SUpnpSubscriptionRequest *)p)->m_serviceId);
	((struct SUpnpSubscriptionRequest *)p)->m_serviceId = UpnpString_dup(s);
}


void UpnpSubscriptionRequest_strcpy_ServiceId(UpnpSubscriptionRequest *p, const char *s)
{
	UpnpString_delete(((struct SUpnpSubscriptionRequest *)p)->m_serviceId);
	((struct SUpnpSubscriptionRequest *)p)->m_serviceId = UpnpString_new();
	UpnpString_set_String(((struct SUpnpSubscriptionRequest *)p)->m_serviceId, s);
}


const UpnpString *UpnpSubscriptionRequest_get_UDN(const UpnpSubscriptionRequest *p)
{
	return ((struct SUpnpSubscriptionRequest *)p)->m_UDN;
}

const char *UpnpSubscriptionRequest_get_UDN_cstr(const UpnpSubscriptionRequest *p)
{
	return UpnpString_get_String(UpnpSubscriptionRequest_get_UDN(p));
}


void UpnpSubscriptionRequest_set_UDN(UpnpSubscriptionRequest *p, const UpnpString *s)
{
	UpnpString_delete(((struct SUpnpSubscriptionRequest *)p)->m_UDN);
	((struct SUpnpSubscriptionRequest *)p)->m_UDN = UpnpString_dup(s);
}


void UpnpSubscriptionRequest_strcpy_UDN(UpnpSubscriptionRequest *p, const char *s)
{
	UpnpString_delete(((struct SUpnpSubscriptionRequest *)p)->m_UDN);
	((struct SUpnpSubscriptionRequest *)p)->m_UDN = UpnpString_new();
	UpnpString_set_String(((struct SUpnpSubscriptionRequest *)p)->m_UDN, s);
}


const UpnpString *UpnpSubscriptionRequest_get_SID(const UpnpSubscriptionRequest *p)
{
	return ((struct SUpnpSubscriptionRequest *)p)->m_SID;
}

const char *UpnpSubscriptionRequest_get_SID_cstr(const UpnpSubscriptionRequest *p)
{
	return UpnpString_get_String(UpnpSubscriptionRequest_get_SID(p));
}


void UpnpSubscriptionRequest_set_SID(UpnpSubscriptionRequest *p, const UpnpString *s)
{
	UpnpString_delete(((struct SUpnpSubscriptionRequest *)p)->m_SID);
	((struct SUpnpSubscriptionRequest *)p)->m_SID = UpnpString_dup(s);
}


void UpnpSubscriptionRequest_strcpy_SID(UpnpSubscriptionRequest *p, const char *s)
{
	UpnpString_delete(((struct SUpnpSubscriptionRequest *)p)->m_SID);
	((struct SUpnpSubscriptionRequest *)p)->m_SID = UpnpString_new();
	UpnpString_set_String(((struct SUpnpSubscriptionRequest *)p)->m_SID, s);
}

