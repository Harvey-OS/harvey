#include "ssh.h"

struct CipherState
{
	RC4state enc;
	RC4state dec;
};

static CipherState*
initrc4(Conn *c, int isserver)
{
	CipherState *cs;

	cs = emalloc(sizeof(CipherState));
	if(isserver){
		setupRC4state(&cs->enc, c->sesskey, 16);
		setupRC4state(&cs->dec, c->sesskey+16, 16);
	}else{
		setupRC4state(&cs->dec, c->sesskey, 16);
		setupRC4state(&cs->enc, c->sesskey+16, 16);
	}
	return cs;
}

static void
encryptrc4(CipherState *cs, uchar *buf, int nbuf)
{
	rc4(&cs->enc, buf, nbuf);
}

static void
decryptrc4(CipherState *cs, uchar *buf, int nbuf)
{
	rc4(&cs->dec, buf, nbuf);
}

Cipher cipherrc4 =
{
	SSH_CIPHER_RC4,
	"rc4",
	initrc4,
	encryptrc4,
	decryptrc4,
};

