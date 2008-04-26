

#include "config.h"


#include "ssdp_ResultData.h"


#include <stdlib.h> // for calloc(), free()


struct SSSDPResultData
{
	UpnpDiscovery *m_param;
	void *m_cookie;
	Upnp_FunPtr m_ctrlpt_callback;
};


SSDPResultData *SSDPResultData_new()
{
	struct SSSDPResultData *p = calloc(1, sizeof (struct SSSDPResultData));

	p->m_param = UpnpDiscovery_new();
#if 0
	p->m_cookie = NULL;
	p->m_ctrlpt_callback = NULL;
#endif

	return (SSDPResultData *)p;
}


void SSDPResultData_delete(SSDPResultData *p)
{
	struct SSSDPResultData *q = (struct SSSDPResultData *)p;

	UpnpDiscovery_delete(q->m_param);
	q->m_param = NULL;
	
	q->m_cookie = NULL;

	q->m_ctrlpt_callback = NULL;

	free(p);
}


SSDPResultData *SSDPResultData_dup(const SSDPResultData *p)
{
	SSDPResultData *q = SSDPResultData_new();
	
	SSDPResultData_assign(q, p);
	
	return q;
}


void SSDPResultData_assign(SSDPResultData *q, const SSDPResultData *p)
{
	if (q != p) {
		SSDPResultData_set_Param(q, SSDPResultData_get_Param(p));
		SSDPResultData_set_Cookie(q, SSDPResultData_get_Cookie(p));
		SSDPResultData_set_CtrlptCallback(q, SSDPResultData_get_CtrlptCallback(p));
	}
}


UpnpDiscovery *SSDPResultData_get_Param(const SSDPResultData *p)
{
	return ((struct SSSDPResultData *)p)->m_param;
}


void SSDPResultData_set_Param(SSDPResultData *p, const UpnpDiscovery *d)
{
	UpnpDiscovery_assign(((struct SSSDPResultData *)p)->m_param, d);
}


void *SSDPResultData_get_Cookie(const SSDPResultData *p)
{
	return ((struct SSSDPResultData *)p)->m_cookie;
}


void SSDPResultData_set_Cookie(SSDPResultData *p, void *c)
{
	((struct SSSDPResultData *)p)->m_cookie = c;
}


Upnp_FunPtr SSDPResultData_get_CtrlptCallback(const SSDPResultData *p)
{
	return ((struct SSSDPResultData *)p)->m_ctrlpt_callback;
}


void SSDPResultData_set_CtrlptCallback(SSDPResultData *p, Upnp_FunPtr f)
{
	((struct SSSDPResultData *)p)->m_ctrlpt_callback = f;
}


void SSDPResultData_Callback(const SSDPResultData *p)
{
	struct SSSDPResultData *q = (struct SSSDPResultData *)p;
	q->m_ctrlpt_callback(
		UPNP_DISCOVERY_SEARCH_RESULT,
		q->m_param,
		q->m_cookie);
}

