/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "stdinc.h"
#include <ctype.h>
#include "dat.h"
#include "fns.h"

uint32_t	maxblocksize;
int	readonly;

static int
strtoullsuf(char *p, char **pp, int rad, uint64_t *u)
{
	uint64_t v;

	if(!isdigit((unsigned char)*p))
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
parsepart(char *name, char **file, uint64_t *lo, uint64_t *hi)
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
	uint64_t lo, hi;

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
partblocksize(Part *part, uint32_t blocksize)
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

int
rwpart(Part *part, int isread, uint64_t offset0, uint8_t *buf0,
       uint32_t count0)
{
	uint32_t count, opsize;
	int n;
	uint8_t *buf;
	uint64_t offset;

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
readpart(Part *part, uint64_t offset, uint8_t *buf, uint32_t count)
{
	return rwpart(part, 1, offset, buf, count);
}

int
writepart(Part *part, uint64_t offset, uint8_t *buf, uint32_t count)
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
