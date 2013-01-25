#define checkmmu(a, b)
#define countpagerefs(a, b)

#include "../port/portfns.h"

extern int led(int, int);
extern void ledexit(int);
extern void delay(int);
extern void _uartputs(char*, int);
extern int _uartprint(char*, ...);

#pragma	varargck argpos	_uartprint 1

extern void archreboot(void);
extern void archreset(void);
extern void cachedinv(void);
extern void cachedinvse(void*, int);
extern void cachedwb(void);
extern void cachedwbinv(void);
extern void cachedwbinvse(void*, int);
extern void cachedwbse(void*, int);
extern void cacheiinv(void);
extern void cacheinfo(int level, Memcache *cp);
extern void cacheuwbinv(void);
extern uintptr cankaddr(uintptr pa);
extern void chkmissing(void);
extern void clockshutdown(void);
extern int clz(ulong);
extern int cmpswap(long*, long, long);
extern void coherence(void);
extern void configscreengpio(void);
extern u32int controlget(void);
extern u32int cpctget(void);
extern u32int cpidget(void);
extern ulong cprd(int cp, int op1, int crn, int crm, int op2);
extern ulong cprdsc(int op1, int crn, int crm, int op2);
extern void cpuidprint(void);
extern void cpwr(int cp, int op1, int crn, int crm, int op2, ulong val);
extern void cpwrsc(int op1, int crn, int crm, int op2, ulong val);
#define cycles(ip) *(ip) = lcycles()
extern u32int dacget(void);
extern void dacput(u32int);
extern void dmainit(void);
extern int dmastart(void *, int, void *, int, uint, Rendez *, int *);
extern void dmatest(void);
extern u32int farget(void);
extern ulong fprd(int fpreg);
extern void fpwr(int fpreg, ulong val);
extern u32int fsrget(void);
extern u32int getscr(void);
extern u32int getpsr(void);
extern ulong getwayssets(void);
extern u32int ifsrget(void);
extern void intrsoff(void);
extern int isaconfig(char*, int, ISAConf*);
extern int isdmadone(int);
extern int ispow2(uvlong);
extern void kbdenable(void);
extern void l2cacheuinv(void);
extern void l2cacheuwb(void);
extern void l2cacheuwbinv(void);
extern void lastresortprint(char *buf, long bp);
extern int log2(ulong);
extern void machinit(void);
extern void mmuidmap(uintptr phys, int mbs);
extern void mmuinvalidate(void);		/* 'mmu' or 'tlb'? */
extern void mmuinvalidateaddr(u32int);		/* 'mmu' or 'tlb'? */
extern void mousectl(Cmdbuf *cb);
extern u32int pidget(void);
extern void pidput(u32int);
extern vlong probeaddr(uintptr);
extern void procrestore(Proc *);
extern void procsave(Proc*);
extern void procsetup(Proc*);
extern void _reset(void);
extern void screenclockson(void);
extern void screeninit(void);
extern void serialputs(char* s, int n);
extern void setcachelvl(int);
extern void setr13(int, u32int*);
extern int tas(void *);
extern u32int ttbget(void);
extern void ttbput(u32int);
extern void watchdoginit(void);

extern int irqenable(int, void (*)(Ureg*, void*), void*, char*);
extern int irqdisable(int, void (*)(Ureg*, void*), void*, char*);
#define intrenable(i, f, a, b, n)	irqenable((i), (f), (a), (n))
#define intrdisable(i, f, a, b, n)	irqdisable((i), (f), (a), (n))
extern void vectors(void);
extern void vtable(void);

/* dregs, going away */
extern int inb(int);
extern void outb(int, int);

/*
 * Things called in main.
 */
extern void archconfinit(void);
extern void clockinit(void);
extern int i8250console(void);
extern void links(void);
extern void mmuinit(void);
extern void touser(uintptr);
extern void trapinit(void);


extern int fpiarm(Ureg*);
extern int fpudevprocio(Proc*, void*, long, uintptr, int);
extern void fpuinit(void);
extern void fpunoted(void);
extern void fpunotify(Ureg*);
extern void fpuprocrestore(Proc*);
extern void fpuprocsave(Proc*);
extern void fpusysprocsetup(Proc*);
extern void fpusysrfork(Ureg*);
extern void fpusysrforkchild(Proc*, Ureg*, Proc*);
extern int fpuemu(Ureg*);

/*
 * Miscellaneous machine dependent stuff.
 */
extern char* getenv(char*, char*, int);
char*	getconf(char*);
uintptr mmukmap(uintptr, uintptr, usize);
uintptr mmukunmap(uintptr, uintptr, usize);
extern void* mmuuncache(void*, usize);
extern void* ucalloc(usize);
extern Block* ucallocb(int);
extern void* ucallocalign(usize size, int align, int span);
extern void ucfree(void*);
extern void ucfreeb(Block*);

/*
 * Things called from port.
 */
extern void delay(int);				/* only scheddump() */
extern int islo(void);
extern void microdelay(int);			/* only edf.c */
extern void evenaddr(uintptr);
extern void idlehands(void);
extern void setkernur(Ureg*, Proc*);		/* only devproc.c */
extern void* sysexecregs(uintptr, ulong, int);
extern void sysprocsetup(Proc*);

/*
 * PCI stuff.
 */

int	cas32(void*, u32int, u32int);
int	tas32(void*);

#define CASU(p, e, n)	cas32((p), (u32int)(e), (u32int)(n))
#define CASV(p, e, n)	cas32((p), (u32int)(e), (u32int)(n))
#define CASW(addr, exp, new)	cas32((addr), (exp), (new))
#define TAS(addr)	tas32(addr)

extern void forkret(void);
extern int userureg(Ureg*);
void*	vmap(uintptr, usize);
void	vunmap(void*, usize);

extern void kexit(Ureg*);

#define	getpgcolor(a)	0
#define	kmapinval()

#define PTR2UINT(p)	((uintptr)(p))
#define UINT2PTR(i)	((void*)(i))

#define	waserror()	(up->nerrlab++, setlabel(&up->errlab[up->nerrlab-1]))

#define KADDR(pa)	UINT2PTR(KZERO    | ((uintptr)(pa) & ~KSEGM))
#define PADDR(va)	PTR2UINT(PHYSDRAM | ((uintptr)(va) & ~KSEGM))

#define wave(c) *(ulong *)PHYSCONS = (c)

#define MASK(v)	((1UL << (v)) - 1)	/* mask `v' bits wide */
