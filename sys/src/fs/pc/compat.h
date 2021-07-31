/*
 * fs kernel compatibility hacks for drivers from the cpu/terminal kernel
 */
#define ETHERIQ(a, b, c) 	etheriq((a), (b))
#define SETWPCNT(bp, cnt)	(bp)->count = (cnt)
/*
 * mballoc does:	mb->data = mb->xdata+256;
 */
#define BLKRESET(bp)		((bp)->data = (bp)->xdata +256, (bp)->count = 0)
#define INCRPTR(bp, incr)	(bp)->count += (incr)

#define Block	Msgbuf
#define rp	data			/* Block member → Msgbuf member */
#define Etherpkt Enpkt
#define Eaddrlen Easize

#define KADDR(a)	((void*)((ulong)(a)|KZERO))
#define PCIWINDOW	0
#define PCIWADDR(va)	(PADDR(va)+PCIWINDOW)

#define BLEN(bp) ((bp)->count)
#define iprint print
#define allocb(sz)  mballoc((sz), 0, Maeth1)
#define iallocb(sz) mballoc((sz), 0, Mbeth1)
#define malloc(sz)  ialloc((sz), 0)
#define xspanalloc(sz, align, span) ialloc((sz)+(align)+(span), (align))

#define waserror() 0
#define poperror()
#define nexterror() return
#define error(x) goto err

#define qsetlimit(q, lim)
#define ioalloc(a, b, c, d)	0
#define strtol strtoul
#define kproc(name, f, arg)	userinit(f, arg, name)

/* see portdat.h for Msgbuf flags */
void	freeb(Block *b);
void	freeblist(Block *b);
void	free(void *p);
void	*mallocz(ulong sz, int clr);

/* header files mysteriously fail to declare this */
ulong	upamalloc(ulong addr, int size, int align);
