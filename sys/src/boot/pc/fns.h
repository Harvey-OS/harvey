#include "portfns.h"

#define	clearmmucache()		/* 386 doesn't have one */
void	clock(Ureg*);
void	clockinit(void);
void	delay(int);
#define	evenaddr(x)		/* 386 doesn't care */
void	i8042a20(void);
void	fault386(Ureg*);
void	faultinit(void);
int	floppyinit(void);
long	floppyseek(int, long);
long	floppyread(int, void*, long);
#define	flushvirt();
void	fpsave(FPsave*);
void	fprestore(FPsave*);
ulong	getcr2(void);
int	hardinit(void);
long	hardseek(int, long);
long	hardread(int, void*, long);
long	hardwrite(int, void*, long);
void	heada20(void);
void	idle(void);
int	inb(int);
void	inss(int, void*, int);
void	kbdinit(void);
void	kbdintr(Ureg*);
void	meminit(void);
void	mmuinit(void);
char	*nextelem(char*, char*);
uchar	nvramread(int);
void	outb(int, int);
void	outss(int, void*, int);
int	p9boot(int, long (*)(int, long), long (*)(int, void*, int));
void	prhex(ulong);
void	procrestore(Proc*, uchar*);
void	procsave(uchar*, int);
void	procsetup(Proc*);
void	putgdt(Segdesc*, int);
void	putidt(Segdesc*, int);
void	putcr3(ulong);
void	puttr(ulong);
void	screeninit(void);
void	screenputc(int);
void	screenputs(char*, int);
void	setvec(int, void (*)(Ureg*));
int	sethardpart(int, char*);
void	systrap(void);
void	touser(void);
void	trapinit(void);
int	tas(Lock*);
void	vgainit(void);
#define	waserror()	(u->nerrlab++, setlabel(&u->errlab[u->nerrlab-1]))

#define	GSHORT(p) (((p)[1]<<8)|(p)[0])
#define	GLONG(p) ((GSHORT(p+2)<<16)|GSHORT(p))
#define	GLSHORT(p) (((p)[0]<<8)|(p)[1])
#define	GLLONG(p) ((GLSHORT(p)<<16)|GLSHORT(p+2))
