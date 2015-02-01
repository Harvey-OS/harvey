/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "ssh.h"

struct CipherState
{
	BFstate enc;
	BFstate dec;
};

static CipherState*
initblowfish(Conn *c, int)
{
	CipherState *cs;

	cs = emalloc(sizeof(CipherState));
	setupBFstate(&cs->enc, c->sesskey, SESSKEYLEN, nil);
	setupBFstate(&cs->dec, c->sesskey, SESSKEYLEN, nil);
	return cs;
}

static void
encryptblowfish(CipherState *cs, uchar *buf, int nbuf)
{
	bfCBCencrypt(buf, nbuf, &cs->enc);
}

static void
decryptblowfish(CipherState *cs, uchar *buf, int nbuf)
{
	bfCBCdecrypt(buf, nbuf, &cs->dec);
}

Cipher cipherblowfish = 
{
	SSH_CIPHER_BLOWFISH,
	"blowfish",
	initblowfish,
	encryptblowfish,
	decryptblowfish,
};

