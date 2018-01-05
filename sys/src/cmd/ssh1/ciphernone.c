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
initnone(Conn*, int)
{
	/* must be non-nil */
	return (CipherState*)~0;
}

static void
encryptnone(CipherState*, uint8_t*, int)
{
}

static void
decryptnone(CipherState*, uint8_t*, int)
{
}

Cipher ciphernone =
{
	SSH_CIPHER_NONE,
	"none",
	initnone,
	encryptnone,
	decryptnone,
};
