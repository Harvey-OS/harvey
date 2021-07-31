#include <u.h>
#include <libc.h>
#include <auth.h>
#include <bio.h>
#include <ndb.h>
#include <regexp.h>
#include "authsrv.h"

int debug;
Ndb *db;
char raddr[128];

int	ticketrequest(Ticketreq*);
void	challengebox(Ticketreq*);
void	changepasswd(Ticketreq*);
int	speaksfor(char*, char*);
void	translate(char*, char*);
int	readn(int, char*, int);
void	replyerror(char*, ...);
void	getraddr(char*);
void	mkkey(char*);

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

	srand(time(0)*getpid());
	for(;;){
		if(readn(0, buf, TICKREQLEN) < 0)
			exits(0);
	
		db = ndbopen("/lib/ndb/auth");
		if(db == 0)
			syslog(0, AUTHLOG, "can't open database");
	
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
		syslog(0, AUTHLOG, "t-fail authid %s", raddr);
	}
	if(findkey(KEYDB, tr->hostid, hkey) == 0){
		/* make one up so caller doesn't know it was wrong */
		mkkey(hkey);
		syslog(0, AUTHLOG, "t-fail hostid %s", raddr);
	}

	memset(&t, 0, sizeof(t));
	memmove(t.chal, tr->chal, CHALLEN);
	strcpy(t.cuid, tr->uid);
	if(speaksfor(tr->hostid, tr->uid))
		strcpy(t.suid, tr->uid);
	else
		strcpy(t.suid, "none");
	translate(tr->authdom, t.suid);

	srand(time(0) ^ hkey[0] ^ akey[0]);
	mkkey(t.key);

	tbuf[0] = AuthOK;
	t.num = AuthTc;
	convT2M(&t, tbuf+1, hkey);
	t.num = AuthTs;
	convT2M(&t, tbuf+1+TICKETLEN, akey);
	if(write(1, tbuf, 2*TICKETLEN+1) < 0){
		syslog(0, AUTHLOG, "can't reply %s", raddr);
		exits(0);
	}
	syslog(0, AUTHLOG, "t-ok %s %s", t.suid, raddr);

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
		syslog(0, AUTHLOG, "cr-fail uid %s", raddr);
	}
	if(findkey(KEYDB, tr->hostid, hkey) == 0){
		/* make one up so caller doesn't know it was wrong */
		mkkey(hkey);
		syslog(0, AUTHLOG, "cr-fail hostid %s", raddr);
	}

	/*
	 * challenge-response
	 */
	memset(buf, 0, sizeof(buf));
	buf[0] = AuthOK;
	srand(time(0) ^ kbuf[0] ^ nkbuf[0]);
	chal = lnrand(MAXNETCHAL);
	sprint(buf+1, "%lud", chal);
	if(write(1, buf, NETCHLEN+1) < 0)
		exits(0);
	if(readn(0, buf, NETCHLEN) < 0)
		exits(0);
	if(!(key && netcheck(key, chal, buf))
	&& !(netkey && netcheck(netkey, chal, buf))){
		replyerror("cr-fail bad response %s", raddr);
		logfail(tr->uid);
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
	srand(time(0) ^ hkey[0]);
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

	syslog(0, AUTHLOG, "cr-ok %s %s", t.suid, raddr);
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
	srand(time(0));
	mkkey(t.key);
	tbuf[0] = AuthOK;
	t.num = AuthTc;
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
			replyerror("protocol botch %s", raddr);
			exits(0);
		}
		passtokey(nkey, pr.old);
		if(memcmp(nkey, okey, DESKEYLEN)){
			replyerror("protocol botch %s", raddr);
			continue;
		}
		err = okpasswd(pr.new);
		if(err){
			replyerror("%s %s", err, raddr);
			continue;
		}
		passtokey(nkey, pr.new);
		if(setkey(KEYDB, tr->uid, nkey) != 0)
			break;
		replyerror("can't write key %s", raddr);
	}

	prbuf[0] = AuthOK;
	write(1, prbuf, 1);
	succeed(tr->uid);
	return;
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

	memset(buf, 0, sizeof(buf));
	doprint(buf + 1, buf + sizeof(buf) / sizeof(*buf), fmt, &fmt + 1);
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
	int i;

	for(i = 0; i < DESKEYLEN; i++)
		k[i] = nrand(256);
}
