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

