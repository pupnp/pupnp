#ifndef UPNPUNISTD_H
#define UPNPUNISTD_H

#ifdef _WIN32
	/* Do not #include <unistd.h> on WIN32. */
#else /* _WIN32 */
	#include <unistd.h> /* for close() */
#endif /* _WIN32 */

#endif /* UPNPUNISTD_H */
