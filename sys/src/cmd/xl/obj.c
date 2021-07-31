#include	"l.h"
#include	<ar.h>

#ifndef	DEFAULT
#define	DEFAULT	'9'
#endif

char	*noname		= "<none>";
char	symname[]	= SYMDEF;
char	thechar		= 'x';
char	*thestring 	= "3210";

/*
 *	-H0				is coff format
 * 	-H1				is text followed by data, no header
 *	-H2 -T4128 -R4096		is plan9 format
 */
void
main(int argc, char *argv[])
{
	int c;
	char *a;

	Binit(&bso, 1, OWRITE);
	cout = -1;
	listinit();
	outfile = "x.out";
	nerrors = 0;
	curtext = P;
	header = -1;
	textstart = -1;
	datstart = -1;
	datrnd = -1;
	entry = 0;
	maxram = 0;		/* maximum amount of internal ram to use */
	ramoffset = 0x50030000;

	/*
	 * options:
	 * 'e':		big endian
	 * 'p':		profile
	 * '1':		alternate prof style
	 * 'l':		don't load default libraries
	 * 's':		don't output symbol tables
	 * 'v':		verbose
	 * '9':		plan 9 header and libraries
	 * 'U':		unix header and libraries
	 * 'B':		basic header and unix libraries
	 *
	 * debugging:
	 * 'G':		pc-sp
	 * 'L':		pc-line
	 * 'W':		print intermediate files
	 * 'X':		scheduling
	 * 'a':		text asm
	 * 'd':		data asm
	 * 'n':		symbol table
	 */
	ARGBEGIN {
	default:
		c = ARGC();
		if(c >= 0 && c < sizeof(debug))
			debug[c]++;
		break;
	case 'o':
		outfile = ARGF();
		break;
	case 'E':	/* name of entry routine */
		a = ARGF();
		if(a)
			entry = a;
		break;
	case 'T':
		a = ARGF();
		if(a)
			textstart = atolwhex(a);
		break;
	case 'c':
		a = ARGF();
		if(a){
			lookup(a, 0)->isram = 1;
			lookup(a, 0)->type = SXREF;
		}
		break;
	case 'm':
		a = ARGF();
		if(a)
			maxram = atolwhex(a);
		break;
	case 'D':
		a = ARGF();
		if(a)
			datstart = atolwhex(a);
		break;
	case 'R':
		a = ARGF();
		if(a)
			datrnd = atolwhex(a);
		break;
	case 'H':
		a = ARGF();
		if(a)
			header = atolwhex(a);
		break;
	} ARGEND
	USED(argc);

	ramstart = ramoffset + 0xe000;
	maxram += ramstart;

	if(*argv == 0) {
		diag("usage: %cl [-options] objects\n", thechar);
		errorexit();
	}
	if(!debug['9'] && !debug['U'] && !debug['B'])
		debug[DEFAULT] = 1;
	if(header == -1) {
		if(debug['U'])
			header = 0;
		if(debug['B'])
			header = 1;
		if(debug['9'])
			header = 2;
	}
	switch(header) {
	default:
		diag("unknown -H option");
		errorexit();

	case 0:	/* coff */
		headlen = 20+1+3*40;
		if(textstart == -1)
			textstart = 0L;
		if(datstart == -1)
			datstart = 0;
		if(datrnd == -1)
			datrnd = 4096L;
		break;
	case 1:	/* garbage */
		headlen = 0;
		if(textstart == -1)
			textstart = 0L;
		if(datstart == -1)
			datstart = 0;
		if(datrnd == -1)
			datrnd = 4;
		break;
	case 2:	/* plan 9 */
		headlen = 32L;
		if(textstart == -1)
			textstart = 4128;
		if(datstart == -1)
			datstart = 0;
		if(datrnd == -1)
			datrnd = 4096;
		break;
	}
	if(datstart != 0 && datrnd != 0)
		print("warning: -D0x%lux is ignored because of -R0x%lux\n",
			datstart, datrnd);
	if(datstart == 0 && datrnd == 0)
		datrnd = 4;
	if(debug['v'])
		Bprint(&bso, "HEADER = -H0x%ld -T0x%lux -D0x%lux -R0x%lux\n",
			header, textstart, datstart, datrnd);
	Bflush(&bso);
	zprg.as = AGOK;
	zprg.reg = NREG;
	zprg.stkoff = -1;
	zprg.from.name = D_NONE;
	zprg.from.type = D_NONE;
	zprg.from.reg = NREG;
	zprg.from1 = zprg.from;
	zprg.from2 = zprg.from;
	zprg.to = zprg.from;
	buildop();
	histgen = 0;
	textp = P;
	datap = P;
	pc = 0;
	cout = create(outfile, OWRITE, 0777);
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

	if(entry == 0) {
		entry = "_main";
		if(debug['p'])
			entry = "_mainp";
	}
	if(!debug['l'])
		lookup(entry, 0)->type = SXREF;

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
	expand();
	span(1);
	noops();
	span(0);
	adjdata();
	asmb();
	undef();
out:
	if(debug['v']) {
		Bprint(&bso, "%5.2f cpu time\n", cputime());
		Bprint(&bso, "%ld memory used\n", tothunk);
		Bprint(&bso, "%d sizeof adr\n", sizeof(Adr));
		Bprint(&bso, "%d sizeof prog\n", sizeof(Prog));
	}
	errorexit();
}

/*
 * load the automatic libraries
 */
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

	Bflush(&bso);
	if(nerrors) {
		if(cout >= 0)
			remove(outfile);
		exits("error");
	}
	exits(0);
}

/*
 * determine if file is a library
 * load a normal file or the sources  in the library
 * which define undefined symbols
*/
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
	int i, c;
	long l;
	Sym *s;
	Auto *u;

	c = p[2];
	if(c < 0 || c > NSYM){
		print("sym out of range: %d\n", c);
		p[0] = AEND+1;
		return 0;
	}
	a->type = p[0];
	a->reg = p[1];
	a->sym = h[c];
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
		return 0;	/* force real diagnostic */

	case D_NONE:
	case D_REG:
	case D_INDREG:
	case D_INC:
	case D_DEC:
	case D_FREG:
	case D_CREG:
		break;
	case D_INCREG:
		a->increg = p[4];
		c++;
		break;
	case D_NAME:
		a->offset = p[4] | (p[5]<<8) | (p[6]<<16) | (p[7]<<24);
		c += 4;
		break;
	case D_BRANCH:
	case D_OREG:
	case D_CONST:
		a->offset = p[4] | (p[5]<<8) | (p[6]<<16) | (p[7]<<24);
		c += 4;
		break;
	case D_SCONST:
		memmove(a->sval, p+4, NSNAME);
		c += NSNAME;
		break;
	case D_FCONST:
		a->dsp = p[4] | (p[5]<<8) | (p[6]<<16) | (p[7]<<24);
		c += 4;
		break;
	case D_AFCONST:
		a->dsp = p[4] | (p[5]<<8) | (p[6]<<16) | (p[7]<<24);
		c += 4;
		installfconst(a);
		a->type = D_CONST;
		a->name = D_EXTERN;
		a->offset = 0;
		break;
	}
	s = a->sym;
	if(s == S)
		goto out;
	i = a->name;
	if(i != D_AUTO && i != D_PARAM)
		goto out;

	l = a->offset;
	for(u=curauto; u; u=u->link)
		if(u->sym == s)
		if(u->type == i) {
			if(u->offset > l)
				u->offset = l;
			goto out;
		}

	u = malloc(sizeof(Auto));

	u->link = curauto;
	curauto = u;
	u->sym = s;
	u->offset = l;
	u->type = i;
out:
	return c;
}

/*
 * add a library to the list of automatic libraries
 */
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
	n = IOMAX - n;
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
	Prog *p, *t;
	Sym *h[NSYM], *s;
	int v, o, i, r;
	long ipc;
	uchar *bloc, *bsize, *stop;

	bsize = buf.xbuf;
	bloc = buf.xbuf;

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
		bsize = readsome(f, buf.xbuf, bloc, bsize, c);
		if(bsize == 0)
			goto eof;
		bloc = buf.xbuf;
		goto loop;
	}
	o = bloc[0];		/* as */
	if(o <= 0 || o > AEND) {
		diag("%s: opcode out of range %d\n", pn, o);
		print("	probably not a .%c file\n", thechar);
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

	if(nhunk < sizeof(Prog))
		gethunk();
	p = (Prog*)hunk;
	nhunk -= sizeof(Prog);
	hunk += sizeof(Prog);

	p->as = o;
	p->stkoff = -1;
	i = bloc[1];
	if(i & 0x80)
		p->nosched = 1;
	p->reg = i & 0x7f;
	i = bloc[2];
	p->cc = i & 0x3f;
	i >>= 6;
	p->isf = i;
	p->line = bloc[3] | (bloc[4]<<8) | (bloc[5]<<16) | (bloc[6]<<24);
	r = 7;
	r += zaddr(bloc+r, &p->from, h);
	if(i > 1)
		r += zaddr(bloc+r, &p->from1, h);
	else
		p->from1 = zprg.from1;
	if(i > 2)
		r += zaddr(bloc+r, &p->from2, h);
	else
		p->from2 = zprg.from2;
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
		s = p->from.sym;
		if(s == S) {
			diag("TEXT must have a name\n%P\n", p);
			errorexit();
		}
		if(s->type != 0 && s->type != SXREF)
			diag("redefinition: %s\n%P\n", s->name, p);
		s->type = STEXT;
		s->value = pc;
		if(s->isram){
			s->ram = ram;
			ram = s;
		}
		if(p->to.offset < -4){
			diag("bad argument count for %s: %d\n", s->name, p->to.offset);
			p->to.offset = 0;
		}
		p->to.offset = (p->to.offset+3L) & ~3L;
		if(textp != P && (!entry || strcmp(entry, s->name) != 0)){
			for(t = textp; t->cond != P; t = t->cond)
				;
			t->cond = p;
		}else{
			p->cond = textp;
			textp = p;
		}
		lastp->link = p;
		lastp = p;
		p->pc = pc;
		pc++;
		break;

	case AFMOVF:
	case AFMOVFN:
	case AFMOVWF:
	case AFMOVFW:
	case AFMOVHF:
	case AFMOVFH:
		/*
		 * just enter the data now;
		 * change the instruction later
		 */
		if(p->from.type == D_FCONST)
			installfconst(&p->from);
		goto casedef;

	case AMUL:
		s = lookup("_mul", 0);
		if(s->type == 0)
			s->type = SXREF;
		goto casedef;
	case ADIV:
	case AMOD:
	case ADIVL:
	case AMODL:
		s = lookup("_div", 0);
		if(s->type == 0)
			s->type = SXREF;
		goto casedef;
	case AFDIV:
		s = lookup("_fdiv", 0);
		if(s->type == 0)
			s->type = SXREF;
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
	s->ram = S;
	s->isram = 0;
	s->size = 0;
	s->version = v;
	s->value = 0;
	hash[h] = s;
	return s;
}

/*
 * allocate a new prog
 */
Prog*
prg(void)
{
	Prog *p;
	int n;

	n = (sizeof(Prog) + 3) & ~3;
	while(nhunk < n)
		gethunk();

	p = (Prog*)hunk;
	nhunk -= n;
	hunk += n;

	*p = zprg;
	return p;
}

Prog*
appendp(Prog *p)
{
	Prog *q;

	q = prg();
	q->link = p->link;
	p->link = q;
	q->line = p->line;
	q->stkoff = p->stkoff;
	return q;
}

void
gethunk(void)
{
	char *h;

	h = sbrk((int)NHUNK);
	if(h == (char *)-1) {
		diag("out of memory\n");
		errorexit();
	}

	hunk = h;
	nhunk = NHUNK;
	tothunk += NHUNK;
}

void
installfconst(Adr *a)
{
	Sym *s;
	Prog *t;

	/* size sb 9 max */
	sprint(literal, "$%lux", a->dsp);
	s = lookup(literal, 0);
	a->sym = s;
	if(s->type)
		return;
	s->type = SBSS;
	s->value = 4;
	t = prg();
	t->as = ADATA;
	t->from.type = D_NAME;
	t->from.sym = s;
	t->from.name = D_EXTERN;
	t->reg = 4;
	t->to = *a;
	t->to.type = D_FCONST;
	t->link = datap;
	datap = t;
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
			q->from.type = D_NAME;
			q->from.name = D_EXTERN;
			q->from.offset = n*4;
			q->from.sym = s;
			q->reg = 4;
			q->to = p->from;
			q->to.type = D_CONST;

			p = appendp(p);
			p->as = AMOVW;
			p->from.type = D_NAME;
			p->from.name = D_EXTERN;
			p->from.sym = s;
			p->from.offset = n*4 + 4;
			p->to.type = D_REG;
			p->to.reg = REGTMP;

			p = appendp(p);
			p->as = AADD;
			p->from.type = D_CONST;
			p->from.offset = 1;
			p->to.type = D_REG;
			p->to.reg = REGTMP;

			p = appendp(p);
			p->as = AMOVW;
			p->from.type = D_REG;
			p->from.reg = REGTMP;
			p->to.type = D_NAME;
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
	q->from.type = D_NAME;
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
				p->reg = 1;
				ps2 = p;
			}
			if(p->from.sym == s4) {
				p->reg = 1;
				ps4 = p;
			}
		}
	}
	for(p = firstp; p != P; p = p->link) {
		if(p->as == ATEXT) {
			curtext = p;

			if(p->reg != NREG) {	/* dont profile */
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
			p = appendp(p);
			p->as = ACALL;
			p->to.type = D_BRANCH;
			p->cond = ps2;
			p->to.sym = s2;

			continue;
		}
		if(p->as == ARETURN) {

			/*
			 * RETURN
			 */
			q = appendp(p);
			q->as = ARETURN;
			q->from = p->from;
			q->to = p->to;

			/*
			 * JAL	profout
			 */
			p->as = ACALL;
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
	}
	if(debug['e'])
		for(i=0; i<4; i++) {
			c = find1(0x01020304L, i+1);
			if(i >= 2)
				inuxi2[i-2] = c;
			if(i >= 3)
				inuxi1[i-3] = c;
			inuxi4[i] = c;
			fnuxi4[i] = c;
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

/*
 * convert a dsp floating value to native double
 * format: N = (-2^sign + 0.mant) * 2^(exp-128)
 * so abs(mant) >= 1 and  abs(mant) <= 2
 */
double
dsptod(ulong v)
{
	long mant, exp;
	int neg;

	exp = v & Dspexp;
	if(!exp)
		return 0.0;
	exp -= Dspbias;
	neg = v >> 31;
	mant = (v >> 8) & Dspmask;
	/*
	 * add hidden bits and sign
	 */
	if(neg)
		mant = (-2<<Dspbits) + mant;	/* -2. + 0.mant */
	else
		mant = (1<<Dspbits) + mant;	/* 1. + 0.mant */
	return ldexp(mant, exp - Dspbits);
}

/*
 * fake malloc
 */
void*
malloc(long n)
{
	void *v;

	n = (n + 7) & ~7;
	while(nhunk < n)
		gethunk();

	v = (void*)hunk;
	nhunk -= n;
	hunk += n;

	memset(v, 0, n);
	return v;
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
