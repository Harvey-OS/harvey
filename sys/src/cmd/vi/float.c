#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#define Extern extern
#include "mips.h"

void	unimp(ulong);
void	Ifcmp(ulong);
void	Ifdiv(ulong);
void	Ifmul(ulong);
void	Ifadd(ulong);
void	Ifsub(ulong);
void	Ifmov(ulong);
void	Icvtd(ulong);
void	Icvtw(ulong);
void	Icvts(ulong);
void	Ifabs(ulong);
void	Ifneg(ulong);

Inst cop1[] = {
	{ Ifadd,	"add.f", Ifloat },
	{ Ifsub,	"sub.f", Ifloat },
	{ Ifmul,	"mul.f", Ifloat },
	{ Ifdiv,	"div.f", Ifloat },
	{ unimp,	"", },
	{ Ifabs,	"abs.f", Ifloat },
	{ Ifmov,	"mov.f", Ifloat },
	{ Ifneg,	"neg.f", Ifloat },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ Icvts,	"cvt.s", Ifloat },
	{ Icvtd,	"cvt.d", Ifloat },
	{ unimp,	"", },
	{ unimp,	"", },
	{ Icvtw,	"cvt.w", Ifloat },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ Ifcmp,	"c.f",	 Ifloat },
	{ Ifcmp,	"c.un",  Ifloat },
	{ Ifcmp,	"c.eq",  Ifloat },
	{ Ifcmp,	"c.ueq", Ifloat },
	{ Ifcmp,	"c.olt", Ifloat },
	{ Ifcmp,	"c.ult", Ifloat },
	{ Ifcmp,	"c.ole", Ifloat },
	{ Ifcmp,	"c.ule", Ifloat },
	{ Ifcmp,	"c,sf",  Ifloat },
	{ Ifcmp,	"c.ngle",Ifloat },
	{ Ifcmp,	"c.seq", Ifloat },
	{ Ifcmp,	"c.ngl", Ifloat },
	{ Ifcmp,	"c.lt",  Ifloat },
	{ Ifcmp,	"c.nge", Ifloat },
	{ Ifcmp,	"c.le",  Ifloat },
	{ Ifcmp,	"c.ngt", Ifloat },
	{ 0 }
};

void
unimp(ulong inst)
{
	print("op %ld\n", inst&0x3f);
	Bprint(bioout, "Unimplemented floating point Trap IR %.8lux\n", inst);
	longjmp(errjmp, 0);
}

void
inval(ulong inst)
{
	Bprint(bioout, "Invalid Operation Exception IR %.8lux\n", inst);
	longjmp(errjmp, 0);
}

void
ifmt(int r)
{
	Bprint(bioout, "Invalid Floating Data Format f%d pc 0x%lux\n", r, reg.pc);
	longjmp(errjmp, 0);
}

void
floatop(int dst, int s1, int s2)
{
	if(reg.ft[s1] == FPd && s1 != 24)
		ifmt(s1);
	if(reg.ft[s2] == FPd && s2 != 24)
		ifmt(s2);
	reg.ft[dst] = FPs;
}

void
doubop(int dst, int s1, int s2)
{
	ulong l;

	if(reg.ft[s1] != FPd) {
		if(reg.ft[s1] == FPs && s1 != 24)
			ifmt(s1);
		l = reg.di[s1];
		reg.di[s1] = reg.di[s1+1]; 
		reg.di[s1+1] = l; 
		reg.ft[s1] = FPd;
	}
	if(reg.ft[s2] != FPd) {
		if(reg.ft[s2] == FPs && s2 != 24)
			ifmt(s2);
		l = reg.di[s2];
		reg.di[s2] = reg.di[s2+1]; 
		reg.di[s2+1] = l; 
		reg.ft[s2] = FPd;
	}
	reg.ft[dst] = FPd;
}

void
Iswc1(ulong inst)
{
	int off;
	ulong l;
	int rt, rb, ert;

	Getrbrt(rb, rt, inst);
	off = (short)(inst&0xffff);

	if(trace)
		itrace("swc1\tf%d,0x%x(r%d) ea=%lux", rt, off, rb, reg.r[rb]+off);

	ert = rt&~1;
	if(reg.ft[ert] == FPd) {
		l = reg.di[ert];
		reg.di[ert] = reg.di[ert+1]; 
		reg.di[ert+1] = l; 
		reg.ft[ert] = FPmemory;
	}
	putmem_w(reg.r[rb]+off, reg.di[rt]);
}

void
Ifsub(ulong ir)
{
	char fmt;
	int fs, ft, fd;

	Getf3(fs, ft, fd, ir);

	switch((ir>>21)&0xf) {
	default:
		unimp(ir);
	case 0:	/* single */
		fmt = 's';
		floatop(fd, fs, ft);
		reg.fl[fd] = reg.fl[fs] - reg.fl[ft];
		break;	
	case 1: /* double */
		fmt = 'd';
		doubop(fd, fs, ft);
		reg.fd[fd>>1] = reg.fd[fs>>1] - reg.fd[ft>>1];
		break;	
	case 4:
		fmt = 'w';
		reg.di[fd] = reg.di[fs] - reg.di[ft];
		break;
	}
	if(trace)
		itrace("sub.%c\tf%d,f%d,f%d", fmt, fd, fs, ft);
}

void
Ifmov(ulong ir)
{
	char fmt;
	int fs, fd;

	Getf2(fs, fd, ir);

	switch((ir>>21)&0xf) {
	default:
		unimp(ir);
	case 0:	/* single */
		fmt = 's';
		reg.fl[fd] = reg.fl[fs];
		reg.ft[fd] = reg.ft[fs];
		break;	
	case 1: /* double */
		fmt = 'd';
		reg.fd[fd>>1] = reg.fd[fs>>1];
		reg.ft[fd] = reg.ft[fs];
		break;	
	case 4:
		fmt = 'w';
		reg.di[fd] = reg.di[fs];
		reg.ft[fd] = reg.ft[fs];
		break;
	}
	if(trace)
		itrace("mov.%c\tf%d,f%d", fmt, fd, fs);
}

void
Ifabs(ulong ir)
{
	char fmt;
	int fs, fd;

	Getf2(fs, fd, ir);

	switch((ir>>21)&0xf) {
	default:
		unimp(ir);
	case 0:	/* single */
		fmt = 's';
		floatop(fd, fs, fs);
		if(reg.fl[fs] < 0.0)
			reg.fl[fd] = -reg.fl[fs];
		else
			reg.fl[fd] = reg.fl[fs];
		break;	
	case 1: /* double */
		fmt = 'd';
		doubop(fd, fs, fs);
		if(reg.fd[fs>>1] < 0.0)
			reg.fd[fd>>1] = -reg.fd[fs>>1];
		else
			reg.fd[fd>>1] = reg.fd[fs>>1];
		break;	
	case 4:
		fmt = 'w';
		if((long)reg.di[fs] < 0)
			reg.di[fd] = -reg.di[fs];
		else
			reg.di[fd] = reg.di[fs];
		break;
	}
	if(trace)
		itrace("abs.%c\tf%d,f%d", fmt, fd, fs);
}

void
Ifneg(ulong ir)
{
	char fmt;
	int fs, fd;

	Getf2(fs, fd, ir);

	switch((ir>>21)&0xf) {
	default:
		unimp(ir);
	case 0:	/* single */
		fmt = 's';
		floatop(fd, fs, fs);
		reg.fl[fd] = -reg.fl[fs];
		break;	
	case 1: /* double */
		fmt = 'd';
		doubop(fd, fs, fs);
		reg.fd[fd>>1] = -reg.fd[fs>>1];
		break;	
	case 4:
		fmt = 'w';
		reg.di[fd] = -reg.di[fs];
		break;
	}
	if(trace)
		itrace("neg.%c\tf%d,f%d", fmt, fd, fs);
}

void
Icvtd(ulong ir)
{
	char fmt;
	int fs, fd;

	Getf2(fs, fd, ir);

	switch((ir>>21)&0xf) {
	default:
		unimp(ir);
	case 0:	/* single */
		fmt = 's';
		floatop(fs, fs, fs);
		reg.fd[fd>>1] = reg.fl[fs];
		reg.ft[fd] = FPd;
		break;	
	case 1: /* double */
		fmt = 'd';
		doubop(fd, fs, fs);
		reg.fd[fd>>1] = reg.fd[fs>>1];
		break;	
	case 4:
		fmt = 'w';
		reg.fd[fd>>1] = (long)reg.di[fs];
		reg.ft[fd] = FPd;
		break;
	}
	if(trace)
		itrace("cvt.d.%c\tf%d,f%d", fmt, fd, fs);
}

void
Icvts(ulong ir)
{
	char fmt;
	int fs, fd;

	Getf2(fs, fd, ir);

	switch((ir>>21)&0xf) {
	default:
		unimp(ir);
	case 0:	/* single */
		fmt = 's';
		floatop(fd, fs, fs);
		reg.fl[fd] = reg.fl[fs];
		break;	
	case 1: /* double */
		fmt = 'd';
		doubop(fs, fs, fs);
		reg.fl[fd] = reg.fd[fs>>1];
		reg.ft[fd] = FPs;
		break;	
	case 4:
		fmt = 'w';
		reg.fl[fd] = (long)reg.di[fs];
		reg.ft[fd] = FPs;
		break;
	}
	if(trace)
		itrace("cvt.s.%c\tf%d,f%d", fmt, fd, fs);
}

void
Icvtw(ulong ir)
{
	long v;
	char fmt;
	int fs, fd;

	Getf2(fs, fd, ir);

	switch((ir>>21)&0xf) {
	default:
		unimp(ir);
	case 0:	/* single */
		fmt = 's';
		floatop(fs, fs, fs);
		v = reg.fl[fs];
		break;	
	case 1: /* double */
		fmt = 'd';
		doubop(fs, fs, fs);
		v = reg.fd[fs>>1];
		break;	
	case 4:
		fmt = 'w';
		v = reg.di[fs];
		break;
	}
	reg.di[fd] = v;
	reg.ft[fd] = FPmemory;
	if(trace)
		itrace("cvt.w.%c\tf%d,f%d", fmt, fd, fs);
}

void
Ifadd(ulong ir)
{
	char fmt;
	int fs, ft, fd;

	Getf3(fs, ft, fd, ir);

	switch((ir>>21)&0xf) {
	default:
		unimp(ir);
	case 0:	/* single */
		fmt = 's';
		floatop(fd, fs, ft);
		reg.fl[fd] = reg.fl[fs] + reg.fl[ft];
		break;	
	case 1: /* double */
		fmt = 'd';
		doubop(fd, fs, ft);
		reg.fd[fd>>1] = reg.fd[fs>>1] + reg.fd[ft>>1];
		break;	
	case 4:
		fmt = 'w';
		reg.di[fd] = reg.di[fs] + reg.di[ft];
		break;
	}
	if(trace)
		itrace("add.%c\tf%d,f%d,f%d", fmt, fd, fs, ft);
}

void
Ifmul(ulong ir)
{
	char fmt;
	int fs, ft, fd;

	Getf3(fs, ft, fd, ir);

	switch((ir>>21)&0xf) {
	default:
		unimp(ir);
	case 0:	/* single */
		fmt = 's';
		floatop(fd, fs, ft);
		reg.fl[fd] = reg.fl[fs] * reg.fl[ft];
		break;	
	case 1: /* double */
		fmt = 'd';
		doubop(fd, fs, ft);
		reg.fd[fd>>1] = reg.fd[fs>>1] * reg.fd[ft>>1];
		break;	
	case 4:
		fmt = 'w';
		reg.di[fd] = reg.di[fs] * reg.di[ft];
		break;
	}
	if(trace)
		itrace("mul.%c\tf%d,f%d,f%d", fmt, fd, fs, ft);
}

void
Ifdiv(ulong ir)
{
	char fmt;
	int fs, ft, fd;

	Getf3(fs, ft, fd, ir);

	switch((ir>>21)&0xf) {
	default:
		unimp(ir);
	case 0:	/* single */
		fmt = 's';
		floatop(fd, fs, ft);
		reg.fl[fd] = reg.fl[fs] / reg.fl[ft];
		break;	
	case 1: /* double */
		fmt = 'd';
		doubop(fd, fs, ft);
		reg.fd[fd>>1] = reg.fd[fs>>1] / reg.fd[ft>>1];
		break;	
	case 4:
		fmt = 'w';
		reg.di[fd] = reg.di[fs] / reg.di[ft];
		break;
	}
	if(trace)
		itrace("div.%c\tf%d,f%d,f%d", fmt, fd, fs, ft);
}

void
Ilwc1(ulong inst)
{
	int rt, rb;
	int off;

	Getrbrt(rb, rt, inst);
	off = (short)(inst&0xffff);

	if(trace)
		itrace("lwc1\tf%d,0x%x(r%d) ea=%lux", rt, off, rb, reg.r[rb]+off);

	reg.di[rt] = getmem_w(reg.r[rb]+off);
	reg.ft[rt] = FPmemory;
}

void
Ibcfbct(ulong inst)
{
	int takeit;
	int off;
	ulong npc;

	off = (short)(inst&0xffff);

	takeit = 0;
	npc = reg.pc + (off<<2) + 4;
	if(inst&(1<<16)) {
		if(trace)
			itrace("bc1t\t0x%lux", npc);

		if(reg.fpsr&FP_CBIT)
			takeit = 1;
	}
	else {
		if(trace)
			itrace("bc1f\t0x%lux", npc);

		if((reg.fpsr&FP_CBIT) == 0)
			takeit = 1;
	}

	if(takeit) {
		/* Do the delay slot */
		reg.ir = ifetch(reg.pc+4);
		Statbra();
		Iexec(reg.ir);
		reg.pc = npc-4;
	}
}

void
Imtct(ulong ir)
{
	int rt, fs;

	SpecialGetrtrd(rt, fs, ir);
	if(ir&(1<<22)) {			/* CT */
		if(trace)
			itrace("ctc1\tr%d,f%d", rt, fs);
	}
	else {					/* MT */
		if(trace)
			itrace("mtc1\tr%d,f%d", rt, fs);

		reg.di[fs] = reg.r[rt];
		reg.ft[fs] = FPmemory;
	}
}

void
Imfcf(ulong ir)
{
	int rt, fs;

	SpecialGetrtrd(rt, fs, ir);
	if(ir&(1<<22)) {	/* CF */
		if(trace)
			itrace("cfc1\tr%d,f%d", rt, fs);
	}
	else {			/* MF */
		if(trace)
			itrace("mfc1\tr%d,f%d", rt, fs);

		reg.r[rt] = reg.di[fs];
	}
}

void
Icop1(ulong ir)
{
	Inst *i;

	switch((ir>>23)&7) {
	case 0:
		Imfcf(ir);
		break;
	case 1:
		Imtct(ir);
		break;
	case 2:
	case 3:
		Ibcfbct(ir);
		break;
	case 4:
	case 5:
	case 6:
	case 7:
		i = &cop1[ir&0x3f];
		i->count++;
		(*i->func)(ir);
	}
}

void
Ifcmp(ulong ir)
{
	char fmt;
	int fc;
	int ft, fs;

	SpecialGetrtrd(ft, fs, ir);

	SET(fc);
	switch((ir>>21)&0xf) {
	default:
		unimp(ir);
	case 0:	/* single */
		fmt = 's';
		floatop(fs, fs, ft);
		if(isNaN(reg.fl[fs]) || isNaN(reg.fl[ft])) {
			fc = FP_U;
			break;
		}
		if(reg.fl[fs] == reg.fl[ft]) {
			fc = FP_E;
			break;
		}
		if(reg.fl[fs] < reg.fl[ft]) {
			fc = FP_L;
			break;
		}
		if(reg.fl[fs] > reg.fl[ft]) {
			fc = FP_G;
			break;
		}
		print("vi: bad in fcmp");
		break;	
	case 1: /* double */
		fmt = 'd';
		doubop(fs, fs, ft);
		if(isNaN(reg.fd[fs>>1]) || isNaN(reg.fd[ft>>1])) {
			fc = FP_U;
			break;
		}
		if(reg.fd[fs>>1] == reg.fd[ft>>1]) {
			fc = FP_E;
			break;
		}
		if(reg.fd[fs>>1] < reg.fd[ft>>1]) {
			fc = FP_L;
			break;
		}
		if(reg.fd[fs>>1] > reg.fd[ft>>1]) {
			fc = FP_G;
			break;
		}
		print("vi: bad in fcmp");
		break;	
	case 4:
		fmt = 'w';
		if(reg.di[fs] == reg.di[ft]) {
			fc = FP_E;
			break;
		}
		if(reg.di[fs] < reg.di[ft]) {
			fc = FP_L;
			break;
		}
		if(reg.di[fs] > reg.di[ft]) {
			fc = FP_G;
			break;
		}
		break;
	}

	reg.fpsr &= ~FP_CBIT;
	switch(ir&0xf) {
	case 0:
		if(trace)
			itrace("c.f.%c\tf%d,f%d", fmt, fs, ft);
		break;
	case 1:
		if(trace)
			itrace("c.un.%c\tf%d,f%d", fmt, fs, ft);
		if(fc == FP_U)
			reg.fpsr |= FP_CBIT;	
		break;
	case 2:
		if(trace)
			itrace("c.eq.%c\tf%d,f%d", fmt, fs, ft);
		if(fc == FP_E)
			reg.fpsr |= FP_CBIT;	
		break;
	case 3:
		if(trace)
			itrace("c.ueq.%c\tf%d,f%d", fmt, fs, ft);
		if(fc == FP_E || fc == FP_U)
			reg.fpsr |= FP_CBIT;	
		break;
 	case 4:
		if(trace)
			itrace("c.lt.%c\tf%d,f%d", fmt, fs, ft);
		if(fc == FP_L)
			reg.fpsr |= FP_CBIT;	
		break;
	case 5:
		if(trace)
			itrace("c.ult.%c\tf%d,f%d", fmt, fs, ft);
		if(fc == FP_L || fc == FP_U)
			reg.fpsr |= FP_CBIT;	
		break;
	case 6:
		if(trace)
			itrace("c.le.%c\tf%d,f%d", fmt, fs, ft);
		if(fc == FP_E || fc == FP_L)
			reg.fpsr |= FP_CBIT;	
		break;
	case 7:
		if(trace)
			itrace("c.ule.%c\tf%d,f%d", fmt, fs, ft);
		if(fc == FP_E || fc == FP_L || fc == FP_U)
			reg.fpsr |= FP_CBIT;	
		break;
	case 8:
		if(trace)
			itrace("c.sf.%c\tf%d,f%d", fmt, fs, ft);
		if(fc == FP_U)
			inval(ir);
		break;
	case 9:
		if(trace)
			itrace("c.ngle.%c\tf%d,f%d", fmt, fs, ft);
		if(fc == FP_U) {
			reg.fpsr |= FP_CBIT;
			inval(ir);
		}
		break;
	case 10:
		if(trace)
			itrace("c.seq.%c\tf%d,f%d", fmt, fs, ft);
		if(fc == FP_E)
			reg.fpsr |= FP_CBIT;	
		if(fc == FP_U)
			inval(ir);
		break;
	case 11:
		if(trace)
			itrace("c.ngl.%c\tf%d,f%d", fmt, fs, ft);
		if(fc == FP_E || fc == FP_U)
			reg.fpsr |= FP_CBIT;	
		if(fc == FP_U)
			inval(ir);
		break;
 	case 12:
		if(trace)
			itrace("c.lt.%c\tf%d,f%d", fmt, fs, ft);
		if(fc == FP_L)
			reg.fpsr |= FP_CBIT;	
		if(fc == FP_U)
			inval(ir);
		break;
	case 13:
		if(trace)
			itrace("c.nge.%c\tf%d,f%d", fmt, fs, ft);
		if(fc == FP_L || fc == FP_U)
			reg.fpsr |= FP_CBIT;	
		if(fc == FP_U)
			inval(ir);
		break;
	case 14:
		if(trace)
			itrace("c.le.%c\tf%d,f%d", fmt, fs, ft);
		if(fc == FP_E || fc == FP_L)
			reg.fpsr |= FP_CBIT;	
		if(fc == FP_U)
			inval(ir);
		break;
	case 15:
		if(trace)
			itrace("c.ngt.%c\tf%d,f%d", fmt, fs, ft);
		if(fc == FP_E || fc == FP_L || fc == FP_U)
			reg.fpsr |= FP_CBIT;	
		if(fc == FP_U)
			inval(ir);
		break;
	}
	USED(fmt);
}
