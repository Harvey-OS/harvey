/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * arm.h
 */
#ifndef EXTERN
#define EXTERN extern
#endif

typedef struct Registers Registers;
typedef struct Segment Segment;
typedef struct Memory Memory;
typedef struct Mul Mul;
typedef struct Mulu Mulu;
typedef struct Inst Inst;
typedef struct Icache Icache;
typedef struct Tlb Tlb;
typedef struct Breakpoint Breakpoint;

enum {
	Instruction = 1,
	Read = 2,
	Write = 4,
	Access = 2 | 4,
	Equal = 4 | 8,
};

struct Breakpoint {
	int type;	 /* Instruction/Read/Access/Write/Equal */
	uint32_t addr;    /* Place at address */
	int count;	/* To execute count times or value */
	int done;	 /* How many times passed through */
	Breakpoint *next; /* Link to next one */
};

enum {
	Imem,
	Iarith,
	Ibranch,
	Isyscall,
};

enum {
	Nmaxtlb = 64,
	REGARG = 0,
	REGRET = 0,
	REGPC = 15,
	REGLINK = 14,
	REGSP = 13,
};

struct Tlb {
	int on;			  /* Being updated */
	int tlbsize;		  /* Number of entries */
	uint32_t tlbent[Nmaxtlb]; /* Virtual address tags */
	int hit;		  /* Number of successful tag matches */
	int miss;		  /* Number of failed tag matches */
};

struct Icache {
	int on;			/* Turned on */
	int linesize;		/* Line size in bytes */
	int stall;		/* Cache stalls */
	int *lines;		/* Tag array */
	int *(*hash)(uint32_t); /* Hash function */
	char *hashtext;		/* What the function looks like */
};

struct Inst {
	void (*func)(uint32_t);
	char *name;
	int type;
	int count;
	int taken;
	int useddelay;
};

struct Registers {
	uint32_t ar;
	uint32_t ir;
	Inst *ip;
	long r[16];
	long cc1;
	long cc2;
	int class;
	int cond;
	int compare_op;
	int cbit;
	int cout;
};

enum {
	FPd = 0,
	FPs,
	FPmemory,
};

enum {
	MemRead,
	MemReadstring,
	MemWrite,
};

enum {
	CCcmp,
	CCtst,
	CCteq,
};

enum {
	Stack,
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
	uchar **table;
};

struct Memory {
	Segment seg[Nseg];
};

void Ssyscall(uint32_t);
int armclass(long);
void breakpoint(char *, char *);
void brkchk(uint32_t, int);
void cmd(void);
void delbpt(char *);
void dobplist(void);
void dumpdreg(void);
void dumpfreg(void);
void dumpreg(void);
void *emalloc(uint32_t);
void *erealloc(void *, uint32_t, uint32_t);
uint32_t expr(char *);
void fatal(int, char *, ...);
uint32_t getmem_2(uint32_t);
uint32_t getmem_4(uint32_t);
uchar getmem_b(uint32_t);
ushort getmem_h(uint32_t);
uvlong getmem_v(uint32_t);
uint32_t getmem_w(uint32_t);
uint32_t ifetch(uint32_t);
void inithdr(int);
void initicache(void);
void initmap(void);
void initstk(int, char **);
void iprofile(void);
void isum(void);
void itrace(char *, ...);
long lnrand(long);
char *memio(char *, uint32_t, int, int);
int _mipscoinst(Map *, uint32_t, char *, int);
Mul mul(long, long);
Mulu mulu(uint32_t, uint32_t);
char *nextc(char *);
void printlocals(Symbol *, uint32_t);
void printparams(Symbol *, uint32_t);
void printsource(long);
void procinit(int);
void putmem_b(uint32_t, uchar);
void putmem_h(uint32_t, ushort);
void putmem_v(uint32_t, uvlong);
void putmem_w(uint32_t, uint32_t);
void reset(void);
void run(void);
void segsum(void);
void stktrace(int);
void tlbsum(void);
void undef(uint32_t);
void updateicache(uint32_t addr);
void *vaddr(uint32_t);

/* Globals */
EXTERN Registers reg;
EXTERN Memory memory;
EXTERN int text;
EXTERN int trace;
EXTERN int sysdbg;
EXTERN int calltree;
EXTERN Inst itab[];
EXTERN Inst ispec[];
EXTERN Icache icache;
EXTERN Tlb tlb;
EXTERN int count;
EXTERN jmp_buf errjmp;
EXTERN Breakpoint *bplist;
EXTERN int atbpt;
EXTERN int membpt;
EXTERN int cmdcount;
EXTERN int nopcount;
EXTERN uint32_t dot;
EXTERN char *file;
EXTERN Biobuf *bioout;
EXTERN Biobuf *bin;
EXTERN uint32_t *iprof;
EXTERN int datasize;
EXTERN Map *symmap;

/* Plan9 Kernel constants */
enum {
	BY2PG = 4096,
	BY2WD = 4,
	UTZERO = 0x1000,
	STACKTOP = 0x80000000,
	STACKSIZE = 0x10000,

	PROFGRAN = 4,
	Sbit = 1 << 20,
	SIGNBIT = 0x80000000,

	FP_U = 3,
	FP_L = 1,
	FP_G = 2,
	FP_E = 0,
	FP_CBIT = 1 << 23,
};
