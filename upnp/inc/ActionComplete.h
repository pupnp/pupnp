

#ifndef ACTIONCOMPLETE_H
#define ACTIONCOMPLETE_H


/*!
 * \file
 *
 * \brief UpnpActionComplete object declararion.
 *
 * \author Marcelo Roberto Jimenez
 *
 */


#include "ixml.h"       /* for IXML_Document */
#include "UpnpGlobal.h" /* for EXPORT_SPEC */
#include "UpnpString.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*!
 * \brief The type of an UpnpActionComplete object.
 */
typedef struct s_UpnpActionComplete UpnpActionComplete;


/*!
 * \brief Constructor.
 *
 * \return Pointer to the newly created object.
 */
EXPORT_SPEC UpnpActionComplete *UpnpActionComplete_new();


/*!
 * \brief Destructor.
 */
EXPORT_SPEC void UpnpActionComplete_delete(
	/*! [in] \b this pointer. */
	UpnpActionComplete *p);


/*!
 * \brief Copy Constructor.
 */
EXPORT_SPEC UpnpActionComplete *UpnpActionComplete_dup(
	/*! [in] \b this pointer. */
	const UpnpActionComplete *p);


/*!
 * \brief Assignment operator.
 */
EXPORT_SPEC void UpnpActionComplete_assign(
	/*! [in] \b this pointer. */
	UpnpActionComplete *q,
	/*! [in] \b that pointer. */
	const UpnpActionComplete *p);


/*!
 * \brief Error code getter.
 */
EXPORT_SPEC int UpnpActionComplete_get_ErrCode(
	/*! [in] \b this pointer. */
	const UpnpActionComplete *p);


/*!
 * \brief Error code setter.
 */
EXPORT_SPEC void UpnpActionComplete_set_ErrCode(
	/*! [in] \b this pointer. */
	UpnpActionComplete *p,
	/*! [in] The error code to set. */
	int n);


/*!
 * \brief Control URL getter.
 *
 * \return The control URL string.
 */
EXPORT_SPEC const UpnpString *UpnpActionComplete_get_CtrlUrl(
	/*! [in] \b this pointer. */
	const UpnpActionComplete *p);


/*!
 * \brief Control URL setter.
 */
EXPORT_SPEC void UpnpActionComplete_set_CtrlUrl(
	/*! [in] \b this pointer. */
	UpnpActionComplete *p,
	/*! [in] The control URL string to copy. */
	const UpnpString *s);


/*!
 * \brief Set the control URL from a null terminated C string.
 */
EXPORT_SPEC void UpnpActionComplete_strcpy_CtrlUrl(
	/*! [in] \b this pointer. */
	UpnpActionComplete *p,
	/*! [in] The null terminated control URL C string to copy. */
	const char *s);


/*!
 * \brief ActionRequest document getter.
 *
 * \return A pointer to the document object.
 */
EXPORT_SPEC IXML_Document *UpnpActionComplete_get_ActionRequest(
	/*! [in] \b this pointer. */
	const UpnpActionComplete *p);


/*!
 * \brief ActionRequest document setter.
 *
 * \note The ActionComplete object takes ownership of the document parameter,
 * i.e. it is responsible for deleting it upon destruction.
 */
EXPORT_SPEC void UpnpActionComplete_set_ActionRequest(
	/*! [in] \b this pointer. */
	UpnpActionComplete *p,
	/*! [in] Document to copy. */
	IXML_Document *d);


/*!
 * \brief ActionResult document getter.
 */
EXPORT_SPEC IXML_Document *UpnpActionComplete_get_ActionResult(
	/*! [in] \b this pointer. */
	const UpnpActionComplete *p);


/*!
 * \brief ActionResult document setter.
 *
 * \note The ActionComplete object takes ownership of the document parameter,
 * i.e. it is responsible for deleting it upon destruction.
 */
EXPORT_SPEC void UpnpActionComplete_set_ActionResult(
	/*! [in] \b this pointer. */
	UpnpActionComplete *p,
	/*! [in]  Document to copy. */
	IXML_Document *d);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* ACTIONCOMPLETE_H */

