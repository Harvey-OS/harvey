#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include "util.h"

int dflag = 0;

#define errstr (strerror(errno))
static void sysfatal(char *fmt, ...);
static void say(char *fmt, ...);
static void warn(char *fmt, ...);

enum {
	ANamelen =	28,
	ADomlen =	48,
	AErrlen =	64,
	Challen =	8,
	Secretlen =	32,

	Ticketreqlen =	1+ANamelen+ADomlen+Challen+ANamelen+ANamelen,
	Ticketlen =	1+Challen+2*ANamelen+Deskeylen,
	Passreqlen =	1+2*ANamelen+1+Secretlen,

	ATreq =		1,
	APass =		3,
	AOk =		4,
	AErr =		5,

	ATs =		64,
	ATc =		65,
	ATp =		68,
};

typedef struct User User;
struct User
{
	int	keyok;
	uchar	key[Deskeylen];
	int	status;
	int	expire;
};

typedef struct Ticketreq Ticketreq;
typedef struct Ticket Ticket;
typedef struct Passreq Passreq;
struct Ticketreq
{
	char	which;
	char	authid[ANamelen+1];
	char	authdom[ADomlen+1];
	uchar	chal[Challen];
	char	hostid[ANamelen+1];
	char	uid[ANamelen+1];
};

struct Ticket
{
	uchar	which;
	uchar	chal[Challen];
	char	cuid[ANamelen+1];
	char	suid[ANamelen+1];
	uchar	key[Deskeylen];
};

struct Passreq
{
	int	which;
	char	oldpw[ANamelen+1];
	char	newpw[ANamelen+1];
	int	changesecret;
	char	secret[Secretlen+1];
};

static void transact(void);


static void
usage(void)
{
	fprint(2, "usage: authsrv9 [-d]\n");
	exit(1);
}

int
main(int argc, char *argv[])
{
	int c;

	randominit();
	openlog("authsrv9", 0, LOG_AUTH);

	while((c = getopt(argc, argv, "d")) != -1)
		switch(c) {
		case 'd':
			dflag++;
			break;
		default:
			usage();
		}
	argc -= optind;
	argv += optind;
	if(argc != 0)
		usage();

	/* openbsd's inetd redirects stderr to stdout (the network)... */
	close(2);
	open("/dev/null", O_WRONLY);

	alarm(2*60);
	for(;;)
		transact();
}

static void
ewrite(uchar *p, int n)
{
	if(write(1, p, n) != n)
		sysfatal("write: %s", errstr);
	say("wrote %d bytes", n);
}

static void
autherror(int fatal, char *err)
{
	uchar buf[1+AErrlen];

	say("autherror, fatal %d, err %s", fatal, err);

	memset(buf, 0, sizeof buf);
	buf[0] = AErr;
	memmove(buf+1, err, min(strlen(err), AErrlen));
	ewrite(buf, sizeof buf);

	if(fatal)
		exit(1);
}

static int
readfile(char *path, void *buf, int buflen, int exact)
{
	int fd;
	int n;

	memset(buf, 0, buflen);
	fd = open(path, O_RDONLY);
	if(fd < 0)
		return -1;
	n = readn(fd, buf, buflen);
	close(fd);
	if(n < 0)
		return -1;
	if(!exact && n == buflen)
		return -1;
	if(exact && n != buflen)
		return -1;
	return n;
}

static int
writefile(char *path, uchar *buf, int buflen)
{
	int fd;
	int n;

	fd = open(path, O_WRONLY|O_CREAT|O_TRUNC);
	if(fd < 0)
		return -1;
	n = write(fd, buf, buflen);
	close(fd);
	return n;
}

static int
getinfo(char **authid, char **authdom)
{
	char aid[ANamelen+1], adom[ANamelen+1];
	int naid, nadom;

	naid = readfile("/auth/authid", aid, sizeof aid-1, 0);
	if(naid < 0)
		return -1;
	nadom = readfile("/auth/authdom", adom, sizeof adom-1, 0);
	if(nadom < 0)
		return -1;
	aid[naid] = '\0';
	adom[nadom] = '\0';
	*authid = estrdup(aid);
	*authdom = estrdup(adom);
	return 0;
}

static void
getuserinfo(char *uid, User *u)
{
	char pre[128];
	char path[128];
	char buf[128];
	User t;
	char *e;
	uint expire;

	snprintf(pre, sizeof pre, "/auth/users/%s", uid);

	u->keyok = 0;
	memset(u->key, 0, sizeof u->key);
	u->status = 0;
	u->expire = 1;

	snprintf(path, sizeof path, "%s/key", pre);
	if(readfile(path, t.key, sizeof t.key, 1) < 0)
		return;

	snprintf(path, sizeof path, "%s/status", pre);
	if(readfile(path, buf, sizeof buf, 0) < 0)
		return;
	t.status = 0;
	if(eq(buf, "ok"))
		t.status = 1;
	else if(!eq(buf, "disabled"))
		warn("bad status in %s: %s", path, buf);

	snprintf(path, sizeof path, "%s/expire", pre);
	if(readfile(path, buf, sizeof buf, 0) < 0)
		return;
	t.expire = 1;
	if(eq(buf, "never")) {
		t.expire = 0;
	} else {
		expire = (uint)strtol(buf, &e, 10);
		if(*e == '\0')
			t.expire = expire;
		else
			warn("bad expire in %s: %s (remainder %s)", path, buf, e);
	}
	t.keyok = 1;
	memmove(u, &t, sizeof t);
}

static int
move(void *to, void *from, int n)
{
	memmove(to, from, n);
	return n;
}

static void
ticketrequnpack(Ticketreq *tr, uchar *p)
{
	memset(tr, 0, sizeof tr[0]);
	tr->which = *p++;
	p += move(tr->authid, p, ANamelen);
	p += move(tr->authdom, p, ADomlen);
	p += move(tr->chal, p, Challen);
	p += move(tr->hostid, p, ANamelen);
	p += move(tr->uid, p, ANamelen);
}

static void
ticketmk(Ticket *t, int which, uchar *chal, char *hostid, char *idr, uchar *key)
{
	memset(t, 0, sizeof (Ticket));
	t->which = which;
	memmove(t->chal, chal, sizeof t->chal);
	strcpy(t->cuid, hostid);
	strcpy(t->suid, idr);
	memmove(t->key, key, sizeof t->key);
}

static void
ticketpack(Ticket *t, uchar *buf)
{
	uchar *p;

	p = buf;
	*p++ = t->which;
	p += move(p, t->chal, Challen);
	p += move(p, t->cuid, ANamelen);
	p += move(p, t->suid, ANamelen);
	p += move(p, t->key, Deskeylen);
}

static void
genkey(uchar *p)
{
	randombuf(p, Deskeylen);
}

static void
clear(void *p, int n)
{
	memset(p, 0, n);
}

int
allowed(char *uid)
{
	char *path;
	char buf[2*1024+1];
	int fd;
	int n;
	char *p, *e;

	path = "/auth/badusers";
	fd = open(path, O_RDONLY);
	if(fd < 0) {
		warn("open %s: %s", path, errstr);
		return 0;
	}
	n = readn(fd, buf, sizeof buf-1);
	if(n < 0) {
		warn("read %s: %s", path, errstr);
		return 0;
	}
	if(n == sizeof buf) {
		warn("%s too long", path);
		return 0;
	}
	buf[n] = '\0';
	p = buf;
	for(;;) {
		e = strchr(p, '\n');
		if(e == nil)
			break;
		*e = '\0';
		if(strcmp(p, uid) == 0)
			return 0;
		p = e+1;
	}
	return 1;
}

static void
authtreq(Ticketreq *tr)
{
	char *authid, *authdom;
	User au, u;
	uchar ks[Deskeylen], kc[Deskeylen], kn[Deskeylen];
	uint now;
	char idr[ANamelen+1];
	Ticket tc, ts;
	uchar tcbuf[Ticketlen], tsbuf[Ticketlen];
	uchar tresp[1+Ticketlen+Ticketlen];
	int cok, sok;
	char *msg;

	/* # C->A: AuthTreq, IDs, DN, CHs, IDc, IDr */

	if(getinfo(&authid, &authdom) < 0) {
		warn("could not get authid & authdom");
		sysfatal("could not get authid & authdom");
	}

	now = time(nil);
	getuserinfo(authid, &au);
	say("authid, keyok %d, status %d, expire %u, now %u\n", au.keyok, au.status, au.expire, now);
	genkey(ks);
	sok = eq(tr->authid, authid) && eq(tr->authdom, authdom) && au.keyok && au.status && (au.expire == 0 || now > au.expire);

	getuserinfo(tr->hostid, &u);
	say("hostid '%s', user '%s', keyok %d, status %d, expire %u, now %u\n", tr->hostid, tr->uid, u.keyok, u.status, u.expire, now);
	genkey(kc);
	cok = u.keyok && u.status && (u.expire == 0 || u.expire > now);
	if(sok && cok) {
		memmove(ks, au.key, Deskeylen);
		memmove(kc, u.key, Deskeylen);
	}

	/* A->C: AuthOK, Kc{AuthTc, CHs, IDc, IDr, Kn}, Ks{AuthTs, CHs, IDc, IDr, Kn} */
	msg = "";
	strcpy(idr, "");
        if(eq(tr->hostid, tr->uid) || (eq(tr->hostid, authid) && allowed(tr->uid)))
                strcpy(idr, tr->uid);
	else
		msg = ", but hostid does not speak for uid";
	warn("ticketreq %s: authid %s, remote hostid %s, uid %s%s", (sok&&cok) ? "ok" : "bad", authid, tr->hostid, tr->uid, msg);

        genkey(kn);
	ticketmk(&tc, ATc, tr->chal, tr->hostid, idr, kn);
	ticketpack(&tc, tcbuf);
	authencrypt(kc, tcbuf, Ticketlen);
	if(0)say("ticket, cipher %s", hex(tcbuf, Ticketlen));

	ticketmk(&ts, ATs, tr->chal, tr->hostid, idr, kn);
	ticketpack(&ts, tsbuf);
	authencrypt(ks, tsbuf, Ticketlen);
	if(0)say("ticket, cipher %s", hex(tsbuf, Ticketlen));

	clear(kc, sizeof kc);
	clear(ks, sizeof ks);
	clear(kn, sizeof kn);
	clear(&tc, sizeof tc);
	clear(&ts, sizeof ts);
	clear(&au, sizeof au);
	clear(&u, sizeof u);

        tresp[0] = AOk;
	memmove(tresp+1, tcbuf, sizeof tcbuf);
	memmove(tresp+1+sizeof tcbuf, tsbuf, sizeof tsbuf);
	ewrite(tresp, sizeof tresp);
}

static void
passwordrequnpack(Passreq *pr, uchar *buf)
{
	uchar *p;

	memset(pr, 0, sizeof pr[0]);
	p = buf;
	pr->which = *p++;
	p += move(pr->oldpw, p, ANamelen);
	p += move(pr->newpw, p, ANamelen);
	pr->changesecret = *p++;
	p += move(pr->secret, p, Secretlen);
}

static char*
checkpass(char *pw)
{
	if(strlen(pw) < 8)
		return "bad password: too short";
	if(strlen(pw) > ANamelen)
		return "bad password: too long";
	return nil;
}

static void
authpass(Ticketreq *tr)
{
	User u;
	uint now;
	uchar kc[Deskeylen], kn[Deskeylen];
	Ticket tp;
	uchar tresp[1+Ticketlen];
	uchar prbuf[Passreqlen];
	Passreq pr;
	uchar okey[Deskeylen], nkey[Deskeylen];
	char *badpw;
	char path[128];
	int n;

	/*
	 * C->A: AuthPass, IDc, DN, CHc, IDc, IDc
	 * (request to change pass for tr->uid)
	 */
	getuserinfo(tr->uid, &u);

	/* A->C: Kc{AuthTp, CHc, IDc, IDc, Kn} */
	now = time(nil);
	say("authpass for user %s, u.keyok %d, u.status %d, u.expire %u, now %u", tr->uid, u.keyok, u.status, u.expire, now);
        genkey(kc);
        if(u.keyok && u.status && (u.expire == 0 || now > u.expire))
                memmove(kc, u.key, Deskeylen);
	clear(&u, sizeof u);
        genkey(kn);
        ticketmk(&tp, ATp, tr->chal, tr->uid, tr->uid, kn);
	tresp[0] = AOk;
	ticketpack(&tp, tresp+1);
	clear(&tp, sizeof tp);
	authencrypt(kc, tresp+1, Ticketlen);
        ewrite(tresp, sizeof tresp);

	for(;;) {
                /* C->A: Kn{AuthPass, old, new, changesecret, secret} */

                n = readn(0, prbuf, sizeof prbuf);
                if(n < 0)
                        sysfatal("read passwordreq: %s", errstr);
                if(n != sizeof prbuf)
                        sysfatal("short read for password request, want %d, got %d", sizeof prbuf, n);
		authdecrypt(kn, prbuf, sizeof prbuf);
                passwordrequnpack(&pr, prbuf);
		clear(prbuf, sizeof prbuf);

                if(pr.which != APass) {
			warn("pass change: for uid %s: wrong message type, want %d, saw %d (wrong password used)", tr->uid, APass, pr.which);
			clear(&pr, sizeof pr);
			clear(kc, sizeof kc);
                        autherror(1, "wrong message type for Passreq");
		}

                if(pr.changesecret) {
			warn("pass change: uid %s tried to change apop secret, not supported", tr->uid);
                        autherror(0, "changing apop secret not supported");
			clear(&pr, sizeof pr);
                        continue;
                }

		passtokey(okey, pr.oldpw);
		passtokey(nkey, pr.newpw);
                if(!memeq(kc, okey, sizeof okey)) {
			clear(&pr, sizeof pr);
			clear(okey, sizeof okey);
			clear(nkey, sizeof nkey);
			warn("pass change: uid %s gave bad old password", tr->uid);
                        autherror(0, "bad old password");
                        continue;
                }

                badpw = checkpass(pr.newpw);
                if(badpw != nil) {
			clear(&pr, sizeof pr);
			clear(okey, sizeof okey);
			clear(nkey, sizeof nkey);
			warn("pass change: uid %s gave bad new password: %s", tr->uid, badpw);
                        autherror(0, badpw);
                        continue;
                }

                snprintf(path, sizeof path, "/auth/users/%s/key", tr->uid);
                if(writefile(path, nkey, Deskeylen) < 0) {
			clear(&pr, sizeof pr);
			clear(okey, sizeof okey);
			clear(nkey, sizeof nkey);
			clear(kc, sizeof kc);
                        warn("pass change: storing new key for user %s failed: %s", tr->uid, errstr);
                        autherror(1, "storing new key failed");
                }

		clear(&pr, sizeof pr);
		clear(okey, sizeof okey);
		clear(nkey, sizeof nkey);
		clear(kc, sizeof kc);
		warn("pass change: password changed for user %s", tr->uid);

                /* A->C: AuthOK or AuthErr, 64-byte error message */
                tresp[0] = AOk;
                ewrite(tresp, 1);
		return;
        }
}

static void
transact(void)
{
	Ticketreq tr;
	uchar trbuf[Ticketreqlen];
	int n;

	say("reading ticketrequest");

	/* read ticket */
	n = readn(0, trbuf, sizeof trbuf);
	if(n < 0)
		sysfatal("read ticketreq: %s", errstr);
	if(n == 0)
		exit(0);
	if(n != sizeof trbuf)
		sysfatal("read ticketreq, want %d, got %d", sizeof trbuf, n);

        ticketrequnpack(&tr, trbuf);
	say("have ticketreq, which %d, authid %s authdom %s, chal %s hostid %s uid %s", tr.which, tr.authid, tr.authdom, hex(tr.chal, Challen), tr.hostid, tr.uid);

	switch(tr.which) {
	case ATreq:
		authtreq(&tr);
		break;
	case APass:
		authpass(&tr);
		break;
	default:
		autherror(1, "not supported");
		break;
	}

}

static void
log(int level, char *fmt, va_list ap)
{
	char *p;

	if(vasprintf(&p, fmt, ap) < 0)
		return;
	syslog(level, "%s (%s)", p, remoteaddr(0));
	free(p);
}

static void
sysfatal(char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	log(LOG_NOTICE, fmt, ap);
	va_end(ap);
	exit(1);
}

static void
warn(char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	log(LOG_WARNING, fmt, ap);
	va_end(ap);
}

static void
say(char *fmt, ...)
{
	va_list ap;

	if(!dflag)
		return;

	va_start(ap, fmt);
	log(LOG_INFO, fmt, ap);
	va_end(ap);
}
