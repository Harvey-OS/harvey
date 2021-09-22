#define	EXTERN
#include	"l.h"
#include	<ar.h>

#ifndef	DEFAULT
#define	DEFAULT	'9'
#endif

char	thechar		= '7';
char	*thestring 	= "arm64";

/*
 *	-H0				no header
 *	-H2 -T0x100028 -R0x100000		is plan9 format
 *	-H6 -R4096		no header with segments padded to pages
 *	-H7				is elf
 */

void
usage(void)
{
	diag("usage: %s [-options] objects", argv0);
	errorexit();
}

void
main(int argc, char *argv[])
{
	int c;
	char *a;

	Binit(&bso, 1, OWRITE);
	cout = -1;
	listinit();
	outfile = 0;
	nerrors = 0;
	curtext = P;
	HEADTYPE = -1;
	INITTEXT = -1;
	INITTEXTP = -1;
	INITDAT = -1;
	INITRND = -1;
	INITENTRY = 0;

	ARGBEGIN {
	default:
		c = ARGC();
		if(c >= 0 && c < sizeof(debug))
			debug[c]++;
		break;
	case 'o':
		outfile = ARGF();
		break;
	case 'E':
		a = ARGF();
		if(a)
			INITENTRY = a;
		break;
	case 'L':
		addlibpath(EARGF(usage()));
		break;
	case 'T':
		a = ARGF();
		if(a)
			INITTEXT = atolwhex(a);
		break;
	case 'P':
		a = ARGF();
		if(a)
			INITTEXTP = atolwhex(a);
		break;
	case 'D':
		a = ARGF();
		if(a)
			INITDAT = atolwhex(a);
		break;
	case 'R':
		a = ARGF();
		if(a)
			INITRND = atolwhex(a);
		break;
	case 'H':
		a = ARGF();
		if(a)
			HEADTYPE = atolwhex(a);
		break;
	case 'x':	/* produce export table */
		doexp = 1;
		if(argv[1] != nil && argv[1][0] != '-' && !isobjfile(argv[1]))
			readundefs(ARGF(), SEXPORT);
		break;
	case 'u':	/* produce dynamically loadable module */
		dlm = 1;
		if(argv[1] != nil && argv[1][0] != '-' && !isobjfile(argv[1]))
			readundefs(ARGF(), SIMPORT);
		break;
	} ARGEND

	USED(argc);

	if(*argv == 0)
		usage();
	if(!debug['9'] && !debug['U'] && !debug['B'])
		debug[DEFAULT] = 1;
	addlibroot();
	if(HEADTYPE == -1) {
		if(debug['U'])
			HEADTYPE = 0;
		if(debug['B'])
			HEADTYPE = 1;
		if(debug['9'])
			HEADTYPE = 2;
	}
	switch(HEADTYPE) {
	default:
		diag("unknown -H option");
		errorexit();
	case 0:	/* no header */
	case 6:	/* no header, padded segments */
		HEADR = 0L;
		if(INITTEXT == -1)
			INITTEXT = 0;
		if(INITDAT == -1)
			INITDAT = 0;
		if(INITRND == -1)
			INITRND = 4;
		break;
	case 2:	/* plan 9 */
		HEADR = 40L;
		if(INITTEXT == -1)
			INITTEXT = 0x100000+HEADR;
		if(INITDAT == -1)
			INITDAT = 0;
		if(INITRND == -1)
			INITRND = 0x100000;
		break;
	case 7:	/* elf executable */
		HEADR = rnd(Ehdr64sz+3*Phdr64sz, 16);
		if(INITTEXT == -1)
			INITTEXT = 4096+HEADR;
		if(INITDAT == -1)
			INITDAT = 0;
		if(INITRND == -1)
			INITRND = 4;
		break;
	}
	if (INITTEXTP == -1)
		INITTEXTP = INITTEXT;
	if(INITDAT != 0 && INITRND != 0)
		print("warning: -D0x%lux is ignored because of -R0x%lux\n",
			INITDAT, INITRND);
	if(debug['v'])
		Bprint(&bso, "HEADER = -H0x%d -T0x%lux -D0x%lux -R0x%lux\n",
			HEADTYPE, INITTEXT, INITDAT, INITRND);
	Bflush(&bso);
	zprg.as = AGOK;
	zprg.reg = NREG;
	zprg.from.name = D_NONE;
	zprg.from.type = D_NONE;
	zprg.from.reg = NREG;
	zprg.to = zprg.from;
	zprg.from3 = zprg.from;
	buildop();
	histgen = 0;
	textp = P;
	datap = P;
	pc = 0;
	dtype = 4;
	if(outfile == 0)
		outfile = "7.out";
	cout = create(outfile, 1, 0775);
	if(cout < 0) {
		diag("cannot create %s: %r", outfile);
		errorexit();
	}
	nuxiinit();

	version = 0;
	cbp = buf.cbuf;
	cbc = sizeof(buf.cbuf);
	firstp = prg();
	lastp = firstp;

	if(INITENTRY == 0) {
		INITENTRY = "_main";
		if(debug['p'])
			INITENTRY = "_mainp";
		if(!debug['l'])
			lookup(INITENTRY, 0)->type = SXREF;
	} else if(!(*INITENTRY >= '0' && *INITENTRY <= '9'))
		lookup(INITENTRY, 0)->type = SXREF;

	while(*argv)
		objfile(*argv++);
	if(!debug['l'])
		loadlib();
	firstp = firstp->link;
	if(firstp == P)
		goto out;
	if(doexp || dlm){
		EXPTAB = "_exporttab";
		zerosig(EXPTAB);
		zerosig("etext");
		zerosig("edata");
		zerosig("end");
		if(dlm){
			import();
			HEADTYPE = 2;
			INITTEXT = INITDAT = 0;
			INITRND = 8;
			INITENTRY = EXPTAB;
		}
		export();
	}
	patch();
	if(debug['p'])
		if(debug['1'])
			doprof1();
		else
			doprof2();
	dodata();
	follow();
	if(firstp == P)
		goto out;
	noops();
	span();
	asmb();
	undef();

out:
	if(debug['v']) {
		Bprint(&bso, "%5.2f cpu time\n", cputime());
		Bprint(&bso, "%ld memory used\n", hunkspace());
		Bprint(&bso, "%lld sizeof adr\n", (vlong)sizeof(Adr));
		Bprint(&bso, "%lld sizeof prog\n", (vlong)sizeof(Prog));
	}
	Bflush(&bso);
	errorexit();
}

int
zaddr(uchar *p, Adr *a, Sym *h[])
{
	int i, c;
	long l;
	Sym *s;
	Auto *u;

	c = p[2];
	if(c < 0 || c > NSYM){
		print("sym out of range: %d\n", c);
		p[0] = ALAST+1;
		return 0;
	}
	a->type = p[0];
	a->reg = p[1];
	a->sym = h[c];
	a->name = p[3];
	c = 4;

	if(a->reg < 0 || a->reg > NREG) {
		print("register out of range %d\n", a->reg);
		p[0] = ALAST+1;
		return 0;	/*  force real diagnostic */
	}

	switch(a->type) {
	default:
		print("unknown type %d\n", a->type);
		p[0] = ALAST+1;
		return 0;	/*  force real diagnostic */

	case D_NONE:
	case D_REG:
	case D_SP:
	case D_FREG:
	case D_VREG:
	case D_COND:
		break;

	case D_OREG:
	case D_XPRE:
	case D_XPOST:
	case D_CONST:
	case D_BRANCH:
	case D_SHIFT:
	case D_EXTREG:
	case D_ROFF:
	case D_SPR:
		l = p[4] | (p[5]<<8) | (p[6]<<16) | (p[7]<<24);
		a->offset = l;
		c += 4;
		if(a->type == D_CONST && l == 0)
			a->reg = REGZERO;
		break;

	case D_DCONST:
		l = p[4] | (p[5]<<8) | (p[6]<<16) | (p[7]<<24);
		a->offset = (uvlong)l & 0xFFFFFFFFUL;
		l = p[8] | (p[9]<<8) | (p[10]<<16) | (p[11]<<24);
		a->offset |= (vlong)l << 32;
		c += 8;
		a->type = D_CONST;
		if(a->offset == 0)
			a->reg = REGZERO;
		break;

	case D_SCONST:
		a->sval = halloc(NSNAME);
		memmove(a->sval, p+4, NSNAME);
		c += NSNAME;
		break;

	case D_FCONST:
		a->ieee = halloc(sizeof(Ieee));
		a->ieee->l = p[4] | (p[5]<<8) |
			(p[6]<<16) | (p[7]<<24);
		a->ieee->h = p[8] | (p[9]<<8) |
			(p[10]<<16) | (p[11]<<24);
		c += 8;
		break;
	}
	s = a->sym;
	if(s == S)
		return c;
	i = a->name;
	if(i != D_AUTO && i != D_PARAM)
		return c;

	l = a->offset;
	for(u=curauto; u; u=u->link)
		if(u->asym == s)
		if(u->type == i) {
			if(u->aoffset > l)
				u->aoffset = l;
			return c;
		}

	u = halloc(sizeof(Auto));
	u->link = curauto;
	curauto = u;
	u->asym = s;
	u->aoffset = l;
	u->type = i;
	return c;
}

void
nopout(Prog *p)
{
	p->as = ANOP;
	p->from.type = D_NONE;
	p->to.type = D_NONE;
}

static int
isnegoff(Prog *p)
{
	if(p->from.type == D_CONST &&
	   p->from.name == D_NONE &&
	   p->from.offset < 0)
		return 1;
	return 0;
}

void
ldobj(int f, long c, char *pn)
{
	vlong ipc;
	Prog *p, *t;
	uchar *bloc, *bsize, *stop;
	Sym *h[NSYM], *s, *di;
	int v, o, r, skip;
	ulong sig;
	static int files;
	static char **filen;
	char **nfilen;

	if((files&15) == 0){
		nfilen = malloc((files+16)*sizeof(char*));
		memmove(nfilen, filen, files*sizeof(char*));
		free(filen);
		filen = nfilen;
	}
	filen[files++] = strdup(pn);

	bsize = buf.xbuf;
	bloc = buf.xbuf;
	di = S;

newloop:
	memset(h, 0, sizeof(h));
	version++;
	histfrogp = 0;
	ipc = pc;
	skip = 0;

loop:
	if(c <= 0)
		goto eof;
	r = bsize - bloc;
	if(r < 100 && r < c) {		/* enough for largest prog */
		bsize = readsome(f, buf.xbuf, bloc, bsize, c);
		if(bsize == 0)
			goto eof;
		bloc = buf.xbuf;
		goto loop;
	}
	o = bloc[0] | (bloc[1] << 8);		/* as */
	if(o <= AXXX || o >= ALAST) {
		diag("%s: line %lld: opcode out of range %d", pn, pc-ipc, o);
		print("	probably not a .7 file\n");
		errorexit();
	}
	if(o == ANAME || o == ASIGNAME) {
		sig = 0;
		if(o == ASIGNAME){
			sig = bloc[2] | (bloc[3]<<8) | (bloc[4]<<16) | (bloc[5]<<24);
			bloc += 4;
			c -= 4;
		}
		stop = memchr(&bloc[4], 0, bsize-&bloc[4]);
		if(stop == 0){
			bsize = readsome(f, buf.xbuf, bloc, bsize, c);
			if(bsize == 0)
				goto eof;
			bloc = buf.xbuf;
			stop = memchr(&bloc[4], 0, bsize-&bloc[4]);
			if(stop == 0){
				fprint(2, "%s: name too long\n", pn);
				errorexit();
			}
		}
		v = bloc[2];	/* type */
		o = bloc[3];	/* sym */
		bloc += 4;
		c -= 4;

		r = 0;
		if(v == D_STATIC)
			r = version;
		s = lookup((char*)bloc, r);
		c -= &stop[1] - bloc;
		bloc = stop + 1;

		if(sig != 0){
			if(s->sig != 0 && s->sig != sig)
				diag("incompatible type signatures %lux(%s) and %lux(%s) for %s", s->sig, filen[s->file], sig, pn, s->name);
			s->sig = sig;
			s->file = files-1;
		}

		if(debug['W'])
			print("	ANAME	%s\n", s->name);
		h[o] = s;
		if((v == D_EXTERN || v == D_STATIC) && s->type == 0)
			s->type = SXREF;
		if(v == D_FILE) {
			if(s->type != SFILE) {
				histgen++;
				s->type = SFILE;
				s->value = histgen;
			}
			if(histfrogp < MAXHIST) {
				histfrog[histfrogp] = s;
				histfrogp++;
			} else
				collapsefrog(s);
		}
		goto loop;
	}

	p = halloc(sizeof(Prog));
	p->as = o;
	p->reg = bloc[2] & 0x3F;
	if(bloc[2] & 0x80)
		p->mark = NOSCHED;
	p->line = bloc[3] | (bloc[4]<<8) | (bloc[5]<<16) | (bloc[6]<<24);

	r = zaddr(bloc+7, &p->from, h) + 7;
	if(bloc[2] & 0x40)
		r += zaddr(bloc+r, &p->from3, h);
	else
		p->from3 = zprg.from3;
	r += zaddr(bloc+r, &p->to, h);
	bloc += r;
	c -= r;

	if(p->reg > NREG)
		diag("register out of range %d", p->reg);

	p->link = P;
	p->cond = P;

	if(debug['W'])
		print("%P\n", p);

	switch(o) {
	case AHISTORY:
		if(p->to.offset == -1) {
			addlib(pn);
			histfrogp = 0;
			goto loop;
		}
		addhist(p->line, D_FILE);		/* 'z' */
		if(p->to.offset)
			addhist(p->to.offset, D_FILE1);	/* 'Z' */
		histfrogp = 0;
		goto loop;

	case AEND:
		histtoauto();
		if(curtext != P)
			curtext->to.autom = curauto;
		curauto = 0;
		curtext = P;
		if(c)
			goto newloop;
		return;

	case AGLOBL:
		s = p->from.sym;
		if(s == S) {
			diag("GLOBL must have a name\n%P", p);
			errorexit();
		}
		if(s->type == 0 || s->type == SXREF) {
			s->type = SBSS;
			s->value = 0;
		}
		if(s->type != SBSS) {
			diag("redefinition: %s\n%P", s->name, p);
			s->type = SBSS;
			s->value = 0;
		}
		if(p->to.offset > s->value)
			s->value = p->to.offset;
		break;

	case ADYNT:
		if(p->to.sym == S) {
			diag("DYNT without a sym\n%P", p);
			break;
		}
		di = p->to.sym;
		p->reg = 4;
		if(di->type == SXREF) {
			if(debug['z'])
				Bprint(&bso, "%P set to %d\n", p, dtype);
			di->type = SCONST;
			di->value = dtype;
			dtype += 4;
		}
		if(p->from.sym == S)
			break;

		p->from.offset = di->value;
		p->from.sym->type = SDATA;
		if(curtext == P) {
			diag("DYNT not in text: %P", p);
			break;
		}
		p->to.sym = curtext->from.sym;
		p->to.type = D_CONST;
		p->link = datap;
		datap = p;
		break;

	case AINIT:
		if(p->from.sym == S) {
			diag("INIT without a sym\n%P", p);
			break;
		}
		if(di == S) {
			diag("INIT without previous DYNT\n%P", p);
			break;
		}
		p->from.offset = di->value;
		p->from.sym->type = SDATA;
		p->link = datap;
		datap = p;
		break;
	
	case ADATA:
		if(p->from.sym == S) {
			diag("DATA without a sym\n%P", p);
			break;
		}
		p->link = datap;
		datap = p;
		break;

	case AGOK:
		diag("unknown opcode\n%P", p);
		p->pc = pc;
		pc++;
		break;

	case ATEXT:
		if(curtext != P) {
			histtoauto();
			curtext->to.autom = curauto;
			curauto = 0;
		}
		skip = 0;
		curtext = p;
		if(p->to.offset > 0){
			autosize = (p->to.offset+7L) & ~7L;
			p->to.offset = autosize;
			autosize += PCSZ;
		}else
			autosize = 0;
		s = p->from.sym;
		if(s == S) {
			diag("TEXT must have a name\n%P", p);
			errorexit();
		}
		if(s->type != 0 && s->type != SXREF) {
			if(p->reg & DUPOK) {
				skip = 1;
				goto casedef;
			}
			diag("redefinition: %s\n%P", s->name, p);
		}
		s->type = STEXT;
		s->value = pc;
		lastp->link = p;
		lastp = p;
		p->pc = pc;
		pc++;
		if(textp == P) {
			textp = p;
			etextp = p;
			goto loop;
		}
		etextp->cond = p;
		etextp = p;
		break;

	case ASUB:
		if(isnegoff(p)){
			p->from.offset = -p->from.offset;
			p->as = AADD;
		}
		goto casedef;

	case ASUBW:
		if(isnegoff(p)){
			p->from.offset = -p->from.offset;
			p->as = AADDW;
		}
		goto casedef;

	case ASUBS:
		if(isnegoff(p)){
			p->from.offset = -p->from.offset;
			p->as = AADDS;
		}
		goto casedef;

	case ASUBSW:
		if(isnegoff(p)){
			p->from.offset = -p->from.offset;
			p->as = AADDSW;
		}
		goto casedef;

	case AADD:
		if(isnegoff(p)){
			p->from.offset = -p->from.offset;
			p->as = ASUB;
		}
		goto casedef;

	case AADDW:
		if(isnegoff(p)){
			p->from.offset = -p->from.offset;
			p->as = ASUBW;
		}
		goto casedef;

	case AADDS:
		if(isnegoff(p)){
			p->from.offset = -p->from.offset;
			p->as = ASUBS;
		}
		goto casedef;

	case AADDSW:
		if(isnegoff(p)){
			p->from.offset = -p->from.offset;
			p->as = ASUBSW;
		}
		goto casedef;

	case AFCVTDS:
		if(p->from.type != D_FCONST)
			goto casedef;
		p->as = AFMOVS;
		/* fall through */
	case AFMOVS:
		if(skip)
			goto casedef;

		if(p->from.type == D_FCONST && chipfloat(p->from.ieee) < 0) {
			/* size sb 9 max */
			sprint(literal, "$%lux", ieeedtof(p->from.ieee));
			s = lookup(literal, 0);
			if(s->type == 0) {
				s->type = SBSS;
				s->value = 4;
				t = prg();
				t->as = ADATA;
				t->line = p->line;
				t->from.type = D_OREG;
				t->from.sym = s;
				t->from.name = D_EXTERN;
				t->reg = 4;
				t->to = p->from;
				t->link = datap;
				datap = t;
			}
			p->from.type = D_OREG;
			p->from.sym = s;
			p->from.name = D_EXTERN;
			p->from.offset = 0;
		}
		goto casedef;

	case AFMOVD:
		if(skip)
			goto casedef;

		if(p->from.type == D_FCONST && chipfloat(p->from.ieee) < 0) {
			/* size sb 18 max */
			sprint(literal, "$%lux.%lux",
				p->from.ieee->l, p->from.ieee->h);
			s = lookup(literal, 0);
			if(s->type == 0) {
				s->type = SBSS;
				s->value = 8;
				t = prg();
				t->as = ADATA;
				t->line = p->line;
				t->from.type = D_OREG;
				t->from.sym = s;
				t->from.name = D_EXTERN;
				t->reg = 8;
				t->to = p->from;
				t->link = datap;
				datap = t;
			}
			p->from.type = D_OREG;
			p->from.sym = s;
			p->from.name = D_EXTERN;
			p->from.offset = 0;
		}
		goto casedef;

	default:
	casedef:
		if(skip)
			nopout(p);

		if(p->to.type == D_BRANCH)
			p->to.offset += ipc;
		if(p->from.type == D_BRANCH)
			p->from.offset += ipc;
		lastp->link = p;
		lastp = p;
		p->pc = pc;
		pc++;
		break;
	}
	goto loop;

eof:
	diag("truncated object file: %s", pn);
}

void
doprof1(void)
{
	Sym *s;
	long n;
	Prog *p, *q;

	if(debug['v'])
		Bprint(&bso, "%5.2f profile 1\n", cputime());
	Bflush(&bso);
	s = lookup("__mcount", 0);
	n = 1;
	for(p = firstp->link; p != P; p = p->link) {
		if(p->as == ATEXT) {
			q = prg();
			q->line = p->line;
			q->link = datap;
			datap = q;
			q->as = ADATA;
			q->from.type = D_OREG;
			q->from.name = D_EXTERN;
			q->from.offset = n*4;
			q->from.sym = s;
			q->reg = 4;
			q->to = p->from;
			q->to.type = D_CONST;

			q = prg();
			q->line = p->line;
			q->pc = p->pc;
			q->link = p->link;
			p->link = q;
			p = q;
			p->as = AMOVW;
			p->from.type = D_OREG;
			p->from.name = D_EXTERN;
			p->from.sym = s;
			p->from.offset = n*PCSZ + PCSZ;
			p->to.type = D_REG;
			p->to.reg = REGTMP;

			q = prg();
			q->line = p->line;
			q->pc = p->pc;
			q->link = p->link;
			p->link = q;
			p = q;
			p->as = AADD;
			p->from.type = D_CONST;
			p->from.offset = 1;
			p->to.type = D_REG;
			p->to.reg = REGTMP;

			q = prg();
			q->line = p->line;
			q->pc = p->pc;
			q->link = p->link;
			p->link = q;
			p = q;
			p->as = AMOVW;
			p->from.type = D_REG;
			p->from.reg = REGTMP;
			p->to.type = D_OREG;
			p->to.name = D_EXTERN;
			p->to.sym = s;
			p->to.offset = n*PCSZ + PCSZ;

			n += 2;
			continue;
		}
	}
	q = prg();
	q->line = 0;
	q->link = datap;
	datap = q;

	q->as = ADATA;
	q->from.type = D_OREG;
	q->from.name = D_EXTERN;
	q->from.sym = s;
	q->reg = 4;
	q->to.type = D_CONST;
	q->to.offset = n;

	s->type = SBSS;
	s->value = n*4;
}

void
doprof2(void)
{
	Sym *s2, *s4;
	Prog *p, *q, *q2, *ps2, *ps4;

	if(debug['v'])
		Bprint(&bso, "%5.2f profile 2\n", cputime());
	Bflush(&bso);

	if(debug['e']){
		s2 = lookup("_tracein", 0);
		s4 = lookup("_traceout", 0);
	}else{
		s2 = lookup("_profin", 0);
		s4 = lookup("_profout", 0);
	}
	if(s2->type != STEXT || s4->type != STEXT) {
		if(debug['e'])
			diag("_tracein/_traceout not defined %d %d", s2->type, s4->type);
		else
			diag("_profin/_profout not defined");
		return;
	}

	ps2 = P;
	ps4 = P;
	for(p = firstp; p != P; p = p->link) {
		if(p->as == ATEXT) {
			if(p->from.sym == s2) {
				ps2 = p;
				p->reg = 1;
			}
			if(p->from.sym == s4) {
				ps4 = p;
				p->reg = 1;
			}
		}
	}
	for(p = firstp; p != P; p = p->link) {
		if(p->as == ATEXT) {
			if(p->reg & NOPROF) {
				for(;;) {
					q = p->link;
					if(q == P)
						break;
					if(q->as == ATEXT)
						break;
					p = q;
				}
				continue;
			}

			/*
			 * BL	profin
			 */
			q = prg();
			q->line = p->line;
			q->pc = p->pc;
			q->link = p->link;
			if(debug['e']){		/* embedded tracing */
				q2 = prg();
				p->link = q2;
				q2->link = q;

				q2->line = p->line;
				q2->pc = p->pc;

				q2->as = AB;
				q2->to.type = D_BRANCH;
				q2->to.sym = p->to.sym;
				q2->cond = q->link;
			}else
				p->link = q;
			p = q;
			p->as = ABL;
			p->to.type = D_BRANCH;
			p->cond = ps2;
			p->to.sym = s2;

			continue;
		}
		if(p->as == ARETURN) {
			/*
			 * RET (default)
			 */
			if(debug['e']){		/* embedded tracing */
				q = prg();
				q->line = p->line;
				q->pc = p->pc;
				q->link = p->link;
				p->link = q;
				p = q;
			}

			/*
			 * RETURN
			 */
			q = prg();
			q->as = p->as;
			q->from = p->from;
			q->to = p->to;
			q->cond = p->cond;
			q->link = p->link;
			q->reg = p->reg;
			p->link = q;

			/*
			 * BL	profout
			 */
			p->as = ABL;
			p->from = zprg.from;
			p->to = zprg.to;
			p->to.type = D_BRANCH;
			p->cond = ps4;
			p->to.sym = s4;

			p = q;
			continue;
		}
	}
}

void
nuxiinit(void)
{

	int i, c;

	for(i=0; i<4; i++) {
		c = find1(0x04030201L, i+1);
		if(i < 2)
			inuxi2[i] = c;
		if(i < 1)
			inuxi1[i] = c;
		inuxi4[i] = c;
		fnuxi4[i] = c;
		inuxi8[i] = c;
		inuxi8[i+4] = c+4;
		fnuxi8[i] = c;
		fnuxi8[i+4] = c+4;
	}
	if(debug['v']) {
		Bprint(&bso, "inuxi = ");
		for(i=0; i<1; i++)
			Bprint(&bso, "%d", inuxi1[i]);
		Bprint(&bso, " ");
		for(i=0; i<2; i++)
			Bprint(&bso, "%d", inuxi2[i]);
		Bprint(&bso, " ");
		for(i=0; i<4; i++)
			Bprint(&bso, "%d", inuxi4[i]);
		Bprint(&bso, " ");
		for(i=0; i<8; i++)
			Bprint(&bso, "%d", inuxi8[i]);
		Bprint(&bso, "\nfnuxi = ");
		for(i=0; i<4; i++)
			Bprint(&bso, "%d", fnuxi4[i]);
		Bprint(&bso, " ");
		for(i=0; i<8; i++)
			Bprint(&bso, "%d", fnuxi8[i]);
		Bprint(&bso, "\n");
	}
	Bflush(&bso);
}
