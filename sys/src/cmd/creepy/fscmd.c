#include "all.h"

/*
 * HUGE warning:
 * these commands do not perform checks at all.
 * that means you know what you are doing if you use them.
 * e.g., you can create multiple files with the same name
 * in the same directory.
 *
 * This tool is only an aid for testing and debugging.
 */

enum 
{
	Nels = 64
};

static char *fsdir;
static int verb;

int
member(int uid, int member)
{
	return uid == member;
}

int
allowed(int)
{
	return 1;
}

int
usrid(char*)
{
	return 0;
}

char*
usrname(int)
{
	return getuser();
}

void
meltfids(void)
{
}

void
rwusers(Memblk*)
{
}

char*
ninestats(char *s, char*, int)
{
	return s;
}

char*
ixstats(char *s, char*, int)
{
	return s;
}

void
countfidrefs(void)
{
}

/*
 * Walks elems starting at f.
 * Ok if nelems is 0.
 */
static Path*
walkpath(Memblk *f, char *elems[], int nelems)
{
	int i;
	Memblk *nf;
	Path *p;

	p = newpath(f);
	if(catcherror()){
		putpath(p);
		error(nil);
	}
	isfile(f);
	for(i = 0; i < nelems; i++){
		if((f->d.mode&DMDIR) == 0)
			error("not a directory");
		rwlock(f, Rd);
		if(catcherror()){
			rwunlock(f, Rd);
			error("walk: %r");
		}
		nf = dfwalk(f, elems[i], Rd);
		rwunlock(f, Rd);
		addelem(&p, nf);
		mbput(nf);
		f = nf;
		USED(&f);	/* in case of error() */
		noerror();
	}
	noerror();
	return p;
}

static char*
fsname(char *p)
{
	if(p[0] == '/')
		return strdup(p);
	if(fsdir)
		return smprint("%s/%s", fsdir, p);
	return strdup(p);
}

static Path*
walkto(char *a, char **lastp)
{
	char *els[Nels], *path;
	int nels;
	Path *p;

	path = fsname(a);
	nels = gettokens(path, els, Nels, "/");
	if(nels < 1){
		free(path);
		error("invalid path");
	}
	if(catcherror()){
		free(path);
		error("walkpath: %r");
	}
	if(lastp != nil){
		p = walkpath(fs->root, els, nels-1);
		*lastp = a + strlen(a) - strlen(els[nels-1]);
	}else
		p = walkpath(fs->root, els, nels);
	free(path);
	noerror();
	if(verb)
		print("walked to %H\n", p->f[p->nf-1]);
	return p;
}

static void
fscd(int, char *argv[])
{
	free(fsdir);
	fsdir = strdup(argv[1]);
}

/*
 * This is unrealistic in that it keeps the file locked
 * during the entire put. This means that we can only give
 * fslru() a chance before each put, and not before each
 * write, because everything is going to be in use and dirty if
 * we run out of memory.
 */
static void
fsput(int, char *argv[])
{
	int fd;
	char *fn;
	Memblk *m, *f;
	Dir *d;
	char buf[4096];
	uvlong off;
	long nw, nr;
	Path *p;

	fd = open(argv[1], OREAD);
	if(fd < 0)
		error("open: %r\n");
	d = dirfstat(fd);
	if(d == nil){
		error("dirfstat: %r\n");
	}
	if(catcherror()){
		close(fd);
		free(d);
		error(nil);
	}
	p = walkto(argv[2], &fn);
	if(catcherror()){
		putpath(p);
		error(nil);
	}
	dfmelt(&p, p->nf);
	m = p->f[p->nf-1];
	if(catcherror()){
		rwunlock(m, Wr);
		error(nil);
	}
	f = dfcreate(m, fn, usrid(d->uid), d->mode&(DMDIR|0777));
	noerror();
	addelem(&p, f);
	decref(f);	/* kept now in p */
	rwlock(f, Wr);
	rwunlock(m, Wr);
	if(catcherror()){
		rwunlock(f, Wr);
		error(nil);
	}
	if((d->mode&DMDIR) == 0){
		off = 0;
		for(;;){
			if(fsmemfree() < Mminfree)
				fslru();
			nr = read(fd, buf, sizeof buf);
			if(nr <= 0)
				break;
			nw = dfpwrite(f, buf, nr, &off);
			dWprint("wrote %ld of %ld bytes\n", nw, nr);
			off += nr;
		}
	}
	noerror();
	noerror();
	noerror();
	if(verb)
		print("created %H\nat %H\n", f, m);
	rwunlock(f, Wr);
	putpath(p);
	close(fd);
	free(d);
}

static void
fscat(int, char *argv[])
{
	Memblk *f;
	Mfile *m;
	char buf[4096];
	uvlong off;
	long nr;
	Path *p;

	p = walkto(argv[2], nil);
	f = p->f[p->nf-1];
	rwlock(f, Rd);
	if(catcherror()){
		rwunlock(f, Rd);
		putpath(p);
		error(nil);
	}
	m = f->mf;
	print("cat %-30s\t%M\t%5ulld\t%s %ulld refs\n",
		m->name, (ulong)f->d.mode, f->d.length, m->uid, dbgetref(f->addr));
	if((f->d.mode&DMDIR) == 0){
		off = 0;
		for(;;){
			if(fsmemfree() < Mminfree)
				fslru();
			nr = dfpread(f, buf, sizeof buf, off);
			if(nr <= 0)
				break;
			write(1, buf, nr);
			off += nr;
		}
	}
	noerror();
	rwunlock(f, Rd);
	putpath(p);
}

static void
fsget(int, char *argv[])
{
	Memblk *f;
	Mfile *m;
	char buf[4096];
	uvlong off;
	long nr;
	int fd;
	Path *p;

	fd = create(argv[1], OWRITE, 0664);
	if(fd < 0)
		error("create: %r\n");
	if(catcherror()){
		close(fd);
		error(nil);
	}
	p = walkto(argv[2], nil);
	f = p->f[p->nf-1];
	rwlock(f, Rd);
	if(catcherror()){
		rwunlock(f, Rd);
		putpath(p);
		error(nil);
	}
	m = f->mf;
	print("get %-30s\t%M\t%5ulld\t%s %ulld refs\n",
		m->name, (ulong)f->d.mode, f->d.length, m->uid, dbgetref(f->addr));
	if((f->d.mode&DMDIR) == 0){
		off = 0;
		for(;;){
			if(fsmemfree() < Mminfree)
				fslru();
			nr = dfpread(f, buf, sizeof buf, off);
			if(nr <= 0)
				break;
			if(write(fd, buf, nr) != nr){
				fprint(2, "%s: error: %r\n", argv[0]);
				break;
			}
			off += nr;
		}
	}
	close(fd);
	noerror();
	noerror();
	rwunlock(f, Rd);
	putpath(p);
}

static void
fssnap(int, char**)
{
	fssync();
}

static void
fsrcl(int, char**)
{
	fsreclaim();
	fssync();	/* commit changes to disk */
}

static void
fsdmp(int, char *argv[])
{
	fsdump(*argv[0] == 'l', strstr(argv[0], "all") != 0);
}

static void
fsdbg(int, char *argv[])
{
	char *s;

	memset(dbg, 0, sizeof dbg);
	for(s = argv[1]; *s; s++)
		dbg['D'] = dbg[*s] = 1;
}

static void
fsout(int, char*[])
{
	fslru();
}

static void
fsrm(int, char *argv[])
{
	Memblk *f, *pf;
	Path *p;

	p = walkto(argv[1], nil);	
	if(catcherror()){
		putpath(p);
		error(nil);
	}
	if(p->nf < 2)
		error("short path for rm");
	dfmelt(&p, p->nf-1);
	f = p->f[p->nf-1];
	pf = p->f[p->nf-2];
	rwlock(f, Wr);
	if(catcherror()){
		rwunlock(f, Wr);
		rwunlock(pf, Wr);
		error(nil);
	}
	dfremove(pf, f);
	p->f[p->nf-1] = nil;
	noerror();
	noerror();
	rwunlock(pf, Wr);
	putpath(p);
}

static void
fsst(int, char**)
{
	fprint(2, "%s\n", updatestats(0));
}

static void
fschk(int, char**)
{
	if(fscheck() != 0)
		error("check failed");
}

static void
fserr(int, char *argv[])
{
	if(*argv[0] == 'r'){
		swreaderr = atoi(argv[1]);
		print("sw read err count = %d\n", swreaderr);
	}else{
		swwriteerr = atoi(argv[1]);
		print("sw write err count = %d\n", swwriteerr);
	}
}

static void
fspol(int, char**)
{
	fspolicy();
}

static void
usage(void)
{
	fprint(2, "usage: %s [-DFLAGS] [-dv] [-f disk] cmd...\n", argv0);
	exits("usage");
}

static Cmd cmds[] =
{
	{"cd",	fscd,	2, "cd!where"},
	{"put",	fsput,	3, "put!src!dst"},
	{"get",	fsget,	3, "get!dst!src"},
	{"cat",	fscat,	3, "cat!what"},
	{"dump",	fsdmp,	1, "dump"},
	{"dumpall",	fsdmp, 1, "dumpall"},
	{"ldump",	fsdmp,	1, "ldump"},
	{"ldumpall",	fsdmp, 1, "ldumpall"},
	{"sync", fssnap, 1, "sync"},
	{"snap", fssnap, 1, "snap"},
	{"rcl",	fsrcl,	1, "rcl"},
	{"dbg", fsdbg,	2, "dbg!n"},
	{"out", fsout, 1, "out"},
	{"rm",	fsrm,	2, "rm!what"},
	{"stats", fsst, 1, "stats"},
	{"check", fschk, 1, "check"},
	{"rerr", fserr, 2, "rerr!n"},
	{"werr", fserr, 2, "werr!n"},
	{"pol", fspol, 1, "pol"},
};

void
threadmain(int argc, char *argv[])
{
	char *dev;
	char *args[Nels];
	int i, j, nargs;

	dev = "disk";
	ARGBEGIN{
	case 'v':
		verb++;
		break;
	case 'f':
		dev = EARGF(usage());
		break;
	default:
		if(ARGC() >= 'A' && ARGC() <= 'Z'){
			dbg['d'] = 1;
			dbg[ARGC()] = 1;
		}else
			usage();
	}ARGEND;
	if(argc == 0)
		usage();
	fatalaborts = 1;
	fmtinstall('H', mbfmt);
	fmtinstall('M', dirmodefmt);
	errinit(Errstack);
	if(catcherror()){
		fprint(2, "cmd failed: %r\n");
		threadexitsall("failed");
	}
	fsopen(dev);
	for(i = 0; i < argc; i++){
		if(verb>1)
			fsdump(0, Mem);
		print("%% %s\n", argv[i]);
		nargs = gettokens(argv[i], args, Nels, "!");
		for(j = 0; j < nelem(cmds); j++){
			if(strcmp(cmds[j].name, argv[i]) != 0)
				continue;
			if(cmds[j].nargs != 0 && cmds[j].nargs != nargs)
				fprint(2, "usage: %s\n", cmds[j].usage);
			else
				cmds[j].f(nargs, args);
			break;
		}
		if(j == nelem(cmds)){
			fprint(2, "no such command\n");
			for(j = 0; j < nelem(cmds); j++)
				fprint(2, "\t%s\n", cmds[j].usage);
			break;
		}
	}
	if(verb>1)
		fsdump(0, Mem);
	noerror();
	threadexitsall(nil);
}

