#include	"dat.h"
#include	"fns.h"
#include	"error.h"

typedef struct Srv Srv;
struct Srv
{
	Ref;
	int	fd;	/* fd for opened /srv or /srv/X, or -1 */
	int	sfd;	/* fd for created /srv entry or -1 */
	uvlong	path;
	Srv	*next;
};

static QLock	srv9lk;
static Srv	*srv9;
static Srv	*srvroot;

static char*
srvname(Chan *c)
{
	char *p;

	p = strrchr(c->name->s, '/');
	if(p == nil)
		return "";
	return p+1;
}

static Srv*
srvget(uvlong path)
{
	Srv *sv;

	qlock(&srv9lk);
	for(sv = srv9; sv != nil; sv = sv->next)
		if(sv->path == path){
			incref(sv);
			qunlock(&srv9lk);
			return sv;
		}
	sv = smalloc(sizeof(*sv));
	sv->path = path;
	sv->fd = -1;
	sv->sfd = -1;
	sv->ref = 1;
	sv->next = srv9;
	srv9 = sv;
	qunlock(&srv9lk);
	return sv;
}

static void
srvput(Srv *sv)
{
	Srv **l;
	int fd, sfd;

	if(sv != nil && decref(sv) == 0){
		qlock(&srv9lk);
		for(l = &srv9; *l != nil; l = &(*l)->next)
			if(*l == sv){
				*l = sv->next;
				break;
			}
		qunlock(&srv9lk);
		fd = sv->fd;
		sfd = sv->sfd;
		free(sv);
		if(sfd >= 0){
			osenter();
			close(sfd);
			osleave();
		}
		if(fd >= 0){
			osenter();
			close(fd);
			osleave();
		}
	}
}

static void
srv9init(void)
{
	Srv *sv;

	sv = mallocz(sizeof(*srvroot), 1);
	sv->path = 0;
	sv->fd = -1;
	sv->ref = 1;	/* subsequently never reaches zero */
	srvroot = srv9 = sv;
}

static Chan*
srv9attach(char *spec)
{
	Chan *c;

	if(*spec)
		p9error(Ebadspec);
	c = devattach(L'₪', spec);
	if(c != nil){
		incref(srvroot);
		c->aux = srvroot;
	}
	return c;
}

static Walkqid*
srv9walk(Chan *c, Chan *nc, char **name, int nname)
{
	int j, alloc;
	Walkqid *wq;
	char *n;
	Dir *d;

	if(nname > 0)
		isdir(c);

	alloc = 0;
	wq = smalloc(sizeof(Walkqid)+(nname-1)*sizeof(Qid));
	if(waserror()){
		if(alloc)
			cclose(wq->clone);
		free(wq);
		return nil;
	}
	if(nc == nil){
		nc = devclone(c);
		nc->type = 0;	/* device doesn't know about this channel yet */
		alloc = 1;
	}
	wq->clone = nc;

	for(j=0; j<nname; j++){
		if(!(nc->qid.type&QTDIR)){
			if(j==0)
				p9error(Enotdir);
			break;
		}
		n = name[j];
		if(strcmp(n, ".") != 0 && strcmp(n, "..") != 0){
			snprint(up->genbuf, sizeof(up->genbuf), "/srv/%s", n);
			d = dirstat(up->genbuf);
			if(d == nil){
				if(j == 0)
					p9error(Enonexist);
				kstrcpy(up->env->errstr, Enonexist, ERRMAX);
				break;
			}
			nc->qid = d->qid;
			free(d);
		}
		wq->qid[wq->nqid++] = nc->qid;
	}
	poperror();
	if(wq->nqid < nname){
		if(alloc)
			cclose(wq->clone);
		wq->clone = nil;
	}else{
		/* attach cloned channel to device */
		wq->clone->type = c->type;
		if(wq->clone != c)
			nc->aux = srvget(nc->qid.path);
	}
	return wq;
}

static int
srv9stat(Chan *c, uchar *db, int n)
{
	Srv *sv;
	Dir d;

	if(c->qid.type & QTDIR){
		devdir(c, c->qid, "#₪", 0, eve, 0775, &d);
		n = convD2M(&d, db, n);
		if(n == 0)
			p9error(Eshortstat);
		return n;
	}
	sv = c->aux;
	if(sv->fd >= 0){
		osenter();
		n = fstat(sv->fd, db, n);
		osleave();
	}else{
		osenter();
		n = stat(srvname(c), db, n);
		osleave();
	}
	return n;
}

static Chan*
srv9open(Chan *c, int omode)
{
	Srv *sv;
	char *args[10];
	int fd[2], i, ifd, is9p;
	Dir *d;

	sv = c->aux;
	if(c->qid.type == QTDIR){
		osenter();
		sv->fd = open("/srv", omode);
		osleave();
		if(sv->fd < 0)
			oserror();
		c->mode = omode;
		c->flag |= COPEN;
		c->offset = 0;
		return c;
	}

	if(omode&OTRUNC || openmode(omode) != ORDWR)
		p9error(Eperm);
	if(sv->fd < 0){
		snprint(up->genbuf, sizeof(up->genbuf), "/srv/%s", srvname(c));

		/* check permission */
		osenter();
		ifd = open(up->genbuf, omode);
		osleave();
		if(ifd < 0)
			oserror();
		osenter();
		d = dirfstat(ifd);
		is9p = d != nil && d->qid.type & QTMOUNT;
		free(d);
		osleave();

		if(is9p){
			close(ifd);

			/* spawn exportfs */
			args[0] = "exportfs";
			args[1] = "-S";
			args[2] = up->genbuf;
			args[3] = nil;
			if(pipe(fd) < 0)
				oserror();
			/* TO DO: without RFMEM there's a copy made of each page touched by any kproc until the exec */
			switch(rfork(RFPROC|RFNOWAIT|RFREND|RFFDG|RFNAMEG|RFENVG)){	/* no sharing except NOTEG */
			case -1:
				oserrstr(up->genbuf, sizeof(up->genbuf));
				close(fd[0]);
				close(fd[1]);
				p9error(up->genbuf);
			case 0:
				for(i=3; i<MAXNFD; i++)
					if(i != fd[1])
						close(i);
				dup(fd[1], 0);
				if(fd[0] != 0)
					close(fd[0]);
				dup(0, 1);
				exec("/bin/exportfs", args);
				exits("exportfs failed");
			default:
				sv->fd = fd[0];
				close(fd[1]);
				break;
			}
		}else
			sv->fd = ifd;
	}

	c->mode = ORDWR;
	c->offset = 0;
	c->flag |= COPEN;
	return c;
}

static void
srv9close(Chan *c)
{
	srvput(c->aux);
}

static long
srv9read(Chan *c, void *va, long n, vlong off)
{
	Srv *sv;

	sv = c->aux;
	osenter();
	n = pread(sv->fd, va, n, off);
	osleave();
	if(n < 0)
		oserror();
	return n;
}

static long
srv9write(Chan *c, void *va, long n, vlong off)
{
	Srv *sv;

	sv = c->aux;
	osenter();
	n = pwrite(sv->fd, va, n, off);
	osleave();
	if(n == 0)
		p9error(Ehungup);
	if(n < 0)
		oserror();
	return n;
}

static void
srv9create(Chan *c, char *name, int omode, ulong perm)
{
	Srv *sv;
	int sfd, fd[2];
	vlong path;
	Dir *d;

	if(openmode(omode) != ORDWR)
		p9error(Eperm);

	if(pipe(fd) < 0)
		oserror();
	if(waserror()){
		close(fd[0]);
		close(fd[1]);
		nexterror();
	}

	snprint(up->genbuf, sizeof(up->genbuf), "/srv/%s", name);
	osenter();
	sfd = create(up->genbuf, OWRITE|ORCLOSE, perm);
	osleave();
	if(sfd < 0)
		oserror();
	if(waserror()){
		close(sfd);
		nexterror();
	}
	osenter();
	if(fprint(sfd, "%d", fd[1]) < 0){
		osleave();
		oserror();
	}
	d = dirfstat(sfd);
	osleave();
	if(d != nil){
		path = d->qid.path;
		free(d);
	}else
		oserror();

	poperror();
	poperror();
	close(fd[1]);

	if(waserror()){
		close(sfd);
		close(fd[0]);
		nexterror();
	}
	sv = srvget(path);
	sv->fd = fd[0];
	sv->sfd = sfd;
	poperror();

	srvput((Srv*)c->aux);
	c->qid.type = QTFILE;
	c->qid.path = path;
	c->aux = sv;
	c->flag |= COPEN;
	c->mode = ORDWR;
	c->offset = 0;
}

Dev srv9devtab = {
	L'₪',
	"srv9",

	srv9init,	
	srv9attach,
	srv9walk,
	srv9stat,
	srv9open,
	srv9create,	/* TO DO */
	srv9close,
	srv9read,
	devbread,
	srv9write,
	devbwrite,
	devremove,	/* TO DO */
	devwstat,	/* TO DO */
};
