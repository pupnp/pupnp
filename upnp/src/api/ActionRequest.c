

#include "config.h"


#include "ActionRequest.h"


#include <stdlib.h> // for calloc(), free()
#include <string.h> // for memset(), strlen(), strdup()


struct SUpnpActionRequest
{
	int m_errCode;
	int m_socket;
	UpnpString *m_errStr;
	UpnpString *m_actionName;
	UpnpString *m_devUDN;
	UpnpString *m_serviceID;
	IXML_Document *m_actionRequest;
	IXML_Document *m_actionResult;
	IXML_Document *m_soapHeader;
	struct sockaddr_storage m_ctrlPtIPAddr;
};


UpnpActionRequest *UpnpActionRequest_new()
{
	struct SUpnpActionRequest *p = calloc(1, sizeof (struct SUpnpActionRequest));

#if 0
	p->m_errCode = 0;
	p->m_socket = 0;
#endif
	p->m_errStr = UpnpString_new();
	p->m_actionName = UpnpString_new();
	p->m_devUDN = UpnpString_new();
	p->m_serviceID = UpnpString_new();
#if 0
	p->m_actionRequest = NULL;
	p->m_actionResult = NULL;
	p->m_soapHeader = NULL;
	memset(&p->m_ctrlPtIPAddr, 0, sizeof (struct sockaddr_storage));
#endif
	return (UpnpActionRequest *)p;
}


void UpnpActionRequest_delete(UpnpActionRequest *p)
{
	struct SUpnpActionRequest *q = (struct SUpnpActionRequest *)p;

	q->m_errCode = 0;

	q->m_socket = 0;

	UpnpString_delete(q->m_errStr);
	q->m_errStr = NULL;

	UpnpString_delete(q->m_actionName);
	q->m_actionName = NULL;

	UpnpString_delete(q->m_devUDN);
	q->m_devUDN = NULL;

	UpnpString_delete(q->m_serviceID);
	q->m_serviceID = NULL;

	UpnpActionRequest_set_ActionRequest(p, NULL);

	UpnpActionRequest_set_ActionResult(p, NULL);

	UpnpActionRequest_set_SoapHeader(p, NULL);

	memset(&q->m_ctrlPtIPAddr, 0, sizeof (struct sockaddr_storage));

	free(p);
}


UpnpActionRequest *UpnpActionRequest_dup(const UpnpActionRequest *p)
{
	UpnpActionRequest *q = UpnpActionRequest_new();
	
	UpnpActionRequest_assign(q, p);
	
	return q;
}


void UpnpActionRequest_assign(UpnpActionRequest *q, const UpnpActionRequest *p)
{
	if (q != p) {
		UpnpActionRequest_set_ErrCode(q, UpnpActionRequest_get_ErrCode(p));
		UpnpActionRequest_set_Socket(q, UpnpActionRequest_get_Socket(p));
		UpnpActionRequest_set_ErrStr(q, UpnpActionRequest_get_ErrStr(p));
		UpnpActionRequest_set_ActionName(q, UpnpActionRequest_get_ActionName(p));
		UpnpActionRequest_set_DevUDN(q, UpnpActionRequest_get_DevUDN(p));
		UpnpActionRequest_set_ServiceID(q, UpnpActionRequest_get_ServiceID(p));
		UpnpActionRequest_set_ActionRequest(q, UpnpActionRequest_get_ActionRequest(p));
		UpnpActionRequest_set_ActionResult(q, UpnpActionRequest_get_ActionResult(p));
		UpnpActionRequest_set_CtrlPtIPAddr(q, UpnpActionRequest_get_CtrlPtIPAddr(p));
		UpnpActionRequest_set_SoapHeader(q, UpnpActionRequest_get_SoapHeader(p));
	}
}


int UpnpActionRequest_get_ErrCode(const UpnpActionRequest *p)
{
	return ((struct SUpnpActionRequest *)p)->m_errCode;
}


void UpnpActionRequest_set_ErrCode(UpnpActionRequest *p, int n)
{
	((struct SUpnpActionRequest *)p)->m_errCode = n;
}


int UpnpActionRequest_get_Socket(const UpnpActionRequest *p)
{
	return ((struct SUpnpActionRequest *)p)->m_socket;
}


void UpnpActionRequest_set_Socket(UpnpActionRequest *p, int n)
{
	((struct SUpnpActionRequest *)p)->m_socket = n;
}


const UpnpString *UpnpActionRequest_get_ErrStr(const UpnpActionRequest *p)
{
	return ((struct SUpnpActionRequest *)p)->m_errStr;
}


void UpnpActionRequest_set_ErrStr(UpnpActionRequest *p, const UpnpString *s)
{
	UpnpString_delete(((struct SUpnpActionRequest *)p)->m_errStr);
	((struct SUpnpActionRequest *)p)->m_errStr = UpnpString_dup(s);
}


void UpnpActionRequest_strcpy_ErrStr(UpnpActionRequest *p, const char *s)
{
	UpnpString_delete(((struct SUpnpActionRequest *)p)->m_errStr);
	((struct SUpnpActionRequest *)p)->m_errStr = UpnpString_new();
	UpnpString_set_String(((struct SUpnpActionRequest *)p)->m_errStr, s);
}


const UpnpString *UpnpActionRequest_get_ActionName(const UpnpActionRequest *p)
{
	return ((struct SUpnpActionRequest *)p)->m_actionName;
}


void UpnpActionRequest_set_ActionName(UpnpActionRequest *p, const UpnpString *s)
{
	UpnpString_delete(((struct SUpnpActionRequest *)p)->m_actionName);
	((struct SUpnpActionRequest *)p)->m_actionName = UpnpString_dup(s);
}


void UpnpActionRequest_strcpy_ActionName(UpnpActionRequest *p, const char *s)
{
	UpnpString_delete(((struct SUpnpActionRequest *)p)->m_actionName);
	((struct SUpnpActionRequest *)p)->m_actionName = UpnpString_new();
	UpnpString_set_String(((struct SUpnpActionRequest *)p)->m_actionName, s);
}


const UpnpString *UpnpActionRequest_get_DevUDN(const UpnpActionRequest *p)
{
	return ((struct SUpnpActionRequest *)p)->m_devUDN;
}


void UpnpActionRequest_set_DevUDN(UpnpActionRequest *p, const UpnpString *s)
{
	UpnpString_delete(((struct SUpnpActionRequest *)p)->m_devUDN);
	((struct SUpnpActionRequest *)p)->m_devUDN = UpnpString_dup(s);
}


const UpnpString *UpnpActionRequest_get_ServiceID(const UpnpActionRequest *p)
{
	return ((struct SUpnpActionRequest *)p)->m_serviceID;
}


void UpnpActionRequest_set_ServiceID(UpnpActionRequest *p, const UpnpString *s)
{
	UpnpString_delete(((struct SUpnpActionRequest *)p)->m_serviceID);
	((struct SUpnpActionRequest *)p)->m_serviceID = UpnpString_dup(s);
}


IXML_Document *UpnpActionRequest_get_ActionRequest(const UpnpActionRequest *p)
{
	return ((struct SUpnpActionRequest *)p)->m_actionRequest;
}


void UpnpActionRequest_set_ActionRequest(UpnpActionRequest *p, IXML_Document *d)
{
	ixmlDocument_free(((struct SUpnpActionRequest *)p)->m_actionRequest);
	((struct SUpnpActionRequest *)p)->m_actionRequest = d;
}


IXML_Document *UpnpActionRequest_get_ActionResult(const UpnpActionRequest *p)
{
	return ((struct SUpnpActionRequest *)p)->m_actionResult;
}


void UpnpActionRequest_set_ActionResult(UpnpActionRequest *p, IXML_Document *d)
{
	ixmlDocument_free(((struct SUpnpActionRequest *)p)->m_actionResult);
	((struct SUpnpActionRequest *)p)->m_actionResult = d;
}


struct sockaddr_storage *UpnpActionRequest_get_CtrlPtIPAddr(const UpnpActionRequest *p)
{
	return &((struct SUpnpActionRequest *)p)->m_ctrlPtIPAddr;
}


void UpnpActionRequest_set_CtrlPtIPAddr(UpnpActionRequest *p, struct sockaddr_storage *ia)
{
	((struct SUpnpActionRequest *)p)->m_ctrlPtIPAddr = *ia;
}


IXML_Document *UpnpActionRequest_get_SoapHeader(const UpnpActionRequest *p)
{
	return ((struct SUpnpActionRequest *)p)->m_soapHeader;
}


void UpnpActionRequest_set_SoapHeader(UpnpActionRequest *p, IXML_Document *d)
{
	ixmlDocument_free(((struct SUpnpActionRequest *)p)->m_soapHeader);
	((struct SUpnpActionRequest *)p)->m_soapHeader = d;
}

