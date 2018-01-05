/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "all.h"
#include "9p1.h"

#define	CHAR(x)		*p++ = f->x
#define	SHORT(x)	{ uint32_t vvv = f->x; p[0] = vvv; p[1] = vvv>>8; p += 2; }
#define	VLONG(q)	p[0] = (q); p[1] = (q)>>8; p[2] = (q)>>16; p[3] = (q)>>24; p += 4
#define	LONG(x)		{ uint32_t vvv = f->x; VLONG(vvv); }
#define	BYTES(x,n)	memmove(p, f->x, n); p += n
#define	STRING(x,n)	strncpy((char*)p, f->x, n); p += n

int
convS2M9p1(Oldfcall *f, uint8_t *ap)
{
	uint8_t *p;
	int t;

	p = ap;
	CHAR(type);
	t = f->type;
	SHORT(tag);
	switch(t)
	{
	default:
		print("convS2M9p1: bad type: %d\n", t);
		return 0;

	case Tnop9p1:
	case Tosession9p1:
		break;

	case Tsession9p1:
		BYTES(chal, sizeof(f->chal));
		break;

	case Tflush9p1:
		SHORT(oldtag);
		break;

	case Tattach9p1:
		SHORT(fid);
		STRING(uname, sizeof(f->uname));
		STRING(aname, sizeof(f->aname));
		BYTES(ticket, sizeof(f->ticket));
		BYTES(auth, sizeof(f->auth));
		break;

	case Toattach9p1:
		SHORT(fid);
		STRING(uname, sizeof(f->uname));
		STRING(aname, sizeof(f->aname));
		BYTES(ticket, NAMELEN);
		break;

	case Tclone9p1:
		SHORT(fid);
		SHORT(newfid);
		break;

	case Twalk9p1:
		SHORT(fid);
		STRING(name, sizeof(f->name));
		break;

	case Tclwalk9p1:
		SHORT(fid);
		SHORT(newfid);
		STRING(name, sizeof(f->name));
		break;

	case Topen9p1:
		SHORT(fid);
		CHAR(mode);
		break;

	case Tcreate9p1:
		SHORT(fid);
		STRING(name, sizeof(f->name));
		LONG(perm);
		CHAR(mode);
		break;

	case Tread9p1:
		SHORT(fid);
		LONG(offset); VLONG(0);
		SHORT(count);
		break;

	case Twrite9p1:
		SHORT(fid);
		LONG(offset); VLONG(0);
		SHORT(count);
		p++;
		if((uint8_t*)p == (uint8_t*)f->data) {
			p += f->count;
			break;
		}
		BYTES(data, f->count);
		break;

	case Tclunk9p1:
	case Tremove9p1:
	case Tstat9p1:
		SHORT(fid);
		break;

	case Twstat9p1:
		SHORT(fid);
		BYTES(stat, sizeof(f->stat));
		break;
/*
 */
	case Rnop9p1:
	case Rosession9p1:
	case Rflush9p1:
		break;

	case Rsession9p1:
		BYTES(chal, sizeof(f->chal));
		BYTES(authid, sizeof(f->authid));
		BYTES(authdom, sizeof(f->authdom));
		break;

	case Rerror9p1:
		STRING(ename, sizeof(f->ename));
		break;

	case Rclone9p1:
	case Rclunk9p1:
	case Rremove9p1:
	case Rwstat9p1:
		SHORT(fid);
		break;

	case Rwalk9p1:
	case Ropen9p1:
	case Rcreate9p1:
	case Rclwalk9p1:
		SHORT(fid);
		LONG(qid.path);
		LONG(qid.version);
		break;

	case Rattach9p1:
		SHORT(fid);
		LONG(qid.path);
		LONG(qid.version);
		BYTES(rauth, sizeof(f->rauth));
		break;

	case Roattach9p1:
		SHORT(fid);
		LONG(qid.path);
		LONG(qid.version);
		break;

	case Rread9p1:
		SHORT(fid);
		SHORT(count);
		p++;
		if((uint8_t*)p == (uint8_t*)f->data) {
			p += f->count;
			break;
		}
		BYTES(data, f->count);
		break;

	case Rwrite9p1:
		SHORT(fid);
		SHORT(count);
		break;

	case Rstat9p1:
		SHORT(fid);
		BYTES(stat, sizeof(f->stat));
		break;
	}
	return p - (uint8_t*)ap;
}

/*
 * buggery to give false qid for
 * the top 2 levels of the dump fs
 */
static uint32_t
fakeqid9p1(Dentry *f)
{
	uint32_t q;
	int c;

	q = f->qid.path;
	if(q == (QPROOT|QPDIR)) {
		c = f->name[0];
		if(c >= '0' && c <= '9') {
			q = 3|QPDIR;
			c = (c-'0')*10 + (f->name[1]-'0');
			if(c >= 1 && c <= 12)
				q = 4|QPDIR;
		}
	}
	return q;
}

int
convD2M9p1(Dentry *f, char *ap)
{
	uint8_t *p;
	uint32_t q;

	p = (uint8_t*)ap;
	STRING(name, sizeof(f->name));

	memset(p, 0, 2*NAMELEN);
	uidtostr((char*)p, f->uid);
	p += NAMELEN;

	uidtostr((char*)p, f->gid);
	p += NAMELEN;

	q = fakeqid9p1(f);
	VLONG(q);
	LONG(qid.version);
	{
		q = f->mode & 0x0fff;
		if(f->mode & DDIR)
			q |= PDIR;
		if(f->mode & DAPND)
			q |= PAPND;
		if(f->mode & DLOCK)
			q |= PLOCK;
		VLONG(q);
	}
	LONG(atime);
	LONG(mtime);
	LONG(size); VLONG(0);
	VLONG(0);
	return p - (uint8_t*)ap;
}

#undef	CHAR
#undef	SHORT
#undef	LONG
#undef	VLONG
#undef	BYTES
#undef	STRING

#define	CHAR(x)		f->x = *p++
#define	SHORT(x)	f->x = (p[0] | (p[1]<<8)); p += 2
#define	VLONG(q)	q = (p[0] | (p[1]<<8) | (p[2]<<16) | (p[3]<<24)); p += 4
#define	LONG(x)		VLONG(f->x)
#define	BYTES(x,n)	memmove(f->x, p, n); p += n
#define	STRING(x,n)	memmove(f->x, p, n); p += n

int
convM2S9p1(uint8_t *ap, Oldfcall *f, int n)
{
	uint8_t *p;
	int t;

	p = ap;
	CHAR(type);
	t = f->type;
	SHORT(tag);
	switch(t)
	{
	default:
		/*
		 * only whine if it couldn't be a 9P2000 Tversion9p1.
		 */
		if(t != 19 || ap[4] != 100)
			print("convM2S9p1: bad type: %d\n", f->type);
		return 0;

	case Tnop9p1:
	case Tosession9p1:
		break;

	case Tsession9p1:
		BYTES(chal, sizeof(f->chal));
		break;

	case Tflush9p1:
		SHORT(oldtag);
		break;

	case Tattach9p1:
		SHORT(fid);
		BYTES(uname, sizeof(f->uname));
		BYTES(aname, sizeof(f->aname));
		BYTES(ticket, sizeof(f->ticket));
		BYTES(auth, sizeof(f->auth));
		break;

	case Toattach9p1:
		SHORT(fid);
		BYTES(uname, sizeof(f->uname));
		BYTES(aname, sizeof(f->aname));
		BYTES(ticket, NAMELEN);
		break;

	case Tclone9p1:
		SHORT(fid);
		SHORT(newfid);
		break;

	case Twalk9p1:
		SHORT(fid);
		BYTES(name, sizeof(f->name));
		break;

	case Tclwalk9p1:
		SHORT(fid);
		SHORT(newfid);
		BYTES(name, sizeof(f->name));
		break;

	case Tremove9p1:
		SHORT(fid);
		break;

	case Topen9p1:
		SHORT(fid);
		CHAR(mode);
		break;

	case Tcreate9p1:
		SHORT(fid);
		BYTES(name, sizeof(f->name));
		LONG(perm);
		CHAR(mode);
		break;

	case Tread9p1:
		SHORT(fid);
		LONG(offset); p += 4;
		SHORT(count);
		break;

	case Twrite9p1:
		SHORT(fid);
		LONG(offset); p += 4;
		SHORT(count);
		p++;
		f->data = (char*)p; p += f->count;
		break;

	case Tclunk9p1:
	case Tstat9p1:
		SHORT(fid);
		break;

	case Twstat9p1:
		SHORT(fid);
		BYTES(stat, sizeof(f->stat));
		break;

/*
 */
	case Rnop9p1:
	case Rosession9p1:
		break;

	case Rsession9p1:
		BYTES(chal, sizeof(f->chal));
		BYTES(authid, sizeof(f->authid));
		BYTES(authdom, sizeof(f->authdom));
		break;

	case Rerror9p1:
		BYTES(ename, sizeof(f->ename));
		break;

	case Rflush9p1:
		break;

	case Rclone9p1:
	case Rclunk9p1:
	case Rremove9p1:
	case Rwstat9p1:
		SHORT(fid);
		break;

	case Rwalk9p1:
	case Rclwalk9p1:
	case Ropen9p1:
	case Rcreate9p1:
		SHORT(fid);
		LONG(qid.path);
		LONG(qid.version);
		break;

	case Rattach9p1:
		SHORT(fid);
		LONG(qid.path);
		LONG(qid.version);
		BYTES(rauth, sizeof(f->rauth));
		break;

	case Roattach9p1:
		SHORT(fid);
		LONG(qid.path);
		LONG(qid.version);
		break;

	case Rread9p1:
		SHORT(fid);
		SHORT(count);
		p++;
		f->data = (char*)p; p += f->count;
		break;

	case Rwrite9p1:
		SHORT(fid);
		SHORT(count);
		break;

	case Rstat9p1:
		SHORT(fid);
		BYTES(stat, sizeof(f->stat));
		break;
	}
	if((uint8_t*)ap+n == p)
		return n;
	return 0;
}

int
convM2D9p1(char *ap, Dentry *f)
{
	uint8_t *p;
	char str[28];

	p = (uint8_t*)ap;
	BYTES(name, sizeof(f->name));

	memmove(str, p, NAMELEN);
	p += NAMELEN;
	f->uid = strtouid(str);

	memmove(str, p, NAMELEN);
	p += NAMELEN;
	f->gid = strtouid(str);

	LONG(qid.path);
	LONG(qid.version);
	{
		LONG(atime);
		f->mode = (f->atime & 0x0fff) | DALLOC;
		if(f->atime & PDIR)
			f->mode |= DDIR;
		if(f->atime & PAPND)
			f->mode |= DAPND;
		if(f->atime & PLOCK)
			f->mode |= DLOCK;
	}
	LONG(atime);
	LONG(mtime);
	LONG(size); p += 4;
	p += 4;
	return p - (uint8_t*)ap;
}
