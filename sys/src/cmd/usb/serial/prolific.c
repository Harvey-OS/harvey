#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"
#include "usbfs.h"
#include "serial.h"
#include "prolific.h"

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

int
plmatch(char *info)
{
	Cinfo *ip;
	char buf[50];

	for(ip = plinfo; ip->vid != 0; ip++){
		snprint(buf, sizeof buf, "vid %#06x did %#06x",
			ip->vid, ip->did);
		dsprint(2, "serial: %s %s\n", buf, info);
		if(strstr(info, buf) != nil)
			return 0;
	}
	return -1;
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
	dsprint(2, "serial: vendorread res:%d\n", res);
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
	dsprint(2, "serial: vendorwrite res:%d\n", res);
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
	dsprint(2, "serial: getparam res: %d\n", res);
	return res;
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
	plmodemctl(p, p->mctl);
	plgetparam(p);		/* make sure our state corresponds */

	dsprint(2, "serial: setparam res: %d\n", res);
	return res;
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
	st = emallocz(255, 1);
	qlock(ser);
	if(serialdebug)
		serdumpst(p, st, 255);
	dsprint(2, st);
	free(st);
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
	char err[40];
	uchar buf[VendorReqSz];
	Serial *ser;

	ser = p->s;

	qlock(ser);
	dsprint(2, "serial: reading from interrupt\n");
	dfd = p->epintr->dfd;

	qunlock(ser);
	nr = read(dfd, buf, sizeof buf);
	qlock(ser);
	snprint(err, sizeof err, "%r");
	dsprint(2, "serial: interrupt read %d %r\n", nr);

	if(nr < 0 && strstr(err, "timed out") == nil){
		dsprint(2, "serial: need to recover, status read %d %r\n", nr);
		if(serialrecover(ser, nil, nil, err) < 0){
			qunlock(ser);
			return -1;
		}
	}
	if(nr < 0)
		dsprint(2, "serial: reading status: %r");
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
	dsprint(2, "serial: finished read from interrupt %d\n", nr);
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

Serialops plops = {
	.init		= plinit,
	.getparam	= plgetparam,
	.setparam	= plsetparam,
	.clearpipes	= plclearpipes,
	.sendlines	= plsendlines,
	.modemctl	= plmodemctl,
	.setbreak	= plsetbreak,
	.seteps		= plseteps,
};
