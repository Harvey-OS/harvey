#include "lib9.h"
#include "auth.h"
#include "fcall.h"

static void dumpsome(char*, char*, long);
static void fdirconv(char*, Dir*);

int
fcallconv(va_list *v, Fconv *f1)
{
	Fcall *f;
	int fid, type, tag, n;
	char buf[512];
	Dir d;

	f = va_arg(*v, Fcall*);
	type = f->type;
	fid = f->fid;
	tag = f->tag;
	switch(type){
	case Tnop:	/* 50 */
		sprint(buf, "Tnop tag %ud", tag);
		break;
	case Rnop:
		sprint(buf, "Rnop tag %ud", tag);
		break;
	case Tsession:	/* 52 */
		sprint(buf, "Tsession tag %ud", tag);
		break;
	case Rsession:
		sprint(buf, "Rsession tag %ud", tag);
		break;
	case Rerror:	/* 55 */
		sprint(buf, "Rerror tag %ud error %.64s", tag, f->ename);
		break;
	case Tflush:	/* 56 */
		sprint(buf, "Tflush tag %ud oldtag %d", tag, f->oldtag);
		break;
	case Rflush:
		sprint(buf, "Rflush tag %ud", tag);
		break;
	case Tattach:	/* 58 */
		sprint(buf, "Tattach tag %ud fid %d uname %.28s aname %.28s auth %.28s",
			tag, f->fid, f->uname, f->aname, f->auth);
		break;
	case Rattach:
		sprint(buf, "Rattach tag %ud fid %d qid 0x%lux|0x%lux",
			tag, fid, f->qid.path, f->qid.vers);
		break;
	case Tclone:	/* 60 */
		sprint(buf, "Tclone tag %ud fid %d newfid %d", tag, fid, f->newfid);
		break;
	case Rclone:
		sprint(buf, "Rclone tag %ud fid %d", tag, fid);
		break;
	case Twalk:	/* 62 */
		sprint(buf, "Twalk tag %ud fid %d name %.28s", tag, fid, f->name);
		break;
	case Rwalk:
		sprint(buf, "Rwalk tag %ud fid %d qid 0x%lux|0x%lux",
			tag, fid, f->qid.path, f->qid.vers);
		break;
	case Topen:	/* 64 */
		sprint(buf, "Topen tag %ud fid %d mode %d", tag, fid, f->mode);
		break;
	case Ropen:
		sprint(buf, "Ropen tag %ud fid %d qid 0x%lux|0x%lux",
			tag, fid, f->qid.path, f->qid.vers);
		break;
	case Tcreate:	/* 66 */
		sprint(buf, "Tcreate tag %ud fid %d name %.28s perm 0x%lux mode %d",
			tag, fid, f->name, f->perm, f->mode);
		break;
	case Rcreate:
		sprint(buf, "Rcreate tag %ud fid %d qid 0x%lux|0x%lux",
			tag, fid, f->qid.path, f->qid.vers);
		break;
	case Tread:	/* 68 */
		sprint(buf, "Tread tag %ud fid %d offset %ld count %d",
			tag, fid, f->offset, f->count);
		break;
	case Rread:
		n = sprint(buf, "Rread tag %ud fid %d count %d ", tag, fid, f->count);
			dumpsome(buf+n, f->data, f->count);
		break;
	case Twrite:	/* 70 */
		n = sprint(buf, "Twrite tag %ud fid %d offset %ld count %d ",
			tag, fid, f->offset, f->count);
		dumpsome(buf+n, f->data, f->count);
		break;
	case Rwrite:
		sprint(buf, "Rwrite tag %ud fid %d count %d", tag, fid, f->count);
		break;
	case Tclunk:	/* 72 */
		sprint(buf, "Tclunk tag %ud fid %d", tag, fid);
		break;
	case Rclunk:
		sprint(buf, "Rclunk tag %ud fid %d", tag, fid);
		break;
	case Tremove:	/* 74 */
		sprint(buf, "Tremove tag %ud fid %d", tag, fid);
		break;
	case Rremove:
		sprint(buf, "Rremove tag %ud fid %d", tag, fid);
		break;
	case Tstat:	/* 76 */
		sprint(buf, "Tstat tag %ud fid %d", tag, fid);
		break;
	case Rstat:
		n = sprint(buf, "Rstat tag %ud fid %d", tag, fid);
		convM2D(f->stat, &d);
		sprint(buf+n, " stat ");
		fdirconv(buf+n+6, &d);
		break;
	case Twstat:	/* 78 */
		convM2D(f->stat, &d);
		n = sprint(buf, "Twstat tag %ud fid %d stat ", tag, fid);
		fdirconv(buf+n, &d);
		break;
	case Rwstat:
		sprint(buf, "Rwstat tag %ud fid %d", tag, fid);
		break;
	default:
		sprint(buf,  "unknown type %d", type);
	}
	strconv(buf, f1);
	return 0;
}

int
dirconv(va_list *v, Fconv *f)
{
	char buf[160];

	fdirconv(buf, va_arg(*v, Dir*));
	strconv(buf, f);
	return 0;
}

static void
fdirconv(char *buf, Dir *d)
{
	sprint(buf, "'%s' '%s' '%s' q %#lux|%#lux m %#luo at %ld mt %ld l %ld t %d d %d\n",
			d->name, d->uid, d->gid,
			d->qid.path, d->qid.vers, d->mode,
			d->atime, d->mtime, d->length,
			d->type, d->dev);
}

/*
 * dump out count (or DUMPL, if count is bigger) bytes from
 * buf to ans, as a string if they are all printable,
 * else as a series of hex bytes
 */
#define DUMPL 24

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
