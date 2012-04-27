#include <u.h>
#include <libc.h>
#include <seg.h>
#include <tube.h>

enum
{
	Tnamelen = 16,
	Tbufsz = 128,
	Tsegsz = 64 * 1024,
};


typedef struct Ntube Ntube;
typedef struct Tdir Tdir;

struct Tdir
{
	Lock;
	void	*end;
	long	avail;
	char	name[Tnamelen];
	Ntube	*t;
	Tdir	*next;
};

struct Ntube
{
	char	name[Tnamelen];
	Tube*	t;
	Ntube*	next;
};

static Tdir *dir;
static Lock tlck;

int namedtubedebug;

#define dprint	if(namedtubedebug)print

static void
dumpdir(char *s)
{
	Tdir *tl;
	Ntube *nt;

	if(namedtubedebug == 0)
		return;
	if(s == nil)
		print("named tubes:\n");
	else
		print("%s:\n", s);
	for(tl = dir; tl != nil; tl = tl->next){
		print("\t%s at %#p\n", tl->name, tl->t ? tl->t->t : nil);
		for(nt = tl->t; nt != nil; nt = nt->next)
			print("\t\t%s at %#p\n", nt->name, nt->t);
	}
}

static Tdir*
dirlookup(char *name, int mkit)
{
	Tdir *tl;

	dprint("dirlookup %s mk=%d\n", name, mkit);
	lock(&tlck);
	for(tl = dir; tl != nil; tl = tl->next)
		if(strcmp(name, tl->name) == 0){
			break;
		}
	if(tl == nil && !mkit)
		werrstr("segment not found");
	if(tl == nil && mkit){
		tl = newseg(name, 0, Tsegsz);
		if(tl != nil){
			strncpy(tl->name, name, sizeof tl->name);
			tl->end = &tl[1];
			tl->avail = Tsegsz;
			tl->next = dir;
			dir = tl;
		}
		dumpdir("after newseg");
	}
	unlock(&tlck);
	dprint("dirlookup %s: %#p\n", name, tl);
	return tl;
}

static Tube*
tubelookup(Tdir *dir, char *name, ulong elsz, ulong n, int mkit)
{
	Ntube *nt;
	uchar *p;

	dprint("tubelookup %s elsz=%uld mk=%d\n", name, elsz, mkit);
	if(elsz <= 0 || n <= 0){
		werrstr("bad argument");
		dprint("tubelookup %s: %r\n", name);
		return nil;
	}
	lock(dir);
	for(nt = dir->t; nt != nil; nt = nt->next)
		if(strcmp(nt->name, name) == 0)
			break;
	if(nt == nil && !mkit){
		werrstr("tube not found");
		dprint("tubelookup %s: %r\n", name);
	}
	if(nt == nil && mkit){
		/*
		 * This may overflow the segment, and we'll trap in
		 * that case.
		 */
		dir->avail -= sizeof *nt + sizeof(Tube) + n*elsz;
		if(dir->avail < 0){
			unlock(dir);
			werrstr("segment exhausted");
			dprint("tubelookup %s: %r\n", name);
			return nil;
		}
		p = dir->end;
		nt = dir->end;
		p += sizeof *nt;
		dir->end = p;
		strncpy(nt->name, name, sizeof nt->name);
		nt->t = dir->end;
		p += sizeof(Tube) + n*elsz;
		dir->end = p;
		nt->t->msz = elsz;
		nt->t->tsz = n;
		nt->t->nhole = n;
		nt->next = dir->t;
		dir->t = nt;
	}
	unlock(dir);
	if(nt == nil)
		return nil;
	if(nt->t->msz != elsz){
		werrstr("wrong element size");
		dprint("tubelookup %s %r\n", name);
		return nil;
	}
	dprint("tubelookup %s: found at %#p\n", name, nt->t);
	return nt->t;
}

/*
 * Return a tube for name segmentname!tubename,
 * creating any of them if mkit is true and it is not found.
 */
Tube*
namedtube(char *name, ulong msz, int n, int mkit)
{
	char *dir, *tname;
	Tdir *tl;
	Tube *t;

	dumpdir("dir before namedtube");
	name = strdup(name);
	if(name == nil)
		return nil;
	tname = utfrune(name, '!');
	if(tname == nil){
		dir = "tubes";
		tname = name;
	}else{
		dir = name;
		*tname++ = 0;
	}
	t = nil;
	tl = dirlookup(dir, mkit);
	if(tl != nil)
		t = tubelookup(tl, tname, msz, n, mkit);
	free(name);
	return t;
}
