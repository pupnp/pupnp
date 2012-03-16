/*******************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation 
 * All rights reserved. 
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

#ifndef GENLIB_NET_URI_H
#define GENLIB_NET_URI_H

/*!
 * \file
 */

#if !defined(WIN32)
	#include <sys/param.h>
#endif

#include "UpnpGlobal.h" /* for */
#include "UpnpInet.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#ifdef WIN32
	#ifndef UPNP_USE_MSVCPP
		/* VC Winsocks2 includes these functions */
		#include "inet_pton.h"
	#endif
#else
	#include <netdb.h>      /* for struct addrinfo */
#endif

#ifdef WIN32
	#define strncasecmp strnicmp
#else
	/* Other systems have strncasecmp */
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*! length for HTTP DATE: "DATE: Sun, 01 Jul 2000 08:15:23 GMT<cr><lf>" */
#define HTTP_DATE_LENGTH 37

#define SEPARATORS "()<>@,;:\\\"/[]?={} \t"
#define MARK "-_.!~*'()"

/*! added {} for compatibility */
#define RESERVED ";/?:@&=+$,{}"

#define HTTP_SUCCESS 1
#define FALSE 0
#define TAB 9
#define CR 13
#define LF 10
#define SOCKET_BUFFER_SIZE 5000

enum hostType {
	HOSTNAME,
	IPv4address
};

enum pathType {
	ABS_PATH,
	REL_PATH,
	OPAQUE_PART
};

#ifdef WIN32
	/* there is a conflict in windows with other symbols. */
	enum uriType  {
		absolute,
		relative
	};
#else
	enum uriType  {
		ABSOLUTE,
		RELATIVE
	};
#endif

/*! 
 * \brief Buffer used in parsinghttp messages, urls, etc. generally this simply
 * holds a pointer into a larger array.
 */
typedef struct TOKEN {
	const char *buff;
	size_t size;
} token;

/*!
 * \brief Represents a host port: e.g. "127.127.0.1:80" text is a token
 * pointing to the full string representation.
 */
typedef struct HOSTPORT {
	/*! Full host port. */
	token text;
	/* Network Byte Order */
	struct sockaddr_storage IPaddress;
} hostport_type;

/*!
 * \brief Represents a URI used in parse_uri and elsewhere
 */
typedef struct URI{
	enum uriType type;
	token scheme;
	enum pathType path_type;
	token pathquery;
	token fragment;
	hostport_type hostport;
} uri_type;

/*!
 * \brief Represents a list of URLs as in the "callback" header of SUBSCRIBE
 * message in GENA. "char *" URLs holds dynamic memory.
 */
typedef struct URL_LIST {
	/*! */
	size_t size;
	/*! All the urls, delimited by <> */
	char *URLs;
	/*! */
	uri_type *parsedURLs;
} URL_list;

/*!
 * \brief Replaces an escaped sequence with its unescaped version as in
 * http://www.ietf.org/rfc/rfc2396.txt  (RFC explaining URIs)
 *
 * Size of array is NOT checked (MUST be checked by caller)
 *
 * \note This function modifies the string. If the sequence is an escaped
 * sequence it is replaced, the other characters in the string are shifted
 * over, and NULL characters are placed at the end of the string.
 *
 * \return 
 */
int replace_escaped(
	/*! [in,out] String of characters. */
	char *in,
	/*! [in] Index at which to start checking the characters. */
	size_t index,
	/*! [out] . */
	size_t *max);

/*!
 * \brief Copies one URL_list into another.
 *
 * This includes dynamically allocating the out->URLs field (the full string),
 * and the structures used to hold the parsedURLs. This memory MUST be freed
 * by the caller through: free_URL_list(&out).
 *
 * \return
 * 	\li HTTP_SUCCESS - On Success.
 * 	\li UPNP_E_OUTOF_MEMORY - On Failure to allocate memory.
 */
int copy_URL_list(
	/*! [in] Source URL list. */
	URL_list *in,
	/*! [out] Destination URL list. */
	URL_list *out);

/*!
 * \brief Frees the memory associated with a URL_list.
 *
 * Frees the dynamically allocated members of of list. Does NOT free the
 * pointer to the list itself ( i.e. does NOT free(list)).
 */
void free_URL_list(
	/*! [in] URL list object. */
	URL_list *list);

/*!
 * \brief Function useful in debugging for printing a parsed uri.
 */
#ifdef DEBUG
void print_uri(
	/*! [in] URI object to print. */
	uri_type *in);
#else
static UPNP_INLINE void print_uri(uri_type *in)
{
	return;
	in = in;
}
#endif

/*!
 * \brief Function useful in debugging for printing a token.
 */
#ifdef DEBUG
void print_token(
	/*! [in] Token object to print. */
	token *in);
#else
static UPNP_INLINE void print_token(
	/*! [in] Token object to print. */
	token *in)
{
	return;
	in = in;
}
#endif

/*!
 * \brief Compares buffer in the token object with the buffer in in2.
 *
 * \return 
 * 	\li < 0, if string1 is less than string2.
 * 	\li == 0, if string1 is identical to string2 .
 * 	\li > 0, if string1 is greater than string2.
 */
int token_string_casecmp(
	/*! [in] Token object whose buffer is to be compared. */
	token *in1,
	/*! [in] String of characters to compare with. */
	const char *in2);

/*!
 * \brief Compares a null terminated string to a token (exact).
 *
 * \return 
 * 	\li < 0, if string1 is less than string2.
 * 	\li == 0, if string1 is identical to string2 .
 * 	\li > 0, if string1 is greater than string2.
 */
int token_string_cmp(
	/*! [in] Token object whose buffer is to be compared. */
	token *in1,
	/*! [in] String of characters to compare with. */
	char *in2);

/*!
 * \brief Compares two tokens.
 *
 * \return 
 * 	\li < 0, if string1 is less than string2.
 * 	\li == 0, if string1 is identical to string2 .
 * 	\li > 0, if string1 is greater than string2.
 */
int token_cmp(
	/*! [in] First token object whose buffer is to be compared. */
	token *in1,
	/*! [in] Second token object used for the comparison. */
	token *in2);

/*!
 * \brief Removes http escaped characters such as: "%20" and replaces them with
 * their character representation. i.e. "hello%20foo" -> "hello foo".
 *
 * The input IS MODIFIED in place (shortened). Extra characters are replaced
 * with \b NULL.
 *
 * \return UPNP_E_SUCCESS.
 */
int remove_escaped_chars(
	/*! [in,out] String of characters to be modified. */
	char *in,
	/*! [in,out] Size limit for the number of characters. */
	size_t *size);

/*!
 * \brief Removes ".", and ".." from a path.
 *
 * If a ".." can not be resolved (i.e. the .. would go past the root of the
 * path) an error is returned.
 *
 * The input IS modified in place.)
 *
 * \note Examples
 * 	char path[30]="/../hello";
 * 	remove_dots(path, strlen(path)) -> UPNP_E_INVALID_URL
 * 	char path[30]="/./hello";
 * 	remove_dots(path, strlen(path)) -> UPNP_E_SUCCESS, 
 * 	in = "/hello"
 * 	char path[30]="/./hello/foo/../goodbye" -> 
 * 	UPNP_E_SUCCESS, in = "/hello/goodbye"
 *
 * \return 
 * 	\li UPNP_E_SUCCESS - On Success.
 * 	\li UPNP_E_OUTOF_MEMORY - On failure to allocate memory.
 * 	\li UPNP_E_INVALID_URL - Failure to resolve URL.
 */
int remove_dots(
	/*! [in] String of characters from which "dots" have to be removed. */
	char *in,
	/*! [in] Size limit for the number of characters. */
	size_t size);

/*!
 * \brief resolves a relative url with a base url returning a NEW (dynamically
 * allocated with malloc) full url.
 *
 * If the base_url is \b NULL, then a copy of the  rel_url is passed back if
 * the rel_url is absolute then a copy of the rel_url is passed back if neither
 * the base nor the rel_url are Absolute then NULL is returned. Otherwise it
 * tries and resolves the relative url with the base as described in
 * http://www.ietf.org/rfc/rfc2396.txt (RFCs explaining URIs).
 *
 * The resolution of '..' is NOT implemented, but '.' is resolved.
 *
 * \return 
 */
char *resolve_rel_url(
	/*! [in] Base URL. */
	char *base_url,
	/*! [in] Relative URL. */
	char *rel_url);

/*!
 * \brief Parses a uri as defined in http://www.ietf.org/rfc/rfc2396.txt
 * (RFC explaining URIs).
 *
 * Handles absolute, relative, and opaque uris. Parses into the following
 * pieces: scheme, hostport, pathquery, fragment (path and query are treated
 * as one token)
 *
 * Caller should check for the pieces they require.
 *
 * \return
 */
int parse_uri(
	/*! [in] Character string containing uri information to be parsed. */
	const char *in,
	/*! [in] Maximum limit on the number of characters. */
	size_t max,
	/*! [out] Output parameter which will have the parsed uri information. */
	uri_type *out);

/*!
 * \brief Same as parse_uri(), except that all strings are unescaped
 * (%XX replaced by chars).
 *
 * \note This modifies 'pathquery' and 'fragment' parts of the input.
 *
 * \return 
 */
int parse_uri_and_unescape(
	/*! [in] Character string containing uri information to be parsed. */
	char *in,
	/*! [in] Maximum limit on the number of characters. */
	size_t max,
	/*! [out] Output parameter which will have the parsed uri information. */
	uri_type *out);

/*!
 * \brief 
 *
 * \return 
 */
int parse_token(
	/*! [in] . */
	char *in,
	/*! [out] . */
	token *out,
	/*! [in] . */
	int max_size);

/* Commented #defines, functions and typdefs */

#if 0
#define HTTP_E_BAD_URL UPNP_E_INVALID_URL
#define HTTP_E_READ_SOCKET  UPNP_E_SOCKET_READ
#define HTTP_E_BIND_SOCKET  UPNP_E_SOCKET_BIND
#define HTTP_E_WRITE_SOCKET  UPNP_E_SOCKET_WRITE
#define HTTP_E_CONNECT_SOCKET  UPNP_E_SOCKET_CONNECT
#define HTTP_E_SOCKET    UPNP_E_OUTOF_SOCKET
#define HTTP_E_BAD_RESPONSE UPNP_E_BAD_RESPONSE
#define HTTP_E_BAD_REQUEST UPNP_E_BAD_REQUEST
#define HTTP_E_BAD_IP_ADDRESS UPNP_E_INVALID_URL

#define RESPONSE_TIMEOUT 30
#endif

#if 0
/*!
 * Buffer used to store data read from a socket during an http transfer in
 * function read_bytes.
 */
typedef struct SOCKET_BUFFER{
	char buff[SOCKET_BUFFER_SIZE];
	int size;
	struct SOCKET_BUFFER *next;
} socket_buffer;


typedef struct HTTP_HEADER {
	token header;
	token value;
	struct HTTP_HEADER * next;
} http_header;


typedef struct HTTP_STATUS_LINE{
	token http_version;
	token status_code;
	token reason_phrase;
} http_status;


typedef struct HTTP_REQUEST_LINE {
	token http_version;
	uri_type request_uri;
	token method;
} http_request;


/*!
 * Represents a parsed HTTP_MESSAGE head_list is dynamically allocated
 */
typedef struct HTTP_MESSAGE {
	http_status status;
	http_request request;
	http_header * header_list;
	token content;
} http_message;
#endif


#if 0
int transferHTTP(
	char *request,
	char *toSend, 
	int toSendSize,
	char **out,
	char *Url);


int transferHTTPRaw(
	char *toSend,
	int toSendSize, 
	char **out,
	char *URL);


/*!
 * \brief helper function.
 */
int transferHTTPparsedURL(
	char *request, 
	char *toSend,
	int toSendSize, 
	char **out,
	uri_type *URL);


/*!
 * \brief assumes that char * out has enough space ( 38 characters)
 * outputs the current time in the following null terminated string:
 * "DATE: Sun, Jul 06 2000 08:53:01 GMT\r\n"
 */
void currentTmToHttpDate(
	char *out);


int parse_http_response(
	char *in,
	http_message *out,
	int max_len);


int parse_http_request(
	char *in,
	http_message *out,
	int max_len);


void print_http_message(
	http_message *message);


int search_for_header(
	http_message *in,
	char *header,
	token *out_value);


void print_status_line(
	http_status *in);


void print_request_line(
	http_request *in);


int parse_http_line(
	char *in,
	int max_size);


int parse_not_LWS(
	char *in,
	token *out,
	int max_size);


int parse_LWS(
	char *in,
	int max_size);


size_t write_bytes(
	int fd,
	char *bytes,
	size_t n,
	int timeout);


void free_http_message(
	http_message *message);


#endif


#ifdef __cplusplus
}
#endif


#endif /* GENLIB_NET_URI_H */

