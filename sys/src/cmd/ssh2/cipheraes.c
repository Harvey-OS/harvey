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
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include <libsec.h>
#include "netssh.h"

static QLock aeslock;

struct CipherState {
	AESstate state;
};

static CipherState *
initaes(Conn *c, int dir, int bits)
{
	CipherState *cs;

	qlock(&aeslock);
	cs = emalloc9p(sizeof(CipherState));
	if(dir)
		setupAESstate(&cs->state, c->s2cek, bits/8, c->s2civ);
	else
		setupAESstate(&cs->state, c->c2sek, bits/8, c->c2siv);
	qunlock(&aeslock);
	return cs;
}

static CipherState*
initaes128(Conn *c, int dir)
{
	return initaes(c, dir, 128);
}

static CipherState*
initaes192(Conn *c, int dir)
{
	return initaes(c, dir, 192);
}

static CipherState*
initaes256(Conn *c, int dir)
{
	return initaes(c, dir, 256);
}

static void
encryptaes(CipherState *cs, uchar *buf, int nbuf)
{
	if(cs->state.setup != 0xcafebabe || cs->state.rounds > AESmaxrounds)
		return;
	qlock(&aeslock);
	aesCBCencrypt(buf, nbuf, &cs->state);
	qunlock(&aeslock);
}

static void
decryptaes(CipherState *cs, uchar *buf, int nbuf)
{
	if(cs->state.setup != 0xcafebabe || cs->state.rounds > AESmaxrounds)
		return;
	qlock(&aeslock);
	aesCBCdecrypt(buf, nbuf, &cs->state);
	qunlock(&aeslock);
}

Cipher cipheraes128 = {
	"aes128-cbc",
	AESbsize,
	initaes128,
	encryptaes,
	decryptaes,
};

Cipher cipheraes192 = {
	"aes192-cbc",
	AESbsize,
	initaes192,
	encryptaes,
	decryptaes,
};

Cipher cipheraes256 = {
	"aes256-cbc",
	AESbsize,
	initaes256,
	encryptaes,
	decryptaes,
};
