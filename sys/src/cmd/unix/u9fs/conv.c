#include "u.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "libc.h"
#include "9p.h"

extern	void	error(char*);

#define	CHAR(x)		*p++ = f->x
#define	SHORT(x)	p[0] = f->x; p[1] = f->x>>8; p += 2
#define	LONG(x)		p[0] = f->x; p[1] = f->x>>8; p[2] = f->x>>16; p[3] = f->x>>24; p += 4
#define	VLONG(x,y)	p[0] = f->x; p[1] = f->x>>8; p[2] = f->x>>16; p[3] = f->x>>24;\
				p[4] = f->y; p[5] = f->y>>8; p[6] = f->y>>16; p[7] = f->y>>24; p += 8
#define	STRING(x,n)	memmove(p, f->x, n); p += n

int
convD2M(Dir *f, char *ap)
{
	Uchar *p;

	p = (Uchar*)ap;
	STRING(name, sizeof(f->name));
	STRING(uid, sizeof(f->uid));
	STRING(gid, sizeof(f->gid));
	LONG(qid.path);
	LONG(qid.vers);
	LONG(mode);
	LONG(atime);
	LONG(mtime);
	VLONG(len.length, len.hlength);
	SHORT(type);
	SHORT(dev);
	return p - (Uchar*)ap;
}

#undef	CHAR
#undef	SHORT
#undef	LONG
#undef	VLONG
#undef	STRING
#define	CHAR(x)		f->x = *p++
#define	SHORT(x)	f->x = (p[0] | (p[1]<<8)); p += 2
#define	LONG(x)		f->x = (p[0] | (p[1]<<8) | (p[2]<<16) | (p[3]<<24)); p += 4
#define	VLONG(x,y)	f->x = (p[0] | (p[1]<<8) | (p[2]<<16) | (p[3]<<24)); \
				f->y = (p[4] | (p[5]<<8) | (p[6]<<16) | (p[7]<<24)); p += 8
#define	STRING(x,n)	memmove(f->x, p, n); p += n

int
convM2D(char *ap, Dir *f)
{
	Uchar *p;

	p = (Uchar*)ap;
	STRING(name, sizeof(f->name));
	f->name[sizeof(f->name)-1] = '\0';
	STRING(uid, sizeof(f->uid));
	f->uid[sizeof(f->uid)-1] = '\0';
	STRING(gid, sizeof(f->gid));
	f->gid[sizeof(f->gid)-1] = '\0';
	LONG(qid.path);
	LONG(qid.vers);
	LONG(mode);
	LONG(atime);
	LONG(mtime);
	VLONG(len.length, len.hlength);
	SHORT(type);
	SHORT(dev);
	return p - (Uchar*)ap;
}

#undef	CHAR
#undef	SHORT
#undef	LONG
#undef	VLONG
#undef	STRING
#define	CHAR(x)		f->x = *p++
#define	SHORT(x)	f->x = (p[0] | (p[1]<<8)); p += 2
#define	LONG(x)		f->x = (p[0] | (p[1]<<8) |\
				(p[2]<<16) | (p[3]<<24)); p += 4
#define	VLONG(x)	f->x = (p[0] | (p[1]<<8) |\
				(p[2]<<16) | (p[3]<<24)); p += 8
#define	STRING(x,n)	memmove(f->x, p, n); p += n

int
convM2S(char *ap, Fcall *f, int n)
{
	Uchar *p;

	p = (Uchar*)ap;
	CHAR(type);
	SHORT(tag);
	switch(f->type)
	{
	default:
		error("bad convM2S type");

	case Tnop:
		break;

	case Tsession:
		STRING(chal, sizeof(f->chal));
		break;

	case Tflush:
		SHORT(oldtag);
		break;

	case Tattach:
		SHORT(fid);
		STRING(uname, sizeof(f->uname));
		f->uname[sizeof(f->uname)-1] = '\0';
		STRING(aname, sizeof(f->aname));
		f->aname[sizeof(f->aname)-1] = '\0';
		STRING(ticket, sizeof(f->ticket));
		STRING(auth, sizeof(f->auth));
		break;

	case Tclone:
		SHORT(fid);
		SHORT(newfid);
		break;

	case Twalk:
		SHORT(fid);
		STRING(name, sizeof(f->name));
		f->name[sizeof(f->name)-1] = '\0';
		break;

	case Topen:
		SHORT(fid);
		CHAR(mode);
		break;

	case Tcreate:
		SHORT(fid);
		STRING(name, sizeof(f->name));
		f->name[sizeof(f->name)-1] = '\0';
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
		p++;	/* pad(1) */
		f->data = (char*)p; p += f->count;
		break;

	case Tclunk:
		SHORT(fid);
		break;

	case Tremove:
		SHORT(fid);
		break;

	case Tstat:
		SHORT(fid);
		break;

	case Twstat:
		SHORT(fid);
		STRING(stat, sizeof(f->stat));
		break;

	case Tclwalk:
		SHORT(fid);
		SHORT(newfid);
		STRING(name, sizeof(f->name));
		break;
/*
 */
	case Rnop:
		break;

	case Rsession:
		STRING(chal, sizeof(f->chal));
		STRING(authid, sizeof(f->authid));
		STRING(authdom, sizeof(f->authdom));
		break;

	case Rerror:
		STRING(ename, sizeof(f->ename));
		break;

	case Rflush:
		break;

	case Rattach:
		SHORT(fid);
		LONG(qid.path);
		LONG(qid.vers);
		STRING(rauth, sizeof(f->rauth));
		break;

	case Rclone:
		SHORT(fid);
		break;

	case Rwalk:
	case Rclwalk:
		SHORT(fid);
		LONG(qid.path);
		LONG(qid.vers);
		break;

	case Ropen:
		SHORT(fid);
		LONG(qid.path);
		LONG(qid.vers);
		break;

	case Rcreate:
		SHORT(fid);
		LONG(qid.path);
		LONG(qid.vers);
		break;

	case Rread:
		SHORT(fid);
		SHORT(count);
		p++;	/* pad(1) */
		f->data = (char*)p; p += f->count;
		break;

	case Rwrite:
		SHORT(fid);
		SHORT(count);
		break;

	case Rclunk:
		SHORT(fid);
		break;

	case Rremove:
		SHORT(fid);
		break;

	case Rstat:
		SHORT(fid);
		STRING(stat, sizeof(f->stat));
		break;

	case Rwstat:
		SHORT(fid);
		break;
	}
	return (ap+n) - (char*)p;
}

#undef	CHAR
#undef	SHORT
#undef	LONG
#undef	VLONG
#undef	STRING
#define	CHAR(x)		*p++ = f->x
#define	SHORT(x)	p[0] = f->x; p[1] = f->x>>8; p += 2
#define	LONG(x)		p[0] = f->x; p[1] = f->x>>8; p[2] = f->x>>16; p[3] = f->x>>24; p += 4
#define	VLONG(x)	p[0] = f->x; p[1] = f->x>>8; p[2] = f->x>>16; p[3] = f->x>>24;\
				p[4] = 0; p[5] = 0; p[6] = 0; p[7] = 0; p += 8
#define	STRING(x,n)	memmove(p, f->x, n); p += n

int
convS2M(Fcall *f, char *ap)
{
	Uchar *p;

	p = (Uchar*)ap;
	CHAR(type);
	SHORT(tag);
	switch(f->type)
	{
	default:
		return 0;

	case Tnop:
		break;

	case Tsession:
		STRING(chal, sizeof(f->chal));
		break;

	case Tflush:
		SHORT(oldtag);
		break;

	case Tattach:
		SHORT(fid);
		STRING(uname, sizeof(f->uname));
		STRING(aname, sizeof(f->aname));
		STRING(ticket, sizeof(f->ticket));
		STRING(auth, sizeof(f->auth));
		break;

	case Tclone:
		SHORT(fid);
		SHORT(newfid);
		break;

	case Twalk:
		SHORT(fid);
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
		p++;	/* pad(1) */
		STRING(data, f->count);
		break;

	case Tclunk:
		SHORT(fid);
		break;

	case Tremove:
		SHORT(fid);
		break;

	case Tstat:
		SHORT(fid);
		break;

	case Twstat:
		SHORT(fid);
		STRING(stat, sizeof(f->stat));
		break;

	case Tclwalk:
		SHORT(fid);
		SHORT(newfid);
		STRING(name, sizeof(f->name));
		break;
/*
 */
	case Rnop:
		break;

	case Rsession:
		STRING(chal, sizeof(f->chal));
		STRING(authid, sizeof(f->authid));
		STRING(authdom, sizeof(f->authdom));
		break;

	case Rerror:
		STRING(ename, sizeof(f->ename));
		break;

	case Rflush:
		break;

	case Rattach:
		SHORT(fid);
		LONG(qid.path);
		LONG(qid.vers);
		STRING(rauth, sizeof(f->rauth));
		break;

	case Rclone:
		SHORT(fid);
		break;

	case Rwalk:
	case Rclwalk:
		SHORT(fid);
		LONG(qid.path);
		LONG(qid.vers);
		break;

	case Ropen:
		SHORT(fid);
		LONG(qid.path);
		LONG(qid.vers);
		break;

	case Rcreate:
		SHORT(fid);
		LONG(qid.path);
		LONG(qid.vers);
		break;

	case Rread:
		SHORT(fid);
		SHORT(count);
		p++;	/* pad(1) */
		STRING(data, f->count);
		break;

	case Rwrite:
		SHORT(fid);
		SHORT(count);
		break;

	case Rclunk:
		SHORT(fid);
		break;

	case Rremove:
		SHORT(fid);
		break;

	case Rstat:
		SHORT(fid);
		STRING(stat, sizeof(f->stat));
		break;

	case Rwstat:
		SHORT(fid);
		break;
	}
	return p - (Uchar*)ap;
}
