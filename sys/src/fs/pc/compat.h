/*
 * fs kernel compatibility hacks for drivers from the cpu/terminal kernel
 */
#define ETHERIQ(a, b, c) 	etheriq((a), (b))
/*
 * cpu kernel uses bp->rp to point to start of packet and bp->wp to point
 * just past valid data in the packet.
 * fs kernel uses bp->data to point to start of packet and bp->data+bp->count
 * points just past valid data.
 * except beware that mballoc(count, ...) sets  bp->count = count(!)
 */
#define BLEN(bp)		(bp)->count
#define SETWPCNT(bp, cnt)	(bp)->count = (cnt)
/* mballoc does:	mb->data = mb->xdata+256; */
#define BLKRESET(bp)		((bp)->data = (bp)->xdata +256, (bp)->count = 0)
#define INCRPTR(bp, incr)	(bp)->count += (incr)
#define ENDDATA(bp)		((bp)->data + (bp)->count)

#define	ROUND(s, sz)	(((s)+((sz)-1))&~((sz)-1))

#define Block	Msgbuf
#define rp	data			/* Block member → Msgbuf member */
#define Etherpkt Enpkt
#define Eaddrlen Easize
#define ETHERHDRSIZE Ensize

#ifndef CACHELINESZ
#define CACHELINESZ	32		/* pentium & later */
#endif

#define KNAMELEN NAMELEN
#define READSTR 128

#define KADDR(a)	((void*)((ulong)(a)|KZERO))
#define PCIWINDOW	0
#define PCIWADDR(va)	(PADDR(va)+PCIWINDOW)

#define iprint print

/* buffers */
#define allocb(sz)  mballoc((sz), 0, Maeth1)
#define iallocb(sz) mballoc((sz), 0, Mbeth1)

/* other memory */
#define malloc(sz)  ialloc((sz), 0)
#define xspanalloc(sz, align, span)	ialloc((sz)+(align)+(span), (align))
/* offset==0 in all uses in fs */
#define mallocalign(sz, align, offset, span) \
			ialloc((sz)+(align)+(span), (align))
/* sleazy hacks; really need better allocators */
#define xalloc(sz) malloc(sz)
#define xfree(p)
#define smalloc(sz) malloc(sz)

#define waserror() 0
#define poperror()
#define nexterror() return
#define error(x) goto err

#define qsetlimit(q, lim)
#define ioalloc(a, b, c, d)	0
#define iofree(p)
#define strtol strtoul
#define PROCARG(arg)
#define GETARG(arg) getarg()

#define vmap(bar, size) upamalloc(bar, size, 0)

/* see portdat.h for Msgbuf flags */
void	freeb(Block *b);
void	freeblist(Block *b);
void	free(void *p);
void	*mallocz(ulong sz, int clr);
char	*strdup(char *);			/* port/config.c */
void	kstrdup(char **p, char *s);

/* header files mysteriously fail to declare this */
ulong	upamalloc(ulong addr, int size, int align);

int	readstr(vlong, void *, int, char *);
void	addethercard(char *, int (*)(struct Ether *));
void	kproc(char *text, void (*f)(void), void *arg);

/* pc-specific? */
int	intrdisable(int irq, void (*f)(Ureg *, void *), void *a, int, char *);
void	intrenable(int irq, void (*f)(Ureg*, void*), void* a, int, char *name);
