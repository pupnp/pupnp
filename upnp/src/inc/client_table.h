///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000-2003 Intel Corporation 
// All rights reserved. 
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions are met: 
//
// * Redistributions of source code must retain the above copyright notice, 
// this list of conditions and the following disclaimer. 
// * Redistributions in binary form must reproduce the above copyright notice, 
// this list of conditions and the following disclaimer in the documentation 
// and/or other materials provided with the distribution. 
// * Neither name of Intel Corporation nor the names of its contributors 
// may be used to endorse or promote products derived from this software 
// without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR 
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY 
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////


#ifndef CLIENT_TABLE_H
#define CLIENT_TABLE_H


#ifdef __cplusplus
extern "C" {
#endif


#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#include "service_table.h"
#include "UpnpString.h"
#include "TimerThread.h"
#include "upnp.h"
#include "upnp_timeout.h"
#include "uri.h"


extern TimerThread gTimerThread;


#ifdef INCLUDE_CLIENT_APIS


typedef struct {} ClientSubscription;


/** Constructor */
ClientSubscription *UpnpClientSubscription_new();

/** Destructor */
void UpnpClientSubscription_delete(ClientSubscription *p);

/** Copy Constructor */
ClientSubscription *UpnpClientSubscription_dup(const ClientSubscription *p);

/** Assignment operator */
void UpnpClientSubscription_assign(ClientSubscription *q, const ClientSubscription *p);

/*  */
int UpnpClientSubscription_get_RenewEventId(const ClientSubscription *p);
void UpnpClientSubscription_set_RenewEventId(ClientSubscription *p, int n);

/*  */
const UpnpString *UpnpClientSubscription_get_SID(const ClientSubscription *p);
void UpnpClientSubscription_set_SID(ClientSubscription *p, const UpnpString *s);
void UpnpClientSubscription_strcpy_SID(ClientSubscription *p, const char *s);

/*  */
const UpnpString *UpnpClientSubscription_get_ActualSID(const ClientSubscription *p);
void UpnpClientSubscription_set_ActualSID(ClientSubscription *p, const UpnpString *s);
void UpnpClientSubscription_strcpy_ActualSID(ClientSubscription *p, const char *s);

/*  */
const UpnpString *UpnpClientSubscription_get_EventURL(const ClientSubscription *p);
void UpnpClientSubscription_set_EventURL(ClientSubscription *p, const UpnpString *s);
void UpnpClientSubscription_strcpy_EventURL(ClientSubscription *p, const char *s);

/*  */
ClientSubscription *UpnpClientSubscription_get_Next(const ClientSubscription *p);
void UpnpClientSubscription_set_Next(ClientSubscription *p, ClientSubscription *q);



/************************************************************************
 * Function: free_client_subscription
 *
 * Parameters:
 *	ClientSubscription *sub;	- Client subscription to be freed
 *
 * Description: Free memory allocated for client subscription data.
 *	Remove timer thread associated with this subscription event.
 ************************************************************************/
void free_client_subscription(ClientSubscription * sub);


/************************************************************************
 * Function: freeClientSubList
 *
 * Parameters:
 *	ClientSubscription *list;	Client subscription 
 *
 * Description: Free the client subscription table.
 *
 * Return: void
 ************************************************************************/
void freeClientSubList(ClientSubscription *list);


/************************************************************************
 * Function: RemoveClientSubClientSID
 *
 * Parameters:
 *	ClientSubscription **head;	Head of the subscription list	
 *	const UpnpString sid;		Subscription ID to be mactched
 *
 * Description: Remove the client subscription matching the 
 *	subscritpion id represented by the const Upnp_SID sid parameter 
 *	from the table and update the table.
 *
 * Return: void
 ************************************************************************/
void RemoveClientSubClientSID(ClientSubscription **head, const UpnpString *sid);


/************************************************************************
 * Function: GetClientSubClientSID
 *
 * Parameters:
 *	ClientSubscription *head;	Head of the subscription list	
 *	const UpnpString *sid;		Subscription ID to be matched
 *
 * Description: Return the client subscription from the client table 
 *	that matches const Upnp_SID sid subscrition id value. 
 *
 * Return: ClientSubscription *	The matching subscription
 ************************************************************************/
ClientSubscription *GetClientSubClientSID(
	ClientSubscription *head,
	const UpnpString *sid);


/************************************************************************
 * Function: GetClientSubActualSID
 *
 * Parameters:
 *	ClientSubscription *head;	Head of the subscription list		
 *	token *sid;			Subscription ID to be matched
 *
 * Description: Returns the client subscription from the client 
 *	subscription table that has the matching token *sid buffer value.
 *
 * Return: ClientSubscription *;	The matching subscription
 ************************************************************************/
ClientSubscription *GetClientSubActualSID(ClientSubscription *head, token *sid);


#endif /* INCLUDE_CLIENT_APIS */


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* CLIENT_TABLE_H */

