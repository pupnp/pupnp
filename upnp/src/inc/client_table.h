
#ifndef CLIENT_TABLE_H
#define CLIENT_TABLE_H


/*!
 * \file
 */


#ifdef __cplusplus
extern "C" {
#endif


#include "ClientSubscription.h"
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


/*!
 * \brief Free memory allocated for client subscription data.
 *
 * Remove timer thread associated with this subscription event.
 */
void free_client_subscription(
	/*! [in] Client subscription to be freed. */
	GenlibClientSubscription *sub);


/*!
 * \brief Free the client subscription table.
 */
void freeClientSubList(
	/*! [in] Client subscription list to be freed. */
	GenlibClientSubscription *list);


/*!
 * \brief Remove the client subscription matching the subscritpion id
 * represented by the const Upnp_SID sid parameter from the table and
 * update the table.
 */
void RemoveClientSubClientSID(
	/*! [in] Head of the subscription list. */
	GenlibClientSubscription **head,
	/*! [in] Subscription ID to be mactched. */
	const UpnpString *sid);


/*!
 * \brief Return the client subscription from the client table that matches
 * const Upnp_SID sid subscrition id value.
 *
 * \return The matching subscription.
 */
GenlibClientSubscription *GetClientSubClientSID(
	/*! [in] Head of the subscription list. */
	GenlibClientSubscription *head,
	/*! [in] Subscription ID to be mactched. */
	const UpnpString *sid);


/*!
 * \brief Returns the client subscription from the client subscription table
 * that has the matching token *sid buffer value.
 *
 * \return The matching subscription.
 */
GenlibClientSubscription *GetClientSubActualSID(
	/*! [in] Head of the subscription list. */
	GenlibClientSubscription *head,
	/*! [in] Subscription ID to be mactched. */
	token *sid);


#endif /* INCLUDE_CLIENT_APIS */


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* CLIENT_TABLE_H */

