#include "ssh.h"

static ulong sum32(ulong, void*, int);

char *msgnames[] =
{
/* 0 */
	"SSH_MSG_NONE",
	"SSH_MSG_DISCONNECT",
	"SSH_SMSG_PUBLIC_KEY",
	"SSH_CMSG_SESSION_KEY",
	"SSH_CMSG_USER",
	"SSH_CMSG_AUTH_RHOSTS",
	"SSH_CMSG_AUTH_RSA",
	"SSH_SMSG_AUTH_RSA_CHALLENGE",
	"SSH_CMSG_AUTH_RSA_RESPONSE",
	"SSH_CMSG_AUTH_PASSWORD",

/* 10 */
	"SSH_CMSG_REQUEST_PTY",
	"SSH_CMSG_WINDOW_SIZE",
	"SSH_CMSG_EXEC_SHELL",
	"SSH_CMSG_EXEC_CMD",
	"SSH_SMSG_SUCCESS",
	"SSH_SMSG_FAILURE",
	"SSH_CMSG_STDIN_DATA",
	"SSH_SMSG_STDOUT_DATA",
	"SSH_SMSG_STDERR_DATA",
	"SSH_CMSG_EOF",

/* 20 */
	"SSH_SMSG_EXITSTATUS",
	"SSH_MSG_CHANNEL_OPEN_CONFIRMATION",
	"SSH_MSG_CHANNEL_OPEN_FAILURE",
	"SSH_MSG_CHANNEL_DATA",
	"SSH_MSG_CHANNEL_INPUT_EOF",
	"SSH_MSG_CHANNEL_OUTPUT_CLOSED",
	"SSH_MSG_UNIX_DOMAIN_X11_FORWARDING (obsolete)",
	"SSH_SMSG_X11_OPEN",
	"SSH_CMSG_PORT_FORWARD_REQUEST",
	"SSH_MSG_PORT_OPEN",

/* 30 */
	"SSH_CMSG_AGENT_REQUEST_FORWARDING",
	"SSH_SMSG_AGENT_OPEN",
	"SSH_MSG_IGNORE",
	"SSH_CMSG_EXIT_CONFIRMATION",
	"SSH_CMSG_X11_REQUEST_FORWARDING",
	"SSH_CMSG_AUTH_RHOSTS_RSA",
	"SSH_MSG_DEBUG",
	"SSH_CMSG_REQUEST_COMPRESSION",
	"SSH_CMSG_MAX_PACKET_SIZE",
	"SSH_CMSG_AUTH_TIS",

/* 40 */
	"SSH_SMSG_AUTH_TIS_CHALLENGE",
	"SSH_CMSG_AUTH_TIS_RESPONSE",
	"SSH_CMSG_AUTH_KERBEROS",
	"SSH_SMSG_AUTH_KERBEROS_RESPONSE",
	"SSH_CMSG_HAVE_KERBEROS_TGT"
};

void
badmsg(Msg *m, int want)
{
	char *s, buf[20+ERRMAX];

	if(m==nil){
		snprint(buf, sizeof buf, "<early eof: %r>");
		s = buf;
	}else{
		snprint(buf, sizeof buf, "<unknown type %d>", m->type);
		s = buf;
		if(0 <= m->type && m->type < nelem(msgnames))
			s = msgnames[m->type];
	}
	if(want)
		error("got %s message expecting %s", s, msgnames[want]);
	error("got unexpected %s message", s);
}

Msg*
allocmsg(Conn *c, int type, int len)
{
	uchar *p;
	Msg *m;

	if(len > 256*1024)
		abort();

	m = (Msg*)emalloc(sizeof(Msg)+4+8+1+len+4);
	setmalloctag(m, getcallerpc(&c));
	p = (uchar*)&m[1];
	m->c = c;
	m->bp = p;
	m->ep = p+len;
	m->wp = p;
	m->type = type;
	return m;
}

void
unrecvmsg(Conn *c, Msg *m)
{
	debug(DBG_PROTO, "unreceived %s len %ld\n", msgnames[m->type], m->ep - m->rp);
	free(c->unget);
	c->unget = m;
}

static Msg*
recvmsg0(Conn *c)
{
	int pad;
	uchar *p, buf[4];
	ulong crc, crc0, len;
	Msg *m;

	if(c->unget){
		m = c->unget;
		c->unget = nil;
		return m;
	}

	if(readn(c->fd[0], buf, 4) != 4){
		werrstr("short net read: %r");
		return nil;
	}

	len = LONG(buf);
	if(len > 256*1024){
		werrstr("packet size far too big: %.8lux", len);
		return nil;
	}

	pad = 8 - len%8;

	m = (Msg*)emalloc(sizeof(Msg)+pad+len);
	setmalloctag(m, getcallerpc(&c));
	m->c = c;
	m->bp = (uchar*)&m[1];
	m->ep = m->bp + pad+len-4;	/* -4: don't include crc */
	m->rp = m->bp;

	if(readn(c->fd[0], m->bp, pad+len) != pad+len){
		werrstr("short net read: %r");
		free(m);
		return nil;
	}

	if(c->cipher)
		c->cipher->decrypt(c->cstate, m->bp, len+pad);

	crc = sum32(0, m->bp, pad+len-4);
	p = m->bp + pad+len-4;
	crc0 = LONG(p);
	if(crc != crc0){
		werrstr("bad crc %#lux != %#lux (packet length %lud)", crc, crc0, len);
		free(m);
		return nil;
	}

	m->rp += pad;
	m->type = *m->rp++;

	return m;
}

Msg*
recvmsg(Conn *c, int type)
{
	Msg *m;

	while((m = recvmsg0(c)) != nil){
		debug(DBG_PROTO, "received %s len %ld\n", msgnames[m->type], m->ep - m->rp);
		if(m->type != SSH_MSG_DEBUG && m->type != SSH_MSG_IGNORE)
			break;
		if(m->type == SSH_MSG_DEBUG)
			debug(DBG_PROTO, "remote DEBUG: %s\n", getstring(m));
		free(m);
	}
	if(type == 0){
		/* no checking */
	}else if(type == -1){
		/* must not be nil */
		if(m == nil)
			error(Ehangup);
	}else{
		/* must be given type */
		if(m==nil || m->type!=type)
			badmsg(m, type);
	}
	setmalloctag(m, getcallerpc(&c));
	return m;
}

int
sendmsg(Msg *m)
{
	int i, pad;
	uchar *p;
	ulong datalen, len, crc;
	Conn *c;

	datalen = m->wp - m->bp;
	len = datalen + 5;
	pad = 8 - len%8;

	debug(DBG_PROTO, "sending %s len %lud\n", msgnames[m->type], datalen);

	p = m->bp;
	memmove(m->bp+4+pad+1, m->bp, datalen);	/* slide data to correct position */

	PLONG(p, len);
	p += 4;

	if(m->c->cstate){
		for(i=0; i<pad; i++)
			*p++ = fastrand();
	}else{
		memset(p, 0, pad);
		p += pad;
	}

	*p++ = m->type;

	/* data already in position */
	p += datalen;

	crc = sum32(0, m->bp+4, pad+1+datalen);
	PLONG(p, crc);
	p += 4;

	c = m->c;
	qlock(c);
	if(c->cstate)
		c->cipher->encrypt(c->cstate, m->bp+4, len+pad);

	if(write(c->fd[1], m->bp, p - m->bp) != p-m->bp){
		qunlock(c);
		free(m);
		return -1;
	}
	qunlock(c);
	free(m);
	return 0;
}

uchar
getbyte(Msg *m)
{
	if(m->rp >= m->ep)
		error(Edecode);
	return *m->rp++;
}

ushort
getshort(Msg *m)
{
	ushort x;

	if(m->rp+2 > m->ep)
		error(Edecode);

	x = SHORT(m->rp);
	m->rp += 2;
	return x;
}

ulong
getlong(Msg *m)
{
	ulong x;

	if(m->rp+4 > m->ep)
		error(Edecode);

	x = LONG(m->rp);
	m->rp += 4;
	return x;
}

char*
getstring(Msg *m)
{
	char *p;
	ulong len;

	/* overwrites length to make room for NUL */
	len = getlong(m);
	if(m->rp+len > m->ep)
		error(Edecode);
	p = (char*)m->rp-1;
	memmove(p, m->rp, len);
	p[len] = '\0';
	return p;
}

void*
getbytes(Msg *m, int n)
{
	uchar *p;

	if(m->rp+n > m->ep)
		error(Edecode);
	p = m->rp;
	m->rp += n;
	return p;
}

mpint*
getmpint(Msg *m)
{
	int n;

	n = (getshort(m)+7)/8;	/* getshort returns # bits */
	return betomp(getbytes(m, n), n, nil);
}

RSApub*
getRSApub(Msg *m)
{
	RSApub *key;

	getlong(m);
	key = rsapuballoc();
	if(key == nil)
		error(Ememory);
	key->ek = getmpint(m);
	key->n = getmpint(m);
	setmalloctag(key, getcallerpc(&m));
	return key;
}

void
putbyte(Msg *m, uchar x)
{
	if(m->wp >= m->ep)
		error(Eencode);
	*m->wp++ = x;
}

void
putshort(Msg *m, ushort x)
{
	if(m->wp+2 > m->ep)
		error(Eencode);
	PSHORT(m->wp, x);
	m->wp += 2;
}

void
putlong(Msg *m, ulong x)
{
	if(m->wp+4 > m->ep)
		error(Eencode);
	PLONG(m->wp, x);
	m->wp += 4;
}

void
putstring(Msg *m, char *s)
{
	int len;

	len = strlen(s);
	putlong(m, len);
	putbytes(m, s, len);
}

void
putbytes(Msg *m, void *a, long n)
{
	if(m->wp+n > m->ep)
		error(Eencode);
	memmove(m->wp, a, n);
	m->wp += n;
}

void
putmpint(Msg *m, mpint *b)
{
	int bits, n;

	bits = mpsignif(b);
	putshort(m, bits);
	n = (bits+7)/8;
	if(m->wp+n > m->ep)
		error(Eencode);
	mptobe(b, m->wp, n, nil);
	m->wp += n;
}

void
putRSApub(Msg *m, RSApub *key)
{
	putlong(m, mpsignif(key->n));
	putmpint(m, key->ek);
	putmpint(m, key->n);
}

static ulong crctab[256];

static void
initsum32(void)
{
	ulong crc, poly;
	int i, j;

	poly = 0xEDB88320;
	for(i = 0; i < 256; i++){
		crc = i;
		for(j = 0; j < 8; j++){
			if(crc & 1)
				crc = (crc >> 1) ^ poly;
			else
				crc >>= 1;
		}
		crctab[i] = crc;
	}
}

static ulong
sum32(ulong lcrc, void *buf, int n)
{
	static int first=1;
	uchar *s = buf;
	ulong crc = lcrc;

	if(first){
		first=0;
		initsum32();
	}
	while(n-- > 0)
		crc = crctab[(crc^*s++)&0xff] ^ (crc>>8);
	return crc;
}

mpint*
rsapad(mpint *b, int n)
{
	int i, pad, nbuf;
	uchar buf[2560];
	mpint *c;

	if(n > sizeof buf)
		error("buffer too small in rsapad");

	nbuf = (mpsignif(b)+7)/8;
	pad = n - nbuf;
	assert(pad >= 3);
	mptobe(b, buf, nbuf, nil);
	memmove(buf+pad, buf, nbuf);

	buf[0] = 0;
	buf[1] = 2;
	for(i=2; i<pad-1; i++)
		buf[i]=1+fastrand()%255;
	buf[pad-1] = 0;
	c = betomp(buf, n, nil);
	memset(buf, 0, sizeof buf);
	return c;
}

mpint*
rsaunpad(mpint *b)
{
	int i, n;
	uchar buf[2560];

	n = (mpsignif(b)+7)/8;
	if(n > sizeof buf)
		error("buffer too small in rsaunpad");
	mptobe(b, buf, n, nil);

	/* the initial zero has been eaten by the betomp -> mptobe sequence */
	if(buf[0] != 2)
		error("bad data in rsaunpad");
	for(i=1; i<n; i++)
		if(buf[i]==0)
			break;
	return betomp(buf+i, n-i, nil);
}

void
mptoberjust(mpint *b, uchar *buf, int len)
{
	int n;

	n = mptobe(b, buf, len, nil);
	assert(n >= 0);
	if(n < len){
		len -= n;
		memmove(buf+len, buf, n);
		memset(buf, 0, len);
	}
}

mpint*
rsaencryptbuf(RSApub *key, uchar *buf, int nbuf)
{
	int n;
	mpint *a, *b, *c;

	n = (mpsignif(key->n)+7)/8;
	a = betomp(buf, nbuf, nil);
	b = rsapad(a, n);
	mpfree(a);
	c = rsaencrypt(key, b, nil);
	mpfree(b);
	return c;
}

