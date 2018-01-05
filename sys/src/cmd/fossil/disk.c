/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "stdinc.h"
#include "dat.h"
#include "fns.h"
#include "error.h"

static void diskThread(void *a);

enum {
	/*
	 * disable measurement since it gets alignment faults on BG
	 * and the guts used to be commented out.
	 */
	Timing	= 0,			/* flag */
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

/* keep in sync with Part* enum in dat.h */
static char *partname[] = {
	[PartError]	= "error",
	[PartSuper]	= "super",
	[PartLabel]	= "label",
	[PartData]	= "data",
	[PartVenti]	= "venti",
};

Disk *
diskAlloc(int fd)
{
	uint8_t buf[HeaderSize];
	Header h;
	Disk *disk;

	if(pread(fd, buf, HeaderSize, HeaderOffset) < HeaderSize){
		vtSetError("short read: %r");
		vtOSError();
		return nil;
	}

	if(!headerUnpack(&h, buf)){
		vtSetError("bad disk header");
		return nil;
	}
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

static uint32_t
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


static uint32_t
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
diskReadRaw(Disk *disk, int part, uint32_t addr, uint8_t *buf)
{
	uint32_t start, end;
	uint64_t offset;
	int n, nn;

	start = partStart(disk, part);
	end = partEnd(disk, part);

	//print("Inicio: %u Fin: %u\n", start, end);
	//print("Diferencia: %u >= %d\n", addr, end-start);
	if(addr >= end-start){
		vtSetError(EBadAddr);
		return 0;
	}

	offset = ((uint64_t)(addr + start))*disk->h.blockSize;
	//print("Offset: %u Blocksize: %u \n",offset, disk->h.blockSize);
	n = disk->h.blockSize;
	while(n > 0){
		nn = pread(disk->fd, buf, n, offset);
		if(nn < 0){
			vtOSError();
			return 0;
		}
		if(nn == 0){
			vtSetError("eof reading disk");
			return 0;
		}
		n -= nn;
		offset += nn;
		buf += nn;
	}
	return 1;
}

int
diskWriteRaw(Disk *disk, int part, uint32_t addr, uint8_t *buf)
{
	uint32_t start, end;
	uint64_t offset;
	int n;

	start = partStart(disk, part);
	end = partEnd(disk, part);

	if(addr >= end - start){
		vtSetError(EBadAddr);
		return 0;
	}

	offset = ((uint64_t)(addr + start))*disk->h.blockSize;
	n = pwrite(disk->fd, buf, disk->h.blockSize, offset);
	if(n < 0){
		vtOSError();
		return 0;
	}
	if(n < disk->h.blockSize) {
		vtSetError("short write");
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
	assert(b->nlock == 1);
	assert(b->iostate == BioDirty);
	blockSetIOState(b, BioWriting);
	diskQueue(disk, b);
}

void
diskWriteAndWait(Disk *disk, Block *b)
{
	int nlock;

	/*
	 * If b->nlock > 1, the block is aliased within
	 * a single thread.  That thread is us.
	 * DiskWrite does some funny stuff with VtLock
	 * and blockPut that basically assumes b->nlock==1.
	 * We humor diskWrite by temporarily setting
	 * nlock to 1.  This needs to be revisited.
	 */
	nlock = b->nlock;
	if(nlock > 1)
		b->nlock = 1;
	diskWrite(disk, b);
	while(b->iostate != BioClean)
		vtSleep(b->ioready);
	b->nlock = nlock;
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

uint32_t
diskSize(Disk *disk, int part)
{
	return partEnd(disk, part) - partStart(disk, part);
}

static uintptr
mypc(int x)
{
	return getcallerpc();
}

static char *
disk2file(Disk *disk)
{
	static char buf[256];

	if (fd2path(disk->fd, buf, sizeof buf) < 0)
		strncpy(buf, "GOK", sizeof buf);
	return buf;
}

static void
diskThread(void *a)
{
	Disk *disk = a;
	Block *b;
	uint8_t *buf, *p;
	double t;
	int nio;

	vtThreadSetName("disk");

//fprint(2, "diskThread %d\n", getpid());

	buf = vtMemAlloc(disk->h.blockSize);

	vtLock(disk->lk);
	if (Timing) {
		nio = 0;
		t = -nsec();
	}
	for(;;){
		while(disk->nqueue == 0){
			if (Timing) {
				t += nsec();
				if(nio >= 10000){
					fprint(2, "disk: io=%d at %.3fms\n",
						nio, t*1e-6/nio);
					nio = 0;
					t = 0;
				}
			}
			if(disk->die != nil)
				goto Done;
			vtSleep(disk->starve);
			if (Timing)
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
if(0)fprint(2, "fossil: diskThread: %d:%d %x\n", getpid(), b->part, b->addr);
		bwatchLock(b);
		vtLock(b->lk);
		b->pc = mypc(0);
		assert(b->nlock == 1);
		switch(b->iostate){
		default:
			abort();
		case BioReading:
			if(!diskReadRaw(disk, b->part, b->addr, b->data)){
				fprint(2, "fossil: diskReadRaw failed: %s: "
					"score %V: part=%s block %u: %r\n",
					disk2file(disk), b->score,
					partname[b->part], b->addr);
				blockSetIOState(b, BioReadError);
			}else
				blockSetIOState(b, BioClean);
			break;
		case BioWriting:
			p = blockRollback(b, buf);
			/* NB: ctime result ends with a newline */
			if(!diskWriteRaw(disk, b->part, b->addr, p)){
				fprint(2, "fossil: diskWriteRaw failed: %s: "
				    "score %V: date %s part=%s block %u: %r\n",
					disk2file(disk), b->score,
					ctime(time(0)),
					partname[b->part], b->addr);
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
		if(Timing)
			nio++;
	}
Done:
//fprint(2, "diskThread done\n");
	disk->ref--;
	vtWakeup(disk->die);
	vtUnlock(disk->lk);
	vtMemFree(buf);
}
