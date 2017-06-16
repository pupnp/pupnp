#ifndef UPNPSTDINT_H
#define UPNPSTDINT_H

/* Sized integer types. */
#include <stdint.h>

#if !defined(UPNP_USE_BCBPP)

#ifdef UPNP_USE_MSVCPP
	/* no ssize_t defined for VC */
	#ifdef  _WIN64
		typedef int64_t ssize_t;
	#else
		typedef int32_t ssize_t;
	#endif
#endif

#endif /* !defined(UPNP_USE_BCBPP) */

#endif /* UPNPSTDINT_H */
