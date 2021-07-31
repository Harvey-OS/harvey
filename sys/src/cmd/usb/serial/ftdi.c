/* Future Technology Devices International serial ports */
#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"
#include "usbfs.h"
#include "serial.h"
#include "ftdi.h"

/*
 * BUG: This keeps growing, there has to be a better way, but without
 * devices to try it...  We can probably simply look for FTDI in the
 * string, or use regular expressions somehow.
 */
Cinfo ftinfo[] = {
	{ FTVid, FTACTZWAVEDid },
	{ FTSheevaVid, FTSheevaDid },
	{ FTVid, FTIRTRANSDid },
	{ FTVid, FTIPLUSDid },
	{ FTVid, FTSIODid },
	{ FTVid, FT8U232AMDid },
	{ FTVid, FT8U232AMALTDid },
	{ FTVid, FT8U2232CDid },
	{ FTVid, FTRELAISDid },
	{ INTERBIOMVid, INTERBIOMIOBRDDid },
	{ INTERBIOMVid, INTERBIOMMINIIOBRDDid },
	{ FTVid, FTXF632Did },
	{ FTVid, FTXF634Did },
	{ FTVid, FTXF547Did },
	{ FTVid, FTXF633Did },
	{ FTVid, FTXF631Did },
	{ FTVid, FTXF635Did },
	{ FTVid, FTXF640Did },
	{ FTVid, FTXF642Did },
	{ FTVid, FTDSS20Did },
	{ FTNFRICVid, FTNFRICDid },
	{ FTVid, FTVNHCPCUSBDDid },
	{ FTVid, FTMTXORB0Did },
	{ FTVid, FTMTXORB1Did },
	{ FTVid, FTMTXORB2Did },
	{ FTVid, FTMTXORB3Did },
	{ FTVid, FTMTXORB4Did },
	{ FTVid, FTMTXORB5Did },
	{ FTVid, FTMTXORB6Did },
	{ FTVid, FTPERLEULTRAPORTDid },
	{ FTVid, FTPIEGROUPDid },
	{ SEALEVELVid, SEALEVEL2101Did },
	{ SEALEVELVid, SEALEVEL2102Did },
	{ SEALEVELVid, SEALEVEL2103Did },
	{ SEALEVELVid, SEALEVEL2104Did },
	{ SEALEVELVid, SEALEVEL22011Did },
	{ SEALEVELVid, SEALEVEL22012Did },
	{ SEALEVELVid, SEALEVEL22021Did },
	{ SEALEVELVid, SEALEVEL22022Did },
	{ SEALEVELVid, SEALEVEL22031Did },
	{ SEALEVELVid, SEALEVEL22032Did },
	{ SEALEVELVid, SEALEVEL24011Did },
	{ SEALEVELVid, SEALEVEL24012Did },
	{ SEALEVELVid, SEALEVEL24013Did },
	{ SEALEVELVid, SEALEVEL24014Did },
	{ SEALEVELVid, SEALEVEL24021Did },
	{ SEALEVELVid, SEALEVEL24022Did },
	{ SEALEVELVid, SEALEVEL24023Did },
	{ SEALEVELVid, SEALEVEL24024Did },
	{ SEALEVELVid, SEALEVEL24031Did },
	{ SEALEVELVid, SEALEVEL24032Did },
	{ SEALEVELVid, SEALEVEL24033Did },
	{ SEALEVELVid, SEALEVEL24034Did },
	{ SEALEVELVid, SEALEVEL28011Did },
	{ SEALEVELVid, SEALEVEL28012Did },
	{ SEALEVELVid, SEALEVEL28013Did },
	{ SEALEVELVid, SEALEVEL28014Did },
	{ SEALEVELVid, SEALEVEL28015Did },
	{ SEALEVELVid, SEALEVEL28016Did },
	{ SEALEVELVid, SEALEVEL28017Did },
	{ SEALEVELVid, SEALEVEL28018Did },
	{ SEALEVELVid, SEALEVEL28021Did },
	{ SEALEVELVid, SEALEVEL28022Did },
	{ SEALEVELVid, SEALEVEL28023Did },
	{ SEALEVELVid, SEALEVEL28024Did },
	{ SEALEVELVid, SEALEVEL28025Did },
	{ SEALEVELVid, SEALEVEL28026Did },
	{ SEALEVELVid, SEALEVEL28027Did },
	{ SEALEVELVid, SEALEVEL28028Did },
	{ SEALEVELVid, SEALEVEL28031Did },
	{ SEALEVELVid, SEALEVEL28032Did },
	{ SEALEVELVid, SEALEVEL28033Did },
	{ SEALEVELVid, SEALEVEL28034Did },
	{ SEALEVELVid, SEALEVEL28035Did },
	{ SEALEVELVid, SEALEVEL28036Did },
	{ SEALEVELVid, SEALEVEL28037Did },
	{ SEALEVELVid, SEALEVEL28038Did },
	{ IDTECHVid, IDTECHIDT1221UDid },
	{ OCTVid, OCTUS101Did },
	{ FTVid, FTHETIRA1Did }, /* special quirk div = 240 baud = B38400 rtscts = 1 */
	{ FTVid, FTUSBUIRTDid }, /* special quirk div = 77, baud = B38400 */
	{ FTVid, PROTEGOSPECIAL1 },
	{ FTVid, PROTEGOR2X0 },
	{ FTVid, PROTEGOSPECIAL3 },
	{ FTVid, PROTEGOSPECIAL4 },
	{ FTVid, FTGUDEADSE808Did },
	{ FTVid, FTGUDEADSE809Did },
	{ FTVid, FTGUDEADSE80ADid },
	{ FTVid, FTGUDEADSE80BDid },
	{ FTVid, FTGUDEADSE80CDid },
	{ FTVid, FTGUDEADSE80DDid },
	{ FTVid, FTGUDEADSE80EDid },
	{ FTVid, FTGUDEADSE80FDid },
	{ FTVid, FTGUDEADSE888Did },
	{ FTVid, FTGUDEADSE889Did },
	{ FTVid, FTGUDEADSE88ADid },
	{ FTVid, FTGUDEADSE88BDid },
	{ FTVid, FTGUDEADSE88CDid },
	{ FTVid, FTGUDEADSE88DDid },
	{ FTVid, FTGUDEADSE88EDid },
	{ FTVid, FTGUDEADSE88FDid },
	{ FTVid, FTELVUO100Did },
	{ FTVid, FTELVUM100Did },
	{ FTVid, FTELVUR100Did },
	{ FTVid, FTELVALC8500Did },
	{ FTVid, FTPYRAMIDDid },
	{ FTVid, FTELVFHZ1000PCDid },
	{ FTVid, FTELVCLI7000Did },
	{ FTVid, FTELVPPS7330Did },
	{ FTVid, FTELVTFM100Did },
	{ FTVid, FTELVUDF77Did },
	{ FTVid, FTELVUIO88Did },
	{ FTVid, FTELVUAD8Did },
	{ FTVid, FTELVUDA7Did },
	{ FTVid, FTELVUSI2Did },
	{ FTVid, FTELVT1100Did },
	{ FTVid, FTELVPCD200Did },
	{ FTVid, FTELVULA200Did },
	{ FTVid, FTELVCSI8Did },
	{ FTVid, FTELVEM1000DLDid },
	{ FTVid, FTELVPCK100Did },
	{ FTVid, FTELVRFP500Did },
	{ FTVid, FTELVFS20SIGDid },
	{ FTVid, FTELVWS300PCDid },
	{ FTVid, FTELVFHZ1300PCDid },
	{ FTVid, FTELVWS500Did },
	{ FTVid, LINXSDMUSBQSSDid },
	{ FTVid, LINXMASTERDEVEL2Did },
	{ FTVid, LINXFUTURE0Did },
	{ FTVid, LINXFUTURE1Did },
	{ FTVid, LINXFUTURE2Did },
	{ FTVid, FTCCSICDU200Did },
	{ FTVid, FTCCSICDU401Did },
	{ FTVid, INSIDEACCESSO },
	{ INTREDidVid, INTREDidVALUECANDid },
	{ INTREDidVid, INTREDidNEOVIDid },
	{ FALCOMVid, FALCOMTWISTDid },
	{ FALCOMVid, FALCOMSAMBADid },
	{ FTVid, FTSUUNTOSPORTSDid },
	{ FTVid, FTRMCANVIEWDid },
	{ BANDBVid, BANDBUSOTL4Did },
	{ BANDBVid, BANDBUSTL4Did },
	{ BANDBVid, BANDBUSO9ML2Did },
	{ FTVid, EVERECOPROCDSDid },
	{ FTVid, FT4NGALAXYDE0Did },
	{ FTVid, FT4NGALAXYDE1Did },
	{ FTVid, FT4NGALAXYDE2Did },
	{ FTVid, XSENSCONVERTER0Did },
	{ FTVid, XSENSCONVERTER1Did },
	{ FTVid, XSENSCONVERTER2Did },
	{ FTVid, XSENSCONVERTER3Did },
	{ FTVid, XSENSCONVERTER4Did },
	{ FTVid, XSENSCONVERTER5Did },
	{ FTVid, XSENSCONVERTER6Did },
	{ FTVid, XSENSCONVERTER7Did },
	{ MOBILITYVid, MOBILITYUSBSERIALDid },
	{ FTVid, FTACTIVEROBOTSDid },
	{ FTVid, FTMHAMKWDid },
	{ FTVid, FTMHAMYSDid },
	{ FTVid, FTMHAMY6Did },
	{ FTVid, FTMHAMY8Did },
	{ FTVid, FTMHAMICDid },
	{ FTVid, FTMHAMDB9Did },
	{ FTVid, FTMHAMRS232Did },
	{ FTVid, FTMHAMY9Did },
	{ FTVid, FTTERATRONIKVCPDid },
	{ FTVid, FTTERATRONIKD2XXDid },
	{ EVOLUTIONVid, EVOLUTIONER1Did },
	{ FTVid, FTARTEMISDid },
	{ FTVid, FTATIKATK16Did },
	{ FTVid, FTATIKATK16CDid },
	{ FTVid, FTATIKATK16HRDid },
	{ FTVid, FTATIKATK16HRCDid },
	{ KOBILVid, KOBILCONVB1Did },
	{ KOBILVid, KOBILCONVKAANDid },
	{ POSIFLEXVid, POSIFLEXPP7000Did },
	{ FTVid, FTTTUSBDid },
	{ FTVid, FTECLOCOM1WIREDid },
	{ FTVid, FTWESTREXMODEL777Did },
	{ FTVid, FTWESTREXMODEL8900FDid },
	{ FTVid, FTPCDJDAC2Did },
	{ FTVid, FTRRCIRKITSLOCOBUFFERDid },
	{ FTVid, FTASKRDR400Did },
	{ ICOMID1Vid, ICOMID1Did },
	{ PAPOUCHVid, PAPOUCHTMUDid },
	{ FTVid, FTACGHFDUALDid },
	{ 0,	0 },
};

enum {
	Packsz = 1024,
};

static int
ftdiread(Serial *ser, int val, int index, int req, uchar *buf)
{
	int res;

	if(req != FTGETE2READ)
		index |= ser->interfc + 1;
	dsprint(2, "serial: ftdiread req: %#x val: %#x idx:%d buf:%p\n",
		req, val, index, buf);
	res = usbcmd(ser->dev,  Rd2h | Rftdireq | Rdev, req, val, index, buf, 1);
	dsprint(2, "serial: ftdiread res:%d\n", res);
	return res;
}

static int
ftdiwrite(Serial *ser, int val, int index, int req)
{
	int res;

	index |= ser->interfc + 1;
	dsprint(2, "serial: ftdiwrite  req: %#x val: %#x idx:%d\n",
		req, val, index);
	res = usbcmd(ser->dev, Rh2d | Rftdireq | Rdev, req, val, index, nil, 0);
	dsprint(2, "serial: ftdiwrite res:%d\n", res);
	return res;
}

static int
ftmodemctl(Serial *ser, int set)
{
	if(set == 0){
		ser->mctl = 0;
		ftdiwrite(ser, 0, 0, FTSETMODEMCTRL);
		return 0;
	}
	ser->mctl = 1;
	ftdiwrite(ser, 0, FTRTSCTSHS, FTSETFLOWCTRL);
	return 0;
}

static ushort
ft232ambaudbase2div(int baud, int base)
{
	int divisor3;
	ushort divisor;

	divisor3 = (base / 2) / baud;
 	if((divisor3 & 7) == 7)
		divisor3++; 			/* round x.7/8 up to x+1 */
 	divisor = divisor3 >> 3;
	divisor3 &= 7;

	if(divisor3 == 1)
		divisor |= 0xc000;		/*	0.125 */
	else if(divisor3 >= 4)
		divisor |= 0x4000;		/*	0.5	*/
	else if(divisor3 != 0)
		divisor |= 0x8000;		/*	0.25	*/
 	if( divisor == 1)
		divisor = 0;		/* special case for maximum baud rate */
	return divisor;
}

enum{
	ClockNew	= 48000000,
	ClockOld	= 12000000 / 16,
	HetiraDiv	= 240,
	UirtDiv		= 77,
};

static ushort
ft232ambaud2div(int baud)
{
	return ft232ambaudbase2div(baud, ClockNew);
}

static ulong divfrac[8] = { 0, 3, 2, 4, 1, 5, 6, 7};

static ulong
ft232bmbaudbase2div(int baud, int base)
{
	int divisor3;
	u32int divisor;

	divisor3 = (base / 2) / baud;
	divisor = divisor3 >> 3 | divfrac[divisor3 & 7] << 14;

 	/* Deal with special cases for highest baud rates. */
 	if( divisor == 1)
		divisor = 0; 			/* 1.0 */
	else if( divisor == 0x4001)
		divisor = 1;			/* 1.5 */
	return divisor;
}

static ulong
ft232bmbaud2div (int baud)
{
	return ft232bmbaudbase2div (baud, ClockNew);
}

static int
customdiv(Serial *ser)
{
	if(ser->dev->usb->vid == FTVid && ser->dev->usb->did == FTHETIRA1Did)
		return HetiraDiv;
	else if(ser->dev->usb->vid == FTVid && ser->dev->usb->did == FTUSBUIRTDid)
		return UirtDiv;

	fprint(2, "serial: weird custom divisor\n");
	return 0;		/* shouldn't happen, break as much as I can */
}

static ulong
ftbaudcalcdiv(Serial *ser, int baud)
{
	int cusdiv;
	ulong divval;

	if(baud == 38400 && (cusdiv = customdiv(ser)) != 0)
		baud = ser->baudbase / cusdiv;

	if(baud == 0)
		baud = 9600;

 	switch(ser->type) {
 	case SIO:
		switch(baud) {
		case 300:
			divval = FTb300;
			break;
		case 600:
			divval = FTb600;
			break;
		case 1200:
			divval = FTb1200;
			break;
		case 2400:
			divval = FTb2400;
			break;
		case 4800:
			divval = FTb4800;
			break;
		case 9600:
			divval = FTb9600;
			break;
		case 19200:
			divval = FTb19200;
			break;
		case 38400:
			divval = FTb38400;
			break;
		case 57600:
			divval = FTb57600;
			break;
		case 115200:
			divval = FTb115200;
			break;
		default:
			divval = FTb9600;
			break;
		}
		break;
 	case FT8U232AM:
	 	if(baud <= 3000000)
			divval = ft232ambaud2div(baud);
		else
			divval = ft232ambaud2div(9600);
 		break;
	case FT232BM:
	case FT2232C:
		if(baud <= 3000000)
			divval = ft232bmbaud2div(baud);
		else
			divval = ft232bmbaud2div(9600);
		break;
	default:
		divval = ft232bmbaud2div(9600);
		break;
	}
	return divval;
}

static int
ftsetparam(Serial *ser)
{
	int res;
	ushort val;
	ulong bauddiv;

	val = 0;
	if(ser->stop == 1)
		val |= FTSETDATASTOPBITS1;
	else if(ser->stop == 2)
		val |= FTSETDATASTOPBITS2;
	else if(ser->stop == 15)
		val |= FTSETDATASTOPBITS15;
	switch(ser->parity){
	case 0:
		val |= FTSETDATAParNONE;
		break;
	case 1:
		val |= FTSETDATAParODD;
		break;
	case 2:
		val |= FTSETDATAParEVEN;
		break;
	case 3:
		val |= FTSETDATAParMARK;
		break;
	case 4:
		val |= FTSETDATAParSPACE;
		break;
	};

	dsprint(2, "serial: setparam\n");

	res = ftdiwrite(ser, val, 0, FTSETDATA);
	if(res < 0)
		return res;

	res = ftmodemctl(ser, ser->mctl);
	if(res < 0)
		return res;

	bauddiv = ftbaudcalcdiv(ser, ser->baud);
	res = ftdiwrite(ser, bauddiv, 0x1&(bauddiv>>16), FTSETBaudRate);

	dsprint(2, "serial: setparam res: %d\n", res);
	return res;
}

/* ser locked */
static void
ftgettype(Serial *ser)
{
	int inter, nifcs, i, outhdrsz, dno;
	ulong baudbase;
	Conf *cnf;

	inter = 0;
 	/* Assume it is not the original SIO device for now. */
	baudbase = ClockNew / 2;
	outhdrsz = 0;
	nifcs = 0;
	dno = ser->dev->usb->dno;
	cnf = ser->dev->usb->conf[0];
	for(i = 0; i < Niface; i++)
		if(cnf->iface[i] != nil)
			nifcs++;
	if(nifcs> 1) {
		/* Multiple interfaces.  Assume FT2232C. */
		ser->type = FT2232C;
		/*
		 * BUG: If there is more than one interface, we use the second.
		 * We only support 1 interface at the moment...
		 * This works well for the sheeva/JTAG, but in other
		 * cases we are ignoring nifcs-1 interfaces and using
		 * the second (weird).  The problem is we have to rethink
		 * the whole scheme to make it work with various ifaces.
		 */
		inter = PITA;

		/*
		 * BM-type devices have a bug where dno gets set
		 * to 0x200 when serial is 0.
		 */
		if(dno < 0x500)
			fprint(2, "serial: warning: dno too low for multi-interfacedevice\n");
	} else if(dno < 0x200) {
		/* Old device.  Assume it is the original SIO. */
		ser->type = SIO;
		baudbase = ClockOld/16;
		outhdrsz = 1;
	} else if(dno < 0x400)
		/*
		 * Assume its an FT8U232AM (or FT8U245AM)
		 * (It might be a BM because of the iSerialNumber bug,
		 * but it will still work as an AM device.)
		 */
		ser->type = FT8U232AM;
	else			/* Assume it is an FT232BM (or FT245BM) */
		ser->type = FT232BM;

	ser->baudbase = baudbase;
	ser->outhdrsz = outhdrsz;
	ser->inhdrsz = 2;
	ser->interfc = inter;
	dsprint (2, "serial: detected type: %#x\n", ser->type);
}

int
ftmatch(Serial *ser, char *info)
{
	Cinfo *ip;
	char buf[50];

	for(ip = ftinfo; ip->vid != 0; ip++){
		snprint(buf, sizeof buf, "vid %#06x did %#06x", ip->vid, ip->did);
		dsprint(2, "serial: %s %s\n", buf, info);
		if(strstr(info, buf) != nil){
			if(ser != nil){
				qlock(ser);
				ftgettype(ser);
				qunlock(ser);
			}
			return 0;
		}
	}
	return -1;
}

static int
ftuseinhdr(Serial *ser, uchar *b)
{
	if(b[0] & FTICTS)
		ser->cts = 1;
	else
		ser->cts = 0;
	if(b[0] & FTIDSR)
		ser->dsr = 1;
	else
		ser->dsr = 0;
	if(b[0] & FTIRI)
		ser->ring = 1;
	else
		ser->ring = 0;
	if(b[0] & FTIRLSD)
		ser->rlsd = 1;
	else
		ser->rlsd = 0;

	if(b[1] & FTIOE)
		ser->novererr++;
	if(b[1] & FTIPE)
		ser->nparityerr++;
	if(b[1] & FTIFE)
		ser->nframeerr++;
	if(b[1] & FTIBI)
		ser->nbreakerr++;
	return 0;
}

static int
ftsetouthdr(Serial *ser, uchar *b, int len)
{
	if(ser->outhdrsz != 0)
		b[0] = FTOPORT | (FTOLENMSK & len);
	return ser->outhdrsz;
}

static int
wait4data(Serial *ser, uchar *data, int count)
{
	qunlock(ser);
	recvul(ser->w4data);
	qlock(ser);
	if(ser->ndata >= count)
		ser->ndata -= count;
	else{
		count = ser->ndata;
		ser->ndata = 0;
	}
	memmove(data, ser->data, count);
	if(ser->ndata != 0)
		memmove(ser->data, ser->data+count, ser->ndata);

	sendul(ser->gotdata, 1);
	return count;
}

static int
wait4write(Serial *ser, uchar *data, int count)
{
	int off, fd;
	uchar *b;

	// qunlock(ser);
	// recvul(ser->w4empty);	should I really?
	// qlock(ser);

	b = emallocz(count+ser->outhdrsz, 1);
	off = ftsetouthdr(ser, b, count);
	memmove(b+off, data, count);

	fd = ser->epout->dfd;
	qunlock(ser);
	count = write(fd, b, count+off);
	qlock(ser);
	free(b);
	return count;
}

typedef struct Packser Packser;
struct Packser{
	int	nb;
	uchar	b[Packsz];
};

typedef struct Areader Areader;
struct Areader{
	Serial	*s;
	Channel	*c;
};

static void
epreader(void *u)
{
	int dfd, rcount;
	char err[40];
	Areader *a;
	Channel *c;
	Packser *p;
	Serial *ser;

	threadsetname("epreader proc");
	a = u;
	ser = a->s;
	c = a->c;
	free(a);

	qlock(ser);
	dfd = ser->epin->dfd;
	qunlock(ser);

	do {
		p = emallocz(sizeof(Packser), 1);
		rcount = read(dfd, p->b, Packsz);
		if(serialdebug > 5)
			dsprint(2, "%d %#ux%#ux ", rcount, ser->data[0],
				ser->data[1]);
		if(rcount <= 0)
			break;
		if(rcount >= ser->inhdrsz){
			ftuseinhdr(ser, p->b);
			rcount -= ser->inhdrsz;
			memmove(p->b, p->b+ser->inhdrsz, rcount);
			if(rcount != 0){
				p->nb = rcount;
				sendp(c, p);
			}
		}
	} while(rcount >= 0 || (rcount < 0 && strstr(err, "timed out") != nil));

	if(rcount < 0)
		fprint(2, "error reading: %r\n");
	sendp(c, nil);
	closedev(ser->dev);
}

static void
statusreader(void *u)
{
	Areader *a;
	Channel *c;
	Packser *p;
	Serial *ser;

	ser = u;
	threadsetname("statusreader thread");
	/* big buffering, fewer bytes lost */
	c = chancreate(sizeof(Packser *), 128);
	a = emallocz(sizeof(Areader), 1);
	a->s = ser;
	a->c = c;
	incref(ser->dev);
	proccreate(epreader, a, 16*1024);

	while((p = recvp(c)) != nil){
		memmove(ser->data, p->b, p->nb);
		ser->ndata = p->nb;
		dsprint(2, "serial: status reader %d \n", p->nb);
		/* consume it all */
		while(ser->ndata != 0){
			dsprint(2, "serial: status reader to consume: %d\n",
				p->nb);
			sendul(ser->w4data, 1);
			recvul(ser->gotdata);
		}
		free(p);
	}
	free(a);
	free(c);
	closedev(ser->dev);
}

static int
ftreset(Serial *ser)
{
	ftdiwrite(ser, FTRESETCTLVAL, 0, FTRESET);
	return 0;
}

static int
ftinit(Serial *ser)
{
	qlock(ser);
	serialreset(ser);
	qunlock(ser);

	incref(ser->dev);
	threadcreate(statusreader, ser, 8*1024);
	return 0;
}

static int
ftsetbreak(Serial *ser, int val)
{
	return ftdiwrite(ser, (val != 0? FTSETBREAK: 0), 0, FTSETDATA);
}

static int
ftclearpipes(Serial *ser)
{
	/* maybe can be done in one... */
	ftdiwrite(ser, FTRESETCTLVALPURGETX, 0, FTRESET);
	ftdiwrite(ser, FTRESETCTLVALPURGERX, 0, FTRESET);
	return 0;
}

static int
setctlline(Serial *ser, uchar val)
{
	return ftdiwrite(ser, val | (val << 8), 0, FTSETMODEMCTRL);
}

static void
updatectlst(Serial *ser, int val)
{
	if(ser->rts)
		ser->ctlstate |= val;
	else
		ser->ctlstate &= ~val;
}

static int
setctl(Serial *ser)
{
	int res;

	if(ser->dev->usb->vid == FTVid && ser->dev->usb->did ==  FTHETIRA1Did){
		fprint(2, "serial: cannot set lines for this device\n");
		updatectlst(ser, CtlRTS|CtlDTR);
		ser->rts = ser->dtr = 1;
		return -1;
	}

	/* NB: you can not set DTR and RTS with one control message */
	updatectlst(ser, CtlRTS);
	res = setctlline(ser, (CtlRTS<<8)|ser->ctlstate);
	if(res < 0)
		return res;

	updatectlst(ser, CtlDTR);
	res = setctlline(ser, (CtlDTR<<8)|ser->ctlstate);
	if(res < 0)
		return res;

	return 0;
}

static int
ftsendlines(Serial *ser)
{
	int res;

	dsprint(2, "serial: sendlines: %#2.2x\n", ser->ctlstate);
	res = setctl(ser);
	dsprint(2, "serial: sendlines res: %d\n", res);
	return 0;
}

static int
ftseteps(Serial *ser)
{
	char *s;

	s = smprint("maxpkt %d", Packsz);
	devctl(ser->epin, s);
	ser->maxread = Packsz;
	devctl(ser->epout, s);
	ser->maxwrite = Packsz;
	free(s);
	return 0;
}

Serialops ftops = {
	.init		= ftinit,
	.seteps		= ftseteps,
	.setparam	= ftsetparam,
	.clearpipes	= ftclearpipes,
	.reset		= ftreset,
	.sendlines	= ftsendlines,
	.modemctl	= ftmodemctl,
	.setbreak	= ftsetbreak,
	.wait4data	= wait4data,
	.wait4write	= wait4write,
};
