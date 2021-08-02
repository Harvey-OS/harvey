#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>

#include "git.h"

enum {
	Qroot,
	Qhead,
	Qbranch,
	Qcommit,
	Qcommitmsg,
	Qcommitparent,
	Qcommittree,
	Qcommitdata,
	Qcommithash,
	Qcommitauthor,
	Qobject,
	Qctl,
	Qmax,
	Internal=1<<7,
};

typedef struct Gitaux Gitaux;
typedef struct Crumb Crumb;
typedef struct Cache Cache;
typedef struct Uqid Uqid;
struct Crumb {
	char	*name;
	Object	*obj;
	Qid	qid;
	int	mode;
	vlong	mtime;
};

struct Gitaux {
	int	ncrumb;
	Crumb	*crumb;
	char	*refpath;
	int	qdir;

	/* For listing object dir */
	Objlist	*ols;
	Object	*olslast;
};

struct Uqid {
	vlong	uqid;

	vlong	ppath;
	vlong	oid;
	int	t;
	int	idx;
};

struct Cache {
	Uqid *cache;
	int n;
	int max;
};

char *qroot[] = {
	"HEAD",
	"branch",
	"object",
	"ctl",
};

#define Eperm	"permission denied";
#define Eexist	"does not exist";
#define E2long	"path too long";
#define Enodir	"not a directory";
#define Erepo	"unable to read repo";
#define Egreg	"wat";
#define Ebadobj	"invalid object";

char	gitdir[512];
char	*username;
char	*groupname;
char	*mntpt = ".git/fs";
char	**branches = nil;
Cache	uqidcache[512];
vlong	nextqid = Qmax;

static Object*	walklink(Gitaux *, char *, int, int, int*);

vlong
qpath(Crumb *p, int idx, vlong id, vlong t)
{
	int h, i;
	vlong pp;
	Cache *c;
	Uqid *u;

	pp = p ? p->qid.path : 0;
	h = (pp*333 + id*7 + t) & (nelem(uqidcache) - 1);
	c = &uqidcache[h];
	u = c->cache;
	for(i=0; i <c->n ; i++){
		if(u->ppath == pp && u->oid == id && u->t == t && u->idx == idx)
			return (u->uqid << 8) | t;
		u++;
	}
	if(c->n == c->max){
		c->max += c->max/2 + 1;
		c->cache = erealloc(c->cache, c->max*sizeof(Uqid));
	}
	nextqid++;
	c->cache[c->n] = (Uqid){nextqid, pp, id, t, idx};
	c->n++;
	return (nextqid << 8) | t;
}

static Crumb*
crumb(Gitaux *aux, int n)
{
	if(n < aux->ncrumb)
		return &aux->crumb[aux->ncrumb - n - 1];
	return nil;
}

static void
popcrumb(Gitaux *aux)
{
	Crumb *c;

	if(aux->ncrumb > 1){
		c = crumb(aux, 0);
		free(c->name);
		unref(c->obj);
		aux->ncrumb--;
	}
}

static vlong
branchid(Gitaux *aux, char *path)
{
	int i;

	for(i = 0; branches[i]; i++)
		if(strcmp(path, branches[i]) == 0)
			goto found;
	branches = realloc(branches, sizeof(char *)*(i + 2));
	branches[i] = estrdup(path);
	branches[i + 1] = nil;

found:
	if(aux){
		if(aux->refpath)
			free(aux->refpath);
		aux->refpath = estrdup(branches[i]);
	}
	return i;
}

static void
obj2dir(Dir *d, Crumb *c, Object *o, char *name)
{
	d->qid = c->qid;
	d->atime = c->mtime;
	d->mtime = c->mtime;
	d->mode = c->mode;
	d->name = estrdup9p(name);
	d->uid = estrdup9p(username);
	d->gid = estrdup9p(groupname);
	d->muid = estrdup9p(username);
	if(o->type == GBlob || o->type == GTag){
		d->qid.type = 0;
		d->mode &= 0777;
		d->length = o->size;
	}

}

static int
rootgen(int i, Dir *d, void *p)
{
	Crumb *c;

	c = crumb(p, 0);
	if (i >= nelem(qroot))
		return -1;
	d->mode = 0555 | DMDIR;
	d->name = estrdup9p(qroot[i]);
	d->qid.vers = 0;
	d->qid.type = strcmp(qroot[i], "ctl") == 0 ? 0 : QTDIR;
	d->qid.path = qpath(nil, i, i, Qroot);
	d->uid = estrdup9p(username);
	d->gid = estrdup9p(groupname);
	d->muid = estrdup9p(username);
	d->mtime = c->mtime;
	return 0;
}

static int
branchgen(int i, Dir *d, void *p)
{
	Gitaux *aux;
	Dir *refs;
	Crumb *c;
	int n;

	aux = p;
	c = crumb(aux, 0);
	refs = nil;
	d->qid.vers = 0;
	d->qid.type = QTDIR;
	d->qid.path = qpath(c, i, branchid(aux, aux->refpath), Qbranch | Internal);
	d->mode = 0555 | DMDIR;
	d->uid = estrdup9p(username);
	d->gid = estrdup9p(groupname);
	d->muid = estrdup9p(username);
	d->mtime = c->mtime;
	d->atime = c->mtime;
	if((n = slurpdir(aux->refpath, &refs)) < 0)
		return -1;
	if(i < n){
		d->name = estrdup9p(refs[i].name);
		free(refs);
		return 0;
	}else{
		free(refs);
		return -1;
	}
}

static int
gtreegen(int i, Dir *d, void *p)
{
	Object *o, *l, *e;
	Gitaux *aux;
	Crumb *c;
	int m;

	aux = p;
	c = crumb(aux, 0);
	e = c->obj;
	if(i >= e->tree->nent)
		return -1;
	m = e->tree->ent[i].mode;
	if(e->tree->ent[i].ismod)
		o = emptydir();
	else if((o = readobject(e->tree->ent[i].h)) == nil)
		sysfatal("could not read object %H: %r", e->tree->ent[i].h);
	if(e->tree->ent[i].islink)
		if((l = walklink(aux, o->data, o->size, 0, &m)) != nil)
			o = l;
	d->qid.vers = 0;
	d->qid.type = o->type == GTree ? QTDIR : 0;
	d->qid.path = qpath(c, i, o->id, aux->qdir);
	d->mode = m;
	d->atime = c->mtime;
	d->mtime = c->mtime;
	d->uid = estrdup9p(username);
	d->gid = estrdup9p(groupname);
	d->muid = estrdup9p(username);
	d->name = estrdup9p(e->tree->ent[i].name);
	d->length = o->size;
	return 0;
}

static int
gcommitgen(int i, Dir *d, void *p)
{
	Object *o;
	Crumb *c;

	c = crumb(p, 0);
	o = c->obj;
	d->uid = estrdup9p(username);
	d->gid = estrdup9p(groupname);
	d->muid = estrdup9p(username);
	d->mode = 0444;
	d->atime = o->commit->ctime;
	d->mtime = o->commit->ctime;
	d->qid.type = 0;
	d->qid.vers = 0;

	switch(i){
	case 0:
		d->mode = 0755 | DMDIR;
		d->name = estrdup9p("tree");
		d->qid.type = QTDIR;
		d->qid.path = qpath(c, i, o->id, Qcommittree);
		break;
	case 1:
		d->name = estrdup9p("parent");
		d->qid.path = qpath(c, i, o->id, Qcommitparent);
		break;
	case 2:
		d->name = estrdup9p("msg");
		d->qid.path = qpath(c, i, o->id, Qcommitmsg);
		break;
	case 3:
		d->name = estrdup9p("hash");
		d->qid.path = qpath(c, i, o->id, Qcommithash);
		break;
	case 4:
		d->name = estrdup9p("author");
		d->qid.path = qpath(c, i, o->id, Qcommitauthor);
		break;
	default:
		return -1;
	}
	return 0;
}


static int
objgen(int i, Dir *d, void *p)
{
	Gitaux *aux;
	Object *o;
	Crumb *c;
	char name[64];
	Objlist *ols;
	Hash h;

	aux = p;
	c = crumb(aux, 0);
	if(!aux->ols)
		aux->ols = mkols();
	ols = aux->ols;
	o = nil;
	/* We tried to sent it, but it didn't fit */
	if(aux->olslast && ols->idx == i + 1){
		snprint(name, sizeof(name), "%H", aux->olslast->hash);
		obj2dir(d, c, aux->olslast, name);
		return 0;
	}
	while(ols->idx <= i){
		if(olsnext(ols, &h) == -1)
			return -1;
		if((o = readobject(h)) == nil){
			fprint(2, "corrupt object %H\n", h);
			return -1;
		}
	}
	if(o != nil){
		snprint(name, sizeof(name), "%H", o->hash);
		obj2dir(d, c, o, name);
		unref(aux->olslast);
		aux->olslast = ref(o);
		return 0;
	}
	return -1;
}

static void
objread(Req *r, Gitaux *aux)
{
	Object *o;

	o = crumb(aux, 0)->obj;
	switch(o->type){
	case GBlob:
		readbuf(r, o->data, o->size);
		break;
	case GTag:
		readbuf(r, o->data, o->size);
		break;
	case GTree:
		dirread9p(r, gtreegen, aux);
		break;
	case GCommit:
		dirread9p(r, gcommitgen, aux);
		break;
	default:
		sysfatal("invalid object type %d", o->type);
	}
}

static void
readcommitparent(Req *r, Object *o)
{
	char *buf, *p;
	int i, n;

	n = o->commit->nparent * (40 + 2);
	buf = emalloc(n);
	p = buf;
	for (i = 0; i < o->commit->nparent; i++)
		p += sprint(p, "%H\n", o->commit->parent[i]);
	readbuf(r, buf, n);
	free(buf);
}


static void
gitattach(Req *r)
{
	Gitaux *aux;
	Dir *d;

	if((d = dirstat(".git")) == nil)
		sysfatal("git/fs: %r");
	if(getwd(gitdir, sizeof(gitdir)) == nil)
		sysfatal("getwd: %r");
	aux = emalloc(sizeof(Gitaux));
	aux->crumb = emalloc(sizeof(Crumb));
	aux->crumb[0].qid = (Qid){Qroot, 0, QTDIR};
	aux->crumb[0].obj = nil;
	aux->crumb[0].mode = DMDIR | 0555;
	aux->crumb[0].mtime = d->mtime;
	aux->crumb[0].name = estrdup("/");
	aux->ncrumb = 1;
	r->ofcall.qid = (Qid){Qroot, 0, QTDIR};
	r->fid->qid = r->ofcall.qid;
	r->fid->aux = aux;
	respond(r, nil);
}

static Object*
walklink(Gitaux *aux, char *link, int nlink, int ndotdot, int *mode)
{
	char *p, *e, *path;
	Object *o, *n;
	int i;

	path = emalloc(nlink + 1);
	memcpy(path, link, nlink);
	cleanname(path);

	o = crumb(aux, ndotdot)->obj;
	assert(o->type == GTree);
	for(p = path; *p; p = e){
		n = nil;
		e = p + strcspn(p, "/");
		if(*e == '/')
			*e++ = '\0';
		/*
		 * cleanname guarantees these show up at the start of the name,
		 * which allows trimming them from the end of the trail of crumbs
		 * instead of needing to keep track of full parentage.
		 */
		if(strcmp(p, "..") == 0)
			n = crumb(aux, ++ndotdot)->obj;
		else if(o->type == GTree)
			for(i = 0; i < o->tree->nent; i++)
				if(strcmp(o->tree->ent[i].name, p) == 0){
					*mode = o->tree->ent[i].mode;
					n = readobject(o->tree->ent[i].h);
					break;
				}
		o = n;
		if(o == nil)
			break;
	}
	free(path);
	return o;
}

static char *
objwalk1(Qid *q, Object *o, Crumb *p, Crumb *c, char *name, vlong qdir, Gitaux *aux)
{
	Object *w, *l;
	char *e;
	int i, m;

	w = nil;
	e = nil;
	if(!o)
		return Eexist;
	if(o->type == GTree){
		q->type = 0;
		for(i = 0; i < o->tree->nent; i++){
			if(strcmp(o->tree->ent[i].name, name) != 0)
				continue;
			m = o->tree->ent[i].mode;
			w = readobject(o->tree->ent[i].h);
			if(!w && o->tree->ent[i].ismod)
				w = emptydir();
			if(w && o->tree->ent[i].islink)
				if((l = walklink(aux, w->data, w->size, 1, &m)) != nil)
					w = l;
			if(!w)
				return Ebadobj;
			q->type = (w->type == GTree) ? QTDIR : 0;
			q->path = qpath(c, i, w->id, qdir);
			c->mode = m;
			c->mode |= (w->type == GTree) ? DMDIR|0755 : 0644;
			c->obj = w;
			break;
		}
		if(!w)
			e = Eexist;
	}else if(o->type == GCommit){
		q->type = 0;
		c->mtime = o->commit->mtime;
		c->mode = 0644;
		assert(qdir == Qcommit || qdir == Qobject || qdir == Qcommittree || qdir == Qhead);
		if(strcmp(name, "msg") == 0)
			q->path = qpath(p, 0, o->id, Qcommitmsg);
		else if(strcmp(name, "parent") == 0)
			q->path = qpath(p, 1, o->id, Qcommitparent);
		else if(strcmp(name, "hash") == 0)
			q->path = qpath(p, 2, o->id, Qcommithash);
		else if(strcmp(name, "author") == 0)
			q->path = qpath(p, 3, o->id, Qcommitauthor);
		else if(strcmp(name, "tree") == 0){
			q->type = QTDIR;
			q->path = qpath(p, 4, o->id, Qcommittree);
			unref(c->obj);
			c->mode = DMDIR | 0755;
			c->obj = readobject(o->commit->tree);
			if(c->obj == nil)
				sysfatal("could not read object %H: %r", o->commit->tree);
		}
		else
			e = Eexist;
	}else if(o->type == GTag){
		e = "tag walk unimplemented";
	}
	return e;
}

static Object *
readref(char *pathstr)
{
	char buf[128], path[128], *p, *e;
	Hash h;
	int n, f;

	snprint(path, sizeof(path), "%s", pathstr);
	while(1){
		if((f = open(path, OREAD)) == -1)
			return nil;
		if((n = readn(f, buf, sizeof(buf) - 1)) == -1)
			return nil;
		close(f);
		buf[n] = 0;
		if(strncmp(buf, "ref:", 4) !=  0)
			break;

		p = buf + 4;
		while(isspace(*p))
			p++;
		if((e = strchr(p, '\n')) != nil)
			*e = 0;
		snprint(path, sizeof(path), ".git/%s", p);
	}

	if(hparse(&h, buf) == -1)
		return nil;

	return readobject(h);
}

static char*
gitwalk1(Fid *fid, char *name, Qid *q)
{
	char path[128];
	Gitaux *aux;
	Crumb *c, *o;
	char *e;
	Dir *d;
	Hash h;

	e = nil;
	aux = fid->aux;
	
	q->vers = 0;
	if(strcmp(name, "..") == 0){
		popcrumb(aux);
		c = crumb(aux, 0);
		*q = c->qid;
		fid->qid = *q;
		return nil;
	}
	
	aux->crumb = realloc(aux->crumb, (aux->ncrumb + 1) * sizeof(Crumb));
	aux->ncrumb++;
	c = crumb(aux, 0);
	o = crumb(aux, 1);
	memset(c, 0, sizeof(Crumb));
	c->mode = o->mode;
	c->mtime = o->mtime;
		c->obj = o->obj ? ref(o->obj) : nil;
	
	switch(QDIR(&fid->qid)){
	case Qroot:
		if(strcmp(name, "HEAD") == 0){
			*q = (Qid){Qhead, 0, QTDIR};
			c->mode = DMDIR | 0555;
			c->obj = readref(".git/HEAD");
		}else if(strcmp(name, "object") == 0){
			*q = (Qid){Qobject, 0, QTDIR};
			c->mode = DMDIR | 0555;
		}else if(strcmp(name, "branch") == 0){
			*q = (Qid){Qbranch, 0, QTDIR};
			aux->refpath = estrdup(".git/refs/");
			c->mode = DMDIR | 0555;
		}else if(strcmp(name, "ctl") == 0){
			*q = (Qid){Qctl, 0, 0};
			c->mode = 0644;
		}else{
			e = Eexist;
		}
		break;
	case Qbranch:
		if(strcmp(aux->refpath, ".git/refs/heads") == 0 && strcmp(name, "HEAD") == 0)
			snprint(path, sizeof(path), ".git/HEAD");
		else
			snprint(path, sizeof(path), "%s/%s", aux->refpath, name);
		q->type = QTDIR;
		d = dirstat(path);
		if(d && d->qid.type == QTDIR)
			q->path = qpath(o, Qbranch, branchid(aux, path), Qbranch);
		else if(d && (c->obj = readref(path)) != nil)
			q->path = qpath(o, Qbranch, c->obj->id, Qcommit);
		else
			e = Eexist;
		free(d);
		break;
	case Qobject:
		if(c->obj){
			e = objwalk1(q, o->obj, o, c, name, Qobject, aux);
		}else{
			if(hparse(&h, name) == -1)
				return "invalid object name";
			if((c->obj = readobject(h)) == nil)
				return "could not read object";
			if(c->obj->type == GBlob || c->obj->type == GTag){
				c->mode = 0644;
				q->type = 0;
			}else{
				c->mode = DMDIR | 0755;
				q->type = QTDIR;
			}
			q->path = qpath(o, Qobject, c->obj->id, Qobject);
			q->vers = 0;
		}
		break;
	case Qhead:
		e = objwalk1(q, o->obj, o, c, name, Qhead, aux);
		break;
	case Qcommit:
		e = objwalk1(q, o->obj, o, c, name, Qcommit, aux);
		break;
	case Qcommittree:
		e = objwalk1(q, o->obj, o, c, name, Qcommittree, aux);
		break;
	case Qcommitparent:
	case Qcommitmsg:
	case Qcommitdata:
	case Qcommithash:
	case Qcommitauthor:
	case Qctl:
		return Enodir;
	default:
		return Egreg;
	}

	c->name = estrdup(name);
	c->qid = *q;
	fid->qid = *q;
	return e;
}

static char*
gitclone(Fid *o, Fid *n)
{
	Gitaux *aux, *oaux;
	int i;

	oaux = o->aux;
	aux = emalloc(sizeof(Gitaux));
	aux->ncrumb = oaux->ncrumb;
	aux->crumb = eamalloc(oaux->ncrumb, sizeof(Crumb));
	for(i = 0; i < aux->ncrumb; i++){
		aux->crumb[i] = oaux->crumb[i];
		aux->crumb[i].name = estrdup(oaux->crumb[i].name);
		if(aux->crumb[i].obj)
			aux->crumb[i].obj = ref(oaux->crumb[i].obj);
	}
	if(oaux->refpath)
		aux->refpath = strdup(oaux->refpath);
	aux->qdir = oaux->qdir;
	n->aux = aux;
	return nil;
}

static void
gitdestroyfid(Fid *f)
{
	Gitaux *aux;
	int i;

	if((aux = f->aux) == nil)
		return;
	for(i = 0; i < aux->ncrumb; i++){
		if(aux->crumb[i].obj)
			unref(aux->crumb[i].obj);
		free(aux->crumb[i].name);
	}
	olsfree(aux->ols);
	free(aux->refpath);
	free(aux->crumb);
	free(aux);
}

static char *
readctl(Req *r)
{
	char data[1024], ref[512], *s, *e;
	int fd, n;

	if((fd = open(".git/HEAD", OREAD)) == -1)
		return Erepo;
	/* empty HEAD is invalid */
	if((n = readn(fd, ref, sizeof(ref) - 1)) <= 0)
		return Erepo;
	close(fd);

	s = ref;
	ref[n] = 0;
	if(strncmp(s, "ref:", 4) == 0)
		s += 4;
	while(*s == ' ' || *s == '\t')
		s++;
	if((e = strchr(s, '\n')) != nil)
		*e = 0;
	if(strstr(s, "refs/") == s)
		s += strlen("refs/");

	snprint(data, sizeof(data), "branch %s\nrepo %s\n", s, gitdir);
	readstr(r, data);
	return nil;
}

static void
gitread(Req *r)
{
	char buf[256], *e;
	Gitaux *aux;
	Object *o;
	Qid *q;

	aux = r->fid->aux;
	q = &r->fid->qid;
	o = crumb(aux, 0)->obj;
	e = nil;

	switch(QDIR(q)){
	case Qroot:
		dirread9p(r, rootgen, aux);
		break;
	case Qbranch:
		if(o)
			objread(r, aux);
		else
			dirread9p(r, branchgen, aux);
		break;
	case Qobject:
		if(o)
			objread(r, aux);
		else
			dirread9p(r, objgen, aux);
		break;
	case Qcommitmsg:
		readbuf(r, o->commit->msg, o->commit->nmsg);
		break;
	case Qcommitparent:
		readcommitparent(r, o);
		break;
	case Qcommithash:
		snprint(buf, sizeof(buf), "%H\n", o->hash);
		readstr(r, buf);
		break;
	case Qcommitauthor:
		snprint(buf, sizeof(buf), "%s\n", o->commit->author);
		readstr(r, buf);
		break;
	case Qctl:
		e = readctl(r);
		break;
	case Qhead:
		/* Empty repositories have no HEAD */
		if(o == nil)
			r->ofcall.count = 0;
		else
			objread(r, aux);
		break;
	case Qcommit:
	case Qcommittree:
	case Qcommitdata:
		objread(r, aux);
		break;
	default:
		e = Egreg;
	}
	respond(r, e);
}

static void
gitstat(Req *r)
{
	Gitaux *aux;
	Crumb *c;

	aux = r->fid->aux;
	c = crumb(aux, 0);
	r->d.uid = estrdup9p(username);
	r->d.gid = estrdup9p(groupname);
	r->d.muid = estrdup9p(username);
	r->d.qid = r->fid->qid;
	r->d.mtime = c->mtime;
	r->d.atime = c->mtime;
	r->d.mode = c->mode;
	if(c->obj)
		obj2dir(&r->d, c, c->obj, c->name);
	else
		r->d.name = estrdup9p(c->name);
	respond(r, nil);
}

Srv gitsrv = {
	.attach=gitattach,
	.walk1=gitwalk1,
	.clone=gitclone,
	.read=gitread,
	.stat=gitstat,
	.destroyfid=gitdestroyfid,
};

void
usage(void)
{
	fprint(2, "usage: %s [-d]\n", argv0);
	fprint(2, "\t-d:	debug\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	Dir *d;

	gitinit();
	ARGBEGIN{
	case 'd':
		chatty9p++;
		break;
	case 'm':
		mntpt = EARGF(usage());
		break;
	default:
		usage();
		break;
	}ARGEND;
	if(argc != 0)
		usage();

	if((d = dirstat(".git")) == nil)
		sysfatal("dirstat .git: %r");
	username = strdup(d->uid);
	groupname = strdup(d->gid);
	free(d);

	branches = emalloc(sizeof(char*));
	branches[0] = nil;
	postmountsrv(&gitsrv, nil, mntpt, MCREATE);
	exits(nil);
}
