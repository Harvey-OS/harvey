#include <plan9.h>
#include <fcall.h>
#include <oldfcall.h>

/*
 * routines to package the old protocol in the new structures.
 */

#define	SHORT(x)	p[0]=f->x; p[1]=f->x>>8; p += 2
#define	LONG(x)		p[0]=f->x; p[1]=f->x>>8; p[2]=f->x>>16; p[3]=f->x>>24; p += 4
#define	VLONG(x)	p[0]=f->x;	p[1]=f->x>>8;\
			p[2]=f->x>>16;	p[3]=f->x>>24;\
			p[4]=f->x>>32;	p[5]=f->x>>40;\
			p[6]=f->x>>48;	p[7]=f->x>>56;\
			p += 8
#define	STRING(x,n)	strecpy((char*)p, (char*)p+n, f->x); p += n;
#define	FIXQID(q)		q.path ^= (q.path>>33); q.path &= 0x7FFFFFFF; q.path |= (q.type&0x80)<<24

uint
oldhdrsize(uchar type)
{
	switch(type){
	default:
		return 0;
	case oldTnop:
		return 3;
	case oldTflush:
		return 3+2;
	case oldTclone:
		return 3+2+2;
	case oldTwalk:
		return 3+2+28;
	case oldTopen:
		return 3+2+1;
	case oldTcreate:
		return 3+2+28+4+1;
	case oldTread:
		return 3+2+8+2;
	case oldTwrite:
		return 3+2+8+2+1;
	case oldTclunk:
		return 3+2;
	case oldTremove:
		return 3+2;
	case oldTstat:
		return 3+2;
	case oldTwstat:
		return 3+2+116;
	case oldTsession:
		return 3+8;
	case oldTattach:
		return 3+2+28+28+72+13;
	}
}

uint
iosize(uchar *p)
{
	if(p[0] != oldTwrite)
		return 0;
	return p[3+2+8] | (p[3+2+8+1]<<8);
}

uint
sizeS2M(Fcall *f)
{
	switch(f->type)
	{
	default:
		abort();
		return 0;

	/* no T messages */

/*
 */
	case Rversion:
		return 1+2;

/*
	case Rsession:
		return 1+2+8+28+48;
*/

	case Rattach:
		return 1+2+2+4+4+13;

	case Rerror:
		return 1+2+64;

	case Rflush:
		if(f->tag&0x8000)
			return 1+2+8+28+48;	/* session */
		return 1+2;

	/* assumes we don't ever see Tclwalk requests ... */
	case Rwalk:
		if(f->nwqid == 0)
			return 1+2+2;
		else
			return 1+2+2+4+4;

	case Ropen:
		return 1+2+2+4+4;

	case Rcreate:
		return 1+2+2+4+4;

	case Rread:
		return 1+2+2+2+1+f->count;

	case Rwrite:
		return 1+2+2+2;

	case Rclunk:
		return 1+2+2;

	case Rremove:
		return 1+2+2;

	case Rstat:
		return 1+2+2+116;

	case Rwstat:
		return 1+2+2;
	}
}

uint
convS2Mold(Fcall *f, uchar *ap, uint nap)
{
	uchar *p;

	if(nap < sizeS2M(f))
		return 0;

	p = ap;
	switch(f->type)
	{
	default:
		abort();
		return 0;

	/* no T messages */

/*
 */
	case Rversion:
		*p++ = oldRnop;
		SHORT(tag);
		break;

/*
	case Rsession:
		*p++ = oldRsession;
		SHORT(tag);

		if(f->nchal > 8)
			f->nchal = 8;
		memmove(p, f->chal, f->nchal);
		p += f->nchal;
		if(f->nchal < 8){
			memset(p, 0, 8 - f->nchal);
			p += 8 - f->nchal;
		}

		STRING(authid, 28);
		STRING(authdom, 48);
		break;
*/

	case Rattach:
		*p++ = oldRattach;
		SHORT(tag);
		SHORT(fid);
		FIXQID(f->qid);
		LONG(qid.path);
		LONG(qid.vers);
		memset(p, 0, 13);
		p += 13;
		break;

	case Rerror:
		*p++ = oldRerror;
		SHORT(tag);
		STRING(ename, 64);
		break;

	case Rflush:
		if(f->tag&0x8000){
			*p++ = oldRsession;
			f->tag &= ~0x8000;
			SHORT(tag);
			memset(p, 0, 8+28+48);
			p += 8+28+48;
		}else{
			*p++ = oldRflush;
			SHORT(tag);
		}
		break;

	/* assumes we don't ever see Tclwalk requests ... */
	case Rwalk:
		if(f->nwqid == 0){	/* successful clone */
			*p++ = oldRclone;
			SHORT(tag);
			SHORT(fid);
		}else{			/* successful 1-element walk */
			*p++ = oldRwalk;
			SHORT(tag);
			SHORT(fid);
			FIXQID(f->wqid[0]);
			LONG(wqid[0].path);
			LONG(wqid[0].vers);
		}
		break;

	case Ropen:
		*p++ = oldRopen;
		SHORT(tag);
		SHORT(fid);
		FIXQID(f->qid);
		LONG(qid.path);
		LONG(qid.vers);
		break;

	case Rcreate:
		*p++ = oldRcreate;
		SHORT(tag);
		SHORT(fid);
		FIXQID(f->qid);
		LONG(qid.path);
		LONG(qid.vers);
		break;

	case Rread:
		*p++ = oldRread;
		SHORT(tag);
		SHORT(fid);
		SHORT(count);
		p++;	/* pad(1) */
		memmove(p, f->data, f->count);
		p += f->count;
		break;

	case Rwrite:
		*p++ = oldRwrite;
		SHORT(tag);
		SHORT(fid);
		SHORT(count);
		break;

	case Rclunk:
		*p++ = oldRclunk;
		SHORT(tag);
		SHORT(fid);
		break;

	case Rremove:
		*p++ = oldRremove;
		SHORT(tag);
		SHORT(fid);
		break;

	case Rstat:
		*p++ = oldRstat;
		SHORT(tag);
		SHORT(fid);
		memmove(p, f->stat, 116);
		p += 116;
		break;

	case Rwstat:
		*p++ = oldRwstat;
		SHORT(tag);
		SHORT(fid);
		break;
	}
	return p - ap;
}

uint
sizeD2Mold(Dir *d)
{
	return 116;
}

uint
convD2Mold(Dir *f, uchar *ap, uint nap)
{
	uchar *p;

	if(nap < 116)
		return 0;

	p = ap;
	STRING(name, 28);
	STRING(uid, 28);
	STRING(gid, 28);
	FIXQID(f->qid);
	LONG(qid.path);
	LONG(qid.vers);
	LONG(mode);
	LONG(atime);
	LONG(mtime);
	VLONG(length);
	SHORT(type);
	SHORT(dev);

	return p - ap;
}

#undef SHORT
#undef LONG
#undef VLONG
#undef STRING
#define	CHAR(x)	f->x = *p++
#define	SHORT(x)	f->x = (p[0] | (p[1]<<8)); p += 2
#define	LONG(x)		f->x = (p[0] | (p[1]<<8) |\
				(p[2]<<16) | (p[3]<<24)); p += 4
#define	VLONG(x)	f->x = (ulong)(p[0] | (p[1]<<8) |\
					(p[2]<<16) | (p[3]<<24)) |\
				((vlong)(p[4] | (p[5]<<8) |\
					(p[6]<<16) | (p[7]<<24)) << 32); p += 8
#define	STRING(x,n)	f->x = (char*)p; p += n

uint
convM2Sold(uchar *ap, uint nap, Fcall *f)
{
	uchar *p, *q, *ep;

	p = ap;
	ep = p + nap;

	if(p+3 > ep)
		return 0;

	switch(*p++){
	case oldTnop:
		f->type = Tversion;
		SHORT(tag);
		f->msize = 0;
		f->version = "9P1";
		break;

	case oldTflush:
		f->type = Tflush;
		SHORT(tag);
		if(p+2 > ep)
			return 0;
		SHORT(oldtag);
		break;

	case oldTclone:
		f->type = Twalk;
		SHORT(tag);
		if(p+2+2 > ep)
			return 0;
		SHORT(fid);
		SHORT(newfid);
		f->nwname = 0;
		break;

	case oldTwalk:
		f->type = Twalk;
		SHORT(tag);
		if(p+2+28 > ep)
			return 0;
		SHORT(fid);
		f->newfid = f->fid;
		f->nwname = 1;
		f->wname[0] = (char*)p;
		p += 28;
		break;

	case oldTopen:
		f->type = Topen;
		SHORT(tag);
		if(p+2+1 > ep)
			return 0;
		SHORT(fid);
		CHAR(mode);
		break;

	case oldTcreate:
		f->type = Tcreate;
		SHORT(tag);
		if(p+2+28+4+1 > ep)
			return 0;
		SHORT(fid);
		f->name = (char*)p;
		p += 28;
		LONG(perm);
		CHAR(mode);
		break;

	case oldTread:
		f->type = Tread;
		SHORT(tag);
		if(p+2+8+2 > ep)
			return 0;
		SHORT(fid);
		VLONG(offset);
		SHORT(count);
		break;

	case oldTwrite:
		f->type = Twrite;
		SHORT(tag);
		if(p+2+8+2+1 > ep)
			return 0;
		SHORT(fid);
		VLONG(offset);
		SHORT(count);
		p++;	/* pad(1) */
		if(p+f->count > ep)
			return 0;
		f->data = (char*)p;
		p += f->count;
		break;

	case oldTclunk:
		f->type = Tclunk;
		SHORT(tag);
		if(p+2 > ep)
			return 0;
		SHORT(fid);
		break;

	case oldTremove:
		f->type = Tremove;
		SHORT(tag);
		if(p+2 > ep)
			return 0;
		SHORT(fid);
		break;

	case oldTstat:
		f->type = Tstat;
		f->nstat = 116;
		SHORT(tag);
		if(p+2 > ep)
			return 0;
		SHORT(fid);
		break;

	case oldTwstat:
		f->type = Twstat;
		SHORT(tag);
		if(p+2+116 > ep)
			return 0;
		SHORT(fid);
		f->stat = p;
		q = p+28*3+5*4;
		memset(q, 0xFF, 8);	/* clear length to ``don't care'' */
		p += 116;
		break;

/*
	case oldTsession:
		f->type = Tsession;
		SHORT(tag);
		if(p+8 > ep)
			return 0;
		f->chal = p;
		p += 8;
		f->nchal = 8;
		break;
*/
	case oldTsession:
		f->type = Tflush;
		SHORT(tag);
		f->tag |= 0x8000;
		f->oldtag = f->tag;
		p += 8;
		break;

	case oldTattach:
		f->type = Tattach;
		SHORT(tag);
		if(p+2+28+28+72+13 > ep)
			return 0;
		SHORT(fid);
		STRING(uname, 28);
		STRING(aname, 28);
		p += 72+13;
		f->afid = NOFID;
		break;

	default:
		return 0;
	}

	return p-ap;
}

uint
convM2Dold(uchar *ap, uint nap, Dir *f, char *strs)
{
	uchar *p;

	USED(strs);

	if(nap < 116)
		return 0;

	p = (uchar*)ap;
	STRING(name, 28);
	STRING(uid, 28);
	STRING(gid, 28);
	LONG(qid.path);
	LONG(qid.vers);
	LONG(mode);
	LONG(atime);
	LONG(mtime);
	VLONG(length);
	SHORT(type);
	SHORT(dev);
	f->qid.type = (f->mode>>24)&0xF0;
	return p - (uchar*)ap;
}
