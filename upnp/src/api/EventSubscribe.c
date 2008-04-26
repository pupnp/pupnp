

#include "config.h"


#include "EventSubscribe.h"


#include <stdlib.h> // for calloc(), free()


struct SEventSubscribe
{
	int m_errCode;
	int m_timeOut;
	UpnpString *m_SID;
	UpnpString *m_publisherUrl;
};


EventSubscribe *UpnpEventSubscribe_new()
{
	struct SEventSubscribe *p = calloc(1, sizeof (struct SEventSubscribe));
#if 0
	p->errCode = 0;
	p->timeOut = 0;
#endif
	p->m_SID = UpnpString_new();
	p->m_publisherUrl = UpnpString_new();

	return (EventSubscribe *)p;
}


void UpnpEventSubscribe_delete(EventSubscribe *p)
{
	struct SEventSubscribe *q = (struct SEventSubscribe *)p;

	q->m_errCode = 0;

	q->m_timeOut = 0;

	UpnpString_delete(q->m_publisherUrl);
	q->m_publisherUrl = NULL;

	UpnpString_delete(q->m_SID);
	q->m_SID = NULL;

	free(p);
}


EventSubscribe *UpnpEventSubscribe_dup(const EventSubscribe *p)
{
	EventSubscribe *q = UpnpEventSubscribe_new();
	
	UpnpEventSubscribe_assign(q, p);
	
	return q;
}


void UpnpEventSubscribe_assign(EventSubscribe *q, const EventSubscribe *p)
{
	if (q != p) {
		UpnpEventSubscribe_set_ErrCode(q, UpnpEventSubscribe_get_ErrCode(p));
		UpnpEventSubscribe_set_TimeOut(q, UpnpEventSubscribe_get_TimeOut(p));
		UpnpEventSubscribe_set_SID(q, UpnpEventSubscribe_get_SID(p));
		UpnpEventSubscribe_set_PublisherUrl(q, UpnpEventSubscribe_get_PublisherUrl(p));
	}
}


int UpnpEventSubscribe_get_ErrCode(const EventSubscribe *p)
{
	return ((struct SEventSubscribe *)p)->m_errCode;
}


void UpnpEventSubscribe_set_ErrCode(EventSubscribe *p, int n)
{
	((struct SEventSubscribe *)p)->m_errCode = n;
}


int UpnpEventSubscribe_get_TimeOut(const EventSubscribe *p)
{
	return ((struct SEventSubscribe *)p)->m_timeOut;
}


void UpnpEventSubscribe_set_TimeOut(EventSubscribe *p, int n)
{
	((struct SEventSubscribe *)p)->m_timeOut = n;
}


const UpnpString *UpnpEventSubscribe_get_SID(const EventSubscribe *p)
{
	return ((struct SEventSubscribe *)p)->m_SID;
}


void UpnpEventSubscribe_set_SID(EventSubscribe *p, const UpnpString *s)
{
	UpnpString_delete(((struct SEventSubscribe *)p)->m_SID);
	((struct SEventSubscribe *)p)->m_SID = UpnpString_dup(s);
}


void UpnpEventSubscribe_strcpy_SID(EventSubscribe *p, const char *s)
{
	UpnpString_delete(((struct SEventSubscribe *)p)->m_SID);
	((struct SEventSubscribe *)p)->m_SID = UpnpString_new();
	UpnpString_set_String(((struct SEventSubscribe *)p)->m_SID, s);
}


const UpnpString *UpnpEventSubscribe_get_PublisherUrl(const EventSubscribe *p)
{
	return ((struct SEventSubscribe *)p)->m_publisherUrl;
}


void UpnpEventSubscribe_set_PublisherUrl(EventSubscribe *p, const UpnpString *s)
{
	UpnpString_delete(((struct SEventSubscribe *)p)->m_publisherUrl);
	((struct SEventSubscribe *)p)->m_publisherUrl = UpnpString_dup(s);
}


void UpnpEventSubscribe_strcpy_PublisherUrl(EventSubscribe *p, const char *s)
{
	UpnpString_delete(((struct SEventSubscribe *)p)->m_publisherUrl);
	((struct SEventSubscribe *)p)->m_publisherUrl = UpnpString_new();
	UpnpString_set_String(((struct SEventSubscribe *)p)->m_publisherUrl, s);
}

