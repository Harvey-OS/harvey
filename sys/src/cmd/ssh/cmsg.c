#include "ssh.h"

static void
recv_ssh_smsg_public_key(Conn *c)
{
	Msg *m;

	m = recvmsg(c, SSH_SMSG_PUBLIC_KEY);
	memmove(c->cookie, getbytes(m, COOKIELEN), COOKIELEN);
	c->serverkey = getRSApub(m);
	c->hostkey = getRSApub(m);
	c->flags = getlong(m);
	c->ciphermask = getlong(m);
	c->authmask = getlong(m);
	free(m);
}

static void
send_ssh_cmsg_session_key(Conn *c)
{
	int i, n, buflen, serverkeylen, hostkeylen;
	mpint *b;
	uchar *buf;
	Msg *m;
	RSApub *ksmall, *kbig;

	m = allocmsg(c, SSH_CMSG_SESSION_KEY, 2048);
	putbyte(m, c->cipher->id);
	putbytes(m, c->cookie, COOKIELEN);

	serverkeylen = mpsignif(c->serverkey->n);
	hostkeylen = mpsignif(c->hostkey->n);
	ksmall = kbig = nil;
	if(serverkeylen+128 <= hostkeylen){
		ksmall = c->serverkey;
		kbig = c->hostkey;
	}else if(hostkeylen+128 <= serverkeylen){
		ksmall = c->hostkey;
		kbig = c->serverkey;
	}else
		error("server session and host keys do not differ by at least 128 bits");
	
	buflen = (mpsignif(kbig->n)+7)/8;
	buf = emalloc(buflen);

	debug(DBG_CRYPTO, "session key is %.*H\n", SESSKEYLEN, c->sesskey);
	memmove(buf, c->sesskey, SESSKEYLEN);
	for(i = 0; i < SESSIDLEN; i++)
		buf[i] ^= c->sessid[i];
	debug(DBG_CRYPTO, "munged session key is %.*H\n", SESSKEYLEN, buf);

	b = rsaencryptbuf(ksmall, buf, SESSKEYLEN);
	n = (mpsignif(ksmall->n)+7) / 8;
	mptoberjust(b, buf, n);
	mpfree(b);
	debug(DBG_CRYPTO, "encrypted with ksmall is %.*H\n", n, buf);

	b = rsaencryptbuf(kbig, buf, n);
	putmpint(m, b);
	debug(DBG_CRYPTO, "encrypted with kbig is %B\n", b);
	mpfree(b);

	memset(buf, 0, buflen);
	free(buf);

	putlong(m, c->flags);
	sendmsg(m);
}

static int
authuser(Conn *c)
{
	int i;
	Msg *m;

	m = allocmsg(c, SSH_CMSG_USER, 4+strlen(c->user));
	putstring(m, c->user);
	sendmsg(m);

	m = recvmsg(c, -1);
	switch(m->type){
	case SSH_SMSG_SUCCESS:
		free(m);
		return 0;
	case SSH_SMSG_FAILURE:
		free(m);
		break;
	default:
		badmsg(m, 0);
	}

	for(i=0; i<c->nokauth; i++){
		debug(DBG_AUTH, "authmask %#lux, consider %s (%#x)\n",
			c->authmask, c->okauth[i]->name, 1<<c->okauth[i]->id);
		if(c->authmask & (1<<c->okauth[i]->id))
			if((*c->okauth[i]->fn)(c) == 0)
				return 0;
	}

	debug(DBG_AUTH, "no auth methods worked; (authmask=%#lux)\n", c->authmask);
	return -1;
}

static char
ask(Conn *c, char *answers, char *question)
{
	int fd;
	char buf[256];

	if(!c->interactive)
		return answers[0];

	if((fd = open("/dev/cons", ORDWR)) < 0)
		return answers[0];

	fprint(fd, "%s", question);
	if(read(fd, buf, 256) <= 0 || buf[0]=='\n'){
		close(fd);
		return answers[0];
	}
	close(fd);
	return buf[0];
}
static void
checkkey(Conn *c)
{
	char *home, *keyfile;

	debug(DBG_CRYPTO, "checking key %B %B\n", c->hostkey->n, c->hostkey->ek);
	switch(findkey("/sys/lib/ssh/keyring", c->aliases, c->hostkey)){
	default:
		abort();
	case KeyOk:
		return;
	case KeyWrong:
		fprint(2, "server presented public key different than expected\n");
		fprint(2, "(expected key in /sys/lib/ssh/keyring).  will not continue.\n");
		error("bad server key");

	case NoKey:
	case NoKeyFile:
		break;
	}

	home = getenv("home");
	if(home == nil){
		fprint(2, "server %s not on keyring; will not continue.\n", c->host);
		error("bad server key");
	}
	
	keyfile = smprint("%s/lib/keyring", home);
	if(keyfile == nil)
		error("out of memory");

	switch(findkey(keyfile, c->aliases, c->hostkey)){
	default:
		abort();
	case KeyOk:
		return;
	case KeyWrong:
		fprint(2, "server %s presented public key different than expected\n", c->host);
		fprint(2, "(expected key in %s).  will not continue.\n", keyfile);
		fprint(2, "this could be a man-in-the-middle attack.\n");
		switch(ask(c, "eri", "replace key in keyfile (r), continue without replacing key (c), or exit (e) [e]")){
		case 'e':
			error("bad key");
		case 'r':
			if(replacekey(keyfile, c->aliases, c->hostkey) < 0)
				error("replacekey: %r");
			break;
		case 'c':
			break;
		}
		return;
	case NoKey:
	case NoKeyFile:
		fprint(2, "server %s not on keyring.\n", c->host);
		switch(ask(c, "eac", "add key to keyfile (a), continue without adding key (c), or exit (e) [e]")){
		case 'e':
			error("bad key");
		case 'a':
			if(appendkey(keyfile, c->aliases, c->hostkey) < 0)
				error("appendkey: %r");
			break;
		case 'c':
			break;
		}
		return;
	}
}

void
sshclienthandshake(Conn *c)
{
	char buf[128], *p;
	int i;
	Msg *m;

	/* receive id string */
	if(readstrnl(c->fd[0], buf, sizeof buf) < 0)
		error("reading server version: %r");

	/* id string is "SSH-m.n-comment".  We need m=1, n>=5. */
	if(strncmp(buf, "SSH-", 4) != 0
	|| strtol(buf+4, &p, 10) != 1
	|| *p != '.'
	|| strtol(p+1, &p, 10) < 5
	|| *p != '-')
		error("protocol mismatch; got %s, need SSH-1.x for x>=5", buf);

	/* send id string */
	fprint(c->fd[1], "SSH-1.5-Plan 9\n");

	recv_ssh_smsg_public_key(c);
	checkkey(c);

	for(i=0; i<SESSKEYLEN; i++)
		c->sesskey[i] = fastrand();
	c->cipher = nil;
	for(i=0; i<c->nokcipher; i++)
		if((1<<c->okcipher[i]->id) & c->ciphermask){
			c->cipher = c->okcipher[i];
			break;
		}
	if(c->cipher == nil)
		error("can't agree on ciphers: remote side supports %#lux", c->ciphermask);

	calcsessid(c);

	send_ssh_cmsg_session_key(c);

	c->cstate = (*c->cipher->init)(c, 0);		/* turns on encryption */
	m = recvmsg(c, SSH_SMSG_SUCCESS);
	free(m);
	
	if(authuser(c) < 0)
		error("client authentication failed");
}

static int
intgetenv(char *name, int def)
{
	char *s;
	int n, val;

	val = def;
	if((s = getenv(name))!=nil){
		if((n=atoi(s)) > 0)
			val = n;
		free(s);
	}
	return val;
}

/*
 * assumes that if you care, you're running under vt
 * and therefore these are set.
 */
int
readgeom(int *nrow, int *ncol, int *width, int *height)
{
	static int fd = -1;
	char buf[64];

	if(fd < 0 && (fd = open("/dev/wctl", OREAD)) < 0)
		return -1;
	/* wait for event, but don't care what it says */
	if(read(fd, buf, sizeof buf) < 0)
		return -1;
	*nrow = intgetenv("LINES", 24);
	*ncol = intgetenv("COLS", 80);
	*width = intgetenv("XPIXELS", 640);
	*height = intgetenv("YPIXELS", 480);
	return 0;
}

void
sendwindowsize(Conn *c, int nrow, int ncol, int width, int height)
{
	Msg *m;

	m = allocmsg(c, SSH_CMSG_WINDOW_SIZE, 4*4);
	putlong(m, nrow);
	putlong(m, ncol);
	putlong(m, width);
	putlong(m, height);
	sendmsg(m);
}

/*
 * In each option line, the first byte is the option number
 * and the second is either a boolean bit or actually an
 * ASCII code.
 */
static uchar ptyopt[] =
{
	0x01, 0x7F,	/* interrupt = DEL */
	0x02, 0x11,	/* quit = ^Q */
	0x03, 0x08,	/* backspace = ^H */
	0x04, 0x15,	/* line kill = ^U */
	0x05, 0x04,	/* EOF = ^D */
	0x20, 0x00,	/* don't strip high bit */
	0x48, 0x01,	/* give us CRs */

	0x00,		/* end options */
};

static uchar rawptyopt[] = 
{
	30,	0,		/* ignpar */
	31,	0,		/* parmrk */
	32,	0,		/* inpck */
	33,	0,		/* istrip */
	34,	0,		/* inlcr */
	35,	0,		/* igncr */
	36,	0,		/* icnrl */
	37,	0,		/* iuclc */
	38,	0,		/* ixon */
	39,	1,		/* ixany */
	40,	0,		/* ixoff */
	41,	0,		/* imaxbel */

	50,	0,		/* isig: intr, quit, susp processing */
	51,	0,		/* icanon: erase and kill processing */
	52,	0,		/* xcase */

	53,	0,		/* echo */

	57,	0,		/* noflsh */
	58,	0,		/* tostop */
	59,	0,		/* iexten: impl defined control chars */

	70,	0,		/* opost */

	0x00,
};

void
requestpty(Conn *c)
{
	char *term;
	int nrow, ncol, width, height;
	Msg *m;

	m = allocmsg(c, SSH_CMSG_REQUEST_PTY, 1024);
	if((term = getenv("TERM")) == nil)
		term = "9term";
	putstring(m, term);

	readgeom(&nrow, &ncol, &width, &height);
	putlong(m, nrow);	/* characters */
	putlong(m, ncol);
	putlong(m, width);	/* pixels */
	putlong(m, height);

	if(rawhack)
		putbytes(m, rawptyopt, sizeof rawptyopt);
	else
		putbytes(m, ptyopt, sizeof ptyopt);

	sendmsg(m);

	m = recvmsg(c, 0);
	switch(m->type){
	case SSH_SMSG_SUCCESS:
		debug(DBG_IO, "PTY allocated\n");
		break;
	case SSH_SMSG_FAILURE:
		debug(DBG_IO, "PTY allocation failed\n");
		break;
	default:
		badmsg(m, 0);
	}
	free(m);
}

