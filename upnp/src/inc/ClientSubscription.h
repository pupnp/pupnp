
#ifndef CLIENTSUBSCRIPTION_H
#define CLIENTSUBSCRIPTION_H


/*!
 * \file
 */


#ifdef __cplusplus
extern "C" {
#endif


#include "UpnpString.h"


#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#ifdef INCLUDE_CLIENT_APIS


typedef struct s_ClientSubscription ClientSubscription;


/*!
 * \brief Constructor.
 */
ClientSubscription *GenlibClientSubscription_new();


/*!
 * \brief Destructor.
 */
void GenlibClientSubscription_delete(
	/*! [in] The \b this pointer. */
	ClientSubscription *p);


/*!
 * \brief Copy Constructor.
 */
ClientSubscription *GenlibClientSubscription_dup(
	/*! [in] The \b this pointer. */
	const ClientSubscription *p);


/*!
 * \brief Assignment operator.
 */
void GenlibClientSubscription_assign(
	/*! [in] The \b this pointer. */
	ClientSubscription *q,
	const ClientSubscription *p);


/*!
 * \brief 
 */
int GenlibClientSubscription_get_RenewEventId(
	/*! [in] The \b this pointer. */
	const ClientSubscription *p);


/*!
 * \brief 
 */
void GenlibClientSubscription_set_RenewEventId(
	/*! [in] The \b this pointer. */
	ClientSubscription *p,
	/*! [in] . */
	int n);


/*!
 * \brief 
 */
const UpnpString *GenlibClientSubscription_get_SID(
	/*! [in] The \b this pointer. */
	const ClientSubscription *p);


/*!
 * \brief 
 */
const char *GenlibClientSubscription_get_SID_cstr(
	/*! [in] The \b this pointer. */
	const ClientSubscription *p);


/*!
 * \brief 
 */
void GenlibClientSubscription_set_SID(
	/*! [in] The \b this pointer. */
	ClientSubscription *p,
	const UpnpString *s);


/*!
 * \brief 
 */
void GenlibClientSubscription_strcpy_SID(
	/*! [in] The \b this pointer. */
	ClientSubscription *p,
	const char *s);


/*!
 * \brief 
 */
const UpnpString *GenlibClientSubscription_get_ActualSID(
	/*! [in] The \b this pointer. */
	const ClientSubscription *p);


/*!
 * \brief 
 */
const char *GenlibClientSubscription_get_ActualSID_cstr(
	/*! [in] The \b this pointer. */
	const ClientSubscription *p);


/*!
 * \brief 
 */
void GenlibClientSubscription_set_ActualSID(
	/*! [in] The \b this pointer. */
	ClientSubscription *p,
	const UpnpString *s);


/*!
 * \brief 
 */
void GenlibClientSubscription_strcpy_ActualSID(
	/*! [in] The \b this pointer. */
	ClientSubscription *p,
	const char *s);


/*!
 * \brief 
 */
const UpnpString *GenlibClientSubscription_get_EventURL(
	/*! [in] The \b this pointer. */
	const ClientSubscription *p);


/*!
 * \brief 
 */
void GenlibClientSubscription_set_EventURL(
	/*! [in] The \b this pointer. */
	ClientSubscription *p,
	const UpnpString *s);


/*!
 * \brief 
 */
void GenlibClientSubscription_strcpy_EventURL(
	/*! [in] The \b this pointer. */
	ClientSubscription *p,
	const char *s);


/*!
 * \brief 
 */
ClientSubscription *GenlibClientSubscription_get_Next(
	/*! [in] The \b this pointer. */
	const ClientSubscription *p);


/*!
 * \brief 
 */
void GenlibClientSubscription_set_Next(
	/*! [in] The \b this pointer. */
	ClientSubscription *p,
	ClientSubscription *q);


#endif /* INCLUDE_CLIENT_APIS */


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* CLIENTSUBSCRIPTION_H */

