#include <u.h>
#include <libc.h>
#include <mp.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include <libsec.h>
#include "netssh.h"

struct CipherState {
	RC4state state;
};

static CipherState*
initrc4(Conn *c, int dir)
{
	CipherState *cs;

	cs = emalloc9p(sizeof(CipherState));
	if(dir)
		setupRC4state(&cs->state, c->s2cek, 16);
	else
		setupRC4state(&cs->state, c->c2sek, 16);
	return cs;
}

static void
encryptrc4(CipherState *cs, uchar *buf, int nbuf)
{
	rc4(&cs->state, buf, nbuf);
}

static void
decryptrc4(CipherState *cs, uchar *buf, int nbuf)
{
	rc4(&cs->state, buf, nbuf);
}

Cipher cipherrc4 = {
	"arcfour",
	8,
	initrc4,
	encryptrc4,
	decryptrc4,
};

