

/*!
 * \file
 *
 * \brief UpnpStateVarRequest object implementation.
 *
 * \author Marcelo Roberto Jimenez
 */


#include "config.h"


#include "StateVarRequest.h"


#include <stdlib.h> /* for calloc(), free() */
#include <string.h> /* for memset(), strlen(), strdup() */


struct SUpnpStateVarRequest
{
	int m_errCode;
	int m_socket;
	UpnpString *m_errStr;
	UpnpString *m_devUDN;
	UpnpString *m_serviceID;
	UpnpString *m_stateVarName;
	/* Variables should be declared with struct sockaddr_storage,
	 * but users must only see a struct sockaddr pointer */
	struct sockaddr_storage m_ctrlPtIPAddr;
	DOMString m_currentVal;
};


UpnpStateVarRequest *UpnpStateVarRequest_new()
{
	struct SUpnpStateVarRequest *p = calloc(1, sizeof (struct SUpnpStateVarRequest));

#if 0
	p->m_errCode = 0;
	p->m_socket = 0;
#endif
	p->m_errStr = UpnpString_new();
	p->m_devUDN = UpnpString_new();
	p->m_serviceID = UpnpString_new();
	p->m_stateVarName = UpnpString_new();
#if 0
	memset(&q->m_ctrlPtIPAddr, 0, sizeof (struct sockaddr_storage));
	p->m_currentVal = NULL;
#endif

	return (UpnpStateVarRequest *)p;
}


void UpnpStateVarRequest_delete(UpnpStateVarRequest *p)
{
	struct SUpnpStateVarRequest *q = (struct SUpnpStateVarRequest *)p;

	q->m_errCode = 0;

	q->m_socket = 0;

	UpnpString_delete(q->m_errStr);
	q->m_errStr = NULL;

	UpnpString_delete(q->m_devUDN);
	q->m_devUDN = NULL;

	UpnpString_delete(q->m_serviceID);
	q->m_serviceID = NULL;

	UpnpString_delete(q->m_stateVarName);
	q->m_stateVarName = NULL;

	memset(&q->m_ctrlPtIPAddr, 0, sizeof (struct sockaddr_storage));

	ixmlFreeDOMString(q->m_currentVal);
	q->m_currentVal = NULL;

	free(p);
}


UpnpStateVarRequest *UpnpStateVarRequest_dup(const UpnpStateVarRequest *p)
{
	UpnpStateVarRequest *q = UpnpStateVarRequest_new();
	
	UpnpStateVarRequest_assign(q, p);
	
	return q;
}


void UpnpStateVarRequest_assign(UpnpStateVarRequest *p, const UpnpStateVarRequest *q)
{
	if (p != q) {
		UpnpStateVarRequest_set_ErrCode(p, UpnpStateVarRequest_get_ErrCode(q));
		UpnpStateVarRequest_set_Socket(p, UpnpStateVarRequest_get_Socket(q));
		UpnpStateVarRequest_set_ErrStr(p, UpnpStateVarRequest_get_ErrStr(q));
		UpnpStateVarRequest_set_StateVarName(p, UpnpStateVarRequest_get_StateVarName(q));
		UpnpStateVarRequest_set_DevUDN(p, UpnpStateVarRequest_get_DevUDN(q));
		UpnpStateVarRequest_set_ServiceID(p, UpnpStateVarRequest_get_ServiceID(q));
		UpnpStateVarRequest_set_CtrlPtIPAddr(p, UpnpStateVarRequest_get_CtrlPtIPAddr(q));
		UpnpStateVarRequest_set_CurrentVal(p, UpnpStateVarRequest_get_CurrentVal(q));
	}
}


int UpnpStateVarRequest_get_ErrCode(const UpnpStateVarRequest *p)
{
	return ((struct SUpnpStateVarRequest *)p)->m_errCode;
}


void UpnpStateVarRequest_set_ErrCode(UpnpStateVarRequest *p, int n)
{
	((struct SUpnpStateVarRequest *)p)->m_errCode = n;
}


int UpnpStateVarRequest_get_Socket(const UpnpStateVarRequest *p)
{
	return ((struct SUpnpStateVarRequest *)p)->m_socket;
}


void UpnpStateVarRequest_set_Socket(UpnpStateVarRequest *p, int n)
{
	((struct SUpnpStateVarRequest *)p)->m_socket = n;
}


const UpnpString *UpnpStateVarRequest_get_ErrStr(const UpnpStateVarRequest *p)
{
	return ((struct SUpnpStateVarRequest *)p)->m_errStr;
}


void UpnpStateVarRequest_set_ErrStr(UpnpStateVarRequest *p, const UpnpString *s)
{
	UpnpString_delete(((struct SUpnpStateVarRequest *)p)->m_errStr);
	((struct SUpnpStateVarRequest *)p)->m_errStr = UpnpString_dup(s);
}


void UpnpStateVarRequest_strcpy_ErrStr(UpnpStateVarRequest *p, const char *s)
{
	UpnpString_delete(((struct SUpnpStateVarRequest *)p)->m_errStr);
	((struct SUpnpStateVarRequest *)p)->m_errStr = UpnpString_new();
	UpnpString_set_String(((struct SUpnpStateVarRequest *)p)->m_errStr, s);
}


const UpnpString *UpnpStateVarRequest_get_DevUDN(const UpnpStateVarRequest *p)
{
	return ((struct SUpnpStateVarRequest *)p)->m_devUDN;
}


void UpnpStateVarRequest_set_DevUDN(UpnpStateVarRequest *p, const UpnpString *s)
{
	UpnpString_delete(((struct SUpnpStateVarRequest *)p)->m_devUDN);
	((struct SUpnpStateVarRequest *)p)->m_devUDN = UpnpString_dup(s);
}


const UpnpString *UpnpStateVarRequest_get_ServiceID(const UpnpStateVarRequest *p)
{
	return ((struct SUpnpStateVarRequest *)p)->m_serviceID;
}


void UpnpStateVarRequest_set_ServiceID(UpnpStateVarRequest *p, const UpnpString *s)
{
	UpnpString_delete(((struct SUpnpStateVarRequest *)p)->m_serviceID);
	((struct SUpnpStateVarRequest *)p)->m_serviceID = UpnpString_dup(s);
}


const UpnpString *UpnpStateVarRequest_get_StateVarName(const UpnpStateVarRequest *p)
{
	return ((struct SUpnpStateVarRequest *)p)->m_stateVarName;
}


void UpnpStateVarRequest_set_StateVarName(UpnpStateVarRequest *p, const UpnpString *s)
{
	UpnpString_delete(((struct SUpnpStateVarRequest *)p)->m_stateVarName);
	((struct SUpnpStateVarRequest *)p)->m_stateVarName = UpnpString_dup(s);
}


void UpnpStateVarRequest_strcpy_StateVarName(UpnpStateVarRequest *p, const char *s)
{
	UpnpString_delete(((struct SUpnpStateVarRequest *)p)->m_errStr);
	((struct SUpnpStateVarRequest *)p)->m_errStr = UpnpString_new();
	UpnpString_set_String(((struct SUpnpStateVarRequest *)p)->m_errStr, s);
}


struct sockaddr *UpnpStateVarRequest_get_CtrlPtIPAddr(const UpnpStateVarRequest *p)
{
	return (struct sockaddr *)&((struct SUpnpStateVarRequest *)p)->m_ctrlPtIPAddr;
}


void UpnpStateVarRequest_set_CtrlPtIPAddr(UpnpStateVarRequest *p, struct sockaddr *sa)
{
	((struct SUpnpStateVarRequest *)p)->m_ctrlPtIPAddr = *(struct sockaddr_storage *)sa;
}


const DOMString UpnpStateVarRequest_get_CurrentVal(const UpnpStateVarRequest *p)
{
	return ((struct SUpnpStateVarRequest *)p)->m_currentVal;
}


void UpnpStateVarRequest_set_CurrentVal(UpnpStateVarRequest *p, const DOMString s)
{
	ixmlFreeDOMString(((struct SUpnpStateVarRequest *)p)->m_currentVal);
	((struct SUpnpStateVarRequest *)p)->m_currentVal = ixmlCloneDOMString(s);
}

