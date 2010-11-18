/*
 * Copyright (C) 2010 Marcelo Roberto Jimenez <mroberto@users.sourceforge.net>
 */

#ifndef COMMON_DATA
#define COMMON_DATA

/*!
 * \file
 */

#ifdef ALLOC_COMMON_DATA
	/*! Service types for tv services. */
	const char *TvServiceType[] = {
		"urn:schemas-upnp-org:service:tvcontrol:1",
		"urn:schemas-upnp-org:service:tvpicture:1"
	};
#else /* ALLOC_COMMON_DATA */
	extern const char *TvServiceType[];
#endif /* ALLOC_COMMON_DATA */

#endif /* COMMON_DATA */

