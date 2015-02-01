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

struct CipherState {
	DES3state state;
};

static CipherState*
init3des(Conn *c, int dir)
{
	CipherState *cs;
	uchar key[3][8];

	cs = emalloc9p(sizeof(CipherState));
	if(dir){
		memmove(key, c->s2cek, sizeof key);
		setupDES3state(&cs->state, key, c->s2civ);
	} else {
		memmove(key, c->c2sek, sizeof key);
		setupDES3state(&cs->state, key, c->c2siv);
	}
	return cs;
}

static void
encrypt3des(CipherState *cs, uchar *buf, int nbuf)
{
	des3CBCencrypt(buf, nbuf, &cs->state);
}

static void
decrypt3des(CipherState *cs, uchar *buf, int nbuf)
{
	des3CBCdecrypt(buf, nbuf, &cs->state);
}

Cipher cipher3des = {
	"3des-cbc",
	8,
	init3des,
	encrypt3des,
	decrypt3des,
};
