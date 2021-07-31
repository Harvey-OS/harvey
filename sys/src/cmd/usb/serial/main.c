#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"
#include "usbfs.h"
#include "serial.h"
#include "prolific.h"

typedef struct Parg Parg;

enum {
	Arglen = 80,
};

Cinfo cinfo[] = {
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

static void
usage(void)
{
	fprint(2, "usage: %s [-d] [-a n] [dev...]\n", argv0);
	threadexitsall("usage");
}

static int
matchserial(char *info, void*)
{
	Cinfo *ip;
	char buf[50];

	for(ip = cinfo; ip->vid != 0; ip++){
		snprint(buf, sizeof buf, "vid %#06x did %#06x",
			ip->vid, ip->did);
		dsprint(2, "serial: %s %s", buf, info);
		if(strstr(info, buf) != nil)
			return 0;
	}
	return -1;
}

void
threadmain(int argc, char **argv)
{
	char *mnt, *srv, *as, *ae;
	char args[Arglen];

	mnt = "/dev";
	srv = nil;

	quotefmtinstall();
	ae = args + sizeof args;
	as = seprint(args, ae, "serial");
	ARGBEGIN{
	case 'D':
		usbfsdebug++;
		break;
	case 'd':
		usbdebug++;
		as = seprint(as, ae, " -d");
		break;
	case 'm':
		mnt = EARGF(usage());
		break;
	case 's':
		srv = EARGF(usage());
		break;
	default:
		usage();
		break;
	}ARGEND;

	rfork(RFNOTEG);
	fmtinstall('U', Ufmt);
	threadsetgrp(threadid());

	usbfsinit(srv, mnt, &usbdirfs, MAFTER|MCREATE);
	startdevs(args, argv, argc, matchserial, nil, serialmain);
	threadexits(nil);
}
