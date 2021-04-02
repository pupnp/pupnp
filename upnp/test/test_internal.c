#include "autoconfig.h"

#include "upnp.h"

#include <stdio.h>
#include <string.h>

#if UPNP_HAVE_TOOLS
#include "upnptools.h"
#endif
#include "UpnpLog.h"

int main(int argc, char *argv[])
{
        (void)argc;
        (void)argv;
        int ok = 1;

        ok = ok && UpnpInternalUnitTest("httpreadwrite");

        if (ok) {
                exit(EXIT_SUCCESS);
        } else {
                exit(EXIT_FAILURE);
        }
}
