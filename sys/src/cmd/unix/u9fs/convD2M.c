/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <plan9.h>
#include <fcall.h>

uint
sizeD2M(Dir *d)
{
	char *sv[4];
	int i, ns;

	sv[0] = d->name;
	sv[1] = d->uid;
	sv[2] = d->gid;
	sv[3] = d->muid;

	ns = 0;
	for(i = 0; i < 4; i++)
		ns += strlen(sv[i]);

	return STATFIXLEN + ns;
}

uint
convD2M(Dir *d, uint8_t *buf, uint nbuf)
{
	uint8_t *p, *ebuf;
	char *sv[4];
	int i, ns, nsv[4], ss;

	if(nbuf < BIT16SZ)
		return 0;

	p = buf;
	ebuf = buf + nbuf;

	sv[0] = d->name;
	sv[1] = d->uid;
	sv[2] = d->gid;
	sv[3] = d->muid;

	ns = 0;
	for(i = 0; i < 4; i++) {
		nsv[i] = strlen(sv[i]);
		ns += nsv[i];
	}

	ss = STATFIXLEN + ns;

	/* set size befor erroring, so user can know how much is needed */
	/* note that length excludes count field itself */
	PBIT16(p, ss - BIT16SZ);
	p += BIT16SZ;

	if(ss > nbuf)
		return BIT16SZ;

	PBIT16(p, d->type);
	p += BIT16SZ;
	PBIT32(p, d->dev);
	p += BIT32SZ;
	PBIT8(p, d->qid.type);
	p += BIT8SZ;
	PBIT32(p, d->qid.vers);
	p += BIT32SZ;
	PBIT64(p, d->qid.path);
	p += BIT64SZ;
	PBIT32(p, d->mode);
	p += BIT32SZ;
	PBIT32(p, d->atime);
	p += BIT32SZ;
	PBIT32(p, d->mtime);
	p += BIT32SZ;
	PBIT64(p, d->length);
	p += BIT64SZ;

	for(i = 0; i < 4; i++) {
		ns = nsv[i];
		if(p + ns + BIT16SZ > ebuf)
			return 0;
		PBIT16(p, ns);
		p += BIT16SZ;
		memmove(p, sv[i], ns);
		p += ns;
	}

	if(ss != p - buf)
		return 0;

	return p - buf;
}
