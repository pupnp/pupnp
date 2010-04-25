

/*!
 * \file
 *
 * \brief UpnpEvent object implementation.
 *
 * \author Marcelo Roberto Jimenez
 */


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

	if (!q) return;

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


void UpnpEvent_assign(UpnpEvent *p, const UpnpEvent *q)
{
	if (p != q) {
		UpnpEvent_set_EventKey(p, UpnpEvent_get_EventKey(q));
		UpnpEvent_set_ChangedVariables(p, UpnpEvent_get_ChangedVariables(q));
		UpnpEvent_set_SID(p, UpnpEvent_get_SID(q));
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


const UpnpString *UpnpEvent_get_SID(const UpnpEvent *p)
{
	return ((struct SUpnpEvent *)p)->m_SID;
}

const char *UpnpEvent_get_SID_cstr(const UpnpEvent *p)
{
	return UpnpString_get_String(UpnpEvent_get_SID(p));
}


void UpnpEvent_set_SID(UpnpEvent *p, const UpnpString *s)
{
	UpnpString_delete(((struct SUpnpEvent *)p)->m_SID);
	((struct SUpnpEvent *)p)->m_SID = UpnpString_dup(s);
}

