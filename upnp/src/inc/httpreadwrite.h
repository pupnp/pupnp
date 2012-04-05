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

#ifndef GENLIB_NET_HTTP_HTTPREADWRITE_H
#define GENLIB_NET_HTTP_HTTPREADWRITE_H

/*
 * \file
 */

#include "config.h"
#include "upnputil.h"
#include "sock.h"
#include "httpparser.h"

/*! timeout in secs. */
#define HTTP_DEFAULT_TIMEOUT	30

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
struct tm *http_gmtime_r(const time_t *clock, struct tm *result);
#else
#define http_gmtime_r gmtime_r
#endif

int http_CancelHttpGet(IN void *Handle);

/*!
 * \brief Validates URL.
 *
 * \return
 * 	\li \c UPNP_E_INVALID_URL
 * 	\li \c UPNP_E_SUCCESS
 */
int http_FixUrl(
	/*! [in] URL to be validated and fixed. */
	uri_type *url,
	/*! [out] URL after being fixed. */
	uri_type *fixed_url);

/*!
 * \brief Parses URL and then validates URL.
 *
 * \return
 * 	\li \c UPNP_E_INVALID_URL
 * 	\li \c UPNP_E_SUCCESS
 */
int http_FixStrUrl(
	/*! [in] Character string as a URL. */
	const char *urlstr,
	/*! [in] Length of the character string. */
	size_t urlstrlen,
	/*! [out] Fixed and corrected URL. */
	uri_type *fixed_url);

/*!
 * \brief Gets destination address from URL and then connects to the
 * remote end.
 *
 * \return Socket descriptor on success, or on error:
 * 	\li \c UPNP_E_OUTOF_SOCKET
 * 	\li \c UPNP_E_SOCKET_CONNECT
 */
SOCKET http_Connect(
	/*! [in] URL containing destination information. */
	uri_type *destination_url,
	/*! [out] Fixed and corrected URL. */
	uri_type *url);


/************************************************************************
 * Function: http_RecvMessage
 *
 * Parameters:
 *	IN SOCKINFO *info;			Socket information object
 *	OUT http_parser_t* parser;		HTTP parser object
 *	IN http_method_t request_method;	HTTP request method
 *	IN OUT int* timeout_secs;		time out
 *	OUT int* http_error_code;		HTTP error code returned
 *
 * Description:
 *	Get the data on the socket and take actions based on the read data
 *	to modify the parser objects buffer. If an error is reported while
 *	parsing the data, the error code is passed in the http_errr_code
 *	parameter
 *
 * Returns:
 *	 UPNP_E_BAD_HTTPMSG
 * 	 UPNP_E_SUCCESS
 ************************************************************************/
int http_RecvMessage( IN SOCKINFO *info, OUT http_parser_t* parser,
		IN http_method_t request_method, 
		IN OUT int* timeout_secs,
		OUT int* http_error_code );


/*!
 * \brief Sends a message to the destination based on the format parameter.
 *
 * fmt types:
 * \li \c 'f': arg = "const char *" file name
 * \li \c 'b': arg1 = "const char *" mem_buffer; arg2 = "size_t" buffer length.
 * \li \c 'I': arg = "struct SendInstruction *"
 *
 * E.g.:
 \verbatim
 	char *buf = "POST /xyz.cgi http/1.1\r\n\r\n";
 	char *filename = "foo.dat";
 	int status = http_SendMessage(tcpsock, "bf",
 		buf, strlen(buf),	// args for memory buffer
 		filename);		// arg for file
 \endverbatim
 *
 * \return
 * \li \c UPNP_E_OUTOF_MEMORY
 * \li \c UPNP_E_FILE_READ_ERROR
 * \li \c UPNP_E_SUCCESS
 */
int http_SendMessage(
	/* [in] Socket information object. */
	SOCKINFO *info,
	/* [in,out] Time out value. */
	int* timeout_secs, 
	/* [in] Pattern format to take actions upon. */
	const char* fmt,
	/* [in] Variable parameter list. */
	...);

/************************************************************************
 * Function: http_RequestAndResponse
 *
 * Parameters:
 *	IN uri_type* destination;	Destination URI object which contains
 *					remote IP address among other elements
 *	IN const char* request;		Request to be sent
 *	IN size_t request_length;	Length of the request
 *	IN http_method_t req_method;	HTTP Request method
 *	IN int timeout_secs;		time out value
 *	OUT http_parser_t* response;	Parser object to receive the repsonse
 *
 * Description:
 *	Initiates socket, connects to the destination, sends a
 *	request and waits for the response from the remote end
 *
 * Returns:
 *	UPNP_E_SOCKET_ERROR
 * 	UPNP_E_SOCKET_CONNECT
 *	Error Codes returned by http_SendMessage
 *	Error Codes returned by http_RecvMessage
 ************************************************************************/
int http_RequestAndResponse(
	IN uri_type* destination,
	IN const char* request,
	IN size_t request_length,
	IN http_method_t req_method,
	IN int timeout_secs, 
	OUT http_parser_t* response );


/************************************************************************
 * return codes:
 *	0 -- success
 *	UPNP_E_OUTOF_MEMORY
 *	UPNP_E_TIMEDOUT
 *	UPNP_E_BAD_REQUEST
 *	UPNP_E_BAD_RESPONSE
 *	UPNP_E_INVALID_URL
 *	UPNP_E_SOCKET_READ
 *	UPNP_E_SOCKET_WRITE
 ************************************************************************/


/************************************************************************
 * Function: http_Download
 *
 * Parameters:
 *	IN const char* url_str;	String as a URL
 *	IN int timeout_secs;	time out value
 *	OUT char** document;	buffer to store the document extracted
 *				from the donloaded message.
 *	OUT size_t* doc_length;	length of the extracted document
 *	OUT char* content_type;	Type of content
 *
 * Description:
 *	Download the document message and extract the document 
 *	from the message.
 *
 * Return: int
 *	UPNP_E_SUCCESS
 *	UPNP_E_INVALID_URL
 ************************************************************************/
int http_Download(
	IN const char* url, 
	IN int timeout_secs,
	OUT char** document,
	OUT size_t *doc_length,
	OUT char* content_type );


/************************************************************************
 * Function: http_HttpGetProgress
 *
 * Parameters:
 *	IN void *Handle;	Handle to the HTTP get object
 *	OUT size_t *length;	Buffer to get the read and parsed data
 *	OUT size_t *total;	Size of tge buffer passed
 *
 * Description:
 *	Extracts information from the Handle to the HTTP get object.
 *
 * Return: int
 *	UPNP_E_SUCCESS		- On Sucess
 *	UPNP_E_INVALID_PARAM	- Invalid Parameter
 ************************************************************************/
int http_HttpGetProgress(
	IN void *Handle,
	OUT size_t *length,
	OUT size_t *total);


/*!
 * \brief Opens a connection to the server.
 *
 * The SDK allocates the memory for \b handle, the
 * application is responsible for freeing this memory.
 *
 *  \return An integer representing one of the following:
 *      \li \c UPNP_E_SUCCESS: The operation completed successfully.
 *      \li \c UPNP_E_INVALID_PARAM: Either \b url, or \b handle
 *              is not a valid pointer.
 *      \li \c UPNP_E_INVALID_URL: The \b url is not a valid
 *              URL.
 *      \li \c UPNP_E_OUTOF_MEMORY: Insufficient resources exist to
 *              download this file.
 *      \li \c UPNP_E_SOCKET_ERROR: Error occured allocating a socket and
 *		resources or an error occurred binding a socket.
 *      \li \c UPNP_E_SOCKET_WRITE: An error or timeout occurred writing
 *              to a socket.
 *      \li \c UPNP_E_SOCKET_CONNECT: An error occurred connecting a
 *              socket.
 *      \li \c UPNP_E_OUTOF_SOCKET: Too many sockets are currently
 *              allocated.
 */
EXPORT_SPEC int http_OpenHttpConnection(
	/*! [in] The URL which contains the host, and the scheme to make the connection. */
	const char *url,
	/*! [in,out] A pointer in which to store the handle for this connection. This
	 * handle is required for futher operations over this connection. */
	void **handle,
	/*! [in] The time out value sent with the request during which a response
	 * is expected from the receiver, failing which, an error is reported.
	 * If value is negative, timeout is infinite. */
	int timeout);


/*!
 * \brief Makes a HTTP request using a connection previously created by
 * \b UpnpOpenHttpConnection.
 *
 * \note Trying to make another request while a request is already being processed
 * results in undefined behavior. It's up to the user to end a previous
 * request by calling \b UpnpEndHttpRequest.
 *
 *  \return An integer representing one of the following:
 *      \li \c UPNP_E_SUCCESS: The operation completed successfully.
 *      \li \c UPNP_E_INVALID_PARAM: Either \b url, \b handle
 *              or \b contentType is not a valid pointer.
 *      \li \c UPNP_E_INVALID_URL: The \b url is not a valid
 *              URL.
 *      \li \c UPNP_E_OUTOF_MEMORY: Insufficient resources exist to
 *              download this file.
 *      \li \c UPNP_E_SOCKET_ERROR: Error occured allocating a socket and
 *		resources or an error occurred binding a socket.
 *      \li \c UPNP_E_SOCKET_WRITE: An error or timeout occurred writing
 *              to a socket.
 *      \li \c UPNP_E_SOCKET_CONNECT: An error occurred connecting a
 *              socket.
 *      \li \c UPNP_E_OUTOF_SOCKET: Too many sockets are currently
 *              allocated.
 */
EXPORT_SPEC int http_MakeHttpRequest(
	/* ![in] The method to use to make the request. */
	Upnp_HttpMethod method,
	/*! [in] The URL to use to make the request. The URL should use the same
	 *  host and scheme used to create the connection. */
	const char *url,
	/*! [in] The handle to the connection. */
	void *handle,
	/*! [in] Headers to be used for the request. Each header should be terminated by a CRLF as specified
	 *  in the HTTP specification. If NULL then the default headers will be used. */
	UpnpString *headers,
	/*! [in] The media type of content being sent. Can be NULL. */
	const char *contentType,
	/*! [in] The length of the content being sent, in bytes. Set to \b UPNP_USING_CHUNKED to use
	 *  chunked encoding, or \b UPNP_UNTIL_CLOSE to avoid specifying the content length to the server.
	 *  In this case the request is considered unfinished until the connection is closed. */
	int contentLength,
	/*! [in] The time out value sent with the request during which a response
	 * is expected from the receiver, failing which, an error is reported.
	 * If value is negative, timeout is infinite. */
	int timeout);


/*!
 * \brief Writes the content of a HTTP request initiated by a \b UpnpMakeHttpRequest call.
 * The end of the content should be indicated by a call to \b UpnpEndHttpRequest
 *
 *  \return An integer representing one of the following:
 *      \li \c UPNP_E_SUCCESS: The operation completed successfully.
 *      \li \c UPNP_E_INVALID_PARAM: Either \b handle, \b buf
 *              or \b size is not a valid pointer.
 *      \li \c UPNP_E_SOCKET_WRITE: An error or timeout occurred writing
 *              to a socket.
 *      \li \c UPNP_E_OUTOF_SOCKET: Too many sockets are currently
 *              allocated.
 */
EXPORT_SPEC int http_WriteHttpRequest(
	/*! [in] The handle of the connection created by the call to
	 * \b UpnpOpenHttpConnection. */
	void *handle,
	/*! [in] The buffer containing date to be written. */
	char *buf,
	/*! [in] The size, in bytes of \b buf. */
	size_t *size,
	/*! [in] A timeout value sent with the request during which a response is
	 * expected from the server, failing which, an error is reported. If
	 * value is negative, timeout is infinite. */
	int timeout);


/*!
 * \brief Indicates the end of a HTTP request previously made by
 * \b UpnpMakeHttpRequest.
 *
 *  \return An integer representing one of the following:
 *      \li \c UPNP_E_SUCCESS: The operation completed successfully.
 *      \li \c UPNP_E_INVALID_PARAM: \b handle is not a valid pointer.
 *      \li \c UPNP_E_OUTOF_MEMORY: Insufficient resources exist to
 *              download this file.
 *      \li \c UPNP_E_SOCKET_ERROR: Error occured allocating a socket and
 *		resources or an error occurred binding a socket.
 *      \li \c UPNP_E_SOCKET_WRITE: An error or timeout occurred writing
 *              to a socket.
 *      \li \c UPNP_E_SOCKET_CONNECT: An error occurred connecting a
 *              socket.
 *      \li \c UPNP_E_OUTOF_SOCKET: Too many sockets are currently
 *              allocated.
 */
EXPORT_SPEC int http_EndHttpRequest(
	/*! [in] The handle to the connection. */
	void *handle,
	/*! [in] The time out value sent with the request during which a response
	 * is expected from the receiver, failing which, an error is reported.
	 * If value is negative, timeout is infinite. */
	int timeout);


/*!
 * \brief Gets the response from the server using a connection previously created
 * by \b UpnpOpenHttpConnection
 *
 * \note Memory for \b contentType is only valid until the next call to the HTTP API
 * for the same connection.
 *
 * \return An integer representing one of the following:
 *     \li \c UPNP_E_SUCCESS: The operation completed successfully.
 *     \li \c UPNP_E_INVALID_PARAM: Either \b handle,
 *             \b contentType, \b contentLength or \b httpStatus
 *     	is not a valid pointer.
 *     \li \c UPNP_E_INVALID_URL: The \b url is not a valid
 *             URL.
 *     \li \c UPNP_E_OUTOF_MEMORY: Insufficient resources exist to
 *             download this file.
 *     \li \c UPNP_E_NETWORK_ERROR: A network error occurred.
 *     \li \c UPNP_E_SOCKET_WRITE: An error or timeout occurred writing
 *             to a socket.
 *     \li \c UPNP_E_SOCKET_READ: An error or timeout occurred reading
 *             from a socket.
 *     \li \c UPNP_E_SOCKET_BIND: An error occurred binding a socket.
 *     \li \c UPNP_E_SOCKET_CONNECT: An error occurred connecting a
 *             socket.
 *     \li \c UPNP_E_OUTOF_SOCKET: Too many sockets are currently
 *             allocated.
 *     \li \c UPNP_E_BAD_RESPONSE: A bad response was received from the
 *             remote server.
 */
EXPORT_SPEC int http_GetHttpResponse(
	/*! [in] The handle of the connection created by the call to
	 * \b UpnpOpenHttpConnection. */
	void *handle,
	/*! [in] Headers sent by the server for the response. If NULL then the
	 * headers are not copied. */
	UpnpString *headers,
	/*! [out] A buffer to store the media type of the item. */
	char **contentType,
	/*! [out] A pointer to store the length of the item. */
	int *contentLength,
	/*! [out] The status returned on receiving a response message. */
	int *httpStatus,
	/*! [in] The time out value sent with the request during which a response
	 * is expected from the server, failing which, an error is reported
	 * back to the user. If value is negative, timeout is infinite. */
	int timeout);


/*!
 * \brief Reads the content of a response using a connection previously created
 * by \b UpnpOpenHttpConnection.
 *
 *  \return An integer representing one of the following:
 *      \li \c UPNP_E_SUCCESS: The operation completed successfully.
 *      \li \c UPNP_E_INVALID_PARAM: Either \b handle, \b buf
 *              or \b size is not a valid pointer.
 *	    \li \c UPNP_E_BAD_RESPONSE: A bad response was received from the
 *	            remote server.
 *      \li \c UPNP_E_BAD_HTTPMSG: Either the request or response was in
 *              the incorrect format.
 *      \li \c UPNP_E_CANCELED: another thread called UpnpCancelHttpGet.
 *
 *  Note: In case of return values, the status code parameter of the passed
 *        in handle value may provide additional information on the return
 *        value.
 */
EXPORT_SPEC int http_ReadHttpResponse(
	/*! [in] The handle of the connection created by the call to
	 * \b UpnpOpenHttpConnection. */
	void *handle,
	/*! [in,out] The buffer to store the read item. */
	char *buf,
	/*! [in,out] The size of the buffer to be read. */
	size_t *size,
	/*! [in] The time out value sent with the request during which a response is
	 * expected from the server, failing which, an error is reported back to
	 * the user. If value is negative, timeout is infinite. */
	int timeout);


/*!
 * \brief Closes the connection created with \b UpnpOpenHttpConnection
 * and frees any memory associated with the connection.
 *
 * \return An integer representing one of the following:
 *     \li \c UPNP_E_SUCCESS: The operation completed successfully.
 *     \li \c UPNP_E_INVALID_PARAM: \b handle, or is not a valid pointer.
 *     \li \c UPNP_E_SOCKET_READ: An error or timeout occurred reading
 *             from a socket.
 *     \li \c UPNP_E_OUTOF_SOCKET: Too many sockets are currently
 *             allocated.
 */
EXPORT_SPEC int http_CloseHttpConnection(
	/*! [in] The handle of the connection to close, created by the call to
	 * \b UpnpOpenHttpPost. */
	void *handle);


/************************************************************************
 * Function: http_SendStatusResponse
 *
 * Parameters:
 *	IN SOCKINFO *info;		Socket information object
 *	IN int http_status_code;	error code returned while making 
 *					or sending the response message
 *	IN int request_major_version;	request major version
 *	IN int request_minor_version;	request minor version
 *
 * Description:
 *	Generate a response message for the status query and send the
 *	status response.
 *
 * Return: int
 *	0 -- success
 *	UPNP_E_OUTOF_MEMORY
 *	UPNP_E_SOCKET_WRITE
 *	UPNP_E_TIMEDOUT
 ************************************************************************/
int http_SendStatusResponse(
	IN SOCKINFO *info,
	IN int http_status_code,
	IN int request_major_version,
	IN int request_minor_version );

/*!
 * \brief Generate an HTTP message based on the format that is specified in
 * the input parameters.
 *
\verbatim
Format types:
	'B':	arg = int status_code		-- appends content-length, content-type and HTML body for given code.
	'b':	arg1 = const char *buf;
		arg2 = size_t buf_length memory ptr
	'C':	(no args)			-- appends a HTTP CONNECTION: close header depending on major, minor version.
	'c':	(no args)			-- appends CRLF "\r\n"
	'D':	(no args)			-- appends HTTP DATE: header
	'd':	arg = int number		-- appends decimal number
	'G':	arg = range information		-- add range header
	'h':	arg = off_t number		-- appends off_t number
	'K':	(no args)			-- add chunky header
	'L':	arg = language information	-- add Content-Language header if Accept-Language header is not empty and if
						   WEB_SERVER_CONTENT_LANGUAGE is not empty
	'N':	arg1 = off_t content_length	-- content-length header
	'q':	arg1 = http_method_t		-- request start line and HOST header
		arg2 = (uri_type *)
	'Q':	arg1 = http_method_t;		-- start line of request
		arg2 = char* url;
		arg3 = size_t url_length 
	'R':	arg = int status_code		-- adds a response start line
	'S':	(no args)			-- appends HTTP SERVER: header
	's':	arg = const char *		-- C_string
	'T':	arg = char * content_type;	-- format e.g: "text/html"; content-type header
	't':	arg = time_t * gmt_time		-- appends time in RFC 1123 fmt
	'U':	(no args)			-- appends HTTP USER-AGENT: header
	'X':	arg = const char *		-- useragent; "redsonic" HTTP X-User-Agent: useragent
\endverbatim
 *
 * \return
 * 	\li \c 0 - On Success
 * 	\li \c UPNP_E_OUTOF_MEMORY
 * 	\li \c UPNP_E_INVALID_URL
 */
int http_MakeMessage(
	/* [in,out] Buffer with the contents of the message. */
	INOUT membuffer* buf, 
	/* [in] HTTP major version. */
	IN int http_major_version,
	/* [in] HTTP minor version. */
	IN int http_minor_version,
	/* [in] Pattern format. */
	IN const char* fmt,
	/* [in] Format arguments. */
	... );


/************************************************************************
 * Function: http_CalcResponseVersion
 *
 * Parameters:
 *	IN int request_major_vers;	Request major version
 *	IN int request_minor_vers;	Request minor version
 *	OUT int* response_major_vers;	Response mojor version
 *	OUT int* response_minor_vers;	Response minor version
 *
 * Description:
 *	Calculate HTTP response versions based on the request versions.
 *
 * Return: void
 ************************************************************************/
void http_CalcResponseVersion( 
	IN int request_major_vers,
	IN int request_minor_vers,
	OUT int* response_major_vers,
	OUT int* response_minor_vers );


/************************************************************************
 * Function: http_OpenHttpGetEx
 *
 * Parameters:
 *	IN const char *url_str;		String as a URL
 *	IN OUT void **Handle;		Pointer to buffer to store HTTP
 *					post handle
 *	IN OUT char **contentType;	Type of content
 *	OUT int *contentLength;		length of content
 *	OUT int *httpStatus;		HTTP status returned on receiving a
 *					response message
 *	IN int timeout;			time out value
 *
 * Description:
 *	Makes the HTTP GET message, connects to the peer, 
 *	sends the HTTP GET request, gets the response and parses the 
 *	response.
 *
 * Return: int
 *	UPNP_E_SUCCESS		- On Success
 *	UPNP_E_INVALID_PARAM	- Invalid Paramters
 *	UPNP_E_OUTOF_MEMORY
 *	UPNP_E_SOCKET_ERROR
 *	UPNP_E_BAD_RESPONSE
 ************************************************************************/
int http_OpenHttpGetEx(IN const char *url_str,
		     IN OUT void **Handle,
		     IN OUT char **contentType,
		     OUT int *contentLength,
		     OUT int *httpStatus,
			 IN int lowRange,
			 IN int highRange,
		     IN int timeout);


/************************************************************************
 * Function: get_sdk_info
 *
 * Parameters:
 *	OUT char *info;	buffer to store the operating system information
 *	IN size_t infoSize; size of buffer
 *
 * Description:
 *	Returns the server information for the operating system
 *
 * Return:
 *	UPNP_INLINE void
 ************************************************************************/
void get_sdk_info( OUT char *info, IN size_t infoSize );

#ifdef __cplusplus
}	/* #extern "C" */
#endif


#endif /* GENLIB_NET_HTTP_HTTPREADWRITE_H */

