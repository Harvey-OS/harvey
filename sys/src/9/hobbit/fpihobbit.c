#include <u.h>
#include "fpi.h"
#include "machine.h"

typedef ushort Parcel;
typedef struct {
	ulong	sp;
	ulong	msp;
	ulong	pc;
	ulong	psw;
} Context;

static Parcel
getparcel(ulong pc)
{
	ulong *w = (ulong*)(pc & ~3);

	if(pc & 3)
		return *w & 0xFFFF;
	return *w>>16;
}

static void
absolute(Context *context, Parcel instr, void **v)
{
	Parcel p0, p1;

	p0 = getparcel(context->pc);
	if(instr & 0x4000){
		context->pc += 2;
		p1 = getparcel(context->pc);
		context->pc += 2;
		*v = (void*)((p0<<16)|p1);
		return;
	}
	context->pc += 2;
	*v = (void*)((context->pc & 0xE0000000)|p0);
}

static void
stack(Context *context, Parcel instr, void **v)
{
	Parcel p0, p1;

	p0 = getparcel(context->pc);
	if(instr & 0x4000){
		context->pc += 2;
		p1 = getparcel(context->pc);
		context->pc += 2;
		*v = (void*)(context->sp + ((p0<<16)|p1));
		return;
	}
	context->pc += 2;
	*v = (void*)(context->sp + p0);
}

static void
indirect(Context *context, Parcel instr, void **v)
{
	Parcel p0, p1;

	p0 = getparcel(context->pc);
	if(instr & 0x4000){
		context->pc += 2;
		p1 = getparcel(context->pc);
		context->pc += 2;
		*v = (*(void**)(context->sp + ((p0<<16)|p1)));
		return;
	}
	context->pc += 2;
	*v = (*(void**)(context->sp + p0));
}

static void
immediate(Context *context, Parcel instr, void **v)
{
	Parcel p0, p1;

	USED(instr);
	p0 = getparcel(context->pc);
	context->pc += 2;
	p1 = getparcel(context->pc);
	context->pc += 2;
	*v = (void*)((p0<<16)|p1);
}

static void
fpis2i(Internal *i, void *v)
{
	Single *s = v;

	i->s = (*s & 0x80000000) ? 1: 0;
	if((*s & ~0x80000000) == 0){
		SetZero(i);
		return;
	}
	i->e = ((*s>>23) & 0x00FF) - SingleExpBias + ExpBias;
	i->h = (*s & 0x007FFFFF)<<(1+NGuardBits);
	i->l = 0;
	if(i->e)
		i->h |= HiddenBit;
	else
		i->e++;
}

static void
fpid2i(Internal *i, void *v)
{
	Double *d = v;

	i->s = (d->h & 0x80000000) ? 1: 0;
	i->e = (d->h>>20) & 0x07FF;
	i->h = ((d->h & 0x000FFFFF)<<(4+NGuardBits))|((d->l>>25) & 0x7F);
	i->l = (d->l & 0x01FFFFFF)<<NGuardBits;
	if(i->e)
		i->h |= HiddenBit;
	else
		i->e++;
}

static void
fpiw2i(Internal *i, void *v)
{
	Word w, word = *(Word*)v;
	short e;

	if(word < 0){
		i->s = 1;
		word = -word;
	}
	else
		i->s = 0;
	if(word == 0){
		SetZero(i);
		return;
	}
	for (e = 0, w = word; w; w >>= 1, e++)
		;
	if(e > FractBits){
		i->h = word>>(e - FractBits);
		i->l = (word & ((1<<(e - FractBits)) - 1))<<(2*FractBits - e);
	}
	else {
		i->h = word<<(FractBits - e);
		i->l = 0;
	}
	i->e = (e - 1) + ExpBias;
}

static void
fpii2i(Internal *i, void *v)
{
	Single s = (Single)v;

	fpis2i(i, &s);
}

static void
fpii2s(Single *s, Internal *i)
{
	short e;

	fpiround(i);
	if(i->h & HiddenBit)
		i->h &= ~HiddenBit;
	else
		i->e--;
	*s = i->s ? 0x80000000: 0;
	e = i->e;
	if(e < ExpBias){
		if(e <= (ExpBias - SingleExpBias))
			return;
		e = SingleExpBias - (ExpBias - e);
	}
	else  if(e >= (ExpBias + (SingleExpMax-SingleExpBias))){
		*s |= SingleExpMax<<23;
		return;
	}
	else
		e = SingleExpBias + (e - ExpBias);
	*s |= (e<<23)|(i->h>>(1+NGuardBits));
}

static void
fpii2d(Double *d, Internal *i)
{
	fpiround(i);
	if(i->h & HiddenBit)
		i->h &= ~HiddenBit;
	else
		i->e--;
	i->l = ((i->h & GuardMask)<<25)|(i->l>>NGuardBits);
	i->h >>= NGuardBits;
	d->h = i->s ? 0x80000000: 0;
	d->h |= (i->e<<20)|((i->h & 0x00FFFFFF)>>4);
	d->l = (i->h<<28)|i->l;
}

static void
fpii2w(Word *word, Internal *i)
{
	Word w;
	short e;

	fpiround(i);
	e = (i->e - ExpBias) + 1;
	if(e <= 0)
		w = 0;
	else if(e > 31)
		w = 0x7FFFFFFF;
	else if(e > FractBits)
		w = (i->h<<(e - FractBits))|(i->l>>(2*FractBits - e));
	else
		w = i->h>>(FractBits-e);
	if(i->s)
		w = -w;
	*word = w;
}

typedef struct {
	void (*addr)(Context*, Parcel, void**);
	void (*get)(Internal*, void*);
	void (*put)(void*, Internal*);
} OpMode;

static void
getopmode(int mode, OpMode *m)
{
	switch(mode){

	case 0:					/* "*$%lux.F", */
		m->addr = absolute;
		m->get = fpis2i;
		m->put = fpii2s;
		break;

	case 1:					/* "*$%lux.D", */
		m->addr = absolute;
		m->get = fpid2i;
		m->put = fpii2d;
		break;

	case 4:					/* "R%d.F", */
		m->addr = stack;
		m->get = fpis2i;
		m->put = fpii2s;
		break;

	case 5:					/* "R%d.D", */
		m->addr = stack;
		m->get = fpid2i;
		m->put = fpii2d;
		break;

	case 8:					/* "*R%d.F", */
		m->addr = indirect;
		m->get = fpis2i;
		m->put = fpii2s;
		break;

	case 9:					/* "*R%d.D", */
		m->addr = indirect;
		m->get = fpid2i;
		m->put = fpii2d;
		break;

	case 12:				/* "*$%lux.W", */
		m->addr = absolute;
		m->get = fpiw2i;
		m->put = fpii2w;
		break;

	case 13:				/* "R%d.W", */
		m->addr = stack;
		m->get = fpiw2i;
		m->put = fpii2w;
		break;

	case 14:				/* "*R%d.W", */
		m->addr = indirect;
		m->get = fpiw2i;
		m->put = fpii2w;
		break;

	case 15:				/* "$%lux", */
		m->addr = immediate;
		m->get = fpii2i;
		m->put = 0;
		break;

	default:
		m->addr = 0;
		m->get = 0;
		m->put = 0;
		break;
	}
}

static void
fmov(Context *context, Internal *l, Internal *r, Internal *i)
{
	USED(context, r);
	*i = *l;
}

static void
fcmpgt(Context *context, Internal *l, Internal *r, Internal *i)
{
	USED(i);
	if(fpicmp(l, r) == 1)
		context->psw |= PSW_F;
	else
		context->psw &= ~PSW_F;
}

static void
fcmpeq(Context *context, Internal *l, Internal *r, Internal *i)
{
	USED(i);
	if(fpicmp(l, r) == 0)
		context->psw |= PSW_F;
	else
		context->psw &= ~PSW_F;
}

static void
fsub(Context *context, Internal *l, Internal *r, Internal *i)
{
	USED(context);
	l->s ^= 1;
	(*(l->s == r->s ? fpiadd: fpisub))(l, r, i);
}

static void
fmul(Context *context, Internal *l, Internal *r, Internal *i)
{
	USED(context);
	fpimul(l, r, i);
}

static void
fdiv(Context *context, Internal *l, Internal *r, Internal *i)
{
	USED(context);
	fpidiv(l, r, i);
}

static void
fadd(Context *context, Internal *l, Internal *r, Internal *i)
{
	USED(context);
	(*(l->s == r->s ? fpiadd: fpisub))(l, r, i);
}

typedef struct {
	void (*f)(Context*, Internal*, Internal*, Internal*);
	char sequence[5];
} OpCode;

static int
getopcode(int code, OpCode *op)
{
	op->sequence[0] = '0';
	op->sequence[1] = '1';
	op->sequence[2] = 'f';
	op->sequence[3] = '2';
	op->sequence[4] = 0;

	switch(code){

	case 17:				/* FMOV */
		op->sequence[1] = 'f';
		op->sequence[2] = '2';
		op->sequence[3] = 0;
		op->f = fmov;
		break;

	case 25:				/* FCMPGT */
		op->sequence[3] = 0;
		op->f = fcmpgt;
		break;

	case 26:				/* FCMPEQ */
		op->sequence[3] = 0;
		op->f = fcmpeq;
		break;

	case 40:				/* FSUB */
		op->f = fsub;
		break;

	case 41:				/* FMUL */
		op->f = fmul;
		break;

	case 42:				/* FDIV */
		op->f = fdiv;
		break;

	case 43:				/* FADD */
		op->f = fadd;
		break;

	case 56:				/* FSUB3 */
		op->sequence[3] = '3';
		op->f = fsub;
		break;

	case 57:				/* FMUL3 */
		op->sequence[3] = '3';
		op->f = fmul;
		break;

	case 58:				/* FDIV3 */
		op->sequence[3] = '3';
		op->f = fdiv;
		break;

	case 59:				/* FADD3 */
		op->sequence[3] = '3';
		op->f = fadd;
		break;

	default:
		return 0;
	}
	return 1;
}

void
fpi(void *v)
{
	Context *context = v;
	Parcel p;
	ulong pc;
	OpCode op;
	OpMode lm, rm;
	void *lv, *rv;
	Internal l, r, i;
	char *sequence;

	pc = context->pc;
	while(((p = getparcel(pc)) & 0x8000) && getopcode((p>>8) & 0x3F, &op)){
		getopmode((p>>4) & 0x0F, &lm);
		getopmode(p & 0x0F, &rm);
		context->pc += 2;
		(*lm.addr)(context, p, &lv);
		(*rm.addr)(context, p, &rv);
		for(sequence = op.sequence; *sequence; sequence++){
			switch(*sequence){
	
			case '0':
				(*lm.get)(&l, lv);
				break;
	
			case '1':
				(*rm.get)(&r, rv);
				break;
	
			case 'f':
				(*op.f)(context, &l, &r, &i);
				break;

			case '2':
				(*rm.put)(rv, &i);
				break;
	
			case '3':
				(*rm.put)((void*)(context->sp+4), &i);
				break;
			}
		}
		pc = context->pc;
	}
}
