/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * sparc sim.h
 *
 * The integer instruction side of this emulator is portable if sizeof(long) >=
 *4
 * Floating point emulation however is not. Assumptions made are:
 * sizeof(ulong) == sizeof(float)
 * sizeof(ulong)*2 == sizeof(double)
 * bits of floating point in memory may be reversed lsw/msw
 * unions of double & 2*float & 2*long have no padding
 */
#include "/sparc/include/ureg.h"
#define USERADDR 0xC0000000
#define UREGADDR (USERADDR + BY2PG - 4 - 0xA0)
#define USER_REG(x) (UREGADDR + (ulong)(x))
#define REGOFF(x) (USER_REG(&((struct Ureg*)0)->x))

typedef struct Registers Registers;
typedef struct Segment Segment;
typedef struct Memory Memory;
typedef struct Mul Mul;
typedef struct Mulu Mulu;
typedef struct Inst Inst;
typedef struct Icache Icache;
typedef struct Breakpoint Breakpoint;

enum { Instruction = 1,
       Read = 2,
       Write = 4,
       Access = 2 | 4,
       Equal = 4 | 8,
};

struct Breakpoint {
	int type;         /* Instruction/Read/Access/Write/Equal */
	uint32_t addr;    /* Place at address */
	int count;        /* To execute count times or value */
	int done;         /* How many times passed through */
	Breakpoint* next; /* Link to next one */
};

enum { Iload,
       Istore,
       Iarith,
       Ibranch,
       Ireg,
       Isyscall,
       Ifloat,
       Inop,
};

struct Icache {
	int on;                 /* Turned on */
	int linesize;           /* Line size in bytes */
	int stall;              /* Cache stalls */
	int* lines;             /* Tag array */
	int* (*hash)(uint32_t); /* Hash function */
	char* hashtext;         /* What the function looks like */
};

struct Inst {
	void (*func)(uint32_t);
	char* name;
	int type;
	int count;
	int taken;
	int useddelay;
};

struct Registers {
	uint32_t pc;
	uint32_t ir;
	Inst* ip;
	long r[32];
	uint32_t Y;
	uint32_t psr;
	uint32_t fpsr;

	union {
		double fd[16];
		float fl[32];
		uint32_t di[32];
	};
};

struct Mulu {
	uint32_t lo;
	uint32_t hi;
};

struct Mul {
	long lo;
	long hi;
};

enum { MemRead,
       MemReadstring,
       MemWrite,
};

enum { Stack,
       Text,
       Data,
       Bss,
       Nseg,
};

struct Segment {
	short type;
	uint32_t base;
	uint32_t end;
	uint32_t fileoff;
	uint32_t fileend;
	int rss;
	int refs;
	uchar** table;
};

struct Memory {
	Segment seg[Nseg];
};

void fatal(int, char*, ...);
void run(void);
void undef(uint32_t);
void dumpreg(void);
void dumpfreg(void);
void dumpdreg(void);
void* emalloc(uint32_t);
void* erealloc(void*, uint32_t, uint32_t);
void* vaddr(uint32_t);
void itrace(char*, ...);
void segsum(void);
void ta(uint32_t);
char* memio(char*, uint32_t, int, int);
uint32_t getmem_w(uint32_t);
uint32_t ifetch(uint32_t);
ushort getmem_h(uint32_t);
void putmem_w(uint32_t, uint32_t);
uchar getmem_b(uint32_t);
void putmem_b(uint32_t, uchar);
uint32_t getmem_4(uint32_t);
uint32_t getmem_2(uint32_t);
void putmem_h(uint32_t, short);
Mul mul(long, long);
Mulu mulu(uint32_t, uint32_t);
void isum(void);
void initicache(void);
void updateicache(uint32_t addr);
long lnrand(long);
void randseed(long, long);
void cmd(void);
void brkchk(uint32_t, int);
void delbpt(char*);
void breakpoint(char*, char*);
char* nextc(char*);
uint32_t expr(char*);
void initstk(int, char**);
void initmap(void);
void inithdr(int);
void reset(void);
void dobplist(void);
void procinit(int);
void printsource(long);
void printparams(Symbol*, uint32_t);
void printlocals(Symbol*, uint32_t);
void stktrace(int);
void delay(uint32_t);
void iprofile(void);

/* Globals */
Extern Registers reg;
Extern Memory memory;
Extern int text;
Extern int trace;
Extern int sysdbg;
Extern int calltree;
Extern Icache icache;
Extern int count;
Extern jmp_buf errjmp;
Extern Breakpoint* bplist;
Extern int atbpt;
Extern int membpt;
Extern int cmdcount;
Extern int nopcount;
Extern ulong dot;
extern char* file;
Extern Biobuf* bioout;
Extern Biobuf* bin;
Extern Inst* ci;
Extern ulong* iprof;
Extern ulong loadlock;
Extern ulong anulled;
extern int datasize;
extern int printcol;
Extern Map* symmap;

/* Plan9 Kernel constants */
#define BY2PG 4096
#define BY2WD 4
#define UTZERO 0x1000
#define TSTKSIZ 32
#define TSTACKTOP 0x10000000
#define STACKTOP (TSTACKTOP - TSTKSIZ * BY2PG)
#define STACKSIZE (4 * 1024 * 1024)

#define ANUL (1 << 29)
#define PROFGRAN 4
#define NOP 0x80300000
#define SIGNBIT 0x80000000
#define IMMBIT (1 << 13)
#define getrop23(i)                                                            \
	rd = (i >> 25) & 0x1f;                                                 \
	rs1 = (i >> 14) & 0x1f;                                                \
	rs2 = i & 0x1f;
#define ximm(xx, ii)                                                           \
	xx = ii & 0x1FFF;                                                      \
	if(xx & 0x1000)                                                        \
	xx |= ~0x1FFF

#define PSR_n (1 << 23)
#define PSR_z (1 << 22)
#define PSR_v (1 << 21)
#define PSR_c (1 << 20)

#define FP_U 3
#define FP_L 1
#define FP_G 2
#define FP_E 0
