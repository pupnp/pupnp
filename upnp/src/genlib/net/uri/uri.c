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


/*******************************************************************************
 * Purpose: This file contains functions for uri, url parsing utility. 
 ******************************************************************************/


#ifdef __FreeBSD__
	#include <osreldate.h>
	#if __FreeBSD_version < 601103
		#include <lwres/netdb.h>
	#endif
#endif


#include "config.h"
#include "uri.h"
#include "upnpapi.h"


/************************************************************************
*	Function :	is_reserved
*
*	Parameters :
*		char in ;	char to be matched for RESERVED characters 
*
*	Description : Returns a 1 if a char is a RESERVED char as defined in 
*		http://www.ietf.org/rfc/rfc2396.txt RFC explaining URIs)
*
*	Return : int ;
*
*	Note :
************************************************************************/
int
is_reserved( char in )
{
    if( strchr( RESERVED, in ) )
        return 1;
    else
        return 0;
}

/************************************************************************
*	Function :	is_mark
*
*	Parameters :
*		char in ; character to be matched for MARKED characters
*
*	Description : Returns a 1 if a char is a MARK char as defined in 
*		http://www.ietf.org/rfc/rfc2396.txt (RFC explaining URIs)
*
*	Return : int ;
*
*	Note :
************************************************************************/
int
is_mark( char in )
{
    if( strchr( MARK, in ) )
        return 1;
    else
        return 0;
}

/************************************************************************
*	Function :	is_unreserved
*
*	Parameters :
*		char in ;	character to be matched for UNRESERVED characters
*
*	Description : Returns a 1 if a char is an unreserved char as defined in 
*		http://www.ietf.org/rfc/rfc2396.txt (RFC explaining URIs)	
*
*	Return : int ;
*
*	Note :
************************************************************************/
int
is_unreserved( char in )
{
    if( isalnum( in ) || ( is_mark( in ) ) )
        return 1;
    else
        return 0;
}

/************************************************************************
*	Function :	is_escaped
*
*	Parameters :
*		char * in ;	character to be matched for ESCAPED characters
*
*	Description : Returns a 1 if a char[3] sequence is escaped as defined 
*		in http://www.ietf.org/rfc/rfc2396.txt (RFC explaining URIs)
*               size of array is NOT checked (MUST be checked by caller)	
*
*	Return : int ;
*
*	Note :
************************************************************************/
int
is_escaped( const char *in )
{

    if( ( in[0] == '%' ) && ( isxdigit( in[1] ) ) && isxdigit( in[2] ) ) {

        return 1;
    } else
        return 0;
}

/************************************************************************
*	Function :	replace_escaped
*
*	Parameters :
*		char * in ;	string of characters
*		int index ;	index at which to start checking the characters
*		int *max ;	
*
*	Description : Replaces an escaped sequence with its unescaped version 
*		as in http://www.ietf.org/rfc/rfc2396.txt  (RFC explaining URIs)
*       Size of array is NOT checked (MUST be checked by caller)
*
*	Return : int ;
*
*	Note : This function modifies the string. If the sequence is an 
*		escaped sequence it is replaced, the other characters in the 
*		string are shifted over, and NULL characters are placed at the 
*		end of the string.
************************************************************************/
int
replace_escaped( char *in,
                 int index,
                 size_t *max )
{
    int tempInt = 0;
    char tempChar = 0;
    int i = 0;
    int j = 0;

    if( ( in[index] == '%' ) && ( isxdigit( in[index + 1] ) )
        && isxdigit( in[index + 2] ) ) {
        //Note the "%2x", makes sure that we convert a maximum of two
        //characters.
        if( sscanf( &in[index + 1], "%2x", &tempInt ) != 1 )
            return 0;

        tempChar = ( char )tempInt;

        for( i = index + 3, j = index; j < ( *max ); i++, j++ ) {
            in[j] = tempChar;
            if( i < ( *max ) )
                tempChar = in[i];
            else
                tempChar = 0;
        }
        ( *max ) -= 2;
        return 1;
    } else
        return 0;
}

/************************************************************************
*	Function :	parse_uric
*
*	Parameters :
*		char *in ;	string of characters
*		int max ;	maximum limit
*		token *out ; token object where the string of characters is 
*					 copied
*
*	Description : Parses a string of uric characters starting at in[0]
*		as defined in http://www.ietf.org/rfc/rfc2396.txt (RFC explaining 
*		URIs)	
*
*	Return : int ;
*
*	Note :
************************************************************************/
int
parse_uric( const char *in,
            int max,
            token * out )
{
    int i = 0;

    while( ( i < max )
           && ( ( is_unreserved( in[i] ) ) || ( is_reserved( in[i] ) )
                || ( ( i + 2 < max ) && ( is_escaped( &in[i] ) ) ) ) ) {
        i++;
    }

    out->size = i;
    out->buff = in;
    return i;
}


/************************************************************************
*	Function :	copy_token
*
*	Parameters :
*		const token *in ;		source token	
*		const char * in_base ;	
*		token * out ;			destination token
*		char * out_base ;	
*
*	Description : Tokens are generally pointers into other strings
*		this copies the offset and size from a token (in) relative to 
*		one string (in_base) into a token (out) relative to another 
*		string (out_base)
*
*	Return : void ;
*
*	Note :
************************************************************************/
static void
copy_token( const token * in,
            const char *in_base,
            token * out,
            char *out_base )
{
    out->size = in->size;
    out->buff = out_base + ( in->buff - in_base );
}

/************************************************************************
*	Function :	copy_URL_list
*
*	Parameters :
*		URL_list *in ;	Source URL list
*		URL_list *out ;	Destination URL list
*
*	Description : Copies one URL_list into another. This includes 
*		dynamically allocating the out->URLs field (the full string),
*       and the structures used to hold the parsedURLs. This memory MUST 
*		be freed by the caller through: free_URL_list(&out)
*
*	Return : int ;
*		HTTP_SUCCESS - On Success
*		UPNP_E_OUTOF_MEMORY - On Failure to allocate memory
*
*	Note :
************************************************************************/
int
copy_URL_list( URL_list * in,
               URL_list * out )
{
    int len = strlen( in->URLs ) + 1;
    int i = 0;

    out->URLs = NULL;
    out->parsedURLs = NULL;
    out->size = 0;

    out->URLs = ( char * )malloc( len );
    out->parsedURLs =
        ( uri_type * ) malloc( sizeof( uri_type ) * in->size );

    if( ( out->URLs == NULL ) || ( out->parsedURLs == NULL ) )
        return UPNP_E_OUTOF_MEMORY;

    memcpy( out->URLs, in->URLs, len );

    for( i = 0; i < in->size; i++ ) {
        //copy the parsed uri
        out->parsedURLs[i].type = in->parsedURLs[i].type;
        copy_token( &in->parsedURLs[i].scheme, in->URLs,
                    &out->parsedURLs[i].scheme, out->URLs );

        out->parsedURLs[i].path_type = in->parsedURLs[i].path_type;
        copy_token( &in->parsedURLs[i].pathquery, in->URLs,
                    &out->parsedURLs[i].pathquery, out->URLs );
        copy_token( &in->parsedURLs[i].fragment, in->URLs,
                    &out->parsedURLs[i].fragment, out->URLs );
        copy_token( &in->parsedURLs[i].hostport.text,
                    in->URLs, &out->parsedURLs[i].hostport.text,
                    out->URLs );

        memcpy( &out->parsedURLs[i].hostport.IPaddress,
            &in->parsedURLs[i].hostport.IPaddress, 
            sizeof(struct sockaddr_storage) );
    }
    out->size = in->size;
    return HTTP_SUCCESS;

}

/************************************************************************
*	Function :	free_URL_list
*
*	Parameters :
*		URL_list * list ;	URL List object
*
*	Description : Frees the memory associated with a URL_list. Frees the 
*		dynamically allocated members of of list. Does NOT free the 
*		pointer to the list itself ( i.e. does NOT free(list))
*
*	Return : void ;
*
*	Note :
************************************************************************/
void
free_URL_list( URL_list * list )
{
    if( list->URLs )
        free( list->URLs );
    if( list->parsedURLs )
        free( list->parsedURLs );
    list->size = 0;
}

/************************************************************************
*	Function :	print_uri
*
*	Parameters :
*		uri_type *in ;	URI object
*
*	Description : Function useful in debugging for printing a parsed uri.
*
*	Return : void ;
*
*	Note :
************************************************************************/
#ifdef DEBUG
void print_uri( uri_type *in )
{
    print_token( &in->scheme );
    print_token( &in->hostport.text );
    print_token( &in->pathquery );
    print_token( &in->fragment );
}
#endif

/************************************************************************
*	Function :	print_token
*
*	Parameters :
*		token * in ;	token
*
*	Description : Function useful in debugging for printing a token.
*
*	Return : void ;
*
*	Note :
************************************************************************/
#ifdef DEBUG
void print_token(token * in)
{
    int i = 0;
    printf( "Token Size : %"PRIzu"\n\'", in->size );
    for( i = 0; i < in->size; i++ ) {
        putchar( in->buff[i] );
    }
    putchar( '\'' );
    putchar( '\n' );
}
#endif

/************************************************************************
*	Function :	token_string_casecmp
*
*	Parameters :
*		token * in1 ;	Token object whose buffer is to be compared
*		char * in2 ;	string of characters to compare with
*
*	Description :	Compares buffer in the token object with the buffer 
*		in in2
*
*	Return : int ;
*		< 0 string1 less than string2 
*		0 string1 identical to string2 
*		> 0 string1 greater than string2 
*
*	Note :
************************************************************************/
int token_string_casecmp(
    token * in1,
    char *in2 )
{
    int in2_length = strlen( in2 );

    if( in1->size != in2_length )
        return 1;
    else
        return strncasecmp( in1->buff, in2, in1->size );
}

/************************************************************************
*	Function :	token_string_cmp
*
*	Parameters :
*		token * in1 ;	Token object whose buffer is to be compared
*		char * in2 ;	string of characters to compare with
*
*	Description : Compares a null terminated string to a token (exact)	
*
*	Return : int ;
*		< 0 string1 less than string2 
*		0 string1 identical to string2 
*		> 0 string1 greater than string2 
*
*	Note :
************************************************************************/
int
token_string_cmp( token * in1,
                  char *in2 )
{
    int in2_length = strlen( in2 );

    if( in1->size != in2_length )
        return 1;
    else
        return strncmp( in1->buff, in2, in1->size );
}

/************************************************************************
*	Function :	token_cmp
*
*	Parameters :
*		token *in1 ;	First token object whose buffer is to be compared
*		token *in2 ;	Second token object used for the comparison
*
*	Description : Compares two tokens	
*
*	Return : int ;
*		< 0 string1 less than string2 
*		0 string1 identical to string2 
*		> 0 string1 greater than string2 
*
*	Note :
************************************************************************/
int
token_cmp( token * in1,
           token * in2 )
{
    if( in1->size != in2->size )
        return 1;
    else
        return memcmp( in1->buff, in2->buff, in1->size );
}


/************************************************************************
*	Function :	parse_hostport
*
*	Parameters :
*		char *in ;	string of characters representing host and port
*		int max ;	sets a maximum limit (not used).
*		hostport_type *out ;	out parameter where the host and port
*					are represented as an internet address
*
*	Description : Parses a string representing a host and port
*		(e.g. "127.127.0.1:80" or "localhost") and fills out a 
*		hostport_type struct with internet address and a token 
*		representing the full host and port.  uses gethostbyname.
*
*	Returns: The number of characters that form up the host and port.
*            UPNP_E_INVALID_URL on error.
*
*	Note :
************************************************************************/
int
parse_hostport( const char *in,
                int max,
                hostport_type * out )
{
    char workbuf[256];
    char* c;
    struct sockaddr_in* sai4 = (struct sockaddr_in*)&out->IPaddress;
    struct sockaddr_in6* sai6 = (struct sockaddr_in6*)&out->IPaddress;
    char *srvname = NULL;
    char *srvport = NULL;
    char *last_dot = NULL;
    unsigned short int port;
    int af = AF_UNSPEC;
    int hostport_size;
    int has_port = 0;
    int ret;

    memset( out, 0, sizeof(hostport_type) );

    // Work on a copy of the input string.
    strncpy( workbuf, in, sizeof(workbuf) );

    c = workbuf;
    if( *c == '[' ) {
        // IPv6 addresses are enclosed in square brackets.
        srvname = ++c;
        while( *c != '\0'  &&  *c != ']' ) {
            c++;
        }
        if( *c == '\0' ) {
            // did not find closing bracket.
            return UPNP_E_INVALID_URL;
        }
        // NULL terminate the srvname and then increment c.
        *c++ = '\0';    // overwrite the ']'
        if( *c == ':' ) {
            has_port = 1;
            c++;
        }
        af = AF_INET6;
    }
    else {
        // IPv4 address -OR- host name.
        srvname = c;
        while( (*c != ':') && (*c != '/') && ( (isalnum(*c)) || (*c == '.') || (*c == '-') ) ) {
            if( *c == '.' )
                last_dot = c;
            c++;
        }
        has_port = (*c == ':') ? 1 : 0;
        // NULL terminate the srvname
        *c = '\0';
        if( has_port == 1 )
            c++;

        if( last_dot != NULL  &&  isdigit(*(last_dot+1)) ) {
            // Must be an IPv4 address.
            af = AF_INET;
        }
        else {
            // Must be a host name.
            struct addrinfo hints, *res, *res0;

            memset(&hints, 0, sizeof(hints));
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;

            ret = getaddrinfo(srvname, NULL, &hints, &res0);
            if( ret == 0 ) {
                for (res = res0; res; res = res->ai_next) {
                    if( res->ai_family == AF_INET || 
                        res->ai_family == AF_INET6 ) {
                        // Found a valid IPv4 or IPv6 address.
                        memcpy( &out->IPaddress, res->ai_addr, 
                            res->ai_addrlen );
                        break;
                    }
                }
                freeaddrinfo(res0);

                if( res == NULL ) {
                    // Didn't find an AF_INET or AF_INET6 address.
                    return UPNP_E_INVALID_URL;
                }
            }
            else {
                // getaddrinfo failed.
                return UPNP_E_INVALID_URL;
            }
        }
    }

    // Check if a port is specified.
    if( has_port == 1 ) {
        // Port is specified.
        srvport = c;
        while( *c != '\0'  &&  isdigit(*c) ) {
            c++;
        }
        port = (unsigned short int)atoi(srvport);
        if( port == 0 ) {
            // Bad port number.
            return UPNP_E_INVALID_URL;
        }
    }
    else {
        // Port was not specified, use default port.
        port = 80;
    }

    // The length of the host and port string can be calculated by
    // subtracting pointers.
    hostport_size = (int)(c - workbuf);

    // Fill in the 'out' information.
    if( af == AF_INET ) {
        sai4->sin_family = AF_INET;
        sai4->sin_port = htons(port);
        ret = inet_pton(AF_INET, srvname, &sai4->sin_addr);
    }
    else if( af == AF_INET6 ) {
        sai6->sin6_family = AF_INET6;
        sai6->sin6_port = htons(port);
        sai6->sin6_scope_id = gIF_INDEX;
        ret = inet_pton(AF_INET6, srvname, &sai6->sin6_addr);
    } else {
        // IP address was set by the hostname (getaddrinfo).
        // Override port:
        if( out->IPaddress.ss_family == AF_INET )
            sai4->sin_port = htons(port);
        else
            sai6->sin6_port = htons(port);
        ret = 1;
    }

    // Check if address was converted successfully.
    if( ret <= 0 ) {
        return UPNP_E_INVALID_URL;
    }

    out->text.size = hostport_size;
    out->text.buff = in;
    return hostport_size;
}

/************************************************************************
*	Function :	parse_scheme
*
*	Parameters :
*		char * in ;	string of characters representing a scheme
*		int max ;	maximum number of characters
*		token * out ;	output parameter whose buffer is filled in with 
*					the scheme
*
*	Description : parses a uri scheme starting at in[0] as defined in 
*		http://www.ietf.org/rfc/rfc2396.txt (RFC explaining URIs)
*		(e.g. "http:" -> scheme= "http"). 
*		Note, string MUST include ':' within the max charcters
*
*	Return : int ;
*
*	Note :
************************************************************************/
int
parse_scheme( const char *in,
              int max,
              token * out )
{
    int i = 0;

    out->size = 0;
    out->buff = NULL;

    if( ( max == 0 ) || ( !isalpha( in[0] ) ) )
        return FALSE;

    i++;
    while( ( i < max ) && ( in[i] != ':' ) ) {

        if( !( isalnum( in[i] ) || ( in[i] == '+' ) || ( in[i] == '-' )
               || ( in[i] == '.' ) ) )
            return FALSE;

        i++;
    }
    if( i < max ) {
        out->size = i;
        out->buff = &in[0];
        return i;
    }

    return FALSE;

}

/************************************************************************
*	Function :	remove_escaped_chars
*
*	Parameters :
*		INOUT char *in ;	string of characters to be modified
*		INOUT int *size ;	size limit for the number of characters
*
*	Description : removes http escaped characters such as: "%20" and 
*		replaces them with their character representation. i.e. 
*		"hello%20foo" -> "hello foo". The input IS MODIFIED in place. 
*		(shortened). Extra characters are replaced with NULL.
*
*	Return : int ;
*		UPNP_E_SUCCESS
*
*	Note :
************************************************************************/
int
remove_escaped_chars( INOUT char *in,
                      INOUT size_t *size )
{
    int i = 0;

    for( i = 0; i < *size; i++ ) {
        replace_escaped( in, i, size );
    }
    return UPNP_E_SUCCESS;
}

/************************************************************************
*	Function :	remove_dots
*
*	Parameters :
*		char *in ;	string of characters from which "dots" have to be 
*					removed
*		int size ;	size limit for the number of characters
*
*	Description : Removes ".", and ".." from a path. If a ".." can not
*		be resolved (i.e. the .. would go past the root of the path) an 
*		error is returned. The input IS modified in place.)
*
*	Return : int ;
*		UPNP_E_SUCCESS - On Success
*		UPNP_E_OUTOF_MEMORY - On failure to allocate memory
*		UPNP_E_INVALID_URL - Failure to resolve URL
*
*	Note :
*		Examples
*       char path[30]="/../hello";
*       remove_dots(path, strlen(path)) -> UPNP_E_INVALID_URL
*       char path[30]="/./hello";
*       remove_dots(path, strlen(path)) -> UPNP_E_SUCCESS, 
*       in = "/hello"
*       char path[30]="/./hello/foo/../goodbye" -> 
*       UPNP_E_SUCCESS, in = "/hello/goodbye"

************************************************************************/
int
remove_dots( char *in,
             int size )
{
    char *copyTo = in;
    char *copyFrom = in;
    char *max = in + size;
    char **Segments = NULL;
    int lastSegment = -1;

    Segments = malloc( sizeof( char * ) * size );

    if( Segments == NULL )
        return UPNP_E_OUTOF_MEMORY;

    Segments[0] = NULL;
    UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
        "REMOVE_DOTS: before: %s\n", in );
    while( ( copyFrom < max ) && ( *copyFrom != '?' )
           && ( *copyFrom != '#' ) ) {

        if( ( ( *copyFrom ) == '.' )
            && ( ( copyFrom == in ) || ( *( copyFrom - 1 ) == '/' ) ) ) {
            if( ( copyFrom + 1 == max )
                || ( *( copyFrom + 1 ) == '/' ) ) {

                copyFrom += 2;
                continue;
            } else if( ( *( copyFrom + 1 ) == '.' )
                       && ( ( copyFrom + 2 == max )
                            || ( *( copyFrom + 2 ) == '/' ) ) ) {
                copyFrom += 3;

                if( lastSegment > 0 ) {
                    copyTo = Segments[--lastSegment];
                } else {
                    free( Segments );
                    //TRACE("ERROR RESOLVING URL, ../ at ROOT");
                    return UPNP_E_INVALID_URL;
                }
                continue;
            }
        }

        if( ( *copyFrom ) == '/' ) {

            lastSegment++;
            Segments[lastSegment] = copyTo + 1;
        }
        ( *copyTo ) = ( *copyFrom );
        copyTo++;
        copyFrom++;
    }
    if( copyFrom < max ) {
        while( copyFrom < max ) {
            ( *copyTo ) = ( *copyFrom );
            copyTo++;
            copyFrom++;
        }
    }
    ( *copyTo ) = 0;
    free( Segments );
    UpnpPrintf( UPNP_ALL, API, __FILE__, __LINE__,
        "REMOVE_DOTS: after: %s\n", in );
    return UPNP_E_SUCCESS;
}

/************************************************************************
*	Function :	resolve_rel_url
*
*	Parameters :
*		char * base_url ;	Base URL
*		char * rel_url ;	Relative URL
*
*	Description : resolves a relative url with a base url returning a NEW 
*		(dynamically allocated with malloc) full url. If the base_url is 
*		NULL, then a copy of the  rel_url is passed back if the rel_url 
*		is absolute then a copy of the rel_url is passed back if neither 
*		the base nor the rel_url are Absolute then NULL is returned.
*		otherwise it tries and resolves the relative url with the base 
*		as described in: http://www.ietf.org/rfc/rfc2396.txt (RFCs 
*		explaining URIs) 
*       : resolution of '..' is NOT implemented, but '.' is resolved 
*
*	Return : char * ;
*
*	Note :
************************************************************************/
char *
resolve_rel_url( char *base_url,
                 char *rel_url )
{
    uri_type base;
    uri_type rel;
    char temp_path = '/';

    int i = 0;
    char *finger = NULL;

    char *last_slash = NULL;

    char *out = NULL;
    char *out_finger = NULL;

    if( base_url && rel_url ) {
        out =
            ( char * )malloc( strlen( base_url ) + strlen( rel_url ) + 2 );
        out_finger = out;
    } else {
        if( rel_url )
            return strdup( rel_url );
        else
            return NULL;
    }

    if( out == NULL ) {
        return NULL;
    }

    if( ( parse_uri( rel_url, strlen( rel_url ), &rel ) ) == HTTP_SUCCESS ) {

        if( rel.type == ABSOLUTE ) {

            strcpy( out, rel_url );
        } else {

            if( ( parse_uri( base_url, strlen( base_url ), &base ) ==
                  HTTP_SUCCESS )
                && ( base.type == ABSOLUTE ) ) {

                if( strlen( rel_url ) == 0 ) {
                    strcpy( out, base_url );
                } else {
                    memcpy( out, base.scheme.buff, base.scheme.size );
                    out_finger += base.scheme.size;
                    ( *out_finger ) = ':';
                    out_finger++;

                    if( rel.hostport.text.size > 0 ) {
                        sprintf( out_finger, "%s", rel_url );
                    } else {
                        if( base.hostport.text.size > 0 ) {
                            memcpy( out_finger, "//", 2 );
                            out_finger += 2;
                            memcpy( out_finger, base.hostport.text.buff,
                                    base.hostport.text.size );
                            out_finger += base.hostport.text.size;
                        }

                        if( rel.path_type == ABS_PATH ) {
                            strcpy( out_finger, rel_url );

                        } else {

                            if( base.pathquery.size == 0 ) {
                                base.pathquery.size = 1;
                                base.pathquery.buff = &temp_path;
                            }

                            finger = out_finger;
                            last_slash = finger;
                            i = 0;

                            while( ( i < base.pathquery.size ) &&
                                   ( base.pathquery.buff[i] != '?' ) ) {
                                ( *finger ) = base.pathquery.buff[i];
                                if( base.pathquery.buff[i] == '/' )
                                    last_slash = finger + 1;
                                i++;
                                finger++;

                            }
                            i = 0;
                            strcpy( last_slash, rel_url );
                            if( remove_dots( out_finger,
                                             strlen( out_finger ) ) !=
                                UPNP_E_SUCCESS ) {
                                free( out );
                                //free(rel_url);
                                return NULL;
                            }
                        }

                    }
                }
            } else {
                free( out );
                //free(rel_url);
                return NULL;
            }
        }
    } else {
        free( out );
        //free(rel_url);            
        return NULL;
    }

    //free(rel_url);
    return out;
}

/************************************************************************
*	Function :	parse_uri
*
*	Parameters :
*		char * in ;	character string containing uri information to be 
*					parsed
*		int max ;	maximum limit on the number of characters
*		uri_type * out ; out parameter which will have the parsed uri
*					information	
*
*	Description : parses a uri as defined in http://www.ietf.org/rfc/
*		rfc2396.txt (RFC explaining URIs)
*		Handles absolute, relative, and opaque uris. Parses into the 
*		following pieces: scheme, hostport, pathquery, fragment (path and
*		query are treated as one token)
*       Caller should check for the pieces they require.
*
*	Return : int ;
*
*	Note :
************************************************************************/
int
parse_uri( const char *in,
           int max,
           uri_type * out )
{
    int begin_path = 0;
    int begin_hostport = 0;
    int begin_fragment = 0;

    if( ( begin_hostport = parse_scheme( in, max, &out->scheme ) ) ) {
        out->type = ABSOLUTE;
        out->path_type = OPAQUE_PART;
        begin_hostport++;
    } else {
        out->type = RELATIVE;
        out->path_type = REL_PATH;
    }

    if( ( ( begin_hostport + 1 ) < max ) && ( in[begin_hostport] == '/' )
        && ( in[begin_hostport + 1] == '/' ) ) {
        begin_hostport += 2;

        if( ( begin_path = parse_hostport( &in[begin_hostport],
                                           max - begin_hostport,
                                           &out->hostport ) ) >= 0 ) {
            begin_path += begin_hostport;
        } else
            return begin_path;

    } else {
        memset( &out->hostport, 0, sizeof(out->hostport) );
        begin_path = begin_hostport;
    }

    begin_fragment =
        parse_uric( &in[begin_path], max - begin_path,
                    &out->pathquery ) + begin_path;

    if( ( out->pathquery.size ) && ( out->pathquery.buff[0] == '/' ) ) {
        out->path_type = ABS_PATH;
    }

    if( ( begin_fragment < max ) && ( in[begin_fragment] == '#' ) ) {
        begin_fragment++;
        parse_uric( &in[begin_fragment], max - begin_fragment,
                    &out->fragment );
    } else {
        out->fragment.buff = NULL;
        out->fragment.size = 0;
    }
    return HTTP_SUCCESS;
}

/************************************************************************
*	Function :	parse_uri_and_unescape
*
*	Parameters :
*		char * in ;	
*		int max ;	
*		uri_type * out ;	
*
*	Description : Same as parse_uri, except that all strings are 
*		unescaped (%XX replaced by chars)
*
*	Return : int ;
*
*	Note: This modifies 'pathquery' and 'fragment' parts of the input
************************************************************************/
int
parse_uri_and_unescape( char *in,
                        int max,
                        uri_type *out )
{
    int ret;

    if( ( ret = parse_uri( in, max, out ) ) != HTTP_SUCCESS )
        return ret;
    if( out->pathquery.size > 0 )
        remove_escaped_chars( (char *)out->pathquery.buff, &out->pathquery.size );
    if( out->fragment.size > 0 )
        remove_escaped_chars( (char *)out->fragment.buff, &out->fragment.size );
    return HTTP_SUCCESS;
}

