#include <u.h>
#include <libc.h>
#include <auth.h>
#include	"9p1.h"
#pragma	varargck	type	"D"	Dir*	/* from fcall.h */

static void dumpsome(char*, char*, long);

int
fcallfmt9p1(Fmt *f1)
{
	Fcall9p1 *f;
	int fid, type, tag, n;
	char buf[512];
	Dir d;

	f = va_arg(f1->args, Fcall9p1*);
	type = f->type;
	fid = f->fid;
	tag = f->tag;
	switch(type){
	case Tnop9p1:	/* 50 */
		sprint(buf, "old Tnop tag %ud", tag);
		break;
	case Rnop9p1:
		sprint(buf, "old Rnop tag %ud", tag);
		break;
	case Tsession9p1:	/* 52 */
		sprint(buf, "old Tsession tag %ud", tag);
		break;
	case Rsession9p1:
		sprint(buf, "old Rsession tag %ud", tag);
		break;
	case Rerror9p1:	/* 55 */
		sprint(buf, "old Rerror tag %ud error %.64s", tag, f->ename);
		break;
	case Tflush9p1:	/* 56 */
		sprint(buf, "old Tflush tag %ud oldtag %d", tag, f->oldtag);
		break;
	case Rflush9p1:
		sprint(buf, "old Rflush tag %ud", tag);
		break;
	case Tattach9p1:	/* 58 */
		sprint(buf, "old Tattach tag %ud fid %d uname %.28s aname %.28s auth %.28s",
			tag, f->fid, f->uname, f->aname, f->auth);
		break;
	case Rattach9p1:
		sprint(buf, "old Rattach tag %ud fid %d qid 0x%lux|0x%lux",
			tag, fid, f->qid.path, f->qid.version);
		break;
	case Tclone9p1:	/* 60 */
		sprint(buf, "old Tclone tag %ud fid %d newfid %d", tag, fid, f->newfid);
		break;
	case Rclone9p1:
		sprint(buf, "old Rclone tag %ud fid %d", tag, fid);
		break;
	case Twalk9p1:	/* 62 */
		sprint(buf, "old Twalk tag %ud fid %d name %.28s", tag, fid, f->name);
		break;
	case Rwalk9p1:
		sprint(buf, "old Rwalk tag %ud fid %d qid 0x%lux|0x%lux",
			tag, fid, f->qid.path, f->qid.version);
		break;
	case Topen9p1:	/* 64 */
		sprint(buf, "old Topen tag %ud fid %d mode %d", tag, fid, f->mode);
		break;
	case Ropen9p1:
		sprint(buf, "old Ropen tag %ud fid %d qid 0x%lux|0x%lux",
			tag, fid, f->qid.path, f->qid.version);
		break;
	case Tcreate9p1:	/* 66 */
		sprint(buf, "old Tcreate tag %ud fid %d name %.28s perm 0x%lux mode %d",
			tag, fid, f->name, f->perm, f->mode);
		break;
	case Rcreate9p1:
		sprint(buf, "old Rcreate tag %ud fid %d qid 0x%lux|0x%lux",
			tag, fid, f->qid.path, f->qid.version);
		break;
	case Tread9p1:	/* 68 */
		sprint(buf, "old Tread tag %ud fid %d offset %ld count %ld",
			tag, fid, f->offset, f->count);
		break;
	case Rread9p1:
		n = sprint(buf, "old Rread tag %ud fid %d count %ld ", tag, fid, f->count);
			dumpsome(buf+n, f->data, f->count);
		break;
	case Twrite9p1:	/* 70 */
		n = sprint(buf, "old Twrite tag %ud fid %d offset %ld count %ld ",
			tag, fid, f->offset, f->count);
		dumpsome(buf+n, f->data, f->count);
		break;
	case Rwrite9p1:
		sprint(buf, "old Rwrite tag %ud fid %d count %ld", tag, fid, f->count);
		break;
	case Tclunk9p1:	/* 72 */
		sprint(buf, "old Tclunk tag %ud fid %d", tag, fid);
		break;
	case Rclunk9p1:
		sprint(buf, "old Rclunk tag %ud fid %d", tag, fid);
		break;
	case Tremove9p1:	/* 74 */
		sprint(buf, "old Tremove tag %ud fid %d", tag, fid);
		break;
	case Rremove9p1:
		sprint(buf, "old Rremove tag %ud fid %d", tag, fid);
		break;
	case Tstat9p1:	/* 76 */
		sprint(buf, "old Tstat tag %ud fid %d", tag, fid);
		break;
	case Rstat9p1:
		convM2D9p1(f->stat, &d);
		sprint(buf, "old Rstat tag %ud fid %d stat %D", tag, fid, &d);
		break;
	case Twstat9p1:	/* 78 */
		convM2D9p1(f->stat, &d);
		sprint(buf, "old Twstat tag %ud fid %d stat %D", tag, fid, &d);
		break;
	case Rwstat9p1:
		sprint(buf, "old Rwstat tag %ud fid %d", tag, fid);
		break;
	case Tclwalk9p1:	/* 81 */
		sprint(buf, "old Tclwalk tag %ud fid %d newfid %d name %.28s",
			tag, fid, f->newfid, f->name);
		break;
	case Rclwalk9p1:
		sprint(buf, "old Rclwalk tag %ud fid %d qid 0x%lux|0x%lux",
			tag, fid, f->qid.path, f->qid.version);
		break;
	default:
		sprint(buf,  "unknown type %d", type);
	}
	return fmtstrcpy(f1, buf);
}

/*
 * dump out count (or DUMPL, if count is bigger) bytes from
 * buf to ans, as a string if they are all printable,
 * else as a series of hex bytes
 */
#define DUMPL 64

static void
dumpsome(char *ans, char *buf, long count)
{
	int i, printable;
	char *p;

	printable = 1;
	if(count > DUMPL)
		count = DUMPL;
	for(i=0; i<count && printable; i++)
		if((buf[i]<32 && buf[i] !='\n' && buf[i] !='\t') || (uchar)buf[i]>127)
			printable = 0;
	p = ans;
	*p++ = '\'';
	if(printable){
		memmove(p, buf, count);
		p += count;
	}else{
		for(i=0; i<count; i++){
			if(i>0 && i%4==0)
				*p++ = ' ';
			sprint(p, "%2.2ux", buf[i]);
			p += 2;
		}
	}
	*p++ = '\'';
	*p = 0;
}

#define	CHAR(x)		*p++ = f->x
#define	SHORT(x)	{ ulong vvv = f->x; p[0] = vvv; p[1] = vvv>>8; p += 2; }
#define	VLONG(q)	p[0] = (q); p[1] = (q)>>8; p[2] = (q)>>16; p[3] = (q)>>24; p += 4
#define	LONG(x)		{ ulong vvv = f->x; VLONG(vvv); }
#define	BYTES(x,n)	memmove(p, f->x, n); p += n
#define	STRING(x,n)	strncpy((char*)p, f->x, n); p += n

int
convS2M9p1(Fcall9p1 *f, char *ap)
{
	uchar *p;
	int t;

	p = (uchar*)ap;
	CHAR(type);
	t = f->type;
	SHORT(tag);
	switch(t)
	{
	default:
		fprint(2, "convS2M9p1: bad type: %d\n", t);
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
		BYTES(ticket, NAMEREC);
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
		if((uchar*)p == (uchar*)f->data) {
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
		if((uchar*)p == (uchar*)f->data) {
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
	return p - (uchar*)ap;
}

int
convD2M9p1(Dir *f, char *ap)
{
	uchar *p;
	ulong q;

	p = (uchar*)ap;
	STRING(name, NAMEREC);
	STRING(uid, NAMEREC);
	STRING(gid, NAMEREC);

	q = f->qid.path & ~0x80000000;
	if(f->qid.type & QTDIR)
		q |= 0x80000000;
	VLONG(q);
	LONG(qid.vers);
	LONG(mode);
	LONG(atime);
	LONG(mtime);
	LONG(length); VLONG(0);
	VLONG(0);
	return p - (uchar*)ap;
}

int
convA2M9p1(Authenticator *f, char *ap, char *key)
{
	int n;
	uchar *p;

	p = (uchar*)ap;
	CHAR(num);
	STRING(chal, CHALLEN);
	LONG(id);
	n = p - (uchar*)ap;
	if(key)
		encrypt(key, ap, n);
	return n;
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
convM2S9p1(char *ap, Fcall9p1 *f, int n)
{
	uchar *p;
	int t;

	p = (uchar*)ap;
	CHAR(type);
	t = f->type;
	SHORT(tag);
	switch(t)
	{
	default:
		fprint(2, "convM2S9p1: bad type: %d\n", f->type);
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
		BYTES(ticket, NAMEREC);
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
	if((uchar*)ap+n == p)
		return n;
	return 0;
}

int
convM2D9p1(char *ap, Dir *f)
{
	uchar *p;

	p = (uchar*)ap;
	f->name = (char*)p;
	p += NAMEREC;
	f->uid = (char*)p;
	f->muid = (char*)p;
	p += NAMEREC;
	f->gid = (char*)p;
	p += NAMEREC;

	LONG(qid.path);
	f->qid.path &= ~0x80000000;

	LONG(qid.vers);
	LONG(mode);
	f->qid.type = f->mode >> 24;
	LONG(atime);
	LONG(mtime);
	LONG(length); p += 4;
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
	STRING(chal, CHALLEN);
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
	STRING(chal, CHALLEN);
	STRING(cuid, NAMEREC);
	f->cuid[NAMEREC-1] = 0;
	STRING(suid, NAMEREC);
	f->suid[NAMEREC-1] = 0;
	STRING(key, DESKEYLEN);
	USED(p);
};
