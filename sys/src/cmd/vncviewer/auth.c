#include "vnc.h"
#include <libsec.h>

/*
 * Encrypt n bytes using the password
 * as key, padded with zeros to 8 bytes.
 */
enum
{
	VerLen	= 12
};

static char version[VerLen] = "RFB 003.003\n";

static uchar tab[256];

/* VNC reverses the bits of each byte before using as a des key */
static void
mktab(void)
{
	int i, j, k;
	static int once;

	if(once)
		return;
	once = 1;

	for(i=0; i<256; i++) {
		j=i;
		tab[i] = 0;
		for(k=0; k<8; k++) {
			tab[i] = (tab[i]<<1) | (j&1);
			j >>= 1;
		}
	}
}

static void
vncencrypt(uchar *buf, int n, char *pw)
{
	uchar *p;
	uchar key[9];
	DESstate s;

	mktab();
	memset(key, 0, sizeof key);
	strncpy((char*)key, pw, 8);
	for(p=key; *p; p++)
		*p = tab[*p];

	setupDESstate(&s, key, nil);
	desECBencrypt(buf, n, &s);
}

static int
readln(char *prompt, char *line, int len)
{
	char *p;
	int fd, ctl, n, nr;

	fd = open("/dev/cons", ORDWR);
	if(fd < 0)
		sysfatal("couldn't open cons");
	ctl = open("/dev/consctl", OWRITE);
	if(ctl < 0)
		sysfatal("couldn't open consctl");
	write(ctl, "rawon", 5);
	fprint(fd, "%s", prompt);
	nr = 0;
	p = line;
	for(;;){
		n = read(fd, p, 1);
		if(n < 0){
			close(fd);
			close(ctl);
			return -1;
		}
		if(n == 0 || *p == '\n' || *p == '\r'){
			*p = '\0';
			write(fd, "\n", 1);
			close(fd);
			close(ctl);
			return nr;
		}
		if(*p == '\b'){
			if(nr > 0){
				nr--;
				p--;
			}
		}else if(*p == 21){		/* cntrl-u */
			fprint(fd, "\n%s", prompt);
			nr = 0;
			p = line;
		}else{
			nr++;
			p++;
		}
		if(nr == len){
			fprint(fd, "line too long; try again\n%s", prompt);
			nr = 0;
			p = line;
		}
	}
	return -1;
}

int
vncauth(Vnc *v, int shared)
{
	char msg[VerLen+1], pw[128], *reason;
	uchar chal[VncChalLen];
	ulong auth;

	msg[VerLen] = '\0';
	vncrdbytes(v, msg, VerLen);
	if(strncmp(msg, "RFB ", 4) != 0) {
		werrstr("bad rfb version \"%s\"", msg);
		return -1;
	}
	if(verbose)
		print("server version %s\n", msg);

	vncwrbytes(v, version, VerLen);
	vncflush(v);

	auth = vncrdlong(v);
	switch(auth) {
	default:
		werrstr("unknown auth type 0x%lux", auth);
		if(verbose)
			print("unknown auth type 0x%lux", auth);
		return -1;

	case AFailed:
		reason = vncrdstring(v);
		werrstr("%s", reason);
		if(verbose)
			print("auth failed: %s\n", reason);
		return -1;

	case ANoAuth:
		if(verbose)
			print("no auth needed");
		break;

	case AVncAuth:
		vncrdbytes(v, chal, VncChalLen);

		readln("password: ", pw, sizeof(pw));

		vncencrypt(chal, VncChalLen, pw);
		memset(pw, 0, sizeof pw);
		vncwrbytes(v, chal, VncChalLen);
		vncflush(v);

		auth = vncrdlong(v);
		switch(auth) {
		default:
			werrstr("unknown auth response 0x%lux", auth);
			return -1;
		case VncAuthFailed:
			werrstr("authentication failed");
			return -1;
		case VncAuthTooMany:
			werrstr("authentication failed - too many tries");
			return -1;
		case VncAuthOK:
			break;
		}
		break;
	}

	vncwrlong(v, shared);
	vncflush(v);

	v->dim = vncrdpoint(v);
	v->Pixfmt = vncrdpixfmt(v);
	v->name = vncrdstring(v);
	return 0;
}

int
vncauth_srv(Vnc *v)
{
	char msg[VerLen+1];

	vncwrbytes(v, version, VerLen);
	vncflush(v);

	msg[VerLen] = '\0';
	vncrdbytes(v, msg, VerLen);
	if(verbose)
		print("client version: %s", msg);

	vncwrlong(v, ANoAuth);
	vncflush(v);

	return 0;
}
