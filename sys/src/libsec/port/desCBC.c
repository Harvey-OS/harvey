/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <mp.h>
#include <libsec.h>

// Because of the way that non multiple of 8
// buffers are handled, the decryptor must
// be fed buffers of the same size as the
// encryptor


// If the length is not a multiple of 8, I encrypt
// the overflow to be compatible with lacy's cryptlib
void
desCBCencrypt(uint8_t *p, int len, DESstate *s)
{
	uint8_t *p2, *ip, *eip;

	for(; len >= 8; len -= 8){
		p2 = p;
		ip = s->ivec;
		for(eip = ip+8; ip < eip; )
			*p2++ ^= *ip++;
		block_cipher(s->expanded, p, 0);
		memmove(s->ivec, p, 8);
		p += 8;
	}

	if(len > 0){
		ip = s->ivec;
		block_cipher(s->expanded, ip, 0);
		for(eip = ip+len; ip < eip; )
			*p++ ^= *ip++;
	}
}

void
desCBCdecrypt(uint8_t *p, int len, DESstate *s)
{
	uint8_t *ip, *eip, *tp;
	uint8_t tmp[8];

	for(; len >= 8; len -= 8){
		memmove(tmp, p, 8);
		block_cipher(s->expanded, p, 1);
		tp = tmp;
		ip = s->ivec;
		for(eip = ip+8; ip < eip; ){
			*p++ ^= *ip;
			*ip++ = *tp++;
		}
	}

	if(len > 0){
		ip = s->ivec;
		block_cipher(s->expanded, ip, 0);
		for(eip = ip+len; ip < eip; )
			*p++ ^= *ip++;
	}
}
