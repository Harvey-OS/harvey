#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#define Extern extern
#include "sparc.h"

void
ldf(ulong ir)
{
	ulong ea;
	int rd, rs1, rs2;

	getrop23(ir);
	if(ir&IMMBIT) {
		ximm(ea, ir);
		if(trace)
			itrace("ldf\tf%d,0x%lux(r%d) ea=%lux",rd, ea, rs1, ea+reg.r[rs1]);
		ea += reg.r[rs1];
	}
	else {
		ea = reg.r[rs1] + reg.r[rs2];
		if(trace)
			itrace("ldf\tf%d,[r%d+r%d] ea=%lux", rd, rs1, rs2, ea);
	}

	reg.di[rd] = getmem_w(ea);
}

void
lddf(ulong ir)
{
	ulong ea;
	int rd, rs1, rs2;

	getrop23(ir);
	if(ir&IMMBIT) {
		ximm(ea, ir);
		if(trace)
			itrace("lddf\tf%d,0x%lux(r%d) ea=%lux",
							rd, ea, rs1, ea+reg.r[rs1]);
		ea += reg.r[rs1];
	}
	else {
		ea = reg.r[rs1] + reg.r[rs2];
		if(trace)
			itrace("lddf\tf%d,[r%d+r%d] ea=%lux", rd, rs1, rs2, ea);
	}

	if(ea&7) {
		Bprint(bioout, "mem_address_not_aligned [load addr %.8lux]\n", ea);
		longjmp(errjmp, 0);
	}
	if(rd&1)
		undef(ir);

	reg.di[rd] = getmem_w(ea);
	reg.di[rd+1] = getmem_w(ea+4);
}

void
stf(ulong ir)
{
	ulong ea;
	int rd, rs1, rs2;

	getrop23(ir);
	if(ir&IMMBIT) {
		ximm(ea, ir);
		if(trace)
			itrace("stf\tf%d,0x%lux(r%d) %lux=%g",
					rd, ea, rs1, ea+reg.r[rs1], reg.fl[rd]);
		ea += reg.r[rs1];
	}
	else {
		ea = reg.r[rs1] + reg.r[rs2];
		if(trace)
			itrace("stf\tf%d,[r%d+r%d] %lux=%lux",
					rd, rs1, rs2, ea, reg.r[rd]);
	}

	putmem_w(ea, reg.di[rd]);
}

void
stdf(ulong ir)
{
	ulong ea;
	int rd, rs1, rs2;

	getrop23(ir);
	if(ir&IMMBIT) {
		ximm(ea, ir);
		if(trace)
			itrace("stdf\tf%d,0x%lux(r%d) %lux=%g",
					rd, ea, rs1, ea+reg.r[rs1], reg.fl[rd]);
		ea += reg.r[rs1];
	}
	else {
		ea = reg.r[rs1] + reg.r[rs2];
		if(trace)
			itrace("stdf\tf%d,[r%d+r%d] %lux=%lux",
					rd, rs1, rs2, ea, reg.r[rd]);
	}

	if(ea&7) {
		Bprint(bioout, "mem_address_not_aligned [store addr %.8lux]\n", ea);
		longjmp(errjmp, 0);
	}
	if(rd&1)
		undef(ir);

	putmem_w(ea, reg.di[rd]);
	putmem_w(ea+4, reg.di[rd+1]);
}

void
fcmp(ulong ir)
{
	int fc, rd, rs1, rs2;

	getrop23(ir);
	USED(rd);
	SET(fc);
	switch((ir>>5)&0x1FF) {
	default:
		undef(ir);
	case 0x51:		/* fcmps */
		if(trace)
			itrace("fcmps\tf%d,f%d", rs1, rs2);
		if(isNaN(reg.fl[rs1]) || isNaN(reg.fl[rs2])) {
			fc = 3;
			break;
		}
		if(reg.fl[rs1] == reg.fl[rs2]) {
			fc = 0;
			break;
		}
		if(reg.fl[rs1] < reg.fl[rs2]) {
			fc = 1;
			break;
		}
		if(reg.fl[rs1] > reg.fl[rs2]) {
			fc = 2;
			break;
		}
		print("ki: fcmp error\n");
		break;
	case 0x52:
		if(trace)
			itrace("fcmpd\tf%d,f%d", rs1, rs2);
		rs1 >>= 1;
		rs2 >>= 1;
		if(isNaN(reg.fd[rs1]) || isNaN(reg.fd[rs2])) {
			fc = 3;
			break;
		}
		if(reg.fd[rs1] == reg.fd[rs2]) {
			fc = 0;
			break;
		}
		if(reg.fd[rs1] < reg.fd[rs2]) {
			fc = 1;
			break;
		}
		if(reg.fd[rs1] > reg.fd[rs2]) {
			fc = 2;
			break;
		}
		print("ki: fcmp error\n");
		break;
	case 0x55:		/* fcmpes */
		if(trace)
			itrace("fcmpes\tf%d,f%d", rs1, rs2);
		rs1 >>= 1;
		rs2 >>= 2;
		if(isNaN(reg.fl[rs1]) || isNaN(reg.fl[rs2])) {
			Bprint(bioout, "invalid_fp_register\n");
			longjmp(errjmp, 0);
		}
		if(reg.fl[rs1] == reg.fl[rs2]) {
			fc = 0;
			break;
		}
		if(reg.fl[rs1] < reg.fl[rs2]) {
			fc = 1;
			break;
		}
		if(reg.fl[rs1] > reg.fl[rs2]) {
			fc = 2;
			break;
		}
		print("ki: fcmp error\n");
		break;
	case 0x56:
		if(trace)
			itrace("fcmped\tf%d,f%d", rs1, rs2);
		if(isNaN(reg.fd[rs1]) || isNaN(reg.fd[rs2])) {
			Bprint(bioout, "invalid_fp_register\n");
			longjmp(errjmp, 0);
		}
		if(reg.fd[rs1] == reg.fd[rs2]) {
			fc = 0;
			break;
		}
		if(reg.fd[rs1] < reg.fd[rs2]) {
			fc = 1;
			break;
		}
		if(reg.fd[rs1] > reg.fd[rs2]) {
			fc = 2;
			break;
		}
		print("ki: fcmp error\n");
		break;

	}
	reg.fpsr = (reg.fpsr&~(0x3<<10)) | (fc<<10);
}

void
fbcc(ulong ir)
{
	char *op;
	ulong npc;
	int takeit, fc, ba, anul;

	fc = (reg.fpsr>>10)&3;
	ba = 0;
	SET(op, takeit);
	switch((ir>>25)&0x0F) {
	case 0:
		op = "fbn";
		takeit = 0;
		break;
	case 1:
		op = "fbne";
		takeit = fc == FP_L || fc == FP_G || fc == FP_U;
		break;
	case 2:
		op = "fblg";
		takeit = fc == FP_L || fc == FP_G;
		break;
	case 3:
		op = "fbul";
		takeit = fc == FP_L || fc == FP_U;
		break;
	case 4:
		op = "fbl";
		takeit = fc == FP_L;
		break;
	case 5:
		op = "fbug";
		takeit = fc == FP_U || fc == FP_G;
		break;
	case 6:
		op = "fbg";
		takeit = fc == FP_G;
		break;
	case 7:
		op = "fbu";
		takeit = fc == FP_U;
		break;
	case 8:
		op = "fba";
		ba = 1;
		takeit = 1;
		break;
	case 9:
		op = "fbe";
		takeit = fc == FP_E;
		break;
	case 10:
		op = "fbue";
		takeit = fc == FP_E || fc == FP_U;
		break;
	case 11:
		op = "fbge";
		takeit = fc == FP_E || fc == FP_G;
		break;
	case 12:
		op = "fbuge";
		takeit = fc == FP_E || fc == FP_G || fc == FP_U;
		break;
	case 13:
		op = "fble";
		takeit = fc == FP_E || fc == FP_L;
		break;
	case 14:
		op = "fbule";
		takeit = fc == FP_E || fc == FP_L || fc == FP_U;
		break;
	case 15:
		op = "fbo";
		takeit = fc == FP_E || fc == FP_L || fc == FP_G;
		break;
	}

	npc = ir & 0x3FFFFF;
	if(npc & (1<<21))
		npc |= ~((1<<22)-1);
	npc = (npc<<2) + reg.pc;

	anul = ir&ANUL;
	if(trace) {
		if(anul)
			itrace("%s,a\t%lux", op, npc);
		else
			itrace("%s\t%lux", op, npc);
	}

	if(takeit == 0) {
		reg.pc += 4;
		if(anul == 0) {
			reg.ir = ifetch(reg.pc);
			delay(reg.pc+4);
		}
		else
			anulled++;
		return;
	}

	ci->taken++;
	if(ba && anul) {
		reg.pc = npc-4;
		anulled++;
		return;	
	}
	reg.ir = ifetch(reg.pc+4);
	delay(npc);
	reg.pc = npc-4;
}

void
farith(ulong ir)
{
	char *op;
	long v;
	int rd, rs1, rs2, fmt;

	fmt = 0;
	getrop23(ir);
	switch((ir>>5)&0x1FF) {
	default:
		undef(ir);
	case 0x41:
		reg.fl[rd] = reg.fl[rs1] + reg.fl[rs2];
		op = "fadds";
		break;
	case 0x42:
		reg.fd[rd>>1] = reg.fd[rs1>>1] + reg.fd[rs2>>1];
		op = "faddd";
		break;
	case 0x45:
		reg.fl[rd] = reg.fl[rs1] - reg.fl[rs2];
		op = "fsubs";
		break;
	case 0x46:
		reg.fd[rd>>1] = reg.fd[rs1>>1] - reg.fd[rs2>>1];
		op = "fsubd";
		break;
	case 0x4d:
		if(reg.fl[rs2] == 0.0) {
			Bprint(bioout, "fp_exception DZ\n");
			longjmp(errjmp, 0);
		}
		reg.fl[rd] = reg.fl[rs1] / reg.fl[rs2];
		op = "fdivs";
		break;
	case 0x4e:
		if(reg.fd[rs2>>1] == 0.0) {
			Bprint(bioout, "fp_exception DZ\n");
			longjmp(errjmp, 0);
		}
		reg.fd[rd>>1] = reg.fd[rs1>>1] / reg.fd[rs2>>1];
		op = "fdivd";
		break;
	case 0x49:
		reg.fl[rd] = reg.fl[rs1] * reg.fl[rs2];
		op = "fmuls";
		break;
	case 0x4a:
		reg.fd[rd>>1] = reg.fd[rs1>>1] * reg.fd[rs2>>1];
		op = "fmuld";
		break;
	case 0xc4:
		reg.fl[rd] = (long)reg.di[rs2];
		fmt = 1;
		op = "fitos";
		break;
	case 0xc8:
		reg.fd[rd>>1] = (long)reg.di[rs2];
		fmt = 1;
		op = "fitod";
		break;
	case 0xd1:
		v = reg.fl[rs2];
		reg.di[rd] = v;
		fmt = 1;
		op = "fstoi";
		break;
	case 0xd2:
		v = reg.fd[rs2>>1];
		reg.di[rd] = v;
		fmt = 1;
		op = "fdtoi";
		break;
	case 0x01:
		reg.di[rd] = reg.di[rs2];
		fmt = 1;
		op = "fmovs";
		break;
	case 0x05:
		reg.fl[rd] = -reg.fl[rs2];
		fmt = 1;
		op = "fnegs";
		break;
	case 0x09:
		reg.fl[rd] = fabs(reg.fl[rs2]);
		fmt = 1;
		op = "fabss";
		break;
	case 0xc9:
		reg.fd[rd>>1] = reg.fl[rs2];
		fmt = 1;
		op = "fstod";
		break;
	case 0xc6:
		reg.fl[rd] = reg.fd[rs2>>1];
		fmt = 1;
		op = "fdtos";
		break;
	}

	if(trace) {
		switch(fmt) {
		case 0:
			itrace("%s\tf%d,f%d,f%d", op, rs1, rs2, rd);
			break;
		case 1:
			itrace("%s\tf%d,f%d", op, rs2, rd);
			break;
		}
	}
}
