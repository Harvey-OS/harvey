#include <u.h>
#include <libc.h>
#include <bio.h>

#define BSIZE	8192

typedef struct Dest {
	char *name;
	char *spooldir;
} Dest;

typedef struct Option {
	char *flags;
	char *parms;
} Option;

typedef struct Olist {
	struct Olist *next;
	int min, max;
	int p, n;
} Olist;

extern void	queuefile(char *);
extern void	simplecopy(void);
extern void	indexcopy(void);
extern void	putpage(long);
extern void	putindex(void);
extern long	getpage(void);
extern long	gettrpage(void);
extern int	minuso(char *, Olist **);
extern int	inrange(int, Olist **);
extern int	Open(void);
extern char *	Read(char *);
extern void	checkopts(int, char **);
extern void	putopts(int, char **);
extern Option *	classoptf(int);
extern char *	filestr(char *);
extern Dest *	getdest(char *);
extern void	fatal(char *, char *);

char *dest, *outfile, *buf, username[NAMELEN];

long bufsiz, nchsrc, nchout;

int infd, outfd, seqno;

Biobuf outb;

Olist *olist, *bigolist, *phead, *ptail; int modflag;

int class, debug, proofflag, Gflag, Hflag, Iflag, lflag, nlpage, troff, oddball;

int pageno, pageord;

int Argc; char *Prog, **Argv;

Option 	*classopt;

#define PARGVAL		((*Argv)[2] ? *Argv+2 : Argc>1 ? (--Argc,*++Argv) : (char *)0)

Option 	passopt = { "", "%coOUruW" };
Option 	stopopt = { "DGHIp", "df" };
Option classopts[] = {
	/* b */	"", "m",
	/* c */ "", "lsxy",
	/* d */ "", "xy",
	/* m */ "s", "MCLATtl",
	/* r */ "", "xy",
};

unsigned char optindex[] = {
	/*    a  b  c  d  e  f  g */
	      0, 1, 2, 3, 0, 0, 0,

	/* h  i  j  k  l  m  n  o */
	   0, 0, 0, 0, 0, 4, 0, 0,

	/* p  q  r  s  t  u  v  w */
	   0, 0, 5, 0, 0, 0, 0, 0,

	/* x  y  z  */
	   0, 0, 0,
};

Dest dests[] = {
	"g", "/usr/lbp/spool",
	"h", "/usr/hsb/spool",
	"r", "/usr/rob/spool",
	"t", "/usr/tom/spool",
	"u", "/usr/ulbp/spool",
	0,
};

void
main(int argc, char **argv)
{
	Dest *d;
	char name[128];
	int c, done;

	modflag = -1;
	nlpage = 66;
	if (Prog = strrchr(*argv, '/'))
		++Prog;
	else
		Prog = *argv;
	strcpy(username, getenv("user"));

	if (!(buf = malloc((unsigned int)(bufsiz = 2*BSIZE))))
		fatal("cannot malloc input buffer","");

	if(argc>1 && strncmp(argv[1], "-s", 2) == 0)
		class=argv[1][2], --argc, ++argv;
	Argc = argc; Argv = argv; done = 0;

	if (class != 0 && class != 'c' && class != 'd')
		++oddball;
	else
		class = 'c';
	classopt = classoptf(class);

	while (Argc > 1 && Argv[1][0] == '-' && !done) switch (--Argc, c=(*++Argv)[1]) {
	case '-':
		++done; break;
	case 'd':
		dest = PARGVAL; break;
	case 'p':
		++proofflag; break;
	case 'f':
		outfile = PARGVAL; break;
	case 'G':
		++Gflag; break;
	case 'H':
		++Hflag; break;
	case 'I':
		++Iflag; break;
	case 'l':
		++lflag; nlpage = atoi(PARGVAL); break;
	case '%':
		modflag = atoi(PARGVAL)%2; break;
	case 'D':
		debug++; break;
	case 'o':
		if (minuso(PARGVAL, &olist) >= 0)
			break;
	case 'O':
		if (minuso(PARGVAL, &bigolist) >= 0)
			break;
		fatal("bad option ", Argv[0]);
	default:
		if (strchr(passopt.flags, c) || strchr(classopt->flags, c))
			break;
		if (!strchr(passopt.parms, c) && !strchr(classopt->parms, c))
			fatal("illegal option ", Argv[0]);
		if (Argc > 1 && (*Argv)[2] == 0 ) {
		 	--Argc;
			++Argv;
		}
		break;
	}

	if (!(d = getdest(dest)))
		fatal("unknown destination ", dest);
	if (debug)
		fprint(2, "dest = %s, %s\n",
			d->name, d->spooldir);

	if (debug) {
		Olist *o;
		if (modflag >= 0)
			fprint(2, "modflag = %d\n", modflag);
		for (o=olist; o; o=o->next)
			fprint(2, "o: [%d, %d]\n", o->min, o->max);
		for (o=bigolist; o; o=o->next)
			fprint(2, "O: [%d, %d]\n", o->min, o->max);
	}

	if ((infd = (Argc > 1) ? Open() : 0) < 0)
		exits(0);
	if ((nchsrc = read(infd, buf, BSIZE)) <= 0)
		exits(0);

	if(!oddball){
		int rle=0, bits=0;
		if(!lflag){
			troff = nchsrc >= 4 && strncmp(buf, "x T ", 4) == 0;
			rle = nchsrc >= 9 && strncmp(buf, "TYPE=rle\n", 9) == 0;
			bits = nchsrc >= 8 && strncmp(buf, "TYPE=t6\n", 8) == 0;
			bits |= nchsrc >= 14 && strncmp(buf, "TYPE=ccitt-g4\n", 11) == 0;
		}
		if(troff)
			class = 'd';
		else if(rle)
			class = 'r', oddball=1;
		else if(bits)
			class = 'b', oddball=1;
		else
			class = 'c';
		classopt = classoptf(class);
	}

	if (debug)
		fprint(2, "class = %c, lflag = %d, nlpage = %d, troff = %d\n",
			class, lflag, nlpage, troff);

	checkopts(argc, argv);
	if (outfile) {
		if ((outfd = create(outfile, OWRITE, 0664)) < 0)
			fatal("can't create ", outfile);
	} else {
		queuefile(d->spooldir);
	}
	Binit(&outb, outfd, OWRITE);
	putopts(argc, argv);

	switch (class) {
	case 'c':
	case 'd':
		if (!Iflag) {
			indexcopy();
			putindex();
			break;
		} /* else fall through */
	default:
		simplecopy();
	}
	Bflush(&outb);
	if (outfile) {
		close(outfd);
	} else {
		Dir dir;
		if (dirfstat(outfd, &dir)<0)
			fatal("dirfstat", "");
		sprint(dir.name, "q%5.5d", seqno);
		if (dirfwstat(outfd, &dir)<0)
			fatal("dirfwstat", "");
		close(outfd);
		sprint(name, "%s/!%5.5d", d->spooldir, seqno);
		if ((outfd = create(name, OWRITE, 0666L))<0)
			fatal("can't create ", name);
		close(outfd);
	}
	exits(0);
}

void
queuefile(char *dname)
{
	char name[128]; Dir dir;
	int i, fd; long oseq;

	sprint(name, "%s/ctrl/seqno", dname);


	oseq = 0;
	for (i=0; i<10; i++) {
		if (dirstat(name, &dir) < 0)
			fatal("can't stat ", name);
		oseq = dir.qid.vers;
		if ((fd = open(name, OWRITE|OTRUNC))<0)
			fatal("can't open ", name);
		if (dirstat(name, &dir) < 0)
			fatal("can't re-stat ", name);
		close(fd);
		seqno = dir.qid.vers;
		if (seqno == oseq+1)
			break;
	}
	if (seqno != oseq+1)
		fatal("can't incr ", name);
	
	sprint(name, "%s/.%5.5d", dname, seqno);
	if ((outfd = create(name, OWRITE, 0666L))<0)
		fatal("can't create ", name);
}

void
simplecopy(void)
{
	do {
		Bwrite(&outb, buf, nchsrc);
		nchsrc = 0;
		Read(buf);
	} while (nchsrc > 0);
}

void
indexcopy(void)
{
	long nch;
	if (troff)
		pageord = -1;
	for (;;) {
		++pageord;
		if (troff)
			nch = gettrpage();
		else
			++pageno, nch = getpage();
		if (nch <= 0)
			break;
		if (pageord <= 0) {
			putpage(nch);
		} else if (olist || bigolist) {
			if (inrange(pageno, &olist) || inrange(pageord, &bigolist))
				putpage(nch);
		} else if (modflag < 0 || pageno%2 == modflag)
			putpage(nch);
		memmove(buf, buf+nch, (int)nchsrc);
	}
}

void
putpage(long nch)
{
	Olist *p;

	if (!(p = malloc(sizeof(Olist))))
		fatal("cannot malloc output index", "");
	p->next = 0;
	p->min = nchout;
	p->max = nchout += nch;
	p->p = pageno;
	p->n = pageord;
	if (ptail)
		ptail->next = p;
	else
		phead = p;
	ptail = p;
	Bwrite(&outb, buf, nch);
}

void
putindex(void)
{
	Olist *p;

	Bseek(&outb, 4, 0);
	Bprint(&outb, "-0%13d\n", nchout);
	Bseek(&outb, nchout, 0);
	for (p=phead; p; p=p->next)
		Bprint(&outb, "- %d %d %d %d\n", p->min, p->max, p->p, p->n);
}

long
getpage(void)
{
	char *src = buf;
	int lcount = 0;

	for (;;) {
		if (nchsrc <= 0) {
			src = Read(src);
			if (nchsrc <= 0)
				return src - buf;
		}
		do switch (--nchsrc, *src++) {
		case '\n':
			if (++lcount >= nlpage) {
				if (nchsrc <= 0)
					src = Read(src);
				if (nchsrc > 0 && *src == '\014')
					--nchsrc, ++src;
				return src - buf;
			}
			break;
		case '\014':
			return src - buf;
		} while (nchsrc > 0);
	}
}

long
gettrpage(void)
{
	char *src = buf;
	long lcount = 0;

	for (;;) {
		if (nchsrc <= 0) {
			src = Read(src);
			if (nchsrc <= 0)
				return src - buf;
		}
		do switch (--nchsrc, *src++) {
		case '\n':
			if (lcount++ == 0 && buf[0] == 'p') {
				pageno = atoi(buf+1);
				/*if (debug)
					fprint(2, "\"%8.8s\" %d\n", buf, pageno);*/
			}
			if (nchsrc <= 0)
				src = Read(src);
			if (nchsrc > 0 && *src == 'p')
				return src - buf;
		} while (nchsrc > 0);
	}
}

int
minuso(char *str, Olist **op)
{
	int n = 0, c;
	int dash = 0, min = -999999, digits = 0;
	Olist *o;

	o = 0;
	if (str) do switch (c = *str++) {
	case '0': case '1': case '2': case '3':
	case '4': case '5': case '6': case '7':
	case '8': case '9':
		++digits;
		n = 10*n + c - '0';
		break;
	case '-':
		if (digits)
			min = n;
		++dash; digits = 0; n = 0;
		break;
	case ',':
	case '\0':
		if (dash || digits) {
			o = malloc(sizeof(Olist));
			o->next = *op;
			*op = o;
		}
		if (dash) {
			o->min = min;
			o->max = digits ? n : 999999;
		} else if (digits) {
			o->min = n;
			o->max = n;
		}
		min = -999999;
		dash = n = digits = 0;
		break;
	default:
		return -1;
	} while (c);
	return 0;
}

int
inrange(int n, Olist **op)
{
	Olist *o;
	Olist *prev;

	for (prev=0, o=*op; o; prev=o, o=o->next) {
		if (o->min <= n && n <= o->max) {
			if (prev) {
				prev->next = o->next;
				o->next = *op;
				*op = o;
			}
			return (modflag < 0) || (n%2 == modflag);
		}
	}
	return 0;
}

int
Open(void)
{
	int infd = -1;
	while (--Argc > 0) {
		if (strcmp("-",*++Argv) == 0)
			infd = 0;
		else
			infd = open(*Argv, OREAD);
		if (infd >= 0)
			break;
		fprint(2, "cannot open %s\n", *Argv);
	}
	return infd;
}

char *
Read(char *cur)
{
	char *p = cur + nchsrc;
	int nbuf = p - buf;

	if (bufsiz - nbuf < BSIZE) {
		buf = realloc(buf, (unsigned int)(bufsiz += BSIZE));
		if (!buf)
			fatal("cannot realloc input buffer", "");
		p = buf + nbuf;
		cur = p - nchsrc;
	}
	while ((nbuf = read(infd, p, BSIZE)) <= 0) {
		if (infd)
			close(infd);
		if ((infd = Open()) < 0)
			break;
	}
	if (nbuf > 0)
		nchsrc += nbuf;
	return cur;
}

void
checkopts(int argc, char **argv)
{
	int i, pass, stop;
	char *p;
	while (--argc > 0 && (*++argv)[0] == '-' && (i = (*argv)[1]) != '-') {
		if (strchr(stopopt.flags, i))
			continue;
		if (strchr(passopt.flags, i) || strchr(classopt->flags, i))
			continue;
		stop = strchr(stopopt.parms, i) != 0;
		pass = strchr(passopt.parms, i) || strchr(classopt->parms, i);
		if (!stop && !pass)
			fatal("unknown option ", *argv);
		p = (*argv)[2] ? *argv+2 : argc>1 ? (--argc,*++argv) : 0;
		if (!p)
			fatal("flag requires value: ", *argv);
		if (stop)
			continue;
		if (strchr(p, '\n'))
			fatal("string may not contain newline: ", p);
	}
}

void
putopts(int argc, char **argv)
{
	int i; long t;
	char *p;
	Bprint(&outb, "-?%c\n", class);
	if (!oddball)
		Bprint(&outb, "-0%13.13d\n", 0);
	if (p = filestr("/etc/whoami"))
		Bprint(&outb, "-U%s\n", p);
	if (!Hflag) {
		t = time((long *)0);
		Bprint(&outb, "-W%s", ctime(t));
		Bprint(&outb, "-u%s\n", username);
	}
	while (--argc > 0 && (*++argv)[0] == '-' && (i = (*argv)[1]) != '-') {
		if (strchr(stopopt.flags, i))
			continue;
		if (strchr(passopt.flags, i) || strchr(classopt->flags, i)) {
			Bprint(&outb, "-%c\n", i);
			continue;
		}
		p = (*argv)[2] ? *argv+2 : argc>1 ? (--argc,*++argv) : 0;
		if (strchr(stopopt.parms, i) == 0)
			Bprint(&outb, "-%c%s\n", i, p);
	}
	Bprint(&outb, "--\n");
	nchout = Bseek(&outb, 0, 1);
	if (debug)
		fprint(2, "putopts: nchout=%d\n", nchout);
}

Option *
classoptf(int arg)
{
	char buf[2];
	int i = arg;
	if ((i -= 'a') < 0 || i >= sizeof optindex || !(i = optindex[i])) {
		buf[0] = arg;
		buf[1] = 0;
		fatal("unknown service class ", buf);
	}
	return &classopts[i-1];
}

char *
filestr(char *name)
{
	static char buf[128];
	char *p = 0;
	int fd = open(name, OREAD), n;
	if (fd < 0)
		return 0;
	if ((n = read(fd, buf, sizeof buf-1)) <= 1)
		goto out;
	buf[n] = 0;
	if (!(p = strchr(buf, '\n')))
		goto out;
	*p = 0;
	p = buf;
out:
	close(fd);
	return p;
}

Dest *
getdest(char *dname)
{
	Dest *d;
	if (!dname)
		dname = getenv("candest");
	if (!dname)
		dname = filestr("/sys/lib/candest");
	if (!dname)
		dname = "g";
	for (d = dests; d->name; d++)
		if (strcmp(dname, d->name) == 0)
			break;
	if (!d->name)
		return 0;
	if (proofflag) {
		d->spooldir = malloc(64);
		sprint(d->spooldir, "/usr/%s/pspool", username);
	} else if (Gflag) {
		d->spooldir = malloc(64);
		sprint(d->spooldir, "/usr/%s/spool", username);
	}
	return d;
}

void
fatal(char *str1, char *str2)
{
	fprint(2, "%s: %s%s\n", Prog, str1, str2);
	exits("gcan_go_boom");
}
