#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"ureg.h"
#include	"io.h"
#include	"../port/error.h"

static	int	unfinished(FPsave*);
static	void	fixq(FPsave*, int);
extern	void*	bbmalloc(int);

int
fptrap(void)
{
	int n, ret;
	ulong fsr;

	n = getfpq(&u->fpsave.q[0].a);
	if(n > NFPQ)
		panic("FPQ %d\n", n);
	/*
	 * If faulted during restart, recover PC for debugging
	 */
	if(u->fpsave.fppc)
		u->fpsave.q[0].a = u->fpsave.fppc;
	fsr = getfsr();
	savefpregs(&u->fpsave);
	u->fpsave.fsr = fsr;
	ret = 0;
	switch((fsr>>14) & 7){
	case 2:
		ret = unfinished(&u->fpsave);
		break;
	}
	if(ret){
		enabfp();	/* savefpregs disables it */
		clearftt(fsr & ~0x1F);	/* clear ftt and cexc */
		restfpregs(&u->fpsave, fsr);
		enabfp();	/* restfpregs disables it */
		fixq(&u->fpsave, n);
	}
	return ret;
}

static void
unpack(FPsave *f, int size, int reg, int *sign, int *exp)
{
	*sign = 1;
	if(f->fpreg[reg] & 0x80000000)
		*sign = -1;
	switch(size){
	case 1:
		*exp = ((f->fpreg[reg]>>23)&0xFF) - ((1<<7)-2);
		break;
	case 2:
		if(reg & 1){
			pprint("unaligned double fp register\n");
			reg &= ~1;
		}
		*exp = ((f->fpreg[reg]>>20)&0x7FF) - ((1<<10)-2);
		break;
	case 3:
		if(reg & 3){
			pprint("unaligned quad fp register\n");
			reg &= ~3;
		}
		*exp = ((f->fpreg[reg]>>16)&0x7FFF) - ((1<<14)-2);
		break;
	}
}

static void
zeroreg(FPsave *f, int size, int reg, int sign)
{
	switch(size){
	case 1:
		size = 4;
		break;
	case 2:
		if(reg & 1)
			reg &= ~1;
		size = 8;
		break;
	case 3:
		if(reg & 3)
			reg &= ~3;
		size = 16;
		break;
	}
	memset(&f->fpreg[reg], 0, size);
	if(sign < 0)
		f->fpreg[reg] |= 0x80000000;
}

static int
unfinished(FPsave *f)
{
	ulong instr;
	int size, maxe, maxm, op, rd, rs1, rs2;
	int sd, ss1, ss2;
	int ed, es1, es2;

	instr = f->q[0].i;
	if((instr&0xC1F80000) != 0x81A00000){
    bad:
		pprint("unknown unfinished instruction %lux\n", instr);
		return 0;
	}
	size = (instr>>5) & 0x3;
	if(size == 0)
		goto bad;
	maxe = 0;
	maxm = 0;
	switch(size){
	case 1:
		maxe = 1<<7;
		maxm = 24;
		break;
	case 2:
		maxe = 1<<10;
		maxm = 53;
		break;
	case 3:
		maxe = 1<<14;
		maxm = 113;
		break;
	}
	rd = (instr>>25) & 0x1F;
	rs1 = (instr>>14) & 0x1F;
	rs2 = (instr>>0) & 0x1F;
	unpack(f, size, rs1, &ss1, &es1);
	unpack(f, size, rs2, &ss2, &es2);
	op = (instr>>7) & 0x7F;
	ed = 0;
	switch(op){
	case 0x11:	/* FSUB */
		ss2 = -ss2;
	case 0x10:	/* FADD */
		if(es1<-(maxe-maxm) && es2<-(maxe-maxm))
			ed = -maxe;
		if(es1 > es2)
			sd = es1;
		else
			sd = es2;
		break;

	case 0x13:	/* FDIV */
		es2 = -es2;
	case 0x12:	/* FMUL */
		sd = 1;
		if(ss1 != ss2)
			sd = -1;
		ed = es1 + es2;
		break;

	case 0x31:	/* F?TOS */
	case 0x32:	/* F?TOD */
	case 0x33:	/* F?TOQ */
		if(es2 == maxe)		/* NaN or Inf */
			return 0;
		sd = ss2;
		ed = es2;	/* if underflow, this will do the trick */
		break;

	default:
		goto bad;
	}
	if(ed <= -(maxe-5)){	/* guess: underflow */
		zeroreg(f, size, rd, sd);
		return 1;
	}
	return 0;
}

static void
fixq(FPsave *f, int n)
{
	ulong instr;
	ulong *ip;

	while(n > 1){
		--n;
		memmove(&f->q[0], &f->q[1], n*sizeof f->q[0]);
		memset(&f->q[n], 0, (NFPQ-n)*sizeof f->q[0]);
		instr = f->q[0].i;
		/*
		 * Sparc says you have to emulate.  To hell with that.
		 * We can compile on the fly instead.
		 * Run one instruction and wait for it to complete or trap.
		 * If it traps, we'll recurse but we won't overwrite
		 * our own data structures because there will be only
		 * one instruction uncompleted in the FPU.
		 * If that instruction generates an unfixable trap,
		 * stop the processing, which means a buglet: if the user
		 * process is attempting to resolve FP errors, it must
		 * find and extract the uncompleted instructions behind
		 * the trap by examining the non-zero members of the FPQ.
		 * Tough.
		 */
		ip = bbmalloc(3*sizeof(ulong));
		ip[0] = instr;
		ip[1] = 0x81c3e008;	/* JMPL #8(R15), R0 [RETURN] */
		ip[2] = 0x01000000;	/* SETHI #0, R0 [NOP] */
		f->fppc = f->q[0].a;
		(*(void(*)(void))ip)();	 /* You are expected to understand this cast */
		f->fppc = 0;
		if(!fpquiet())
			break;
	}
}

/*
 * Must only be called splhi() when it is safe to spllo().  Because the FP unit
 * traps if you touch it when an exception is pending, and because if you
 * trap with ET==0 you halt, this routine sets some global flags to enable
 * the rest of the system to handle the trap that might occur here without
 * upsetting the kernel.  Shouldn't be necessary, but safety first.
 */
int
fpquiet(void)
{
	int i, notrap;
	ulong fsr;
	char buf[128];

	i = 0;
	notrap = 1;
	m->fpunsafe = 1;
	u->p->fpstate = FPinactive;
	for(;;){
		m->fptrap = 0;
		fsr = getfsr();
		if(m->fptrap){
			/* trap occurred and u->fpsave contains state */
			sprint(buf, "sys: %s", excname(8));
			postnote(u->p, 1, buf, NDebug);
			notrap = 0;
			break;
		}
		if((fsr&(1<<13)) == 0)
			break;
		if(++i > 1000){
			print("fp not quiescent\n");
			break;
		}
	}
	m->fpunsafe = 0;
	u->p->fpstate = FPactive;
	return notrap;
}
