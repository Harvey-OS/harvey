#include	"l.h"
#include	<ar.h>

#ifndef	DEFAULT
#define	DEFAULT	'9'
#endif

char	*noname		= "<none>";
char	symname[] 	= SYMDEF;
char	thechar		= '6';
char	*thestring 	= "960";

/*
 *	-H0 -T0x40004C -D0x10000000	is garbage unix
 *	-H1 -T0xd0 -R4			is unix coff
 *	-H2 -T4128 -R4096		is plan9 format
 *	-H3 -Tx -Rx			is msdos boot
 *	-H4 -T0x8000 -R4		is Intel 960 coff
 */

void
main(int argc, char *argv[])
{
	int i, c;
	char *a;

	Binit(&bso, 1, OWRITE);
	cout = -1;
	listinit();
	memset(debug, 0, sizeof(debug));
	nerrors = 0;
	outfile = "6.out";
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
	case 'o': /* output to (next arg) */
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
		diag("usage: 6l [-options] objects\n");
		errorexit();
	}
	if(!debug['9'] && !debug['U'] && !debug['B'])
		debug[DEFAULT] = 1;
	if(HEADTYPE == -1) {
		if(debug['U'])
			HEADTYPE = 1;
		if(debug['B'])
			HEADTYPE = 2;
		if(debug['9'])
			HEADTYPE = 2;
	}
	switch(HEADTYPE) {
	default:
		diag("unknown -H option");
		errorexit();

	case 0:	/* this is garbage */
		HEADR = 20L+56L;
		if(INITTEXT == -1)
			INITTEXT = 0x40004CL;
		if(INITDAT == -1)
			INITDAT = 0x10000000L;
		if(INITRND == -1)
			INITRND = 0;
		break;
	case 1:	/* is unix coff */
		HEADR = 0xd0L;
		if(INITTEXT == -1)
			INITTEXT = 0xd0;
		if(INITDAT == -1)
			INITDAT = 0x400000;
		if(INITRND == -1)
			INITRND = 0;
		break;
	case 2:	/* plan 9 */
		HEADR = 32L;
		if(INITTEXT == -1)
			INITTEXT = 4096+32;
		if(INITDAT == -1)
			INITDAT = 0;
		if(INITRND == -1)
			INITRND = 4096;
		break;
	case 3:	/* msdos boot */
		HEADR = 0;
		if(INITTEXT == -1)
			INITTEXT = 0x0100;
		if(INITDAT == -1)
			INITDAT = 0;
		if(INITRND == -1)
			INITRND = 4;
		break;
	case 4:	/* is 960 coff */
		HEADR = 0x8CL;
		if(INITTEXT == -1)
			INITTEXT = 0x8000;
		if(INITDAT == -1)
			INITDAT = 0;
		if(INITRND == -1)
			INITRND = 4;
		break;
	}
	if(INITDAT != 0 && INITRND != 0)
		print("warning: -D0x%lux is ignored because of -R0x%lux\n",
			INITDAT, INITRND);
	if(debug['v'])
		Bprint(&bso, "HEADER = -H0x%ld -T0x%lux -D0x%lux -R0x%lux\n",
			HEADTYPE, INITTEXT, INITDAT, INITRND);
	Bflush(&bso);
	for(i=1; optab[i].as; i++)
		if(i != optab[i].as) {
			diag("phase error in optab: %d\n", i);
			errorexit();
		}
	maxop = i;

	for(i=0; i<Ymax; i++)
		ycover[i*Ymax + i] = 1;

	ycover[Yi5*Ymax + Yri5] = 1;
	ycover[Yr*Ymax + Yri5] = 1;

	ycover[Yi5*Ymax + Ynri5] = 1;
	ycover[Yr*Ymax + Ynri5] = 1;
	ycover[Ynone*Ymax + Ynri5] = 1;

	ycover[Yi5*Ymax + Yi32] = 1;

	zprg.link = P;
	zprg.cond = P;
	zprg.back = 2;
	zprg.as = AGOK;
	zprg.type = D_NONE;
	zprg.offset = 0;
	zprg.from.type = D_NONE;
	zprg.from.index = D_NONE;
	zprg.from.scale = 1;
	zprg.to = zprg.from;

	pcstr = "%.6lux ";
	nuxiinit();
	histgen = 0;
	textp = P;
	datap = P;
	edatap = P;
	pc = 0;
	cout = create(outfile, 1, 0775);
	if(cout < 0) {
		diag("cannot create %s\n", outfile);
		errorexit();
	}
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
	} else
		lookup(INITENTRY, 0)->type = SXREF;

	while(*argv)
		objfile(*argv++);
	if(!debug['l'])
		loadlib(0, libraryp);
	firstp = firstp->link;
	if(firstp == P)
		errorexit();
	patch();
	if(debug['p'])
		doprof2();
	dodata();
	follow();
	noops();
	span();
	doinit();
	asmb();
	undef();
	if(debug['v']) {
		Bprint(&bso, "%5.2f cpu time\n", cputime());
		Bprint(&bso, "%ld symbols\n", nsymbol);
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
	long off, esym, cnt, l;
	int f, work;
	Sym *s;
	char magbuf[SARMAG];
	char name[100], pname[150];
	struct ar_hdr arhdr;
	char *e, *start, *stop;

	if(file[0] == '-' && file[1] == 'l') {
		if(debug['9'])
			sprint(name, "/%s/lib/lib", thestring);
		else
			sprint(name, "/usr/%clib/lib", thechar);
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
	if(l != SARMAG || strncmp(magbuf, ARMAG, SARMAG)){
		/* load it as a regular file */
		l = seek(f, 0L, 2);
		seek(f, 0L, 0);
		ldobj(f, l, file);
		close(f);
		return;
	}

	if(debug['v'])
		Bprint(&bso, "%5.2f ldlib: %s\n", cputime(), file);
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
	off = SARMAG + sizeof(struct ar_hdr);

	/*
	 * just bang the whole symbol file into memory
	 */
	seek(f, off, 0);
	cnt = esym - off;
	start = malloc(cnt + 10);
	cnt = read(f, start, cnt);
	if(cnt <= 0){
		close(f);
		return;
	}
	stop = &start[cnt];
	memset(stop, 0, 10);

	work = 1;
	while(work){
		if(debug['v'])
			Bprint(&bso, "%5.2f library pass: %s\n", cputime(), file);
		Bflush(&bso);
		work = 0;
		for(e = start; e < stop; e = strchr(e+5, 0) + 1) {
			s = lookup(e+5, 0);
			if(s->type != SXREF)
				continue;
			sprint(pname, "%s(%s)", file, s->name);
			if(debug['v'])
				Bprint(&bso, "%5.2f library: %s\n", cputime(), pname);
			Bflush(&bso);
			l = e[1] & 0xff;
			l |= (e[2] & 0xff) << 8;
			l |= (e[3] & 0xff) << 16;
			l |= (e[4] & 0xff) << 24;
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
	}
	return;

bad:
	diag("%s: bad or out of date archive\n", file);
out:
	close(f);
}

int
zaddr(uchar *p, Adr *a, Sym *h[])
{
	int c, t, i;
	long l;
	Sym *s;
	Auto *u;

	t = p[0];

	c = 1;
	if(t & T_INDEX) {
		a->index = p[c];
		a->scale = p[c+1];
		c += 2;
	} else {
		a->index = D_NONE;
		a->scale = 0;
	}
	a->offset = 0;
	if(t & T_OFFSET) {
		a->offset = p[c] | (p[c+1]<<8) | (p[c+2]<<16) | (p[c+3]<<24);
		c += 4;
	}
	a->sym = S;
	if(t & T_SYM) {
		a->sym = h[p[c]];
		c++;
	}
	a->type = D_NONE;
	if(t & T_FCONST) {
		a->ieee.l = p[c] | (p[c+1]<<8) | (p[c+2]<<16) | (p[c+3]<<24);
		a->ieee.h = p[c+4] | (p[c+5]<<8) | (p[c+6]<<16) | (p[c+7]<<24);
		c += 8;
		a->type = D_FCONST;
	} else
	if(t & T_SCONST) {
		for(i=0; i<NSNAME; i++)
			a->scon[i] = p[c+i];
		c += NSNAME;
		a->type = D_SCONST;
	}
	if(t & T_TYPE) {
		a->type = p[c];
		c++;
	}
	s = a->sym;
	if(s == S)
		return c;

dosym:
	t = a->type;
	if(t != D_AUTO && t != D_PARAM)
		return c;
	l = a->offset;
	for(u=curauto; u; u=u->link) {
		if(u->sym == s)
		if(u->type == t) {
			if(u->offset > l)
				u->offset = l;
			return c;
		}
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
	u->type = t;
	return c;
}

void
addlib(long line)
{
	char name[MAXHIST*NAMELEN], comp[4*NAMELEN], *p;
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
			sprint(name, "/%s/lib", thestring);
		else
			sprint(name, "/usr/%clib", thechar);
		i = 0;
	}

	for(; i<histfrogp; i++) {
		snprint(comp, 2*NAMELEN, histfrog[i]->name+1);
		for(;;) {
			p = strstr(comp, "$O");
			if(p == 0)
				break;
			memmove(p+1, p+2, strlen(p+2)+1);
			p[0] = thechar;
		}
		for(;;) {
			p = strstr(comp, "$M");
			if(p == 0)
				break;
			memmove(p+strlen(thestring), p+2, strlen(p+2)+1);
			memmove(p, thestring, strlen(thestring));
			if(strlen(comp) > NAMELEN) {
				diag("library component too long");
				return;
			}
		}
		if(strlen(comp) > NAMELEN || strlen(name) + strlen(comp) + 3 >= sizeof(name)) {
			diag("library component too long");
			return;
		}
		strcat(name, "/");
		strcat(name, comp);
	}
	for(i=0; i<libraryp; i++)
		if(strcmp(name, library[i]) == 0)
			return;

	p = malloc(strlen(name) + 1);
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

	u = malloc(sizeof(Auto));
	s = malloc(sizeof(Sym));
	s->name = malloc(2*(histfrogp+1) + 1);

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
collapsefrog(Sym *s)
{
	int i;

	/*
	 * bad encoding of path components only allows
	 * MAXHIST components. if there is an overflow,
	 * first try to collapse xxx/..
	 */
	for(i=1; i<histfrogp; i++)
		if(strcmp(histfrog[i]->name+1, "..") == 0) {
			memmove(histfrog+i-1, histfrog+i+1,
				(histfrogp-i-1)*sizeof(histfrog[0]));
			histfrogp--;
			goto out;
		}

	/*
	 * next try to collapse .
	 */
	for(i=0; i<histfrogp; i++)
		if(strcmp(histfrog[i]->name+1, ".") == 0) {
			memmove(histfrog+i, histfrog+i+1,
				(histfrogp-i-1)*sizeof(histfrog[0]));
			goto out;
		}

	/*
	 * last chance, just truncate from front
	 */
	memmove(histfrog+0, histfrog+1,
		(histfrogp-1)*sizeof(histfrog[0]));

out:
	histfrog[histfrogp-1] = s;
}

uchar*
readsome(int f, uchar *buf, uchar *good, uchar *stop, int max)
{
	int n;

	n = stop - good;
	memmove(buf, good, stop - good);
	stop = buf + n;
	n = MAXIO - n;
	if(n > max)
		n = max;
	n = read(f, stop, n);
	if(n <= 0)
		return 0;
	return stop + n;
}

void
ldobj(int f, long c, char *pn)
{
	Prog *p;
	Sym *h[NSYM], *s;
	int v, o, r;
	long ipc;
	uchar *bloc, *bsize, *stop;

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
		bsize = readsome(f, buf.xbuf, bloc, bsize, c);
		if(bsize == 0)
			goto eof;
		bloc = buf.xbuf;
		goto loop;
	}
	o = bloc[0];
	if(o <= 0 || o >= maxop) {
		if(o < 0)
			goto eof;
		diag("%s: opcode out of range %d\n", pn, o);
		print("	probably not a .8 file\n");
		errorexit();
	}

	if(o == ANAME) {
		stop = memchr(&bloc[3], 0, bsize-&bloc[3]);
		if(stop == 0){
			bsize = readsome(f, buf.xbuf, bloc, bsize, c);
			if(bsize == 0)
				goto eof;
			bloc = buf.xbuf;
			stop = memchr(&bloc[3], 0, bsize-&bloc[3]);
			if(stop == 0){
				fprint(2, "%s: name too long\n", pn);
				errorexit();
			}
		}
		v = bloc[1];	/* type */
		o = bloc[2];	/* sym */
		bloc += 3;
		c -= 3;

		r = 0;
		if(v == D_STATIC)
			r = version;
		s = lookup((char*)bloc, r);
		c -= &stop[1] - bloc;
		bloc = stop + 1;

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

	while(nhunk < sizeof(Prog))
		gethunk();
	p = (Prog*)hunk;
	nhunk -= sizeof(Prog);
	hunk += sizeof(Prog);

	p->as = o;
	p->line = bloc[1] | (bloc[2] << 8) | (bloc[3] << 16) | (bloc[4] << 24);
	p->back = 2;
	r = bloc[5];
	p->type = D_NONE;
	p->offset = 0;
	if(r > 0) {
		if(r >= 33) {
			p->type = D_CONST;
			p->offset = r - 33;
		} else
			p->type = D_R0 + (r - 1);
	}
	r = zaddr(bloc+6, &p->from, h) + 6;
	r += zaddr(bloc+r, &p->to, h);
	bloc += r;
	c -= r;

	if(debug['W'])
		print("%P\n", p);

	switch(p->as) {
	case AHISTORY:
		if(p->to.offset == -1) {
			addlib(p->line);
			histfrogp = 0;
			goto loop;
		}
		addhist(p->line, D_FILE);
		if(p->to.offset)
			addhist(p->to.offset, D_FILE1);
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
		if(edatap == P)
			datap = p;
		else
			edatap->link = p;
		edatap = p;
		p->link = P;
		goto loop;

	case AGOK:
		diag("%s: GOK opcode in %s\n", pn, TNAME);
		pc++;
		goto loop;

	case ATEXT:
		if(curtext != P) {
			histtoauto();
			curtext->to.autom = curauto;
			curauto = 0;
		}
		curtext = p;
		lastp->link = p;
		lastp = p;
		p->pc = pc;
		s = p->from.sym;
		if(s == S) {
			diag("%s: no TEXT symbol: %P\n", pn, p);
			errorexit();
		}
		if(s->type != 0 && s->type != SXREF)
			diag("%s: redefinition: %s\n", pn, s->name);
		s->type = STEXT;
		s->value = p->pc;
		pc++;
		p->cond = P;
		if(textp == P) {
			textp = p;
			etextp = p;
			goto loop;
		}
		etextp->cond = p;
		etextp = p;
		goto loop;

	case AADDO:
		if(p->from.type == D_CONST)
		if(p->from.offset < 0) {
			p->as = ASUBO;
			p->from.offset = -p->from.offset;
		}
		goto casdef;

	case AADDI:
		if(p->from.type == D_CONST)
		if(p->from.offset < 0) {
			p->as = ASUBI;
			p->from.offset = -p->from.offset;
		}
		goto casdef;

	case ASUBO:
		if(p->from.type == D_CONST)
		if(p->from.offset < 0) {
			p->as = AADDO;
			p->from.offset = -p->from.offset;
		}
		goto casdef;

	case ASUBI:
		if(p->from.type == D_CONST)
		if(p->from.offset < 0) {
			p->as = AADDI;
			p->from.offset = -p->from.offset;
		}
		goto casdef;

	casdef:
	default:
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
	int c, l;

	h = v;
	for(p=symb; c = *p; p++)
		h = h+h+h + c;
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

	s->name = malloc(l);
	memmove(s->name, symb, l);

	s->link = hash[h];
	s->type = 0;
	s->version = v;
	s->value = 0;
	hash[h] = s;
	nsymbol++;
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

Prog*
copyp(Prog *q)
{
	Prog *p;

	p = prg();
	*p = *q;
	return p;
}

Prog*
appendp(Prog *q)
{
	Prog *p;

	p = prg();
	p->link = q->link;
	q->link = p;
	p->line = q->line;
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
{}

void
doprof2(void)
{}

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
		for(i=0; i<4; i++)
			Bprint(&bso, "%d", fnuxi4[i]);
		Bprint(&bso, " ");
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
find2(long l, int c)
{
	short *p;
	int i;

	p = (short*)&l;
	for(i=0; i<4; i+=2) {
		if(((*p >> 8) & 0xff) == c)
			return i;
		if((*p++ & 0xff) == c)
			return i+1;
	}
	return 0;
}

long
ieeedtof(Ieee *e)
{
	int exp;
	long v;

	if(e->h == 0)
		return 0;
	exp = (e->h>>20) & ((1L<<11)-1L);
	exp -= (1L<<10) - 2L;
	v = (e->h & 0xfffffL) << 3;
	v |= (e->l >> 29) & 0x7L;
	if((e->l >> 28) & 1) {
		v++;
		if(v & 0x800000L) {
			v = (v & 0x7fffffL) >> 1;
			exp++;
		}
	}
	if(exp <= -126 || exp >= 130)
		diag("double fp to single fp overflow\n");
	v |= ((exp + 126) & 0xffL) << 23;
	v |= e->h & 0x80000000L;
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

	while(n & 7)
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
