#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#define Extern extern
#include "power.h"


void	lfs(ulong);
void	lfd(ulong);
void	stfs(ulong);
void	stfd(ulong);
/* indexed versions are in 31 */

void	addic(ulong);
void	addiccc(ulong);
void	addi(ulong);
void	addis(ulong);
void	andicc(ulong);
void	andiscc(ulong);
void	bcx(ulong);
void	bx(ulong);
void	cmpi(ulong);
void	cmpli(ulong);
void	lbz(ulong);
void	lha(ulong);
void	lhz(ulong);
void	lmw(ulong);
void	lwz(ulong);
void	mulli(ulong);
void	ori(ulong);
void	oris(ulong);
void	rlwimi(ulong);
void	rlwinm(ulong);
void	rlwnm(ulong);
void	sc(ulong);
void	stb(ulong);
void	sth(ulong);
void	stmw(ulong);
void	stw(ulong);
void	subfic(ulong);
void	twi(ulong);
void	xori(ulong);
void	xoris(ulong);

Inst	op0[] = {
[3] {twi, "twi", Ibranch},
[7] {mulli, "mulli", Iarith},
[8] {subfic, "subfic", Iarith},
[10] {cmpli, "cmpli", Iarith},
[11] {cmpi, "cmpi", Iarith},
[12] {addic, "addic", Iarith},
[13] {addiccc, "addic.", Iarith},
[14] {addi, "addi", Iarith},
[15] {addis, "addis", Iarith},
[16] {bcx, "bc⋯", Ibranch},
[17] {sc, "sc", Isyscall},
[18] {bx, "b⋯", Ibranch},
/* group 19; branch unit */
[20] {rlwimi, "rlwimi", Ilog},
[21] {rlwinm, "rlwinm", Ilog},
[23] {rlwnm, "rlwnm", Ilog},
[24] {ori, "ori", Ilog},
[25] {oris, "oris", Ilog},
[26] {xori, "xori", Ilog},
[27] {xoris, "xoris", Ilog},
[28] {andicc, "andi.", Ilog},
[29] {andiscc, "andis.", Ilog},
/* group 31; integer & misc. */
[32] {lwz, "lwz", Iload},
[33] {lwz, "lwzu", Iload},
[34] {lbz, "lbz", Iload},
[35] {lbz, "lbzu", Iload},
[36] {stw, "stw", Istore},
[37] {stw, "stwu", Istore},
[38] {stb, "stb", Istore},
[39] {stb, "stbu", Istore},
[40] {lhz, "lhz", Iload},
[41] {lhz, "lhzu", Iload},
[42] {lha, "lha", Iload},
[43] {lha, "lhau", Iload},
[44] {sth, "sth", Istore},
[45] {sth, "sthu", Istore},
[46] {lmw, "lmw", Iload},
[47] {stmw, "stmw", Istore},
[48] {lfs, "lfs", Iload},
[49] {lfs, "lfsu", Iload},
[50] {lfd, "lfd", Iload},
[51] {lfd, "lfdu", Iload},
[52] {stfs, "stfs", Istore},
[53] {stfs, "stfsu", Istore},
[54] {stfd, "stfd", Istore},
[55] {stfd, "stfdu", Istore},
/* group 59; single precision floating point */
/* group 63; double precision floating point; fpscr */
	{0, 0, 0},
};

Inset	ops0 = {op0, nelem(op0)-1};

static	char	oemflag[] = {
	[104] 1,
	[10] 1,
	[136] 1,
	[138] 1,
	[200] 1,
	[202] 1,
	[232] 1,
	[234] 1,
	[235] 1,
	[266] 1,
	[40] 1,
	[459] 1,
	[491] 1,
	[8] 1,
};


void
run(void)
{
	int xo, f;

	do {
		reg.ir = ifetch(reg.pc);
		ci = 0;
		switch(reg.ir>>26) {
		default:
			xo = reg.ir>>26;
			if(xo >= nelem(op0))
				break;
			ci = &op0[xo];
			break;
		case 19:
			xo = getxo(reg.ir);
			if(xo >= ops19.nel)
				break;
			ci = &ops19.tab[xo];
			break;
		case 31:
			xo = getxo(reg.ir);
			f = xo & ~getxo(OE);
			if(reg.ir&OE && f < sizeof(oemflag) && oemflag[f])
				xo = f;
			if(xo >= ops31.nel)
				break;
			ci = &ops31.tab[xo];
			break;
		case 59:
			xo = getxo(reg.ir) & 0x1F;
			if(xo >= ops59.nel)
				break;
			ci = &ops59.tab[xo];
			break;
		case 63:
			xo = getxo(reg.ir) & 0x1F;
			if(xo < ops63a.nel) {
				ci = &ops63a.tab[xo];
				if(ci->func || ci->name)
					break;
				ci = 0;
			}
			xo = getxo(reg.ir);
			if(xo >= ops63b.nel)
				break;
			ci = &ops63b.tab[xo];
			break;
		}
		if(ci && ci->func){
			ci->count++;
			(*ci->func)(reg.ir);
		} else {
			if(ci && ci->name && trace)
				itrace("%s\t[not yet done]", ci->name);
			else
				undef(reg.ir);
		}
		reg.pc += 4;
		if(bplist)
			brkchk(reg.pc, Instruction);
	}while(--count);
}

void
ilock(int)
{
}

void
undef(ulong ir)
{
/*	Bprint(bioout, "op=%d op2=%d op3=%d\n", ir>>30, (ir>>21)&0x7, (ir>>19)&0x3f); */
	Bprint(bioout, "illegal_instruction IR #%.8lux (op=%ld/%ld, pc=#%.8lux)\n", ir, getop(ir), getxo(ir), reg.pc);
	if(ci && ci->name && ci->func==0)
		Bprint(bioout, "(%s not yet implemented)\n", ci->name);
	longjmp(errjmp, 0);
}

void
unimp(ulong ir)
{
/*	Bprint(bioout, "op=%d op2=%d op3=%d\n", ir>>30, (ir>>21)&0x7, (ir>>19)&0x3f); */
	Bprint(bioout, "illegal_instruction IR #%.8lux (op=%ld/%ld, pc=#%.8lux) %s not in MPC601\n", ir, getop(ir), getxo(ir), reg.pc, ci->name?ci->name: "-");
	longjmp(errjmp, 0);
}
