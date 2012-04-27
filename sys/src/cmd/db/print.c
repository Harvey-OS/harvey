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

/* general printing routines ($) */

char	*Ipath = INCDIR;
static	int	tracetype;
static void	printfp(Map*, int);

/*
 *	callback on stack trace
 */
static void
ptrace(Map *map, uvlong pc, uvlong sp, Symbol *sym)
{
	char buf[512];

	USED(map);
	dprint("%s(", sym->name);
	printparams(sym, sp);
	dprint(") ");
	printsource(sym->value);
	dprint(" called from ");
	symoff(buf, 512, pc, CTEXT);
	dprint("%s ", buf);
	printsource(pc);
	dprint("\n");
	if(tracetype == 'C')
		printlocals(sym, sp);
}

void
printtrace(int modif)
{
	int i;
	uvlong pc, sp, link;
	ulong w;
	BKPT *bk;
	Symbol s;
	int stack;
	char *fname;
	char buf[512];

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
		attachprocess();
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
		printfp(cormap, modif);
		return;

	case 'c':
	case 'C':
		tracetype = modif;
		if (machdata->ctrace) {
			if (adrflg) {
				/*
				 * trace from jmpbuf for multi-threaded code.
				 * assume sp and pc are in adjacent locations
				 * and mach->szaddr in size.
				 */
				if (geta(cormap, adrval, &sp) < 0 ||
					geta(cormap, adrval+mach->szaddr, &pc) < 0)
						error("%r");
			} else {
				sp = rget(cormap, mach->sp);
				pc = rget(cormap, mach->pc);
			}
			if(mach->link)
				link = rget(cormap, mach->link);
			else
				link = 0;
			if (machdata->ctrace(cormap, pc, sp, link, ptrace) <= 0)
				error("no stack frame");
		}
		break;

		/*print externals*/
	case 'e':
		for (i = 0; globalsym(&s, i); i++) {
			if (get4(cormap, s.value, &w) > 0)
				dprint("%s/%12t%#lux\n", s.name, w);
		}
		break;

		/*print breakpoints*/
	case 'b':
	case 'B':
		for (bk=bkpthead; bk; bk=bk->nxtbkpt)
			if (bk->flag) {
				symoff(buf, 512, (WORD)bk->loc, CTEXT);
				dprint(buf);
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

static void
printfp(Map *map, int modif)
{
	Reglist *rp;
	int i;
	int ret;
	char buf[512];

	for (i = 0, rp = mach->reglist; rp->rname; rp += ret) {
		ret = 1;
		if (!(rp->rflags&RFLT))
			continue;
		ret = fpformat(map, rp, buf, sizeof(buf), modif);
		if (ret < 0) {
			werrstr("Register %s: %r", rp->rname);
			error("%r");
		}
			/* double column print */
		if (i&0x01)
			dprint("%40t%-8s%-12s\n", rp->rname, buf);
		else
			dprint("\t%-8s%-12s", rp->rname, buf);
		i++;
	}
}

void
redirin(int stack, char *file)
{
	char *pfile;

	if (file == 0) {
		iclose(-1, 0);
		return;
	}
	iclose(stack, 0);
	if ((infile = open(file, 0)) < 0) {
		pfile = smprint("%s/%s", Ipath, file);
		infile = open(pfile, 0);
		free(pfile);
		if(infile < 0) {
			infile = STDIN;
			error("cannot open");
		}
	}
}

void
printmap(char *s, Map *map)
{
	int i;

	if (!map)
		return;
	if (map == symmap)
		dprint("%s%12t`%s'\n", s, fsym < 0 ? "-" : symfil);
	else if (map == cormap)
		dprint("%s%12t`%s'\n", s, fcor < 0 ? "-" : corfil);
	else
		dprint("%s\n", s);
	for (i = 0; i < map->nsegs; i++) {
		if (map->seg[i].inuse)
			dprint("%s%8t%-16#llux %-16#llux %-16#llux\n",
				map->seg[i].name, map->seg[i].b,
				map->seg[i].e, map->seg[i].f);
	}
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
			dprint("%16#llux t %s\n", sp->value, sp->name);
			break;
		case 'T':
		case 'L':
			dprint("%16#llux T %s\n", sp->value, sp->name);
			break;
		case 'D':
		case 'd':
		case 'B':
		case 'b':
		case 'a':
		case 'p':
		case 'm':
			dprint("%16#llux %c %s\n", sp->value, sp->type, sp->name);
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
printsource(ADDR dot)
{
	char str[STRINGSZ];

	if (fileline(str, STRINGSZ, dot))
		dprint("%s", str);
}

void
printpc(void)
{
	char buf[512];

	dot = rget(cormap, mach->pc);
	if(dot){
		printsource((long)dot);
		printc(' ');
		symoff(buf, sizeof(buf), (long)dot, CTEXT);
		dprint("%s/", buf);
		if (machdata->das(cormap, dot, 'i', buf, sizeof(buf)) < 0)
			error("%r");
		dprint("%16t%s\n", buf);
	}
}

void
printlocals(Symbol *fn, ADDR fp)
{
	int i;
	ulong w;
	Symbol s;

	s = *fn;
	for (i = 0; localsym(&s, i); i++) {
		if (s.class != CAUTO)
			continue;
		if (get4(cormap, fp-s.value, &w) > 0)
			dprint("%8t%s.%s/%10t%#lux\n", fn->name, s.name, w);
		else
			dprint("%8t%s.%s/%10t?\n", fn->name, s.name);
	}
}

void
printparams(Symbol *fn, ADDR fp)
{
	int i;
	Symbol s;
	ulong w;
	int first = 0;

	fp += mach->szaddr;			/* skip saved pc */
	s = *fn;
	for (i = 0; localsym(&s, i); i++) {
		if (s.class != CPARAM)
			continue;
		if (first++)
			dprint(", ");
		if (get4(cormap, fp+s.value, &w) > 0)
			dprint("%s=%#lux", s.name, w);
	}
}
