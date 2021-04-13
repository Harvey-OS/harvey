/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	<u.h>
#include	<libc.h>
#include	<fcall.h>

int
statcheck(u8 *buf, u32 nbuf)
{
	u8 *ebuf;
	int i;

	ebuf = buf + nbuf;

	if(nbuf < STATFIXLEN || nbuf != BIT16SZ + GBIT16(buf)) {
		return -1;
	}

	buf += STATFIXLEN - 4 * BIT16SZ;

	for(i = 0; i < 4; i++){
		if(buf + BIT16SZ > ebuf) {
			return -1;
		}
		buf += BIT16SZ + GBIT16(buf);
	}

	if(buf != ebuf) {
		return -1;
	}

	return 0;
}

static char nullstring[] = "";

u32
convM2D(u8 *buf, u32 nbuf, Dir *d, char *strs)
{
	u8 *p, *ebuf;
	char *sv[4];
	int i, ns;

	if(nbuf < STATFIXLEN)
		return 0;

	p = buf;
	ebuf = buf + nbuf;

	p += BIT16SZ;	/* ignore size */
	d->type = GBIT16(p);
	p += BIT16SZ;
	d->dev = GBIT32(p);
	p += BIT32SZ;
	d->qid.type = GBIT8(p);
	p += BIT8SZ;
	d->qid.vers = GBIT32(p);
	p += BIT32SZ;
	d->qid.path = GBIT64(p);
	p += BIT64SZ;
	d->mode = GBIT32(p);
	p += BIT32SZ;
	d->atime = GBIT32(p);
	p += BIT32SZ;
	d->mtime = GBIT32(p);
	p += BIT32SZ;
	d->length = GBIT64(p);
	p += BIT64SZ;

	for(i = 0; i < 4; i++){
		if(p + BIT16SZ > ebuf)
			return 0;
		ns = GBIT16(p);
		p += BIT16SZ;
		if(p + ns > ebuf)
			return 0;
		if(strs){
			sv[i] = strs;
			memmove(strs, p, ns);
			strs += ns;
			*strs++ = '\0';
		}
		p += ns;
	}

	if(strs){
		d->name = sv[0];
		d->uid = sv[1];
		d->gid = sv[2];
		d->muid = sv[3];
	}else{
		d->name = nullstring;
		d->uid = nullstring;
		d->gid = nullstring;
		d->muid = nullstring;
	}

	return p - buf;
}

u32
convLM2D(u8 *buf, u32 nbuf, Dir *d)
{
	u8 *p, *ebuf;

	if(nbuf < 22)
		return 0;

	p = buf;
	ebuf = buf + nbuf;
	//qid[13] offset[8] type[1] name[s]

	d->qid.type = GBIT8(p);
	p += BIT8SZ;
	d->qid.vers = GBIT32(p);
	p += BIT32SZ;
	d->qid.path = GBIT64(p);
	p += BIT64SZ;

	// scarf offset in length
	d->length = GBIT64(p);
	p += BIT64SZ;

	d->type = GBIT8(p);
	p += BIT8SZ;
	// ignore the offset.
	// No mode information.
	d->mode = 0;
	// No time information.
	d->atime = 0;
	d->mtime = 0;
	// nothing to do with type for now.

	if(p + BIT16SZ > ebuf)
		return 0;
	//ns = GBIT16(p);
	p += BIT16SZ;
	// TODO: null terminate?
	d->name =  (char *)p;
	return p - buf;
}
