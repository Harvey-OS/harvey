#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"devtab.h"
#include	"io.h"

int	scsiintr(void);

#define	DPRINT	if(debug)kprint

int	scsidebugs[8];
int	scsiownid = 0x08|7; /* enable advanced features */

Scsibuf *
scsialloc(ulong n)
{
	Scsibuf *b;

	b = xalloc(sizeof(Scsibuf));
	b->virt = b->phys = xalloc(n);
	return b;
}

typedef struct Scsictl {
	uchar	asr;
	uchar	data;
	uchar	stat;
	uchar	dma;
} Scsictl;

#define	Scsiaddr	48
#define	DEV	((Scsictl *)&PORT[Scsiaddr])

static long	poot;
#define	WAIT	(poot=0, poot==0?0:poot)

#define	PUT(a,d)	(DEV->asr=(a), WAIT, DEV->data=(d))
#define	GET(a)		(DEV->asr=(a), WAIT, DEV->data)

enum Int_status {
	Inten = 0x01, Scsirst = 0x02,
	INTRQ = 0x01, DMA = 0x02,
};

enum SBIC_regs {
	Own_id=0x00, Control=0x01, CDB=0x03, Target_LUN=0x0f,
	Cmd_phase=0x10, Tc_hi=0x12,
	Dest_id=0x15, Src_id=0x16, SCSI_Status=0x17,
	Cmd=0x18, Data=0x19,
};

enum Commands {
	Reset = 0x00,
	Assert_ATN = 0x02,
	Negate_ACK = 0x03,
	Select_with_ATN = 0x06,
	Select_with_ATN_and_Xfr = 0x08,
	Select_and_Xfr = 0x09,
	Transfer_Info = 0x20,
	SBT = 0x80,		/* modifier for single-byte transfer */
};

enum Aux_status {
	INT=0x80, LCI=0x40, BSY=0x20, CIP=0x10,
	PE=0x02, DBR=0x01,
};

static int	isscsi;
static QLock	scsilock;
static Rendez	scsirendez;
static uchar *	datap;
static uchar *	datalim;
static long	debug, scsirflag, scsibusy, scsiinservice;

static void
nop(void)
{}

void
resetscsi(void)
{
	static int inited;

	if(!inited)
		addportintr(scsiintr);
	inited = 1;
}

void
initscsi(void)
{
	isscsi = portprobe("scsi", -1, Scsiaddr, -1, 0L);
	if (isscsi >= 0) {
		DEV->stat = Scsirst;
		delay(100);
		DEV->stat = Inten;
		while (DEV->stat & (INTRQ|DMA))
			nop();
		scsiownid &= 0x0f; /* possibly advanced features */
		scsiownid |= 0x80; /* 16MHz */
		PUT(Own_id, scsiownid);
		PUT(Cmd, Reset);
	}
}

static int
scsidone(void *arg)
{
	USED(arg);
	return (scsibusy == 0);
}

int
scsiexec(Scsi *p, int rflag)
{
	long n;

	debug = scsidebugs[p->target&7];
	DPRINT("scsi %d.%d %2.2ux ", p->target, p->lun, *(p->cmd.ptr));
	qlock(&scsilock);
	if(waserror()){
		qunlock(&scsilock);
		DPRINT(" error return\n");
		nexterror();
	}
	scsirflag = rflag;
	p->rflag = rflag;
	datap = p->data.base;
	datalim = p->data.lim;
	if ((scsiownid & 0x08) && rflag)
		PUT(Dest_id, 0x40|p->target);
	else
		PUT(Dest_id, p->target);
	PUT(Target_LUN, p->lun);
	n = p->data.lim - p->data.base;
	PUT(Tc_hi, n>>16);
	DEV->data = n>>8;
	DEV->data = n;
	if (scsiownid & 0x08) {
		n = p->cmd.lim - p->cmd.ptr;
		DPRINT("len=%d ", n);
		PUT(Own_id, n);
	}
	PUT(CDB, *(p->cmd.ptr)++);
	while (p->cmd.ptr < p->cmd.lim)
		DEV->data = *(p->cmd.ptr)++;
	scsibusy = 1;
	PUT(Cmd, Select_and_Xfr);
	/*PUT(Cmd, Select_with_ATN_and_Xfr);*/
	DPRINT("S<");
	sleep(&scsirendez, scsidone, 0);
	DPRINT(">\n");
	p->data.ptr = datap;
	p->status = GET(Target_LUN);
	p->status |= DEV->data<<8;
	poperror();
	qunlock(&scsilock);
	debug = 0;
	return p->status;
}

void
scsirun(void)
{
	wakeup(&scsirendez);
	scsibusy = 0;
}

void
scsireset0(void)
{
/*	PUT(Control, 0x29);	/* burst DMA, halt on parity error */
	PUT(Control, 0x28);	/* burst DMA, allow parity errors */
	PUT(Control+1, 0xff);	/* timeout */
	PUT(Src_id, 0x80);	/* enable reselection */
	scsirun();
	/*qunlock(&scsilock);*/
}

int
scsiintr(void)
{
	int status, s;

	if(isscsi < 0 || scsiinservice
		|| !((status = DEV->stat) & (DMA|INTRQ)))
			return 0;
	DEV->stat = 0;
	scsiinservice = 1;
	s = spl1();
	DPRINT("i%x ", status);
	do{
		if (status & DMA)
			scsidmaintr();
		if (status & INTRQ)
			scsictrlintr();
	}while ((status = DEV->stat) & (DMA|INTRQ));
	splx(s);
	scsiinservice = 0;
	DEV->stat = Inten;
	return 1;
}

void
scsidmaintr(void)
{
	uchar *p = 0;
/*
 *	if (scsirflag) {
 *		unsigned char *p;
 *		DPRINT("R", p=datap);
 *		do
 *			*datap++ = DEV->dma;
 *		while (DEV->stat & DMA);
 *		DPRINT("%d ", datap-p);
 *	} else {
 *		unsigned char *p;
 *		DPRINT("W", p=datap);
 *		do
 *			DEV->dma = *datap++;
 *		while (DEV->stat & DMA);
 *		DPRINT("%d ", datap-p);
 *	}
 */
	if(scsirflag){
		DPRINT("R", p=datap);
		datap = scsirecv(datap);
		DPRINT("%d ", datap-p);
	}else{
		DPRINT("X", p=datap);
		datap = scsixmit(datap);
		DPRINT("%d ", datap-p);
	}
}

void
scsictrlintr(void)
{
	int status, phase;
	long n;

	status = GET(SCSI_Status);
	DPRINT("I%2.2x ", status);
	switch(status){
	case 0x00:			/* reset by command or power-up */
	case 0x01:			/* reset by command or power-up */
		scsireset0();
		break;
	case 0x21:			/* Save Data Pointers message received */
		n = GET(Tc_hi);
		n = (n<<8)|DEV->data;
		n = (n<<8)|DEV->data;
		datap = datalim - n;
		PUT(Cmd_phase, 0x41);
		PUT(Cmd, Select_and_Xfr);
		break;
	case 0x16:			/* select-and-transfer completed */
	case 0x42:			/* timeout during select */
		scsirun();
		break;
	case 0x4b:			/* unexpected status phase */
		DEV->asr = Target_LUN;
		kprint("lun/status 0x%ux\n", DEV->data);
		phase = DEV->data;
		kprint("phase 0x%ux\n", phase);
		PUT(Tc_hi, 0);
		DEV->data = 0;
		DEV->data = 0;
		switch(phase){
		case 0x50:
		case 0x60:
			break;
		default:
			phase = 0x46;
			break;
		}
		PUT(Cmd_phase, phase);
		PUT(Cmd, Select_and_Xfr);
		break;
	default:
		kprint("scsintr 0x%ux\n", status);
		DEV->asr = Target_LUN;
		kprint("lun/status 0x%ux\n", DEV->data);
		kprint("phase 0x%ux\n", DEV->data);
		switch (status&0xf0) {
		case 0x00:
		case 0x10:
		case 0x20:
		case 0x40:
		case 0x80:
			if(status & 0x08){
				kprint("count 0x%ux", GET(Tc_hi));
				kprint(" 0x%ux", DEV->data);
				kprint(" 0x%ux\n", DEV->data);
			}
			scsirun();
			break;
		default:
			panic("scsi status 0x%2.2ux", status);
		}
		kprint("resetting...");
		PUT(Own_id, scsiownid);
		PUT(Cmd, Reset);
		break;
	}
}
