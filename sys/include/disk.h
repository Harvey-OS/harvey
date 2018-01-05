/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#pragma src "/sys/src/libdisk"
#pragma lib "libdisk.a"

/* SCSI interface */
typedef struct Scsi Scsi;
struct Scsi {
	QLock QLock;
	char*	inquire;
	int	rawfd;
	int	nchange;
	unsigned long	changetime;
};

enum {
	Sread = 0,
	Swrite,
	Snone,
};

char*	scsierror(int, int);
int		scsicmd(Scsi*, uint8_t*, int, void*, int, int);
int		scsi(Scsi*, uint8_t*, int, void*, int, int);
Scsi*		openscsi(char*);
void		closescsi(Scsi*);
int		scsiready(Scsi*);

extern int		scsiverbose;

/* disk partition interface */
typedef struct Disk Disk;
struct Disk {
	char *prefix;
	char *part;
	int fd;
	int wfd;
	int ctlfd;
	int rdonly;
	int type;

	int64_t secs;
	int64_t secsize;
	int64_t size;
	int64_t offset;	/* within larger disk, perhaps */
	int width;	/* of disk size in bytes as decimal string */
	int c;
	int h;
	int s;
	int chssrc;
};

Disk*	opendisk(char*, int, int);

enum {
	Tfile = 0,
	Tsd,
	Tfloppy,

	Gpart = 0,	/* partition info source */
	Gdisk,
	Gguess,
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
	ScmdRcapacity16	= 0x9e,		/* long read capacity */
	ScmdRformatcap	= 0x23,		/* read format capacity */
	ScmdExtread	= 0x28,		/* extended read (10 bytes) */
	ScmdRead16	= 0x88,		/* long read (16 bytes) */
	ScmdExtwrite	= 0x2A,		/* extended write (10 bytes) */
	ScmdExtwritever = 0x2E,		/* extended write and verify (10) */
	ScmdWrite16	= 0x8A,		/* long write (16 bytes) */
	ScmdExtseek	= 0x2B,		/* extended seek */

	ScmdSynccache	= 0x35,		/* flush cache */
	ScmdRTOC	= 0x43,		/* read TOC data */
	ScmdRdiscinfo	= 0x51,		/* read disc information */
	ScmdRtrackinfo	= 0x52,		/* read track information */
	ScmdReserve	= 0x53,		/* reserve track */
	ScmdBlank	= 0xA1,		/* blank *-RW media */

	ScmdCDpause	= 0x4B,		/* pause/resume */
	ScmdCDstop	= 0x4E,		/* stop play/scan */
	ScmdCDplay	= 0xA5,		/* play audio */
	ScmdCDload	= 0xA6,		/* load/unload */
	ScmdCDscan	= 0xBA,		/* fast forward/reverse */
	ScmdCDstatus	= 0xBD,		/* mechanism status */
	Scmdgetconf	= 0x46,		/* get configuration */

	ScmdEInitialise	= 0x07,		/* initialise element status */
	ScmdMMove	= 0xA5,		/* move medium */
	ScmdEStatus	= 0xB8,		/* read element status */
	ScmdMExchange	= 0xA6,		/* exchange medium */
	ScmdEposition	= 0x2B,		/* position to element */

	ScmdReadDVD	= 0xAD,		/* read dvd structure */
	ScmdReportKey	= 0xA4,		/* read dvd key */
	ScmdSendKey	= 0xA3,		/* write dvd key */

	ScmdClosetracksess= 0x5B,
	ScmdRead12	= 0xA8,
	ScmdSetcdspeed	= 0xBB,
	ScmdReadcd	= 0xBE,

	/* vendor-specific */
	ScmdFwaddr	= 0xE2,		/* first writeable address */
	ScmdTreserve	= 0xE4,		/* reserve track */
	ScmdTinfo	= 0xE5,		/* read track info */
	ScmdTwrite	= 0xE6,		/* write track */
	ScmdMload	= 0xE7,		/* medium load/unload */
	ScmdFixation	= 0xE9,		/* fixation */
};

/* proto file parsing */
typedef void Protoenum(char *new, char *old, Dir *d, void *a);
typedef void Protowarn(char *msg, void *a);
int rdproto(char*, char*, Protoenum*, Protowarn*, void*);
