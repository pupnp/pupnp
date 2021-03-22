/**************************************************************************
 *
 * Copyright (c) 2018 Jean-Francois Dockes <jfd@recoll.org>
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
#include "upnpconfig.h"

#include "UpnpLog.h"
#include "upnp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{
#if UPNP_HAVE_DEBUG
        int i;
        /* Hopefully big enough buffer */
        static char upnplib_buffer[4096];
        UpnpLib *p = (UpnpLib *)&upnplib_buffer;

        if (!p) {
                UpnpPrintf(p,
                        UPNP_CRITICAL,
                        API,
                        __FILE__,
                        __LINE__,
                        "Insuficient memory!\n");
                exit(1);
        }
        /* Try a few random calls (let's crash it...) */
        UpnpCloseLog(p);
        UpnpCloseLog(p);
        UpnpPrintf(p,
                UPNP_CRITICAL,
                API,
                __FILE__,
                __LINE__,
                "This should not be printed!\n");
        FILE *fp = UpnpGetDebugFile(p, UPNP_CRITICAL, API);
        if (fp) {
                fprintf(stderr, "Log FP not NULL before init was called!\n");
                exit(1);
        }

        /* Let's really init. Request log to stderr */
        UpnpSetLogFileNames(p, NULL, NULL);
        UpnpSetLogLevel(p, UPNP_ERROR);
        UpnpInitLog(p);

        UpnpPrintf(p, UPNP_CRITICAL, API, __FILE__, __LINE__, "Hello LOG!\n");
        UpnpPrintf(p,
                UPNP_INFO,
                API,
                __FILE__,
                __LINE__,
                "This should not be here!\n");

        /* Let's try to a file */
        UpnpSetLogFileNames(p, "libupnp_err.log", 0);
        UpnpInitLog(p);
        UpnpPrintf(p,
                UPNP_CRITICAL,
                API,
                __FILE__,
                __LINE__,
                "Hello from the log file\n");

        /* Close and retry stuff */
        UpnpCloseLog(p);
        UpnpPrintf(p, UPNP_INFO, API, __FILE__, __LINE__, "Not here either!\n");
        UpnpSetLogFileNames(p, 0, 0);
        UpnpInitLog(p);
        UpnpPrintf(p, UPNP_CRITICAL, API, __FILE__, __LINE__, "I'm back!\n");
        for (i = 0; i < 10000; i++) {
                UpnpInitLog(p);
                UpnpCloseLog(p);
        }
        UpnpCloseLog(p);
#else
        printf("DEBUG is not configured\n");
#endif

        exit(0);
}
