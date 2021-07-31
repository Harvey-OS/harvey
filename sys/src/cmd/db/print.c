/*
 *
 *	debugger
 *
 */
#include "defs.h"
#include "fns.h"

extern	int	infile;
extern	int	outfile;
extern	int	maxpos;
extern	char	lastc;


/* general printing routines ($) */

char	*Ipath = INCDIR;

void
printtrace(int modif)
{
	int	i;
	long v;
	BKPT *bk;
	Symbol s;
	int	stack;
	char	*fname;

	if (cntflg==0)
		cntval = -1;
	switch (modif) {

	case '<':
		if (cntval == 0) {
			while (readchar() != EOR)
				;
			reread();
			break;
		}
		if (rdc() == '<')
			stack = 1;
		else {
			stack = 0;
			reread();
		}
		fname = getfname();
		redirin(stack, fname);
		break;

	case '>':
		fname = getfname();
		redirout(fname);
		break;

	case 'a':
		attachproc();
		break;

	case 'p':
		kmproc();
		break;

	case 'k':
		kmsys();
		break;

	case 'q':
	case 'Q':
		done();

	case 'w':
		maxpos=(adrflg?adrval:MAXPOS);
		break;

	case 'S':
		printsym();
		break;

	case 's':
		maxoff=(adrflg?adrval:MAXOFF);
		break;

	case 'v':
		for (i=0;i<NVARS;i++) {
			if (var[i])
				dprint("%-8lux >%c\n", var[i],
					(i<=9 ? '0' : 'a'-10) + i);
		}
		break;

	case 'm':
		printmap("? map", symmap);
		printmap("/ map", cormap);
		break;

	case 0:
	case '?':
		if (pid)
			dprint("pid = %d\n",pid);
		else
			prints("no process\n");
		flushbuf();

	case 'r':
	case 'R':
		printregs(modif);
		return;

	case 'f':
	case 'F':
		if(machdata->fpprint)
			machdata->fpprint(modif);
		else
			dprint("no fpreg print routine\n");
		return;

	case 'c':
	case 'C':
		if (machdata->ctrace)
			machdata->ctrace(modif);
		break;

		/*print externals*/
	case 'e':
		for (i = 0; globalsym(&s, i); i++) {
			if (get4(cormap, s.value,SEGDATA,&v))
				dprint("%s/%12t%lux\n", s.name,	v);
		}
		break;

		/*print breakpoints*/
	case 'b':
	case 'B':
		for (bk=bkpthead; bk; bk=bk->nxtbkpt)
			if (bk->flag) {
				psymoff((WORD)bk->loc,SEGTEXT,"");
				if (bk->count != 1)
					dprint(",%d", bk->count);
				dprint(":%c %s", bk->flag == BKPTTMP ? 'B' : 'b', bk->comm);
			}
		break;

	case 'M':
		fname = getfname();
		if (machbyname(fname) == 0)
			dprint("unknown name\n");;
		break;
	default:
		error("bad `$' command");
	}

}

char *
getfname(void)
{
	static char fname[ARB];
	char *p;

	if (rdc() == EOR) {
		reread();
		return (0);
	}
	p = fname;
	do {
		*p++ = lastc;
		if (p >= &fname[ARB-1])
			error("filename too long");
	} while (rdc() != EOR);
	*p = 0;
	reread();
	return (fname);
}

void
redirin(int stack, char *file)
{
	char pfile[ARB];

	if (file == 0) {
		iclose(-1, 0);
		return;
	}
	iclose(stack, 0);
	if ((infile = open(file, 0)) < 0) {
		strcpy(pfile, Ipath);
		strcat(pfile, "/");
		strcat(pfile, file);
		if ((infile = open(pfile, 0)) < 0) {
			infile = STDIN;
			error("cannot open");
		}
	}
	if (cntflg)
		var[9] = cntval;
	else
		var[9] = 1;
}

void
printmap(char *s, Map *mp)
{
	if (mp == symmap)
		dprint("%s%12t`%s'\n", s, fsym < 0 ? "-" : symfil);
	else if (mp == cormap)
		dprint("%s%12t`%s'\n", s, fcor < 0 ? "-" : corfil);
	else
		dprint("%s\n", s);
	if (mp->seg[SEGTEXT].inuse)
		dprint("%s%8t%-16lux %-16lux %-16lux\n", mp->seg[SEGTEXT].name,
			mp->seg[SEGTEXT].b, mp->seg[SEGTEXT].e, mp->seg[SEGTEXT].f);
	if (mp->seg[SEGDATA].inuse)
		dprint("%s%8t%-16lux %-16lux %-16lux\n", mp->seg[SEGDATA].name,
			mp->seg[SEGDATA].b, mp->seg[SEGDATA].e, mp->seg[SEGDATA].f);
	if (mp->seg[SEGUBLK].inuse)
		dprint("%s%8t%-16lux %-16lux %-16lux\n", mp->seg[SEGUBLK].name,
		mp->seg[SEGUBLK].b, mp->seg[SEGUBLK].e, mp->seg[SEGUBLK].f);
	if (mp->seg[SEGREGS].inuse)
		dprint("%s%8t%-16lux %-16lux %-16lux\n", mp->seg[SEGREGS].name,
			mp->seg[SEGREGS].b, mp->seg[SEGREGS].e, mp->seg[SEGREGS].f);
}

void
printparcel(ulong p, int zeros)
{
	ulong d;
	static char hex[] = "0123456789abcdef";

	if(d = p/16)
		printparcel(d, zeros-1);
	else
		while(zeros--)
			printc('0');
	printc(hex[p%16]);
}

/*
 * Print value v as name[+offset] and then the string s.
 */
void
psymoff(WORD v, int type, char *str)
{
	Symbol s;
	int t;
	int r;
	int delta;

	r = delta = 0;		/* to shut compiler up */
	switch(type) {
	case SEGREGS:
		dprint("%%%lux", v);
		dprint(str);
		return;
	case SEGANY:
		t = CANY;
		break;
	case SEGDATA:
		t = CDATA;
		break;
	case SEGTEXT:
		t = CTEXT;
		break;
	case SEGNONE:
	default:
		return;
	}
	if (v) {
		r = findsym(v, t, &s);
		if (r)
			delta = v-s.value;
	}
	if (v == 0 || r == 0 || delta >= maxoff)
		dprint("%lux", v);
	else {
		dprint("%s", s.name);
		if (delta)
			dprint("+%lux", delta);
	}
	dprint(str);
}
/*
 *	dump the raw symbol table
 */
void
printsym(void)
{
	int i;
	Sym *sp;

	for (i = 0; sp = getsym(i); i++) {
		switch(sp->type) {
		case 't':
		case 'l':
			dprint("%8lux t %s\n", sp->value, sp->name);
			break;
		case 'T':
		case 'L':
			dprint("%8lux T %s\n", sp->value, sp->name);
			break;
		case 'D':
		case 'd':
		case 'B':
		case 'b':
		case 'a':
		case 'p':
		case 'm':
			dprint("%8lux %c %s\n", sp->value, sp->type, sp->name);
			break;
		default:
			break;
		}
	}
}

#define	STRINGSZ	128

/*
 *	print the value of dot as file:line
 */
void
printsource(long dot)
{
	char str[STRINGSZ];

	if (fileline(str, STRINGSZ, dot))
		dprint("%s", str);
}


void
printpc(void)
{
	dot = (ADDR)rget(mach->pc);
	if(dot){
		printsource((WORD)dot);
		printc(' ');
		psymoff((WORD)dot, SEGTEXT, "?%16t");
		machdata->printins(symmap, 'i', SEGTEXT);
		printc(EOR);
	}
}

void
printlocals(Symbol *fn, ADDR fp)
{
	int i;
	WORD val;
	Symbol s;

	s = *fn;
	for (i = 0; localsym(&s, i); i++) {
		if (s.class != CAUTO)
			continue;
		if (get4(cormap, fp-s.value, SEGDATA, &val))
			dprint("%8t%s.%s/%10t%lux\n", fn->name, s.name, val);
		else {
			dprint("%8t%s.%s/%10t?\n", fn->name, s.name);
			errflg = 0;
		}
	}
}

void
printparams(Symbol *fn, ADDR fp)
{
	int i;
	Symbol s;
	long v;
	int first = 0;

	fp += mach->szreg;			/* skip saved pc */
	s = *fn;
	for (i = 0; localsym(&s, i); i++) {
		if (s.class != CPARAM)
			continue;
		if (first++)
			dprint(", ");
		if (get4(cormap, fp+s.value, SEGDATA, &v))
			dprint("%s=%lux", s.name, v);
	}
}
