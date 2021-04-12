/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "../port/portfns.h"

/* assembly code support from asm.S */
/* Startmach is passed an argument which is its own stack. */
void startmach(void (*f)(void *), void *m, void *stack);
Mach *machp(void);

/* other functions */
void intrac(Proc *);
void acinit(void);
int acpiinit(void);
void actrapenable(int, char *(*)(Ureg *, void *), void *, char *);
void apicipi(int);
void apicpri(int);
void acsysret(void);
void actouser(void);
void runacore(void);
void aamloop(int);
Dirtab *addarchfile(char *, int,
		    i32 (*)(Chan *, void *, i32, i64),
		    i32 (*)(Chan *, void *, i32, i64));
void acmmuswitch(void);
void acmodeset(int);
void archfmtinstall(void);
void archidle(void);
int archmmu(void);
int asmfree(u64, u64, int);
u64 asmalloc(u64, u64, int, int);
void asminit(void);
void asmmapinit(u64, u64, int);
extern void asmmodinit(u32, u32, char *);
void noerrorsleft(void);
void archinit(void);
void archreset(void);
i64 archhz(void);
int cgaprint(int off, char *fmt, ...);
int cgaclearln(int off, int c);
void cgaconsputs(char *, int);
void cgainit(void);
void cgapost(int);
void checkpa(char *, u64);
#define clearmmucache() /* x86 doesn't have one */
void (*coherence)(void);
int corecolor(int);
u32 cpuid(u32, u32, u32[4]);
int dbgprint(char *, ...);
int decref(Ref *);
void delay(int);
void dumpmmu(Proc *);
void dumpmmuwalk(u64 pa);
void dumpptepg(int lvl, usize pa);
#define evenaddr(x) /* x86 doesn't care */
void *findKSeg2(void);
int fpudevprocio(Proc *, void *, i32, usize, int);
void fpuinit(void);
void fpunoted(void);
void fpunotify(Ureg *);
void fpuprocrestore(Proc *);
void fpuprocsave(Proc *);
void fpusysprocsetup(Proc *);
void fpusysrfork(Ureg *);
void fpusysrforkchild(Proc *, Proc *);
Mach *getac(Proc *, int);
char *getconf(char *);
void gdb2ureg(usize *g, Ureg *u);
void halt(void);
void hardhalt(void);
void mouseenable(void);
void i8042systemreset(void);
int mousecmd(int);
void mouseenable(void);
void i8250console(char *);
void *i8250alloc(int, int, int);
i64 i8254hz(u32 *info0, u32 *info1);
void idlehands(void);
void acidthandlers(void);
void idthandlers(void);
int inb(int);
int incref(Ref *);
void insb(int, void *, int);
u16 ins(int);
void inss(int, void *, int);
u32 inl(int);
void insl(int, void *, int);
int intrdisable(void *);
void *intrenable(int, void (*)(Ureg *, void *), void *, int, char *);
void invlpg(usize);
void iofree(int);
void ioinit(void);
int iounused(int, int);
int ioalloc(int, int, int, char *);
int ioreserve(int, int, int, char *);
int iprint(char *, ...);
int isaconfig(char *, int, ISAConf *);
void kexit(Ureg *);
void keybenable(void);
void keybinit(void);
#define kmapinval()
void lfence(void);
void links(void);
void machinit(void);
void mach0init(void);
void mapraminit(u64, u64);
void mapupainit(u64, u32);
void meminit(void);
void mfence(void);
void mmuflushtlb(void);
void mmuinit(void);
usize mmukmap(usize, usize, usize);
int mmukmapsync(u64);
u64 mmuphysaddr(usize);
int mmuwalk(PTE *, usize, int, PTE **, PTE (*)(usize));
int multiboot(u32, u32, int);
void ndnr(void);
unsigned char nvramread(int);
void nvramwrite(int, unsigned char);
void optionsinit(char *);
void outb(int, int);
void outsb(int, void *, int);
void outs(int, u16);
void outss(int, void *, int);
void outl(int, u32);
void outsl(int, void *, int);
void pcireset(void);
int pickcore(int, int);
void printcpufreq(void);
void putac(Mach *);
void runapcore(int);
int screenprint(char *, ...); /* debugging */
void sfence(void);
void spldone(void);
u64 splhi(void);
u64 spllo(void);
void splx(u64);
void splxpc(u64);
void kstackok(void);	      /* panic if kstack guards garbaged, works with and without externup */
Stackframe *stackframe(void); /* asm.S */
void stacksnippet(void);
void stopac(void);
void syncclock(void);
void syscall(unsigned int scallnr, Ureg *ureg);
void *sysexecregs(usize, u32, void *);
usize sysexecstack(usize, int);
void sysprocsetup(Proc *);
void tssrsp0(Mach *, u64);
void trapenable(int, void (*)(Ureg *, void *), void *, char *);
void trapinit(void);
void trap(Ureg *);
void umeminit(void);
void ureg2gdb(Ureg *u, usize *g);
int userureg(Ureg *);
void *vmap(usize, usize);
void vsvminit(int, int, Mach *);
void vunmap(void *, usize);

extern u64 rootget(void);
extern void rootput(usize);
extern void idtput(int, u64);
extern u64 rdtsc(void);

extern int islo(void);
extern void spldone(void);
extern Mpl splhi(void);
extern Mpl spllo(void);
extern void splx(Mpl);

int cas32(void *, u32, u32);
int cas64(void *, u64, u64);
int tas32(void *);
u64 fas64(u64 *, u64);

#define CASU(p, e, n) cas64((p), (u64)(e), (u64)(n))
#define CASV(p, e, n) cas64((p), (u64)(e), (u64)(n))
#define CASP(p, e, n) cas64((p), (u64)(e), (u64)(n))
#define CASW(p, e, n) cas32((p), (e), (n))
#define TAS(addr) tas32((addr))
#define FASP(p, v) ((void *)fas64((u64 *)(p), (u64)(v)))

void touser(Ureg *);
void syscallentry(void);
void acsyscallentry(void);
void syscallreturn(void);
void sysrforkret(void);

#define waserror() setlabel(&up->errlab[up->nerrlab++])
#define poperror() up->nerrlab--

#define dcflush(a, b)

#define PTR2UINT(p) ((usize)(p))
#define UINT2PTR(i) ((void *)(i))

void *KADDR(usize);
u64 PADDR(void *);

#define BIOSSEG(a) KADDR(((uint)(a)) << 4)

/*
 * apic.c
 */
extern int apiceoi(int);
extern void apicipi(int);
extern void apicinit(int, u64, int);
extern int apicisr(int);
extern int apiconline(void);
extern void apicpri(int);
extern void apicsipi(int, u64);

extern void ioapicinit(int, u64);

/*
 * archamd64.c
 */
extern void millidelay(int);
extern void k10mwait(void *);

/*
 * i8259.c
 */
extern int i8259init(int);
extern int i8259irqdisable(int);
extern int i8259irqenable(int);
extern int i8259isr(int);

/*
 * mp.c
 */
extern void mpsinit(int);

/*
 * sipi.c
 */
extern void sipi(void);

/*
 * debug
 */
void HERE(void);
void DONE(void);

/* all these go to 0x3f8 */
void hi(char *s);

Mach *machp(void);
Proc *externup(void);

/* temporary. */
void die(char *);
void dumpgpr(Ureg *ureg);

/* debug support. */
int backtrace_list(usize pc, usize fp, usize *pcs, usize nr_slots);

/* horror */
static inline void
__clobber_callee_regs(void)
{
	// Arrived at through trial and error, this toolchain is a pain.
	asm volatile(""
		     :
		     :
		     : "x2", "x3", "x4", "x5", "x6", "x7", /*"x8", */ "x9", "x10", "x11", "x12", "x13" /*, "x15"*/);
	// TODO: f0-f15?
}

int slim_setlabel(Label *) __attribute__((returns_twice));

#define setlabel(label) ({int err;                                                 \
                    __clobber_callee_regs();                               \
                    err = slim_setlabel(label);                                     \
                    err; })

/* sbi calls. They are all too different to bother with a typedef. */
void sbi_set_mtimecmp(u64 t);
