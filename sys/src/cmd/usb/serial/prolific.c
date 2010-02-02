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
vendorread(Serial *ser, int val, int index, uchar *buf)
{
	int res;

	dsprint(2, "serial: vendorread val: 0x%x idx:%d buf:%p\n",
		val, index, buf);
	res = usbcmd(ser->dev,  Rd2h | Rvendor | Rdev, VendorReadReq,
		val, index, buf, 1);
	dsprint(2, "serial: vendorread res:%d\n", res);
	return res;
}

static int
vendorwrite(Serial *ser, int val, int index)
{
	int res;

	dsprint(2, "serial: vendorwrite val: 0x%x idx:%d\n", val, index);
	res = usbcmd(ser->dev, Rh2d | Rvendor | Rdev, VendorWriteReq,
		val, index, nil, 0);
	dsprint(2, "serial: vendorwrite res:%d\n", res);
	return res;
}

/* BUG: I could probably read Dcr0 and set only the bits */
static int
plmodemctl(Serial *ser, int set)
{
	if(set == 0){
		ser->mctl = 0;
		vendorwrite(ser, Dcr0Idx|DcrSet, Dcr0Init);
		return 0;
	}

	ser->mctl = 1;
	if(ser->type == TypeHX)
		vendorwrite(ser, Dcr0Idx|DcrSet, Dcr0Init|Dcr0HwFcX);
	else
		vendorwrite(ser, Dcr0Idx|DcrSet, Dcr0Init|Dcr0HwFcH);
	return 0;
}

static int
plgetparam(Serial *ser)
{
	uchar buf[ParamReqSz];
	int res;

	res = usbcmd(ser->dev, Rd2h | Rclass | Riface, GetLineReq,
		0, 0, buf, sizeof buf);
	ser->baud = GET4(buf);

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
		ser->stop = 1;
	else if(buf[4] == 2)
		ser->stop = 2;
	ser->parity = buf[5];
	ser->bits = buf[6];

	dsprint(2, "serial: getparam: ");
	if(serialdebug)
		dumpbuf(buf, sizeof buf);
	dsprint(2, "serial: getparam res: %d\n", res);
	return res;
}

static int
plsetparam(Serial *ser)
{
	uchar buf[ParamReqSz];
	int res;

	PUT4(buf, ser->baud);

	if(ser->stop == 1)
		buf[4] = 0;
	else if(ser->stop == 2)
		buf[4] = 2; 			/* see comment in getparam */
	buf[5] = ser->parity;
	buf[6] = ser->bits;

	dsprint(2, "serial: setparam: ");
	if(serialdebug)
		dumpbuf(buf, sizeof buf);
	res = usbcmd(ser->dev, Rh2d | Rclass | Riface, SetLineReq,
		0, 0, buf, sizeof buf);
	plmodemctl(ser, ser->mctl);
	plgetparam(ser);		/* make sure our state corresponds */

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
plinit(Serial *ser)
{
	char *st;
	uchar *buf;
	ulong csp, maxpkt, dno;

	buf = emallocz(VendorReqSz, 1);
	qlock(ser);
	serialreset(ser);

	csp = ser->dev->usb->csp;
	maxpkt = ser->dev->maxpkt;
	dno = ser->dev->usb->dno;

	if((ser->type = revid(dno)) == TypeUnk)
		ser->type = heuristicid(csp, maxpkt);

	dsprint(2, "serial: type %d\n", ser->type);

	vendorread(ser, 0x8484, 0, buf);
	vendorwrite(ser, 0x0404, 0);
	vendorread(ser, 0x8484, 0, buf);
	vendorread(ser, 0x8383, 0, buf);
	vendorread(ser, 0x8484, 0, buf);
	vendorwrite(ser, 0x0404, 1);
	vendorread(ser, 0x8484, 0, buf);
	vendorread(ser, 0x8383, 0, buf);

	vendorwrite(ser, Dcr0Idx|DcrSet, Dcr0Init);
	vendorwrite(ser, Dcr1Idx|DcrSet, Dcr1Init);

	if(ser->type == TypeHX)
		vendorwrite(ser, Dcr2Idx|DcrSet, Dcr2InitX);
	else
		vendorwrite(ser, Dcr2Idx|DcrSet, Dcr2InitH);

	plgetparam(ser);
	qunlock(ser);
	free(buf);
	st = emallocz(255, 1);
	qlock(ser);
	if(serialdebug)
		serdumpst(ser, st, 255);
	dsprint(2, st);
	qunlock(ser);
	free(st);
	/* ser gets freed by closedev, the process has a reference */
	incref(ser->dev);
	proccreate(statusreader, ser, 8*1024);
	return 0;
}

static int
plsetbreak(Serial *ser, int val)
{
	return usbcmd(ser->dev, Rh2d | Rclass | Riface,
		(val != 0? BreakOn: BreakOff), val, 0, nil, 0);
}

static int
plclearpipes(Serial *ser)
{
	if(ser->type == TypeHX){
		vendorwrite(ser, PipeDSRst, 0);
		vendorwrite(ser, PipeUSRst, 0);
	}else{
		if(unstall(ser->dev, ser->epout, Eout) < 0)
			dprint(2, "disk: unstall epout: %r\n");
		if(unstall(ser->dev, ser->epin, Ein) < 0)
			dprint(2, "disk: unstall epin: %r\n");
		if(unstall(ser->dev, ser->epintr, Ein) < 0)
			dprint(2, "disk: unstall epintr: %r\n");
	}
	return 0;
}

static int
setctlline(Serial *ser, uchar val)
{
	return usbcmd(ser->dev, Rh2d | Rclass | Riface, SetCtlReq,
		val, 0, nil, 0);
}

static void
composectl(Serial *ser)
{
	if(ser->rts)
		ser->ctlstate |= CtlRTS;
	else
		ser->ctlstate &= ~CtlRTS;
	if(ser->dtr)
		ser->ctlstate |= CtlDTR;
	else
		ser->ctlstate &= ~CtlDTR;
}

int
plsendlines(Serial *ser)
{
	int res;

	dsprint(2, "serial: sendlines: %#2.2x\n", ser->ctlstate);
	composectl(ser);
	res = setctlline(ser, ser->ctlstate);
	dsprint(2, "serial: sendlines res: %d\n", res);
	return 0;
}

static int
plreadstatus(Serial *ser)
{
	int nr, dfd;
	char err[40];
	uchar buf[VendorReqSz];

	qlock(ser);
	dsprint(2, "serial: reading from interrupt\n");
	dfd = ser->epintr->dfd;

	qunlock(ser);
	nr = read(dfd, buf, sizeof buf);
	qlock(ser);
	snprint(err, sizeof err, "%r");
	dsprint(2, "serial: interrupt read %d %r\n", nr);

	if(nr < 0 && strstr(err, "timed out") == nil){
		dsprint(2, "serial: need to recover, status read %d %r\n", nr);
		if(serialrecover(ser, err) < 0){
			qunlock(ser);
			return -1;
		}
	}
	if(nr < 0)
		dsprint(2, "serial: reading status: %r");
	else if(nr >= sizeof buf - 1){
		ser->dcd = buf[8] & DcdStatus;
		ser->dsr = buf[8] & DsrStatus;
		ser->cts = buf[8] & BreakerrStatus;
		ser->ring = buf[8] & RingStatus;
		ser->cts = buf[8] & CtsStatus;
		if(buf[8] & FrerrStatus)
			ser->nframeerr++;
		if(buf[8] & ParerrStatus)
			ser->nparityerr++;
		if(buf[8] & OvererrStatus)
			ser->novererr++;
	} else
		dsprint(2, "serial: bad status read %d\n", nr);
	dsprint(2, "serial: finished read from interrupt %d\n", nr);
	qunlock(ser);
	return 0;
}

static void
statusreader(void *u)
{
	Serial *ser;

	ser = u;
	threadsetname("statusreaderproc");
	while(plreadstatus(ser) >= 0)
		;
	fprint(2, "serial: statusreader exiting\n");
	closedev(ser->dev);
}

/*
 * Maximum number of bytes transferred per frame
 * The output buffer size cannot be increased due to the size encoding
 */

static int
plseteps(Serial *ser)
{
	devctl(ser->epin,  "maxpkt 256");
	devctl(ser->epout, "maxpkt 256");
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
