#include <u.h>
#include <libc.h>
#include <thread.h>
#include <fcall.h>
#include <9p.h>
#include "usb.h"
#include "serial.h"

enum {
	/* flavours of the device */
	TypeH,
	TypeHX,
	TypeUnk,

	RevH		= 0x0202,
	RevX		= 0x0300,
	RevHX		= 0x0400,
	Rev1		= 0x0001,

	/* usbcmd parameters */
	SetLineReq	= 0x20,

	SetCtlReq	= 0x22,

	BreakReq	= 0x23,
	BreakOn		= 0xffff,
	BreakOff	= 0x0000,

	GetLineReq	= 0x21,

	VendorWriteReq	= 0x01,		/* BUG: is this a standard request? */
	VendorReadReq	= 0x01,

	ParamReqSz	= 7,
	VendorReqSz	= 10,

	/* status read from interrupt endpoint */
	DcdStatus	= 0x01,
	DsrStatus	= 0x02,
	BreakerrStatus	= 0x04,
	RingStatus	= 0x08,
	FrerrStatus	= 0x10,
	ParerrStatus	= 0x20,
	OvererrStatus	= 0x40,
	CtsStatus	= 0x80,

	DcrGet		= 0x80,
	DcrSet		= 0x00,

	Dcr0Idx		= 0x00,

	Dcr0Init	= 0x0001,
	Dcr0HwFcH	= 0x0040,
	Dcr0HwFcX	= 0x0060,

	Dcr1Idx		= 0x01,

	Dcr1Init	= 0x0000,
	Dcr1InitH	= 0x0080,
	Dcr1InitX	= 0x0000,

	Dcr2Idx		= 0x02,

	Dcr2InitH	= 0x0024,
	Dcr2InitX	= 0x0044,

	PipeDSRst	= 0x08,
	PipeUSRst	= 0x09,

};

enum {
	PL2303Vid	= 0x067b,
	PL2303Did	= 0x2303,
	PL2303DidRSAQ2	= 0x04bb,
	PL2303DidDCU11	= 0x1234,
	PL2303DidPHAROS	= 0xaaa0,
	PL2303DidRSAQ3	= 0xaaa2,
	PL2303DidALDIGA	= 0x0611,
	PL2303DidMMX	= 0x0612,
	PL2303DidGPRS	= 0x0609,

	ATENVid		= 0x0557,
	ATENVid2	= 0x0547,
	ATENDid		= 0x2008,

	IODATAVid	= 0x04bb,
	IODATADid	= 0x0a03,
	IODATADidRSAQ5	= 0x0a0e,

	ELCOMVid	= 0x056e,
	ELCOMDid	= 0x5003,
	ELCOMDidUCSGT	= 0x5004,

	ITEGNOVid	= 0x0eba,
	ITEGNODid	= 0x1080,
	ITEGNODid2080	= 0x2080,

	MA620Vid	= 0x0df7,
	MA620Did	= 0x0620,

	RATOCVid	= 0x0584,
	RATOCDid	= 0xb000,

	TRIPPVid	= 0x2478,
	TRIPPDid	= 0x2008,

	RADIOSHACKVid	= 0x1453,
	RADIOSHACKDid	= 0x4026,

	DCU10Vid	= 0x0731,
	DCU10Did	= 0x0528,

	SITECOMVid	= 0x6189,
	SITECOMDid	= 0x2068,

	 /* Alcatel OT535/735 USB cable */
	ALCATELVid	= 0x11f7,
	ALCATELDid	= 0x02df,

	/* Samsung I330 phone cradle */
	SAMSUNGVid	= 0x04e8,
	SAMSUNGDid	= 0x8001,

	SIEMENSVid	= 0x11f5,
	SIEMENSDidSX1	= 0x0001,
	SIEMENSDidX65	= 0x0003,
	SIEMENSDidX75	= 0x0004,
	SIEMENSDidEF81	= 0x0005,

	SYNTECHVid	= 0x0745,
	SYNTECHDid	= 0x0001,

	/* Nokia CA-42 Cable */
	NOKIACA42Vid	= 0x078b,
	NOKIACA42Did	= 0x1234,

	/* CA-42 CLONE Cable www.ca-42.com chipset: Prolific Technology Inc */
	CA42CA42Vid	= 0x10b5,
	CA42CA42Did	= 0xac70,

	SAGEMVid	= 0x079b,
	SAGEMDid	= 0x0027,

	/* Leadtek GPS 9531 (ID 0413:2101) */
	LEADTEKVid	= 0x0413,
	LEADTEK9531Did	= 0x2101,

	 /* USB GSM cable from Speed Dragon Multimedia, Ltd */
	SPEEDDRAGONVid	= 0x0e55,
	SPEEDDRAGONDid	= 0x110b,

	/* DATAPILOT Universal-2 Phone Cable */
	BELKINVid	= 0x050d,
	BELKINDid	= 0x0257,

	/* Belkin "F5U257" Serial Adapter */
	DATAPILOTU2Vid	= 0x0731,
	DATAPILOTU2Did	= 0x2003,

	ALCORVid	= 0x058F,
	ALCORDid	= 0x9720,

	/* Willcom WS002IN Data Driver (by NetIndex Inc.) */,
	WS002INVid	= 0x11f6,
	WS002INDid	= 0x2001,

	/* Corega CG-USBRS232R Serial Adapter */,
	COREGAVid	= 0x07aa,
	COREGADid	= 0x002a,

	/* Y.C. Cable U.S.A., Inc - USB to RS-232 */,
	YCCABLEVid	= 0x05ad,
	YCCABLEDid	= 0x0fba,

	/* "Superial" USB - Serial */,
	SUPERIALVid	= 0x5372,
	SUPERIALDid	= 0x2303,

	/* Hewlett-Packard LD220-HP POS Pole Display */,
	HPVid		= 0x03f0,
	HPLD220Did	= 0x3524,
};

Cinfo plinfo[] = {
	{ PL2303Vid,	PL2303Did },
	{ PL2303Vid,	PL2303DidRSAQ2 },
	{ PL2303Vid,	PL2303DidDCU11 },
	{ PL2303Vid,	PL2303DidRSAQ3 },
	{ PL2303Vid,	PL2303DidPHAROS },
	{ PL2303Vid,	PL2303DidALDIGA },
	{ PL2303Vid,	PL2303DidMMX },
	{ PL2303Vid,	PL2303DidGPRS },
	{ IODATAVid,	IODATADid },
	{ IODATAVid,	IODATADidRSAQ5 },
	{ ATENVid,	ATENDid },
	{ ATENVid2,	ATENDid },
	{ ELCOMVid,	ELCOMDid },
	{ ELCOMVid,	ELCOMDidUCSGT },
	{ ITEGNOVid,	ITEGNODid },
	{ ITEGNOVid,	ITEGNODid2080 },
	{ MA620Vid,	MA620Did },
	{ RATOCVid,	RATOCDid },
	{ TRIPPVid,	TRIPPDid },
	{ RADIOSHACKVid,RADIOSHACKDid },
	{ DCU10Vid,	DCU10Did },
	{ SITECOMVid,	SITECOMDid },
	{ ALCATELVid,	ALCATELDid },
	{ SAMSUNGVid,	SAMSUNGDid },
	{ SIEMENSVid,	SIEMENSDidSX1 },
	{ SIEMENSVid,	SIEMENSDidX65 },
	{ SIEMENSVid,	SIEMENSDidX75 },
	{ SIEMENSVid,	SIEMENSDidEF81 },
	{ SYNTECHVid,	SYNTECHDid },
	{ NOKIACA42Vid,	NOKIACA42Did },
	{ CA42CA42Vid,	CA42CA42Did },
	{ SAGEMVid,	SAGEMDid },
	{ LEADTEKVid,	LEADTEK9531Did },
	{ SPEEDDRAGONVid,SPEEDDRAGONDid },
	{ DATAPILOTU2Vid,DATAPILOTU2Did },
	{ BELKINVid,	BELKINDid },
	{ ALCORVid,	ALCORDid },
	{ WS002INVid,	WS002INDid },
	{ COREGAVid,	COREGADid },
	{ YCCABLEVid,	YCCABLEDid },
	{ SUPERIALVid,	SUPERIALDid },
	{ HPVid,	HPLD220Did },
	{ 0,		0 },
};

static Serialops plops;

int
plprobe(Serial *ser)
{
	Usbdev *ud = ser->dev->usb;

	if(matchid(plinfo, ud->vid, ud->did) == nil)
		return -1;
	ser->hasepintr = 1;
	ser->Serialops = plops;
	return 0;
}

static void	statusreader(void *u);

static void
dumpbuf(uchar *buf, int bufsz)
{
	int i;

	for(i=0; i<bufsz; i++)
		print("buf[%d]=%#ux ", i, buf[i]);
	print("\n");
}

static int
vendorread(Serialport *p, int val, int index, uchar *buf)
{
	int res;
	Serial *ser;

	ser = p->s;

	dsprint(2, "serial: vendorread val: 0x%x idx:%d buf:%p\n",
		val, index, buf);
	res = usbcmd(ser->dev,  Rd2h | Rvendor | Rdev, VendorReadReq,
		val, index, buf, 1);
	if(res != 1) fprint(2, "serial: vendorread failed with res=%d\n", res);
	return res;
}

static int
vendorwrite(Serialport *p, int val, int index)
{
	int res;
	Serial *ser;

	ser = p->s;

	dsprint(2, "serial: vendorwrite val: 0x%x idx:%d\n", val, index);
	res = usbcmd(ser->dev, Rh2d | Rvendor | Rdev, VendorWriteReq,
		val, index, nil, 0);
	if(res != 8) fprint(2, "serial: vendorwrite failed with res=%d\n", res);
	return res;
}

/* BUG: I could probably read Dcr0 and set only the bits */
static int
plmodemctl(Serialport *p, int set)
{
	Serial *ser;

	ser = p->s;

	if(set == 0){
		p->mctl = 0;
		vendorwrite(p, Dcr0Idx|DcrSet, Dcr0Init);
		return 0;
	}

	p->mctl = 1;
	if(ser->type == TypeHX)
		vendorwrite(p, Dcr0Idx|DcrSet, Dcr0Init|Dcr0HwFcX);
	else
		vendorwrite(p, Dcr0Idx|DcrSet, Dcr0Init|Dcr0HwFcH);
	return 0;
}

static int
plgetparam(Serialport *p)
{
	uchar buf[ParamReqSz];
	int res;
	Serial *ser;

	ser = p->s;


	res = usbcmd(ser->dev, Rd2h | Rclass | Riface, GetLineReq,
		0, 0, buf, sizeof buf);
	if(res != ParamReqSz)
		memset(buf, 0, sizeof(buf));
	p->baud = GET4(buf);

	/*
	 * with the Pl9 interface it is not possible to set `1.5' as stop bits
	 * for the prologic:
	 *	0 is 1 stop bit
	 *	1 is 1.5 stop bits
	 *	2 is 2 stop bits
	 */
	if(buf[4] == 1)
		fprint(2, "warning, stop bit set to 1.5 unsupported");
	else if(buf[4] == 0)
		p->stop = 1;
	else if(buf[4] == 2)
		p->stop = 2;
	p->parity = buf[5];
	p->bits = buf[6];

	dsprint(2, "serial: getparam: ");
	if(serialdebug)
		dumpbuf(buf, sizeof buf);

	if(res == ParamReqSz)
		return 0;
	fprint(2, "serial: plgetparam failed with res=%d\n", res);
	if(res >= 0) werrstr("plgetparam failed with res=%d", res);
	return -1;
}

static int
plsetparam(Serialport *p)
{
	uchar buf[ParamReqSz];
	int res;
	Serial *ser;

	ser = p->s;

	PUT4(buf, p->baud);

	if(p->stop == 1)
		buf[4] = 0;
	else if(p->stop == 2)
		buf[4] = 2; 			/* see comment in getparam */
	buf[5] = p->parity;
	buf[6] = p->bits;

	dsprint(2, "serial: setparam: ");
	if(serialdebug)
		dumpbuf(buf, sizeof buf);
	res = usbcmd(ser->dev, Rh2d | Rclass | Riface, SetLineReq,
		0, 0, buf, sizeof buf);
	if(res != 8+ParamReqSz){
		fprint(2, "serial: plsetparam failed with res=%d\n", res);
		if(res >= 0) werrstr("plsetparam failed with res=%d", res);
		return -1;
	}
	plmodemctl(p, p->mctl);
	if(plgetparam(p) < 0)		/* make sure our state corresponds */
		return -1;

	return 0;
}

static int
revid(ulong devno)
{
	switch(devno){
	case RevH:
		return TypeH;
	case RevX:
	case RevHX:
	case Rev1:
		return TypeHX;
	default:
		return TypeUnk;
	}
}

/* linux driver says the release id is not always right */
static int
heuristicid(ulong csp, ulong maxpkt)
{
	if(Class(csp) == 0x02)
		return TypeH;
	else if(maxpkt == 0x40)
		return TypeHX;
	else if(Class(csp) == 0x00 || Class(csp) == 0xFF)
		return TypeH;
	else{
		fprint(2, "serial: chip unknown, setting to HX version\n");
		return TypeHX;
	}
}

static int
plinit(Serialport *p)
{
	char *st;
	uchar *buf;
	ulong csp, maxpkt, dno;
	Serial *ser;

	ser = p->s;
	buf = emallocz(VendorReqSz, 1);
	dsprint(2, "plinit\n");

	csp = ser->dev->usb->csp;
	maxpkt = ser->dev->maxpkt;
	dno = ser->dev->usb->dno;

	if((ser->type = revid(dno)) == TypeUnk)
		ser->type = heuristicid(csp, maxpkt);

	dsprint(2, "serial: type %d\n", ser->type);

	vendorread(p, 0x8484, 0, buf);
	vendorwrite(p, 0x0404, 0);
	vendorread(p, 0x8484, 0, buf);
	vendorread(p, 0x8383, 0, buf);
	vendorread(p, 0x8484, 0, buf);
	vendorwrite(p, 0x0404, 1);
	vendorread(p, 0x8484, 0, buf);
	vendorread(p, 0x8383, 0, buf);

	vendorwrite(p, Dcr0Idx|DcrSet, Dcr0Init);
	vendorwrite(p, Dcr1Idx|DcrSet, Dcr1Init);

	if(ser->type == TypeHX)
		vendorwrite(p, Dcr2Idx|DcrSet, Dcr2InitX);
	else
		vendorwrite(p, Dcr2Idx|DcrSet, Dcr2InitH);

	plgetparam(p);
	qunlock(ser);
	free(buf);
	qlock(ser);
	if(serialdebug){
		st = emallocz(255, 1);
		serdumpst(p, st, 255);
		dsprint(2, "%s", st);
		free(st);
	}
	/* p gets freed by closedev, the process has a reference */
	incref(ser->dev);
	proccreate(statusreader, p, 8*1024);
	return 0;
}

static int
plsetbreak(Serialport *p, int val)
{
	Serial *ser;

	ser = p->s;
	return usbcmd(ser->dev, Rh2d | Rclass | Riface,
		(val != 0? BreakOn: BreakOff), val, 0, nil, 0);
}

static int
plclearpipes(Serialport *p)
{
	Serial *ser;

	ser = p->s;

	if(ser->type == TypeHX){
		vendorwrite(p, PipeDSRst, 0);
		vendorwrite(p, PipeUSRst, 0);
	}else{
		if(unstall(ser->dev, p->epout, Eout) < 0)
			dprint(2, "disk: unstall epout: %r\n");
		if(unstall(ser->dev, p->epin, Ein) < 0)
			dprint(2, "disk: unstall epin: %r\n");
		if(unstall(ser->dev, p->epintr, Ein) < 0)
			dprint(2, "disk: unstall epintr: %r\n");
	}
	return 0;
}

static int
setctlline(Serialport *p, uchar val)
{
	Serial *ser;

	ser = p->s;
	return usbcmd(ser->dev, Rh2d | Rclass | Riface, SetCtlReq,
		val, 0, nil, 0);
}

static void
composectl(Serialport *p)
{
	if(p->rts)
		p->ctlstate |= CtlRTS;
	else
		p->ctlstate &= ~CtlRTS;
	if(p->dtr)
		p->ctlstate |= CtlDTR;
	else
		p->ctlstate &= ~CtlDTR;
}

static int
plsendlines(Serialport *p)
{
	int res;

	dsprint(2, "serial: sendlines: %#2.2x\n", p->ctlstate);
	composectl(p);
	res = setctlline(p, p->ctlstate);
	dsprint(2, "serial: sendlines res: %d\n", res);
	return 0;
}

static int
plreadstatus(Serialport *p)
{
	int nr, dfd;
	char err[ERRMAX];
	uchar buf[VendorReqSz];
	Serial *ser;

	ser = p->s;

	qlock(ser);
	dfd = p->epintr->dfd;
	qunlock(ser);
	nr = read(dfd, buf, sizeof buf);
	qlock(ser);
	rerrstr(err, sizeof err);
	if(nr < 0 && strstr(err, "timed out") == nil){
		if(serialrecover(ser, nil, nil, err) < 0){
			qunlock(ser);
			return -1;
		}
	}
	if(nr < 0)
		dsprint(2, "serial: reading status: %r\n");
	else if(nr >= sizeof buf - 1){
		p->dcd = buf[8] & DcdStatus;
		p->dsr = buf[8] & DsrStatus;
		p->cts = buf[8] & BreakerrStatus;
		p->ring = buf[8] & RingStatus;
		p->cts = buf[8] & CtsStatus;
		if(buf[8] & FrerrStatus)
			p->nframeerr++;
		if(buf[8] & ParerrStatus)
			p->nparityerr++;
		if(buf[8] & OvererrStatus)
			p->novererr++;
	} else
		dsprint(2, "serial: bad status read %d\n", nr);
	qunlock(ser);
	return 0;
}

static void
statusreader(void *u)
{
	Serialport *p;
	Serial *ser;

	p = u;
	ser = p->s;
	threadsetname("statusreaderproc");
	while(plreadstatus(p) >= 0)
		;
	fprint(2, "serial: statusreader exiting\n");
	closedev(ser->dev);
}

/*
 * Maximum number of bytes transferred per frame
 * The output buffer size cannot be increased due to the size encoding
 */

static int
plseteps(Serialport *p)
{
	devctl(p->epin,  "maxpkt 256");
	devctl(p->epout, "maxpkt 256");
	return 0;
}

static Serialops plops = {
	.init		= plinit,
	.getparam	= plgetparam,
	.setparam	= plsetparam,
	.clearpipes	= plclearpipes,
	.sendlines	= plsendlines,
	.modemctl	= plmodemctl,
	.setbreak	= plsetbreak,
	.seteps		= plseteps,
};
