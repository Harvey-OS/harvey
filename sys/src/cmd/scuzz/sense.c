#include <u.h>
#include <libc.h>
#include <bio.h>

#include "scsireq.h"

static char *key[16] = {
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

static char *code[256] = {
[0x00]	0,				/* "no additional sense information", */

[0x03]	"tray out",
[0x04]	"drive not ready",
[0x08]	"communication failure",
[0x09]	"track following error",

[0x11]	"unrecovered read error",
[0x15]	"positioning error",
[0x17]	"recovered read data with retries",
[0x18]	"recovered read with ecc correction",
[0x1A]	"parameter list length error",

[0x20]	"invalid command",
[0x21]	"invalid block address",
[0x24]	"illegal field in command list",
[0x25]	"invalid lun",
[0x26]	"invalid field parameter list",
[0x28]	"medium changed",
[0x29]	"power-on reset or bus reset occurred",
[0x2C]	"command sequence error",

[0x31]	"medium format corrupted",
[0x33]	"monitor atip error",
[0x34]	"absorption control error",
[0x3A]	"medium not present",
[0x3D]	"invalid bits in identify message",

[0x40]	"diagnostic failure",
[0x42]	"power-on or self test failure",
[0x44]	"internal controller error",
[0x47]	"scsi parity error",

[0x50]	"write append error",
[0x53]	"medium load or eject failed",
[0x57]	"unable to read toc, pma or subcode",
[0x5A]	"operator medium removal request",

[0x63]	"end of user area encountered on this track",
[0x64]	"illegal mode for this track",
[0x65]	"verify failed",

[0x81]	"illegal track",
[0x82]	"command now not valid",
[0x83]	"medium removal is prevented",

[0xA0]	"stopped on non-data block",
[0xA1]	"invalid start address",
[0xA2]	"attempt to cross track boundary",
[0xA3]	"illegal medium",
[0xA4]	"disc write-protected",
[0xA5]	"application code conflict",
[0xA6]	"illegal block-size for command",
[0xA7]	"block-size conflict",
[0xA8]	"illegal transfer-length",
[0xA9]	"request for fixation failed",
[0xAA]	"end of medium reached",
[0xAB]	"illegal track number",
[0xAC]	"data track length error",
[0xAD]	"buffer underrun",
[0xAE]	"illegal track mode",
[0xAF]	"optimum power calibration error",

[0xB0]	"calibration area almost full",
[0xB1]	"current programme area empty",
[0xB2]	"no efm at search address",
[0xB3]	"link area encountered",
[0xB4]	"calibration area full",
[0xB5]	"dummy blocks added",
[0xB6]	"block size format conflict",
[0xB7]	"current command aborted",

[0xD0]	"recovery needed",
[0xD1]	"can't recover from track",
[0xD2]	"can't recover from program memory area",
[0xD3]	"can't recover from leadin area",
[0xD4]	"can't recover from leadout area",
[0xD5]	"can't recover from optical power calibration area",
[0xD6]	"eeprom failure",
};

extern Biobuf bout;

void
makesense(ScsiReq *rp)
{
	int i, asc;

	Bprint(&bout, "sense data: %s", key[rp->sense[2] & 0x0F]);
	asc = rp->sense[0x0C];
	if(rp->sense[7] >= 5 && code[asc])
		Bprint(&bout, ": %s", code[asc]);
	Bprint(&bout, "\n\t");
	for(i = 0; i < 8+rp->sense[7]; i++)
		Bprint(&bout, " %2.2ux", rp->sense[i]);
	Bprint(&bout, "\n");
}
