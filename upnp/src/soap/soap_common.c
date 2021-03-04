
#include "config.h"
#if EXCLUDE_SOAP == 0

        #include "httpparser.h"
        #include "soaplib.h"
        #include "sock.h"

const char *ContentTypeHeader = "CONTENT-TYPE: text/xml; charset=\"utf-8\"\r\n";

#endif /* EXCLUDE_SOAP */
