#include	"all.h"

typedef	struct	Wren	Wren;
struct	Wren
{
	long	block;			/* size of a block -- from config */
	long	nblock;			/* number of blocks -- from config */
	long	mult;			/* multiplier to get physical blocks */
	long	max;			/* number of logical blocks */
};

void
wreninit(Device *d)
{
	int s;
	uchar cmd[10], buf[8];
	Wren *dr;

	dr = d->private;
	if(dr)
		return;
	dr = ialloc(sizeof(Wren), 0);
	d->private = dr;

loop:
	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0x25;					/* read capacity */
	s = scsiio(d, SCSIread, cmd, sizeof(cmd), buf, sizeof(buf));
	if(s) {
		print("wreninit: %Z bad status %.4x\n", d, s);
		delay(1000);
		goto loop;
	}
	dr->nblock =
		(buf[0]<<24) |
		(buf[1]<<16) |
		(buf[2]<<8) |
		(buf[3]<<0);
	dr->block =
		(buf[4]<<24) |
		(buf[5]<<16) |
		(buf[6]<<8) |
		(buf[7]<<0);
	if(dr->block <= 0 || dr->block >= 16*1024) {
		print("	wreninit %Z block size %ld setting to 512\n", d, dr->block);
		dr->block = 512;
	}
	dr->mult =
		(RBUFSIZE + dr->block - 1) /
		dr->block;
	dr->max =
		(dr->nblock + 1) / dr->mult;
	print("	drive %Z:\n", d);
	print("		%ld blocks at %ld bytes each\n",
		dr->nblock, dr->block);
	print("		%ld logical blocks at %d bytes each\n",
		dr->max, RBUFSIZE);
	print("		%ld multiplier\n",
		dr->mult);
}

long
wrensize(Device *d)
{
	Wren *dr;

	dr = d->private;
	return dr->max;
}

int
wreniocmd(Device *d, int io, long b, void *c)
{
	long l, m;
	uchar cmd[10];
	Wren *dr;

	dr = d->private;
	if(d == 0) {
		print("wreniocmd: no drive - a=%Z b=%ld\n", d, b);
		return 0x40;
	}
	if(b >= dr->max) {
		print("wreniocmd out of range a=%Z b=%ld\n", d, b);
		return 0x40;
	}

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0x28;	/* extended read */
	if(io != SCSIread)
		cmd[0] = 0x2a;	/* extended write */

	m = dr->mult;
	l = b * m;
	cmd[2] = l>>24;
	cmd[3] = l>>16;
	cmd[4] = l>>8;
	cmd[5] = l;

	cmd[7] = m>>8;
	cmd[8] = m;

	return scsiio(d, io, cmd, sizeof(cmd), c, RBUFSIZE);
}

int
wrenread(Device *d, long b, void *c)
{
	int s;

	s = wreniocmd(d, SCSIread, b, c);
	if(s) {
		print("wrenread: %Z(%ld) bad status %.4x\n", d, b, s);
		cons.nwormre++;
		return 1;
	}
	return 0;
}

int
wrenwrite(Device *d, long b, void *c)
{
	int s;

	s = wreniocmd(d, SCSIwrite, b, c);
	if(s) {
		print("wrenwrite: %Z(%ld) bad status %.4x\n", d, b, s);
		cons.nwormwe++;
		return 1;
	}
	return 0;
}
