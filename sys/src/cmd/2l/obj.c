#include	"l.h"
#include	<ar.h>

#ifndef	DEFAULT
#define	DEFAULT	'9'
#endif

char	*noname		= "<none>";
char	symname[]	= SYMDEF;

/*
 *	-H0 -T0x40004C -D0x10000000	is garbage unix
 *	-H1 -T0x80020000 -R4		is garbage format
 *	-H2 -T8224 -R8192		is plan9 format
 *	-H3 -Tx -Rx			is next boot
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
	outfile = "2.out";
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
		diag("usage: 2l [-options] objects\n");
		errorexit();
	}
	if(!debug['9'] && !debug['U'] && !debug['B'])
		debug[DEFAULT] = 1;
	if(HEADTYPE == -1) {
		if(debug['U'])
			HEADTYPE = 2;
		if(debug['B'])
			HEADTYPE = 2;
		if(debug['9'])
			HEADTYPE = 2;
	}
	if(INITDAT != -1 && INITRND == -1)
		INITRND = 0;
	switch(HEADTYPE) {
	default:
		diag("unknown -H option %d", HEADTYPE);
		errorexit();

	case 0:	/* this is garbage */
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
	case 1:	/* plan9 boot data goes into text */
		HEADR = 32L;
		if(INITTEXT == -1)
			INITTEXT = 8224;
		if(INITDAT == -1)
			INITDAT = 0;
		if(INITDAT != 0 && INITRND == -1)
			INITRND = 0;
		if(INITRND == -1)
			INITRND = 8192;
		break;
	case 2:	/* plan 9 */
		HEADR = 32L;
		if(INITTEXT == -1)
			INITTEXT = 8224;
		if(INITDAT == -1)
			INITDAT = 0;
		if(INITDAT != 0 && INITRND == -1)
			INITRND = 0;
		if(INITRND == -1)
			INITRND = 8192;
		break;
	case 3:	/* next boot */
	case 4:
		HEADR = 28+124+192+24;
		if(INITTEXT == -1)
			INITTEXT = 0x04002000;
		if(INITDAT == -1)
			INITDAT = 0;
		if(INITDAT != 0 && INITRND == -1)
			INITRND = 0;
		if(INITRND == -1)
			INITRND = 8192L;
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

	zprg.link = P;
	zprg.cond = P;
	zprg.back = 2;
	zprg.as = AGOK;
	zprg.from.type = D_NONE;
	zprg.from.index = D_NONE;
	zprg.to = zprg.from;

	memset(special, 0, sizeof(special));
	special[D_CCR] = 1;
	special[D_SR] = 1;
	special[D_SFC] = 1;
	special[D_CACR] = 1;
	special[D_USP] = 1;
	special[D_VBR] = 1;
	special[D_CAAR] = 1;
	special[D_MSP] = 1;
	special[D_ISP] = 1;
	special[D_DFC] = 1;
	special[D_FPCR] = 1;
	special[D_FPSR] = 1;
	special[D_FPIAR] = 1;
 	special[D_TC] = 1;
	special[D_ITT0] = 1;
	special[D_ITT1] = 1;
	special[D_DTT0] = 1;
	special[D_DTT1] = 1;
	special[D_MMUSR] = 1;
	special[D_URP] = 1;
	special[D_SRP] = 1;
	memset(simple, 0177, sizeof(simple));
	for(i=0; i<8; i++) {
		simple[D_R0+i] = i;
		simple[D_F0+i] = i+0100;
		simple[D_A0+i] = i+010;
		simple[D_A0+I_INDIR+i] = i+020;
		simple[D_A0+I_INDINC+i] = i+030;
		simple[D_A0+I_INDDEC+i] = i+040;
	}
	nuxiinit();
	histgen = 0;
	textp = P;
	datap = P;
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
	patch();
	if(debug['p'])
		if(debug['1'])
			doprof1();
		else
			doprof2();
	follow();
	dodata();
	dostkoff();
	span();
	asmb();
	undef();
	if(debug['v']) {
		Bprint(&bso, "%5.2f cpu time\n", cputime());
		Bprint(&bso, "%ld+%ld = %ld data statements\n",
			ndata, ncase, ndata+ncase);
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
	long off, esym, cnt, i, l;
	int f, work;
	Sym *s;
	char magbuf[SARMAG];
	char name[100], pname[150];
	struct ar_hdr arhdr;
	Rlent *e;

	if(file[0] == '-' && file[1] == 'l') {
		if(debug['9'])
			strcpy(name, "/68020/lib/lib");
		else
			strcpy(name, "/usr/2lib/lib");
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
	int c, t, i;
	long l;
	Sym *s;
	Auto *u;

	t = p[0];

	/*
	 * first try the high-time formats
	 */
	if(t == 0) {
		a->index = D_NONE;
		a->type = p[1];
		return 2;
	}
	if(t == T_OFFSET) {
		a->index = D_NONE;
		a->offset = p[1] | (p[2]<<8) | (p[3]<<16) | (p[4]<<24);
		a->type = p[5];
		return 6;
	}
	if(t == (T_OFFSET|T_SYM)) {
		a->index = D_NONE;
		a->offset = p[1] | (p[2]<<8) | (p[3]<<16) | (p[4]<<24);
		s = h[p[5]];
		a->sym = s;
		a->type = p[6];
		c = 7;
		goto dosym;
	}
	if(t == T_SYM) {
		a->index = D_NONE;
		s = h[p[1]];
		a->sym = s;
		a->type = p[2];
		c = 3;
		goto dosym;
	}
	if(t == (T_INDEX|T_OFFSET|T_SYM)) {
		a->index = p[1] | (p[2]<<8);
		a->scale = p[3];
		a->displace = p[4] | (p[5]<<8) | (p[6]<<16) | (p[7]<<24);
		a->offset = p[8] | (p[9]<<8) | (p[10]<<16) | (p[11]<<24);
		s = h[p[12]];
		a->sym = s;
		a->type = p[13];
		c = 14;
		goto dosym;
	}

	/*
	 * now do it the hard way
	 */
	c = 1;
	a->index = D_NONE;
	if(t & T_FIELD) {
		a->field = p[c] | (p[c+1]<<8);
		c += 2;
	}
	if(t & T_INDEX) {
		a->index = p[c] | (p[c+1]<<8);
		a->scale = p[c+2];
		a->displace = p[c+3] | (p[c+4]<<8) | (p[c+5]<<16) | (p[c+6]<<24);
		c += 7;
	}
	if(t & T_OFFSET) {
		a->offset = p[c] | (p[c+1]<<8) | (p[c+2]<<16) | (p[c+3]<<24);
		c += 4;
	}
	if(t & T_SYM) {
		a->sym = h[p[c]];
		c += 1;
	}
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
	} else
	if(t & T_TYPE) {
		a->type = p[c] | (p[c+1]<<8);
		c += 2;
	} else {
		a->type = p[c];
		c++;
	}
	s = a->sym;
	if(s == S)
		return c;

dosym:
	t = a->type & D_MASK;
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
			strcpy(name, "/68020/lib");
		else
			strcpy(name, "/usr/2lib");
		i = 0;
	}

	for(; i<histfrogp; i++) {
		sprint(comp, histfrog[i]->name+1);
		for(;;) {
			p = strstr(comp, "$O");
			if(p == 0)
				break;
			memmove(p+1, p+2, strlen(p+2)+1);
			p[0] = '2';
		}
		for(;;) {
			p = strstr(comp, "$M");
			if(p == 0)
				break;
			memmove(p+5, p+2, strlen(p+2)+1);
			p[0] = '6';
			p[1] = '8';
			p[2] = '0';
			p[3] = '2';
			p[4] = '0';
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
	Prog *p;
	Sym *h[NSYM], *s;
	char name[NNAME];
	int i, v, o, r;
	long ipc, lv;
	double dv;
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
	o = bloc[0] | (bloc[1] << 8);
	if(o <= 0 || o >= maxop) {
		if(o < 0)
			goto eof;
		diag("%s: opcode out of range %d\n", pn, o);
		print("	probably not a .2 file\n");
		errorexit();
	}

	if(o == ANAME) {
		v = bloc[2];	/* type */
		o = bloc[3];	/* sym */
		bloc += 4;
		c -= 4;
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

	while(nhunk < sizeof(Prog))
		gethunk();
	p = (Prog*)hunk;
	nhunk -= sizeof(Prog);
	hunk += sizeof(Prog);

	p->as = o;
	p->line = bloc[2] | (bloc[3] << 8) | (bloc[4] << 16) | (bloc[5] << 24);
	p->back = 2;
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
		if(p->to.offset)
			addhist(p->to.offset, D_DFC);
		addhist(p->line, D_FILE);
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

	case ABCASE:
		ncase++;
		goto casdef;

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
			histtoauto();
			curtext->to.autom = curauto;
			curauto = 0;
		}
		curtext = p;
		lastp->link = p;
		lastp = p;
		p->pc = pc;
		s = p->from.sym;
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

	case AJSR:
		p->as = ABSR;

	case ABSR:
		if(p->to.index != D_NONE)
			p->as = AJSR;
		if(p->to.type != D_EXTERN && p->to.type != D_STATIC)
			p->as = AJSR;
		goto casdef;

	case AMOVL:
	case AMOVB:
	case AMOVW:
		if(p->from.type != D_CONST)
			goto casdef;
		lv = p->from.offset;
		if(lv >= -128 && lv < 128)
		if(p->to.type >= D_R0 && p->to.type < D_R0+8)
		if(p->to.index == D_NONE) {
			p->from.type = D_QUICK;
			goto casdef;
		}

		if(lv >= -0x7fff && lv <= 0x7fff)
		if(p->to.type >= D_A0 && p->to.type < D_A0+8)
		if(p->to.index == D_NONE)
		if(p->as == AMOVL)
			p->as = AMOVW;
		goto casdef;

	case AADDB:
	case AADDL:
	case AADDW:
		if(p->from.type != D_CONST)
			goto casdef;
		lv = p->from.offset;
		if(lv < 0) {
			lv = -lv;
			p->from.offset = lv;
			if(p->as == AADDB)
				p->as = ASUBB;
			else
			if(p->as == AADDW)
				p->as = ASUBW;
			else
			if(p->as == AADDL)
				p->as = ASUBL;
		}
		if(lv > 0)
		if(lv <= 8)
			p->from.type = D_QUICK;
		goto casdef;

	case ASUBB:
	case ASUBL:
	case ASUBW:
		if(p->from.type != D_CONST)
			goto casdef;
		lv = p->from.offset;
		if(lv < 0) {
			lv = -lv;
			p->from.offset = lv;
			if(p->as == ASUBB)
				p->as = AADDB;
			else
			if(p->as == ASUBW)
				p->as = AADDW;
			else
			if(p->as == ASUBL)
				p->as = AADDL;
		}
		if(lv > 0)
		if(lv <= 8)
			p->from.type = D_QUICK;
		goto casdef;

	case AROTRB:
	case AROTRL:
	case AROTRW:
	case AROTLB:
	case AROTLL:
	case AROTLW:

	case AASLB:
	case AASLL:
	case AASLW:
	case AASRB:
	case AASRL:
	case AASRW:
	case ALSLB:
	case ALSLL:
	case ALSLW:
	case ALSRB:
	case ALSRL:
	case ALSRW:
		if(p->from.type == D_CONST)
		if(p->from.offset > 0)
		if(p->from.offset <= 8)
			p->from.type = D_QUICK;
		goto casdef;

	case ATSTL:
		if(p->to.type >= D_A0 && p->to.type < D_A0+8) {
			p->as = ACMPW;
			p->from = p->to;
			p->to.type = D_CONST;
			p->to.offset = 0;
		}
		goto casdef;

	case ACMPL:
		if(p->to.type != D_CONST)
			goto casdef;
		lv = p->to.offset;
		if(lv >= -0x7fff && lv <= 0x7fff)
		if(p->from.type >= D_A0 && p->from.type < D_A0+8)
		if(p->from.index == D_NONE)
			p->as = ACMPW;
		goto casdef;

	case ACLRL:
		if(p->to.type >= D_A0 && p->to.type < D_A0+8) {
			p->as = AMOVW;
			p->from.type = D_CONST;
			p->from.offset = 0;
		}
		goto casdef;

	casdef:
	default:
		if(p->from.type == D_FCONST)
		if(optab[p->as].fas != AXXX) {
			dv = ieeedtod(&p->from.ieee);
			if(dv >= -(1L<<30) && dv <= (1L<<30)) {
				lv = dv;
				if(lv == dv) {
					p->as = optab[p->as].fas;
					p->from.type = D_CONST;
					p->from.offset = lv;
					p->from.displace = 0;
				}
			}
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
	diag("%s: truncated object file in %s\n", pn, TNAME);
}

Sym*
lookup(char *symb, int v)
{
	Sym *s;
	char *p;
	int i, c;
	long h;

	h = v;
	p = symb;
	for(i=0; i<NNAME; i++) {
		c = *p++;
		if(c == 0)
			break;
		h = h+h+h + c;
	}
	if(h < 0)
		h = ~h;
	h %= NHASH;
	for(s = hash[h]; s != S; s = s->link) {
		if(s->version == v)
		if(strncmp(s->name, symb, NNAME) == 0)
			return s;
	}

	while(nhunk < sizeof(Sym))
		gethunk();
	s = (Sym*)hunk;
	nhunk -= sizeof(Sym);
	hunk += sizeof(Sym);

	if(strlen(symb) >= sizeof(s->name))
		diag("long name in lookup %s\n", symb);
	strncpy(s->name, symb, NNAME);
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
			q->as = AADDL;
			q->line = p->line;
			q->pc = p->pc;
			q->link = p->link;
			p->link = q;
			q->from.type = D_CONST;
			q->from.offset = 1;
			q->to.type = D_EXTERN;
			q->to.sym = s;
			q->to.offset = n*4 + 4;

			q = prg();
			q->as = ADATA;
			q->line = p->line;
			q->link = datap;
			datap = q;
			q->from.type = D_EXTERN;
			q->from.sym = s;
			q->from.offset = n*4;
			q->from.displace = 4;
			q->to.type = D_EXTERN;
			q->to.sym = p->from.sym;
			n += 2;
			continue;
		}
	}
	q = prg();
	q->line = 0;
	q->as = ADATA;
	q->link = datap;
	datap = q;
	q->from.type = D_EXTERN;
	q->from.sym = s;
	q->from.displace = 4;
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
				p->from.displace = 1;
			}
			if(p->from.sym == s4) {
				ps4 = p;
				p->from.displace = 1;
			}
		}
	}
	for(p = firstp; p != P; p = p->link) {
		if(p->as == ATEXT) {
			if(p->from.displace != 0) {
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

			q = prg();
			q->line = p->line;
			q->pc = p->pc;
			q->link = p->link;
			p->link = q;
			p = q;
			p->as = ABSR;
			p->to.type = D_BRANCH;
			p->cond = ps2;
			p->to.sym = s2;

			continue;
		}
		if(p->as == ARTS) {
			/*
			 * JAL	_profout
			 */
			p->as = ABSR;
			p->to.type = D_BRANCH;
			p->cond = ps4;
			p->to.sym = s4;

			
			/*
			 * RTS
			 */
			q = prg();
			q->line = p->line;
			q->pc = p->pc;
			q->link = p->link;
			p->link = q;
			p = q;
			p->as = ARTS;

			continue;
		}
	}
}

long
reuse(Prog *r, Sym *s)
{
	Prog *p;


	if(r == P)
		return 0;
	for(p = datap; p != r; p = p->link)
		if(p->to.sym == s)
			return p->from.offset;
	return 0;
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
		c = find2(0x01020304L, i+1);
		gnuxi8[i] = c+4;
		gnuxi8[i+4] = c;
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
		Bprint(&bso, "\n[fg]nuxi = ");
		for(i=0; i<8; i++)
			Bprint(&bso, "%d", fnuxi8[i]);
		Bprint(&bso, " ");
		for(i=0; i<8; i++)
			Bprint(&bso, "%d", gnuxi8[i]);
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
	USED(p);
	USED(n);
	fprint(2, "realloc called\n");
	abort();
	return 0;
}
