#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "ureg.h"
#include "../port/error.h"

#include "../port/sd.h"

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
#define DEBUG		(DbgDEBUG|DbgSTATE)

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

enum {					/* internal flags */
	Lba48		= 0x1,		/* LBA48 mode */
	Lba48always	= 0x2,		/* ... */
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
	Hob		= 0x80,		/* High Order Bit [sic] */
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
	Icfapwr		= 160,		/* CFA power mode */
	Imediaserial	= 176,		/* current media serial number */
	Icksum		= 255,		/* checksum */
};

enum {					/* bit masks for config identify info */
	Mpktsz		= 0x0003,	/* packet command size */
	Mincomplete	= 0x0004,	/* incomplete information */
	Mdrq		= 0x0060,	/* DRQ type */
	Mrmdev		= 0x0080,	/* device is removable */
	Mtype		= 0x1F00,	/* device type */
	Mproto		= 0x8000,	/* command protocol */
};

enum {					/* bit masks for capabilities identify info */
	Mdma		= 0x0100,	/* DMA supported */
	Mlba		= 0x0200,	/* LBA supported */
	Mnoiordy	= 0x0400,	/* IORDY may be disabled */
	Miordy		= 0x0800,	/* IORDY supported */
	Msoftrst	= 0x1000,	/* needs soft reset when Bsy */
	Mstdby		= 0x2000,	/* standby supported */
	Mqueueing	= 0x4000,	/* queueing overlap supported */
	Midma		= 0x8000,	/* interleaved DMA supported */
};

enum {					/* bit masks for supported/enabled features */
	Msmart		= 0x0001,
	Msecurity	= 0x0002,
	Mrmmedia	= 0x0004,
	Mpwrmgmt	= 0x0008,
	Mpkt		= 0x0010,
	Mwcache		= 0x0020,
	Mlookahead	= 0x0040,
	Mrelirq		= 0x0080,
	Msvcirq		= 0x0100,
	Mreset		= 0x0200,
	Mprotected	= 0x0400,
	Mwbuf		= 0x1000,
	Mrbuf		= 0x2000,
	Mnop		= 0x4000,
	Mmicrocode	= 0x0001,
	Mqueued		= 0x0002,
	Mcfa		= 0x0004,
	Mapm		= 0x0008,
	Mnotify		= 0x0010,
	Mstandby	= 0x0020,
	Mspinup		= 0x0040,
	Mmaxsec		= 0x0100,
	Mautoacoustic	= 0x0200,
	Maddr48		= 0x0400,
	Mdevconfov	= 0x0800,
	Mflush		= 0x1000,
	Mflush48	= 0x2000,
	Msmarterror	= 0x0001,
	Msmartselftest	= 0x0002,
	Mmserial	= 0x0004,
	Mmpassthru	= 0x0008,
	Mlogging	= 0x0020,
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
	int	bmiba;			/* bus master interface base address */

	Pcidev*	pcidev;
	void	(*ienable)(Ctlr*);
	void	(*idisable)(Ctlr*);
	SDev*	sdev;

	Drive*	drive[2];

	Prd*	prdt;			/* physical region descriptor table */
	void*	prdtbase;

	QLock;				/* current command */
	Drive*	curdrive;
	int	command;		/* last command issued (debugging) */
	Rendez;
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

	int	dma;			/* DMA R/W possible */
	int	dmactl;
	int	rwm;			/* read/write multiple possible */
	int	rwmctl;

	int	pkt;			/* PACKET device, length of pktcmd */
	uchar	pktcmd[16];
	int	pktdma;			/* this PACKET command using dma */

	uchar	sense[18];
	uchar	inquiry[48];

	QLock;				/* drive access */
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

static void
atadumpstate(Drive* drive, uchar* cmd, vlong lba, int count)
{
	Prd *prd;
	Pcidev *p;
	Ctlr *ctlr;
	int i, bmiba;

	if(!(DEBUG & DbgSTATE)){
		USED(drive, cmd, lba, count);
		return;
	}

	ctlr = drive->ctlr;
	print("command %2.2uX\n", ctlr->command);
	print("data %8.8p limit %8.8p dlen %d status %uX error %uX\n",
		drive->data, drive->limit, drive->dlen,
		drive->status, drive->error);
	if(cmd != nil){
		print("lba %d -> %lld, count %d -> %d (%d)\n",
			(cmd[2]<<24)|(cmd[3]<<16)|(cmd[4]<<8)|cmd[5], lba,
			(cmd[7]<<8)|cmd[8], count, drive->count);
	}
	if(!(inb(ctlr->ctlport+As) & Bsy)){
		for(i = 1; i < 7; i++)
			print(" 0x%2.2uX", inb(ctlr->cmdport+i));
		print(" 0x%2.2uX\n", inb(ctlr->ctlport+As));
	}
	if(drive->command == Cwd || drive->command == Crd){
		bmiba = ctlr->bmiba;
		prd = ctlr->prdt;
		print("bmicx %2.2uX bmisx %2.2uX prdt %8.8p\n",
			inb(bmiba+Bmicx), inb(bmiba+Bmisx), prd);
		for(;;){
			print("pa 0x%8.8luX count %8.8uX\n",
				prd->pa, prd->count);
			if(prd->count & PrdEOT)
				break;
			prd++;
		}
	}
	if(ctlr->pcidev && ctlr->pcidev->vid == 0x8086){
		p = ctlr->pcidev;
		print("0x40: %4.4uX 0x42: %4.4uX",
			pcicfgr16(p, 0x40), pcicfgr16(p, 0x42));
		print("0x48: %2.2uX\n", pcicfgr8(p, 0x48));
		print("0x4A: %4.4uX\n", pcicfgr16(p, 0x4A));
	}
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
			/* nothing to do */
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

/*
static int
atacsf(Drive* drive, vlong csf, int supported)
{
	ushort *info;
	int cmdset, i, x;

	if(supported)
		info = &drive->info[Icsfs];
	else
		info = &drive->info[Icsfe];

	for(i = 0; i < 3; i++){
		x = (csf>>(16*i)) & 0xFFFF;
		if(x == 0)
			continue;
		cmdset = info[i];
		if(cmdset == 0 || cmdset == 0xFFFF)
			return 0;
		return cmdset & x;
	}

	return 0;
}
*/

static int
atadone(void* arg)
{
	return ((Ctlr*)arg)->done;
}

static int
atarwmmode(Drive* drive, int cmdport, int ctlport, int dev)
{
	int as, maxrwm, rwm;

	maxrwm = (drive->info[Imaxrwm] & 0xFF);
	if(maxrwm == 0)
		return 0;

	/*
	 * Sometimes drives come up with the current count set
	 * to 0; if so, set a suitable value, otherwise believe
	 * the value in Irwm if the 0x100 bit is set.
	 */
	if(drive->info[Irwm] & 0x100)
		rwm = (drive->info[Irwm] & 0xFF);
	else
		rwm = 0;
	if(rwm == 0)
		rwm = maxrwm;
	if(rwm > 16)
		rwm = 16;
	if(ataready(cmdport, ctlport, dev, Bsy|Drq, Drdy, 102*1000) < 0)
		return 0;
	outb(cmdport+Count, rwm);
	outb(cmdport+Command, Csm);
	microdelay(1);
	as = ataready(cmdport, ctlport, 0, Bsy, Drdy|Df|Err, 1000);
	inb(cmdport+Status);
	if(as < 0 || (as & (Df|Err)))
		return 0;

	drive->rwm = rwm;

	return rwm;
}

static int
atadmamode(Drive* drive)
{
	int dma;

	/*
	 * Check if any DMA mode enabled.
	 * Assumes the BIOS has picked and enabled the best.
	 * This is completely passive at the moment, no attempt is
	 * made to ensure the hardware is correctly set up.
	 */
	dma = drive->info[Imwdma] & 0x0707;
	drive->dma = (dma>>8) & dma;
	if(drive->dma == 0 && (drive->info[Ivalid] & 0x04)){
		dma = drive->info[Iudma] & 0x7F7F;
		drive->dma = (dma>>8) & dma;
		if(drive->dma)
			drive->dma |= 'U'<<16;
	}

	return dma;
}

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
			print(" %4.4uX", *sp);
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
		}else{
			drive->c = drive->info[Ilcyl];
			drive->h = drive->info[Ilhead];
			drive->s = drive->info[Ilsec];
		}
		if(drive->info[Icapabilities] & Mlba){
			if(drive->info[Icsfs+1] & Maddr48){
				drive->sectors = drive->info[Ilba48]
					| (drive->info[Ilba48+1]<<16)
					| ((vlong)drive->info[Ilba48+2]<<32);
				drive->flags |= Lba48;
			}else{
				drive->sectors = (drive->info[Ilba+1]<<16)
					 |drive->info[Ilba];
			}
			drive->dev |= Lba;
		}else
			drive->sectors = drive->c*drive->h*drive->s;
		atarwmmode(drive, cmdport, ctlport, dev);
	}
	atadmamode(drive);	

	if(DEBUG & DbgCONFIG){
		print("dev %2.2uX port %uX config %4.4uX capabilities %4.4uX",
			dev, cmdport, iconfig, drive->info[Icapabilities]);
		print(" mwdma %4.4uX", drive->info[Imwdma]);
		if(drive->info[Ivalid] & 0x04)
			print(" udma %4.4uX", drive->info[Iudma]);
		print(" dma %8.8uX rwm %ud\n", drive->dma, drive->rwm);
		if(drive->flags&Lba48)
			print("\tLLBA sectors %lld\n", drive->sectors);
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

	if(ioalloc(cmdport, 8, 0, "atacmd") < 0) {
		print("ataprobe: Cannot allocate %X\n", cmdport);
		return nil;
	}
	if(ioalloc(ctlport+As, 1, 0, "atactl") < 0){
		print("ataprobe: Cannot allocate %X\n", ctlport + As);
		iofree(cmdport);
		return nil;
	}

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
				iofree(cmdport);
				iofree(ctlport+As);
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
	memset(ctlr, 0, sizeof(Ctlr));
	if((sdev = malloc(sizeof(SDev))) == nil){
		free(ctlr);
		free(drive);
		goto release;
	}
	memset(sdev, 0, sizeof(SDev));
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

static void
ataclear(SDev *sdev)
{
	Ctlr* ctlr;

	ctlr = sdev->ctlr;
	iofree(ctlr->cmdport);
	iofree(ctlr->ctlport + As);

	if (ctlr->drive[0])
		free(ctlr->drive[0]);
	if (ctlr->drive[1])
		free(ctlr->drive[1]);
	if (sdev->name)
		free(sdev->name);
	if (sdev->unitflg)
		free(sdev->unitflg);
	if (sdev->unit)
		free(sdev->unit);
	free(ctlr);
	free(sdev);
}

static char *
atastat(SDev *sdev, char *p, char *e)
{
	Ctlr *ctlr = sdev->ctlr;

	return seprint(p, e, "%s ata port %X ctl %X irq %d\n", 
		    	       sdev->name, ctlr->cmdport, ctlr->ctlport, ctlr->irq);
}

static SDev*
ataprobew(DevConf *cf)
{
	if (cf->nports != 2)
		error(Ebadarg);

	return ataprobe(cf->ports[0].port, cf->ports[1].port, cf->intnum);
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
atastandby(Drive* drive, int period)
{
	Ctlr* ctlr;
	int cmdport, done;

	ctlr = drive->ctlr;
	drive->command = Cstandby;
	qlock(ctlr);

	cmdport = ctlr->cmdport;
	ilock(ctlr);
	outb(cmdport+Count, period);
	outb(cmdport+Dh, drive->dev);
	ctlr->done = 0;
	ctlr->curdrive = drive;
	ctlr->command = Cstandby;	/* debugging */
	outb(cmdport+Command, Cstandby);
	iunlock(ctlr);

	while(waserror())
		;
	tsleep(ctlr, atadone, ctlr, 30*1000);
	poperror();

	done = ctlr->done;
	qunlock(ctlr);

	if(!done || (drive->status & Err))
		return atasetsense(drive, SDcheck, 4, 8, drive->error);
	return SDok;
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
	if(drive->info[Icsfs] & Mnop)
		atanop(drive, 0);
	else{
		atasrst(drive->ctlr->ctlport);
		drive->error |= Abrt;
	}
	if(dolock)
		iunlock(drive->ctlr);
}

static int
atadmasetup(Drive* drive, int len)
{
	Prd *prd;
	ulong pa;
	Ctlr *ctlr;
	int bmiba, bmisx, count;

	pa = PCIWADDR(drive->data);
	if(pa & 0x03)
		return -1;
	ctlr = drive->ctlr;
	prd = ctlr->prdt;

	/*
	 * Sometimes drives identify themselves as being DMA capable
	 * although they are not on a busmastering controller.
	 */
	if(prd == nil){
		drive->dmactl = 0;
		print("disabling dma: not on a busmastering controller\n");
		return -1;
	}

	for(;;){
		prd->pa = pa;
		count = 64*1024 - (pa & 0xFFFF);
		if(count >= len){
			prd->count = PrdEOT|(len & 0xFFFF);
			break;
		}
		prd->count = count;
		len -= count;
		pa += count;
		prd++;
	}

	bmiba = ctlr->bmiba;
	outl(bmiba+Bmidtpx, PCIWADDR(ctlr->prdt));
	if(drive->write)
		outb(ctlr->bmiba+Bmicx, 0);
	else
		outb(ctlr->bmiba+Bmicx, Rwcon);
	bmisx = inb(bmiba+Bmisx);
	outb(bmiba+Bmisx, bmisx|Ideints|Idedmae);

	return 0;
}

static void
atadmastart(Ctlr* ctlr, int write)
{
	if(write)
		outb(ctlr->bmiba+Bmicx, Ssbm);
	else
		outb(ctlr->bmiba+Bmicx, Rwcon|Ssbm);
}

static int
atadmastop(Ctlr* ctlr)
{
	int bmiba;

	bmiba = ctlr->bmiba;
	outb(bmiba+Bmicx, inb(bmiba+Bmicx) & ~Ssbm);

	return inb(bmiba+Bmisx);
}

static void
atadmainterrupt(Drive* drive, int count)
{
	Ctlr* ctlr;
	int bmiba, bmisx;

	ctlr = drive->ctlr;
	bmiba = ctlr->bmiba;
	bmisx = inb(bmiba+Bmisx);
	switch(bmisx & (Ideints|Idedmae|Bmidea)){
	case Bmidea:
		/*
		 * Data transfer still in progress, nothing to do
		 * (this should never happen).
		 */
		return;

	case Ideints:
	case Ideints|Bmidea:
		/*
		 * Normal termination, tidy up.
		 */
		drive->data += count;
		break;

	default:
		/*
		 * What's left are error conditions (memory transfer
		 * problem) and the device is not done but the PRD is
		 * exhausted. For both cases must somehow tell the
		 * drive to abort.
		 */
		ataabort(drive, 0);
		break;
	}
	atadmastop(ctlr);
	ctlr->done = 1;
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
		if(drive->pktdma)
			atadmainterrupt(drive, drive->dlen);
		else
			ctlr->done = 1;
		break;
	}
}

static int
atapktio(Drive* drive, uchar* cmd, int clen)
{
	Ctlr *ctlr;
	int as, cmdport, ctlport, len, r, timeo;

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

	if(ataready(cmdport, ctlport, drive->dev, Bsy|Drq, 0, 107*1000) < 0){
		qunlock(ctlr);
		return -1;
	}

	ilock(ctlr);
	if(drive->dlen && drive->dmactl && !atadmasetup(drive, drive->dlen))
		drive->pktdma = Dma;
	else
		drive->pktdma = 0;

	outb(cmdport+Features, drive->pktdma);
	outb(cmdport+Count, 0);
	outb(cmdport+Sector, 0);
	len = 16*drive->secsize;
	outb(cmdport+Bytelo, len);
	outb(cmdport+Bytehi, len>>8);
	outb(cmdport+Dh, drive->dev);
	ctlr->done = 0;
	ctlr->curdrive = drive;
	ctlr->command = Cpkt;		/* debugging */
	if(drive->pktdma)
		atadmastart(ctlr, drive->write);
	outb(cmdport+Command, Cpkt);

	if((drive->info[Iconfig] & Mdrq) != 0x0020){
		microdelay(1);
		as = ataready(cmdport, ctlport, 0, Bsy, Drq|Chk, 4*1000);
		if(as < 0)
			r = SDtimeout;
		else if(as & Chk)
			r = SDcheck;
		else
			atapktinterrupt(drive);
	}
	iunlock(ctlr);

	while(waserror())
		;
	if(!drive->pktdma)
		sleep(ctlr, atadone, ctlr);
	else for(timeo = 0; !ctlr->done; timeo++){
		tsleep(ctlr, atadone, ctlr, 1000);
		if(ctlr->done)
			break;
		ilock(ctlr);
		atadmainterrupt(drive, 0);
		if(!drive->error && timeo > 10){
			ataabort(drive, 0);
			atadmastop(ctlr);
			drive->dmactl = 0;
			drive->error |= Abrt;
		}
		if(drive->error){
			drive->status |= Chk;
			ctlr->curdrive = nil;
		}
		iunlock(ctlr);
	}
	poperror();

	qunlock(ctlr);

	if(drive->status & Chk)
		r = SDcheck;

	return r;
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
	}else{
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
	if(drive->dmactl && !atadmasetup(drive, drive->count*drive->secsize)){
		if(drive->write)
			drive->command = Cwd;
		else
			drive->command = Crd;
	}
	else if(drive->rwmctl){
		drive->block = drive->rwm*drive->secsize;
		if(drive->write)
			drive->command = Cwsm;
		else
			drive->command = Crsm;
	}
	else{
		drive->block = drive->secsize;
		if(drive->write)
			drive->command = Cws;
		else
			drive->command = Crs;
	}
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
		atadmastart(ctlr, drive->write);
		break;
	}
	iunlock(ctlr);

	return 0;
}

static int
atagenioretry(Drive* drive)
{
	if(drive->dmactl){
		drive->dmactl = 0;
		print("atagenioretry: disabling dma\n");
	}
	else if(drive->rwmctl)
		drive->rwmctl = 0;
	else
		return atasetsense(drive, SDcheck, 4, 8, drive->error);

	return SDretry;
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

		while(waserror())
			;
		tsleep(ctlr, atadone, ctlr, 30*1000);
		poperror();
		if(!ctlr->done){
			/*
			 * What should the above timeout be? In
			 * standby and sleep modes it could take as
			 * long as 30 seconds for a drive to respond.
			 * Very hard to get out of this cleanly.
			 */
			atadumpstate(drive, cmd, lba, count);
			ataabort(drive, 1);
			qunlock(ctlr);
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
retry:
	drive->write = r->write;
	drive->data = r->data;
	drive->dlen = r->dlen;

	drive->status = 0;
	drive->error = 0;
	if(drive->pkt)
		status = atapktio(drive, cmdp, clen);
	else
		status = atagenio(drive, cmdp, clen);
	if(status == SDretry){
		if(DbgDEBUG)
			print("%s: retry: dma %8.8uX rwm %4.4uX\n",
				unit->name, drive->dmactl, drive->rwmctl);
		goto retry;
	}
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
		atadmainterrupt(drive, drive->count*drive->secsize);
		break;

	case Cstandby:
		ctlr->done = 1;
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
		case (0x3373<<16)|0x105A:	/* Promise 20378 RAID */
		case (0x3149<<16)|0x1106:	/* VIA VT8237 SATA/RAID */
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
				if(ispc87415) {
					ctlr->ienable = pc87415ienable;
					print("pc87415disable: not yet implemented\n");
				}

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
			if(!(pi & 0x80))
				continue;
			ctlr->bmiba = (p->mem[4].bar & ~0x01) + channel*8;
		}
	}

if(0){
	int port;
	ISAConf isa;

	/*
	 * Hack for PCMCIA drives.
	 * This will be tidied once we figure out how the whole
	 * removeable device thing is going to work.
	 */
	memset(&isa, 0, sizeof(isa));
	isa.port = 0x180;		/* change this for your machine */
	isa.irq = 11;			/* change this for your machine */

	port = isa.port+0x0C;
	channel = pcmspecial("MK2001MPL", &isa);
	if(channel == -1)
		channel = pcmspecial("SunDisk", &isa);
	if(channel == -1){
		isa.irq = 10;
		channel = pcmspecial("CF", &isa);
	}
	if(channel == -1){
		isa.irq = 10;
		channel = pcmspecial("OLYMPUS", &isa);
	}
	if(channel == -1){
		port = isa.port+0x204;
		channel = pcmspecial("ATA/ATAPI", &isa);
	}
	if(channel >= 0 && (sdev = ataprobe(isa.port, port, isa.irq)) != nil){
		if(head != nil)
			tail->next = sdev;
		else
			head = sdev;
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
	char name[32];

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
			snprint(name, sizeof(name), "sd%c", sdev->idno);
			kstrdup(&sdev->name, name);
		}
		sdev = sdev->next;
	}

	return nil;
}

static int
ataenable(SDev* sdev)
{
	Ctlr *ctlr;
	char name[32];

	ctlr = sdev->ctlr;

	if(ctlr->bmiba){
#define ALIGN	(4 * 1024)
		if(ctlr->pcidev != nil)
			pcisetbme(ctlr->pcidev);
		// ctlr->prdt = xspanalloc(Nprd*sizeof(Prd), 4, 4*1024);
		ctlr->prdtbase = xalloc(Nprd * sizeof(Prd) + ALIGN);
		ctlr->prdt = (Prd *)(((ulong)ctlr->prdtbase + ALIGN) & ~(ALIGN - 1));
	}
	snprint(name, sizeof(name), "%s (%s)", sdev->name, sdev->ifc->name);
	intrenable(ctlr->irq, atainterrupt, ctlr, ctlr->tbdf, name);
	outb(ctlr->ctlport+Dc, 0);
	if(ctlr->ienable)
		ctlr->ienable(ctlr);

	return 1;
}

static int
atadisable(SDev *sdev)
{
	Ctlr *ctlr;
	char name[32];

	ctlr = sdev->ctlr;
	outb(ctlr->ctlport+Dc, Nien);		/* disable interrupts */
	if (ctlr->idisable)
		ctlr->idisable(ctlr);
	snprint(name, sizeof(name), "%s (%s)", sdev->name, sdev->ifc->name);
	intrdisable(ctlr->irq, atainterrupt, ctlr, ctlr->tbdf, name);
	if (ctlr->bmiba) {
		if (ctlr->pcidev)
			pciclrbme(ctlr->pcidev);
		xfree(ctlr->prdtbase);
	}
	return 0;
}

static int
atarctl(SDunit* unit, char* p, int l)
{
	int n;
	Ctlr *ctlr;
	Drive *drive;

	if((ctlr = unit->dev->ctlr) == nil || ctlr->drive[unit->subno] == nil)
		return 0;
	drive = ctlr->drive[unit->subno];

	qlock(drive);
	n = snprint(p, l, "config %4.4uX capabilities %4.4uX",
		drive->info[Iconfig], drive->info[Icapabilities]);
	if(drive->dma)
		n += snprint(p+n, l-n, " dma %8.8uX dmactl %8.8uX",
			drive->dma, drive->dmactl);
	if(drive->rwm)
		n += snprint(p+n, l-n, " rwm %ud rwmctl %ud",
			drive->rwm, drive->rwmctl);
	if(drive->flags&Lba48)
		n += snprint(p+n, l-n, " lba48always %s",
			(drive->flags&Lba48always) ? "on" : "off");
	n += snprint(p+n, l-n, "\n");
	if(drive->sectors){
		n += snprint(p+n, l-n, "geometry %lld %d",
			drive->sectors, drive->secsize);
		if(drive->pkt == 0)
			n += snprint(p+n, l-n, " %d %d %d",
				drive->c, drive->h, drive->s);
		n += snprint(p+n, l-n, "\n");
	}
	qunlock(drive);

	return n;
}

static int
atawctl(SDunit* unit, Cmdbuf* cb)
{
	int period;
	Ctlr *ctlr;
	Drive *drive;

	if((ctlr = unit->dev->ctlr) == nil || ctlr->drive[unit->subno] == nil)
		return 0;
	drive = ctlr->drive[unit->subno];

	qlock(drive);
	if(waserror()){
		qunlock(drive);
		nexterror();
	}

	/*
	 * Dma and rwm control is passive at the moment,
	 * i.e. it is assumed that the hardware is set up
	 * correctly already either by the BIOS or when
	 * the drive was initially identified.
	 */
	if(strcmp(cb->f[0], "dma") == 0){
		if(cb->nf != 2 || drive->dma == 0)
			error(Ebadctl);
		if(strcmp(cb->f[1], "on") == 0)
			drive->dmactl = drive->dma;
		else if(strcmp(cb->f[1], "off") == 0)
			drive->dmactl = 0;
		else
			error(Ebadctl);
	}
	else if(strcmp(cb->f[0], "rwm") == 0){
		if(cb->nf != 2 || drive->rwm == 0)
			error(Ebadctl);
		if(strcmp(cb->f[1], "on") == 0)
			drive->rwmctl = drive->rwm;
		else if(strcmp(cb->f[1], "off") == 0)
			drive->rwmctl = 0;
		else
			error(Ebadctl);
	}
	else if(strcmp(cb->f[0], "standby") == 0){
		switch(cb->nf){
		default:
			error(Ebadctl);
		case 2:
			period = strtol(cb->f[1], 0, 0);
			if(period && (period < 30 || period > 240*5))
				error(Ebadctl);
			period /= 5;
			break;
		}
		if(atastandby(drive, period) != SDok)
			error(Ebadctl);
	}
	else if(strcmp(cb->f[0], "lba48always") == 0){
		if(cb->nf != 2 || !(drive->flags&Lba48))
			error(Ebadctl);
		if(strcmp(cb->f[1], "on") == 0)
			drive->flags |= Lba48always;
		else if(strcmp(cb->f[1], "off") == 0)
			drive->flags &= ~Lba48always;
		else
			error(Ebadctl);
	}
	else
		error(Ebadctl);
	qunlock(drive);
	poperror();

	return 0;
}

SDifc sdataifc = {
	"ata",				/* name */

	atapnp,				/* pnp */
	atalegacy,			/* legacy */
	ataid,				/* id */
	ataenable,			/* enable */
	atadisable,		/* disable */

	scsiverify,			/* verify */
	scsionline,			/* online */
	atario,				/* rio */
	atarctl,			/* rctl */
	atawctl,			/* wctl */

	scsibio,			/* bio */
	ataprobew,		/* probe */
	ataclear,			/* clear */
	atastat,			/* stat */
};
