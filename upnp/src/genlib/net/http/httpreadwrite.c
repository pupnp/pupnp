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

/*!
 * \file
 *
 * Purpose: This file defines the functionality making use of the http.
 * It defines functions to receive messages, process messages, send messages.
 */

#include "config.h"

#include "httpreadwrite.h"

#include "unixutil.h"
#include "upnp.h"
#include "upnpapi.h"
#include "membuffer.h"
#include "uri.h"
#include "statcodes.h"
#include "sock.h"
#include "UpnpInet.h"
#include "UpnpIntTypes.h"
#include "UpnpStdInt.h"
#include "webserver.h"

#include <assert.h>
#include <stdarg.h>

#ifdef WIN32
	#include <malloc.h>
	#define fseeko fseek
	#define snprintf _snprintf
#else
	#include <arpa/inet.h>
	#include <sys/types.h>
	#include <sys/time.h>
	#include <sys/wait.h>
	#include <sys/utsname.h>
#endif

/* 
 * Please, do not change these to const int while MSVC cannot understand
 * const int in array dimensions.
 */
/*
const int CHUNK_HEADER_SIZE = 10;
const int CHUNK_TAIL_SIZE = 10;
*/
#define CHUNK_HEADER_SIZE (size_t)10
#define CHUNK_TAIL_SIZE (size_t)10

#ifndef UPNP_ENABLE_BLOCKING_TCP_CONNECTIONS

/* in seconds */
#define DEFAULT_TCP_CONNECT_TIMEOUT 5

/*!
 * \brief Checks socket connection and wait if it is not connected.
 * It should be called just after connect.
 *
 * \return 0 if successful, else -1.
 */
static int Check_Connect_And_Wait_Connection(
	/*! [in] socket. */
	SOCKET sock,
	/*! [in] result of connect. */
	int connect_res)
{
	struct timeval tmvTimeout = {DEFAULT_TCP_CONNECT_TIMEOUT, 0};
	int result;
#ifdef WIN32
	struct fd_set fdSet;
#else
	fd_set fdSet;
#endif
	FD_ZERO(&fdSet);
	FD_SET(sock, &fdSet);

	if (connect_res < 0) {
#ifdef WIN32
		if (WSAEWOULDBLOCK == WSAGetLastError() ) {
#else
		if (EINPROGRESS == errno ) {
#endif
			result = select(sock + 1, NULL, &fdSet, NULL, &tmvTimeout);
			if (result < 0) {
#ifdef WIN32
				/* WSAGetLastError(); */
#else
				/* errno */
#endif
				return -1;
			} else if (result == 0) {
				/* timeout */
				return -1;
#ifndef WIN32
			} else {
				int valopt = 0;
				socklen_t len = sizeof(valopt);
				if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (void *) &valopt, &len) < 0) {
					/* failed to read delayed error */
					return -1;
				} else if (valopt) {
					/* delayed error = valopt */
					return -1;
				}
#endif
			}
		}
	}

	return 0;
}
#endif /* UPNP_ENABLE_BLOCKING_TCP_CONNECTIONS */

static int private_connect(
	SOCKET sockfd,
	const struct sockaddr *serv_addr,
	socklen_t addrlen)
{
#ifndef UPNP_ENABLE_BLOCKING_TCP_CONNECTIONS
	int ret = sock_make_no_blocking(sockfd);
	if (ret != - 1) {
		ret = connect(sockfd, serv_addr, addrlen);
		ret = Check_Connect_And_Wait_Connection(sockfd, ret);
		if (ret != - 1) {
			ret = sock_make_blocking(sockfd);
		}
	}

	return ret;
#else
	return connect(sockfd, serv_addr, addrlen);
#endif /* UPNP_ENABLE_BLOCKING_TCP_CONNECTIONS */
}

#ifdef WIN32
struct tm *http_gmtime_r(const time_t *clock, struct tm *result)
{
	if (clock == NULL || *clock < 0 || result == NULL)
		return NULL;

	/* gmtime in VC runtime is thread safe. */
	*result = *gmtime(clock);
	return result;
}
#endif

static int get_hoststr(const char* url_str,
                       char **hoststr,
                       size_t *hostlen)
{
	char *urlPath = alloca(strlen(url_str) + 1);
	char *temp;
	memset(urlPath, 0, strlen(url_str) + 1);
	strncpy(urlPath, url_str, strlen(url_str));
	*hoststr = strstr(urlPath, "//");
	if (*hoststr == NULL)
		return UPNP_E_INVALID_URL;
	*hoststr += 2;
	temp = strchr(*hoststr, '/');
	if (temp == NULL)
		return UPNP_E_INVALID_URL;
	*temp = '\0';
	*hostlen = strlen(*hoststr);
	*temp = '/';
	return UPNP_E_SUCCESS;
}

static void copy_msg_headers(IN LinkedList *msgHeaders,
                             OUT UpnpString *headers)
{
	return;
/* TODO: */
#if 0
	ListNode *node;
	UpnpHttpHeader *header;
	http_header_t *msgHeader;
	if (headers) {
		ListInit(headers, NULL, (free_function) UpnpHttpHeader_delete);
		node = ListHead(msgHeaders);
		while(node) {
			msgHeader = (http_header_t*) node->item;
			header = UpnpHttpHeader_new();
			UpnpHttpHeader_strncpy_Name(
				header,
				msgHeader->name.buf,
				msgHeader->name.length);
			UpnpHttpHeader_strncpy_Value(
				header,
				msgHeader->value.buf,
				msgHeader->value.length);
			node = ListNext(msgHeaders, node);
		}
	}
#endif
}


int http_FixUrl(IN uri_type *url, OUT uri_type *fixed_url)
{
	const char *temp_path = "/";

	*fixed_url = *url;
#ifdef UPNP_ENABLE_OPEN_SSL
	if (token_string_casecmp(&fixed_url->scheme, "http") != 0 &&
	    token_string_casecmp(&fixed_url->scheme, "https") != 0)
	{
		return UPNP_E_INVALID_URL;
	}
#else
	if (token_string_casecmp(&fixed_url->scheme, "http") != 0)
	{
		return UPNP_E_INVALID_URL;
	}
#endif
	if( fixed_url->hostport.text.size == ( size_t ) 0 ) {
		return UPNP_E_INVALID_URL;
	}
	/* set pathquery to "/" if it is empty */
	if (fixed_url->pathquery.size == (size_t)0) {
		fixed_url->pathquery.buff = temp_path;
		fixed_url->pathquery.size = (size_t)1;
	}

	return UPNP_E_SUCCESS;
}

int http_FixStrUrl(
	IN const char *urlstr,
	IN size_t urlstrlen,
	OUT uri_type *fixed_url)
{
	uri_type url;

	if (parse_uri(urlstr, urlstrlen, &url) != HTTP_SUCCESS) {
		return UPNP_E_INVALID_URL;
	}

	return http_FixUrl(&url, fixed_url);
}

/************************************************************************
 * Function: http_Connect
 *
 * Parameters:
 *	IN uri_type* destination_url;	URL containing destination information
 *	OUT uri_type *url;		Fixed and corrected URL
 *
 * Description:
 *	Gets destination address from URL and then connects to the remote end
 *
 *  Returns:
 *	socket descriptor on success
 *	UPNP_E_OUTOF_SOCKET
 *	UPNP_E_SOCKET_CONNECT on error
 ************************************************************************/
SOCKET http_Connect(
	IN uri_type *destination_url,
	OUT uri_type *url)
{
	SOCKET connfd;
	socklen_t sockaddr_len;
	int ret_connect;
	char errorBuffer[ERROR_BUFFER_LEN];

	http_FixUrl(destination_url, url);

	connfd = socket((int)url->hostport.IPaddress.ss_family,
		SOCK_STREAM, 0);
	if (connfd == INVALID_SOCKET) {
		return (SOCKET)(UPNP_E_OUTOF_SOCKET);
	}
	sockaddr_len = (socklen_t)(url->hostport.IPaddress.ss_family == AF_INET6 ?
		sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in));
	ret_connect = private_connect(connfd,
		(struct sockaddr *)&url->hostport.IPaddress, sockaddr_len);
	if (ret_connect == -1) {
#ifdef WIN32
		UpnpPrintf(UPNP_CRITICAL, HTTP, __FILE__, __LINE__,
			"connect error: %d\n", WSAGetLastError());
#endif
		if (shutdown(connfd, SD_BOTH) == -1) {
			strerror_r(errno, errorBuffer, ERROR_BUFFER_LEN);
			UpnpPrintf(UPNP_INFO, HTTP, __FILE__, __LINE__,
				   "Error in shutdown: %s\n", errorBuffer);
		}
		UpnpCloseSocket(connfd);
		return (SOCKET)(UPNP_E_SOCKET_CONNECT);
	}

	return connfd;
}


/*!
 * \brief Get the data on the socket and take actions based on the read data to
 * modify the parser objects buffer.
 *
 * If an error is reported while parsing the data, the error code is passed in
 * the http_errr_code parameter.
 *
 * Parameters:
 *	IN SOCKINFO *info;			Socket information object
 *	OUT http_parser_t* parser;		HTTP parser object
 *	IN http_method_t request_method;	HTTP request method
 *	IN OUT int* timeout_secs;		time out
 *	OUT int* http_error_code;		HTTP error code returned
 *
 * \return
 * 	 UPNP_E_SUCCESS
 *	 UPNP_E_BAD_HTTPMSG
 */
int http_RecvMessage(
	IN SOCKINFO *info,
	OUT http_parser_t *parser,
	IN http_method_t request_method,
	IN OUT int *timeout_secs,
	OUT int *http_error_code)
{
	int ret = UPNP_E_SUCCESS;
	int line = 0;
	parse_status_t status;
	int num_read;
	int ok_on_close = FALSE;
	char buf[2 * 1024];

	if (request_method == (http_method_t)HTTPMETHOD_UNKNOWN) {
		parser_request_init(parser);
	} else {
		parser_response_init(parser, request_method);
	}

	while (TRUE) {
		num_read = sock_read(info, buf, sizeof buf, timeout_secs);
		if (num_read > 0) {
			/* got data */
			status = parser_append(parser, buf, (size_t)num_read);
			switch (status) {
			case PARSE_SUCCESS:
				UpnpPrintf( UPNP_INFO, HTTP, __FILE__, __LINE__,
					"<<< (RECVD) <<<\n%s\n-----------------\n",
					parser->msg.msg.buf );
				print_http_headers( &parser->msg );
				if (g_maxContentLength > (size_t)0 && parser->content_length > (unsigned int)g_maxContentLength) {
					*http_error_code = HTTP_REQ_ENTITY_TOO_LARGE;
					line = __LINE__;
					ret = UPNP_E_OUTOF_BOUNDS;
					goto ExitFunction;
				}
				line = __LINE__;
				ret = 0;
				goto ExitFunction;
			case PARSE_FAILURE:
			case PARSE_NO_MATCH:
				*http_error_code = parser->http_error_code;
				line = __LINE__;
				ret = UPNP_E_BAD_HTTPMSG;
				goto ExitFunction;
			case PARSE_INCOMPLETE_ENTITY:
				/* read until close */
				ok_on_close = TRUE;
				break;
			case PARSE_CONTINUE_1:
				/* Web post request. */
				line = __LINE__;
				ret = PARSE_SUCCESS;
				goto ExitFunction;
			default:
				break;
			}
		} else if (num_read == 0) {
			if (ok_on_close) {
				UpnpPrintf( UPNP_INFO, HTTP, __FILE__, __LINE__,
					"<<< (RECVD) <<<\n%s\n-----------------\n",
					parser->msg.msg.buf );
				print_http_headers(&parser->msg);
				line = __LINE__;
				ret = 0;
				goto ExitFunction;
			} else {
				/* partial msg */
				*http_error_code = HTTP_BAD_REQUEST;    /* or response */
				line = __LINE__;
				ret = UPNP_E_BAD_HTTPMSG;
				goto ExitFunction;
			}
		} else {
			*http_error_code = parser->http_error_code;
			line = __LINE__;
			ret = num_read;
			goto ExitFunction;
		}
	}

ExitFunction:
	if (ret != UPNP_E_SUCCESS) {
		UpnpPrintf(UPNP_ALL, HTTP, __FILE__, line,
			"(http_RecvMessage): Error %d, http_error_code = %d.\n",
			ret,
			*http_error_code);
	}

	return ret;
}

int http_SendMessage(SOCKINFO *info, int *TimeOut, const char *fmt, ...)
{
#if EXCLUDE_WEB_SERVER == 0
	FILE *Fp;
	struct SendInstruction *Instr = NULL;
	char *filename = NULL;
	char *file_buf = NULL;
	char *ChunkBuf = NULL;
	/* 10 byte allocated for chunk header. */
	char Chunk_Header[CHUNK_HEADER_SIZE];
	size_t num_read;
	size_t amount_to_be_read = (size_t)0;
	size_t Data_Buf_Size = WEB_SERVER_BUF_SIZE;
#endif /* EXCLUDE_WEB_SERVER */
	va_list argp;
	char *buf = NULL;
	char c;
	int nw;
	int RetVal = 0;
	size_t buf_length;
	size_t num_written;

#if EXCLUDE_WEB_SERVER == 0
	memset(Chunk_Header, 0, sizeof(Chunk_Header));
#endif /* EXCLUDE_WEB_SERVER */
	va_start(argp, fmt);
	while ((c = *fmt++)) {
#if EXCLUDE_WEB_SERVER == 0
		if (c == 'I') {
			Instr = va_arg(argp, struct SendInstruction *);
			if (Instr->ReadSendSize >= 0)
				amount_to_be_read = (size_t)Instr->ReadSendSize;
			else
				amount_to_be_read = Data_Buf_Size;
			if (amount_to_be_read < WEB_SERVER_BUF_SIZE)
				Data_Buf_Size = amount_to_be_read;
			ChunkBuf = malloc((size_t)
				(Data_Buf_Size + CHUNK_HEADER_SIZE +
				CHUNK_TAIL_SIZE));
			if (!ChunkBuf) {
				RetVal = UPNP_E_OUTOF_MEMORY;
				goto ExitFunction;
			}
			file_buf = ChunkBuf + CHUNK_HEADER_SIZE;
		} else if (c == 'f') {
			/* file name */
			filename = va_arg(argp, char *);
			if (Instr && Instr->IsVirtualFile)
				Fp = (virtualDirCallback.open)(filename, UPNP_READ);
			else
				Fp = fopen(filename, "rb");
			if (Fp == NULL) {
				RetVal = UPNP_E_FILE_READ_ERROR;
				goto ExitFunction;
			}
			if (Instr && Instr->IsRangeActive && Instr->IsVirtualFile) {
				if (virtualDirCallback.seek(Fp, Instr->RangeOffset,
				    SEEK_CUR) != 0) {
					RetVal = UPNP_E_FILE_READ_ERROR;
					goto Cleanup_File;
				}
			} else if (Instr && Instr->IsRangeActive) {
				if (fseeko(Fp, Instr->RangeOffset, SEEK_CUR) != 0) {
					RetVal = UPNP_E_FILE_READ_ERROR;
					goto Cleanup_File;
				}
			}
			while (amount_to_be_read) {
				if (Instr) {
					int nr;
					size_t n = amount_to_be_read >= Data_Buf_Size ?
					    	Data_Buf_Size : amount_to_be_read;
					if (Instr->IsVirtualFile) {
						nr = virtualDirCallback.read(Fp, file_buf, n);
						num_read = (size_t)nr;
					} else {
						num_read = fread(file_buf, (size_t)1, n, Fp);
					}
					amount_to_be_read -= num_read;
					if (Instr->ReadSendSize < 0) {
						/* read until close */
						amount_to_be_read = Data_Buf_Size;
					}
				} else {
					num_read = fread(file_buf, (size_t)1, Data_Buf_Size, Fp);
				}
				if (num_read == (size_t)0) {
					/* EOF so no more to send. */
					if (Instr && Instr->IsChunkActive) {
						const char *str = "0\r\n\r\n";
						nw = sock_write(info, str,
							       strlen(str),
							       TimeOut);
					} else {
						RetVal = UPNP_E_FILE_READ_ERROR;
					}
					goto Cleanup_File;
				}
				/* Create chunk for the current buffer. */
				if (Instr && Instr->IsChunkActive) {
					int rc;
					/* Copy CRLF at the end of the chunk */
					memcpy(file_buf + num_read, "\r\n", (size_t)2);
					/* Hex length for the chunk size. */
					memset(Chunk_Header, 0,
						sizeof(Chunk_Header));
					rc = snprintf(Chunk_Header,
						sizeof(Chunk_Header) - strlen ("\r\n"),
						"%" PRIzx, num_read);
					if (rc < 0 || (unsigned int) rc >= sizeof(Chunk_Header) - strlen ("\r\n")) {
						RetVal = UPNP_E_INTERNAL_ERROR;
						goto Cleanup_File;
					}
					strncat(Chunk_Header, "\r\n", strlen ("\r\n"));
					/* Copy the chunk size header  */
					memcpy(file_buf - strlen(Chunk_Header),
					       Chunk_Header,
					       strlen(Chunk_Header));
					/* on the top of the buffer. */
					/*file_buf[num_read+strlen(Chunk_Header)] = NULL; */
					/*printf("Sending %s\n",file_buf-strlen(Chunk_Header)); */
					nw = sock_write(info,
						file_buf - strlen(Chunk_Header),
						num_read + strlen(Chunk_Header) + (size_t)2,
						TimeOut);
					num_written = (size_t)nw;
					if (nw <= 0 || num_written != num_read + strlen(Chunk_Header) + (size_t)2)
						/* Send error nothing we can do. */
						goto Cleanup_File;
				} else {
					/* write data */
					nw = sock_write(info, file_buf, num_read, TimeOut);
					UpnpPrintf(UPNP_INFO, HTTP, __FILE__, __LINE__,
						   ">>> (SENT) >>>\n%.*s\n------------\n",
						   nw, file_buf);
					/* Send error nothing we can do */
					num_written = (size_t)nw;
					if (nw <= 0 || num_written != num_read) {
						goto Cleanup_File;
					}
				}
			} /* while */
Cleanup_File:
			if (Instr && Instr->IsVirtualFile) {
				virtualDirCallback.close(Fp);
			} else {
				fclose(Fp);
			}
			goto ExitFunction;
		} else
#endif /* EXCLUDE_WEB_SERVER */
		if (c == 'b') {
			/* memory buffer */
			buf = va_arg(argp, char *);
			buf_length = va_arg(argp, size_t);
			if (buf_length > (size_t)0) {
				nw = sock_write(info, buf, buf_length, TimeOut);
				num_written = (size_t)nw;
				UpnpPrintf(UPNP_INFO, HTTP, __FILE__, __LINE__,
					   ">>> (SENT) >>>\n"
					   "%.*s\nbuf_length=%" PRIzd ", num_written=%" PRIzd "\n"
					   "------------\n",
					   (int)buf_length, buf, buf_length, num_written);
				if (num_written != buf_length) {
					RetVal = 0;
					goto ExitFunction;
				}
			}
		}
	}

ExitFunction:
	va_end(argp);
#if EXCLUDE_WEB_SERVER == 0
	free(ChunkBuf);
#endif /* EXCLUDE_WEB_SERVER */
	return RetVal;
}


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
	IN uri_type *destination,
	IN const char *request,
	IN size_t request_length,
	IN http_method_t req_method,
	IN int timeout_secs,
	OUT http_parser_t *response)
{
	SOCKET tcp_connection;
	int ret_code;
	size_t sockaddr_len;
	int http_error_code;
	SOCKINFO info;

	tcp_connection = socket(
		(int)destination->hostport.IPaddress.ss_family, SOCK_STREAM, 0);
	if (tcp_connection == INVALID_SOCKET) {
		parser_response_init(response, req_method);
		return UPNP_E_SOCKET_ERROR;
	}
	if (sock_init(&info, tcp_connection) != UPNP_E_SUCCESS) {
		parser_response_init(response, req_method);
		ret_code = UPNP_E_SOCKET_ERROR;
		goto end_function;
	}
	/* connect */
	sockaddr_len = destination->hostport.IPaddress.ss_family == AF_INET6 ?
		sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in);
	ret_code = private_connect(info.socket,
		(struct sockaddr *)&(destination->hostport.IPaddress),
		(socklen_t)sockaddr_len);
	if (ret_code == -1) {
		parser_response_init(response, req_method);
		ret_code = UPNP_E_SOCKET_CONNECT;
		goto end_function;
	}
	/* send request */
	ret_code = http_SendMessage(&info, &timeout_secs, "b",
		request, request_length);
	if (ret_code != 0) {
		parser_response_init(response, req_method);
		goto end_function;
	}
	/* recv response */
	ret_code = http_RecvMessage(&info, response, req_method,
		&timeout_secs, &http_error_code);

end_function:
	/* should shutdown completely */
	sock_destroy(&info, SD_BOTH);

	return ret_code;
}


/************************************************************************
 * Function: http_Download
 *
 * Parameters:
 *	IN const char* url_str;	String as a URL
 *	IN int timeout_secs;	time out value
 *	OUT char** document;	buffer to store the document extracted
 *				from the donloaded message.
 *	OUT int* doc_length;	length of the extracted document
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
int http_Download( IN const char *url_str,
               IN int timeout_secs,
               OUT char **document,
               OUT size_t *doc_length,
               OUT char *content_type )
{
	int ret_code;
	uri_type url;
	char *msg_start;
	char *entity_start;
	char *hoststr;
	char *temp;
	http_parser_t response;
	size_t msg_length;
	size_t hostlen;
	memptr ctype;
	size_t copy_len;
	membuffer request;
	char *urlPath = alloca(strlen(url_str) + (size_t)1);

	/*ret_code = parse_uri( (char*)url_str, strlen(url_str), &url ); */
	UpnpPrintf(UPNP_INFO, HTTP, __FILE__, __LINE__,
		   "DOWNLOAD URL : %s\n", url_str);
	ret_code = http_FixStrUrl((char *)url_str, strlen(url_str), &url);
	if (ret_code != UPNP_E_SUCCESS)
		return ret_code;
	/* make msg */
	membuffer_init(&request);
	memset(urlPath, 0, strlen(url_str) + (size_t)1);
	strncpy(urlPath, url_str, strlen(url_str));
	hoststr = strstr(urlPath, "//");
	if (hoststr == NULL)
		return UPNP_E_INVALID_URL;
	hoststr += 2;
	temp = strchr(hoststr, '/');
	if (temp) {
		*temp = '\0';
		hostlen = strlen(hoststr);
		*temp = '/';
	} else {
		hostlen = strlen(hoststr);
	}
	UpnpPrintf(UPNP_INFO, HTTP, __FILE__, __LINE__,
		   "HOSTNAME : %s Length : %" PRIzu "\n", hoststr, hostlen);
	ret_code = http_MakeMessage(&request, 1, 1,
				    "Q" "s" "bcDCUc",
				    HTTPMETHOD_GET, url.pathquery.buff,
				    url.pathquery.size, "HOST: ", hoststr,
				    hostlen);
	if (ret_code != 0) {
		UpnpPrintf(UPNP_INFO, HTTP, __FILE__, __LINE__,
			   "HTTP Makemessage failed\n");
		membuffer_destroy(&request);
		return ret_code;
	}
	UpnpPrintf(UPNP_INFO, HTTP, __FILE__, __LINE__,
		   "HTTP Buffer:\n%s\n" "----------END--------\n", request.buf);
	/* get doc msg */
	ret_code =
	    http_RequestAndResponse(&url, request.buf, request.length,
				    HTTPMETHOD_GET, timeout_secs, &response);

	if (ret_code != 0) {
		httpmsg_destroy(&response.msg);
		membuffer_destroy(&request);
		return ret_code;
	}
	UpnpPrintf(UPNP_INFO, HTTP, __FILE__, __LINE__, "Response\n");
	print_http_headers(&response.msg);
	/* optional content-type */
	if (content_type) {
		if (httpmsg_find_hdr(&response.msg, HDR_CONTENT_TYPE, &ctype) ==
		    NULL) {
			*content_type = '\0';	/* no content-type */
		} else {
			/* safety */
			copy_len = ctype.length < LINE_SIZE - (size_t)1 ?
			    ctype.length : LINE_SIZE - (size_t)1;

			memcpy(content_type, ctype.buf, copy_len);
			content_type[copy_len] = '\0';
		}
	}
	/* extract doc from msg */
	if ((*doc_length = response.msg.entity.length) == (size_t)0) {
		/* 0-length msg */
		*document = NULL;
	} else if (response.msg.status_code == HTTP_OK) {
		/*LEAK_FIX_MK */
		/* copy entity */
		entity_start = response.msg.entity.buf;	/* what we want */
		msg_length = response.msg.msg.length;	/* save for posterity    */
		msg_start = membuffer_detach(&response.msg.msg);	/* whole msg */
		/* move entity to the start; copy null-terminator too */
		memmove(msg_start, entity_start, *doc_length + (size_t)1);
		/* save mem for body only */
		*document = realloc(msg_start, *doc_length + (size_t)1);	/*LEAK_FIX_MK */
		/* *document = Realloc( msg_start,msg_length, *doc_length + 1 ); LEAK_FIX_MK */
		/* shrink can't fail */
		assert(msg_length > *doc_length);
		assert(*document != NULL);
		if (msg_length <= *doc_length || *document == NULL)
			UpnpPrintf(UPNP_INFO, HTTP, __FILE__, __LINE__,
				"msg_length(%" PRIzu ") <= *doc_length(%"
				PRIzu ") or document is NULL",
				msg_length, *doc_length);
	}
	if (response.msg.status_code == HTTP_OK) {
		ret_code = 0;	/* success */
	} else {
		/* server sent error msg (not requested doc) */
		ret_code = response.msg.status_code;
	}
	httpmsg_destroy(&response.msg);
	membuffer_destroy(&request);

	return ret_code;
}


/************************************************************************
 * Function: MakeGenericMessage
 *
 * Parameters:
 *  http_method_t method;   The type of HTTP method.
 *	const char *url_str;		String as a URL
 *	membuffer *request;		Buffer containing the request
 *	uri_type *url; 			URI object containing the scheme,
 *					path query token, etc.
 *	int contentLength;		length of content
 *	const char *contentType;	Type of content
 *
 * Description:
 *	Makes the message for the HTTP POST message
 *
 * Returns:
 *	UPNP_E_INVALID_URL
 * 	UPNP_E_INVALID_PARAM
 *	UPNP_E_SUCCESS
 ************************************************************************/
int MakeGenericMessage(http_method_t method,
		       const char *url_str, membuffer *request,
		       uri_type *url, int contentLength, const char *contentType,
		       const UpnpString *headers)
{
	int ret_code = 0;
	size_t hostlen = (size_t)0;
	char *hoststr;

	UpnpPrintf(UPNP_INFO, HTTP, __FILE__, __LINE__,
		   "URL: %s method: %d\n", url_str, method);
	ret_code = http_FixStrUrl(url_str, strlen(url_str), url);
	if (ret_code != UPNP_E_SUCCESS)
		return ret_code;
	/* make msg */
	membuffer_init(request);
	ret_code = http_MakeMessage(request, 1, 1, "Q",
				    method,
				    url->pathquery.buff,
				    url->pathquery.size);
	/* add request headers if specified, otherwise use default headers */
	if (ret_code == 0) {
		if (headers) {
			ret_code = http_MakeMessage(request, 1, 1,
						    "s",
						    UpnpString_get_String(headers));
		}
		else {
			ret_code = get_hoststr(url_str, &hoststr, &hostlen);
			if (ret_code != UPNP_E_SUCCESS)
				return ret_code;
			UpnpPrintf(UPNP_INFO, HTTP, __FILE__, __LINE__,
				   "HOSTNAME : %s Length : %" PRIzu "\n", hoststr, hostlen);
			ret_code = http_MakeMessage(request, 1, 1,
						    "s" "bcDCU",
						    "HOST: ", hoststr, hostlen);
		}
	}

	/* add the content-type header */
	if (ret_code == 0 && contentType) {
		ret_code = http_MakeMessage(request, 1, 1,
					    "T",
					    contentType);
	}
	/* add content-length header. */
	if (ret_code == 0) {
		if (contentLength >= 0)
			ret_code = http_MakeMessage(request, 1, 1, "Nc",
						    (off_t) contentLength);
		else if (contentLength == UPNP_USING_CHUNKED)
			ret_code = http_MakeMessage(request, 1, 1, "Kc");
		else if (contentLength == UPNP_UNTIL_CLOSE)
			ret_code = http_MakeMessage(request, 1, 1, "c");
		else
			ret_code = UPNP_E_INVALID_PARAM;
	}
	if (ret_code != 0) {
		UpnpPrintf(UPNP_INFO, HTTP, __FILE__, __LINE__,
			   "HTTP Makemessage failed\n");
		membuffer_destroy(request);
		return ret_code;
	}
	UpnpPrintf(UPNP_INFO, HTTP, __FILE__, __LINE__,
		   "HTTP Buffer:\n%s\n" "----------END--------\n",
		   request->buf);

	return UPNP_E_SUCCESS;
}

typedef struct HTTPCONNECTIONHANDLE {
	SOCKINFO sock_info;
	int contentLength;
	http_parser_t response;
	int requestStarted;
	int cancel;
} http_connection_handle_t;


/*!
 * \brief Parses already exiting data. If not complete reads more 
 * data on the connected socket. The read data is then parsed. The 
 * same methid is carried out for headers.
 *
 * \return integer:
 *	\li \c PARSE_OK - On Success
 *	\li \c PARSE_FAILURE - Failure to parse data correctly
 *	\li \c UPNP_E_BAD_HTTPMSG - Socker read() returns an error
 */
static int ReadResponseLineAndHeaders(
	/*! Socket information object. */
	IN SOCKINFO *info,
	/*! HTTP Parser object. */
	IN OUT http_parser_t *parser,
	/*! Time out value. */
	IN OUT int *timeout_secs,
	/*! HTTP errror code returned. */
	IN OUT int *http_error_code)
{
	parse_status_t status;
	int num_read;
	char buf[2 * 1024];
	int done = 0;
	int ret_code = 0;

	/*read response line */
	status = parser_parse_responseline(parser);
	switch (status) {
	case PARSE_OK:
		done = 1;
		break;
	case PARSE_INCOMPLETE:
		done = 0;
		break;
	default:
		/*error */
		return status;
	}
	while (!done) {
		num_read = sock_read(info, buf, sizeof(buf), timeout_secs);
		if (num_read > 0) {
			/* append data to buffer */
			ret_code =
				membuffer_append(&parser->msg.msg, buf, (size_t)num_read);
			if (ret_code != 0) {
				/* set failure status */
				parser->http_error_code =
				    HTTP_INTERNAL_SERVER_ERROR;
				return PARSE_FAILURE;
			}
			status = parser_parse_responseline(parser);
			switch (status) {
			case PARSE_OK:
				done = 1;
				break;
			case PARSE_INCOMPLETE:
				done = 0;
				break;
			default:
				/*error */
				return status;
			}
		} else if (num_read == 0) {
			/* partial msg */
			*http_error_code = HTTP_BAD_REQUEST;	/* or response */
			return UPNP_E_BAD_HTTPMSG;
		} else {
			*http_error_code = parser->http_error_code;
			return num_read;
		}
	}
	status = parser_parse_headers(parser);
	if ((status == (parse_status_t)PARSE_OK) &&
		(parser->position == (parser_pos_t)POS_ENTITY))
		done = 1;
	else if (status == (parse_status_t)PARSE_INCOMPLETE)
		done = 0;
	else
		/*error */
		return status;
	/*read headers */
	while (!done) {
		num_read = sock_read(info, buf, sizeof(buf), timeout_secs);
		if (num_read > 0) {
			/* append data to buffer */
			ret_code =
			    membuffer_append(&parser->msg.msg, buf, (size_t)num_read);
			if (ret_code != 0) {
				/* set failure status */
				parser->http_error_code = HTTP_INTERNAL_SERVER_ERROR;
				return PARSE_FAILURE;
			}
			status = parser_parse_headers(parser);
			if (status == (parse_status_t)PARSE_OK &&
				parser->position == (parser_pos_t)POS_ENTITY)
				done = 1;
			else if (status == (parse_status_t)PARSE_INCOMPLETE)
				done = 0;
			else
				/*error */
				return status;
		} else if (num_read == 0) {
			/* partial msg */
			*http_error_code = HTTP_BAD_REQUEST;	/* or response */
			return UPNP_E_BAD_HTTPMSG;
		} else {
			*http_error_code = parser->http_error_code;
			return num_read;
		}
	}

	return PARSE_OK;
}


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
	OUT size_t *total)
{
	http_connection_handle_t *handle = Handle;

	if (!handle || !length || !total) {
		return UPNP_E_INVALID_PARAM;
	}
	*length = handle->response.msg.entity.length;
	*total = handle->response.content_length;

	return UPNP_E_SUCCESS;
}

/************************************************************************
 * Function: http_CancelHttpGet
 *
 * Parameters:
 *	IN void *Handle;	Handle to HTTP get object
 *
 * Description:
 *	Set the cancel flag of the HttpGet handle
 *
 * Return: int
 *	UPNP_E_SUCCESS		- On Success
 *	UPNP_E_INVALID_PARAM	- Invalid Parameter
 ************************************************************************/
int http_CancelHttpGet(IN void *Handle)
{
	http_connection_handle_t *handle = Handle;

	if (!handle)
		return UPNP_E_INVALID_PARAM;
	handle->cancel = 1;

	return UPNP_E_SUCCESS;
}


int http_OpenHttpConnection(const char *url_str, void **Handle, int timeout)
{
	int ret_code;
	size_t sockaddr_len;
	SOCKET tcp_connection;
	http_connection_handle_t *handle = NULL;
	uri_type url;
	if (!url_str || !Handle)
		return UPNP_E_INVALID_PARAM;
	*Handle = handle;
	/* parse url_str */
	ret_code = http_FixStrUrl(url_str, strlen(url_str), &url);
	if (ret_code != UPNP_E_SUCCESS)
		return ret_code;
	/* create the handle */
	handle = malloc(sizeof(http_connection_handle_t));
	if (!handle) {
		return UPNP_E_OUTOF_MEMORY;
	}
	handle->requestStarted = 0;
	memset(&handle->response, 0, sizeof(handle->response));
	/* connect to the server */
	tcp_connection = socket(url.hostport.IPaddress.ss_family, SOCK_STREAM, 0);
	if (tcp_connection == INVALID_SOCKET) {
		ret_code = UPNP_E_SOCKET_ERROR;
		goto errorHandler;
	}
	if (sock_init(&handle->sock_info, tcp_connection) != UPNP_E_SUCCESS) {
		sock_destroy(&handle->sock_info, SD_BOTH);
		ret_code = UPNP_E_SOCKET_ERROR;
		goto errorHandler;
	}
	sockaddr_len = url.hostport.IPaddress.ss_family == AF_INET6 ?
		sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in);
	ret_code = private_connect(handle->sock_info.socket,
				   (struct sockaddr *)&(url.hostport.IPaddress),
				   (socklen_t) sockaddr_len);
	if (ret_code == -1) {
		sock_destroy(&handle->sock_info, SD_BOTH);
		ret_code = UPNP_E_SOCKET_CONNECT;
		goto errorHandler;
	}
#ifdef UPNP_ENABLE_OPEN_SSL
	/* For HTTPS connections start the TLS/SSL handshake. */
	if (token_string_casecmp(&url.scheme, "https") == 0) {
		ret_code = sock_ssl_connect(&handle->sock_info);
		if (ret_code != UPNP_E_SUCCESS) {
			sock_destroy(&handle->sock_info, SD_BOTH);
			goto errorHandler;
		}
	}
#endif
errorHandler:
	*Handle = handle;
	return ret_code;
	/* Unused parameter */
	timeout = timeout;
}

int http_MakeHttpRequest(Upnp_HttpMethod method,
			 const char *url_str, void *Handle, UpnpString *headers,
			 const char *contentType, int contentLength,
			 int timeout)
{
	int ret_code;
	membuffer request;
	http_connection_handle_t *handle = Handle;
	uri_type url;
	if (!url_str || !Handle)
		return UPNP_E_INVALID_PARAM;
	if (handle->requestStarted) {
		/* TODO: Log an error that a previous request is already in progress. */
	}
	handle->requestStarted = 1;
	handle->cancel = 0;
	ret_code = MakeGenericMessage(method, url_str, &request, &url, contentLength,
				      contentType, headers);
	if (ret_code != UPNP_E_SUCCESS)
		return ret_code;
	/* send request */
	ret_code = http_SendMessage(&handle->sock_info, &timeout, "b",
				    request.buf, request.length);
	membuffer_destroy(&request);
	httpmsg_destroy(&handle->response.msg);
	parser_response_init(&handle->response, method);
	return ret_code;
}

int http_WriteHttpRequest(void *Handle, char *buf,
			  size_t *size, int timeout)
{
	http_connection_handle_t *handle = (http_connection_handle_t *)Handle;
	char *tempbuf = NULL;
	size_t tempbufSize = 0;
	int freeTempbuf = 0;
	int numWritten = 0;

	if (!handle || !size || !buf) {
		if (size)
			*size = 0;
		return UPNP_E_INVALID_PARAM;
	}
	if (handle->contentLength == UPNP_USING_CHUNKED) {
		if (*size) {
			size_t tempSize = 0;
			tempbuf = malloc(*size +
					 CHUNK_HEADER_SIZE + CHUNK_TAIL_SIZE);
			if (!tempbuf)
				return UPNP_E_OUTOF_MEMORY;
			/* begin chunk */
			sprintf(tempbuf, "%zx\r\n", *size);
			tempSize = strlen(tempbuf);
			memcpy(tempbuf + tempSize, buf, *size);
			memcpy(tempbuf + tempSize + *size, "\r\n", 2);
			/* end of chunk */
			tempbufSize = tempSize + *size + 2;
			freeTempbuf = 1;
		}
	} else {
		tempbuf = buf;
		tempbufSize = *size;
	}
	numWritten =
		sock_write(&handle->sock_info, tempbuf, tempbufSize, &timeout);
	if (freeTempbuf)
		free(tempbuf);
	if (numWritten < 0) {
		*size = 0;
		return numWritten;
	} else {
		*size = (size_t)numWritten;
		return UPNP_E_SUCCESS;
	}
}

int http_EndHttpRequest(void *Handle, int timeout)
{
	int retc = 0;
	const char *zcrlf = "0\r\n\r\n";
	http_connection_handle_t *handle = Handle;
	if (!handle)
		return UPNP_E_INVALID_PARAM;
	if (!handle->requestStarted) {
		return UPNP_E_SUCCESS;
	}
	handle->requestStarted = 0;
	if (handle->contentLength == UPNP_USING_CHUNKED)
		/*send last chunk */
		retc = sock_write(&handle->sock_info, zcrlf, strlen(zcrlf), &timeout);

	return retc >= 0 ? UPNP_E_SUCCESS : UPNP_E_SOCKET_WRITE;
}

int http_GetHttpResponse(void *Handle, UpnpString *headers,
			 char **contentType, int *contentLength,
			 int *httpStatus, int timeout)
{
	int ret_code;
	int http_error_code;
	memptr ctype;
	http_connection_handle_t *handle = Handle;
	parse_status_t status;

	status = ReadResponseLineAndHeaders(&handle->sock_info,
					    &handle->response, &timeout,
					    &http_error_code);
	if (status != (parse_status_t)PARSE_OK) {
		ret_code = UPNP_E_BAD_RESPONSE;
		goto errorHandler;
	}
	status = parser_get_entity_read_method(&handle->response);
	switch (status) {
	case PARSE_CONTINUE_1:
	case PARSE_SUCCESS:
		break;
	default:
		ret_code = UPNP_E_BAD_RESPONSE;
		goto errorHandler;
	}
	ret_code = UPNP_E_SUCCESS;
	if (httpStatus) {
		*httpStatus = handle->response.msg.status_code;
	}
	if (contentType) {
		if (!httpmsg_find_hdr(&handle->response.msg, HDR_CONTENT_TYPE, &ctype))
			/* no content-type */
			*contentType = NULL;
		else
			*contentType = ctype.buf;
	}
	if (contentLength) {
		if (handle->response.position == (parser_pos_t)POS_COMPLETE)
			*contentLength = 0;
		else if (handle->response.ent_position == ENTREAD_USING_CHUNKED)
			*contentLength = UPNP_USING_CHUNKED;
		else if (handle->response.ent_position == ENTREAD_USING_CLEN)
			*contentLength = (int)handle->response.content_length;
		else if (handle->response.ent_position == ENTREAD_UNTIL_CLOSE)
			*contentLength = UPNP_UNTIL_CLOSE;
	}

	if (headers) {
		copy_msg_headers(&handle->response.msg.headers, headers);

	}

errorHandler:
	if (ret_code != UPNP_E_SUCCESS)
		httpmsg_destroy(&handle->response.msg);
	return ret_code;
}

int http_ReadHttpResponse(void *Handle, char *buf, size_t *size, int timeout)
{
	http_connection_handle_t *handle = Handle;
	parse_status_t status;
	int num_read;
	int ok_on_close = FALSE;
	char tempbuf[2 * 1024];
	int ret_code = 0;

	if (!handle || !size || (*size > 0 && !buf)) {
		if (size)
			*size = 0;
		return UPNP_E_INVALID_PARAM;
	}
	/* first parse what has already been gotten */
	if (handle->response.position != POS_COMPLETE)
		status = parser_parse_entity(&handle->response);
	else
		status = PARSE_SUCCESS;
	if (status == PARSE_INCOMPLETE_ENTITY)
		/* read until close */
		ok_on_close = TRUE;
	else if ((status != PARSE_SUCCESS)
		 && (status != PARSE_CONTINUE_1)
		 && (status != PARSE_INCOMPLETE)) {
		/*error */
		*size = 0;
		return UPNP_E_BAD_RESPONSE;
	}
	/* read more if necessary entity */
	while (handle->response.msg.amount_discarded + *size >
	       handle->response.msg.entity.length &&
	       !handle->cancel &&
	       handle->response.position != POS_COMPLETE) {
		num_read = sock_read(&handle->sock_info, tempbuf,
				     sizeof(tempbuf), &timeout);
		if (num_read > 0) {
			/* append data to buffer */
			ret_code = membuffer_append(&handle->response.msg.msg,
						    tempbuf, (size_t)num_read);
			if (ret_code != 0) {
				/* set failure status */
				handle->response.http_error_code =
					HTTP_INTERNAL_SERVER_ERROR;
				*size = 0;
				return PARSE_FAILURE;
			}
			status = parser_parse_entity(&handle->response);
			if (status == PARSE_INCOMPLETE_ENTITY) {
				/* read until close */
				ok_on_close = TRUE;
			} else if ((status != PARSE_SUCCESS)
				   && (status != PARSE_CONTINUE_1)
				   && (status != PARSE_INCOMPLETE)) {
				/*error */
				*size = 0;
				return UPNP_E_BAD_RESPONSE;
			}
		} else if (num_read == 0) {
			if (ok_on_close) {
				UpnpPrintf(UPNP_INFO, HTTP, __FILE__, __LINE__,
					   "<<< (RECVD) <<<\n%s\n-----------------\n",
					   handle->response.msg.msg.buf);
				handle->response.position = POS_COMPLETE;
			} else {
				/* partial msg */
				*size = 0;
				handle->response.http_error_code = HTTP_BAD_REQUEST;	/* or response */
				return UPNP_E_BAD_HTTPMSG;
			}
		} else {
			*size = 0;
			return num_read;
		}
	}
	if (handle->cancel) {
		return UPNP_E_CANCELED;
	}
	/* truncate size to fall within available data */
	if (handle->response.msg.amount_discarded + *size >
	    handle->response.msg.entity.length)
		*size = handle->response.msg.entity.length -
			handle->response.msg.amount_discarded;
	/* copy data to user buffer. delete copied data */
	if (*size > 0) {
		memcpy(buf, &handle->response.msg.msg.buf[handle->response.entity_start_position],
		       *size);
		membuffer_delete(&handle->response.msg.msg,
				 handle->response.entity_start_position, *size);
		/* update scanner position. needed for chunked transfers */
		handle->response.scanner.cursor -= *size;
		/* update amount discarded */
		handle->response.msg.amount_discarded += *size;
	}

	return UPNP_E_SUCCESS;
}

int http_CloseHttpConnection(void *Handle)
{
	http_connection_handle_t *handle = Handle;
	if (!handle)
		return UPNP_E_INVALID_PARAM;
	/*should shutdown completely */
	sock_destroy(&handle->sock_info, SD_BOTH);
	httpmsg_destroy(&handle->response.msg);
	free(handle);
	return UPNP_E_SUCCESS;
}

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
int http_SendStatusResponse(IN SOCKINFO *info, IN int http_status_code,
	IN int request_major_version, IN int request_minor_version)
{
	int response_major, response_minor;
	membuffer membuf;
	int ret;
	int timeout;

	http_CalcResponseVersion(request_major_version, request_minor_version,
				 &response_major, &response_minor);
	membuffer_init(&membuf);
	membuf.size_inc = (size_t)70;
	/* response start line */
	ret = http_MakeMessage(&membuf, response_major, response_minor, "RSCB",
			       http_status_code, http_status_code);
	if (ret == 0) {
		timeout = HTTP_DEFAULT_TIMEOUT;
		ret = http_SendMessage(info, &timeout, "b",
		       membuf.buf, membuf.length);
	}
	membuffer_destroy(&membuf);

	return ret;
}

int http_MakeMessage(membuffer *buf, int http_major_version,
	int http_minor_version, const char *fmt, ...)
{
	char c;
	char *s = NULL;
	size_t num;
	off_t bignum;
	size_t length;
	time_t *loc_time;
	time_t curr_time;
	struct tm date_storage;
	struct tm *date;
	const char *start_str;
	const char *end_str;
	int status_code;
	const char *status_msg;
	http_method_t method;
	const char *method_str;
	const char *url_str;
	const char *temp_str;
	uri_type url;
	uri_type *uri_ptr;
	int error_code = 0;
	va_list argp;
	char tempbuf[200];
	const char *weekday_str = "Sun\0Mon\0Tue\0Wed\0Thu\0Fri\0Sat";
	const char *month_str = "Jan\0Feb\0Mar\0Apr\0May\0Jun\0"
	    "Jul\0Aug\0Sep\0Oct\0Nov\0Dec";
	int rc = 0;

	memset(tempbuf, 0, sizeof(tempbuf));
	va_start(argp, fmt);
	while ((c = *fmt++)) {
		if (c == 's') {
			/* C string */
			s = (char *)va_arg(argp, char *);
			assert(s);
			UpnpPrintf(UPNP_ALL, HTTP, __FILE__, __LINE__,
				   "Adding a string : %s\n", s);
			if (membuffer_append(buf, s, strlen(s)))
				goto error_handler;
		} else if (c == 'K') {
			/* Add Chunky header */
			if (membuffer_append(buf, "TRANSFER-ENCODING: chunked\r\n",
				strlen("Transfer-Encoding: chunked\r\n")))
				goto error_handler;
		} else if (c == 'G') {
			/* Add Range header */
			struct SendInstruction *RespInstr;
			RespInstr = (struct SendInstruction *)
			    va_arg(argp, struct SendInstruction *);
			assert(RespInstr);
			/* connection header */
			if (membuffer_append(buf, RespInstr->RangeHeader,
				strlen(RespInstr->RangeHeader)))
				goto error_handler;
		} else if (c == 'b') {
			/* mem buffer */
			s = (char *)va_arg(argp, char *);
			UpnpPrintf(UPNP_ALL, HTTP, __FILE__, __LINE__,
				"Adding a char Buffer starting with: %c\n", (int)s[0]);
			assert(s);
			length = (size_t) va_arg(argp, size_t);
			if (membuffer_append(buf, s, length))
				goto error_handler;
		} else if (c == 'c') {
			/* crlf */
			if (membuffer_append(buf, "\r\n", (size_t)2))
				goto error_handler;
		} else if (c == 'd') {
			/* integer */
			num = (size_t)va_arg(argp, int);
			rc = snprintf(tempbuf, sizeof(tempbuf), "%" PRIzu, num);
			if (rc < 0 || (unsigned int) rc >= sizeof(tempbuf) ||
				membuffer_append(buf, tempbuf, strlen(tempbuf)))
				goto error_handler;
		} else if (c == 'h') {
			/* off_t */
			bignum = (off_t) va_arg(argp, off_t);
			rc = snprintf(tempbuf, sizeof(tempbuf), "%" PRId64,
				(int64_t) bignum);
			if (rc < 0 || (unsigned int) rc >= sizeof(tempbuf) ||
				membuffer_append(buf, tempbuf, strlen(tempbuf)))
				goto error_handler;
		} else if (c == 't' || c == 'D') {
			/* date */
			if (c == 'D') {
				/* header */
				start_str = "DATE: ";
				end_str = "\r\n";
				curr_time = time(NULL);
				loc_time = &curr_time;
			} else {
				/* date value only */
				start_str = end_str = "";
				loc_time = (time_t *)va_arg(argp, time_t *);
			}
			assert(loc_time);
			date = http_gmtime_r(loc_time, &date_storage);
			if (date == NULL)
				goto error_handler;
			rc = snprintf(tempbuf, sizeof(tempbuf),
				"%s%s, %02d %s %d %02d:%02d:%02d GMT%s",
				start_str, &weekday_str[date->tm_wday * 4],
				date->tm_mday, &month_str[date->tm_mon * 4],
				date->tm_year + 1900, date->tm_hour,
				date->tm_min, date->tm_sec, end_str);
			if (rc < 0 || (unsigned int) rc >= sizeof(tempbuf) ||
				membuffer_append(buf, tempbuf, strlen(tempbuf)))
				goto error_handler;
		} else if (c == 'L') {
			/* Add CONTENT-LANGUAGE header only if WEB_SERVER_CONTENT_LANGUAGE */
			/* is not empty and if Accept-Language header is not empty */
			struct SendInstruction *RespInstr;
			RespInstr = (struct SendInstruction *)
			    va_arg(argp, struct SendInstruction *);
			assert(RespInstr);
			if (strcmp(RespInstr->AcceptLanguageHeader, "") &&
			    strcmp(WEB_SERVER_CONTENT_LANGUAGE, "") &&
			    http_MakeMessage(buf, http_major_version,
					http_minor_version, "ssc",
					"CONTENT-LANGUAGE: ",
					WEB_SERVER_CONTENT_LANGUAGE) != 0)
				goto error_handler;
		} else if (c == 'C') {
			if ((http_major_version > 1) ||
			    (http_major_version == 1 && http_minor_version == 1)
			    ) {
				/* connection header */
				if (membuffer_append_str(buf, "CONNECTION: close\r\n"))
					goto error_handler;
			}
		} else if (c == 'N') {
			/* content-length header */
			bignum = (off_t) va_arg(argp, off_t);
			assert(bignum >= 0);
			if (http_MakeMessage(buf, http_major_version, http_minor_version,
					     "shc", "CONTENT-LENGTH: ", bignum) != 0)
				goto error_handler;
		} else if (c == 'S' || c == 'U') {
			/* SERVER or USER-AGENT header */
			temp_str = (c == 'S') ? "SERVER: " : "USER-AGENT: ";
			get_sdk_info(tempbuf, sizeof(tempbuf));
			if (http_MakeMessage(buf, http_major_version, http_minor_version,
					     "ss", temp_str, tempbuf) != 0)
				goto error_handler;
		} else if (c == 'X') {
			/* C string */
			s = (char *)va_arg(argp, char *);
			assert(s);
			if (membuffer_append_str(buf, "X-User-Agent: ") != 0)
				goto error_handler;
			if (membuffer_append(buf, s, strlen(s)) != 0)
				goto error_handler;
		} else if (c == 'R') {
			/* response start line */
			/*   e.g.: 'HTTP/1.1 200 OK' code */
			status_code = (int)va_arg(argp, int);
			assert(status_code > 0);
			rc = snprintf(tempbuf, sizeof(tempbuf), "HTTP/%d.%d %d ",
				http_major_version, http_minor_version,
				status_code);
			/* str */
			status_msg = http_get_code_text(status_code);
			if (rc < 0 || (unsigned int) rc >= sizeof(tempbuf) ||
				http_MakeMessage(buf, http_major_version, http_minor_version,
					     "ssc", tempbuf, status_msg) != 0)
				goto error_handler;
		} else if (c == 'B') {
			/* body of a simple reply */
			status_code = (int)va_arg(argp, int);
			rc = snprintf(tempbuf, sizeof(tempbuf), "%s%d %s%s",
				"<html><body><h1>",
				status_code, http_get_code_text(status_code),
				"</h1></body></html>");
			if (rc < 0 || (unsigned int) rc >= sizeof(tempbuf))
				goto error_handler;
			bignum = (off_t)strlen(tempbuf);
			if (http_MakeMessage(buf, http_major_version, http_minor_version,
					     "NTcs", bignum,	/* content-length */
					     "text/html",	/* content-type */
					     tempbuf) != 0	/* body */
			    )
				goto error_handler;
		} else if (c == 'Q') {
			/* request start line */
			/* GET /foo/bar.html HTTP/1.1\r\n */
			method = (http_method_t) va_arg(argp, http_method_t);
			method_str = method_to_str(method);
			url_str = (const char *)va_arg(argp, const char *);
			num = (size_t) va_arg(argp, size_t);		/* length of url_str */
			if (http_MakeMessage(buf, http_major_version, http_minor_version,
					     "ssbsdsdc", method_str,	/* method */
					     " ", url_str, num,		/* url */
					     " HTTP/", http_major_version, ".",
					     http_minor_version) != 0)
				goto error_handler;
		} else if (c == 'q') {
			/* request start line and HOST header */
			method = (http_method_t) va_arg(argp, http_method_t);
			uri_ptr = (uri_type *) va_arg(argp, uri_type *);
			assert(uri_ptr);
			if (http_FixUrl(uri_ptr, &url) != 0) {
				error_code = UPNP_E_INVALID_URL;
				goto error_handler;
			}
			if (http_MakeMessage(buf, http_major_version, http_minor_version,
					"Q" "sbc", method, url.pathquery.buff,
					url.pathquery.size, "HOST: ",
					url.hostport.text.buff,
					url.hostport.text.size) != 0)
				goto error_handler;
		} else if (c == 'T') {
			/* content type header */
			temp_str = (const char *)va_arg(argp, const char *);	/* type/subtype format */
			if (http_MakeMessage(buf, http_major_version, http_minor_version,
					     "ssc", "CONTENT-TYPE: ", temp_str) != 0)
				goto error_handler;
		} else {
			assert(0);
		}
	}
	goto ExitFunction;

error_handler:
	/* Default is out of memory error. */
	if (!error_code)
		error_code = UPNP_E_OUTOF_MEMORY;
	membuffer_destroy(buf);

ExitFunction:
	va_end(argp);
	return error_code;
}


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
void http_CalcResponseVersion( IN int request_major_vers,
                          IN int request_minor_vers,
                          OUT int *response_major_vers,
                          OUT int *response_minor_vers)
{
	if ((request_major_vers > 1) ||
	    (request_major_vers == 1 && request_minor_vers >= 1)) {
		*response_major_vers = 1;
		*response_minor_vers = 1;
	} else {
		*response_major_vers = request_major_vers;
		*response_minor_vers = request_minor_vers;
	}
}

/************************************************************************
* Function: MakeGetMessageEx
*
* Parameters:
*	const char *url_str;	String as a URL
*	membuffer *request;	Buffer containing the request
*	uri_type *url; 		URI object containing the scheme, path
*				query token, etc.
*
* Description:
*	Makes the message for the HTTP GET method
*
* Returns:
*	UPNP_E_INVALID_URL
* 	Error Codes returned by http_MakeMessage
*	UPNP_E_SUCCESS
************************************************************************/
int MakeGetMessageEx( const char *url_str,
                  membuffer * request,
                  uri_type * url,
                  struct SendInstruction *pRangeSpecifier)
{
	int errCode = UPNP_E_SUCCESS;
	char *urlPath = NULL;
	size_t hostlen = (size_t)0;
	char *hoststr, *temp;

	do {
		UpnpPrintf(UPNP_INFO, HTTP, __FILE__, __LINE__,
			   "DOWNLOAD URL : %s\n", url_str);
		if ((errCode = http_FixStrUrl((char *)url_str,
					      strlen(url_str),
					      url)) != UPNP_E_SUCCESS) {
			break;
		}
		/* make msg */
		membuffer_init(request);
		urlPath = alloca(strlen(url_str) + (size_t)1);
		if (!urlPath) {
			errCode = UPNP_E_OUTOF_MEMORY;
			break;
		}
		memset(urlPath, 0, strlen(url_str) + (size_t)1);
		strncpy(urlPath, url_str, strlen(url_str));
		hoststr = strstr(urlPath, "//");
		if (hoststr == NULL) {
			errCode = UPNP_E_INVALID_URL;
			break;
		}
		hoststr += 2;
		temp = strchr(hoststr, '/');
		if (temp == NULL) {
			errCode = UPNP_E_INVALID_URL;
			break;
		}
		*temp = '\0';
		hostlen = strlen(hoststr);
		*temp = '/';
		UpnpPrintf(UPNP_INFO, HTTP, __FILE__, __LINE__,
			   "HOSTNAME : %s Length : %" PRIzu "\n",
			   hoststr, hostlen);
		errCode = http_MakeMessage(request, 1, 1,
					   "Q" "s" "bc" "GDCUc",
					   HTTPMETHOD_GET, url->pathquery.buff,
					   url->pathquery.size, "HOST: ",
					   hoststr, hostlen, pRangeSpecifier);
		if (errCode != 0) {
			UpnpPrintf(UPNP_INFO, HTTP, __FILE__, __LINE__,
				   "HTTP Makemessage failed\n");
			membuffer_destroy(request);
			return errCode;
		}
	} while (0);
	UpnpPrintf(UPNP_INFO, HTTP, __FILE__, __LINE__,
		   "HTTP Buffer:\n%s\n" "----------END--------\n",
		   request->buf);

	return errCode;
}

#define SIZE_RANGE_BUFFER 50

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
int http_OpenHttpGetEx(
	IN const char *url_str,
	IN OUT void **Handle,
	IN OUT char **contentType,
	OUT int *contentLength,
	OUT int *httpStatus,
	IN int lowRange,
	IN int highRange,
	IN int timeout)
{
	int http_error_code;
	memptr ctype;
	SOCKET tcp_connection;
	size_t sockaddr_len;
	membuffer request;
	http_connection_handle_t *handle = NULL;
	uri_type url;
	parse_status_t status;
	int errCode = UPNP_E_SUCCESS;
	/* char rangeBuf[SIZE_RANGE_BUFFER]; */
	struct SendInstruction rangeBuf;
	int rc = 0;

	membuffer_init(&request);

	do {
		/* Checking Input parameters */
		if (!url_str || !Handle || !contentType || !httpStatus ) {
			errCode = UPNP_E_INVALID_PARAM;
			break;
		}
		/* Initialize output parameters */
		*httpStatus = 0;
		*Handle = handle;
		*contentType = NULL;
		*contentLength = 0;
		if (lowRange > highRange) {
			errCode = UPNP_E_INTERNAL_ERROR;
			break;
		}
		memset(&rangeBuf, 0, sizeof(rangeBuf));
		rc = snprintf(rangeBuf.RangeHeader, sizeof(rangeBuf.RangeHeader),
			"Range: bytes=%d-%d\r\n", lowRange, highRange);
		if (rc < 0 || (unsigned int) rc >= sizeof(rangeBuf.RangeHeader))
			break;
		membuffer_init(&request);
		errCode = MakeGetMessageEx(url_str, &request, &url, &rangeBuf);
		if (errCode != UPNP_E_SUCCESS)
			break;
		handle = (http_connection_handle_t *)malloc(sizeof(http_connection_handle_t));
		if (!handle) {
			errCode = UPNP_E_OUTOF_MEMORY;
			break;
		}
		memset(handle, 0, sizeof(*handle));
		parser_response_init(&handle->response, HTTPMETHOD_GET);
		tcp_connection = socket((int)url.hostport.IPaddress.ss_family,
			SOCK_STREAM, 0);
		if (tcp_connection == INVALID_SOCKET) {
			errCode = UPNP_E_SOCKET_ERROR;
			free(handle);
			break;
		}
		if (sock_init(&handle->sock_info, tcp_connection) != UPNP_E_SUCCESS) {
			sock_destroy(&handle->sock_info, SD_BOTH);
			errCode = UPNP_E_SOCKET_ERROR;
			free(handle);
			break;
		}
		sockaddr_len = url.hostport.IPaddress.ss_family == AF_INET6 ?
			sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in);
		errCode  = private_connect(handle->sock_info.socket,
			(struct sockaddr *)&(url.hostport.IPaddress),
			(socklen_t)sockaddr_len);
		if (errCode == -1) {
			sock_destroy(&handle->sock_info, SD_BOTH);
			errCode = UPNP_E_SOCKET_CONNECT;
			free(handle);
			break;
		}
		/* send request */
		errCode = http_SendMessage(&handle->sock_info, &timeout,
			"b", request.buf, request.length);
		if (errCode != UPNP_E_SUCCESS) {
			sock_destroy(&handle->sock_info, SD_BOTH);
			free(handle);
			break;
		}
		if (ReadResponseLineAndHeaders(&handle->sock_info,
			&handle->response, &timeout, &http_error_code) != (int)PARSE_OK) {
			errCode = UPNP_E_BAD_RESPONSE;
			free(handle);
			break;
		}
		status = parser_get_entity_read_method(&handle->response);
		if (status != (parse_status_t)PARSE_CONTINUE_1 &&
			status != (parse_status_t)PARSE_SUCCESS) {
			errCode = UPNP_E_BAD_RESPONSE;
			free(handle);
			break;
		}
		*httpStatus = handle->response.msg.status_code;
		errCode = UPNP_E_SUCCESS;

		if (!httpmsg_find_hdr(&handle->response.msg, HDR_CONTENT_TYPE, &ctype))
			/* no content-type */
			*contentType = NULL;
		else
			*contentType = ctype.buf;
		if (handle->response.position == (parser_pos_t)POS_COMPLETE)
			*contentLength = 0;
		else if(handle->response.ent_position == ENTREAD_USING_CHUNKED)
			*contentLength = UPNP_USING_CHUNKED;
		else if(handle->response.ent_position == ENTREAD_USING_CLEN)
			*contentLength = (int)handle->response.content_length;
		else if(handle->response.ent_position == ENTREAD_UNTIL_CLOSE)
			*contentLength = UPNP_UNTIL_CLOSE;
		*Handle = handle;
	} while (0);

	membuffer_destroy(&request);

	return errCode;
}


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
/* 'info' should have a size of at least 100 bytes */
void get_sdk_info(OUT char *info, IN size_t infoSize)
{
#ifdef UPNP_ENABLE_UNSPECIFIED_SERVER
	snprintf(info, infoSize, "Unspecified, UPnP/1.0, Unspecified\r\n");
#else /* UPNP_ENABLE_UNSPECIFIED_SERVER */
#ifdef WIN32
	OSVERSIONINFO versioninfo;
	versioninfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	if (GetVersionEx(&versioninfo) != 0)
		snprintf(info, infoSize,
			"%d.%d.%d %d/%s, UPnP/1.0, Portable SDK for UPnP devices/"
			PACKAGE_VERSION "\r\n", versioninfo.dwMajorVersion,
			versioninfo.dwMinorVersion, versioninfo.dwBuildNumber,
			versioninfo.dwPlatformId, versioninfo.szCSDVersion);
	else
		*info = '\0';
#else
	int ret_code;
	struct utsname sys_info;

	ret_code = uname(&sys_info);
	if (ret_code == -1)
		*info = '\0';
	snprintf(info, infoSize,
		"%s/%s, UPnP/1.0, Portable SDK for UPnP devices/"
		PACKAGE_VERSION "\r\n", sys_info.sysname, sys_info.release);
#endif
#endif /* UPNP_ENABLE_UNSPECIFIED_SERVER */
}

