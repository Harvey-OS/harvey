#include "all.h"
#include "io.h"
#include "mem.h"

typedef int (*Io)(Device, int, uchar*, int, void*, int);

typedef struct Ctlr {
	ISAConf;
	Io	io;

	int	probe;
	Drive	drive[NTarget];
} Ctlr;

static Ctlr scsi[MaxScsi];

extern Io (*aha1542reset)(int, ISAConf*);
extern Io (*bus4201reset)(int, ISAConf*);
extern Io (*ultra14freset)(int, ISAConf*);

static struct {
	char	*type;
	Io	(*reset)(int, ISAConf*);
} cards[] = {
	{ "aha1542", aha1542reset, },
	{ "bus4201", bus4201reset, },
	{ "ultra14f", ultra14freset, },
	{ 0, }
};

static void
stats(Ctlr *ctlr)
{
	Drive *drive;

	print("scsi%d stats\n", ctlr - scsi);
	for(drive = ctlr->drive; drive < &ctlr->drive[NTarget]; drive++){
		if(drive->fflag == 0)
			continue;
		if(drive->status != Dready)
			print("	drive %.2o (%d)\n", drive - ctlr->drive, drive->status);
		else
			print("	drive %.2o = %10ld\n", drive - ctlr->drive, drive->max);
		print("		work = %F\n", (Filta){&drive->work, 1});
		print("		rate = %F tBps\n", (Filta){&drive->rate, 1000});
	}
}

static void
cmd_stats(int argc, char *argv[])
{
	Ctlr *ctlr;

	USED(argc, argv);

	for(ctlr = &scsi[0]; ctlr < &scsi[MaxScsi]; ctlr++){
		if(ctlr->io)
			stats(ctlr);
	}
}

void
scsiinit(void)
{
	Ctlr *ctlr;
	int n, ctlrno;

	for(ctlrno = 0; ctlrno < MaxScsi; ctlrno++){
		ctlr = &scsi[ctlrno];
		memset(ctlr, 0, sizeof(Ctlr));
		if(isaconfig("scsi", ctlrno, ctlr) == 0)
			continue;

		for(n = 0; cards[n].type; n++){
			if(strcmp(cards[n].type, ctlr->type))
				continue;

			if((ctlr->io = (*cards[n].reset)(ctlrno, ctlr)) == 0) {
				print("scsi%d: %s: port %lux Failed to INIT controller\n", ctlrno, ctlr->type, ctlr->port);
				break;
			}

			print("scsi%d: %s: port %lux irq %d addr %lux size %d\n",
				ctlrno, ctlr->type, ctlr->port,
				ctlr->irq, ctlr->mem, ctlr->size);
			break;
		}
	}
	cmd_install("statd", "-- scsi stats", cmd_stats);
}

static int
reqsense(Ctlr *ctlr, Device d)
{
	uchar cmd[6], sense[255];
	int status;

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0x03;
	cmd[4] = sizeof(sense);
	memset(sense, 0, sizeof(sense));

	status = (*ctlr->io)(d, SCSIread, cmd, sizeof(cmd), sense, sizeof(sense));
	if(status != 0x6000)
		return status;

	/*
	 * Unit attention. We can handle that.
	 */
	if((sense[2] & 0x0F) == 0x00 || (sense[2] & 0x0F) == 0x06)
		return 0;

	/*
	 * Recovered error. Why bother telling me.
	 */
	if((sense[2] & 0x0F) == 0x01)
		return 0;

	print("scsi reqsense %D: key #%2.2ux code #%2.2ux #%2.2ux\n",
		d, sense[2], sense[12], sense[13]);

	return 0x6002;
}

int
scsiio(Device d, int rw, uchar *cmd, int clen, void *data, int dlen)
{
	Drive *drive;
	int status;

	if(d.ctrl >= MaxScsi || scsi[d.ctrl].io == 0)
		panic("scsiio: d.ctrl = %d\n", d.ctrl);

	drive = &scsi[d.ctrl].drive[d.unit];
	if(drive->fflag == 0) {
		dofilter(&drive->rate);
		dofilter(&drive->work);
		drive->fflag = 1;
	}
	drive->work.count++;
	drive->rate.count += dlen;

retry:
	if((status = (*scsi[d.ctrl].io)(d, rw, cmd, clen, data, dlen)) == 0x6000)
		status = 0;
	if(status == 0x6002 && reqsense(&scsi[d.ctrl], d) == 0){
		print("scsiio: retry I/O on %D\n", d);
		goto retry;
	}

	return status;
}

static int
testur(Ctlr *ctlr, Device d)
{
	uchar cmd[6];

	/* Test unit ready */
	memset(cmd, 0, sizeof(cmd));
	return (*ctlr->io)(d, SCSIread, cmd, sizeof(cmd), 0, 0);
}

static void
scsiprobe(Ctlr *ctlr, Device d)
{
	Drive *drive;
	int i, t, status;

	for(i = 0; i < NTarget; i++){
		drive = &ctlr->drive[i];
		drive->status = Dready;
		d.unit = i;

		status = 0;
		for(t = 0; t < 3; t++) {
			status = testur(ctlr, d);
			if(status == 0x6000)
				break;
		}
		if(status == 0x6000) {
			print("%D: ready\n", d);
			continue;
		}
		if(status == 0x0300){
			drive->status = Dself;
			continue;
		}
		if((status & 0xFF00) == 0x1100){
			drive->status = Dunavail;
			continue;
		}
		if(status == 0x6002 && reqsense(ctlr, d) == 0)
			continue;
		drive->status = Dnready;
	}
}

Drive*
scsidrive(Device d)
{
	Ctlr *ctlr;

	if(d.ctrl >= MaxScsi || scsi[d.ctrl].io == 0 || d.unit >= NTarget)
		return 0;

	ctlr = &scsi[d.ctrl];
	ctlr->drive[d.unit].dev = d;
	if(ctlr->probe == 0) {
		scsiprobe(ctlr, d);
		ctlr->probe = 1;
	}

	if(ctlr->drive[d.unit].status == Dself)
		return 0;

	return &ctlr->drive[d.unit];
}
