#include "stdinc.h"
#include "dat.h"
#include "fns.h"
#include "error.h"

static void diskThread(void *a);

enum {
	QueueSize = 100,		/* maximum block to queue */
};

struct Disk {
	VtLock *lk;
	int ref;

	int fd;
	Header h;

	VtRendez *flow;
	VtRendez *starve;
	VtRendez *flush;
	VtRendez *die;

	int nqueue;

	Block *cur;		/* block to do on current scan */
	Block *next;		/* blocks to do next scan */
};


Disk *
diskAlloc(int fd)
{
	u8int buf[HeaderSize];
	Header h;
	Disk *disk;
	
	if(pread(fd, buf, HeaderSize, HeaderOffset) < HeaderSize){
		vtOSError();
		return nil;
	}

	if(!headerUnpack(&h, buf))
		return nil;
	disk = vtMemAllocZ(sizeof(Disk));
	disk->lk = vtLockAlloc();
	disk->starve = vtRendezAlloc(disk->lk);
	disk->flow = vtRendezAlloc(disk->lk);
	disk->flush = vtRendezAlloc(disk->lk);
	disk->fd = fd;
	disk->h = h;

	disk->ref = 2;
	vtThread(diskThread, disk);

	return disk;
}

void
diskFree(Disk *disk)
{
	diskFlush(disk);

	/* kill slave */
	vtLock(disk->lk);
	disk->die = vtRendezAlloc(disk->lk);
	vtWakeup(disk->starve);
	while(disk->ref > 1)
		vtSleep(disk->die);
	vtUnlock(disk->lk);
	vtRendezFree(disk->flow);
	vtRendezFree(disk->starve);
	vtRendezFree(disk->die);
	vtLockFree(disk->lk);
	close(disk->fd);
	vtMemFree(disk);
}

static u32int
partStart(Disk *disk, int part)
{
	switch(part){
	default:
		assert(0);
	case PartSuper:
		return disk->h.super;
	case PartLabel:
		return disk->h.label;
	case PartData:
		return disk->h.data;
	}
}


static u32int
partEnd(Disk *disk, int part)
{
	switch(part){
	default:
		assert(0);
	case PartSuper:
		return disk->h.super+1;
	case PartLabel:
		return disk->h.data;
	case PartData:
		return disk->h.end;
	}
}

int
diskReadRaw(Disk *disk, int part, u32int addr, uchar *buf)
{
	ulong start, end;
	u64int offset;
	int n, nn;

	start = partStart(disk, part);
	end = partEnd(disk, part);

	if(addr >= end-start){
		vtSetError(EBadAddr);
		return 0;
	}

	offset = ((u64int)(addr + start))*disk->h.blockSize;
	n = disk->h.blockSize;
	while(n > 0){
		nn = pread(disk->fd, buf, n, offset);
		if(nn < 0){
			vtOSError();
			return 0;
		}
		if(nn == 0){
			vtSetError(EIO);
			return 0;
		}
		n -= nn;
		offset += nn;
		buf += nn;
	}
	return 1;
}

int
diskWriteRaw(Disk *disk, int part, u32int addr, uchar *buf)
{
	ulong start, end;
	u64int offset;
	int n;

	start = partStart(disk, part);
	end = partEnd(disk, part);

	if(addr >= end-start){
		vtSetError(EBadAddr);
		return 0;
	}

	offset = ((u64int)(addr + start))*disk->h.blockSize;
	n = disk->h.blockSize;
	if(pwrite(disk->fd, buf, n, offset) < n){
		vtOSError();
		return 0;
	}

	return 1;
}

static void
diskQueue(Disk *disk, Block *b)
{
	Block **bp, *bb;

	vtLock(disk->lk);
	while(disk->nqueue >= QueueSize)
		vtSleep(disk->flow);
	if(disk->cur == nil || b->addr > disk->cur->addr)
		bp = &disk->cur;
	else
		bp = &disk->next;

	for(bb=*bp; bb; bb=*bp){
		if(b->addr < bb->addr)
			break;
		bp = &bb->ionext;
	}
	b->ionext = bb;	
	*bp = b;
	if(disk->nqueue == 0)
		vtWakeup(disk->starve);
	disk->nqueue++;
	vtUnlock(disk->lk);
}


void
diskRead(Disk *disk, Block *b)
{
	assert(b->iostate == BioEmpty || b->iostate == BioLabel);
	blockSetIOState(b, BioReading);
	diskQueue(disk, b);
}

void
diskWrite(Disk *disk, Block *b)
{
	assert(b->iostate == BioDirty);
	blockSetIOState(b, BioWriting);
	diskQueue(disk, b);
}

int
diskBlockSize(Disk *disk)
{
	return disk->h.blockSize;	/* immuttable */
}

int
diskFlush(Disk *disk)
{
	Dir dir;

	vtLock(disk->lk);
	while(disk->nqueue > 0)
		vtSleep(disk->flush);
	vtUnlock(disk->lk);

	/* there really should be a cleaner interface to flush an fd */
	nulldir(&dir);
	if(dirfwstat(disk->fd, &dir) < 0){
		vtOSError();
		return 0;
	}
	return 1;
}

u32int
diskSize(Disk *disk, int part)
{
	return partEnd(disk, part) - partStart(disk, part);
}

static void
diskThread(void *a)
{
	Disk *disk = a;
	Block *b;
	uchar *buf, *p;
	double t;
	int nio;

	vtThreadSetName("disk");

fprint(2, "diskThread %d\n", getpid());

	buf = vtMemAlloc(disk->h.blockSize);

	vtLock(disk->lk);
	nio = 0;
	t = -nsec();
	for(;;){
		while(disk->nqueue == 0){
			t += nsec();
if(nio >= 10000){
fprint(2, "disk: io=%d at %.3fms\n", nio, t*1e-6/nio);
nio = 0;
t = 0.;
}
			if(disk->die != nil)
				goto Done;
			vtSleep(disk->starve);
			t -= nsec();
		}
		assert(disk->cur != nil || disk->next != nil);

		if(disk->cur == nil){
			disk->cur = disk->next;
			disk->next = nil;
		}
		b = disk->cur;
		disk->cur = b->ionext;
		vtUnlock(disk->lk);

		/*
		 * no one should hold onto blocking in the
		 * reading or writing state, so this lock should
		 * not cause deadlock.
		 */
if(0)fprint(2, "diskThread: %d:%d %x\n", getpid(), b->part, b->addr);
		bwatchLock(b);
		vtLock(b->lk);
		assert(b->nlock == 1);

		switch(b->iostate){
		default:
			abort();
		case BioReading:
			if(!diskReadRaw(disk, b->part, b->addr, b->data)){
fprint(2, "diskReadRaw failed: part=%d addr=%ux: %r", b->part, b->addr);
				blockSetIOState(b, BioReadError);
			}else
				blockSetIOState(b, BioClean);
			break;
		case BioWriting:
			p = blockRollback(b, buf);
			if(!diskWriteRaw(disk, b->part, b->addr, p)){
fprint(2, "diskWriteRaw failed: part=%d addr=%ux: %r", b->part, b->addr);
				break;
			}
			if(p != buf)
				blockSetIOState(b, BioClean);
			else
				blockSetIOState(b, BioDirty);
			break;
		}
		
		blockPut(b);		/* remove extra reference, unlock */
		vtLock(disk->lk);
		disk->nqueue--;
		if(disk->nqueue == QueueSize-1)
			vtWakeup(disk->flow);
		if(disk->nqueue == 0)
			vtWakeup(disk->flush);
		nio++;
	}
Done:
fprint(2, "diskThread done\n");
	disk->ref--;
	vtWakeup(disk->die);
	vtUnlock(disk->lk);
	vtMemFree(buf);
}
