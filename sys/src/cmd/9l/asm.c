#include	"l.h"

long	OFFSET;
/*
long	BADOFFSET	=	-1;

		if(OFFSET <= BADOFFSET && OFFSET+4 > BADOFFSET)\
			abort();\
		OFFSET += 4;\

		if(OFFSET == BADOFFSET)\
			abort();\
		OFFSET++;\
*/

#define	LPUT(c)\
	{\
		cbp[0] = (c)>>24;\
		cbp[1] = (c)>>16;\
		cbp[2] = (c)>>8;\
		cbp[3] = (c);\
		cbp += 4;\
		cbc -= 4;\
		if(cbc <= 0)\
			cflush();\
	}

#define	CPUT(c)\
	{\
		cbp[0] = (c);\
		cbp++;\
		cbc--;\
		if(cbc <= 0)\
			cflush();\
	}


long
entryvalue(void)
{
	char *a;
	Sym *s;

	a = INITENTRY;
	if(*a >= '0' && *a <= '9')
		return atolwhex(a);
	s = lookup(a, 0);
	if(s->type == 0)
		return INITTEXT;
	if(s->type != STEXT && s->type != SLEAF)
		diag("entry not text: %s", s->name);
	return s->value;
}

void
asmb(void)
{
	Prog *p;
	long t;
	Optab *o;

	if(debug['v'])
		Bprint(&bso, "%5.2f asm\n", cputime());
	Bflush(&bso);
	OFFSET = HEADR;
	seek(cout, OFFSET, 0);
	pc = INITTEXT;
	for(p = firstp; p != P; p = p->link) {
		if(p->as == ATEXT) {
			curtext = p;
			autosize = p->to.offset + 4;
		}
		if(p->pc != pc) {
			diag("phase error %lux sb %lux\n",
				p->pc, pc);
			if(!debug['a'])
				prasm(curp);
			pc = p->pc;
		}
		curp = p;
		o = oplook(p);	/* could probably avoid this call */
		if(asmout(p, o, 0)) {
			p = p->link;
			pc += 4;
		}
		pc += o->size;
	}
	if(debug['a'])
		Bprint(&bso, "\n");
	Bflush(&bso);
	cflush();

	curtext = P;
	switch(HEADTYPE) {
	case 4:
		OFFSET = rnd(HEADR+textsize, 4096);
		seek(cout, OFFSET, 0);
		break;
	case 0:
	case 1:
	case 2:
	case 3:
	case 5:
		OFFSET = HEADR+textsize;
		seek(cout, OFFSET, 0);
		break;
	}
	for(t = 0; t < datsize; t += sizeof(buf)-100) {
		if(datsize-t > sizeof(buf)-100)
			datblk(t, sizeof(buf)-100);
		else
			datblk(t, datsize-t);
	}

	symsize = 0;
	lcsize = 0;
	if(!debug['s']) {
		if(debug['v'])
			Bprint(&bso, "%5.2f sym\n", cputime());
		Bflush(&bso);
		switch(HEADTYPE) {
		case 4:
			OFFSET = rnd(HEADR+textsize, 4096)+datsize;
			seek(cout, OFFSET, 0);
			break;
		case 0:
		case 3:
		case 2:
		case 1:
		case 5:
			OFFSET = HEADR+textsize+datsize;
			seek(cout, OFFSET, 0);
			break;
		}
		if(!debug['s'])
			asmsym();
		if(debug['v'])
			Bprint(&bso, "%5.2f pc\n", cputime());
		Bflush(&bso);
		if(!debug['s'])
			asmlc();
		cflush();
	}

	if(debug['v'])
		Bprint(&bso, "%5.2f header\n", cputime());
	Bflush(&bso);
	OFFSET = 0;
	seek(cout, OFFSET, 0);
	switch(HEADTYPE) {
	case 0:
		break;
	case 1:
		lput(0x160L<<16);		/* magic and sections */
		lput(0L);			/* time and date */
		lput(HEADR+textsize+datsize);
		lput(symsize);			/* nsyms */
		lput((0x38L<<16)|7L);		/* size of optional hdr and flags */

		lput((0407<<16)|0437L);		/* magic and version */
		lput(textsize);			/* sizes */
		lput(datsize);
		lput(bsssize);
		lput(entryvalue());		/* va of entry */
		lput(INITTEXT);			/* va of base of text */
		lput(INITDAT);			/* va of base of data */
		lput(INITDAT+datsize);		/* va of base of bss */
		lput(~0L);			/* gp reg mask */
		lput(lcsize);
		lput(0L);
		lput(0L);
		lput(0L);
		lput(~0L);			/* gp value ?? */
		lput(0L);			/* complete mystery */
		break;
	case 2:
		lput(1451);			/* magic */
		lput(textsize);			/* sizes */
		lput(datsize);
		lput(bsssize);
		lput(symsize);			/* nsyms */
		lput(entryvalue());		/* va of entry */
		lput(0L);
		lput(lcsize);
		break;
	case 3:
		lput((0x160L<<16)|3L);		/* magic and sections */
		lput(time(0));			/* time and date */
		lput(HEADR+textsize+datsize);
		lput(symsize);			/* nsyms */
		lput((0x38L<<16)|7L);		/* size of optional hdr and flags */

		lput((0407<<16)|0437L);		/* magic and version */
		lput(textsize);			/* sizes */
		lput(datsize);
		lput(bsssize);
		lput(entryvalue());		/* va of entry */
		lput(INITTEXT);			/* va of base of text */
		lput(INITDAT);			/* va of base of data */
		lput(INITDAT+datsize);		/* va of base of bss */
		lput(~0L);			/* gp reg mask */
		lput(lcsize);
		lput(0L);
		lput(0L);
		lput(0L);
		lput(~0L);			/* gp value ?? */

		strnput(".text", 8);		/* text segment */
		lput(INITTEXT);			/* address */
		lput(INITTEXT);
		lput(textsize);
		lput(HEADR);
		lput(0L);
		lput(HEADR+textsize+datsize+symsize);
		lput(lcsize);			/* line number size */
		lput(0x20L);			/* flags */

		strnput(".data", 8);		/* data segment */
		lput(INITDAT);			/* address */
		lput(INITDAT);
		lput(datsize);
		lput(HEADR+textsize);
		lput(0L);
		lput(0L);
		lput(0L);
		lput(0x40L);			/* flags */

		strnput(".bss", 8);		/* bss segment */
		lput(INITDAT+datsize);		/* address */
		lput(INITDAT+datsize);
		lput(bsssize);
		lput(0L);
		lput(0L);
		lput(0L);
		lput(0L);
		lput(0x80L);			/* flags */
		break;
	case 4:

		lput((0x160L<<16)|3L);		/* magic and sections */
		lput(time(0));			/* time and date */
		lput(rnd(HEADR+textsize, 4096)+datsize);
		lput(symsize);			/* nsyms */
		lput((0x38L<<16)|7L);		/* size of optional hdr and flags */

		lput((0413<<16)|01012L);	/* magic and version */
		lput(textsize);			/* sizes */
		lput(datsize);
		lput(bsssize);
		lput(entryvalue());		/* va of entry */
		lput(INITTEXT);			/* va of base of text */
		lput(INITDAT);			/* va of base of data */
		lput(INITDAT+datsize);		/* va of base of bss */
		lput(~0L);			/* gp reg mask */
		lput(lcsize);
		lput(0L);
		lput(0L);
		lput(0L);
		lput(~0L);			/* gp value ?? */

		strnput(".text", 8);		/* text segment */
		lput(INITTEXT);			/* address */
		lput(INITTEXT);
		lput(textsize);
		lput(HEADR);
		lput(0L);
		lput(HEADR+textsize+datsize+symsize);
		lput(lcsize);			/* line number size */
		lput(0x20L);			/* flags */

		strnput(".data", 8);		/* data segment */
		lput(INITDAT);			/* address */
		lput(INITDAT);
		lput(datsize);
		lput(rnd(HEADR+textsize, 4096));	/* sizes */
		lput(0L);
		lput(0L);
		lput(0L);
		lput(0x40L);			/* flags */

		strnput(".bss", 8);		/* bss segment */
		lput(INITDAT+datsize);		/* address */
		lput(INITDAT+datsize);
		lput(bsssize);
		lput(0L);
		lput(0L);
		lput(0L);
		lput(0L);
		lput(0x80L);			/* flags */
		break;
	case 5:
		strnput("\177ELF", 4);		/* e_ident */
		CPUT(1);			/* class = 32 bit */
		CPUT(2);			/* data = MSB */
		CPUT(1);			/* version = CURRENT */
		strnput("", 9);
		lput((2L<<16)|8L);		/* type = EXEC; machine = MIPS */
		lput(1L);			/* version = CURRENT */
		lput(entryvalue());		/* entry vaddr */
		lput(52L);			/* offset to first phdr */
		lput(0L);			/* offset to first shdr */
		lput(0L);			/* flags = MIPS */
		lput((52L<<16)|32L);		/* Ehdr & Phdr sizes*/
		lput((3L<<16)|0L);		/* # Phdrs & Shdr size */
		lput((0L<<16)|0L);		/* # Shdrs & shdr string size */

		lput(1L);			/* text - type = PT_LOAD */
		lput(0L);			/* file offset */
		lput(INITTEXT-HEADR);		/* vaddr */
		lput(INITTEXT-HEADR);		/* paddr */
		lput(HEADR+textsize);		/* file size */
		lput(HEADR+textsize);		/* memory size */
		lput(0x05L);			/* protections = RWX */
		lput(0x10000L);			/* alignment code?? */

		lput(1L);			/* data - type = PT_LOAD */
		lput(HEADR+textsize);		/* file offset */
		lput(INITDAT);			/* vaddr */
		lput(INITDAT);			/* paddr */
		lput(datsize);			/* file size */
		lput(datsize+bsssize);		/* memory size */
		lput(0x06L);			/* protections = RW */
		lput(0x10000L);			/* alignment code?? */

		lput(0L);			/* data - type = PT_NULL */
		lput(HEADR+textsize+datsize);	/* file offset */
		lput(0L);
		lput(0L);
		lput(symsize);			/* symbol table size */
		lput(lcsize);			/* line number size */
		lput(0x04L);			/* protections = R */
		lput(0x04L);			/* alignment code?? */
	}
	cflush();
}

void
strnput(char *s, int n)
{
	for(; *s; s++){
		CPUT(*s);
		n--;
	}
	for(; n > 0; n--)
		CPUT(0);
}

void
lput(long l)
{

	LPUT(l);
}

void
cflush(void)
{
	int n;

	n = sizeof(buf.cbuf) - cbc;
	if(n)
		write(cout, buf.cbuf, n);
	cbp = buf.cbuf;
	cbc = sizeof(buf.cbuf);
}

void
nopstat(char *f, Count *c)
{
	if(c->outof)
	Bprint(&bso, "%s delay %ld/%ld (%.2f)\n", f,
		c->outof - c->count, c->outof,
		(double)(c->outof - c->count)/c->outof);
}

void
asmsym(void)
{
	Prog *p;
	Auto *a;
	Sym *s;
	int h;

	s = lookup("etext", 0);
	if(s->type == STEXT)
		putsymb(s->name, 'T', s->value, s->version);

	for(h=0; h<NHASH; h++)
		for(s=hash[h]; s!=S; s=s->link)
			switch(s->type) {
			case SCONST:
				putsymb(s->name, 'D', s->value, s->version);
				continue;

			case SDATA:
				putsymb(s->name, 'D', s->value+INITDAT, s->version);
				continue;

			case SBSS:
				putsymb(s->name, 'B', s->value+INITDAT, s->version);
				continue;

			case SFILE:
				putsymb(s->name, 'f', s->value, s->version);
				continue;
			}

	for(p=textp; p!=P; p=p->cond) {
		s = p->from.sym;
		if(s->type != STEXT && s->type != SLEAF)
			continue;

		/* filenames first */
		for(a=p->to.autom; a; a=a->link)
			if(a->type == D_FILE)
				putsymb(a->sym->name, 'z', a->offset, 0);
			else
			if(a->type == D_FILE1)
				putsymb(a->sym->name, 'Z', a->offset, 0);

		if(s->type == STEXT)
			putsymb(s->name, 'T', s->value, s->version);
		else
			putsymb(s->name, 'L', s->value, s->version);

		/* frame, auto and param after */
		putsymb(".frame", 'm', p->to.offset+4, 0);
		for(a=p->to.autom; a; a=a->link)
			if(a->type == D_AUTO)
				putsymb(a->sym->name, 'a', -a->offset, 0);
			else
			if(a->type == D_PARAM)
				putsymb(a->sym->name, 'p', a->offset, 0);
	}
	if(debug['v'] || debug['n'])
		Bprint(&bso, "symsize = %lud\n", symsize);
	Bflush(&bso);
}

void
putsymb(char *s, int t, long v, int ver)
{
	int i, f;

	if(t == 'f')
		s++;
	LPUT(v);
	if(ver)
		t += 'a' - 'A';
	CPUT(t+0x80);			/* 0x80 is variable length */

	if(t == 'Z' || t == 'z') {
		CPUT(s[0]);
		for(i=1; s[i] != 0 || s[i+1] != 0; i += 2) {
			CPUT(s[i]);
			CPUT(s[i+1]);
		}
		CPUT(0);
		CPUT(0);
		i++;
	}
	else {
		for(i=0; s[i]; i++)
			CPUT(s[i]);
		CPUT(0);
	}
	symsize += 4 + 1 + i + 1;

	if(debug['n']) {
		if(t == 'z' || t == 'Z') {
			Bprint(&bso, "%c %.8lux ", t, v);
			for(i=1; s[i] != 0 || s[i+1] != 0; i+=2) {
				f = ((s[i]&0xff) << 8) | (s[i+1]&0xff);
				Bprint(&bso, "/%x", f);
			}
			Bprint(&bso, "\n");
			return;
		}
		if(ver)
			Bprint(&bso, "%c %.8lux %s<%d>\n", t, v, s, ver);
		else
			Bprint(&bso, "%c %.8lux %s\n", t, v, s);
	}
}

#define	MINLC	4
void
asmlc(void)
{
	long oldpc, oldlc;
	Prog *p;
	long v, s;

	oldpc = INITTEXT;
	oldlc = 0;
	for(p = firstp; p != P; p = p->link) {
		if(p->line == oldlc || p->as == ATEXT || p->as == ANOP) {
			if(p->as == ATEXT)
				curtext = p;
			if(debug['L'])
				Bprint(&bso, "%6lux %P\n",
					p->pc, p);
			continue;
		}
		if(debug['L'])
			Bprint(&bso, "\t\t%6ld", lcsize);
		v = (p->pc - oldpc) / MINLC;
		while(v) {
			s = 127;
			if(v < 127)
				s = v;
			CPUT(s+128);	/* 129-255 +pc */
			if(debug['L'])
				Bprint(&bso, " pc+%ld*%d(%ld)", s, MINLC, s+128);
			v -= s;
			lcsize++;
		}
		s = p->line - oldlc;
		oldlc = p->line;
		oldpc = p->pc + MINLC;
		if(s > 64 || s < -64) {
			CPUT(0);	/* 0 vv +lc */
			CPUT(s>>24);
			CPUT(s>>16);
			CPUT(s>>8);
			CPUT(s);
			if(debug['L']) {
				if(s > 0)
					Bprint(&bso, " lc+%ld(%d,%ld)\n",
						s, 0, s);
				else
					Bprint(&bso, " lc%ld(%d,%ld)\n",
						s, 0, s);
				Bprint(&bso, "%6lux %P\n",
					p->pc, p);
			}
			lcsize += 5;
			continue;
		}
		if(s > 0) {
			CPUT(0+s);	/* 1-64 +lc */
			if(debug['L']) {
				Bprint(&bso, " lc+%ld(%ld)\n", s, 0+s);
				Bprint(&bso, "%6lux %P\n",
					p->pc, p);
			}
		} else {
			CPUT(64-s);	/* 65-128 -lc */
			if(debug['L']) {
				Bprint(&bso, " lc%ld(%ld)\n", s, 64-s);
				Bprint(&bso, "%6lux %P\n",
					p->pc, p);
			}
		}
		lcsize++;
	}
	while(lcsize & 1) {
		s = 129;
		CPUT(s);
		lcsize++;
	}
	if(debug['v'] || debug['L'])
		Bprint(&bso, "lcsize = %ld\n", lcsize);
	Bflush(&bso);
}

void
datblk(long s, long n)
{
	Prog *p;
	char *cast;
	long l, fl, j, d;
	int i, c;

	memset(buf.dbuf, 0, n+100);
	for(p = datap; p != P; p = p->link) {
		curp = p;
		l = p->from.sym->value + p->from.offset - s;
		c = p->reg;
		i = 0;
		if(l < 0) {
			if(l+c <= 0)
				continue;
			while(l < 0) {
				l++;
				i++;
			}
		}
		if(l >= n)
			continue;
		if(p->as != AINIT && p->as != ADYNT) {
			for(j=l+(c-i)-1; j>=l; j--)
				if(buf.dbuf[j]) {
					print("%P\n", p);
					diag("multiple initialization\n");
					break;
				}
		}
		switch(p->to.type) {
		default:
			diag("unknown mode in initialization\n%P\n", p);
			break;

		case D_FCONST:
			switch(c) {
			default:
			case 4:
				fl = ieeedtof(p->to.ieee);
				cast = (char*)&fl;
				for(; i<c; i++) {
					buf.dbuf[l] = cast[fnuxi8[i+4]];
					l++;
				}
				break;
			case 8:
				cast = (char*)p->to.ieee;
				for(; i<c; i++) {
					buf.dbuf[l] = cast[fnuxi8[i]];
					l++;
				}
				break;
			}
			break;

		case D_SCONST:
			for(; i<c; i++) {
				buf.dbuf[l] = p->to.sval[i];
				l++;
			}
			break;

		case D_CONST:
			d = p->to.offset;
			if(p->to.sym) {
				if(p->to.sym->type == STEXT ||
				   p->to.sym->type == SLEAF)
					d += p->to.sym->value;
				if(p->to.sym->type == SDATA)
					d += p->to.sym->value + INITDAT;
				if(p->to.sym->type == SBSS)
					d += p->to.sym->value + INITDAT;
			}
			cast = (char*)&d;
			switch(c) {
			default:
				diag("bad nuxi %d %d\n%P\n", c, i, curp);
				break;
			case 1:
				for(; i<c; i++) {
					buf.dbuf[l] = cast[inuxi1[i]];
					l++;
				}
				break;
			case 2:
				for(; i<c; i++) {
					buf.dbuf[l] = cast[inuxi2[i]];
					l++;
				}
				break;
			case 4:
				for(; i<c; i++) {
					buf.dbuf[l] = cast[inuxi4[i]];
					l++;
				}
				break;
			}
			break;
		}
	}
	write(cout, buf.dbuf, n);
}
enum
{
	BCONSTP		= 0x03,
	BCONSTN		= 0x01,
	BCONSTH		= 0x02,

	BLOAD		= 0x16,
	BSTORE		= 0x1e,
	BLOADSET	= 0x26,
	BLOADM		= 0x36,
	BSTOREM		= 0x3e,

	BJMP		= 0xa0,
	BJMPF		= 0xa4,
	BCALL		= 0xa8,
	BJMPT		= 0xac,
};

int
opas(int a)
{
	switch(a) {
	case AJMP:	return BJMP;
	case ACALL:	return BCALL;
	case AJMPF:	return BJMPF;
	case AJMPT:	return BJMPT;
	}
	diag("bad opas %A\n", a);
	return 0;
}

int
asmout(Prog *p, Optab *o, int aflag)
{
	long o1, o2, o3, o4, o5, v;
	Prog *ct;
	int r, a;

	o1 = 0;
	o2 = 0;
	o3 = 0;
	o4 = 0;
	o5 = 0;
	a = p->as;
	switch(o->type) {
	default:
		diag("unknown type %d\n%P\n", o->type, p);
		break;

	case 0:		/* pseudo ops */
		if(aflag) {
			if(p->link) {
				if(a == ATEXT) {
					ct = curtext;
					o2 = autosize;
					curtext = p;
					autosize = p->to.offset + 4;
					o1 = asmout(p->link, oplook(p->link), aflag);
					curtext = ct;
					autosize = o2;
				} else
					o1 = asmout(p->link, oplook(p->link), aflag);
			}
			return o1;
		}
		break;

	case 1:		/* mov r, r */
		r = p->from.reg;
		o1 = oprrr(AORL, r, r, p->to.reg);
		break;
	case 2:		/* mov $[pn]con, r */
		v = regoff(&p->from);
		if(v >= 0)
			o1 = opir(BCONSTP, v, p->to.reg);
		else
			o1 = opir(BCONSTN, v, p->to.reg);
		break;
	case 3:		/* mov $lcon, r */
		v = regoff(&p->from);
		o1 = opir(BCONSTP, v, p->to.reg);
		o2 = opir(BCONSTH, v>>16, p->to.reg);
		break;
	case 4:		/* mov $sacon, r */
		v = regoff(&p->from);
		if(v >= 0)
			o1 = oprrr(AADDL, v, REGSP, p->to.reg);
		else
			o1 = oprrr(ASUBL, -v, REGSP, p->to.reg);
		o1 |= 1<<24;
		break;
	case 5:		/* mov $pacon, r */
		v = regoff(&p->from);
		if(v >= 0)
			o1 = opir(BCONSTP, v, REGTMP);
		else
			o1 = opir(BCONSTN, v, REGTMP);
		o2 = oprrr(AADDL, REGTMP, REGSP, p->to.reg);
		break;
	case 6:		/* mov $pacon, r */
		v = regoff(&p->from);
		o1 = opir(BCONSTP, v, REGTMP);
		o2 = opir(BCONSTH, v>>16, REGTMP);
		o3 = oprrr(AADDL, REGTMP, REGSP, p->to.reg);
		break;

	case 10:	/* mov r,o(zcon) */
		r = p->to.reg;
		if(r == NREG)
			r = o->param;
		o1 = opmem(BSTORE, a, r, p->from.reg);
		break;
	case 11:	/* mov r,o(scon) */
		r = p->to.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->to);
		if(v >= 0)
			o1 = oprrr(AADDL, v, r, REGTMP);
		else
			o1 = oprrr(ASUBL, -v, r, REGTMP);
		o1 |= 1<<24;
		o2 = opmem(BSTORE, a, REGTMP, p->from.reg);
		break;
	case 12:	/* mov r,o(pcon) */
		r = p->to.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->to);
		if(v >= 0)
			o1 = opir(BCONSTP, v, REGTMP);
		else
			o1 = opir(BCONSTN, v, REGTMP);
		o2 = oprrr(AADDL, r, REGTMP, REGTMP);
		o3 = opmem(BSTORE, a, REGTMP, p->from.reg);
		break;
	case 13:	/* mov r,o(lcon) */
		r = p->to.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->to);
		o1 = opir(BCONSTP, v, REGTMP);
		o2 = opir(BCONSTH, v>>16, REGTMP);
		o3 = oprrr(AADDL, r, REGTMP, REGTMP);
		o4 = opmem(BSTORE, a, REGTMP, p->from.reg);
		break;
	case 14:	/* mov r,sext */
		v = regoff(&p->to);
		if(v >= INITDAT && v <= INITDAT+0xff)
			o1 = oprrr(ASUBL, (INITDAT+0xff)-v, REGSB, REGTMP) | (1<<24);
		else
		if(v >= INITDAT+0xff && v <= INITDAT+0xff+0xff)
			o1 = oprrr(AADDL, v-(INITDAT+0xff), REGSB, REGTMP) | (1<<24);
		else
			o1 = opir(BCONSTP, v, REGTMP);
		o2 = opmem(BSTORE, a, REGTMP, p->from.reg);
		break;
	case 15:	/* mov r,lext */
		v = regoff(&p->to);
		o1 = opir(BCONSTP, v, REGTMP);
		o2 = opir(BCONSTH, v>>16, REGTMP);
		o3 = opmem(BSTORE, a, REGTMP, p->from.reg);
		break;

	case 20:	/* mov o(zcon),r */
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		o1 = opmem(BLOAD, a, r, p->to.reg);
		break;
	case 21:	/* mov o(scon),r */
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->from);
		if(v >= 0)
			o1 = oprrr(AADDL, v, r, REGTMP);
		else
			o1 = oprrr(ASUBL, -v, r, REGTMP);
		o1 |= 1<<24;
		o2 = opmem(BLOAD, a, REGTMP, p->to.reg);
		break;
	case 22:	/* mov o(pcon),r */
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->from);
		if(v >= 0)
			o1 = opir(BCONSTP, v, REGTMP);
		else
			o1 = opir(BCONSTN, v, REGTMP);
		o2 = oprrr(AADDL, r, REGTMP, REGTMP);
		o3 = opmem(BLOAD, a, REGTMP, p->to.reg);
		break;
	case 23:	/* mov o(lcon),r */
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->from);
		o1 = opir(BCONSTP, v, REGTMP);
		o2 = opir(BCONSTH, v>>16, REGTMP);
		o3 = oprrr(AADDL, r, REGTMP, REGTMP);
		o4 = opmem(BLOAD, a, REGTMP, p->to.reg);
		break;
	case 24:	/* mov sext,r */
		v = regoff(&p->from);
		if(v >= INITDAT && v <= INITDAT+0xff)
			o1 = oprrr(ASUBL, (INITDAT+0xff)-v, REGSB, REGTMP) | (1<<24);
		else
		if(v >= INITDAT+0xff && v <= INITDAT+0xff+0xff)
			o1 = oprrr(AADDL, v-(INITDAT+0xff), REGSB, REGTMP) | (1<<24);
		else
			o1 = opir(BCONSTP, v, REGTMP);
		o2 = opmem(BLOAD, a, REGTMP, p->to.reg);
		break;
	case 25:	/* mov lext,r */
		v = regoff(&p->from);
		o1 = opir(BCONSTP, v, REGTMP);
		o2 = opir(BCONSTH, v>>16, REGTMP);
		o3 = opmem(BLOAD, a, REGTMP, p->to.reg);
		break;

	case 30:	/* add r,[r],r */
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		o1 = oprrr(p->as, p->from.reg, r, p->to.reg);
		break;
	case 31:	/* add $scon,r,r */
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		v = regoff(&p->from);
		o1 = oprrr(p->as, v, r, p->to.reg);
		o1 |= 1<<24;	/* const */
		break;
	case 32:	/* add $pcon,r,r */
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		v = regoff(&p->from);
		if(v >= 0)
			o1 = opir(BCONSTP, v, REGTMP);
		else
			o1 = opir(BCONSTN, v, REGTMP);
		o2 = oprrr(p->as, REGTMP, r, p->to.reg);
		break;
	case 33:	/* add $lcon,r,r */
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		v = regoff(&p->from);
		o1 = opir(BCONSTP, v, REGTMP);
		o2 = opir(BCONSTH, v>>16, REGTMP);
		o3 = oprrr(p->as, REGTMP, r, p->to.reg);
		break;

	case 40:	/* jmp [r],sbra */
		if(aflag)
			return 0;
		v = 0;
		if(p->cond != P)
			v = (p->cond->pc - p->pc) >> 2;
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		o1 = opir(opas(a), v, r);
		if(0 && !debug['Y'] && p->link && p->cond && isnop(p->link)) {
			nop.branch.count--;
			nop.branch.outof--;
			nop.jump.outof++;
			o2 = asmout(p->cond, oplook(p->cond), 1);
			if(o2) {
				o1 += 1;
				if(debug['a'])
					Bprint(&bso, " %.8lux: %.8lux %.8lux%P\n",
						p->pc, o1, o2, p);
				LPUT(o1);
				LPUT(o2);
				return 1;
			}
		}
		break;
	case 41:	/* jmpf [r],sbra */
		if(aflag)
			return 0;
		v = 0;
		if(p->cond != P)
			v = (p->cond->pc - p->pc) >> 2;
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		o1 = opir(opas(a), v, r);
		break;
	case 42:	/* jmp [r],lbra */
		v = p->pc;
		if(p->cond != P)
			v = p->cond->pc;
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		o1 = opir(BCONSTP, v, REGTMP);
		o2 = opir(BCONSTH, v>>16, REGTMP);
		o3 = oprrr(a, REGTMP, r, 0);
		break;
	case 43:	/* jmp [r],zoreg */
		if(aflag)
			return 0;
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		o1 = oprrr(a, p->to.reg, r, 0);
		break;

	case 50:	/* delay */
		o1 = oprrr(AASEQ, 1, 1, 0x40);
		break;
	case 51:	/* emulate */
		o1 = oprrr(AEMULATE, 0, 0, 0);
		break;
	case 52:	/* mtsr */
		o1 = oprrr(a, p->from.reg, p->to.reg, 0);
		break;
	case 53:	/* mtsrim $con, r */
		v = regoff(&p->from);
		o1 = ((0x04)<<24) | ((v & 0xff00)<<8) | (p->to.reg<<8) | (v & 0xff);
		break;
	case 54:	/* mtsr $lcon, r */
		v = regoff(&p->from);
		o1 = opir(BCONSTP, v, REGTMP);
		o2 = opir(BCONSTH, v>>16, REGTMP);
		o3 = oprrr(a, REGTMP, p->to.reg, 0);
		break;
	case 55:	/* mfsr */
		o1 = oprrr(a, 0, p->from.reg, p->to.reg);
		break;
	case 56:	/* iret, halt, etc */
		o1 = oprrr(a, 0, 0, 0);
		break;
	case 57:	/* inv */
		o1 = oprrr(a, 0, 0, p->to.offset & 0x3);
		break;
	case 60:	/* movd r,o(zcon) */
		o1 = opir(BCONSTP, 1, 135);
		r = p->to.reg;
		if(r == NREG)
			r = o->param;
		o2 = opmem(BSTORE, ASTOREM, r, p->from.reg);
		break;
	case 61:	/* movd r,o(scon) */
		o1 = opir(BCONSTP, 1, 135);
		r = p->to.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->to);
		if(v >= 0)
			o2 = oprrr(AADDL, v, r, REGTMP);
		else
			o2 = oprrr(ASUBL, -v, r, REGTMP);
		o2 |= 1<<24;
		o3 = opmem(BSTORE, ASTOREM, REGTMP, p->from.reg);
		break;
	case 62:	/* movd r,o(pcon) */
		o1 = opir(BCONSTP, 1, 135);
		r = p->to.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->to);
		if(v >= 0)
			o2 = opir(BCONSTP, v, REGTMP);
		else
			o2 = opir(BCONSTN, v, REGTMP);
		o3 = oprrr(AADDL, r, REGTMP, REGTMP);
		o4 = opmem(BSTORE, ASTOREM, REGTMP, p->from.reg);
		break;
	case 63:	/* movd r,o(lcon) */
		o1 = opir(BCONSTP, 1, 135);
		r = p->to.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->to);
		o2 = opir(BCONSTP, v, REGTMP);
		o3 = opir(BCONSTH, v>>16, REGTMP);
		o4 = oprrr(AADDL, r, REGTMP, REGTMP);
		o5 = opmem(BSTORE, ASTOREM, REGTMP, p->from.reg);
		break;
	case 64:	/* movd r,sext */
		o1 = opir(BCONSTP, 1, 135);
		v = regoff(&p->to);
		if(v >= INITDAT && v <= INITDAT+0xff)
			o2 = oprrr(ASUBL, (INITDAT+0xff)-v, REGSB, REGTMP) | (1<<24);
		else
		if(v >= INITDAT+0xff && v <= INITDAT+0xff+0xff)
			o2 = oprrr(AADDL, v-(INITDAT+0xff), REGSB, REGTMP) | (1<<24);
		else
			o2 = opir(BCONSTP, v, REGTMP);
		o3 = opmem(BSTORE, ASTOREM, REGTMP, p->from.reg);
		break;
	case 65:	/* movd r,lext */
		o1 = opir(BCONSTP, 1, 135);
		v = regoff(&p->to);
		o2 = opir(BCONSTP, v, REGTMP);
		o3 = opir(BCONSTH, v>>16, REGTMP);
		o4 = opmem(BSTORE, ASTOREM, REGTMP, p->from.reg);
		break;
	case 66:	/* word */
		o1 = p->to.offset;
		break;
	case 70:	/* mov o(zcon),r */
		o1 = opir(BCONSTP, 1, 135);
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		o2 = opmem(BLOAD, ALOADM, r, p->to.reg);
		break;
	case 71:	/* movd o(scon),r */
		o1 = opir(BCONSTP, 1, 135);
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->from);
		if(v >= 0)
			o2 = oprrr(AADDL, v, r, REGTMP);
		else
			o2 = oprrr(ASUBL, -v, r, REGTMP);
		o2 |= 1<<24;
		o3 = opmem(BLOAD, ALOADM, REGTMP, p->to.reg);
		break;
	case 72:	/* movd o(pcon),r */
		o1 = opir(BCONSTP, 1, 135);
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->from);
		if(v >= 0)
			o2 = opir(BCONSTP, v, REGTMP);
		else
			o2 = opir(BCONSTN, v, REGTMP);
		o3 = oprrr(AADDL, r, REGTMP, REGTMP);
		o4 = opmem(BLOAD, ALOADM, REGTMP, p->to.reg);
		break;
	case 73:	/* movd o(lcon),r */
		o1 = opir(BCONSTP, 1, 135);
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->from);
		o2 = opir(BCONSTP, v, REGTMP);
		o3 = opir(BCONSTH, v>>16, REGTMP);
		o4 = oprrr(AADDL, r, REGTMP, REGTMP);
		o5 = opmem(BLOAD, ALOADM, REGTMP, p->to.reg);
		break;
	case 74:	/* movd sext,r */
		o1 = opir(BCONSTP, 1, 135);
		v = regoff(&p->from);
		if(v >= INITDAT && v <= INITDAT+0xff)
			o2 = oprrr(ASUBL, (INITDAT+0xff)-v, REGSB, REGTMP) | (1<<24);
		else
		if(v >= INITDAT+0xff && v <= INITDAT+0xff+0xff)
			o2 = oprrr(AADDL, v-(INITDAT+0xff), REGSB, REGTMP) | (1<<24);
		else
			o2 = opir(BCONSTP, v, REGTMP);
		o3 = opmem(BLOAD, ALOADM, REGTMP, p->to.reg);
		break;
	case 75:	/* movd lext,r */
		o1 = opir(BCONSTP, 1, 135);
		v = regoff(&p->from);
		o2 = opir(BCONSTP, v, REGTMP);
		o3 = opir(BCONSTH, v>>16, REGTMP);
		o4 = opmem(BLOAD, ALOADM, REGTMP, p->to.reg);
		break;
	case 76:		/* movd r, r */
		r = p->from.reg;
		o1 = oprrr(AORL, r+0, r+0, p->to.reg+0);
		o2 = oprrr(AORL, r+1, r+1, p->to.reg+1);
		break;
	}
	if(aflag)
		return o1;
	v = p->pc;
	switch(o->size) {
	default:
		if(debug['a'])
			Bprint(&bso, " %.8lux:\t\t\t%P\n", v, p);
		break;
	case 4:
		if(debug['a'])
			Bprint(&bso, " %.8lux: %.8lux\t\t%P\n", v, o1, p);
		LPUT(o1);
		break;
	case 8:
		if(debug['a'])
			Bprint(&bso, " %.8lux: %.8lux %.8lux\t%P\n", v, o1, o2, p);
		LPUT(o1);
		LPUT(o2);
		break;
	case 12:
		if(debug['a'])
			Bprint(&bso, " %.8lux: %.8lux %.8lux %.8lux%P\n", v, o1, o2, o3, p);
		LPUT(o1);
		LPUT(o2);
		LPUT(o3);
		break;
	case 16:
		if(debug['a'])
			Bprint(&bso, " %.8lux: %.8lux %.8lux %.8lux %.8lux%P\n",
				v, o1, o2, o3, o4, p);
		LPUT(o1);
		LPUT(o2);
		LPUT(o3);
		LPUT(o4);
		break;
	case 20:
		if(debug['a'])
			Bprint(&bso, " %.8lux: %.8lux %.8lux %.8lux %.8lux %.8lux%P\n",
				v, o1, o2, o3, o4, o5, p);
		LPUT(o1);
		LPUT(o2);
		LPUT(o3);
		LPUT(o4);
		LPUT(o5);
		break;
	}
	return 0;
}

int
isnop(Prog *p)
{
	if(p->as == ADELAY)
		return 1;
	return 0;
}

long
oprrr(int a, int rb, int ra, int rc)
{
	long o;

	switch(a) {
	default:
		diag("bad oprrr %A\n", a);
	case AMOVL:	o = 0x00<<24; break;

	case AADDL:	o = 0x14<<24; break;
	case ASUBL:	o = 0x24<<24; break;
	case AMULL:	o = 0xe0<<24; break;
	case AMULUL:	o = 0xe2<<24; break;
	case AMULML:	o = 0xde<<24; break;
	case AMULMUL:	o = 0xdf<<24; break;
	case AISUBL:	o = 0x34<<24; break;
	case AORL:	o = 0x92<<24; break;
	case AANDL:	o = 0x90<<24; break;
	case AXORL:	o = 0x94<<24; break;
	case AXNORL:	o = 0x96<<24; break;
	case ASRAL:	o = 0x86<<24; break;
	case ASRLL:	o = 0x82<<24; break;
	case ASLLL:	o = 0x80<<24; break;

	case ACPEQL:	o = 0x60<<24; break;
	case ACPGEL:	o = 0x4c<<24; break;
	case ACPGEUL:	o = 0x4e<<24; break;
	case ACPGTL:	o = 0x48<<24; break;
	case ACPGTUL:	o = 0x4a<<24; break;
	case ACPLEL:	o = 0x44<<24; break;
	case ACPLEUL:	o = 0x46<<24; break;
	case ACPLTL:	o = 0x40<<24; break;
	case ACPLTUL:	o = 0x42<<24; break;
	case ACPNEQL:	o = 0x62<<24; break;

	case AADDD:	o = 0xf1<<24; break;
	case ASUBD:	o = 0xf3<<24; break;
	case AMULD:	o = 0xf5<<24; break;
	case ADIVD:	o = 0xf7<<24; break;
	case AADDF:	o = 0xf0<<24; break;
	case ASUBF:	o = 0xf2<<24; break;
	case AMULF:	o = 0xf4<<24; break;
	case ADIVF:	o = 0xf6<<24; break;

	case AEQD:	o = 0xeb<<24; break;
	case AGED:	o = 0xef<<24; break;
	case AGTD:	o = 0xed<<24; break;
	case AEQF:	o = 0xea<<24; break;
	case AGEF:	o = 0xee<<24; break;
	case AGTF:	o = 0xec<<24; break;

	case AMSTEPL:	o = 0x64<<24; break;
	case AMSTEPUL:	o = 0x74<<24; break;
	case AMSTEPLL:	o = 0x66<<24; break;
	case ADSTEPL:	o = 0x6a<<24; break;
	case ADSTEP0L:	o = 0x68<<24; break;
	case ADSTEPLL:	o = 0x6c<<24; break;
	case ADSTEPRL:	o = 0x6e<<24; break;

	case AJMP:	o = 0xc0<<24; break;
	case AJMPF:	o = 0xc4<<24; break;
	case AJMPT:	o = 0xcc<<24; break;
	case ACALL:	o = 0xc8<<24; break;
	case AEMULATE:	o = 0xd7<<24; break;
	case ASETIP:	o = 0x9e<<24; break;
	case AMFSR:	o = 0xc6<<24; break;
	case AMTSR:	o = 0xce<<24; break;
	case AIRET:	o = 0x88<<24; break;
	case AINV:	o = 0x9f<<24; break;

	case AASEQ:	o = 0x70<<24; break;
	}
	return o | ((rb&0xff)<<0) | ((ra&0xff)<<8) | ((rc&0xff)<<16);
}

long
opir(int o, int i, int r)
{

	return oprrr(AMOVL, i, r, i>>8) | (o<<24);
}

long
opmem(int m, int a, int ir, int r)
{
	long o;

	switch(a) {
	default:
		diag("bad opmem %A\n", a);
	case ALOADSET:
		if(m != BLOAD)
			diag("bad loadset code %A\n", a);
		o = 0x00<<16;
		m = BLOADSET;
		break;
	case ALOADM:
		if(m != BLOAD)
			diag("bad loadm code %A\n", a);
		o = 0x00<<16;
		m = BLOADM;
		break;
	case ASTOREM:
		if(m != BSTORE)
			diag("bad storem code %A\n", a);
		o = 0x00<<16;
		m = BSTOREM;
		break;
	case AMOVF:
	case AMOVL:	o = 0x00<<16; break;
	case AMOVH:	o = 0x12<<16; break;
	case AMOVHU:	o = 0x02<<16; break;
	case AMOVB:	o = 0x11<<16; break;
	case AMOVBU:	o = 0x01<<16; break;
	}
	return o | ((ir&0xff) << 0) | ((r&0xff) << 8) | (m << 24);
}
