#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ndb.h>
#include <regexp.h>
#include <mp.h>
#include <libsec.h>
#include <authsrv.h>
#include "authcmdlib.h"

int debug;
Ndb *db;
char raddr[128];

/* Microsoft auth constants */
enum {
	MShashlen = 16,
	MSchallen = 8,
	MSresplen = 24,
};

int	ticketrequest(Ticketreq*);
void	challengebox(Ticketreq*);
void	changepasswd(Ticketreq*);
void	apop(Ticketreq*, int);
void	chap(Ticketreq*);
void	mschap(Ticketreq*);
void	http(Ticketreq*);
void	vnc(Ticketreq*);
int	speaksfor(char*, char*);
void	replyerror(char*, ...);
void	getraddr(char*);
void	mkkey(char*);
void	randombytes(uchar*, int);
void	nthash(uchar hash[MShashlen], char *passwd);
void	lmhash(uchar hash[MShashlen], char *passwd);
void	mschalresp(uchar resp[MSresplen], uchar hash[MShashlen], uchar chal[MSchallen]);
void	desencrypt(uchar data[8], uchar key[7]);
int	tickauthreply(Ticketreq*, char*);
void	safecpy(char*, char*, int);


void
main(int argc, char *argv[])
{
	char buf[TICKREQLEN];
	Ticketreq tr;

	ARGBEGIN{
	case 'd':
		debug++;
	}ARGEND

	strcpy(raddr, "unknown");
	if(argc >= 1)
		getraddr(argv[argc-1]);

	alarm(10*60*1000);	/* kill a connection after 10 minutes */

	db = ndbopen("/lib/ndb/auth");
	if(db == 0)
		syslog(0, AUTHLOG, "no /lib/ndb/auth");

	srand(time(0)*getpid());
	for(;;){
		if(readn(0, buf, TICKREQLEN) <= 0)
			exits(0);

		convM2TR(buf, &tr);
		switch(buf[0]){
		case AuthTreq:
			ticketrequest(&tr);
			break;
		case AuthChal:
			challengebox(&tr);
			break;
		case AuthPass:
			changepasswd(&tr);
			break;
		case AuthApop:
			apop(&tr, AuthApop);
			break;
		case AuthChap:
			chap(&tr);
			break;
		case AuthMSchap:
			mschap(&tr);
			break;
		case AuthCram:
			apop(&tr, AuthCram);
			break;
		case AuthHttp:
			http(&tr);
			break;
		case AuthVNC:
			vnc(&tr);
			break;
		default:
			syslog(0, AUTHLOG, "unknown ticket request type: %d", buf[0]);
			exits(0);
		}
	}
	/* not reached */
}

int
ticketrequest(Ticketreq *tr)
{
	char akey[DESKEYLEN];
	char hkey[DESKEYLEN];
	Ticket t;
	char tbuf[2*TICKETLEN+1];

	if(findkey(KEYDB, tr->authid, akey) == 0){
		/* make one up so caller doesn't know it was wrong */
		mkkey(akey);
		if(debug)
			syslog(0, AUTHLOG, "tr-fail authid %s", raddr);
	}
	if(findkey(KEYDB, tr->hostid, hkey) == 0){
		/* make one up so caller doesn't know it was wrong */
		mkkey(hkey);
		if(debug)
			syslog(0, AUTHLOG, "tr-fail hostid %s(%s)", tr->hostid, raddr);
	}

	memset(&t, 0, sizeof(t));
	memmove(t.chal, tr->chal, CHALLEN);
	strcpy(t.cuid, tr->uid);
	if(speaksfor(tr->hostid, tr->uid))
		strcpy(t.suid, tr->uid);
	else {
		mkkey(akey);
		mkkey(hkey);
		if(debug)
			syslog(0, AUTHLOG, "tr-fail %s@%s(%s) -> %s@%s no speaks for",
				tr->uid, tr->hostid, raddr, tr->uid, tr->authid);
	}

	mkkey(t.key);

	tbuf[0] = AuthOK;
	t.num = AuthTc;
	convT2M(&t, tbuf+1, hkey);
	t.num = AuthTs;
	convT2M(&t, tbuf+1+TICKETLEN, akey);
	if(write(1, tbuf, 2*TICKETLEN+1) < 0){
		if(debug)
			syslog(0, AUTHLOG, "tr-fail %s@%s(%s): hangup",
				tr->uid, tr->hostid, raddr);
		exits(0);
	}
	if(debug)
		syslog(0, AUTHLOG, "tr-ok %s@%s(%s) -> %s@%s",
			tr->uid, tr->hostid, raddr, tr->uid, tr->authid);

	return 0;
}

void
challengebox(Ticketreq *tr)
{
	long chal;
	char *key, *netkey;
	char kbuf[DESKEYLEN], nkbuf[DESKEYLEN], hkey[DESKEYLEN];
	char buf[NETCHLEN+1];
	char *err;

	key = findkey(KEYDB, tr->uid, kbuf);
	netkey = findkey(NETKEYDB, tr->uid, nkbuf);
	if(key == 0 && netkey == 0){
		/* make one up so caller doesn't know it was wrong */
		mkkey(nkbuf);
		netkey = nkbuf;
		if(debug)
			syslog(0, AUTHLOG, "cr-fail uid %s@%s", tr->uid, raddr);
	}
	if(findkey(KEYDB, tr->hostid, hkey) == 0){
		/* make one up so caller doesn't know it was wrong */
		mkkey(hkey);
		if(debug)
			syslog(0, AUTHLOG, "cr-fail hostid %s %s@%s", tr->hostid,
				tr->uid, raddr);
	}

	/*
	 * challenge-response
	 */
	memset(buf, 0, sizeof(buf));
	buf[0] = AuthOK;
	chal = lnrand(MAXNETCHAL);
	sprint(buf+1, "%lud", chal);
	if(write(1, buf, NETCHLEN+1) < 0)
		exits(0);
	if(readn(0, buf, NETCHLEN) < 0)
		exits(0);
	if(!(key && netcheck(key, chal, buf))
	&& !(netkey && netcheck(netkey, chal, buf))
	&& (err = secureidcheck(tr->uid, buf)) != nil){
		replyerror("cr-fail %s %s %s", err, tr->uid, raddr);
		logfail(tr->uid);
		if(debug)
			syslog(0, AUTHLOG, "cr-fail %s@%s(%s): bad resp",
				tr->uid, tr->hostid, raddr);
		return;
	}
	succeed(tr->uid);

	/*
	 *  reply with ticket & authenticator
	 */
	if(tickauthreply(tr, hkey) < 0){
		if(debug)
			syslog(0, AUTHLOG, "cr-fail %s@%s(%s): hangup",
				tr->uid, tr->hostid, raddr);
		exits(0);
	}

	if(debug)
		syslog(0, AUTHLOG, "cr-ok %s@%s(%s)",
			tr->uid, tr->hostid, raddr);
}

void
changepasswd(Ticketreq *tr)
{
	Ticket t;
	char tbuf[TICKETLEN+1];
	char prbuf[PASSREQLEN];
	Passwordreq pr;
	char okey[DESKEYLEN], nkey[DESKEYLEN];
	char *err;

	if(findkey(KEYDB, tr->uid, okey) == 0){
		/* make one up so caller doesn't know it was wrong */
		mkkey(okey);
		syslog(0, AUTHLOG, "cp-fail uid %s", raddr);
	}

	/* send back a ticket with a new key */
	memmove(t.chal, tr->chal, CHALLEN);
	mkkey(t.key);
	tbuf[0] = AuthOK;
	t.num = AuthTp;
	safecpy(t.cuid, tr->uid, sizeof(t.cuid));
	safecpy(t.suid, tr->uid, sizeof(t.suid));
	convT2M(&t, tbuf+1, okey);
	write(1, tbuf, sizeof(tbuf));

	/* loop trying passwords out */
	for(;;){
		if(readn(0, prbuf, PASSREQLEN) < 0)
			exits(0);
		convM2PR(prbuf, &pr, t.key);
		if(pr.num != AuthPass){
			replyerror("protocol botch1: %s", raddr);
			exits(0);
		}
		passtokey(nkey, pr.old);
		if(memcmp(nkey, okey, DESKEYLEN)){
			replyerror("protocol botch2: %s", raddr);
			continue;
		}
		if(*pr.new){
			err = okpasswd(pr.new);
			if(err){
				replyerror("%s %s", err, raddr);
				continue;
			}
			passtokey(nkey, pr.new);
		}
		if(pr.changesecret && setsecret(KEYDB, tr->uid, pr.secret) == 0){
			replyerror("can't write secret %s", raddr);
			continue;
		}
		if(*pr.new && setkey(KEYDB, tr->uid, nkey) == 0){
			replyerror("can't write key %s", raddr);
			continue;
		}
		break;
	}

	prbuf[0] = AuthOK;
	write(1, prbuf, 1);
	succeed(tr->uid);
	return;
}

void
http(Ticketreq *tr)
{
	Ticket t;
	char tbuf[TICKETLEN+1];
	char key[DESKEYLEN];
	char *p;
	Biobuf *b;
	int n;

	n = strlen(tr->uid);
	b = Bopen("/sys/lib/httppasswords", OREAD);
	if(b == nil){
		replyerror("no password file", raddr);
		return;
	}

	/* find key */
	for(;;){
		p = Brdline(b, '\n');
		if(p == nil)
			break;
		p[Blinelen(b)-1] = 0;
		if(strncmp(p, tr->uid, n) == 0)
		if(p[n] == ' ' || p[n] == '\t'){
			p += n;
			break;
		}
	}
	Bterm(b);
	if(p == nil) {
		randombytes((uchar*)key, DESKEYLEN);
	} else {
		while(*p == ' ' || *p == '\t')
			p++;
		passtokey(key, p);
	}

	/* send back a ticket encrypted with the key */
	randombytes((uchar*)t.chal, CHALLEN);
	mkkey(t.key);
	tbuf[0] = AuthOK;
	t.num = AuthHr;
	safecpy(t.cuid, tr->uid, sizeof(t.cuid));
	safecpy(t.suid, tr->uid, sizeof(t.suid));
	convT2M(&t, tbuf+1, key);
	write(1, tbuf, sizeof(tbuf));
}

static char*
domainname(void)
{
	static char sysname[Maxpath];
	static char *domain;
	int n;

	if(domain)
		return domain;
	if(*sysname)
		return sysname;

	domain = csgetvalue(0, "sys", sysname, "dom", nil);
	if(domain)
		return domain;

	n = readfile("/dev/sysname", sysname, sizeof(sysname)-1);
	if(n < 0){
		strcpy(sysname, "kremvax");
		return sysname;
	}
	sysname[n] = 0;

	return sysname;
}

static int
h2b(char c)
{
	if(c >= '0' && c <= '9')
		return c - '0';
	if(c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	if(c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	return 0;
}

void
apop(Ticketreq *tr, int type)
{
	int challen, i, tries;
	char *secret, *hkey, *p;
	Ticketreq treq;
	DigestState *s;
	char sbuf[SECRETLEN], hbuf[DESKEYLEN];
	char tbuf[TICKREQLEN];
	char buf[MD5dlen*2];
	uchar digest[MD5dlen], resp[MD5dlen];
	ulong rb[4];
	char chal[256];

	USED(tr);

	/*
	 *  Create a challenge and send it.
	 */
	randombytes((uchar*)rb, sizeof(rb));
	p = chal;
	p += snprint(p, sizeof(chal), "<%lux%lux.%lux%lux@%s>",
		rb[0], rb[1], rb[2], rb[3], domainname());
	challen = p - chal;
	print("%c%-5d%s", AuthOKvar, challen, chal);

	/* give user a few attempts */
	for(tries = 0; ; tries++) {
		/*
		 *  get ticket request
		 */
		if(readn(0, tbuf, TICKREQLEN) < 0)
			exits(0);
		convM2TR(tbuf, &treq);
		tr = &treq;
		if(tr->type != type)
			exits(0);

		/*
		 * read response
		 */
		if(readn(0, buf, MD5dlen*2) < 0)
			exits(0);
		for(i = 0; i < MD5dlen; i++)
			resp[i] = (h2b(buf[2*i])<<4)|h2b(buf[2*i+1]);

		/*
		 * lookup
		 */
		secret = findsecret(KEYDB, tr->uid, sbuf);
		hkey = findkey(KEYDB, tr->hostid, hbuf);
		if(hkey == 0 || secret == 0){
			replyerror("apop-fail bad response %s", raddr);
			logfail(tr->uid);
			if(tries > 5)
				return;
			continue;
		}

		/*
		 *  check for match
		 */
		if(type == AuthCram){
			hmac_md5((uchar*)chal, challen,
				(uchar*)secret, strlen(secret),
				digest, nil);
		} else {
			s = md5((uchar*)chal, challen, 0, 0);
			md5((uchar*)secret, strlen(secret), digest, s);
		}
		if(memcmp(digest, resp, MD5dlen) != 0){
			replyerror("apop-fail bad response %s", raddr);
			logfail(tr->uid);
			if(tries > 5)
				return;
			continue;
		}
		break;
	}

	succeed(tr->uid);

	/*
	 *  reply with ticket & authenticator
	 */
	if(tickauthreply(tr, hkey) < 0)
		exits(0);

	if(debug){
		if(type == AuthCram)
			syslog(0, AUTHLOG, "cram-ok %s %s", tr->uid, raddr);
		else
			syslog(0, AUTHLOG, "apop-ok %s %s", tr->uid, raddr);
	}
}

enum {
	VNCchallen=	16,
};

/* VNC reverses the bits of each byte before using as a des key */
uchar swizzletab[256] = {
 0x0, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
 0x8, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8, 0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
 0x4, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4, 0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
 0xc, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec, 0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
 0x2, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2, 0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
 0xa, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea, 0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
 0x6, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6, 0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
 0xe, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee, 0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
 0x1, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1, 0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
 0x9, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9, 0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
 0x5, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5, 0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
 0xd, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed, 0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
 0x3, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3, 0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
 0xb, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb, 0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
 0x7, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7, 0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
 0xf, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
};

void
vnc(Ticketreq *tr)
{
	uchar chal[VNCchallen+6];
	uchar reply[VNCchallen];
	char *secret, *hkey;
	char sbuf[SECRETLEN], hbuf[DESKEYLEN];
	DESstate s;
	int i;

	/*
	 *  Create a challenge and send it.
	 */
	randombytes(chal+6, VNCchallen);
	chal[0] = AuthOKvar;
	sprint((char*)chal+1, "%-5d", VNCchallen);
	if(write(1, chal, sizeof(chal)) != sizeof(chal))
		return;

	/*
	 *  lookup keys (and swizzle bits)
	 */
	memset(sbuf, 0, sizeof(sbuf));
	secret = findsecret(KEYDB, tr->uid, sbuf);
	if(secret == 0){
		randombytes((uchar*)sbuf, sizeof(sbuf));
		secret = sbuf;
	}
	for(i = 0; i < 8; i++)
		secret[i] = swizzletab[(uchar)secret[i]];

	hkey = findkey(KEYDB, tr->hostid, hbuf);
	if(hkey == 0){
		randombytes((uchar*)hbuf, sizeof(hbuf));
		hkey = hbuf;
	}

	/*
	 *  get response
	 */
	if(readn(0, reply, sizeof(reply)) != sizeof(reply))
		return;

	/*
	 *  decrypt response and compare
	 */
	setupDESstate(&s, (uchar*)secret, nil);
	desECBdecrypt(reply, sizeof(reply), &s);
	if(memcmp(reply, chal+6, VNCchallen) != 0){
		replyerror("vnc-fail bad response %s", raddr);
		logfail(tr->uid);
		return;
	}
	succeed(tr->uid);

	/*
	 *  reply with ticket & authenticator
	 */
	if(tickauthreply(tr, hkey) < 0)
		exits(0);

	if(debug)
		syslog(0, AUTHLOG, "vnc-ok %s %s", tr->uid, raddr);
}

void
chap(Ticketreq *tr)
{
	char *secret, *hkey;
	DigestState *s;
	char sbuf[SECRETLEN], hbuf[DESKEYLEN];
	uchar digest[MD5dlen];
	char chal[CHALLEN];
	OChapreply reply;

	/*
	 *  Create a challenge and send it.
	 */
	randombytes((uchar*)chal, sizeof(chal));
	write(1, chal, sizeof(chal));

	/*
	 *  get chap reply
	 */
	if(readn(0, &reply, sizeof(reply)) < 0)
		exits(0);
	safecpy(tr->uid, reply.uid, sizeof(tr->uid));

	/*
	 * lookup
	 */
	secret = findsecret(KEYDB, tr->uid, sbuf);
	hkey = findkey(KEYDB, tr->hostid, hbuf);
	if(hkey == 0 || secret == 0){
		replyerror("chap-fail bad response %s", raddr);
		logfail(tr->uid);
		exits(0);
	}

	/*
	 *  check for match
	 */
	s = md5(&reply.id, 1, 0, 0);
	md5((uchar*)secret, strlen(secret), 0, s);
	md5((uchar*)chal, sizeof(chal), digest, s);

	if(memcmp(digest, reply.resp, MD5dlen) != 0){
		replyerror("chap-fail bad response %s", raddr);
		logfail(tr->uid);
		exits(0);
	}

	succeed(tr->uid);

	/*
	 *  reply with ticket & authenticator
	 */
	if(tickauthreply(tr, hkey) < 0)
		exits(0);

	if(debug)
		syslog(0, AUTHLOG, "chap-ok %s %s", tr->uid, raddr);
}

void
printresp(uchar resp[MSresplen])
{
	char buf[200], *p;
	int i;

	p = buf;
	for(i=0; i<MSresplen; i++)
		p += sprint(p, "%.2ux ", resp[i]);
	syslog(0, AUTHLOG, "resp = %s", buf);
}


void
mschap(Ticketreq *tr)
{

	char *secret, *hkey;
	char sbuf[SECRETLEN], hbuf[DESKEYLEN];
	uchar chal[CHALLEN];
	uchar hash[MShashlen];
	uchar hash2[MShashlen];
	uchar resp[MSresplen];
	OMSchapreply reply;
	int dupe, lmok, ntok;
	DigestState *s;
	uchar digest[SHA1dlen];

	/*
	 *  Create a challenge and send it.
	 */
	randombytes((uchar*)chal, sizeof(chal));
	write(1, chal, sizeof(chal));

	/*
	 *  get chap reply
	 */
	if(readn(0, &reply, sizeof(reply)) < 0)
		exits(0);

	safecpy(tr->uid, reply.uid, sizeof(tr->uid));
	/*
	 * lookup
	 */
	secret = findsecret(KEYDB, tr->uid, sbuf);
	hkey = findkey(KEYDB, tr->hostid, hbuf);
	if(hkey == 0 || secret == 0){
		replyerror("mschap-fail bad response %s/%s(%s)",
			tr->uid, tr->hostid, raddr);
		logfail(tr->uid);
		exits(0);
	}

	lmhash(hash, secret);
	mschalresp(resp, hash, chal);
	lmok = memcmp(resp, reply.LMresp, MSresplen) == 0;
	nthash(hash, secret);
	mschalresp(resp, hash, chal);
	ntok = memcmp(resp, reply.NTresp, MSresplen) == 0;
	dupe = memcmp(reply.LMresp, reply.NTresp, MSresplen) == 0;

	/*
	 * It is valid to send the same response in both the LM and NTLM 
	 * fields provided one of them is correct, if neither matches,
	 * or the two fields are different and either fails to match, 
	 * the whole sha-bang fails.
	 *
	 * This is an improvement in security as it allows clients who
	 * wish to do NTLM auth (which is insecure) not to send
	 * LM tokens (which is very insecure).
	 *
	 * Windows servers supports clients doing this also though
	 * windows clients don't seem to use the feature.
	 */
	if((!ntok && !lmok) || ((!ntok || !lmok) && !dupe)){
		replyerror("mschap-fail bad response %s/%s(%s) %d,%d,%d",
			tr->uid, tr->hostid, raddr, dupe, lmok, ntok);
		logfail(tr->uid);
		exits(0);
	}

	succeed(tr->uid);

	/*
	 *  reply with ticket & authenticator
	 */
	if(tickauthreply(tr, hkey) < 0)
		exits(0);

	if(debug)
		replyerror("mschap-ok %s/%s(%s) %ux",
			tr->uid, tr->hostid, raddr);

	nthash(hash, secret);
	md4(hash, 16, hash2, 0);
	s = sha1(hash2, 16, 0, 0);
	sha1(hash2, 16, 0, s);
	sha1(chal, 8, digest, s);

	if(write(1, digest, 16) < 0)
		exits(0);
}

void
nthash(uchar hash[MShashlen], char *passwd)
{
	uchar buf[512];
	int i;

	for (i = 0; *passwd && i + 1 < sizeof(buf);) {
		Rune r;
		passwd += chartorune(&r, passwd);
		buf[i++] = r;
		buf[i++] = r >> 8;
	}

	memset(hash, 0, 16);

	md4(buf, i, hash, 0);
}

void
lmhash(uchar hash[MShashlen], char *passwd)
{
	uchar buf[14];
	char *stdtext = "KGS!@#$%";
	int i;

	strncpy((char*)buf, passwd, sizeof(buf));
	for(i=0; i<sizeof(buf); i++)
		if(buf[i] >= 'a' && buf[i] <= 'z')
			buf[i] += 'A' - 'a';

	memset(hash, 0, 16);
	memcpy(hash, stdtext, 8);
	memcpy(hash+8, stdtext, 8);

	desencrypt(hash, buf);
	desencrypt(hash+8, buf+7);
}

void
mschalresp(uchar resp[MSresplen], uchar hash[MShashlen], uchar chal[MSchallen])
{
	int i;
	uchar buf[21];

	memset(buf, 0, sizeof(buf));
	memcpy(buf, hash, MShashlen);

	for(i=0; i<3; i++) {
		memmove(resp+i*MSchallen, chal, MSchallen);
		desencrypt(resp+i*MSchallen, buf+i*7);
	}
}

void
desencrypt(uchar data[8], uchar key[7])
{
	ulong ekey[32];

	key_setup(key, ekey);
	block_cipher(ekey, data, 0);
}

/*
 *  return true of the speaker may speak for the user
 *
 *  a speaker may always speak for himself/herself
 */
int
speaksfor(char *speaker, char *user)
{
	Ndbtuple *tp, *ntp;
	Ndbs s;
	int ok;
	char notuser[Maxpath];

	if(strcmp(speaker, user) == 0)
		return 1;

	if(db == 0)
		return 0;

	tp = ndbsearch(db, &s, "hostid", speaker);
	if(tp == 0)
		return 0;

	ok = 0;
	snprint(notuser, sizeof notuser, "!%s", user);
	for(ntp = tp; ntp; ntp = ntp->entry)
		if(strcmp(ntp->attr, "uid") == 0){
			if(strcmp(ntp->val, notuser) == 0){
				ok = 0;
				break;
			}
			if(*ntp->val == '*' || strcmp(ntp->val, user) == 0)
				ok = 1;
		}
	ndbfree(tp);
	return ok;
}

/*
 *  return an error reply
 */
void
replyerror(char *fmt, ...)
{
	char buf[AERRLEN+1];
	va_list arg;

	memset(buf, 0, sizeof(buf));
	va_start(arg, fmt);
	vseprint(buf + 1, buf + sizeof(buf), fmt, arg);
	va_end(arg);
	buf[AERRLEN] = 0;
	buf[0] = AuthErr;
	write(1, buf, AERRLEN+1);
	syslog(0, AUTHLOG, buf+1);
}

void
getraddr(char *dir)
{
	int n;
	char *cp;
	char file[Maxpath];

	raddr[0] = 0;
	snprint(file, sizeof(file), "%s/remote", dir);
	n = readfile(file, raddr, sizeof(raddr)-1);
	if(n < 0)
		return;
	raddr[n] = 0;

	cp = strchr(raddr, '\n');
	if(cp)
		*cp = 0;
	cp = strchr(raddr, '!');
	if(cp)
		*cp = 0;
}

void
mkkey(char *k)
{
	randombytes((uchar*)k, DESKEYLEN);
}

void
randombytes(uchar *buf, int len)
{
	int i;

	if(readfile("/dev/random", (char*)buf, len) >= 0)
		return;

	for(i = 0; i < len; i++)
		buf[i] = rand();
}

/*
 *  reply with ticket and authenticator
 */
int
tickauthreply(Ticketreq *tr, char *hkey)
{
	Ticket t;
	Authenticator a;
	char buf[TICKETLEN+AUTHENTLEN+1];

	memset(&t, 0, sizeof(t));
	memmove(t.chal, tr->chal, CHALLEN);
	safecpy(t.cuid, tr->uid, sizeof t.cuid);
	safecpy(t.suid, tr->uid, sizeof t.suid);
	mkkey(t.key);
	buf[0] = AuthOK;
	t.num = AuthTs;
	convT2M(&t, buf+1, hkey);
	memmove(a.chal, t.chal, CHALLEN);
	a.num = AuthAc;
	a.id = 0;
	convA2M(&a, buf+TICKETLEN+1, t.key);
	if(write(1, buf, TICKETLEN+AUTHENTLEN+1) < 0)
		return -1;
	return 0;
}

void
safecpy(char *to, char *from, int len)
{
	strncpy(to, from, len);
	to[len-1] = 0;
}
