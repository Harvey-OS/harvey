#include	"l.h"

#define	LPUT(c)\
	{\
		cbp[0] = (c);\
		cbp[1] = (c)>>8;\
		cbp[2] = (c)>>16;\
		cbp[3] = (c)>>24;\
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

#define	VLPUT(c)\
	{\
		cbp[0] = (c);\
		cbp[1] = (c)>>8;\
		cbp[2] = (c)>>16;\
		cbp[3] = (c)>>24;\
		cbp[4] = (c)>>32;\
		cbp[5] = (c)>>40;\
		cbp[6] = (c)>>48;\
		cbp[7] = (c)>>56;\
		cbp += 8;\
		cbc -= 8;\
		if(cbc <= 0)\
			cflush();\
	}
#define	LPUTBE(c)\
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
	vlong t;
	Optab *o;

	if(debug['v'])
		Bprint(&bso, "%5.2f asm\n", cputime());
	Bflush(&bso);
	seek(cout, HEADR, 0);
	pc = INITTEXT;
	for(p = firstp; p != P; p = p->link) {
		if(p->as == ATEXT) {
			curtext = p;
			autosize = p->to.offset + 8;
		}
		if(p->pc != pc) {
			diag("phase error %lux sb %lux",
				p->pc, pc);
			if(!debug['a'])
				prasm(curp);
			pc = p->pc;
		}
		if (p->as == AMOVQ || p->as == AMOVT) {
			if ((p->from.reg == REGSP) && (p->from.offset&7) != 0
			|| (p->to.reg == REGSP) && (p->to.offset&7) != 0)
				diag("bad stack alignment: %P", p);
			if ((p->from.reg == REGSB) && (p->from.offset&7) != 0
			|| (p->to.reg == REGSB) && (p->to.offset&7) != 0)
				diag("bad global alignment: %P", p);
		}
		curp = p;
		o = oplook(p);	/* could probably avoid this call */
		if(asmout(p, o)) {
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
		seek(cout, rnd(HEADR+textsize, 8192), 0);
		break;
	case 1:
	case 2:
	case 3:
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
			seek(cout, rnd(HEADR+textsize, 8192)+datsize, 0);
			break;
		case 2:
		case 1:
		case 3:
			seek(cout, HEADR+textsize+datsize, 0);
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
	seek(cout, 0L, 0);
	switch(HEADTYPE) {
	case 0:
		lput(0x0183L);		/* magic and sections */
		lput(0L);			/* time and date */
		vlput(rnd(HEADR+textsize, 8192)+datsize);
		lput(symsize);			/* nsyms */
		lput(0x50L|(7L<<16));		/* size of optional hdr and flags */
		lput(0413|(0x101L<<16));	/* magic and version */
		lput(-1);			/* pad for alignment */

		vlput(rnd(HEADR+textsize, 8192));		/* sizes */
		vlput(datsize);
		vlput(bsssize);
		vlput(entryvalue());		/* va of entry */
		vlput(INITTEXT-HEADR);		/* va of base of text */
		vlput(INITDAT);			/* va of base of data */
		vlput(INITDAT+datsize);		/* va of base of bss */
		lput(~0L);			/* gp reg mask */
		/* dubious stuff starts here */
		lput(0L);
		lput(0L);
		lput(0L);
		lput(0L);
		lput(~0L);			/* gp value ?? */
		break;
	case 1:
		lput(0x0183L);		/* magic and sections */
		lput(0L);			/* time and date */
		vlput(HEADR+textsize+datsize);
		lput(symsize);			/* nsyms */
		lput(0x54L|(7L<<16));		/* size of optional hdr and flags */
		lput(0407|(0x101L<<16));	/* magic and version */
		lput(-1);			/* pad for alignment */

		vlput(textsize);		/* sizes */
		vlput(datsize);
		vlput(bsssize);
		vlput(entryvalue());		/* va of entry */
		vlput(INITTEXT);		/* va of base of text */
		vlput(INITDAT);			/* va of base of data */
		vlput(INITDAT+datsize);		/* va of base of bss */
		lput(~0L);			/* gp reg mask */
		/* dubious stuff starts here */
		lput(lcsize);
		lput(0L);
		lput(0L);
		lput(0L);
		lput(~0L);			/* gp value ?? */
		lput(0L);			/* complete mystery */
		break;
	case 2:
		lputbe(0x84b);			/* magic */
		lputbe(textsize);		/* sizes */
		lputbe(datsize);
		lputbe(bsssize);
		lputbe(symsize);		/* nsyms */
		lputbe(entryvalue());		/* va of entry */
		lputbe(0L);
		lputbe(lcsize);
		break;
	case 3:
		/* ``headerless'' boot image -- magic no is a branch */
		lput(0xc3e00007);		/* magic (branch) */
		lputbe(textsize);		/* sizes */
		lputbe(datsize);
		lputbe(bsssize);
		lputbe(symsize);		/* nsyms */
		lputbe(entryvalue());		/* va of entry */
		lputbe(0L);
		lputbe(lcsize);
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
lputbe(long l)
{
	LPUTBE(l);
}

void
vlput(vlong l)
{
	VLPUT(l);
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
				putsymb(a->sym->name, 'z', a->offset, 0);
			else
			if(a->type == D_FILE1)
				putsymb(a->sym->name, 'Z', a->offset, 0);

		if(s->type == STEXT)
			putsymb(s->name, 'T', s->value, s->version);
		else
			putsymb(s->name, 'L', s->value, s->version);

		/* frame, auto and param after */
		putsymb(".frame", 'm', p->to.offset+8, 0);
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
	LPUTBE(v);
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
				fl = ieeedtof(p->to.ieee);
				cast = (char*)&fl;
				for(; i<c; i++) {
					buf.dbuf[l] = cast[fnuxi8[i]];
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
			case 8:
				for(; i<4; i++) {
					buf.dbuf[l] = cast[inuxi4[i]];
					l++;
				}
				d = p->to.offset >> 32;
				for(; i<c; i++) {
					buf.dbuf[l] = cast[inuxi4[i-4]];
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
	(op|(((r1)&31L)<<16)|(((r2)&31L)<<21)|((r3)&31L))
#define	OP_IRR(op,i,r2,r3)\
	(op|(((i)&255L)<<13)|0x1000|(((r2)&31L)<<21)|((r3)&31L))
#define	OP_MEM(op,d,r1,r2)\
	(op|(((r1)&31L)<<16)|(((r2)&31L)<<21)|((d)&0xffff))
#define OP_BR(op,d,r1)\
	(op|((d)&0x1fffff)|(((r1)&31L)<<21))

int
asmout(Prog *p, Optab *o)
{
	long o1, o2, o3, o4, o5, o6;
	vlong v;
	int r, a;

	o1 = 0;
	o2 = 0;
	o3 = 0;
	o4 = 0;
	o5 = 0;
	o6 = 0;
	switch(o->type) {
	default:
		diag("unknown type %d", o->type);
		if(!debug['a'])
			prasm(p);
		break;

	case 0:		/* pseudo ops */
		break;

	case 1:		/* register-register moves */
		if(p->as == AMOVB || p->as == AMOVW)		/* noop should rewrite */
			diag("forbidden SEX: %P", p);
		if(p->as == AMOVBU || p->as == AMOVWU) {
			v = 1;
			if (p->as == AMOVWU)
				v = 3;
			o1 = OP_IRR(opcode(AZAPNOT), v, p->from.reg, p->to.reg);
		}
		else {
			a = AOR;
			if(p->as == AMOVL)
				a = AADDL;
			if(p->as == AMOVLU)
				a = AEXTLL;
			o1 = OP_RRR(opcode(a), REGZERO, p->from.reg, p->to.reg);
		}
		break;

	case 2:		/* <operate> r1,[r2],r3 */
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		o1 = OP_RRR(opcode(p->as), p->from.reg, r, p->to.reg);
		break;

	case 3:		/* <operate> $n,[r2],r3 */
		v = regoff(&p->from);
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		o1 = OP_IRR(opcode(p->as), v, r, p->to.reg);
		break;

	case 4:		/* beq r1,sbra */
		if(p->cond == P)
			v = -4 >> 2;
		else
			v = (p->cond->pc - pc-4) >> 2;
		o1 = OP_BR(opcode(p->as), v, p->from.reg);
		break;

	case 5:		/* jmp [r1],0(r2) */
		r = p->reg;
		a = p->as;
		if(r == NREG) {
			r = o->param;
/*			if(a == AJMP && p->to.reg == REGLINK)
				a = ARET; /* this breaks the kernel -- maybe we need to clear prediction stack on each context switch... */
		}
		o1 = OP_MEM(opcode(a), 0, p->to.reg, r);
		break;

	case 6:		/* movq $n,r1 and movq $soreg,r1 */
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->from);
		o1 = OP_MEM(opcode(AMOVA), v, r, p->to.reg);
		break;

	case 7:		/* movbu r1, r2 */
		v = 1;
		if (p->as == AMOVWU)
			v = 3;
		o1 = OP_IRR(opcode(AZAPNOT), v, p->from.reg, p->to.reg);
		break;

	case 8:		/* mov r, soreg ==> stq o(r) */
		r = p->to.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->to);
		if (p->as == AMOVQ || p->as == AMOVT)
			if ((r == REGSP || r == REGSB) && (v&7) != 0)
				diag("bad alignment: %P", p);
		o1 = OP_MEM(opcode(p->as+AEND), v, r, p->from.reg);
		break;

	case 9:		/* mov soreg, r ==> ldq o(r) */
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->from);
		if (p->as == AMOVQ || p->as == AMOVT)
			if ((r == REGSP || r == REGSB) && (v&7) != 0)
				diag("bad alignment: %P", p);
		o1 = OP_MEM(opcode(p->as), v, r, p->to.reg);
		break;

	case 10:	/* movb r1,r2 */
		v = 64 - 8;
		if (p->as == AMOVW)
			v = 64 - 16;
		o1 = OP_IRR(opcode(ASLLQ), v, p->from.reg, p->to.reg);
		o2 = OP_IRR(opcode(ASRAQ), v, p->to.reg, p->to.reg);
		break;

	case 11:	/* jmp lbra */
		if(p->cond == P)
			v = -4 >> 2;
		else
			v = (p->cond->pc - pc-4) >> 2;
		a = ABR;
		r = REGZERO;
		if (p->as == AJSR) {
			a = ABSR;
			r = REGLINK;
		}
		o1 = OP_BR(opcode(a), v, r);
		break;

	case 12:	/* addq $n,[r2],r3 ==> lda */
		v = regoff(&p->from);
		if (p->as == ASUBQ)
			v = -v;
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		o1 = OP_MEM(opcode(AMOVA), v, r, p->to.reg);
		break;

	case 13:	/* <op> $scon,[r2],r3 */
		v = regoff(&p->from);
		if(p->to.reg == REGTMP || p->reg == REGTMP)
			diag("cant synthesize large constant\n%P", p);
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		o1 = OP_MEM(opcode(AMOVA), v, REGZERO, REGTMP);
		o2 = OP_RRR(opcode(p->as), REGTMP, r, p->to.reg);
		break;

	case 14:	/* <op> $lcon,[r2],r3 */
		v = regoff(&p->from);
		if(v & 0x8000)
			v += 0x10000;
		if(p->to.reg == REGTMP || p->reg == REGTMP)
			diag("cant synthesize large constant\n%P", p);
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		o1 = OP_MEM(opcode(AMOVA), v, REGZERO, REGTMP);
		o2 = OP_MEM(opcode(AMOVAH), v>>16, REGTMP, REGTMP);
		o3 = OP_RRR(opcode(p->as), REGTMP, r, p->to.reg);
		break;

	case 15:	/* mov $lcon,r1 */
		v = regoff(&p->from);
		if(v & 0x8000)
			v += 0x10000;
		o1 = OP_MEM(opcode(AMOVA), v, o->param, REGTMP);
		o2 = OP_MEM(opcode(AMOVAH), v>>16, REGTMP, p->to.reg);
		break;

	case 16:	/* mov $qcon,r1 */
		v = regoff(&p->from);
		if(v & 0x8000)
			v += 0x10000;
		if((v>>31)&1)
			v += (1LL<<32);
		if((v>>47)&1)
			v += (1LL<<48);
		o1 = OP_MEM(opcode(AMOVA), v>>32, o->param, REGTMP);
		o2 = OP_MEM(opcode(AMOVAH), v>>48, REGTMP, REGTMP);
		o3 = OP_IRR(opcode(ASLLQ), 32, REGTMP, REGTMP);
		o4 = OP_MEM(opcode(AMOVA), v, REGTMP, REGTMP);
		o5 = OP_MEM(opcode(AMOVAH), v>>16, REGTMP, p->to.reg);
		break;

	case 17:	/* mov f1,f2 ==> fcpys f1,f1,f2 */
		o1 = OP_RRR(opcode(ACPYS), p->from.reg, p->from.reg, p->to.reg);
		break;

	case 18:	/* call_pal imm */
		v = regoff(&p->from);
		o1 = OP_MEM(opcode(ACALL_PAL), v, 0, 0);
		break;

	case 19:	/* mov r, loreg ==> ldah,stq */
		r = p->to.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->to);
		if (p->as == AMOVQ || p->as == AMOVT)
			if ((r == REGSP || r == REGSB) && (v&7) != 0)
				diag("bad alignment: %P", p);
		if(v & 0x8000)
			v += 0x10000;
		o1 = OP_MEM(opcode(AMOVAH), v>>16, r, REGTMP);
		o2 = OP_MEM(opcode(p->as+AEND), v, REGTMP, p->from.reg);
		break;

	case 20:	/* mov loreg, r ==> ldah,ldq */
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->from);
		if (p->as == AMOVQ || p->as == AMOVT)
			if ((r == REGSP || r == REGSB) && (v&7) != 0)
				diag("bad alignment: %P", p);
		if(v & 0x8000)
			v += 0x10000;
		o1 = OP_MEM(opcode(AMOVAH), v>>16, r, REGTMP);
		o2 = OP_MEM(opcode(p->as), v, REGTMP, p->to.reg);
		break;

#ifdef	NEVER
	case 21:	/* mov r1,$qoreg */
		r = p->to.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->to);
		if(v & 0x8000)
			v += 0x10000;
		if((v>>31)&1)
			v += (1LL<<32);
		if((v>>47)&1)
			v += (1LL<<48);
		o1 = OP_MEM(opcode(AMOVA), v>>32, r, REGTMP);
		o2 = OP_MEM(opcode(AMOVAH), v>>48, REGTMP, REGTMP);
		o3 = OP_IRR(opcode(ASLLQ), 32, REGTMP, REGTMP);
		o4 = OP_MEM(opcode(AMOVAH), v>>16, REGTMP, REGTMP);
		o5 = OP_MEM(opcode(p->as+AEND), v, REGTMP, p->from.reg);
		break;

	case 22:	/* mov $qoreg,r1 */
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->from);
		if(v & 0x8000)
			v += 0x10000;
		if((v>>31)&1)
			v += (1LL<<32);
		if((v>>47)&1)
			v += (1LL<<48);
		o1 = OP_MEM(opcode(AMOVA), v>>32, r, REGTMP);
		o2 = OP_MEM(opcode(AMOVAH), v>>48, REGTMP, REGTMP);
		o3 = OP_IRR(opcode(ASLLQ), 32, REGTMP, REGTMP);
		o4 = OP_MEM(opcode(AMOVAH), v>>16, REGTMP, REGTMP);
		o5 = OP_MEM(opcode(p->as), v, REGTMP, p->to.reg);
		break;
#endif

	case 23:	/* <op> $qcon,r1 */
		if(p->to.reg == REGTMP || p->reg == REGTMP)
			diag("cant synthesize large constant\n%P", p);
		v = regoff(&p->from);
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		if(v & 0x8000)
			v += 0x10000;
		if((v>>31)&1)
			v += (1LL<<32);
		if((v>>47)&1)
			v += (1LL<<48);
		o1 = OP_MEM(opcode(AMOVA), v>>32, REGZERO, REGTMP);
		o2 = OP_MEM(opcode(AMOVAH), v>>48, REGTMP, REGTMP);
		o3 = OP_IRR(opcode(ASLLQ), 32, REGTMP, REGTMP);
		o4 = OP_MEM(opcode(AMOVA), v, REGTMP, REGTMP);
		o5 = OP_MEM(opcode(AMOVAH), v>>16, REGTMP, REGTMP);
		o6 = OP_RRR(opcode(p->as), REGTMP, r, p->to.reg);
		break;

	case 24:	/* movq Fn, FPCR */
		r = p->from.reg;
		o1 = OP_RRR(opcode(AADDT+AEND), r, r, r);
		break;

	case 25:	/* movq FPCR, Fn */
		r = p->to.reg;
		o1 = OP_RRR(opcode(AADDS+AEND), r, r, r);
		break;

	case 26:	/* movq Rn, C_PREG */
		r = p->from.reg;
		o1 = OP_RRR(opcode(ASUBQ+AEND), r, r, 0) | p->to.reg & 255;
		break;

	case 27:	/* movq C_PREG, Rn */
		r = p->to.reg;
		o1 = OP_RRR(opcode(AADDQ+AEND), r, r, 0) | p->from.reg & 255;
		break;

	case 28:	/* cvttq r1,r3 */
		r = p->from.reg;
		o1 = OP_RRR(opcode(p->as), r, REGZERO, p->to.reg);
		break;

	case 29:	/* movq pcc, rpcc -> Rn */
		o1 = OP_MEM(opcode(ARPCC), 0, REGZERO, p->to.reg);
		break;

	case 30:	/* rei/mb/trapb */
		o1 = OP_MEM(opcode(p->as), 0, REGZERO, REGZERO);
		break;

	case 31:	/* fetch (Rn) */
		o1 = OP_MEM(opcode(p->as), 0, REGZERO, p->from.reg);
		break;

	case 32:	/* movqp r, soreg ==> stqp o(r) */
		r = p->to.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->to);
		if (v < -0x800 || v >= 0x800)
			diag("physical store out of range\n%P", p);
		v &= 0xfff;
		o1 = OP_MEM(opcode(p->as+AEND), v, r, p->from.reg);
		break;

	case 33:	/* movqp soreg, r ==> ldqp o(r) */
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->from);
		if (v < -0x800 || v >= 0x800)
			diag("physical load out of range\n%P", p);
		v &= 0xfff;
		o1 = OP_MEM(opcode(p->as), v, r, p->to.reg);
		break;

	case 34:	/* <operate> $-n,[r2],r3 */
		v = regoff(&p->from);
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		switch (a = p->as) {
		case AAND:
			a = AANDNOT;
			break;
		case AANDNOT:
			a = AAND;
			break;
		case AOR:
			a = AORNOT;
			break;
		case AORNOT:
			a = AOR;
			break;
		case AXOR:
			a = AXORNOT;
			break;
		case AXORNOT:
			a = AXOR;
			break;
		default:
			diag("bad in NCON case: %P", p);
		}
		v = ~v;
		o1 = OP_IRR(opcode(a), v, r, p->to.reg);
		break;

	case 40:	/* word */
		o1 = regoff(&p->to);
		break;

	}
	switch(o->size) {
	default:
		if(debug['a'])
			Bprint(&bso, " %.8lux:\t\t%P\n", p->pc, p);
		break;
	case 4:
		if(debug['a'])
			Bprint(&bso, " %.8lux: %.8lux\t%P\n", p->pc, o1, p);
		LPUT(o1);
		break;
	case 8:
		if(debug['a'])
			Bprint(&bso, " %.8lux: %.8lux %.8lux %P\n", p->pc, o1, o2, p);
		LPUT(o1);
		LPUT(o2);
		break;
	case 12:
		if(debug['a'])
			Bprint(&bso, " %.8lux: %.8lux %.8lux %.8lux %P\n", p->pc, o1, o2, o3, p);
		LPUT(o1);
		LPUT(o2);
		LPUT(o3);
		break;
	case 16:
		if(debug['a'])
			Bprint(&bso, " %.8lux: %.8lux %.8lux %.8lux %.8lux %P\n",
				p->pc, o1, o2, o3, o4, p);
		LPUT(o1);
		LPUT(o2);
		LPUT(o3);
		LPUT(o4);
		break;
	case 20:
		if(debug['a'])
			Bprint(&bso, " %.8lux: %.8lux %.8lux %.8lux %.8lux %.8lux %P\n",
				p->pc, o1, o2, o3, o4, o5, p);
		LPUT(o1);
		LPUT(o2);
		LPUT(o3);
		LPUT(o4);
		LPUT(o5);
		break;
	case 24:
		if(debug['a'])
			Bprint(&bso, " %.8lux: %.8lux %.8lux %.8lux %.8lux %.8lux %.8lux %P\n",
				p->pc, o1, o2, o3, o4, o5, o6, p);
		LPUT(o1);
		LPUT(o2);
		LPUT(o3);
		LPUT(o4);
		LPUT(o5);
		LPUT(o6);
		break;
	}
	return 0;
}

#define	OP(x,y)	(((x)<<26)|((y)<<5))
#define	FP(x)	OP(22, (x)|0xc0)	/* note: this sets round/trap modes (dynamic, software?). not used for cvtxx? */
#define	FP2(x)	OP(22, (x) /*|0x080*/)	/* note: this sets round/trap modes (chopped, software?). used for cvtxx? */
#define	FP3(x)	OP(22, (x)|0x080)	/* note: this sets round/trap modes (dynamic, software?). not used for cvtxx? */

long
opcode(int a)
{
	switch (a) {
	/* loads */
	case AMOVB:		/* misnomer; pretend it's ok for now */
diag("opcode(AMOVB)");
	case AMOVBU:		return OP(10, 0);	/* v 3 */
	case AMOVW:		/* misnomer; pretend it's ok for now */
diag("opcode(AMOVW)");
	case AMOVWU:		return OP(12, 0);	/* v 3 */
	case AMOVL:		return OP(40, 0);
	case AMOVQ:		return OP(41, 0);
	case AMOVQU:		return OP(11, 0);
	case AMOVS:		return OP(34, 0);
	case AMOVT:		return OP(35, 0);

	/* stores */
	case AMOVB+AEND:		/* misnomer; pretend it's ok for now */
	case AMOVBU+AEND:	return OP(14, 0);	/* v 3 */
	case AMOVW+AEND:	/* misnomer; pretend it's ok for now */
	case AMOVWU+AEND:	return OP(13, 0);	/* v 3 */
	case AMOVL+AEND:	return OP(44, 0);
	case AMOVQ+AEND:	return OP(45, 0);
	case AMOVQU+AEND:	return OP(15, 0);
	case AMOVS+AEND:	return OP(38, 0);
	case AMOVT+AEND:	return OP(39, 0);

	/* physical */
	case AMOVLP+AEND:	return OP(31, 0)|0x8000;
	case AMOVQP+AEND:	return OP(31, 0)|0x9000;
	case AMOVLP:		return OP(27, 0)|0x8000;
	case AMOVQP:		return OP(27, 0)|0x9000;

	/* load address */
	case AMOVA:		return OP(8, 0);
	case AMOVAH:		return OP(9, 0);

	/* locking */
	case AMOVLL:		return OP(42, 0);	/* load locked */
	case AMOVQL:		return OP(43, 0);	/* load locked */
	case AMOVLC+AEND:	return OP(46, 0);	/* store cond */
	case AMOVQC+AEND:	return OP(47, 0);	/* store cond */

	case AADDL:		return OP(16, 0);
	case AADDLV:		return OP(16, 64);
	case AADDQ:		return OP(16, 32);
	case AADDQV:		return OP(16, 96);
	case AS4ADDL:		return OP(16, 2);
	case AS4ADDQ:		return OP(16, 34);
	case AS8ADDL:		return OP(16, 18);
	case AS8ADDQ:		return OP(16, 50);
	case AS4SUBL:		return OP(16, 11);
	case AS4SUBQ:		return OP(16, 43);
	case AS8SUBL:		return OP(16, 27);
	case AS8SUBQ:		return OP(16, 59);
	case ASUBL:		return OP(16, 9);
	case ASUBLV:		return OP(16, 73);
	case ASUBQ:		return OP(16, 41);
	case ASUBQV:		return OP(16, 105);
	case ACMPEQ:		return OP(16, 45);
	case ACMPGT:		return OP(16, 77);
	case ACMPGE:		return OP(16, 109);
	case ACMPUGT:		return OP(16, 29);
	case ACMPUGE:		return OP(16, 61);
	case ACMPBLE:		return OP(16, 15);

	case AAND:		return OP(17, 0);
	case AANDNOT:		return OP(17, 8);
	case AOR:		return OP(17, 32);
	case AORNOT:		return OP(17, 40);
	case AXOR:		return OP(17, 64);
	case AXORNOT:		return OP(17, 72);

	case ACMOVEQ:		return OP(17, 36);
	case ACMOVNE:		return OP(17, 38);
	case ACMOVLT:		return OP(17, 68);
	case ACMOVGE:		return OP(17, 70);
	case ACMOVLE:		return OP(17, 100);
	case ACMOVGT:		return OP(17, 102);
	case ACMOVLBS:		return OP(17, 20);
	case ACMOVLBC:		return OP(17, 22);

	case AMULL:		return OP(19, 0);
	case AMULQ:		return OP(19, 32);
	case AMULLV:		return OP(19, 64);
	case AMULQV:		return OP(19, 96);
	case AUMULH:		return OP(19, 48);

	case ASLLQ:		return OP(18, 57);
	case ASRLQ:		return OP(18, 52);
	case ASRAQ:		return OP(18, 60);

	case AEXTBL:		return OP(18, 6);
	case AEXTWL:		return OP(18, 22);
	case AEXTLL:		return OP(18, 38);
	case AEXTQL:		return OP(18, 54);
	case AEXTWH:		return OP(18, 90);
	case AEXTLH:		return OP(18, 106);
	case AEXTQH:		return OP(18, 122);

	case AINSBL:		return OP(18, 11);
	case AINSWL:		return OP(18, 27);
	case AINSLL:		return OP(18, 43);
	case AINSQL:		return OP(18, 59);
	case AINSWH:		return OP(18, 87);
	case AINSLH:		return OP(18, 103);
	case AINSQH:		return OP(18, 119);

	case AMSKBL:		return OP(18, 2);
	case AMSKWL:		return OP(18, 18);
	case AMSKLL:		return OP(18, 34);
	case AMSKQL:		return OP(18, 50);
	case AMSKWH:		return OP(18, 82);
	case AMSKLH:		return OP(18, 98);
	case AMSKQH:		return OP(18, 114);

	case AZAP:		return OP(18, 48);
	case AZAPNOT:		return OP(18, 49);

	case AJMP:		return OP(26, 0);
	case AJSR:		return OP(26, 512);
	case ARET:		return OP(26, 1024);

	case ABR:		return OP(48, 0);
	case ABSR:		return OP(52, 0);

	case ABEQ:		return OP(57, 0);
	case ABNE:		return OP(61, 0);
	case ABLT:		return OP(58, 0);
	case ABGE:		return OP(62, 0);
	case ABLE:		return OP(59, 0);
	case ABGT:		return OP(63, 0);
	case ABLBC:		return OP(56, 0);
	case ABLBS:		return OP(60, 0);

	case AFBEQ:		return OP(49, 0);
	case AFBNE:		return OP(53, 0);
	case AFBLT:		return OP(50, 0);
	case AFBGE:		return OP(54, 0);
	case AFBLE:		return OP(51, 0);
	case AFBGT:		return OP(55, 0);

	case ATRAPB:		return OP(24, 0);
	case AMB:		return OP(24, 0x200);
	case AFETCH:		return OP(24, 0x400);
	case AFETCHM:		return OP(24, 0x500);
	case ARPCC:		return OP(24, 0x600);

	case ACPYS:		return OP(23, 32);
	case ACPYSN:		return OP(23, 33);
	case ACPYSE:		return OP(23, 34);
	case AADDS+AEND:	return OP(23, 37);	/* MF_FPCR */
	case AADDT+AEND:	return OP(23, 36);	/* MT_FPCR */
	case ACVTLQ:		return OP(23, 16);
	case ACVTQL:		return OP(23, 48);	/* XXX trap mode */
	case AFCMOVEQ:		return OP(23, 42);
	case AFCMOVNE:		return OP(23, 43);
	case AFCMOVLT:		return OP(23, 44);
	case AFCMOVGE:		return OP(23, 45);
	case AFCMOVLE:		return OP(23, 46);
	case AFCMOVGT:		return OP(23, 47);

	case AADDS:		return FP(0);
	case AADDT:		return FP(32);
	case ACMPTEQ:		return FP3(37);
	case ACMPTGT:		return FP3(38);
	case ACMPTGE:		return FP3(39);
	case ACMPTUN:		return FP3(36);

	case ACVTQS:		return FP2(60);
	case ACVTQT:		return FP2(62);
	case ACVTTS:		return FP2(44);
	case ACVTTQ:		return FP2(47);

	case ADIVS:		return FP(3);
	case ADIVT:		return FP(35);
	case AMULS:		return FP(2);
	case AMULT:		return FP(34);
	case ASUBS:		return FP(1);
	case ASUBT:		return FP(33);

	case ACALL_PAL:		return 0;
	case AREI:		return OP(30, 0x400);	/* HW_REI */

	case AADDQ+AEND:	return OP(25,0);	/* HW_MFPR */
	case ASUBQ+AEND:	return OP(29,0);	/* HW_MTPR */
	}
	diag("bad op %A(%d)", a, a);
	return 0;
}

