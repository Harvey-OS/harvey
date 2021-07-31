enum {					/* fundamental constants/defaults */
	NTargetID	= 8,		/* number of target IDs */
	CtlrID		= 7,		/* default controller target ID */
	MaxDirData	= 255,		/* max. direct data returned */
//	MaxIOsize	= /*32*512*/ 96*1024,	/* max. I/O size */
	MaxIOsize	= 126*512,	/* max. I/O size (e.g. exabyte @ tar 126) */
	LBsize		= 512,		/* default logical-block size */
};

typedef struct {
	uchar *p;
	long count;
	uchar write;
} ScsiPtr;

typedef struct {
	int flags;
	char *unit;			/* unit directory */
	int lun;
	ulong lbsize;
	ulong offset;
	int fd;
	ScsiPtr cmd;
	ScsiPtr data;
	int status;			/* returned status */
	uchar sense[MaxDirData];	/* returned sense data */
	uchar inquiry[MaxDirData];	/* returned inquiry data */
} ScsiReq;

enum {					/* flags */
	Fopen		= 0x0001,	/* open */
	Fseqdev		= 0x0002,	/* sequential-access device */
	Fwritten	= 0x0004,	/* device written */
	Fronly		= 0x0008,	/* device is read-only */
	Fwormdev	= 0x0010,	/* write-once read-multiple device */
	Fprintdev	= 0x0020,	/* printer */
	Fbfixed		= 0x0040,	/* fixed block size */
	Fchanger	= 0x0080,	/* medium-changer device */
	Finqok		= 0x0100,	/* inquiry data is OK */
	Fmode6		= 0x0200,	/* use 6-byte modeselect */
	Frw10		= 0x0400,	/* use 10-byte read/write */
};

enum {
	STnomem		=-4,		/* buffer allocation failed */
	STharderr	=-3,		/* controller error of some kind */
	STtimeout	=-2,		/* bus timeout */
	STok		= 0,		/* good */
	STcheck		= 0x02,		/* check condition */
	STcondmet	= 0x04,		/* condition met/good */
	STbusy		= 0x08,		/* busy */
	STintok		= 0x10,		/* intermediate/good */
	STintcondmet	= 0x14,		/* intermediate/condition met/good */
	STresconf	= 0x18,		/* reservation conflict */
	STterminated	= 0x22,		/* command terminated */
	STqfull		= 0x28,		/* queue full */
};

enum {					/* status */
	Status_SD	= 0x80,		/* sense-data available */
	Status_SW	= 0x83,		/* internal software error */
	Status_BADARG	= 0x84,		/* bad argument to request */
	Status_RO	= 0x85,		/* device is read-only */
};

enum {					/* SCSI command codes */
	ScmdTur		= 0x00,		/* test unit ready */
	ScmdRewind	= 0x01,		/* rezero/rewind */
	ScmdRsense	= 0x03,		/* request sense */
	ScmdFormat	= 0x04,		/* format unit */
	ScmdRblimits	= 0x05,		/* read block limits */
	ScmdRead	= 0x08,		/* read */
	ScmdWrite	= 0x0A,		/* write */
	ScmdSeek	= 0x0B,		/* seek */
	ScmdFmark	= 0x10,		/* write filemarks */
	ScmdSpace	= 0x11,		/* space forward/backward */
	ScmdInq		= 0x12,		/* inquiry */
	ScmdMselect6	= 0x15,		/* mode select */
	ScmdMselect10	= 0x55,		/* mode select */
	ScmdMsense6	= 0x1A,		/* mode sense */
	ScmdMsense10	= 0x5A,		/* mode sense */
	ScmdStart	= 0x1B,		/* start/stop unit */
	ScmdRcapacity	= 0x25,		/* read capacity */
	ScmdExtread	= 0x28,		/* extended read */
	ScmdExtwrite	= 0x2A,		/* extended write */
	ScmdExtseek	= 0x2B,		/* extended seek */

	ScmdSynccache	= 0x35,		/* flush cache */
	ScmdRTOC	= 0x43,		/* read TOC data */
	ScmdRdiscinfo	= 0x51,		/* read disc information */
	ScmdRtrackinfo	= 0x52,		/* read track information */
	ScmdBlank	= 0xA1,		/* blank CD-RW media */

	ScmdCDpause	= 0x4B,		/* pause/resume */
	ScmdCDstop	= 0x4E,		/* stop play/scan */
	ScmdCDplay	= 0xA5,		/* play audio */
	ScmdCDload	= 0xA6,		/* load/unload */
	ScmdCDscan	= 0xBA,		/* fast forward/reverse */
	ScmdCDstatus	= 0xBD,		/* mechanism status */
	Scmdgetconf	= 0x46,		/* get configuration */

	ScmdFwaddr	= 0xE2,		/* first writeable address */
	ScmdTreserve	= 0xE4,		/* reserve track */
	ScmdTinfo	= 0xE5,		/* read track info */
	ScmdTwrite	= 0xE6,		/* write track */
	ScmdMload	= 0xE7,		/* medium load/unload */
	ScmdFixation	= 0xE9,		/* fixation */

	ScmdEInitialise	= 0x07,		/* initialise element status */
	ScmdMMove	= 0xA5,		/* move medium */
	ScmdEStatus	= 0xB8,		/* read element status */
	ScmdMExchange	= 0xA6,		/* exchange medium */
	ScmdEposition	= 0x2B,		/* position to element */

	ScmdReadDVD	= 0xAD,		/* read dvd structure */
	ScmdReportKey	= 0xA4,		/* read dvd key */
	ScmdSendKey	= 0xA3,		/* write dvd key */
};

extern long SRready(ScsiReq*);
extern long SRrewind(ScsiReq*);
extern long SRreqsense(ScsiReq*);
extern long SRformat(ScsiReq*);
extern long SRrblimits(ScsiReq*, uchar*);
extern long SRread(ScsiReq*, void*, long);
extern long SRwrite(ScsiReq*, void*, long);
extern long SRseek(ScsiReq*, long, int);
extern long SRfilemark(ScsiReq*, ulong);
extern long SRspace(ScsiReq*, uchar, long);
extern long SRinquiry(ScsiReq*);
extern long SRmodeselect6(ScsiReq*, uchar*, long);
extern long SRmodeselect10(ScsiReq*, uchar*, long);
extern long SRmodesense6(ScsiReq*, uchar, uchar*, long);
extern long SRmodesense10(ScsiReq*, uchar, uchar*, long);
extern long SRstart(ScsiReq*, uchar);
extern long SRrcapacity(ScsiReq*, uchar*);

extern long SRblank(ScsiReq*, uchar, uchar);	/* MMC CD-R/CD-RW commands */
extern long SRsynccache(ScsiReq*);
extern long SRTOC(ScsiReq*, void*, int, uchar, uchar);
extern long SRrdiscinfo(ScsiReq*, void*, int);
extern long SRrtrackinfo(ScsiReq*, void*, int, int);

extern long SRcdpause(ScsiReq*, int);		/* MMC CD audio commands */
extern long SRcdstop(ScsiReq*);
extern long SRcdload(ScsiReq*, int, int);
extern long SRcdplay(ScsiReq*, int, long, long);
extern long SRcdstatus(ScsiReq*, uchar*, int);
extern long SRgetconf(ScsiReq*, uchar*, int);

extern long SRfwaddr(ScsiReq*, uchar, uchar, uchar, uchar*);	/* old CD-R/CD-RW commands */
extern long SRtreserve(ScsiReq*, long);
extern long SRtinfo(ScsiReq*, uchar, uchar*);
extern long SRwtrack(ScsiReq*, void*, long, uchar, uchar);
extern long SRmload(ScsiReq*, uchar);
extern long SRfixation(ScsiReq*, uchar);

extern long SReinitialise(ScsiReq*);		/* CHANGER commands */
extern long SRestatus(ScsiReq*, uchar, uchar*, int);
extern long SRmmove(ScsiReq*, int, int, int, int);

extern long SRrequest(ScsiReq*);
extern int  SRclose(ScsiReq*);
extern int  SRopenraw(ScsiReq*, char*);
extern int  SRopen(ScsiReq*, char*);

extern void makesense(ScsiReq*);

