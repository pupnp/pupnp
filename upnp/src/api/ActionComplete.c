

#include "config.h"


#include "ActionComplete.h"


#include <stdlib.h> // for calloc(), free()
#include <string.h> // for strlen(), strdup()


struct SUpnpActionComplete
{
	int m_errCode;
	UpnpString *m_ctrlUrl;
	IXML_Document *m_actionRequest;
	IXML_Document *m_actionResult;
};


UpnpActionComplete *UpnpActionComplete_new()
{
	struct SUpnpActionComplete *p = calloc(1, sizeof (struct SUpnpActionComplete));

#if 0
	p->m_errCode = 0;
#endif
	p->m_ctrlUrl = UpnpString_new();
#if 0
	p->m_actionRequest = NULL;
	p->m_actionResult = NULL;
#endif

	return (UpnpActionComplete *)p;
}


void UpnpActionComplete_delete(UpnpActionComplete *p)
{
	struct SUpnpActionComplete *q = (struct SUpnpActionComplete *)p;

	q->m_errCode = 0;

	UpnpString_delete(q->m_ctrlUrl);
	q->m_ctrlUrl = NULL;
	
	UpnpActionComplete_set_ActionRequest(p, NULL);

	UpnpActionComplete_set_ActionResult(p, NULL);

	free(p);
}


UpnpActionComplete *UpnpActionComplete_dup(const UpnpActionComplete *p)
{
	UpnpActionComplete *q = UpnpActionComplete_new();
	
	UpnpActionComplete_assign(q, p);
	
	return q;
}


void UpnpActionComplete_assign(UpnpActionComplete *q, const UpnpActionComplete *p)
{
	if (q != p) {
		UpnpActionComplete_set_ErrCode(q, UpnpActionComplete_get_ErrCode(p));
		UpnpActionComplete_set_CtrlUrl(q, UpnpActionComplete_get_CtrlUrl(p));
		UpnpActionComplete_set_ActionRequest(q, UpnpActionComplete_get_ActionRequest(p));
		UpnpActionComplete_set_ActionResult(q, UpnpActionComplete_get_ActionResult(p));
	}
}


int UpnpActionComplete_get_ErrCode(const UpnpActionComplete *p)
{
	return ((struct SUpnpActionComplete *)p)->m_errCode;
}


void UpnpActionComplete_set_ErrCode(UpnpActionComplete *p, int n)
{
	((struct SUpnpActionComplete *)p)->m_errCode = n;
}


const UpnpString *UpnpActionComplete_get_CtrlUrl(const UpnpActionComplete *p)
{
	return ((struct SUpnpActionComplete *)p)->m_ctrlUrl;
}


void UpnpActionComplete_set_CtrlUrl(UpnpActionComplete *p, const UpnpString *s)
{
	UpnpString_delete(((struct SUpnpActionComplete *)p)->m_ctrlUrl);
	((struct SUpnpActionComplete *)p)->m_ctrlUrl = UpnpString_dup(s);
}


void UpnpActionComplete_strcpy_CtrlUrl(UpnpActionComplete *p, const char *s)
{
	UpnpString_delete(((struct SUpnpActionComplete *)p)->m_ctrlUrl);
	((struct SUpnpActionComplete *)p)->m_ctrlUrl = UpnpString_new();
	UpnpString_set_String(((struct SUpnpActionComplete *)p)->m_ctrlUrl, s);
}


IXML_Document *UpnpActionComplete_get_ActionRequest(const UpnpActionComplete *p)
{
	return ((struct SUpnpActionComplete *)p)->m_actionRequest;
}


void UpnpActionComplete_set_ActionRequest(UpnpActionComplete *p, IXML_Document *d)
{
	ixmlDocument_free(((struct SUpnpActionComplete *)p)->m_actionRequest);
	((struct SUpnpActionComplete *)p)->m_actionRequest = d;
}


IXML_Document *UpnpActionComplete_get_ActionResult(const UpnpActionComplete *p)
{
	return ((struct SUpnpActionComplete *)p)->m_actionResult;
}


void UpnpActionComplete_set_ActionResult(UpnpActionComplete *p, IXML_Document *d)
{
	ixmlDocument_free(((struct SUpnpActionComplete *)p)->m_actionResult);
	((struct SUpnpActionComplete *)p)->m_actionResult = d;
}

