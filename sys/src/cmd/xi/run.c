#include <u.h>
#include <libc.h>
#include <bio.h>
#define Extern extern
#include <mach.h>
#include "3210.h"

char	*widthname[] =
{
	"byte",
	"char",
	"ushort",
	"short",
	"hbyte",
	"cast5",
	"cast6",
	"long"
};

int	widthsize[] = { 1, 1, 2, 2, 1, 0, 0, 4 };

Inst	biginst[]= {
	{Igoto,		Dgoto,	"goto",		Tgoto},
	{Iloadi,	Dloadi,	"set24",	Tset},
	{Icalli,	Dcalli,	"calli",	Tcalli},
	0,
};

void	(*ldfunc)(int, int, int, int);
ulong	ldsrc;
int	lddest, ldwidth;

static ulong	pc;

int
fmtins(char *buf, int n, int mem, ulong dot, ulong w)
{
	Inst *inst;
	int i;

	pc = ~0;
	if(mem){
		w = getmem_4(dot);
		pc = dot + 8;
	}

	i = (w >> 25) & 0x7f;
	if(i < 0x50)
		inst = &itab[i];
	else
		inst = &biginst[(i >> 4) - 5];

	(*inst->print)(buf, n, w, pc, 0);
	return 4;
}

ulong
run(void)
{
	Inst *inst;
	char buf[128];
	ulong npc, w;
	int i;

	for(; count > 0; count--){
		pc = reg.ip;
		reg.ir = w = ifetch(pc);
		npc = reg.pc;
		reg.pc += 4;
		reg.r[0] = 0;

		i = (w >> 25) & 0x7f;
		if(i < 0x50)
			inst = &itab[i];
		else
			inst = &biginst[(i >> 4) - 5];

		if(trace){
			Bprint(bioout, "%.8lux %.8lux ", pc, w);
			(*inst->print)(buf, sizeof buf, w, reg.pc, 1);
			Bprint(bioout, "%s\n", buf);
		}
		inst->count++;
		reg.inst = inst;
		(*inst->func)(w);
		reg.r[0] = 0;
		fpuop();
		if(reg.iter){
			if(reg.count == 0){
				reg.iter--;
				if(reg.iter){
					npc = reg.start;
					reg.pc = npc + BY2WD;
					reg.count = reg.size;
				}else
					lock = 0;
			}
			reg.count--;
		}
		reg.last = pc;
		reg.ip = npc;
		if(bplist)
			brkchk(reg.ip, Instruction);
	}
	return reg.ip;
}

void
resetld(void)
{
	ldfunc = 0;
	reg.last = 0;
}

/*
 * complete loads & fpu operations from the last cycle
 * return whether or not there was a dau y load
 */
int
last(int s, int t)
{
	s = regmap[s];
	t = regmap[t];
	fpuupdate(s, t, 0, 0);
	if(ldfunc){
		(*ldfunc)(s, t, 0, 0);
		ldfunc = 0;
	}
	return fpufetch();
}

int
ldlast(int s, int t, int u, int v)
{
	if(ldfunc){
		(*ldfunc)(s, t, u, v);
		ldfunc = 0;
	}
	return fpufetch();
}

void
Dundef(char *buf, int n, ulong inst, ulong pc, int rvalid)
{
	USED(pc, rvalid);
	snprint(buf, n, "reserved instruction %.8lux", inst);
}

void
Iundef(ulong inst)
{
	error("reserved instruction %.8lux", inst);
}

void
Isyscall(ulong inst)
{
	USED(inst);
	if(!syscalls){
		Iundef(inst);
		return;
	}
	reg.r[REGRET] = syscall(reg.r[REGRET], reg.r[REGSP] + 4);
	Bflush(bioout);
}

void
Dsyscall(char *buf, int n, ulong inst, ulong pc, int rvalid)
{
	if(!syscalls){
		Dundef(buf, n, inst, pc, rvalid);
		return;
	}
	snprint(buf, n, "syscall");
}

void
Dloadi(char *buf, int n, ulong inst, ulong pc, int rvalid)
{
	USED(pc, rvalid);
	snprint(buf, n, "%s = (ushort24)%X", regname[reg3(inst)], imm24(inst));
}

void
Iloadi(ulong inst)
{
	int rd;
	ulong v;

	rd = regno(reg3(inst));
	v = imm24(inst);
	last(0, 0);
	reg.r[rd] = v;
}

void
Dcall(char *buf, int n, ulong inst, ulong pc, int rvalid)
{
	char t[64];

	USED(rvalid);
	textname(t, reg3(inst), imm16(inst), pc);
	snprint(buf, n, "call %s (%s)", t, regname[reg4(inst)]);
}

void
Icall(ulong inst)
{
	ulong s, imm;
	int rd, rs;

	rd = regno(reg4(inst));
	rs = reg3(inst);
	s = regval(rs, 0);
	imm = imm16(inst);
	last(rs, 0);

	if(calltree)
		tracecall(s + imm);

	reg.r[rd] = reg.pc;
	reg.pc = s + imm;
}

void
Dcalli(char *buf, int n, ulong inst, ulong pc, int rvalid)
{
	char t[64];

	USED(rvalid);
	textname(t, 0, imm24(inst), pc);
	snprint(buf, n, "call %s (%s)", t, regname[reg3(inst)]);
}

void
Icalli(ulong inst)
{
	int rd;
	ulong imm;

	rd = regno(reg3(inst));
	imm = imm24(inst);
	last(0, 0);

	if(calltree)
		tracecall(imm);

	reg.r[rd] = reg.pc;
	reg.pc = imm;
}

void
Dcgoto(char *buf, int n, ulong inst, ulong pc, int rvalid)
{
	char t[64];

	USED(rvalid);
	textname(t, reg3(inst), imm16(inst), pc);
	if(inst == INOP)
		snprint(buf, n, "nop");
	else
		snprint(buf, n, "%sgoto %s", condname(cond2(inst)), t);
}

void
Icgoto(ulong inst)
{
	ulong npc;
	int rs, c;

	rs = reg3(inst);
	npc = regval(rs, 0) + imm16(inst);
	c = condset(cond2(inst));
	last(rs, 0);

	if(calltree && c && imm16(inst) == 0)
		traceret(npc, regmap[rs]);

	if(c){
		reg.pc = npc;
		reg.inst->taken++;
	}else if(inst == INOP){
		reg.inst->count--;
		nopcount++;
		mprof->nops++;
	}
}

void
Dgoto(char *buf, int n, ulong inst, ulong pc, int rvalid)
{
	char t[64];

	USED(rvalid);
	textname(t, reg3(inst), imm24(inst), pc);
	snprint(buf, n, "goto %s", t);
}

void
Igoto(ulong inst)
{
	ulong npc;
	int rs;

	rs = reg3(inst);
	npc = regval(rs, 0) + imm24(inst);
	last(rs, 0);

	if(calltree && imm24(inst) == 0)
		traceret(npc, regmap[rs]);

	reg.pc = npc;
}

void
Ddecgoto(char *buf, int n, ulong inst, ulong pc, int rvalid)
{
	char t[64];

	USED(rvalid);
	textname(t, reg3(inst), imm16(inst), pc);
	snprint(buf, n, "if(%s-- >= 0) goto %s", regname[reg4(inst)], t);
}

void
Idecgoto(ulong inst)
{
	ulong s, imm;
	long v;
	int rd, rno, rs;

	rno = reg4(inst);
	rd = regno(rno);
	rs = reg3(inst);
	s = regval(rs, 0);
	imm = imm16(inst);
	last(rno, rs);

	v = reg.r[rd];
	reg.r[rd] = v - 1;
	if(v >= 0){
		reg.pc = s + imm;
		reg.inst->taken++;
	}
}

void
Ddo(char *buf, int n, ulong inst, ulong pc, int rvalid)
{
	char *s;

	USED(pc, rvalid);
	switch((inst >> 23) & 3){
	case 0:
		s = "do";
		break;
	case 1:
		s = "doblock";
		break;
	case 2:
		s = "dolock";
		break;
	default:
		s = "do?";
		break;
	}
	snprint(buf, n, "%s %X,%s", s, imm7(inst), regname[reg1(inst)]);
}

void
Ido(ulong inst)
{
	ulong pc;
	int rs, n, iter;

	rs = reg1(inst);
	n = imm7(inst) + 1;
	iter = regval(rs, 0);
	last(rs, 0);

	if(reg.iter)
		error("nested do loops");
	if(islock(inst))
		lock = 1;
	reg.loop = reg.ip;
	pc = reg.ip + BY2WD;
	reg.start = pc;
	reg.size = n;
	reg.count = n;
	reg.iter = iter + 1;
}

void
Ddoi(char *buf, int n, ulong inst, ulong pc, int rvalid)
{
	char *s;

	USED(pc, rvalid);
	switch((inst >> 23) & 3){
	case 0:
		s = "do";
		break;
	case 1:
		s = "doblock";
		break;
	case 2:
		s = "dolock";
		break;
	default:
		s = "do?";
		break;
	}
	snprint(buf, n, "%s %X,%X", s, imm7(inst), imm11(inst));
}

void
Idoi(ulong inst)
{
	ulong pc;
	int n, iter;

	n = imm7(inst) + 1;
	iter = imm11(inst);
	last(0, 0);

	if(reg.iter)
		error("nested do loops");
	if(islock(inst))
		lock = 1;
	reg.loop = reg.ip;
	pc = reg.ip + BY2WD;
	reg.start = pc;
	reg.size = n;
	reg.count = n;
	reg.iter = iter + 1;
}

void
Dmovi(char *buf, int n, ulong inst, ulong pc, int rvalid)
{
	Symbol s;
	char *rd, t[64];
	ulong imm, x;
	int w, len;

	USED(pc, rvalid);
	imm = imm16(inst) & 0xffff;
	imm += dspmem;
	rd = regname[reg3(inst)];
	w = mvwid(inst);
	x = findsym(imm, CDATA, &s);
	if(x == 0 || imm - s.value >= 0x1000)
		sprint(t, "%X", imm);
	else{
		len = sprint(t, "%s", s.name);
		if(imm - s.value)
			sprint(buf+len, "+%X", imm - s.value);
	}
	if(tomem(inst))
		snprint(buf, n, "*%s = (%s)%s", t, widthname[w], rd);
	else
		snprint(buf, n, "%s = (%s)*%s", rd, widthname[w], t);
}

void
Imovi(ulong inst)
{
	ulong imm;
	int rd, rno, w, wasy;

	imm = imm16(inst) & 0xffff;
	imm += dspmem;
	rno = reg3(inst);
	rd = regno(rno);
	w = mvwid(inst);
	wasy = last(rno, 0);
	if(tomem(inst)){
		if(wasy)
			warn("store conflicts with previous instruction's floating fetch");
		store(reg.r[rd], imm, w);
	}else if(w == Wres1 || w == Wres2)
		error("illegal width %d in memory fetch", w);
	else{
		lddest = rd;
		ldsrc = imm;
		ldwidth = w;
		ldfunc = memfetch;
	}
}

void
Dmove(char *buf, int n, ulong inst, ulong pc, int rvalid)
{
	if(inst & 0x400)
		Dmovrio(buf, n, inst, pc, rvalid);
	else
		Dmovr(buf, n, inst, pc, rvalid);
}

void
Imove(ulong inst)
{
	if(inst & 0x400)
		Imovrio(inst);
	else
		Imovr(inst);
}

void
Dmovr(char *buf, int n, ulong inst, ulong pc, int rvalid)
{
	char *rd, *rp, *inc;
	int w;

	USED(pc, rvalid);
	rd = regname[reg3(inst)];
	rp = regname[reg2(inst)];
	w = mvwid(inst);
	inc = incname[reg1(inst)];
	if(tomem(inst))
		snprint(buf, n, "*%s%s = (%s)%s", rp, inc, widthname[w], rd);
	else
		snprint(buf, n, "%s = (%s)*%s%s", rd, widthname[w], rp, inc);
}

void
Imovr(ulong inst)
{
	ulong s, inc;
	int d, rd, rs, rt, w, wasy;

	rd = reg3(inst);
	rs = reg2(inst);
	rt = reg1(inst);
	d = regno(rd);
	s = regno(rs);
	w = mvwid(inst);
	inc = regval(rt, widthsize[w]);
	wasy = last(rd, rs);
	if(tomem(inst)){
		if(wasy)
			warn("store conflicts with previous instruction's floating fetch");
		store(reg.r[d], reg.r[s], w);
	}else if(w == Wres1 || w == Wres2)
		error("illegal width %d in memory fetch", w);
	else{
		lddest = d;
		ldsrc = reg.r[s];
		ldwidth = w;
		ldfunc = memfetch;
	}
	reg.r[s] += inc;
}

void
Dmovrio(char *buf, int n, ulong inst, ulong pc, int rvalid)
{
	char *rn, *in;
	int r, i, w;

	USED(pc, rvalid);
	r = reg3(inst);
	i = reg1(inst);
	w = mvwid(inst);
	if(i == 10 && r == 0 && tomem(inst)
	&& (w == Long || w == Short)){
		if(w == Long)
			snprint(buf, n, "waiti");
		else
			snprint(buf, n, "sftrst");
		return;
	}
	in = iorname[i];
	rn = regname[r];
	if(tomem(inst))
		snprint(buf, n, "%s = (%s)%s", in, widthname[w], rn);
	else
		snprint(buf, n, "%s = (%s)%s", rn, widthname[w], in);
}

void
Imovrio(ulong inst)
{
	error("move from reg to io unimplemented", inst);
}

void
Dmovio(char *buf, int n, ulong inst, ulong pc, int rvalid)
{
	char *rd, *rp, *inc;
	int w;

	USED(pc, rvalid);
	rd = iorname[reg3(inst)];
	rp = regname[reg2(inst)];
	w = mvwid(inst);
	inc = incname[reg1(inst)];
	if(tomem(inst))
		snprint(buf, n, "*%s%s = (%s)%s", rp, inc, widthname[w], rd);
	else
		snprint(buf, n, "%s = (%s)*%s%s", rd, widthname[w], rp, inc);
}

void
Imovio(ulong inst)
{
	error("move from memory to io unimplemented", inst);
}

void
memfetch(int s, int t, int u, int v)
{
	if(lddest == s || lddest == t || lddest == u || lddest == v)
		warn("r%d was loaded by the previous instruction", lddest);
	reg.r[lddest] = fetch(ldsrc, ldwidth);
}

ulong
fetch(ulong addr, int w)
{
	ulong v;

	iloads++;
	switch(w){
	case Char:
		v = getmem_b(addr);
		if(v & 0x80)
			v |= ~0xff;
		return v;
	case Uchar:
		return getmem_b(addr);
	case Hchar:
		return getmem_b(addr+1);
	case Short:
		v = getmem_h(addr);
		if(v & 0x8000)
			v |= ~0xffff;
		return v;
	case Ushort:
		return getmem_h(addr);
	case Long:
		return getmem_w(addr);
	}
	error("illegal width %d in memory fetch", w);
	return 0;
}

void
store(ulong v, ulong addr, int w)
{
	istores++;
	switch(w){
	case Char:
	case Uchar:
		putmem_b(addr, v);
		return;
	case Hchar:
		putmem_b(addr+1, v);
		return;
	case Short:
	case Ushort:
		putmem_h(addr, v);
		return;
	case Long:
		putmem_w(addr, v);
		return;
	}
	error("illegal width %d in memory store", w);
}

/*
 * set n, z flags, clear c, v flags
 */
void
cpuflags(ulong d)
{
	reg.c[CTR] = (reg.c[CTR] << 1) & 0x3e;
	reg.c[PS] &= ~(Neg|Zero|Carry|Over);
	if(!d)
		reg.c[PS] |= Zero;
	if((long)d < 0)
		reg.c[PS] |= Neg;
}

/*
 * decode number of a destination reg
 */
int
regno(int r)
{
	int m;

	m = regmap[r];
	if(m < 0)
		error("illegal register %x", r);
	return m;
}

/*
 * get the value for the reg field
 */
ulong
regval(int r, int inc)
{
	int m;

	m = regmap[r];
	if(m >= 0)
		return reg.r[m];
	if(r == 15)
		return reg.pc;
	if(r == 30)
		return reg.pcsh;
	if(r == 22)
		return inc ? -inc : -1;
	if(r == 23)
		return inc ? inc : 1;
	error("illegal register %x", r);
	return 0;
}

int regmap[] = 
{
	0,
	1,
	2,
	3,
	4,
	5,
	6,
	7,
	8,
	9,
	10,
	11,
	12,
	13,
	14,
	-1,
	-1,
	15,
	16,
	17,
	18,
	19,
	-1,
	-1,
	20,
	21,
	22,
	-1,
	-1,
	-1,
	-1,
	-1,
};

char *incname[] = 
{
	"",
	"++r1",
	"++r2",
	"++r3",
	"++r4",
	"++r5",
	"++r6",
	"++r7",
	"++r8",
	"++r9",
	"++r10",
	"++r11",
	"++r12",
	"++r13",
	"++r14",
	"++pc",
	"++r?",
	"++r15",
	"++r16",
	"++r17",
	"++r18",
	"++r19",
	"--",
	"++",
	"++r20",
	"++r21",
	"++r22",
	"++r?",
	"++r?",
	"++r?",
	"++pcsh",
	"++r?",
};

char *regname[] = 
{
	"r0",
	"r1",
	"r2",
	"r3",
	"r4",
	"r5",
	"r6",
	"r7",
	"r8",
	"r9",
	"r10",
	"r11",
	"r12",
	"r13",
	"r14",
	"pc",
	"r?16",
	"r15",
	"r16",
	"r17",
	"r18",
	"r19",
	"-1",
	"1",
	"r20",
	"r21",
	"r22",
	"r?27",
	"r?28",
	"r?29",
	"pcsh",
	"r?31",
};

char	iorsize[] = {
	Short,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	Short,
	-1,
	-1,
	-1,
	Short,
	-1,
	Char,
	Char,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
};

char	*iorname[] =
{
	"ps",
	"ior?",
	"ior?",
	"ior?",
	"ior?",
	"ior?",
	"ior?",
	"ior?",
	"emr",
	"ior?",
	"ior?",
	"ior?",
	"pcw",
	"ior?",
	"dauc",
	"ctr",
	"ior?",
	"ior?",
	"ior?",
	"ior?",
	"ior?",
	"ior?",
	"ior?",
	"ior?",
	"ior?",
	"ior?",
	"ior?",
	"ior?",
	"ior?",
	"ior?",
	"ior?",
	"ior?",
	"ior?",
};

/*
 * is condition code set?
 */
int
condset(int cond)
{
	switch(cond){
	case Cfalse:	return 0;
	case Ctrue:	return 1;
	case Cpl:	return !isneg();
	case Cmi:	return isneg();
	case Cne:	return !iszero();
	case Ceq:	return iszero();
	case Cvc:	return !isover();
	case Cvs:	return isover();
	case Ccc:	return !iscarry();
	case Ccs:	return iscarry();
	case Cge:	return !(isneg() ^ isover());
	case Clt:	return isneg() ^ isover();
	case Cgt:	return !(iszero() | (isneg() ^ isover()));
	case Cle:	return iszero() | (isneg() ^ isover());
	case Chi:	return !(iscarry() | iszero());
	case Cls:	return iscarry() | iszero();
	case Cauc:	return !isfunder();
	case Caus:	return isfunder();
	case Cage:	return !isfneg();
	case Calt:	return isfneg();
	case Cane:	return !isfzero();
	case Caeq:	return isfzero();
	case Cavc:	return !isfover();
	case Cavs:	return isfover();
	case Cagt:	return !(isfneg() | isfzero());
	case Cale:	return isfneg() | isfzero();
	default:
		error("illegal condition %lux", cond);
	}
	return 0;
}

char *
condname(int cond)
{
	switch(cond){
	case Cfalse:	return "if(false) ";
	case Ctrue:	return "";
	case Cpl:	return "if(pl) ";
	case Cmi:	return "if(mi) ";
	case Cne:	return "if(ne) ";
	case Ceq:	return "if(eq) ";
	case Cvc:	return "if(vc) ";
	case Cvs:	return "if(vs) ";
	case Ccc:	return "if(cc) ";
	case Ccs:	return "if(cs) ";
	case Cge:	return "if(ge) ";
	case Clt:	return "if(lt) ";
	case Cgt:	return "if(gt) ";
	case Cle:	return "if(le) ";
	case Chi:	return "if(hi) ";
	case Cls:	return "if(ls) ";
	case Cauc:	return "if(auc) ";
	case Caus:	return "if(aus) ";
	case Cage:	return "if(age) ";
	case Calt:	return "if(alt) ";
	case Cane:	return "if(ane) ";
	case Caeq:	return "if(aeq) ";
	case Cavc:	return "if(avc) ";
	case Cavs:	return "if(avs) ";
	case Cagt:	return "if(agt) ";
	case Cale:	return "if(ale) ";
	case Cibe:	return "if(ibe) ";
	case Cibf:	return "if(ibf) ";
	case Cobf:	return "if(obf) ";
	case Cobe:	return "if(obe) ";
	case Csyc:	return "if(syc) ";
	case Csys:	return "if(sys) ";
	case Cfbc:	return "if(fbc) ";
	case Cfbs:	return "if(fbs) ";
	case Cir0c:	return "if(ir0c) ";
	case Cir0s:	return "if(ir0s) ";
	case Cir1c:	return "if(ir1c) ";
	case Cir1s:	return "if(ir1s) ";
	default:	return "if ??? ";
	}
}

void
textname(char *buf, int r, long imm, ulong pc)
{
	Symbol s;
	ulong a;
	int n;

	a = 1;
	if(r == 0)
		a = imm;
	if(r == 15 && pc != ~0)	/* pc */
		a = imm + pc;
	if(a != 1 && a >= Btext && a < Etext && findsym(a, CTEXT, &s)){
		n = sprint(buf, "%s", s.name);
		if(a - s.value)
			sprint(buf+n, "+%X", a - s.value);
	}else
		sprint(buf, "%s + %X", regname[r], imm);
}
