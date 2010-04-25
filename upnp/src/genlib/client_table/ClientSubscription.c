
/************************************************************************
 * Purpose: This file defines the functions for clients. It defines 
 * functions for adding and removing clients to and from the client table, 
 * adding and accessing subscription and other attributes pertaining to the 
 * client  
 ************************************************************************/


#include "config.h"


#include "ClientSubscription.h"


#ifdef INCLUDE_CLIENT_APIS


#include <stdlib.h> // for calloc(), free()


struct SClientSubscription {
	int m_renewEventId;
	UpnpString *m_SID;
	UpnpString *m_actualSID;
	UpnpString *m_eventURL;
	struct SClientSubscription *m_next;
};


ClientSubscription *GenlibClientSubscription_new()
{
	struct SClientSubscription *p = calloc(1, sizeof (struct SClientSubscription));
#if 0
	p->renewEventId =  0;
#endif
	p->m_SID = UpnpString_new();
	p->m_actualSID = UpnpString_new();
	p->m_eventURL = UpnpString_new();
	p->m_next = NULL;

	return (ClientSubscription *)p;
}


void GenlibClientSubscription_delete(ClientSubscription *p)
{
	struct SClientSubscription *q = (struct SClientSubscription *)p;

	if (!q) return;

	q->m_renewEventId = 0;

	UpnpString_delete(q->m_SID);
	q->m_SID = NULL;

	UpnpString_delete(q->m_actualSID);
	q->m_actualSID = NULL;

	UpnpString_delete(q->m_eventURL);
	q->m_eventURL = NULL;

	q->m_next = NULL;

	free(p);
}


ClientSubscription *GenlibClientSubscription_dup(const ClientSubscription *p)
{
	ClientSubscription *q = GenlibClientSubscription_new();
	
	GenlibClientSubscription_assign(q, p);
	
	return q;
}


void GenlibClientSubscription_assign(ClientSubscription *q, const ClientSubscription *p)
{
	if (q != p) {
		/*struct SClientSubscription *_p = (struct SClientSubscription *)p;*/
		struct SClientSubscription *_q = (struct SClientSubscription *)q;
		// Do not copy RenewEventId
		_q->m_renewEventId = -1;
		GenlibClientSubscription_set_SID(q, GenlibClientSubscription_get_SID(p));
		GenlibClientSubscription_set_ActualSID(q, GenlibClientSubscription_get_ActualSID(p));
		GenlibClientSubscription_set_EventURL(q, GenlibClientSubscription_get_EventURL(p));
		// Do not copy m_next
		_q->m_next = NULL;
	}
}


int GenlibClientSubscription_get_RenewEventId(const ClientSubscription *p)
{
	return ((struct SClientSubscription *)p)->m_renewEventId;
}


void GenlibClientSubscription_set_RenewEventId(ClientSubscription *p, int n)
{
	((struct SClientSubscription *)p)->m_renewEventId = n;
}


const UpnpString *GenlibClientSubscription_get_SID(const ClientSubscription *p)
{
	return ((struct SClientSubscription *)p)->m_SID;
}

const char *GenlibClientSubscription_get_SID_cstr(const ClientSubscription *p)
{
	return UpnpString_get_String(GenlibClientSubscription_get_SID(p));
}


void GenlibClientSubscription_set_SID(ClientSubscription *p, const UpnpString *s)
{
	UpnpString_delete(((struct SClientSubscription *)p)->m_SID);
	((struct SClientSubscription *)p)->m_SID = UpnpString_dup(s);
}


void GenlibClientSubscription_strcpy_SID(ClientSubscription *p, const char *s)
{
	UpnpString_delete(((struct SClientSubscription *)p)->m_SID);
	((struct SClientSubscription *)p)->m_SID = UpnpString_new();
	UpnpString_set_String(((struct SClientSubscription *)p)->m_SID, s);
}


const UpnpString *GenlibClientSubscription_get_ActualSID(const ClientSubscription *p)
{
	return ((struct SClientSubscription *)p)->m_actualSID;
}


const char *GenlibClientSubscription_get_ActualSID_cstr(const ClientSubscription *p)
{
	return UpnpString_get_String(GenlibClientSubscription_get_ActualSID(p));
}


void GenlibClientSubscription_set_ActualSID(ClientSubscription *p, const UpnpString *s)
{
	UpnpString_delete(((struct SClientSubscription *)p)->m_actualSID);
	((struct SClientSubscription *)p)->m_actualSID = UpnpString_dup(s);
}


void GenlibClientSubscription_strcpy_ActualSID(ClientSubscription *p, const char *s)
{
	UpnpString_delete(((struct SClientSubscription *)p)->m_actualSID);
	((struct SClientSubscription *)p)->m_actualSID = UpnpString_new();
	UpnpString_set_String(((struct SClientSubscription *)p)->m_actualSID, s);
}


const UpnpString *GenlibClientSubscription_get_EventURL(const ClientSubscription *p)
{
	return ((struct SClientSubscription *)p)->m_eventURL;
}


const char *GenlibClientSubscription_get_EventURL_cstr(const ClientSubscription *p)
{
	return UpnpString_get_String(GenlibClientSubscription_get_EventURL(p));
}


void GenlibClientSubscription_set_EventURL(ClientSubscription *p, const UpnpString *s)
{
	UpnpString_delete(((struct SClientSubscription *)p)->m_eventURL);
	((struct SClientSubscription *)p)->m_eventURL = UpnpString_dup(s);
}


void GenlibClientSubscription_strcpy_EventURL(ClientSubscription *p, const char *s)
{
	UpnpString_delete(((struct SClientSubscription *)p)->m_eventURL);
	((struct SClientSubscription *)p)->m_eventURL = UpnpString_new();
	UpnpString_set_String(((struct SClientSubscription *)p)->m_eventURL, s);
}


ClientSubscription *GenlibClientSubscription_get_Next(const ClientSubscription *p)
{
	return (ClientSubscription *)(((struct SClientSubscription *)p)->m_next);
}


void GenlibClientSubscription_set_Next(ClientSubscription *p, ClientSubscription *q)
{
	((struct SClientSubscription *)p)->m_next = (struct SClientSubscription *)q;
}


 #endif /* INCLUDE_CLIENT_APIS */

