#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#define Extern extern
#include "3210.h"

void	Aadd(int, int, ulong, ulong);
void	Aand(int, int, ulong, ulong);
void	Aandnot(int, int, ulong, ulong);
void	Abtst(int, int, ulong, ulong);
void	Acmp(int, int, ulong, ulong);
void	Acradd(int, int, ulong, ulong);
void	Aor(int, int, ulong, ulong);
void	Arol(int, int, ulong, ulong);
void	Aror(int, int, ulong, ulong);
void	Asll(int, int, ulong, ulong);
void	Asra(int, int, ulong, ulong);
void	Asrl(int, int, ulong, ulong);
void	Asub(int, int, ulong, ulong);
void	Asubrev(int, int, ulong, ulong);
void	Aundef(int, int, ulong, ulong);
void	Axor(int, int, ulong, ulong);
void	alufmt(char*, int, char*, char*, char*, char*, char*, char*);
void	revfmt(char*, int, char*, char*, char*, char*, char*, char*);
void	cmpfmt(char*, int, char*, char*, char*, char*, char*, char*);
void	rotfmt(char*, int, char*, char*, char*, char*, char*, char*);

Alu alutab[16] = {
	{Aadd,		alufmt,	"+"},
	{Asll,		alufmt,	"<<"},
	{Asubrev,	revfmt,	"-"},
	{Acradd,	alufmt,	"#"},
	{Asub,		alufmt,	"-"},
	{Aundef,	alufmt,	"op5"},
	{Aandnot,	alufmt,	"&~"},
	{Acmp,		cmpfmt,	"-"},
	{Axor,		alufmt,	"^"},
	{Aror,		rotfmt,	">>>"},
	{Aor,		alufmt,	"|"},
	{Arol,		rotfmt,	"<<<"},
	{Asrl,		alufmt,	">>"},
	{Asra,		alufmt,	"$>>"},
	{Aand,		alufmt,	"&"},
	{Abtst,		cmpfmt,	"&"},
};

void
alufmt(char *buf, int n, char *cc, char *rd, char *cast, char *rs, char *op, char *rt)
{
	snprint(buf, n, "%s%s = %s%s %s %s", cc, rd, cast, rs, op, rt);
}

void
rotfmt(char *buf, int n, char *cc, char *rd, char *cast, char *rs, char *op, char *rt)
{
	USED(rt);
	snprint(buf, n, "%s%s = %s%s %s 1", cc, rd, cast, rs, op);
}

void
cmpfmt(char *buf, int n, char *cc, char *rd, char *cast, char *rs, char *op, char *rt)
{
	USED(rd);
	snprint(buf, n, "%s%s%s %s %s", cc, cast, rs, op, rt);
}

void
revfmt(char *buf, int n, char *cc, char *rd, char *cast, char *rs, char *op, char *rt)
{
	snprint(buf, n, "%s%s = %s%s %s %s", cc, rd, cast, rt, op, rs);
}

/*
 * [if(cond)] rd = rs op rt
 */
void
Dcalu(char *buf, int n, ulong inst, ulong pc, int rvalid)
{
	Alu *a;

	USED(pc, rvalid);
	a = &alutab[aluop(inst)];
	(*a->fmt)(buf, n, condname(cond1(inst)),
		regname[reg3(inst)], alu32(inst) ? "" : "(short)",
		regname[reg2(inst)], a->name,
		regname[reg1(inst)]);
}

void
Icalu(ulong inst)
{
	Alu *a;
	ulong s, t;
	int rd, rs, rt, c, is32;

	a = &alutab[aluop(inst)];
	a->rcount++;
	c = condset(cond1(inst));
	rd = regno(reg3(inst));
	rs = reg2(inst);
	rt = reg1(inst);
	s = regval(rs, 0);
	t = regval(rt, 0);
	last(rs, rt);
	is32 = alu32(inst);
	if(c){
		a->taken++;
		(*a->func)(is32, rd, s, t);
	}
}

/*
 * rd = rd op imm
 */
void
Dalui(char *buf, int n, ulong inst, ulong pc, int rvalid)
{
	Alu *a;
	char *rd, imm[32];

	USED(pc, rvalid);
	a = &alutab[aluop(inst)];
	rd = regname[reg3(inst)];
	sprint(imm, "%X", imm16(inst));
	(*a->fmt)(buf, n, "", rd, alu32(inst) ? "" : "(short)", rd, a->name, imm);
}

void
Ialui(ulong inst)
{
	Alu *a;
	ulong d, imm;
	int rd, rno, is32;

	a = &alutab[aluop(inst)];
	a->icount++;
	rno = reg3(inst);
	rd = regno(rno);
	imm = imm16(inst);
	d = reg.r[rd];
	last(rno, 0);
	is32 = alu32(inst);
	(*a->func)(is32, rd, d, imm);
}

/*
 * rd = rs + imm
 */
void
Daddi(char *buf, int n, ulong inst, ulong pc, int rvalid)
{
	USED(pc, rvalid);
	snprint(buf, n, "%s = %s%s + %X", regname[reg4(inst)], alu32(inst) ? "" : "(short)",
			regname[reg3(inst)], imm16(inst));
}

void
Iaddi(ulong inst)
{
	ulong s, imm;
	int rd, rs, is32;

	rd = regno(reg4(inst));
	rs = reg3(inst);
	s = regval(rs, 0);
	imm = imm16(inst);
	last(rs, 0);
	is32 = alu32(inst);

	Aadd(is32, rd, s, imm);
}

/*
 * rd = rs + imm
 */
void
Dshiftor(char *buf, int n, ulong inst, ulong pc, int rvalid)
{
	ulong imm;
	char *rd;

	USED(pc, rvalid);
	rd = regname[reg4(inst)];
	imm = imm16(inst);
	snprint(buf, n, "%s = %s <<| %X", rd, regname[reg3(inst)], imm & 0xffff);
}

void
Ishiftor(ulong inst)
{
	ulong s, imm;
	int rd, rs;

	rd = regno(reg4(inst));
	rs = reg3(inst);
	s = regval(rs, 0);
	imm = imm16(inst);
	last(rs, 0);

	reg.r[rd] = s |= (imm << 16);
	cpuflags(s);
}

void
Aadd(int is32, int rd, ulong s, ulong t)
{
	ulong v;

	v = s + t;
	if(!is32 && v & 0x8000)
		v |= ~0xffff;
	reg.r[rd] = v;
	addflags(is32, v, s, t);
}

void
Acradd(int is32, int rd, ulong s, ulong t)
{
	ulong v;
	int c;

	c = 0;
	while(t){
		v = s ^ t;
		t = s & t;
		c |= t;
		t >>= 1;
		s = v;
	}
	if(!is32 && s & 0x8000)
		s |= ~0xffff;
	reg.r[rd] = s;
	cpuflags(s);
	if(c & 1)
		reg.c[PS] |= Carry;
}

void
Asub(int is32, int rd, ulong s, ulong t)
{
	ulong v;

	v = s - t;
	if(!is32 && v & 0x8000)
		v |= ~0xffff;
	reg.r[rd] = v;
	subflags(is32, v, s, t);
}

void
Asubrev(int is32, int rd, ulong s, ulong t)
{
	ulong v;

	v = t - s;
	if(!is32 && v & 0x8000)
		v |= ~0xffff;
	reg.r[rd] = v;
	subflags(is32, v, t, s);
}

void
Acmp(int is32, int rd, ulong s, ulong t)
{
	ulong v;

	USED(rd);
	v = s - t;
	if(!is32 && v & 0x8000)
		v |= ~0xffff;
	subflags(is32, v, s, t);
}

void
Aand(int is32, int rd, ulong s, ulong t)
{
	ulong v;

	v = s & t;
	if(!is32 && v & 0x8000)
		v |= ~0xffff;
	reg.r[rd] = v;
	cpuflags(v);
}

void
Abtst(int is32, int rd, ulong s, ulong t)
{
	ulong v;

	USED(rd);
	v = s & t;
	if(!is32 && v & 0x8000)
		v |= ~0xffff;
	cpuflags(v);
}

void
Aandnot(int is32, int rd, ulong s, ulong t)
{
	ulong v;

	v = s & ~t;
	if(!is32 && v & 0x8000)
		v |= ~0xffff;
	reg.r[rd] = v;
	cpuflags(v);
}

void
Aor(int is32, int rd, ulong s, ulong t)
{
	ulong v;

	v = s | t;
	if(!is32 && v & 0x8000)
		v |= ~0xffff;
	reg.r[rd] = v;
	cpuflags(v);
}

void
Axor(int is32, int rd, ulong s, ulong t)
{
	ulong v;

	v = s ^ t;
	if(!is32 && v & 0x8000)
		v |= ~0xffff;
	reg.r[rd] = v;
	cpuflags(v);
}

void
Asll(int is32, int rd, ulong s, ulong t)
{
	ulong v;

	t &= 31;
	v = s << t;
	if(!is32)
		v &= 0xffff;
	reg.r[rd] = v;
	cpuflags(v);
}

/*
 * truncate before shifting
 */
void
Asrl(int is32, int rd, ulong s, ulong t)
{
	ulong v;

	t &= 31;
	if(!is32)
		s &= 0xffff;
	v = s >> t;
	reg.r[rd] = v;
	cpuflags(v);
}

/*
 * sign extend before and after shifting
 */
void
Asra(int is32, int rd, ulong s, ulong t)
{
	ulong v;

	t &= 31;
	if(!is32 && s & 0x8000)
		s |= ~0xffff;
	v = s >> t;
	if(t && (s & 0x80000000))
		v |= ~((1 << (32 - t)) - 1);
	reg.r[rd] = v;
	cpuflags(v);
}

void
Arol(int is32, int rd, ulong s, ulong t)
{
	int c;

	USED(t);
	c = reg.c[PS] & Carry;
	Aadd(is32, rd, s, s);
	if(c)
		reg.r[rd] += 1;
}

void
Aror(int is32, int rd, ulong s, ulong t)
{
	ulong v;

	USED(t);
	if(!is32)
		s &= 0xffff;
	v = s >> 1;
	if(reg.c[PS] & Carry)
		v |= 1 << (is32 ? 31 : 15);
	if(!is32 && v & 0x8000)
		v |= ~0xffff;
	reg.r[rd] = v;
	cpuflags(v);
	if(s & 1)
		reg.c[PS] |= Carry;
}

void
Aundef(int is32, int rd, ulong s, ulong t)
{
	USED(is32, rd, s, t);
	error("reserved operation called");
}

/*
 * set n, z, c, v flags
 */
void
addflags(int is32, ulong d, ulong s, ulong t)
{
	cpuflags(d);

	/*
	 * adjust for 16 bit op
	 */
	if(!is32){
		d <<= 16;
		s <<= 16;
		t <<= 16;
	}
	if(d < s)
		reg.c[PS] |= Carry;
	d >>= 31;
	s >>= 31;
	t >>= 31;
	if(s == t && s != d)
		reg.c[PS] |= Over;
}

/*
 * set n, z, c, v flags
 */
void
subflags(int is32, ulong d, ulong s, ulong t)
{
	cpuflags(d);

	/*
	 * adjust for 16 bit op
	 */
	if(!is32){
		d <<= 16;
		s <<= 16;
		t <<= 16;
	}
	if(s < t)
		reg.c[PS] |= Carry;
	d >>= 31;
	s >>= 31;
	t >>= 31;
	if(s != t && s != d)
		reg.c[PS] |= Over;
}
