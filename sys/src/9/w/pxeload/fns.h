/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

void aamloop(int);
void addconf(char*, ...);
Alarm* alarm(int, void (*)(Alarm*), void*);
void alarminit(void);
Block* allocb(int);
void apminit(void);
int bootpboot(int, char*, Boot*);
int bootpass(Boot*, void*, int);
void cancel(Alarm*);
int cdinit(void);
void check(char*);
void cgaconsputs(char*, int);
void cgainit(void);
void cgapost(int);
int cistrcmp(char*, char*);
int cistrncmp(char*, char*, int);
void changeconf(char*, ...);
void checkalarms(void);
void clockinit(void);
#define coherence() mb386()
void consdrain(void);
int consinit(char*, char*);
void consputs(char*, int);
void delay(int);
uchar* etheraddr(int);
int etherinit(void);
void etherinitdev(int, char*);
void etherprintdevs(int);
int etherrxflush(int);
int etherrxpkt(int, Etherpkt*, int);
int ethertxpkt(int, Etherpkt*, int, int);
#define evenaddr(x) /* 386 doesn't care */
int floppyboot(int, char*, Boot*);
int floppyinit(void);
void floppyinitdev(int, char*);
void floppyprintdevs(int);
void* floppygetfspart(int, char*, int);
void freeb(Block*);
char* getconf(char*);
uint32_t getcr0(void);
uint32_t getcr2(void);
uint32_t getcr3(void);
uint32_t getcr4(void);
int getfields(char*, char**, int, char);
int getstr(char*, char*, int, char*, int);
int gunzip(uchar*, int, uchar*, int);
void i8042a20(void);
void i8042init(void);
void i8042reset(void);
void* ialloc(uint32_t, int);
void idle(void);
void ilock(Lock*);
void impulse(void);
int inb(int);
ushort ins(int);
uint32_t inl(int);
void insb(int, void*, int);
void inss(int, void*, int);
void insl(int, void*, int);
#define ioalloc(addr, len, align, name) (addr)
#define iofree(addr)
void iunlock(Lock*);
int isaconfig(char*, int, ISAConf*);
void kbdinit(void);
void kbdchar(int);
void lgdt(ushort[3]);
void lidt(ushort[3]);
void machinit(void);
void mapraminit(uvlong, uint32_t);
void mapupainit(uvlong, uint32_t);
void mb386(void);
void meminit(u32int);
void microdelay(int);
void mkmultiboot(void);
void mmuinit(void);
#define nelem(x) (sizeof(x) / sizeof(x[0]))
char* nextelem(char*, char*);
uchar nvramread(int);
void outb(int, int);
void outs(int, ushort);
void outl(int, uint32_t);
void outsb(int, void*, int);
void outss(int, void*, int);
void outsl(int, void*, int);
void panic(char*, ...);
uint32_t pcibarsize(Pcidev*, int);
int pcicfgr8(Pcidev*, int);
int pcicfgr16(Pcidev*, int);
int pcicfgr32(Pcidev*, int);
void pcicfgw8(Pcidev*, int, int);
void pcicfgw16(Pcidev*, int, int);
void pcicfgw32(Pcidev*, int, int);
void pciclrbme(Pcidev*);
void pciclrioe(Pcidev*);
void pciclrmwi(Pcidev*);
int pcigetpms(Pcidev*);
void pcihinv(Pcidev*);
uchar pciipin(Pcidev*, uchar);
Pcidev* pcimatch(Pcidev*, int, int);
Pcidev* pcimatchtbdf(int);
void pcireset(void);
void pcisetbme(Pcidev*);
void pcisetioe(Pcidev*);
void pcisetmwi(Pcidev*);
int pcisetpms(Pcidev*, int);
void pcmcisread(PCMslot*);
int pcmcistuple(int, int, int, void*, int);
PCMmap* pcmmap(int, uint32_t, int, int);
int pcmspecial(char*, ISAConf*);
void pcmspecialclose(int);
void pcmunmap(int, PCMmap*);
void ptcheck(char*);
void putcr3(uint32_t);
void putidt(Segdesc*, int);
void* pxegetfspart(int, char*, int);
void qinit(IOQ*);
void readlsconf(void);
void sdaddconf(int);
int sdboot(int, char*, Boot*);
void sdcheck(char*);
void* sdgetfspart(int, char*, int);
int sdinit(void);
void sdinitdev(int, char*);
void sdprintdevs(int);
int sdsetpart(int, char*);
void setvec(int, void (*)(Ureg*, void*), void*);
int splhi(void);
int spllo(void);
void splx(int);
void startwheel(void);
void stopwheel(void);
void trapdisable(void);
void trapenable(void);
void trapinit(void);
void turnwheel(int);
void uartdrain(IOQ*);
int uartspecial(int, void (*)(int), int (*)(void), int);
void uartputs(IOQ*, char*, int);
uint32_t umbmalloc(uint32_t, int, int);
void umbfree(uint32_t, int);
uint32_t umbrwmalloc(uint32_t, int, int);
void upafree(uint32_t, int);
uint32_t upamalloc(uint32_t, int, int);
void warp64(uvlong);
void warp86(char*, uint32_t);
void warp9(uint32_t);
int x86cpuid(int*, int*);
void* xspanalloc(uint32_t, int, uint32_t);

#define malloc(n) ialloc(n, 0)
#define mallocz(n, c) ialloc(n, 0)
#define free(v) while(0)

#define GSHORT(p) (((p)[1] << 8) | (p)[0])
#define GLONG(p) ((GSHORT(p + 2) << 16) | GSHORT(p))
#define GLSHORT(p) (((p)[0] << 8) | (p)[1])
#define GLLONG(p) (((uint32_t)GLSHORT(p) << 16) | GLSHORT(p + 2))
#define PLLONG(p, v)                                                           \
	(p)[3] = (v);                                                          \
	(p)[2] = (v) >> 8;                                                     \
	(p)[1] = (v) >> 16;                                                    \
	(p)[0] = (v) >> 24

#define KADDR(a) ((void*)((uint32_t)(a) | KZERO))
#define PADDR(a) ((uint32_t)(a) & ~0xF0000000)

#define HOWMANY(x, y) (((x) + ((y)-1)) / (y))
#define ROUNDUP(x, y) (HOWMANY((x), (y)) * (y))

#define xalloc(n) ialloc(n, 0)
#define xfree(v) while(0)
#define lock(l)                                                                \
	if(l) {/* nothing to do */                                             \
		;                                                              \
	} else {/* nothing to do */                                            \
		;                                                              \
	}
#define unlock(l)                                                              \
	if(l) {/* nothing to do */                                             \
		;                                                              \
	} else {/* nothing to do */                                            \
		;                                                              \
	}

int dmacount(int);
int dmadone(int);
void dmaend(int);
void dmainit(int);
long dmasetup(int, void*, long, int);

extern int (*_pcmspecial)(char*, ISAConf*);
extern void (*_pcmspecialclose)(int);
extern void devi82365link(void);
extern void devpccardlink(void);

#pragma incomplete Ureg
