#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

/*
 * BUG: only LUN 0
 */

static Scsi staticcmd;

static Disc *sdrive;
static bufbusy;

Scsibuf *
scsibuf(void)
{
	static Scsibuf b;

	if(bufbusy)
		panic("bufbusy\n");
	bufbusy++;
	if(b.virt == 0){
		b.virt = ialloc(Maxxfer, 0);
		b.phys = (void *)(PADDR(b.virt));
	}
	return &b;
}

void
scsifree(Scsibuf*)
{
	if(bufbusy == 0)
		panic("buf not busy\n");
	bufbusy = 0;
}

Scsi *
scsicmd(int dev, int cmdbyte, Scsibuf *b, long size)
{
	Scsi *cmd = &staticcmd;

	cmd->target = dev /*>> 3*/;				/* BUG */
	cmd->lun = 0/*dev & 7*/;				/* BUG */
	cmd->cmd.base = cmd->cmdblk;
	cmd->cmd.ptr = cmd->cmd.base;
	memset(cmd->cmdblk, 0, sizeof cmd->cmdblk);
	cmd->cmdblk[0] = cmdbyte;
	cmd->cmdblk[1] = cmd->lun << 5;
	switch(cmdbyte >> 5){
	case 0:
		cmd->cmd.lim = &cmd->cmdblk[6];
		break;
	case 1:
		cmd->cmd.lim = &cmd->cmdblk[10];
		break;
	default:
		cmd->cmd.lim = &cmd->cmdblk[12];
		break;
	}
	switch(cmdbyte){
	case ScsiTestunit:
		break;
	case ScsiStartunit:
		cmd->cmdblk[4] = 1;
		break;
	case ScsiModesense:
		cmd->cmdblk[2] = 1;
		/* fall through */
	case ScsiExtsens:
	case ScsiInquiry:
		cmd->cmdblk[4] = size;
		break;
	case ScsiGetcap:
		break;
	}
	cmd->b = b;
	cmd->data.base = b->virt;
	cmd->data.lim = cmd->data.base + size;
	cmd->data.ptr = cmd->data.base;
	cmd->save = cmd->data.base;
	return cmd;
}

int
scsiready(int dev)
{
	Scsi *cmd;
	int status;

	cmd = scsicmd(dev, ScsiTestunit, scsibuf(), 0);
	status = scsiexec(cmd, ScsiOut);
	scsifree(cmd->b);
	return status&0xff;
}

int
scsisense(int dev, void *p)
{
	Scsi *cmd;
	int status;

	cmd = scsicmd(dev, ScsiExtsens, scsibuf(), 18);
	status = scsiexec(cmd, ScsiIn);
	memmove(p, cmd->data.base, 18);
	scsifree(cmd->b);
	return status&0xff;
}

int
scsistartstop(int dev, int cmdbyte)
{
	Scsi *cmd;
	int status;

	cmd = scsicmd(dev, cmdbyte, scsibuf(), 0);
	status = scsiexec(cmd, ScsiOut);
	scsifree(cmd->b);
	return status&0xff;
}

int
scsicap(int dev, void *p)
{
	Scsi *cmd;
	int status;

	cmd = scsicmd(dev, ScsiGetcap, scsibuf(), 8);
	status = scsiexec(cmd, ScsiIn);
	memmove(p, cmd->data.base, 8);
	scsifree(cmd->b);
	return status&0xFF;
}

int
scsibread(int dev, Scsibuf *b, long n, long blocksize, long blockno)
{
	Scsi *cmd;
	int cmdbyte;

	if(blockno <= 0x1fffff && n <= 256)
		cmdbyte = ScsiRead;
	else
		cmdbyte = ScsiExtread;

	cmd = scsicmd(dev, cmdbyte, b, n*blocksize);
	switch(cmdbyte){
	case ScsiRead:
		cmd->cmdblk[1] |= blockno >> 16;
		cmd->cmdblk[2] = blockno >> 8;
		cmd->cmdblk[3] = blockno;
		cmd->cmdblk[4] = n;
		break;
	default:
		cmd->cmdblk[2] = blockno >> 24;
		cmd->cmdblk[3] = blockno >> 16;
		cmd->cmdblk[4] = blockno >> 8;
		cmd->cmdblk[5] = blockno;
		cmd->cmdblk[7] = n>>8;
		cmd->cmdblk[8] = n;
		break;
	}
	scsiexec(cmd, ScsiIn);
	n = cmd->data.ptr - cmd->data.base;
	return n;
}

int
scsibwrite(int dev, Scsibuf *b, long n, long blocksize, long blockno)
{
	Scsi *cmd;

	int cmdbyte;

	if(blockno <= 0x1fffff && n <= 256)
		cmdbyte = ScsiWrite;
	else
		cmdbyte = ScsiExtwrite;

	cmd = scsicmd(dev, cmdbyte, b, n*blocksize);
	switch(cmdbyte){
	case ScsiWrite:
		cmd->cmdblk[1] |= blockno >> 16;
		cmd->cmdblk[2] = blockno >> 8;
		cmd->cmdblk[3] = blockno;
		cmd->cmdblk[4] = n;
		break;
	default:
		cmd->cmdblk[2] = blockno >> 24;
		cmd->cmdblk[3] = blockno >> 16;
		cmd->cmdblk[4] = blockno >> 8;
		cmd->cmdblk[5] = blockno;
		cmd->cmdblk[7] = n>>8;
		cmd->cmdblk[8] = n;
		break;
	}
	scsiexec(cmd, ScsiOut);
	n = cmd->data.ptr - cmd->data.base;
	return n;
}

long
scsiseek(int dev, long off)
{
	sdrive[dev].offset = off;
	return off;
}

/*
 *  read partition table.  The partition table is just ascii strings.
 */
#define MAGIC "plan9 partitions"
static void
scsipart(int dev)
{
	Disc *dp;
	Partition *pp;
	char *line[Npart+1], *cp;
	char *field[3];
	ulong n;
	int i;
	Scsibuf *b;

	dp = &sdrive[dev];

	/*
	 *  we always have a partition for the whole disk
	 *  and one for the partition table
	 */
	pp = &dp->p[0];
	strcpy(pp->name, "disk");
	pp->start = 0;
	pp->end = dp->cap;
	pp++;
	strcpy(pp->name, "partition");
	pp->start = dp->p[0].end - 1;
	pp->end = dp->p[0].end;
	dp->npart = 2;

	/*
	 *  read partition table from disk, null terminate
	 */
	b = scsibuf();
	if(scsibread(dev, b, 1, dp->bytes, dp->cap-1) <= 0){
		scsifree(b);
		print("can't read partition block\n");
		return;
	}
	cp = b->virt;
	cp[dp->bytes-1] = 0;

	/*
	 *  parse partition table.
	 */
	n = getfields(cp, line, Npart+1, '\n');
	if(n && strncmp(line[0], MAGIC, sizeof(MAGIC)-1) == 0){
		for(i = 1; i < n; i++){
			pp++;
			if(getfields(line[i], field, 3, ' ') != 3)
				break;
			strncpy(pp->name, field[0], NAMELEN);
			pp->start = strtoul(field[1], 0, 0);
			pp->end = strtoul(field[2], 0, 0);
			if(pp->start > pp->end || pp->start >= dp->p[0].end)
				break;
			dp->npart++;
		}
	}
	scsifree(b);
	return;
}

Partition*
setscsipart(int dev, char *p)
{
	Partition *pp;
	Disc *dp;

	dp = &sdrive[dev];
	for(pp = dp->p; pp < &dp->p[dp->npart]; pp++)
		if(strcmp(pp->name, p) == 0){
			dp->current = pp;
			return pp;
		}
	return 0;
}

static long
scsirw(int dev, int write, char *a, ulong len, ulong offset)
{
	Disc *d;
	Partition *p;
	Scsibuf *b;
	ulong block, n, max, x;

	d = &sdrive[dev];
	p = d->current;
	if(p == 0)
		return -1;

	block = offset / d->bytes + p->start;
	n = (offset + len + d->bytes - 1) / d->bytes + p->start - block;
	max = Maxxfer / d->bytes;
	if(n > max)
		n = max;
	if(block + n > p->end)
		n = p->end - block;
	if(block >= p->end || n == 0)
		return 0;
	b = scsibuf();

	offset %= d->bytes;
	if(write){
		if(offset || len % d->bytes){
			x = scsibread(dev, b, n, d->bytes, block);
			if(x < n * d->bytes){
				n = x / d->bytes;
				x = n * d->bytes - offset;
				if(len > x)
					len = x;
			}
		}
		memmove((char*)b->virt + offset, a, len);
		x = scsibwrite(dev, b, n, d->bytes, block);
		if(x < offset)
			len = 0;
		else if(len > x - offset)
			len = x - offset;
	}else{
		x = scsibread(dev, b, n, d->bytes, block);
		if(x < offset)
			len = 0;
		else if(len > x - offset)
			len = x - offset;
		memmove(a, (char*)b->virt + offset, len);
	}

	scsifree(b);
	return len;
}

long
scsiread(int dev, void *a, long n)
{
	long len;

	len = scsirw(dev, 0, a, n, sdrive[dev].offset);
	if(len > 0)
		sdrive[dev].offset += len;
	return len;
}

extern int (*aha1542reset(void))(Scsi*, int);
extern int (*ultra14freset(void))(Scsi*, int);

static int (*exec)(Scsi*, int);

int
scsiexec(Scsi *p, int rflag)
{
	if(exec == 0)
		return 0;
	return (*exec)(p, rflag);
}

int
scsiinit(void)
{
	int i, online;
	uchar buf[32];

	if((exec = ultra14freset()) == 0 && (exec = aha1542reset()) == 0)
		return 0;
	sdrive = ialloc(8 * sizeof(Disc), 0);
	online = 0;
	for(i = 0; i < 7; i++){
		if(scsiready(i) == 0x20)		/* timeout */
			continue;
		scsisense(i, buf);
		scsistartstop(i, ScsiStartunit);
		scsisense(i, buf);
		if(scsicap(i, buf))
			continue;
		online |= (1<<i);
		sdrive[i].online = 1;
		sdrive[i].cap = ((buf[0]<<24)|(buf[1]<<16)|(buf[2]<<8)|buf[3])+1;
		sdrive[i].bytes = (buf[4]<<24)|(buf[5]<<16)|(buf[6]<<8)|buf[7];
		print("scsi%d: cap %lud, sec %d\n", i, sdrive[i].cap, sdrive[i].bytes);
		scsipart(i);
	}
	return online;
}
