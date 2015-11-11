/*	contrib/pgcrypto/sha2.h */
/*	$OpenBSD: sha2.h,v 1.2 2004/04/28 23:11:57 millert Exp $	*/

/*
 * FILE:	sha2.h
 * AUTHOR:	Aaron D. Gifford <me@aarongifford.com>
 *
 * Copyright (c) 2000-2001, Aaron D. Gifford
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	  notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	  notice, this list of conditions and the following disclaimer in the
 *	  documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of contributors
 *	  may be used to endorse or promote products derived from this software
 *	  without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTOR(S) ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.	IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTOR(S) BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $From: sha2.h,v 1.1 2001/11/08 00:02:01 adg Exp adg $
 */


/*** SHA-224/256/384/512 Various Length Definitions ***********************/
enum{
	SHA224BlockLength		= 64,
	SHA224DigestLength 	= 28,
	SHA224_digest_string_length = (SHA224DigestLength * 2 + 1),
	SHA256BlockLength		= 64,
	SHA256DigestLength	= 32,
	SHA256DigestStringLength = (SHA256DigestLength * 2 + 1),
	SHA384BlockLength		= 128,
	SHA384DigestLength	= 48,
	SHA384_digest_string_length = (SHA384DigestLength * 2 + 1),
	SHA512_block_length		= 128,
	SHA512DigestLength	= 64,
	SHA512DigestStringLength = (SHA512DigestLength * 2 + 1)
};

/*** SHA-256/384/512 Context Structures *******************************/
typedef struct _SHA256Ctx
{
	uint32_t		state[8];
	uint64_t		bitcount;
	uint8_t		buffer[SHA256BlockLength];
} SHA256Ctx;
typedef struct _SHA512Ctx
{
	uint64_t		state[8];
	uint64_t		bitcount[2];
	uint8_t		buffer[SHA512_block_length];
} SHA512Ctx;

typedef SHA256Ctx SHA224Ctx;
typedef SHA512Ctx SHA384Ctx;

void		SHA224_Init(SHA224Ctx *);
void		SHA224_Update(SHA224Ctx *, const uint8_t *, size_t);
void		SHA224_Final(uint8_t[SHA224DigestLength], SHA224Ctx *);

void		SHA256_Init(SHA256Ctx *);
void		SHA256_Update(SHA256Ctx *, const uint8_t *, size_t);
void		SHA256_Final(uint8_t[SHA256DigestLength], SHA256Ctx *);

void		SHA384_Init(SHA384Ctx *);
void		SHA384_Update(SHA384Ctx *, const uint8_t *, size_t);
void		SHA384_Final(uint8_t[SHA384DigestLength], SHA384Ctx *);

void		SHA512_Init(SHA512Ctx *);
void		SHA512_Update(SHA512Ctx *, const uint8_t *, size_t);
void		SHA512_Final(uint8_t[SHA512DigestLength], SHA512Ctx *);

