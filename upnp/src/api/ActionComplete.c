

/*!
 * \file
 *
 * \brief UpnpActionComplete object implementation.
 *
 * \author Marcelo Roberto Jimenez
 */


#include "config.h"


#include "ActionComplete.h"


#include <stdlib.h> /* for calloc(), free() */
#include <string.h> /* for strlen(), strdup() */


/*!
 * \brief Internal implementation of the UpnpActionComplete object.
 */
struct SUpnpActionComplete
{
	/*! The result of the operation */
	int m_errCode;
	/*! The control URL for service. */
	UpnpString *m_ctrlUrl;
	/*! The DOM document describing the action. */
	IXML_Document *m_actionRequest;
	/*! The DOM document describing the result of the action */
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

	if (!q) return;

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


void UpnpActionComplete_assign(UpnpActionComplete *p, const UpnpActionComplete *q)
{
	if (p != q) {
		UpnpActionComplete_set_ErrCode(p, UpnpActionComplete_get_ErrCode(q));
		UpnpActionComplete_set_CtrlUrl(p, UpnpActionComplete_get_CtrlUrl(q));
		UpnpActionComplete_set_ActionRequest(p, UpnpActionComplete_get_ActionRequest(q));
		UpnpActionComplete_set_ActionResult(p, UpnpActionComplete_get_ActionResult(q));
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

const char *UpnpActionComplete_get_CtrlUrl_cstr(const UpnpActionComplete *p)
{
	return UpnpString_get_String(UpnpActionComplete_get_CtrlUrl(p));
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

