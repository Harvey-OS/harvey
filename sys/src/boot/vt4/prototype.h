#define PTR2UINT(p)	((uintptr)(p))
#define UINT2PTR(i)	((void*)(i))

/* physical == virtual in our world! */
#define PADDR(a)	((ulong)(a) & ~0xF0000000)

#define	GSHORT(p)	(((p)[1]<<8)|(p)[0])
#define	GLONG(p)	((GSHORT(p+2)<<16)|GSHORT(p))
#define	GLSHORT(p)	(((p)[0]<<8)|(p)[1])
#define	GLLONG(p)	(((ulong)GLSHORT(p)<<16)|GLSHORT(p+2))
#define	PLLONG(p,v)	(p)[3]=(v);(p)[2]=(v)>>8;(p)[1]=(v)>>16;(p)[0]=(v)>>24

#define mallocalign(n, a, o, s)	ialloc((n), (a))

/* emergency debugging printing, from uart.c */
#define Uarttxfifo	((ulong *)(Uartlite + 4))
#define PROG(c)		{ coherence(); *Uarttxfifo = (uchar)(c); coherence(); }

/*
 * l32p.s
 */
void	cachesinvalidate(void);
void	dcbi(void*);
void	dcflush(uintptr, usize);
void	eieio(void);
u32int	esrget(void);
void	esrput(u32int);
// char*	getconf(char*);
#define getconf(s) nil
u32int	getccr0(void);
u32int	getdar(void);
u32int	getdear(void);
u32int	getdec(void);
u32int	getesr(void);
u32int	getmsr(void);
u32int	getpid(void);
u32int	getpir(void);
u32int	getpit(void);
u32int	getpvr(void);
u32int	gettbl(void);
u32int	gettsr(void);
void	icflush(uintptr, usize);
int	islo(void);
void	microdelay(int);
u32int	mmucrget(void);
void	mmucrput(u32int);
void	putdec(ulong);
void	putesr(ulong);
void	putevpr(ulong);
void	putmsr(u32int);
void	putpid(u32int);
void	putpit(u32int);
void	putsdr1(u32int);
void	puttcr(u32int);
void	puttsr(u32int);
void	setsp(uintptr);
void	spldone(void);
int	splhi(void);
int	spllo(void);
void	splx(int);
void	splxpc(int);
void	sync(void);
void	syncall(void);
int	tas32(uint*);
void	trapvec(void);
int	_xdec(long *);
int	_xinc(long *);

/*
 * trap.c
 */
void	trapdisable(void);
void	trapinit(void);

/*
 * intr.c
 */
void	intr(Ureg *ur);
void	intrack(ulong);
void	intrenable(ulong bit, int (*func)(ulong));
void	intrinit(void);
ulong	lddbg(void);

/*
 * uart.c
 */
int	vuartgetc(void);
void	vuartputc(int c);
int	vuartputs(char *s, int len);

/*
 * clock.c
 */
void	clockinit(void);
void	clockintr(Ureg *ureg);
void	delay(int);
void	prcpuid(void);
void	timerintr(Ureg*);

/*
 * dma.c
 */
void	dmacpy(void *dest, void *src, ulong len, ulong flags);
void	dmainit(void);
void	dmastart(void *dest, void *src, ulong len, ulong flags);
void	dmawait(void);

/*
 * ether.c
 */
uchar	*etheraddr(int ctlrno);
int	etherinit(void);
void	etherinitdev(int, char*);
void	etherprintdevs(int);
int	etherrxflush(int ctlrno);
int	etherrxpkt(int ctlrno, Etherpkt* pkt, int timo);
int	ethertxpkt(int ctlrno, Etherpkt* pkt, int len, int);

/*
 * llfifo.c
 */
void	llfifoinit(Ether *);
int	llfifointr(ulong bit);
void	llfiforeset(void);
void	llfifotransmit(uchar *ubuf, unsigned len);

/*
 * load.c
 */
int	getfields(char*, char**, int, char);
void*	ialloc(ulong, int);

/*
 * boot.c
 */
int	bootpass(Boot *b, void *vbuf, int nbuf);

/*
 * bootp.c
 */
int	bootpboot(int, char*, Boot*);
void*	pxegetfspart(int, char*, int);

/*
 * conf.c
 */
int	dotini(Fs *fs);

/*
 * console.c
 */
void	consinit(char*, char*);
int	getstr(char*, char*, int, char*, int);
void	kbdinit(void);
void	warp86(char*, ulong);
void	warp9(ulong);

/*
 * inflate.c
 */
int	gunzip(uchar*, int, uchar*, int);

/*
 * misc.
 */

void	ilock(Lock*);
void	iunlock(Lock*);
int	lock(Lock*);
void	unlock(Lock*);

void	panic(char *fmt, ...);

#define TAS(l)		tas32(l)

#define iprint		print

#define coherence()	eieio()
#define exit(n)		for(;;) ;

void	cacheline0(uintptr addr);
ulong	cacheson(void);
void	clrmchk(void);
void	dump(void *, int);
void	dumpregs(Ureg *);
void	flushwrbufs(void);
uintptr	memsize(void);
vlong	probeaddr(uintptr addr);
/*
 * qtm.c
 */
int	qtmerrfmt(char *, int);
void	qtmerrtest(char *);
void	qtmerrtestaddr(ulong);
int	qtmmemreset(void);
vlong	qtmprobeaddr(uintptr addr);
