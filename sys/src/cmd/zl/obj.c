#include	"l.h"
#include	<ar.h>

#ifndef	DEFAULT
#define	DEFAULT	'9'
#endif

char	*noname		= "<none>";
char	symname[]	= SYMDEF;

void
main(int argc, char *argv[])
{
	int i, c;
	char *a;

	Binit(&bso, 1, OWRITE);
	cout = -1;
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
	case 'H':
		a = ARGF();
		if(a)
			HEADTYPE = atolwhex(a);
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
	} ARGEND

	USED(argc);

	if(*argv == 0) {
		diag("usage: zl [-options] objects\n");
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
	if(INITDAT != -1 && INITRND == -1)
		INITRND = 0;

	switch(HEADTYPE) {
	default:
		diag("unknown -H option %d", HEADTYPE);
		errorexit();

	case 0:	/* unix simple */
		HEADR = 20L+56L;
		if(INITTEXT == -1)
			INITTEXT = 0x40004CL;
		if(INITDAT == -1)
			INITDAT = 0x10000000L;
		if(INITDAT != 0 && INITRND == -1)
			INITRND = 0;
		if(INITRND == -1)
			INITRND = 0;
		break;
	case 1:	/* boot */
		HEADR = 20L+60L;
		if(INITTEXT == -1)
			INITTEXT = 0x80020000L;
		if(INITDAT == -1)
			INITDAT = 0;
		if(INITDAT != 0 && INITRND == -1)
			INITRND = 0;
		if(INITRND == -1)
			INITRND = 4;
		break;
	case 2:	/* plan 9 */
		HEADR = 32L;
		if(INITTEXT == -1)
			INITTEXT = 4128;
		if(INITDAT == -1)
			INITDAT = 0;
		if(INITDAT != 0 && INITRND == -1)
			INITRND = 0;
		if(INITRND == -1)
			INITRND = 4096;
		break;
	}
	if(INITDAT != 0 && INITRND != 0)
		Bprint(&bso, "warning: -D0x%lux is ignored because of -R0x%lux\n",
			INITDAT, INITRND);
	if(debug['v'])
		Bprint(&bso, "HEADER = -H0x%ld -T0x%lux -D0x%lux -R0x%lux\n",
			HEADTYPE, INITTEXT, INITDAT, INITRND);
	Bflush(&bso);

	for(i=1; optab[i].as; i++)
		if(i != optab[i].as) {
			diag("phase error in optab: %A\n", i);
			errorexit();
		}

	textp = P;
	datap = P;
	pc = 0;
	if(outfile == 0)
		outfile = "z.out";
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
		errorexit();
	called();
	dodata();
	patch();
	if(debug['p'])
		if(debug['1'])
			doprof1();
		else
			doprof2();
	follow();
	bugs();
	span();
	asmb();
	undef();
	if(debug['v']) {
		Bprint(&bso, "%5.2f cpu time\n", cputime());
		Bprint(&bso, "%ld memory used\n", tothunk);
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
			strcpy(name, "/hobbit/lib/lib");
		else
			strcpy(name, "/usr/zlib/lib");
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
	if(debug['v'])
		Bprint(&bso, "%5.2f search lib: %s\n", cputime(), file);
	Bflush(&bso);
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
	int c, n;
	long l;
	Sym *s;
	Auto *u;

	a->type = p[0];
	a->width = p[1];
	a->sym = h[p[2]];

	switch(n = a->type & ~D_INDIR) {
	default:
		Bprint(&bso, "unknown type %d\n", a->type);
		p[0] = AEND+1;	/* force later diagnostic */
		return 0;

	case D_NONE:
		c = 3;
		break;

	case D_CPU:
	case D_AUTO:
	case D_PARAM:
	case D_CONST:
	case D_ADDR:
	case D_REG:
	case D_EXTERN:
	case D_STATIC:
	case D_BRANCH:
		a->offset = p[3] | (p[4]<<8) |
			(p[5]<<16) | (p[6]<<24);
		c = 7;
		break;

	case D_SCONST:
		c = p[3];	/* width */
		if(c > NSNAME) {
			diag("width > NSNAME\n");
			c = NSNAME;
		}
		memmove(a->sval, p+4, c);
		a->width = c;
		c += 4;
		break;

	case D_FCONST:
		a->ieee.l = p[3] | (p[4]<<8) |
			(p[5]<<16) | (p[6]<<24);
		a->ieee.h = p[7] | (p[8]<<8) |
			(p[9]<<16) | (p[10]<<24);
		c = 11;
		break;
	}

	if(n != D_AUTO && n != D_PARAM)
		goto out;
	s = a->sym;
	if(s == S)
		goto out;
	l = a->offset;
	for(u=curauto; u; u=u->link)
		if(u->sym == s)
		if(u->type == n) {
			if(u->offset > l)
				u->offset = l;
			goto out;
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
	u->type = n;

out:
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
			strcpy(name, "/hobbit/lib");
		else
			strcpy(name, "/usr/zlib");
		i = 0;
	}

	for(; i<histfrogp; i++) {
		sprint(comp, histfrog[i]->name+1);
		for(;;) {
			p = strstr(comp, "$O");
			if(p == 0)
				break;
			memmove(p+1, p+2, strlen(p+2)+1);
			p[0] = 'z';
		}
		for(;;) {
			p = strstr(comp, "$M");
			if(p == 0)
				break;
			memmove(p+6, p+2, strlen(p+2)+1);
			p[0] = 'h';
			p[1] = 'o';
			p[2] = 'b';
			p[3] = 'b';
			p[4] = 'i';
			p[5] = 't';
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
	Auto *u, *t;
	Sym *s;
	int i, j, k;

	while(nhunk < sizeof(Auto))
		gethunk();
	u = (Auto*)hunk;
	nhunk -= sizeof(Auto);
	hunk += sizeof(Auto);

	if(curauto) {
		for(t=curauto; t->link; t=t->link)
			;
		t->link = u;
	} else
		curauto = u;

	while(nhunk < sizeof(Sym))
		gethunk();
	s = (Sym*)hunk;
	nhunk -= sizeof(Sym);
	hunk += sizeof(Sym);

	u->sym = s;
	u->link = 0;
	u->type = type;
	u->offset = line;

	j = 1;
	s->name[0] = 0;
	for(i=0; i<histfrogp; i++) {
		k = histfrog[i]->value;
		s->name[j+0] = k>>8;
		s->name[j+1] = k;
		j += 2;
	}
}

void
ldobj(int f, long c, char *pn)
{
	Prog *p, *t;
	Sym *h[NSYM], *s;
	char name[NNAME];
	int v, o, i, r;
	uchar *bloc, *bsize;
	long ipc;

	bsize = buf.xbuf;
	bloc = buf.xbuf;
	s = 0; /* set */

newloop:
	memset(h, 0, sizeof(h));
	histfrogp = 0;
	version++;
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
		if(o < 0)
			goto eof;
		diag("%s: opcode out of range %d\n", pn, o);
		Bprint(&bso, "	probably not a .z file\n");
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
			Bprint(&bso, "	ANAME	%s\n", s->name);
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
	while(nhunk < sizeof(Prog))
		gethunk();
	p = (Prog*)hunk;
	nhunk -= sizeof(Prog);
	hunk += sizeof(Prog);
/*	*p = zprg; */

	p->as = o;
	p->line = bloc[1] | (bloc[2]<<8) | (bloc[3]<<16) | (bloc[4]<<24);
	r = zaddr(bloc+5, &p->from, h) + 5;
	r += zaddr(bloc+r, &p->to, h);
	bloc += r;
	c -= r;

	p->back = 2;

	if(debug['W'])
		Bprint(&bso, "%P\n", p);

	switch(p->as) {
	case AHISTORY:
		if(p->to.offset == -1) {
			addlib(p->line);
			histfrogp = 0;
			goto loop;
		}
		if(p->to.offset)
			addhist(p->to.offset, D_CPU);
		addhist(p->line, D_FILE);
		histfrogp = 0;
		goto loop;

	case AEND:
		if(curtext != P)
			curtext->to.autom = curauto;
		curtext = P;
		curauto = 0;
		if(c)
			goto newloop;
		return;

	case AGLOBL:
		s = p->from.sym;
		if(s->type == 0 || s->type == SXREF) {
			s->type = SBSS;
			s->value = 0;
		}
		if(s->type != SBSS) {
			diag("%s: redefinition: %s in %s\n",
				pn, s->name, TNAME);
			s->type = SBSS;
			s->value = 0;
		}
		if(p->to.offset > s->value)
			s->value = p->to.offset;
		goto loop;

	case ADATA:
		p->link = datap;
		datap = p;
		ndata++;
		goto loop;

	case AGOK:
		diag("%s: unknown opcode in %s\n", pn, TNAME);
		pc++;
		goto loop;

	case ATEXT:
		if(curtext != P) {
			curtext->to.autom = curauto;
			curauto = 0;
		}
		curtext = p;
		autosize = p->to.offset;
		autosize += 15;
		autosize &= ~15;
		p->to.offset = autosize;

		lastp->link = p;
		lastp = p;
		p->pc = pc;
		s = p->from.sym;
		s->lstack = autosize;
		if(autosize > SBSIZE)
			s->lstack = SBSIZE;
		s->rstack = 0;
		if(s->type != 0 && s->type != SXREF)
			diag("%s: redefinition: %s\n", pn, s->name);
		s->type = STEXT;
		s->value = p->pc;
		pc++;
		p->cond = P;
		if(textp == P) {
			textp = p;
			goto loop;
		}
		for(t = textp; t->cond != P; t = t->cond)
			;
		t->cond = p;
		goto loop;

	case AWORD:
		if(p->from.type != D_NONE && p->to.type != D_CONST) {
			diag("%s: bad addressing: %s\n", pn, s->name);
			p->from.type = D_NONE;
			p->to.type = D_CONST;
			p->to.offset = 0;
		}
		goto casedef;

	case AJMP1:
		p->as = AJMP;

	case AJMP:
		s = p->to.sym;
		if(p->to.type != D_BRANCH)
			p->as = AJMP1;
		goto casedef;

	case AJMPF1:
		p->as = AJMPF;

	case AJMPF:
		s = p->to.sym;
		if(p->to.type != D_BRANCH)
			p->as = AJMPF1;
		goto casedef;

	case AJMPT1:
		p->as = AJMPT;

	case AJMPT:
		s = p->to.sym;
		if(p->to.type != D_BRANCH)
			p->as = AJMPT1;
		goto casedef;

	case ACALL1:
		p->as = ACALL;

	case ACALL:
		s = p->to.sym;
		if(p->to.type != D_EXTERN && p->to.type != D_STATIC) {
			p->as = ACALL1;
			s = S;
		}
		if(s != S) {
			for(t = s->calledby; t != P; t = t->link)
				if(t->to.sym == curtext->from.sym)
					break;
			if(t == P) {
				t = prg();
				t->link = s->calledby;
				s->calledby = t;
				t->to.sym = curtext->from.sym;
				t->from.sym = s;
			}
		}
		goto casedef;

/*
	case AMOV:
		if(p->from.type == D_ADDR) {
			p->as = AMOVA;
			p->from.type = p->from.width + D_NONE;
			p->from.width = W_NONE;
		}
		goto casedef;
 */

	case ASUB:
		if(p->from.type == D_CONST) {
			p->as = AADD;
			p->from.offset = -p->from.offset;
		}
		goto casedef;

	case ACMPEQ:
	case AADD3:
		if(p->to.type == D_CONST) {
			Adr a;
			a = p->from;
			p->from = p->to;
			p->to = a;
		}
		goto casedef;

	default:
	casedef:
		if(p->from.type == D_FCONST)
		if(!exactieeef(&p->from.ieee)) {
			/* size sb 18 max */
			sprint(literal, "$%lux_%lux",
				p->from.ieee.l, p->from.ieee.h);
			s = lookup(literal, 0);
			if(s->type == 0) {
				s->type = SBSS;
				s->value = 8;
				t = prg();
				t->as = ADATA;
				t->line = p->line;
				t->from.type = D_EXTERN;
				t->from.sym = s;
				t->from.width = W_D;	/* jmk 12/27/91 */
				t->to = p->from;
				t->link = datap;
				datap = t;
			}
			p->from.type = D_EXTERN;
			p->from.sym = s;
			p->from.offset = 0;
			p->from.width = W_D;
		}
		if(p->to.type == D_BRANCH)
			p->to.offset += ipc;
		lastp->link = p;
		lastp = p;
		p->pc = pc;
		pc++;
		goto loop;
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

	h = v;
	for(p=symb; *p;) {
		h *= 3L;
		h += *p++;
	}
	if(h < 0)
		h = ~h;
	h %= NHASH;
	for(s = hash[h]; s != S; s = s->link)
		if(s->version == v)
		if(strcmp(s->name, symb) == 0)
			return s;
	while(nhunk < sizeof(Sym))
		gethunk();
	s = (Sym*)hunk;
	nhunk -= sizeof(Sym);
	hunk += sizeof(Sym);
	strcpy(s->name, symb);
	s->link = hash[h];
	s->type = 0;
	s->version = v;
	s->value = 0;
	s->calledby = P;
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

	p->link = P;
	p->cond = P;
	p->back = 2;
	p->as = AGOK;
	p->from.type = D_NONE;
	p->to.type = D_NONE;
	return p;
}

void
gethunk(void)
{
	char *h;

	h = sbrk(NHUNK);
	if(h == (char*)-1) {
		diag("out of memory\n");
		errorexit();
	}
	if(hunk == 0)
		hunk = h;
	nhunk += NHUNK;
	tothunk += NHUNK;
}

void
doprof1(void)
{
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
				p->from.width = W_B;
			}
			if(p->from.sym == s4) {
				ps4 = p;
				p->from.width = W_B;
			}
		}
	}
	for(p = firstp; p != P; p = p->link) {
		if(p->as == ATEXT) {
			if(p->from.width == W_B) {
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
			 * CALL	profin
			 */
			q = prg();
			q->line = p->line;
			q->pc = p->pc;
			q->link = p->link;
			p->link = q;
			p = q;
			p->as = ACALL;
			p->to.type = D_BRANCH;
			p->cond = ps2;
			p->to.sym = s2;

			continue;
		}
		if(p->as == ARETURN) {
			q = prg();
			*q = *p;

			/*
			 * CALL	profout
			 */
			p->as = ACALL;
			p->to.type = D_BRANCH;
			p->cond = ps4;
			p->to.sym = s4;

			
			/*
			 * RETURN
			 */
			q->link = p->link;
			p->link = q;
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
		Bprint(&bso, "\nfnuxi = ");
		for(i=0; i<8; i++)
			Bprint(&bso, "%d", fnuxi8[i]);
		Bprint(&bso, "\n");
	}
	Bflush(&bso);
}

int
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

int
exactieeef(Ieee *ieee)
{
	int exp;

	if(ieee->h == 0)
		return 1;	/* zero is exact */
	if(ieee->l & ((1L<<30)-1L))
		return 0;	/* low bits is not exact */
	exp = (ieee->h>>20) & ((1L<<11)-1L);
	exp -= (1L<<10) - 2L;
	if(exp <= -126 || exp >= 130)
		return 0;	/* large exponents is not exact */
	return 1;
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
