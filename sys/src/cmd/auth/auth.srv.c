#include <u.h>
#include <libc.h>
#include <auth.h>
#include <bio.h>
#include <ndb.h>
#include <regexp.h>
#include <mp.h>
#include <libsec.h>
#include "authsrv.h"

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
int	speaksfor(char*, char*);
void	translate(char*, char*);
void	replyerror(char*, ...);
void	getraddr(char*);
void	mkkey(char*);
void	randombytes(uchar*, int);
void	nthash(uchar hash[MShashlen], char *passwd);
void	lmhash(uchar hash[MShashlen], char *passwd);
void	mschalresp(uchar resp[MSresplen], uchar hash[MShashlen], uchar chal[MSchallen]);
void	desencrypt(uchar data[8], uchar key[7]);

extern	secureidcheck(char*, char*);

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
		syslog(0, AUTHLOG, "can't open database");
	
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
		default:
			syslog(0, AUTHLOG, "unknown ticket request type: %d", buf[0]);
			break;
		}
	}
	exits(0);
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
	else
		strcpy(t.suid, "none");
	translate(tr->authdom, t.suid);

	mkkey(t.key);

	tbuf[0] = AuthOK;
	t.num = AuthTc;
	convT2M(&t, tbuf+1, hkey);
	t.num = AuthTs;
	convT2M(&t, tbuf+1+TICKETLEN, akey);
	if(write(1, tbuf, 2*TICKETLEN+1) < 0){
		if(debug)
			syslog(0, AUTHLOG, "tr-fail %s@%s(%s) -> %s: hangup",
				tr->uid, tr->hostid, raddr, t.suid);
		exits(0);
	}
	if(debug)
		syslog(0, AUTHLOG, "tr-ok %s@%s(%s) -> %s@%s",
			tr->uid, tr->hostid, raddr, t.suid, tr->authid);

	return 0;
}

void
challengebox(Ticketreq *tr)
{
	long chal;
	char *key, *netkey;
	Ticket t;
	Authenticator a;
	char kbuf[DESKEYLEN], nkbuf[DESKEYLEN], hkey[DESKEYLEN];
	char buf[TICKETLEN+AUTHENTLEN+1];

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
	&& !secureidcheck(tr->uid, buf)){
		replyerror("cr-fail bad response %s %s", tr->uid, raddr);
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
	memset(&t, 0, sizeof(t));
	memmove(t.chal, tr->chal, CHALLEN);
	strcpy(t.cuid, tr->uid);
	strcpy(t.suid, tr->uid);
	mkkey(t.key);
	buf[0] = AuthOK;
	t.num = AuthTs;
	convT2M(&t, buf+1, hkey);
	memmove(a.chal, t.chal, CHALLEN);
	a.num = AuthAc;
	a.id = 0;
	convA2M(&a, buf+TICKETLEN+1, t.key);
	if(write(1, buf, TICKETLEN+AUTHENTLEN+1) < 0){
		if(debug)
			syslog(0, AUTHLOG, "cr-fail %s@%s(%s) -> %s: hangup",
				tr->uid, tr->hostid, raddr, t.suid);
		exits(0);
	}

	if(debug)
		syslog(0, AUTHLOG, "cr-ok %s@%s(%s) -> %s",
			tr->uid, tr->hostid, raddr, t.suid);
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
	memmove(t.cuid, tr->uid, NAMELEN);
	memmove(t.suid, tr->uid, NAMELEN);
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
	memmove(t.cuid, tr->uid, NAMELEN);
	memmove(t.suid, tr->uid, NAMELEN);
	convT2M(&t, tbuf+1, key);
	write(1, tbuf, sizeof(tbuf));
}

static char*
domainname(void)
{
	static char sysname[NAMELEN];
	static char domain[Ndbvlen];
	Ndbtuple *t;

	if(*domain)
		return domain;

	if(*sysname)
		return sysname;

	if(readfile("/dev/sysname", sysname, sizeof(sysname)) < 0){
		strcpy(sysname, "kremvax");
		return sysname;
	}

	t = csgetval(0, "sys", sysname, "dom", domain);
	if(t == 0)
		return sysname;

	ndbfree(t);
	return domain;
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
	Ticket t;
	Ticketreq treq;
	Authenticator a;
	DigestState *s;
	char sbuf[SECRETLEN], hbuf[DESKEYLEN];
	char tbuf[TICKREQLEN];
	char buf[TICKREQLEN];
	uchar digest[MD5dlen], resp[MD5dlen];
	ulong rb[4];
	char chal[APOPCHLEN];

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
	memset(&t, 0, sizeof(t));
	memmove(t.chal, tr->chal, CHALLEN);
	strcpy(t.cuid, tr->uid);
	strcpy(t.suid, tr->uid);
	mkkey(t.key);
	buf[0] = AuthOK;
	t.num = AuthTs;
	convT2M(&t, buf+1, hkey);
	memmove(a.chal, t.chal, CHALLEN);
	a.num = AuthAc;
	a.id = 0;
	convA2M(&a, buf+TICKETLEN+1, t.key);
	if(write(1, buf, TICKETLEN+AUTHENTLEN+1) < 0)
		exits(0);

	if(debug){
		if(type == AuthCram)
			syslog(0, AUTHLOG, "cram-ok %s %s", t.suid, raddr);
		else
			syslog(0, AUTHLOG, "apop-ok %s %s", t.suid, raddr);
	}
}

void
chap(Ticketreq *tr)
{
	char *secret, *hkey;
	Ticket t;
	Authenticator a;
	DigestState *s;
	char sbuf[SECRETLEN], hbuf[DESKEYLEN];
	char buf[TICKREQLEN];
	uchar digest[MD5dlen];
	char chal[CHALLEN];
	Chapreply reply;
	uchar md5buf[512];
	int n;

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
	
	memcpy(tr->uid, reply.uid, NAMELEN);
	tr->uid[NAMELEN-1] = 0;
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

	md5buf[0] = reply.id;
	n = 1;
	memmove(md5buf+n, secret, strlen(secret));
	n += strlen(secret);
	memmove(md5buf+n, chal, sizeof(chal));
	n += sizeof(chal);
	md5(md5buf, n, digest, 0);

	if(memcmp(digest, reply.resp, MD5dlen) != 0){
		replyerror("chap-fail bad response %s", raddr);
		logfail(tr->uid);
		exits(0);
	}

	succeed(tr->uid);

	/*
	 *  reply with ticket & authenticator
	 */
	memset(&t, 0, sizeof(t));
	memmove(t.chal, tr->chal, CHALLEN);
	strcpy(t.cuid, tr->uid);
	strcpy(t.suid, tr->uid);
	mkkey(t.key);
	buf[0] = AuthOK;
	t.num = AuthTs;
	convT2M(&t, buf+1, hkey);
	memmove(a.chal, t.chal, CHALLEN);
	a.num = AuthAc;
	a.id = 0;
	convA2M(&a, buf+TICKETLEN+1, t.key);
	if(write(1, buf, TICKETLEN+AUTHENTLEN+1) < 0)
		exits(0);

	if(debug)
		syslog(0, AUTHLOG, "chap-ok %s %s", t.suid, raddr);
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
	Ticket t;
	Authenticator a;
	char sbuf[SECRETLEN], hbuf[DESKEYLEN];
	char buf[TICKREQLEN];
	uchar chal[CHALLEN];
	uchar hash[MShashlen];
	uchar hash2[MShashlen];
	uchar resp[MSresplen];
	MSchapreply reply;
	int lmok, ntok;
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
	
	memcpy(tr->uid, reply.uid, NAMELEN);
	tr->uid[NAMELEN-1] = 0;
	/*
	 * lookup
	 */
	secret = findsecret(KEYDB, tr->uid, sbuf);
	hkey = findkey(KEYDB, tr->hostid, hbuf);
	if(hkey == 0 || secret == 0){
		replyerror("mschap-fail bad response %s", raddr);
		logfail(tr->uid);
		exits(0);
	}

	/*
	 *  check for match on LM algorithm
	 */
	lmhash(hash, secret);
	mschalresp(resp, hash, chal);
	lmok = memcmp(resp, reply.LMresp, MSresplen) == 0;

	nthash(hash, secret);
	mschalresp(resp, hash, chal);
	ntok = memcmp(resp, reply.NTresp, MSresplen) == 0;

	if(!ntok){
		replyerror("mschap-fail bad response %s %ux", raddr, (lmok<<1)|ntok);
		logfail(tr->uid);
		exits(0);
	}

	succeed(tr->uid);

	/*
	 *  reply with ticket & authenticator
	 */
	memset(&t, 0, sizeof(t));
	memmove(t.chal, tr->chal, CHALLEN);
	strcpy(t.cuid, tr->uid);
	strcpy(t.suid, tr->uid);
	mkkey(t.key);
	buf[0] = AuthOK;
	t.num = AuthTs;
	convT2M(&t, buf+1, hkey);
	memmove(a.chal, t.chal, CHALLEN);
	a.num = AuthAc;
	a.id = 0;
	convA2M(&a, buf+TICKETLEN+1, t.key);
	if(write(1, buf, TICKETLEN+AUTHENTLEN+1) < 0)
		exits(0);

	if(debug)
		syslog(0, AUTHLOG, "mschap-ok %s %s %ux", t.suid, raddr, (lmok<<1)|ntok);

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
	
	for(i=0; *passwd && i<sizeof(buf); passwd++) {
		buf[i++] = *passwd;
		buf[i++] = 0;
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
	char notuser[NAMELEN+2];
	
	if(strcmp(speaker, user) == 0)
		return 1;

	if(db == 0)
		return 0;
	
	tp = ndbsearch(db, &s, "hostid", speaker);
	if(tp == 0)
		return 0;

	ok = 0;
	strcpy(notuser, "!");
	strcat(notuser, user);
	for(ntp = tp; ntp; ntp = ntp->entry)
		if(strcmp(ntp->attr, "uid") == 0){
			if(strcmp(ntp->val, notuser) == 0)
				break;
			if(*ntp->val == '*' || strcmp(ntp->val, user) == 0)
				ok = 1;
		}
	ndbfree(tp);
	return ok;
}

/*
 *  translate a userid for the destination domain
 */
void
translate(char *domain, char *user)
{
	Ndbtuple *tp, *ntp, *mtp;
	Ndbs s;
	
	if(*domain == 0)
		return;

	if(db == 0)
		return;
	
	tp = ndbsearch(db, &s, "authdom", domain);
	if(tp == 0)
		return;

	for(ntp = tp; ntp; ntp = ntp->entry)
		if(strcmp(ntp->attr, "cuid") == 0 && strcmp(ntp->val, user) == 0)
			break;

	if(ntp == 0){
		ndbfree(tp);
		return;
	}

	for(mtp = ntp->line; mtp != ntp; mtp = mtp->line)
		if(strcmp(mtp->attr, "suid") == 0){
			memset(user, 0, NAMELEN);
			strncpy(user, mtp->val, NAMELEN-1);
			break;
		}
	ndbfree(tp);
}

/*
 *  return an error reply
 */
void
replyerror(char *fmt, ...)
{
	char buf[ERRLEN+1];
	va_list arg;

	memset(buf, 0, sizeof(buf));
	va_start(arg, fmt);
	doprint(buf + 1, buf + sizeof(buf), fmt, arg);
	va_end(arg);
	buf[ERRLEN] = 0;
	buf[0] = AuthErr;
	write(1, buf, ERRLEN+1);
	syslog(0, AUTHLOG, buf+1);
}

void
getraddr(char *dir)
{
	int n, fd;
	char *cp;
	char file[3*NAMELEN];

	snprint(file, sizeof(file), "%s/remote", dir);
	fd = open(file, OREAD);
	if(fd < 0)
		return;
	n = read(fd, raddr, sizeof(raddr)-1);
	close(fd);
	if(n <= 0)
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
