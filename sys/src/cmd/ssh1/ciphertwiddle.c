/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "ssh.h"

static CipherState*
inittwiddle(Conn *c, int)
{
	/* must be non-nil */
	fprint(2, "twiddle key is %.*H\n", SESSKEYLEN, c->sesskey);
	return (CipherState*)~0;
}

static void
twiddle(CipherState*, uchar *buf, int n)
{
	int i;

	for(i=0; i<n; i++)
		buf[i] ^= 0xFF;
}

Cipher ciphertwiddle =
{
	SSH_CIPHER_TWIDDLE,
	"twiddle",
	inittwiddle,
	twiddle,
	twiddle,
};

