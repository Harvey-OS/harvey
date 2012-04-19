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

