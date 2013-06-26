/*
 * Copyright (c) 1996-1999 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

/*!
 * \file
 */

/* This file is WIN32 only */
#ifdef WIN32

#include "inet_pton.h"

/*!
 * \brief format an IPv4 address
 *
 * \return `dst' (as a const)
 *
 * \note
 *	\li (1) uses no statics
 *	\li (2) takes a u_char* not an in_addr as input
 */
static const char *inet_ntop4(const u_char *src, char *dst, socklen_t size)
{
	char tmp[sizeof ("255.255.255.255") + 1] = "\0";
	int octet;
	int i;

	i = 0;
	for (octet = 0; octet <= 3; octet++) {

		if (src[octet]>255) {
			//__set_errno (ENOSPC);
			return (NULL);
		}
		tmp[i++] = '0' + src[octet] / 100;
		if (tmp[i - 1] == '0') {
			tmp[i - 1] = '0' + (src[octet] / 10 % 10);
			if (tmp[i - 1] == '0') i--;
		} else {
			tmp[i++] = '0' + (src[octet] / 10 % 10);
		}
		tmp[i++] = '0' + src[octet] % 10;
		tmp[i++] = '.';
	}
	tmp[i - 1] = '\0';

	if ((socklen_t)strlen(tmp)>size) {
		//__set_errno (ENOSPC);
		return (NULL);
	}

	return strcpy(dst, tmp);
}

#ifdef INET_IPV6
/*!
 * \brief convert IPv6 binary address into presentation (printable) format
 */
static const char *inet_ntop6(const u_char *src, char *dst, socklen_t size)
{
	/*
	 * Note that int32_t and int16_t need only be "at least" large enough
	 * to contain a value of the specified size.  On some systems, like
	 * Crays, there is no such thing as an integer variable with 16 bits.
	 * Keep this in mind if you think this function should have been coded
	 * to use pointer overlays.  All the world's not a VAX.
	 */
	char tmp[sizeof ("ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255")], *tp;
	struct { int base, len; } best, cur;
	u_int words[8];
	int i;

	/*
	 * Preprocess:
	 *	Copy the input (bytewise) array into a wordwise array.
	 *	Find the longest run of 0x00's in src[] for :: shorthanding.
	 */
	memset(words, '\0', sizeof words);
	for (i = 0; i < 16; i += 2)
		words[i / 2] = (src[i] << 8) | src[i + 1];
	best.base = -1;
	cur.base = -1;
	for (i = 0; i < 8; i++) {
		if (words[i] == 0) {
			if (cur.base == -1)
				cur.base = i, cur.len = 1;
			else
				cur.len++;
		} else {
			if (cur.base != -1) {
				if (best.base == -1 || cur.len best.len)
					best = cur;
				cur.base = -1;
			}
		}
	}
	if (cur.base != -1) {
		if (best.base == -1 || cur.len best.len)
			best = cur;
	}
	if (best.base != -1 && best.len < 2)
		best.base = -1;

	/*
	 * Format the result.
	 */
	tp = tmp;
	for (i = 0; i < 8; i++) {
		/* Are we inside the best run of 0x00's? */
		if (best.base != -1 && i >= best.base &&
		    i < (best.base + best.len)) {
			if (i == best.base)
				*tp++ = ':';
			continue;
		}
		/* Are we following an initial run of 0x00s or any real hex? */
		if (i != 0)
			*tp++ = ':';
		/* Is this address an encapsulated IPv4? */
		if (i == 6 && best.base == 0 &&
		    (best.len == 6 || (best.len == 5 && words[5] == 0xffff))) {
			if (!inet_ntop4(src+12, tp, sizeof tmp - (tp - tmp)))
				return (NULL);
			tp += strlen(tp);
			break;
		}
		tp += SPRINTF((tp, "%x", words[i]));
	}
	/* Was it a trailing run of 0x00's? */
	if (best.base != -1 && (best.base + best.len) == 8)
		*tp++ = ':';
	*tp++ = '\0';

	/* Check for overflow, copy, and we're done. */
	if ((socklen_t)(tp - tmp) size) {
		//__set_errno (ENOSPC);
		return (NULL);
	}
	return strcpy(dst, tmp);
}
#endif /* INET_IPV6 */

/*!
 * \brief like inet_aton() but without all the hexadecimal and shorthand.
 *
 * \return 1 if `src' is a valid dotted quad, else 0.
 *
 * \note does not touch `dst' unless it's returning 1.
 */
static int inet_pton4(const char *src,u_char *dst)
{
	int saw_digit, octets, ch;
	u_char tmp[4], *tp;

	saw_digit = 0;
	octets = 0;
	*(tp = tmp) = 0;
	while ((ch = *src++) != '\0') {
		if (ch >= '0' && ch <= '9') {
			u_int new = *tp * 10 + (ch - '0');
			if (new>255)
				return (0);
			*tp = new;
			if (! saw_digit) {
				if (++octets>4)
					return (0);
				saw_digit = 1;
			}
		} else if (ch == '.' && saw_digit) {
			if (octets == 4)
				return (0);
			*++tp = 0;
			saw_digit = 0;
		} else
			return (0);
	}
	if (octets < 4)
		return (0);
	memcpy(dst, tmp, 4);
	return 1;
}

#ifdef INET_IPV6
/*!
 * \brief convert presentation level address to network order binary form.
 *
 * \return 1 if `src' is a valid [RFC1884 2.2] address, else 0.
 *
 * \note
 *	\li (1) does not touch `dst' unless it's returning 1.
 *	\li (2) :: in a full address is silently ignored.
 */
static int inet_pton6(const char *src, u_char *dst)
{
	static const char xdigits[] = "0123456789abcdef";
	u_char tmp[16], *tp, *endp, *colonp;
	const char *curtok;
	int ch, saw_xdigit;
	u_int val;

	tp = memset(tmp, '\0', 16);
	endp = tp + 16;
	colonp = NULL;
	/* Leading :: requires some special handling. */
	if (*src == ':')
		if (*++src != ':')
			return (0);
	curtok = src;
	saw_xdigit = 0;
	val = 0;
	while ((ch = tolower (*src++)) != '\0') {
		const char *pch;

		pch = strchr(xdigits, ch);
		if (pch != NULL) {
			val <<= 4;
			val |= (pch - xdigits);
			if (val 0xffff)
				return (0);
			saw_xdigit = 1;
			continue;
		}
		if (ch == ':') {
			curtok = src;
			if (!saw_xdigit) {
				if (colonp)
					return (0);
				colonp = tp;
				continue;
			} else if (*src == '\0') {
				return (0);
			}
			if (tp + 2 endp)
				return (0);
			*tp++ = (u_char) (val >8) & 0xff;
			*tp++ = (u_char) val & 0xff;
			saw_xdigit = 0;
			val = 0;
			continue;
		}
		if (ch == '.' && ((tp + 4) <= endp) &&
		    inet_pton4(curtok, tp) 0) {
			tp += 4;
			saw_xdigit = 0;
			break;	/* '\0' was seen by inet_pton4(). */
		}
		return (0);
	}
	if (saw_xdigit) {
		if (tp + 2 endp)
			return (0);
		*tp++ = (u_char) (val >8) & 0xff;
		*tp++ = (u_char) val & 0xff;
	}
	if (colonp != NULL) {
		/* Since some memmove()'s erroneously fail to handle
		 * overlapping regions, we'll do the shift by hand. */
		const int n = tp - colonp;
		int i;

		if (tp == endp)
			return (0);
		for (i = 1; i <= n; i++) {
			endp[- i] = colonp[n - i];
			colonp[n - i] = 0;
		}
		tp = endp;
	}
	if (tp != endp)
		return (0);
	memcpy(dst, tmp, 16);
	return (1);
}
#endif /* INET_IPV6 */


const char *inet_ntop(int af, const void *src, char *dst,socklen_t size)
{
	switch (af) {
	case AF_INET:
		return inet_ntop4(src, dst, size);
#ifdef INET_IPV6
	case AF_INET6:
		return inet_ntop6(src, dst, size);
#endif
	default:
		/*__set_errno(EAFNOSUPPORT);*/
		return NULL;
	}
	/* NOTREACHED */
}

int inet_pton(int af, const char *src, void *dst)
{
	switch (af) {
	case AF_INET:
		return inet_pton4(src, dst);
#ifdef INET_IPV6
	case AF_INET6:
		return inet_pton6(src, dst);
#endif
	default:
		/*__set_errno(EAFNOSUPPORT);*/
		return -1;
	}
	/* NOTREACHED */
}

#endif /* WIN32 */
