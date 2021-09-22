#include	"l.h"

long	OFFSET;

xlong
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
	return s->value + INITTEXT;
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
	pc = 0;
	for(p = firstp; p != P; p = p->link) {
		if(p->as == ATEXT) {
			/* align on word boundary */
			if(!debug['c'] && (pc & 2) != 0){
				nopalign.pc = pc;
				pc += asmout(&nopalign, oplook(&nopalign), 0);
			}
			curtext = p;
			autosize = p->to.offset + ptrsize;
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
		pc += asmout(p, o, 0);
	}
	if(debug['a'])
		Bprint(&bso, "\n");
	Bflush(&bso);
	cflush();

	etext = textsize;
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
	case 6:
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
		case 6:
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
	case 1:
		break;
	case 2:
		/* XXX expanded header needed? */
		t = thechar == 'j' ? 30 : 29;
		lput(((((4*t)+0)*t)+7));	/* magic */
		lput(textsize);			/* sizes */
		lput(datsize);
		lput(bsssize);
		lput(symsize);			/* nsyms */
		lput(entryvalue());		/* va of entry */
		lput(0L);
		lput(lcsize);
		break;
	case 5:
		if(thechar == 'j')
			elf64(243, ELFDATA2LSB, 0, nil);		/* 243 is RISCV */
		else
			elf32(243, ELFDATA2LSB, 0, nil);
	}
	cflush();
}

void
strnput(char *s, int n)
{
	for(; *s; s++){
		cput(*s);
		n--;
	}
	for(; n > 0; n--)
		cput(0);
}

void
cput(int c)
{
	cbp[0] = c;
	cbp++;
	cbc--;
	if(cbc <= 0)
		cflush();
}

void
wput(long l)
{

	cbp[0] = l>>8;
	cbp[1] = l;
	cbp += 2;
	cbc -= 2;
	if(cbc <= 0)
		cflush();
}

void
wputl(long l)
{

	cbp[0] = l;
	cbp[1] = l>>8;
	cbp += 2;
	cbc -= 2;
	if(cbc <= 0)
		cflush();
}

void
lput(long l)
{

	cbp[0] = l>>24;
	cbp[1] = l>>16;
	cbp[2] = l>>8;
	cbp[3] = l;
	cbp += 4;
	cbc -= 4;
	if(cbc <= 0)
		cflush();
}

void
lputl(long l)
{

	cbp[3] = l>>24;
	cbp[2] = l>>16;
	cbp[1] = l>>8;
	cbp[0] = l;
	cbp += 4;
	cbc -= 4;
	if(cbc <= 0)
		cflush();
}

void
llput(vlong v)
{
	lput(v>>32);
	lput(v);
}

void
llputl(vlong v)
{
	lputl(v);
	lputl(v>>32);
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
		putsymb(s->name, 'T', s->value+INITTEXT, s->version);

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
			putsymb(s->name, 'T', s->value+INITTEXT, s->version);
		else
			putsymb(s->name, 'L', s->value+INITTEXT, s->version);

		/* frame, auto and param after */
		putsymb(".frame", 'm', p->to.offset+ptrsize, 0);
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
putsymb(char *s, int t, vlong v, int ver)
{
	int i, f, l;

	if(t == 'f')
		s++;
	if(thechar == 'j'){
		l = 8;
		llput(v);
	}else{
		l = 4;
		lput(v);
	}
	if(ver)
		t += 'a' - 'A';
	cput(t+0x80);			/* 0x80 is variable length */

	if(t == 'Z' || t == 'z') {
		cput(s[0]);
		for(i=1; s[i] != 0 || s[i+1] != 0; i += 2) {
			cput(s[i]);
			cput(s[i+1]);
		}
		cput(0);
		cput(0);
		i++;
	}
	else {
		for(i=0; s[i]; i++)
			cput(s[i]);
		cput(0);
	}
	symsize += l + 1 + i + 1;

	if(debug['n']) {
		if(t == 'z' || t == 'Z') {
			Bprint(&bso, "%c %.8llux ", t, v);
			for(i=1; s[i] != 0 || s[i+1] != 0; i+=2) {
				f = ((s[i]&0xff) << 8) | (s[i+1]&0xff);
				Bprint(&bso, "/%x", f);
			}
			Bprint(&bso, "\n");
			return;
		}
		if(ver)
			Bprint(&bso, "%c %.8llux %s<%d>\n", t, v, s, ver);
		else
			Bprint(&bso, "%c %.8llux %s\n", t, v, s);
	}
}

#define	MINLC	2
void
asmlc(void)
{
	long oldpc, oldlc;
	Prog *p;
	long v, s;

	oldpc = 0;
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
			cput(s+128);	/* 129-255 +pc */
			if(debug['L'])
				Bprint(&bso, " pc+%ld*%d(%ld)", s, MINLC, s+128);
			v -= s;
			lcsize++;
		}
		s = p->line - oldlc;
		oldlc = p->line;
		oldpc = p->pc + MINLC;
		if(s > 64 || s < -64) {
			cput(0);	/* 0 vv +lc */
			cput(s>>24);
			cput(s>>16);
			cput(s>>8);
			cput(s);
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
			cput(0+s);	/* 1-64 +lc */
			if(debug['L']) {
				Bprint(&bso, " lc+%ld(%ld)\n", s, 0+s);
				Bprint(&bso, "%6lux %P\n",
					p->pc, p);
			}
		} else {
			cput(64-s);	/* 65-128 -lc */
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
		cput(s);
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
	vlong d;
	long l, fl, j;
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

		case D_VCONST:
			d = *p->to.vval;
			goto dconst;

		case D_CONST:
			d = p->to.offset;
		dconst:
			if(p->to.sym) {
				switch(p->to.sym->type) {
				case STEXT:
				case SLEAF:
				case SSTRING:
					d += (p->to.sym->value + INITTEXT);
					break;
				case SDATA:
				case SBSS:
					d += (p->to.sym->value + INITDAT);
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
			case 8:
				for(; i<c; i++) {
					buf.dbuf[l] = cast[inuxi8[i]];
					l++;
				}
				break;
			}
			break;
		}
		if(debug['d'] && i == c) {
			Bprint(&bso, "%.8llux", l+s+INITDAT-c);
			for(j = -c; j<0; j++)
				Bprint(&bso, " %.2ux", ((uchar*)buf.dbuf)[l + j]);
			Bprint(&bso, "\t%P\n", curp);
		}
	}
	write(cout, buf.dbuf, n);
}

#define R(r) ((r)&0x1F)
#define OPX	(0x3 | o->op<<2)
#define OPF (OPX | o->func3<<12)
#define OP_R(rs1,rs2,rd)\
	(OPF | rd<<7 | R(rs1)<<15 | R(rs2)<<20 | o->param<<25)
#define OP_RF(rs1,rs2,rd,rm)\
	(OPX | rm<<12 | rd<<7 | R(rs1)<<15 | R(rs2)<<20 | o->param<<25)
#define OP_RO(rs1,rs2,rd)\
	(0x3 | OOP<<2 | o->func3<<12 | rd<<7 | R(rs1)<<15 | R(rs2)<<20)
#define OP_ADD(rs1,rs2,rd)\
	(0x3 | OOP<<2 | rd<<7 | R(rs1)<<15 | R(rs2)<<20)
#define OP_I(rs1,rd,imm)\
	(OPF | rd<<7 | R(rs1)<<15 | (imm)<<20)
#define OP_FI(func3,rs1,rd,imm)\
	(OPX | func3<<12 | rd<<7 | R(rs1)<<15 | (imm)<<20)
#define OP_S(rs1,rs2,imm)\
	(OPF | (imm&0x1F)<<7 | R(rs1)<<15 | R(rs2)<<20 | (imm>>5)<<25)
#define OP_B(rs1,rs2,imm)\
	(OPF | R(rs1)<<15 | R(rs2)<<20 | (imm&0x800)>>4 | (imm&0x1E)<<7 | \
	 (imm&0x7E0)<<20 | (imm&0x1000)<<19)
#define OP_U(rd,imm)\
	(0x3 | OLUI<<2 | rd<<7 | (imm&0xFFFFF000))
#define OP_UP(rd,imm)\
	(0x3 | OAUIPC<<2 | rd<<7 | (imm&0xFFFFF000))
#define OP_J(rd,imm)\
	(OPX | rd<<7 | (imm&0xff000) | (imm&0x800)<<9 | (imm&0x7FE)<<20 | (imm&0x100000)<<11)

/*
 * aflag: 0 - assemble to object file
 *        1 - return assembled instruction
 *        2 - first pass, return length of assembled instruction
 *        3 - later pass, return length of assembled instruction
 */
int
asmout(Prog *p, Optab *o, int aflag)
{
	vlong vv;
	long o1, o2, o3, v;
	int r;

	o1 = 0;
	o2 = 0;
	o3 = 0;
	r = p->reg;
	if(r == NREG) switch(p->as){
		case AMOVF:
		case AMOVD:
			if(p->from.type == D_FREG && p->to.type == D_FREG)
				r = p->from.reg;
			break;
		case AMOV:
		case AJMP:
			r = REGZERO;
			break;
		case AJAL:
			r = REGLINK;
			break;
		default:
			r = p->to.reg;
			break;
	}
	if(!debug['c'] && o->ctype){
		o1 = asmcompressed(p, o, r, aflag == 2);
		if(o1 != 0){
			switch(aflag){
			case 1:
				return o1;
			case 2:
			case 3:
				return 2;
			}
			if(debug['a']){
				v = p->pc + INITTEXT;
				Bprint(&bso, " %.8lux: %.4lux    \t%P\n", v, o1, p);
			}
			wputl(o1);
			return 2;
		}
	}
	if(aflag >= 2)
		return o->size;
	switch(o->type) {
	default:
		diag("unknown type %d", o->type);
		if(!debug['a'])
			prasm(p);
		break;

	case 0:		/* add S,[R,]D */
		o1 = OP_R(r, p->from.reg, p->to.reg);
		break;

	case 1:		/* slli $I,[R,]D */
		v = p->from.offset & 0x3F;
		if(thechar != 'j')
			v &= 0x1F;
		v |= (o->param<<5);
		o1 = OP_I(r, p->to.reg, v);
		break;

	case 2:		/* addi $I,[R,]D */
		v = p->from.offset;
		if(v < -BIG || v >= BIG)
			diag("imm out of range\n%P", p);
		o1 = OP_I(r, p->to.reg, v);
		break;
	
	case 3:		/* beq S,[R,]L */
		if(r == NREG)
			r = REGZERO;
		if(p->cond == P)
			v = 0;
		else
			v = (p->cond->pc - pc);
		if(v < -0x1000 || v >= 0x1000)
			diag("branch out of range\n%P", p);
		o1 = OP_B(r, p->from.reg, v);
		break;

	case 4:		/* jal	[D,]L */
		if(p->cond == P)
			v = 0;
		else
			v = (p->cond->pc - pc);
		if(v < -0x100000 || v >= 0x100000)
			diag("jump out of range\n%P", p);
		o1 = OP_J(r, v);
		break;

	case 5:		/* jalr [D,]I(S) */
		v = regoff(&p->to);
		if(v < -BIG || v >= BIG)
			diag("imm out of range\n%P", p);
		o1 = OP_I(classreg(&p->to), r, v);
		break;

	case 6:		/* sb	R,I(S) */
		v = regoff(&p->to);
		r = classreg(&p->to);
		if(v < -BIG || v >= BIG)
			diag("imm out of range\n%P", p);
		o1 = OP_S(r, p->from.reg, v);
		break;

	case 7:		/* lb	I(S),D */
		v = regoff(&p->from);
		r = classreg(&p->from);
		if(v < -BIG || v >= BIG)
			diag("imm out of range\n%P", p);
		o1 = OP_I(r, p->to.reg, v);
		break;

	case 8:		/* lui	I,D */
		v = p->from.offset;
		o1 = OP_U(p->to.reg, v);
		break;

	case 9:		/* lui	I1,D; addi I0,D */
		v = regoff(&p->from);
		if(thechar == 'j' && v >= 0x7ffff800){
			/* awkward edge case */
			o1 = OP_U(p->to.reg, -v);
			v &= 0xFFF;
			o2 = OP_FI(4, p->to.reg, p->to.reg, v);	/* xori */
			break;
		}
		if(v & 0x800)
			v += 0x1000;
		o1 = OP_U(p->to.reg, v);
		v &= 0xFFF;
		o2 = OP_I(p->to.reg, p->to.reg, v);
		break;

	case 10:	/* sign extend */
		if(p->as == AMOVBU) {
			o1 = OP_I(p->from.reg, p->to.reg, o->param);
			break;
		}
		v = o->param;
		if(thechar == 'j')
			v += 32;
		o1 = OP_FI(1, p->from.reg, p->to.reg, v & 0x3F);	/* slli */
		o2 = OP_I(p->to.reg, p->to.reg, v);	/* srli or srai */
		break;

	case 11:		/* addi $I,R,D */
		v = regoff(&p->from);
		if(v < -BIG || v >= BIG)
			diag("imm out of range\n%P", p);
		o1 = OP_I(classreg(&p->from), p->to.reg, v);
		break;

	case 12:		/* mov r,lext => lui/auipc I1,T; sb r,I0(T) */
		v = regoff(&p->to);
		if(thechar == 'j'){
			vv = v + INITDAT + BIG - INITTEXT - pc;
			v = (long)vv;
			if(v != vv || (v&~0x7FF) == 0x7FFFF800)
				diag("address out of range\n%P", p);
		}else
			v += INITDAT + BIG;
		if(v & 0x800)
			v += 0x1000;
		o1 = thechar == 'j' ? OP_UP(REGTMP, v) : OP_U(REGTMP, v);
		v &= 0xFFF;
		o2 = OP_S(REGTMP, p->from.reg, v);
		break;

	case 13:		/* mov lext, r => lui/auipc I1,T; lb r,I0(T) */
		v = regoff(&p->from);
		if(thechar == 'j'){
			vv = v + INITDAT + BIG - INITTEXT - pc;
			v = (long)vv;
			if(v != vv || (v&~0x7FF) == 0x7FFFF800)
				diag("address out of range\n%P", p);
		}else
				v += INITDAT + BIG;
		if(v & 0x800)
			v += 0x1000;
		o1 = thechar == 'j' ? OP_UP(REGTMP, v) : OP_U(REGTMP, v);
		v &= 0xFFF;
		o2 = OP_I(REGTMP, p->to.reg, v);
		break;

	case 14:		/* op $lcon,[r,]d => lui L1,T; addi $L0,T,T; op T,r,d */
		v = regoff(&p->from);
		if(p->as == AMOVW || p->as == AMOV)
			r = classreg(&p->from);
		if(thechar == 'j' && v >= 0x7ffff800){
			/* awkward edge case */
			o1 = OP_U(p->to.reg, -v);
			v &= 0xFFF;
			o2 = OP_FI(4, p->to.reg, p->to.reg, v);	/* xori */
		}else{
			if(v & 0x800)
				v += 0x1000;
			o1 = OP_U(REGTMP, v);
			v &= 0xFFF;
			o2 = OP_FI(0, REGTMP, REGTMP, v);	/* addi */
		}
		o3 = OP_RO(r, REGTMP, p->to.reg);
		break;

	case 15:		/* mov r,L(s) => lui L1,T; add s,T,T; sb r,L0(T) */
		v = regoff(&p->to);
		r = classreg(&p->to);
		if(v & 0x800)
			v += 0x1000;
		o1 = OP_U(REGTMP, v);
		v &= 0xFFF;
		o2 = OP_ADD(r, REGTMP, REGTMP);
		o3 = OP_S(REGTMP, p->from.reg, v);
		break;

	case 16:		/* mov L(s),r => lui L1,T; add s,T,T; lb r,L0(T) */
		v = regoff(&p->from);
		r = classreg(&p->from);
		if(v & 0x800)
			v += 0x1000;
		o1 = OP_U(REGTMP, v);
		v &= 0xFFF;
		o2 = OP_ADD(r, REGTMP, REGTMP);
		o3 = OP_I(REGTMP, p->to.reg, v);
		break;

	case 17:	/* fcvt S,D */
		v = 7;	/* dynamic rounding mode */
		if(o->a3 == C_REG)	/* convert to int? */
			v = 1;	/* round towards zero */
		o1 = OP_RF(p->from.reg, o->func3, p->to.reg, v);
		break;

	case 18:	/* lui L1, T; jal [r,]L0(T) */
		if(p->cond == P)
			v = 0;
		else
			v = p->cond->pc;
		if(thechar == 'j'){
			vv = v + INITTEXT;
			v = (long)vv;
			if(v != vv || (v&~0x7FF) == 0x7FFFF800)
				diag("branch out of range\n%P", p);
		}else
			v += INITTEXT;
		if(v & 0x800)
			v += 0x1000;
		o1 = thechar == 'j' ? OP_UP(REGTMP, v) : OP_U(REGTMP, v);
		v &= 0xFFF;
		o2 = OP_I(REGTMP, r, v);
		break;

	case 19:	/* addiw $0, rs, rd */
		v = 0;
		o1 = OP_I(p->from.reg, p->to.reg, v);
		break;

	case 20:	/* lui/auipc I1,D; addi I0; D */
		if(thechar == 'j'){
			vv = regoff(&p->from) + instoffx - (pc + INITTEXT);
			v = (long)vv;
			if(v != vv || (v&~0x7FF) == 0x7FFFF800)
				diag("address %llux out of range\n%P", regoff(&p->from) + instoffx, p);
		}else{
			v = regoff(&p->from) + instoffx;
		}
		if(v & 0x800)
			v += 0x1000;
		o1 = thechar == 'j' ? OP_UP(p->to.reg, v) : OP_U(p->to.reg, v);
		v &= 0xFFF;
		o2 = OP_I(p->to.reg, p->to.reg, v);
		break;

	case 21:	/* lui I,D; s[lr]ai N,D */
		vv = *p->from.vval;
		v = vconshift(vv);
		if(v < 0)
			diag("64 bit constant error:\n%P", p);
		if(v < 12)
			vv <<= 12 - v;
		else
			vv >>= v - 12;
		o1 = OP_U(p->to.reg, (long)vv);
		if (v > 12)
			o2 = OP_FI(1, p->to.reg, p->to.reg, v - 12);	/* slli */
		else
			o2 = OP_I(p->to.reg, p->to.reg, 12 - v);	/* srai */
		break; 

	case 22:	/* CSRRx C, rs, rd */
		v = p->from.offset & 0xFFF;
		o1 = OP_I(p->reg, p->to.reg, v);
		break;

	case 24:		/* SYS */
		v = p->to.offset & 0xFFF;
		o1 = OP_I(0, 0, v);
		break;

	case 25:		/* word $x */
		o1 = regoff(&p->to);
		break;

	case 26:		/* pseudo ops */
		break;
	}
	if(aflag)
		return o1;
	v = p->pc + INITTEXT;
	switch(o->size) {
	default:
		if(debug['a'])
			Bprint(&bso, " %.8lux:\t\t%P\n", v, p);
		break;
	case 4:
		if(debug['a'])
			Bprint(&bso, " %.8lux: %.8lux\t%P\n", v, o1, p);
		lputl(o1);
		break;
	case 8:
		if(debug['a'])
			Bprint(&bso, " %.8lux: %.8lux %.8lux%P\n", v, o1, o2, p);
		lputl(o1);
		lputl(o2);
		break;
	case 12:
		if(debug['a'])
			Bprint(&bso, " %.8lux: %.8lux %.8lux %.8lux%P\n", v, o1, o2, o3, p);
		lputl(o1);
		lputl(o2);
		lputl(o3);
		break;
	}
	return o->size;
}

