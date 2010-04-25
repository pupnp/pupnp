
#ifndef CLIENT_TABLE_H
#define CLIENT_TABLE_H


/*!
 * \file
 */


#ifdef __cplusplus
extern "C" {
#endif


#include "service_table.h"
#include "upnp.h"
#include "UpnpString.h"
#include "upnp_timeout.h"
#include "uri.h"
#include "TimerThread.h"


#include <stdio.h>
#include <stdlib.h>
#include <time.h>


extern TimerThread gTimerThread;


#ifdef INCLUDE_CLIENT_APIS


typedef struct s_ClientSubscription ClientSubscription;


/*!
 * \brief Constructor.
 */
ClientSubscription *UpnpClientSubscription_new();


/*!
 * \brief Destructor.
 */
void UpnpClientSubscription_delete(
	/*! [in] The \b this pointer. */
	ClientSubscription *p);


/*!
 * \brief Copy Constructor.
 */
ClientSubscription *UpnpClientSubscription_dup(
	/*! [in] The \b this pointer. */
	const ClientSubscription *p);


/*!
 * \brief Assignment operator.
 */
void UpnpClientSubscription_assign(
	/*! [in] The \b this pointer. */
	ClientSubscription *q,
	const ClientSubscription *p);


/*!
 * \brief 
 */
int UpnpClientSubscription_get_RenewEventId(
	/*! [in] The \b this pointer. */
	const ClientSubscription *p);


/*!
 * \brief 
 */
void UpnpClientSubscription_set_RenewEventId(
	/*! [in] The \b this pointer. */
	ClientSubscription *p,
	/*! [in] . */
	int n);


/*!
 * \brief 
 */
const UpnpString *UpnpClientSubscription_get_SID(
	/*! [in] The \b this pointer. */
	const ClientSubscription *p);


/*!
 * \brief 
 */
void UpnpClientSubscription_set_SID(
	/*! [in] The \b this pointer. */
	ClientSubscription *p,
	const UpnpString *s);


/*!
 * \brief 
 */
void UpnpClientSubscription_strcpy_SID(
	/*! [in] The \b this pointer. */
	ClientSubscription *p,
	const char *s);


/*!
 * \brief 
 */
const UpnpString *UpnpClientSubscription_get_ActualSID(
	/*! [in] The \b this pointer. */
	const ClientSubscription *p);


/*!
 * \brief 
 */
void UpnpClientSubscription_set_ActualSID(
	/*! [in] The \b this pointer. */
	ClientSubscription *p,
	const UpnpString *s);


/*!
 * \brief 
 */
void UpnpClientSubscription_strcpy_ActualSID(
	/*! [in] The \b this pointer. */
	ClientSubscription *p,
	const char *s);


/*!
 * \brief 
 */
const UpnpString *UpnpClientSubscription_get_EventURL(
	/*! [in] The \b this pointer. */
	const ClientSubscription *p);


/*!
 * \brief 
 */
void UpnpClientSubscription_set_EventURL(
	/*! [in] The \b this pointer. */
	ClientSubscription *p,
	const UpnpString *s);


/*!
 * \brief 
 */
void UpnpClientSubscription_strcpy_EventURL(
	/*! [in] The \b this pointer. */
	ClientSubscription *p,
	const char *s);


/*!
 * \brief 
 */
ClientSubscription *UpnpClientSubscription_get_Next(
	/*! [in] The \b this pointer. */
	const ClientSubscription *p);


/*!
 * \brief 
 */
void UpnpClientSubscription_set_Next(
	/*! [in] The \b this pointer. */
	ClientSubscription *p,
	ClientSubscription *q);



/*!
 * \brief Free memory allocated for client subscription data.
 *
 * Remove timer thread associated with this subscription event.
 */
void free_client_subscription(
	/*! [in] Client subscription to be freed. */
	ClientSubscription *sub);


/*!
 * \brief Free the client subscription table.
 */
void freeClientSubList(
	/*! [in] Client subscription list to be freed. */
	ClientSubscription *list);


/*!
 * \brief Remove the client subscription matching the subscritpion id
 * represented by the const Upnp_SID sid parameter from the table and
 * update the table.
 */
void RemoveClientSubClientSID(
	/*! [in] Head of the subscription list. */
	ClientSubscription **head,
	/*! [in] Subscription ID to be mactched. */
	const UpnpString *sid);


/*!
 * \brief Return the client subscription from the client table that matches
 * const Upnp_SID sid subscrition id value.
 *
 * \return The matching subscription.
 */
ClientSubscription *GetClientSubClientSID(
	/*! [in] Head of the subscription list. */
	ClientSubscription *head,
	/*! [in] Subscription ID to be mactched. */
	const UpnpString *sid);


/*!
 * \brief Returns the client subscription from the client subscription table
 * that has the matching token *sid buffer value.
 *
 * \return The matching subscription.
 */
ClientSubscription *GetClientSubActualSID(
	/*! [in] Head of the subscription list. */
	ClientSubscription *head,
	/*! [in] Subscription ID to be mactched. */
	token *sid);


#endif /* INCLUDE_CLIENT_APIS */


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* CLIENT_TABLE_H */

