#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "ureg.h"
#include "error.h"

#include "sd.h"

extern SDifc sdataifc;

enum {
	DbgCONFIG	= 0x01,		/* detected drive config info */
	DbgIDENTIFY	= 0x02,		/* detected drive identify info */
	DbgSTATE	= 0x04,		/* dump state on panic */
	DbgPROBE	= 0x08,		/* trace device probing */
};
#define DEBUG		0
/* (DbgSTATE|DbgCONFIG) */

enum {					/* I/O ports */
	Data		= 0,
	Error		= 1,		/* (read) */
	Features	= 1,		/* (write) */
	Count		= 2,		/* sector count */
	Ir		= 2,		/* interrupt reason (PACKET) */
	Sector		= 3,		/* sector number, LBA<7-0> */
	Cyllo		= 4,		/* cylinder low, LBA<15-8> */
	Bytelo		= 4,		/* byte count low (PACKET) */
	Cylhi		= 5,		/* cylinder high, LBA<23-16> */
	Bytehi		= 5,		/* byte count hi (PACKET) */
	Dh		= 6,		/* Device/Head, LBA<32-14> */
	Status		= 7,		/* (read) */
	Command		= 7,		/* (write) */

	As		= 2,		/* Alternate Status (read) */
	Dc		= 2,		/* Device Control (write) */
};

enum {					/* Error */
	Med		= 0x01,		/* Media error */
	Ili		= 0x01,		/* command set specific (PACKET) */
	Nm		= 0x02,		/* No Media */
	Eom		= 0x02,		/* command set specific (PACKET) */
	Abrt		= 0x04,		/* Aborted command */
	Mcr		= 0x08,		/* Media Change Request */
	Idnf		= 0x10,		/* no user-accessible address */
	Mc		= 0x20,		/* Media Change */
	Unc		= 0x40,		/* Uncorrectable data error */
	Wp		= 0x40,		/* Write Protect */
	Icrc		= 0x80,		/* Interface CRC error */
};

enum {					/* Features */
	Dma		= 0x01,		/* data transfer via DMA (PACKET) */
	Ovl		= 0x02,		/* command overlapped (PACKET) */
};

enum {					/* Interrupt Reason */
	Cd		= 0x01,		/* Command/Data */
	Io		= 0x02,		/* I/O direction */
	Rel		= 0x04,		/* Bus Release */
};

enum {					/* Device/Head */
	Dev0		= 0xA0,		/* Master */
	Dev1		= 0xB0,		/* Slave */
	Lba		= 0x40,		/* LBA mode */
};

enum {					/* Status, Alternate Status */
	Err		= 0x01,		/* Error */
	Chk		= 0x01,		/* Check error (PACKET) */
	Drq		= 0x08,		/* Data Request */
	Dsc		= 0x10,		/* Device Seek Complete */
	Serv		= 0x10,		/* Service */
	Df		= 0x20,		/* Device Fault */
	Dmrd		= 0x20,		/* DMA ready (PACKET) */
	Drdy		= 0x40,		/* Device Ready */
	Bsy		= 0x80,		/* Busy */
};

enum {					/* Command */
	Cnop		= 0x00,		/* NOP */
	Cdr		= 0x08,		/* Device Reset */
	Crs		= 0x20,		/* Read Sectors */
	Cws		= 0x30,		/* Write Sectors */
	Cedd		= 0x90,		/* Execute Device Diagnostics */
	Cpkt		= 0xA0,		/* Packet */
	Cidpkt		= 0xA1,		/* Identify Packet Device */
	Crsm		= 0xC4,		/* Read Multiple */
	Cwsm		= 0xC5,		/* Write Multiple */
	Crdq		= 0xC7,		/* Read DMA queued */
	Crd		= 0xC8,		/* Read DMA */
	Cwd		= 0xCA,		/* Write DMA */
	Cwdq		= 0xCC,		/* Write DMA queued */
	Cid		= 0xEC,		/* Identify Device */
	Csf		= 0xEF,		/* Set Features */
};

enum {					/* Device Control */
	Nien		= 0x02,		/* (not) Interrupt Enable */
	Srst		= 0x04,		/* Software Reset */
};

enum {					/* PCI Configuration Registers */
	Bmiba		= 0x20,		/* Bus Master Interface Base Address */
	Idetim		= 0x40,		/* IE Timing */
	Sidetim		= 0x44,		/* Slave IE Timing */
	Udmactl		= 0x48,		/* Ultra DMA/33 Control */
	Udmatim		= 0x4A,		/* Ultra DMA/33 Timing */
};

enum {					/* Bus Master IDE I/O Ports */
	Bmicx		= 0,		/* Command */
	Bmisx		= 2,		/* Status */
	Bmidtpx		= 4,		/* Descriptor Table Pointer */
};

enum {					/* Bmicx */
	Ssbm		= 0x01,		/* Start/Stop Bus Master */
	Rwcon		= 0x08,		/* Read/Write Control */
};

enum {					/* Bmisx */
	Bmidea		= 0x01,		/* Bus Master IDE Active */
	Idedmae		= 0x02,		/* IDE DMA Error  (R/WC) */
	Ideints		= 0x04,		/* IDE Interrupt Status (R/WC) */
	Dma0cap		= 0x20,		/* Drive 0 DMA Capable */
	Dma1cap		= 0x40,		/* Drive 0 DMA Capable */
};
enum {					/* Physical Region Descriptor */
	PrdEOT		= 0x80000000,	/* Bus Master IDE Active */
};

enum {					/* offsets into the identify info. */
	Iconfig		= 0,		/* general configuration */
	Ilcyl		= 1,		/* logical cylinders */
	Ilhead		= 3,		/* logical heads */
	Ilsec		= 6,		/* logical sectors per logical track */
	Iserial		= 10,		/* serial number */
	Ifirmware	= 23,		/* firmware revision */
	Imodel		= 27,		/* model number */
	Imaxrwm		= 47,		/* max. read/write multiple sectors */
	Icapabilities	= 49,		/* capabilities */
	Istandby	= 50,		/* device specific standby timer */
	Ipiomode	= 51,		/* PIO data transfer mode number */
	Ivalid		= 53,
	Iccyl		= 54,		/* cylinders if (valid&0x01) */
	Ichead		= 55,		/* heads if (valid&0x01) */
	Icsec		= 56,		/* sectors if (valid&0x01) */
	Iccap		= 57,		/* capacity if (valid&0x01) */
	Irwm		= 59,		/* read/write multiple */
	Ilba0		= 60,		/* LBA size */
	Ilba1		= 61,		/* LBA size */
	Imwdma		= 63,		/* multiword DMA mode */
	Iapiomode	= 64,		/* advanced PIO modes supported */
	Iminmwdma	= 65,		/* min. multiword DMA cycle time */
	Irecmwdma	= 66,		/* rec. multiword DMA cycle time */
	Iminpio		= 67,		/* min. PIO cycle w/o flow control */
	Iminiordy	= 68,		/* min. PIO cycle with IORDY */
	Ipcktbr		= 71,		/* time from PACKET to bus release */
	Iserbsy		= 72,		/* time from SERVICE to !Bsy */
	Iqdepth		= 75,		/* max. queue depth */
	Imajor		= 80,		/* major version number */
	Iminor		= 81,		/* minor version number */
	Icmdset0	= 82,		/* command sets supported */
	Icmdset1	= 83,		/* command sets supported */
	Icmdset2	= 84,		/* command sets supported extension */
	Icmdset3	= 85,		/* command sets enabled */
	Icmdset4	= 86,		/* command sets enabled */
	Icmdset5	= 87,		/* command sets enabled extension */
	Iudma		= 88,		/* ultra DMA mode */
	Ierase		= 89,		/* time for security erase */
	Ieerase		= 90,		/* time for enhanced security erase */
	Ipower		= 91,		/* current advanced power management */
	Irmsn		= 127,		/* removable status notification */
	Istatus		= 128,		/* security status */
};

typedef struct Ctlr Ctlr;
typedef struct Drive Drive;

typedef struct Prd {
	ulong	pa;			/* Physical Base Address */
	int	count;
} Prd;

enum {
	Nprd		= SDmaxio/(64*1024)+2,
};

typedef struct Ctlr {
	int	cmdport;
	int	ctlport;
	int	irq;
//	int	bmiba;			/* bus master interface base address */

	Pcidev*	pcidev;
	void	(*ienable)(Ctlr*);

	Drive*	drive[2];

//	Prd*	prdt;			/* physical region descriptor table */

//	QLock;				/* current command */
	Drive*	curdrive;
//	Rendez;
	int	done;

	Lock;				/* register access */
} Ctlr;

typedef struct Drive {
	Ctlr*	ctlr;

	int	dev;
	ushort	info[256];
	int	c;			/* cylinder */
	int	h;			/* head */
	int	s;			/* sector */
	int	sectors;		/* total */
	int	secsize;		/* sector size */
	int	block;			/* R/W multiple size */
	int	pior;			/* PIO read command */
	int	piow;			/* PIO write command */
	int	dma;			/* DMA R/W possible */
	int	pkt;			/* PACKET device, length of pktcmd */

	uchar	sense[18];
	uchar	inquiry[48];

//	QLock;				/* drive access */
	int	command;		/* current command */
	int	write;
	uchar*	data;
	int	dlen;
	uchar*	limit;
	int	count;			/* sectors */
	int	status;
	int	error;

	uchar	pktcmd[16];
	int	pktdma;
} Drive;

static void
pc87415ienable(Ctlr* ctlr)
{
	Pcidev *p;
	int x;

	p = ctlr->pcidev;
	if(p == nil)
		return;

	x = pcicfgr32(p, 0x40);
	if(ctlr->cmdport == p->mem[0].bar)
		x &= ~0x00000100;
	else
		x &= ~0x00000200;
	pcicfgw32(p, 0x40, x);
}

int
atadebug(int cmdport, int ctlport, char* fmt, ...)
{
	int i, n;
	va_list arg;
	char buf[PRINTSIZE];

	if(!DEBUG){
		USED(cmdport, ctlport, fmt);
		return 0;
	}

	va_start(arg, fmt);
	n = doprint(buf, buf+sizeof(buf), fmt, arg) - buf;
	va_end(arg);

	if(cmdport){
		if(buf[n-1] == '\n')
			n--;
		n += snprint(buf+n, PRINTSIZE-n, " ataregs 0x%uX:",
			cmdport);
		for(i = 1; i < 7; i++)
			n += snprint(buf+n, PRINTSIZE-n, " 0x%2.2uX",
				inb(cmdport+i));
		if(ctlport)
			n += snprint(buf+n, PRINTSIZE-n, " 0x%2.2uX",
				inb(ctlport+As));
		n += snprint(buf+n, PRINTSIZE-n, "\n");
	}
	putstrn(buf, n);

	return n;
}

static int
ataready(int cmdport, int ctlport, int dev, int reset, int ready, int micro)
{
	int as;

	atadebug(cmdport, ctlport, "ataready: dev %uX reset %uX ready %uX",
		dev, reset, ready);

	for(;;){
		/*
		 * Wait for the controller to become not busy and
		 * possibly for a status bit to become true (usually
		 * Drdy). Must change to the appropriate device
		 * register set before testing for ready.
		 * Always run through the loop at least once so it
		 * can be used as a test for !Bsy.
		 */
		as = inb(ctlport+As);
		if(dev && !(as & (Bsy|Drq))){
			outb(cmdport+Dh, dev);
			dev = 0;
			continue;
		}
		else if(!(as & reset)){
			if(ready == 0 || (as & ready)){
				atadebug(0, 0, "ataready: %d 0x%2.2uX\n",
					micro, as);
				return as;
			}
		}

		if(micro-- <= 0){
			atadebug(0, 0, "ataready: %d 0x%2.2uX\n",
				micro, as);
			break;
		}
		microdelay(1);
	}
	atadebug(cmdport, ctlport, "ataready: timeout");

	return -1;
}

static int
atacsfenabled(Drive* drive, vlong csf)
{
	int cmdset, i, x;

	for(i = 0; i < 3; i++){
		x = (csf>>(16*i)) & 0xFFFF;
		if(x == 0)
			continue;
		cmdset = drive->info[Icmdset3+i];
		if(cmdset == 0 || cmdset == 0xFFFF)
			return 0;
		return cmdset & x;
	}

	return 0;
}

static int
atasetrwmode(Drive* drive, int cmdport, int ctlport, int dev)
{
	int as, block;

	drive->block = drive->secsize;
	drive->pior = Crs;
	drive->piow = Cws;
	if((block = drive->info[Imaxrwm] & 0xFF) == 0)
		return 0;

	/*
	 * Prior to ATA-4 there was no way to determine the
	 * current block count (now in Irwm).
	 * Sometimes drives come up with the current count set
	 * to 0, so always set a suitable value.
	 */
	if(ataready(cmdport, ctlport, dev, Bsy|Drq, Drdy, 100*1000) < 0)
		return 0;
	outb(cmdport+Sector, block);
	outb(cmdport+Command, Csf);
	microdelay(1);
	as = ataready(cmdport, ctlport, dev, Bsy, Drdy|Df|Err, 1000);
	if(as < 0 || (as & (Df|Err)))
		return -1;

	drive->info[Irwm] = 0x100|block;
	drive->block = block*drive->secsize;
	drive->pior = Crsm;
	drive->piow = Cwsm;

	return block;
}

static Drive*
ataidentify(int cmdport, int ctlport, int dev)
{
	Drive *drive;
	uchar buf[512], *p;
	int command, i, as;
	ushort *sp;

	atadebug(0, 0, "identify: port 0x%uX dev 0x%2.2uX\n", cmdport, dev);
	command = Cidpkt;
retry:
	as = ataready(cmdport, ctlport, dev, Bsy|Drq, 0, 100*1000);
	if(as < 0)
		return nil;
	outb(cmdport+Command, command);
	microdelay(1);

	as = ataready(cmdport, ctlport, dev, Bsy, Drq|Err, 100*1000);
	if(as < 0)
		return nil;
	if(as & Err){
		if(command == Cid)
			return nil;
		command = Cid;
		goto retry;
	}
	memset(buf, 0, sizeof(buf));
	inss(cmdport+Data, buf, 256);
	inb(cmdport+Status);

	if(DEBUG & DbgIDENTIFY){
		int i;
		ushort *sp;

		sp = (ushort*)buf;
		for(i = 0; i < 256; i++){
			if(i && (i%16) == 0)
				print("\n");
			print("%4.4uX ", *sp);
			sp++;
		}
		print("\n");
	}

	if((drive = malloc(sizeof(Drive))) == nil)
		return nil;
	drive->dev = dev;
	memmove(drive->info, buf, sizeof(drive->info));
	drive->sense[0] = 0x70;
	drive->sense[7] = sizeof(drive->sense)-7;

	drive->inquiry[2] = 2;
	drive->inquiry[3] = 2;
	drive->inquiry[4] = sizeof(drive->inquiry)-4;
	p = &drive->inquiry[8];
	sp = &drive->info[Imodel];
	for(i = 0; i < 20; i++){
		*p++ = *sp>>8;
		*p++ = *sp++;
	}

	drive->secsize = 512;
	if((drive->info[Iconfig] & 0xC000) == 0x8000){
		if(drive->info[Iconfig] & 0x01)
			drive->pkt = 16;
		else
			drive->pkt = 12;
	}
	else{
		if(drive->info[Ivalid] & 0x0001){
			drive->c = drive->info[Ilcyl];
			drive->h = drive->info[Ilhead];
			drive->s = drive->info[Ilsec];
		}
		else{
			drive->c = drive->info[Iccyl];
			drive->h = drive->info[Ichead];
			drive->s = drive->info[Icsec];
		}
		if(drive->info[Icapabilities] & 0x0200){
			drive->sectors = (drive->info[Ilba1]<<16)
					 |drive->info[Ilba0];
			drive->dev |= Lba;
		}
		else
			drive->sectors = drive->c*drive->h*drive->s;
		atasetrwmode(drive, cmdport, ctlport, dev);
	}	

	drive->dma = 0;

	if(DEBUG & DbgCONFIG){
		print("dev %2.2uX capabilities %4.4uX config %4.4uX",
			dev, drive->info[Icapabilities], drive->info[Iconfig]);
		print(" mwdma %4.4uX dma %8.8uX", 
			drive->info[Imwdma], drive->dma);
		if(drive->info[Ivalid] & 0x04)
			print(" udma %4.4uX", drive->info[Iudma]);
		print("\n");
	}

	return drive;
}

static void
atasrst(int ctlport)
{
	/*
	 * Srst is a big stick and may cause problems if further
	 * commands are tried before the drives become ready again.
	 * Also, there will be problems here if overlapped commands
	 * are ever supported.
	 */
	microdelay(5);
	outb(ctlport+Dc, Srst);
	microdelay(5);
	outb(ctlport+Dc, 0);
	microdelay(2*1000);
}

static SDev*
ataprobe(int cmdport, int ctlport, int irq)
{
	Ctlr* ctlr;
	SDev *sdev;
	Drive *drive;
	int dev, error, rhi, rlo;

	/*
	 * Try to detect a floating bus.
	 * Bsy should be cleared. If not, see if the cylinder registers
	 * are read/write capable.
	 * If the master fails, try the slave to catch slave-only
	 * configurations.
	 * There's no need to restore the tested registers as they will
	 * be reset on any detected drives by the Cedd command.
	 * All this indicates is that there is at least one drive on the
	 * controller; when the non-existent drive is selected in a
	 * single-drive configuration the registers of the existing drive
	 * are often seen, only command execution fails.
	 */
	dev = Dev0;
	if(inb(ctlport+As) & Bsy){
		outb(cmdport+Dh, dev);
trydev1:
		atadebug(cmdport, ctlport, "ataprobe bsy");
		outb(cmdport+Cyllo, 0xAA);
		outb(cmdport+Cylhi, 0x55);
		outb(cmdport+Sector, 0xFF);
		rlo = inb(cmdport+Cyllo);
		rhi = inb(cmdport+Cylhi);
		if(rlo != 0xAA && (rlo == 0xFF || rhi != 0x55)){
			if(dev == Dev1){
release:
				return nil;
			}
			dev = Dev1;
			if(ataready(cmdport, ctlport, dev, Bsy, 0, 20*1000) < 0)
				goto trydev1;
		}
	}

	/*
	 * Disable interrupts on any detected controllers.
	 */
	outb(ctlport+Dc, Nien);
tryedd1:
	if(ataready(cmdport, ctlport, dev, Bsy|Drq, 0, 100*1000) < 0){
		/*
		 * There's something there, but it didn't come up clean,
		 * so try hitting it with a big stick. The timing here is
		 * wrong but this is a last-ditch effort and it sometimes
		 * gets some marginal hardware back online.
		 */
		atasrst(ctlport);
		if(ataready(cmdport, ctlport, dev, Bsy|Drq, 0, 100*1000) < 0)
			goto release;
	}

	/*
	 * Can only get here if controller is not busy.
	 * If there are drives Bsy will be set within 400nS,
	 * must wait 2mS before testing Status.
	 * Wait for the command to complete (6 seconds max).
	 */
	outb(cmdport+Command, Cedd);
	delay(2);
	if(ataready(cmdport, ctlport, dev, Bsy|Drq, 0, 6*1000*1000) < 0)
		goto release;

	/*
	 * If bit 0 of the error register is set then the selected drive
	 * exists. This is enough to detect single-drive configurations.
	 * However, if the master exists there is no way short of executing
	 * a command to determine if a slave is present.
	 * It appears possible to get here testing Dev0 although it doesn't
	 * exist and the EDD won't take, so try again with Dev1.
	 */
	error = inb(cmdport+Error);
	atadebug(cmdport, ctlport, "ataprobe: dev %uX", dev);
	if((error & ~0x80) != 0x01){
		if(dev == Dev1)
			goto release;
		dev = Dev1;
		goto tryedd1;
	}

	/*
	 * At least one drive is known to exist, try to
	 * identify it. If that fails, don't bother checking
	 * any further.
	 * If the one drive found is Dev0 and the EDD command
	 * didn't indicate Dev1 doesn't exist, check for it.
	 */
	if((drive = ataidentify(cmdport, ctlport, dev)) == nil)
		goto release;
	if((ctlr = malloc(sizeof(Ctlr))) == nil){
		free(drive);
		goto release;
	}
	if((sdev = malloc(sizeof(SDev))) == nil){
		free(ctlr);
		free(drive);
		goto release;
	}
	drive->ctlr = ctlr;
	if(dev == Dev0){
		ctlr->drive[0] = drive;
		if(!(error & 0x80)){
			/*
			 * Always leave Dh pointing to a valid drive,
			 * otherwise a subsequent call to ataready on
			 * this controller may try to test a bogus Status.
			 * Ataprobe is the only place possibly invalid
			 * drives should be selected.
			 */
			drive = ataidentify(cmdport, ctlport, Dev1);
			if(drive != nil){
				drive->ctlr = ctlr;
				ctlr->drive[1] = drive;
			}
			else
				outb(cmdport+Dh, Dev0);
		}
	}
	else
		ctlr->drive[1] = drive;

	ctlr->cmdport = cmdport;
	ctlr->ctlport = ctlport;
	ctlr->irq = irq;

	sdev->ifc = &sdataifc;
	sdev->ctlr = ctlr;
	sdev->nunit = 2;

	return sdev;
}

static int
atasetsense(Drive* drive, int status, int key, int asc, int ascq)
{
	drive->sense[2] = key;
	drive->sense[12] = asc;
	drive->sense[13] = ascq;

	return status;
}

static int
atamodesense(Drive* drive, uchar* cmd)
{
	int len;

	/*
	 * Fake a vendor-specific request with page code 0,
	 * return the drive info.
	 */
	if((cmd[2] & 0x3F) != 0 && (cmd[2] & 0x3F) != 0x3F)
		return atasetsense(drive, SDcheck, 0x05, 0x24, 0);
	len = (cmd[7]<<8)|cmd[8];
	if(len == 0)
		return SDok;
	if(len < 8+sizeof(drive->info))
		return atasetsense(drive, SDcheck, 0x05, 0x1A, 0);
	if(drive->data == nil || drive->dlen < len)
		return atasetsense(drive, SDcheck, 0x05, 0x20, 1);
	memset(drive->data, 0, 8);
	drive->data[0] = sizeof(drive->info)>>8;
	drive->data[1] = sizeof(drive->info);
	memmove(drive->data+8, drive->info, sizeof(drive->info));
	drive->data += 8+sizeof(drive->info);

	return SDok;
}

static void
atanop(Drive* drive, int subcommand)
{
	Ctlr* ctlr;
	int as, cmdport, ctlport, timeo;

	/*
	 * Attempt to abort a command by using NOP.
	 * In response, the drive is supposed to set Abrt
	 * in the Error register, set (Drdy|Err) in Status
	 * and clear Bsy when done. However, some drives
	 * (e.g. ATAPI Zip) just go Bsy then clear Status
	 * when done, hence the timeout loop only on Bsy
	 * and the forced setting of drive->error.
	 */
	ctlr = drive->ctlr;
	cmdport = ctlr->cmdport;
	outb(cmdport+Features, subcommand);
	outb(cmdport+Dh, drive->dev);
	outb(cmdport+Command, 0);

	microdelay(1);
	ctlport = ctlr->ctlport;
	for(timeo = 0; timeo < 1000; timeo++){
		as = inb(ctlport+As);
		if(!(as & Bsy))
			break;
		microdelay(1);
	}
	drive->error |= Abrt;
}

static int
atapktiodone(void* arg)
{
	return ((Ctlr*)arg)->done;
}

static void
atapktinterrupt(Drive* drive)
{
	Ctlr* ctlr;
	int cmdport, len;

	ctlr = drive->ctlr;
	cmdport = ctlr->cmdport;
	switch(inb(cmdport+Ir) & (/*Rel|*/Io|Cd)){
	case Cd:
		outss(cmdport+Data, drive->pktcmd, drive->pkt/2);
		break;

	case 0:
		len = (inb(cmdport+Bytehi)<<8)|inb(cmdport+Bytelo);
		if(drive->data+len > drive->limit){
			atanop(drive, 0);
			break;
		}
		outss(cmdport+Data, drive->data, len/2);
		drive->data += len;
		break;

	case Io:
		len = (inb(cmdport+Bytehi)<<8)|inb(cmdport+Bytelo);
		if(drive->data+len > drive->limit){
			atanop(drive, 0);
			break;
		}
		inss(cmdport+Data, drive->data, len/2);
		drive->data += len;
		break;

	case Io|Cd:
ctlr->done=1;
		break;
	}
}

static int
atapktio(Drive* drive, uchar* cmd, int clen)
{
	Ctlr *ctlr;
	int as, cmdport, ctlport, len, r;

	if(cmd[0] == 0x5A && (cmd[2] & 0x3F) == 0)
		return atamodesense(drive, cmd);

	r = SDok;

	drive->command = Cpkt;
	memmove(drive->pktcmd, cmd, clen);
	memset(drive->pktcmd+clen, 0, drive->pkt-clen);
	drive->limit = drive->data+drive->dlen;

	ctlr = drive->ctlr;
	cmdport = ctlr->cmdport;
	ctlport = ctlr->ctlport;

	qlock(ctlr);

	if(ataready(cmdport, ctlport, drive->dev, Bsy|Drq, 0, 100*1000) < 0)
		return -1;

	ilock(ctlr);
drive->pktdma = 0;

	outb(cmdport+Features, drive->pktdma);
	outb(cmdport+Count, 0);
	outb(cmdport+Sector, 0);
	len = 16*drive->secsize;
	outb(cmdport+Bytelo, len);
	outb(cmdport+Bytehi, len>>8);
	outb(cmdport+Dh, drive->dev);
	ctlr->done = 0;
	outb(cmdport+Command, Cpkt);

	if((drive->info[Iconfig] & 0x0060) != 0x0020){
		as = ataready(cmdport, ctlport,
			drive->dev, Bsy, Drq|Chk, 4*1000);
		if(as < 0)
			r = SDtimeout;
		else if(as & Chk)
			r = SDcheck;
		else
			atapktinterrupt(drive);
	}
	ctlr->curdrive = drive;
	iunlock(ctlr);

	tsleep(ctlr, atapktiodone, ctlr, 10*1000);
	if(!ctlr->done)
		panic("atapktiodone");

	qunlock(ctlr);

	if(drive->status & Chk)
		r = SDcheck;

	return r;
}

static int
atageniodone(void* arg)
{
	return ((Ctlr*)arg)->done;
}

static int
atageniostart(Drive* drive, int lba)
{
	Ctlr *ctlr;
	int as, c, cmdport, ctlport, h, len, s;

	if(drive->dev & Lba){
		c = (lba>>8) & 0xFFFF;
		h = (lba>>24) & 0x0F;
		s = lba & 0xFF;
	}
	else{
		c = lba/(drive->s*drive->h);
		h = ((lba/drive->s) % drive->h);
		s = (lba % drive->s) + 1;
	}

	ctlr = drive->ctlr;
	cmdport = ctlr->cmdport;
	ctlport = ctlr->ctlport;
	if(ataready(cmdport, ctlport, drive->dev, Bsy|Drq, 0, 100*1000) < 0)
		return -1;

	ilock(ctlr);
	if(drive->write)
		drive->command = drive->piow;
	else
		drive->command = drive->pior;

	outb(cmdport+Count, drive->count);
	outb(cmdport+Sector, s);
	outb(cmdport+Dh, drive->dev|h);
	outb(cmdport+Cyllo, c);
	outb(cmdport+Cylhi, c>>8);
	ctlr->done = 0;
	outb(cmdport+Command, drive->command);
	microdelay(1);

	switch(drive->command){
	case Cws:
	case Cwsm:
		as = ataready(cmdport, ctlport, drive->dev, Bsy, Drq|Err, 1000);
		if(as < 0 || (as & Err)){
			iunlock(ctlr);
			return -1;
		}
		len = drive->block;
		if(drive->data+len > drive->limit)
			len = drive->limit-drive->data;
		outss(cmdport+Data, drive->data, len/2);
		break;

	case Crd:
	case Cwd:
panic("atadmastart");
		break;
	}
	ctlr->curdrive = drive;
	iunlock(ctlr);

	return 0;
}

static int
atagenio(Drive* drive, uchar* cmd, int)
{
	uchar *p;
	Ctlr *ctlr;
	int count, lba, len;

	/*
	 * Map SCSI commands into ATA commands for discs.
	 * Fail any command with a LUN except INQUIRY which
	 * will return 'logical unit not supported'.
	 */
	if((cmd[1]>>5) && cmd[0] != 0x12)
		return atasetsense(drive, SDcheck, 0x05, 0x25, 0);

	switch(cmd[0]){
	default:
		return atasetsense(drive, SDcheck, 0x05, 0x20, 0);

	case 0x00:			/* test unit ready */
		return SDok;

	case 0x03:			/* request sense */
		if(cmd[4] < sizeof(drive->sense))
			len = cmd[4];
		else
			len = sizeof(drive->sense);
		if(drive->data && drive->dlen >= len){
			memmove(drive->data, drive->sense, len);
			drive->data += len;
		}
		return SDok;

	case 0x12:			/* inquiry */
		if(cmd[4] < sizeof(drive->inquiry))
			len = cmd[4];
		else
			len = sizeof(drive->inquiry);
		if(drive->data && drive->dlen >= len){
			memmove(drive->data, drive->inquiry, len);
			drive->data += len;
		}
		return SDok;

	case 0x1B:			/* start/stop unit */
		/*
		 * NOP for now, can use the power management feature
		 * set later.
		 */
		return SDok;

	case 0x25:			/* read capacity */
		if((cmd[1] & 0x01) || cmd[2] || cmd[3])
			return atasetsense(drive, SDcheck, 0x05, 0x24, 0);
		if(drive->data == nil || drive->dlen < 8)
			return atasetsense(drive, SDcheck, 0x05, 0x20, 1);
		/*
		 * Read capacity returns the LBA of the last sector.
		 */
		len = drive->sectors-1;
		p = drive->data;
		*p++ = len>>24;
		*p++ = len>>16;
		*p++ = len>>8;
		*p++ = len;
		len = drive->secsize;
		*p++ = len>>24;
		*p++ = len>>16;
		*p++ = len>>8;
		*p = len;
		drive->data += 8;
		return SDok;

	case 0x28:			/* read */
	case 0x2A:			/* write */
		break;

	case 0x5A:
		return atamodesense(drive, cmd);
	}

	ctlr = drive->ctlr;
	lba = (cmd[2]<<24)|(cmd[3]<<16)|(cmd[4]<<8)|cmd[5];
	count = (cmd[7]<<8)|cmd[8];
	if(drive->data == nil)
		return SDok;
	if(drive->dlen < count*drive->secsize)
		count = drive->dlen/drive->secsize;
	qlock(ctlr);
	while(count){
		if(count > 256)
			drive->count = 256;
		else
			drive->count = count;
		drive->limit += drive->count*drive->secsize;
		if(atageniostart(drive, lba)){
			qunlock(ctlr);
			return atasetsense(drive, SDcheck, 2, 5, 0);
		}
		lba += drive->count;

		tsleep(ctlr, atageniodone, ctlr, 10*1000);
		if(!ctlr->done){
			/*
			 * What should the above timeout be? In
			 * standby and sleep modes it could take as
			 * long as 30 seconds for a drive to respond.
			 * Very hard to get out of this cleanly.
			 * Let's see if it ever happens first...
			 */
			panic("atagenio");
		}

		if(drive->status & Err){
			qunlock(ctlr);
			return atasetsense(drive, SDcheck, 4, 8, drive->error);
		}
		if(drive->data != drive->limit)
			print("atagenio: %d != %d\n",
				(int)drive->data, (int)drive->limit);
		count -= drive->count;
	}
	qunlock(ctlr);

	return SDok;
}

static int
atario(SDreq* r)
{
	Ctlr *ctlr;
	Drive *drive;
	SDunit *unit;
	uchar cmd10[10], *cmdp, *p;
	int clen, reqstatus, status;

	unit = r->unit;
	if((ctlr = unit->dev->ctlr) == nil || ctlr->drive[unit->subno] == nil){
		r->status = SDtimeout;
		return SDtimeout;
	}
	drive = ctlr->drive[unit->subno];

	/*
	 * Most SCSI commands can be passed unchanged except for
	 * the padding on the end. The few which require munging
	 * are not used internally. Mode select/sense(6) could be
	 * converted to the 10-byte form but it's not worth the
	 * effort. Read/write(6) are easy.
	 */
	switch(r->cmd[0]){
	case 0x08:			/* read */
	case 0x0A:			/* write */
		cmdp = cmd10;
		memset(cmdp, 0, sizeof(cmd10));
		cmdp[0] = r->cmd[0]|0x20;
		cmdp[1] = r->cmd[1] & 0xE0;
		cmdp[5] = r->cmd[3];
		cmdp[4] = r->cmd[2];
		cmdp[3] = r->cmd[1] & 0x0F;
		cmdp[8] = r->cmd[4];
		clen = sizeof(cmd10);
		break;

	default:
		cmdp = r->cmd;
		clen = r->clen;
		break;
	}

	qlock(drive);
	drive->write = r->write;
	drive->data = r->data;
	drive->dlen = r->dlen;
	drive->limit = r->data;

	drive->status = 0;
	drive->error = 0;
	if(drive->pkt)
		status = atapktio(drive, cmdp, clen);
	else
		status = atagenio(drive, cmdp, clen);
	if(status == SDok){
		atasetsense(drive, SDok, 0, 0, 0);
		if(drive->data){
			p = r->data;
			r->rlen = drive->data - p;
		}
		else
			r->rlen = 0;
	}
	else if(status == SDcheck && !(r->flags & SDnosense)){
		drive->write = 0;
		memset(cmd10, 0, sizeof(cmd10));
		cmd10[0] = 0x03;
		cmd10[1] = r->lun<<5;
		cmd10[4] = sizeof(r->sense)-1;
		drive->data = r->sense;
		drive->dlen = sizeof(r->sense)-1;
		drive->limit = r->sense;
		drive->status = 0;
		drive->error = 0;
		if(drive->pkt)
			reqstatus = atapktio(drive, cmd10, 6);
		else
			reqstatus = atagenio(drive, cmd10, 6);
		if(reqstatus == SDok){
			r->flags |= SDvalidsense;
			atasetsense(drive, SDok, 0, 0, 0);
		}
	}
	qunlock(drive);
	r->status = status;
	if(status != SDok)
		return status;

	/*
	 * Fix up any results.
	 * Many ATAPI CD-ROMs ignore the LUN field completely and
	 * return valid INQUIRY data. Patch the response to indicate
	 * 'logical unit not supported' if the LUN is non-zero.
	 */
	switch(cmdp[0]){
	case 0x12:			/* inquiry */
		if((p = r->data) == nil)
			break;
		if((cmdp[1]>>5) && (!drive->pkt || (p[0] & 0x1F) == 0x05))
			p[0] = 0x7F;
		/*FALLTHROUGH*/
	default:
		break;
	}

	return SDok;
}

static void
atainterrupt(Ureg*, void* arg)
{
	Ctlr *ctlr;
	Drive *drive;
	int cmdport, len, status;

	ctlr = arg;
	
	ilock(ctlr);
	if(inb(ctlr->ctlport+As) & Bsy){
		iunlock(ctlr);
		return;
	}
	cmdport = ctlr->cmdport;
	status = inb(cmdport+Status);
	if((drive = ctlr->curdrive) == nil){
		iunlock(ctlr);
		return;
	}

	if(status & Err)
		drive->error = inb(cmdport+Error);
	else switch(drive->command){
	default:
		drive->error = Abrt;
		break;

	case Crs:
	case Crsm:
		if(!(status & Drq)){
			drive->error = Abrt;
			break;
		}
		len = drive->block;
		if(drive->data+len > drive->limit)
			len = drive->limit-drive->data;
		inss(cmdport+Data, drive->data, len/2);
		drive->data += len;
		if(drive->data >= drive->limit)
			ctlr->done = 1;
		break;

	case Cws:
	case Cwsm:
		len = drive->block;
		if(drive->data+len > drive->limit)
			len = drive->limit-drive->data;
		drive->data += len;
		if(drive->data >= drive->limit){
			ctlr->done = 1;
			break;
		}
		if(!(status & Drq)){
			drive->error = Abrt;
			break;
		}
		len = drive->block;
		if(drive->data+len > drive->limit)
			len = drive->limit-drive->data;
		outss(cmdport+Data, drive->data, len/2);
		break;

	case Cpkt:
		atapktinterrupt(drive);
		break;

	case Crd:
	case Cwd:
		panic("atadmainterrupt");
		break;
	}
	iunlock(ctlr);

	if(drive->error){
		status |= Err;
		ctlr->done = 1;
	}

	if(ctlr->done){
		ctlr->curdrive = nil;
		drive->status = status;
		wakeup(ctlr);
	}
}

static SDev*
atapnp(void)
{
	Ctlr *ctlr;
	Pcidev *p;
	int channel, ispc87415, pi;
	SDev *legacy[2], *sdev, *head, *tail;

	legacy[0] = legacy[1] = head = tail = nil;
	if(sdev = ataprobe(0x1F0, 0x3F4, IrqATA0)){
		head = tail = sdev;
		legacy[0] = sdev;
	}
	if(sdev = ataprobe(0x170, 0x374, IrqATA1)){
		if(head != nil)
			tail->next = sdev;
		else
			head = sdev;
		tail = sdev;
		legacy[1] = sdev;
	}

	p = nil;
	while(p = pcimatch(p, 0, 0)){
		/*
		 * Look for devices with the correct class and sub-class
		 * code and known device and vendor ID; add native-mode
		 * channels to the list to be probed, save info for the
		 * compatibility mode channels.
		 * Note that the legacy devices should not be considered
		 * PCI devices by the interrupt controller.
		 * For both native and legacy, save info for busmastering
		 * if capable.
		 * Promise Ultra ATA/66 (PDC20262) appears to
		 * 1) give a sub-class of 'other mass storage controller'
		 *    instead of 'IDE controller', regardless of whether it's
		 *    the only controller or not;
		 * 2) put 0 in the programming interface byte (probably
		 *    as a consequence of 1) above).
		 */
		if(p->ccrb != 0x01 || (p->ccru != 0x01 && p->ccru != 0x80))
			continue;
		pi = p->ccrp;
		ispc87415 = 0;

		switch((p->did<<16)|p->vid){
		default:
			continue;

		case (0x0002<<16)|0x100B:	/* NS PC87415 */
			/*
			 * Disable interrupts on both channels until
			 * after they are probed for drives.
			 * This must be called before interrupts are
			 * enabled because the IRQ may be shared.
			 */
			ispc87415 = 1;
			pcicfgw32(p, 0x40, 0x00000300);
			break;

		case (0x4D38<<16)|0x105A:	/* Promise PDC20262 */
			pi = 0x85;
			/*FALLTHROUGH*/
		case (0x1230<<16)|0x8086:	/* 82371FB (PIIX) */
		case (0x7010<<16)|0x8086:	/* 82371SB (PIIX3) */
		case (0x7111<<16)|0x8086:	/* 82371[AE]B (PIIX4[E]) */
		case (0x0646<<16)|0x1095:	/* CMD 646 */
		case (0x0571<<16)|0x1106:	/* VIA 82C686 */
			break;
		}

		for(channel = 0; channel < 2; channel++){
			if(pi & (1<<(2*channel))){
				sdev = ataprobe(p->mem[0+2*channel].bar & ~0x01,
						p->mem[1+2*channel].bar & ~0x01,
						p->intl);
				if(sdev == nil)
					continue;

				ctlr = sdev->ctlr;
				if(ispc87415)
					ctlr->ienable = pc87415ienable;

				if(head != nil)
					tail->next = sdev;
				else
					head = sdev;
				tail = sdev;
				ctlr->pcidev = p;
			}
		}
	}

	return head;
}

static SDev*
atalegacy(int port, int irq)
{
	return ataprobe(port, port+0x204, irq);
}

static SDev*
ataid(SDev* sdev)
{
	int i;
	Ctlr *ctlr;

	/*
	 * Legacy controllers are always 'C' and 'D' and if
	 * they exist and have drives will be first in the list.
	 * If there are no active legacy controllers, native
	 * controllers start at 'C'.
	 */
	ctlr = sdev->ctlr;
	if(ctlr->cmdport == 0x1F0 || ctlr->cmdport == 0x170)
		i = 2;
	else
		i = 0;
	while(sdev){
		if(sdev->ifc != &sdataifc)
			continue;
		ctlr = sdev->ctlr;
		if(ctlr->cmdport == 0x1F0)
			sdev->idno = 'C';
		else if(ctlr->cmdport == 0x170)
			sdev->idno = 'D';
		else{
			sdev->idno = 'C'+i;
			i++;
		}
		sdev = sdev->next;
	}

	return nil;
}

static int
ataenable(SDev* sdev)
{
	Ctlr *ctlr;

	ctlr = sdev->ctlr;

	setvec(ctlr->irq+VectorPIC, atainterrupt, ctlr);
	outb(ctlr->ctlport+Dc, 0);
	if(ctlr->ienable)
		ctlr->ienable(ctlr);

	return 1;
}


SDifc sdataifc = {
	"ata",				/* name */

	atapnp,				/* pnp */
	atalegacy,			/* legacy */
	ataid,				/* id */
	ataenable,			/* enable */
	nil,				/* disable */

	scsiverify,			/* verify */
	scsionline,			/* online */
	atario,				/* rio */
	nil,				/* rctl */
	nil,				/* wctl */

	scsibio,			/* bio */
};
