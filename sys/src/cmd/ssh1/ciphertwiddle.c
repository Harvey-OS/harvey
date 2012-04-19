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

