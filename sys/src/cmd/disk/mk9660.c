#include	<u.h>
#include	<libc.h>
#include	<bio.h>

/*
 * write a plan9 ISO 9660 CD-Rom image.
 * input is a mkfs file.
 */


typedef	struct	Direc	Direc;

enum
{
	RSIZE	= 2048,
	RUNOUT	= 150,
	MAXSIZE	= 286000-RUNOUT-4,

	DTdot	= 0,
	DTdotdot,
	DTiden,
	DTroot,
};

struct	Direc
{
	char	name[30];	/* name after conversion */
	char	rname[30];	/* real name in unicode */
	char	uid[30];	/* uid - put in system dep part */
	char	gid[30];	/* gid - put in system dep part */
	long	mode;		/* has CHDIR bit */
	long	length;		/* length in bytes */
	long	offset;		/* if file, pointer into input file */
	long	date;

	char	dup;		/* flag that this name has been seen before */
	char	level;		/* hier level */
	char	conf;		/* this is the map file */
	char	noncon;		/* this file real name doesnt conform */
	char	abst;		/* this is the abstract file */
	char	biblio;		/* this is the bibliographic file */
	char	notice;		/* this is the notice file */
	int	path;
	long	blockno;
	Direc*	first;
	Direc*	next;
	Direc*	parent;
	Direc*	chain;
};

char*	afile;
char*	bfile;
Direc*	chain;
char*	confname;
int	conform;
int	extend;
Biobuf*	ibuf;
long	lpathloc;
int	maxlevel;
long	mpathloc;
char*	nfile;
long	now;
Biobuf*	obuf;
long	offset;
char*	ofile;
int	pass;
char	path[500];
long	pathsize;
char*	proto;
Direc*	root;
char	volid[100];

int	Pconv(void*, Fconv*);
void	bputc(int);
void	bputdate(long);
void	bputdate1(long);
void	bputdir(Direc*, int);
void	bputn(long, int);
void	bputnl(long, int);
void	bputnm(long, int);
void	bputs(char*, int);
void	brepeat(int, int);
void	ckpath(Direc*, int);
int	consub(char*);
int	gethdr(void);
Direc*	lookup(void);
void	padtoblock(void);
void	putevol(void);
void	putprivol(int);
void	wrpath(void);

void
main(int argc, char *argv[])
{
	int n;
	char *p;

	confname = "_conform.map";
	ofile = "cd-rom";
	ARGBEGIN {
	default:
		fprint(2, "unknown option: %c\n", ARGC());
		break;
	case 'c':
		conform = 1;
		break;
	case 'e':
		extend = 1;
		break;
	case 'a':
		afile = ARGF();
		break;
	case 'b':
		bfile = ARGF();
		break;
	case 'n':
		nfile = ARGF();
		break;
	case 'o':
		ofile = ARGF();
		break;
	} ARGEND

	fmtinstall('P', Pconv);
	now = time(0);

	if(argc <= 0) {
		print("usage: mk9660 [-c] [-e] [-[abno] file] protofile\n");
		exits("usage");
	}
	proto = argv[0];
	print("%s\n", proto);

	ibuf = Bopen(proto, OREAD);
	if(ibuf == 0) {
		fprint(2, "open %s\n", proto);
		exits("open");
	}

	p = strrchr(proto, '/');
	if(p == 0)
		p = proto-1;
	strcpy(volid, p+1);
	for(p=volid; *p; p++)
		if(*p >= 'a' && *p <= 'z')
			*p += 'A'-'a';

	root = malloc(sizeof(*root));
	memset(root, 0, sizeof(*root));
	root->mode = CHDIR | 0755;
	strcpy(root->uid, "sys");
	strcpy(root->gid, "sys");
	root->parent = root;
	if(conform) {
		root->first = malloc(sizeof(*root->first));
		memset(root->first, 0, sizeof(*root->first));
		root->first->mode = 0444;
		strcpy(root->first->name, confname);
		strcpy(root->first->uid, "sys");
		strcpy(root->first->gid, "sys");
		root->first->parent = root;
		root->first->conf = 1;
		chain = root->first;
	}

	for(;;)
		if(gethdr())
			break;

	pass = 1;	/* allocate the addresses */
	offset = 0;
	brepeat(0, 18*RSIZE);
	ckpath(root, 0);
	wrpath();
	padtoblock();
	n = offset/RSIZE;
	if(maxlevel > 8)
		fprint(2, "too many directory levels: %d\n", maxlevel);
	if(afile)
		fprint(2, "abstract file not found: %s\n", afile);
	if(bfile)
		fprint(2, "bibliographic file not found: %s\n", bfile);
	if(nfile)
		fprint(2, "notice file not found: %s\n", nfile);
	print("size = %d blocks\n", n);
	if(n > MAXSIZE)
		print("thats too big!!\n");

	pass = 2;	/* do the writing */
	obuf = Bopen(ofile, OWRITE);
	if(obuf == 0) {
		fprint(2, "create %s\n", ofile);
		exits("create");
	}
	offset = 0;
	brepeat(0, 16*RSIZE);
	putprivol(n);
	padtoblock();
	putevol();
	padtoblock();
	ckpath(root, 0);
	wrpath();
	padtoblock();
	brepeat(0, RUNOUT*RSIZE);
	padtoblock();

	Bterm(ibuf);
	Bterm(obuf);
	exits(0);
}

int
Pconv(void *o, Fconv *f)
{
	char path[500];
	Direc *d;

	d = *(Direc**)o;
	if(d->parent == d)
		strcpy(path, "");
	else
		sprint(path, "%P/%s", d->parent, d->name);
	strconv(path, f);
	return sizeof(d);
}

void
bputn(long val, int size)
{

	bputnl(val, size);
	bputnm(val, size);
}

void
bputs(char *s, int size)
{
	int n;

	n = size - strlen(s);
	if(n < 0)
		s -= n;
	while(*s)
		bputc(*s++);
	if(n > 0)
		brepeat(' ', n);
}

void
bputc(int c)
{
	offset++;
	if(pass == 2)
		Bputc(obuf, c);
}

void
bputnl(long val, int size)
{
	switch(size) {
	default:
		fprint(2, "bad size in bputnl: %d\n", size);
		exits("bad size");
	case 2:
		bputc(val);
		bputc(val>>8);
		break;
	case 4:
		bputc(val);
		bputc(val>>8);
		bputc(val>>16);
		bputc(val>>24);
		break;
	}
}

void
bputnm(long val, int size)
{
	switch(size) {
	default:
		fprint(2, "bad size in bputnm: %d\n", size);
		exits("bad size");
	case 2:
		bputc(val>>8);
		bputc(val);
		break;
	case 4:
		bputc(val>>24);
		bputc(val>>16);
		bputc(val>>8);
		bputc(val);
		break;
	}
}

Direc*
lookup(void)
{
	char *p, *q;
	Direc *c, *d;
	char path1[sizeof(path)];

	strcpy(path1, path);
	p = path1;
	strcat(p, "/");
	c = root;

loop:
	if(*p == 0)
		return c;
	q = utfrune(p, '/');
	if(q == p) {
		p++;
		goto loop;
	}

	*q++ = 0;
	for(d=c->first; d; d=d->next)
		if(strcmp(d->name, p) == 0) {
			c = d;
			p = q;
			goto loop;
		}
	d = malloc(sizeof(*d));
	memset(d, 0, sizeof(*d));
	strcpy(d->name, p);
	d->chain = chain;
	chain = d;
	d->parent = c;
	d->next = c->first;
	c->first = d;
	c = d;
	p = q;
	goto loop;
}

int
gethdr(void)
{
	int c, i;
	ulong m;
	Direc *d;
	char *p;

	p = Brdline(ibuf, '\n');
	if(p == 0)
		goto bad;
	*strchr(p, '\n') = 0;
	if(strcmp(p, "end of archive") == 0)
		return 1;

	for(i=0;; i++) {
		c = *p++;
		if(c == 0)
			goto bad;
		if(c == ' ')
			break;
		path[i] = c;
	}
	path[i] = 0;

	d = lookup();
	if(d->dup)
		fprint(2, "%s not unique\n", path);
	d->dup = 1;

	m = 0;
	for(i=0;; i++) {
		c = *p++;
		if(c == ' ')
			break;
		if(c < '0' || c > '7')
			goto bad;
		m = (m<<3) | (c-'0');
	}
	d->mode = m;

	for(i=0;; i++) {
		c = *p++;
		if(c == 0)
			goto bad;
		if(c == ' ')
			break;
		d->uid[i] = c;
	}

	for(i=0;; i++) {
		c = *p++;
		if(c == 0)
			goto bad;
		if(c == ' ')
			break;
		d->gid[i] = c;
	}

	m = 0;
	for(i=0;; i++) {
		c = *p++;
		if(c == ' ')
			break;
		if(c < '0' || c > '9')
			goto bad;
		m = (m*10) + (c-'0');
	}
	d->date = m;	/* not used */

	m = 0;
	for(i=0;; i++) {
		c = *p++;
		if(c == 0)
			break;
		if(c < '0' || c > '9')
			goto bad;
		m = (m*10) + (c-'0');
	}
	d->length = m;

	d->offset = Boffset(ibuf);
	Bseek(ibuf, d->length+d->offset, 0);
	return 0;

bad:
	fprint(2, "cant parse archive header\n");
	return 1;
}

int
dcmp(Direc *a, Direc *b)
{

	return strcmp(a->name, b->name);
}

void
dirsort(Direc *c)
{
	Direc l[1000], *d, *e;
	int n;

	n = 0;
	for(d=c->first; d; d=d->next) {
		if(n >= nelem(l)) {
			fprint(2, "too many entries in a directory\n");
			exits("2big");
		}
		l[n++] = *d;
	}
	if(n == 0)
		return;
	qsort(l, n, sizeof(l[0]), dcmp);
	n = 0;
	for(d=c->first; d; d=d->next) {
		l[n].chain = d->chain;
		l[n].next = d->next;
		*d = l[n++];
		for(e=d->first; e; e=e->next)
			e->parent = d;
	}
}

int
consub(char *p)
{
	int c;

	c = *p;
	if(c >= '0' && c <= '9')
		return 1;
	if(c >= 'a' && c <= 'z') {
		*p = c + ('A'-'a');
		return 1;
	}
	if(c == '_')
		return 1;
	return 0;
}

void
doconf(Direc *c)
{
	int n, i;
	char *s;

	strcpy(c->rname, c->name);	/* real name before conversion */
	s = strchr(c->name, '.');

	/*
	 * names without '.'
	 */
	if(s == 0) {
		s = c->name;
		n = strlen(s);

		/*
		 * if length is > 8
		 */
		if(n > 8)
			goto redo;

		/*
		 * if contains any nasty characters
		 */
		for(i=0; i<n; i++) {
			if(consub(s+i))
				continue;
			goto redo;
		}

		/*
		 * if format is [FD]dddddd
		 *	(after conversion)
		 */
		if(n == 7)
		if(s[0] == 'D' || s[0] == 'F') {
			for(i=1; i<n; i++)
				if(s[i] < '0' || s[i] > '9')
					break;
			if(i >= n)
				goto redo;
		}
		return;
	}

	/*
	 * names with '.'
	 * if prefix length is > 8
	 */
	n = s - c->name;
	if(n > 8)
		goto redo;

	/*
	 * if prefix contains any nasty characters
	 */
	s = c->name;
	for(i=0; i<n; i++) {
		if(consub(s+i))
			continue;
		goto redo;
	}

	/*
	 * if suffix length is > 3
	 */
	s += n+1;
	n = strlen(s);
	if(n > 3)
		goto redo;

	/*
	 * if suffix contains any nasty characters
	 */
	for(i=0; i<n; i++) {
		if(consub(s+i))
			continue;
		goto redo;
	}
	return;


	/*
	 * convert to [FD]dddddd
	 */
redo:
	sprint(c->name, "F%.6d", conform);
	if(c->mode & CHDIR)
		c->name[0] = 'D';
	conform++;
	c->noncon = 1;
}

void
ckpath(Direc *c, int lev)
{
	Direc *d;
	long n;

	if(pass == 1) {
		if(c->length && (c->mode & CHDIR)) {
			fprint(2, "%P:both directory and file\n", c);
			if(c->first)
				c->length = 0;
			else
				c->mode &= ~CHDIR;
		}
		if(c->first && !(c->mode & CHDIR)) {
			fprint(2, "%P: mod not set to DIR\n", c);
			c->mode |= CHDIR;
		}
		c->level = lev;
		if(conform)
			for(d=c->first; d; d=d->next) {
				if(lev == 0 && d->first == 0) {
					if(afile && strcmp(d->name, afile) == 0) {
						d->abst = 1;
						afile = 0;
					}
					if(bfile && strcmp(d->name, bfile) == 0) {
						d->biblio = 1;
						bfile = 0;
					}
					if(nfile && strcmp(d->name, nfile) == 0) {
						d->notice = 1;
						nfile = 0;
					}
				}
				doconf(d);
			}
		dirsort(c);
		if(lev > maxlevel)
			maxlevel = lev;
	}

	padtoblock();
	n = offset;
	if(c->conf)
		return;

	if(pass == 2)
		if(c->blockno != n/RSIZE)
			fprint(2, "phase error 1\n");
	c->blockno = n/RSIZE;
	if(c->mode & CHDIR) {
		bputdir(c, DTdot);
		bputdir(c->parent, DTdotdot);
		for(d=c->first; d; d=d->next) {
			if(d->parent != c)
				print("not parent\n");
			bputdir(d, DTiden);
		}
		padtoblock();
		if(pass == 2)
			if(c->length != offset-n)
				fprint(2, "phase error 2\n");
		c->length = offset - n;

		for(d=c->first; d; d=d->next)
			ckpath(d, lev+1);
		return;
	}

	n = c->length;
	offset += n;
	if(pass == 2) {
		Bseek(ibuf, c->offset, 0);
		while(n > 0) {
			Bputc(obuf, Bgetc(ibuf));
			n--;
		}
	}
}

void
padtoblock(void)
{
	int n;

	n = RSIZE - (offset & (RSIZE-1));
	if(n != RSIZE)
		brepeat(0, n);
}

void
brepeat(int c, int n)
{
	int i;

	offset += n;
	if(pass == 2)
		for(i=0; i<n; i++)
			Bputc(obuf, c);
}

void
putprivol(int size)
{
	Direc *af, *bf, *nf, *d;

	bputc(1);				/* primary volume descriptor */
	bputs("CD001", 5);			/* standard identifier */
	bputc(1);				/* volume descriptor version */
	bputc(0);				/* unused */
	bputs("PLAN 9", 32);			/* system identifier */
	bputs(volid, 32);			/* volume identifier */
	brepeat(0, 8);				/* unused */
	bputn(size, 4);				/* volume space size */
	brepeat(0, 32);				/* unused */
	bputn(1, 2);				/* volume set size */
	bputn(1, 2);				/* volume sequence number */
	bputn(RSIZE, 2);			/* logical block size */
	bputn(pathsize, 4);			/* path table size */
	bputnl(lpathloc, 4);			/* location of Lpath */
	bputnl(0, 4);				/* location of optional Lpath */
	bputnm(mpathloc, 4);			/* location of Mpath */
	bputnm(0, 4);				/* location of optional Mpath */
	bputdir(root, DTroot);			/* root directory */
	bputs("ANDREW_bob", 128);		/* volume set identifier */
	bputs("ROB_bob", 128);			/* publisher identifier */
	bputs("KEN_bob", 128);			/* data preparer identifier */
	bputs("PHILW_bob", 128);		/* application identifier */

	af = 0;
	bf = 0;
	nf = 0;
	for(d=chain; d; d=d->chain) {
		if(d->abst)
			af = d;
		if(d->biblio)
			bf = d;
		if(d->notice)
			nf = d;
	}
	if(nf)					/* copyright file */
		bputs(nf->name, 37);
	else
		bputs("", 37);
	if(af)					/* abstract file */
		bputs(af->name, 37);
	else
		bputs("", 37);
	if(bf)					/* biblographic file */
		bputs(bf->name, 37);
	else
		bputs("", 37);

	bputdate1(now);				/* volume creation date */
	bputdate1(now);				/* volume modification date */
	bputdate1(0);				/* volume expiration date */
	bputdate1(0);				/* volume effective date */
	bputc(1);				/* file structure version */
}

void
putevol(void)
{

	bputc(255);				/* volume descriptor set terminator */
	bputs("CD001", 5);			/* standard identifier */
	bputc(1);				/* volume descriptor version */
}

void
bputdir(Direc *d, int dot)
{
	int f, n, l;
	long o;

	f = 0;
	if(d->mode & CHDIR)
		f |= 2;

	n = 1;
	if(dot == DTiden)
		n = strlen(d->name);

	l = 33+n;
	if(l & 1)
		l++;

	if(extend && dot == DTiden) {
		if(d->noncon)
			l += strlen(d->rname);
		l += strlen(d->uid);
		l += strlen(d->gid);
		l += 3+8;
		if(l & 1)
			l++;
	}

	o = offset;
	o = RSIZE - (o & (RSIZE-1));
	if(o < l)
		padtoblock();


	bputc(l);			/* length of directory record */
	bputc(0);			/* extended attribute record length */
	bputn(d->blockno, 4);		/* location of extent */
	bputn(d->length, 4);		/* data length */
	bputdate(now);			/* recorded date */
	bputc(f);			/* file flags */
	bputc(0);			/* file unit size */
	bputc(0);			/* interleave gap size */
	bputn(1, 2);			/* volume sequence number */
	bputc(n);			/* length of file identifier */

	if(dot == DTiden)		/* identifier */
		bputs(d->name, n);
	else
	if(dot == DTdotdot)
		bputc(1);
	else
		bputc(0);

	if(offset & 1)			/* pad */
		bputc(0);

	/*
	 * plan 9 extension in system use space on end
	 * of each dirsctory. contains the real file
	 * name if the file was not conformal. also
	 * contains string gid and uid and 32-bit mode.
	 */
	if(extend && dot == DTiden) {	/* system use */
		if(d->noncon) {
			n = strlen(d->rname);
			bputc(n);
			bputs(d->rname, n);
		} else
			bputc(0);

		n = strlen(d->uid);
		bputc(n);
		bputs(d->uid, n);

		n = strlen(d->gid);
		bputc(n);
		bputs(d->gid, n);

		if(offset & 1)
			bputc(0);
		bputn(d->mode, 4);
	}
}

void
bputdate(long ust)
{
	Tm *tm;

	if(ust == 0) {
		brepeat(0, 7);
		return;
	}
	tm = gmtime(ust);
	bputc(tm->year);
	bputc(tm->mon+1);
	bputc(tm->mday);
	bputc(tm->hour);
	bputc(tm->min);
	bputc(tm->sec);
	bputc(0);
}

void
bputdate1(long ust)
{
	Tm *tm;
	char str[20];

	if(ust == 0) {
		brepeat('0', 16);
		bputc(0);
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
	bputs(str, 16);
	bputc(0);
}

int
pathcmp(void *o1, void *o2)
{
	Direc *a, *b;
	char pn1[500], pn2[500];
	int n;

	a = *(Direc**)o1;
	b = *(Direc**)o2;
	n = a->level - b->level;
	if(n)
		return n;
	sprint(pn1, "%P", a);
	sprint(pn2, "%P", b);
	return strcmp(pn1, pn2);
}

void
dopath(Direc *c, int endian)
{
	int n, i;

	n = strlen(c->name);
	if(n == 0)
		n = 1;
	bputc(n);
	bputc(0);
	if(endian) {
		bputnm(c->blockno, 4);
		bputnm(c->parent->path, 2);
	} else {
		bputnl(c->blockno, 4);
		bputnl(c->parent->path, 2);
	}
	for(i=0; i<n; i++)
		bputc(c->name[i]);
	if(n & 1)
		bputc(0);
}

void
wrpath(void)
{
	Direc *d, *c;
	char str[100];
	Direc *path[1000];
	int i, np;
	long n;

	/*
	 * add file of non-conforming names
	 */
	c = 0;
	if(conform)
		for(d=chain; d; d=d->chain)
			if(d->conf) {
				c = d;
				break;
			}
	if(c) {
		padtoblock();
		n = offset;
		if(pass == 2)
			if(c->blockno != n/RSIZE)
				fprint(2, "phase error 3\n");
		c->blockno = n/RSIZE;
		for(d=chain; d; d=d->chain)
			if(d->noncon) {
				sprint(str, "%s %s\n", d->name, d->rname);
				bputs(str, strlen(str));
			}
		if(pass == 2)
			if(c->length != offset-n)
				fprint(2, "phase error 4\n");
		c->length = offset - n;
	}

	/*
	 * do the path crap
	 */
	np = 0;
	for(d=chain; d; d=d->chain)
		if(d->mode & CHDIR)
			path[np++] = d;
	path[np++] = root;

	qsort(path, np, sizeof(path[0]), pathcmp);
	for(i=0; i<np; i++)
		path[i]->path = i+1;

	padtoblock();
	n = offset;
	lpathloc = n/RSIZE;
	for(i=0; i<np; i++)
		dopath(path[i], 0);
	pathsize = offset - n;

	padtoblock();
	n = offset;
	mpathloc = n/RSIZE;
	for(i=0; i<np; i++)
		dopath(path[i], 1);
}
