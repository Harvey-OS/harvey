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
		sysfatal("malloc %lud fails\n", sz);
	memset(v, 0, sz);
	return v;
}

static char*
estrdup(char *s)
{
	s = strdup(s);
	if(s == nil)
		sysfatal("strdup (%.10s) fails\n", s);
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
	strcpy(a->uid, f[2]);
	strcpy(a->gid, f[3]);
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
fcreatewalk(File *f, char *name, char *u, char *g, ulong m)
{
	char elem[NAMELEN];
	char *p;
	File *nf;

	if(verbose)
		fprint(2, "fcreatewalk %s\n", name);
	incref(&f->ref);
	while(f && (p = strchr(name, '/'))) {
		memmove(elem, name, p-name);
		elem[p-name] = '\0';
		name = p+1;
		if(strcmp(elem, "") == 0)
			continue;
		/* this would be a race if we were multithreaded */
		decref(&f->ref);
		nf = fwalk(f, elem);
		if(nf == nil)
			nf = fcreate(f, elem, u, g, CHDIR|0777);
		f = nf;
	}
	if(f == nil)
		return nil;

	if(nf = fwalk(f, name)) {
		decref(&f->ref);
		return nf;
	}
	/* another race */
	decref(&f->ref);
	return fcreate(f, name, u, g, m);
}

static void
createfile(char *name, Arch *arch, Dir *d)
{
	File *f;
	f = fcreatewalk(archtree->root, name, d->uid, d->gid, d->mode);
	if(f == nil)
		sysfatal("creating %s: %r", name);
	f->aux = arch;

	f->mtime = d->mtime;
	f->length = d->length;
	decref(&f->ref);
}

static void
archread(Req *r, Fid *fid, void *buf, long *count, vlong offset)
{
	Arch *a;
	long n;
	char err[ERRLEN];

	a = fid->file->aux;
	if(a->length <= offset) 
		*count = 0;
	else if(a->length <= offset+*count)
		*count = a->length - offset;

	werrstr("");
	if(Bseek(b, a->off+offset, 0) < 0 || (n = Bread(b, buf, *count)) < 0) {
		err[0] = '\0';
		errstr(err);
		respond(r, err);
	} else {
		*count = n;
		respond(r, nil);	
	}
}

Srv archsrv = {
	.read=	archread,
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
	char err[ERRLEN];

	flag = 0;
	mtpt = "/mnt/arch";
	ARGBEGIN{
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
		mtpt = ARGF();
		break;
	case 'v':
		verbose = 1;
		break;
	default:
		usage();
		break;
	}ARGEND;

	if(argc != 1)
		usage();

	if((b = Bopen(argv[0], OREAD)) == nil)
		sysfatal("open '%s': %r", argv[0]);

	archtree = archsrv.tree = mktree("sys", "sys", CHDIR|0775);
	while(a = gethdr(b)) {
		createfile(a->name, newarch(Boffset(b), a->length), a);
		Bseek(b, a->length, 1);
	}

	err[0] = '\0';
	errstr(err);
	if(err[0])
		sysfatal("reading archive: %s", err);

	postmountsrv(&archsrv, nil, mtpt, flag);
	exits(0);
}
