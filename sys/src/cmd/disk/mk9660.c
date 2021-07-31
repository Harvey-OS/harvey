#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#include <disk.h>

enum {
	Fconfmap = (1<<1),			/* this is _conform.map */
	Fbadname = (1<<2),		/* utfname does not conform */
	Fabstract = (1<<3),		/* this is the abstract file */
	Fbiblio = (1<<4),		/* this is the bibliography file */
	Fnotice = (1<<5),		/* this is the notice file */

	Ndirblock = 16,	/* directory blocks allocated at once */

	DTdot	= 0,
	DTdotdot,
	DTiden,
	DTroot,

	RUNOUT = 150,
};

/* output buffer */
typedef struct Imgbuf Imgbuf;
struct Imgbuf {
	Biobuf *bio;
	long offset;
};

typedef struct Direc Direc;
struct Direc {
	char utfname[NAMELEN];
	char name[NAMELEN];
	char uid[NAMELEN];
	char gid[NAMELEN];
	char *src;
	long mode;
	long length;
	long mtime;
	long pathid;

	long blockind;	/* index into block number array */

	uchar flags;

	Direc *child;		/* if a directory, its entries */
	int nchild;

	Direc *path;	/* next in breadth first linked list */
	Direc *parent;	/* parent entry */
};

#pragma varargck type "P" Direc*

/* plan 9 utf */
char *abstract;
char *biblio;
char *notice;

/* iso 9660 restricted names */
char *isoabstract;
char *isobiblio;
char *isonotice;

/* joliet restricted names */
char *jabstract;
char *jbiblio;
char *jnotice;

char *proto;
char *src;
char *volume;

char *confname = "_conform.map";
int conform;
int plan9;
int joliet;
int found;

int nconform;

long *blockno;
long nblockno;
ulong now;

int blocksize = 2048;

/* ISO9660 path table */
long isopathsize;
long isolpathloc;
long isompathloc;

/* Joliet path table */
long jpathsize;
long jlpathloc;
long jmpathloc;

/* _conform.map */
long conformloc;
long conformsize;

/* bootable images */
char *bootimage;
long bootimgloc;
long bootcatloc;

long volsize;
long pathid;

int chatty;

Direc root, jroot;

void*
emalloc(ulong sz)
{
	void *v;

	v = malloc(sz);
	if(v == nil)
		sysfatal("malloc %lud fails\n", sz);
	memset(v, 0, sz);
	return v;
}

void*
erealloc(void *v, ulong sz)
{
	v = realloc(v, sz);
	if(v == nil)
		sysfatal("realloc %lud fails\n", sz);
	return v;
}

int
Pconv(va_list *arg, Fconv *f)
{
	char path[500];
	Direc *d;

	d = va_arg(*arg, Direc*);
	if(d->parent == d)
		strcpy(path, "");
	else
		sprint(path, "%P/%s", d->parent, d->name);
	strconv(path, f);
	return 0;
}

char*
strupr(char *p, char *s)
{
	char *op;

	op = p;
	for(; *s; s++)
		*p++ = toupper(*s);
	*p = '\0';

	return op;
}

void
mkdirec(Direc *direc, Dir *d)
{
	memset(direc, 0, sizeof(Direc));
	strcpy(direc->utfname, d->name);
	strcpy(direc->uid, d->uid);
	strcpy(direc->gid, d->gid);
	direc->mode = d->mode;
	direc->length = d->length;
	direc->mtime = d->mtime;

	direc->blockind = nblockno++;
}

/*
 * Binary search a list of directories for the
 * entry with utfname name.
 * If no entry is found, return a pointer to
 * where a new such entry would go.
 */
static Direc*
bsearch(char *name, Direc *d, int n)
{
	int i;
	while(n > 0) {
		i = strcmp(name, d[n/2].utfname);
		if(i < 0)
			n = n/2;
		else if(i > 0) {
			d += n/2+1;
			n -= (n/2+1);
		} else
			return &d[n/2];
	}
	return d;
}

/*
 * Walk to name, starting at d.
 */
Direc*
walkdirec(Direc *d, char *name)
{
	char elem[NAMELEN], *p, *nextp;
	Direc *nd;

	for(p=name; p && *p; p=nextp) {
		if(nextp = strchr(p, '/')) {
			strncpy(elem, p, nextp-p);
			elem[nextp-p] = '\0';
			nextp++;
		} else {
			nextp = p+strlen(p);
			strncpy(elem, p, nextp-p);
			elem[nextp-p] = '\0';
		}

		nd = bsearch(elem, d->child, d->nchild);
		if(nd >= d->child+d->nchild || strcmp(nd->utfname, elem) != 0)
			return nil;
		d = nd;
	}
	return d;
}

/*
 * Add the file ``name'' with attributes d to the
 * directory ``root''.  Name may contain multiple
 * elements; all but the last must exist already.
 * 
 * The child lists are kept sorted by utfname.
 */	
Direc*
adddirec(Direc *root, char *name, Dir *d)
{
	char *p;
	Direc *nd;
	int off;

	if(name[0] == '/')
		name++;
	if(p = strrchr(name, '/')) {
		*p = '\0';
		root = walkdirec(root, name);
		if(root == nil) {
			sysfatal("error in proto file: no entry for /%s but /%s/%s\n", name, name, p+1);
			return nil;
		}
		*p = '/';
		p++;
	} else
		p = name;

	nd = bsearch(p, root->child, root->nchild);
	off = nd - root->child;
	if(off < root->nchild && strcmp(nd->utfname, p) == 0) {
		fprint(2, "warning: proto lists %s twice\n", name);
		return nil;
	}

	if(root->nchild%Ndirblock == 0) {
		root->child = erealloc(root->child, (root->nchild+Ndirblock)*sizeof(Direc));
		nd = root->child + off;
	}

	memmove(nd+1, nd, (root->nchild - off)*sizeof(Direc));
	mkdirec(nd, d);
	root->nchild++;
	return nd;
}

/* 
 * Copy the tree src into dst.
 */
void
copydirec(Direc *dst, Direc *src)
{
	int i, n;

	*dst = *src;

	if((src->mode & CHDIR) == 0)
		return;

	/* directories get fresh blocks */
	dst->blockind = nblockno++;

	n = (src->nchild + Ndirblock - 1);
	n -= n%Ndirblock;
	dst->child = emalloc(n*sizeof(Direc));

	n = dst->nchild;
	for(i=0; i<n; i++)
		copydirec(&dst->child[i], &src->child[i]);
}

/*
 * Sort a directory with a given comparison function.
 * After this is called on a tree, adddirec should not be,
 * since the entries may no longer be sorted as adddirec expects.
 */
void
dsort(Direc *d, int (*cmp)(void*, void*))
{
	int i, n;

	n = d->nchild;
	qsort(d->child, n, sizeof(d[0]), cmp);

	for(i=0; i<n; i++)
		dsort(&d->child[i], cmp);
}

char *strbuf;
int nstrbuf;

/*
 * There could be megabytes of
 * string data, over lots of
 * little strings; it is worth it
 * to use a custom strdup here.
 */
static char*
xstrdup(char *s)
{
	char *p;

	if(nstrbuf < strlen(s)+1) {
		strbuf = emalloc(10240);
		nstrbuf = 10240;
	}
	strcpy(strbuf, s);
	p = strbuf;

	strbuf += strlen(s)+1;
	nstrbuf -= strlen(s)+1;

	return p;
}

void
addfile(char *new, char *old, Dir *d, void *v)
{
	Direc *direc;

	if(direc = adddirec((Direc*)v, new, d)) {
		direc->src = xstrdup(old);
		if(abstract && strcmp(new, abstract) == 0) {
			direc->flags |= Fabstract;
			found |= Fabstract;
		}
		if(biblio && strcmp(new, biblio) == 0) {
			direc->flags |= Fbiblio;
			found |= Fbiblio;
		}
		if(notice && strcmp(new, notice) == 0) {
			direc->flags |= Fnotice;
			found |= Fnotice;
		}
	}
}

static void
printdirec(Direc *d, int dep)
{
	int i;
	static char *tabs = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";

	fprint(2, "%.*s%s (%s)\n", dep, tabs, d->name, d->utfname);
	for(i=0; i<d->nchild; i++)
		printdirec(&d->child[i], dep+1);
}

int
height(Direc *d)
{
	int i, n, m;

	if(d == nil)
		return 0;

	m = 0;
	for(i=0; i<d->nchild; i++)
		if((n = height(&d->child[i])) > m)
			m = n;
	return m+1;
}

/*
 * Set path pointers to point at next
 * directory entry in breadth first 
 * scan of tree.
 *
 * Depth first iterative deepening. 
 * Forgive me, I am weak.
 */
static Direc**
_setpath(Direc *d, Direc **pathp, int ndown)
{
	int i;

	if(ndown == 0) {
		if(d->mode & CHDIR) {
			d->pathid = pathid++;
			*pathp = d;
			return &d->path;
		}
		return pathp;
	}

	for(i=0; i<d->nchild; i++)
		pathp = _setpath(&d->child[i], pathp, ndown-1);
	return pathp;
}

void
setpath(Direc *d)
{
	int i, h;
	Direc **pathp;

	pathid = 1;
	d->pathid = pathid++;
	pathp = &d->path;

	h = height(d);
if(chatty) fprint(2, "height %d\n", h);
	for(i=1; i<h; i++)	/* only the root is at 0 */
		pathp = _setpath(d, pathp, i);

	*pathp = nil;
}

static void
printpath(Direc *d)
{
	for(; d; d=d->path)
		fprint(2, "%P %lud\n", d, d->pathid);
}

/*
 * Set the parent pointers; we couldn't do
 * this while we were creating the tree, since the
 * directory entries were being reallocated.
 */
static void
xsetparents(Direc *d)
{
	int i;

	for(i=0; i<d->nchild; i++) {
		d->child[i].parent = d;
		xsetparents(&d->child[i]);
	}
}

void
setparents(Direc *root)
{
	root->parent = root;
	xsetparents(root);
}

/*
 * Set the names to conform to 8.3
 * by changing them to numbers.
 * Plan 9 gets the right names from its
 * own directory entry; to ``aid'' other systems
 * we write the _conform.map file later.
 *
 * Joliet should take care of most of the
 * interoperability with other systems now.
 * This is mainly for old times' sake.
 */
void
convertnames(Direc *d, char* (*cvt)(char*, char*))
{
	int i;
	char new[NAMELEN];

	if(d->flags & Fbadname) {
		sprint(new, "F%.6d", nconform++);
		if(d->mode & CHDIR)
			new[0] = 'D';
		cvt(d->name, new);
	} else
		cvt(d->name, d->utfname);

	for(i=0; i<d->nchild; i++)
		convertnames(&d->child[i], cvt);
}

/*
 * Turn the Fbadname flag on for any entries
 * that have non-conforming names.
 */
void
checknames(Direc *d, int (*isbadname)(char*))
{
	int i;

	if(d->flags & Fconfmap)
		return;

	if(d != &root && d != &jroot && isbadname(d->utfname))
		d->flags |= Fbadname;

	if(strcmp(d->utfname, "_conform.map") == 0)
		d->flags |= Fbadname;

	for(i=0; i<d->nchild; i++)
		checknames(&d->child[i], isbadname);
}

/*
 * ISO 9660 file names must be uppercase, digits, or underscore.
 * we use lowercase, digits, and underscore, translating lower to upper.
 * files with uppercase letters in their names are thus nonconforming.
 * conforming files also must have a basename
 * at most 8 letters and at most one suffix of at most 3 letters.
 */
static int 
is9660frog(char c)
{
	if(c >= '0' && c <= '9')
		return 0;
	if(c >= 'a' && c <= 'z')
		return 0;
	if(c == '_')
		return 0;

	return 1;
}

int
isbadiso9660(char *s)
{
	char *p, *q;
	int i;

	if(p = strchr(s, '.')) {
		if(p-s > 8)
			return 1;
		for(q=s; q<p; q++)
			if(is9660frog(*q))
				return 1;
		if(strlen(p+1) > 3)
			return 1;
		for(q=p+1; *q; q++)
			if(is9660frog(*q))
				return 1;
	} else {
		if(strlen(s) > 8)
			return 1;
		for(q=s; *q; q++)
			if(is9660frog(*q))
				return 1;

		/*
		 * we rename files of the form [FD]dddddd
		 * so they don't interfere with us.
		 */
		if(strlen(s) == 7 && (s[0] == 'D' || s[0] == 'F')) {
			for(i=1; i<7; i++)
				if(s[i] < '0' || s[i] > '9')
					break;
			if(i == 7)
				return 1;
		}
	}
	return 0;
}

/*
 * Find the conforming name corresponding to
 * a UTF name; this must be called before dsort.
 */
char*
findconform(Direc *r, char *p)
{
	Direc *d;

	if(p == nil)
		return nil;

	d = walkdirec(r, p);
	if(d == nil){
		fprint(2, "warning: could not find %s (shouldn't happen)\n", p);
		return nil;
	}

	return d->name;
}

/*
 * ISO9660 name comparison
 * 
 * The standard algorithm is as follows:
 *   Take the filenames without extensions, pad the shorter with 0x20s (spaces),
 *   and do strcmp.  If they are equal, go on.
 *   Take the extensions, pad the shorter with 0x20s (spaces),
 *   and do strcmp.  If they are equal, go on.
 *   Compare the version numbers.
 *
 * Since Plan 9 names are not allowed to contain characters 0x00-0x1F,
 * the padded comparisons are equivalent to using strcmp directly.
 * We still need to handle the base and extension differently,
 * so that .foo sorts before !foo.foo.
 */
static int
isocmp(void *va, void *vb)
{
	int i;
	char s1[NAMELEN], s2[NAMELEN], *b1, *b2, *e1, *e2;
	Direc *a, *b;

	a = va;
	b = vb;

	b1 = strcpy(s1, a->name);
	b2 = strcpy(s2, b->name);
	if(e1 = strchr(b1, '.'))
		*e1++ = '\0';
	else
		e1 = "";
	if(e2 = strchr(b2, '.'))
		*e2++ = '\0';
	else
		e2 = "";

	if(i = strcmp(b1, b2))
		return i;

	if(i = strcmp(e1, e2))
		return i;

	return 0;
}

Rune*
strtorune(Rune *r, char *s)
{
	Rune *or;

	if(s == nil)
		return nil;

	or = r;
	while(*s)
		s += chartorune(r++, s);
	*r = L'\0';
	return or;
}

Rune*
runechr(Rune *s, Rune c)
{
	for(; *s; s++)
		if(*s == c)
			return s;
	return nil;
}

int
runecmp(Rune *s, Rune *t)
{
	while(*s && *t && *s == *t)
		s++, t++;
	return *s - *t;
}

/*
 * Joliet name validity check 
 * 
 * Joliet names have length at most 128 (not a problem),
 * and cannot contain '*', '/', ':', ';', '?', or '\'.
 */
int
isjolietfrog(Rune r)
{
	return r==L'*' || r==L'/' || r==L':' 
		|| r==';' || r=='?' || r=='\\';
}

int
isbadjoliet(char *s)
{
	Rune r[NAMELEN], *p;

	strtorune(r, s);
	for(p=r; *p; p++)
		if(isjolietfrog(*p))
			return 1;
	return 0;
}

/*
 * Joliet name comparison
 *
 * The standard algorithm is the ISO9660 algorithm but
 * on the encoded Runes.  Runes are encoded in big endian
 * format, so we can just use runecmp.
 * 
 * Padding is with zeros, but that still doesn't affect us.
 */
static int
jolietcmp(void *va, void *vb)
{
	int i;
	Rune s1[NAMELEN], s2[NAMELEN], *b1, *b2, *e1, *e2;
	Direc *a, *b;

	a = va;
	b = vb;

	b1 = strtorune(s1, a->utfname);
	b2 = strtorune(s2, b->utfname);
	if(e1 = runechr(b1, L'.'))
		*e1++ = '\0';
	else
		e1 = L"";
	if(e2 = runechr(b2, L'.'))
		*e2++ = '\0';
	else
		e2 = L"";

	if(i = runecmp(b1, b2))
		return i;

	if(i = runecmp(e1, e2))
		return i;

	return 0;
}

/*
 * CD image buffering routines;
 * b->bio == nil means we are in the first pass
 * and not actually writing.
 */
void
bputc(Imgbuf *b, int c)
{
	b->offset++;
	if(b->bio)
		Bputc(b->bio, c);
}

void
bputnl(Imgbuf *b, ulong val, int size)
{
	switch(size){
	default:
		sysfatal("bad size %d in bputnl", size);
	case 2:
		bputc(b, val);
		bputc(b, val>>8);
		break;
	case 4:
		bputc(b, val);
		bputc(b, val>>8);
		bputc(b, val>>16);
		bputc(b, val>>24);
		break;
	}
}

void
bputnm(Imgbuf *b, ulong val, int size)
{
	switch(size){
	default:
		sysfatal("bad size %d in bputnl", size);
	case 2:
		bputc(b, val>>8);
		bputc(b, val);
		break;
	case 4:
		bputc(b, val>>24);
		bputc(b, val>>16);
		bputc(b, val>>8);
		bputc(b, val);
		break;
	}
}

void
bputn(Imgbuf *b, long val, int size)
{
	bputnl(b, val, size);
	bputnm(b, val, size);
}

/*
 * ASCII/UTF string writing
 */
void
brepeat(Imgbuf *b, int c, int n)
{
	while(n-- > 0)
		bputc(b, c);
}

void
bputs(Imgbuf *b, char *s, int size)
{
	int n;

	if(s == nil) {
		brepeat(b, ' ', size);
		return;
	}

	for(n=0; n<size && *s; n++)
		bputc(b, *s++);
	if(n<size)
		brepeat(b, ' ', size-n);
}

/* 
 * Unicode writing as big endian 16-bit Runes
 */
void
bputr(Imgbuf *b, Rune r)
{
	bputc(b, r>>8);
	bputc(b, r);
}

void
brepeatr(Imgbuf *b, Rune r, int n)
{
	int i;

	for(i=0; i<n; i++)
		bputr(b, r);
}

void
bputrs(Imgbuf *b, Rune *s, int osize)
{
	int n, size;

	size = osize/2;
	if(s == nil)
		brepeatr(b, L' ', size);
	else {
if(chatty) fprint(2, "write %S\n", s);
		for(n=0; *s && n<size; n++)
			bputr(b, *s++);
		if(n<size)
			brepeatr(b, ' ', size-n);
	}
	if(osize&1)
		bputc(b, 0);	/* what else can we do? */
}

void
bputrscvt(Imgbuf *b, char *s, int size)
{
	Rune r[256];

	strtorune(r, s);
	bputrs(b, strtorune(r, s), size);
}

void
bpadblock(Imgbuf *b)
{
	int n;

	n = blocksize - (b->offset % blocksize);
	if(n != blocksize)
		brepeat(b, 0, n);
}

void
bputdate(Imgbuf *b, long ust)
{
	Tm *tm;

	if(ust == 0) {
		brepeat(b, 0, 7);
		return;
	}
	tm = gmtime(ust);
	bputc(b, tm->year);
	bputc(b, tm->mon+1);
	bputc(b, tm->mday);
	bputc(b, tm->hour);
	bputc(b, tm->min);
	bputc(b, tm->sec);
	bputc(b, 0);
}

void
bputdate1(Imgbuf *b, long ust)
{
	Tm *tm;
	char str[20];

	if(ust == 0) {
		brepeat(b, '0', 16);
		bputc(b, 0);
		return;
	}
	tm = gmtime(ust);
	sprint(str, "%.4d%.2d%.2d%.2d%.2d%.4d",
		tm->year+1900,
		tm->mon+1,
		tm->mday,
		tm->hour,
		tm->min,
		tm->sec*100);
	bputs(b, str, 16);
	bputc(b, 0);
}

/*
 * Write a directory entry.
 */
static void
genputdir(Imgbuf *b, Direc *d, int dot, int joliet)
{
	int f, n, l;
	long o;

	f = 0;
	if(d->mode & CHDIR)
		f |= 2;

	n = 1;
	if(dot == DTiden) {
		if(joliet)
			n = 2*utflen(d->name);
		else
			n = strlen(d->name);
	}

	l = 33+n;
	if(l & 1)
		l++;

	if(joliet==0 && plan9 && dot == DTiden) {
		if(d->flags & Fbadname)
			l += strlen(d->utfname);
		l += strlen(d->uid);
		l += strlen(d->gid);
		l += 3+8;
		if(l & 1)
			l++;
	}

	if(d->flags & Fconfmap) {
		d->length = conformsize;
if(chatty) fprint(2, "conformsize %ld\n", conformsize);
	}

	o = b->offset;
	o = blocksize - o % blocksize;
	if(o < l)
		bpadblock(b);

	bputc(b, l);			/* length of directory record */
	bputc(b, 0);			/* extended attribute record length */
	bputn(b, blockno[d->blockind], 4);		/* location of extent */
if(chatty) fprint(2, "%s at %ld %s\n", d->src, blockno[d->blockind], joliet ? "joliet" : "iso9660");
	bputn(b, d->length, 4);		/* data length */
	bputdate(b, now);			/* recorded date */
	bputc(b, f);			/* file flags */
	bputc(b, 0);			/* file unit size */
	bputc(b, 0);			/* interleave gap size */
	bputn(b, 1, 2);			/* volume sequence number */
	bputc(b, n);			/* length of file identifier */

	if(dot == DTiden) {		/* identifier */
		if(joliet)
			bputrscvt(b, d->name, n);
		else
			bputs(b, d->name, n);
	}else
	if(dot == DTdotdot)
		bputc(b, 1);
	else
		bputc(b, 0);

	if(b->offset & 1)			/* pad */
		bputc(b, 0);

	/*
	 * plan 9 extension in system use space on end
	 * of each dirsctory. contains the real file
	 * name if the file was not conformal. also
	 * contains string gid and uid and 32-bit mode.
	 */
	if(joliet==0 && plan9 && dot == DTiden) {
		if(d->flags & Fbadname) {
			n = strlen(d->utfname);
			bputc(b, n);
			bputs(b, d->utfname, n);
		} else
			bputc(b, 0);

		n = strlen(d->uid);
		bputc(b, n);
		bputs(b, d->uid, n);

		n = strlen(d->gid);
		bputc(b, n);
		bputs(b, d->gid, n);

		if(b->offset & 1)
			bputc(b, 0);
		bputn(b, d->mode, 4);
	}
}

void
bputdir(Imgbuf *b, Direc *d, int dot)
{
	genputdir(b, d, dot, 0);
}

void
bputjdir(Imgbuf *b, Direc *d, int dot)
{
	genputdir(b, d, dot, 1);
}

/* 
 * Write the primary volume descriptor.
 */
void
bputprivol(Imgbuf *b)
{
	char upvol[33];
	char *p;

	if(p = strrchr(volume, '/'))
		p++;
	else
		p = volume;

	strncpy(upvol, p, 32);
	upvol[32] = '\0';
	strupr(upvol, upvol);

	bputc(b, 1);				/* primary volume descriptor */
	bputs(b, "CD001", 5);			/* standard identifier */
	bputc(b, 1);				/* volume descriptor version */
	bputc(b, 0);				/* unused */

	bputs(b, "PLAN 9", 32);			/* system identifier */
	bputs(b, upvol, 32);			/* volume identifier */

	brepeat(b, 0, 8);				/* unused */
	bputn(b, volsize, 4);			/* volume space size */
	brepeat(b, 0, 32);				/* unused */
	bputn(b, 1, 2);				/* volume set size */
	bputn(b, 1, 2);				/* volume sequence number */
	bputn(b, blocksize, 2);			/* logical block size */
	bputn(b, isopathsize, 4);			/* path table size */
	bputnl(b, isolpathloc, 4);			/* location of Lpath */
	bputnl(b, 0, 4);				/* location of optional Lpath */
	bputnm(b, isompathloc, 4);			/* location of Mpath */
	bputnm(b, 0, 4);				/* location of optional Mpath */
	bputdir(b, &root, DTroot);			/* root directory */
	bputs(b, upvol, 128);		/* volume set identifier */
	bputs(b, "PLAN9", 128);			/* publisher identifier */
	bputs(b, "PLAN9", 128);			/* data preparer identifier */
	bputs(b, "MK9660", 128);		/* application identifier */
	bputs(b, isonotice, 37);			/* copyright notice */
	bputs(b, isoabstract, 37);			/* abstract */
	bputs(b, isobiblio, 37);			/* bibliographic file */
	bputdate1(b, now);				/* volume creation date */
	bputdate1(b, now);				/* volume modification date */
	bputdate1(b, 0);				/* volume expiration date */
	bputdate1(b, 0);				/* volume effective date */
	bputc(b, 1);				/* file structure version */
	bpadblock(b);
}

/*
 * Write a Joliet secondary volume descriptor.
 */
void
bputjolvol(Imgbuf *b)
{
	bputc(b, 2);				/* secondary volume descriptor */
	bputs(b, "CD001", 5);			/* standard identifier */
	bputc(b, 1);				/* volume descriptor version */
	bputc(b, 0);				/* unused */

	bputrscvt(b, "PLAN 9", 32);			/* system identifier */
	bputrscvt(b, volume, 32);			/* volume identifier */

	brepeat(b, 0, 8);				/* unused */
	bputn(b, volsize, 4);			/* volume space size */
	bputc(b, 0x25);				/* escape sequences: UCS-2 Level 2 */
	bputc(b, 0x2F);
	bputc(b, 0x43);

	brepeat(b, 0, 29);
	bputn(b, 1, 2);				/* volume set size */
	bputn(b, 1, 2);				/* volume sequence number */
	bputn(b, blocksize, 2);			/* logical block size */
	bputn(b, jpathsize, 4);			/* path table size */
	bputnl(b, jlpathloc, 4);			/* location of Lpath */
	bputnl(b, 0, 4);				/* location of optional Lpath */
	bputnm(b, jmpathloc, 4);			/* location of Mpath */
	bputnm(b, 0, 4);				/* location of optional Mpath */
	bputjdir(b, &jroot, DTroot);			/* root directory */
	bputrscvt(b, volume, 128);		/* volume set identifier */
	bputrs(b, L"Plan 9", 128);			/* publisher identifier */
	bputrs(b, L"Plan 9", 128);			/* data preparer identifier */
	bputrs(b, L"disk/mk9660", 128);		/* application identifier */
	bputrscvt(b, notice, 37);			/* copyright notice */
	bputrscvt(b, abstract, 37);			/* abstract */
	bputrscvt(b, biblio, 37);			/* bibliographic file */
	bputdate1(b, now);				/* volume creation date */
	bputdate1(b, now);				/* volume modification date */
	bputdate1(b, 0);				/* volume expiration date */
	bputdate1(b, 0);				/* volume effective date */
	bputc(b, 1);				/* file structure version */
	bpadblock(b);
}

static void
phaseset(Imgbuf *b, long *new, long m, char *s, char *t)
{
	if(b->bio && *new != m)
		sysfatal("phase error %s %s: %ld %ld", s, t, *new, m);
if(chatty) fprint(2, "set %s %s = %ld\n", s, t, m);
	*new = m;
}

static void
phaseoff(Imgbuf *b, long *new, char *s)
{
	phaseset(b, new, b->offset/blocksize, "disk offset", s);
}

/*
 * Write boot validation entry.
 */
void
bputvalidationentry(Imgbuf *b)
{
	int sum;

	bputc(b, 1);	/* header id */
	bputc(b, 0);	/* 0 = 386, 1 = PowerPC, 2 = Mac */
	brepeat(b, 0, 2);	/* unused */
	bputs(b, "PLAN9", 5);
	brepeat(b, 0, 24-5);

	/* little endian sum of buffer */
	sum = 0x0001	/* header id */
		+ ('P'|('L'<<8)) + ('A'|('N'<<8)) + '9' + 0xAA55;
	bputnl(b, 0x10000-(sum&0xFFFF), 2);	/* checksum record */
	bputnl(b, 0xAA55, 2);
}

/*
 * Write single boot entry.
 * Assume media is 1.44MB floppy image.
 * Others are possible, including
 * ``Put image here and just jump to it.''
 */
void
bputbootentry(Imgbuf *b)
{
	bputc(b, 0x88);		/* boot indicator; 0x88 = bootable, 0 = not */
	bputc(b, 2);		/* media emulation : 2 = 1.44MB disk */
	bputnl(b, 0, 2);		/* load segment: 0 means default 7C0 */
	bputc(b, 0);		/* fdisk partition type; ignored for floppy */
	bputc(b, 0);		/* unused */
	bputnl(b, 1, 2);	/* no. of virtual/emulated sectors loaded */
	bputnl(b, bootimgloc, 4);
	brepeat(b, 0, 20);
}

/*
 * Write booting catalog.
 */
void
bputbootcat(Imgbuf *b)
{
	phaseoff(b, &bootcatloc, "boot catalog");
	bputvalidationentry(b);
	bputbootentry(b);	/* initial/default only */
	bpadblock(b);
}

/*
 * Write an ``El Torito'' bootable CD descriptor.
 * Must go into sector 17, as per standard.
 */
void
bputbootvol(Imgbuf *b)
{
	assert(b->offset == 17*blocksize);

	bputc(b, 0);
	bputs(b, "CD001", 5);
	bputc(b, 1);
	bputs(b, "EL TORITO SPECIFICATION", 23);
	brepeat(b, 0, 0x40-23);
	bputnl(b, bootcatloc, 4);	/* XXX */
	bpadblock(b);
}

/*
 * Write the boot file.
 */
void
bputbootimage(Imgbuf *b)
{
	Biobuf *bin;
	int n;

	if((bin = Bopen(bootimage, OREAD)) == nil)
		sysfatal("cannot open boot image: %r");

	phaseoff(b, &bootimgloc, "boot image");
	n = 1440*1024;
	if(b->bio) {
		while(n-- > 0)
			bputc(b, Bgetc(bin));
	} else
		b->offset += n;
	Bterm(bin);
	bpadblock(b);
}

void
bputendvol(Imgbuf *b)
{
	bputc(b, 255);				/* volume descriptor set terminator */
	bputs(b, "CD001", 5);			/* standard identifier */
	bputc(b, 1);				/* volume descriptor version */
	bpadblock(b);
}

/* 
 * Write a path entry for the stupid path table.
 */
void
bputpath(Imgbuf *b, Direc *c, int bigendian, int runes)
{
	int n;
	long bno;

	if(runes)
		n = 2*utflen(c->name);
	else
		n = strlen(c->name);

	if(n == 0)	/* root */
		n = 1;

	bputc(b, n);
	bputc(b, 0);
	bno = blockno[c->blockind];
	if(bigendian) {
		bputnm(b, bno, 4);
		bputnm(b, c->parent->pathid, 2);
	} else {
		bputnl(b, bno, 4);
		bputnl(b, c->parent->pathid, 2);
	}
	if(runes)
		bputrscvt(b, c->name, n);
	else
		bputs(b, c->name, n);

	if(n & 1)
		bputc(b, 0);
}

/*
 * Write the stupid path table.
 */
long
bputpathtable(Imgbuf *b, Direc *root, int bigendian, int runes)
{
	Direc *d;
	long n;

	n = b->offset;
	for(d=root; d; d=d->path)
		if(d->mode & CHDIR)
			bputpath(b, d, bigendian, runes);
	n = b->offset - n;
	bpadblock(b);
	return n;
}

/* 
 * Write the contents of the _conform.map file.
 */
void
bputconform(Imgbuf *b, Direc *d)
{
	int i;
	char str[100];

	if(d->flags & Fbadname) {
		sprint(str, "%s %s\n", d->name, d->utfname);
		bputs(b, str, strlen(str));
	}

	for(i=0; i<d->nchild; i++)
		bputconform(b, &d->child[i]);
}

/*
 * Write each non-directory file.
 */
void
bputfiles(Imgbuf *b, Direc *d)
{
	int i;
	long n;
	Biobuf *bin;

	if(d->mode & CHDIR) {
		for(i=0; i<d->nchild; i++)
			bputfiles(b, &d->child[i]);
		return;
	}

	if(d->flags & Fconfmap) {
		blockno[d->blockind] = conformloc;
		return;
	}

	phaseoff(b, &blockno[d->blockind], d->src);

	if((bin = Bopen(d->src, OREAD)) == nil)
		sysfatal("cannot open '%s': %r", d->src);
	if(b->bio == nil)	/* just checking */
		b->offset += d->length;
	else {
		n = d->length;
		while(n-- > 0)
			bputc(b, Bgetc(bin));
	}
	Bterm(bin);
	bpadblock(b);
}

/*
 * Write each directory.
 */
void
bputdirs(Imgbuf *b, Direc *d, void (*put)(Imgbuf*, Direc*, int))
{
	int i;
	long n;

	if((d->mode & CHDIR) == 0)
		return;

	n = b->offset;
	phaseoff(b, &blockno[d->blockind], d->src);
if(chatty) fprint(2, "dir %s block %ld\n", d->src, blockno[d->blockind]);
	(*put)(b, d, DTdot);
	(*put)(b, d->parent, DTdotdot);
	for(i=0; i<d->nchild; i++)
		(*put)(b, &d->child[i], DTiden);
	bpadblock(b);
	phaseset(b, &d->length, b->offset - n, "dir length", d->src);

	for(i=0; i<d->nchild; i++)
		bputdirs(b, &d->child[i], put);
}
		
void
mk9660(Imgbuf *b)
{
	int n;

	b->offset = 0;

	brepeat(b, 0, 16*blocksize);

if(chatty) fprint(2, "%ld primary\n", b->offset);
	bputprivol(b);
if(chatty) fprint(2, "%ld joliet\n", b->offset);
	if(bootimage)
		bputbootvol(b);
	if(joliet)
		bputjolvol(b);
if(chatty) fprint(2, "%ld end\n", b->offset);
	bputendvol(b);

	if(bootimage) {
		bputbootcat(b);
		bputbootimage(b);
	}

	if(conform || joliet) {
		if(chatty) fprint(2, "%ld _conform.map\n", b->offset);
		phaseoff(b, &conformloc, "_conform.map");
		n = b->offset;
		bputconform(b, &root);
		bputconform(b, &jroot);
		phaseset(b, &conformsize, b->offset - n, "size", "_conform.map");
		bpadblock(b);
	}

	if(chatty) fprint(2, "%ld directory\n", b->offset);
	bputdirs(b, &root, bputdir);

	if(joliet) {
		if(chatty) fprint(2, "%ld joliet directory\n", b->offset);
		bputdirs(b, &jroot, bputjdir);
	}

	if(chatty) fprint(2, "%ld files\n", b->offset);
	bputfiles(b, &root);

	if(chatty) fprint(2, "%ld little path\n", b->offset);
	phaseoff(b, &isolpathloc, "little path table");
	isopathsize = bputpathtable(b, &root, 0, 0);
	

	if(chatty) fprint(2, "%ld big path\n", b->offset);
	phaseoff(b, &isompathloc, "big path table");
	bputpathtable(b, &root, 1, 0);

	if(joliet) {
		if(chatty) fprint(2, "%ld joliet little path\n", b->offset);
		phaseoff(b, &jlpathloc, "little path table");
		jpathsize = bputpathtable(b, &jroot, 0, 1);

		if(chatty) fprint(2, "%ld joliet big path\n", b->offset);
		phaseoff(b, &jmpathloc, "big path table");
		bputpathtable(b, &jroot, 1, 1);
	}
		
if(chatty) fprint(2, "%ld volume\n", b->offset);
	phaseoff(b, &volsize, "volume size");
}

void
usage(void)
{
	fprint(2, "usage: disk/mk9660 [-9cj] [-a abstract] [-b biblio] [-n notice] [-s src] [-v volume] [proto]\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	Dir d;
	Direc *direc;
	Biobuf bout;
	Imgbuf img;
	int n;

	ARGBEGIN{
	case '9':
		plan9 = 1;
		break;
	case 'a':
		abstract = ARGF();
		break;
	case 'B':
		bootimage = ARGF();
		break;
	case 'b':
		biblio = ARGF();
		break;
	case 'c':
		conform = 1;
		break;
	case 'j':
		joliet = 1;
		break;
	case 'k':
		blocksize = atoi(ARGF());
		break;
	case 'n':
		notice = ARGF();
		break;
	case 's':
		src = ARGF();
		break;
	case 'v':
		volume = ARGF();
		break;
	case 'D':
		chatty++;
		break;
	default:
		usage();
	}ARGEND

	now = time(0);

	switch(argc){
	case 0:
		proto = "/sys/lib/sysconfig/proto/allproto";
		break;
	case 1:
		proto = argv[0];
		break;
	default:
		usage();
	}

	if(volume == nil)
		volume = proto;

	fmtinstall('P', Pconv);

	if(abstract && strchr(abstract, '/')) {
		fprint(2, "warning: ignoring abstract (must not contain /)\n");
		abstract = nil;
	}
	if(biblio && strchr(biblio, '/')) {
		fprint(2, "warning: ignoring bibliographic file (must not contain /)\n");
		biblio = nil;
	}
	if(notice && strchr(notice, '/')) {
		fprint(2, "warning: ignoring copyright notice (must not contain /)\n");
		notice = nil;
	}

	/* create ISO9660/Plan 9 tree in memory */
	memset(&d, 0, sizeof d);
	strcpy(d.name, "");
	strcpy(d.uid, "sys");
	strcpy(d.gid, "sys");
	d.mode = CHDIR | 0755;
	d.mtime = now;
	mkdirec(&root, &d);
	root.src = src;

	if(conform || joliet) {
		strcpy(d.name, "_conform.map");
		d.mode = 0444;
		direc = adddirec(&root, "_conform.map", &d);
		direc->flags |= Fconfmap;
		direc->src = "<none>";
	}

	if(rdproto(proto, src, addfile, nil, &root) < 0)
		sysfatal("rdproto: %r");

	/* create Joliet tree */
	if(joliet)
		copydirec(&jroot, &root);

	/* file block number locations */
	blockno = emalloc(nblockno*sizeof(blockno[0]));

	if(conform && !joliet && (n=height(&root)) > 8)
		fprint(2, "warning: too many directory levels (%d > 8)\n", n);
	if(abstract && !(found & Fabstract))
		fprint(2, "warning: abstract file '%s' not in proto\n", abstract);
	if(biblio && !(found & Fbiblio))
		fprint(2, "warning: bibliographic file '%s' not in proto\n", biblio);
	if(notice && !(found & Fnotice))
		fprint(2, "warning: notice file '%s' not in proto\n", notice);

	if(conform) {
		checknames(&root, isbadiso9660);
		convertnames(&root, strupr);
	} else
		convertnames(&root, strcpy);

	isoabstract = findconform(&root, abstract);
	isobiblio = findconform(&root, biblio);
	isonotice = findconform(&root, notice);

	dsort(&root, isocmp);
	setparents(&root);
	setpath(&root);
	if(chatty) printdirec(&root, 0);
	if(chatty) printpath(&root);

	if(joliet) {
		jabstract = findconform(&jroot, abstract);
		jbiblio = findconform(&jroot, biblio);
		jnotice = findconform(&jroot, notice);

		checknames(&jroot, isbadjoliet);
		convertnames(&jroot, strcpy);
		dsort(&jroot, jolietcmp);
		setparents(&jroot);
		setpath(&jroot);
		if(chatty) printdirec(&jroot, 0);
		if(chatty) printpath(&root);
	}

	/* test pass to allocate blocks and the like */
	memset(&img, 0, sizeof img);
	img.bio = nil;
	mk9660(&img);

	/* actually write the cd image */
	memset(&img, 0, sizeof img);
	Binit(&bout, 1, OWRITE);
	img.bio = &bout;
	mk9660(&img);
	brepeat(&img, 0, RUNOUT*blocksize);
	Bterm(&bout);
	exits(0);
}
