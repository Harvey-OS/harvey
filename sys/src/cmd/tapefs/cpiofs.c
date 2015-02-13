/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include "tapefs.h"

/*
 * File system for cpio tapes (read-only)
 */

#define TBLOCK	512
#define NBLOCK	40	/* maximum blocksize */
#define DBLOCK	20	/* default blocksize */
#define TNAMSIZ	100

union hblock {
	int8_t dummy[TBLOCK];
	int8_t tbuf[Maxbuf];
	struct header {
		int8_t magic[6];
		int8_t dev[6];
		int8_t ino[6];
		int8_t mode[6];
		int8_t uid[6];
		int8_t gid[6];
		int8_t nlink[6];
		int8_t rdev[6];
		int8_t mtime[11];
		int8_t namesize[6];
		int8_t size[11];
	} dbuf;
	struct hname {
		struct	header x;
		int8_t	name[1];
	} nbuf;
} dblock;

int	tapefile;
int64_t	getoct(int8_t*, int);

void
populate(int8_t *name)
{
	int64_t offset;
	int32_t isabs, magic, namesize, mode;
	Fileinf f;

	tapefile = open(name, OREAD);
	if (tapefile<0)
		error("Can't open argument file");
	replete = 1;
	for (offset = 0;;) {
		seek(tapefile, offset, 0);
		if (read(tapefile, (int8_t *)&dblock.dbuf, TBLOCK)<TBLOCK)
			break;
		magic = getoct(dblock.dbuf.magic, sizeof(dblock.dbuf.magic));
		if (magic != 070707){
			print("%lo\n", magic);
			error("out of phase--get help");
		}
		if (dblock.nbuf.name[0]=='\0' || strcmp(dblock.nbuf.name, "TRAILER!!!")==0)
			break;
		mode = getoct(dblock.dbuf.mode, sizeof(dblock.dbuf.mode));
		f.mode = mode&0777;
		switch(mode & 0170000) {
		case 0040000:
			f.mode |= DMDIR;
			break;
		case 0100000:
			break;
		default:
			f.mode = 0;
			break;
		}
		f.uid = getoct(dblock.dbuf.uid, sizeof(dblock.dbuf.uid));
		f.gid = getoct(dblock.dbuf.gid, sizeof(dblock.dbuf.gid));
		f.size = getoct(dblock.dbuf.size, sizeof(dblock.dbuf.size));
		f.mdate = getoct(dblock.dbuf.mtime, sizeof(dblock.dbuf.mtime));
		namesize = getoct(dblock.dbuf.namesize, sizeof(dblock.dbuf.namesize));
		f.addr = offset+sizeof(struct header)+namesize;
		isabs = dblock.nbuf.name[0]=='/';
		f.name = &dblock.nbuf.name[isabs];
		poppath(f, 1);
		offset += sizeof(struct header)+namesize+f.size;
	}
}

int64_t
getoct(int8_t *p, int l)
{
	int64_t r;

	for (r=0; l>0; p++, l--){
		r <<= 3;
		r += *p-'0';
	}
	return r;
}

void
dotrunc(Ram *r)
{
	USED(r);
}

void
docreate(Ram *r)
{
	USED(r);
}

int8_t *
doread(Ram *r, int64_t off, int32_t cnt)
{
	seek(tapefile, r->addr+off, 0);
	if (cnt>sizeof(dblock.tbuf))
		error("read too big");
	read(tapefile, dblock.tbuf, cnt);
	return dblock.tbuf;
}

void
popdir(Ram *r)
{
	USED(r);
}

void
dowrite(Ram *r, int8_t *buf, int32_t off, int32_t cnt)
{
	USED(r); USED(buf); USED(off); USED(cnt);
}

int
dopermw(Ram *r)
{
	USED(r);
	return 0;
}
