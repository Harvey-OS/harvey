#include "stdinc.h"
#include "dat.h"
#include "fns.h"

u32int	maxBlockSize;
int	readonly;

Part*
initPart(char *name, int writable)
{
	Part *part;
	Dir *dir;
	int how;

	part = MK(Part);
	part->name = estrdup(name);
	if(!writable && readonly)
		how = OREAD;
	else
		how = ORDWR;
	part->fd = open(name, how);
	if(part->fd < 0){
		if(how == ORDWR)
			part->fd = open(name, OREAD);
		if(part->fd < 0){
			freePart(part);
			setErr(EOk, "can't open partition='%s': %r", name);
			return nil;
		}
		fprint(2, "warning: %s opened for reading only\n", name);
	}
	dir = dirfstat(part->fd);
	if(dir == nil){
		freePart(part);
		setErr(EOk, "can't stat partition='%s': %r", name);
		return nil;
	}
	part->size = dir->length;
	part->blockSize = 0;
	free(dir);
	return part;
}

void
freePart(Part *part)
{
	if(part == nil)
		return;
	close(part->fd);
	free(part->name);
	free(part);
}

void
partBlockSize(Part *part, u32int blockSize)
{
	if(part->blockSize)
		fatal("resetting partition=%s's block size", part->name);
	part->blockSize = blockSize;
	if(blockSize > maxBlockSize)
		maxBlockSize = blockSize;
}

int
writePart(Part *part, u64int addr, u8int *buf, u32int n)
{
	long m, mm, nn;

	vtLock(stats.lock);
	stats.diskWrites++;
	stats.diskBWrites += n;
	vtUnlock(stats.lock);

	if(addr > part->size || addr + n > part->size){
		setErr(ECorrupt, "out of bounds write to partition='%s'", part->name);
		return 0;
	}
	for(nn = 0; nn < n; nn += m){
		mm = n - nn;
		if(mm > MaxIo)
			mm = MaxIo;
		m = pwrite(part->fd, &buf[nn], mm, addr + nn);
		if(m != mm){
			if(m < 0){
				setErr(EOk, "can't write partition='%s': %r", part->name);
				return 0;
			}
			logErr(EOk, "truncated write to partition='%s' n=%ld wrote=%ld", part->name, mm, m);
		}
	}
	return 1;
}

int
readPart(Part *part, u64int addr, u8int *buf, u32int n)
{
	long m, mm, nn;

	vtLock(stats.lock);
	stats.diskReads++;
	stats.diskBReads += n;
	vtUnlock(stats.lock);

	if(addr > part->size || addr + n > part->size){
		setErr(ECorrupt, "out of bounds read from partition='%s': addr=%lld n=%d size=%lld", part->name, addr, n, part->size);
		return 0;
	}
	for(nn = 0; nn < n; nn += m){
		mm = n - nn;
		if(mm > MaxIo)
			mm = MaxIo;
		m = pread(part->fd, &buf[nn], mm, addr + nn);
		if(m != mm){
			if(m < 0){
				setErr(EOk, "can't read partition='%s': %r", part->name);
				return 0;
			}
			logErr(EOk, "warning: truncated read from partition='%s' n=%ld read=%ld", part->name, mm, m);
		}
	}
	return 1;
}
