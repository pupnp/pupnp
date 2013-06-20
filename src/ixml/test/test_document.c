/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Rémi Turboult <r3mi@users.sourceforge.net>
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


#include "ixml.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>


static const char*
get_ixml_error_string (IXML_ERRORCODE code)
{
#define CASE(CODE)	case IXML_ ## CODE: return #CODE

	switch (code) {
		CASE(INDEX_SIZE_ERR);
		CASE(DOMSTRING_SIZE_ERR);
		CASE(HIERARCHY_REQUEST_ERR);
		CASE(WRONG_DOCUMENT_ERR);
		CASE(INVALID_CHARACTER_ERR);
		CASE(NO_DATA_ALLOWED_ERR);
		CASE(NO_MODIFICATION_ALLOWED_ERR);
		CASE(NOT_FOUND_ERR);
		CASE(NOT_SUPPORTED_ERR);
		CASE(INUSE_ATTRIBUTE_ERR);
		CASE(INVALID_STATE_ERR);
		CASE(SYNTAX_ERR);
		CASE(INVALID_MODIFICATION_ERR);
		CASE(NAMESPACE_ERR);
		CASE(INVALID_ACCESS_ERR);
		CASE(SUCCESS);
		CASE(NO_SUCH_FILE);
		CASE(INSUFFICIENT_MEMORY);
		CASE(FILE_DONE);
		CASE(INVALID_PARAMETER);
		CASE(FAILED);
		CASE(INVALID_ITEM_NUMBER);
	}
	return "** UNKNOWN EROR CODE !! **";

#undef CASE
}



int
main (int argc, char* argv[])
{
	int i;

	if (argc < 2) {
		fprintf (stderr, "Usage: %s [xml files to load]\n",
			 argv[0]);
		exit (EXIT_FAILURE); // ---------->
	}

	for (i = 1; i < argc; i++) {
		int rc;
		IXML_Document* doc = NULL;
		DOMString s;
		char* p;

		printf ("Test \"%s\" \n", argv[i]);
		printf ("    Loading ... ");
		fflush (stdout);

		rc = ixmlLoadDocumentEx (argv[i], &doc);
		if (rc != IXML_SUCCESS) {
			fprintf (stderr, 
				 "** error : can't load document %s : "
				 "error %d (%s)\n",
				 argv[i], rc, get_ixml_error_string (rc));
			exit (EXIT_FAILURE); // ---------->
		}

		printf ("OK\n");

		printf ("    Printing ... ");
		fflush (stdout);
		
		s = ixmlPrintDocument (doc);
		if (s == NULL || s[0] == '\0') {
			fprintf (stderr, 
				 "** error : can't print loaded document %s\n",
				 argv[i]);
			exit (EXIT_FAILURE); // ---------->
		}
		p = s + strlen(s)-1;
		while (isspace(*p) && p > s)
			p--;
		if (*s != '<' || *p != '>') {
			fprintf (stderr, 
				 "** error : malformed printed document '%s' :"
				 "%s\n", argv[i], s);
			exit (EXIT_FAILURE); // ---------->
		}

		printf ("OK\n");

		ixmlFreeDOMString (s);
		ixmlDocument_free (doc);
	}
	
	exit (EXIT_SUCCESS);
}




