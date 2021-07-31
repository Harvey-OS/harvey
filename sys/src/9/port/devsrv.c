#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	"devtab.h"

typedef struct Srv Srv;
struct Srv
{
	char	name[NAMELEN];
	char	owner[NAMELEN];
	ulong	perm;
	Chan	*chan;
	Srv	*link;
	ulong	path;
};

static QLock	srvlk;
static Srv	*srv;
static int	path;

int
srvgen(Chan *c, Dirtab *tab, int ntab, int s, Dir *dp)
{
	Srv *sp;

	USED(tab);
	USED(ntab);
	qlock(&srvlk);
	for(sp = srv; sp && s; sp = sp->link)
		s--;

	if(sp == 0) {
		qunlock(&srvlk);
		return -1;
	}
	devdir(c, (Qid){sp->path, 0}, sp->name, 0, sp->owner, sp->perm, dp);
	qunlock(&srvlk);
	return 1;
}

void
srvinit(void)
{
	path = 1;
}

void
srvreset(void)
{
}

Chan*
srvattach(char *spec)
{
	return devattach('s', spec);
}

Chan*
srvclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int
srvwalk(Chan *c, char *name)
{
	return devwalk(c, name, 0, 0, srvgen);
}

void
srvstat(Chan *c, char *db)
{
	devstat(c, db, 0, 0, srvgen);
}

Chan*
srvopen(Chan *c, int omode)
{
	Srv *sp;

	if(c->qid.path == CHDIR){
		if(omode != OREAD)
			error(Eisdir);
		c->mode = omode;
		c->flag |= COPEN;
		c->offset = 0;
		return c;
	}
	qlock(&srvlk);
	if(waserror()){
		qunlock(&srvlk);
		nexterror();
	}

	for(sp = srv; sp; sp = sp->link)
		if(sp->path == c->qid.path)
			break;

	if(sp == 0 || sp->chan == 0)
		error(Eshutdown);

	if(omode&OTRUNC)
		error(Eperm);
	if(omode!=sp->chan->mode && sp->chan->mode!=ORDWR)
		error(Eperm);

	close(c);
	incref(sp->chan);
	qunlock(&srvlk);
	poperror();
	return sp->chan;
}

void
srvcreate(Chan *c, char *name, int omode, ulong perm)
{
	Srv *sp;

	if(omode != OWRITE)
		error(Eperm);

	sp = malloc(sizeof(Srv));
	if(sp == 0)
		error(Enomem);

	qlock(&srvlk);
	if(waserror()){
		qunlock(&srvlk);
		nexterror();
	}
	sp->path = path++;
	sp->link = srv;
	c->qid.path = sp->path;
	srv = sp;
	qunlock(&srvlk);
	poperror();

	strncpy(sp->name, name, NAMELEN);
	strncpy(sp->owner, u->p->user, NAMELEN);
	sp->perm = perm&0777;

	c->flag |= COPEN;
	c->mode = OWRITE;
}

void
srvremove(Chan *c)
{
	Srv *sp, **l;

	if(c->qid.path == CHDIR)
		error(Eperm);

	qlock(&srvlk);
	if(waserror()){
		qunlock(&srvlk);
		nexterror();
	}
	l = &srv;
	for(sp = *l; sp; sp = sp->link) {
		if(sp->path == c->qid.path)
			break;

		l = &srv->link;
	}
	if(sp == 0)
		error(Enonexist);

	if(strcmp(sp->name, "boot") == 0)
		error(Eperm);

	*l = sp->link;
	qunlock(&srvlk);
	poperror();

	if(sp->chan)
		close(sp->chan);
	free(sp);
}

void
srvwstat(Chan *c, char *dp)
{
	USED(c, dp);
	error(Egreg);
}

void
srvclose(Chan *c)
{
	USED(c);
}

long
srvread(Chan *c, void *va, long n, ulong offset)
{
	USED(offset);
	isdir(c);
	return devdirread(c, va, n, 0, 0, srvgen);
}

long
srvwrite(Chan *c, void *va, long n, ulong offset)
{
	Srv *sp;
	Chan *c1;
	int fd;
	char buf[32];

	USED(offset);
	if(n >= sizeof buf)
		error(Egreg);
	memmove(buf, va, n);	/* so we can NUL-terminate */
	buf[n] = 0;
	fd = strtoul(buf, 0, 0);

	c1 = fdtochan(fd, -1, 0, 1);	/* error check only */

	qlock(&srvlk);
	if(waserror()) {
		qunlock(&srvlk);
		close(c1);
		nexterror();
	}
	for(sp = srv; sp; sp = sp->link)
		if(sp->path == c->qid.path)
			break;

	if(sp == 0)
		error(Enonexist);

	if(sp->chan)
		panic("srvwrite");

	sp->chan = c1;
	qunlock(&srvlk);
	poperror();
	return n;
}
