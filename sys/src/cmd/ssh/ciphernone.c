#include "ssh.h"

static CipherState*
initnone(Conn*, int)
{
	/* must be non-nil */
	return (CipherState*)~0;
}

static void
encryptnone(CipherState*, uchar*, int)
{
}

static void
decryptnone(CipherState*, uchar*, int)
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

