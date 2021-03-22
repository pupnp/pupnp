/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**************************************************************************
 *
 * Copyright (c) 2006 Rémi Turboult <r3mi@users.sourceforge.net>
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
 *************************************************************************/

#include "autoconfig.h"

#include "upnp.h"

#include <stdio.h>
#include <stdlib.h>

#if UPNP_HAVE_TOOLS
#include "upnptools.h"
#endif
#include "UpnpLog.h"

int main(int argc, char *argv[])
{
        int rc;
        int a, b, c;
        (void)argc;
        (void)argv;
        const char *log_file_name = "test_init.log";
        UpnpLib *p;

        /*
         * Check library version (and formats)
         */
        printf("\n");

        printf("UPNP_VERSION_STRING = \"%s\"\n", UPNP_VERSION_STRING);
        printf("UPNP_VERSION_MAJOR  = %d\n", UPNP_VERSION_MAJOR);
        printf("UPNP_VERSION_MINOR  = %d\n", UPNP_VERSION_MINOR);
        printf("UPNP_VERSION_PATCH  = %d\n", UPNP_VERSION_PATCH);
        printf("UPNP_VERSION        = %d\n", UPNP_VERSION);

        if (sscanf(UPNP_VERSION_STRING, "%d.%d.%d", &a, &b, &c) != 3 ||
                a != UPNP_VERSION_MAJOR || b != UPNP_VERSION_MINOR ||
                c != UPNP_VERSION_PATCH) {
                printf("** ERROR malformed UPNP_VERSION_STRING\n");
                exit(EXIT_FAILURE);
        }

        /*
         * Check library optional features
         */
        printf("\n");

#if UPNP_HAVE_DEBUG
        printf("UPNP_HAVE_DEBUG \t= yes\n");
#else
        printf("UPNP_HAVE_DEBUG \t= no\n");
#endif

#if UPNP_HAVE_CLIENT
        printf("UPNP_HAVE_CLIENT\t= yes\n");
#else
        printf("UPNP_HAVE_CLIENT\t= no\n");
#endif

#if UPNP_HAVE_DEVICE
        printf("UPNP_HAVE_DEVICE\t= yes\n");
#else
        printf("UPNP_HAVE_DEVICE\t= no\n");
#endif

#if UPNP_HAVE_WEBSERVER
        printf("UPNP_HAVE_WEBSERVER\t= yes\n");
#else
        printf("UPNP_HAVE_WEBSERVER\t= no\n");
#endif

#if UPNP_HAVE_TOOLS
        printf("UPNP_HAVE_TOOLS \t= yes\n");
#else
        printf("UPNP_HAVE_TOOLS \t= no\n");
#endif

        /*
         * Test library initialization
         */
        printf("\n");
        printf("Initializing UPnP ... \n");
        unlink(log_file_name);
        p = 0;
        rc = UpnpInit2(&p, NULL, 0, log_file_name);
        if (UPNP_E_SUCCESS == rc) {
                UpnpPrintf(p,
                        UPNP_INFO,
                        API,
                        __FILE__,
                        __LINE__,
                        "Will call UpnpGetServerIpAddress(p);\n");
                const char *ip_address = UpnpGetServerIpAddress(p);
                UpnpPrintf(p,
                        UPNP_INFO,
                        API,
                        __FILE__,
                        __LINE__,
                        "Will call UpnpGetServerPort(p);\n");
                unsigned short port = UpnpGetServerPort(p);
                UpnpPrintf(p,
                        UPNP_INFO,
                        API,
                        __FILE__,
                        __LINE__,
                        "UPnP Initialized OK ip=%s, port=%d\n",
                        (ip_address ? ip_address : "UNKNOWN"),
                        port);
                printf("UPnP Initialized OK ip=%s, port=%d\n",
                        (ip_address ? ip_address : "UNKNOWN"),
                        port);
        } else {
                UpnpPrintf(p,
                        UPNP_INFO,
                        API,
                        __FILE__,
                        __LINE__,
                        "** ERROR UpnpInit2(): %d",
                        rc);
                printf("** ERROR UpnpInit2(): %d", rc);
#if UPNP_HAVE_TOOLS
                UpnpPrintf(p,
                        UPNP_INFO,
                        API,
                        __FILE__,
                        __LINE__,
                        " %s",
                        UpnpGetErrorMessage(rc));
                printf(" %s", UpnpGetErrorMessage(rc));
#endif
                printf("\n");
                exit(EXIT_FAILURE);
        }

        UpnpFinish(p);
        printf("\n");

        exit(EXIT_SUCCESS);
}
