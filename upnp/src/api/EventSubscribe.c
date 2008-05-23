

#include "config.h"


#include "EventSubscribe.h"


#include <stdlib.h> /* for calloc(), free() */


struct SEventSubscribe
{
	int m_errCode;
	int m_timeOut;
	UpnpString *m_SID;
	UpnpString *m_publisherUrl;
};


UpnpEventSubscribe *UpnpEventSubscribe_new()
{
	struct SEventSubscribe *p = calloc(1, sizeof (struct SEventSubscribe));
#if 0
	p->errCode = 0;
	p->timeOut = 0;
#endif
	p->m_SID = UpnpString_new();
	p->m_publisherUrl = UpnpString_new();

	return (UpnpEventSubscribe *)p;
}


void UpnpEventSubscribe_delete(UpnpEventSubscribe *p)
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


UpnpEventSubscribe *UpnpEventSubscribe_dup(const UpnpEventSubscribe *p)
{
	UpnpEventSubscribe *q = UpnpEventSubscribe_new();
	
	UpnpEventSubscribe_assign(q, p);
	
	return q;
}


void UpnpEventSubscribe_assign(UpnpEventSubscribe *q, const UpnpEventSubscribe *p)
{
	if (q != p) {
		UpnpEventSubscribe_set_ErrCode(q, UpnpEventSubscribe_get_ErrCode(p));
		UpnpEventSubscribe_set_TimeOut(q, UpnpEventSubscribe_get_TimeOut(p));
		UpnpEventSubscribe_set_SID(q, UpnpEventSubscribe_get_SID(p));
		UpnpEventSubscribe_set_PublisherUrl(q, UpnpEventSubscribe_get_PublisherUrl(p));
	}
}


int UpnpEventSubscribe_get_ErrCode(const UpnpEventSubscribe *p)
{
	return ((struct SEventSubscribe *)p)->m_errCode;
}


void UpnpEventSubscribe_set_ErrCode(UpnpEventSubscribe *p, int n)
{
	((struct SEventSubscribe *)p)->m_errCode = n;
}


int UpnpEventSubscribe_get_TimeOut(const UpnpEventSubscribe *p)
{
	return ((struct SEventSubscribe *)p)->m_timeOut;
}


void UpnpEventSubscribe_set_TimeOut(UpnpEventSubscribe *p, int n)
{
	((struct SEventSubscribe *)p)->m_timeOut = n;
}


const UpnpString *UpnpEventSubscribe_get_SID(const UpnpEventSubscribe *p)
{
	return ((struct SEventSubscribe *)p)->m_SID;
}


void UpnpEventSubscribe_set_SID(UpnpEventSubscribe *p, const UpnpString *s)
{
	UpnpString_delete(((struct SEventSubscribe *)p)->m_SID);
	((struct SEventSubscribe *)p)->m_SID = UpnpString_dup(s);
}


void UpnpEventSubscribe_strcpy_SID(UpnpEventSubscribe *p, const char *s)
{
	UpnpString_delete(((struct SEventSubscribe *)p)->m_SID);
	((struct SEventSubscribe *)p)->m_SID = UpnpString_new();
	UpnpString_set_String(((struct SEventSubscribe *)p)->m_SID, s);
}


const UpnpString *UpnpEventSubscribe_get_PublisherUrl(const UpnpEventSubscribe *p)
{
	return ((struct SEventSubscribe *)p)->m_publisherUrl;
}


void UpnpEventSubscribe_set_PublisherUrl(UpnpEventSubscribe *p, const UpnpString *s)
{
	UpnpString_delete(((struct SEventSubscribe *)p)->m_publisherUrl);
	((struct SEventSubscribe *)p)->m_publisherUrl = UpnpString_dup(s);
}


void UpnpEventSubscribe_strcpy_PublisherUrl(UpnpEventSubscribe *p, const char *s)
{
	UpnpString_delete(((struct SEventSubscribe *)p)->m_publisherUrl);
	((struct SEventSubscribe *)p)->m_publisherUrl = UpnpString_new();
	UpnpString_set_String(((struct SEventSubscribe *)p)->m_publisherUrl, s);
}

