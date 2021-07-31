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
	DbgCONFIG	= 0x0001,	/* detected drive config info */
	DbgIDENTIFY	= 0x0002,	/* detected drive identify info */
	DbgSTATE	= 0x0004,	/* dump state on panic */
	DbgPROBE	= 0x0008,	/* trace device probing */
	DbgDEBUG	= 0x0080,	/* the current problem... */
	DbgINL		= 0x0100,	/* That Inil20+ message we hate */
	Dbg48BIT	= 0x0200,	/* 48-bit LBA */
	DbgBsy		= 0x0400,	/* interrupt but Bsy (shared IRQ) */
};
#define DEBUG		(DbgDEBUG|DbgCONFIG)

enum {					/* I/O ports */
	Data		= 0,
	Error		= 1,		/* (read) */
	Features	= 1,		/* (write) */
	Count		= 2,		/* sector count<7-0>, sector count<15-8> */
	Ir		= 2,		/* interrupt reason (PACKET) */
	Sector		= 3,		/* sector number */
	Lbalo		= 3,		/* LBA<7-0>, LBA<31-24> */
	Cyllo		= 4,		/* cylinder low */
	Bytelo		= 4,		/* byte count low (PACKET) */
	Lbamid		= 4,		/* LBA<15-8>, LBA<39-32> */
	Cylhi		= 5,		/* cylinder high */
	Bytehi		= 5,		/* byte count hi (PACKET) */
	Lbahi		= 5,		/* LBA<23-16>, LBA<47-40> */
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
	Crs48		= 0x24,		/* Read Sectors Ext */
	Crd48		= 0x25,		/* Read w/ DMA Ext */
	Crdq48		= 0x26,		/* Read w/ DMA Queued Ext */
	Crsm48		= 0x29,		/* Read Multiple Ext */
	Cws		= 0x30,		/* Write Sectors */
	Cws48		= 0x34,		/* Write Sectors Ext */
	Cwd48		= 0x35,		/* Write w/ DMA Ext */
	Cwdq48		= 0x36,		/* Write w/ DMA Queued Ext */
	Cwsm48		= 0x39,		/* Write Multiple Ext */
	Cedd		= 0x90,		/* Execute Device Diagnostics */
	Cpkt		= 0xA0,		/* Packet */
	Cidpkt		= 0xA1,		/* Identify Packet Device */
	Crsm		= 0xC4,		/* Read Multiple */
	Cwsm		= 0xC5,		/* Write Multiple */
	Csm		= 0xC6,		/* Set Multiple */
	Crdq		= 0xC7,		/* Read DMA queued */
	Crd		= 0xC8,		/* Read DMA */
	Cwd		= 0xCA,		/* Write DMA */
	Cwdq		= 0xCC,		/* Write DMA queued */
	Cstandby	= 0xE2,		/* Standby */
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
	Ilba		= 60,		/* LBA size */
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
	Icsfs		= 82,		/* command set/feature supported */
	Icsfe		= 85,		/* command set/feature enabled */
	Iudma		= 88,		/* ultra DMA mode */
	Ierase		= 89,		/* time for security erase */
	Ieerase		= 90,		/* time for enhanced security erase */
	Ipower		= 91,		/* current advanced power management */
	Ilba48		= 100,		/* 48-bit LBA size (64 bits in 100-103) */
	Irmsn		= 127,		/* removable status notification */
	Isecstat	= 128,		/* security status */
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
	int	tbdf;

	Pcidev*	pcidev;
	void	(*ienable)(Ctlr*);
	SDev*	sdev;

	Drive*	drive[2];

	Prd*	prdt;			/* physical region descriptor table */

//	QLock;				/* current command */
	Drive*	curdrive;
	int	command;		/* last command issued (debugging) */
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
	vlong	sectors;		/* total */
	int	secsize;		/* sector size */

//	int	dma;			/* DMA R/W possible */
//	int	dmactl;
//	int	rwm;			/* read/write multiple possible */
//	int	rwmctl;

	int	pkt;			/* PACKET device, length of pktcmd */
	uchar	pktcmd[16];
//	int	pktdma;			/* this PACKET command using dma */

	uchar	sense[18];
	uchar	inquiry[48];

//	QLock;				/* drive access */
	int	command;		/* current command */
	int	write;
	uchar*	data;
	int	dlen;
	uchar*	limit;
	int	count;			/* sectors */
	int	block;			/* R/W bytes per block */
	int	status;
	int	error;
	int	flags;			/* internal flags */
} Drive;

enum {					/* internal flags */
	Lba48		= 0x1,		/* LBA48 mode */
	Lba48always	= 0x2,		/* ... */
};

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

static int
atadebug(int cmdport, int ctlport, char* fmt, ...)
{
	int i, n;
	va_list arg;
	char buf[PRINTSIZE];

	if(!(DEBUG & DbgPROBE)){
		USED(cmdport, ctlport, fmt);
		return 0;
	}

	va_start(arg, fmt);
	n = vseprint(buf, buf+sizeof(buf), fmt, arg) - buf;
	va_end(arg);

	if(cmdport){
		if(buf[n-1] == '\n')
			n--;
		n += snprint(buf+n, PRINTSIZE-n, " ataregs 0x%uX:",
			cmdport);
		for(i = Features; i < Command; i++)
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
		 * register set if necessary before testing for ready.
		 * Always run through the loop at least once so it
		 * can be used as a test for !Bsy.
		 */
		as = inb(ctlport+As);
		if(as & reset){
			/* nothing to do */;
		}
		else if(dev){
			outb(cmdport+Dh, dev);
			dev = 0;
		}
		else if(ready == 0 || (as & ready)){
			atadebug(0, 0, "ataready: %d 0x%2.2uX\n", micro, as);
			return as;
		}

		if(micro-- <= 0){
			atadebug(0, 0, "ataready: %d 0x%2.2uX\n", micro, as);
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
		cmdset = drive->info[Icsfe+i];
		if(cmdset == 0 || cmdset == 0xFFFF)
			return 0;
		return cmdset & x;
	}

	return 0;
}

/*
static int
atasf(int cmdport, int ctlport, int dev, uchar* command)
{
	int as, i;

	if(ataready(cmdport, ctlport, dev, Bsy|Drq, Drdy, 108*1000) < 0)
		return -1;

	for(i = Features; i < Dh; i++)
		outb(cmdport+i, command[i]);
	outb(cmdport+Command, Csf);
	microdelay(100);
	as = ataready(cmdport, ctlport, 0, Bsy, Drdy|Df|Err, 109*1000);
	if(as < 0 || (as & (Df|Err)))
		return -1;
	return 0;
}
 */

static int
ataidentify(int cmdport, int ctlport, int dev, int pkt, void* info)
{
	int as, command, drdy;

	if(pkt){
		command = Cidpkt;
		drdy = 0;
	}
	else{
		command = Cid;
		drdy = Drdy;
	}
	as = ataready(cmdport, ctlport, dev, Bsy|Drq, drdy, 103*1000);
	if(as < 0)
		return as;
	outb(cmdport+Command, command);
	microdelay(1);

	as = ataready(cmdport, ctlport, 0, Bsy, Drq|Err, 400*1000);
	if(as < 0)
		return -1;
	if(as & Err)
		return as;

	memset(info, 0, 512);
	inss(cmdport+Data, info, 256);
	inb(cmdport+Status);

	if(DEBUG & DbgIDENTIFY){
		int i;
		ushort *sp;

		sp = (ushort*)info;
		for(i = 0; i < 256; i++){
			if(i && (i%16) == 0)
				print("\n");
			print(" %4.4uX ", *sp);
			sp++;
		}
		print("\n");
	}

	return 0;
}

static Drive*
atadrive(int cmdport, int ctlport, int dev)
{
	Drive *drive;
	int as, i, pkt;
	uchar buf[512], *p;
	ushort iconfig, *sp;

	atadebug(0, 0, "identify: port 0x%uX dev 0x%2.2uX\n", cmdport, dev);
	pkt = 1;
retry:
	as = ataidentify(cmdport, ctlport, dev, pkt, buf);
	if(as < 0)
		return nil;
	if(as & Err){
		if(pkt == 0)
			return nil;
		pkt = 0;
		goto retry;
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

	/*
	 * Beware the CompactFlash Association feature set.
	 * Now, why this value in Iconfig just walks all over the bit
	 * definitions used in the other parts of the ATA/ATAPI standards
	 * is a mystery and a sign of true stupidity on someone's part.
	 * Anyway, the standard says if this value is 0x848A then it's
	 * CompactFlash and it's NOT a packet device.
	 */
	iconfig = drive->info[Iconfig];
	if(iconfig != 0x848A && (iconfig & 0xC000) == 0x8000){
		if(iconfig & 0x01)
			drive->pkt = 16;
		else
			drive->pkt = 12;
	}
	else{
		if(drive->info[Ivalid] & 0x0001){
			drive->c = drive->info[Iccyl];
			drive->h = drive->info[Ichead];
			drive->s = drive->info[Icsec];
		}
		else{
			drive->c = drive->info[Ilcyl];
			drive->h = drive->info[Ilhead];
			drive->s = drive->info[Ilsec];
		}
		if(drive->info[Icapabilities] & 0x0200){
			if(drive->info[Icsfs+1] & 0x0400){
				drive->sectors = drive->info[Ilba48]
					|(drive->info[Ilba48+1]<<16)
					|((vlong)drive->info[Ilba48+2]<<32);
				drive->flags |= Lba48;
			}
			else{
				drive->sectors = (drive->info[Ilba+1]<<16)
					 |drive->info[Ilba];
			}
			drive->dev |= Lba;
		}
		else
			drive->sectors = drive->c*drive->h*drive->s;
	//	atarwmmode(drive, cmdport, ctlport, dev);
	}
//	atadmamode(drive);	

	if(DEBUG & DbgCONFIG){
		print("dev %2.2uX port %uX config %4.4uX capabilities %4.4uX",
			dev, cmdport, iconfig, drive->info[Icapabilities]);
		print(" mwdma %4.4uX", drive->info[Imwdma]);
		if(drive->info[Ivalid] & 0x04)
			print(" udma %4.4uX", drive->info[Iudma]);
//		print(" dma %8.8uX rwm %ud", drive->dma, drive->rwm);
		if(drive->flags&Lba48)
			print("\tLLBA sectors %lld", drive->sectors);
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

//	if(ioalloc(cmdport, 8, 0, "atacmd") < 0)
//		return nil;
//	if(ioalloc(ctlport+As, 1, 0, "atactl") < 0){
//		iofree(cmdport);
//		return nil;
//	}

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
		microdelay(1);
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
			//	iofree(cmdport);
			//	iofree(ctlport+As);
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
	if(ataready(cmdport, ctlport, dev, Bsy|Drq, 0, 105*1000) < 0){
		/*
		 * There's something there, but it didn't come up clean,
		 * so try hitting it with a big stick. The timing here is
		 * wrong but this is a last-ditch effort and it sometimes
		 * gets some marginal hardware back online.
		 */
		atasrst(ctlport);
		if(ataready(cmdport, ctlport, dev, Bsy|Drq, 0, 106*1000) < 0)
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
	if((drive = atadrive(cmdport, ctlport, dev)) == nil)
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
			drive = atadrive(cmdport, ctlport, Dev1);
			if(drive != nil){
				drive->ctlr = ctlr;
				ctlr->drive[1] = drive;
			}
			else{
				outb(cmdport+Dh, Dev0);
				microdelay(1);
			}
		}
	}
	else
		ctlr->drive[1] = drive;

	ctlr->cmdport = cmdport;
	ctlr->ctlport = ctlport;
	ctlr->irq = irq;
	ctlr->tbdf = BUSUNKNOWN;
	ctlr->command = Cedd;		/* debugging */

	sdev->ifc = &sdataifc;
	sdev->ctlr = ctlr;
	sdev->nunit = 2;
	ctlr->sdev = sdev;

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
	ctlr->command = Cnop;		/* debugging */
	outb(cmdport+Command, Cnop);

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

static void
ataabort(Drive* drive, int dolock)
{
	/*
	 * If NOP is available (packet commands) use it otherwise
	 * must try a software reset.
	 */
	if(dolock)
		ilock(drive->ctlr);
	if(atacsfenabled(drive, 0x0000000000004000LL))
		atanop(drive, 0);
	else{
		atasrst(drive->ctlr->ctlport);
		drive->error |= Abrt;
	}
	if(dolock)
		iunlock(drive->ctlr);
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
	//	if(drive->pktdma)
	//		atadmainterrupt(drive, drive->dlen);
	//	else
			ctlr->done = 1;
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

	as = ataready(cmdport, ctlport, drive->dev, Bsy|Drq, 0, 107*1000);
	if(as < 0 || (as&Chk)){
		qunlock(ctlr);
		return -1;
	}

	ilock(ctlr);
//	if(drive->dlen && drive->dmactl && !atadmasetup(drive, drive->dlen))
//		drive->pktdma = Dma;
//	else
//		drive->pktdma = 0;

	outb(cmdport+Features, 0/*drive->pktdma*/);
	outb(cmdport+Count, 0);
	outb(cmdport+Sector, 0);
	len = 16*drive->secsize;
	outb(cmdport+Bytelo, len);
	outb(cmdport+Bytehi, len>>8);
	outb(cmdport+Dh, drive->dev);
	ctlr->done = 0;
	ctlr->curdrive = drive;
	ctlr->command = Cpkt;		/* debugging */
//	if(drive->pktdma)
//		atadmastart(ctlr, drive->write);
	outb(cmdport+Command, Cpkt);

	if((drive->info[Iconfig] & 0x0060) != 0x0020){
		microdelay(1);
		as = ataready(cmdport, ctlport, 0, Bsy, Drq|Chk, 4*1000);
		if(as < 0 || (as & (Bsy|Chk))){
			drive->status = as<0 ? 0 : as;
			ctlr->curdrive = nil;
			ctlr->done = 1;
			r = SDtimeout;
		}else
			atapktinterrupt(drive);
	}
	iunlock(ctlr);

	sleep(ctlr, atapktiodone, ctlr);

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

static uchar cmd48[256] = {
	[Crs]	Crs48,
	[Crd]	Crd48,
	[Crdq]	Crdq48,
	[Crsm]	Crsm48,
	[Cws]	Cws48,
	[Cwd]	Cwd48,
	[Cwdq]	Cwdq48,
	[Cwsm]	Cwsm48,
};

static int
atageniostart(Drive* drive, vlong lba)
{
	Ctlr *ctlr;
	uchar cmd;
	int as, c, cmdport, ctlport, h, len, s, use48;

	use48 = 0;
	if((drive->flags&Lba48always) || (lba>>28) || drive->count > 256){
		if(!(drive->flags & Lba48))
			return -1;
		use48 = 1;
		c = h = s = 0;
	}else if(drive->dev & Lba){
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
	if(ataready(cmdport, ctlport, drive->dev, Bsy|Drq, 0, 101*1000) < 0)
		return -1;

	ilock(ctlr);

	drive->block = drive->secsize;
	if(drive->write)
		drive->command = Cws;
	else
		drive->command = Crs;

	drive->limit = drive->data + drive->count*drive->secsize;
	cmd = drive->command;
	if(use48){
		outb(cmdport+Count, (drive->count>>8) & 0xFF);
		outb(cmdport+Count, drive->count & 0XFF);
		outb(cmdport+Lbalo, (lba>>24) & 0xFF);
		outb(cmdport+Lbalo, lba & 0xFF);
		outb(cmdport+Lbamid, (lba>>32) & 0xFF);
		outb(cmdport+Lbamid, (lba>>8) & 0xFF);
		outb(cmdport+Lbahi, (lba>>40) & 0xFF);
		outb(cmdport+Lbahi, (lba>>16) & 0xFF);
		outb(cmdport+Dh, drive->dev|Lba);
		cmd = cmd48[cmd];

		if(DEBUG & Dbg48BIT)
			print("using 48-bit commands\n");
	}else{
		outb(cmdport+Count, drive->count);
		outb(cmdport+Sector, s);
		outb(cmdport+Cyllo, c);
		outb(cmdport+Cylhi, c>>8);
		outb(cmdport+Dh, drive->dev|h);
	}
	ctlr->done = 0;
	ctlr->curdrive = drive;
	ctlr->command = drive->command;	/* debugging */
	outb(cmdport+Command, cmd);

	switch(drive->command){
	case Cws:
	case Cwsm:
		microdelay(1);
		as = ataready(cmdport, ctlport, 0, Bsy, Drq|Err, 1000);
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
	//	atadmastart(ctlr, drive->write);
		break;
	}
	iunlock(ctlr);

	return 0;
}

static int
atagenioretry(Drive* drive)
{
	return atasetsense(drive, SDcheck, 4, 8, drive->error);
}

static int
atagenio(Drive* drive, uchar* cmd, int)
{
	uchar *p;
	Ctlr *ctlr;
	int count, max;
	vlong lba, len;

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

	case 0x9E:			/* long read capacity */
		if((cmd[1] & 0x01) || cmd[2] || cmd[3])
			return atasetsense(drive, SDcheck, 0x05, 0x24, 0);
		if(drive->data == nil || drive->dlen < 8)
			return atasetsense(drive, SDcheck, 0x05, 0x20, 1);
		/*
		 * Read capacity returns the LBA of the last sector.
		 */
		len = drive->sectors-1;
		p = drive->data;
		*p++ = len>>56;
		*p++ = len>>48;
		*p++ = len>>40;
		*p++ = len>>32;
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
		max = (drive->flags&Lba48) ? 65536 : 256;
		if(count > max)
			drive->count = max;
		else
			drive->count = count;
		if(atageniostart(drive, lba)){
			ilock(ctlr);
			atanop(drive, 0);
			iunlock(ctlr);
			qunlock(ctlr);
			return atagenioretry(drive);
		}

		tsleep(ctlr, atageniodone, ctlr, 10*1000);
		if(!ctlr->done){
			/*
			 * What should the above timeout be? In
			 * standby and sleep modes it could take as
			 * long as 30 seconds for a drive to respond.
			 * Very hard to get out of this cleanly.
			 */
		//	atadumpstate(drive, cmd, lba, count);
			ataabort(drive, 1);
			return atagenioretry(drive);
		}

		if(drive->status & Err){
			qunlock(ctlr);
			return atasetsense(drive, SDcheck, 4, 8, drive->error);
		}
		count -= drive->count;
		lba += drive->count;
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
		if(DEBUG & DbgBsy)
			print("IBsy+");
		return;
	}
	cmdport = ctlr->cmdport;
	status = inb(cmdport+Status);
	if((drive = ctlr->curdrive) == nil){
		iunlock(ctlr);
		if((DEBUG & DbgINL) && ctlr->command != Cedd)
			print("Inil%2.2uX+", ctlr->command);
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
	//	atadmainterrupt(drive, drive->count*drive->secsize);
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
	int channel, ispc87415, pi, r;
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
		 * Sub-class code 0x04 is 'RAID controller', e.g. VIA VT8237.
		 */
		if(p->ccrb != 0x01)
			continue;
		if(p->ccru != 0x01 && p->ccru != 0x04 && p->ccru != 0x80)
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
		case (0x1000<<16)|0x1042:	/* PC-Tech RZ1000 */
			/*
			 * Turn off prefetch. Overkill, but cheap.
			 */
			r = pcicfgr32(p, 0x40);
			r &= ~0x2000;
			pcicfgw32(p, 0x40, r);
			break;
		case (0x4D38<<16)|0x105A:	/* Promise PDC20262 */
		case (0x4D30<<16)|0x105A:	/* Promise PDC202xx */
		case (0x4D68<<16)|0x105A:	/* Promise PDC20268 */
		case (0x4D69<<16)|0x105A:	/* Promise Ultra/133 TX2 */
		case (0x3373<<16)|0x105A:	/* Promise 20378 RAID */
		case (0x3149<<16)|0x1106:	/* VIA VT8237 SATA/RAID */
		case (0x4379<<16)|0x1002:	/* ATI 4379 SATA*/
		case (0x3112<<16)|0x1095:	/* SiL 3112 SATA (DMA busted?) */
		case (0x3114<<16)|0x1095:	/* SiL 3114 SATA/RAID */
			pi = 0x85;
			break;
		case (0x0004<<16)|0x1103:	/* HighPoint HPT-370 */
			pi = 0x85;
			/*
			 * Turn off fast interrupt prediction.
			 */
			if((r = pcicfgr8(p, 0x51)) & 0x80)
				pcicfgw8(p, 0x51, r & ~0x80);
			if((r = pcicfgr8(p, 0x55)) & 0x80)
				pcicfgw8(p, 0x55, r & ~0x80);
			break;
		case (0x0640<<16)|0x1095:	/* CMD 640B */
			/*
			 * Bugfix code here...
			 */
			break;
		case (0x7441<<16)|0x1022:	/* AMD 768 */
			/*
			 * Set:
			 *	0x41	prefetch, postwrite;
			 *	0x43	FIFO configuration 1/2 and 1/2;
			 *	0x44	status register read retry;
			 *	0x46	DMA read and end of sector flush.
			 */
			r = pcicfgr8(p, 0x41);
			pcicfgw8(p, 0x41, r|0xF0);
			r = pcicfgr8(p, 0x43);
			pcicfgw8(p, 0x43, (r & 0x90)|0x2A);
			r = pcicfgr8(p, 0x44);
			pcicfgw8(p, 0x44, r|0x08);
			r = pcicfgr8(p, 0x46);
			pcicfgw8(p, 0x46, (r & 0x0C)|0xF0);
			/*FALLTHROUGH*/
		case (0x7469<<16)|0x1022:	/* AMD 3111 */
			/*
			 * This can probably be lumped in with the 768 above.
			 */
			/*FALLTHROUGH*/
		case (0x209A<<16)|0x1022:	/* AMD CS5536 */
		case (0x01BC<<16)|0x10DE:	/* nVidia nForce1 */
		case (0x0065<<16)|0x10DE:	/* nVidia nForce2 */
		case (0x0085<<16)|0x10DE:	/* nVidia nForce2 MCP */
		case (0x00D5<<16)|0x10DE:	/* nVidia nForce3 */
		case (0x00E5<<16)|0x10DE:	/* nVidia nForce3 Pro */
		case (0x0035<<16)|0x10DE:	/* nVidia nForce3 MCP */
		case (0x0053<<16)|0x10DE:	/* nVidia nForce4 */
		case (0x0054<<16)|0x10DE:	/* nVidia nForce4 SATA */
		case (0x0055<<16)|0x10DE:	/* nVidia nForce4 SATA */
			/*
			 * Ditto, although it may have a different base
			 * address for the registers (0x50?).
			 */
			break;
		case (0x0646<<16)|0x1095:	/* CMD 646 */
		case (0x0571<<16)|0x1106:	/* VIA 82C686 */
		case (0x0211<<16)|0x1166:	/* ServerWorks IB6566 */
		case (0x1230<<16)|0x8086:	/* 82371FB (PIIX) */
		case (0x7010<<16)|0x8086:	/* 82371SB (PIIX3) */
		case (0x7111<<16)|0x8086:	/* 82371[AE]B (PIIX4[E]) */
		case (0x2411<<16)|0x8086:	/* 82801AA (ICH) */
		case (0x2421<<16)|0x8086:	/* 82801AB (ICH0) */
		case (0x244A<<16)|0x8086:	/* 82801BA (ICH2, Mobile) */
		case (0x244B<<16)|0x8086:	/* 82801BA (ICH2, High-End) */
		case (0x248A<<16)|0x8086:	/* 82801CA (ICH3, Mobile) */
		case (0x248B<<16)|0x8086:	/* 82801CA (ICH3, High-End) */
		case (0x24CA<<16)|0x8086:	/* 82801DBM (ICH4, Mobile) */
		case (0x24CB<<16)|0x8086:	/* 82801DB (ICH4, High-End) */
		case (0x24DB<<16)|0x8086:	/* 82801EB (ICH5) */
		case (0x266F<<16)|0x8086:	/* 82801FB (ICH6) */
		case (0x27C4<<16)|0x8086:	/* 82801GBM SATA (ICH7) */
		case (0x27C5<<16)|0x8086:	/* 82801GBM SATA AHCI (ICH7) */
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
				ctlr->tbdf = p->tbdf;
			}
			else if((sdev = legacy[channel]) == nil)
				continue;
			else
				ctlr = sdev->ctlr;

			ctlr->pcidev = p;
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
	if(sdev == nil)
		return nil;
	ctlr = sdev->ctlr;
	if(ctlr->cmdport == 0x1F0 || ctlr->cmdport == 0x170)
		i = 2;
	else
		i = 0;
	while(sdev){
		if(sdev->ifc == &sdataifc){
			ctlr = sdev->ctlr;
			if(ctlr->cmdport == 0x1F0)
				sdev->idno = 'C';
			else if(ctlr->cmdport == 0x170)
				sdev->idno = 'D';
			else{
				sdev->idno = 'C'+i;
				i++;
			}
		//	snprint(sdev->name, NAMELEN, "sd%c", sdev->idno);
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
	nil,			/* rctl */
	nil,			/* wctl */

	scsibio,			/* bio */
};
