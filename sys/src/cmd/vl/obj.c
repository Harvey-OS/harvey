#include	"l.h"
#include	<ar.h>

#ifndef	DEFAULT
#define	DEFAULT	'9'
#endif

char	*noname		= "<none>";
char	symname[]	= SYMDEF;

/*
 *	-H0 -T0x40004C -D0x10000000	is abbrev unix
 *	-H1 -T0x80020000 -R4		is bootp() format
 *	-H2 -T4128 -R4096		is plan9 format
 */

void
main(int argc, char *argv[])
{
	int c;
	char *a;

	Binit(&bso, 1, OWRITE);
	listinit();
	outfile = 0;
	nerrors = 0;
	curtext = P;
	HEADTYPE = -1;
	INITTEXT = -1;
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
	case 'T':
		a = ARGF();
		if(a)
			INITTEXT = atolwhex(a);
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
		/* do something about setting INITTEXT */
		break;
	} ARGEND

	USED(argc);

	if(*argv == 0) {
		diag("usage: vl [-options] objects\n");
		errorexit();
	}
	if(!debug['9'] && !debug['U'] && !debug['B'])
		debug[DEFAULT] = 1;
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

	case 0:	/* unix simple */
		HEADR = 20L+56L;
		if(INITTEXT == -1)
			INITTEXT = 0x40004CL;
		if(INITDAT == -1)
			INITDAT = 0x10000000L;
		if(INITRND == -1)
			INITRND = 0;
		break;
	case 1:	/* boot */
		HEADR = 20L+60L;
		if(INITTEXT == -1)
			INITTEXT = 0x80020000L;
		if(INITDAT == -1)
			INITDAT = 0;
		if(INITRND == -1)
			INITRND = 4;
		break;
	case 2:	/* plan 9 */
		HEADR = 32L;
		if(INITTEXT == -1)
			INITTEXT = 4128;
		if(INITDAT == -1)
			INITDAT = 0;
		if(INITRND == -1)
			INITRND = 4096;
		break;
	}
	if(INITDAT != 0 && INITRND != 0)
		print("warning: -D0x%lux is ignored because of -R0x%lux\n",
			INITDAT, INITRND);
	if(debug['v'])
		Bprint(&bso, "HEADER = -H0x%ld -T0x%lux -D0x%lux -R0x%lux\n",
			HEADTYPE, INITTEXT, INITDAT, INITRND);
	Bflush(&bso);
	zprg.as = AGOK;
	zprg.reg = NREG;
	zprg.from.name = D_NONE;
	zprg.from.type = D_NONE;
	zprg.from.reg = NREG;
	zprg.to = zprg.from;
	buildop();
	histgen = 0;
	textp = P;
	datap = P;
	pc = 0;
	if(outfile == 0)
		outfile = "v.out";
	cout = create(outfile, 1, 0775);
	if(cout < 0) {
		diag("%s: cannot create\n", outfile);
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
	}
	if(!debug['l'])
		lookup(INITENTRY, 0)->type = SXREF;

	while(*argv)
		objfile(*argv++);
	if(!debug['l'])
		loadlib(0, libraryp);
	firstp = firstp->link;
	if(firstp == P)
		goto out;
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
		Bprint(&bso, "%ld memory used\n", thunk);
		Bprint(&bso, "%d sizeof adr\n", sizeof(Adr));
		Bprint(&bso, "%d sizeof prog\n", sizeof(Prog));
	}
	Bflush(&bso);
	errorexit();
}

void
loadlib(int beg, int end)
{
	int i, t;

	for(i=end-1; i>=beg; i--) {
		t = libraryp;
		if(debug['v'])
			Bprint(&bso, "%5.2f autolib: %s\n", cputime(), library[i]);
		objfile(library[i]);
		if(t != libraryp)
			loadlib(t, libraryp);
	}
}

void
errorexit(void)
{

	if(nerrors) {
		if(cout >= 0)
			remove(outfile);
		exits("error");
	}
	exits(0);
}

void
objfile(char *file)
{
	long off, esym, cnt, i, l;
	int f, work;
	Sym *s;
	char magbuf[SARMAG];
	char name[100], pname[150];
	struct ar_hdr arhdr;
	Rlent *e;

	if(file[0] == '-' && file[1] == 'l') {
		if(debug['9'])
			strcpy(name, "/mips/lib/lib");
		else
			strcpy(name, "/usr/vlib/lib");
		strcat(name, file+2);
		strcat(name, ".a");
		file = name;
	}
	if(debug['v'])
		Bprint(&bso, "%5.2f ldobj: %s\n", cputime(), file);
	Bflush(&bso);
	f = open(file, 0);
	if(f < 0) {
		diag("cannot open file: %s\n", file);
		errorexit();
	}
	l = read(f, magbuf, SARMAG);
	if(l != SARMAG)
		goto ldone;
	if(strncmp(magbuf, ARMAG, SARMAG))
		goto ldone;

	l = read(f, &arhdr, sizeof(struct ar_hdr));
	if(l != sizeof(struct ar_hdr)) {
		diag("%s: short read on archive file symbol header\n", file);
		goto out;
	}
	if(strncmp(arhdr.name, symname, strlen(symname))) {
		diag("%s: first entry not symbol header\n", file);
		goto out;
	}
	esym = SARMAG + sizeof(struct ar_hdr) + atolwhex(arhdr.size);

loop:	/* start over */
	work = 0;
	off = SARMAG + sizeof(struct ar_hdr);

l1:	/* read next block */
	seek(f, off, 0);
	cnt = esym - off;
	if(cnt > sizeof(buf.rlent))
		cnt = sizeof(buf.rlent);
	cnt = read(f, buf.rlent, cnt);
	if(cnt <= 0) {
		if(work)
			goto loop;
		close(f);
		return;
	}
	cnt /= sizeof(buf.rlent[0]);
	off += cnt*sizeof(buf.rlent[0]);
	e = &buf.rlent[0];
	for(i=0; i<cnt; i++,e++) {
		e->name[NNAME-1] = 0;
		s = lookup(e->name, 0);
		if(s->type != SXREF)
			continue;
		sprint(pname, "%s(%s)", file, e->name);
		if(debug['v'])
			Bprint(&bso, "%5.2f library: %s\n", cputime(), pname);
		Bflush(&bso);
		l = e->coffset[0] & 0xff;
		l |= (e->coffset[1] & 0xff) << 8;
		l |= (e->coffset[2] & 0xff) << 16;
		l |= (e->coffset[3] & 0xff) << 24;
		seek(f, l, 0);
		l = read(f, &arhdr, sizeof(struct ar_hdr));
		if(l != sizeof(struct ar_hdr))
			goto bad;
		if(strncmp(arhdr.fmag, ARFMAG, sizeof(arhdr.fmag)))
			goto bad;
		l = atolwhex(arhdr.size);
		ldobj(f, l, pname);
		if(s->type == SXREF) {
			diag("%s: failed to load: %s\n", file, s->name);
			errorexit();
		}
		work = 1;
	}
	goto l1;

ldone:	/* load it as a regular file */
	l = seek(f, 0L, 2);
	seek(f, 0L, 0);
	ldobj(f, l, file);
	close(f);
	return;

bad:
	diag("%s: bad or out of date archive\n", file);
out:
	close(f);
}

int
zaddr(uchar *p, Adr *a, Sym *h[])
{
	int i, c;
	long l;
	Sym *s;
	Auto *u;

	a->type = p[0];
	a->reg = p[1];
	a->sym = h[p[2]];
	a->name = p[3];
	c = 4;

	if(a->reg < 0 || a->reg > NREG) {
		print("register out of range %d\n", a->reg);
		p[0] = AEND+1;
		return 0;	/*  force real diagnostic */
	}

	switch(a->type) {
	default:
		print("unknown type %d\n", a->type);
		p[0] = AEND+1;
		return 0;	/*  force real diagnostic */

	case D_NONE:
	case D_REG:
	case D_FREG:
	case D_MREG:
	case D_FCREG:
	case D_LO:
	case D_HI:
		break;

	case D_BRANCH:
	case D_OREG:
	case D_CONST:
		a->offset = p[4] | (p[5]<<8) |
			(p[6]<<16) | (p[7]<<24);
		c += 4;
		break;

	case D_SCONST:
		while(nhunk < NSNAME)
			gethunk();
		a->sval = (char*)hunk;
		nhunk -= NSNAME;
		hunk += NSNAME;

		memmove(a->sval, p+4, NSNAME);
		c += NSNAME;
		break;

	case D_FCONST:
		while(nhunk < sizeof(Ieee))
			gethunk();
		a->ieee = (Ieee*)hunk;
		nhunk -= NSNAME;
		hunk += NSNAME;

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
		if(u->sym == s)
		if(u->type == i) {
			if(u->offset > l)
				u->offset = l;
			return c;
		}

	while(nhunk < sizeof(Auto))
		gethunk();
	u = (Auto*)hunk;
	nhunk -= sizeof(Auto);
	hunk += sizeof(Auto);

	u->link = curauto;
	curauto = u;
	u->sym = s;
	u->offset = l;
	u->type = i;
	return c;
}

void
addlib(long line)
{
	char name[100], comp[100], *p;
	int i;

	USED(line);
	if(histfrogp <= 0)
		return;

	if(histfrog[0]->name[1] == '/') {
		sprint(name, "");
		i = 1;
	} else
	if(histfrog[0]->name[1] == '.') {
		sprint(name, ".");
		i = 0;
	} else {
		if(debug['9'])
			strcpy(name, "/mips/lib");
		else
			strcpy(name, "/usr/vlib");
		i = 0;
	}

	for(; i<histfrogp; i++) {
		sprint(comp, histfrog[i]->name+1);
		for(;;) {
			p = strstr(comp, "$O");
			if(p == 0)
				break;
			memmove(p+1, p+2, strlen(p+2)+1);
			p[0] = 'v';
		}
		for(;;) {
			p = strstr(comp, "$M");
			if(p == 0)
				break;
			memmove(p+4, p+2, strlen(p+2)+1);
			p[0] = 'm';
			p[1] = 'i';
			p[2] = 'p';
			p[3] = 's';
		}
		if(strlen(name) + strlen(comp) + 3 >= sizeof(name)) {
			diag("library component too long");
			return;
		}
		strcat(name, "/");
		strcat(name, comp);
	}
	for(i=0; i<libraryp; i++)
		if(strcmp(name, library[i]) == 0)
			return;

	i = strlen(name) + 1;
	while(i & 3)
		i++;
	while(nhunk < i)
		gethunk();
	p = (char*)hunk;
	nhunk -= i;
	hunk += i;

	strcpy(p, name);
	library[libraryp] = p;
	libraryp++;
}

void
addhist(long line, int type)
{
	Auto *u;
	Sym *s;
	int i, j, k;

	while(nhunk < sizeof(Auto))
		gethunk();
	u = (Auto*)hunk;
	nhunk -= sizeof(Auto);
	hunk += sizeof(Auto);

	while(nhunk < sizeof(Sym))
		gethunk();
	s = (Sym*)hunk;
	nhunk -= sizeof(Sym);
	hunk += sizeof(Sym);

	u->sym = s;
	u->type = type;
	u->offset = line;
	u->link = curhist;
	curhist = u;

	j = 1;
	for(i=0; i<histfrogp; i++) {
		k = histfrog[i]->value;
		s->name[j+0] = k>>8;
		s->name[j+1] = k;
		j += 2;
	}
}

void
histtoauto(void)
{
	Auto *l;

	while(l = curhist) {
		curhist = l->link;
		l->link = curauto;
		curauto = l;
	}
}

void
ldobj(int f, long c, char *pn)
{
	Prog *p, *t;
	Sym *h[NSYM], *s;
	char name[NNAME];
	int v, o, i, r;
	long ipc;
	uchar *bloc, *bsize;

	bsize = buf.xbuf;
	bloc = buf.xbuf;

newloop:
	memset(h, 0, sizeof(h));
	version++;
	histfrogp = 0;
	ipc = pc;

loop:
	if(c <= 0)
		goto eof;
	r = bsize - bloc;
	if(r < 100 && r < c) {		/* enough for largest prog */
		i = (ulong)r & 7;	/* non allignment factor */
		if(i)
			i = 8-i;
		memmove(buf.xbuf+i, bloc, r);
		bloc = buf.xbuf+i;
		v = sizeof(buf.xbuf) - (i+r);
		if(v > c)
			v = c;
		v = read(f, bloc+r, v);
		if(v <= 0)
			goto eof;
		bsize = bloc+r + v;
		goto loop;
	}
	o = bloc[0];		/* as */
	if(o <= 0 || o > AEND) {
		diag("%s: opcode out of range %d\n", pn, o);
		print("	probably not a .v file\n");
		errorexit();
	}
	if(o == ANAME) {
		v = bloc[1];	/* type */
		o = bloc[2];	/* sym */
		bloc += 3;
		c -= 3;
		memset(name, 0, NNAME);
		for(i=0;; i++) {
			r = bloc[0];
			bloc++;
			c--;
			if(r == 0)
				break;
			if(i < NNAME)
				name[i] = r;
		}
		r = 0;
		if(v == D_STATIC)
			r = version;
		s = lookup(name, r);
		if(debug['W'])
			print("	ANAME	%s\n", s->name);
		h[o] = s;
		if(v == D_EXTERN && s->type == 0)
			s->type = SXREF;
		if(v == D_FILE) {
			if(s->type != SFILE) {
				histgen++;
				s->type = SFILE;
				s->value = histgen;
			}
			if(histfrogp < nelem(histfrog)) {
				histfrog[histfrogp] = s;
				histfrogp++;
			}
		}
		goto loop;
	}

	if(nhunk < sizeof(Prog))
		gethunk();
	p = (Prog*)hunk;
	nhunk -= sizeof(Prog);
	hunk += sizeof(Prog);

	p->as = o;
	p->reg = bloc[1];
	p->line = bloc[2] | (bloc[3]<<8) | (bloc[4]<<16) | (bloc[5]<<24);

	r = zaddr(bloc+6, &p->from, h) + 6;
	r += zaddr(bloc+r, &p->to, h);
	bloc += r;
	c -= r;

	if(p->reg < 0 || p->reg > NREG)
		diag("register out of range %d\n", p->reg);

	p->link = P;
	p->cond = P;

	if(debug['W'])
		print("%P\n", p);

	switch(o) {
	case AHISTORY:
		if(p->to.offset == -1) {
			addlib(p->line);
			histfrogp = 0;
			goto loop;
		}
		addhist(p->line, D_FILE);		/* 'z' */
		if(p->to.offset)
			addhist(p->to.offset, D_MREG);	/* 'Z' */
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
			diag("GLOBL must have a name\n%P\n", p);
			errorexit();
		}
		if(s->type == 0 || s->type == SXREF) {
			s->type = SBSS;
			s->value = 0;
		}
		if(s->type != SBSS) {
			diag("redefinition: %s\n%P\n", s->name, p);
			s->type = SBSS;
			s->value = 0;
		}
		if(p->to.offset > s->value)
			s->value = p->to.offset;
		break;

	case ADATA:
		if(p->from.sym == S) {
			diag("DATA without a sym\n%P\n", p);
			break;
		}
		p->link = datap;
		datap = p;
		break;

	case AGOK:
		diag("unknown opcode\n%P\n", p);
		p->pc = pc;
		pc++;
		break;

	case ATEXT:
		if(curtext != P) {
			histtoauto();
			curtext->to.autom = curauto;
			curauto = 0;
		}
		curtext = p;
		autosize = (p->to.offset+3L) & ~3L;
		p->to.offset = autosize;
		autosize += 4;
		s = p->from.sym;
		if(s == S) {
			diag("TEXT must have a name\n%P\n", p);
			errorexit();
		}
		if(s->type != 0 && s->type != SXREF)
			diag("redefinition: %s\n%P\n", s->name, p);
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
	case ASUBU:
		if(p->from.type == D_CONST)
		if(p->from.name == D_NONE) {
			p->from.offset = -p->from.offset;
			if(p->as == ASUB)
				p->as = AADD;
			else
				p->as = AADDU;
		}
		goto casedef;

	case AMOVF:
		if(p->from.type == D_FCONST) {
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

	case AMOVD:
		if(p->from.type == D_FCONST) {
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
		if(p->to.type == D_BRANCH)
			p->to.offset += ipc;
		lastp->link = p;
		lastp = p;
		p->pc = pc;
		pc++;
		break;
	}
	goto loop;

eof:
	diag("truncated object file: %s\n", pn);
}

Sym*
lookup(char *symb, int v)
{
	Sym *s;
	char *p;
	long h;
	int c0, l;

	h = v;
	for(p=symb; c0 = *p; p++)
		h = h+h+h + c0;
	l = (p - symb) + 1;
	if(h < 0)
		h = ~h;
	h %= NHASH;
	for(s = hash[h]; s != S; s = s->link)
		if(s->version == v)
		if(memcmp(s->name, symb, l) == 0)
			return s;

	while(nhunk < sizeof(Sym))
		gethunk();
	s = (Sym*)hunk;
	nhunk -= sizeof(Sym);
	hunk += sizeof(Sym);

	if(l > sizeof(s->name))
		diag("long name in lookup %s\n", symb);
	memmove(s->name, symb, l);
	s->link = hash[h];
	s->type = 0;
	s->version = v;
	s->value = 0;
	hash[h] = s;
	return s;
}

Prog*
prg(void)
{
	Prog *p;

	while(nhunk < sizeof(Prog))
		gethunk();
	p = (Prog*)hunk;
	nhunk -= sizeof(Prog);
	hunk += sizeof(Prog);

	*p = zprg;
	return p;
}

void
gethunk(void)
{
	char *h;
	long nh;

	nh = NHUNK;
	if(thunk >= 5L*NHUNK) {
		nh = 5L*NHUNK;
		if(thunk >= 25L*NHUNK)
			nh = 25L*NHUNK;
	}
	h = sbrk(nh);
	if(h == (char*)-1) {
		diag("out of memory\n");
		errorexit();
	}
	hunk = h;
	nhunk = nh;
	thunk += nh;
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
			p->from.offset = n*4 + 4;
			p->to.type = D_REG;
			p->to.reg = REGTMP;

			q = prg();
			q->line = p->line;
			q->pc = p->pc;
			q->link = p->link;
			p->link = q;
			p = q;
			p->as = AADDU;
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
			p->to.offset = n*4 + 4;

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
	Prog *p, *q, *ps2, *ps4;

	if(debug['v'])
		Bprint(&bso, "%5.2f profile 2\n", cputime());
	Bflush(&bso);
	s2 = lookup("_profin", 0);
	s4 = lookup("_profout", 0);
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
			if(p->reg != NREG) {
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
			 * JAL	profin, R2
			 */
			q = prg();
			q->line = p->line;
			q->pc = p->pc;
			q->link = p->link;
			p->link = q;
			p = q;
			p->as = AJAL;
			p->to.type = D_BRANCH;
			p->cond = ps2;
			p->to.sym = s2;

			continue;
		}
		if(p->as == ARET) {
			/*
			 * JAL	profout
			 */
			p->as = AJAL;
			p->to.type = D_BRANCH;
			p->cond = ps4;
			p->to.sym = s4;

			
			/*
			 * RET
			 */
			q = prg();
			q->line = p->line;
			q->pc = p->pc;
			q->link = p->link;
			p->link = q;
			p = q;
			p->as = ARET;

			continue;
		}
	}
}

void
nuxiinit(void)
{
	int i, c;

	for(i=0; i<4; i++) {
		c = find1(0x01020304L, i+1);
		if(i >= 2)
			inuxi2[i-2] = c;
		if(i >= 3)
			inuxi1[i-3] = c;
		inuxi4[i] = c;

		fnuxi8[i] = c+4;
		fnuxi8[i+4] = c;
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
		Bprint(&bso, "\nfnuxi = ");
		for(i=0; i<8; i++)
			Bprint(&bso, "%d", fnuxi8[i]);
		Bprint(&bso, "\n");
	}
	Bflush(&bso);
}

find1(long l, int c)
{
	char *p;
	int i;

	p = (char*)&l;
	for(i=0; i<4; i++)
		if(*p++ == c)
			return i;
	return 0;
}

long
ieeedtof(Ieee *ieee)
{
	int exp;
	long v;

	if(ieee->h == 0)
		return 0;
	exp = (ieee->h>>20) & ((1L<<11)-1L);
	exp -= (1L<<10) - 2L;
	v = (ieee->h & 0xfffffL) << 3;
	v |= (ieee->l >> 29) & 0x7L;
	if((ieee->l >> 28) & 1) {
		v++;
		if(v & 0x800000L) {
			v = (v & 0x7fffffL) >> 1;
			exp++;
		}
	}
	if(exp <= -126 || exp >= 130)
		diag("double fp to single fp overflow\n");
	v |= ((exp + 126) & 0xffL) << 23;
	v |= ieee->h & 0x80000000L;
	return v;
}

double
ieeedtod(Ieee *ieee)
{
	Ieee e;
	double fr;
	int exp;

	if(ieee->h & (1L<<31)) {
		e.h = ieee->h & ~(1L<<31);
		e.l = ieee->l;
		return -ieeedtod(&e);
	}
	if(ieee->l == 0 && ieee->h == 0)
		return 0;
	fr = ieee->l & ((1L<<16)-1L);
	fr /= 1L<<16;
	fr += (ieee->l>>16) & ((1L<<16)-1L);
	fr /= 1L<<16;
	fr += (ieee->h & (1L<<20)-1L) | (1L<<20);
	fr /= 1L<<21;
	exp = (ieee->h>>20) & ((1L<<11)-1L);
	exp -= (1L<<10) - 2L;
	return ldexp(fr, exp);
}

/*
 * fake malloc
 */
void*
malloc(long n)
{
	void *p;

	while(n & 3)
		n++;
	while(nhunk < n)
		gethunk();
	p = hunk;
	nhunk -= n;
	hunk += n;
	return p;
}

void
free(void *p)
{
	USED(p);
}

void*
calloc(long m, long n)
{
	void *p;

	n *= m;
	p = malloc(n);
	memset(p, 0, n);
	return p;
}

void*
realloc(void *p, long n)
{
	fprint(2, "realloc called\n", p, n);
	abort();
	return 0;
}
