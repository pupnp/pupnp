

#include "config.h"


#include "Event.h"


#include <stdlib.h> /* for calloc(), free() */


struct SUpnpEvent
{
	int m_eventKey;
	IXML_Document *m_changedVariables;
	UpnpString *m_SID;
};


UpnpEvent *UpnpEvent_new()
{
	struct SUpnpEvent *p = calloc(1, sizeof (struct SUpnpEvent));

#if 0
	p->m_eventKey = 0;
	p->m_changedVariables = NULL;
#endif
	p->m_SID = UpnpString_new();

	return (UpnpEvent *)p;
}


void UpnpEvent_delete(UpnpEvent *p)
{
	struct SUpnpEvent *q = (struct SUpnpEvent *)p;

	q->m_eventKey = 0;

	q->m_changedVariables = NULL;

	UpnpString_delete(q->m_SID);
	q->m_SID = NULL;

	free(p);
}


UpnpEvent *UpnpEvent_dup(const UpnpEvent *p)
{
	UpnpEvent *q = UpnpEvent_new();
	
	UpnpEvent_assign(q, p);
	
	return q;
}


void UpnpEvent_assign(UpnpEvent *q, const UpnpEvent *p)
{
	if (q != p) {
		UpnpEvent_set_EventKey(q, UpnpEvent_get_EventKey(p));
		UpnpEvent_set_ChangedVariables(q, UpnpEvent_get_ChangedVariables(p));
		UpnpEvent_set_SID(q, UpnpEvent_get_SID(p));
	}
}


int UpnpEvent_get_EventKey(const UpnpEvent *p)
{
	return ((struct SUpnpEvent *)p)->m_eventKey;
}


void UpnpEvent_set_EventKey(UpnpEvent *p, int n)
{
	((struct SUpnpEvent *)p)->m_eventKey = n;
}


IXML_Document *UpnpEvent_get_ChangedVariables(const UpnpEvent *p)
{
	return ((struct SUpnpEvent *)p)->m_changedVariables;
}


void UpnpEvent_set_ChangedVariables(UpnpEvent *p, IXML_Document *d)
{
	ixmlDocument_free(((struct SUpnpEvent *)p)->m_changedVariables);
	((struct SUpnpEvent *)p)->m_changedVariables = d;
}


UpnpString *UpnpEvent_get_SID(const UpnpEvent *p)
{
	return ((struct SUpnpEvent *)p)->m_SID;
}


void UpnpEvent_set_SID(UpnpEvent *p, const UpnpString *s)
{
	UpnpString_delete(((struct SUpnpEvent *)p)->m_SID);
	((struct SUpnpEvent *)p)->m_SID = UpnpString_dup(s);
}

