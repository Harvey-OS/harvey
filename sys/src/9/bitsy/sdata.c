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

//BUG?
#define PCIWADDR(x)	((ulong)(x))

enum {
	DbgCONFIG	= 0x01,		/* detected drive config info */
	DbgIDENTIFY	= 0x02,		/* detected drive identify info */
	DbgSTATE	= 0x04,		/* dump state on panic */
	DbgPROBE	= 0x08,		/* trace device probing */
	DbgDEBUG	= 0x80,		/* the current problem... */
};
#define DEBUG		(DbgDEBUG|DbgSTATE|DbgCONFIG)

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
	Icsfs		= 82,		/* command set/feature supported */
	Icsfe		= 85,		/* command set/feature enabled */
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
	int	tbdf;
	int	bmiba;			/* bus master interface base address */

	void	(*ienable)(Ctlr*);
	SDev*	sdev;

	Drive*	drive[2];

	Prd*	prdt;			/* physical region descriptor table */

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
	int	sectors;		/* total */
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
} Drive;



static void
atadumpstate(Drive* drive, uchar* cmd, int lba, int count)
{
	Prd *prd;
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
		print("lba %d -> %d, count %d -> %d (%d)\n",
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
		if((as & reset) == 0){
			if(dev){
				outb(cmdport+Dh, dev);
				dev = 0;
			}
			else if(ready == 0 || (as & ready)){
				atadebug(0, 0, "ataready: %d 0x%2.2uX\n", micro, as);
				return as;
			}
		}
		if(micro-- <= 0){
			atadebug(0, 0, "ataready: %d 0x%2.2uX\n", micro, as);
			break;
		}
		microdelay(4);
	}
	atadebug(cmdport, ctlport, "ataready: timeout");

	return -1;
}

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
	microdelay(4);
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
		dma = drive->info[Iudma] & 0x1F1F;
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
	microdelay(4);

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
		for(i = 0; i < 32; i++){
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
	ushort *sp;
	Drive *drive;
	int as, i, pkt;
	uchar buf[512], *p;

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
	if(drive->info[Iconfig] != 0x848A && (drive->info[Iconfig] & 0xC000) == 0x8000){
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
		atarwmmode(drive, cmdport, ctlport, dev);
	}
	atadmamode(drive);	

	if(DEBUG & DbgCONFIG){
		print("dev %2.2uX port %uX config %4.4uX capabilities %4.4uX",
			dev, cmdport,
			drive->info[Iconfig], drive->info[Icapabilities]);
		print(" mwdma %4.4uX", drive->info[Imwdma]);
		if(drive->info[Ivalid] & 0x04)
			print(" udma %4.4uX", drive->info[Iudma]);
		print(" dma %8.8uX rwm %ud\n", drive->dma, drive->rwm);
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
	microdelay(20);
	outb(ctlport+Dc, Srst);
	microdelay(20);
	outb(ctlport+Dc, 0);
	microdelay(4*1000);
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
		microdelay(5);
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
	delay(5);
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
#ifdef notdef
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
#endif
	}
	else
		ctlr->drive[1] = drive;

	ctlr->cmdport = cmdport;
	ctlr->ctlport = ctlport;
	ctlr->irq = irq;
	ctlr->tbdf = -1;
	ctlr->command = Cedd;		/* debugging */

	sdev->ifc = &sdataifc;
	sdev->ctlr = ctlr;
	sdev->idno = 'C';
	sdev->nunit = 1;
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
	if(atacsf(drive, 0x0000000000004000LL, 0))
		atanop(drive, 0);
	else{
		atasrst(drive->ctlr->ctlport);
		drive->error |= Abrt;
	}
	if(dolock)
		iunlock(drive->ctlr);
}

static int
atadmasetup(Drive* drive, int )
{
	drive->dmactl = 0;
	return -1;

#ifdef notdef
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
#endif
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

	if((drive->info[Iconfig] & 0x0060) != 0x0020){
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

	outb(cmdport+Count, drive->count);
	outb(cmdport+Sector, s);
	outb(cmdport+Dh, drive->dev|h);
	outb(cmdport+Cyllo, c);
	outb(cmdport+Cylhi, c>>8);
	ctlr->done = 0;
	ctlr->curdrive = drive;
	ctlr->command = drive->command;	/* debugging */
	outb(cmdport+Command, drive->command);

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
	if(drive->dmactl)
		drive->dmactl = 0;
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
		if(DEBUG & DbgDEBUG)
			print("IBsy+");
		return;
	}
	cmdport = ctlr->cmdport;
	status = inb(cmdport+Status);
	if((drive = ctlr->curdrive) == nil){
		iunlock(ctlr);
		if((DEBUG & DbgDEBUG) && ctlr->command != Cedd)
			print("Inil%2.2uX/%2.2uX+", ctlr->command, status);
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

#ifdef notdef
static SDev*
atapnp(void)
{
	int	cmdport;
	int	ctlport;
	int	irq;

	cmdport = 0x200;
	ctlport = cmdport + 0x0C;
	irq = 10;
	return ataprobe(cmdport, ctlport, irq);
}
#endif


static SDev*
atalegacy(int port, int irq)
{
	return ataprobe(port, port+0x204, irq);
}

static int ataitype;
static int atairq;
static int
ataenable(SDev* sdev)
{
	Ctlr *ctlr;
	char name[KNAMELEN];

	ctlr = sdev->ctlr;

	if(ctlr->bmiba){
		ctlr->prdt = xspanalloc(Nprd*sizeof(Prd), 4, 4*1024);
	}
	snprint(name, KNAMELEN, "%s (%s)", sdev->name, sdev->ifc->name);
//	intrenable(ctlr->irq, atainterrupt, ctlr, ctlr->tbdf, name);
	outb(ctlr->ctlport+Dc, 0);
	intrenable(ataitype, atairq, atainterrupt, ctlr, name);
	if(ctlr->ienable)
		ctlr->ienable(ctlr);

	return 1;
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
	n += snprint(p+n, l-n, "\n");
	if(unit->sectors){
		n += snprint(p+n, l-n, "geometry %llud %ld",
			unit->sectors, unit->secsize);
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
	else
		error(Ebadctl);
	qunlock(drive);
	poperror();

	return 0;
}

static int
scsitest(SDreq* r)
{
	r->write = 0;
	memset(r->cmd, 0, sizeof(r->cmd));
	r->cmd[1] = r->lun<<5;
	r->clen = 6;
	r->data = nil;
	r->dlen = 0;
	r->flags = 0;

	r->status = ~0;

	return r->unit->dev->ifc->rio(r);
}

static int
scsirio(SDreq* r)
{
	/*
	 * Perform an I/O request, returning
	 *	-1	failure
	 *	 0	ok
	 *	 1	no medium present
	 *	 2	retry
	 * The contents of r may be altered so the
	 * caller should re-initialise if necesary.
	 */
	r->status = ~0;
	switch(r->unit->dev->ifc->rio(r)){
	default:
		break;
	case SDcheck:
		if(!(r->flags & SDvalidsense))
			break;
		switch(r->sense[2] & 0x0F){
		case 0x00:		/* no sense */
		case 0x01:		/* recovered error */
			return 2;
		case 0x06:		/* check condition */
			/*
			 * 0x28 - not ready to ready transition,
			 *	  medium may have changed.
			 * 0x29 - power on or some type of reset.
			 */
			if(r->sense[12] == 0x28 && r->sense[13] == 0)
				return 2;
			if(r->sense[12] == 0x29)
				return 2;
			break;
		case 0x02:		/* not ready */
			/*
			 * If no medium present, bail out.
			 * If unit is becoming ready, rather than not
			 * not ready, wait a little then poke it again. 				 */
			if(r->sense[12] == 0x3A)
				break;
			if(r->sense[12] != 0x04 || r->sense[13] != 0x01)
				break;

			while(waserror())
				;
			tsleep(&up->sleep, return0, 0, 500);
			poperror();
			scsitest(r);
			return 2;
		default:
			break;
		}
		break;
	case SDok:
		return 0;
	}
	return -1;
}


static int
ataverify(SDunit* unit)
{
	SDreq *r;
	int i, status;
	uchar *inquiry;

	if((r = malloc(sizeof(SDreq))) == nil)
		return 0;
	if((inquiry = sdmalloc(sizeof(unit->inquiry))) == nil){
		free(r);
		return 0;
	}
	r->unit = unit;
	r->lun = 0;		/* ??? */

	memset(unit->inquiry, 0, sizeof(unit->inquiry));
	r->write = 0;
	r->cmd[0] = 0x12;
	r->cmd[1] = r->lun<<5;
	r->cmd[4] = sizeof(unit->inquiry)-1;
	r->clen = 6;
	r->data = inquiry;
	r->dlen = sizeof(unit->inquiry)-1;
	r->flags = 0;

	r->status = ~0;
	if(unit->dev->ifc->rio(r) != SDok){
		free(r);
		return 0;
	}
	memmove(unit->inquiry, inquiry, r->dlen);
	free(inquiry); 

	SET(status);
	for(i = 0; i < 3; i++){
		while((status = scsitest(r)) == SDbusy)
			;
		if(status == SDok || status != SDcheck)
			break;
		if(!(r->flags & SDvalidsense))
			break;
		if((r->sense[2] & 0x0F) != 0x02)
			continue;
		/*
		 * Unit is 'not ready'.
		 * If it needs an initialising command, set status
		 * so it will be spun-up below.
		 * If there's no medium, that's OK too, but don't
		 * try to spin it up.
		 */
		if(r->sense[12] == 0x04 && r->sense[13] == 0x02){
			status = SDok;
			break;
		}
		if(r->sense[12] == 0x3A)
			break;
	}

	if(status == SDok){
		/*
		 * Try to ensure a direct-access device is spinning.
		 * Don't wait for completion, ignore the result.
		 */
		if((unit->inquiry[0] & 0x1F) == 0){
			memset(r->cmd, 0, sizeof(r->cmd));
			r->write = 0;
			r->cmd[0] = 0x1B;
			r->cmd[1] = (r->lun<<5)|0x01;
			r->cmd[4] = 1;
			r->clen = 6;
			r->data = nil;
			r->dlen = 0;
			r->flags = 0;

			r->status = ~0;
			unit->dev->ifc->rio(r);
		}
	}
	free(r);

	if(status == SDok || status == SDcheck)
		return 1;
	return 0;
}

static int
ataonline(SDunit* unit)
{
	SDreq *r;
	uchar *p;
	int ok, retries;

	if((r = malloc(sizeof(SDreq))) == nil)
		return 0;
	if((p = sdmalloc(8)) == nil){
		free(r);
		return 0;
	}

	ok = 0;

	r->unit = unit;
	r->lun = 0;				/* ??? */
	for(retries = 0; retries < 10; retries++){
		/*
		 * Read-capacity is mandatory for DA, WORM, CD-ROM and
		 * MO. It may return 'not ready' if type DA is not
		 * spun up, type MO or type CD-ROM are not loaded or just
		 * plain slow getting their act together after a reset.
		 */
		r->write = 0;
		memset(r->cmd, 0, sizeof(r->cmd));
		r->cmd[0] = 0x25;
		r->cmd[1] = r->lun<<5;
		r->clen = 10;
		r->data = p;
		r->dlen = 8;
		r->flags = 0;
	
		r->status = ~0;
		switch(scsirio(r)){
		default:
			break;
		case 0:
			unit->sectors = (p[0]<<24)|(p[1]<<16)|(p[2]<<8)|p[3];
			/*
			 * Read-capacity returns the LBA of the last sector,
			 * therefore the number of sectors must be incremented.
			 */
			unit->sectors++;
			unit->secsize = (p[4]<<24)|(p[5]<<16)|(p[6]<<8)|p[7];

			/*
			 * Some ATAPI CD readers lie about the block size.
			 * Since we don't read audio via this interface
			 * it's okay to always fudge this.
			 */
			if(unit->secsize == 2352)
				unit->secsize = 2048;
			ok = 1;
			break;
		case 1:
			ok = 1;
			break;
		case 2:
			continue;
		}
		break;
	}
	free(p);
	free(r);

	if(ok)
		return ok+retries;
	else
		return 0;
}

static long
atabio(SDunit* unit, int lun, int write, void* data, long nb, uvlong bno)
{
	SDreq *r;
	long rlen;

	if((r = malloc(sizeof(SDreq))) == nil)
		error(Enomem);
	r->unit = unit;
	r->lun = lun;
again:
	r->write = write;
	if(write == 0)
		r->cmd[0] = 0x28;
	else
		r->cmd[0] = 0x2A;
	r->cmd[1] = (lun<<5);
	r->cmd[2] = bno>>24;
	r->cmd[3] = bno>>16;
	r->cmd[4] = bno>>8;
	r->cmd[5] = bno;
	r->cmd[6] = 0;
	r->cmd[7] = nb>>8;
	r->cmd[8] = nb;
	r->cmd[9] = 0;
	r->clen = 10;
	r->data = data;
	r->dlen = nb*unit->secsize;
	r->flags = 0;

	r->status = ~0;
	switch(scsirio(r)){
	default:
		rlen = -1;
		break;
	case 0:
		rlen = r->rlen;
		break;
	case 2:
		rlen = -1;
		if(!(r->flags & SDvalidsense))
			break;
		switch(r->sense[2] & 0x0F){
		default:
			break;
		case 0x06:		/* check condition */
			/*
			 * Check for a removeable media change.
			 * If so, mark it by zapping the geometry info
			 * to force an online request.
			 */
			if(r->sense[12] != 0x28 || r->sense[13] != 0)
				break;
			if(unit->inquiry[1] & 0x80)
				unit->sectors = 0;
			break;
		case 0x02:		/* not ready */
			/*
			 * If unit is becoming ready,
			 * rather than not not ready, try again.
			 */
			if(r->sense[12] == 0x04 && r->sense[13] == 0x01)
				goto again;
			break;
		}
		break;
	}
	free(r);

	return rlen;
}


struct Try {
	int p;
	int c;
} tries[] = {
		   { 0, 0x0c },
		   { 0, 0 }, 
};

static SDev*
ataprobew(DevConf *cf)
{
	int	cmdport;
	int	ctlport;
	int	irq;
	SDev*	rc;
	struct Try *try;

	rc = nil;
	for (try = &tries[0]; try->p != 0 || try->c != 0; try++){
		ataitype = cf->itype;
		atairq  = cf->intnum;
		cmdport = cf->ports[0].port + try->p;
		ctlport = cmdport + try->c;
		irq = cf->intnum;
		rc = ataprobe(cmdport, ctlport, irq);
		if (rc)
			break;
	}
	return rc;
}

static void
ataclear(SDev *sdev)
{
	Ctlr* ctlr;

	ctlr = sdev->ctlr;

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


SDifc sdataifc = {
	"ata",				/* name */

	nil,				/* pnp */
	atalegacy,			/* legacy */
	ataenable,			/* enable */
	nil,				/* disable */

	ataverify,			/* verify */
	ataonline,			/* online */
	atario,				/* rio */
	atarctl,			/* rctl */
	atawctl,			/* wctl */

	atabio,				/* bio */
	ataprobew,			/* probew */
	ataclear,			/* clear */
	atastat,			/* stat */
};

