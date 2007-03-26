#include "all.h"

/* BUG transition */
// int client9p = 2;
// int kernel9p = 2;

#include "9p1.h"

#define	CHAR(x)		*p++ = f->x
#define	SHORT(x)	{ ulong vvv = f->x; *p++ = vvv; *p++ = vvv>>8; }
#define	LONGINT(q) {*p++ = (q); *p++ = (q)>>8; *p++ = (q)>>16; *p++ = (q)>>24;}
#define	LONG(x)		{ ulong vvv = f->x; LONGINT(vvv); }
#define	VLONG(x) { \
	uvlong q = f->x; \
	*p++ = (q)>> 0; *p++ = (q)>> 8; *p++ = (q)>>16; *p++ = (q)>>24; \
	*p++ = (q)>>32; *p++ = (q)>>40; *p++ = (q)>>48; *p++ = (q)>>56; \
	}

#define	BYTES(x,n)	memmove(p, f->x, n); p += n
#define	STRING(x,n)	strncpy((char*)p, f->x, n); p += n

int
convS2M9p1(Fcall *f, uchar *ap)
{
	uchar *p;
	int t;

	p = ap;
	CHAR(type);
	t = f->type;
	SHORT(tag);
	switch(t) {
	default:
		print("convS2M9p1: bad type: %d\n", t);
		return 0;

	case Tnop:
	case Tosession:
		break;

	case Tsession:
		BYTES(chal, sizeof(f->chal));
		break;

	case Tflush:
		SHORT(oldtag);
		break;

	case Tattach:
		SHORT(fid);
		STRING(uname, sizeof(f->uname));
		STRING(aname, sizeof(f->aname));
		BYTES(ticket, sizeof(f->ticket));
		BYTES(auth, sizeof(f->auth));
		break;

	case Toattach:
		SHORT(fid);
		STRING(uname, sizeof(f->uname));
		STRING(aname, sizeof(f->aname));
		BYTES(ticket, NAMELEN);
		break;

	case Tclone:
		SHORT(fid);
		SHORT(newfid);
		break;

	case Twalk:
		SHORT(fid);
		STRING(name, sizeof(f->name));
		break;

	case Tclwalk:
		SHORT(fid);
		SHORT(newfid);
		STRING(name, sizeof(f->name));
		break;

	case Topen:
		SHORT(fid);
		CHAR(mode);
		break;

	case Tcreate:
		SHORT(fid);
		STRING(name, sizeof(f->name));
		LONG(perm);
		CHAR(mode);
		break;

	case Tread:
		SHORT(fid);
		VLONG(offset);
		SHORT(count);
		break;

	case Twrite:
		SHORT(fid);
		VLONG(offset);
		SHORT(count);
		p++;
		if((uchar*)p == (uchar*)f->data) {
			p += f->count;
			break;
		}
		BYTES(data, f->count);
		break;

	case Tclunk:
	case Tremove:
	case Tstat:
		SHORT(fid);
		break;

	case Twstat:
		SHORT(fid);
		BYTES(stat, sizeof(f->stat));
		break;
/*
 */
	case Rnop:
	case Rosession:
	case Rflush:
		break;

	case Rsession:
		BYTES(chal, sizeof(f->chal));
		BYTES(authid, sizeof(f->authid));
		BYTES(authdom, sizeof(f->authdom));
		break;

	case Rerror:
		STRING(ename, sizeof(f->ename));
		break;

	case Rclone:
	case Rclunk:
	case Rremove:
	case Rwstat:
		SHORT(fid);
		break;

	case Rwalk:
	case Ropen:
	case Rcreate:
	case Rclwalk:
		SHORT(fid);
		LONG(qid.path);
		LONG(qid.version);
		break;

	case Rattach:
		SHORT(fid);
		LONG(qid.path);
		LONG(qid.version);
		BYTES(rauth, sizeof(f->rauth));
		break;

	case Roattach:
		SHORT(fid);
		LONG(qid.path);
		LONG(qid.version);
		break;

	case Rread:
		SHORT(fid);
		SHORT(count);
		p++;
		if((uchar*)p == (uchar*)f->data) {
			p += f->count;
			break;
		}
		BYTES(data, f->count);
		break;

	case Rwrite:
		SHORT(fid);
		SHORT(count);
		break;

	case Rstat:
		SHORT(fid);
		BYTES(stat, sizeof(f->stat));
		break;
	}
	return p - (uchar*)ap;
}

/*
 * buggery to give false qid for
 * the top 2 levels of the dump fs
 */
static ulong
fakeqid9p1(Dentry *f)
{
	ulong q;
	int c;

	q = f->qid.path;
	if(q == (QPROOT|QPDIR)) {
		c = f->name[0];
		if(isascii(c) && isdigit(c)) {
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
	uchar *p;
	ulong q;

	p = (uchar*)ap;
	STRING(name, sizeof(f->name));

	memset(p, 0, 2*NAMELEN);
	uidtostr((char*)p, f->uid, 1);
	p += NAMELEN;

	uidtostr((char*)p, f->gid, 1);
	p += NAMELEN;

	q = fakeqid9p1(f);
	LONGINT(q);
	LONG(qid.version);

	q = f->mode & 0x0fff;
	if(f->mode & DDIR)
		q |= PDIR;
	if(f->mode & DAPND)
		q |= PAPND;
	if(f->mode & DLOCK)
		q |= PLOCK;
	LONGINT(q);

	LONG(atime);
	LONG(mtime);
	VLONG(size);
	LONGINT(0);
	return p - (uchar*)ap;
}

int
convA2M9p1(Authenticator *f, char *ap, char *key)
{
	int n;
	uchar *p;

	p = (uchar*)ap;
	CHAR(num);
	BYTES(chal, CHALLEN);
	LONG(id);
	n = p - (uchar*)ap;
	if(key)
		encrypt(key, ap, n);
	return n;
}

#undef	CHAR
#undef	SHORT
#undef	LONG
#undef	LONGINT
#undef	VLONG
#undef	BYTES
#undef	STRING

#define	CHAR(x)		f->x = *p++
#define	SHORT(x)	f->x = (p[0] | (p[1]<<8)); p += 2
#define	LONG(x)	f->x = p[0] | (p[1]<<8) | (p[2]<<16) | (p[3]<<24); p += 4
#define	VLONG(x) { \
	f->x =	    (p[0] | (p[1]<<8) | (p[2]<<16) | (p[3]<<24)) | \
	    (uvlong)(p[4] | (p[5]<<8) | (p[6]<<16) | (p[7]<<24)) << 32; \
	p += 8; \
}

#define	BYTES(x,n)	memmove(f->x, p, n); p += n
#define	STRING(x,n)	memmove(f->x, p, n); p += n

int
convM2S9p1(uchar *ap, Fcall *f, int n)
{
	uchar *p;
	int t;

	p = ap;
	CHAR(type);
	t = f->type;
	SHORT(tag);
	switch(t) {
	default:
		/*
		 * only whine if it couldn't be a 9P2000 Tversion.
		 */
		if(t != 19 || ap[4] != 100)
			print("convM2S9p1: bad type: %d\n", f->type);
		return 0;

	case Tnop:
	case Tosession:
		break;

	case Tsession:
		BYTES(chal, sizeof(f->chal));
		break;

	case Tflush:
		SHORT(oldtag);
		break;

	case Tattach:
		SHORT(fid);
		BYTES(uname, sizeof(f->uname));
		BYTES(aname, sizeof(f->aname));
		BYTES(ticket, sizeof(f->ticket));
		BYTES(auth, sizeof(f->auth));
		break;

	case Toattach:
		SHORT(fid);
		BYTES(uname, sizeof(f->uname));
		BYTES(aname, sizeof(f->aname));
		BYTES(ticket, NAMELEN);
		break;

	case Tclone:
		SHORT(fid);
		SHORT(newfid);
		break;

	case Twalk:
		SHORT(fid);
		BYTES(name, sizeof(f->name));
		break;

	case Tclwalk:
		SHORT(fid);
		SHORT(newfid);
		BYTES(name, sizeof(f->name));
		break;

	case Tremove:
		SHORT(fid);
		break;

	case Topen:
		SHORT(fid);
		CHAR(mode);
		break;

	case Tcreate:
		SHORT(fid);
		BYTES(name, sizeof(f->name));
		LONG(perm);
		CHAR(mode);
		break;

	case Tread:
		SHORT(fid);
		VLONG(offset);
		SHORT(count);
		break;

	case Twrite:
		SHORT(fid);
		VLONG(offset);
		SHORT(count);
		p++;
		f->data = (char*)p; p += f->count;
		break;

	case Tclunk:
	case Tstat:
		SHORT(fid);
		break;

	case Twstat:
		SHORT(fid);
		BYTES(stat, sizeof(f->stat));
		break;

/*
 */
	case Rnop:
	case Rosession:
		break;

	case Rsession:
		BYTES(chal, sizeof(f->chal));
		BYTES(authid, sizeof(f->authid));
		BYTES(authdom, sizeof(f->authdom));
		break;

	case Rerror:
		BYTES(ename, sizeof(f->ename));
		break;

	case Rflush:
		break;

	case Rclone:
	case Rclunk:
	case Rremove:
	case Rwstat:
		SHORT(fid);
		break;

	case Rwalk:
	case Rclwalk:
	case Ropen:
	case Rcreate:
		SHORT(fid);
		LONG(qid.path);
		LONG(qid.version);
		break;

	case Rattach:
		SHORT(fid);
		LONG(qid.path);
		LONG(qid.version);
		BYTES(rauth, sizeof(f->rauth));
		break;

	case Roattach:
		SHORT(fid);
		LONG(qid.path);
		LONG(qid.version);
		break;

	case Rread:
		SHORT(fid);
		SHORT(count);
		p++;
		f->data = (char*)p; p += f->count;
		break;

	case Rwrite:
		SHORT(fid);
		SHORT(count);
		break;

	case Rstat:
		SHORT(fid);
		BYTES(stat, sizeof(f->stat));
		break;
	}
	if((uchar*)ap+n == p)
		return n;
	return 0;
}

int
convM2D9p1(char *ap, Dentry *f)
{
	uchar *p;
	char str[NAMELEN];

	p = (uchar*)ap;
	BYTES(name, sizeof(f->name));

	memmove(str, p, NAMELEN);
	p += NAMELEN;
	f->uid = strtouid(str);

	memmove(str, p, NAMELEN);
	p += NAMELEN;
	f->gid = strtouid(str);

	LONG(qid.path);
	LONG(qid.version);

	LONG(atime);
	f->mode = (f->atime & 0x0fff) | DALLOC;
	if(f->atime & PDIR)
		f->mode |= DDIR;
	if(f->atime & PAPND)
		f->mode |= DAPND;
	if(f->atime & PLOCK)
		f->mode |= DLOCK;

	LONG(atime);
	LONG(mtime);
	VLONG(size);
	p += 4;
	return p - (uchar*)ap;
}

void
convM2A9p1(char *ap, Authenticator *f, char *key)
{
	uchar *p;

	if(key)
		decrypt(key, ap, AUTHENTLEN);
	p = (uchar*)ap;
	CHAR(num);
	BYTES(chal, CHALLEN);
	LONG(id);
	USED(p);
}

void
convM2T9p1(char *ap, Ticket *f, char *key)
{
	uchar *p;

	if(key)
		decrypt(key, ap, TICKETLEN);
	p = (uchar*)ap;
	CHAR(num);
	BYTES(chal, CHALLEN);
	STRING(cuid, NAMELEN);
	f->cuid[NAMELEN-1] = 0;
	STRING(suid, NAMELEN);
	f->suid[NAMELEN-1] = 0;
	BYTES(key, DESKEYLEN);
	USED(p);
}
