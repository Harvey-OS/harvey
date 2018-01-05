/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * sed -- stream editor
 */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <regexp.h>

enum {
	DEPTH		= 20,		/* max nesting depth of {} */
	MAXCMDS		= 512,		/* max sed commands */
	ADDSIZE		= 10000,	/* size of add & read buffer */
	MAXADDS		= 20,		/* max pending adds and reads */
	LBSIZE		= 8192,		/* input line size */
	LABSIZE		= 50,		/* max number of labels */
	MAXSUB		= 10,		/* max number of sub reg exp */
	MAXFILES	= 120,		/* max output files */
};

/*
 * An address is a line #, a R.E., "$", a reference to the last
 * R.E., or nothing.
 */
typedef struct {
	enum {
		A_NONE,
		A_DOL,
		A_LINE,
		A_RE,
		A_LAST,
	}type;
	union {
		int32_t	line;		/* Line # */
		Reprog	*rp;		/* Compiled R.E. */
	};
} Addr;

typedef struct	SEDCOM {
	Addr	ad1;			/* optional start address */
	Addr	ad2;			/* optional end address */
	union {
		Reprog	*re1;		/* compiled R.E. */
		Rune	*text;		/* added text or file name */
		struct	SEDCOM	*lb1;	/* destination command of branch */
	};
	Rune	*rhs;			/* Right-hand side of substitution */
	Biobuf*	fcode;			/* File ID for read and write */
	char	command;		/* command code -see below */
	char	gfl;			/* 'Global' flag for substitutions */
	char	pfl;			/* 'print' flag for substitutions */
	char	active;			/* 1 => data between start and end */
	char	negfl;			/* negation flag */
} SedCom;

/* Command Codes for field SedCom.command */
#define ACOM	01
#define BCOM	020
#define CCOM	02
#define	CDCOM	025
#define	CNCOM	022
#define COCOM	017
#define	CPCOM	023
#define DCOM	03
#define ECOM	015
#define EQCOM	013
#define FCOM	016
#define GCOM	027
#define CGCOM	030
#define HCOM	031
#define CHCOM	032
#define ICOM	04
#define LCOM	05
#define NCOM	012
#define PCOM	010
#define QCOM	011
#define RCOM	06
#define SCOM	07
#define TCOM	021
#define WCOM	014
#define	CWCOM	024
#define	YCOM	026
#define XCOM	033

typedef struct label {			/* Label symbol table */
	Rune	uninm[9];		/* Label name */
	SedCom	*chain;
	SedCom	*address;		/* Command associated with label */
} Label;

typedef	struct	FILE_CACHE {		/* Data file control block */
	struct FILE_CACHE *next;	/* Forward Link */
	char	*name;			/* Name of file */
} FileCache;

SedCom pspace[MAXCMDS];			/* Command storage */
SedCom *pend = pspace+MAXCMDS;		/* End of command storage */
SedCom *rep = pspace;			/* Current fill point */

Reprog	*lastre = 0;			/* Last regular expression */
Resub	subexp[MAXSUB];			/* sub-patterns of pattern match*/

Rune	addspace[ADDSIZE];		/* Buffer for a, c, & i commands */
Rune	*addend = addspace+ADDSIZE;

SedCom	*abuf[MAXADDS];			/* Queue of pending adds & reads */
SedCom	**aptr = abuf;

struct {				/* Sed program input control block */
	enum PTYPE { 			/* Either on command line or in file */
		P_ARG,
		P_FILE,
	} type;
	union PCTL {			/* Pointer to data */
		Biobuf	*bp;
		char	*curr;
	} PCTL;
} prog;

Rune	genbuf[LBSIZE];			/* Miscellaneous buffer */

FileCache	*fhead = 0;		/* Head of File Cache Chain */
FileCache	*ftail = 0;		/* Tail of File Cache Chain */

Rune	*loc1;				/* Start of pattern match */
Rune	*loc2;				/* End of pattern match */
Rune	seof;				/* Pattern delimiter char */

Rune	linebuf[LBSIZE+1];		/* Input data buffer */
Rune	*lbend = linebuf+LBSIZE;	/* End of buffer */
Rune	*spend = linebuf;		/* End of input data */
Rune	*cp;				/* Current scan point in linebuf */

Rune	holdsp[LBSIZE+1];		/* Hold buffer */
Rune	*hend = holdsp+LBSIZE;		/* End of hold buffer */
Rune	*hspend = holdsp;		/* End of hold data */

int	nflag;				/* Command line flags */
int	gflag;

int	dolflag;			/* Set when at true EOF */
int	sflag;				/* Set when substitution done */
int	jflag;				/* Set when jump required */
int	delflag;			/* Delete current line when set */

int64_t	lnum = 0;			/* Input line count */

char	fname[MAXFILES][40];		/* File name cache */
Biobuf	*fcode[MAXFILES];		/* File ID cache */
int	nfiles = 0;			/* Cache fill point */

Biobuf	fout;				/* Output stream */
Biobuf	stdin;				/* Default input */
Biobuf*	f = 0;				/* Input data */

Label	ltab[LABSIZE];			/* Label name symbol table */
Label	*labend = ltab+LABSIZE;		/* End of label table */
Label	*lab = ltab+1;			/* Current Fill point */

int	depth = 0;			/* {} stack pointer */

Rune	bad;				/* Dummy err ptr reference */
Rune	*badp = &bad;


char	CGMES[]	 = 	"%S command garbled: %S";
char	TMMES[]	 = 	"Too much text: %S";
char	LTL[]	 = 	"Label too int32_t: %S";
char	AD0MES[] =	"No addresses allowed: %S";
char	AD1MES[] =	"Only one address allowed: %S";

void	address(Addr *);
void	arout(void);
int	cmp(char *, char *);
int	rcmp(Rune *, Rune *);
void	command(SedCom *);
Reprog	*compile(void);
Rune	*compsub(Rune *, Rune *);
void	dechain(void);
void	dosub(Rune *);
int	ecmp(Rune *, Rune *, int);
void	enroll(char *);
void	errexit(void);
int	executable(SedCom *);
void	execute(void);
void	fcomp(void);
int32_t	getrune(void);
Rune	*gline(Rune *);
int	match(Reprog *, Rune *);
void	newfile(enum PTYPE, char *);
int 	opendata(void);
Biobuf	*open_file(char *);
Rune	*place(Rune *, Rune *, Rune *);
void	quit(char *, ...);
int	rline(Rune *, Rune *);
Label	*search(Label *);
int	substitute(SedCom *);
char	*text(char *);
Rune	*stext(Rune *, Rune *);
int	ycomp(SedCom *);
char *	trans(int c);
void	putline(Biobuf *bp, Rune *buf, int n);

void
main(int argc, char **argv)
{
	int compfl;

	lnum = 0;
	Binit(&fout, 1, OWRITE);
	fcode[nfiles++] = &fout;
	compfl = 0;

	if(argc == 1)
		exits(0);
	ARGBEGIN{
	case 'e':
		if (argc <= 1)
			quit("missing pattern");
		newfile(P_ARG, ARGF());
		fcomp();
		compfl = 1;
		continue;
	case 'f':
		if(argc <= 1)
			quit("no pattern-file");
		newfile(P_FILE, ARGF());
		fcomp();
		compfl = 1;
		continue;
	case 'g':
		gflag++;
		continue;
	case 'n':
		nflag++;
		continue;
	default:
		fprint(2, "sed: Unknown flag: %c\n", ARGC());
		continue;
	} ARGEND

	if(compfl == 0) {
		if (--argc < 0)
			quit("missing pattern");
		newfile(P_ARG, *argv++);
		fcomp();
	}

	if(depth)
		quit("Too many {'s");

	ltab[0].address = rep;

	dechain();

	if(argc <= 0)
		enroll(0);		/* Add stdin to cache */
	else
		while(--argc >= 0)
			enroll(*argv++);
	execute();
	exits(0);
}

void
fcomp(void)
{
	int	i;
	Label	*lpt;
	Rune	*tp;
	SedCom	*pt, *pt1;
	static Rune	*p = addspace;
	static SedCom	**cmpend[DEPTH];	/* stack of {} operations */

	while (rline(linebuf, lbend) >= 0) {
		cp = linebuf;
comploop:
		while(*cp == L' ' || *cp == L'\t')
			cp++;
		if(*cp == L'\0' || *cp == L'#')
			continue;
		if(*cp == L';') {
			cp++;
			goto comploop;
		}

		address(&rep->ad1);
		if (rep->ad1.type != A_NONE) {
			if (rep->ad1.type == A_LAST) {
				if (!lastre)
					quit("First RE may not be null");
				rep->ad1.type = A_RE;
				rep->ad1.rp = lastre;
			}
			if(*cp == L',' || *cp == L';') {
				cp++;
				address(&rep->ad2);
				if (rep->ad2.type == A_LAST) {
					rep->ad2.type = A_RE;
					rep->ad2.rp = lastre;
				}
			} else
				rep->ad2.type = A_NONE;
		}
		while(*cp == L' ' || *cp == L'\t')
			cp++;

swit:
		switch(*cp++) {
		default:
			quit("Unrecognized command: %S", linebuf);

		case '!':
			rep->negfl = 1;
			goto swit;

		case '{':
			rep->command = BCOM;
			rep->negfl = !rep->negfl;
			cmpend[depth++] = &rep->lb1;
			if(++rep >= pend)
				quit("Too many commands: %S", linebuf);
			if(*cp == '\0')
				continue;
			goto comploop;

		case '}':
			if(rep->ad1.type != A_NONE)
				quit(AD0MES, linebuf);
			if(--depth < 0)
				quit("Too many }'s");
			*cmpend[depth] = rep;
			if(*cp == 0)
				continue;
			goto comploop;

		case '=':
			rep->command = EQCOM;
			if(rep->ad2.type != A_NONE)
				quit(AD1MES, linebuf);
			break;

		case ':':
			if(rep->ad1.type != A_NONE)
				quit(AD0MES, linebuf);

			while(*cp == L' ')
				cp++;
			tp = lab->uninm;
			while (*cp && *cp != L';' && *cp != L' ' &&
			    *cp != L'\t' && *cp != L'#') {
				*tp++ = *cp++;
				if(tp >= &lab->uninm[8])
					quit(LTL, linebuf);
			}
			*tp = L'\0';

			if (*lab->uninm == L'\0')		/* no label? */
				quit(CGMES, L":", linebuf);
			if((lpt = search(lab)) != nil) {
				if(lpt->address)
					quit("Duplicate labels: %S", linebuf);
			} else {
				lab->chain = 0;
				lpt = lab;
				if(++lab >= labend)
					quit("Too many labels: %S", linebuf);
			}
			lpt->address = rep;
			if (*cp == L'#')
				continue;
			rep--;			/* reuse this slot */
			break;

		case 'a':
			rep->command = ACOM;
			if(rep->ad2.type != A_NONE)
				quit(AD1MES, linebuf);
			if(*cp == L'\\')
				cp++;
			if(*cp++ != L'\n')
				quit(CGMES, L"a", linebuf);
			rep->text = p;
			p = stext(p, addend);
			break;
		case 'c':
			rep->command = CCOM;
			if(*cp == L'\\')
				cp++;
			if(*cp++ != L'\n')
				quit(CGMES, L"c", linebuf);
			rep->text = p;
			p = stext(p, addend);
			break;
		case 'i':
			rep->command = ICOM;
			if(rep->ad2.type != A_NONE)
				quit(AD1MES, linebuf);
			if(*cp == L'\\')
				cp++;
			if(*cp++ != L'\n')
				quit(CGMES, L"i", linebuf);
			rep->text = p;
			p = stext(p, addend);
			break;

		case 'g':
			rep->command = GCOM;
			break;

		case 'G':
			rep->command = CGCOM;
			break;

		case 'h':
			rep->command = HCOM;
			break;

		case 'H':
			rep->command = CHCOM;
			break;

		case 't':
			rep->command = TCOM;
			goto jtcommon;

		case 'b':
			rep->command = BCOM;
jtcommon:
			while(*cp == L' ')
				cp++;
			if(*cp == L'\0' || *cp == L';') {
				/* no label; jump to end */
				if((pt = ltab[0].chain) != nil) {
					while((pt1 = pt->lb1) != nil)
						pt = pt1;
					pt->lb1 = rep;
				} else
					ltab[0].chain = rep;
				break;
			}

			/* copy label into lab->uninm */
			tp = lab->uninm;
			while((*tp = *cp++) != L'\0' && *tp != L';')
				if(++tp >= &lab->uninm[8])
					quit(LTL, linebuf);
			cp--;
			*tp = L'\0';

			if (*lab->uninm == L'\0')
				/* shouldn't get here */
				quit(CGMES, L"b or t", linebuf);
			if((lpt = search(lab)) != nil) {
				if(lpt->address)
					rep->lb1 = lpt->address;
				else {
					for(pt = lpt->chain; pt != nil &&
					    (pt1 = pt->lb1) != nil; pt = pt1)
						;
					if (pt)
						pt->lb1 = rep;
				}
			} else {			/* add new label */
				lab->chain = rep;
				lab->address = 0;
				if(++lab >= labend)
					quit("Too many labels: %S", linebuf);
			}
			break;

		case 'n':
			rep->command = NCOM;
			break;

		case 'N':
			rep->command = CNCOM;
			break;

		case 'p':
			rep->command = PCOM;
			break;

		case 'P':
			rep->command = CPCOM;
			break;

		case 'r':
			rep->command = RCOM;
			if(rep->ad2.type != A_NONE)
				quit(AD1MES, linebuf);
			if(*cp++ != L' ')
				quit(CGMES, L"r", linebuf);
			rep->text = p;
			p = stext(p, addend);
			break;

		case 'd':
			rep->command = DCOM;
			break;

		case 'D':
			rep->command = CDCOM;
			rep->lb1 = pspace;
			break;

		case 'q':
			rep->command = QCOM;
			if(rep->ad2.type != A_NONE)
				quit(AD1MES, linebuf);
			break;

		case 'l':
			rep->command = LCOM;
			break;

		case 's':
			rep->command = SCOM;
			seof = *cp++;
			if ((rep->re1 = compile()) == 0) {
				if(!lastre)
					quit("First RE may not be null.");
				rep->re1 = lastre;
			}
			rep->rhs = p;
			if((p = compsub(p, addend)) == 0)
				quit(CGMES, L"s", linebuf);
			if(*cp == L'g') {
				cp++;
				rep->gfl++;
			} else if(gflag)
				rep->gfl++;

			if(*cp == L'p') {
				cp++;
				rep->pfl = 1;
			}

			if(*cp == L'P') {
				cp++;
				rep->pfl = 2;
			}

			if(*cp == L'w') {
				cp++;
				if(*cp++ !=  L' ')
					quit(CGMES, L"s", linebuf);
				text(fname[nfiles]);
				for(i = nfiles - 1; i >= 0; i--)
					if(cmp(fname[nfiles], fname[i]) == 0) {
						rep->fcode = fcode[i];
						goto done;
					}
				if(nfiles >= MAXFILES)
					quit("Too many files in w commands 1");
				rep->fcode = open_file(fname[nfiles]);
			}
			break;

		case 'w':
			rep->command = WCOM;
			if(*cp++ != L' ')
				quit(CGMES, L"w", linebuf);
			text(fname[nfiles]);
			for(i = nfiles - 1; i >= 0; i--)
				if(cmp(fname[nfiles], fname[i]) == 0) {
					rep->fcode = fcode[i];
					goto done;
				}
			if(nfiles >= MAXFILES){
				fprint(2, "sed: Too many files in w commands 2 \n");
				fprint(2, "nfiles = %d; MAXF = %d\n",
					nfiles, MAXFILES);
				errexit();
			}
			rep->fcode = open_file(fname[nfiles]);
			break;

		case 'x':
			rep->command = XCOM;
			break;

		case 'y':
			rep->command = YCOM;
			seof = *cp++;
			if (ycomp(rep) == 0)
				quit(CGMES, L"y", linebuf);
			break;

		}
done:
		if(++rep >= pend)
			quit("Too many commands, last: %S", linebuf);
		if(*cp++ != L'\0') {
			if(cp[-1] == L';')
				goto comploop;
			quit(CGMES, cp - 1, linebuf);
		}
	}
}

Biobuf *
open_file(char *name)
{
	int fd;
	Biobuf *bp;

	if ((bp = malloc(sizeof(Biobuf))) == 0)
		quit("Out of memory");
	if ((fd = open(name, OWRITE)) < 0 &&
	    (fd = create(name, OWRITE, 0666)) < 0)
		quit("Cannot create %s", name);
	Binit(bp, fd, OWRITE);
	Bseek(bp, 0, 2);
	fcode[nfiles++] = bp;
	return bp;
}

Rune *
compsub(Rune *rhs, Rune *end)
{
	Rune r;

	while ((r = *cp++) != '\0') {
		if(r == '\\') {
			if (rhs < end)
				*rhs++ = Runemax;
			else
				return 0;
			r = *cp++;
			if(r == 'n')
				r = '\n';
		} else {
			if(r == seof) {
				if (rhs < end)
					*rhs++ = '\0';
				else
					return 0;
				return rhs;
			}
		}
		if (rhs < end)
			*rhs++ = r;
		else
			return 0;
	}
	return 0;
}

Reprog *
compile(void)
{
	Rune c;
	char *ep;
	char expbuf[512];

	if((c = *cp++) == seof)		/* L'//' */
		return 0;
	ep = expbuf;
	do {
		if (c == L'\0' || c == L'\n')
			quit(TMMES, linebuf);
		if (c == L'\\') {
			if (ep >= expbuf+sizeof(expbuf))
				quit(TMMES, linebuf);
			ep += runetochar(ep, &c);
			if ((c = *cp++) == L'n')
				c = L'\n';
		}
		if (ep >= expbuf + sizeof(expbuf))
			quit(TMMES, linebuf);
		ep += runetochar(ep, &c);
	} while ((c = *cp++) != seof);
	*ep = 0;
	return lastre = regcomp(expbuf);
}

void
regerror(char *s)
{
	USED(s);
	quit(CGMES, L"r.e.-using", linebuf);
}

void
newfile(enum PTYPE type, char *name)
{
	if (type == P_ARG)
		prog.PCTL.curr = name;
	else if ((prog.PCTL.bp = Bopen(name, OREAD)) == 0)
		quit("Cannot open pattern-file: %s\n", name);
	prog.type = type;
}

int
rline(Rune *buf, Rune *end)
{
	int32_t c;
	Rune r;

	while ((c = getrune()) >= 0) {
		r = c;
		if (r == '\\') {
			if (buf <= end)
				*buf++ = r;
			if ((c = getrune()) < 0)
				break;
			r = c;
		} else if (r == '\n') {
			*buf = '\0';
			return 1;
		}
		if (buf <= end)
			*buf++ = r;
	}
	*buf = '\0';
	return -1;
}

int32_t
getrune(void)
{
	int32_t c;
	Rune r;
	char *p;

	if (prog.type == P_ARG) {
		if ((p = prog.PCTL.curr) != 0) {
			if (*p) {
				prog.PCTL.curr += chartorune(&r, p);
				c = r;
			} else {
				c = '\n';	/* fake an end-of-line */
				prog.PCTL.curr = 0;
			}
		} else
			c = -1;
	} else if ((c = Bgetrune(prog.PCTL.bp)) < 0)
		Bterm(prog.PCTL.bp);
	return c;
}

void
address(Addr *ap)
{
	int c;
	int32_t lno;

	if((c = *cp++) == '$')
		ap->type = A_DOL;
	else if(c == '/') {
		seof = c;
		if ((ap->rp = compile()) != nil)
			ap->type = A_RE;
		else
			ap->type = A_LAST;
	}
	else if (c >= '0' && c <= '9') {
		lno = c - '0';
		while ((c = *cp) >= '0' && c <= '9')
			lno = lno*10 + *cp++ - '0';
		if(!lno)
			quit("line number 0 is illegal",0);
		ap->type = A_LINE;
		ap->line = lno;
	}
	else {
		cp--;
		ap->type = A_NONE;
	}
}

int
cmp(char *a, char *b)		/* compare characters */
{
	while(*a == *b++)
		if (*a == '\0')
			return 0;
		else
			a++;
	return 1;
}

int
rcmp(Rune *a, Rune *b)		/* compare runes */
{
	while(*a == *b++)
		if (*a == '\0')
			return 0;
		else
			a++;
	return 1;
}

char *
text(char *p)		/* extract character string */
{
	Rune r;

	while(*cp == ' ' || *cp == '\t')
		cp++;
	while (*cp) {
		if ((r = *cp++) == '\\' && (r = *cp++) == '\0')
			break;
		if (r == '\n')
			while (*cp == ' ' || *cp == '\t')
				cp++;
		p += runetochar(p, &r);
	}
	*p++ = '\0';
	return p;
}

Rune *
stext(Rune *p, Rune *end)		/* extract rune string */
{
	while(*cp == L' ' || *cp == L'\t')
		cp++;
	while (*cp) {
		if (*cp == L'\\' && *++cp == L'\0')
			break;
		if (p >= end-1)
			quit(TMMES, linebuf);
		if ((*p++ = *cp++) == L'\n')
			while(*cp == L' ' || *cp == L'\t')
				cp++;
	}
	*p++ = 0;
	return p;
}


Label *
search(Label *ptr)
{
	Label	*rp;

	for (rp = ltab; rp < ptr; rp++)
		if(rcmp(rp->uninm, ptr->uninm) == 0)
			return(rp);
	return(0);
}

void
dechain(void)
{
	Label	*lptr;
	SedCom	*rptr, *trptr;

	for(lptr = ltab; lptr < lab; lptr++) {
		if(lptr->address == 0)
			quit("Undefined label: %S", lptr->uninm);
		if(lptr->chain) {
			rptr = lptr->chain;
			while((trptr = rptr->lb1) != nil) {
				rptr->lb1 = lptr->address;
				rptr = trptr;
			}
			rptr->lb1 = lptr->address;
		}
	}
}

int
ycomp(SedCom *r)
{
	int i;
	Rune *rp, *sp, *tsp;
	Rune c, highc;

	highc = 0;
	for(tsp = cp; *tsp != seof; tsp++) {
		if(*tsp == L'\\')
			tsp++;
		if(*tsp == L'\n' || *tsp == L'\0')
			return 0;
		if (*tsp > highc)
			highc = *tsp;
	}
	tsp++;
	if ((rp = r->text = (Rune *)malloc(sizeof(Rune) * (highc+2))) == nil)
		quit("Out of memory");
	*rp++ = highc;				/* save upper bound */
	for (i = 0; i <= highc; i++)
		rp[i] = i;
	sp = cp;
	while((c = *sp++) != seof) {
		if(c == L'\\' && *sp == L'n') {
			sp++;
			c = L'\n';
		}
		if((rp[c] = *tsp++) == L'\\' && *tsp == L'n') {
			rp[c] = L'\n';
			tsp++;
		}
		if(rp[c] == seof || rp[c] == L'\0') {
			free(r->re1);
			r->re1 = nil;
			return 0;
		}
	}
	if(*tsp != seof) {
		free(r->re1);
		r->re1 = nil;
		return 0;
	}
	cp = tsp+1;
	return 1;
}

void
execute(void)
{
	SedCom	*ipc;

	while ((spend = gline(linebuf)) != nil){
		for(ipc = pspace; ipc->command; ) {
			if (!executable(ipc)) {
				ipc++;
				continue;
			}
			command(ipc);

			if(delflag)
				break;
			if(jflag) {
				jflag = 0;
				if((ipc = ipc->lb1) == 0)
					break;
			} else
				ipc++;
		}
		if(!nflag && !delflag)
			putline(&fout, linebuf, spend - linebuf);
		if(aptr > abuf)
			arout();
		delflag = 0;
	}
}

/* determine if a statement should be applied to an input line */
int
executable(SedCom *ipc)
{
	if (ipc->active) {	/* Addr1 satisfied - accept until Addr2 */
		if (ipc->active == 1)		/* Second line */
			ipc->active = 2;
		switch(ipc->ad2.type) {
		case A_NONE:		/* No second addr; use first */
			ipc->active = 0;
			break;
		case A_DOL:		/* Accept everything */
			return !ipc->negfl;
		case A_LINE:		/* Line at end of range? */
			if (lnum <= ipc->ad2.line) {
				if (ipc->ad2.line == lnum)
					ipc->active = 0;
				return !ipc->negfl;
			}
			ipc->active = 0;	/* out of range */
			return ipc->negfl;
		case A_RE:		/* Check for matching R.E. */
			if (match(ipc->ad2.rp, linebuf))
				ipc->active = 0;
			return !ipc->negfl;
		default:
			quit("Internal error");
		}
	}
	switch (ipc->ad1.type) {	/* Check first address */
	case A_NONE:			/* Everything matches */
		return !ipc->negfl;
	case A_DOL:			/* Only last line */
		if (dolflag)
			return !ipc->negfl;
		break;
	case A_LINE:			/* Check line number */
		if (ipc->ad1.line == lnum) {
			ipc->active = 1;	/* In range */
			return !ipc->negfl;
		}
		break;
	case A_RE:			/* Check R.E. */
		if (match(ipc->ad1.rp, linebuf)) {
			ipc->active = 1;	/* In range */
			return !ipc->negfl;
		}
		break;
	default:
		quit("Internal error");
	}
	return ipc->negfl;
}

int
match(Reprog *pattern, Rune *buf)
{
	if (!pattern)
		return 0;
	subexp[0].rsp = buf;
	subexp[0].ep = 0;
	if (rregexec(pattern, linebuf, subexp, MAXSUB) > 0) {
		loc1 = subexp[0].rsp;
		loc2 = subexp[0].rep;
		return 1;
	}
	loc1 = loc2 = 0;
	return 0;
}

int
substitute(SedCom *ipc)
{
	int len;

	if(!match(ipc->re1, linebuf))
		return 0;

	/*
	 * we have at least one match.  some patterns, e.g. '$' or '^', can
	 * produce 0-length matches, so during a global substitute we must
	 * bump to the character after a 0-length match to keep from looping.
	 */
	sflag = 1;
	if(ipc->gfl == 0)			/* single substitution */
		dosub(ipc->rhs);
	else
		do{				/* global substitution */
			len = loc2 - loc1;	/* length of match */
			dosub(ipc->rhs);	/* dosub moves loc2 */
			if(*loc2 == 0)		/* end of string */
				break;
			if(len == 0)		/* zero-length R.E. match */
				loc2++;		/* bump over 0-length match */
			if(*loc2 == 0)		/* end of string */
				break;
		} while(match(ipc->re1, loc2));
	return 1;
}

void
dosub(Rune *rhsbuf)
{
	int c, n;
	Rune *lp, *sp, *rp;

	lp = linebuf;
	sp = genbuf;
	rp = rhsbuf;
	while (lp < loc1)
		*sp++ = *lp++;
	while((c = *rp++) != 0){
		if (c == '&') {
			sp = place(sp, loc1, loc2);
			continue;
		}
		if (c == Runemax && (c = *rp++) >= '1' && c < MAXSUB + '0') {
			n = c-'0';
			if (subexp[n].rsp && subexp[n].rep) {
				sp = place(sp, subexp[n].rsp, subexp[n].rep);
				continue;
			}
			else {
				fprint(2, "sed: Invalid back reference \\%d\n",n);
				errexit();
			}
		}
		*sp++ = c;
		if (sp >= &genbuf[LBSIZE])
			fprint(2, "sed: Output line too int32_t.\n");
	}
	lp = loc2;
	loc2 = sp - genbuf + linebuf;
	while ((*sp++ = *lp++) != 0)
		if (sp >= &genbuf[LBSIZE])
			fprint(2, "sed: Output line too int32_t.\n");
	lp = linebuf;
	sp = genbuf;
	while ((*lp++ = *sp++) != 0)
		;
	spend = lp - 1;
}

Rune *
place(Rune *sp, Rune *l1, Rune *l2)
{
	while (l1 < l2) {
		*sp++ = *l1++;
		if (sp >= &genbuf[LBSIZE])
			fprint(2, "sed: Output line too int32_t.\n");
	}
	return sp;
}

char *
trans(int c)
{
	static char buf[] = "\\x0000";
	static char hex[] = "0123456789abcdef";

	switch(c) {
	case '\b':
		return "\\b";
	case '\n':
		return "\\n";
	case '\r':
		return "\\r";
	case '\t':
		return "\\t";
	case '\\':
		return "\\\\";
	}
	buf[2] = hex[(c>>12)&0xF];
	buf[3] = hex[(c>>8)&0xF];
	buf[4] = hex[(c>>4)&0xF];
	buf[5] = hex[c&0xF];
	return buf;
}

void
command(SedCom *ipc)
{
	int i, c;
	char *ucp;
	Rune *execp, *p1, *p2, *rp;

	switch(ipc->command) {
	case ACOM:
		*aptr++ = ipc;
		if(aptr >= abuf+MAXADDS)
			quit("sed: Too many appends after line %ld\n",
				(char *)lnum);
		*aptr = 0;
		break;
	case CCOM:
		delflag = 1;
		if(ipc->active == 1) {
			for(rp = ipc->text; *rp; rp++)
				Bputrune(&fout, *rp);
			Bputc(&fout, '\n');
		}
		break;
	case DCOM:
		delflag++;
		break;
	case CDCOM:
		p1 = p2 = linebuf;
		while(*p1 != '\n') {
			if(*p1++ == 0) {
				delflag++;
				return;
			}
		}
		p1++;
		while((*p2++ = *p1++) != 0)
			;
		spend = p2 - 1;
		jflag++;
		break;
	case EQCOM:
		Bprint(&fout, "%ld\n", lnum);
		break;
	case GCOM:
		p1 = linebuf;
		p2 = holdsp;
		while((*p1++ = *p2++) != 0)
			;
		spend = p1 - 1;
		break;
	case CGCOM:
		*spend++ = '\n';
		p1 = spend;
		p2 = holdsp;
		while((*p1++ = *p2++) != 0)
			if(p1 >= lbend)
				break;
		spend = p1 - 1;
		break;
	case HCOM:
		p1 = holdsp;
		p2 = linebuf;
		while((*p1++ = *p2++) != 0);
		hspend = p1 - 1;
		break;
	case CHCOM:
		*hspend++ = '\n';
		p1 = hspend;
		p2 = linebuf;
		while((*p1++ = *p2++) != 0)
			if(p1 >= hend)
				break;
		hspend = p1 - 1;
		break;
	case ICOM:
		for(rp = ipc->text; *rp; rp++)
			Bputrune(&fout, *rp);
		Bputc(&fout, '\n');
		break;
	case BCOM:
		jflag = 1;
		break;
	case LCOM:
		c = 0;
		for (i = 0, rp = linebuf; *rp; rp++) {
			c = *rp;
			if(c >= 0x20 && c < 0x7F && c != '\\') {
				Bputc(&fout, c);
				if(i++ > 71) {
					Bprint(&fout, "\\\n");
					i = 0;
				}
			} else {
				for (ucp = trans(*rp); *ucp; ucp++){
					c = *ucp;
					Bputc(&fout, c);
					if(i++ > 71) {
						Bprint(&fout, "\\\n");
						i = 0;
					}
				}
			}
		}
		if(c == ' ')
			Bprint(&fout, "\\n");
		Bputc(&fout, '\n');
		break;
	case NCOM:
		if(!nflag)
			putline(&fout, linebuf, spend-linebuf);

		if(aptr > abuf)
			arout();
		if((execp = gline(linebuf)) == 0) {
			delflag = 1;
			break;
		}
		spend = execp;
		break;
	case CNCOM:
		if(aptr > abuf)
			arout();
		*spend++ = '\n';
		if((execp = gline(spend)) == 0) {
			delflag = 1;
			break;
		}
		spend = execp;
		break;
	case PCOM:
		putline(&fout, linebuf, spend-linebuf);
		break;
	case CPCOM:
cpcom:
		for(rp = linebuf; *rp && *rp != '\n'; rp++)
			Bputc(&fout, *rp);
		Bputc(&fout, '\n');
		break;
	case QCOM:
		if(!nflag)
			putline(&fout, linebuf, spend-linebuf);
		if(aptr > abuf)
			arout();
		exits(0);
	case RCOM:
		*aptr++ = ipc;
		if(aptr >= &abuf[MAXADDS])
			quit("sed: Too many reads after line %ld\n",
				(char *)lnum);
		*aptr = 0;
		break;
	case SCOM:
		i = substitute(ipc);
		if(i && ipc->pfl){
			if(ipc->pfl == 1)
				putline(&fout, linebuf, spend-linebuf);
			else
				goto cpcom;
		}
		if(i && ipc->fcode)
			goto wcom;
		break;

	case TCOM:
		if(sflag) {
			sflag = 0;
			jflag = 1;
		}
		break;

	case WCOM:
wcom:
		putline(ipc->fcode,linebuf, spend - linebuf);
		break;
	case XCOM:
		p1 = linebuf;
		p2 = genbuf;
		while((*p2++ = *p1++) != 0)
			;
		p1 = holdsp;
		p2 = linebuf;
		while((*p2++ = *p1++) != 0)
			;
		spend = p2 - 1;
		p1 = genbuf;
		p2 = holdsp;
		while((*p2++ = *p1++) != 0)
			;
		hspend = p2 - 1;
		break;
	case YCOM:
		p1 = linebuf;
		p2 = ipc->text;
		for (i = *p2++;	*p1; p1++)
			if (*p1 <= i)
				*p1 = p2[*p1];
		break;
	}
}

void
putline(Biobuf *bp, Rune *buf, int n)
{
	while (n--)
		Bputrune(bp, *buf++);
	Bputc(bp, '\n');
}

int
ecmp(Rune *a, Rune *b, int count)
{
	while(count--)
		if(*a++ != *b++)
			return 0;
	return 1;
}

void
arout(void)
{
	int	c;
	char	*s;
	char	buf[128];
	Rune	*p1;
	Biobuf	*fi;

	for (aptr = abuf; *aptr; aptr++) {
		if((*aptr)->command == ACOM) {
			for(p1 = (*aptr)->text; *p1; p1++ )
				Bputrune(&fout, *p1);
			Bputc(&fout, '\n');
		} else {
			for(s = buf, p1 = (*aptr)->text; *p1; p1++)
				s += runetochar(s, p1);
			*s = '\0';
			if((fi = Bopen(buf, OREAD)) == 0)
				continue;
			while((c = Bgetc(fi)) >= 0)
				Bputc(&fout, c);
			Bterm(fi);
		}
	}
	aptr = abuf;
	*aptr = 0;
}

void
errexit(void)
{
	exits("error");
}

void
quit(char *fmt, ...)
{
	char *p, *ep;
	char msg[256];
	va_list arg;

	ep = msg + sizeof msg;
	p = seprint(msg, ep, "sed: ");
	va_start(arg, fmt);
	p = vseprint(p, ep, fmt, arg);
	va_end(arg);
	p = seprint(p, ep, "\n");
	write(2, msg, p - msg);
	errexit();
}

Rune *
gline(Rune *addr)
{
	int32_t c;
	Rune *p;
	static int32_t peekc = 0;

	if (f == 0 && opendata() < 0)
		return 0;
	sflag = 0;
	lnum++;
/*	Bflush(&fout);********* dumped 4/30/92 - bobf****/
	do {
		p = addr;
		for (c = (peekc? peekc: Bgetrune(f)); c >= 0; c = Bgetrune(f)) {
			if (c == '\n') {
				if ((peekc = Bgetrune(f)) < 0 && fhead == 0)
					dolflag = 1;
				*p = '\0';
				return p;
			}
			if (c && p < lbend)
				*p++ = c;
		}
		/* return partial final line, adding implicit newline */
		if(p != addr) {
			*p = '\0';
			peekc = -1;
			if (fhead == 0)
				dolflag = 1;
			return p;
		}
		peekc = 0;
		Bterm(f);
	} while (opendata() > 0);		/* Switch to next stream */
	f = 0;
	return 0;
}

/*
 * Data file input section - the intent is to transparently
 *	catenate all data input streams.
 */
void
enroll(char *filename)		/* Add a file to the input file cache */
{
	FileCache *fp;

	if ((fp = (FileCache *)malloc(sizeof (FileCache))) == nil)
		quit("Out of memory");
	if (ftail == nil)
		fhead = fp;
	else
		ftail->next = fp;
	ftail = fp;
	fp->next = nil;
	fp->name = filename;		/* 0 => stdin */
}

int
opendata(void)
{
	if (fhead == nil)
		return -1;
	if (fhead->name) {
		if ((f = Bopen(fhead->name, OREAD)) == nil)
			quit("Can't open %s", fhead->name);
	} else {
		Binit(&stdin, 0, OREAD);
		f = &stdin;
	}
	fhead = fhead->next;
	return 1;
}
