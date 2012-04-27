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
	char *name;
	Dir;
};

typedef struct Arch Arch;
struct Arch {
	vlong off;
	vlong length;
};

static void*
emalloc(long sz)
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
	a->name = estrdup(f[0]);
	a->mode = strtoul(f[1], 0, 8);
	a->uid = estrdup(f[2]);
	a->gid = estrdup(f[3]);
	a->mtime = strtoll(f[4], 0, 10);
	a->length = strtoll(f[5], 0, 10);
	return a;
}

static Arch*
newarch(vlong off, vlong length)
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
createpath(File *f, char *name, char *u, ulong m)
{
	char *p;
	File *nf;

	if(verbose)
		fprint(2, "createpath %s\n", name);
	incref(f);
	while(f && (p = strchr(name, '/'))) {
		*p = '\0';
		if(strcmp(name, "") != 0 && strcmp(name, ".") != 0){
			/* this would be a race if we were multithreaded */
			incref(f);	/* so walk doesn't kill it immediately on failure */
			if((nf = walkfile(f, name)) == nil)
				nf = createfile(f, name, u, DMDIR|0777, nil);
			decref(f);
			f = nf;
		}
		*p = '/';
		name = p+1;
	}
	if(f == nil)
		return nil;

	incref(f);
	if((nf = walkfile(f, name)) == nil)
		nf = createfile(f, name, u, m, nil);
	decref(f);
	return nf;
}

static void
archcreatefile(char *name, Arch *arch, Dir *d)
{
	File *f;
	f = createpath(archtree->root, name, d->uid, d->mode);
	if(f == nil)
		sysfatal("creating %s: %r", name);
	free(f->gid);
	f->gid = estrdup9p(d->gid);
	f->aux = arch;
	f->mtime = d->mtime;
	f->length = d->length;
	decref(f);
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
	ulong flag;
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
		archcreatefile(a->name, newarch(Boffset(b), a->length), a);
		Bseek(b, a->length, 1);
	}

	err[0] = '\0';
	errstr(err, sizeof err);
	if(err[0])
		sysfatal("reading archive: %s", err);

	postmountsrv(&fs, nil, mtpt, flag);
	exits(0);
}
