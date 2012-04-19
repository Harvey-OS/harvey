#include "ssh.h"

struct CipherState
{
	DESstate enc3des[3];
	DESstate dec3des[3];
};

static CipherState*
init3des(Conn *c, int)
{
	int i;
	CipherState *cs;

	cs = emalloc(sizeof(CipherState));
	for(i=0; i<3; i++){
		setupDESstate(&cs->enc3des[i], c->sesskey+8*i, nil);
		setupDESstate(&cs->dec3des[i], c->sesskey+8*i, nil);
	}
	return cs;
}

static void
encrypt3des(CipherState *cs, uchar *buf, int nbuf)
{
	desCBCencrypt(buf, nbuf, &cs->enc3des[0]);
	desCBCdecrypt(buf, nbuf, &cs->enc3des[1]);
	desCBCencrypt(buf, nbuf, &cs->enc3des[2]);
}

static void
decrypt3des(CipherState *cs, uchar *buf, int nbuf)
{
	desCBCdecrypt(buf, nbuf, &cs->dec3des[2]);
	desCBCencrypt(buf, nbuf, &cs->dec3des[1]);
	desCBCdecrypt(buf, nbuf, &cs->dec3des[0]);
}

Cipher cipher3des =
{
	SSH_CIPHER_3DES,
	"3des",
	init3des,
	encrypt3des,
	decrypt3des,
};

