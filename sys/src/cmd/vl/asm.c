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
	case 0:
		OFFSET = rnd(HEADR+textsize, 4096);
		seek(cout, OFFSET, 0);
		break;
	case 1:
	case 2:
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
		case 0:
			OFFSET = rnd(HEADR+textsize, 4096)+datsize;
			seek(cout, OFFSET, 0);
			break;
		case 2:
		case 1:
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
	char name[NNAME];

	s = lookup("etext", 0);
	if(s->type == STEXT)
		putsymb(s->name, 'T', s->value, s->version);

	for(h=0; h<NHASH; h++)
		for(s=hash[h]; s!=S; s=s->link)
			switch(s->type) {
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
			if(a->type == D_MREG)
				putsymb(a->sym->name, 'Z', a->offset, 0);

		if(s->type == STEXT)
			putsymb(s->name, 'T', s->value, s->version);
		else
			putsymb(s->name, 'L', s->value, s->version);

		/* frame, auto and param after */
		strncpy(name, ".frame", NNAME);
		putsymb(name, 'm', p->to.offset+4, 0);
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
	char str[STRINGSZ];

	if(t == 'f')
		s++;
	LPUT(v);
	if(ver)
		t += 'a' - 'A';
	CPUT(t);
	for(i=0; i<NNAME; i++)
		CPUT(s[i]);
	CPUT(0);
	CPUT(0);
	CPUT(0);
	symsize += 4 + 1 + NNAME + 3;
	if(debug['n']) {
		if(t == 'z' || t == 'Z') {
			str[0] = 0;
			for(i=1; i<NNAME; i+=2) {
				f = ((s[i]&0xff) << 8) | (s[i+1]&0xff);
				if(f == 0)
					break;
				sprint(strchr(str, 0), "/%x", f);
			}
			Bprint(&bso, "%c %.8lux %s\n", t, v, str);
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
		for(j=l+(c-i)-1; j>=l; j--)
			if(buf.dbuf[j]) {
				print("%P\n", p);
				diag("multiple initialization\n");
				break;
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

int
asmout(Prog *p, Optab *o, int aflag)
{
	long o1, o2, o3, o4, v;
	Prog *ct;
	int r, a;

	o1 = 0;
	o2 = 0;
	o3 = 0;
	o4 = 0;
	switch(o->type) {
	default:
		diag("unknown type %d\n", o->type);
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
		a = AADD;
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
		o1 = OP_IRR(opirr(p->as+AEND), v, r, p->to.reg);
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
			r = AADD;
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
		if(p->link && p->cond && p->link->as == ANOR) {
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
		o1 = OP_IRR(opirr(AEND), v>>16, REGZERO, p->to.reg);
		o2 = OP_IRR(opirr(AOR), v, p->to.reg, p->to.reg);
		break;

	case 20:	/* mov $lohi,r */
		r = ADIV;
		if(p->from.type == D_LO)
			r = AMUL;
		o1 = OP_RRR(oprrr(r+AEND), REGZERO, REGZERO, p->to.reg);
		break;

	case 21:	/* mov r,$lohi */
		r = ADIVU;
		if(p->to.type == D_LO)
			r = AMULU;
		o1 = OP_RRR(oprrr(r+AEND), REGZERO, p->from.reg, REGZERO);
		break;

	case 22:	/* mul r1,r2 */
		o1 = OP_RRR(oprrr(p->as), p->from.reg, p->reg, REGZERO);
		break;

	case 23:	/* add $lcon,r1,r2 ==> lu+or+add */
		v = regoff(&p->from);
		if(p->to.reg == REGTMP || p->reg == REGTMP)
			diag("cant synthesize large constant\n%P\n", p);
		o1 = OP_IRR(opirr(AEND), v>>16, REGZERO, REGTMP);
		o2 = OP_IRR(opirr(AOR), v, REGTMP, REGTMP);
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		o3 = OP_RRR(oprrr(p->as), REGTMP, r, p->to.reg);
		break;

	case 24:	/* mov $ucon,,r ==> lu r */
		v = regoff(&p->from);
		o1 = OP_IRR(opirr(AEND), v>>16, REGZERO, p->to.reg);
		break;

	case 25:	/* add/and $ucon,[r1],r2 ==> lu $con,t; add t,[r1],r2 */
		v = regoff(&p->from);
		o1 = OP_IRR(opirr(AEND), v>>16, REGZERO, REGTMP);
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		o2 = OP_RRR(oprrr(p->as), REGTMP, r, p->to.reg);
		break;

	case 26:	/* mov $lsext/auto/oreg,,r2 ==> lu+or+add */
		v = regoff(&p->from);
		if(p->to.reg == REGTMP)
			diag("cant synthesize large constant\n%P\n", p);
		o1 = OP_IRR(opirr(AEND), v>>16, REGZERO, REGTMP);
		o2 = OP_IRR(opirr(AOR), v, REGTMP, REGTMP);
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		o3 = OP_RRR(oprrr(AADD), REGTMP, r, p->to.reg);
		break;

	case 27:		/* mov [sl]ext/auto/oreg,fr ==> lwc1 o(r) */
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->from);
		switch(o->size) {
		case 16:
			if(v & 0x8000)
				v += 0x10000;	/* assumes 2's comp */
			o1 = OP_IRR(opirr(AEND), v>>16, REGZERO, REGTMP);
			o2 = OP_RRR(oprrr(AADD), r, REGTMP, REGTMP);
			o3 = OP_IRR(opirr(AMOVF+AEND), v, REGTMP, p->to.reg+1);
			o4 = OP_IRR(opirr(AMOVF+AEND), v+4, REGTMP, p->to.reg);
			break;
		case 12:
			if(v & 0x8000)
				v += 0x10000;	/* assumes 2's comp */
			o1 = OP_IRR(opirr(AEND), v>>16, REGZERO, REGTMP);
			o2 = OP_RRR(oprrr(AADD), r, REGTMP, REGTMP);
			o3 = OP_IRR(opirr(AMOVF+AEND), v, REGTMP, p->to.reg);
			break;
		case 8:
			o1 = OP_IRR(opirr(AMOVF+AEND), v, r, p->to.reg+1);
			o2 = OP_IRR(opirr(AMOVF+AEND), v+4, r, p->to.reg);
			break;
		case 4:
			o1 = OP_IRR(opirr(AMOVF+AEND), v, r, p->to.reg);
			break;
		}
		break;

	case 28:		/* mov fr,[sl]ext/auto/oreg ==> swc1 o(r) */
		r = p->to.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->to);
		switch(o->size) {
		case 16:
			if(v & 0x8000)
				v += 0x10000;	/* assumes 2's comp */
			o1 = OP_IRR(opirr(AEND), v>>16, REGZERO, REGTMP);
			o2 = OP_RRR(oprrr(AADD), r, REGTMP, REGTMP);
			o3 = OP_IRR(opirr(AMOVF), v, REGTMP, p->from.reg+1);
			o4 = OP_IRR(opirr(AMOVF), v+4, REGTMP, p->from.reg);
			break;
		case 12:
			if(v & 0x8000)
				v += 0x10000;	/* assumes 2's comp */
			o1 = OP_IRR(opirr(AEND), v>>16, REGZERO, REGTMP);
			o2 = OP_RRR(oprrr(AADD), r, REGTMP, REGTMP);
			o3 = OP_IRR(opirr(AMOVF), v, REGTMP, p->from.reg);
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
		o1 = OP_RRR(oprrr(AMOVWF+AEND), p->from.reg, 0, p->to.reg);
		break;

	case 31:	/* movw fr,r */
		o1 = OP_RRR(oprrr(AMOVFW+AEND), p->to.reg, 0, p->from.reg);
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
		r = AADD;
		if(o->a1 == C_ANDCON)
			r = AOR;
		o1 = OP_IRR(opirr(r), v, 0, REGTMP);
		o2 = OP_RRR(oprrr(AMOVWF+AEND), REGTMP, 0, p->to.reg);
		break;

	case 35:	/* mov r,lext/luto/oreg ==> sw o(r) */
		v = regoff(&p->to);
		if(v & 0x8000L)
			v += 0x10000L;
		if(p->to.reg == REGTMP || p->reg == REGTMP)
			diag("cant synthesize large constant\n%P\n", p);
		o1 = OP_IRR(opirr(AEND), v>>16, REGZERO, REGTMP);
		r = p->to.reg;
		if(r == NREG)
			r = o->param;
		o2 = OP_RRR(oprrr(AADDU), r, REGTMP, REGTMP);
		o3 = OP_IRR(opirr(p->as), v, REGTMP, p->from.reg);
		break;

	case 36:	/* mov lext/lauto/lreg,r ==> lw o(r30) */
		v = regoff(&p->from);
		if(v & 0x8000L)
			v += 0x10000L;
		if(p->to.reg == REGTMP || p->reg == REGTMP)
			diag("cant synthesize large constant\n%P\n", p);
		o1 = OP_IRR(opirr(AEND), v>>16, REGZERO, REGTMP);
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		o2 = OP_RRR(oprrr(AADDU), r, REGTMP, REGTMP);
		o3 = OP_IRR(opirr(p->as+AEND), v, REGTMP, p->to.reg);
		break;

	case 37:	/* movw r,mr */
		o1 = OP_RRR(oprrr(ATLBP+AEND), p->from.reg, 0, p->to.reg);
		break;

	case 38:	/* movw mr,r */
		o1 = OP_RRR(oprrr(ATLBR+AEND), p->to.reg, 0, p->from.reg);
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
		o1 = OP_RRR(oprrr(ATLBWI+AEND), p->from.reg, 0, p->to.reg);
		break;

	case 42:	/* movw fcr,r */
		o1 = OP_RRR(oprrr(ATLBWR+AEND), p->to.reg, 0, p->from.reg);
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
	}
	return 0;
}

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

	case ATLBP+AEND:	return SP(2,0)|(4<<21); /* mtc0 */
	case ATLBR+AEND:	return SP(2,0)|(0<<21); /* mfc0 */

	case AMOVWF+AEND:	return SP(2,1)|(4<<21); /* mtc1 */
	case AMOVFW+AEND:	return SP(2,1)|(0<<21); /* mfc1 */

	case ATLBWI+AEND:	return SP(2,1)|(6<<21); /* mtcc1 */
	case ATLBWR+AEND:	return SP(2,1)|(2<<21); /* mfcc1 */

	case AMUL+AEND:		return OP(2,2);	/* mflo */
	case ADIV+AEND:		return OP(2,0);	/* mfhi */
	case AMULU+AEND:	return OP(2,3);	/* mtlo */
	case ADIVU+AEND:	return OP(2,1);	/* mthi */
	}
	diag("bad rrr %d\n", a);
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
	case AEND:	return SP(1,7);
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
	case AMOVF:	return SP(7,1);
	case AMOVWL:	return SP(5,2);
	case AMOVWR:	return SP(5,6);

	case ABREAK:	return SP(5,7);

	case AMOVWL+AEND:	return SP(4,2);
	case AMOVWR+AEND:	return SP(4,6);
	case AMOVB+AEND:	return SP(4,0);
	case AMOVBU+AEND:	return SP(4,4);
	case AMOVH+AEND:	return SP(4,1);
	case AMOVHU+AEND:	return SP(4,5);
	case AMOVW+AEND:	return SP(4,3);
	case AMOVF+AEND:	return SP(6,1);

	}
	diag("bad irr %d\n", a);
	return 0;
}
