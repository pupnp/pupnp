

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


void UpnpSubscriptionRequest_assign(UpnpSubscriptionRequest *q, const UpnpSubscriptionRequest *p)
{
	if (q != p) {
		UpnpSubscriptionRequest_set_ServiceId(q, UpnpSubscriptionRequest_get_ServiceId(p));
		UpnpSubscriptionRequest_set_UDN(q, UpnpSubscriptionRequest_get_UDN(p));
		UpnpSubscriptionRequest_set_SID(q, UpnpSubscriptionRequest_get_SID(p));
	}
}


const UpnpString *UpnpSubscriptionRequest_get_ServiceId(const UpnpSubscriptionRequest *p)
{
	return ((struct SUpnpSubscriptionRequest *)p)->m_serviceId;
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

