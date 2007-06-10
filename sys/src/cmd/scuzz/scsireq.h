typedef struct Umsc Umsc;
#pragma incomplete Umsc

enum {					/* fundamental constants/defaults */
	NTargetID	= 8,		/* number of target IDs */
	CtlrID		= 7,		/* default controller target ID */
	MaxDirData	= 255,		/* max. direct data returned */
	LBsize		= 512,		/* default logical-block size */
};

typedef struct {
	uchar	*p;
	long	count;
	uchar	write;
} ScsiPtr;

typedef struct {
	int	flags;
	char	*unit;			/* unit directory */
	int	lun;
	ulong	lbsize;
	ulong	offset;			/* in blocks of lbsize bytes */
	int	fd;
	Umsc	*umsc;			/* lun */
	ScsiPtr	cmd;
	ScsiPtr	data;
	int	status;			/* returned status */
	uchar	sense[MaxDirData];	/* returned sense data */
	uchar	inquiry[MaxDirData];	/* returned inquiry data */
	int	readblock;		/* flag: read a block since open */
} ScsiReq;

enum {					/* software flags */
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
	Fusb		= 0x0800,	/* USB transparent scsi */
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

enum {
	/* sense data byte 0 */
	Sd0valid	= 0x80,		/* valid sense data present */

	/* sense data byte 2 */
	/* incorrect-length indicator, difference in bytes 3—6 */
	Sd2ili		= 0x20,
	Sd2eom		= 0x40,		/* end of medium (tape) */
	Sd2filemark	= 0x80,		/* at a filemark (tape) */

	/* command byte 1 */
	Cmd1fixed	= 1,		/* use fixed-length blocks */
	Cmd1sili	= 2,		/* don't set Sd2ili */

	/* limit of block #s in 24-bit ccbs */
	Max24off	= (1<<21) - 1,	/* 2⁲ⁱ - 1 */

	/* mode pages */
	Allmodepages = 0x3F,
};

/* p arguments should be of type uchar* */
#define GETBELONG(p) ((ulong)(p)[0]<<24 | (ulong)(p)[1]<<16 | (p)[2]<<8 | (p)[3])
#define PUTBELONG(p, ul) ((p)[0] = (ul)>>24, (p)[1] = (ul)>>16, \
			  (p)[2] = (ul)>>8,  (p)[3] = (ul))
#define GETBE24(p)	((ulong)(p)[0]<<16 | (p)[1]<<8 | (p)[2])
#define PUTBE24(p, ul)	((p)[0] = (ul)>>16, (p)[1] = (ul)>>8, (p)[2] = (ul))

extern long maxiosize;

long	SRready(ScsiReq*);
long	SRrewind(ScsiReq*);
long	SRreqsense(ScsiReq*);
long	SRformat(ScsiReq*);
long	SRrblimits(ScsiReq*, uchar*);
long	SRread(ScsiReq*, void*, long);
long	SRwrite(ScsiReq*, void*, long);
long	SRseek(ScsiReq*, long, int);
long	SRfilemark(ScsiReq*, ulong);
long	SRspace(ScsiReq*, uchar, long);
long	SRinquiry(ScsiReq*);
long	SRmodeselect6(ScsiReq*, uchar*, long);
long	SRmodeselect10(ScsiReq*, uchar*, long);
long	SRmodesense6(ScsiReq*, uchar, uchar*, long);
long	SRmodesense10(ScsiReq*, uchar, uchar*, long);
long	SRstart(ScsiReq*, uchar);
long	SRrcapacity(ScsiReq*, uchar*);

long	SRblank(ScsiReq*, uchar, uchar);	/* MMC CD-R/CD-RW commands */
long	SRsynccache(ScsiReq*);
long	SRTOC(ScsiReq*, void*, int, uchar, uchar);
long	SRrdiscinfo(ScsiReq*, void*, int);
long	SRrtrackinfo(ScsiReq*, void*, int, int);

long	SRcdpause(ScsiReq*, int);		/* MMC CD audio commands */
long	SRcdstop(ScsiReq*);
long	SRcdload(ScsiReq*, int, int);
long	SRcdplay(ScsiReq*, int, long, long);
long	SRcdstatus(ScsiReq*, uchar*, int);
long	SRgetconf(ScsiReq*, uchar*, int);

/*	old CD-R/CD-RW commands */
long	SRfwaddr(ScsiReq*, uchar, uchar, uchar, uchar*);
long	SRtreserve(ScsiReq*, long);
long	SRtinfo(ScsiReq*, uchar, uchar*);
long	SRwtrack(ScsiReq*, void*, long, uchar, uchar);
long	SRmload(ScsiReq*, uchar);
long	SRfixation(ScsiReq*, uchar);

long	SReinitialise(ScsiReq*);		/* CHANGER commands */
long	SRestatus(ScsiReq*, uchar, uchar*, int);
long	SRmmove(ScsiReq*, int, int, int, int);

long	SRrequest(ScsiReq*);
int	SRclose(ScsiReq*);
int	SRopenraw(ScsiReq*, char*);
int	SRopen(ScsiReq*, char*);

void	makesense(ScsiReq*);

long	umsrequest(struct Umsc*, ScsiPtr*, ScsiPtr*, int*);
