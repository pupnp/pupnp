

#include "config.h"


#include "StateVarComplete.h"


#include <stdlib.h> // for calloc(), free()
#include <string.h> // for strlen(), strdup()


struct SUpnpStateVarComplete
{
	int m_errCode;
	UpnpString *m_ctrlUrl;
	UpnpString *m_stateVarName;
	DOMString m_currentVal;
};


UpnpStateVarComplete *UpnpStateVarComplete_new()
{
	struct SUpnpStateVarComplete *p = calloc(1, sizeof (struct SUpnpStateVarComplete));

#if 0
	p->m_errCode = 0;
#endif
	p->m_ctrlUrl = UpnpString_new();
	p->m_stateVarName = UpnpString_new();
#if 0
	p->m_currentVal = NULL;
#endif

	return (UpnpStateVarComplete *)p;
}


void UpnpStateVarComplete_delete(UpnpStateVarComplete *p)
{
	struct SUpnpStateVarComplete *q = (struct SUpnpStateVarComplete *)p;

	q->m_errCode = 0;

	UpnpString_delete(q->m_ctrlUrl);
	q->m_ctrlUrl = NULL;

	UpnpString_delete(q->m_stateVarName);
	q->m_stateVarName = NULL;

	ixmlFreeDOMString(q->m_currentVal);
	q->m_currentVal = NULL;

	free(p);
}


UpnpStateVarComplete *UpnpStateVarComplete_dup(const UpnpStateVarComplete *p)
{
	UpnpStateVarComplete *q = UpnpStateVarComplete_new();
	
	UpnpStateVarComplete_assign(q, p);
	
	return q;
}


void UpnpStateVarComplete_assign(UpnpStateVarComplete *q, const UpnpStateVarComplete *p)
{
	if (q != p) {
		UpnpStateVarComplete_set_ErrCode(q, UpnpStateVarComplete_get_ErrCode(p));
		UpnpStateVarComplete_set_CtrlUrl(q, UpnpStateVarComplete_get_CtrlUrl(p));
		UpnpStateVarComplete_set_StateVarName(q, UpnpStateVarComplete_get_StateVarName(p));
		UpnpStateVarComplete_set_CurrentVal(q, UpnpStateVarComplete_get_CurrentVal(p));
	}
}


int UpnpStateVarComplete_get_ErrCode(const UpnpStateVarComplete *p)
{
	return ((struct SUpnpStateVarComplete *)p)->m_errCode;
}


void UpnpStateVarComplete_set_ErrCode(UpnpStateVarComplete *p, int n)
{
	((struct SUpnpStateVarComplete *)p)->m_errCode = n;
}


const UpnpString *UpnpStateVarComplete_get_CtrlUrl(const UpnpStateVarComplete *p)
{
	return ((struct SUpnpStateVarComplete *)p)->m_ctrlUrl;
}


void UpnpStateVarComplete_set_CtrlUrl(UpnpStateVarComplete *p, const UpnpString *s)
{
	UpnpString_delete(((struct SUpnpStateVarComplete *)p)->m_ctrlUrl);
	((struct SUpnpStateVarComplete *)p)->m_ctrlUrl = UpnpString_dup(s);
}


void UpnpStateVarComplete_strcpy_CtrlUrl(UpnpStateVarComplete *p, const char *s)
{
	UpnpString_delete(((struct SUpnpStateVarComplete *)p)->m_ctrlUrl);
	((struct SUpnpStateVarComplete *)p)->m_ctrlUrl = UpnpString_new();
	UpnpString_set_String(((struct SUpnpStateVarComplete *)p)->m_ctrlUrl, s);
}


const UpnpString *UpnpStateVarComplete_get_StateVarName(const UpnpStateVarComplete *p)
{
	return ((struct SUpnpStateVarComplete *)p)->m_stateVarName;
}


void UpnpStateVarComplete_set_StateVarName(UpnpStateVarComplete *p, const UpnpString *s)
{
	UpnpString_delete(((struct SUpnpStateVarComplete *)p)->m_stateVarName);
	((struct SUpnpStateVarComplete *)p)->m_stateVarName = UpnpString_dup(s);
}


void UpnpStateVarComplete_strcpy_StateVarName(UpnpStateVarComplete *p, const char *s)
{
	UpnpString_delete(((struct SUpnpStateVarComplete *)p)->m_ctrlUrl);
	((struct SUpnpStateVarComplete *)p)->m_ctrlUrl = UpnpString_new();
	UpnpString_set_String(((struct SUpnpStateVarComplete *)p)->m_ctrlUrl, s);
}


const DOMString UpnpStateVarComplete_get_CurrentVal(const UpnpStateVarComplete *p)
{
	return ((struct SUpnpStateVarComplete *)p)->m_currentVal;
}


void UpnpStateVarComplete_set_CurrentVal(UpnpStateVarComplete *p, const DOMString s)
{
	ixmlFreeDOMString(((struct SUpnpStateVarComplete *)p)->m_currentVal);
	((struct SUpnpStateVarComplete *)p)->m_currentVal = ixmlCloneDOMString(s);
}

