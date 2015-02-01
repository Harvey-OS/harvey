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
	DESstate enc;
	DESstate dec;
};

static CipherState*
initdes(Conn *c, int)
{
	CipherState *cs;

	cs = emalloc(sizeof(CipherState));
	setupDESstate(&cs->enc, c->sesskey, nil);
	setupDESstate(&cs->dec, c->sesskey, nil);
	return cs;
}

static void
encryptdes(CipherState *cs, uchar *buf, int nbuf)
{
	desCBCencrypt(buf, nbuf, &cs->enc);
}

static void
decryptdes(CipherState *cs, uchar *buf, int nbuf)
{
	desCBCdecrypt(buf, nbuf, &cs->dec);
}

Cipher cipherdes =
{
	SSH_CIPHER_DES,
	"des",
	initdes,
	encryptdes,
	decryptdes,
};

