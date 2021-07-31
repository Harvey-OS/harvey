#include <u.h>
#include <libc.h>
#include <auth.h>
#include "authsrv.h"

char	server[NAMELEN] = "unknown";

enum{
	Rauthlen= 2*AUTHLEN + 3*NAMELEN,
	Wauthlen= AUTHLEN + 2*AUTHLEN + 2*DESKEYLEN,
};

int	fsauth(char*, char*);
int	fsfail(char*, char*, char*, ...);

void main(int argc, char *argv[]){
	char r[Rauthlen], w[Wauthlen];
	char buf[8192];
	int n;

	USED(argc, argv);
	argv0 = "fsauth";

	for(;;){
		n = read(0, r, Rauthlen);
		switch(n){
		case Rauthlen:
			if(fsauth(r, w))
				if(write(1, w, Wauthlen) != Wauthlen){
					syslog(0, AUTHLOG, "error: %r replying to %s", server);
					exits("err");
				}
			break;
		case 0:
			syslog(0, AUTHLOG, "EOF from %s", server);
			exits("eof");
		case -1:
			syslog(0, AUTHLOG, "error: %r from %s", server);
			exits("err");
		default:
			syslog(0, AUTHLOG, "bad read count %d from %s; skipping", n, server);
			read(0, buf, sizeof buf);
			break;
		}
	}
}

int
fsfail(char *skey, char *chal, char *fmt, ...)
{
	char buf[AUTHLEN+ERRLEN];

	memset(buf, 0, sizeof buf);
	memmove(buf, chal, AUTHLEN);
	buf[0] = FSerr;
	doprint(buf+AUTHLEN, buf + sizeof(buf) / sizeof(*buf), fmt, &fmt + 1);
	syslog(0, AUTHLOG, "%s", buf+AUTHLEN);
	if(skey){
		encrypt(skey, buf, sizeof buf);
		write(2, buf, sizeof buf);
	}
	return 0;
}

/*
 * S->A	KS{FSschal, ChS, C, KC{FScchal, ChC, S}}, S
 * A->S	KS{FSok, ChS, KC{FSctick, ChC, KN, KS{FSstick, ChS, KN}}}
 * or	KS{FSerr, ChS, error}
 */
int
fsauth(char *in, char *out)
{
	char *skey, *ckey;
	char *srv, *chc, *csrv, *chs, *client;
	int i, j;

	chs = in;
	chc = in + AUTHLEN + NAMELEN;
	client = chs + AUTHLEN;
	csrv = chc + AUTHLEN;
	srv = csrv + NAMELEN;
	srv[NAMELEN-1] = '\0';
	skey = findkey(srv);
	if(!skey)
		return fsfail(skey, chs, "server %s unknown", srv);
	memcpy(server, srv, NAMELEN);

	decrypt(skey, chs, 2*(AUTHLEN+NAMELEN));

	if(chs[0] != FSschal)
		return fsfail(skey, chs, "server %s: first byte not FSschal", srv);
	client[NAMELEN-1] = '\0';
	ckey = findkey(client);
	if(!ckey)
		return fsfail(skey, chs, "server %s: client unknown", srv);
	decrypt(ckey, chc, AUTHLEN + NAMELEN);
	if(chc[0] != FScchal)
		return fsfail(skey, chs,
			"server %s: ChC[0] not FScchal", srv);
	csrv[NAMELEN-1] = '\0';
	if(strncmp(csrv, srv, NAMELEN) != 0 && strcmp(csrv, "any") != 0)
		return fsfail(skey, chs, "server %s not %s", srv, csrv);

	chc[0] = FSctick;
	chs[0] = FSstick;
	i = 0;
	memmove(out, chs, AUTHLEN);
	out[0] = FSok;
	i += AUTHLEN;
	memmove(&out[i], chc, AUTHLEN);
	i += AUTHLEN;
	for(j=0; j<DESKEYLEN; j++)
		out[i++] = nrand(256);
	memmove(&out[i], chs, AUTHLEN);
	i += AUTHLEN;
	memmove(&out[i], out+2*AUTHLEN, DESKEYLEN);
	i += DESKEYLEN;
	encrypt(skey, out+2*AUTHLEN+DESKEYLEN, AUTHLEN+DESKEYLEN);
	encrypt(ckey, out+AUTHLEN, 2*AUTHLEN+2*DESKEYLEN);
	encrypt(skey, out, i);
	syslog(0, AUTHLOG, "client %s to %s ok", client, srv);
	return 1;
}
