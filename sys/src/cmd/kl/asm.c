#include	"l.h"

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
	seek(cout, HEADR, 0);
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

	curtext = P;
	switch(HEADTYPE) {
	case 0:
	case 3:
		seek(cout, HEADR+textsize, 0);
		break;
	case 1:
	case 2:
		seek(cout, HEADR+textsize, 0);
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
		case 0:
		case 3:
			seek(cout, HEADR+textsize+datsize, 0);
			break;
		case 2:
		case 1:
			seek(cout, HEADR+textsize+datsize, 0);
			break;
		}
		if(!debug['s'])
			asmsym();
		if(debug['v'])
			Bprint(&bso, "%5.2f sp\n", cputime());
		Bflush(&bso);
		if(!debug['s'])
			asmlc();
		 /* round up file length for boot image */
		if(HEADTYPE == 0 || HEADTYPE == 3)
			if((symsize+lcsize) & 1)
				CPUT(0);
		cflush();
	}

	seek(cout, 0L, 0);
	switch(HEADTYPE) {
	case 0:
		lput(0x1030107);		/* magic and sections */
		lput(textsize);			/* sizes */
		lput(datsize);
		lput(bsssize);
		lput(symsize);			/* nsyms */
		lput(entryvalue());		/* va of entry */
		lput(0L);
		lput(lcsize);
		break;
	case 1:
		break;
	case 2:
		lput(4*13*13+7);		/* magic */
		lput(textsize);			/* sizes */
		lput(datsize);
		lput(bsssize);
		lput(symsize);			/* nsyms */
		lput(entryvalue());		/* va of entry */
		lput(0L);
		lput(lcsize);
		break;
	case 3:
		lput(0x1030107);		/* magic and sections */
		lput(0x90100000);
#define SPARC_NOOP 0x01000000
		lput(SPARC_NOOP);
		lput(SPARC_NOOP);
		lput(SPARC_NOOP);
		lput(SPARC_NOOP);
		lput(SPARC_NOOP);
		lput(SPARC_NOOP);
		break;
	}
	cflush();
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
				fl = ieeedtof(&p->to.ieee);
				cast = (char*)&fl;
				for(; i<c; i++) {
					buf.dbuf[l] = cast[fnuxi8[i+4]];
					l++;
				}
				break;
			case 8:
				cast = (char*)&p->to.ieee;
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

#define	OP2(x)	(0x80000000|((x)<<19))
#define	OP3(x)	(0xc0000000|((x)<<19))
#define	OPB(x)	(0x00800000|((x)<<25))
#define	OPT(x)	(0x81d02000|((x)<<25))
#define	OPF1(x)	(0x81a00000|((x)<<5))
#define	OPF2(x)	(0x81a80000|((x)<<5))
#define	OPFB(x)	(0x01800000|((x)<<25))

#define	OP_RRR(op,r1,r2,r3)\
	(0x00000000 | op |\
	(((r1)&31L)<<0) |\
	(((r2)&31L)<<14) |\
	(((r3)&31L)<<25))
#define	OP_IRR(op,i,r2,r3)\
	(0x00002000L | (op) |\
	(((i)&0x1fffL)<<0) |\
	(((r2)&31L)<<14) |\
	(((r3)&31L)<<25))
#define	OP_BRA(op,pc)\
	((op) |\
	(((pc)&0x3fffff)<<0))

int
asmout(Prog *p, Optab *o, int aflag)
{
	long o1, o2, o3, o4, o5, v;
	Prog *ct;
	int r;

	o1 = 0;
	o2 = 0;
	o3 = 0;
	o4 = 0;
	o5 = 0;

	switch(o->type) {
	default:
		if(aflag)
			return 0;
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

	case 1:		/* mov r1,r2 ==> OR r1,r0,r2 */
		o1 = OP_RRR(opcode(AOR), p->from.reg, REGZERO, p->to.reg);
		break;

	case 2:		/* mov $c,r ==> add r1,r0,r2 */
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->from);
		o1 = OP_IRR(opcode(AADD), v, r, p->to.reg);
		break;

	case 3:		/* mov soreg, r */
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->from);
		if(v == 0 && p->reg != NREG)
			o1 = OP_RRR(opcode(p->as), p->reg, r, p->to.reg);
		else
			o1 = OP_IRR(opcode(p->as), v, r, p->to.reg);
		break;

	case 4:		/* mov r, soreg */
		r = p->to.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->to);
		if(v == 0 && p->reg != NREG)
			o1 = OP_RRR(opcode(p->as+AEND), p->reg, r, p->from.reg);
		else
			o1 = OP_IRR(opcode(p->as+AEND), v, r, p->from.reg);
		break;

	case 5:		/* mov $lcon, reg => sethi, add */
		v = regoff(&p->from);
		o1 = 0x1000000 | (REGTMP<<25) | ((v>>10) & 0x3fffff);	/* sethi */
		o2 = OP_IRR(opcode(AADD), (v&0x3ff), REGTMP, p->to.reg);
		break;

	case 6:		/* mov asi, r[+r] */
		o1 = OP_RRR(opcode(p->as), p->reg, p->from.reg, p->to.reg);
		o1 |= (1<<23) | ((p->from.offset&0xff)<<5);
		break;

	case 7:		/* mov [+r]r, asi */
		o1 = OP_RRR(opcode(p->as+AEND), p->reg, p->to.reg, p->from.reg);
		o1 |= (1<<23) | ((p->to.offset&0xff)<<5);
		break;

	case 8:		/* mov r, preg and mov preg, r */
		if(p->to.type == D_PREG) {
			r = p->from.reg;
			switch(p->to.reg)
			{
			default:
				diag("unknown register P%d", p->to.reg);
			case D_Y:
				o1 = OP2(48);	/* wry */
				break;
			case D_PSR:
				o1 = OP2(49);	/* wrpsr */
				break;
			case D_WIM:
				o1 = OP2(50);	/* wrwim */
				break;
			case D_TBR:
				o1 = OP2(51);	/* wrtbr */
				break;
			}
			o1 = OP_IRR(o1, 0, r, 0);
			break;
		}
		if(p->from.type == D_PREG) {
			r = p->to.reg;
			switch(p->from.reg)
			{
			default:
				diag("unknown register P%d", p->to.reg);
			case D_Y:
				o1 = OP2(40);	/* rdy */
				break;
			case D_PSR:
				o1 = OP2(41);	/* rdpsr */
				break;
			case D_WIM:
				o1 = OP2(42);	/* rdwim */
				break;
			case D_TBR:
				o1 = OP2(43);	/* rdtbr */
				break;
			}
			o1 = OP_RRR(o1, 0, 0, r);
			break;
		}
		break;

	case 9:		/* movb r,r */
		v = 24;
		if(p->as == AMOVH || p->as == AMOVHU)
			v = 16;
		r = ASRA;
		if(p->as == AMOVBU || p->as == AMOVHU)
			r = ASRL;
		o1 = OP_IRR(opcode(ASLL), v, p->from.reg, p->to.reg);
		o2 = OP_IRR(opcode(r), v, p->to.reg, p->to.reg);
		break;

	case 10:	/* mov $loreg, reg */
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->from);
		o1 = 0x1000000 | (REGTMP<<25) | ((v>>10) & 0x3fffff);	/* sethi */
		o2 = OP_RRR(opcode(AADD), r, REGTMP, REGTMP);
		o3 = OP_IRR(opcode(AADD), (v&0x3ff), REGTMP, p->to.reg);
		break;

	case 11:	/* mov loreg, r */
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->from);
		o1 = 0x1000000 | (REGTMP<<25) | ((v>>10) & 0x3fffff);	/* sethi */
		o2 = OP_RRR(opcode(AADD), r, REGTMP, REGTMP);
		o3 = OP_IRR(opcode(p->as), (v&0x3ff), REGTMP, p->to.reg);
		break;

	case 12:	/* mov r, loreg */
		r = p->to.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->to);
		o1 = 0x1000000 | (REGTMP<<25) | ((v>>10) & 0x3fffff);	/* sethi */
		o2 = OP_RRR(opcode(AADD), r, REGTMP, REGTMP);
		o3 = OP_IRR(opcode(p->as+AEND), (v&0x3ff), REGTMP, p->from.reg);
		break;

	case 13:	/* mov $ucon, r */
		v = regoff(&p->from);
		o1 = 0x1000000 | (p->to.reg<<25) | ((v>>10) & 0x3fffff);	/* sethi */
		break;

	case 20:	/* op $scon,r */
		v = regoff(&p->from);
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		o1 = OP_IRR(opcode(p->as), v, r, p->to.reg);
		break;

	case 21:	/* op r1,r2 */
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		o1 = OP_RRR(opcode(p->as), p->from.reg, r, p->to.reg);
		break;

	case 22:	/* op $lcon,r */
		v = regoff(&p->from);
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		o1 = 0x1000000 | (REGTMP<<25) | ((v>>10) & 0x3fffff);	/* sethi */
		o2 = OP_IRR(opcode(AADD), (v&0x3ff), REGTMP, REGTMP);
		o3 = OP_RRR(opcode(p->as), REGTMP, r, p->to.reg);
		break;

	case 23:	/* cmp r,r */
		o1 = OP_RRR(opcode(ASUBCC), p->to.reg, p->from.reg, REGZERO);
		break;

	case 24:	/* cmp r,$c */
		v = regoff(&p->to);
		o1 = OP_IRR(opcode(ASUBCC), v, p->from.reg, REGZERO);
		break;

	case 25:	/* cmp $c,r BOTCH, fix compiler */
		v = regoff(&p->from);
		o1 = OP_IRR(opcode(AADD), v, NREG, REGTMP);
		o2 = OP_RRR(opcode(ASUBCC), p->to.reg, REGTMP, REGZERO);
		break;

	case 26:	/* op $ucon,r */
		v = regoff(&p->from);
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		o1 = 0x1000000 | (REGTMP<<25) | ((v>>10) & 0x3fffff);	/* sethi */
		o2 = OP_RRR(opcode(p->as), REGTMP, r, p->to.reg);
		break;

	case 30:	/* jmp/jmpl soreg */
		if(aflag)
			return 0;
		v = regoff(&p->to);
		r = p->reg;
		if(r == NREG && p->as == AJMPL)
			r = 15;
		o1 = OP_IRR(opcode(AJMPL), v, p->to.reg, r);
		break;

	case 31:	/* ba jmp */
		if(aflag)
			return 0;
		r = p->as;
		if(r == AJMP)
			r = ABA;
		v = 0;
		if(p->cond)
			v = p->cond->pc - p->pc;
		o1 = OP_BRA(opcode(r), v/4);
		if(r == ABA && p->link && p->cond && isnop(p->link)) {
			o2 = asmout(p->cond, oplook(p->cond), 1);
			if(o2) {
				o1 += 1;
				if(debug['a'])
					Bprint(&bso, " %.8lux: %.8lux %.8lux%P\n", p->pc, o1, o2, p);
				LPUT(o1);
				LPUT(o2);
				return 1;
			}
			/* cant set annul here because pc has already been counted */
		}
		break;

	case 32:	/* jmpl lbra */
		if(aflag)
			return 0;
		v = 0;
		if(p->cond)
			v = p->cond->pc - p->pc;
		r = p->reg;
		if(r != NREG && r != 15)
			diag("cant jmpl other than R15");
		o1 = 0x40000000 | ((v/4) & 0x3fffffffL);	/* call */
		if(p->link && p->cond && isnop(p->link)) {
			o2 = asmout(p->cond, oplook(p->cond), 1);
			if(o2) {
				o1 += 1;
				if(debug['a'])
					Bprint(&bso, " %.8lux: %.8lux %.8lux%P\n", p->pc, o1, o2, p);
				LPUT(o1);
				LPUT(o2);
				return 1;
			}
		}
		break;

	case 33:	/* trap r */
		if(aflag)
			return 0;
		o1 = opcode(p->as) | (p->from.reg<<14);
		break;

	case 34:	/* rett r1,r2 -> jmpl (r1); rett (r2) */
		if(aflag)
			return 0;
		o1 = OP_IRR(opcode(AJMPL), 0, p->from.reg, REGZERO);
		o2 = OP_IRR(opcode(ARETT), 0, p->to.reg, REGZERO);
		break;

	case 40:	/* ldfsr, stfsr, stdq */
		if(p->to.type == D_PREG) {
			r = p->from.reg;
			if(r == NREG)
				r = o->param;
			v = regoff(&p->from);
			if(p->to.reg == D_FSR) {
				o1 = OP_IRR(OP3(33), v, r, 0);
				break;
			}
			diag("unknown reg load %d", p->to.reg);
		} else {
			r = p->to.reg;
			if(r == NREG)
				r = o->param;
			v = regoff(&p->to);
			if(p->from.reg == D_FSR) {
				o1 = OP_IRR(OP3(37), v, r, 0);
				break;
			}
			if(p->as == AMOVD && p->from.reg == D_FPQ) {
				o1 = OP_IRR(OP3(38), v, r, 0);
				break;
			}
			diag("unknown reg store %d", p->from.reg);
		}
		break;

	case 41:	/* ldf,ldd */
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->from);
		if(p->as == AFMOVF || p->as == AMOVW) {
			o1 = OP_IRR(OP3(32), v, r, p->to.reg);
			break;
		}
		if(p->as == AMOVD || p->as == AFMOVD) {
			o1 = OP_IRR(OP3(35), v, r, p->to.reg);
			break;
		}
		diag("only MOVD and MOVW to FREG");
		break;

	case 42:	/* ldd -> ldf,ldf */
		/* note should be ldd with proper allignment */
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->from);
		o1 = OP_IRR(OP3(32), v, r, p->to.reg);
		o2 = OP_IRR(OP3(32), v+4, r, p->to.reg+1);
		break;

	case 43:	/* stf */
		r = p->to.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->to);
		if(p->as == AFMOVF || p->as == AMOVW) {
			o1 = OP_IRR(OP3(36), v, r, p->from.reg);
			break;
		}
		if(p->as == AMOVD || p->as == AFMOVD) {
			o1 = OP_IRR(OP3(39), v, r, p->from.reg);
			break;
		}
		diag("only MOVD and MOVW from FREG");
		break;

	case 44:	/* std -> stf,stf */
		/* note should be std with proper allignment */
		r = p->to.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->to);
		o1 = OP_IRR(OP3(36), v, r, p->from.reg);
		o2 = OP_IRR(OP3(36), v+4, r, p->from.reg+1);
		break;

	case 45:	/* ldf lorg */
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->from);
		o1 = 0x1000000 | (REGTMP<<25) | ((v>>10) & 0x3fffff);	/* sethi */
		o2 = OP_RRR(opcode(AADD), r, REGTMP, REGTMP);
		o3 = OP_IRR(OP3(32), v&0x3ff, REGTMP, p->to.reg);
		break;

	case 46:	/* ldd lorg -> ldf,ldf */
		/* note should be ldd with proper allignment */
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->from);
		o1 = 0x1000000 | (REGTMP<<25) | ((v>>10) & 0x3fffff);	/* sethi */
		o2 = OP_RRR(opcode(AADD), r, REGTMP, REGTMP);
		o3 = OP_IRR(OP3(32), (v&0x3ff), REGTMP, p->to.reg);
		o4 = OP_IRR(OP3(32), (v&0x3ff)+4, REGTMP, p->to.reg+1);
		break;

	case 47:	/* stf lorg */
		r = p->to.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->to);
		o1 = 0x1000000 | (REGTMP<<25) | ((v>>10) & 0x3fffff);	/* sethi */
		o2 = OP_RRR(opcode(AADD), r, REGTMP, REGTMP);
		o3 = OP_IRR(OP3(36), v&0x3ff, REGTMP, p->from.reg);
		break;

	case 48:	/* std lorg -> stf,stf */
		/* note should be std with proper allignment */
		r = p->to.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->to);
		o1 = 0x1000000 | (REGTMP<<25) | ((v>>10) & 0x3fffff);	/* sethi */
		o2 = OP_RRR(opcode(AADD), r, REGTMP, REGTMP);
		o3 = OP_IRR(OP3(36), (v&0x3ff), REGTMP, p->from.reg);
		o4 = OP_IRR(OP3(36), (v&0x3ff)+4, REGTMP, p->from.reg+1);
		break;

	case 49:	/* fmovd -> fmovf,fmovf */
		o1 = OP_RRR(opcode(AFMOVF), p->from.reg, 0, p->to.reg);
		o2 = OP_RRR(opcode(AFMOVF), p->from.reg+1, 0, p->to.reg+1);
		break;

	case 50:	/* fcmp */
		o1 = OP_RRR(opcode(p->as), p->to.reg, p->from.reg, 0);
		break;

	case 51:	/* word */
		if(aflag)
			return 0;
		o1 = regoff(&p->from);
		break;

	case 52:	/* div */
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		o1 = OP_IRR(opcode(ASRA), 31, r, REGTMP);
		o2 = OP_IRR(OP2(48), 0, REGTMP, 0);
		o3 = OP_RRR(opcode(ADIV), p->from.reg, r, p->to.reg);
		break;

	case 53:	/* divl */
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		o1 = OP_IRR(OP2(48), 0, REGZERO, 0);
		o2 = OP_RRR(opcode(ADIVL), p->from.reg, r, p->to.reg);
		break;

	case 54:	/* mod */
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		o1 = OP_IRR(opcode(ASRA), 31, r, REGTMP);
		o2 = OP_IRR(OP2(48), 0, REGTMP, 0);
		o3 = OP_RRR(opcode(ADIV), p->from.reg, r, REGTMP);
		o4 = OP_RRR(opcode(AMUL), p->from.reg, REGTMP, REGTMP);
		o5 = OP_RRR(opcode(ASUB), REGTMP, r, p->to.reg);
		break;

	case 55:	/* modl */
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		o1 = OP_IRR(OP2(48), 0, REGZERO, 0);
		o2 = OP_RRR(opcode(ADIVL), p->from.reg, r, REGTMP);
		o3 = OP_RRR(opcode(AMUL), p->from.reg, REGTMP, REGTMP);
		o4 = OP_RRR(opcode(ASUB), REGTMP, r, p->to.reg);
		break;

	case 56:	/* b(cc) -- annullable */
		if(aflag)
			return 0;
		r = p->as;
		v = 0;
		if(p->cond)
			v = p->cond->pc - p->pc;
		o1 = OP_BRA(opcode(r), v/4);
		if(p->link && p->cond && isnop(p->link))
		if(!debug['A']) {
			o2 = asmout(p->cond, oplook(p->cond), 2);
			if(o2) {
				o1 |= 1<<29;	/* annul */
				o1 += 1;
				if(debug['a'])
					Bprint(&bso, " %.8lux: %.8lux %.8lux%P\n", p->pc, o1, o2, p);
				LPUT(o1);
				LPUT(o2);
				return 1;
			}
		}
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
	}
	return 0;
}

int
isnop(Prog *p)
{
	if(p->as != AORN)
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
opcode(int a)
{
	switch(a) {
	case AADD:	return OP2(0);
	case AADDCC:	return OP2(16);
	case AADDX:	return OP2(8);
	case AADDXCC:	return OP2(24);

	case AMUL:	return OP2(10);
	case ADIV:	return OP2(15);
	case ADIVL:	return OP2(14);

	case ATADDCC:	return OP2(32);
	case ATADDCCTV:	return OP2(34);

	case ASUB:	return OP2(4);
	case ASUBCC:	return OP2(20);
	case ASUBX:	return OP2(12);
	case ASUBXCC:	return OP2(28);

	case ATSUBCC:	return OP2(33);
	case ATSUBCCTV:	return OP2(35);

	case AMULSCC:	return OP2(36);
	case ASAVE:	return OP2(60);
	case ARESTORE:	return OP2(61);

	case AAND:	return OP2(1);
	case AANDCC:	return OP2(17);
	case AANDN:	return OP2(5);
	case AANDNCC:	return OP2(21);

	case AOR:	return OP2(2);
	case AORCC:	return OP2(18);
	case AORN:	return OP2(6);
	case AORNCC:	return OP2(22);

	case AXOR:	return OP2(3);
	case AXORCC:	return OP2(19);
	case AXNOR:	return OP2(7);
	case AXNORCC:	return OP2(23);

	case ASLL:	return OP2(37);
	case ASRL:	return OP2(38);
	case ASRA:	return OP2(39);

	case AJMPL:
	case AJMP:	return OP2(56);
	case ARETT:	return OP2(57);

	case AMOVBU:	return OP3(1);	/* ldub */
	case AMOVB:	return OP3(9);	/* ldsb */
	case AMOVHU:	return OP3(2);	/* lduh */
	case AMOVH:	return OP3(10);	/* ldsh */
	case AMOVW:	return OP3(0);	/* ld */
	case AMOVD:	return OP3(3);	/* ldd */

	case AMOVBU+AEND:
	case AMOVB+AEND:return OP3(5);	/* stb */

	case AMOVHU+AEND:
	case AMOVH+AEND:return OP3(6);	/* sth */

	case AMOVW+AEND:return OP3(4);	/* st */

	case AMOVD+AEND:return OP3(7);	/* std */

	case ASWAP:			/* swap is symmetric */
	case ASWAP+AEND:return OP3(15);

	case ATAS:	return OP3(13);	/* tas is really ldstub */

	case ABN:	return OPB(0);
	case ABE:	return OPB(1);
	case ABLE:	return OPB(2);
	case ABL:	return OPB(3);
	case ABLEU:	return OPB(4);
	case ABCS:	return OPB(5);
	case ABNEG:	return OPB(6);
	case ABVS:	return OPB(7);
	case ABA:	return OPB(8);
	case ABNE:	return OPB(9);
	case ABG:	return OPB(10);
	case ABGE:	return OPB(11);
	case ABGU:	return OPB(12);
	case ABCC:	return OPB(13);
	case ABPOS:	return OPB(14);
	case ABVC:	return OPB(15);

	case AFBA:	return OPFB(8);
	case AFBE:	return OPFB(9);
	case AFBG:	return OPFB(6);
	case AFBGE:	return OPFB(11);
	case AFBL:	return OPFB(4);
	case AFBLE:	return OPFB(13);
	case AFBLG:	return OPFB(2);
	case AFBN:	return OPFB(0);
	case AFBNE:	return OPFB(1);
	case AFBO:	return OPFB(15);
	case AFBU:	return OPFB(7);
	case AFBUE:	return OPFB(10);
	case AFBUG:	return OPFB(5);
	case AFBUGE:	return OPFB(12);
	case AFBUL:	return OPFB(3);
	case AFBULE:	return OPFB(14);

	case ATN:	return OPT(0);
	case ATE:	return OPT(1);
	case ATLE:	return OPT(2);
	case ATL:	return OPT(3);
	case ATLEU:	return OPT(4);
	case ATCS:	return OPT(5);
	case ATNEG:	return OPT(6);
	case ATVS:	return OPT(7);
	case ATA:	return OPT(8);
	case ATNE:	return OPT(9);
	case ATG:	return OPT(10);
	case ATGE:	return OPT(11);
	case ATGU:	return OPT(12);
	case ATCC:	return OPT(13);
	case ATPOS:	return OPT(14);
	case ATVC:	return OPT(15);

	case AFADDF:	return OPF1(65);
	case AFADDD:	return OPF1(66);
	case AFADDX:	return OPF1(67);
	case AFSUBF:	return OPF1(69);
	case AFSUBD:	return OPF1(70);
	case AFSUBX:	return OPF1(71);
	case AFMULF:	return OPF1(73);
	case AFMULD:	return OPF1(74);
	case AFMULX:	return OPF1(75);
	case AFDIVF:	return OPF1(77);
	case AFDIVD:	return OPF1(78);
	case AFDIVX:	return OPF1(79);

	case AFMOVF:	return OPF1(1);
	case AFNEGF:	return OPF1(5);
	case AFABSF:	return OPF1(9);

	case AFSQRTF:	return OPF1(41);
	case AFSQRTD:	return OPF1(42);
	case AFSQRTX:	return OPF1(43);

	case AFMOVWF:	return OPF1(196);
	case AFMOVWD:	return OPF1(200);
	case AFMOVWX:	return OPF1(204);
	case AFMOVFW:	return OPF1(209);
	case AFMOVDW:	return OPF1(210);
	case AFMOVXW:	return OPF1(211);
	case AFMOVFD:	return OPF1(201);
	case AFMOVFX:	return OPF1(205);
	case AFMOVDF:	return OPF1(198);
	case AFMOVDX:	return OPF1(206);
	case AFMOVXF:	return OPF1(199);
	case AFMOVXD:	return OPF1(203);

	case AFCMPF:	return OPF2(81);
	case AFCMPD:	return OPF2(82);
	case AFCMPX:	return OPF2(83);
	case AFCMPEF:	return OPF2(85);
	case AFCMPED:	return OPF2(86);
	case AFCMPEX:	return OPF2(87);

	case AUNIMP:	return 0;
	}
	diag("bad opcode %A", a);
	return 0;
}
