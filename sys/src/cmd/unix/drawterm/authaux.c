#include	"lib9.h"
#include	"sys.h"
#include	"error.h"

typedef struct Crypt	Crypt;
struct Crypt
{
	Crypt		*next;
	Ticket		t;
	Authenticator	a;
	char		tbuf[TICKETLEN];	/* remote ticket */
};

struct Session
{
	Lock	lk;
	Lock	send;
	Crypt	*cache;			/* cache of tickets */
	char	cchal[CHALLEN];		/* client challenge */
	char	schal[CHALLEN];		/* server challenge */
	char	authid[NAMELEN];	/* server encryption uid */
	char	authdom[DOMLEN];	/* server encryption domain */
	ulong	cid;			/* challenge id */
	int	valid;
};

struct
{
	Lock	lk;
	Crypt	*free;
} cryptalloc;

char	eve[NAMELEN] = "none";
char	evekey[DESKEYLEN];
char	hostdomain[DOMLEN];

/*
 *  return true if current user is eve
 */
int
iseve(void)
{
	return strcmp(eve, up->user) == 0;
}

/*
 * crypt entries are allocated from a pool rather than allocated using malloc so
 * the memory can be protected from reading by devproc. The base and top of the
 * crypt arena is stored in palloc for devproc.
 */
Crypt*
newcrypt(void)
{
	Crypt *c;

	lock(&cryptalloc.lk);
	if(cryptalloc.free) {
		c = cryptalloc.free;
		cryptalloc.free = c->next;
		unlock(&cryptalloc.lk);
		memset(c, 0, sizeof(Crypt));
		return c;
	}

	cryptalloc.free = mallocz(sizeof(Crypt));
	if(cryptalloc.free == 0)
		panic("newcrypt");

/*
	palloc.cmembase = (ulong)cryptalloc.free;
	palloc.cmemtop = palloc.cmembase+(sizeof(Crypt));
*/
	unlock(&cryptalloc.lk);
	return newcrypt();
}

void
freecrypt(Crypt *c)
{
	lock(&cryptalloc.lk);
	c->next = cryptalloc.free;
	cryptalloc.free = c;
	unlock(&cryptalloc.lk);
}

/*
 *  return the info received in the session message on this channel.
 *  if no session message has been exchanged, do it.
 */
int
sysfsession(ulong *arg)
{
	int i, n;
	Chan *c;
	Crypt *cp;
	Session *s;
	Ticketreq tr;
	Fcall f;
	char buf[MAXMSG];

	c = fdtochan(arg[0], OWRITE, 0, 1);
	if(waserror()){
		cclose(c);
		nexterror();
	}

	/* add a session structure to the channel if it has none */
	lock(&c->r.l);
	s = c->session;
	if(s == 0){
		s = mallocz(sizeof(Session));
		if(s == 0){
			unlock(&c->r.l);
			error(Enomem);
		}
		c->session = s;
	}
	unlock(&c->r.l);

	lock(&s->send);

	if(s->valid == 0 && (c->flag & CMSG) == 0){

		/*
		 *  Exchange a session message with the server.
		 *  If an error occurs reading or writing,
		 *  assume this is a mount of a mount and turn off
		 *  authentication.
		 */
		if(!waserror()){
			for(i = 0; i < CHALLEN; i++)
				s->cchal[i] = nrand(256);
			f.tag = NOTAG;
			f.type = Tsession;
			memmove(f.chal, s->cchal, CHALLEN);
			n = convS2M(&f, buf);
			if((*devtab[c->type].write)(c, buf, n, 0) != n)
				error(Emountrpc);
			n = (*devtab[c->type].read)(c, buf, sizeof buf, 0);
			if(n == 2 && buf[0] == 'O' && buf[1] == 'K')
				n = (*devtab[c->type].read)(c, buf, sizeof buf, 0);
			poperror();
			if(convM2S(buf, &f, n) == 0){
				unlock(&s->send);
				error(Emountrpc);
			}
			switch(f.type){
			case Rsession:
				memmove(s->schal, f.chal, CHALLEN);
				memmove(s->authid, f.authid, NAMELEN);
				memmove(s->authdom, f.authdom, DOMLEN);
				break;
			case Rerror:
				unlock(&s->send);
				error(f.ename);
			default:
				unlock(&s->send);
				error(Emountrpc);
			}
		}
		s->valid = 1;
	}
	unlock(&s->send);

	/* 
	 *  If server requires no ticket, or user is "none", or a ticket
	 *  is already cached, zero the request type
	 */
	tr.type = AuthTreq;
	if(strcmp(up->user, "none") == 0 || s->authid[0] == 0)
		tr.type = 0;
	else{
		lock(&s->lk);
		for(cp = s->cache; cp; cp = cp->next)
			if(strcmp(cp->t.cuid, up->user) == 0){
				tr.type = 0;
				break;
			}
		unlock(&s->lk);
	}

	/*  create ticket request */
	memmove(tr.chal, s->schal, CHALLEN);
	memmove(tr.authid, s->authid, NAMELEN);
	memmove(tr.authdom, s->authdom, DOMLEN);
	memmove(tr.uid, up->user, NAMELEN);
	memmove(tr.hostid, eve, NAMELEN);
	convTR2M(&tr, (char*)arg[1]);

	cclose(c);
	poperror();
	return 0;
}

/*
 *  attach tickets to a session
 */
long
sysfauth(ulong *arg)
{
	Chan *c;
	char *ta;
	Session *s;
	Crypt *cp, *ncp, **l;
	char tbuf[2*TICKETLEN];

	c = fdtochan(arg[0], OWRITE, 0, 1);
	s = c->session;
	if(s == 0)
		error("fauth must follow fsession");
	cp = newcrypt();
	if(waserror()){
		freecrypt(cp);
		nexterror();
	}

	/*  ticket supplied, use it */
	ta = (char*)arg[1];
	memmove(tbuf, ta, 2*TICKETLEN);
	convM2T(tbuf, &cp->t, evekey);
	if(cp->t.num != AuthTc)
		error("bad AuthTc in ticket");
	if(strncmp(up->user, cp->t.cuid, NAMELEN) != 0)
		error("bad uid in ticket");
	if(memcmp(cp->t.chal, s->schal, CHALLEN) != 0)
		error("bad chal in ticket");
	memmove(cp->tbuf, tbuf+TICKETLEN, TICKETLEN);

	/* string onto local list, replace old version */
	lock(&s->lk);
	l = &s->cache;
	for(ncp = s->cache; ncp; ncp = *l){
		if(strcmp(ncp->t.cuid, up->user) == 0){
			*l = ncp->next;
			freecrypt(ncp);
			break;
		}
		l = &ncp->next;
	}
	cp->next = s->cache;
	s->cache = cp;
	unlock(&s->lk);
	poperror();
	return 0;
}

/*
 *  free a session created by fsession
 */
void
freesession(Session *s)
{
	Crypt *cp, *next;

	for(cp = s->cache; cp; cp = next) {
		next = cp->next;
		freecrypt(cp);
	}
	free(s);
}

/*
 *  called by mattach() to fill in the Tattach message
 */
ulong
authrequest(Session *s, Fcall *f)
{
	Crypt *cp;
	ulong id, dofree;

	/* no authentication if user is "none" or if no ticket required by remote */
	if(s == 0 || s->authid[0] == 0 || strcmp(up->user, "none") == 0){
		memset(f->ticket, 0, TICKETLEN);
		memset(f->auth, 0, AUTHENTLEN);
		return 0;
	}

	/* look for ticket in cache */
	dofree = 0;
	lock(&s->lk);
	for(cp = s->cache; cp; cp = cp->next)
		if(strcmp(cp->t.cuid, up->user) == 0)
			break;

	id = s->cid++;
	unlock(&s->lk);

	if(cp == 0){
		/*
		 *  create a ticket using hostkey, this solves the
		 *  chicken and egg problem
		 */
		cp = newcrypt();
		cp->t.num = AuthTs;
		memmove(cp->t.chal, s->schal, CHALLEN);
		memmove(cp->t.cuid, up->user, NAMELEN);
		memmove(cp->t.suid, up->user, NAMELEN);
		memmove(cp->t.key, evekey, DESKEYLEN);
		convT2M(&cp->t, f->ticket, evekey);
		dofree = 1;
	} else
		memmove(f->ticket, cp->tbuf, TICKETLEN);

	/* create an authenticator */
	memmove(cp->a.chal, s->schal, CHALLEN);
	cp->a.num = AuthAc;
	cp->a.id = id;
	convA2M(&cp->a, f->auth, cp->t.key);
	if(dofree)
		freecrypt(cp);
	return id;
}

/*
 *  called by mattach() to check the Rattach message
 */
void
authreply(Session *s, ulong id, Fcall *f)
{
	Crypt *cp;

	if(s == 0)
		return;

	lock(&s->lk);
	for(cp = s->cache; cp; cp = cp->next)
		if(strcmp(cp->t.cuid, up->user) == 0)
			break;
	unlock(&s->lk);

	/* we're getting around authentication */
	if(s == 0 || cp == 0 || s->authid[0] == 0 || strcmp(up->user, "none") == 0)
		return;

	convM2A(f->rauth, &cp->a, cp->t.key);
	if(cp->a.num != AuthAs){
		print("bad encryption type\n");
		error("server lies");
	}
	if(memcmp(cp->a.chal, s->cchal, sizeof(cp->a.chal))){
		print("bad returned challenge\n");
		error("server lies");
	}	
	if(cp->a.id != id){
		print("bad returned id\n");
		error("server lies");
	}
}

/*
 *  called by devcons() for #c/authenticate
 *
 *  The protocol is
 *	1) read ticket request from #c/authenticate
 *	2) write ticket+authenticator to #c/authenticate. if it matches
 *	  the challenge the user is changed to the suid field of the ticket
 *	3) read authenticator (to confirm this is the server advertised)
 */
long
authread(Chan *c, char *a, int n)
{
	Crypt *cp;
	int i;
	Ticketreq tr;

	if(c->u.aux == 0){
		/*
		 *  first read returns a ticket request
		 */
		if(n != TICKREQLEN)
			error(Ebadarg);
		c->u.aux = newcrypt();
		cp = c->u.aux;

		memset(&tr, 0, sizeof(tr));
		tr.type = AuthTreq;
		strcpy(tr.hostid, eve);
		strcpy(tr.authid, eve);
		strcpy(tr.authdom, hostdomain);
		strcpy(tr.uid, up->user);
		for(i = 0; i < CHALLEN; i++)
			tr.chal[i] = nrand(256);
		memmove(cp->a.chal, tr.chal, CHALLEN);
		convTR2M(&tr, a);
	} else {
		/*
		 *  subsequent read returns an authenticator
		 */
		if(n != AUTHENTLEN)
			error(Ebadarg);
		cp = c->u.aux;

		cp->a.num = AuthAs;
		memmove(cp->a.chal, cp->t.chal, CHALLEN);
		cp->a.id = 0;
		convA2M(&cp->a, cp->tbuf, cp->t.key);
		memmove(a, cp->tbuf, AUTHENTLEN);

		freecrypt(cp);
		c->u.aux = 0;
	}
	return n;
}

long
authwrite(Chan *c, char *a, int n)
{
	Crypt *cp;

	if(n != TICKETLEN+AUTHENTLEN)
		error(Ebadarg);
	if(c->u.aux == 0)
		error(Ebadarg);
	cp = c->u.aux;

	memmove(cp->tbuf, a, TICKETLEN);
	convM2T(cp->tbuf, &cp->t, evekey);
	if(cp->t.num != AuthTs || memcmp(cp->a.chal, cp->t.chal, CHALLEN))
		error(Eperm);

	memmove(cp->tbuf, a+TICKETLEN, AUTHENTLEN);
	convM2A(cp->tbuf, &cp->a, cp->t.key);
	if(cp->a.num != AuthAc || memcmp(cp->a.chal, cp->t.chal, CHALLEN))
		error(Eperm);

	memmove(up->user, cp->t.suid, NAMELEN);
	return n;
}

/*
 *  called by devcons() for #c/authcheck
 *
 *  a write of a ticket+authenticator [+challenge+id] succeeds if they match
 */
long
authcheck(Chan *c, char *a, int n)
{
	Crypt *cp;
	char *chal;
	ulong id;

	if(n != TICKETLEN+AUTHENTLEN && n != TICKETLEN+AUTHENTLEN+CHALLEN+4)
		error(Ebadarg);
	if(c->u.aux == 0)
		c->u.aux = newcrypt();
	cp = c->u.aux;

	memmove(cp->tbuf, a, TICKETLEN);
	convM2T(cp->tbuf, &cp->t, evekey);
	if(cp->t.num != AuthTc)
		error(Ebadarg);
	if(strcmp(up->user, cp->t.cuid))
		error(cp->t.cuid);

	memmove(cp->tbuf, a+TICKETLEN, AUTHENTLEN);
	convM2A(cp->tbuf, &cp->a, cp->t.key);
	if(n == TICKETLEN+AUTHENTLEN+CHALLEN+4){
		uchar *p = (uchar *)&a[TICKETLEN+AUTHENTLEN+CHALLEN];
		id = p[0] | (p[1]<<8) | (p[2]<<16) | (p[3]<<24);
		chal = &a[TICKETLEN+AUTHENTLEN];
	}else{
		id = 0;
		chal = cp->t.chal;
	}
	if(cp->a.num != AuthAs || memcmp(chal, cp->a.chal, CHALLEN) || cp->a.id != id)
		error(Eperm);

	return n;
}

/*
 *  called by devcons() for #c/authenticator
 *
 *  a read after a write of a ticket (or ticket+id) returns an authenticator
 *  for that ticket.
 */
long
authentwrite(Chan *c, char *a, int n)
{
	Crypt *cp;

	if(n != TICKETLEN && n != TICKETLEN+4)
		error(Ebadarg);
	if(c->u.aux == 0)
		c->u.aux = newcrypt();
	cp = c->u.aux;

	memmove(cp->tbuf, a, TICKETLEN);
	convM2T(cp->tbuf, &cp->t, evekey);
	if(cp->t.num != AuthTc || strcmp(cp->t.cuid, up->user)){
		freecrypt(cp);
		c->u.aux = 0;
		error(Ebadarg);
	}
	if(n == TICKETLEN+4){
		uchar *p = (uchar *)&a[TICKETLEN];
		cp->a.id = p[0] | (p[1]<<8) | (p[2]<<16) | (p[3]<<24);
	}else
		cp->a.id = 0;

	return n;
}

long
authentread(Chan *c, char *a, int n)
{
	Crypt *cp;

	cp = c->u.aux;
	if(cp == 0)
		error("authenticator read must follow a write");

	cp->a.num = AuthAc;
	memmove(cp->a.chal, cp->t.chal, CHALLEN);
	convA2M(&cp->a, cp->tbuf, cp->t.key);
	memmove(a, cp->tbuf, AUTHENTLEN);

	return n;
}

void
authclose(Chan *c)
{
	if(c->u.aux)
		freecrypt(c->u.aux);
	c->u.aux = 0;
}

/*
 *  called by devcons() for key device
 */
long
keyread(char *a, int n, long offset)
{
	if(n<DESKEYLEN || offset != 0)
		error(Ebadarg);
	if(1)
		error(Eperm);
	memmove(a, evekey, DESKEYLEN);
	return DESKEYLEN;
}

long
keywrite(char *a, int n)
{
	if(n != DESKEYLEN)
		error(Ebadarg);
	if(!iseve())
		error(Eperm);
	memmove(evekey, a, DESKEYLEN);
	return DESKEYLEN;
}

/*
 *  called by devcons() for user device
 *
 *  anyone can become none
 */
long
userwrite(char *a, int n)
{
	if(n >= NAMELEN)
		error(Ebadarg);
	if(strcmp(a, "none") != 0)
		error(Eperm);
	memset(up->user, 0, NAMELEN);
	strcpy(up->user, "none");
	return n;
}

/*
 *  called by devcons() for host owner/domain
 *
 *  writing hostowner also sets user
 */
long
hostownerwrite(char *a, int n)
{
	char buf[NAMELEN];

	if(!iseve())
		error(Eperm);
	if(n >= NAMELEN)
		error(Ebadarg);
	memset(buf, 0, NAMELEN);
	strncpy(buf, a, n);
	if(buf[0] == 0)
		error(Ebadarg);
	memmove(eve, buf, NAMELEN);
	memmove(up->user, buf, NAMELEN);
	return n;
}

long
hostdomainwrite(char *a, int n)
{
	char buf[DOMLEN];

	if(!iseve())
		error(Eperm);
	if(n >= DOMLEN)
		error(Ebadarg);
	memset(buf, 0, DOMLEN);
	strncpy(buf, a, n);
	if(buf[0] == 0)
		error(Ebadarg);
	memmove(hostdomain, buf, DOMLEN);
	return n;
}
