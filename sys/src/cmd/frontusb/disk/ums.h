/*
 * mass storage transport protocols and subclasses,
 * from usb mass storage class specification overview rev 1.2
 */

typedef struct Umsc Umsc;
typedef struct Ums Ums;
typedef struct Cbw Cbw;			/* command block wrapper */
typedef struct Csw Csw;			/* command status wrapper */
typedef struct Part Part;

enum
{
	Protocbi =	0,	/* control/bulk/interrupt; mainly floppies */
	Protocb =	1,	/*   "  with no interrupt; mainly floppies */
	Protobulk =	0x50,	/* bulk only */
	Protouas =	0x62,	/* USB-attached SCSI */

	Subrbc =	1,	/* reduced blk cmds */
	Subatapi =	2,	/* cd/dvd using sff-8020i or mmc-2 cmd blks */
	Subqic 	=	3,	/* QIC-157 tapes */
	Subufi =	4,	/* floppy */
	Sub8070 =	5,	/* removable media, atapi-like */
	Subscsi =	6,	/* scsi transparent cmd set */
	Subisd200 =	7,	/* ISD200 ATA */
	Subdev =	0xff,	/* use device's value */

	Umsreset =	0xFF,
	Getmaxlun =	0xFE,

//	Maxlun		= 256,
	Maxlun		= 32,

	CMreset = 1,

	Pcmd = 0,
	Pdata,
	Pstatus,

	CbwLen		= 31,
	CbwDataIn	= 0x80,
	CbwDataOut	= 0x00,
	CswLen		= 13,
	CswOk		= 0,
	CswFailed	= 1,
	CswPhaseErr	= 2,
	
	Maxparts		= 16,
};

/*
 * corresponds to a lun.
 * these are ~600+Maxiosize bytes each; ScsiReq is not tiny.
 */

struct Part
{
	int id;
	int inuse;
	int vers;
	u32 mode;
	char	*name;
	i64 offset;		/* in lbsize units */
	i64 length;		/* in lbsize units */
};


struct Umsc
{
	ScsiReq;
	char name[40];
	u64	blocks;
	i64	capacity;

	/* from setup */
	char	*bufp;
	i32	off;		/* offset within a block */
	i32	nb;		/* byte count */

	QLock;

	/* partitions */
	Part part[Maxparts];

	u8 	rawcmd[16];
	u8	phase;
	char	*inq;
	Ums	*ums;
	char	buf[Maxiosize];
};

struct Ums
{
	Dev	*epin;
	Dev	*epout;
	Umsc	*lun;
	u8	maxlun;
	int	seq;
	int	nerrs;
	int	wrongresidues;
};

/*
 * USB transparent SCSI devices
 */
struct Cbw
{
	char	signature[4];		/* "USBC" */
	i32	tag;
	i32	datalen;
	u8	flags;
	u8	lun;
	u8	len;
	char	command[16];
};

struct Csw
{
	char	signature[4];		/* "USBS" */
	i32	tag;
	i32	dataresidue;
	u8	status;
};
