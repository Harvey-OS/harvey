#include "all.h"
#include "/sys/src/libauth/authlocal.h"

static Chalstuff *head;

static void	prticketreq(int, Ticketreq*);

Xfid *
xfauth(Xfile *root, String *user)
{
	char ubuf[NAMELEN], *uid;
	Xfile *xp;
	Xfid *xf;

	if(user->n >= NAMELEN-1)
		return 0;
	memcpy(ubuf, &user->s[1], user->n-1);
	ubuf[user->n-1] = 0;
	uid = strstore(ubuf);
	xp = xfile((ulong)uid, root->s->cchal, 1);
	if(xp == 0)
		return 0;
	xp->parent = root;
	xp->name = uid;
	xf = xfid(uid, xp, 1);
	return xf;
}

long
xfauthread(Xfid *xf, long offset, uchar *readptr, long count)
{
	Chalstuff *cp;
	int r;

	chat("xfauthread %s...", xf->uid);
	if(offset >= NETCHLEN)
		return 0;
	if(offset+count > NETCHLEN)
		count = NETCHLEN-offset;
	if(xfid(xf->uid, xf->xp->parent, 0)){
		chat("authorized...");
		return 0;
	}
	cp = (Chalstuff *)xf->offset;
	if(cp == 0){
		cp = calloc(1, sizeof(Chalstuff));
		cp->afd = -1;
		cp->asfd = -1;
		r = getchal(cp, xf->uid);
		if(r < 0){
			chat("%r...");
			free(cp);
			return 0;
		}
		cp->next = head;
		head = cp;
		cp->xf = xf;
		xf->offset = (ulong)cp;
	}
	xf->mode |= Open;
	cp->tstale = nfstime + 5*60;
	memcpy(readptr, &cp->chal[offset], count);
	return count;
}

long
xfauthwrite(Xfid *xf, long offset, uchar *readptr, long count)
{
	char response[NETCHLEN];
	Chalstuff *cp = (Chalstuff *)xf->offset;
	Session *s = xf->xp->parent->s;
	int r = -1, pfd[2];
	Xfid *newxf;
	Fid *f = 0;

	pfd[0] = pfd[1] = -1;
	chat("xfauthwrite %s...", xf->uid);
	if(cp == 0 || offset > 0 || count > NETCHLEN)
		goto out;
	memset(response, 0, NETCHLEN);
	memcpy(response, readptr, count);
	if(pipe(pfd) < 0){
		chat("pipe: %r...");
		goto out;
	}
	++s->count;
	f = newfid(s);
	switch(rfork(RFFDG|RFPROC|RFNOWAIT)){
	case -1:
		chat("fork: %r...");
		goto out;
	case 0:
		close(pfd[0]);
		if(chalreply(cp, response) < 0){
			clog("chalreply: %r\n");
			_exits("Sorry");
		}
		if(xfattach(s, xf->uid, f-s->fids) < 0)
			_exits("Sorry");
		write(pfd[1], response, 1);
		_exits(0);
	default:
		close(pfd[1]);
		pfd[1] = -1;
	}
	r = niread(pfd[0], response, NETCHLEN);
	chat("read pfd %d...", r);
	if(r < 0)
		chat("%r...");
out:
	xfauthclose(xf);
	if(pfd[0] >= 0)
		close(pfd[0]);
	if(pfd[1] >= 0)
		close(pfd[1]);
	if(r <= 0){
		if(f)
			putfid(s, f);
	}else{
		newxf = xfid(xf->uid, s->root, 1);
		newxf->urfid = f;
		clog("uid=%s fid=%d...", newxf->uid, newxf->urfid-s->fids);
	}
	return r;
}

void
xfauthclose(Xfid *xf)
{
	Chalstuff *cp = (Chalstuff *)xf->offset;
	Chalstuff *t, *prev;

	chat("authclose(\"%s\")...", xf->uid);
	xf->mode &= ~Open;
	xf->offset = 0;
	if(cp){
		for(prev=0,t=head; t; prev=t,t=t->next)
			if(cp == t)
				break;
		if(t == 0)
			panic("unlinked chal");
		if(prev)
			prev->next = cp->next;
		else
			head = cp->next;
		if(cp->afd >= 0)
			close(cp->afd);
		if(cp->asfd >= 0)
			close(cp->asfd);
		free(cp);
	}
}

void
authtimer(long now)
{
	Chalstuff *cp, *t;
	int n;

	cp = head;
	n = 0;
	while(cp){
		t = cp;
		cp = cp->next;
		if(now >= t->tstale)
			++n, xfauthclose(t->xf);
	}
	if(n > 0)
		chat("authtimer\n");
}

int
xfauthremove(Xfid *xf, char *uid)
{
	int n;

	chat("authremove...");
	if(strcmp(xf->uid, uid) != 0)
		return -1;
	if(xfid(xf->uid, xf->xp->parent, 0)){
		n = xfpurgeuid(xf->xp->parent->s, xf->uid);
		chat("cleared %d fids...", n);
	}
	return 0;
}

int
xfattach(Session *s, char *uid, int fid)
{
	char tbuf[2*TICKETLEN+AUTHENTLEN];

	chat("xfattach...");
	if(getticket(s, tbuf) < 0){
		chat("getticket: %r...");
		return -1;
	}
	s->f.fid = fid;
	strncpy(s->f.uname, uid, NAMELEN);
	strncpy(s->f.aname, s->spec, NAMELEN);
	memcpy(s->f.ticket, &tbuf[TICKETLEN], TICKETLEN);
	memcpy(s->f.auth, &tbuf[2*TICKETLEN], AUTHENTLEN);
	if(xmesg(s, Tattach) < 0){
		chat("xfattach: %r...");
		return -1;
	}
	return checkreply(s, tbuf);
}

int
getticket(Session *s, char *tbuf)
{
	Ticketreq tr;
	char ebuf[ERRLEN];
	char trbuf[TICKREQLEN];
	char xtbuf[TICKETLEN+4];
	int afd, rv;
	uchar *p;

	/* create ticket request */
	memset(&tr, 0, sizeof(Ticketreq));
	tr.type = AuthTreq;
	strncpy(tr.authid, s->authid, NAMELEN-1);
	strncpy(tr.authdom, s->authdom, DOMLEN-1);
	memcpy(tr.chal, s->schal, CHALLEN);
	_asrdfile("/dev/hostowner", tr.hostid, NAMELEN);
	_asrdfile("/dev/user", tr.uid, NAMELEN);
	if(chatty)
		prticketreq(2, &tr);
	convTR2M(&tr, trbuf);

	/* get ticket */
	afd = authdial();
	if(afd < 0){
		snprint(ebuf, ERRLEN, "authdial: %r\n");
		werrstr(ebuf);
		return -1;
	}
	rv = _asgetticket(afd, trbuf, tbuf);
	if(rv < 0){
		snprint(ebuf, ERRLEN, "getticket: %r\n");
		close(afd);
		werrstr(ebuf);
		return -1;
	}
	close(afd);

	/* get authenticator */
	afd = open("/dev/authenticator", ORDWR);
	if(afd < 0){
		snprint(ebuf, ERRLEN, "authenticator: %r\n");
		werrstr(ebuf);
		return -1;
	}
	memcpy(xtbuf, tbuf, TICKETLEN);
	p = (uchar *)&xtbuf[TICKETLEN];
	p[0] = s->count;
	p[1] = s->count>>8;
	p[2] = s->count>>16;
	p[3] = s->count>>24;
	if(write(afd, xtbuf, TICKETLEN+4) < 0){
		snprint(ebuf, ERRLEN, "authenticator: %r\n");
		close(afd);
		werrstr(ebuf);
		return -1;
	}
	if(niread(afd, tbuf+2*TICKETLEN, AUTHENTLEN) < 0){
		snprint(ebuf, ERRLEN, "authenticator: %r\n");
		close(afd);
		werrstr(ebuf);
		return -1;
	}
	close(afd);
	return 0;
}

int
checkreply(Session *s, char *tbuf)
{
	char xtbuf[TICKETLEN+AUTHENTLEN+CHALLEN+4];
	int afd;
	uchar *p;

	/* check authenticator */
	afd = open("/dev/authcheck", OWRITE);
	if(afd < 0){
		chat("open authcheck: %r...");
		return -1;
	}
	memcpy(xtbuf, tbuf, TICKETLEN);
	memcpy(xtbuf+TICKETLEN, s->f.rauth, AUTHENTLEN);
	memcpy(xtbuf+TICKETLEN+AUTHENTLEN, s->cchal, CHALLEN);
	p = (uchar *)&xtbuf[TICKETLEN+AUTHENTLEN+CHALLEN];
	p[0] = s->count;
	p[1] = s->count>>8;
	p[2] = s->count>>16;
	p[3] = s->count>>24;
	if(write(afd, xtbuf, TICKETLEN+AUTHENTLEN+CHALLEN+4) < 0){
		chat("write authcheck: %r...");
		close(afd);
		return -1;
	}
	chat("authcheck OK...");
	close(afd);

	return 0;
}

static char *authtypes[] = {
	[AuthTreq]	"AuthTreq",
	[AuthChal]	"AuthChal",
	[AuthPass]	"AuthPass",
	[AuthOK]	"AuthOK",
	[AuthErr]	"AuthErr",
	[AuthTs]	"AuthTs",
	[AuthTc]	"AuthTc",
	[AuthAs]	"AuthAs",
	[AuthAc]	"AuthAc",
};

static char *
authtype(int type)
{
	static char buf[16];
	char *p;

	if(type < 0 || type >= nelem(authtypes))
		p = 0;
	else
		p = authtypes[type];
	if(p == 0)
		sprint(p=buf, "<%d>", type);
	return p;
}

static void
prticketreq(int fd, Ticketreq *tr)
{
	uchar *p = (uchar *)tr->chal;

	fprint(fd, "ticket request: %s\n", authtype(tr->type));
	fprint(fd, "\tauthid=%s, authdom=%s\n", tr->authid, tr->authdom);
	fprint(fd, "\tchal = %.2ux %.2ux %.2ux %.2ux %.2ux %.2ux %.2ux %.2ux\n",
		p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
	fprint(fd, "\thostid=%s, uid=%s\n", tr->hostid, tr->uid);
}
