/*******************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation 
 * All rights reserved. 
 * Copyright (c) 2012 France Telecom All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met: 
 *
 * - Redistributions of source code must retain the above copyright notice, 
 * this list of conditions and the following disclaimer. 
 * - Redistributions in binary form must reproduce the above copyright notice, 
 * this list of conditions and the following disclaimer in the documentation 
 * and/or other materials provided with the distribution. 
 * - Neither name of Intel Corporation nor the names of its contributors 
 * may be used to endorse or promote products derived from this software 
 * without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR 
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY 
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

#ifndef SERVICE_TABLE_H
#define SERVICE_TABLE_H

/*!
 * \file
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "uri.h"
#include "ixml.h"
#include "upnp.h"
#include "upnpdebug.h"

#include <stdio.h>
#include <time.h>

#define SID_SIZE  (size_t)41

#ifdef INCLUDE_DEVICE_APIS

typedef struct SUBSCRIPTION {
	Upnp_SID sid;
	int eventKey;
	int ToSendEventKey;
	time_t expireTime;
	int active;
	URL_list DeliveryURLs;
	struct SUBSCRIPTION *next;
} subscription;

typedef struct SERVICE_INFO {
	DOMString	serviceType;
	DOMString	serviceId;
	char		*SCPDURL ;
	char		*controlURL;
	char		*eventURL;
	DOMString	UDN;
	int		active;
	int		TotalSubscriptions;
	subscription	*subscriptionList;
	struct SERVICE_INFO	 *next;
} service_info;

typedef struct SERVICE_TABLE {
	DOMString URLBase;
	service_info *serviceList;
	service_info *endServiceList;
} service_table;

/* Functions for Subscriptions */

/*!
 * \brief Makes a copy of the subscription.
 *
 * \return HTTP_SUCCESS on success.
 */
int copy_subscription(
	/*! [in] Source subscription. */
	subscription *in,
	/*! [in] Destination subscription. */
	subscription *out);

/*
 * \brief Remove the subscription represented by the const Upnp_SID sid parameter
 * from the service table and update the service table.
 */
void RemoveSubscriptionSID(
	/*! [in] Subscription ID. */
	Upnp_SID sid,
	/*! [in] Service object providing the list of subscriptions. */
	service_info *service);

/*!
 * \brief Return the subscription from the service table that matches
 * const Upnp_SID sid value.
 *
 * \return Pointer to the matching subscription node.
 */
subscription *GetSubscriptionSID(
	/*! [in] Subscription ID. */
	const Upnp_SID sid,
	/*! [in] Service object providing the list of subscriptions. */
	service_info *service); 

/*!
 * \brief Gets pointer to the first subscription node in the service table.
 *
 * \return Pointer to the first subscription node.
 */
subscription *GetFirstSubscription(
	/*! [in] Service object providing the list of subscriptions. */
	service_info *service);

/*!
 * \brief Get current and valid subscription from the service table.
 *
 * \return Pointer to the next subscription node.
 */
subscription *GetNextSubscription(
	/*! [in] Service object providing the list of subscriptions. */
	service_info *service,
	/*! [in] Current subscription object. */
	subscription *current);

/*!
 * \brief Free's the memory allocated for storing the URL of the subscription.
 */
void freeSubscription(
	/*! [in] Subscription object to be freed. */
	subscription *sub);

/*!
 * \brief Free's memory allocated for all the subscriptions in the service table.
 */
void freeSubscriptionList(
	/*! [in] Head of the subscription list. */
	subscription * head);

/*!
 * \brief Traverses through the service table and returns a pointer to the
 * service node that matches a known service id and a known UDN.
 *
 * \return Pointer to the matching service_info node.
 */
service_info *FindServiceId(
	/*! [in] Service table. */
	service_table *table, 
	/*! [in] String representing the service id to be found among those
	 * in the table. */
	const char *serviceId,
	/*! [in] String representing the UDN to be found among those in the
	 * table. */
	const char *UDN);

/*!
 * \brief Traverses the service table and finds the node whose event URL Path
 * matches a know value.
 *
 * \return Pointer to the service list node from the service table whose event
 * URL matches a known event URL.
 */
service_info *FindServiceEventURLPath(
	/*! [in] Service table. */
	service_table *table,
	/*! [in] Event URL path used to find a service from the table. */
	char *eventURLPath);

/*!
 * \brief Traverses the service table and finds the node whose control URL Path
 * matches a know value.
 *
 * \return Pointer to the service list node from the service table whose control
 * URL Path matches a known value.
 */
service_info * FindServiceControlURLPath(
	/*! [in] Service table. */
	service_table *table,
	/*! [in] Control URL path used to find a service from the table. */
	const char *controlURLPath);

/*!
 * \brief For debugging purposes prints information from the service passed
 * into the function.
 */
#ifdef DEBUG
void printService(
	/*! [in] Service whose information is to be printed. */
	service_info *service,
	/*! [in] Debug level specified to the print function. */
	Upnp_LogLevel level,
	/*! [in] Debug module specified to the print function. */
	Dbg_Module module);
#else
static UPNP_INLINE void printService(
	service_info *service,
	Upnp_LogLevel level,
	Dbg_Module module)
{
	return;
	service = service;
	level = level;
	module = module;
}
#endif

/*!
 * \brief For debugging purposes prints information of each service from the
 * service table passed into the function.
 */
#ifdef DEBUG
void printServiceList(
	/*! [in] Service whose information is to be printed. */
	service_info *service,
	/*! [in] Debug level specified to the print function. */
	Upnp_LogLevel level,
	/*! [in] Debug module specified to the print function. */
	Dbg_Module module);
#else
static UPNP_INLINE void printServiceList(
	service_info *service,
	Upnp_LogLevel level,
	Dbg_Module module)
{
	return;
	service = service;
	level = level;
	module = module;
}
#endif

/*!
 * \brief For debugging purposes prints the URL base of the table and information
 * of each service from the service table passed into the function.
 */
#ifdef DEBUG
void printServiceTable(
	/*! [in] Service table to be printed. */
	service_table *table,
	/*! [in] Debug level specified to the print function. */
	Upnp_LogLevel level,
	/*! [in] Debug module specified to the print function. */
	Dbg_Module module);
#else
static UPNP_INLINE void printServiceTable(
	service_table *table,
	Upnp_LogLevel level,
	Dbg_Module module)
{
	return;
	table = table;
	level = level;
	module = module;
}
#endif

/*!
 * \brief Free's memory allocated for the various components of the service
 * entry in the service table.
 */
void freeService(
	/*! [in] Service information that is to be freed. */
	service_info *in);

/*!
 * \brief Free's memory allocated for the various components of each service
 * entry in the service table.
 */
void freeServiceList(
	/*! [in] Head of the service list to be freed. */
	service_info *head);

/*!
 * \brief Free's dynamic memory in table (does not free table, only memory
 * within the structure).
 */
void freeServiceTable(
	/*! [in] Service table whose internal memory needs to be freed. */
	service_table *table);

/*!
 * \brief This function assumes that services for a particular root device are
 * placed linearly in the service table, and in the order in which they are
 * found in the description document all services for this root device are
 * removed from the list.
 *
 * \return An integer.
 */
int removeServiceTable(
	/*! [in] XML node information. */
	IXML_Node *node,
	/*! [in] Service table from which services will be removed. */
	service_table *in);

/*!
 * \brief Add Service to the table.
 */
int addServiceTable(
	/*! [in] XML node information. */
	IXML_Node *node,
	/*! [in] Service table that will be initialized with services. */
	service_table *in,
	/*! [in] Default base URL on which the URL will be returned to the
	 * service list. */
	const char *DefaultURLBase);

/*!
 * \brief Retrieve service from the table.
 *
 * \return An integer
 */
int getServiceTable(
	/*! [in] XML node information. */
	IXML_Node *node,
	/*! [in] Output parameter which will contain the service list and URL. */
	service_table *out,
	/*! [in] Default base URL on which the URL will be returned. */
	const char *DefaultURLBase);

/*	Misc helper functions	*/

/*!
 * \brief Returns the clone of the element value.
 *
 * \note Value must be freed with DOMString_free.
 *
 * \return DOMString
 */
DOMString getElementValue(
	/*! [in] Input node which provides the list of child nodes. */
	IXML_Node *node);

/*!
 * \brief Traverses through a list of XML nodes to find the node with the
 * known element name.
 *
 * \return
 * 	\li 1 - On Success
 * 	\li 0 - On Failure
 */
int getSubElement(
	/*! [in] Sub element name to be searched for. */
	const char *element_name,
	/*! [in] Input node which provides the list of child nodes. */
	IXML_Node *node, 
	/*! [out] Ouput node to which the matched child node is returned. */
	IXML_Node **out);

#endif /* INCLUDE_DEVICE_APIS */

#ifdef __cplusplus
}
#endif

#endif /* SERVICE_TABLE */

