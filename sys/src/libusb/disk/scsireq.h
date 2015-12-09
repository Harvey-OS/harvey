/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * This is /sys/src/cmd/scuzz/scsireq.h
 * changed to add more debug support, and to keep
 * disk compiling without a scuzz that includes these changes.
 *
 * scsireq.h is also included by usb/disk and cdfs.
 */
typedef struct Umsc Umsc;

enum {					/* fundamental constants/defaults */
	MaxDirData	= 255,		/* max. direct data returned */
	/*
	 * Because we are accessed via devmnt, we can never get i/o counts
	 * larger than 8216 (Msgsize and devmnt's offered iounit) - 24
	 * (IOHDRSZ) = 8K.
	 */
	Maxiosize	= 8216 - IOHDRSZ, /* max. I/O transfer size */
};

typedef struct {
	uint8_t	*p;
	int32_t	count;
	uint8_t	write;
} ScsiPtr;

typedef struct {
	int	flags;
	char	*unit;			/* unit directory */
	int	lun;
	uint32_t	lbsize;
	uint64_t	offset;			/* in blocks of lbsize bytes */
	int	fd;
	Umsc	*umsc;			/* lun */
	ScsiPtr	cmd;
	ScsiPtr	data;
	int	status;			/* returned status */
	uint8_t	sense[MaxDirData];	/* returned sense data */
	uint8_t	inquiry[MaxDirData];	/* returned inquiry data */
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

/* scsi device types, from the scsi standards */
enum {
	Devdir,			/* usually disk */
	Devseq,			/* usually tape */
	Devprint,
	Dev3,
	Devworm,		/* also direct, but special */
	Devcd,			/* also direct */
	Dev6,
	Devmo,			/* also direct */
	Devjuke,
};

/* p arguments should be of type uint8_t* */
#define GETBELONG(p) ((uint32_t)(p)[0]<<24 | (uint32_t)(p)[1]<<16 | (p)[2]<<8 | (p)[3])
#define PUTBELONG(p, ul) ((p)[0] = (ul)>>24, (p)[1] = (ul)>>16, \
			  (p)[2] = (ul)>>8,  (p)[3] = (ul))
#define GETBE24(p)	((uint32_t)(p)[0]<<16 | (p)[1]<<8 | (p)[2])
#define PUTBE24(p, ul)	((p)[0] = (ul)>>16, (p)[1] = (ul)>>8, (p)[2] = (ul))

int32_t	SRready(ScsiReq*);
int32_t	SRrewind(ScsiReq*);
int32_t	SRreqsense(ScsiReq*);
int32_t	SRformat(ScsiReq*);
int32_t	SRrblimits(ScsiReq*, uint8_t*);
int32_t	SRread(ScsiReq*, void*, int32_t);
int32_t	SRwrite(ScsiReq*, void*, int32_t);
int32_t	SRseek(ScsiReq*, int32_t, int);
int32_t	SRfilemark(ScsiReq*, uint32_t);
int32_t	SRspace(ScsiReq*, uint8_t, int32_t);
int32_t	SRinquiry(ScsiReq*);
int32_t	SRmodeselect6(ScsiReq*, uint8_t*, int32_t);
int32_t	SRmodeselect10(ScsiReq*, uint8_t*, int32_t);
int32_t	SRmodesense6(ScsiReq*, uint8_t, uint8_t*, int32_t);
int32_t	SRmodesense10(ScsiReq*, uint8_t, uint8_t*, int32_t);
int32_t	SRstart(ScsiReq*, uint8_t);
int32_t	SRrcapacity(ScsiReq*, uint8_t*);
int32_t	SRrcapacity16(ScsiReq*, uint8_t*);

int32_t	SRblank(ScsiReq*, uint8_t, uint8_t);	/* MMC CD-R/CD-RW commands */
int32_t	SRsynccache(ScsiReq*);
int32_t	SRTOC(ScsiReq*, void*, int, uint8_t, uint8_t);
int32_t	SRrdiscinfo(ScsiReq*, void*, int);
int32_t	SRrtrackinfo(ScsiReq*, void*, int, int);

int32_t	SRcdpause(ScsiReq*, int);		/* MMC CD audio commands */
int32_t	SRcdstop(ScsiReq*);
int32_t	SRcdload(ScsiReq*, int, int);
int32_t	SRcdplay(ScsiReq*, int, int32_t, int32_t);
int32_t	SRcdstatus(ScsiReq*, uint8_t*, int);
int32_t	SRgetconf(ScsiReq*, uint8_t*, int);

/*	old CD-R/CD-RW commands */
int32_t	SRfwaddr(ScsiReq*, uint8_t, uint8_t, uint8_t, uint8_t*);
int32_t	SRtreserve(ScsiReq*, int32_t);
int32_t	SRtinfo(ScsiReq*, uint8_t, uint8_t*);
int32_t	SRwtrack(ScsiReq*, void*, int32_t, uint8_t, uint8_t);
int32_t	SRmload(ScsiReq*, uint8_t);
int32_t	SRfixation(ScsiReq*, uint8_t);

int32_t	SReinitialise(ScsiReq*);		/* CHANGER commands */
int32_t	SRestatus(ScsiReq*, uint8_t, uint8_t*, int);
int32_t	SRmmove(ScsiReq*, int, int, int, int);

int32_t	SRrequest(ScsiReq*);
int	SRclose(ScsiReq*);
int	SRopenraw(ScsiReq*, char*);
int	SRopen(ScsiReq*, char*);

void	makesense(ScsiReq*);

int32_t	umsrequest(struct Umsc*, ScsiPtr*, ScsiPtr*, int*);

void	scsidebug(int);

char*	scsierrmsg(int n);
