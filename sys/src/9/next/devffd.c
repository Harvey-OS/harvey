/*
 *  Intel 82077AA Floppy disc device driver
 */
#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"io.h"

#include	"devtab.h"

#define min(a, b)	(a < b ? a : b)

/* Registers on the 82077 */

typedef struct Controller Controller;
struct Controller
{
	uchar		sra;		/* (R/O) status register A */		
	uchar		srb;		/* (R/O) status register B */
	uchar		dor;		/* (R/W) digital output register */
	uchar		rsvd3;		/* reserved */
	uchar		msr;		/* (R/O) main status register */
#define dsr	msr
	uchar		fifo;		/* (R/W) data FIFO */
	uchar		rsvd6;		/* reserved */
	uchar		dir;		/* (R/O) digital input register */
	uchar		flpctl;		/* (W/O) control (not in 82077) */
};

enum			/* Useful numbers */
{
	Eject_disc	= 0x80,		/* Eject the floppy */
	Sel_82077	= 0x40,		/* Scsi/82077 select */
	Drv_id		= 0x04,		/* Drive present flag */
	Mid1		= 0x02,		/* Media id bits */
	Mid2		= 0x01,
	Maxdrive	= 4,
	Ok		= 0,
	Error		= -1,
	S512		= 512,
};

enum			/* Command set */
{
	Recal	= 0x07,	
	Seek	= 0x0F,	
	Readdat	= 0x06,
	Writedat= 0x05,
	Sis	= 0x80,
};

enum			/* Main status register */
{
	Drv0_bsy= 0x01,
	Drv1_bsy= 0x02,
	Drv2_bsy= 0x04,
	Drv3_bsy= 0x08,
	Cmd_bsy = 0x10,
	Non_bsy = 0x20,
	Dio	= 0x40,
	Rqm	= 0x80,
};

enum			/* Dor register bits */
{
	Moten3	= 0x80,
	Moten2	= 0x40,
	Moten1	= 0x20,
	Moten0	= 0x10,
	Dmagate = 0x08,
	Reset	= 0x04,
	Drvsel1	= 0x02,
	Drvsel0 = 0x01,
};

/*
 *  floppy types
 */
typedef struct Type Type;
struct Type
{
	char	*name;
	int	bytes;		/* bytes/sector */
	int	sectors;	/* sectors/track */
	int	heads;		/* number of heads */
	int	steps;		/* steps per cylinder */
	int	tracks;		/* tracks/disk */
	int	gpl;		/* intersector gap length for read/write */	
	int	fgpl;		/* intersector gap length for format */	
};

Type floppytype[] =
{
	"MF2HD",	S512,	18,	2,	1,	80,	0x1B,	0x54,
	"MF2DD",	S512,	9,	2,	1,	80,	0x1B,	0x54,
	"F2HD",		S512,	15,	2,	1,	80,	0x2A,	0x50,
	"F2DD",		S512,	8,	2,	1,	40,	0x2A,	0x50,
	"F1DD",		S512,	8,	1,	1,	40,	0x2A,	0x50,
};

typedef struct Drive Drive;
struct Drive
{
	Type	*t;
	uchar	dor;		/* Digital output register (Drive select/Motor on) */
	int	synced;		/* Drive is synced with computer */
	int	tcyl;		/* Current cylinder */
	int	tsec;		/* Current sector */
	char	thead;		/* Current head */
	Rendez	r;		/* Place to wait for interrupts */
	QLock	use;		/* Exclusive use lock */
	char	ist;		/* Interrupt status register */
	char	pcn;		/* Physical cylinder from Sis command */
	char	gotint;
	uchar	cmd[32];	/* Command and return status */
	int	cmdlen;
};

Drive fl[Maxdrive];

static int secbytes[] =
{
	128,
	256,
	512,
	1024
};

struct FLOPdma
{
	ulong	csr;
	uchar	pad1[0x4010-0x14];
	ulong	base;
	ulong	limit;
	ulong	chainbase;
	ulong	chainlimit;
};
	

void 	Xdelay(int);
int	issue(Drive*);
int	recalibrate(Drive*);
int	gotfdintr(void*);
void	waitforint(Drive *);

enum
{
	ffddirqid,
	ffddataqid,
	ffdctlqid,
};

Dirtab ffdtab[]={
	"data",		{ffddataqid},		0,	0666,
	"ctl",		{ffdctlqid},		0,	0666,
};

#define Nffdtab (sizeof(ffdtab)/sizeof(Dirtab))

void
ffdreset(void)
{
	Controller *fdc = (Controller*)(FDCTLRL);

	print("ffdreset\n");
	fdc->flpctl = Sel_82077;
	print("fd: (sra=%lux srb=%lux msr=%lux)\n", fdc->sra, fdc->srb, fdc->msr);
	fdc->dsr = 0x80;
	print("fd: (sra=%lux srb=%lux msr=%lux)\n", fdc->sra, fdc->srb, fdc->msr);

	fl[0].dor = Moten0|Dmagate|Reset;
}

void
ffdinit(void)
{
}

Chan *
ffdattach(char *spec)
{
	return devattach('f', spec);
}

Chan *
ffdclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int
ffdwalk(Chan *c, char *name)
{
	return devwalk(c, name, ffdtab, (long)Nffdtab, devgen);
}

void
ffdstat(Chan *c, char *db)
{
	devstat(c, db, ffdtab, (long)Nffdtab, devgen);
}

Chan *
ffdopen(Chan *c, int omode)
{
	if(c->qid.path == CHDIR){
		if(omode != OREAD)
			error(Eperm);
	}

	if(fl[0].synced == 0)
		recalibrate(fl);

	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	return c;
}

void
ffdcreate(Chan *c, char *name, int omode, ulong perm)
{
	error(Eperm);
}

void
ffdremove(Chan *c)
{
	error(Eperm);
}

void
ffdwstat(Chan *c, char *dp)
{
	error(Eperm);
}

void
ffdclose(Chan *c)
{
}

long
ffdread(Chan *c, void *a, long n, ulong offset)
{

	switch((int)(c->qid.path&~CHDIR)){
	case ffddirqid:
		return devdirread(c, a, n, ffdtab, Nffdtab, devgen);
	case ffddataqid:
		break;
	case ffdctlqid:
		return n;
	default:
		n=0;
		break;
	}
	return n;
}

long
ffdwrite(Chan *c, char *a, long n, ulong offset)
{
	Controller *fdc = (Controller*)(FDCTLRL);
	char buf[32];
	int i;

	switch((int)(c->qid.path&~CHDIR)){
	case ffddataqid:
		break;
	case ffdctlqid:
		strncpy(buf, a, min(sizeof(buf), n));
		if(strncmp(buf, "eject", 5) == 0) {
			fdc->dor = 0x1c;
			fdc->flpctl = Sel_82077;
			Xdelay(100);
			fdc->flpctl = Eject_disc|Sel_82077;
			Xdelay(100);
			fdc->flpctl = Sel_82077;
		}
		else
			error(Ebadarg);
	default:
		error(Ebadusefd);
	}
	return n;
}

int
recalibrate(Drive *dp)
{		
	dp->cmd[0] = Recal;
	dp->cmd[1] = dp - fl;
	dp->cmdlen = 2;

	if(issue(dp) != Ok)
		return Error;

	waitforint(dp);
	dp->synced = 1;			/* Drive in sync with controller */
}

void
waitforint(Drive *dp)
{
	Controller *fdc = (Controller*)(FDCTLRL);
	
	tsleep(&dp->r, gotfdintr, dp, 2000);
	if(dp->gotint == 0) {
		print("fd%d: operation timed out (sra=%lux srb=%lux msr=%lux)\n",
						dp-fl, fdc->sra, fdc->srb, fdc->msr);
	}
	dp->gotint = 0;
}

int
issue(Drive *dp)
{
	Controller *fdc = (Controller*)(FDCTLRL);
	uchar *cdb = dp->cmd;
	int i;

	print("fd: (msr=%lux)\n", fdc->msr);

	for(i = 0; i < 1000; i++) {
		if((fdc->msr&(Cmd_bsy|Rqm|Dio)) != Rqm)
			continue;

		fdc->dor = dp->dor;			/* Motor on and select */

		for(i = 0; i < dp->cmdlen; i++) {	/* Load command fifo */
			while((fdc->msr&Rqm) == 0)
				;
			if(i == 1 && (fdc->msr&(Dio|Cmd_bsy) == (Dio|Cmd_bsy)))
				print("fd: illegal cmd %lux (msr=%lux)\n", cdb[-1], fdc->msr);
			fdc->fifo = *cdb++;
		}

		return Ok;
	}
	print("fd: controller cmd timed out (msr=%lux)\n", fdc->msr);
	return Error;
}

void
block2phys(Drive *dp, ulong off)
{
	int lsec;

	lsec = off/secbytes[dp->t->bytes];
	dp->tcyl = lsec/(dp->t->sectors*dp->t->heads);
	dp->tsec = (lsec % dp->t->sectors) + 1;
	dp->thead = (lsec/dp->t->sectors) % dp->t->heads;
}

int
gotfdintr(void *dp)
{
	return ((Drive*)dp)->gotint;
}

void
floppyintr(void)
{
	Controller *fdc = (Controller*)(FDCTLRL);
	Drive *dp = &fl[fdc->dor&3];

	print("floppy intr %d\n", dp-fl);

	dp->cmd[0] = Sis;
	dp->cmdlen = 1;
	if(issue(dp) != Ok)
		print("fd%d: Sis issue failed\n", dp-fl);

	dp->ist = fdc->fifo;
	dp->pcn = fdc->fifo;

	print("ist %lux %lux\n", dp->ist, dp->pcn);
	if(u) {
		fl[0].gotint = 1;
		wakeup(&fl[0].r);
	}
}
