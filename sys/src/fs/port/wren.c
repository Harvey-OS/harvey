#include	"all.h"

void
wreninit(Device d)
{
	int s;
	uchar cmd[10], buf[8];
	Drive *dr;

	dr = scsidrive(d);
	if(dr == 0 || dr->status != Dready) {
		print("	drive %D: not ready\n", d);
		return;
	}

loop:
	if(dr->mult)
		return;
	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0x25;					/* read capacity */
	s = scsiio(dr->dev, SCSIread, cmd, sizeof(cmd), buf, sizeof(buf));
	if(s) {
		print("wreninit: %D bad status %.4x\n", d, s);
		delay(5000);
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
	dr->mult =
		(RBUFSIZE + dr->block - 1) /
		dr->block;
	dr->max =
		(dr->nblock + 1) / dr->mult;
	print("	drive %D:\n", d);
	print("		%ld blocks at %ld bytes each\n",
		dr->nblock, dr->block);
	print("		%ld logical blocks at %d bytes each\n",
		dr->max, RBUFSIZE);
	print("		%ld multiplier\n",
		dr->mult);
}

long
wrensize(Device a)
{
	Drive *dr;

	dr = scsidrive(a);
	return dr->max;
}

int
wreniocmd(int io, Device a, long b, void *c)
{
	long l, m;
	uchar cmd[10];
	Drive *dr;

	if((dr = scsidrive(a)) == 0) {
		print("wreniocmd: no drive - a=%D b=%ld\n", a, b);
		return 0x40;
	}
	if(b >= dr->max) {
		print("wreniocmd out of range a=%D b=%ld\n", a, b);
		return 0x40;
	}

	cmd[0] = 0x28;	/* extended read */
	if(io != SCSIread)
		cmd[0] = 0x2a;	/* extended write */
	cmd[1] = 0;

	m = dr->mult;
	l = b * m;
	cmd[2] = l>>24;
	cmd[3] = l>>16;
	cmd[4] = l>>8;
	cmd[5] = l;
	cmd[6] = 0;

	cmd[7] = m>>8;
	cmd[8] = m;
	cmd[9] = 0;

	return scsiio(dr->dev, io, cmd, sizeof(cmd), c, RBUFSIZE);
}

int
wrenread(Device a, long b, void *c)
{
	int s;

	s = wreniocmd(SCSIread, a, b, c);
	if(s) {
		print("wrenread: %D(%ld) bad status %.4x\n", a, b, s);
		cons.nwormre++;
		return 1;
	}
	return 0;
}

int
wrenwrite(Device a, long b, void *c)
{
	int s;

	s = wreniocmd(SCSIwrite, a, b, c);
	if(s) {
		print("wrenwrite: %D(%ld) bad status %.4x\n", a, b, s);
		cons.nwormwe++;
		return 1;
	}
	return 0;
}
