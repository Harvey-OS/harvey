/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "os.h"
#include <mp.h>
#include <libsec.h>

// I wasn't sure what to do when the buffer was not
// a multiple of 8.  I did what lacy's cryptolib did
// to be compatible, but it looks dangerous to me
// since its encrypting plain text with the key. -- presotto

void
des3ECBencrypt(uchar *p, int len, DES3state *s)
{
	int i;
	uchar tmp[8];

	for(; len >= 8; len -= 8){
		triple_block_cipher(s->expanded, p, DES3EDE);
		p += 8;
	}
	
	if(len > 0){
		for (i=0; i<8; i++)
			tmp[i] = i;
		triple_block_cipher(s->expanded, tmp, DES3EDE);
		for (i = 0; i < len; i++)
			p[i] ^= tmp[i];
	}
}

void
des3ECBdecrypt(uchar *p, int len, DES3state *s)
{
	int i;
	uchar tmp[8];

	for(; len >= 8; len -= 8){
		triple_block_cipher(s->expanded, p, DES3DED);
		p += 8;
	}
	
	if(len > 0){
		for (i=0; i<8; i++)
			tmp[i] = i;
		triple_block_cipher(s->expanded, tmp, DES3EDE);
		for (i = 0; i < len; i++)
			p[i] ^= tmp[i];
	}
}
