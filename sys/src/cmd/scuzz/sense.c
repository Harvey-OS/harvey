#include <u.h>
#include <libc.h>
#include <bio.h>

#include "scsireq.h"

static
char*	key[16] =
{
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

static
struct
{
	uchar	asc;
	uchar	ascq;
	char*	diag;
} code[] =
{
0x03,0x00,	"tray out",
0x04,0x00,	"drive not ready",
0x08,0x00,	"communication failure",
0x09,0x00,	"track following error",

0x11,0x00,	"unrecovered read error",
0x15,0x00,	"positioning error",
0x17,0x00,	"recovered read data with retries",
0x18,0x00,	"recovered read with ecc correction",
0x1A,0x00,	"parameter list length error",

0x20,0x00,	"invalid command",
0x21,0x00,	"invalid block address",
0x24,0x00,	"illegal field in command list",
0x25,0x00,	"invalid lun",
0x26,0x00,	"invalid field parameter list",
0x28,0x00,	"medium changed",
0x29,0x00,	"power-on reset or bus reset occurred",
0x2C,0x00,	"command sequence error",

0x31,0x00,	"medium format corrupted",
0x33,0x00,	"monitor atip error",
0x34,0x00,	"absorption control error",
0x3A,0x00,	"medium not present",
0x3D,0x00,	"invalid bits in identify message",

0x40,0x00,	"diagnostic failure",
0x42,0x00,	"power-on or self test failure",
0x44,0x00,	"internal controller error",
0x47,0x00,	"scsi parity error",

0x50,0x00,	"write append error",
0x53,0x00,	"medium load or eject failed",
0x57,0x00,	"unable to read toc, pma or subcode",
0x5A,0x00,	"operator medium removal request",

0x63,0x00,	"end of user area encountered on this track",
0x64,0x00,	"illegal mode for this track",
0x65,0x00,	"verify failed",
0x6f,0x01,	"copy protection key exchange failure - key not present",
0x6f,0x02,	"copy protection key exchange failure - key not established",
0x6f,0x03,	"read of scrambled sector without authentication",
0x6f,0x04,	"media region code is mismatched to logical unit region",
0x6f,0x05,	"drive region must be permanent/region reset count error",
0x6f,0x00,	"copy protection key exchange failure",

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
