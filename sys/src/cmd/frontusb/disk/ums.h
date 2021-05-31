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
	ulong mode;
	char	*name;
	vlong offset;		/* in lbsize units */
	vlong length;		/* in lbsize units */
};


struct Umsc
{
	ScsiReq;
	char name[40];
	uvlong	blocks;
	vlong	capacity;

	/* from setup */
	char	*bufp;
	long	off;		/* offset within a block */
	long	nb;		/* byte count */

	QLock;

	/* partitions */
	Part part[Maxparts];

	uchar 	rawcmd[16];
	uchar	phase;
	char	*inq;
	Ums	*ums;
	char	buf[Maxiosize];
};

struct Ums
{
	Dev	*epin;
	Dev	*epout;
	Umsc	*lun;
	uchar	maxlun;
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
	long	tag;
	long	datalen;
	uchar	flags;
	uchar	lun;
	uchar	len;
	char	command[16];
};

struct Csw
{
	char	signature[4];		/* "USBS" */
	long	tag;
	long	dataresidue;
	uchar	status;
};
