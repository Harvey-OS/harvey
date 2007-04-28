#include <u.h>
#include <libc.h>
#include <bio.h>

#include "scsireq.h"

static char* key[16] = {
	"no sense",
	"recovered error",
	"not ready",
	"medium error",
	"hardware error",
	"illegal request",
	"unit attention",
	"data protect",
	"blank check",
	"vendor specific",
	"copy aborted",
	"aborted command",
	"equal",
	"volume overflow",
	"miscompare",
	"reserved",
};

static struct {
	uchar	asc;
	uchar	ascq;
	char*	diag;
} code[] = {				/* see /sys/lib/scsicodes */
0x00,0x01,	"filemark detected",
0x00,0x17,	"cleaning requested",
0x01,0x00,	"no index/sector signal",
0x02,0x00,	"no seek complete",
0x03,0x00,	"peripheral device write fault",
0x04,0x00,	"drive not ready",
0x04,0x01,	"drive becoming ready",
0x04,0x01,	"drive need initializing cmd",
0x05,0x00,	"logical unit does not respond to selection",
0x08,0x00,	"logical unit communication failure",
0x09,0x00,	"track-following error",
0x0c,0x00,	"write error",
0x0c,0x01,	"write error - recovered with auto reallocation",
0x0c,0x02,	"write error - auto reallocation failed",
0x0c,0x03,	"write error - recommend reassignment",

0x11,0x00,	"unrecovered read error",
0x15,0x00,	"positioning error",
0x17,0x00,	"recovered read data with retries",
0x18,0x00,	"recovered read with ecc correction",
0x1A,0x00,	"parameter list length error",
0x1B,0x00,	"synchronous data transfer error",

0x20,0x00,	"invalid command",
0x21,0x00,	"invalid block address",
0x22,0x00,	"illegal function (use 20 00, 24 00, or 26 00)",
0x24,0x00,	"illegal field in command list",
0x25,0x00,	"invalid lun",
0x26,0x00,	"invalid field in parameter list",
0x27,0x00,	"write protected",
0x28,0x00,	"medium changed",
0x29,0x00,	"power-on reset or bus reset occurred",
0x2C,0x00,	"command sequence error",

0x30,0x00,	"incompatible medium installed",
0x31,0x00,	"medium format corrupted",
0x33,0x00,	"tape length error / monitor atip error",
0x34,0x00,	"enclosure failure / absorption control error",
0x3A,0x00,	"medium not present",
0x3B,0x00,	"sequential positioning error",
0x3B,0x01,	"tape position error at beginning-of-medium",
0x3B,0x02,	"tape position error at end-of-medium",
0x3B,0x08,	"reposition error",
0x3B,0x09,	"read past end of medium",
0x3B,0x0A,	"read past beginning of medium",
0x3B,0x0B,	"position past end of medium",
0x3B,0x0C,	"position past beginning of medium",
0x3B,0x0D,	"medium destination element full",
0x3B,0x0E,	"medium source element empty",
0x3B,0x0F,	"end of medium reached",
0x3B,0x16,	"mechanical positioning or changer error",
0x3D,0x00,	"invalid bits in identify message",
0x3E,0x00,	"logical unit has not self-configured yet",

0x40,0x00,	"diagnostic failure",
0x42,0x00,	"power-on or self test failure",
0x44,0x00,	"internal controller error",
0x47,0x00,	"scsi parity error",

0x50,0x00,	"write append error",
0x51,0x00,	"erase failure",
0x53,0x00,	"medium load or eject failed",
0x54,0x00,	"scsi to host system interface failure",
0x55,0x00,	"system resource failure",
0x57,0x00,	"unable to read toc, pma or subcode",
0x5A,0x00,	"operator medium removal request",
0x5D,0x00,	"failure prediction threshold exceeded",

/* cd/dvd? */
0x63,0x00,	"end of user area encountered on this track",
0x64,0x00,	"illegal mode for this track",
0x65,0x00,	"verify failed",
0x67,0x00,	"configuration failure",
0x68,0x00,	"logical unit not configured",
0x69,0x00,	"data loss on logical unit",
0x6f,0x01,	"copy protection key exchange failure - key not present",
0x6f,0x02,	"copy protection key exchange failure - key not established",
0x6f,0x03,	"read of scrambled sector without authentication",
0x6f,0x04,	"media region code is mismatched to logical unit region",
0x6f,0x05,	"drive region must be permanent/region reset count error",
0x6f,0x00,	"copy protection key exchange failure",

0x72,0x00,	"session fixation error",
0x73,0x00,	"cd control error",
0x74,0x00,	"security error",

/* vendor specific */

0x81,0x00,	"illegal track",
0x82,0x00,	"command now not valid",
0x83,0x00,	"medium removal is prevented",

0xA0,0x00,	"stopped on non-data block",
0xA1,0x00,	"invalid start address",
0xA2,0x00,	"attempt to cross track boundary",
0xA3,0x00,	"illegal medium",
0xA4,0x00,	"disc write-protected",
0xA5,0x00,	"application code conflict",
0xA6,0x00,	"illegal block-size for command",
0xA7,0x00,	"block-size conflict",
0xA8,0x00,	"illegal transfer-length",
0xA9,0x00,	"request for fixation failed",
0xAA,0x00,	"end of medium reached",
0xAB,0x00,	"illegal track number",
0xAC,0x00,	"data track length error",
0xAD,0x00,	"buffer underrun",
0xAE,0x00,	"illegal track mode",
0xAF,0x00,	"optimum power calibration error",

0xB0,0x00,	"calibration area almost full",
0xB1,0x00,	"current programme area empty",
0xB2,0x00,	"no efm at search address",
0xB3,0x00,	"link area encountered",
0xB4,0x00,	"calibration area full",
0xB5,0x00,	"dummy blocks added",
0xB6,0x00,	"block size format conflict",
0xB7,0x00,	"current command aborted",

/* cd/dvd burning? */
0xD0,0x00,	"recovery needed",
0xD1,0x00,	"can't recover from track",
0xD2,0x00,	"can't recover from program memory area",
0xD3,0x00,	"can't recover from leadin area",
0xD4,0x00,	"can't recover from leadout area",
0xD5,0x00,	"can't recover from optical power calibration area",
0xD6,0x00,	"eeprom failure",
};

extern Biobuf bout;

void
makesense(ScsiReq *rp)
{
	int i;

	Bprint(&bout, "sense data: %s", key[rp->sense[2] & 0x0F]);
	for(i=0; i<nelem(code); i++)
		if(code[i].asc == rp->sense[0x0C])
			if(code[i].ascq == 0 || code[i].ascq == rp->sense[0x0D])
				break;
	if(rp->sense[7] >= 5 && i < nelem(code))
		Bprint(&bout, ": %s", code[i].diag);
	Bprint(&bout, "\n\t");
	for(i = 0; i < 8+rp->sense[7]; i++)
		Bprint(&bout, " %2.2ux", rp->sense[i]);
	Bprint(&bout, "\n");
}
