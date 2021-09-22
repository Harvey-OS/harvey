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
	{ FTVid, FTOpenRDUltDid},
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
	{ FT8U232AMDid, FT4232HDid },
	{ FTVid, AMONKEYDid },
	{ 0,	0 },
};

enum {
	Packsz		= 64,		/* default size */
	Maxpacksz	= 512,
	Bufsiz		= 4 * 1024,
};

static int
ftdiread(Serialport *p, int index, int req, uchar *buf, int len)
{
	int res;
	Serial *ser;

	ser = p->s;

	if(req != FTGETE2READ)
		index |= p->interfc + 1;
	dsprint(2, "serial: ftdiread %#p [%d] req: %#x val: %#x idx:%d buf:%p len:%d\n",
		p, p->interfc, req, 0, index, buf, len);
	res = usbcmd(ser->dev,  Rd2h | Rftdireq | Rdev, req, 0, index, buf, len);
	dsprint(2, "serial: ftdiread res:%d\n", res);
	return res;
}

static int
ftdiwrite(Serialport *p, int val, int index, int req)
{
	int res;
	Serial *ser;

	ser = p->s;

	if(req != FTGETE2READ || req != FTSETE2ERASE || req != FTSETBAUDRATE)
		index |= p->interfc + 1;
	dsprint(2, "serial: ftdiwrite %#p [%d] req: %#x val: %#x idx:%d\n",
		p, p->interfc, req, val, index);
	res = usbcmd(ser->dev, Rh2d | Rftdireq | Rdev, req, val, index, nil, 0);
	dsprint(2, "serial: ftdiwrite res:%d\n", res);
	return res;
}

static int
ftmodemctl(Serialport *p, int set)
{
	if(set == 0){
		p->mctl = 0;
		ftdiwrite(p, 0, 0, FTSETMODEMCTRL);
		return 0;
	}
	p->mctl = 1;
	ftdiwrite(p, 0, FTRTSCTSHS, FTSETFLOWCTRL);
	return 0;
}

static ushort
ft232ambaudbase2div(int baud, int base)
{
	int divisor3;
	ushort divisor;

	divisor3 = (base / 2) / baud;
	if((divisor3 & 7) == 7)
		divisor3++;			/* round x.7/8 up to x+1 */
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
		divisor = 0;			/* 1.0 */
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
	case FTKINDR:
	case FT2232H:
	case FT4232H:
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
ftsetparam(Serialport *p)
{
	int res;
	ushort val;
	ulong bauddiv;

	val = 0;
	if(p->stop == 1)
		val |= FTSETDATASTOPBITS1;
	else if(p->stop == 2)
		val |= FTSETDATASTOPBITS2;
	else if(p->stop == 15)
		val |= FTSETDATASTOPBITS15;
	switch(p->parity){
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

	res = ftdiwrite(p, val, 0, FTSETDATA);
	if(res < 0)
		return res;

	res = ftmodemctl(p, p->mctl);
	if(res < 0)
		return res;

	bauddiv = ftbaudcalcdiv(p->s, p->baud);
	res = ftdiwrite(p, bauddiv, (bauddiv>>16) & 1, FTSETBAUDRATE);

	dsprint(2, "serial: setparam res: %d\n", res);
	return res;
}

static int
hasjtag(Usbdev *udev)
{
	/* no string, for now, by default we detect no jtag */
	if(udev->product != nil && cistrstr(udev->product, "jtag") != nil)
		return 1;
	return 0;
}

/* ser locked */
static void
ftgettype(Serial *ser)
{
	int i, outhdrsz, dno, pksz;
	ulong baudbase;
	Conf *cnf;

	pksz = Packsz;
	/* Assume it is not the original SIO device for now. */
	baudbase = ClockNew / 2;
	outhdrsz = 0;
	dno = ser->dev->usb->dno;
	cnf = ser->dev->usb->conf[0];
	ser->nifcs = 0;
	for(i = 0; i < Niface; i++)
		if(cnf->iface[i] != nil)
			ser->nifcs++;
	if(ser->nifcs > 1) {
		/*
		 * Multiple interfaces.  default assume FT2232C,
		 */
		if(dno == 0x500)
			ser->type = FT2232C;
		else if(dno == 0x600)
			ser->type = FTKINDR;
		else if(dno == 0x700){
			ser->type = FT2232H;
			pksz = Maxpacksz;
		} else if(dno == 0x800){
			ser->type = FT4232H;
			pksz = Maxpacksz;
		} else
			ser->type = FT2232C;

		if(hasjtag(ser->dev->usb))
			ser->jtag = 0;

		/*
		 * BM-type devices have a bug where dno gets set
		 * to 0x200 when serial is 0.
		 */
		if(dno < 0x500)
			fprint(2, "serial: warning: dno %d too low for "
				"multi-interface device\n", dno);
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

	ser->maxrtrans = ser->maxwtrans = pksz;
	ser->baudbase = baudbase;
	ser->outhdrsz = outhdrsz;
	ser->inhdrsz = 2;

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
ftuseinhdr(Serialport *p, uchar *b)
{
	if(b[0] & FTICTS)
		p->cts = 1;
	else
		p->cts = 0;
	if(b[0] & FTIDSR)
		p->dsr = 1;
	else
		p->dsr = 0;
	if(b[0] & FTIRI)
		p->ring = 1;
	else
		p->ring = 0;
	if(b[0] & FTIRLSD)
		p->rlsd = 1;
	else
		p->rlsd = 0;

	if(b[1] & FTIOE)
		p->novererr++;
	if(b[1] & FTIPE)
		p->nparityerr++;
	if(b[1] & FTIFE)
		p->nframeerr++;
	if(b[1] & FTIBI)
		p->nbreakerr++;
	return 0;
}

static int
ftsetouthdr(Serialport *p, uchar *b, int len)
{
	if(p->s->outhdrsz != 0)
		b[0] = FTOPORT | (FTOLENMSK & len);
	return p->s->outhdrsz;
}

static int
wait4data(Serialport *p, uchar *data, int count)
{
	int d;
	Serial *ser;

	ser = p->s;

	qunlock(ser);
	d = sendul(p->w4data, 1);
	qlock(ser);
	if(d <= 0)
		return -1;
	if(p->ndata >= count)
		p->ndata -= count;
	else{
		count = p->ndata;
		p->ndata = 0;
	}
	assert(count >= 0);
	assert(p->ndata >= 0);
	memmove(data, p->data, count);
	if(p->ndata != 0)
		memmove(p->data, p->data+count, p->ndata);

	recvul(p->gotdata);
	return count;
}

static int
wait4write(Serialport *p, uchar *data, int count)
{
	int off, fd;
	uchar *b;
	Serial *ser;

	ser = p->s;

	b = emallocz(count+ser->outhdrsz, 1);
	off = ftsetouthdr(p, b, count);
	memmove(b+off, data, count);

	fd = p->epout->dfd;
	qunlock(ser);
	count = write(fd, b, count+off);
	qlock(ser);
	free(b);
	return count;
}

typedef struct Packser Packser;
struct Packser{
	int	nb;
	uchar	b[Bufsiz];
};

typedef struct Areader Areader;
struct Areader{
	Serialport	*p;
	Channel	*c;
};

static void
shutdownchan(Channel *c)
{
	Packser *bp;

	while((bp=nbrecvp(c)) != nil)
		free(bp);
	chanfree(c);
}

int
cpdata(Serial *ser, Serialport *port, uchar *out, uchar *in, int sz)
{
	int i, ncp, ntotcp, pksz;

	pksz = ser->maxrtrans;
	ntotcp = 0;

	for(i = 0; i < sz; i+= pksz){
		ftuseinhdr(port, in + i);
		if(sz - i > pksz)
			ncp = pksz - ser->inhdrsz;
		else
			ncp = sz - i - ser->inhdrsz;
		memmove(out, in + i + ser->inhdrsz, ncp);
		out += ncp;
		ntotcp += ncp;
	}
	return ntotcp;
}

static void
epreader(void *u)
{
	int dfd, rcount, cl, ntries, recov;
	char err[40];
	Areader *a;
	Channel *c;
	Packser *pk;
	Serial *ser;
	Serialport *p;

	threadsetname("epreader proc");
	a = u;
	p = a->p;
	ser = p->s;
	c = a->c;
	free(a);

	qlock(ser);	/* this makes the reader wait end of initialization too */
	dfd = p->epin->dfd;
	qunlock(ser);

	ntries = 0;
	pk = nil;
	do {
		if (pk == nil)
			pk = emallocz(sizeof(Packser), 1);
Eagain:
		rcount = read(dfd, pk->b, sizeof pk->b);
		if(serialdebug > 5)
			dsprint(2, "%d %#ux%#ux ", rcount, p->data[0],
				p->data[1]);

		if(rcount < 0){
			if(ntries++ > 100)
				break;
			qlock(ser);
			recov = serialrecover(ser, p, nil, "epreader: bulkin error");
			qunlock(ser);
			if(recov >= 0)
				goto Eagain;
		}
		if(rcount == 0)
			continue;
		if(rcount >= ser->inhdrsz){
			rcount = cpdata(ser, p, pk->b, pk->b, rcount);
			if(rcount != 0){
				pk->nb = rcount;
				cl = sendp(c, pk);
				if(cl < 0){
					/*
					 * if it was a time-out, I don't want
					 * to give back an error.
					 */
					rcount = 0;
					break;
				}
			}else
				free(pk);
			qlock(ser);
			ser->recover = 0;
			qunlock(ser);
			ntries = 0;
			pk = nil;
		}
	} while(rcount >= 0 || (rcount < 0 && strstr(err, "timed out") != nil));

	if(rcount < 0)
		fprint(2, "%s: error reading %s: %r\n", argv0, p->fs.name);
	free(pk);
	nbsendp(c, nil);
	if(p->w4data != nil)
		chanclose(p->w4data);
	if(p->gotdata != nil)
		chanclose(p->gotdata);
	devctl(ser->dev, "detach");
	closedev(ser->dev);
	usbfsdel(&p->fs);
}

static void
statusreader(void *u)
{
	Areader *a;
	Channel *c;
	Packser *pk;
	Serialport *p;
	Serial *ser;
	int cl;

	p = u;
	ser = p->s;
	threadsetname("statusreader thread");
	/* big buffering, fewer bytes lost */
	c = chancreate(sizeof(Packser *), 128);
	a = emallocz(sizeof(Areader), 1);
	a->p = p;
	a->c = c;
	incref(ser->dev);
	proccreate(epreader, a, 16*1024);

	while((pk = recvp(c)) != nil){
		memmove(p->data, pk->b, pk->nb);
		p->ndata = pk->nb;
		free(pk);
		dsprint(2, "serial %p: status reader %d \n", p, p->ndata);
		/* consume it all */
		while(p->ndata != 0){
			dsprint(2, "serial %p: status reader to consume: %d\n",
				p, p->ndata);
			cl = recvul(p->w4data);
			if(cl  < 0)
				break;
			cl = sendul(p->gotdata, 1);
			if(cl  < 0)
				break;
		}
	}

	shutdownchan(c);
	devctl(ser->dev, "detach");
	closedev(ser->dev);
	usbfsdel(&p->fs);
}

static int
ftreset(Serial *ser, Serialport *p)
{
	int i;

	if(p != nil){
		ftdiwrite(p, FTRESETCTLVAL, 0, FTRESET);
		return 0;
	}
	p = ser->p;
	for(i = 0; i < Maxifc; i++)
		if(p[i].s != nil)
			ftdiwrite(&p[i], FTRESETCTLVAL, 0, FTRESET);
	return 0;
}

static int
ftinit(Serialport *p)
{
	Serial *ser;
	uint timerval;
	int res;

	ser = p->s;
	if(p->isjtag){
		res = ftdiwrite(p, FTSETFLOWCTRL, 0, FTDISABLEFLOWCTRL);
		if(res < 0)
			return -1;
		res = ftdiread(p, FTSETLATENCYTIMER, 0, (uchar *)&timerval,
			FTLATENCYTIMERSZ);
		if(res < 0)
			return -1;
		dsprint(2, "serial: jtag latency timer is %d\n", timerval);
		timerval = 2;
		ftdiwrite(p, FTLATENCYDEFAULT, 0, FTSETLATENCYTIMER);
		res = ftdiread(p, FTSETLATENCYTIMER, 0, (uchar *)&timerval,
			FTLATENCYTIMERSZ);
		if(res < 0)
			return -1;

		dsprint(2, "serial: jtag latency timer set to %d\n", timerval);
		/* may be unnecessary */
		devctl(p->epin,  "timeout 5000");
		devctl(p->epout, "timeout 5000");
		/* 0xb is the mask for lines. plug dependant? */
		ftdiwrite(p, BMMPSSE|0x0b, 0, FTSETBITMODE);
	}
	incref(ser->dev);
	threadcreate(statusreader, p, 8*1024);
	return 0;
}

static int
ftsetbreak(Serialport *p, int val)
{
	return ftdiwrite(p, (val != 0? FTSETBREAK: 0), 0, FTSETDATA);
}

static int
ftclearpipes(Serialport *p)
{
	/* maybe can be done in one... */
	ftdiwrite(p, FTRESETCTLVALPURGETX, 0, FTRESET);
	ftdiwrite(p, FTRESETCTLVALPURGERX, 0, FTRESET);
	return 0;
}

static int
setctlline(Serialport *p, uchar val)
{
	return ftdiwrite(p, val | (val << 8), 0, FTSETMODEMCTRL);
}

static void
updatectlst(Serialport *p, int val)
{
	if(p->rts)
		p->ctlstate |= val;
	else
		p->ctlstate &= ~val;
}

static int
setctl(Serialport *p)
{
	int res;
	Serial *ser;

	ser = p->s;

	if(ser->dev->usb->vid == FTVid && ser->dev->usb->did ==  FTHETIRA1Did){
		fprint(2, "serial: cannot set lines for this device\n");
		updatectlst(p, CtlRTS|CtlDTR);
		p->rts = p->dtr = 1;
		return -1;
	}

	/* NB: you can not set DTR and RTS with one control message */
	updatectlst(p, CtlRTS);
	res = setctlline(p, (CtlRTS<<8)|p->ctlstate);
	if(res < 0)
		return res;

	updatectlst(p, CtlDTR);
	res = setctlline(p, (CtlDTR<<8)|p->ctlstate);
	if(res < 0)
		return res;

	return 0;
}

static int
ftsendlines(Serialport *p)
{
	int res;

	dsprint(2, "serial: sendlines: %#2.2x\n", p->ctlstate);
	res = setctl(p);
	dsprint(2, "serial: sendlines res: %d\n", res);
	return 0;
}

static int
ftseteps(Serialport *p)
{
	char *s;
	Serial *ser;

	ser = p->s;

	s = smprint("maxpkt %d", ser->maxrtrans);
	devctl(p->epin, s);
	free(s);

	s = smprint("maxpkt %d", ser->maxwtrans);
	devctl(p->epout, s);
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
