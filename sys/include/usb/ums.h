/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * mass storage transport protocols and subclasses,
 * from usb mass storage class specification overview rev 1.2
 */

typedef struct Umsc Umsc;
typedef struct Ums Ums;
typedef struct Cbw Cbw;			/* command block wrapper */
typedef struct Csw Csw;			/* command status wrapper */

enum
{
	Protocbi =	0,	/* control/bulk/interrupt; mainly floppies */
	Protocb =	1,	/*   "  with no interrupt; mainly floppies */
	Protobulk =	0x50,	/* bulk only */

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
};

/*
 * corresponds to a lun.
 * these are ~600+Maxiosize bytes each; ScsiReq is not tiny.
 */
struct Umsc
{
	ScsiReq	ScsiReq;
	uint64_t	blocks;
	int64_t	capacity;

	/* from setup */
	char	*bufp;
	long	off;		/* offset within a block */
	long	nb;		/* byte count */

	uint8_t 	rawcmd[16];
	uint8_t	phase;
	char	*inq;
	Ums	*ums;
	Usbfs	fs;
	char	buf[Maxiosize];
};

struct Ums
{
	QLock	QLock;
	Dev	*dev;
	Dev	*epin;
	Dev	*epout;
	Umsc	*lun;
	uint8_t	maxlun;
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
	uint8_t	flags;
	uint8_t	lun;
	uint8_t	len;
	char	command[16];
};

struct Csw
{
	char	signature[4];		/* "USBS" */
	long	tag;
	long	dataresidue;
	uint8_t	status;
};


int	diskmain(Dev*, int, char**);
