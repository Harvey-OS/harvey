#include <u.h>
#include <libc.h>
#include <mp.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include <libsec.h>
#include "netssh.h"

struct CipherState {
	BFstate	state;
};

static CipherState*
initblowfish(Conn *c, int dir)
{
	int i;
	CipherState *cs;

	if (debug > 1) {
		fprint(2, "initblowfish dir:%d\ns2cek: ", dir);
		for(i = 0; i < 16; ++i)
			fprint(2, "%02x", c->s2cek[i]);
		fprint(2, "\nc2sek: ");
		for(i = 0; i < 16; ++i)
			fprint(2, "%02x", c->c2sek[i]);
		fprint(2, "\ns2civ: ");
		for(i = 0; i < 8; ++i)
			fprint(2, "%02x", c->s2civ[i]);
		fprint(2, "\nc2siv: ");
		for(i = 0; i < 8; ++i)
			fprint(2, "%02x", c->c2siv[i]);
		fprint(2, "\n");
	}
	cs = emalloc9p(sizeof(CipherState));
	memset(cs, '\0', sizeof *cs);
	fprint(2, "cs: %p\n", cs);
	if(dir)
		setupBFstate(&cs->state, c->s2cek, 16, c->s2civ);
	else
		setupBFstate(&cs->state, c->c2sek, 16, c->c2siv);
	return cs;
}

static void
encryptblowfish(CipherState *cs, uchar *buf, int nbuf)
{
	bfCBCencrypt(buf, nbuf, &cs->state);
}

static void
decryptblowfish(CipherState *cs, uchar *buf, int nbuf)
{
fprint(2, "cs: %p, nb:%d\n", cs, nbuf);
fprint(2, "before decrypt: %02ux %02ux %02ux %02ux\n", buf[0], buf[1], buf[2], buf[3]);
	bfCBCdecrypt(buf, nbuf, &cs->state);
fprint(2, "after decrypt: %02ux %02ux %02ux %02ux\n", buf[0], buf[1], buf[2], buf[3]);
}

Cipher cipherblowfish = {
	"blowfish-cbc",
	8,
	initblowfish,
	encryptblowfish,
	decryptblowfish,
};
