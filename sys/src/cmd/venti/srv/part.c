#include "stdinc.h"
#include <ctype.h>
#include "dat.h"
#include "fns.h"

u32int	maxblocksize;
int	readonly;

static int
strtoullsuf(char *p, char **pp, int rad, u64int *u)
{
	u64int v;

	if(!isdigit((uchar)*p))
		return -1;
	v = strtoull(p, &p, rad);
	switch(*p){
	case 'k':
	case 'K':
		v *= 1024;
		p++;
		break;
	case 'm':
	case 'M':
		v *= 1024*1024;
		p++;
		break;
	case 'g':
	case 'G':
		v *= 1024*1024*1024;
		p++;
		break;
	case 't':
	case 'T':
		v *= 1024*1024;
		v *= 1024*1024;
		p++;
		break;
	}
	*pp = p;
	*u = v;
	return 0;
}
	
static int
parsepart(char *name, char **file, u64int *lo, u64int *hi)
{
	char *p;

	*file = estrdup(name);
	if((p = strrchr(*file, ':')) == nil){
		*lo = 0;
		*hi = 0;
		return 0;
	}
	*p++ = 0;
	if(*p == '-')
		*lo = 0;
	else{
		if(strtoullsuf(p, &p, 0, lo) < 0){
			free(*file);
			return -1;
		}
	}
	if(*p == '-')
		p++;
	if(*p == 0){
		*hi = 0;
		return 0;
	}
	if(strtoullsuf(p, &p, 0, hi) < 0 || *p != 0){
		free(*file);
		return -1;
	}
	return 0;
}

Part*
initpart(char *name, int mode)
{
	Part *part;
	Dir *dir;
	char *file;
	u64int lo, hi;

	if(parsepart(name, &file, &lo, &hi) < 0)
		return nil;
	trace(TraceDisk, "initpart %s file %s lo 0x%llx hi 0x%llx", name, file, lo, hi);
	part = MKZ(Part);
	part->name = estrdup(name);
	part->filename = estrdup(file);
	if(readonly){
		mode &= ~(OREAD|OWRITE|ORDWR);
		mode |= OREAD;
	}
	part->fd = open(file, mode);
	if(part->fd < 0){
		if((mode&(OREAD|OWRITE|ORDWR)) == ORDWR)
			part->fd = open(file, (mode&~ORDWR)|OREAD);
		if(part->fd < 0){
			freepart(part);
			fprint(2, "can't open partition='%s': %r\n", file);
			seterr(EOk, "can't open partition='%s': %r", file);
			fprint(2, "%r\n");
			free(file);
			return nil;
		}
		fprint(2, "warning: %s opened for reading only\n", name);
	}
	part->offset = lo;
	dir = dirfstat(part->fd);
	if(dir == nil){
		freepart(part);
		seterr(EOk, "can't stat partition='%s': %r", file);
		free(file);
		return nil;
	}
	if(dir->length == 0){
		free(dir);
		freepart(part);
		seterr(EOk, "can't determine size of partition %s", file);
		free(file);
		return nil;
	}
	if(dir->length < hi || dir->length < lo){
		freepart(part);
		seterr(EOk, "partition '%s': bounds out of range (max %lld)", name, dir->length);
		free(dir);
		free(file);
		return nil;
	}
	if(hi == 0)
		hi = dir->length;
	part->size = hi - part->offset;
	free(dir);
	return part;
}

int
flushpart(Part *part)
{
	USED(part);
	return 0;
}

void
freepart(Part *part)
{
	if(part == nil)
		return;
	if(part->fd >= 0)
		close(part->fd);
	free(part->name);
	free(part);
}

void
partblocksize(Part *part, u32int blocksize)
{
	if(part->blocksize)
		sysfatal("resetting partition=%s's block size", part->name);
	part->blocksize = blocksize;
	if(blocksize > maxblocksize)
		maxblocksize = blocksize;
}

enum {
	Maxxfer = 64*1024,	/* for NCR SCSI controllers; was 128K */
};

static int reopen(Part*);

int
rwpart(Part *part, int isread, u64int offset0, u8int *buf0, u32int count0)
{
	u32int count, opsize;
	int n;
	u8int *buf;
	u64int offset;

	trace(TraceDisk, "%s %s %ud at 0x%llx", 
		isread ? "read" : "write", part->name, count0, offset0);
	if(offset0 >= part->size || offset0+count0 > part->size){
		seterr(EStrange, "out of bounds %s offset 0x%llux count %ud to partition %s size 0x%llux",
			isread ? "read" : "write", offset0, count0, part->name,
			part->size);
		return -1;
	}

	buf = buf0;
	count = count0;
	offset = offset0;
	while(count > 0){
		opsize = count;
		if(opsize > Maxxfer)
			opsize = Maxxfer;
		if(isread)
			n = pread(part->fd, buf, opsize, offset);
		else
			n = pwrite(part->fd, buf, opsize, offset);
		if(n <= 0){
			seterr(EAdmin, "%s %s offset 0x%llux count %ud buf %p returned %d: %r",
				isread ? "read" : "write", part->filename, offset, opsize, buf, n);
			return -1;
		}
		offset += n;
		count -= n;
		buf += n;
	}

	return count0;
}

int
readpart(Part *part, u64int offset, u8int *buf, u32int count)
{
	return rwpart(part, 1, offset, buf, count);
}

int
writepart(Part *part, u64int offset, u8int *buf, u32int count)
{
	return rwpart(part, 0, offset, buf, count);
}

ZBlock*
readfile(char *name)
{
	Part *p;
	ZBlock *b;

	p = initpart(name, OREAD);
	if(p == nil)
		return nil;
	b = alloczblock(p->size, 0, p->blocksize);
	if(b == nil){
		seterr(EOk, "can't alloc %s: %r", name);
		freepart(p);
		return nil;
	}
	if(readpart(p, 0, b->data, p->size) < 0){
		seterr(EOk, "can't read %s: %r", name);
		freepart(p);
		freezblock(b);
		return nil;
	}
	freepart(p);
	return b;
}
