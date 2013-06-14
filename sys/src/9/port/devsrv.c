#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"


typedef struct Srv Srv;
struct Srv
{
	char	*name;
	char	*owner;
	ulong	perm;
	Chan	*chan;
	Srv	*link;
	ulong	path;
};

static QLock	srvlk;
static Srv	*srv;
static int	qidpath;

static int
srvgen(Chan *c, char*, Dirtab*, int, int s, Dir *dp)
{
	Srv *sp;
	Qid q;

	if(s == DEVDOTDOT){
		devdir(c, c->qid, "#s", 0, eve, 0555, dp);
		return 1;
	}

	qlock(&srvlk);
	for(sp = srv; sp && s; sp = sp->link)
		s--;

	if(sp == 0) {
		qunlock(&srvlk);
		return -1;
	}

	mkqid(&q, sp->path, 0, QTFILE);
	/* make sure name string continues to exist after we release lock */
	kstrcpy(up->genbuf, sp->name, sizeof up->genbuf);
	devdir(c, q, up->genbuf, 0, sp->owner, sp->perm, dp);
	qunlock(&srvlk);
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
	return devwalk(c, nc, name, nname, 0, 0, srvgen);
}

static Srv*
srvlookup(char *name, ulong qidpath)
{
	Srv *sp;
	for(sp = srv; sp; sp = sp->link)
		if(sp->path == qidpath || (name && strcmp(sp->name, name) == 0))
			return sp;
	return nil;
}

static int
srvstat(Chan *c, uchar *db, int n)
{
	return devstat(c, db, n, 0, 0, srvgen);
}

char*
srvname(Chan *c)
{
	int size;
	Srv *sp;
	char *s;

	for(sp = srv; sp; sp = sp->link)
		if(sp->chan == c){
			size = 3+strlen(sp->name)+1;
			s = smalloc(size);
			snprint(s, size, "#s/%s", sp->name);
			return s;
		}
	return nil;
}

static Chan*
srvopen(Chan *c, int omode)
{
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
	incref(sp->chan);
	qunlock(&srvlk);
	poperror();
	return sp->chan;
}

static void
srvcreate(Chan *c, char *name, int omode, ulong perm)
{
	char *sname;
	Srv *sp;

	if(openmode(omode) != OWRITE)
		error(Eperm);

	if(omode & OCEXEC)	/* can't happen */
		panic("someone broke namec");

	sp = smalloc(sizeof *sp);
	sname = smalloc(strlen(name)+1);

	qlock(&srvlk);
	if(waserror()){
		free(sp);
		free(sname);
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

static int
srvwstat(Chan *c, uchar *dp, int n)
{
	char *strs;
	Dir d;
	Srv *sp;

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
	if(d.mode != ~0UL)
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

static long
srvread(Chan *c, void *va, long n, vlong)
{
	isdir(c);
	return devdirread(c, va, n, 0, 0, srvgen);
}

static long
srvwrite(Chan *c, void *va, long n, vlong)
{
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
