#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"


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
static int	qidpath;

static int
srvgen(Chan *c, Dirtab*, int, int s, Dir *dp)
{
	Srv *sp;

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
	devdir(c, (Qid){sp->path, 0}, sp->name, 0, sp->owner, sp->perm, dp);
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

static int
srvwalk(Chan *c, char *name)
{
	return devwalk(c, name, 0, 0, srvgen);
}

static void
srvstat(Chan *c, char *db)
{
	devstat(c, db, 0, 0, srvgen);
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
	Srv *sp;

	if(c->qid.path == CHDIR){
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
	Srv *sp;

	if(openmode(omode) != OWRITE)
		error(Eperm);

	if(omode & OCEXEC)	/* can't happen */
		panic("someone broke namec");

	sp = malloc(sizeof(Srv));
	if(sp == 0)
		error(Enomem);

	qlock(&srvlk);
	if(waserror()){
		qunlock(&srvlk);
		nexterror();
	}
	if(srvlookup(name, -1))
		error(Eexist);

	sp->path = qidpath++;
	sp->link = srv;
	c->qid.path = sp->path;
	srv = sp;
	qunlock(&srvlk);
	poperror();

	strncpy(sp->name, name, NAMELEN);
	strncpy(sp->owner, up->user, NAMELEN);
	sp->perm = perm&0777;

	c->flag |= COPEN;
	c->mode = OWRITE;
}

static void
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

		l = &sp->link;
	}
	if(sp == 0)
		error(Enonexist);

	if(strcmp(sp->name, "boot") == 0)
		error(Eperm);

	*l = sp->link;
	qunlock(&srvlk);
	poperror();

	if(sp->chan)
		cclose(sp->chan);
	free(sp);
}

static void
srvwstat(Chan *c, char *dp)
{
	Dir d;
	Srv *sp;

	if(c->qid.path & CHDIR)
		error(Eperm);

	qlock(&srvlk);
	if(waserror()){
		qunlock(&srvlk);
		nexterror();
	}

	sp = srvlookup(nil, c->qid.path);
	if(sp == 0)
		error(Enonexist);

	if(strcmp(sp->owner, up->user) && !iseve())
		error(Eperm);

	convM2D(dp, &d);
	d.mode &= 0777;
	sp->perm = d.mode;

	qunlock(&srvlk);
	poperror();
}

static void
srvclose(Chan *c)
{
	/*
	 * errors from srvremove will be caught by cclose and ignored.
	 * in theory we need to override any changes in removability
	 * since open, but since all that's checked is the owner,
	 * which is immutable, all is well.
	 */
	if(c->flag & CRCLOSE)
		srvremove(c);
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
	srvattach,
	devclone,
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
