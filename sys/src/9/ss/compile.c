#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

#define	NCODE	1024
static	ulong	code[NCODE];
static	ulong	*codep;

ulong	(*getcontext)(void);
void	(*putcontext)(ulong);
void	(*putenab)(ulong);
ulong	(*getenab)(void);
ulong	(*getsysspace)(ulong);
void	(*putsysspace)(ulong, ulong);
uchar	(*getsysspaceb)(ulong);
void	(*putsysspaceb)(ulong, uchar);
ulong	(*getsegspace)(ulong);
void	(*putsegspace)(ulong, ulong);
ulong	(*getpmegspace)(ulong);
void	(*putpmegspace)(ulong, ulong);
ulong	(*flushpg)(ulong);

/*
 * Compile MMU code for this machine, since the MMU can only
 * be addressed from parameterless machine instructions.
 * What's wrong with MMUs you can talk to from C?
 */

/* op3 */
#define	LD	0
#define	ADD	0
#define	OR	2
#define	LDA	16
#define	LDUBA	17
#define	STA	20
#define	STBA	21
#define	JMPL	56
/* op2 */
#define	SETHI	4

#define	ret()	{*codep++ = (2<<30)|(0<<25)|(JMPL<<19)|(15<<14)|(1<<13)|8;}
#define	nop()	{*codep++ = (0<<30)|(0<<25)|(SETHI<<22)|(0>>10);}

static void
parameter(int param, int reg)
{
	param += 1;	/* 0th parameter is 1st word on stack */
	param *= 4;
	/* LD #param(R1), Rreg */
	*codep++ = (3<<30)|(reg<<25)|(LD<<19)|(1<<14)|(1<<13)|param;
}

static void
constant(ulong c, int reg)
{
	*codep++ = (0<<30)|(reg<<25)|(SETHI<<22)|(c>>10);
	if(c & 0x3FF)
		*codep++ = (2<<30)|(reg<<25)|(OR<<19)|(reg<<14)|(1<<13)|(c&0x3FF);
}

/*
 * value to/from constant address
 * void f(int c) { *(word*,asi)addr = c } for stores
 * ulong f(void)  { return *(word*,asi)addr } for loads
 */
static void*
compileconst(int op3, ulong addr, int asi)
{
	void *a;

	a = codep;
	constant(addr, 8);	/* MOVW $CONSTANT, R8 */
	ret();			/* JMPL 8(R15), R0 */
	/* in delay slot 	   st or ld R7, (R8+R0, asi)	*/
	*codep++ = (3<<30)|(7<<25)|(op3<<19)|(8<<14)|(asi<<5);
	return a;
}

/*
 * value to parameter address
 * void f(ulong addr, int c) { *(word*,asi)addr = c }
 */
static void*
compilestaddr(int op3, int asi)
{
	void *a;

	a = codep;
	parameter(1, 8);	/* MOVW (4*1)(FP), R8 */
	ret();			/* JMPL 8(R15), R0 */
	/* in delay slot 	   st R8, (R7+R0, asi)	*/
	*codep++ = (3<<30)|(8<<25)|(op3<<19)|(7<<14)|(asi<<5);
	return a;
}

/*
 * value from parameter address
 * ulong f(ulong addr)  { return *(word*,asi)addr }
 */
static void*
compileldaddr(int op3, int asi)
{
	void *a;

	a = codep;
	ret();			/* JMPL 8(R15), R0 */
	/* in delay slot 	   ld (R7+R0, asi), R7	*/
	*codep++ = (3<<30)|(7<<25)|(op3<<19)|(7<<14)|(asi<<5);
	return a;
}

/*
 * 1 store of zero
 * ulong f(ulong addr) { *addr=0; addr+=offset; return addr}
 * offset can be anything
 */
static void*
compile1(ulong offset, int asi)
{
	void *a;

	a = codep;
	/* ST R0, (R7+R0, asi)	*/
	*codep++ = (3<<30)|(0<<25)|(STA<<19)|(7<<14)|(asi<<5);
	if(offset < (1<<12)){
		ret();			/* JMPL 8(R15), R0 */
		/* in delay slot ADD $offset, R7 */
		*codep++ = (2<<30)|(7<<25)|(ADD<<19)|(7<<14)|(1<<13)|offset;
	}else{
		constant(offset, 8);
		ret();			/* JMPL 8(R15), R0 */
		/* in delay slot ADD R8, R7 */
		*codep++ = (2<<30)|(7<<25)|(ADD<<19)|(7<<14)|(0<<13)|8;
	}
	return a;
}

/*
 * 16 stores of zero
 * ulong f(ulong addr) { for(i=0;i<16;i++) {*addr=0; addr+=offset}; return addr}
 * offset must be less than 1<<12
 */
static void*
compile16(ulong offset, int asi)
{
	void *a;
	int i;

	a = codep;
	for(i=0; i<16; i++){
		/* ST R0, (R7+R0, asi)	*/
		*codep++ = (3<<30)|(0<<25)|(STA<<19)|(7<<14)|(asi<<5);
		/* ADD $offset, R7 */
		*codep++ = (2<<30)|(7<<25)|(ADD<<19)|(7<<14)|(1<<13)|offset;
	}
	ret();			/* JMPL 8(R15), R0 */
	nop();
	return a;
}

void
compile(void)
{
	codep = code;

	getcontext = compileconst(LDUBA, CONTEXT, 2);
	putcontext = compileconst(STBA, CONTEXT, 2);
	getenab = compileconst(LDUBA, ENAB, 2);
	putenab = compileconst(STBA, ENAB, 2);
	getsysspace = compileldaddr(LDA, 2);
	putsysspace = compilestaddr(STA, 2);
	getsysspaceb = compileldaddr(LDUBA, 2);
	putsysspaceb = compilestaddr(STBA, 2);
	getsegspace = compileldaddr(LDUBA, 3);
	putsegspace = compilestaddr(STBA, 3);
	getpmegspace = compileldaddr(LDA, 4);
	putpmegspace = compilestaddr(STA, 4);
	if(conf.ss2)
		flushpg = compile1(BY2PG, 6);
	else
		flushpg = compile16(conf.vaclinesize, 0xD);
}
