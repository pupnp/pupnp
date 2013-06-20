/*******************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation 
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met: 
 *
 * * Redistributions of source code must retain the above copyright notice, 
 * this list of conditions and the following disclaimer. 
 * * Redistributions in binary form must reproduce the above copyright notice, 
 * this list of conditions and the following disclaimer in the documentation 
 * and/or other materials provided with the distribution. 
 * * Neither name of Intel Corporation nor the names of its contributors 
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


#ifndef UPNP_TOOLS_H
#define UPNP_TOOLS_H


/*!
 * \file
 *
 * \defgroup UPnPTools Optional Tool API
 *
 * \brief Additional, optional utility API that can be helpful in writing
 * applications.
 *
 * This additional API can be compiled out in order to save code size in the
 * library. Refer to the file README for details.
 *
 * @{
 */


#include "ixml.h" /* for IXML_Document */
#include "upnpconfig.h"	/* for UPNP_HAVE_TOOLS */


/* Function declarations only if tools compiled into the library */
#if UPNP_HAVE_TOOLS


#ifdef __cplusplus
extern "C" {
#endif


/*!
 * \brief Converts an SDK error code into a string error message suitable for
 * display. The memory returned from this function should NOT be freed.
 *
 * \return An ASCII text string representation of the error message associated
 * 	with the error code or the string "Unknown error code"
 */
EXPORT_SPEC const char *UpnpGetErrorMessage(
	/*! [in] The SDK error code to convert. */
	int errorcode);


/*!
 * \brief Combines a base URL and a relative URL into a single absolute URL.
 *
 * The memory for \b AbsURL needs to be allocated by the caller and must
 * be large enough to hold the \b BaseURL and \b RelURL combined.
 *
 * \return An integer representing one of the following:
 *	\li <tt>UPNP_E_SUCCESS</tt>: The operation completed successfully.
 *	\li <tt>UPNP_E_INVALID_PARAM</tt>: \b RelURL is <tt>NULL</tt>.
 *	\li <tt>UPNP_E_INVALID_URL</tt>: The \b BaseURL / \b RelURL 
 *              combination does not form a valid URL.
 *	\li <tt>UPNP_E_OUTOF_MEMORY</tt>: Insufficient resources exist to 
 *              complete this operation.
 */
EXPORT_SPEC int UpnpResolveURL(
	/*! [in] The base URL to combine. */
	const char *BaseURL,
	/*! [in] The relative URL to \b BaseURL. */
	const char *RelURL,
	/*! [out] A pointer to a buffer to store the absolute URL. */
	char *AbsURL);


/*!
 * \brief Combines a base URL and a relative URL into a single absolute URL.
 *
 * The memory for \b AbsURL becomes owned by the caller and should be freed
 * later.
 *
 * \return An integer representing one of the following:
 *	\li <tt>UPNP_E_SUCCESS</tt>: The operation completed successfully.
 *	\li <tt>UPNP_E_INVALID_PARAM</tt>: \b RelURL is <tt>NULL</tt>.
 *	\li <tt>UPNP_E_INVALID_URL</tt>: The \b BaseURL / \b RelURL 
 *              combination does not form a valid URL.
 *	\li <tt>UPNP_E_OUTOF_MEMORY</tt>: Insufficient resources exist to 
 *              complete this operation.
 */
EXPORT_SPEC int UpnpResolveURL2(
	/*! [in] The base URL to combine. */
	const char *BaseURL,
	/*! [in] The relative URL to \b BaseURL. */
	const char *RelURL,
	/*! [out] A pointer to a pointer to a buffer to store the
	 * absolute URL. Must be freed later by the caller. */
	char **AbsURL);


/*!
 * \brief Creates an action request packet based on its input parameters
 * (status variable name and value pair).
 *
 * Any number of input parameters can be passed to this function but every
 * input variable name should have a matching value argument. 
 *
 * It is a wrapper function that calls makeAction() function to create the
 * action request.
 * 
 * \return The action node of \b Upnp_Document type or <tt>NULL</tt> if the
 * 	operation failed.
 */
EXPORT_SPEC IXML_Document *UpnpMakeAction(
	/*! [in] Name of the action request or response. */
	const char *ActionName,
	/*! [in] The service type. */
	const char *ServType,
	/*! [in] Number of argument pairs to be passed. */
	int NumArg,
	/*! [in] pointer to the first argument. */
	const char *Arg,
	/*! [in] Argument list. */
	...);


/*!
 * \brief Ceates an action response packet based on its output parameters
 * (status variable name and value pair).
 *
 * Any number of input parameters can be passed to this function but every
 * output variable name should have a matching value argument. 
 *   
 * It is a wrapper function that calls makeAction() function to create the
 * action request.
 * 
 * \return The action node of \b Upnp_Document type or <tt>NULL</tt> if the
 * 	operation failed.
 */
EXPORT_SPEC IXML_Document *UpnpMakeActionResponse(
	/*! [in] The action name. */
	const char *ActionName,
	/*! [in] The service type.. */
	const char *ServType,
	/*! [in] The number of argument pairs passed. */
	int NumArg,
	/*! [in] The status variable name and value pair. */
	const char *Arg,
	/*! [in] Other status variable name and value pairs. */
	...);


/*!
 * \brief Adds the argument in the action request.
 * 
 * This API is specially suitable inside a loop to add any number input
 * parameters into an existing action. If no action document exists in the
 * beginning then a <b>Upnp_Document variable initialized with <tt>NULL</tt></b>
 * should be passed as a parameter.
 *
 * It is a wrapper function that calls addToAction() function to add the
 * argument in the action request.
 * 
 * \return An integer representing one of the following:
 *	\li <tt>UPNP_E_SUCCESS</tt>: The operation completed successfully.
 *	\li <tt>UPNP_E_INVALID_PARAM</tt>: One or more of the parameters are invalid.
 *	\li <tt>UPNP_E_OUTOF_MEMORY</tt>: Insufficient resources exist to
 *		complete this operation.
 */
EXPORT_SPEC int UpnpAddToAction(
	/*! [in,out] A pointer to store the action document node. */
	IXML_Document **ActionDoc,
	/*! [in] The action name. */
	const char *ActionName,
	/*! [in] The service type. */
	const char *ServType,
	/*! [in] The status variable name. */
	const char *ArgName,
	/*! [in] The status variable value. */
	const char *ArgVal);


/*!
 * \brief Creates an action response packet based on its output parameters
 * (status variable name and value pair).
 *
 * This API is especially suitable inside a loop to add any number of input
 * parameters into an existing action response. If no action document exists
 * in the beginning, a \b Upnp_Document variable initialized with <tt>NULL</tt>
 * should be passed as a parameter.
 *
 * It is a wrapper function that calls addToAction() function to add the
 * argument in the action request.
 * 
 * \return An integer representing one of the following:
 *	\li <tt>UPNP_E_SUCCESS</tt>: The operation completed successfully.
 *	\li <tt>UPNP_E_INVALID_PARAM</tt>: One or more of the parameters are invalid.
 *	\li <tt>UPNP_E_OUTOF_MEMORY</tt>: Insufficient resources exist to
 *		complete this operation.
 */
EXPORT_SPEC int UpnpAddToActionResponse(
	/*! [in,out] Pointer to a document to store the action document node. */
        IXML_Document **ActionResponse, 
	/*! [in] The action name. */
        const char *ActionName,
	/*! [in] The service type. */
        const char *ServType,
	/*! [in] The status variable name. */
        const char *ArgName,
	/*! [in] The status variable value. */
        const char *ArgVal);


/*!
 * \brief Creates a property set message packet.
 *
 * Any number of input parameters can be passed to this function but every
 * input variable name should have a matching value input argument.
 *  
 * \return <tt>NULL</tt> on failure, or the property-set document node.
 */
EXPORT_SPEC IXML_Document *UpnpCreatePropertySet(
	/*! [in] The number of argument pairs passed. */
	int NumArg,
	/*! [in] The status variable name and value pair. */
	const char *Arg,
	/*! [in] Variable sized list with the rest of the parameters. */
	...);


/*!
 * \brief Can be used when an application needs to transfer the status of many
 * variables at once.
 *
 * It can be used (inside a loop) to add some extra status variables into an
 * existing property set. If the application does not already have a property
 * set document, the application should create a variable initialized with
 * <tt>NULL</tt> and pass that as the first parameter.
 *  
 * \return An integer representing one of the following:
 *	\li <tt>UPNP_E_SUCCESS</tt>: The operation completed successfully.
 *	\li <tt>UPNP_E_INVALID_PARAM</tt>: One or more of the parameters are invalid.
 *	\li <tt>UPNP_E_OUTOF_MEMORY</tt>: Insufficient resources exist to
 *		complete this operation.
 */
EXPORT_SPEC int UpnpAddToPropertySet(
	/*! [in,out] A pointer to the document containing the property set document node. */
	IXML_Document **PropSet,
	/*! [in] The status variable name. */
	const char *ArgName,
	/*! [in] The status variable value. */
	const char *ArgVal);


#ifdef __cplusplus
}
#endif


/*! @} */


#endif /* UPNP_HAVE_TOOLS */


#endif /* UPNP_TOOLS_H */

