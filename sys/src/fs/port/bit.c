#include	"all.h"
#include	"mem.h"
#include	"io.h"

enum
{
	MaxBit		= 2,
	NOUT		= 100,
	INTDELAY	= 1000,
	DMADELAY	= 1000,
	Ureset		= 0,
	Uread,
	Uwrite,
};

static
struct
{
	Queue*	reply;
	struct	Bp
	{
		Rendez	r;
		QLock	l;
		Chan*	chan;
		int	cmdwait;
		int	dma;
		Vmedevice* vme;

		struct	Rout
		{
			Msgbuf*	msg;
			ulong	u;
			int	t1;
		} rout[NOUT];

		Filter	rate;
		Filter	count;
	} b[MaxBit];
} b;

struct	Bit
{
	struct	localnode
	{
		uchar	junk1;
		uchar	lncommand;
		uchar	junk2;
		uchar	lnstatus;
		uchar	lnaddrmod;
		uchar	junk3;
		uchar	junk4;
		uchar	lnvec;
	};
	union	remotenode
	{
		struct
		{
			uchar	rncommand2;
			uchar	rncommand1;	/* also status */
			ushort	rnpage;
			uchar	rnaddrmod;
			uchar	junk5;
			uchar	rniackhi;
			uchar	rniacklo;
		};
		struct
		{
			uchar	junka1;
			uchar	rnstatus;
			uchar	junka2;
			uchar	junka3;
			uchar	junka4;
			uchar	junka5;
			uchar	junka6;
			uchar	junka7;
		};
	};
	struct	localdma
	{
		uchar	ldcommand;
		uchar	junk6;
		ushort	ldaddrhi;
		ushort	ldaddrlo;
		ushort	ldcount;
	};
	struct	remotedma
	{
		uchar	junk7;
		uchar	junk8;
		ushort	rdaddrhi;
		ushort	rdaddrlo;
		uchar	junk9;
		uchar	junka;
	};
};

int
bitinit(Vmedevice *vp)
{
	Bp *bp;
	Rout *ro;
	Bit *bit;
	int c;

	print("bitinit %s\n", vp->name);
	if(vp->ctlr >= MaxBit)
		return -1;
	bp = &b.b[vp->ctlr];
	if(bp->vme)
		return -1;
	bit = vp->address;
	/*
	 * does the board exist?
	if(probe(&bit->lnstatus, sizeof(bit->lnstatus)))
		return -1;
	 */

	bp->dma = 0;

	c = bit->lnstatus;
	if(c & 1) {
		print("	%s: remote power off\n", vp->name);
		return -1;
	}
	c = bit->rnstatus;
	USED(c);
	bit->lncommand = 0xc0;		/* clear errors, clear PR ff */
	bit->rncommand1 = 0x48;		/* clear PT ff, page mode */
	c = bit->lnstatus;
	if(c & 0xe3) {
		print("	%s: bad local status = %.2x\n", vp->name, c);
		return -1;
	}
	bit->lnvec = vp->vector;

	bit->rnaddrmod = 0x09;		/* A32 non-priv */
	bit->rncommand2 = 0x40;		/* use addr mod reg */
	bit->lnaddrmod = 0x09;		/* A32 non-priv */
	bit->ldcommand = 0x0;		/* dma done, no interrupt */

	ro = bp->rout;
	for(c=0; c<NOUT; c++, ro++) {
		ro->u = 0;
		ro->msg = 0;
		ro->t1 = 0;
	}

	bp->l.name = "bit3";
	qlock(&bp->l);
	qunlock(&bp->l);
	lock(&bp->r);
	unlock(&bp->r);

	dofilter(&bp->count);
	dofilter(&bp->rate);

	bp->vme = vp;
	vp->private = bp;
	return 0;
}

void
bitintr(Vmedevice *vp)
{
	Bp *bp;
	Bit *bit;
	int rst;

	bp = vp->private;
	bit = vp->address;

	if(bp->dma) {
		print("%s: dma set\n", vp->name);
		return;
	}
	rst = bit->rnstatus;
	if(rst == 0xff) {
		print("%s: status r=%.2x\n", vp->name, rst);
		return;
	}
	if(!(rst & 0x02)) {
		print("%s: PT not set r=%.2x\n", vp->name, rst);
		return;
	}

	bit->rncommand1 = 0x48;		/* clear PT ff, page mode */

	if(bp->cmdwait)
		print("%s: second interrupt before reply\n", vp->name);
	bp->cmdwait = 1;
	wakeup(&bp->r);
}

void
bitstore(Bp *bp, ulong va, ulong l)
{
	Bit *bit;
	Vmedevice *vp;
	ulong a;

	vp = bp->vme;
	bit = vp->address;
	a = (va & 0x0ffffffc) | (SLAVE << 28);
	bit->rnpage = a >> 16;
	*(ulong*)(vp->address1 | (a & 0xfffc)) = l;
}

ulong
bitfetch(Bp *bp, ulong va)
{
	Bit *bit;
	Vmedevice *vp;
	ulong a;

	vp = bp->vme;
	bit = vp->address;
	a = (va & 0x0ffffffc) | (SLAVE << 28);
	bit->rnpage = a >> 16;
	return *(ulong*)(vp->address1 | (a & 0xfffc));
}

void
bitcopy(Bp *bp, void *la, ulong ra, int n, int dir)
{
	Bit *bit;
	Vmedevice *vp;
	ulong *l1, *l2, a;
	int i;

	vp = bp->vme;
	bit = vp->address;

	bp->rate.count += n/4;

	a = (ra & 0x0ffffffc) | (SLAVE << 28);
	if(n <= 80 && (((ra+n-1) ^ ra) & 0xffff0000) == 0) {
		bit->rnpage = a >> 16;
		l1 = (ulong*)(vp->address1 | (a & 0xfffc));
		l2 = (ulong*)la;
		if(dir == Uread) {
			while(n > 0) {
				*l2++ = *l1++;
				n -= 4;
			}
		} else {
			while(n > 0) {
				*l1++ = *l2++;
				n -= 4;
			}
		}
		return;
	}

	bit->rdaddrhi = a >> 16;
	bit->rdaddrlo = a >> 0;

	a = ((ulong)la & 0x0ffffffc) | (SLAVE << 28);
	bit->ldaddrhi = a >> 16;
	bit->ldaddrlo = a >> 0;

	bit->ldcount = (n+255) >> 8;

	bit->rnaddrmod = 0x09;	/* A32 non-priv */
	bit->lnaddrmod = 0x09;	/* A32 non-priv */
	bit->lncommand = 0xc4;	/* disable local interrupts */
	bit->rncommand2 = 0x50;	/* no pause, use modreg, disable intr, block mode */

	for(i=0; i<DMADELAY; i++)
		;
	bp->dma = 1;
	bit->rncommand1 = 0x00;	/* no page mode */
	if(dir == Uread)	/* start, to/from, longword, nopause, block mode */
		bit->ldcommand = 0x90;
	else
		bit->ldcommand = 0xb0;	/* why b0 and 91?? */
	wbflush();

	while(!(bit->ldcommand & 0x2))
		;

	bit->ldcommand = 0;	/* clear done */
	bp->dma = 0;
	bit->lncommand = 0xc0;	/* reenable interrupts? */
	bit->rncommand2 = 0x40;	/* reenable interrupts? */
	bit->rncommand1 = 0x08;	/* page mode */
	bit->rnaddrmod = 0x09;	/* A32 */
	bit->lnaddrmod = 0x09;	/* A32 */
}

void
bitsync(Bp *bp, ulong u, int type, Msgbuf *mb)
{
	Rout *ro;
	ulong u2, u3;
	int i, j;

	ro = bp->rout;
	for(i=0; i<NOUT; i++, ro++)
		if(ro->u == u)
			goto found;
	if(type != 0) {
		print("new bitsync and not allocated\n");
		return;
	}
loop:
	ro = bp->rout;
	for(i=0; i<NOUT; i++, ro++)
		if(ro->u == 0)
			goto found;
	print("nout full, flushing all outstanding\n");

	ro = bp->rout;
	for(i=0; i<NOUT; i++, ro++)
		ro->u = 0;
	goto loop;

found:
	switch(type) {
	case 0:			/* write */
		if(cons.flags&Fbit)
			print("s0 i=%d\n", i);
		ro->u = u;
		ro->t1 = 0;
		if(ro->msg)
			mbfree(ro->msg);
		ro->msg = 0;
		return;
	case 1:			/* read */
		if(cons.flags&Fbit)
			print("s1 i=%d m=%lux\n", i, ro->msg);
		ro->t1 = 1;
		break;
	case 2:			/* reply */
		if(cons.flags&Fbit)
			print("s2 i=%d t=%d\n", i, ro->t1);
		ro->msg = mb;
		break;
	}
	if(!ro->t1 || !ro->msg)
		return;
	bitstore(bp, 0x78, 1);			/* disable interrupts */
	for(i=0; i<INTDELAY; i++)
		;

	u2 = bitfetch(bp, u+1*sizeof(ulong));	/* buf */
	u3 = bitfetch(bp, u+2*sizeof(ulong));	/* count */

	mb = ro->msg;
	if(mb->count < u3)
		u3 = mb->count;

	if(cons.flags&Fbit)
		print("se i=%d r=%d b=%lux c=%d\n", i,
			mb->data[0], u2, u3);
	if(cons.flags&Fbitx)
	if(mb->data[0] == 37 && mb->data[6] > 2)
		for(j=9; j<u3; j++)
			if(mb->data[8] != mb->data[j]) {
				print("r %d %x %x\n", j-1, mb->data[8],
					mb->data[j]);
				break;
			}

	bitcopy(bp, mb->data, u2, u3, Uwrite);
	mbfree(mb);
	ro->u = 0;
	ro->t1 = 0;
	ro->msg = 0;
	bitstore(bp, u+3*sizeof(ulong), u3);	/* set reply count */

	bitstore(bp, 0x78, 0);			/* enable interrupts */
}

void
bito(void)
{
	Msgbuf *mb;
	Chan *cp;
	Bp *bp;
	ulong u;

	print("b3o\n");

loop:
	mb = recv(b.reply, 0);
	if(!mb) {
		print("zero message\n");
		goto loop;
	}

	cp = mb->chan;
	bp = cp->bitp.bitp;

	qlock(&bp->l);
	bitsync(bp, mb->param, 2, mb);
	qunlock(&bp->l);

	goto loop;
}

int
bitcmd(void *v)
{
	return ((Bp*)v)->cmdwait;
}

void
biti(void)
{
	Chan *cp;
	Bp *bp;
	Vmedevice *vp;
	Msgbuf *mb;
	ulong u, u1, u2, u3;
	int i, j;

	bp = getarg();
	vp = bp->vme;

	print("b3i: %s\n", vp->name);
	cp = chaninit(Devbit, 1);
	bp->chan = cp;
	cp->send = serveq;
	cp->reply = b.reply;
	cp->bitp.bitp = bp;

loop:
	while(!bitcmd(bp))
		sleep(&bp->r, bitcmd, bp);
	qlock(&bp->l);

	bp->count.count++;
	u = bitfetch(bp, 0x7c);
	u1 = bitfetch(bp, u+0*sizeof(ulong));	/* opcode */
	switch(u1) {
	default:
		print("%s: unknown bit command\n", vp->name);
		break;

	case Ureset:
		print("%s: reset\n", vp->name);
		fileinit(cp);
		cp->whotime = 0;
		strcpy(cp->whoname, "<none>");
		for(i=0; i<NOUT; i++)
			bp->rout[i].u = 0;
		break;

	case Uread:
		bitsync(bp, u, 1, 0);
		break;

	case Uwrite:
		u2 = bitfetch(bp, u+1*sizeof(ulong));	/* bufferp */
		u3 = bitfetch(bp, u+2*sizeof(ulong));	/* count */
		mb = mballoc(u3+256, cp, Mbbit);
		bitcopy(bp, mb->data, u2, u3, Uread);
		mb->count = u3;
		mb->param = u;
		if(cons.flags&Fbitx)
		if(mb->data[0] == 38 && mb->data[12] > 2)
			for(j=15; j<u3; j++)
				if(mb->data[14] != mb->data[j]) {
					print("w %d %x %x\n", j-1,
						mb->data[14],
						mb->data[j]);
					break;
				}
		send(cp->send, mb);
		bitsync(bp, u, 0, 0);
		break;
	}
	bp->cmdwait = 0;
	bitstore(bp, 0x7c, 0);
	qunlock(&bp->l);
	goto loop;
}

static
void
cmd_stats(int argc, char *argv[])
{
	Bp *bp;
	Vmedevice *vp;

	USED(argc);
	USED(argv);

	print("bit stats\n");
	for(bp = &b.b[0]; bp < &b.b[MaxBit]; bp++) {
		vp = bp->vme;
		if(!vp)
			continue;
		print("	%s\n", vp->name);
		print("		work = %F mps\n", (Filta){&bp->count, 1});
		print("		rate = %F tBps\n", (Filta){&bp->rate, 250});
		bp++;
	}
}

void
bitstart(void)
{
	Bp *bp;
	int any;

	any = 0;
	for(bp = &b.b[0]; bp < &b.b[MaxBit]; bp++) {
		if(bp->vme == 0)
			continue;
		if(any == 0) {
			any = 1;
			b.reply = newqueue(NQUEUE);
			userinit(bito, 0, "b3o");
			cmd_install("statb", "-- bit stats", cmd_stats);
		}
		userinit(biti, bp, "b3i");
	}
}
