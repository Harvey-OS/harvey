#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#include <mach.h>
#define Extern extern
#include "acid.h"

/* 
 * Routines needed from db land to support the disassemblers
 */
void
dprint(char *fmt, ...)
{
	char buf[128], *e;

	e = doprint(buf, buf+sizeof(buf), fmt, (&fmt+1));
	if(xprint == 0)
		Bwrite(bout, buf, e-buf);
	else
		strcat(asmbuf, buf);
}

void
psymoff(ulong ival, int t, char *append)
{
	Symbol s;
	int delta, o;
	char buf[128];

	switch(t) {
	case SEGANY:
		t = CANY;
		break;
	case SEGDATA:
		t = CDATA;
		break;
	case SEGTEXT:
		t = CTEXT;
		break;
	default:
		fatal("psymoff %d", t);
	}
	o = findsym(ival, t, &s);
	delta = ival-s.value;
	if (ival == 0 || o == 0 || delta >= 0x8000)
		sprint(buf, "0x%lux%s", ival, append);
	else {
		if (delta)
			sprint(buf, "%s+0x%lux%s", s.name, delta, append);
		else
			sprint(buf, "%s%s", s.name, append);
	}
	if(xprint == 0)
		Bprint(bout, "%s", buf);
	else
		strcat(asmbuf, buf);
}

void
localaddr(Lsym *fn, Lsym *var, Node *r)
{
	Symbol s;
	ulong fp, addr;

	if (lookup(fn->name, 0, &s) == 0)
		error("function not found");

	if ((fp = machdata->findframe(s.value)) == 0)
		error("stack frame not found");

	if (findlocal(&s, var->name, &s) == 0)
		error("bad local variable %s", var->name);

	switch (s.class) {
	case CAUTO:
		addr = fp - s.value;
		break;
	case CPARAM:		/* assume address size is stack width */
		addr = fp + s.value + mach->szaddr;
		break;
	default:
		SET(addr);
		error("bad local symbol");
	}
	indir(cormap, addr, fn->v->fmt, r);
}

static char fpbuf[64];

/*
 * These routines assume that if the number is representable
 * in IEEE floating point, it will be representable in the native
 * double format.  Naive but workable, probably.
 */
char*
ieeedtos(char *fmt, ulong h, ulong l)
{
	double fr;
	int exp;
	char *p = fpbuf;

	if(h & (1L<<31)){
		*p++ = '-';
		h &= ~(1L<<31);
	}
	else
		*p++ = ' ';

	if(l == 0 && h == 0){
		strcpy(p, "0.");
		return fpbuf;
	}
	exp = (h>>20) & ((1L<<11)-1L);
	if(exp == 0){
		sprint(p, "DeN(%.8lux%.8lux)", h, l);
		return fpbuf;
	}
	if(exp == ((1L<<11)-1L)){
		if(l==0 && (h&((1L<<20)-1L)) == 0)
			sprint(p, "Inf");
		else
			sprint(p, "NaN(%.8lux%.8lux)", h&((1L<<20)-1L), l);

		return fpbuf;
	}
	exp -= (1L<<10) - 2L;
	fr = (long)l & ((1L<<16)-1L);
	fr /= 1L<<16;
	fr += (long)(l>>16) & ((1L<<16)-1L);
	fr /= 1L<<16;
	fr += (long)(h & (1L<<20)-1L) | (1L<<20);
	fr /= 1L<<21;
	fr = ldexp(fr, exp);
	sprint(p, fmt, fr);
	return fpbuf;
}

char*
ieeeftos(char *fmt, ulong h)
{
	double fr;
	int exp;
	char *p = fpbuf;

	if(h & (1L<<31)){
		*p++ = '-';
		h &= ~(1L<<31);
	}else
		*p++ = ' ';
	if(h == 0){
		strcpy(p, "0.");
		goto ret;
	}
	exp = (h>>23) & ((1L<<8)-1L);
	if(exp == 0){
		sprint(p, "DeN(%.8lux)", h);
		goto ret;
	}
	if(exp == ((1L<<8)-1L)){
		if((h&((1L<<23)-1L)) == 0)
			sprint(p, "Inf");
		else
			sprint(p, "NaN(%.8lux)", h&((1L<<23)-1L));
		goto ret;
	}
	exp -= (1L<<7) - 2L;
	fr = (long)(h & ((1L<<23)-1L)) | (1L<<23);
	fr /= 1L<<24;
	fr = ldexp(fr, exp);
	sprint(p, fmt, fr);
    ret:
	return fpbuf;
}
/*
 * print a stack traceback
 * give locals if argument == 'C'
 */
#define	EVEN(x)	((x)&~1)

void
ctrace(int modif)
{
	Symbol s;
	int found;
	long moved, j;
	List **tail, *q, *l;

	USED(modif);

	strc.l = al(TLIST);
	tail = &strc.l;

	j = 0;
	while(strc.pc) {
		if ((moved = pc2sp(strc.pc)) == -1)
			break;
		found = findsym(strc.pc, CTEXT, &s);
		if (!found)
			break;

		strc.sp += moved;
		get4(cormap, strc.sp, SEGDATA, (long *)&strc.pc);

		q = al(TLIST);
		*tail = q;
		tail = &q->next;

		l = al(TINT);			/* Function address */
		q->l = l;
		l->ival = s.value;
		l->fmt = 'X';

		l->next = al(TINT);		/* called from address */
		l = l->next;
		l->ival = strc.pc;
		l->fmt = 'X';

		l->next = al(TLIST);		/* make list of params */
		l = l->next;
		l->l = listparams(&s, strc.sp);

		l->next = al(TLIST);		/* make list of locals */
		l = l->next;
		l->l = listlocals(&s, strc.sp);

		strc.sp += mach->szaddr;	/*assumes address size = stack width*/
		if(++j > 40)
			break;
	}
	if(j == 0)
		error("no frames found");
}

ulong
findframe(ulong addr)
{
	Symbol s;
	int moved;
	ulong sp, pc, o;

	o = mach->kbase-flen;
	get4(cormap, o+mach->sp, SEGDATA, (long*)&sp);
	sp = EVEN(sp);
	get4(cormap, o+mach->pc, SEGDATA, (long*)&pc);

	for(;;) {
		if ((moved = -pc2sp(pc)) == 1)
			return sp;
		sp -= moved;
		findsym(pc, CTEXT, &s);
		if (addr == s.value)
			return sp;
		get4(cormap, sp, SEGDATA, (long *) &pc);
		sp += mach->szaddr;	/*assumes sizeof(addr) = stack width*/
	}
	return 0;
}

long
rget(char *reg)
{
	Lsym *s;
	long x;

	s = look(reg);
	if(s == 0)
		fatal("rget: %s\n", reg);

	get4(cormap, s->v->ival, SEGDATA, &x);
	return x;
}
