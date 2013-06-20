
#ifndef CALLBACK_H
#define CALLBACK_H

/*!
 * \file
 */

/*!
 * \brief The reason code for an event callback.
 *
 * The \b Event parameter will be different depending on the reason for the
 * callback. The descriptions for each event type describe the contents of the
 * \b Event parameter.
 */
enum Upnp_EventType_e {
	/*
	 * Control callbacks
	 */

	/*! Received by a device when a control point issues a control
	 * request.  The \b Event parameter contains a pointer to a \b
	 * UpnpActionRequest structure containing the action.  The application
	 * stores the results of the action in this structure. */
	UPNP_CONTROL_ACTION_REQUEST,

	/*! A \b UpnpSendActionAsync call completed. The \b Event
	 * parameter contains a pointer to a \b UpnpActionComplete structure
	 * with the results of the action.  */
	UPNP_CONTROL_ACTION_COMPLETE,

	/*! Received by a device when a query for a single service variable
	 * arrives.  The \b Event parameter contains a pointer to a \b
	 * UpnpStateVarRequest structure containing the name of the variable
	 * and value.  */
	UPNP_CONTROL_GET_VAR_REQUEST,

	/*! A \b UpnpGetServiceVarStatus call completed. The \b Event
	 * parameter contains a pointer to a \b UpnpStateVarComplete structure
	 * containing the value for the variable.  */
	UPNP_CONTROL_GET_VAR_COMPLETE,

	/*
	 * Discovery callbacks
	 */

	/*! Received by a control point when a new device or service is available.  
	 * The \b Event parameter contains a pointer to a \b
	 * UpnpDiscovery structure with the information about the device
	 * or service.  */
	UPNP_DISCOVERY_ADVERTISEMENT_ALIVE,

	/*! Received by a control point when a device or service shuts down. The \b
	 * Event parameter contains a pointer to a \b UpnpDiscovery
	 * structure containing the information about the device or
	 * service.  */
	UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE,

	/*! Received by a control point when a matching device or service responds.
	 * The \b Event parameter contains a pointer to a \b
	 * UpnpDiscovery structure containing the information about
	 * the reply to the search request.  */
	UPNP_DISCOVERY_SEARCH_RESULT,

	/*! Received by a control point when the search timeout expires.  The
	 * SDK generates no more callbacks for this search after this 
	 * event.  The \b Event parameter is \c NULL.  */
	UPNP_DISCOVERY_SEARCH_TIMEOUT,

	/*
	 * Eventing callbacks
	 */

	/*! Received by a device when a subscription arrives.
	 * The \b Event parameter contains a pointer to a \b
	 * UpnpSubscriptionRequest structure.  At this point, the
	 * subscription has already been accepted.  \b UpnpAcceptSubscription
	 * needs to be called to confirm the subscription and transmit the
	 * initial state table.  This can be done during this callback.  The SDK
	 * generates no events for a subscription unless the device 
	 * application calls \b UpnpAcceptSubscription.
	 */
	UPNP_EVENT_SUBSCRIPTION_REQUEST,

	/*! Received by a control point when an event arrives. The \b
	 * Event parameter contains a \b UpnpEvent structure
	 * with the information about the event.  */
	UPNP_EVENT_RECEIVED,

	/*! A \b UpnpRenewSubscriptionAsync call completed. The status of
	 * the renewal is in the \b Event parameter as a \b
	 * Upnp_Event_Subscription structure.  */
	UPNP_EVENT_RENEWAL_COMPLETE,

	/*! A \b UpnpSubscribeAsync call completed. The status of the
	 * subscription is in the \b Event parameter as a \b
	 * Upnp_Event_Subscription structure.  */
	UPNP_EVENT_SUBSCRIBE_COMPLETE,

	/*! A \b UpnpUnSubscribeAsync call completed. The status of the
	 * subscription is in the \b Event parameter as a \b
	 * UpnpEventSubscribe structure.  */
	UPNP_EVENT_UNSUBSCRIBE_COMPLETE,

	/*! The auto-renewal of a client subscription failed.   
	 * The \b Event parameter is a \b UpnpEventSubscribe structure 
	 * with the error code set appropriately. The subscription is no longer 
	 * valid. */
	UPNP_EVENT_AUTORENEWAL_FAILED,

	/*! A client subscription has expired. This will only occur 
	 * if auto-renewal of subscriptions is disabled.
	 * The \b Event parameter is a \b UpnpEventSubscribe
	 * structure. The subscription is no longer valid. */
	UPNP_EVENT_SUBSCRIPTION_EXPIRED
};

typedef enum Upnp_EventType_e Upnp_EventType;

/*!
 *  All callback functions share the same prototype, documented below.
 *  Note that any memory passed to the callback function
 *  is valid only during the callback and should be copied if it
 *  needs to persist.  This callback function needs to be thread
 *  safe.  The context of the callback is always on a valid thread 
 *  context and standard synchronization methods can be used.  Note, 
 *  however, because of this the callback cannot call SDK functions
 *  unless explicitly noted.
 *
 *  \verbatim
      int CallbackFxn(Upnp_EventType EventType, void *Event, void *Cookie);
    \endverbatim 
 *
 *  where \b EventType is the event that triggered the callback, 
 *  \b Event is a structure that denotes event-specific information for that
 *  event, and \b Cookie is the user data passed when the callback was
 *  registered.
 *
 *  See \b Upnp_EventType for more information on the callback values and
 *  the associated \b Event parameter.  
 *
 *  The return value of the callback is currently ignored. It may be used
 *  in the future to communicate results back to the SDK.
 */
typedef int (*Upnp_FunPtr)(
	/*! [in] .*/
	Upnp_EventType EventType,
	/*! [in] .*/
	const void *Event,
	/*! [in] .*/
	void *Cookie);

#endif /* CALLBACK_H */
