#include "all.h"
#include "9p1.h"

static void dumpsome(char*, char*, long);
static void fdirconv(char*, Dentry*);

int
ofcallfmt(Fmt *f1)
{
	char buf[512];
	Oldfcall *f;
	int fid, type, tag, n;
	Dentry d;

	f = va_arg(f1->args, Oldfcall*);
	type = f->type;
	fid = f->fid;
	tag = f->tag;
	switch(type){
	case Tnop9p1:	/* 50 */
		sprint(buf, "Tnop9p1 tag %ud", tag);
		break;
	case Rnop9p1:
		sprint(buf, "Rnop9p1 tag %ud", tag);
		break;
	case Tsession9p1:	/* 52 */
		sprint(buf, "Tsession9p1 tag %ud", tag);
		break;
	case Rsession9p1:
		sprint(buf, "Rsession9p1 tag %ud", tag);
		break;
	case Rerror9p1:	/* 55 */
		sprint(buf, "Rerror9p1 tag %ud error %.64s", tag, f->ename);
		break;
	case Tflush9p1:	/* 56 */
		sprint(buf, "Tflush9p1 tag %ud oldtag %d", tag, f->oldtag);
		break;
	case Rflush9p1:
		sprint(buf, "Rflush9p1 tag %ud", tag);
		break;
	case Tattach9p1:	/* 58 */
		sprint(buf, "Tattach9p1 tag %ud fid %d uname %.28s aname %.28s auth %.28s",
			tag, f->fid, f->uname, f->aname, f->auth);
		break;
	case Rattach9p1:
		sprint(buf, "Rattach9p1 tag %ud fid %d qid 0x%lux|0x%lux",
			tag, fid, f->qid.path, f->qid.version);
		break;
	case Tclone9p1:	/* 60 */
		sprint(buf, "Tclone9p1 tag %ud fid %d newfid %d", tag, fid, f->newfid);
		break;
	case Rclone9p1:
		sprint(buf, "Rclone9p1 tag %ud fid %d", tag, fid);
		break;
	case Twalk9p1:	/* 62 */
		sprint(buf, "Twalk9p1 tag %ud fid %d name %.28s", tag, fid, f->name);
		break;
	case Rwalk9p1:
		sprint(buf, "Rwalk9p1 tag %ud fid %d qid 0x%lux|0x%lux",
			tag, fid, f->qid.path, f->qid.version);
		break;
	case Topen9p1:	/* 64 */
		sprint(buf, "Topen9p1 tag %ud fid %d mode %d", tag, fid, f->mode);
		break;
	case Ropen9p1:
		sprint(buf, "Ropen9p1 tag %ud fid %d qid 0x%lux|0x%lux",
			tag, fid, f->qid.path, f->qid.version);
		break;
	case Tcreate9p1:	/* 66 */
		sprint(buf, "Tcreate9p1 tag %ud fid %d name %.28s perm 0x%lux mode %d",
			tag, fid, f->name, f->perm, f->mode);
		break;
	case Rcreate9p1:
		sprint(buf, "Rcreate9p1 tag %ud fid %d qid 0x%lux|0x%lux",
			tag, fid, f->qid.path, f->qid.version);
		break;
	case Tread9p1:	/* 68 */
		sprint(buf, "Tread9p1 tag %ud fid %d offset %ld count %ld",
			tag, fid, f->offset, f->count);
		break;
	case Rread9p1:
		n = sprint(buf, "Rread9p1 tag %ud fid %d count %ld ", tag, fid, f->count);
		dumpsome(buf+n, f->data, f->count);
		break;
	case Twrite9p1:	/* 70 */
		n = sprint(buf, "Twrite9p1 tag %ud fid %d offset %ld count %ld ",
			tag, fid, f->offset, f->count);
		dumpsome(buf+n, f->data, f->count);
		break;
	case Rwrite9p1:
		sprint(buf, "Rwrite9p1 tag %ud fid %d count %ld", tag, fid, f->count);
		break;
	case Tclunk9p1:	/* 72 */
		sprint(buf, "Tclunk9p1 tag %ud fid %d", tag, fid);
		break;
	case Rclunk9p1:
		sprint(buf, "Rclunk9p1 tag %ud fid %d", tag, fid);
		break;
	case Tremove9p1:	/* 74 */
		sprint(buf, "Tremove9p1 tag %ud fid %d", tag, fid);
		break;
	case Rremove9p1:
		sprint(buf, "Rremove9p1 tag %ud fid %d", tag, fid);
		break;
	case Tstat9p1:	/* 76 */
		sprint(buf, "Tstat9p1 tag %ud fid %d", tag, fid);
		break;
	case Rstat9p1:
		n = sprint(buf, "Rstat9p1 tag %ud fid %d", tag, fid);
		convM2D9p1(f->stat, &d);
		sprint(buf+n, " stat ");
		fdirconv(buf+n+6, &d);
		break;
	case Twstat9p1:	/* 78 */
		convM2D9p1(f->stat, &d);
		n = sprint(buf, "Twstat9p1 tag %ud fid %d stat ", tag, fid);
		fdirconv(buf+n, &d);
		break;
	case Rwstat9p1:
		sprint(buf, "Rwstat9p1 tag %ud fid %d", tag, fid);
		break;
	case Tclwalk9p1:	/* 81 */
		sprint(buf, "Tclwalk9p1 tag %ud fid %d newfid %d name %.28s",
			tag, fid, f->newfid, f->name);
		break;
	case Rclwalk9p1:
		sprint(buf, "Rclwalk9p1 tag %ud fid %d qid 0x%lux|0x%lux",
			tag, fid, f->qid.path, f->qid.version);
		break;
	default:
		sprint(buf,  "unknown type %d", type);
	}
	return fmtstrcpy(f1, buf);
}

static void
fdirconv(char *buf, Dentry *d)
{
	sprint(buf, "'%s' uid=%d gid=%d "
		"q %lux|%lux m %uo "
		"at %ld mt %ld l %ld ",
			d->name, d->uid, d->gid,
			d->qid.path, d->qid.version, d->mode,
			d->atime, d->mtime, d->size);
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
