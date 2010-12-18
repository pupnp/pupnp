#ifndef UUID_H
#define UUID_H

/*
 * Copyright (c) 1990- 1993, 1996 Open Software Foundation, Inc.
 * Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, Ca. &
 * Digital Equipment Corporation, Maynard, Mass.
 * Copyright (c) 1998 Microsoft.
 * To anyone who acknowledges that this file is provided "AS IS"
 * without any express or implied warranty: permission to use, copy,
 * modify, and distribute this file for any purpose is hereby
 * granted without fee, provided that the above copyright notices and
 * this notice appears in all source code copies, and that none of
 * the names of Open Software Foundation, Inc., Hewlett-Packard
 * Company, or Digital Equipment Corporation be used in advertising
 * or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Neither Open Software
 * Foundation, Inc., Hewlett-Packard Company, Microsoft, nor Digital Equipment
 * Corporation makes any representations about the suitability of
 * this software for any purpose.
 */

#include "sysdep.h"

/*! . */
typedef struct _uuid_upnp {
	/*! . */
	uint32_t time_low;
	/*! . */
	uint16_t time_mid;
	/*! . */
	uint16_t time_hi_and_version;
	/*! . */
	uint8_t clock_seq_hi_and_reserved;
	/*! . */
	uint8_t clock_seq_low;
	/*! . */
	uint8_t node[6];
} uuid_upnp;

/*!
 * \brief Generate a UUID.
 */
int uuid_create(
	/*! . */
	uuid_upnp * id);

/*!
 * \brief Out will be xxxx-xx-xx-xx-xxxxxx format.
 */
void uuid_unpack(
	/*! . */
	uuid_upnp * u,
	/*! . */
	char *out);

/*!
 * \brief Create a UUID using a "name" from a "name space"
 */
void uuid_create_from_name(
	/*! Resulting UUID. */
	uuid_upnp * uid,
	/*! UUID to serve as context, so identical names from different name
	* spaces generate different UUIDs. */
	uuid_upnp nsid,
	/*! The name from which to generate a UUID. */
	void *name,
	/*! The length of the name. */
	int namelen);

/*!
 * \brief Compare two UUID's "lexically".
 *
 * \return
 *	-1   u1 is lexically before u2
 *	 0   u1 is equal to u2
 *	 1   u1 is lexically after u2
 *
 * \note Lexical ordering is not temporal ordering!
 */
int uuid_compare(
	/*! . */
	uuid_upnp * u1,
	/*! . */
	uuid_upnp * u2);
#endif /* UUID_H */
