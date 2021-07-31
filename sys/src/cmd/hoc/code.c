#include <u.h>
#include <libc.h>
#include <bio.h>
#include "hoc.h"
#include "y.tab.h"

#define	NSTACK	256

static Datum stack[NSTACK];	/* the stack */
static Datum *stackp;		/* next free spot on stack */

#define	NPROG	2000
Inst	prog[NPROG];	/* the machine */
Inst	*progp;		/* next free spot for code generation */
Inst	*pc;		/* program counter during execution */
Inst	*progbase = prog; /* start of current subprogram */
int	returning;	/* 1 if return stmt seen */
int	indef;	/* 1 if parsing a func or proc */

typedef struct Frame {	/* proc/func call stack frame */
	Symbol	*sp;	/* symbol table entry */
	Inst	*retpc;	/* where to resume after return */
	Datum	*argn;	/* n-th argument on stack */
	int	nargs;	/* number of arguments */
} Frame;
#define	NFRAME	100
Frame	frame[NFRAME];
Frame	*fp;		/* frame pointer */

void
initcode(void)
{
	progp = progbase;
	stackp = stack;
	fp = frame;
	returning = 0;
	indef = 0;
}

void
push(Datum d)
{
	if (stackp >= &stack[NSTACK])
		execerror("stack too deep", 0);
	*stackp++ = d;
}

Datum
pop(void)
{
	if (stackp == stack)
		execerror("stack underflow", 0);
	return *--stackp;
}

void
xpop(void)	/* for when no value is wanted */
{
	if (stackp == stack)
		execerror("stack underflow", (char *)0);
	--stackp;
}

void
constpush(void)
{
	Datum d;
	d.val = ((Symbol *)*pc++)->u.val;
	push(d);
}

void
varpush(void)
{
	Datum d;
	d.sym = (Symbol *)(*pc++);
	push(d);
}

void
whilecode(void)
{
	Datum d;
	Inst *savepc = pc;

	execute(savepc+2);	/* condition */
	d = pop();
	while (d.val) {
		execute(*((Inst **)(savepc)));	/* body */
		if (returning)
			break;
		execute(savepc+2);	/* condition */
		d = pop();
	}
	if (!returning)
		pc = *((Inst **)(savepc+1)); /* next stmt */
}

void
forcode(void)
{
	Datum d;
	Inst *savepc = pc;

	execute(savepc+4);		/* precharge */
	pop();
	execute(*((Inst **)(savepc)));	/* condition */
	d = pop();
	while (d.val) {
		execute(*((Inst **)(savepc+2)));	/* body */
		if (returning)
			break;
		execute(*((Inst **)(savepc+1)));	/* post loop */
		pop();
		execute(*((Inst **)(savepc)));	/* condition */
		d = pop();
	}
	if (!returning)
		pc = *((Inst **)(savepc+3)); /* next stmt */
}

void
ifcode(void) 
{
	Datum d;
	Inst *savepc = pc;	/* then part */

	execute(savepc+3);	/* condition */
	d = pop();
	if (d.val)
		execute(*((Inst **)(savepc)));	
	else if (*((Inst **)(savepc+1))) /* else part? */
		execute(*((Inst **)(savepc+1)));
	if (!returning)
		pc = *((Inst **)(savepc+2)); /* next stmt */
}

void
define(Symbol* sp)	/* put func/proc in symbol table */
{
	sp->u.defn = progbase;	/* start of code */
	progbase = progp;	/* next code starts here */
}

void
call(void) 		/* call a function */
{
	Symbol *sp = (Symbol *)pc[0]; /* symbol table entry */
				      /* for function */
	if (fp++ >= &frame[NFRAME-1])
		execerror(sp->name, "call nested too deeply");
	fp->sp = sp;
	fp->nargs = (int)pc[1];
	fp->retpc = pc + 2;
	fp->argn = stackp - 1;	/* last argument */
	execute(sp->u.defn);
	returning = 0;
}

static void
ret(void) 		/* common return from func or proc */
{
	int i;
	for (i = 0; i < fp->nargs; i++)
		pop();	/* pop arguments */
	pc = (Inst *)fp->retpc;
	--fp;
	returning = 1;
}

void
funcret(void) 	/* return from a function */
{
	Datum d;
	if (fp->sp->type == PROCEDURE)
		execerror(fp->sp->name, "(proc) returns value");
	d = pop();	/* preserve function return value */
	ret();
	push(d);
}

void
procret(void) 	/* return from a procedure */
{
	if (fp->sp->type == FUNCTION)
		execerror(fp->sp->name,
			"(func) returns no value");
	ret();
}

double*
getarg(void) 	/* return pointer to argument */
{
	int nargs = (int) *pc++;
	if (nargs > fp->nargs)
	    execerror(fp->sp->name, "not enough arguments");
	return &fp->argn[nargs - fp->nargs].val;
}

void
arg(void) 	/* push argument onto stack */
{
	Datum d;
	d.val = *getarg();
	push(d);
}

void
argassign(void) 	/* store top of stack in argument */
{
	Datum d;
	d = pop();
	push(d);	/* leave value on stack */
	*getarg() = d.val;
}

void
argaddeq(void) 	/* store top of stack in argument */
{
	Datum d;
	d = pop();
	d.val = *getarg() += d.val;
	push(d);	/* leave value on stack */
}

void
argsubeq(void) 	/* store top of stack in argument */
{
	Datum d;
	d = pop();
	d.val = *getarg() -= d.val;
	push(d);	/* leave value on stack */
}

void
argmuleq(void) 	/* store top of stack in argument */
{
	Datum d;
	d = pop();
	d.val = *getarg() *= d.val;
	push(d);	/* leave value on stack */
}

void
argdiveq(void) 	/* store top of stack in argument */
{
	Datum d;
	d = pop();
	d.val = *getarg() /= d.val;
	push(d);	/* leave value on stack */
}

void
argmodeq(void) 	/* store top of stack in argument */
{
	Datum d;
	double *x;
	long y;
	d = pop();
	/* d.val = *getarg() %= d.val; */
	x = getarg();
	y = *x;
	d.val = *x = y % (long) d.val;
	push(d);	/* leave value on stack */
}

void
bltin(void) 
{

	Datum d;
	d = pop();
	d.val = (*(double (*)(double))*pc++)(d.val);
	push(d);
}

void
add(void)
{
	Datum d1, d2;
	d2 = pop();
	d1 = pop();
	d1.val += d2.val;
	push(d1);
}

void
sub(void)
{
	Datum d1, d2;
	d2 = pop();
	d1 = pop();
	d1.val -= d2.val;
	push(d1);
}

void
mul(void)
{
	Datum d1, d2;
	d2 = pop();
	d1 = pop();
	d1.val *= d2.val;
	push(d1);
}

void
div(void)
{
	Datum d1, d2;
	d2 = pop();
	if (d2.val == 0.0)
		execerror("division by zero", (char *)0);
	d1 = pop();
	d1.val /= d2.val;
	push(d1);
}

void
mod(void)
{
	Datum d1, d2;
	long x;
	d2 = pop();
	if (d2.val == 0.0)
		execerror("division by zero", (char *)0);
	d1 = pop();
	/* d1.val %= d2.val; */
	x = d1.val;
	x %= (long) d2.val;
	d1.val = d2.val = x;
	push(d1);
}

void
negate(void)
{
	Datum d;
	d = pop();
	d.val = -d.val;
	push(d);
}

void
verify(Symbol* s)
{
	if (s->type != VAR && s->type != UNDEF)
		execerror("attempt to evaluate non-variable", s->name);
	if (s->type == UNDEF)
		execerror("undefined variable", s->name);
}

void
eval(void)		/* evaluate variable on stack */
{
	Datum d;
	d = pop();
	verify(d.sym);
	d.val = d.sym->u.val;
	push(d);
}

void
preinc(void)
{
	Datum d;
	d.sym = (Symbol *)(*pc++);
	verify(d.sym);
	d.val = d.sym->u.val += 1.0;
	push(d);
}

void
predec(void)
{
	Datum d;
	d.sym = (Symbol *)(*pc++);
	verify(d.sym);
	d.val = d.sym->u.val -= 1.0;
	push(d);
}

void
postinc(void)
{
	Datum d;
	double v;
	d.sym = (Symbol *)(*pc++);
	verify(d.sym);
	v = d.sym->u.val;
	d.sym->u.val += 1.0;
	d.val = v;
	push(d);
}

void
postdec(void)
{
	Datum d;
	double v;
	d.sym = (Symbol *)(*pc++);
	verify(d.sym);
	v = d.sym->u.val;
	d.sym->u.val -= 1.0;
	d.val = v;
	push(d);
}

void
gt(void)
{
	Datum d1, d2;
	d2 = pop();
	d1 = pop();
	d1.val = (double)(d1.val > d2.val);
	push(d1);
}

void
lt(void)
{
	Datum d1, d2;
	d2 = pop();
	d1 = pop();
	d1.val = (double)(d1.val < d2.val);
	push(d1);
}

void
ge(void)
{
	Datum d1, d2;
	d2 = pop();
	d1 = pop();
	d1.val = (double)(d1.val >= d2.val);
	push(d1);
}

void
le(void)
{
	Datum d1, d2;
	d2 = pop();
	d1 = pop();
	d1.val = (double)(d1.val <= d2.val);
	push(d1);
}

void
eq(void)
{
	Datum d1, d2;
	d2 = pop();
	d1 = pop();
	d1.val = (double)(d1.val == d2.val);
	push(d1);
}

void
ne(void)
{
	Datum d1, d2;
	d2 = pop();
	d1 = pop();
	d1.val = (double)(d1.val != d2.val);
	push(d1);
}

void
and(void)
{
	Datum d1, d2;
	d2 = pop();
	d1 = pop();
	d1.val = (double)(d1.val != 0.0 && d2.val != 0.0);
	push(d1);
}

void
or(void)
{
	Datum d1, d2;
	d2 = pop();
	d1 = pop();
	d1.val = (double)(d1.val != 0.0 || d2.val != 0.0);
	push(d1);
}

void
not(void)
{
	Datum d;
	d = pop();
	d.val = (double)(d.val == 0.0);
	push(d);
}

void
power(void)
{
	Datum d1, d2;
	d2 = pop();
	d1 = pop();
	d1.val = Pow(d1.val, d2.val);
	push(d1);
}

void
assign(void)
{
	Datum d1, d2;
	d1 = pop();
	d2 = pop();
	if (d1.sym->type != VAR && d1.sym->type != UNDEF)
		execerror("assignment to non-variable",
			d1.sym->name);
	d1.sym->u.val = d2.val;
	d1.sym->type = VAR;
	push(d2);
}

void
addeq(void)
{
	Datum d1, d2;
	d1 = pop();
	d2 = pop();
	if (d1.sym->type != VAR && d1.sym->type != UNDEF)
		execerror("assignment to non-variable",
			d1.sym->name);
	d2.val = d1.sym->u.val += d2.val;
	d1.sym->type = VAR;
	push(d2);
}

void
subeq(void)
{
	Datum d1, d2;
	d1 = pop();
	d2 = pop();
	if (d1.sym->type != VAR && d1.sym->type != UNDEF)
		execerror("assignment to non-variable",
			d1.sym->name);
	d2.val = d1.sym->u.val -= d2.val;
	d1.sym->type = VAR;
	push(d2);
}

void
muleq(void)
{
	Datum d1, d2;
	d1 = pop();
	d2 = pop();
	if (d1.sym->type != VAR && d1.sym->type != UNDEF)
		execerror("assignment to non-variable",
			d1.sym->name);
	d2.val = d1.sym->u.val *= d2.val;
	d1.sym->type = VAR;
	push(d2);
}

void
diveq(void)
{
	Datum d1, d2;
	d1 = pop();
	d2 = pop();
	if (d1.sym->type != VAR && d1.sym->type != UNDEF)
		execerror("assignment to non-variable",
			d1.sym->name);
	d2.val = d1.sym->u.val /= d2.val;
	d1.sym->type = VAR;
	push(d2);
}

void
modeq(void)
{
	Datum d1, d2;
	long x;
	d1 = pop();
	d2 = pop();
	if (d1.sym->type != VAR && d1.sym->type != UNDEF)
		execerror("assignment to non-variable",
			d1.sym->name);
	/* d2.val = d1.sym->u.val %= d2.val; */
	x = d1.sym->u.val;
	x %= (long) d2.val;
	d2.val = d1.sym->u.val = x;
	d1.sym->type = VAR;
	push(d2);
}

void
printtop(void)	/* pop top value from stack, print it */
{
	Datum d;
	static Symbol *s;	/* last value computed */
	if (s == 0)
		s = install("_", VAR, 0.0);
	d = pop();
	print("%.12g\n", d.val);
	s->u.val = d.val;
}

void
prexpr(void)	/* print numeric value */
{
	Datum d;
	d = pop();
	print("%.12g ", d.val);
}

void
prstr(void)		/* print string value */ 
{
	print("%s", (char *) *pc++);
}

void
varread(void)	/* read into variable */
{
	Datum d;
	extern Biobuf *bin;
	Symbol *var = (Symbol *) *pc++;
	int c;

  Again:
	do
		c = Bgetc(bin);
	while(c==' ' || c=='\t');
	if(c == Beof){
  Iseof:
		if(moreinput())
			goto Again;
		d.val = var->u.val = 0.0;
		goto Return;
	}

	if(strchr("+-.0123456789", c) == 0)
		execerror("non-number read into", var->name);
	Bungetc(bin);
	if(Bgetd(bin, &var->u.val) == Beof)
		goto Iseof;
	else
		d.val = 1.0;
  Return:
	var->type = VAR;
	push(d);
}

Inst*
code(Inst f)	/* install one instruction or operand */
{
	Inst *oprogp = progp;
	if (progp >= &prog[NPROG])
		execerror("program too big", (char *)0);
	*progp++ = f;
	return oprogp;
}

void
execute(Inst* p)
{
	for (pc = p; *pc != STOP && !returning; )
		(*((++pc)[-1]))();
}
