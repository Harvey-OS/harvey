#include	"all.h"

static	char*	codes[] =
{
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

int
scsicmd(Chan c, uchar *cmd, int ccount, uchar *data, int dcount, int io)
{
	uchar resp[4];
	int n;
	long status;

	if(write(c.fcmd, cmd, ccount) != ccount)
		goto bad;
	if(io != Sread)
		n = write(c.fdata, data, dcount);
	else
		n = read(c.fdata, data, dcount);
	if(read(c.fcmd, resp, sizeof(resp)) != sizeof(resp))
		goto bad;

	status = (resp[0]<<24) | (resp[1]<<16) | (resp[2]<<8) | resp[3];
	if(status == 0x6000)
		return n;

bad:
	return -1;
}

int
unitready(Chan c)
{
	uchar cmd[6], resp[4];
	int status, i;

	for(i=0; i<3; i++) {
		memset(cmd, 0, sizeof(cmd));
		cmd[0] = 0x00;	/* unit ready */
		if(write(c.fcmd, cmd, sizeof(cmd)) != sizeof(cmd))
			goto bad;
		write(c.fdata, resp, 0);
		if(read(c.fcmd, resp, sizeof(resp)) != sizeof(resp))
			goto bad;
		status = (resp[0]<<24) | (resp[1]<<16) | (resp[2]<<8) | resp[3];
		if(status == 0x6000 || status == 0x6002)
			return 0;
	bad:;
	}
	return 1;
}

Chan
openscsi(int target)
{
	char scsiname[50];
	Chan c;

	c.open = 0;
	sprint(scsiname, "#S1/%d/cmd", target);
	c.fcmd = open(scsiname, ORDWR);
	if(c.fcmd < 0)
		return c;

	sprint(scsiname, "#S1/%d/data", target);
	c.fdata = open(scsiname, ORDWR);
	if(c.fdata < 0) {
		close(c.fcmd);
		return c;
	}
	if(unitready(c)) {
		close(c.fcmd);
		close(c.fdata);
		return c;
	}
	c.open = 1;
	return c;
}

void
closescsi(Chan c)
{
	if(c.open) {
		close(c.fcmd);
		close(c.fdata);
		c.open = 0;
	}
}

int
readscsi(Chan c, void *v, long block, long nbyte, int bs, int op, int mod)
{
	uchar cmd[10], cmd1[12], *p;
	long r, n, count, nn;

	if(nbyte >= (1<<16)) {
		print("scsi read too big: %ld\n", nbyte);
		return 1;
	}

	r = nbyte % bs;
	n = nbyte / bs;
	p = (uchar*)v;

	if(op == 0xd8) {	/* plextor */
		memset(cmd1, 0,  sizeof(cmd1));
		cmd1[0] = op;	/* read */
		cmd1[2] = block>>24;
		cmd1[3] = block>>16;
		cmd1[4] = block>>8;
		cmd1[5] = block>>0;
		cmd1[6] = n>>24;
		cmd1[7] = n>>26;
		cmd1[8] = n>>8;
		cmd1[9] = n>>0;
		cmd1[10] = mod;	/* 0 2352, 1 2368, 2 2448, 3 96 */

/*
print("block = %d; nbyte = %d; bs = %d\n", block, nbyte, bs);
print("n = %d; r = %d\n", n, r);

print("%.2ux %.2ux %.2ux %.2ux %.2ux %.2ux %.2ux %.2ux %.2ux %.2ux %.2ux %.2ux\n",
cmd1[0],cmd1[1],cmd1[2],cmd1[3],
cmd1[4],cmd1[5],cmd1[6],cmd1[7],
cmd1[8],cmd1[9],cmd1[10],cmd1[11]);
*/

		count = n*bs;
		nn = scsi(c, cmd1, sizeof(cmd1), p, count, Sread);
	} else {
		memset(cmd, 0,  sizeof(cmd));
		cmd[0] = op;	/* read */
		cmd[1] = mod;
		cmd[2] = block>>24;
		cmd[3] = block>>16;
		cmd[4] = block>>8;
		cmd[5] = block>>0;
		cmd[7] = n>>8;
		cmd[8] = n>>0;

		count = n*bs;
		nn = scsi(c, cmd, sizeof(cmd), p, count, Sread);
	}
	if(nn != count) {
		print("scsi read short: %ld/%ld\n", nn, count);
		return 1;
	}

	if(r) {
		if(bs > sizeof(blkbuf)) {
			print("bs too big: %d\n", bs);
			return 1;
		}
		if(readscsi(c, blkbuf, block+n, bs, bs, op, mod))
			return 1;
		memmove(p+count, blkbuf, r);
	}
	return 0;
}

int
writescsi(Chan c, void *v, long b, long nbyte, int bs)
{
	uchar cmd[10], *p;
	long r, n, count, nn;

	if(nbyte >= (1<<16)) {
		print("scsi read too big: %ld\n", nbyte);
		return 1;
	}

	r = nbyte % bs;
	n = nbyte / bs;
	p = (uchar*)v;

	memset(cmd, 0,  sizeof(cmd));
	cmd[0] = 0x2a;	/* write */
	cmd[2] = b>>24;
	cmd[3] = b>>16;
	cmd[4] = b>>8;
	cmd[5] = b>>0;
	cmd[7] = n>>8;
	cmd[8] = n>>0;

	count = n*bs;
	nn = scsi(c, cmd, sizeof(cmd), p, count, Swrite);
	if(nn != count) {
		print("scsi read short: %ld/%ld\n", nn, count);
		return 1;
	}

	if(r) {
		if(bs > sizeof(blkbuf)) {
			print("bs too big: %d\n", bs);
			return 1;
		}
		memset(blkbuf, 0, bs);
		memmove(blkbuf, p+count, r);
		if(writescsi(c, blkbuf, b+n, bs, bs))
			return 1;
	}
	return 0;
}

int
scsi(Chan c, uchar *cmd, int ccount, uchar *data, int dcount, int io)
{
	uchar req[6], sense[255];
	int code, key, n, f;

	f = 0;

loop:
	n = scsicmd(c, cmd, ccount, data, dcount, io);
	if(n >= 0)
		return n;

	/*
	 * request sense
	 */
	memset(req, 0, sizeof(req));
	req[0] = 0x03;
	req[4] = sizeof(sense);
	scsicmd(c, req, sizeof(req), sense, sizeof(sense), Sread);

	unitready(c);

	key = sense[2];
	code = sense[12];

	if(code == 0x17 || code == 0x18)	/* recovered errors */
		return dcount;
	if(code == 0x28 && cmd[0] == 0x43 && f == 0) {	/* get info and media changed */
		f = 1;
		goto loop;
	}

	print("scsi reqsense cmd #%.2x: key #%2.2ux code #%.2ux #%.2ux\n",
		cmd[0], key, code, sense[13]);
	if(codes[code])
		print("	%s\n", codes[code]);
	if(key == 0)
		return dcount;
	return -1;
}
