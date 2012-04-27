/*
 * du - print disk usage
 */
#include <u.h>
#include <libc.h>
#include <String.h>

extern	vlong	du(char*, Dir*);
extern	void	err(char*);
extern	vlong	blkmultiple(vlong);
extern	int	seen(Dir*);
extern	int	warn(char*);

enum {
	Vkilo = 1024LL,
};

/* rounding up, how many units does amt occupy? */
#define HOWMANY(amt, unit)	(((amt)+(unit)-1) / (unit))
#define ROUNDUP(amt, unit)	(HOWMANY(amt, unit) * (unit))

int	aflag;
int	autoscale;
int	fflag;
int	fltflag;
int	qflag;
int	readflg;
int	sflag;
int	tflag;
int	uflag;

char	*fmt = "%llud\t%q\n";
char	*readbuf;
vlong	blocksize = Vkilo;	/* actually more likely to be 4K or 8K */
vlong	unit;			/* scale factor for output */

static char *pfxes[] = {	/* SI prefixes for units > 1 */
	"",
	"k", "M", "G",
	"T", "P", "E",
	"Z", "Y",
	nil,
};

void
usage(void)
{
	fprint(2, "usage: du [-aefhnqstu] [-b size] [-p si-pfx] [file ...]\n");
	exits("usage");
}

void
printamt(vlong amt, char *name)
{
	if (readflg)
		return;
	if (autoscale) {
		int scale = 0;
		double val = (double)amt/unit;

		while (fabs(val) >= 1024 && scale < nelem(pfxes)-1) {
			scale++;
			val /= 1024;
		}
		print("%.6g%s\t%q\n", val, pfxes[scale], name);
	} else if (fltflag)
		print("%.6g\t%q\n", (double)amt/unit, name);
	else
		print(fmt, HOWMANY(amt, unit), name);
}

void
main(int argc, char *argv[])
{
	int i, scale;
	char *s, *ss, *name;

	doquote = needsrcquote;
	quotefmtinstall();

	ARGBEGIN {
	case 'a':	/* all files */
		aflag = 1;
		break;
	case 'b':	/* block size */
		s = ARGF();
		if(s) {
			blocksize = strtoul(s, &ss, 0);
			if(s == ss)
				blocksize = 1;
			while(*ss++ == 'k')
				blocksize *= 1024;
		}
		break;
	case 'e':	/* print in %g notation */
		fltflag = 1;
		break;
	case 'f':	/* don't print warnings */
		fflag = 1;
		break;
	case 'h':	/* similar to -h in bsd but more precise */
		autoscale = 1;
		break;
	case 'n':	/* all files, number of bytes */
		aflag = 1;
		blocksize = 1;
		unit = 1;
		break;
	case 'p':
		s = ARGF();
		if(s) {
			for (scale = 0; pfxes[scale] != nil; scale++)
				if (cistrcmp(s, pfxes[scale]) == 0)
					break;
			if (pfxes[scale] == nil)
				sysfatal("unknown suffix %s", s);
			unit = 1;
			while (scale-- > 0)
				unit *= Vkilo;
		}
		break;
	case 'q':	/* qid */
		fmt = "%.16llux\t%q\n";
		qflag = 1;
		break;
	case 'r':
		/* undocumented: just read & ignore every block of every file */
		readflg = 1;
		break;
	case 's':	/* only top level */
		sflag = 1;
		break;
	case 't':	/* return modified/accessed time */
		tflag = 1;
		break;
	case 'u':	/* accessed time */
		uflag = 1;
		break;
	default:
		usage();
	} ARGEND

	if (unit == 0)
		if (qflag || tflag || uflag || autoscale)
			unit = 1;
		else
			unit = Vkilo;
	if (blocksize < 1)
		blocksize = 1;

	if (readflg) {
		readbuf = malloc(blocksize);
		if (readbuf == nil)
			sysfatal("out of memory");
	}
	if(argc==0)
		printamt(du(".", dirstat(".")), ".");
	else
		for(i=0; i<argc; i++) {
			name = argv[i];
			printamt(du(name, dirstat(name)), name);
		}
	exits(0);
}

vlong
dirval(Dir *d, vlong size)
{
	if(qflag)
		return d->qid.path;
	else if(tflag) {
		if(uflag)
			return d->atime;
		return d->mtime;
	} else
		return size;
}

void
readfile(char *name)
{
	int n, fd = open(name, OREAD);

	if(fd < 0) {
		warn(name);
		return;
	}
	while ((n = read(fd, readbuf, blocksize)) > 0)
		continue;
	if (n < 0)
		warn(name);
	close(fd);
}

vlong
dufile(char *name, Dir *d)
{
	vlong t = blkmultiple(d->length);

	if(aflag || readflg) {
		String *file = s_copy(name);

		s_append(file, "/");
		s_append(file, d->name);
		if (readflg)
			readfile(s_to_c(file));
		t = dirval(d, t);
		printamt(t, s_to_c(file));
		s_free(file);
	}
	return t;
}

vlong
du(char *name, Dir *dir)
{
	int fd, i, n;
	Dir *buf, *d;
	String *file;
	vlong nk, t;

	if(dir == nil)
		return warn(name);

	if((dir->qid.type&QTDIR) == 0)
		return dirval(dir, blkmultiple(dir->length));

	fd = open(name, OREAD);
	if(fd < 0)
		return warn(name);
	nk = 0;
	while((n=dirread(fd, &buf)) > 0) {
		d = buf;
		for(i = n; i > 0; i--, d++) {
			if((d->qid.type&QTDIR) == 0) {
				nk += dufile(name, d);
				continue;
			}

			if(strcmp(d->name, ".") == 0 ||
			   strcmp(d->name, "..") == 0 ||
			   /* !readflg && */ seen(d))
				continue;	/* don't get stuck */

			file = s_copy(name);
			s_append(file, "/");
			s_append(file, d->name);

			t = du(s_to_c(file), d);

			nk += t;
			t = dirval(d, t);
			if(!sflag)
				printamt(t, s_to_c(file));
			s_free(file);
		}
		free(buf);
	}
	if(n < 0)
		warn(name);
	close(fd);
	return dirval(dir, nk);
}

#define	NCACHE	256	/* must be power of two */

typedef struct
{
	Dir*	cache;
	int	n;
	int	max;
} Cache;
Cache cache[NCACHE];

int
seen(Dir *dir)
{
	Dir *dp;
	int i;
	Cache *c;

	c = &cache[dir->qid.path&(NCACHE-1)];
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
			err("malloc failure");
	}
	c->cache[c->n++] = *dir;
	return 0;
}

void
err(char *s)
{
	fprint(2, "du: %s: %r\n", s);
	exits(s);
}

int
warn(char *s)
{
	if(fflag == 0)
		fprint(2, "du: %s: %r\n", s);
	return 0;
}

/* round up n to nearest block */
vlong
blkmultiple(vlong n)
{
	if(blocksize == 1)		/* no quantization */
		return n;
	return ROUNDUP(n, blocksize);
}
