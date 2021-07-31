ulong	strtoul(char*, char**, int);
vlong	strtoll(char*, char**, int);

#include "../port/portfns.h"

void	aamloop(int);
void	cgaputc(int);
void	cgaputs(char*, int);
int	cistrcmp(char*, char*);
int	cistrncmp(char*, char*, int);
void	(*coherence)(void);
void	etherinit(void);
void	etherstart(void);
int	floppyinit(void);
void	floppyproc(void);
Off	floppyread(int, void*, long);
Devsize	floppyseek(int, Devsize);
Off	floppywrite(int, void*, long);
void	fpinit(void);
char*	getconf(char*);
ulong	getcr0(void);
ulong	getcr2(void);
ulong	getcr4(void);
int	getfields(char*, char**, int, int, char*);
ulong	getstatus(void);
int	atainit(void);
Off	ataread(int, void*, long);
Devsize	ataseek(int, Devsize);
Off	atawrite(int, void*, long);
void	i8042a20(void);
void	i8042reset(void);
int	inb(int);
void	insb(int, void*, int);
ushort	ins(int);
void	inss(int, void*, int);
ulong	inl(int);
void	insl(int, void*, int);
void	kbdinit(void);
int	kbdintr0(void);
int	kbdgetc(void);
long*	mapaddr(ulong);
void	microdelay(int);
void	mmuinit(void);
uchar	nvramread(int);
void	outb(int, int);
void	outsb(int, void*, int);
void	outs(int, ushort);
void	outss(int, void*, int);
void	outl(int, ulong);
void	outsl(int, void*, int);
void	printcpufreq(void);
void	putgdt(Segdesc*, int);
void	putidt(Segdesc*, int);
void	putcr3(ulong);
void	putcr4(ulong);
void	puttr(ulong);
void	rdmsr(int, vlong*);
void	wrmsr(int, vlong);
void	(*cycles)(uvlong*);
void	scsiinit(void);
Off	scsiread(int, void*, long);
Devsize	scsiseek(int, Devsize);
Off	scsiwrite(int, void*, long);
int	setatapart(int, char*);
int	setscsipart(int, char*);
void	setvec(int, void (*)(Ureg*, void*), void*);
int	tas(Lock*);
void	trapinit(void);
void	uartspecial(int, void (*)(int), int (*)(void), int);
int	uartgetc(void);
void	uartputc(int);
void	wbflush(void);
void	cpuid(char*, int*, int*);

#define PADDR(a)	((ulong)(a)&~KZERO)

/* pata */
void	ideinit(Device *d);
Devsize	idesize(Device *d);
int	ideread(Device *d,  Devsize, void*);
int	idewrite(Device *d, Devsize, void*);

/* sata */
void	mvideinit(Device *d);
Devsize	mvidesize(Device *d);
int	mvideread(Device *d,  Devsize, void*);
int	mvidewrite(Device *d, Devsize, void*);
