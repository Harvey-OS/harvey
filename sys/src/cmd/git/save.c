#include <u.h>
#include <libc.h>
#include "git.h"

typedef struct Objbuf Objbuf;
struct Objbuf {
	int off;
	char *hdr;
	int nhdr;
	char *dat;
	int ndat;
};
enum {
	Maxparents = 16,
};

int
gitmode(int m)
{
	if(m & DMDIR)		/* directory */
		return 0040000;
	else if(m & 0111)	/* executable */
		return 0100755;
	else if(m != 0)		/* regular */
		return 0100644;
	else			/* symlink */
		return 0120000;
}

int
entcmp(void *pa, void *pb)
{
	char abuf[256], bbuf[256], *ae, *be;
	Dirent *a, *b;

	a = pa;
	b = pb;
	/*
	 * If the files have the same name, they're equal.
	 * Otherwise, If they're trees, they sort as thoug
	 * there was a trailing slash.
	 *
	 * Wat.
	 */
	if(strcmp(a->name, b->name) == 0)
		return 0;

	ae = seprint(abuf, abuf + sizeof(abuf) - 1, a->name);
	be = seprint(bbuf, bbuf + sizeof(bbuf) - 1, b->name);
	if(a->mode & DMDIR)
		*ae = '/';
	if(b->mode & DMDIR)
		*be = '/';
	return strcmp(abuf, bbuf);
}

static int
bwrite(void *p, void *buf, int nbuf)
{
	return Bwrite(p, buf, nbuf);
}

static int
objbytes(void *p, void *buf, int nbuf)
{
	Objbuf *b;
	int r, n, o;
	char *s;

	b = p;
	n = 0;
	if(b->off < b->nhdr){
		r = b->nhdr - b->off;
		r = (nbuf < r) ? nbuf : r;
		memcpy(buf, b->hdr, r);
		b->off += r;
		nbuf -= r;
		n += r;
	}
	if(b->off < b->ndat + b->nhdr){
		s = buf;
		o = b->off - b->nhdr;
		r = b->ndat - o;
		r = (nbuf < r) ? nbuf : r;
		memcpy(s + n, b->dat + o, r);
		b->off += r;
		n += r;
	}
	return n;
}

void
writeobj(Hash *h, char *hdr, int nhdr, char *dat, int ndat)
{
	Objbuf b = {.off=0, .hdr=hdr, .nhdr=nhdr, .dat=dat, .ndat=ndat};
	char s[64], o[256];
	SHA1state *st;
	Biobuf *f;
	int fd;

	st = sha1((uchar*)hdr, nhdr, nil, nil);
	st = sha1((uchar*)dat, ndat, nil, st);
	sha1(nil, 0, h->h, st);

	snprint(s, sizeof(s), "%H", *h);
	fd = create(".git/objects", OREAD, DMDIR|0755);
	close(fd);
	snprint(o, sizeof(o), ".git/objects/%c%c", s[0], s[1]);
	fd = create(o, OREAD, DMDIR | 0755);
	close(fd);
	snprint(o, sizeof(o), ".git/objects/%c%c/%s", s[0], s[1], s + 2);
	if(readobject(*h) == nil){
		if((f = Bopen(o, OWRITE)) == nil)
			sysfatal("could not open %s: %r", o);
		if(deflatezlib(f, bwrite, &b, objbytes, 9, 0) == -1)
			sysfatal("could not write %s: %r", o);
		Bterm(f);
	}
}

int
writetree(Dirent *ent, int nent, Hash *h)
{
	char *t, *txt, *etxt, hdr[128];
	int nhdr, n;
	Dirent *d, *p;

	t = emalloc((16+256+20) * nent);
	txt = t;
	etxt = t + (16+256+20) * nent;

	/* sqeeze out deleted entries */
	n = 0;
	p = ent;
	for(d = ent; d != ent + nent; d++)
		if(d->name)
			p[n++] = *d;
	nent = n;

	qsort(ent, nent, sizeof(Dirent), entcmp);
	for(d = ent; d != ent + nent; d++){
		if(strlen(d->name) >= 255)
			sysfatal("overly long filename: %s", d->name);
		t = seprint(t, etxt, "%o %s", gitmode(d->mode), d->name) + 1;
		memcpy(t, d->h.h, sizeof(d->h.h));
		t += sizeof(d->h.h);
	}
	nhdr = snprint(hdr, sizeof(hdr), "%T %lld", GTree, (vlong)(t - txt)) + 1;
	writeobj(h, hdr, nhdr, txt, t - txt);
	free(txt);
	return nent;
}

void
blobify(Dir *d, char *path, int *mode, Hash *bh)
{
	char h[64], *buf;
	int f, nh;

	if((d->mode & DMDIR) != 0)
		sysfatal("not file: %s", path);
	*mode = d->mode;
	nh = snprint(h, sizeof(h), "%T %lld", GBlob, d->length) + 1;
	if((f = open(path, OREAD)) == -1)
		sysfatal("could not open %s: %r", path);
	buf = emalloc(d->length);
	if(readn(f, buf, d->length) != d->length)
		sysfatal("could not read blob %s: %r", path);
	writeobj(bh, h, nh, buf, d->length);
	free(buf);
	close(f);
}

int
tracked(char *path)
{
	char ipath[256];
	Dir *d;

	/* Explicitly removed. */
	snprint(ipath, sizeof(ipath), ".git/index9/removed/%s", path);
	if(strstr(cleanname(ipath), ".git/index9/removed") != ipath)
		sysfatal("path %s leaves index", ipath);
	d = dirstat(ipath);
	if(d != nil && d->qid.type != QTDIR){
		free(d);
		return 0;
	}

	/* Explicitly added. */
	snprint(ipath, sizeof(ipath), ".git/index9/tracked/%s", path);
	if(strstr(cleanname(ipath), ".git/index9/tracked") != ipath)
		sysfatal("path %s leaves index", ipath);
	if(access(ipath, AEXIST) == 0)
		return 1;

	return 0;
}

int
pathelt(char *buf, int nbuf, char *p, int *isdir)
{
	char *b;

	b = buf;
	if(*p == '/')
		p++;
	while(*p && *p != '/' && b != buf + nbuf)
		*b++ = *p++;
	*b = '\0';
	*isdir = (*p == '/');
	return b - buf;
}

Dirent*
dirent(Dirent **ent, int *nent, char *name)
{
	Dirent *d;

	for(d = *ent; d != *ent + *nent; d++)
		if(d->name && strcmp(d->name, name) == 0)
			return d;
	*nent += 1;
	*ent = erealloc(*ent, *nent * sizeof(Dirent));
	d = *ent + (*nent - 1);
	memset(d, 0, sizeof(*d));
	d->name = estrdup(name);
	return d;
}

int
treeify(Object *t, char **path, char **epath, int off, Hash *h)
{
	int r, n, ne, nsub, nent, isdir;
	char **p, **ep;
	char elt[256];
	Object **sub;
	Dirent *e, *ent;
	Dir *d;

	r = -1;
	nsub = 0;
	nent = t->tree->nent;
	ent = eamalloc(nent, sizeof(*ent));
	sub = eamalloc((epath - path), sizeof(Object*));
	memcpy(ent, t->tree->ent, nent*sizeof(*ent));
	for(p = path; p != epath; p = ep){
		ne = pathelt(elt, sizeof(elt), *p + off, &isdir);
		for(ep = p; ep != epath; ep++){
			if(strncmp(elt, *ep + off, ne) != 0)
				break;
			if((*ep)[off+ne] != '\0' && (*ep)[off+ne] != '/')
				break;
		}
		e = dirent(&ent, &nent, elt);
		if(e->islink)
			sysfatal("symlinks may not be modified: %s", *path);
		if(e->ismod)
			sysfatal("submodules may not be modified: %s", *path);
		if(isdir){
			e->mode = DMDIR | 0755;
			sub[nsub] = readobject(e->h);
			if(sub[nsub] == nil || sub[nsub]->type != GTree)
				sub[nsub] = emptydir();
			/*
			 * if after processing deletions, a tree is empty,
			 * mark it for removal from the parent.
			 *
			 * Note, it is still written to the object store,
			 * but this is fine -- and ensures that an empty
			 * repository will continue to work.
			 */
			n = treeify(sub[nsub], p, ep, off + ne + 1, &e->h);
			if(n == 0)
				e->name = nil;
			else if(n == -1)
				goto err;
		}else{
			d = dirstat(*p);
			if(d != nil && tracked(*p))
				blobify(d, *p, &e->mode, &e->h);
			else
				e->name = nil;
			free(d);
		}
	}
	if(nent == 0){
		werrstr("%.*s: empty directory", off, *path);
		goto err;
	}

	r = writetree(ent, nent, h);
err:
	free(sub);
	return r;		
}


void
mkcommit(Hash *c, char *msg, char *name, char *email, vlong date, Hash *parents, int nparents, Hash tree)
{
	char *s, h[64];
	int ns, nh, i;
	Fmt f;

	fmtstrinit(&f);
	fmtprint(&f, "tree %H\n", tree);
	for(i = 0; i < nparents; i++)
		fmtprint(&f, "parent %H\n", parents[i]);
	fmtprint(&f, "author %s <%s> %lld +0000\n", name, email, date);
	fmtprint(&f, "committer %s <%s> %lld +0000\n", name, email, date);
	fmtprint(&f, "\n");
	fmtprint(&f, "%s", msg);
	s = fmtstrflush(&f);

	ns = strlen(s);
	nh = snprint(h, sizeof(h), "%T %d", GCommit, ns) + 1;
	writeobj(c, h, nh, s, ns);
	free(s);
}

Object*
findroot(void)
{
	Object *t, *c;
	Hash h;

	if(resolveref(&h, "HEAD") == -1)
		return emptydir();
	if((c = readobject(h)) == nil || c->type != GCommit)
		sysfatal("could not read HEAD %H", h);
	if((t = readobject(c->commit->tree)) == nil)
		sysfatal("could not read tree for commit %H", h);
	return t;
}

void
usage(void)
{
	fprint(2, "usage: %s -n name -e email -m message -d date [files...]\n", argv0);
	exits("usage");
}

void
main(int argc, char **argv)
{
	Hash th, ch, parents[Maxparents];
	char *msg, *name, *email, *dstr, cwd[1024];
	int i, r, ncwd, nparents;
	vlong date;
	Object *t;

	gitinit();
	if(access(".git", AEXIST) != 0)
		sysfatal("could not find git repo: %r");
	if(getwd(cwd, sizeof(cwd)) == nil)
		sysfatal("getcwd: %r");
	msg = nil;
	name = nil;
	email = nil;
	dstr = nil;
	date = time(nil);
	nparents = 0;
	ncwd = strlen(cwd);

	ARGBEGIN{
	case 'm':	msg = EARGF(usage());	break;
	case 'n':	name = EARGF(usage());	break;
	case 'e':	email = EARGF(usage());	break;
	case 'd':	dstr = EARGF(usage());	break;
	case 'p':
		if(nparents >= Maxparents)
			sysfatal("too many parents");
		if(resolveref(&parents[nparents++], EARGF(usage())) == -1)
			sysfatal("invalid parent: %r");
		break;
	default:
		usage();
	}ARGEND;

	if(!msg)
		sysfatal("missing message");
	if(!name)
		sysfatal("missing name");
	if(!email)
		sysfatal("missing email");
	if(dstr){
		date=strtoll(dstr, &dstr, 10);
		if(strlen(dstr) != 0)
			sysfatal("could not parse date %s", dstr);
	}
	if(msg == nil || name == nil)
		usage();
	for(i = 0; i < argc; i++){
		cleanname(argv[i]);
		if(*argv[i] == '/' && strncmp(argv[i], cwd, ncwd) == 0)
			argv[i] += ncwd;
		while(*argv[i] == '/')
			argv[i]++;
	}

	t = findroot();
	r = treeify(t, argv, argv + argc, 0, &th);
	if(r == -1)
		sysfatal("could not commit: %r\n");
	mkcommit(&ch, msg, name, email, date, parents, nparents, th);
	print("%H\n", ch);
	exits(nil);
}
