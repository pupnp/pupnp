/*	$OpenBSD: md5.h,v 1.3 2014/11/16 17:39:09 tedu Exp $	*/

/*
 * This code implements the MD5 message-digest algorithm.
 * The algorithm is due to Ron Rivest.  This code was
 * written by Colin Plumb in 1993, no copyright is claimed.
 * This code is in the public domain; do with it what you wish.
 *
 * Equivalent code is available from RSA Data Security, Inc.
 * This code has been tested against that, and is equivalent,
 * except that you don't need to include two pages of legalese
 * with every copy.
 */

#ifndef _MD5_H_
#define _MD5_H_

#include "UpnpStdInt.h"

#include <stddef.h>

#define MD5_BLOCK_LENGTH 64
#define MD5_DIGEST_LENGTH 16

typedef struct MD5Context
{
        uint32_t state[4]; /* state */
        uint64_t count; /* number of bits, mod 2^64 */
        uint8_t buffer[MD5_BLOCK_LENGTH]; /* input buffer */
} MD5_CTX;

void MD5Init(MD5_CTX *);
void MD5Update(MD5_CTX *, const void *, size_t);
void MD5Final(uint8_t[MD5_DIGEST_LENGTH], MD5_CTX *);
void MD5Transform(uint32_t[4], const uint8_t[MD5_BLOCK_LENGTH]);

#endif /* _MD5_H_ */
