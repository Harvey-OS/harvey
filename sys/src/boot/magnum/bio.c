#include "all.h"

static Scsi	bcmd;
static int	bdev;
static long	maxblock;
static int	blocksize;
static long	base;
static char	*buffer;

#define	BUFSIZE	(4*1024)

void
biogetdev(int dev)
{
	uchar buf[32];

	bdev = dev;
	if(scsiready(bdev)){
		scsisense(bdev, buf);
		if(scsiready(bdev))
			panic("scsi %d.%d not ready: sense 0x%2.2ux, code 0x%2.2ux\n",
				bdev>>3, bdev&7, buf[2], buf[12]);
	}
	if(scsicap(dev, buf) < 0)
		panic("scsicap");
	maxblock = (buf[0]<<24) + (buf[1]<<16) + (buf[2]<<8) + buf[3] + 1;
	blocksize = (buf[4]<<24) + (buf[5]<<16) + (buf[6]<<8) + buf[7];
	buffer = dmaalloc(BUFSIZE);
}

static int
blockread(long blockno, void *p, long len)
{
	Scsi *cmd = &bcmd;

	if(blockno >= maxblock)
		panic("block number out of range\n");
	cmd->target = bdev >> 3;
	cmd->lun = bdev & 7;
	cmd->cmd.base = cmd->cmdblk;
	cmd->cmd.lim = cmd->cmd.base + 6;
	cmd->data.base = p;
	cmd->cmd.ptr = cmd->cmd.base;
	cmd->cmdblk[0] = 0x08;
	cmd->cmdblk[1] = blockno >> 16;
	cmd->cmdblk[2] = blockno >> 8;
	cmd->cmdblk[3] = blockno;
	cmd->cmdblk[4] = len / blocksize;
	cmd->cmdblk[5] = 0x00;
	cmd->data.lim = cmd->data.base + len;
	cmd->data.ptr = cmd->data.base;
	cmd->save = cmd->data.base;
	scsiexec(cmd, 1);
	if(cmd->status != 0x6000){
		print("scsi io failed: biocmd status 0x%4.4ux\n", cmd->status);
		return 1;
	}
	return 0;
}

int
bioread(void *vp, long addr, int len)
{
	long skip, n, m;
	char *p;

	p = vp;
	skip = addr % blocksize;
	for(n = 0; n < len; n += m){
		if(blockread((addr + n) / blocksize, buffer, BUFSIZE))
			return 1;
		m = BUFSIZE - skip;
		if(m + n > len)
			m = len - n;
		memmove(p + n, buffer + skip, m);
		skip = 0;
	}
	return 0;
}

#define MAGIC "plan9 partitions"
long
findpart(char *part)
{
	char *p;
	int i;

	/*
	 *  read partition table from disk, null terminate
	 */
	if(blockread(maxblock-1, buffer, blocksize))
		panic("can't read partitions\n");
	buffer[blocksize-1] = 0;

	/*
	 *  parse partition table.
	 */
	if(strncmp(buffer, MAGIC, sizeof(MAGIC)-1) != 0)
		panic("bad parition magic\n");
	p = buffer;
	while(p = strchr(p, '\n')){
		p++;
		if(strncmp(p, part, strlen(part)) == 0)
			break;
	}
	if(p == 0)
		return -1;
	p = strchr(p, ' ');
	if(p == 0)
		panic("partition table garbled\n");
	return strtoul(p + 1, 0, 0) * blocksize;
}
