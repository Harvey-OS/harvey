/* --------------------------------------------------- */
/* | Copyright (c) 1986 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                            | */
/* --------------------------------------------------- */
/* $Header: dvh.h,v 1.9.1.1 89/01/21 07:44:49 wje Exp $ */

#ifndef	_SYS_DVH_
#define	_SYS_DVH_	1



/*
 * Copyright 1985 by MIPS Computer Systems, Inc.
 */

/*
 * dvh.h -- disk volume header
 */

/*
 * Format for volume header information
 *
 * The volume header is a block located at the beginning of all disk
 * media.  It contains information pertaining to physical device parameters
 * and logical partition information.
 *
 * The volume header is manipulated by disk formatters/verifiers, 
 * partition builders (e.g. newfs/mkfs), and device drivers.
 *
 * A copy of the volume header is located at sector 0 of each track
 * of cylinder 0.  The volume header is constrained to be less than 512
 * bytes long.  A particular copy is assumed valid if no drive errors
 * are detected, the magic number is correct, and the 32 bit 2's complement
 * of the volume header is correct.  The checksum is calculated by initially
 * zeroing vh_csum, summing the entire structure and then storing the
 * 2's complement of the sum.  Thus a checksum to verify the volume header
 * should be 0.
 *
 * The error summary table, bad sector replacement table, and boot blocks are
 * located by searching the volume directory within the volume header.
 *
 * Tables are sized simply by the integral number of table records that
 * will fit in the space indicated by the directory entry.
 *
 * The amount of space allocated to the volume header, replacement blocks,
 * and other tables is user defined when the device is formatted.
 */

/*
 * device parameters are in the volume header to determine mapping
 * from logical block numbers to physical device addresses
 */
struct device_parameters {
	unsigned char	dp_skew;	/* spiral addressing skew */
	unsigned char	dp_gap1;	/* words of 0 before header */
	unsigned char	dp_gap2;	/* words of 0 between hdr and data */
	unsigned char	dp_spare0;	/* spare space */
	unsigned short	dp_cyls;	/* number of cylinders */
	unsigned short	dp_shd0;	/* starting head vol 0 */
	unsigned short	dp_trks0;	/* number of tracks vol 0 */
	unsigned short	dp_shd1;	/* starting head vol 1 */
	unsigned short	dp_trks1;	/* number of tracks vol 1 */
	unsigned short	dp_secs;	/* number of sectors/track */
	unsigned short	dp_secbytes;	/* length of sector in bytes */
	unsigned short	dp_interleave;	/* sector interleave */
	int	dp_flags;		/* controller characteristics */
	int	dp_datarate;		/* bytes/sec for kernel stats */
	int	dp_nretries;		/* max num retries on data error */
	int	dp_spare1;		/* spare entries */
	int	dp_spare2;
	int	dp_spare3;
	int	dp_spare4;
};

/*
 * Device characterization flags
 * (dp_flags)
 */
#define	DP_SECTSLIP	0x00000001	/* sector slip to spare sector */
#define	DP_SECTFWD	0x00000002	/* forward to replacement sector */
#define	DP_TRKFWD	0x00000004	/* forward to replacement track */
#define	DP_MULTIVOL	0x00000008	/* multiple volumes per spindle */
#define	DP_IGNOREERRORS	0x00000010	/* transfer data regardless of errors */
#define DP_RESEEK	0x00000020	/* recalibrate as last resort */

/*
 * Boot blocks, bad sector tables, and the error summary table, are located
 * via the volume_directory.
 */
#define VDNAMESIZE	8

struct volume_directory {
	char	vd_name[VDNAMESIZE];	/* name */
	int	vd_lbn;			/* logical block number */
	int	vd_nbytes;		/* file length in bytes */
};

/*
 * partition table describes logical device partitions
 * (device drivers examine this to determine mapping from logical units
 * to cylinder groups, device formatters/verifiers examine this to determine
 * location of replacement tracks/sectors, etc)
 *
 * NOTE: pt_firstlbn SHOULD BE CYLINDER ALIGNED
 */
struct partition_table {		/* one per logical partition */
	int	pt_nblks;		/* # of logical blks in partition */
	int	pt_firstlbn;		/* first lbn of partition */
	int	pt_type;		/* use of partition */
};

#define	PTYPE_VOLHDR	0		/* partition is volume header */
#define	PTYPE_TRKREPL	1		/* partition is used for repl trks */
#define	PTYPE_SECREPL	2		/* partition is used for repl secs */
#define	PTYPE_RAW	3		/* partition is used for data */
#define	PTYPE_BSD42	4		/* partition is 4.2BSD file system */
#define	PTYPE_SYSV	5		/* partition is SysV file system */
#define	PTYPE_VOLUME	6		/* partition is entire volume */
#define	PTYPE_EFS	7		/* partition is sgi EFS */

#define	VHMAGIC		0xbe5a941	/* randomly chosen value */
#define	NPARTAB		16		/* 16 unix partitions */
#define	NVDIR		15		/* max of 15 directory entries */
#define BFNAMESIZE	16		/* max 16 chars in boot file name */

struct volume_header {
	int vh_magic;				/* identifies volume header */
	short vh_rootpt;			/* root partition number */
	short vh_swappt;			/* swap partition number */
	char vh_bootfile[BFNAMESIZE];		/* name of file to boot */
	struct device_parameters vh_dp;		/* device parameters */
	struct volume_directory vh_vd[NVDIR];	/* other vol hdr contents */
	struct partition_table vh_pt[NPARTAB];	/* device partition layout */
	int vh_csum;				/* volume header checksum */
};

/*
 * The following tables are located via the volume directory
 */

/*
 * error table records media defects
 * allows for "automatic" replacement of bad blocks, and
 * better error logging
 */
#define	ERR_SECC	0	/* soft ecc */
#define	ERR_HECC	1	/* hard ecc */
#define	ERR_HCSUM	2	/* header checksum */
#define	ERR_SOTHER	3	/* any other soft errors */
#define	ERR_HOTHER	4	/* any other hard errors */

#define	NERRTYPES	5	/* Total number of error types */

struct error_table {		/* one per defective logical block */
	int	et_lbn;		/* defective block number */
	int	et_errcount[NERRTYPES];	/* counts for each error type */
};

/*
 * bad sector table
 * The bad sector table is used to map from bad sectors/tracks to replacement
 * sector/tracks.
 *
 * To identify available replacement sectors/tracks, allocate replacements
 * in increasing block number from a replacement partition.  When a new
 * replacement sector/track is needed scan bad sector table to determine current
 * highest replacement sector/track block number and then scan device from next
 * block until a defect free replacement sector/track is found or the end of
 * replacement partition is reached.
 *
 * If bt_rpltype == BSTTYPE_TRKFWD, then bt_badlbn refers to the bad logical
 * block within the bad track, and bt_rpllbn refers to the first sector of 
 * the replacement track.
 *
 * If bt_rpltype == BSTTYPE_SLIPSEC or bt_rpltype == BSTTYPE_SLIPBAD, then
 * bt_rpllbn has no meaning.
 */
struct bst_table {
	int	bt_badlbn;	/* bad logical block */
	int	bt_rpllbn;	/* replacement logical block */
	int	bt_rpltype;	/* replacement method */
};

/*
 * replacement types
 */
#define	BSTTYPE_EMPTY	0	/* slot unused */
#define	BSTTYPE_SLIPSEC	1	/* sector slipped to next sector */
#define	BSTTYPE_SECFWD	2	/* sector forwarded to replacment sector */
#define	BSTTYPE_TRKFWD	3	/* track forwarded to replacement track */
#define BSTTYPE_SLIPBAD	4	/* sector reserved for slipping has defect */

/*
 * The following structs are parameters to various driver ioctls
 * for disk formatting, etc.
 */

/*
 * controller information struct
 * returned via DIOCGETCTLR
 * mostly to determine appropriate method for bad block handling
 */
#define	CITYPESIZE	32

struct ctlr_info {
	int	ci_flags;		/* same as DP_* flags */
	char	ci_type[CITYPESIZE];	/* controller model and manuf. */
};

/*
 * verify sectors information
 * Passed to device driver via ioctl DIOCVFYSEC
 */
struct verify_info {
	int	vi_lbn;		/* logical block number */
	int	vi_bcnt;	/* logical block count */
};

/*
 * cause controller to run diagnostics
 */
struct diag_info {
	int	di_errcode;	/* error code */
	int	di_lbn;		/* logical block number */
	int	di_bcnt;	/* logical block count */
	char	*di_addr;	/* buffer address */
};

/*
 * information necessary to perform one of the following actions:
 * 	format a track
 *	    fmi_cyl and fmi_trk identify track to format
 *	map a track
 *	    fmi_cyl and fmi_trk identify defective track
 *	    fmi_rplcyl and fmi_rpltrk identify replacement track
 *	map a sector
 *	    fmi_cyl, fmi_trk, and fmi_sec identify defective sector
 *	    fmi_rplcyl, fmi_rpltrk, and fmi_rplsec identify replacement sector
 *	slip a sector
 *	    fmi_cyl, fmi_trk, and fmi_sec identify defective sector
 */
#define FMI_FORMAT_TRACK	1	/* format a track */
#define FMI_MAP_TRACK		2	/* map a track */
#define FMI_MAP_SECTOR		3	/* map a sector */
#define FMI_SLIP_SECTOR		4	/* slip a sector */

struct fmt_map_info {
	int	fmi_action;		/* action desired, see FMI_ above */
	unsigned short	fmi_cyl;	/* cylinder with defect or one with */
					/* track to format */
	unsigned char	fmi_trk;	/* track with defect or one to format */
	unsigned char	fmi_sec;	/* sector with defect */
	unsigned short	fmi_rplcyl;	/* replacement cylinder */
	unsigned char	fmi_rpltrk;	/* replacement track */
	unsigned char	fmi_rplsec;	/* replacement sector */
};

struct sdformat {
	u_char		sf_fmtdata;		/* format with defect data */
	u_char		sf_cmplst;		/* complete defect list */
	u_char		sf_lstfmt;		/* defect list format */
	u_char		sf_pattern;		/* format pattern */
	u_short		sf_intleave;		/* interleave factor */
	u_char		sf_deflen;		/* len of defect list */
	u_short		*sf_defptr;		/* pointer to defect data */
};
#define SDMAXDEFECT	256			/* max number of defect */

#endif	_SYS_DVH_
