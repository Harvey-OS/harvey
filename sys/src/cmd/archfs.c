/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * archfs - mount mkfs style archives
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>

Tree *archtree;
Biobuf *b;
int verbose;

typedef struct Ahdr Ahdr;
struct Ahdr {
	char *_name;
	Dir Dir;
};

typedef struct Arch Arch;
struct Arch {
	int64_t off;
	int64_t length;
};

static void*
emalloc(int32_t sz)
{
	void *v;

	v = malloc(sz);
	if(v == nil)
		sysfatal("malloc %lud fails", sz);
	memset(v, 0, sz);
	return v;
}

static char*
estrdup(char *s)
{
	s = strdup(s);
	if(s == nil)
		sysfatal("strdup (%.10s) fails", s);
	return s;
}

static char*
Bgetline(Biobuf *b)
{
	char *p;

	if(p = Brdline(b, '\n'))
		p[Blinelen(b)-1] = '\0';
	return p;
}

Ahdr*
gethdr(Biobuf *b)
{
	Ahdr *a;
	char *p, *f[10];

	if((p = Bgetline(b)) == nil)
		return nil;

	if(strcmp(p, "end of archive") == 0) {
		werrstr("");
		return nil;
	}

	if(tokenize(p, f, nelem(f)) != 6) {
		werrstr("bad format");
		return nil;
	}

	a = emalloc(sizeof(*a));
	a->_name = estrdup(f[0]);
	a->Dir.mode = strtoul(f[1], 0, 8);
	a->Dir.uid = estrdup(f[2]);
	a->Dir.gid = estrdup(f[3]);
	a->Dir.mtime = strtoll(f[4], 0, 10);
	a->Dir.length = strtoll(f[5], 0, 10);
	return a;
}

static Arch*
newarch(int64_t off, int64_t length)
{
	static Arch *abuf;
	static int nabuf;

	if(nabuf == 0) {
		nabuf = 256;
		abuf = emalloc(sizeof(Arch)*nabuf);
	}

	nabuf--;
	abuf->off = off;
	abuf->length = length;
	return abuf++;
}

static File*
createpath(File *f, char *name, char *u, uint32_t m)
{
	char *p;
	File *nf;

	if(verbose)
		fprint(2, "createpath %s\n", name);
	incref(&f->Ref);
	while(f && (p = strchr(name, '/'))) {
		*p = '\0';
		if(strcmp(name, "") != 0 && strcmp(name, ".") != 0){
			/* this would be a race if we were multithreaded */
			incref(&f->Ref);	/* so walk doesn't kill it immediately on failure */
			if((nf = walkfile(f, name)) == nil)
				nf = createfile(f, name, u, DMDIR|0777, nil);
			decref(&f->Ref);
			f = nf;
		}
		*p = '/';
		name = p+1;
	}
	if(f == nil)
		return nil;

	incref(&f->Ref);
	if((nf = walkfile(f, name)) == nil)
		nf = createfile(f, name, u, m, nil);
	decref(&f->Ref);
	return nf;
}

static void
archcreatefile(char *name, Arch *arch, Dir *d)
{
	File *f;
	f = createpath(archtree->root, name, d->uid, d->mode);
	if(f == nil)
		sysfatal("creating %s: %r", name);
	free(f->Dir.gid);
	f->Dir.gid = estrdup9p(d->gid);
	f->aux = arch;
	f->Dir.mtime = d->mtime;
	f->Dir.length = d->length;
	decref(&f->Ref);
}

static void
fsread(Req *r)
{
	Arch *a;
	char err[ERRMAX];
	int n;

	a = r->fid->file->aux;
	if(a->length <= r->ifcall.offset) 
		r->ifcall.count = 0;
	else if(a->length <= r->ifcall.offset+r->ifcall.count)
		r->ifcall.count = a->length - r->ifcall.offset;

	werrstr("unknown error");
	if(Bseek(b, a->off+r->ifcall.offset, 0) < 0 
	|| (n = Bread(b, r->ofcall.data, r->ifcall.count)) < 0) {
		err[0] = '\0';
		errstr(err, sizeof err);
		respond(r, err);
	} else {
		r->ofcall.count = n;
		respond(r, nil);	
	}
}

Srv fs = {
	.read=	fsread,
};

static void
usage(void)
{
	fprint(2, "usage: archfs [-abcC] [-m mtpt] archfile\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	Ahdr *a;
	uint32_t flag;
	char *mtpt;
	char err[ERRMAX];

	flag = 0;
	mtpt = "/mnt/arch";
	ARGBEGIN{
	case 'D':
		chatty9p++;
		break;
	case 'a':
		flag |= MAFTER;
		break;
	case 'b':
		flag |= MBEFORE;
		break;
	case 'c':
		flag |= MCREATE;
		break;
	case 'C':
		flag |= MCACHE;
		break;
	case 'm':
		mtpt = EARGF(usage());
		break;
	default:
		usage();
		break;
	}ARGEND;

	if(argc != 1)
		usage();

	if((b = Bopen(argv[0], OREAD)) == nil)
		sysfatal("open '%s': %r", argv[0]);

	archtree = fs.tree = alloctree("sys", "sys", DMDIR|0775, nil);
	while(a = gethdr(b)) {
		archcreatefile(a->_name, newarch(Boffset(b), a->Dir.length), &a->Dir);
		Bseek(b, a->Dir.length, 1);
	}

	err[0] = '\0';
	errstr(err, sizeof err);
	if(err[0])
		sysfatal("reading archive: %s", err);

	postmountsrv(&fs, nil, mtpt, flag);
	exits(0);
}
