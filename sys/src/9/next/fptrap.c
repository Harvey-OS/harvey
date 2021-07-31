#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"ureg.h"
#include	"io.h"
#include	"../port/error.h"

int	underflow(Ureg*, ushort, FPsave*, ushort*);
int	overflow(Ureg*, ushort, FPsave*, ushort*);
int	operanderr(Ureg*, ushort, FPsave*, ushort*);

int
fptrap(Ureg *ur)
{
	unsigned vo;
	int ret, e, s;
	ushort cmdreg;
	FPsave *f;
	ushort *tmp;

	s = splhi();
	procsave(u->p);
	f = &u->fpsave;
	if(f->type == 0){
    Return0:
		splx(s);
		return 0;
	}
	switch(f->size){
	default:
		pprint("unknown fp exception size %d\n", f->size);
	case 0x00:
		goto Return0;
	case 0x28:
		if((*(uchar*)(&f->type+0x10) & 4) == 0)
			goto Return0;
		e = 1;
		cmdreg = *(ushort*)(&f->type+0x08);
		tmp = (ushort*)(&f->type+0x14);
		break;
	case 0x60:
		if((*(uchar*)(&f->type+0x48) & 6) == 0)
			goto Return0;
		if(*(uchar*)(&f->type+0x48) & 2){
			e = 3;
			cmdreg = *(ushort*)(&f->type+0x34);
		}else{
			e = 1;
			cmdreg = *(ushort*)(&f->type+0x40);
		}
		tmp = (ushort*)(&f->type+0x4C);
		break;
	}
	if(e == 3){	/* shuffle bits */
		ushort t;
		t = cmdreg;
		cmdreg &= ~0x3C;
		t &= 0x3C;
		t <<= 1;
		if(t & 0x40)
			t |= 0x4;
		cmdreg |= t & 0x3C;
	}
	if((cmdreg & 0x7F) == 0x05){	/* FSQRT */
		cmdreg &= 0x7F;
		cmdreg |= 0x04;
	}
	/* now cmdreg command field contains opcode */

	ret = 0;
	vo = (ur->vo & 0x0FFF) >> 2;
	switch(vo){
	default:
		break;
	case 51:
		ret = underflow(ur, cmdreg, f, tmp);
		break;
	case 52:
		ret = operanderr(ur, cmdreg, f, tmp);
		break;
	case 53:
		ret = overflow(ur, cmdreg, f, tmp);
		break;
	}

	/*
	 * Clear E bit in frame; is this enough?  (Seems to be.)
	 */
	switch(f->size){
	case 0x28:
		*(uchar*)(&f->type+0x10) &= ~4;
		break;
	case 0x60:
		if(e == 3)
			*(uchar*)(&f->type+0x48) &= ~2;
		else
			*(uchar*)(&f->type+0x48) &= ~4;
		break;
	}
	procrestore(u->p);
	splx(s);
	return ret;
}

void
zeroreg(FPsave *f, int reg, int sign)
{
	memset(&f->dreg[reg], 0, sizeof f->dreg[reg]);
	if(sign < 0)
		f->dreg[reg].d[0] |= 1<<31;
}

void
infreg(FPsave *f, int reg, int sign)
{
	memset(&f->dreg[reg], 0, sizeof f->dreg[reg]);
	memset(&f->dreg[reg], ~0, 2);	/* 15 bits of exponent, all ones */
	if(sign > 0)
		f->dreg[reg].d[0] &= ~(1<<31);
}

int
underflow(Ureg *ur, ushort cmdreg, FPsave *f, ushort *tmp)
{
	int reg;
	ushort w1, s;
	ulong ea;
	int sign;

	if(f->fpcr & (1<<11))	/* trap enabled */
		return 0;
	reg = (cmdreg>>7)&7;
	sign = 1;
	if(tmp[0] & 0x8000)
		sign = -1;
	switch(cmdreg & 0x7F){
	case 0x00:	/* FMOVE */
		w1 = *(ushort*)(f->fpiar+2);
		if(w1 & (1<<13)){	/* to memory */
			ea = *(ulong*)ur->microstate;
			s = (w1>>10) & 7;
			if(s == 1)
				s = 4;
			else if(s == 5)
				s = 8;
			else
				return 0;
			memset((uchar*)ea, 0, s);
			if(sign < 0)
				((uchar*)ea)[0] |= 0x80;
		}else			/* to register */
			zeroreg(f, reg, sign);
		break;
	case 0x18:	/* FABS */
	case 0x1A:	/* FNEG */
	case 0x04:	/* FSQRT */
	case 0x20:	/* FDIV */
	case 0x22:	/* FADD */
	case 0x23:	/* FMUL */
	case 0x28:	/* FSUB */
		zeroreg(f, reg, sign);
		break;
	default:
		pprint("unknown instruction %lux on underflow\n", cmdreg & 0x7F);
		return 0;
	}
	f->fpsr &= ~(1<<11);
	return 1;
}

int
overflow(Ureg *ur, ushort cmdreg, FPsave *f, ushort *tmp)
{
	int reg;
	ushort w0, w1, s;
	ulong ea;
	int sign;

	if(f->fpcr & (1<<12))	/* trap enabled */
		return 0;
	reg = (cmdreg>>7)&7;
	sign = 1;
	if(tmp[0] & 0x8000)
		sign = -1;
	switch(cmdreg & 0x7F){
	case 0x00:	/* FMOVE */
		w1 = *(ushort*)(f->fpiar+2);
		if(w1 & (1<<13)){	/* to memory */
			w0 = *(ushort*)(f->fpiar+0);
			if((w0&0x30) == 0)	/* data register */
				return 0;	/* maybe do better? */
			ea = *(ulong*)ur->microstate;
			s = (w1>>10) & 7;
			if(s == 1)
				s = 4;
			else if(s == 5)
				s = 8;
			else
				return 0;
			memset((uchar*)ea, 0, s);
			if(s == 4)
				*(ushort*)ea |= 0x7F80;	/* 8-bit exp */
			else
				*(ushort*)ea |= 0x7FF0;	/* 11-bit exp */
			if(sign < 0)
				*(uchar*)ea |= 0x80;
		}else			/* to register */
			infreg(f, reg, sign);
		break;
	case 0x18:	/* FABS */
	case 0x1A:	/* FNEG */
	case 0x04:	/* FSQRT */
	case 0x20:	/* FDIV */
	case 0x22:	/* FADD */
	case 0x23:	/* FMUL */
	case 0x28:	/* FSUB */
		infreg(f, reg, sign);
		break;
	default:
		pprint("unknown instruction %lux on overflow\n", cmdreg & 0x7F);
		return 0;
	}
	f->fpsr &= ~(1<<12);
	return 1;
}

int
operanderr(Ureg *ur, ushort cmdreg, FPsave *f, ushort *tmp)
{
	int isD;
	ushort w0, w1, s;
	ulong ea;

	if(f->fpcr & (1<<13))	/* trap enabled */
		return 0;
	switch(cmdreg & 0x7F){
	case 0x00:	/* FMOVE */
		w1 = *(ushort*)(f->fpiar+2);
		if(w1 & (1<<13)){	/* to memory or integer data reg */
			w0 = *(ushort*)(f->fpiar+0);
			if((w0&0x30) == 0){	/* data register */
				isD = 1;
				ea = (ulong)((&ur->r0) + (w0&7));
			}else{
				isD = 0;
				ea = *(ulong*)ur->microstate;
			}
			s = (w1>>10) & 7;
			if((tmp[0]&0x7FFF) == 0x7FFFF){
				/* infinity or NAN */
				/* should do more for 68881 compatibility */
				return 0;
			}
			/*else must be the stupid chip bug */
			switch(s){
			case 0:		/* long integer */
				*(ulong*)ea = 0x80000000;
				break;
			case 4:		/* word integer */
				if(isD)
					ea += 2;
				*(ushort*)ea = 0x8000;
				break;
			case 6:
				if(isD)
					ea += 3;
				*(uchar*)ea = 0x80;
				break;
			default:
				return 0;
			}
		}else			/* to FP register */
			return 0;
		break;
	default:
		pprint("unknown instruction %lux on operanderr\n", cmdreg & 0x7F);
		return 0;
	}
	f->fpsr &= ~(1<<13);
	return 1;
}
