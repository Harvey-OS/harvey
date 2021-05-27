#include <u.h>
#include <libc.h>
#include "dat.h"
#include "fns.h"

#define sign(s)	(1UL<<((s)-1))
#define mask(s) (sign(s)|(sign(s)-1))

static void
push(Iarg *sp, Iarg *a)
{
	Iarg *p;

	p = amem(sp->cpu, a->len, RSS, ar(sp));
	p->off -= a->len;
	p->off &= mask(sp->len*8);
	aw(p, ar(a));
	aw(sp, p->off);
}

static void
pop(Iarg *sp, Iarg *a)
{
	Iarg *p;

	p = amem(sp->cpu, a->len, RSS, ar(sp));
	aw(a, ar(p));
	aw(sp, p->off + a->len);
}

static void
jump(Iarg *to)
{
	Cpu *cpu;

	cpu = to->cpu;
	switch(to->atype){
	default:
		abort();
	case AMp:
		to = afar(to, 1, to->len);
	case AAp:
		cpu->reg[RCS] = to->seg;
	case AJb:
	case AJv:
		cpu->reg[RIP] = to->off;
		break;
	case AEv:
		cpu->reg[RIP] = ar(to);
		break;
	}
}

static void
opcall(Cpu *cpu, Inst *i)
{
	Iarg *sp;

	sp = areg(cpu, cpu->slen, RSP);
	switch(i->a1->atype){
	default:
		abort();
	case AAp:
	case AMp:
		push(sp, areg(cpu, i->olen, RCS));
	case AJv:
	case AEv:
		push(sp, areg(cpu, i->olen, RIP));
		break;
	}
	jump(i->a1);
}

static void
opint(Cpu *cpu, Inst *i)
{
	cpu->trap = ar(i->a1);
	longjmp(cpu->jmp, 1);
}

static void
opiret(Cpu *cpu, Inst *i)
{
	Iarg *sp;

	if(i->olen != 2)
		trap(cpu, EBADOP);
	sp = areg(cpu, cpu->slen, RSP);
	pop(sp, areg(cpu, 2, RIP));
	pop(sp, areg(cpu, 2, RCS));
	pop(sp, areg(cpu, 2, RFL));
}

static void
opret(Cpu *cpu, Inst *i)
{
	Iarg *sp;
	ulong c;

	sp = areg(cpu, cpu->slen, RSP);
	pop(sp, areg(cpu, i->olen, RIP));
	if(c = ar(i->a1))
		aw(sp, ar(sp) + c);
}

static void
opretf(Cpu *cpu, Inst *i)
{
	Iarg *sp;
	ulong c;

	sp = areg(cpu, cpu->slen, RSP);
	pop(sp, areg(cpu, i->olen, RIP));
	pop(sp, areg(cpu, i->olen, RCS));
	if(c = ar(i->a1))
		aw(sp, ar(sp) + c);
}

static void
openter(Cpu *cpu, Inst *i)
{
	Iarg *sp, *bp;
	ulong oframe, nframe;
	int j, n;

	sp = areg(cpu, cpu->slen, RSP);
	bp = areg(cpu, cpu->slen, RBP);
	push(sp, bp);
	oframe = ar(bp);
	nframe = ar(sp);
	n = ar(i->a2) % 32;
	if(n > 0){
		for(j=1; j<n; j++){
			aw(bp, oframe - i->olen*j);
			push(sp, bp);
		}
		push(sp, acon(cpu, i->olen, nframe));
	}
	aw(bp, nframe);
	aw(sp, nframe - ar(i->a1));
}

static void
opleave(Cpu *cpu, Inst *i)
{
	Iarg *sp;

	sp = areg(cpu, cpu->slen, RSP);
	aw(sp, ar(areg(cpu, cpu->slen, RBP)));
	pop(sp, areg(cpu, i->olen, RBP));
}

static void
oppush(Cpu *cpu, Inst *i)
{
	Iarg *sp;

	sp = areg(cpu, cpu->slen, RSP);
	if(i->a1->len == 1)	/* 0x6A push imm8 */
		push(sp, acon(cpu, i->olen, ar(i->a1)));
	else
		push(sp, i->a1);
}

static void
oppop(Cpu *cpu, Inst *i)
{
	pop(areg(cpu, cpu->slen, RSP), i->a1);
}

static void
oppusha(Cpu *cpu, Inst *i)
{
	Iarg *sp, *osp;

	sp = areg(cpu, cpu->slen, RSP);
	osp = acon(cpu, i->olen, ar(sp));
	push(sp, areg(cpu, i->olen, RAX));
	push(sp, areg(cpu, i->olen, RCX));
	push(sp, areg(cpu, i->olen, RDX));
	push(sp, areg(cpu, i->olen, RBX));
	push(sp, osp);
	push(sp, areg(cpu, i->olen, RBP));
	push(sp, areg(cpu, i->olen, RSI));
	push(sp, areg(cpu, i->olen, RDI));
}

static void
oppopa(Cpu *cpu, Inst *i)
{
	Iarg *sp;

	sp = areg(cpu, cpu->slen, RSP);
	pop(sp, areg(cpu, i->olen, RDI));
	pop(sp, areg(cpu, i->olen, RSI));
	pop(sp, areg(cpu, i->olen, RBP));
	pop(sp, areg(cpu, i->olen, RBX));	// RSP
	pop(sp, areg(cpu, i->olen, RBX));
	pop(sp, areg(cpu, i->olen, RDX));
	pop(sp, areg(cpu, i->olen, RCX));
	pop(sp, areg(cpu, i->olen, RAX));
}

static void
oppushf(Cpu *cpu, Inst *i)
{
	push(areg(cpu, cpu->slen, RSP), areg(cpu, i->olen, RFL));
}

static void
oppopf(Cpu *cpu, Inst *i)
{
	ulong *f, o;

	f = cpu->reg + RFL;
	o = *f;
	pop(areg(cpu, cpu->slen, RSP), areg(cpu, i->olen, RFL));
	*f &= ~(VM|RF);
	*f |= (o & (VM|RF));
}

static void
oplahf(Cpu *cpu, Inst *i)
{
	aw(i->a1, cpu->reg[RFL]);
}

static void
opsahf(Cpu *cpu, Inst *i)
{
	enum { MASK = SF|ZF|AF|PF|CF };
	ulong *f;

	f = cpu->reg + RFL;
	*f &= ~MASK;
	*f |= (ar(i->a1) & MASK);
}

static void
opcli(Cpu *cpu, Inst *)
{
	cpu->reg[RFL] &= ~IF;
}

static void
opsti(Cpu *cpu, Inst *)
{
	cpu->reg[RFL] |= IF;
}

static void
opcld(Cpu *cpu, Inst *)
{
	cpu->reg[RFL] &= ~DF;
}

static void
opstd(Cpu *cpu, Inst *)
{
	cpu->reg[RFL] |= DF;
}

static void
opclc(Cpu *cpu, Inst *)
{
	cpu->reg[RFL] &= ~CF;
}

static void
opstc(Cpu *cpu, Inst *)
{
	cpu->reg[RFL] |= CF;
}

static void
opcmc(Cpu *cpu, Inst *)
{
	cpu->reg[RFL] ^= CF;
}

static void
parity(ulong *f, ulong r)
{
	static ulong tab[8] = {
		0x96696996,
		0x69969669,
		0x69969669,
		0x96696996,
		0x69969669,
		0x96696996,
		0x96696996,
		0x69969669,
	};
	r &= 0xFF;
	if((tab[r/32] >> (r%32)) & 1)
		*f &= ~PF;
	else
		*f |= PF;
}

static ulong
test(ulong *f, long r, int s)
{
	*f &= ~(CF|SF|ZF|OF|PF);
	r &= mask(s);
	if(r == 0)
		*f |= ZF;
	if(r & sign(s))
		*f |= SF;
	parity(f, r);
	return r;
}

static void
opshl(Cpu *cpu, Inst *i)
{
	ulong *f, r, a, h;
	int s, n;

	if((n = ar(i->a2) & 31) == 0)
		return;
	s = i->a1->len*8;
	a = ar(i->a1);
	f = cpu->reg + RFL;
	r = test(f, a<<n, s);
	h = sign(s);
	aw(i->a1, r);
	if((a<<(n-1)) & h)
		*f |= CF;
	if(n == 1 && ((a^r) & h))
		*f |= OF;
}

static void
opshr(Cpu *cpu, Inst *i)
{
	ulong *f, a;
	int s, n;

	if((n = ar(i->a2) & 31) == 0)
		return;
	s = i->a1->len*8;
	a = ar(i->a1);
	f = cpu->reg + RFL;
	aw(i->a1, test(f, a>>n, s));
	if(a & sign(n))
		*f |= CF;
	if(n == 1 && (a & sign(s)))
		*f |= OF;
}

static void
opsar(Cpu *cpu, Inst *i)
{
	ulong *f;
	long a;
	int n;

	if((n = ar(i->a2) & 31) == 0)
		return;
	a = ars(i->a1);
	f = cpu->reg + RFL;
	aw(i->a1, test(f, a>>n, i->a1->len*8));
	if(a & sign(n))
		*f |= CF;
}

static void
opshld(Cpu *cpu, Inst *i)
{
	ulong *f, a;
	int s, n;

	if((n = ar(i->a3) & 31) == 0)
		return;
	s = i->a1->len*8;
	a = ar(i->a1);
	f = cpu->reg + RFL;
	aw(i->a1, test(f, (a<<n)|(ar(i->a2)>>(s-n)), s));
	if((a<<(n-1)) & sign(s))
		*f |= CF;
}

static void
opshrd(Cpu *cpu, Inst *i)
{
	ulong *f, a;
	int s, n;

	if((n = ar(i->a3) & 31) == 0)
		return;
	s = i->a1->len*8;
	a = ar(i->a1);
	f = cpu->reg + RFL;
	aw(i->a1, test(f, (a>>n)|(ar(i->a2)<<(s-n)), s));
	if(a & sign(n))
		*f |= CF;
}


static void
oprcl(Cpu *cpu, Inst *i)
{
	ulong *f, a, r;
	int s, n;

	s = i->a1->len*8;
	n = ar(i->a2) % (s+1);
	a = ar(i->a1);
	r = (a<<n) | ((a>>(s-n))>>1);
	f = cpu->reg + RFL;
	if(*f & CF)
		r |= sign(n);
	aw(i->a1, r);
	*f &= ~(CF|OF);
	if((a>>(s-n)) & 1)
		*f |= CF;
	if((a ^ r) & sign(s))
		*f |= OF;
	parity(f, r);
}

static void
oprcr(Cpu *cpu, Inst *i)
{
	ulong *f, a, r, h;
	int s, n;

	s = i->a1->len*8;
	n = ar(i->a2) % (s+1);
	a = ar(i->a1);
	h = a<<(s-n);
	r = (a>>n) | (h<<1);
	f = cpu->reg + RFL;
	if(*f & CF)
		r |= 1<<(s-n);
	aw(i->a1, r);
	*f &= ~(CF|OF);
	if(h & sign(s))
		*f |= CF;
	if((a ^ r) & sign(s))
		*f |= OF;
	parity(f, r);
}

static void
oprol(Cpu *cpu, Inst *i)
{
	ulong *f, a, r;
	int s, n;

	s = i->a1->len*8;
	n = ar(i->a2) & (s-1);
	a = ar(i->a1);
	r = (a<<n) | (a>>(s-n));
	f = cpu->reg + RFL;
	aw(i->a1, r);
	*f &= ~(CF|OF);
	if(r & 1)
		*f |= CF;
	if((a ^ r) & sign(s))
		*f |= OF;
	parity(f, r);
}

static void
opror(Cpu *cpu, Inst *i)
{
	ulong *f, a, r;
	int s, n;

	s = i->a1->len*8;
	n = ar(i->a2) & (s-1);
	a = ar(i->a1);
	r = (a>>n) | (a<<(s-n));
	aw(i->a1, r);
	f = cpu->reg + RFL;
	*f &= ~(CF|OF);
	if(r & sign(s))
		*f |= CF;
	if((a ^ r) & sign(s))
		*f |= OF;
	parity(f, r);
}

static void
opbittest(Cpu *cpu, Inst *i)
{
	ulong a, m;
	int n, s;
	Iarg *x;

	n = ar(i->a2);
	x = i->a1;
	s = x->len*8;
	if(x->tag == TMEM){
		x = adup(x);
		x->off += (n / s) * x->len;
		x->off &= mask(i->alen*8);
	}
	a = ar(x);
	n &= s-1;
	m = 1<<n;
	if(a & m)
		cpu->reg[RFL] |= CF;			
	else
		cpu->reg[RFL] &= ~CF;
	switch(i->op){
	case OBT:
		break;
	case OBTS:
		aw(x, a | m);
		break;
	case OBTR:
		aw(x, a & ~m);
		break;
	case OBTC:
		aw(x, a ^ m);
		break;
	}
}

static void
opbitscan(Cpu *cpu, Inst *i)
{
	ulong a;

	if((a = ar(i->a2)) == 0)
		cpu->reg[RFL] |= ZF;
	else {
		int j;

		if(i->op == OBSF){
			for(j = 0; (a & (1<<j)) == 0; j++)
				;
		} else {
			for(j = i->a2->len*8-1; (a & (1<<j)) == 0; j--)
				;
		}
		aw(i->a1, j);
		cpu->reg[RFL] &= ~ZF;
	}
}

static void
opand(Cpu *cpu, Inst *i)
{
	aw(i->a1, test(cpu->reg + RFL, ars(i->a1) & ars(i->a2), i->a1->len*8));
}

static void
opor(Cpu *cpu, Inst *i)
{
	aw(i->a1, test(cpu->reg + RFL, ars(i->a1) | ars(i->a2), i->a1->len*8));
}

static void
opxor(Cpu *cpu, Inst *i)
{
	aw(i->a1, test(cpu->reg + RFL, ars(i->a1) ^ ars(i->a2), i->a1->len*8));
}

static void
opnot(Cpu *, Inst *i)
{
	aw(i->a1, ~ar(i->a1));
}

static void
optest(Cpu *cpu, Inst *i)
{
	test(cpu->reg + RFL, ars(i->a1) & ars(i->a2), i->a1->len*8);
}

static ulong
add(ulong *f, long a, long b, int c, int s)
{
	ulong r, cc, m, n;

	*f &= ~(AF|CF|SF|ZF|OF|PF);

	n = sign(s);
	m = mask(s);
	r = a + b + c;
	r &= m;
	if(r == 0)
		*f |= ZF;
	if(r & n)
		*f |= SF;
	cc = (a & b) | (~r & (a | b));
	if(cc & n)
		*f |= CF;
	if((cc ^ (cc >> 1)) & (n>>1))
		*f |= OF;
	parity(f, r);
	return r;
}

static ulong
sub(ulong *f, long a, long b, int c, int s)
{
	ulong r, bc, n;

	*f &= ~(AF|CF|SF|ZF|OF|PF);

	r = a - b - c;
	n = sign(s);
	r &= mask(s);
	if(r == 0)
		*f |= ZF;
	if(r & n)
		*f |= SF;
	a = ~a;
	bc = (a & b) | (r & (a | b));
	if(bc & n)
		*f |= CF;
	if((bc ^ (bc >> 1)) & (n>>1))
		*f |= OF;
	parity(f, r);
	return r;
}

static void
opadd(Cpu *cpu, Inst *i)
{
	aw(i->a1, add(cpu->reg + RFL, ars(i->a1), ars(i->a2), 0, i->a1->len*8));
}

static void
opadc(Cpu *cpu, Inst *i)
{
	ulong *f = cpu->reg + RFL;

	aw(i->a1, add(f, ars(i->a1), ars(i->a2), (*f & CF) != 0, i->a1->len*8));
}

static void
opsub(Cpu *cpu, Inst *i)
{
	aw(i->a1, sub(cpu->reg + RFL, ars(i->a1), ars(i->a2), 0, i->a1->len*8));
}

static void
opsbb(Cpu *cpu, Inst *i)
{
	ulong *f = cpu->reg + RFL;

	aw(i->a1, sub(f, ars(i->a1), ars(i->a2), (*f & CF) != 0, i->a1->len*8));
}

static void
opneg(Cpu *cpu, Inst *i)
{
	aw(i->a1, sub(cpu->reg + RFL, 0, ars(i->a1), 0, i->a1->len*8));
}

static void
opcmp(Cpu *cpu, Inst *i)
{
	sub(cpu->reg + RFL, ars(i->a1), ars(i->a2), 0, i->a1->len*8);
}

static void
opinc(Cpu *cpu, Inst *i)
{
	ulong *f, o;

	f = cpu->reg + RFL;
	o = *f;
	aw(i->a1, add(f, ars(i->a1), 1, 0, i->a1->len*8));
	*f = (*f & ~CF) | (o & CF);
}

static void
opdec(Cpu *cpu, Inst *i)
{
	ulong *f, o;

	f = cpu->reg + RFL;
	o = *f;
	aw(i->a1, sub(f, ars(i->a1), 1, 0, i->a1->len*8));
	*f = (*f & ~CF) | (o & CF);
}

static void
opmul(Cpu *cpu, Inst *i)
{
	Iarg *la, *ha;
	ulong l, h, m;
	uvlong r;
	int s;

	s = i->a2->len*8;
	m = mask(s);
	r = ar(i->a2) * ar(i->a3);
	l = r & m;
	h = (r >> s) & m;
	if(i->a1->atype != AAX)
		abort();
	la = areg(cpu, i->a2->len, RAX);
	if(s == 8){
		ha = adup(la);
		ha->tag |= TH;
	} else
		ha = areg(cpu, i->a2->len, RDX);
	aw(la, l);
	aw(ha, h);
	if(h)
		cpu->reg[RFL] |= (CF|OF);
	else
		cpu->reg[RFL] &= ~(CF|OF);
}

static void
opimul(Cpu *cpu, Inst *i)
{
	ulong l, h, m, n;
	vlong r;
	int s;

	s = i->a2->len*8;
	m = mask(s);
	n = sign(s);
	r = ars(i->a2) * ars(i->a3);
	h = (r >> s) & m;
	l = r & m;
	if(i->a1->atype == AAX){
		Iarg *la, *ha;

		la = areg(cpu, i->a2->len, RAX);
		if(s == 8){
			ha = adup(la);
			ha->tag |= TH;
		}else
			ha = areg(cpu, i->a2->len, RDX);
		aw(la, l);
		aw(ha, h);
	} else
		aw(i->a1, l);
	if((r > (vlong)n-1) || (r < -(vlong)n))
		cpu->reg[RFL] |= (CF|OF);
	else
		cpu->reg[RFL] &= ~(CF|OF);
}

static void
opdiv(Cpu *cpu, Inst *i)
{
	Iarg *qa, *ra;
	uvlong n, q;
	ulong m, r, d;
	int s;

	s = i->a1->len*8;
	m = mask(s);
	d = ar(i->a1);
	if(d == 0)
		trap(cpu, EDIV0);
	if(s == 8){
		qa = areg(cpu, 1, RAX);
		ra = adup(qa);
		ra->tag |= TH;
	} else {
		qa = areg(cpu, i->olen, RAX);
		ra = areg(cpu, i->olen, RDX);
	}
	n = (uvlong)ar(ra)<<s | (uvlong)ar(qa);
	q = n/d;
	if(q > m)
		trap(cpu, EDIV0);
	r = n%d;
	aw(ra, r);
	aw(qa, q);
}

static void
opidiv(Cpu *cpu, Inst *i)
{
	Iarg *qa, *ra;
	vlong n, q, min, max;
	long r, d;
	int s;

	s = i->a1->len*8;
	d = ars(i->a1);
	if(d == 0)
		trap(cpu, EDIV0);
	if(s == 8){
		qa = areg(cpu, 1, RAX);
		ra = adup(qa);
		ra->tag |= TH;
	} else {
		qa = areg(cpu, i->olen, RAX);
		ra = areg(cpu, i->olen, RDX);
	}
	n = (vlong)ars(ra)<<s | (uvlong)ars(qa);
	q = n/d;
	r = n%d;

	max = sign(s)-1;
	min = ~max;

	if(q > max || q < min)
		trap(cpu, EDIV0);

	aw(ra, r);
	aw(qa, q);
}	

static int
cctrue(Cpu *cpu, Inst *i)
{
	enum { SO = 1<<16,	/* pseudo-flag SF != OF */ };
	static ulong test[] = {
		OF,	/* JO, JNO */
		CF,	/* JC, JNC */
		ZF,	/* JZ, JNZ */
		CF|ZF,	/* JBE,JA  */
		SF,	/* JS, JNS */
		PF,	/* JP, JNP */
		SO,	/* JL, JGE */
		SO|ZF,	/* JLE,JG  */
	};
	ulong f, t;
	uchar c;

	c = i->code;
	switch(c){
	case 0xE3:	/* JCXZ */
		return ar(areg(cpu, i->alen, RCX)) == 0;
	case 0xEB:	/* JMP */
	case 0xE9:
	case 0xEA:
	case 0xFF:
		return 1;
	default:
		f = cpu->reg[RFL];
		if(((f&SF)!=0) ^ ((f&OF)!=0))
			f |= SO;
		t = test[(c>>1)&7];
		return ((t&f) != 0) ^ (c&1);
	}
}

static void
opjump(Cpu *cpu, Inst *i)
{
	if(cctrue(cpu, i))
		jump(i->a1);
}

static void
opset(Cpu *cpu, Inst *i)
{
	aw(i->a1, cctrue(cpu, i));
}

static void
oploop(Cpu *cpu, Inst *i)
{
	Iarg *cx;
	ulong c;

	switch(i->op){
	default:
		abort();
	case OLOOPNZ:
		if(cpu->reg[RFL] & ZF)
			return;
		break;
	case OLOOPZ:
		if((cpu->reg[RFL] & ZF) == 0)
			return;
		break;
	case OLOOP:
		break;
	}
	cx = areg(cpu, i->alen, RCX);
	c = ar(cx) - 1;
	aw(cx, c);
	if(c)
		jump(i->a1);
}

static void
oplea(Cpu *, Inst *i)
{
	aw(i->a1, i->a2->off);
}

static void
opmov(Cpu *, Inst *i)
{
	aw(i->a1, ar(i->a2));
}

static void
opcbw(Cpu *cpu, Inst *i)
{
	aw(areg(cpu, i->olen, RAX), ars(areg(cpu, i->olen>>1, RAX)));
}

static void
opcwd(Cpu *cpu, Inst *i)
{
	aw(areg(cpu, i->olen, RDX), ars(areg(cpu, i->olen, RAX))>>(i->olen*8-1));
}

static void
opmovsx(Cpu *, Inst *i)
{
	aw(i->a1, ars(i->a2));
}

static void
opxchg(Cpu *, Inst *i)
{
	ulong x;

	x = ar(i->a1);
	aw(i->a1, ar(i->a2));
	aw(i->a2, x);
}

static void
oplfp(Cpu *, Inst *i)
{
	Iarg *p;

	p = afar(i->a3, i->olen, i->olen);
	aw(i->a1, p->seg);
	aw(i->a2, p->off);
}

static void
opbound(Cpu *cpu, Inst *i)
{
	ulong p, s, e;

	p = ar(i->a1);
	s = ar(i->a2);
	e = ar(i->a3);
	if((p < s) || (p >= e))
		trap(cpu, EBOUND);
}

static void
opxlat(Cpu *cpu, Inst *i)
{
	aw(i->a1, ar(amem(cpu, i->a1->len, i->sreg, ar(areg(cpu, i->alen, i->a2->reg)) + ar(i->a1))));
}

static void
opcpuid(Cpu *cpu, Inst *)
{
	static struct {
		ulong level;

		ulong ax;
		ulong bx;
		ulong cx;
		ulong dx;
	} tab[] = {
		0,
			5,
			0x756e6547, /* Genu */
			0x6c65746e, /* ntel */
			0x49656e69, /* ineI */
		1,
			4<<8,
			0x00000000,
			0x00000000,
			0x00000000,
		2,
			0x00000000,
			0x00000000,
			0x00000000,
			0x00000000,
		3,
			0x00000000,
			0x00000000,
			0x00000000,
			0x00000000,
	};

	int i;

	for(i=0; i<nelem(tab); i++){
		if(tab[i].level == cpu->reg[RAX]){
			cpu->reg[RAX] = tab[i].ax;
			cpu->reg[RBX] = tab[i].bx;
			cpu->reg[RCX] = tab[i].cx;
			cpu->reg[RDX] = tab[i].dx;
			return;
		}
	}
	trap(cpu, EBADOP);
}

static void
opmovs(Cpu *cpu, Inst *i)
{
	Iarg *cx, *d, *s;
	ulong c, m;
	int n;

	m = mask(i->alen*8);
	d = adup(i->a1);
	s = adup(i->a2);
	n = s->len;
	if(cpu->reg[RFL] & DF)
		n = -n;
	if(i->rep){
		cx = areg(cpu, i->alen, RCX);
		c = ar(cx);
	} else {
		cx = nil;
		c = 1;
	}
	while(c){
		aw(d, ar(s));
		d->off += n;
		d->off &= m;
		s->off += n;
		s->off &= m;
		c--;
	}
	aw(areg(cpu, i->alen, RDI), d->off);
	aw(areg(cpu, i->alen, RSI), s->off);
	if(cx)
		aw(cx, 0);
}

static void
oplods(Cpu *cpu, Inst *i)
{
	Iarg *cx, *s;
	ulong c, m;
	int n;

	m = mask(i->alen*8);
	s = adup(i->a2);
	n = s->len;
	if(cpu->reg[RFL] & DF)
		n = -n;
	if(i->rep){
		cx = areg(cpu, i->alen, RCX);
		c = ar(cx);
	} else {
		cx = nil;
		c = 1;
	}
	if(c){
		s->off += n*(c-1);
		s->off &= m;
		aw(i->a1, ar(s));
		s->off += n;
		s->off &= m;
	}
	aw(areg(cpu, i->alen, RSI), s->off);
	if(cx)
		aw(cx, 0);
}

static void
opstos(Cpu *cpu, Inst *i)
{
	Iarg *cx, *d;
	ulong c, a, m;
	int n;

	m = mask(i->alen*8);
	d = adup(i->a1);
	n = d->len;
	if(cpu->reg[RFL] & DF)
		n = -n;
	if(i->rep){
		cx = areg(cpu, i->alen, RCX);
		c = ar(cx);
	} else {
		cx = nil;
		c = 1;
	}
	a = ar(i->a2);
	while(c){
		aw(d, a);
		d->off += n;
		d->off &= m;
		c--;
	}
	aw(areg(cpu, i->alen, RDI), d->off);
	if(cx)
		aw(cx, c);
}

static int
repcond(ulong *f, int rep)
{
	if(rep == OREPNE)
		return (*f & ZF) == 0;
	return (*f & ZF) != 0;
}

static void
opscas(Cpu *cpu, Inst *i)
{
	Iarg *cx, *d;
	ulong *f, c, m;
	long a;
	int n, z;

	m = mask(i->alen*8);
	d = adup(i->a1);
	n = d->len;
	z = n*8;
	f = cpu->reg + RFL;
	if(*f & DF)
		n = -n;
	if(i->rep){
		cx = areg(cpu, i->alen, RCX);
		c = ar(cx);
	} else {
		cx = nil;
		c = 1;
	}
	a = ars(i->a2);
	while(c){
		sub(f, a, ars(d), 0, z);
		d->off += n;
		d->off &= m;
		c--;
		if(!repcond(f, i->rep))
			break;
	}
	aw(areg(cpu, i->alen, RDI), d->off);
	if(cx)
		aw(cx, c);
}

static void
opcmps(Cpu *cpu, Inst *i)
{
	Iarg *cx, *s, *d;
	ulong *f, c, m;
	int n, z;

	m = mask(i->alen*8);
	d = adup(i->a1);
	s = adup(i->a2);
	n = s->len;
	z = n*8;
	f = cpu->reg + RFL;
	if(*f & DF)
		n = -n;
	if(i->rep){
		cx = areg(cpu, i->alen, RCX);
		c = ar(cx);
	} else {
		cx = nil;
		c = 1;
	}
	while(c){
		sub(f, ars(s), ars(d), 0, z);
		s->off += n;
		s->off &= m;
		d->off += n;
		d->off &= m;
		c--;
		if(!repcond(f, i->rep))
			break;
	}
	aw(areg(cpu, i->alen, RDI), d->off);
	aw(areg(cpu, i->alen, RSI), s->off);
	if(cx)
		aw(cx, c);
}

static void
opin(Cpu *cpu, Inst *i)
{
	Bus *io;

	io = cpu->port;
	aw(i->a1, io->r(io->aux, ar(i->a2) & 0xFFFF, i->a1->len));
}

static void
opout(Cpu *cpu, Inst *i)
{
	Bus *io;

	io = cpu->port;
	io->w(io->aux, ar(i->a1) & 0xFFFF, ar(i->a2), i->a2->len);
}

static void
opnop(Cpu *, Inst *)
{
}

static void
ophlt(Cpu *cpu, Inst *)
{
	trap(cpu, EHALT);
}

static void (*exctab[NUMOP])(Cpu *cpu, Inst*) = {
	[OINT] = opint,
	[OIRET] = opiret,

	[OCALL] = opcall,
	[OJUMP] = opjump,
	[OSET] = opset,

	[OLOOP] = oploop,
	[OLOOPZ] = oploop,
	[OLOOPNZ] = oploop,

	[ORET] = opret,
	[ORETF] = opretf,

	[OENTER] = openter,
	[OLEAVE] = opleave,

	[OPUSH] = oppush,
	[OPOP] = oppop,

	[OPUSHF] = oppushf,
	[OPOPF] = oppopf,
	[OLAHF] = oplahf,
	[OSAHF] = opsahf,

	[OPUSHA] = oppusha,
	[OPOPA] = oppopa,

	[OCLI] = opcli,
	[OSTI] = opsti,
	[OCLC] = opclc,
	[OSTC] = opstc,
	[OCMC] = opcmc,
	[OCLD] = opcld,
	[OSTD] = opstd,

	[OSHL] = opshl,
	[OSHR] = opshr,
	[OSAR] = opsar,

	[OSHLD] = opshld,
	[OSHRD] = opshrd,

	[ORCL] = oprcl,
	[ORCR] = oprcr,
	[OROL] = oprol,
	[OROR] = opror,

	[OBT] = opbittest,
	[OBTS] = opbittest,
	[OBTR] = opbittest,
	[OBTC] = opbittest,

	[OBSF] = opbitscan,
	[OBSR] = opbitscan,

	[OAND] = opand,
	[OOR] = opor,
	[OXOR] = opxor,
	[ONOT] = opnot,
	[OTEST] = optest,

	[OADD] = opadd,
	[OADC] = opadc,
	[OSUB] = opsub,
	[OSBB] = opsbb,
	[ONEG] = opneg,
	[OCMP] = opcmp,

	[OINC] = opinc,
	[ODEC] = opdec,

	[OMUL] = opmul,
	[OIMUL] = opimul,
	[ODIV] = opdiv,
	[OIDIV] = opidiv,

	[OLEA] = oplea,
	[OMOV] = opmov,
	[OCBW] = opcbw,
	[OCWD] = opcwd,
	[OMOVZX] = opmov,
	[OMOVSX] = opmovsx,
	[OXCHG] = opxchg,
	[OLFP] = oplfp,
	[OBOUND] = opbound,
	[OXLAT] = opxlat,

	[OCPUID] = opcpuid,

	[OMOVS] = opmovs,
	[OLODS] = oplods,
	[OSTOS] = opstos,
	[OSCAS] = opscas,
	[OCMPS] = opcmps,

	[OIN] = opin,
	[OOUT] = opout,

	[ONOP] = opnop,
	[OHLT] = ophlt,
};

void
trap(Cpu *cpu, int e)
{
	cpu->reg[RIP] = cpu->oldip;
	cpu->trap = e;
	longjmp(cpu->jmp, 1);
}

int
intr(Cpu *cpu, int v)
{
	Iarg *sp, *ip, *cs, *iv;

	if(v < 0 || v > 0xff || cpu->olen != 2)
		return -1;

	sp = areg(cpu, cpu->slen, RSP);
	cs = areg(cpu, 2, RCS);
	ip = areg(cpu, 2, RIP);

	iv = amem(cpu, 2, R0S, v * 4);

	push(sp, areg(cpu, 2, RFL));
	push(sp, cs);
	push(sp, ip);

	cpu->reg[RIP] = ar(iv);
	iv->off += 2;
	cpu->reg[RCS] = ar(iv);
	return 0;
}

int
xec(Cpu *cpu, int n)
{
	if(setjmp(cpu->jmp))
		return cpu->trap;
	while(n--){
		void (*f)(Cpu *, Inst *);
		Iarg *ip;
		Inst i;

		cpu->ic++;

		ip = amem(cpu, 1, RCS, cpu->oldip = cpu->reg[RIP]);
		decode(ip, &i);
		cpu->reg[RIP] = ip->off;
		if((f = exctab[i.op]) == nil)
			trap(cpu, EBADOP);
		f(cpu, &i);
	}
	return n;
}
