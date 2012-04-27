#include "all.h"

/*
 * Attribute handling
 *
 * BUG: we only support the predefined attributes.
 *
 */

typedef struct Adef Adef;

struct Adef
{
	char*	name;
	int	sz;
	long	(*wattr)(Memblk*, void*, long);
	long	(*rattr)(Memblk*, void*, long);
	void	(*cattr)(Memblk*, int, void*, long, void*, long);
};

long wname(Memblk*, void*, long);
static long rname(Memblk*, void*, long);
static long rid(Memblk*, void*, long);
static long wid(Memblk*, void*, long);
static long wmode(Memblk*, void*, long);
static long rmode(Memblk*, void*, long);
static long watime(Memblk*, void*, long);
static long ratime(Memblk*, void*, long);
static long wmtime(Memblk*, void*, long);
static long rmtime(Memblk*, void*, long);
static long wlength(Memblk*, void*, long);
static long rlength(Memblk*, void*, long);
static long wuid(Memblk*, void*, long);
static long ruid(Memblk*, void*, long);
static long wgid(Memblk*, void*, long);
static long rgid(Memblk*, void*, long);
static long wmuid(Memblk*, void*, long);
static long rmuid(Memblk*, void*, long);
static long rstar(Memblk*, void*, long);
static void cstring(Memblk*, int, void*, long, void*, long);
static void cu64int(Memblk*, int, void*, long, void*, long);

static Adef adef[] =
{
	{"name", 0, wname, rname, cstring},
	{"id",	BIT64SZ, nil, rid, cu64int},
	{"atime", BIT64SZ, watime, ratime, cu64int},
	{"mtime", BIT64SZ, wmtime, rmtime, cu64int},
	{"length", BIT64SZ, wlength, rlength, cu64int},
	{"uid", 0, wuid, ruid, cstring},
	{"gid", 0, wgid, rgid, cstring},
	{"muid", 0, wuid, ruid, cstring},
	{"mode", BIT64SZ, wmode, rmode, cu64int},
	{"*", 0, nil, rstar, nil},
};

/*
 * Return size for attributes embedded in file.
 * At least Dminattrsz bytes are reserved in the file block,
 * at most Embedsz.
 * Size is rounded to the size of an address.
 */
ulong
embedattrsz(Memblk *f)
{
	ulong sz;

	sz = f->d.asize;
	sz = ROUNDUP(sz, BIT64SZ);
	if(sz < Dminattrsz)
		sz = Dminattrsz;
	else
		sz = Embedsz;
	return sz;
}

void
gmeta(Memblk *f, void *buf, ulong nbuf)
{
	char *p;
	int i;

	f->mf->uid = usrname(f->d.uid);
	f->mf->gid = usrname(f->d.gid);
	f->mf->muid = usrname(f->d.muid);
	p = buf;
	for(i = 0; i < nbuf; i++)
		if(p[i] == 0)
			break;
	if(i == nbuf)
		error("corrupt meta");
	f->mf->name = buf;
	
}

static ulong
metasize(Memblk *f)
{
	return strlen(f->mf->name) + 1;
}

/*
 * Pack the metadata into buf.
 * pointers in meta are changed to refer to the packed data.
 * Return pointer past the packed metadata.
 * The caller is responsible for ensuring that metadata fits in buf.
 */
ulong
pmeta(void *buf, ulong nbuf, Memblk *f)
{
	char *p, *e;
	ulong sz;

	sz = metasize(f);
	if(sz > nbuf)
		error("attributes are too long");
	p = buf;
	if(f->mf->name != p)
		e = strecpy(p, p+nbuf, f->mf->name);
	else
		e = p + strlen(p);
	e++;
	assert(e-p <= sz);	/* can be <, to leave room for growing */
	f->mf->name = p;
	return sz;
}

static void
ceval(int op, int v)
{
	switch(op){
	case CEQ:
		if(v != 0)
			error("false");
		break;
	case CGE:
		if(v < 0)
			error("false");
		break;
	case CGT:
		if(v <= 0)
			error("false");
		break;
	case CLE:
		if(v > 0)
			error("false");
		break;
	case CLT:
		if(v >= 0)
			error("false");
	case CNE:
		if(v == 0)
			error("false");
		break;
	}
}

static void
cstring(Memblk*, int op, void *buf, long, void *val, long len)
{
	char *p;

	p = val;
	if(len < 1 || p[len-1] != 0)
		error("value must end in \\0");
	ceval(op, strcmp(buf, val));
}

static void
cu64int(Memblk*, int op, void *buf, long, void *val, long)
{
	u64int v1, v2;
	uchar *p1, *p2;

	p1 = buf;
	p2 = val;
	v1 = GBIT64(p1);
	v2 = GBIT64(p2);
	/* avoid overflow */
	if(v1 > v2)
		ceval(op, 1);
	else if(v1 < v2)
		ceval(op, -1);
	else
		ceval(op, 0);
}

long 
wname(Memblk *f, void *buf, long len)
{
	char *p, *old;
	ulong maxsz;

	p = buf;
	if(len < 1 || p[len-1] != 0)
		error("name must end in \\0");
	old = f->mf->name;
	f->mf->name = p;
	maxsz = embedattrsz(f);
	if(metasize(f) > maxsz){
		f->mf->name = old;
		fprint(2, "%s: bug: can't handle DBattr blocks\n", argv0);
		error("no room to grow metadata");
	}
	pmeta(f->d.embed, maxsz, f);
	return len;
}

static long
rstr(char *s, void *buf, long len)
{
	long l;

	l = strlen(s) + 1;
	if(l > len)
		error("buffer too short");
	strcpy(buf, s);
	return l;
}

static long 
rname(Memblk *f, void *buf, long len)
{
	return rstr(f->mf->name, buf, len);
}

static long 
ruid(Memblk *f, void *buf, long len)
{
	return rstr(f->mf->uid, buf, len);
}

static u64int
chkusr(void *buf, long len)
{
	char *p;
	int id;

	p = buf;
	if(len < 1 || p[len-1] != 0)
		error("name must end in \\0");
	id = usrid(buf);
	if(id < 0)
		error("unknown user '%s'", buf);
	return id;
}

static long 
wuid(Memblk *f, void *buf, long len)
{
	f->d.uid = chkusr(buf, len);
	return len;
}

static long 
rgid(Memblk *f, void *buf, long len)
{
	return rstr(f->mf->gid, buf, len);
}

static long 
wgid(Memblk *f, void *buf, long len)
{
	f->d.gid = chkusr(buf, len);
	return len;
}

static long 
rmuid(Memblk *f, void *buf, long len)
{
	return rstr(f->mf->muid, buf, len);
}

static long 
wmuid(Memblk *f, void *buf, long len)
{
	f->d.muid = chkusr(buf, len);
	return len;
}

static long 
wid(Memblk *f, void *buf, long)
{
	u64int *p;

	p = buf;
	f->d.id = *p;
	return BIT64SZ;
}

static long 
rid(Memblk *f, void *buf, long)
{
	u64int *p;

	p = buf;
	*p = f->d.id;
	return BIT64SZ;
}

static long 
watime(Memblk *f, void *buf, long)
{
	u64int *p;

	p = buf;
	f->d.atime = *p;
	return BIT64SZ;
}

static long 
ratime(Memblk *f, void *buf, long)
{
	u64int *p;

	p = buf;
	*p = f->d.atime;
	return BIT64SZ;
}

static long 
wmode(Memblk *f, void *buf, long)
{
	u64int *p;

	p = buf;
	f->d.mode = *p | (f->d.mode&DMUSERS);
	return BIT64SZ;
}

static long 
rmode(Memblk *f, void *buf, long)
{
	u64int *p;

	p = buf;
	*p = f->d.mode & ~DMUSERS;
	return BIT64SZ;
}

static long 
wmtime(Memblk *f, void *buf, long)
{
	u64int *p;

	p = buf;
	f->d.mtime = *p;
	return BIT64SZ;
}

static long 
rmtime(Memblk *f, void *buf, long)
{
	u64int *p;

	p = buf;
	*p = f->d.mtime;
	return BIT64SZ;
}

static uvlong
resized(Memblk *f, uvlong sz)
{
	ulong boff, bno, bend;

	if(f->d.mode&DMDIR)
		error("can't resize a directory");

	if(sz > maxfsz)
		error("max file size exceeded");
	if(sz >= f->d.length)
		return sz;
	bno = dfbno(f, sz, &boff);
	if(boff > 0)
		bno++;
	bend = dfbno(f, sz, &boff);
	if(boff > 0)
		bend++;
	dfdropblks(f, bno, bend);
	return sz;
}

static long 
wlength(Memblk *f, void *buf, long)
{
	u64int *p;

	p = buf;
	f->d.length = *p;
	resized(f, f->d.length);
	return BIT64SZ;
}

static long 
rlength(Memblk *f, void *buf, long)
{
	u64int *p;

	p = buf;
	*p = f->d.length;
	return BIT64SZ;
}

static long 
rstar(Memblk *, void *buf, long len)
{
	char *s, *e;
	int i;

	s = buf;
	e = s + len;
	for(i = 0; i < nelem(adef); i++)
		if(strcmp(adef[i].name, "*") != 0)
			s = seprint(s, e, "%s ", adef[i].name);
	if(s > buf)
		*--s = 0;
	return s - (char*)buf;
}

long
dfwattr(Memblk *f, char *name, void *val, long nval)
{
	int i;
	long tot;

	isfile(f);
	ismelted(f);
	isrwlocked(f, Wr);
	if(fsfull())
		error("file system full");

	for(i = 0; i < nelem(adef); i++)
		if(strcmp(adef[i].name, name) == 0)
			break;
	if(i == nelem(adef))
		error("bug: user defined attributes not yet implemented");
	if(adef[i].wattr == nil)
		error("can't write %s", name);
	if(adef[i].sz != 0 && adef[i].sz != nval)
		error("wrong length for attribute");

	tot = adef[i].wattr(f, val, nval);
	changed(f);
	return tot;
}

long
dfrattr(Memblk *f, char *name, void *val, long count)
{
	int i;
	long tot;

	isfile(f);
	isrwlocked(f, Rd);
	for(i = 0; i < nelem(adef); i++)
		if(strcmp(adef[i].name, name) == 0)
			break;
	if(i == nelem(adef))
		error("no such attribute");
	if(adef[i].sz != 0 && count < adef[i].sz)
		error("buffer too short for attribute");

	tot = adef[i].rattr(f, val, count);
	return tot;
}

/*
 * cond on attribute value
 */
void
dfcattr(Memblk *f, int op, char *name, void *val, long count)
{
	int i;
	long nbuf;
	char buf[128];

	isfile(f);
	isrwlocked(f, Rd);

	nbuf = dfrattr(f, name, buf, sizeof buf);

	for(i = 0; i < nelem(adef); i++)
		if(strcmp(adef[i].name, name) == 0)
			break;
	if(i == nelem(adef))
		error("no such attribute");
	if(adef[i].sz != 0 && count != adef[i].sz)
		error("value size does not match");
	adef[i].cattr(f, op, buf, nbuf, val, count);
}

/*
 * Does not check if the user can't write because of the "write"
 * user.
 * Does check if the user is allowed in config mode.
 */
void
dfaccessok(Memblk *f, int uid, int bits)
{
	uint mode;

	if(allowed(uid))
		return;

	bits &= 3;

	mode = f->d.mode &0777;

	if((mode&bits) == bits)
		return;
	mode >>= 3;
	
	if(member(f->d.gid, uid) && (mode&bits) == bits)
		return;
	mode >>= 3;
	if(f->d.uid == uid && (mode&bits) == bits)
		return;
	error("permission denied");
}
