#include	"lib9.h"
#include	"sys.h"
#include	"error.h"

#define	PCHAR(x)	*p++ = f->x
#define	PSHORT(x)	p[0] = f->x;\
			p[1] = f->x>>8;\
			p += 2
#define	PLONG(x)	p[0] = f->x;\
			p[1] = f->x>>8;\
			p[2] = f->x>>16;\
			p[3] = f->x>>24;\
			p += 4
#define	PVLONG(x)	p[0] = f->x;\
			p[1] = f->x>>8;\
			p[2] = f->x>>16;\
			p[3] = f->x>>24;\
			p[4] = 0;\
			p[5] = 0;\
			p[6] = 0;\
			p[7] = 0;\
			p += 8
#define	PSTRING(x,n)	memmove(p, f->x, n); p += n

#define	GCHAR(x)	f->x =	*p++
#define	GSHORT(x)	f->x =	(p[0]|(p[1]<<8));\
			p += 2
#define	GLONG(x)	f->x =	(p[0]|\
			      	(p[1]<<8)|\
			      	(p[2]<<16)|\
			      	(p[3]<<24));\
			      	p += 4
#define	GVLONG(x)	f->x =	(p[0]|\
				(p[1]<<8)|\
				(p[2]<<16)|\
				(p[3]<<24));\
			p += 8
#define	GSTRING(x,n)	memmove(f->x, p, n); p += n

static uchar msglen[Rattach+1] =
{
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	/*Tnop*/		3,
	/*Rnop*/		3,
	0,0,
	/*Terror*/	0,
	/*Rerror*/	67,
	/*Tflush*/	5,
	/*Rflush*/	3,
	0,0,
	/*Tclone*/	7,
	/*Rclone*/	5,
	/*Twalk*/		33,
	/*Rwalk*/		13,
	/*Topen*/	6,
	/*Ropen*/	13,
	/*Tcreate*/	38,
	/*Rcreate*/	13,
	/*Tread*/		15,
	/*Rread*/		8,		/* header only; excludes data */
	/*Twrite*/	16,		/* header only; excludes data */
	/*Rwrite*/	7,
	/*Tclunk*/	5,
	/*Rclunk*/	5,
	/*Tremove*/	5,
	/*Rremove*/	5,
	/*Tstat*/		5,
	/*Rstat*/		121,
	/*Twstat*/	121,
	/*Rwstat*/	5,
	/*Tclwalk*/	35,
	/*Rclwalk*/	13,
	0,0,
	/*Tsession*/	3+CHALLEN,
	/*Rsession*/	3+NAMELEN+DOMLEN+CHALLEN,
	/*Tattach*/	5+2*NAMELEN+TICKETLEN+AUTHENTLEN,
	/*Rattach*/	13+AUTHENTLEN,
};

int
convD2M(Dir *f, char *ap)
{
	uchar *p;

	p = (uchar*)ap;
	PSTRING(name, sizeof(f->name));
	PSTRING(uid, sizeof(f->uid));
	PSTRING(gid, sizeof(f->gid));
	PLONG(qid.path);
	PLONG(qid.vers);
	PLONG(mode);
	PLONG(atime);
	PLONG(mtime);
	PVLONG(length);
	PSHORT(type);
	PSHORT(dev);
	return p - (uchar*)ap;
}

int
convM2D(char *ap, Dir *f)
{
	uchar *p;

	p = (uchar*)ap;
	GSTRING(name, sizeof(f->name));
	GSTRING(uid, sizeof(f->uid));
	GSTRING(gid, sizeof(f->gid));
	GLONG(qid.path);
	GLONG(qid.vers);
	GLONG(mode);
	GLONG(atime);
	GLONG(mtime);
	GVLONG(length);
	GSHORT(type);
	GSHORT(dev);
	return p - (uchar*)ap;
}

int
convM2S(char *ap, Fcall *f, int n)
{
	uchar *p;

	p = (uchar*)ap;
	if(n < 3)
		return 0;

	GCHAR(type);
	GSHORT(tag);

	if(n < msglen[f->type])
		return 0;

	switch(f->type) {
	default:
		return -1;

	case Tnop:
		break;
	case Tsession:
		GSTRING(chal, sizeof(f->chal));
		break;
	case Tflush:
		GSHORT(oldtag);
		break;
	case Tattach:
		GSHORT(fid);
		GSTRING(uname, sizeof(f->uname));
		GSTRING(aname, sizeof(f->aname));
		GSTRING(ticket, sizeof(f->ticket));
		GSTRING(auth, sizeof(f->auth));
		break;
	case Tclone:
		GSHORT(fid);
		GSHORT(newfid);
		break;
	case Twalk:
		GSHORT(fid);
		GSTRING(name, sizeof(f->name));
		break;
	case Topen:
		GSHORT(fid);
		GCHAR(mode);
		break;
	case Tcreate:
		GSHORT(fid);
		GSTRING(name, sizeof(f->name));
		GLONG(perm);
		GCHAR(mode);
		break;
	case Tread:
		GSHORT(fid);
		GVLONG(offset);
		GSHORT(count);
		break;
	case Twrite:
		GSHORT(fid);
		GVLONG(offset);
		GSHORT(count);
		if(n < msglen[f->type]+f->count)
			return 0;
		p++;	/* pad(1) */
		f->data = (char*)p; p += f->count;
		break;
	case Tclunk:
		GSHORT(fid);
		break;
	case Tremove:
		GSHORT(fid);
		break;
	case Tstat:
		GSHORT(fid);
		break;
	case Twstat:
		GSHORT(fid);
		GSTRING(stat, sizeof(f->stat));
		break;

	case Rnop:
		break;
	case Rsession:
		GSTRING(chal, sizeof(f->chal));
		GSTRING(authid, sizeof(f->authid));
		GSTRING(authdom, sizeof(f->authdom));
		break;
	case Rerror:
		GSTRING(ename, sizeof(f->ename));
		break;
	case Rflush:
		break;
	case Rattach:
		GSHORT(fid);
		GLONG(qid.path);
		GLONG(qid.vers);
		GSTRING(rauth, sizeof(f->rauth));
		break;
	case Rclone:
		GSHORT(fid);
		break;
	case Rwalk:
		GSHORT(fid);
		GLONG(qid.path);
		GLONG(qid.vers);
		break;
	case Ropen:
		GSHORT(fid);
		GLONG(qid.path);
		GLONG(qid.vers);
		break;
	case Rcreate:
		GSHORT(fid);
		GLONG(qid.path);
		GLONG(qid.vers);
		break;
	case Rread:
		GSHORT(fid);
		GSHORT(count);
		if(n < msglen[f->type]+f->count)
			return 0;
		p++;			/* pad(1) */
		f->data = (char*)p;
		p += f->count;
		break;
	case Rwrite:
		GSHORT(fid);
		GSHORT(count);
		break;
	case Rclunk:
		GSHORT(fid);
		break;
	case Rremove:
		GSHORT(fid);
		break;
	case Rstat:
		GSHORT(fid);
		GSTRING(stat, sizeof(f->stat));
		break;
	case Rwstat:
		GSHORT(fid);
		break;
	}
	return (char*)p - ap;
}

int
convS2M(Fcall *f, char *ap)
{
	uchar *p;

	p = (uchar*)ap;
	PCHAR(type);
	PSHORT(tag);
	switch(f->type)
	{
	default:
		return 0;

	case Tnop:
		break;
	case Tsession:
		PSTRING(chal, sizeof(f->chal));
		break;
	case Tflush:
		PSHORT(oldtag);
		break;
	case Tattach:
		PSHORT(fid);
		PSTRING(uname, sizeof(f->uname));
		PSTRING(aname, sizeof(f->aname));
		PSTRING(ticket, sizeof(f->ticket));
		PSTRING(auth, sizeof(f->auth));
		break;
	case Tclone:
		PSHORT(fid);
		PSHORT(newfid);
		break;
	case Twalk:
		PSHORT(fid);
		PSTRING(name, sizeof(f->name));
		break;
	case Topen:
		PSHORT(fid);
		PCHAR(mode);
		break;
	case Tcreate:
		PSHORT(fid);
		PSTRING(name, sizeof(f->name));
		PLONG(perm);
		PCHAR(mode);
		break;
	case Tread:
		PSHORT(fid);
		PVLONG(offset);
		PSHORT(count);
		break;
	case Twrite:
		PSHORT(fid);
		PVLONG(offset);
		PSHORT(count);
		p++;	/* pad(1) */
		PSTRING(data, f->count);
		break;
	case Tclunk:
		PSHORT(fid);
		break;
	case Tremove:
		PSHORT(fid);
		break;
	case Tstat:
		PSHORT(fid);
		break;
	case Twstat:
		PSHORT(fid);
		PSTRING(stat, sizeof(f->stat));
		break;

	case Rnop:
		break;
	case Rsession:
		PSTRING(chal, sizeof(f->chal));
		PSTRING(authid, sizeof(f->authid));
		PSTRING(authdom, sizeof(f->authdom));
		break;
	case Rerror:
		PSTRING(ename, sizeof(f->ename));
		break;
	case Rflush:
		break;
	case Rattach:
		PSHORT(fid);
		PLONG(qid.path);
		PLONG(qid.vers);
		PSTRING(rauth, sizeof(f->rauth));
		break;
	case Rclone:
		PSHORT(fid);
		break;
	case Rwalk:
		PSHORT(fid);
		PLONG(qid.path);
		PLONG(qid.vers);
		break;
	case Ropen:
		PSHORT(fid);
		PLONG(qid.path);
		PLONG(qid.vers);
		break;
	case Rcreate:
		PSHORT(fid);
		PLONG(qid.path);
		PLONG(qid.vers);
		break;
	case Rread:
		PSHORT(fid);
		PSHORT(count);
		p++;	/* pad(1) */
		PSTRING(data, f->count);
		break;
	case Rwrite:
		PSHORT(fid);
		PSHORT(count);
		break;
	case Rclunk:
		PSHORT(fid);
		break;
	case Rremove:
		PSHORT(fid);
		break;
	case Rstat:
		PSHORT(fid);
		PSTRING(stat, sizeof(f->stat));
		break;
	case Rwstat:
		PSHORT(fid);
		break;
	}
	return p - (uchar*)ap;
}

int
convTR2M(Ticketreq *f, char *ap)
{
	int n;
	uchar *p;

	p = (uchar*)ap;
	PCHAR(type);
	PSTRING(authid, NAMELEN);
	PSTRING(authdom, DOMLEN);
	PSTRING(chal, CHALLEN);
	PSTRING(hostid, NAMELEN);
	PSTRING(uid, NAMELEN);
	n = p - (uchar*)ap;
	return n;
}


void
convM2A(char *ap, Authenticator *f, char *key)
{
	uchar *p;

	if(key)
		decrypt(key, ap, AUTHENTLEN);
	p = (uchar*)ap;
	GCHAR(num);
	GSTRING(chal, CHALLEN);
	GLONG(id);
	USED(p);
}

int
convT2M(Ticket *f, char *ap, char *key)
{
	int n;
	uchar *p;

	p = (uchar*)ap;
	PCHAR(num);
	PSTRING(chal, CHALLEN);
	PSTRING(cuid, NAMELEN);
	PSTRING(suid, NAMELEN);
	PSTRING(key, DESKEYLEN);
	n = p - (uchar*)ap;
	if(key)
		encrypt(key, ap, n);
	return n;
}

int
convA2M(Authenticator *f, char *ap, char *key)
{
	int n;
	uchar *p;

	p = (uchar*)ap;
	PCHAR(num);
	PSTRING(chal, CHALLEN);
	PLONG(id);
	n = p - (uchar*)ap;
	if(key)
		encrypt(key, ap, n);
	return n;
}

void
convM2T(char *ap, Ticket *f, char *key)
{
	uchar *p;

	if(key)
		decrypt(key, ap, TICKETLEN);
	p = (uchar*)ap;
	GCHAR(num);
	GSTRING(chal, CHALLEN);
	GSTRING(cuid, NAMELEN);
	f->cuid[NAMELEN-1] = 0;
	GSTRING(suid, NAMELEN);
	f->suid[NAMELEN-1] = 0;
	GSTRING(key, DESKEYLEN);
	USED(p);
}


void
convM2TR(char *ap, Ticketreq *f)
{
	uchar *p;

	p = (uchar*)ap;
	GCHAR(type);
	GSTRING(authid, NAMELEN);
	f->authid[NAMELEN-1] = 0;
	GSTRING(authdom, DOMLEN);
	f->authdom[DOMLEN-1] = 0;
	GSTRING(chal, CHALLEN);
	GSTRING(hostid, NAMELEN);
	f->hostid[NAMELEN-1] = 0;
	GSTRING(uid, NAMELEN);
	f->uid[NAMELEN-1] = 0;
	USED(p);
}
