/*
 * first pass - dumb, single-threaded driver,
 * no sync, no disconnect.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"

int	scsidebugs[8];
int	scsiownid = 7;

static QLock scsilock;		/* access to device */
static Rendez scsirendez;	/* sleep/wakeup for requesting process */
static Scsi *curcmd;		/* currently executing command - barf */

static uchar *datap;		/* pdma - barf */
static int scsirflag;		/* pdma - barf */

static void
scsidmastart(int chno, void *pa, ulong nbytes, int read)
{
	USED(chno, pa, nbytes, read);
	datap = KADDR(pa);
	scsirflag = read;
}

static void
scsidmastop(int chno)
{
	USED(chno);
}

static void
scsidmaintr(int ctlrno, Ureg *ur)
{
	ulong *status;
	uchar *p;

	USED(ctlrno, ur);
	status = &IRQADDR->status1;
	p = datap;
	if(scsirflag){
		while(*status & 0x01)
			*p++ = *SCSIPDMA;
	}
	else {
		while(*status & 0x01)
			*SCSIPDMA = *p++;
	}
	datap = p;
}

/*
 * allocate a scsi buf of any length
 * must be called at ialloc time and never freed
 */
Scsibuf *
scsialloc(ulong n)
{
	Scsibuf *b;

	b = xalloc(sizeof(*b));
	b->virt = xalloc(n);
	b->phys = (void *)(PADDR(b->virt));
	return b;
}

void
initscsi(void)
{
}

static int
scsidone(void *arg)
{
	USED(arg);
	return curcmd == 0;
}

int
scsiexec(Scsi *p, int read)
{
	SCSIdev *addr = SCSIADDR;
	long nbytes;

	qlock(&scsilock);
	if (waserror()) {
		qunlock(&scsilock);
		nexterror();
	}
	p->rflag = read;				/* BUG: fix in caller */

	addr->addr = 0x0F;				/* 0x0F target LUN */
	addr->data = p->lun;

	addr->addr = 0x10;				/* 0x10 command phase */
	addr->data = 0x00;

	addr->addr = 0x11;				/* 0x11 synchronous transfer */
	addr->data = 0;					/* BUG: synch later */

	nbytes = p->data.lim - p->data.base;
	scsidmastart(0, p->b->phys, nbytes, p->rflag);
	addr->data = (nbytes>>16) & 0xFF;		/* 0x12 transfer count */
	addr->data = (nbytes>>8) & 0xFF;		/* 0x13 */
	addr->data = nbytes & 0xFF;			/* 0x14 */
	if (p->rflag)
		addr->data = 0x40|p->target;		/* 0x15 destination ID */
	else
		addr->data = p->target;

	nbytes = p->cmd.lim - p->cmd.ptr;
	addr->addr = 0x00;				/* 0x00 own ID/CDB size */
	addr->data = nbytes;
	addr->addr = 0x03;				/* 0x03 CDB first byte */
	while (nbytes--)
		addr->data = *(p->cmd.ptr)++;

	addr->addr = 0x18;				/* 0x18 command */
	addr->data = 0x09;				/* Select-w/o-ATN-And-Transfer */

	curcmd = p;					/* crap */
	sleep(&scsirendez, scsidone, 0);

	scsidmastop(0);

	p->data.ptr = datap;
	addr->addr = 0x0F;				/* 0x0F target LUN */
	p->status = addr->data & 0xFF;
	p->status |= addr->data<<8;
	poperror();
	qunlock(&scsilock);
	return p->status;
}

void
scsiintr(int ctlrno, Ureg *ur)
{
	SCSIdev *addr = SCSIADDR;
	uchar status, x;

	USED(ctlrno, ur);
	addr->addr = 0x17;				/* status */
	status = addr->data;
	switch (status) {

	case 0x00:					/* initial reset */
		return;

	case 0x01:					/* software reset */
		addr->addr = 0x01;			/* control */
		addr->data = 0x2D;			/* burst DMA, EDI, IDI, HSP */
		addr->addr = 0x02;			/* timeout period */
		addr->data = 0xFF;
		addr->addr = 0x16;			/* source ID */
		addr->data = 0x00;			/* BUG: no reselect */
		/*FALLTHROUGH*/
	case 0x16:					/* select-and-transfer complete */
	case 0x42:					/* select timeout */
		curcmd = 0;				/* crap */
		wakeup(&scsirendez);
		return;

	case 0x4b:					/* unexpected status */
		addr->addr = 0x10;			/* 0x10 command phase */
		addr->data = 0x46;			/*  */
		addr->addr = 0x12;			/* 0x12 transfer count */
		addr->data = 0;				/* 0x12 */
		addr->data = 0;				/* 0x13 */
		addr->data = 0;				/* 0x14 */
		addr->addr = 0x18;			/* 0x18 command */
		addr->data = 0x09;			/* Select-w/o-ATN-And-Transfer */
		return;

	default:
		break;
	}
	print("scsi intr status #%.2ux\n", status);
	print("scsi aux status #%.2ux\n", addr->addr & 0xFF);
	for (addr->addr = 0x00, x = 0x00; x < 0x19; x++) {
		if ((x & 0x07) == 0x07)
			print("#%.2ux: #%.2ux\n", x, addr->data & 0xFF);
		else
			print("#%.2ux: #%.2ux ", x, addr->data & 0xFF);
	}
	panic("scsiintr\n");
}

static Vector vector[2] = {
	{ 0x0400, 0, scsiintr, "scsi0", },
	{ 0x0100, 0, scsidmaintr, "scsidma", },
};

void
resetscsi(void)
{
	SCSIdev *addr = SCSIADDR;

	setvector(&vector[0]);
	setvector(&vector[1]);

	addr->addr = 0x00;			/* own ID/CDB size */
	addr->data = 0x88|(scsiownid & 0x07);	/* 16MHz, advanced features */
	addr->addr = 0x18;			/* command */
	addr->data = 0;				/* reset */
}
