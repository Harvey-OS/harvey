/*
 * n1.c
 *
 *	consume options, initialization, main loop,
 *	input routines, escape function calling
 */

#include "tdef.h"
#include "fns.h"
#include "ext.h"

#include <setjmp.h>
#include <time.h>

int	TROFF	= 1;	/* assume we started in troff... */

jmp_buf sjbuf;
Offset	ipl[NSO];

static	FILE	*ifile	= stdin;
static	FILE	*ifl[NSO];	/* open input file pointers */
char	cfname[NSO+1][NS] = {  "stdin" };	/* file name stack */
int	cfline[NSO];		/* input line count stack */
char	*progname;		/* program name (troff or nroff) */

char	*Version	= "Jan 28, 1992";



main(int argc, char *argv[])
{
	char *p;
	int j;
	Tchar i;
	int eileenct;		/* count to test for "Eileen's loop" */
	char **oargv;
	char buf[100];

	progname = argv[0];
	TROFF = 1;
	if (*progname == 'n')
		TROFF = 0;
	alphabet = 128;	/* unicode for plan 9 */
	oargv = argv;
	mrehash();
	nrehash();
	init0();

	while (--argc > 0 && (++argv)[0][0] == '-')
		switch (argv[0][1]) {

		case 'N':	/* ought to be used first... */
			TROFF = 0;
			break;
		case 'd':
			fprintf(stderr, "troff/nroff version %s\n", Version);
			break;
		case 'F':	/* switch font tables from default */
			if (argv[0][2] != '\0') {
				strcpy(termtab, &argv[0][2]);
				strcpy(fontdir, &argv[0][2]);
			} else {
				argv++; argc--;
				strcpy(termtab, argv[0]);
				strcpy(fontdir, argv[0]);
			}
			break;
		case 0:
			goto start;
		case 'i':
			stdi++;
			break;
		case 'n':
			npn = atoi(&argv[0][2]);
			break;
		case 'u':	/* set emboldening amount */
			bdtab[3] = atoi(&argv[0][2]);
			if (bdtab[3] < 0 || bdtab[3] > 50)
				bdtab[3] = 0;
			break;
		case 's':
			if (!(stop = atoi(&argv[0][2])))
				stop++;
			break;
		case 'r':
			sprintf(buf + strlen(buf), ".nr %c %s\n",
				argv[0][2], &argv[0][3]);
			cpushback(buf);
			/* dotnr(&argv[0][2], &argv[0][3]); */
			break;
		case 'm':
			if (mflg++ >= NMF) {
				ERROR "Too many macro packages: %s", argv[0] WARN;
				break;
			}
			strcpy(mfiles[nmfi], nextf);
			strcat(mfiles[nmfi++], &argv[0][2]);
			break;
		case 'o':
			getpn(&argv[0][2]);
			break;
		case 'T':
			strcpy(devname, &argv[0][2]);
			dotT++;
			break;
		case 'a':
			ascii = 1;
			break;
		case 'h':
			hflg++;
			break;
		case 'e':
			eqflg++;
			break;
		case 'q':
			quiet++;
			save_tty();
			break;
		default:
			ERROR "unknown option %s", argv[0] WARN;
			done(02);
		}

start:
	argp = argv;
	rargc = argc;
	nmfi = 0;
	init2();
	setjmp(sjbuf);
	eileenct = 0;		/*reset count for "Eileen's loop"*/
				/* this is a really sick bit of code */
				/* but i've never been able to figure out */
				/* how to fix it. */
loop:
	copyf = lgf = nb = nflush = nlflg = 0;
	if (ip && rbf0(ip) == 0 && ejf && frame->pframe <= ejl) {
		nflush++;
		trap = 0;
		eject((Stack *)0);
		if (eileenct > 20) {
			ERROR "job looping; check abuse of macros" WARN;
			ejf = 0;	/* try to break Eileen's loop */
			eileenct = 0;
		} else
			eileenct++;
		goto loop;
	}
	eileenct = 0;		/*reset count for "Eileen's loop"*/
	i = getch();
	if (pendt)
		goto Lt;
	if ((j = cbits(i)) == XPAR) {
		copyf++;
		tflg++;
		while (cbits(i) != '\n')
			pchar(i = getch());
		tflg = 0;
		copyf--;
		goto loop;
	}
	if (j == cc || j == c2) {
		if (j == c2)
			nb++;
		copyf++;
		while ((j = cbits(i = getch())) == ' ' || j == '\t')
			;
		ch = i;
		copyf--;
		control(getrq(), 1);
		flushi();
		goto loop;
	}
Lt:
	ch = i;
	text();
	if (nlflg)
		numtab[HP].val = 0;
	goto loop;
}



void init0(void)
{
	numtab[NL].val = -1;
}


void init2(void)
{
	int i;
	char buf[100];

	for (i = NTRTAB; --i; )
		trtab[i] = i;
	trtab[UNPAD] = ' ';
	iflg = 0;
	obufp = obuf;
	if (TROFF)
		t_ptinit();
	else
		n_ptinit();
	mchbits();
	cvtime();
	numtab[PID].val = getpid();
	olinep = oline;
	numtab[HP].val = init = 0;
	numtab[NL].val = -1;
	nfo = 0;
	copyf = raw = 0;
	sprintf(buf, ".ds .T %s\n", devname);
	cpushback(buf);
	numtab[CD].val = -1;	/* compensation */
	nx = mflg;
	frame = stk = (Stack *)setbrk(STACKSIZE);
	dip = &d[0];
	nxf = frame + 1;
	for (i = 1; i < NEV; i++)	/* propagate the environment */
		envcopy(&env[i], &env[0]);
	blockinit();
}

void cvtime(void)
{
	long tt;
	struct tm *ltime;

	time(&tt);
	ltime = localtime(&tt);
	numtab[YR].val = ltime->tm_year;
	numtab[MO].val = ltime->tm_mon + 1;	/* troff uses 1..12 */
	numtab[DY].val = ltime->tm_mday;
	numtab[DW].val = ltime->tm_wday + 1;	/* troff uses 1..7 */
}



char	errbuf[200];

void errprint(void)	/* error message printer */
{
	fprintf(stderr, "%s: ", progname);
	fputs(errbuf, stderr);
	if (numtab[CD].val > 0)
		fprintf(stderr, "; line %d, file %s", numtab[CD].val, cfname[ifi]);
	fputs("\n", stderr);
	stackdump();
}


int control(int a, int b)
{
	int j;

	if (a == 0 || (j = findmn(a)) == -1)
		return(0);
	if (contab[j].f == 0) {
		nxf->nargs = 0;
		if (b)
			collect();
		flushi();
		return pushi(contab[j].mx, a);	/* BUG??? all that matters is 0/!0 */
	}
	if (b)
		 (*contab[j].f)();
	return(0);
}


int getrq(void)
{
	int i, j;

	if ((i = getach()) == 0 || (j = getach()) == 0)
		goto rtn;
	i = PAIR(i, j);
rtn:
	return(i);
}

/*
 * table encodes some special characters, to speed up tests
 * in getch, viz FLSS, RPT, f, \b, \n, fc, tabch, ldrch
 */

char gchtab[NCHARS] = {
	000,004,000,000,010,000,000,000, /* fc, ldr */
	001,002,001,000,001,000,000,000, /* \b, tab, nl, RPT */
	000,000,000,000,000,000,000,000,
	000,001,000,001,000,000,000,000, /* FLSS, ESC */
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,001,000, /* f */
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
};

int realcbits(Tchar c)	/* return character bits, or MOTCH if motion */
{
	if (ismot(c))
		return MOTCH;
	else
		return c & 0xFFFF;
}

Tchar getch(void)
{
	int k;
	Tchar i, j;

g0:
	if (ch) {
		i = ch;
		if (cbits(i) == '\n')
			nlflg++;
		ch = 0;
		return(i);
	}

	if (nlflg)
		return('\n');
	i = getch0();
	if (ismot(i))
		return(i);
	k = cbits(i);
	if (k >= sizeof(gchtab)/sizeof(gchtab[0]) || gchtab[k] == 0)	/* nothing special */
		return(i);
	if (k != ESC) {
		if (k == '\n') {
			nlflg++;
			if (ip == 0)
				numtab[CD].val++; /* line number */
			return(k);
		}
		if (k == FLSS) {
			copyf++; 
			raw++;
			i = getch0();
			if (!fi)
				flss = i;
			copyf--; 
			raw--;
			goto g0;
		}
		if (k == RPT) {
			setrpt();
			goto g0;
		}
		if (!copyf) {
			if (k == 'f' && lg && !lgf) {
				i = getlg(i);
				return(i);
			}
			if (k == fc || k == tabch || k == ldrch) {
				if ((i = setfield(k)) == 0)
					goto g0; 
				else 
					return(i);
			}
			if (k == '\b') {
				i = makem(-width(' ' | chbits));
				return(i);
			}
		}
		return(i);
	}

	k = cbits(j = getch0());
	if (ismot(j))
		return(j);

	switch (k) {
	case 'n':	/* number register */
		setn();
		goto g0;
	case '$':	/* argument indicator */
		seta();
		goto g0;
	case '*':	/* string indicator */
		setstr();
		goto g0;
	case '{':	/* LEFT */
		i = LEFT;
		goto gx;
	case '}':	/* RIGHT */
		i = RIGHT;
		goto gx;
	case '"':	/* comment */
		while (cbits(i = getch0()) != '\n')
			;
		nlflg++;
		if (ip == 0)
			numtab[CD].val++;
		return(i);

/* experiment: put it here instead of copy mode */
	case '(':	/* special char name \(xx */
	case 'C':	/* 		\C'...' */
		if ((i = setch(k)) == 0)
			goto g0;
		goto gx;

	case ESC:	/* double backslash */
		i = eschar;
		goto gx;
	case 'e':	/* printable version of current eschar */
		i = PRESC;
		goto gx;
	case '\n':	/* concealed newline */
		goto g0;
	case ' ':	/* unpaddable space */
		i = UNPAD;
		goto gx;
	case '\'':	/* \(aa */
		i = ACUTE;
		goto gx;
	case '`':	/* \(ga */
		i = GRAVE;
		goto gx;
	case '_':	/* \(ul */
		i = UNDERLINE;
		goto gx;
	case '-':	/* current font minus */
		i = MINUS;
		goto gx;
	case '&':	/* filler */
		i = FILLER;
		goto gx;
	case 'c':	/* to be continued */
		i = CONT;
		goto gx;
	case '!':	/* transparent indicator */
		i = XPAR;
		goto gx;
	case 't':	/* tab */
		i = '\t';
		return(i);
	case 'a':	/* leader (SOH) */
/* old:		*pbp++ = LEADER; goto g0; */
		i = LEADER;
		return i;
	case '%':	/* ohc */
		i = OHC;
		return(i);
	case 'g':	/* return format of a number register */
		setaf();	/* should this really be in copy mode??? */
		goto g0;
	case '.':	/* . */
		i = '.';
gx:
		setsfbits(i, sfbits(j));
		return(i);
	}
	if (copyf) {
		*pbp++ = j;
		return(eschar);
	}
	switch (k) {

	case 'f':	/* font indicator */
		setfont(0);
		goto g0;
	case 's':	/* size indicator */
		setps();
		goto g0;
	case 'v':	/* vert mot */
		if (i = vmot()) {
			return(i);
		}
		goto g0;
	case 'h': 	/* horiz mot */
		if (i = hmot())
			return(i);
		goto g0;
	case '|':	/* narrow space */
		if (NROFF)
			goto g0;
		return(makem((int)(EM)/6));
	case '^':	/* half narrow space */
		if (NROFF)
			goto g0;
		return(makem((int)(EM)/12));
	case 'w':	/* width function */
		setwd();
		goto g0;
	case 'p':	/* spread */
		spread++;
		goto g0;
	case 'N':	/* absolute character number */
		if ((i = setabs()) == 0)
			goto g0;
		return i;
	case 'H':	/* character height */
		return(setht());
	case 'S':	/* slant */
		return(setslant());
	case 'z':	/* zero with char */
		return(setz());
	case 'l':	/* hor line */
		setline();
		goto g0;
	case 'L':	/* vert line */
		setvline();
		goto g0;
	case 'D':	/* drawing function */
		setdraw();
		goto g0;
	case 'X':	/* \X'...' for copy through */
		setxon();
		goto g0;
	case 'b':	/* bracket */
		setbra();
		goto g0;
	case 'o':	/* overstrike */
		setov();
		goto g0;
	case 'k':	/* mark hor place */
		if ((k = findr(getsn())) != -1) {
			numtab[k].val = numtab[HP].val;
		}
		goto g0;
	case '0':	/* number space */
		return(makem(width('0' | chbits)));
	case 'x':	/* extra line space */
		if (i = xlss())
			return(i);
		goto g0;
	case 'u':	/* half em up */
	case 'r':	/* full em up */
	case 'd':	/* half em down */
		return(sethl(k));
	default:
		return(j);
	}
	/* NOTREACHED */
}

void setxon(void)	/* \X'...' for copy through */
{
	Tchar xbuf[NC];
	Tchar *i;
	Tchar c;
	int delim, k;

	if (ismot(c = getch()))
		return;
	delim = cbits(c);
	i = xbuf;
	*i++ = XON | chbits;
	while ((k = cbits(c = getch())) != delim && k != '\n' && i < xbuf+NC-1) {
		if (k == ' ')
			setcbits(c, WORDSP);
		*i++ = c | ZBIT;
	}
	*i++ = XOFF | chbits;
	*i = 0;
	pushback(xbuf);
}


char	ifilt[32] = { 0, 001, 002, 003, 0, 005, 006, 007, 010, 011, 012 };

Tchar getch0(void)
{
	int j;
	Tchar i;

again:
	if (pbp > lastpbp)
		i = *--pbp;
	else if (ip) {
		/* i = rbf(); */
		i = rbf0(ip);
		if (i == 0)
			i = rbf();
		else {
			++ip;
			if (pastend(ip)) {
				--ip;
				rbf();
			}
		}
	} else {
		if (donef || ndone)
			done(0);
		if (nx || 1) {	/* BUG: was ibufp >= eibuf, so EOF test is wrong */
			if (nfo == 0) {
g0:
				if (nextfile()) {
					if (ip)
						goto again;
				}
			}
			nx = 0;
			if ((i = get1ch(ifile)) == EOF)
				goto g0;
			if (ip)
				goto again;
		}
g2:
		if (i >= 040)
			goto g4;
		i = ifilt[i];
	}
	if (cbits(i) == IMP && !raw)
		goto again;
	if (i == 0 && !init && !raw) {
		goto again;
	}
g4:
	if (ismot(i))
		return i;
	if (copyf == 0 && sfbits(i) == 0)
		i |= chbits;
	if (cbits(i) == eschar && !raw)
		setcbits(i, ESC);
	return(i);
}

Tchar get1ch(FILE *fp)	/* get one "character" from input, figure out what alphabet */
{
	wchar_t wc;
	char buf[100], *p;
	int i, n, c;

	for (i = 0, p = buf; i < MB_CUR_MAX; i++) {
		if ((c = getc(fp)) == EOF)
			return c;
		*p++ = c;
		if ((n = mbtowc(&wc, buf, p-buf)) >= 0)
			break;
	}
	if (n == 1)	/* real ascii, presumably */
		return wc;
	if (n == 0)
		return p[-1];	/* illegal, but what else to do? */
	if (c == EOF)
		return EOF;
	*p = 0;
	return chadd(buf, MBchar, Install);	/* add name even if haven't seen it */
}

void pushback(Tchar *b)
{
	Tchar *ob = b;

	while (*b++)
		;
	b--;
	while (b > ob && pbp < &pbbuf[NC-3])
		*pbp++ = *--b;
	if (pbp >= &pbbuf[NC-3]) {
		ERROR "pushback overflow" WARN;
		done(2);
	}
}

void cpushback(char *b)
{
	char *ob = b;

	while (*b++)
		;
	b--;
	while (b > ob && pbp < &pbbuf[NC-3])
		*pbp++ = *--b;
	if (pbp >= &pbbuf[NC-3]) {
		ERROR "cpushback overflow" WARN;
		done(2);
	}
}

int nextfile(void)
{
	char *p;

n0:
	if (ifile != stdin)
		fclose(ifile);
	if (ifi > 0) {
		if (popf())
			goto n0; /* popf error */
		return(1);	 /* popf ok */
	}
	if (nx || nmfi < mflg) {
		p = mfiles[nmfi++];
		if (*p != 0)
			goto n1;
	}
	if (rargc-- <= 0) {
		if ((nfo -= mflg) && !stdi)
			done(0);
		nfo++;
		numtab[CD].val = stdi = mflg = 0;
		ifile = stdin;
		strcpy(cfname[ifi], "stdin");
		return(0);
	}
	p = (argp++)[0];
n1:
	numtab[CD].val = 0;
	if (p[0] == '-' && p[1] == 0) {
		ifile = stdin;
		strcpy(cfname[ifi], "stdin");
	} else if ((ifile = fopen(p, "r")) == NULL) {
		ERROR "cannot open file %s", p WARN;
		nfo -= mflg;
		done(02);
	} else
		strcpy(cfname[ifi],p);
	nfo++;
	return(0);
}


popf(void)
{
	--ifi;
	if (ifi < 0) {
		ERROR "popf went negative" WARN;
		return 1;
	}
	numtab[CD].val = cfline[ifi];	/* restore line counter */
	ip = ipl[ifi];			/* input pointer */
	ifile = ifl[ifi];		/* input FILE * */
	return(0);
}


void flushi(void)
{
	if (nflush)
		return;
	ch = 0;
	copyf++;
	while (!nlflg) {
		if (donef && frame == stk)
			break;
		getch();
	}
	copyf--;
}


getach(void)	/* return ascii/alphabetic character */
{
	Tchar i;
	int j;

	lgf++;
	j = cbits(i = getch());
	if (ismot(i) || !isgraph(j)) {
		ch = i;
		j = 0;
	}
	lgf--;
	return j;
}


void casenx(void)
{
	lgf++;
	skip();
	getname();
	nx++;
	if (nmfi > 0)
		nmfi--;
	strcpy(mfiles[nmfi], nextf);
	nextfile();
	nlflg++;
	ip = 0;
	pendt = 0;
	frame = stk;
	nxf = frame + 1;
}


getname(void)
{
	int j, k;
	Tchar i;

	lgf++;
	for (k = 0; k < NS - 1; k++) {
		if (!isgraph(j = cbits(i = getch())))
			break;
		nextf[k] = j;
	}
	nextf[k] = 0;
	ch = i;
	lgf--;
	return(nextf[0]);
}


void caseso(void)
{
	FILE *fp;
	char *p, *q;

	lgf++;
	nextf[0] = 0;
	if (skip() || !getname() || (fp = fopen(nextf, "r")) == NULL || ifi >= NSO) {
		ERROR "can't open file %s", nextf WARN;
		done(02);
	}
	strcpy(cfname[ifi+1], nextf);
	cfline[ifi] = numtab[CD].val;		/*hold line counter*/
	numtab[CD].val = 0;
	flushi();
	ifl[ifi] = ifile;
	ifile = fp;
	ipl[ifi] = ip;
	ip = 0;
	nx++;
	nflush++;
	ifi++;
}

void caself(void)	/* set line number and file */
{
	int n;

	if (skip())
		return;
	n = atoi0();
	cfline[ifi] = numtab[CD].val = n - 2;
	if (skip())
		return;
	if (getname())
		strcpy(cfname[ifi], nextf);
}


void casecf(void)
{	/* copy file without change */
	int	fd, n;
	char	buf[1024];
	extern int hpos, esc, po;

	/* this may not make much sense in nroff... */

	lgf++;
	nextf[0] = 0;
	if (skip() || !getname() || (fd = open(nextf, 0)) < 0) {
		ERROR "can't open file %s", nextf WARN;
		done(02);
	}
	lgf--;

	/* make it into a clean state, be sure that everything is out */
	tbreak();
	hpos = po;
	esc = 0;
	ptesc();	/* to left margin */
	esc = un;
	ptesc();
	ptlead();
	ptps();
	ptfont();
	flusho();
	while ((n = read(fd, buf, sizeof buf)) > 0)
		fwrite(buf, n, 1, ptid);
	close(fd);
	ptps();
	ptfont();
}

void getline(char *s, int n)	/* get rest of input line into s */
{
	int i;

	lgf++;
	copyf++;
	skip();
	for (i = 0; i < n-1; i++)
		if ((s[i] = cbits(getch())) == '\n' || s[i] == RIGHT)
			break;
	s[i] = 0;
	copyf--;
	lgf--;
}

void casesy(void)	/* call system */
{
	char sybuf[NTM];

	getline(sybuf, NTM);
	system(sybuf);
}


void getpn(char *a)
{
	int n, neg;

	if (*a == 0)
		return;
	neg = 0;
	for ( ; *a; a++)
		switch (*a) {
		case '+':
		case ',':
			continue;
		case '-':
			neg = 1;
			continue;
		default:
			n = 0;
			if (isdigit(*a)) {
				do
					n = 10 * n + *a++ - '0';
				while (isdigit(*a));
				a--;
			} else
				n = 9999;
			*pnp++ = neg ? -n : n;
			neg = 0;
			if (pnp >= &pnlist[NPN-2]) {
				ERROR "too many page numbers" WARN;
				done3(-3);
			}
		}
	if (neg)
		*pnp++ = -9999;
	*pnp = -32767;
	print = 0;
	pnp = pnlist;
	if (*pnp != -32767)
		chkpn();
}


void setrpt(void)
{
	Tchar i, j;

	copyf++;
	raw++;
	i = getch0();
	copyf--;
	raw--;
	if ((long) i < 0 || cbits(j = getch0()) == RPT)
		return;
	while (i > 0 && pbp < &pbbuf[NC-3]) {
		i--;
		*pbp++ = j;
	}
}
