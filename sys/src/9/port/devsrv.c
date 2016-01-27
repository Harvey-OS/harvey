/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	<error.h>


typedef struct Srv Srv;
struct Srv
{
	char	*name;
	char	*owner;
	uint32_t	perm;
	Chan	*chan;
	Srv	*link;
	uint32_t	path;
};

static QLock	srvlk;
static Srv	*srv;
static int	qidpath;

static int
srvgen(Chan *c, char* d, Dirtab* dir, int i, int s, Dir *dp)
{
	Proc *up = externup();
	Srv *sp;
	Qid q;

	if(s == DEVDOTDOT){
		devdir(c, c->qid, "#s", 0, eve, 0555, dp);
		return 1;
	}

	for(sp = srv; sp && s; sp = sp->link)
		s--;

	if(sp == 0) {
		return -1;
	}

	mkqid(&q, sp->path, 0, QTFILE);
	/* make sure name string continues to exist after we release lock */
	kstrcpy(up->genbuf, sp->name, sizeof up->genbuf);
	devdir(c, q, up->genbuf, 0, sp->owner, sp->perm, dp);
	return 1;
}

static void
srvinit(void)
{
	qidpath = 1;
}

static Chan*
srvattach(char *spec)
{
	return devattach('s', spec);
}

static Walkqid*
srvwalk(Chan *c, Chan *nc, char **name, int nname)
{
	Walkqid *wqid;

	qlock(&srvlk);
	wqid = devwalk(c, nc, name, nname, 0, 0, srvgen);
	qunlock(&srvlk);
	return wqid;
}

static Srv*
srvlookup(char *name, uint32_t qidpath)
{
	Srv *sp;
	for(sp = srv; sp; sp = sp->link)
		if(sp->path == qidpath || (name && strcmp(sp->name, name) == 0))
			return sp;
	return nil;
}

static int32_t
srvstat(Chan *c, uint8_t *db, int32_t n)
{
	int32_t r;

	qlock(&srvlk);
	r = devstat(c, db, n, 0, 0, srvgen);
	qunlock(&srvlk);
	return r;
}

char*
srvname(Chan *c)
{
	Srv *sp;
	char *s;

	for(sp = srv; sp; sp = sp->link)
		if(sp->chan == c){
			s = smalloc(3+strlen(sp->name)+1);
			sprint(s, "#s/%s", sp->name);
			return s;
		}
	return nil;
}

static Chan*
srvopen(Chan *c, int omode)
{
	Proc *up = externup();
	Srv *sp;

	if(c->qid.type == QTDIR){
		if(omode & ORCLOSE)
			error(Eperm);
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

	sp = srvlookup(nil, c->qid.path);
	if(sp == 0 || sp->chan == 0)
		error(Eshutdown);

	if(omode&OTRUNC)
		error("srv file already exists");
	if(openmode(omode)!=sp->chan->mode && sp->chan->mode!=ORDWR)
		error(Eperm);
	devpermcheck(sp->owner, sp->perm, omode);

	cclose(c);
	incref(&sp->chan->r);
	qunlock(&srvlk);
	poperror();
	return sp->chan;
}

static void
srvcreate(Chan *c, char *name, int omode, int perm)
{
	Proc *up = externup();
	Srv *sp;
	char *sname;

	if(openmode(omode) != OWRITE)
		error(Eperm);

	if(omode & OCEXEC)	/* can't happen */
		panic("someone broke namec");

	sp = smalloc(sizeof *sp);
	sname = smalloc(strlen(name)+1);

	qlock(&srvlk);
	if(waserror()){
		free(sname);
		free(sp);
		qunlock(&srvlk);
		nexterror();
	}
	if(sp == nil || sname == nil)
		error(Enomem);
	if(srvlookup(name, -1))
		error(Eexist);

	sp->path = qidpath++;
	sp->link = srv;
	strcpy(sname, name);
	sp->name = sname;
	c->qid.type = QTFILE;
	c->qid.path = sp->path;
	srv = sp;
	qunlock(&srvlk);
	poperror();

	kstrdup(&sp->owner, up->user);
	sp->perm = perm&0777;

	c->flag |= COPEN;
	c->mode = OWRITE;
}

static void
srvremove(Chan *c)
{
	Proc *up = externup();
	Srv *sp, **l;

	if(c->qid.type == QTDIR)
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

		l = &sp->link;
	}
	if(sp == 0)
		error(Enonexist);

	/*
	 * Only eve can remove system services.
	 * No one can remove #s/boot.
	 */
	if(strcmp(sp->owner, eve) == 0 && !iseve())
		error(Eperm);
	if(strcmp(sp->name, "boot") == 0)
		error(Eperm);

	/*
	 * No removing personal services.
	 */
	if((sp->perm&7) != 7 && strcmp(sp->owner, up->user) && !iseve())
		error(Eperm);

	*l = sp->link;
	qunlock(&srvlk);
	poperror();

	if(sp->chan)
		cclose(sp->chan);
	free(sp->owner);
	free(sp->name);
	free(sp);
}

static int32_t
srvwstat(Chan *c, uint8_t *dp, int32_t n)
{
	Proc *up = externup();
	Dir d;
	Srv *sp;
	char *strs;

	if(c->qid.type & QTDIR)
		error(Eperm);

	strs = nil;
	qlock(&srvlk);
	if(waserror()){
		qunlock(&srvlk);
		free(strs);
		nexterror();
	}

	sp = srvlookup(nil, c->qid.path);
	if(sp == 0)
		error(Enonexist);

	if(strcmp(sp->owner, up->user) != 0 && !iseve())
		error(Eperm);

	strs = smalloc(n);
	n = convM2D(dp, n, &d, strs);
	if(n == 0)
		error(Eshortstat);
	if(d.mode != (uint32_t)~0UL)
		sp->perm = d.mode & 0777;
	if(d.uid && *d.uid)
		kstrdup(&sp->owner, d.uid);
	if(d.name && *d.name && strcmp(sp->name, d.name) != 0) {
		if(strchr(d.name, '/') != nil)
			error(Ebadchar);
		kstrdup(&sp->name, d.name);
	}

	qunlock(&srvlk);
	free(strs);
	poperror();
	return n;
}

static void
srvclose(Chan *c)
{
	Proc *up = externup();
	/*
	 * in theory we need to override any changes in removability
	 * since open, but since all that's checked is the owner,
	 * which is immutable, all is well.
	 */
	if(c->flag & CRCLOSE){
		if(waserror())
			return;
		srvremove(c);
		poperror();
	}
}

static int32_t
srvread(Chan *c, void *va, int32_t n, int64_t m)
{
	int32_t r;

	isdir(c);
	qlock(&srvlk);
	r = devdirread(c, va, n, 0, 0, srvgen);
	qunlock(&srvlk);
	return r;
}

static int32_t
srvwrite(Chan *c, void *va, int32_t n, int64_t mm)
{
	Proc *up = externup();
	Srv *sp;
	Chan *c1;
	int fd;
	char buf[32];

	if(n >= sizeof buf)
		error(Egreg);
	memmove(buf, va, n);	/* so we can NUL-terminate */
	buf[n] = 0;
	fd = strtoul(buf, 0, 0);

	c1 = fdtochan(fd, -1, 0, 1);	/* error check and inc ref */

	qlock(&srvlk);
	if(waserror()) {
		qunlock(&srvlk);
		cclose(c1);
		nexterror();
	}
	if(c1->flag & (CCEXEC|CRCLOSE))
		error("posted fd has remove-on-close or close-on-exec");
	if(c1->qid.type & QTAUTH)
		error("cannot post auth file in srv");
	sp = srvlookup(nil, c->qid.path);
	if(sp == 0)
		error(Enonexist);

	if(sp->chan)
		error(Ebadusefd);

	sp->chan = c1;
	qunlock(&srvlk);
	poperror();
	return n;
}

Dev srvdevtab = {
	's',
	"srv",

	devreset,
	srvinit,
	devshutdown,
	srvattach,
	srvwalk,
	srvstat,
	srvopen,
	srvcreate,
	srvclose,
	srvread,
	devbread,
	srvwrite,
	devbwrite,
	srvremove,
	srvwstat,
};
