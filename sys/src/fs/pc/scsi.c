#include "all.h"
#include "io.h"
#include "mem.h"

extern Scsiio (*buslogic)(int, ISAConf*);
extern Scsiio (*ncr53c8xxreset)(int, ISAConf*);

static struct {
	char*	type;
	Scsiio	(*reset)(int, ISAConf*);
} scsictlr[] = {
	{ "aha1542", buslogic, },
	{ "buslogic", buslogic, },
	{ "ncr53c8xx", ncr53c8xxreset, },
	{ 0, },
};

enum {
	Ninquiry	= 255,
	Nsense		= 255,

	CMDtest		= 0x00,
	CMDreqsense	= 0x03,
	CMDread6	= 0x08,
	CMDwrite6	= 0x0A,
	CMDinquiry	= 0x12,
	CMDstart	= 0x1B,
	CMDread10	= 0x28,
	CMDwrite10	= 0x2A,
};

typedef struct {
	ISAConf;
	Scsiio	io;

	Target	target[NTarget];
} Ctlr;
static Ctlr scsi[MaxScsi];

static void
cmd_stat(int, char*[])
{
	Ctlr *ctlr;
	int ctlrno, targetno;
	Target *tp;

	for(ctlrno = 0; ctlrno < MaxScsi; ctlrno++){
		ctlr = &scsi[ctlrno];
		if(ctlr->io == 0)
			continue;
		for(targetno = 0; targetno < NTarget; targetno++){
			tp = &ctlr->target[targetno];
			if(tp->fflag == 0)
				continue;
			print("	%d.%d work =%7W%7W%7W pkts\n", ctlrno, targetno,
				tp->work+0, tp->work+1, tp->work+2);
			print("	    rate =%7W%7W%7W tBps\n",
				tp->rate+0, tp->rate+1, tp->rate+2);
		}
	}
}

void
scsiinit(void)
{
	Ctlr *ctlr;
	int ctlrno, n, targetno;
	Target *tp;

	for(ctlrno = 0; ctlrno < MaxScsi; ctlrno++){
		ctlr = &scsi[ctlrno];
		memset(ctlr, 0, sizeof(Ctlr));
		if(!isaconfig("scsi", ctlrno, ctlr))
			continue;

		for(n = 0; scsictlr[n].type; n++) {
			if(strcmp(scsictlr[n].type, ctlr->type))
				continue;
			if((ctlr->io = (*scsictlr[n].reset)(ctlrno, ctlr)) == 0)
				break;

			print("scsi#%d: %s: port 0x%lux irq %lud",
				ctlrno, ctlr->type, ctlr->port,
				ctlr->irq);
			if(ctlr->mem)
				print(" addr 0x%lux", ctlr->mem & ~KZERO);
			if(ctlr->size)
				print(" size 0x%lux", ctlr->size);
			print("\n");

			for(targetno = 0; targetno < NTarget; targetno++){
				tp = &ctlr->target[targetno];

				qlock(tp);
				qunlock(tp);
				sprint(tp->id, "scsi#%d.%d", ctlrno, targetno);
				tp->name = tp->id;

				tp->ctlrno = ctlrno;
				tp->targetno = targetno;
				tp->inquiry = ialloc(Ninquiry, 0);
				tp->sense = ialloc(Nsense, 0);
			}
			break;
		}
		if(ctlr->io == 0)
			print("scsi#%d: %s: port %lux Failed to INIT controller\n",
				ctlrno, ctlr->type, ctlr->port);
	}
	cmd_install("statd", "-- scsi stats", cmd_stat);
}

static uchar lastcmd[16];
static int lastcmdsz;

static int
scsiexec(Target* tp, int rw, uchar* cmd, int cbytes, void* data, int* dbytes)
{
	int s;

	/*
	 * Call the device-specific I/O routine.
	 */
	switch(s = scsi[tp->ctlrno].io(tp, rw, cmd, cbytes, data, dbytes)){

	case STcheck:
		memmove(lastcmd, cmd, cbytes);
		lastcmdsz = cbytes;
		/*FALLTHROUGH*/

	default:
		/*
		 * It's more complicated than this. There are conditions which
		 * are 'ok' but for which the returned status code is not 'STok'.
		 * Also, not all conditions require a reqsense, there may be a
		 * need to do a reqsense here when necessary and making it
		 * available to the caller somehow.
		 *
		 * Later.
		 */
		break;
	}

	return s;
}

static int
scsitest(Target* tp, char lun)
{
	uchar cmd[6];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = CMDtest;
	cmd[1] = lun<<5;
	return scsiexec(tp, SCSIread, cmd, sizeof(cmd), 0, 0);
}

static int
scsistart(Target* tp, char lun, int start)
{
	uchar cmd[6];

	memset(cmd, 0, sizeof cmd);
	cmd[0] = CMDstart;
	cmd[1] = lun<<5;
	if(start)
		cmd[4] = 1;
	return scsiexec(tp, SCSIread, cmd, sizeof(cmd), 0, 0);
}

static int
scsiinquiry(Target* tp, char lun, int* nbytes)
{
	uchar cmd[6];

	memset(cmd, 0, sizeof cmd);
	cmd[0] = CMDinquiry;
	cmd[1] = lun<<5;
	*nbytes = Ninquiry;
	cmd[4] = *nbytes;
	return scsiexec(tp, SCSIread, cmd, sizeof(cmd), tp->inquiry, nbytes);
}

static char *key[] =
{
	"no sense",
	"recovered error",
	"not ready",
	"medium error",
	"hardware error",
	"illegal request",
	"unit attention",
	"data protect",
	"blank check",
	"vendor specific",
	"copy aborted",
	"aborted command",
	"equal",
	"volume overflow",
	"miscompare",
	"reserved"
};

static int
scsireqsense(Target* tp, char lun, int* nbytes, int quiet)
{
	char *s;
	int n, status, try;
	uchar cmd[6], *sense;

	sense = tp->sense;
	for(try = 0; try < 20; try++) {
		memset(cmd, 0, sizeof(cmd));
		cmd[0] = CMDreqsense;
		cmd[1] = lun<<5;
		cmd[4] = Ninquiry;
		memset(sense, 0, Ninquiry);

		*nbytes = Ninquiry;
		status = scsiexec(tp, SCSIread, cmd, sizeof(cmd), sense, nbytes);
		if(status != STok)
			return status;
		*nbytes = sense[0x07]+8;

		switch(sense[2] & 0x0F){

		case 6:						/* unit attention */
			/*
			 * 0x28 - not ready to ready transition,
			 *	  medium may have changed.
			 * 0x29 - power on, RESET or BUS DEVICE RESET occurred.
			 */
			if(sense[12] != 0x28 && sense[12] != 0x29)
				goto buggery;
			/*FALLTHROUGH*/
		case 0:						/* no sense */
		case 1:						/* recovered error */
			return STok;

		case 8:						/* blank data */
			return STblank;

		case 2:						/* not ready */
			if(sense[12] == 0x3A)			/* medium not present */
				goto buggery;
			/*FALLTHROUGH*/

		default:
			/*
			 * If unit is becoming ready, rather than not ready,
			 * then wait a little then poke it again; should this
			 * be here or in the caller?
			 */
			if((sense[12] == 0x04 && sense[13] == 0x01)){
				waitsec(500);
				scsitest(tp, lun);
				break;
			}
			goto buggery;
		}
	}

buggery:
	if(quiet == 0){
		s = key[sense[2]&0x0F];
		print("%s: reqsense: '%s' code #%2.2ux #%2.2ux\n",
			tp->id, s, sense[12], sense[13]);
		print("%s: byte 2: #%2.2ux, bytes 15-17: #%2.2ux #%2.2ux #%2.2ux\n",
			tp->id, sense[2], sense[15], sense[16], sense[17]);
		print("lastcmd (%d): ", lastcmdsz);
		for(n = 0; n < lastcmdsz; n++)
			print(" #%2.2ux", lastcmd[n]);
		print("\n");
	}

	return STcheck;
}

static Target*
scsitarget(Device* d)
{
	int ctlrno, targetno;

	ctlrno = d->wren.ctrl;
	if(ctlrno < 0 || ctlrno >= MaxScsi || scsi[ctlrno].io == 0)
		return 0;
	targetno = d->wren.targ;
	if(targetno < 0 || targetno >= NTarget)
		return 0;
	return &scsi[ctlrno].target[targetno];
}

static void
scsiprobe(Device* d)
{
	Target *tp;
	int nbytes, s;
	uchar *sense;
	int acount;

	if((tp = scsitarget(d)) == 0)
		panic("scsiprobe: device = %Z\n", d);

	acount = 0;
again:
	s = scsitest(tp, d->wren.lun);
	if(s < STok){
		print("%s: test, status %d\n", tp->id, s);
		return;
	}

	/*
	 * Determine if the drive exists and is not ready or
	 * is simply not responding.
	 * If the status is OK but the drive came back with a 'power on' or
	 * 'reset' status, try the test again to make sure the drive is really
	 * ready.
	 * If the drive is not ready and requires intervention, try to spin it
	 * up.
	 */
	s = scsireqsense(tp, d->wren.lun, &nbytes, acount);
	sense = tp->sense;
	switch(s){
	case STok:
		if((sense[2] & 0x0F) == 0x06 && (sense[12] == 0x28 || sense[12] == 0x29)){
			if(acount == 0){
				acount = 1;
				goto again;
			}
		}
		break;
	case STcheck:
		if((sense[2] & 0x0F) == 0x02){
			if(sense[12] == 0x3A)
				break;
			if(sense[12] == 0x04 && sense[13] == 0x02){
				print("%s: starting...\n", tp->id);
				if(scsistart(tp, d->wren.lun, 1) == STok)
					break;
				s = scsireqsense(tp, d->wren.lun, &nbytes, 0);
			}
		}
		/*FALLTHROUGH*/
	default:
		print("%s: unavailable, status %d\n", tp->id, s);
		return;
	}

	/*
	 * Inquire to find out what the device is.
	 * Hardware drivers may need some of the info.
	 */
	s = scsiinquiry(tp, d->wren.lun, &nbytes);
	if(s != STok) {
		print("%s: inquiry failed, status %d\n", tp->id, s);
		return;
	}
	print("%s: %s\n", tp->id, (char*)tp->inquiry+8);
	tp->ok = 1;
}

int
scsiio(Device* d, int rw, uchar* cmd, int cbytes, void* data, int dbytes)
{
	Target *tp;
	int e, nbytes, s;

	if((tp = scsitarget(d)) == 0)
		panic("scsiio: device = %Z\n", d);

	qlock(tp);
	if(tp->ok == 0)
		scsiprobe(d);
	if(tp->fflag == 0) {
		dofilter(tp->rate+0, C0a, C0b, 1);
		dofilter(tp->rate+1, C1a, C1b, 1);
		dofilter(tp->rate+2, C2a, C2b, 1);
		dofilter(tp->work+0, C0a, C0b, 1000);
		dofilter(tp->work+1, C1a, C1b, 1000);
		dofilter(tp->work+2, C2a, C2b, 1000);
		tp->fflag = 1;
	}
	tp->work[0].count++;
	tp->work[1].count++;
	tp->work[2].count++;
	tp->rate[0].count += dbytes;
	tp->rate[1].count += dbytes;
	tp->rate[2].count += dbytes;
	qunlock(tp);

	s = STinit;
	for(e = 0; e < 10; e++){
		for(;;){
			nbytes = dbytes;
			s = scsiexec(tp, rw, cmd, cbytes, data, &nbytes);
			if(s == STok)
				break;
			s = scsireqsense(tp, d->wren.lun, &nbytes, 0);
			if(s == STblank && rw == SCSIread) {
				memset(data, 0, dbytes);
				return STok;
			}
			if(s != STok)
				break;
		}
		if(s == STok)
			break;
	}
	if(e)
		print("%s: retry %d cmd #%x\n", tp->id, e, cmd[0]);
	return s;
}
