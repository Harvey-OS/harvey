/*
 * cdb --
 *	read ascii chess games in either
 *	individual or archive formats and
 *	write 16-bit move binary format
 *	m.out.
 *	-o option will add opening
 *	-f option will use this filename for 'file:' line
 */

#include	<u.h>
#include	<libc.h>
#include	<ctype.h>
#include	<bio.h>
#include	<ar.h>
#include	"../gen.h"

#define	STRSIZE	200
#define	offsetof(t,x)	((ulong)&((t*)0)->x)

char	openfile[]	= "/lib/chess/opening.m.out";

typedef	struct	String	String;
typedef	struct	Open	Open;
typedef	struct	ar_hdr	Ahdr;

struct	String
{
	char	file[STRSIZE];
	char	white[STRSIZE];
	char	black[STRSIZE];
	char	event[STRSIZE];
	char	result[STRSIZE];
	char	opening[STRSIZE];
	char	date[STRSIZE];
};
struct	Open
{
	ulong	xa;
	ulong	xb;
	char*	book;
};

Ahdr	arhdr;
ulong	blposxa;
ulong	blposxb;
ulong	btmxora;
ulong	btmxorb;
Biobuf	bufi;
Biobuf	bufo;
long	fisize;
ulong	initpxora;
ulong	initpxorb;
char*	lastbook;
int	lastposn;
char	line[STRSIZE];
char*	lp;
long	lineno;
int	mverror;
int	nfiles;
int	ngames;
int	oflag;
int	fflag;
long	opencur;
Open*	opening;
long	openmax;
String	strings;
int	subfile;
ulong	whposxa;
ulong	whposxb;
char	word[STRSIZE];
char*	wp;
char	xbd[64];
ulong	xora[16*64];
ulong	xorb[16*64];
char	xpc[32];

void	doxor(int, int);
void	flushgame(char*);
int	getshort(void);
int	getstr(char*);
void	initopen(void);
void	install(char*);
int	istr(char*);
int	ocmp(Open*, Open*);
void	putshort(int);
void	putstr(char*);
int	rline(void);
int	rword(void);
ulong	urand(void);
void	xorinit(void);
void	xormove(int);

void
main(int argc, char* argv[])
{
	int i, fi, fo;
	char fname[200];

	chessinit('G', 0, 2);
	fo = create("m.out", OWRITE, 0666);
	Binit(&bufo, fo, OWRITE);

	ARGBEGIN {
	case 'o':
		oflag++;
		initopen();
		break;
	case 'f':
		fflag++;
		break;
	default:
		fprint(2, "usage: cdb [-o] files\n");
		exits("flag");
	} ARGEND

	for(i=0; i<argc; i++) {
		fi = open(argv[i], 0);
		if(fi < 0) {
			print("cannot open %s\n", argv[i]);
			continue;
		}
		Binit(&bufi, fi, OREAD);
		Bread(&bufi, line, SARMAG);
		if(memcmp(line, ARMAG, SARMAG)) {
			subfile = 0;
			Bseek(&bufi, 0, 0);
			fisize = -1;
			install(argv[i]);
			close(fi);
			continue;
		}
		print("archive %s\n", argv[i]);
		nfiles = 0;
		ngames = 0;
		subfile = -1;
		for(;;) {
			memset(arhdr.fmag, 0, sizeof(arhdr.fmag));
			Bread(&bufi, &arhdr, sizeof(arhdr));
			if(memcmp(arhdr.fmag, ARFMAG, sizeof(arhdr.fmag)))
				break;
			strcpy(fname, argv[i]);
			strcat(fname, ":");
			arhdr.name[sizeof(arhdr.name)-1] = ' ';
			*strchr(arhdr.name, ' ') = 0;
			strcat(fname, arhdr.name);
			fisize = atol(arhdr.size);
			if(fisize & 1)
				fisize++;
			install(fname);
			while(!rline())
				;
			nfiles++;
		}
		if(nfiles != ngames)
			print("%d files and %d games\n", nfiles, ngames);
		close(fi);
	}
	Bflush(&bufo);
	exits(0);
}

void
install(char *fname)
{
	int c, m;
	long k, k1, k2;
	Open *o;

	memset(&strings, 0, sizeof(strings));
	ginit(&initp);
	whposxa = initpxora;
	whposxb = initpxorb;
	lastbook = 0;
	while(utfrune(fname, '/'))
		fname = utfrune(fname, '/')+1;
	lineno = 0;

loop:
	if(rline()) {
		flushgame(fname);
		return;
	}
	lineno++;
	c = *lp;
	switch(c) {
	case 'f':
		if(istr("file: ")) {
			flushgame(fname);
			if(strings.file[0])
				print("%s %ld: two file\n", fname, lineno);
			strcpy(strings.file, lp);
			goto loop;
		}
		break;
	case 'w':
		if(istr("white: ")) {
			flushgame(fname);
			if(strings.white[0])
				print("%s %ld: two whites\n", fname, lineno);
			strcpy(strings.white, lp);
			goto loop;
		}
		break;
	case 'b':
		if(istr("black: ")) {
			flushgame(fname);
			if(strings.black[0])
				print("%s: two blacks\n", fname, lineno);
			strcpy(strings.black, lp);
			goto loop;
		}
		break;
	case 'e':
		if(istr("event: ")) {
			flushgame(fname);
			if(strings.event[0])
				print("%s %ld: two events\n", fname, lineno);
			strcpy(strings.event, lp);
			goto loop;
		}
		break;
	case 'r':
		if(istr("result: ")) {
			flushgame(fname);
			if(strings.result[0])
				print("%s %ld: two results\n", fname, lineno);
			strcpy(strings.result, lp);
			goto loop;
		}
		break;
	case 'o':
		if(istr("opening: ")) {
			flushgame(fname);
			if(strings.opening[0])
				print("%s %ld: two openings\n", fname, lineno);
			strcpy(strings.opening, lp);
			goto loop;
		}
		break;
	}
	for(;;) {
		if(rword())
			goto loop;
		c = *wp;
		while((c>='0' && c<='9') || c == '.') {
			wp++;
			c = *wp;
		}
		if(c == 0)
			continue;
		m = chessin(wp);
		if(m == 0) {
			if(!mverror) {
				print("%s %ld: illegal move %s\n", fname, lineno, word);
				mverror = 1;
			}
			continue;
		}
		if(oflag) {
			xormove(m);
			move(m);
			k1 = 0;
			k2 = opencur;
			for(;;) {
				k = (k1+k2) / 2;
				o = &opening[k];
				if(k <= k1)
					break;
				if(whposxa > o->xa) {
					k1 = k;
					continue;
				}
				if(whposxa < o->xa) {
					k2 = k;
					continue;
				}
				if(whposxb >= o->xb) {
					k1 = k;
					continue;
				}
				k2 = k;
			}
			if(whposxa == o->xa)
			if(whposxb == o->xb) {
				lastbook = o->book;
				lastposn = mvno;
			}
		} else
			move(m);
	}
}

void
flushgame(char *fname)
{
	int i, m, c, pos;
	char *book;

	if(!mverror && moveno == 0)
		return;
	if(!strings.file[0] || fflag)
		if(subfile >= 0)
			sprint(strings.file, "%s:%d", fname, subfile);
		else
			sprint(strings.file, "%s", fname);
	if(!strings.white[0]) {
		print("%s %ld: white missing\n", fname, lineno);
		sprint(strings.white, "none");
	}
	if(!strings.black[0]) {
		print("%s %ld: black missing\n", fname, lineno);
		sprint(strings.black, "none");
	}
	if(!strings.event[0]) {
		print("%s %ld: event missing\n", fname, lineno);
		sprint(strings.event, "none");
	}
	if(!strings.result[0]) {
		print("%s %ld: result missing\n", fname, lineno);
		sprint(strings.result, "none");
	}
	if(mverror)
		strcat(strings.result, " error");
	if(oflag) {
		book = "A00/00";
		pos = 0;
		if(lastbook) {
			book = lastbook;
			pos = lastposn;
		}
		sprint(strings.opening, "%s %d", book, (pos+2)/2);
		book = strchr(strings.opening, 0);
		if(pos & 1)
			*book++ = '.';
		m = mvno;
		while(mvno > pos)
			rmove();
		if(mvno < m)
			sprint(book, " %G", mvlist[mvno]);
		else
			sprint(book, " *");
		while(mvno < m)
			move(mvlist[mvno]);
	}
	if(!strings.opening[0]) {
		print("%s %ld: opening missing\n", fname, lineno);
		sprint(strings.opening, "none");
	}
	m = strlen(strings.event);
	if(m <= 4)
		goto nodate;
	for(i=1; i<=4; i++) {
		c = strings.event[m-i];
		if(c < '0' || c > '9')
			goto nodate;
	}
	if(strings.event[m-5] != ' ')
		goto nodate;
	strcpy(strings.date, strings.event+m-4);
	for(i=1; i<=5; i++)
		strings.event[m-i] = 0;

nodate:
	m = 7;
	m += strlen(strings.file);
	m += strlen(strings.white);
	m += strlen(strings.black);
	m += strlen(strings.event);
	m += strlen(strings.result);
	m += strlen(strings.opening);
	m += strlen(strings.date);
	i = m;
	if(i & 1)
		i++;
	putshort(i/sizeof(short));		/* shorts worth of strings */

	putstr(strings.file);
	putstr(strings.white);
	putstr(strings.black);
	putstr(strings.event);
	putstr(strings.result);
	putstr(strings.opening);
	putstr(strings.date);
	if(m & 1)
		putstr("");
	putshort(mvno);		/* shorts worth of moves */
	for(i=0; i<mvno; i++)
		putshort(mvlist[i]);

	ngames++;

out:
	memset(&strings, 0, sizeof(strings));
	ginit(&initp);
	whposxa = initpxora;
	whposxb = initpxorb;
	lastbook = 0;
	mverror = 0;
	if(subfile >= 0)
		subfile++;
}

int
rline(void)
{
	int c;

	lp = line;
	for(;;) {
		if(fisize == 0)
			return 1;
		fisize--;
		c = Bgetc(&bufi);
		if(c == '\n' || c == '\t') {
			*lp = 0;
			lp = line;
			return 0;
		}
		if(c <= 0) {
			fisize = 0;
			return 1;
		}
		*lp++ = c;
	}
	return 0;
}

int
rword(void)
{
	int c;

	wp = word;
	for(;;) {
		c = *lp;
		if(c == 0)
			return 1;
		if(c != ' ' && c != '\t')
			break;
		lp++;
	}
	for(;;) {
		c = *lp;
		if(c == 0)
			break;
		if(c == ' ' || c == '\t')
			break;
		*wp++ = c;
		lp++;
	}
	*wp = 0;
	wp = word;
	return 0;
}

int
istr(char *s)
{
	char *c;

	for(c = lp; *s;)
		if(*s++ != *c++)
			return 0;
	lp = c;
	return 1;
}

void
putstr(char *s)
{
	int c;

	do {
		c = *s++;
		Bputc(&bufo, c);
	} while(c);
}

void
putshort(int h)
{

	Bputc(&bufo, h>>8);
	Bputc(&bufo, h);
}

ulong
urand(void)
{
	ulong x;

	x = lrand() >> 8;
	x ^= lrand() << 8;
	return x;
}

void
xorinit(void)
{
	int i, j;

	for(i=0; i<nelem(xora); i++) {
		xora[i] = urand();
		xorb[i] = urand();
	}
	btmxora = urand();
	btmxorb = urand();

	for(i=0; i<sizeof(xpc); i++) {
		xpc[i] = 0;
		if(i & 7)
			xpc[i] = (i&017)^BLACK;
	}
	for(i=0; i<sizeof(xbd); i++)
		xbd[i] = i^070;

	ginit(&initp);
	initpxora = 0;
	initpxorb = 0;
	for(i=0; i<64; i++) {
		j = ((board[i]&017)<<6) | i;
		initpxora ^= xora[j];
		initpxorb ^= xorb[j];
	}
}

void
doxor(int p, int at)
{
	int i;

	p &= 017;

	i = (p<<6) | at;
	whposxa ^= xora[i];
	whposxb ^= xorb[i];

	i = (xpc[p]<<6) | xbd[at];
	blposxa ^= xora[i];
	blposxb ^= xorb[i];
}

void
xormove(int mov)
{
	int from, to, typ, p;

	whposxa ^= btmxora;
	whposxb ^= btmxorb;
	blposxa ^= btmxora;
	blposxb ^= btmxorb;

	from = (mov>>6) & 077;
	to = (mov>>0) & 077;

	p = board[from] & 017;
	doxor(board[to], to);
	doxor(p, from);
	doxor(p, to);
	doxor(0, from);

	typ = (mov>>12) & 017;
	switch(typ) {
	case TNPRO:
		doxor(p, to);
		doxor(WKNIGHT | (p&BLACK), to);
		break;
	case TBPRO:
		doxor(p, to);
		doxor(WBISHOP | (p&BLACK), to);
		break;
	case TRPRO:
		doxor(p, to);
		doxor(WROOK | (p&BLACK), to);
		break;
	case TQPRO:
		doxor(p, to);
		doxor(WQUEEN | (p&BLACK), to);
		break;
	case TOO:
		from = to+1;
		to--;
		goto mv;
	case TOOO:
		from = to-2;
		to++;
	mv:
		p = board[from] & 017;
		doxor(board[to], to);
		doxor(p, from);
		doxor(p, to);
		doxor(0, from);
		break;
	case TENPAS:
		if(moveno & 1)
			to -= 8;
		else
			to += 8;
		doxor(board[to], to);
		doxor(0, to);
	}
}

int
ocmp(Open *a, Open *b)
{
	if(a->xa > b->xa)
		return 1;
	if(a->xa < b->xa)
		return -1;
	if(a->xb > b->xb)
		return 1;
	if(a->xb < b->xb)
		return -1;
	return strcmp(a->book, b->book);
}

void
initopen(void)
{
	int f, n, m, i;
	char *book, *bookr;
	Open *o, *oa, *oe;

	print("start oinit ...");
	xorinit();
	f = open(openfile, OREAD);
	if(f < 0) {
		fprint(2, "cannot open %s\n", openfile);
		exits("open");
	}
	Binit(&bufi, f, OREAD);

	openmax = 416100;
	opencur = 0;
	opening = malloc(openmax*sizeof(Open));
	o = &opening[opencur];
	for(;;) {
		n = getshort();
		if(n < 0)
			break;
		m  = getstr(strings.file);
		m += getstr(strings.white);
		m += getstr(strings.black);
		m += getstr(strings.event);
		m += getstr(strings.result);
		m += getstr(strings.opening);
		m += getstr(strings.date);
		if(m & 1) {
			Bgetc(&bufi);
			m++;
		}
		if(n != m/sizeof(short)) {
			fprint(2, "sanity check\n");
			exits("sanity");
		}

		book = strdup(strings.event);
		strcat(strings.event, " cr");
		bookr = strdup(strings.event);

		n = getshort();
		opencur += n*2;
		if(opencur >= openmax) {
			fprint(2, "had to realloc\n");
			exits("realloc");
		}
		ginit(&initp);
		whposxa = initpxora;
		whposxb = initpxorb;
		blposxa = whposxa ^ btmxora;
		blposxb = whposxb ^ btmxorb;
		for(i=0; i<n; i++) {
			m = getshort();
			xormove(m);
			move(m);
			o->xa = whposxa;
			o->xb = whposxb;
			o->book = book;
			o++;
			o->xa = blposxa;
			o->xb = blposxb;
			o->book = bookr;
			o++;
		}
	}
	close(f);

	qsort(opening, opencur, sizeof(Open), ocmp);
	oa = opening;
	oe = oa + opencur;
	for(o=oa+1; o<oe; o++)
		if(o->xa != oa->xa ||
		   o->xb != oa->xb) {
			oa++;
			*oa = *o;
		}
	opencur = oa - opening;
	print("\n");
}

int
getshort(void)
{
	int c1, c2;

	c1 = Bgetc(&bufi);
	if(c1 < 0)
		return c1;
	c2 = Bgetc(&bufi);
	if(c2 < 0)
		return c2;
	return (c1<<8) | c2;
}

int
getstr(char *s)
{
	int c;
	char *is;

	is = s;
	for(;;) {
		c = Bgetc(&bufi);
		if(c < 0) {
			fprint(2, "eof in string\n");
			exits("eof");
		}
		*s++ = c;
		if(c == 0)
			return s-is;
	}
}
