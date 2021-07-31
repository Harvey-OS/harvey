enum {					/* fundamental constants/defaults */
	NTargetID	= 8,		/* number of target IDs */
	CtlrID		= 7,		/* default controller target ID */
	MaxDirData	= 255,		/* max. direct data returned */
	MaxIOsize	= /*32*512*/ 62*1024,	/* max. I/O size */
	LBsize		= 512,		/* default logical-block size */
};

typedef struct {
	int fd;
	uchar *p;
	long count;
	uchar write;
} ScsiPtr;

typedef struct {
	uchar flags;
	uchar status;			/* returned status */
	uchar id;			/* target id */
	ulong lbsize;
	ulong offset;
	ScsiPtr cmd;
	ScsiPtr data;
	uchar sense[MaxDirData];	/* returned sense data */
	uchar inquiry[MaxDirData];	/* returned inquiry data */
} ScsiReq;

enum {					/* flags */
	Fopen		= 0x01,		/* open */
	Fseqdev		= 0x02,		/* sequential-access device */
	Fwritten	= 0x04,		/* device written */
	Fronly		= 0x08,		/* device is read-only */
	Fwormdev	= 0x10,		/* write-once read-multiple device */
	Fprintdev	= 0x20,		/* printer */
};

enum {					/* status */
	Status_OK	= 0x00,
	Status_CC	= 0x02,		/* check condition */
	Status_BUSY	= 0x08,		/* device busy */
	Status_SD	= 0x80,		/* sense-data available */
	Status_NX	= 0x81,		/* select time-out */
	Status_HW	= 0x82,		/* hardware error */
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
	ScmdMselect	= 0x15,		/* mode select */
	ScmdMsense	= 0x1A,		/* mode sense */
	ScmdStart	= 0x1B,		/* start/stop unit */
	ScmdRcapacity	= 0x25,		/* read capacity */

	ScmdFlushcache	= 0x35,		/* flush cache */
	ScmdRdiscinfo	= 0x43,		/* read TOC data */
	ScmdFwaddr	= 0xE2,		/* first writeable address */
	ScmdTreserve	= 0xE4,		/* reserve track */
	ScmdTinfo	= 0xE5,		/* read track info */
	ScmdTwrite	= 0xE6,		/* write track */
	ScmdMload	= 0xE7,		/* medium load/unload */
	ScmdFixation	= 0xE9,		/* fixation */
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
extern long SRmodeselect(ScsiReq*, uchar*, long);
extern long SRmodesense(ScsiReq*, uchar, uchar*, long);
extern long SRstart(ScsiReq*, uchar);
extern long SRrcapacity(ScsiReq*, uchar*);

extern long SRflushcache(ScsiReq*);				/* WORM commands */
extern long SRrdiscinfo(ScsiReq*, void*, uchar, uchar);
extern long SRfwaddr(ScsiReq*, uchar, uchar, uchar, uchar*);
extern long SRtreserve(ScsiReq*, long);
extern long SRtinfo(ScsiReq*, uchar, uchar*);
extern long SRwtrack(ScsiReq*, void*, long, uchar, uchar);
extern long SRmload(ScsiReq*, uchar);
extern long SRfixation(ScsiReq*, uchar);

extern long SRrequest(ScsiReq*);
extern int SRclose(ScsiReq*);
extern int SRopenraw(ScsiReq*, int);
extern int SRopen(ScsiReq*, int);

extern void makesense(ScsiReq*);

extern int bus;
