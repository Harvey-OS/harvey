#include <u.h>
#include <libc.h>
#include "git.h"

#define NCACHE 4096
#define TDIR ".git/index9/tracked"
#define RDIR ".git/index9/removed"
#define HDIR ".git/fs/HEAD/tree"
typedef struct Cache	Cache;
typedef struct Wres	Wres;
struct Cache {
	Dir*	cache;
	int	n;
	int	max;
};

struct Wres {
	char	**path;
	int	npath;
	int	pathsz;
};

enum {
	Rflg	= 1 << 0,
	Mflg	= 1 << 1,
	Aflg	= 1 << 2,
	Tflg	= 1 << 3,
};

Cache seencache[NCACHE];
int quiet;
int printflg;
char *rstr = "R ";
char *tstr = "T ";
char *mstr = "M ";
char *astr = "A ";

int
seen(Dir *dir)
{
	Dir *dp;
	int i;
	Cache *c;

	c = &seencache[dir->qid.path&(NCACHE-1)];
	dp = c->cache;
	for(i=0; i<c->n; i++, dp++)
		if(dir->qid.path == dp->qid.path &&
		   dir->type == dp->type &&
		   dir->dev == dp->dev)
			return 1;
	if(c->n == c->max){
		if (c->max == 0)
			c->max = 8;
		else
			c->max += c->max/2;
		c->cache = realloc(c->cache, c->max*sizeof(Dir));
		if(c->cache == nil)
			sysfatal("realloc: %r");
	}
	c->cache[c->n++] = *dir;
	return 0;
}

void
grow(Wres *r)
{
	if(r->npath == r->pathsz){
		r->pathsz = 2*r->pathsz + 1;
		r->path = erealloc(r->path, r->pathsz * sizeof(char*));
	}
}

int
readpaths(Wres *r, char *pfx, char *dir)
{
	char *f, *sub, *full, *sep;
	Dir *d;
	int fd, ret, i, n;

	d = nil;
	ret = -1;
	sep = "";
	if(dir[0] != 0)
		sep = "/";
	if((full = smprint("%s/%s", pfx, dir)) == nil)
		sysfatal("smprint: %r");
	if((fd = open(full, OREAD)) < 0)
		goto error;
	while((n = dirread(fd, &d)) > 0){
		for(i = 0; i < n; i++){
			if(seen(&d[i]))
				continue;
			if(d[i].qid.type & QTDIR){
				if((sub = smprint("%s%s%s", dir, sep, d[i].name)) == nil)
					sysfatal("smprint: %r");
				if(readpaths(r, pfx, sub) == -1){
					free(sub);
					goto error;
				}
				free(sub);
			}else{
				grow(r);
				if((f = smprint("%s%s%s", dir, sep, d[i].name)) == nil)
					sysfatal("smprint: %r");
				r->path[r->npath++] = f;
			}
		}
		free(d);
	}
	ret = r->npath;
error:
	close(fd);
	free(full);
	return ret;
}

int
cmp(void *pa, void *pb)
{
	return strcmp(*(char **)pa, *(char **)pb);
}

void
dedup(Wres *r)
{
	int i, o;

	if(r->npath <= 1)
		return;
	o = 0;
	qsort(r->path, r->npath, sizeof(r->path[0]), cmp);
	for(i = 1; i < r->npath; i++)
		if(strcmp(r->path[o], r->path[i]) != 0)
			r->path[++o] = r->path[i];
	r->npath = o + 1;
}

int
sameqid(Dir *d, char *qf)
{
	char indexqid[64], fileqid[64], *p;
	int fd, n;

	if(!d)
		return 0;
	if((fd = open(qf, OREAD)) == -1)
		return 0;
	if((n = readn(fd, indexqid, sizeof(indexqid) - 1)) == -1)
		return 0;
	indexqid[n] = 0;
	close(fd);
	if((p = strpbrk(indexqid, "  \t\n\r")) != nil)
		*p = 0;

	snprint(fileqid, sizeof(fileqid), "%ullx.%uld.%.2uhhx",
		d->qid.path, d->qid.vers, d->qid.type);

	if(strcmp(indexqid, fileqid) == 0)
		return 1;
	return 0;
}

void
writeqid(Dir *d, char *qf)
{
	int fd;

	if((fd = create(qf, OWRITE, 0666)) == -1)
		return;
	fprint(fd, "%ullx.%uld.%.2uhhx\n",
		d->qid.path, d->qid.vers, d->qid.type);
	close(fd);
}

int
samedata(char *pa, char *pb)
{
	char ba[32*1024], bb[32*1024];
	int fa, fb, na, nb, same;

	same = 0;
	fa = open(pa, OREAD);
	fb = open(pb, OREAD);
	if(fa == -1 || fb == -1){
		goto mismatch;
	}
	while(1){
		if((na = readn(fa, ba, sizeof(ba))) == -1)
			goto mismatch;
		if((nb = readn(fb, bb, sizeof(bb))) == -1)
			goto mismatch;
		if(na != nb)
			goto mismatch;
		if(na == 0)
			break;
		if(memcmp(ba, bb, na) != 0)
			goto mismatch;
	}
	same = 1;
mismatch:
	if(fa != -1)
		close(fa);
	if(fb != -1)
		close(fb);
	return same;
}

void
usage(void)
{
	fprint(2, "usage: %s [-qbc] [-f filt] [paths...]\n", argv0);
	exits("usage");
}

void
main(int argc, char **argv)
{
	char *rpath, *tpath, *bpath, buf[8], repo[512];
	char *p, *e;
	int i, dirty;
	Wres r;
	Dir *d;

	ARGBEGIN{
	case 'q':
		quiet++;
		break;
	case 'c':
		rstr = "";
		tstr = "";
		mstr = "";
		astr = "";
		break;
	case 'f':
		for(p = EARGF(usage()); *p; p++)
			switch(*p){
			case 'T':	printflg |= Tflg;	break;
			case 'A':	printflg |= Aflg;	break;
			case 'M':	printflg |= Mflg;	break;
			case 'R':	printflg |= Rflg;	break;
			default:	usage();		break;
		}
		break;
	default:
		usage();
	}ARGEND

	if(findrepo(repo, sizeof(repo)) == -1)
		sysfatal("find root: %r");
	if(chdir(repo) == -1)
		sysfatal("chdir: %r");
	if(access(".git/fs/ctl", AEXIST) != 0)
		sysfatal("no running git/fs");
	dirty = 0;
	memset(&r, 0, sizeof(r));
	if(printflg == 0)
		printflg = Tflg | Aflg | Mflg | Rflg;
	if(argc == 0){
		if(access(TDIR, AEXIST) == 0 && readpaths(&r, TDIR, "") == -1)
			sysfatal("read tracked: %r");
		if(access(RDIR, AEXIST) == 0 && readpaths(&r, RDIR, "") == -1)
			sysfatal("read removed: %r");
	}else{
		for(i = 0; i < argc; i++){
			tpath = smprint(TDIR"/%s", argv[i]);
			rpath = smprint(RDIR"/%s", argv[i]);
			if((d = dirstat(tpath)) == nil && (d = dirstat(rpath)) == nil)
				goto nextarg;
			if(d->mode & DMDIR){
				readpaths(&r, TDIR, argv[i]);
				readpaths(&r, RDIR, argv[i]);
			}else{
				grow(&r);
				r.path[r.npath++] = estrdup(argv[i]);
			}
nextarg:
			free(tpath);
			free(rpath);
			free(d);
		}
	}
	dedup(&r);

	for(i = 0; i < r.npath; i++){
		p = r.path[i];
		d = dirstat(p);
		if(d && d->mode & DMDIR)
			goto next;
		rpath = smprint(RDIR"/%s", p);
		tpath = smprint(TDIR"/%s", p);
		bpath = smprint(HDIR"/%s", p);
		/* Fast path: we don't want to force access to the rpath. */
		if(d && sameqid(d, tpath)) {
			if(!quiet && (printflg & Tflg))
				print("%s%s\n", tstr, p);
		}else{
			if(d == nil || access(rpath, AEXIST) == 0){
				dirty |= Rflg;
				if(!quiet && (printflg & Rflg))
					print("%s%s\n", rstr, p);
			}else if(access(bpath, AEXIST) == -1) {
				dirty |= Aflg;
				if(!quiet && (printflg & Aflg))
					print("%s%s\n", astr, p);
			}else if(samedata(p, bpath)){
				if(!quiet && (printflg & Tflg))
					print("%s%s\n", tstr, p);
				writeqid(d, tpath);
			}else{
				dirty |= Mflg;
				if(!quiet && (printflg & Mflg))
					print("%s%s\n", mstr, p);
			}
		}
		free(rpath);
		free(tpath);
		free(bpath);
next:
		free(d);
	}
	if(!dirty)
		exits(nil);

	p = buf;
	e = buf + sizeof(buf);
	for(i = 0; (1 << i) != Tflg; i++)
		if(dirty & (1 << i))
			p = seprint(p, e, "%c", "DMAT"[i]);
	exits(buf);
}
