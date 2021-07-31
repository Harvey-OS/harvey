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
	long t, etext;
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
			diag("phase error %lux sb %lux",
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

	etext = INITTEXT + textsize;
	for(t = pc; t < etext; t += sizeof(buf)-100) {
		if(etext-t > sizeof(buf)-100)
			datblk(t, sizeof(buf)-100, 1);
		else
			datblk(t, etext-t, 1);
	}

	Bflush(&bso);
	cflush();

	curtext = P;
	switch(HEADTYPE) {
	case 0:
	case 4:
		OFFSET = rnd(HEADR+textsize, 4096);
		seek(cout, OFFSET, 0);
		break;
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
			datblk(t, sizeof(buf)-100, 0);
		else
			datblk(t, datsize-t, 0);
	}

	symsize = 0;
	lcsize = 0;
	if(!debug['s']) {
		if(debug['v'])
			Bprint(&bso, "%5.2f sym\n", cputime());
		Bflush(&bso);
		switch(HEADTYPE) {
		case 0:
		case 4:
			OFFSET = rnd(HEADR+textsize, 4096)+datsize;
			seek(cout, OFFSET, 0);
			break;
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
		lput(0x160L<<16);		/* magic and sections */
		lput(0L);			/* time and date */
		lput(rnd(HEADR+textsize, 4096)+datsize);
		lput(symsize);			/* nsyms */
		lput((0x38L<<16)|7L);		/* size of optional hdr and flags */
		lput((0413<<16)|0437L);		/* magic and version */
		lput(rnd(HEADR+textsize, 4096));	/* sizes */
		lput(datsize);
		lput(bsssize);
		lput(entryvalue());		/* va of entry */
		lput(INITTEXT-HEADR);		/* va of base of text */
		lput(INITDAT);			/* va of base of data */
		lput(INITDAT+datsize);		/* va of base of bss */
		lput(~0L);			/* gp reg mask */
		lput(0L);
		lput(0L);
		lput(0L);
		lput(0L);
		lput(~0L);			/* gp value ?? */
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
		lput(0x407);			/* magic */
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
		lput(0x05L);			/* protections = RX */
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

			case SSTRING:
				putsymb(s->name, 'T', s->value, s->version);
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
				putsymb(a->asym->name, 'z', a->aoffset, 0);
			else
			if(a->type == D_FILE1)
				putsymb(a->asym->name, 'Z', a->aoffset, 0);

		if(s->type == STEXT)
			putsymb(s->name, 'T', s->value, s->version);
		else
			putsymb(s->name, 'L', s->value, s->version);

		/* frame, auto and param after */
		putsymb(".frame", 'm', p->to.offset+4, 0);
		for(a=p->to.autom; a; a=a->link)
			if(a->type == D_AUTO)
				putsymb(a->asym->name, 'a', -a->aoffset, 0);
			else
			if(a->type == D_PARAM)
				putsymb(a->asym->name, 'p', a->aoffset, 0);
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
datblk(long s, long n, int str)
{
	Prog *p;
	char *cast;
	long l, fl, j, d;
	int i, c;

	memset(buf.dbuf, 0, n+100);
	for(p = datap; p != P; p = p->link) {
		curp = p;
		if(str != (p->from.sym->type == SSTRING))
			continue;
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
					diag("multiple initialization");
					break;
				}
		}
		switch(p->to.type) {
		default:
			diag("unknown mode in initialization\n%P", p);
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
				switch(p->to.sym->type) {
				case STEXT:
				case SLEAF:
				case SSTRING:
					d += p->to.sym->value;
					break;
				case SDATA:
				case SBSS:
					d += p->to.sym->value + INITDAT;
					break;
				}
			}
			cast = (char*)&d;
			switch(c) {
			default:
				diag("bad nuxi %d %d\n%P", c, i, curp);
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

#define	OP_RRR(op,r1,r2,r3)\
	(op|(((r1)&31L)<<16)|(((r2)&31L)<<21)|(((r3)&31L)<<11))
#define	OP_IRR(op,i,r2,r3)\
	(op|((i)&0xffffL)|(((r2)&31L)<<21)|(((r3)&31L)<<16))
#define	OP_SRR(op,s,r2,r3)\
	(op|(((s)&31L)<<6)|(((r2)&31L)<<16)|(((r3)&31L)<<11))
#define	OP_FRRR(op,r1,r2,r3)\
	(op|(((r1)&31L)<<16)|(((r2)&31L)<<11)|(((r3)&31L)<<6))
#define	OP_JMP(op,i)\
		((op)|((i)&0x3ffffffL))

#define	OP(x,y)\
	(((x)<<3)|((y)<<0))
#define	SP(x,y)\
	(((x)<<29)|((y)<<26))
#define	BCOND(x,y)\
	(((x)<<19)|((y)<<16))
#define	MMU(x,y)\
	(SP(2,0)|(16<<21)|((x)<<3)|((y)<<0))
#define	FPF(x,y)\
	(SP(2,1)|(16<<21)|((x)<<3)|((y)<<0))
#define	FPD(x,y)\
	(SP(2,1)|(17<<21)|((x)<<3)|((y)<<0))
#define	FPW(x,y)\
	(SP(2,1)|(20<<21)|((x)<<3)|((y)<<0))

int vshift(int);

int
asmout(Prog *p, Optab *o, int aflag)
{
	long o1, o2, o3, o4, o5, o6, o7, v;
	Prog *ct;
	int r, a;

	o1 = 0;
	o2 = 0;
	o3 = 0;
	o4 = 0;
	o5 = 0;
	o6 = 0;
	o7 = 0;
	switch(o->type) {
	default:
		diag("unknown type %d", o->type);
		if(!debug['a'])
			prasm(p);
		break;

	case 0:		/* pseudo ops */
		if(aflag) {
			if(p->link) {
				if(p->as == ATEXT) {
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

	case 1:		/* mov[v] r1,r2 ==> OR r1,r0,r2 */
		o1 = OP_RRR(oprrr(AOR), p->from.reg, REGZERO, p->to.reg);
		break;

	case 2:		/* add/sub r1,[r2],r3 */
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		o1 = OP_RRR(oprrr(p->as), p->from.reg, r, p->to.reg);
		break;

	case 3:		/* mov $soreg, r ==> or/add $i,o,r */
		v = regoff(&p->from);
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		a = AADDU;
		if(o->a1 == C_ANDCON)
			a = AOR;
		o1 = OP_IRR(opirr(a), v, r, p->to.reg);
		break;

	case 4:		/* add $scon,[r1],r2 */
		v = regoff(&p->from);
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		o1 = OP_IRR(opirr(p->as), v, r, p->to.reg);
		break;

	case 5:		/* syscall */
		if(aflag)
			return 0;
		o1 = oprrr(p->as);
		break;

	case 6:		/* beq r1,[r2],sbra */
		if(aflag)
			return 0;
		if(p->cond == P)
			v = -4 >> 2;
		else
			v = (p->cond->pc - pc-4) >> 2;
		if(((v << 16) >> 16) != v)
			diag("short branch too far: %d\n%P", v, p);
		o1 = OP_IRR(opirr(p->as), v, p->from.reg, p->reg);
		break;

	case 7:		/* mov r, soreg ==> sw o(r) */
		r = p->to.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->to);
		o1 = OP_IRR(opirr(p->as), v, r, p->from.reg);
		break;

	case 8:		/* mov soreg, r ==> lw o(r) */
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->from);
		o1 = OP_IRR(opirr(p->as+ALAST), v, r, p->to.reg);
		break;

	case 9:		/* asl r1,[r2],r3 */
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		o1 = OP_RRR(oprrr(p->as), r, p->from.reg, p->to.reg);
		break;

	case 10:	/* add $con,[r1],r2 ==> mov $con,t; add t,[r1],r2 */
		v = regoff(&p->from);
		r = AOR;
		if(v < 0)
			r = AADDU;
		o1 = OP_IRR(opirr(r), v, 0, REGTMP);
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		o2 = OP_RRR(oprrr(p->as), REGTMP, r, p->to.reg);
		break;

	case 11:	/* jmp lbra */
		if(aflag)
			return 0;
		if(p->cond == P)
			v = p->pc >> 2;
		else
			v = p->cond->pc >> 2;
		o1 = OP_JMP(opirr(p->as), v);
		if(!debug['Y'] && p->link && p->cond && isnop(p->link)) {
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

	case 12:	/* movbs r,r */
		v = 16;
		if(p->as == AMOVB)
			v = 24;
		o1 = OP_SRR(opirr(ASLL), v, p->from.reg, p->to.reg);
		o2 = OP_SRR(opirr(ASRA), v, p->to.reg, p->to.reg);
		break;

	case 13:	/* movbu r,r */
		if(p->as == AMOVBU)
			o1 = OP_IRR(opirr(AAND), 0xffL, p->from.reg, p->to.reg);
		else
			o1 = OP_IRR(opirr(AAND), 0xffffL, p->from.reg, p->to.reg);
		break;

	case 16:	/* sll $c,[r1],r2 */
		v = regoff(&p->from);
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;

		/* OP_SRR will use only the low 5 bits of the shift value */
		if(v >= 32 && vshift(p->as))
			o1 = OP_SRR(opirr(p->as+ALAST), v-32, r, p->to.reg);
		else 
			o1 = OP_SRR(opirr(p->as), v, r, p->to.reg);
		break;

	case 18:	/* jmp [r1],0(r2) */
		if(aflag)
			return 0;
		r = p->reg;
		if(r == NREG)
			r = o->param;
		o1 = OP_RRR(oprrr(p->as), 0, p->to.reg, r);
		break;

	case 19:	/* mov $lcon,r ==> lu+or */
		v = regoff(&p->from);
		o1 = OP_IRR(opirr(ALAST), v>>16, REGZERO, p->to.reg);
		o2 = OP_IRR(opirr(AOR), v, p->to.reg, p->to.reg);
		break;

	case 20:	/* mov lohi,r */
		r = OP(2,0);		/* mfhi */
		if(p->from.type == D_LO)
			r = OP(2,2);	/* mflo */
		o1 = OP_RRR(r, REGZERO, REGZERO, p->to.reg);
		break;

	case 21:	/* mov r,lohi */
		r = OP(2,1);		/* mthi */
		if(p->to.type == D_LO)
			r = OP(2,3);	/* mtlo */
		o1 = OP_RRR(r, REGZERO, p->from.reg, REGZERO);
		break;

	case 22:	/* mul r1,r2 */
		o1 = OP_RRR(oprrr(p->as), p->from.reg, p->reg, REGZERO);
		break;

	case 23:	/* add $lcon,r1,r2 ==> lu+or+add */
		v = regoff(&p->from);
		if(p->to.reg == REGTMP || p->reg == REGTMP)
			diag("cant synthesize large constant\n%P", p);
		o1 = OP_IRR(opirr(ALAST), v>>16, REGZERO, REGTMP);
		o2 = OP_IRR(opirr(AOR), v, REGTMP, REGTMP);
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		o3 = OP_RRR(oprrr(p->as), REGTMP, r, p->to.reg);
		break;

	case 24:	/* mov $ucon,,r ==> lu r */
		v = regoff(&p->from);
		o1 = OP_IRR(opirr(ALAST), v>>16, REGZERO, p->to.reg);
		break;

	case 25:	/* add/and $ucon,[r1],r2 ==> lu $con,t; add t,[r1],r2 */
		v = regoff(&p->from);
		o1 = OP_IRR(opirr(ALAST), v>>16, REGZERO, REGTMP);
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		o2 = OP_RRR(oprrr(p->as), REGTMP, r, p->to.reg);
		break;

	case 26:	/* mov $lsext/auto/oreg,,r2 ==> lu+or+add */
		v = regoff(&p->from);
		if(p->to.reg == REGTMP)
			diag("cant synthesize large constant\n%P", p);
		o1 = OP_IRR(opirr(ALAST), v>>16, REGZERO, REGTMP);
		o2 = OP_IRR(opirr(AOR), v, REGTMP, REGTMP);
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		o3 = OP_RRR(oprrr(AADDU), REGTMP, r, p->to.reg);
		break;

	case 27:		/* mov [sl]ext/auto/oreg,fr ==> lwc1 o(r) */
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->from);
		switch(o->size) {
		case 20:
			o1 = OP_IRR(opirr(ALAST), v>>16, REGZERO, REGTMP);
			o2 = OP_IRR(opirr(AOR), v, REGTMP, REGTMP);
			o3 = OP_RRR(oprrr(AADDU), r, REGTMP, REGTMP);
			o4 = OP_IRR(opirr(AMOVF+ALAST), 0, REGTMP, p->to.reg+1);
			o5 = OP_IRR(opirr(AMOVF+ALAST), 4, REGTMP, p->to.reg);
			break;
		case 16:
			o1 = OP_IRR(opirr(ALAST), v>>16, REGZERO, REGTMP);
			o2 = OP_IRR(opirr(AOR), v, REGTMP, REGTMP);
			o3 = OP_RRR(oprrr(AADDU), r, REGTMP, REGTMP);
			o4 = OP_IRR(opirr(AMOVF+ALAST), 0, REGTMP, p->to.reg);
			break;
		case 8:
			o1 = OP_IRR(opirr(AMOVF+ALAST), v, r, p->to.reg+1);
			o2 = OP_IRR(opirr(AMOVF+ALAST), v+4, r, p->to.reg);
			break;
		case 4:
			o1 = OP_IRR(opirr(AMOVF+ALAST), v, r, p->to.reg);
			break;
		}
		break;

	case 28:		/* mov fr,[sl]ext/auto/oreg ==> swc1 o(r) */
		r = p->to.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->to);
		switch(o->size) {
		case 20:
			if(r == REGTMP)
				diag("cant synthesize large constant\n%P", p);
			o1 = OP_IRR(opirr(ALAST), v>>16, REGZERO, REGTMP);
			o2 = OP_IRR(opirr(AOR), v, REGTMP, REGTMP);
			o3 = OP_RRR(oprrr(AADDU), r, REGTMP, REGTMP);
			o4 = OP_IRR(opirr(AMOVF), 0, REGTMP, p->from.reg+1);
			o5 = OP_IRR(opirr(AMOVF), 4, REGTMP, p->from.reg);
			break;
		case 16:
			if(r == REGTMP)
				diag("cant synthesize large constant\n%P", p);
			o1 = OP_IRR(opirr(ALAST), v>>16, REGZERO, REGTMP);
			o2 = OP_IRR(opirr(AOR), v, REGTMP, REGTMP);
			o3 = OP_RRR(oprrr(AADDU), r, REGTMP, REGTMP);
			o4 = OP_IRR(opirr(AMOVF), 0, REGTMP, p->from.reg);
			break;
		case 8:
			o1 = OP_IRR(opirr(AMOVF), v, r, p->from.reg+1);
			o2 = OP_IRR(opirr(AMOVF), v+4, r, p->from.reg);
			break;
		case 4:
			o1 = OP_IRR(opirr(AMOVF), v, r, p->from.reg);
			break;
		}
		break;

	case 30:	/* movw r,fr */
		r = SP(2,1)|(4<<21);		/* mtc1 */
		o1 = OP_RRR(r, p->from.reg, 0, p->to.reg);
		break;

	case 31:	/* movw fr,r */
		r = SP(2,1)|(0<<21);		/* mfc1 */
		o1 = OP_RRR(r, p->to.reg, 0, p->from.reg);
		break;

	case 32:	/* fadd fr1,[fr2],fr3 */
		r = p->reg;
		if(r == NREG)
			o1 = OP_FRRR(oprrr(p->as), p->from.reg, p->to.reg, p->to.reg);
		else
			o1 = OP_FRRR(oprrr(p->as), p->from.reg, r, p->to.reg);
		break;

	case 33:	/* fabs fr1,fr3 */
		o1 = OP_FRRR(oprrr(p->as), 0, p->from.reg, p->to.reg);
		break;

	case 34:	/* mov $con,fr ==> or/add $i,r,r2 */
		v = regoff(&p->from);
		r = AADDU;
		if(o->a1 == C_ANDCON)
			r = AOR;
		o1 = OP_IRR(opirr(r), v, 0, REGTMP);
		o2 = OP_RRR(SP(2,1)|(4<<21), REGTMP, 0, p->to.reg);	/* mtc1 */
		break;

	case 35:	/* mov r,lext/luto/oreg ==> sw o(r) */
		/*
		 * the lowbits of the constant cannot
		 * be moved into the offset of the load
		 * because the mips 4000 in 64-bit mode
		 * does a 64-bit add and it will screw up.
		 */
		v = regoff(&p->to);
		r = p->to.reg;
		if(r == NREG)
			r = o->param;
		if(r == REGTMP)
			diag("cant synthesize large constant\n%P", p);
		o1 = OP_IRR(opirr(ALAST), v>>16, REGZERO, REGTMP);
		o2 = OP_IRR(opirr(AOR), v, REGTMP, REGTMP);
		o3 = OP_RRR(oprrr(AADDU), r, REGTMP, REGTMP);
		o4 = OP_IRR(opirr(p->as), 0, REGTMP, p->from.reg);
		break;

	case 36:	/* mov lext/lauto/lreg,r ==> lw o(r30) */
		v = regoff(&p->from);
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		if(r == REGTMP)
			diag("cant synthesize large constant\n%P", p);
		o1 = OP_IRR(opirr(ALAST), v>>16, REGZERO, REGTMP);
		o2 = OP_IRR(opirr(AOR), v, REGTMP, REGTMP);
		o3 = OP_RRR(oprrr(AADDU), r, REGTMP, REGTMP);
		o4 = OP_IRR(opirr(p->as+ALAST), 0, REGTMP, p->to.reg);
		break;

	case 37:	/* movw r,mr */
		r = SP(2,0)|(4<<21);		/* mtc0 */
		if(p->as == AMOVV)
			r = SP(2,0)|(5<<21);	/* dmtc0 */
		o1 = OP_RRR(r, p->from.reg, 0, p->to.reg);
		break;

	case 38:	/* movw mr,r */
		r = SP(2,0)|(0<<21);		/* mfc0 */
		if(p->as == AMOVV)
			r = SP(2,0)|(1<<21);	/* dmfc0 */
		o1 = OP_RRR(r, p->to.reg, 0, p->from.reg);
		break;

	case 39:	/* rfe ==> jmp+rfe */
		if(aflag)
			return 0;
		o1 = OP_RRR(oprrr(AJMP), 0, p->to.reg, REGZERO);
		o2 = oprrr(p->as);
		break;

	case 40:	/* word */
		if(aflag)
			return 0;
		o1 = regoff(&p->to);
		break;

	case 41:	/* movw r,fcr */
		o1 = OP_RRR(SP(2,1)|(2<<21), REGZERO, 0, p->to.reg); 	/* mfcc1 */
		o2 = OP_RRR(SP(2,1)|(6<<21), p->from.reg, 0, p->to.reg);/* mtcc1 */
		break;

	case 42:	/* movw fcr,r */
		o1 = OP_RRR(SP(2,1)|(2<<21), p->to.reg, 0, p->from.reg);/* mfcc1 */
		break;

	case 45:	/* case r */
		if(p->link == P)
			v = p->pc+28;
		else
			v = p->link->pc;
		if(v & (1<<15))
			o1 = OP_IRR(opirr(ALAST), (v>>16)+1, REGZERO, REGTMP);
		else
			o1 = OP_IRR(opirr(ALAST), v>>16, REGZERO, REGTMP);
		o2 = OP_SRR(opirr(ASLL), 2, p->from.reg, p->from.reg);
		o3 = OP_RRR(oprrr(AADD), p->from.reg, REGTMP, REGTMP);
		o4 = OP_IRR(opirr(AMOVW+ALAST), v, REGTMP, REGTMP);
		o5 = OP_RRR(oprrr(ANOR), REGZERO, REGZERO, REGZERO);
		o6 = OP_RRR(oprrr(AJMP), 0, REGTMP, REGZERO);
		o7 = OP_RRR(oprrr(ANOR), REGZERO, REGZERO, REGZERO);
		break;

	case 46:	/* bcase $con,lbra */
		if(p->cond == P)
			v = p->pc;
		else
			v = p->cond->pc;
		o1 = v;
		break;
	}
	if(aflag)
		return o1;
	v = p->pc;
	switch(o->size) {
	default:
		if(debug['a'])
			Bprint(&bso, " %.8lux:\t\t%P\n", v, p);
		break;
	case 4:
		if(debug['a'])
			Bprint(&bso, " %.8lux: %.8lux\t%P\n", v, o1, p);
		LPUT(o1);
		break;
	case 8:
		if(debug['a'])
			Bprint(&bso, " %.8lux: %.8lux %.8lux%P\n", v, o1, o2, p);
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

	case 28:
		if(debug['a'])
			Bprint(&bso, " %.8lux: %.8lux %.8lux %.8lux %.8lux %.8lux %.8lux %.8lux%P\n",
				v, o1, o2, o3, o4, o5, o6, o7, p);
		LPUT(o1);
		LPUT(o2);
		LPUT(o3);
		LPUT(o4);
		LPUT(o5);
		LPUT(o6);
		LPUT(o7);
		break;
	}
	return 0;
}

int
isnop(Prog *p)
{
	if(p->as != ANOR)
		return 0;
	if(p->reg != REGZERO && p->reg != NREG)
		return 0;
	if(p->from.type != D_REG || p->from.reg != REGZERO)
		return 0;
	if(p->to.type != D_REG || p->to.reg != REGZERO)
		return 0;
	return 1;
}

long
oprrr(int a)
{
	switch(a) {
	case AADD:	return OP(4,0);
	case AADDU:	return OP(4,1);
	case ASGT:	return OP(5,2);
	case ASGTU:	return OP(5,3);
	case AAND:	return OP(4,4);
	case AOR:	return OP(4,5);
	case AXOR:	return OP(4,6);
	case ASUB:	return OP(4,2);
	case ASUBU:	return OP(4,3);
	case ANOR:	return OP(4,7);
	case ASLL:	return OP(0,4);
	case ASRL:	return OP(0,6);
	case ASRA:	return OP(0,7);

	case AREM:
	case ADIV:	return OP(3,2);
	case AREMU:
	case ADIVU:	return OP(3,3);
	case AMUL:	return OP(3,0);
	case AMULU:	return OP(3,1);

	case AJMP:	return OP(1,0);
	case AJAL:	return OP(1,1);

	case ABREAK:	return OP(1,5);
	case ASYSCALL:	return OP(1,4);
	case ATLBP:	return MMU(1,0);
	case ATLBR:	return MMU(0,1);
	case ATLBWI:	return MMU(0,2);
	case ATLBWR:	return MMU(0,6);
	case ARFE:	return MMU(2,0);

	case ADIVF:	return FPF(0,3);
	case ADIVD:	return FPD(0,3);
	case AMULF:	return FPF(0,2);
	case AMULD:	return FPD(0,2);
	case ASUBF:	return FPF(0,1);
	case ASUBD:	return FPD(0,1);
	case AADDF:	return FPF(0,0);
	case AADDD:	return FPD(0,0);

	case AMOVFW:	return FPF(4,4);
	case AMOVDW:	return FPD(4,4);
	case AMOVWF:	return FPW(4,0);
	case AMOVDF:	return FPD(4,0);
	case AMOVWD:	return FPW(4,1);
	case AMOVFD:	return FPF(4,1);
	case AABSF:	return FPF(0,5);
	case AABSD:	return FPD(0,5);
	case AMOVF:	return FPF(0,6);
	case AMOVD:	return FPD(0,6);
	case ANEGF:	return FPF(0,7);
	case ANEGD:	return FPD(0,7);

	case ACMPEQF:	return FPF(6,2);
	case ACMPEQD:	return FPD(6,2);
	case ACMPGTF:	return FPF(7,4);
	case ACMPGTD:	return FPD(7,4);
	case ACMPGEF:	return FPF(7,6);
	case ACMPGED:	return FPD(7,6);

	case ADIVV:	return OP(3,6);
	case ADIVVU:	return OP(3,7);
	case AADDV:	return OP(5,4);
	case AADDVU:	return OP(5,5);
	}
	diag("bad rrr %d", a);
	return 0;
}

long
opirr(int a)
{
	switch(a) {
	case AADD:	return SP(1,0);
	case AADDU:	return SP(1,1);
	case ASGT:	return SP(1,2);
	case ASGTU:	return SP(1,3);
	case AAND:	return SP(1,4);
	case AOR:	return SP(1,5);
	case AXOR:	return SP(1,6);
	case ALAST:	return SP(1,7);
	case ASLL:	return OP(0,0);
	case ASRL:	return OP(0,2);
	case ASRA:	return OP(0,3);

	case AJMP:	return SP(0,2);
	case AJAL:	return SP(0,3);
	case ABEQ:	return SP(0,4);
	case ABNE:	return SP(0,5);

	case ABGEZ:	return SP(0,1)|BCOND(0,1);
	case ABGEZAL:	return SP(0,1)|BCOND(2,1);
	case ABGTZ:	return SP(0,7);
	case ABLEZ:	return SP(0,6);
	case ABLTZ:	return SP(0,1)|BCOND(0,0);
	case ABLTZAL:	return SP(0,1)|BCOND(2,0);

	case ABFPT:	return SP(2,1)|(257<<16);
	case ABFPF:	return SP(2,1)|(256<<16);

	case AMOVB:
	case AMOVBU:	return SP(5,0);
	case AMOVH:
	case AMOVHU:	return SP(5,1);
	case AMOVW:	return SP(5,3);
	case AMOVV:	return SP(7,7);
	case AMOVF:	return SP(7,1);
	case AMOVWL:	return SP(5,2);
	case AMOVWR:	return SP(5,6);
	case AMOVVL:	return SP(5,4);
	case AMOVVR:	return SP(5,5);

	case ABREAK:	return SP(5,7);

	case AMOVWL+ALAST:	return SP(4,2);
	case AMOVWR+ALAST:	return SP(4,6);
	case AMOVVL+ALAST:	return SP(3,2);
	case AMOVVR+ALAST:	return SP(3,3);
	case AMOVB+ALAST:	return SP(4,0);
	case AMOVBU+ALAST:	return SP(4,4);
	case AMOVH+ALAST:	return SP(4,1);
	case AMOVHU+ALAST:	return SP(4,5);
	case AMOVW+ALAST:	return SP(4,3);
	case AMOVV+ALAST:	return SP(6,7);
	case AMOVF+ALAST:	return SP(6,1);

	case ASLLV:		return OP(7,0);
	case ASRLV:		return OP(7,2);
	case ASRAV:		return OP(7,3);
	case ASLLV+ALAST:	return OP(7,4);
	case ASRLV+ALAST:	return OP(7,6);
	case ASRAV+ALAST:	return OP(7,7);

	case AADDV:		return SP(3,0);
	case AADDVU:		return SP(3,1);
	}
	diag("bad irr %d", a);
abort();
	return 0;
}

int
vshift(int a)
{
	switch(a){
	case ASLLV:		return 1;
	case ASRLV:		return 1;
	case ASRAV:		return 1;
	}
	return 0;
}
