/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <fcall.h>

static uint dumpsome(char*, char*, char*, int32_t);
static void fdirconv(char*, char*, Dir*);
static char *qidtype(char*, uint8_t);

#define	QIDFMT	"(%.16llux %lu %s)"

int
fcallfmt(Fmt *fmt)
{
	Fcall *f;
	int fid, type, tag, i;
	char buf[512], tmp[200];
	char *p, *e;
	Dir *d;
	Qid *q;

	e = buf+sizeof(buf);
	f = va_arg(fmt->args, Fcall*);
	type = f->type;
	fid = f->fid;
	tag = f->tag;
	switch(type){
	case Tversion:	/* 100 */
		seprint(buf, e, "Tversion tag %u msize %u version '%s'", tag, f->msize, f->version);
		break;
	case Rversion:
		seprint(buf, e, "Rversion tag %u msize %u version '%s'", tag, f->msize, f->version);
		break;
	case Tauth:	/* 102 */
		seprint(buf, e, "Tauth tag %u afid %d uname %s aname %s", tag,
			f->afid, f->uname, f->aname);
		break;
	case Rauth:
		seprint(buf, e, "Rauth tag %u qid " QIDFMT, tag,
			f->aqid.path, f->aqid.vers, qidtype(tmp, f->aqid.type));
		break;
	case Tattach:	/* 104 */
		seprint(buf, e, "Tattach tag %u fid %d afid %d uname %s aname %s", tag,
			fid, f->afid, f->uname, f->aname);
		break;
	case Rattach:
		seprint(buf, e, "Rattach tag %u qid " QIDFMT, tag,
			f->qid.path, f->qid.vers, qidtype(tmp, f->qid.type));
		break;
	case Rerror:	/* 107; 106 (Terror) illegal */
		seprint(buf, e, "Rerror tag %u ename %s", tag, f->ename);
		break;
	case Tflush:	/* 108 */
		seprint(buf, e, "Tflush tag %u oldtag %u", tag, f->oldtag);
		break;
	case Rflush:
		seprint(buf, e, "Rflush tag %u", tag);
		break;
	case Twalk:	/* 110 */
		p = seprint(buf, e, "Twalk tag %u fid %d newfid %d nwname %d ", tag, fid, f->newfid, f->nwname);
		if(f->nwname <= MAXWELEM)
			for(i=0; i<f->nwname; i++)
				p = seprint(p, e, "%d:%s ", i, f->wname[i]);
		break;
	case Rwalk:
		p = seprint(buf, e, "Rwalk tag %u nwqid %u ", tag, f->nwqid);
		if(f->nwqid <= MAXWELEM)
			for(i=0; i<f->nwqid; i++){
				q = &f->wqid[i];
				p = seprint(p, e, "%d:" QIDFMT " ", i,
					q->path, q->vers, qidtype(tmp, q->type));
			}
		break;
	case Topen:	/* 112 */
		seprint(buf, e, "Topen tag %u fid %u mode %d", tag, fid, f->mode);
		break;
	case Ropen:
		seprint(buf, e, "Ropen tag %u qid " QIDFMT " iounit %u ", tag,
			f->qid.path, f->qid.vers, qidtype(tmp, f->qid.type), f->iounit);
		break;
	case Tcreate:	/* 114 */
		seprint(buf, e, "Tcreate tag %u fid %u name %s perm %M mode %d", tag, fid, f->name,
			(uint32_t)f->perm, f->mode);
		break;
	case Rcreate:
		seprint(buf, e, "Rcreate tag %u qid " QIDFMT " iounit %u ", tag,
			f->qid.path, f->qid.vers, qidtype(tmp, f->qid.type), f->iounit);
		break;
	case Tread:	/* 116 */
		seprint(buf, e, "Tread tag %u fid %d offset %lld count %u",
			tag, fid, f->offset, f->count);
		break;
	case Rread:
		p = seprint(buf, e, "Rread tag %u count %u ", tag, f->count);
			dumpsome(p, e, f->data, f->count);
		break;
	case Twrite:	/* 118 */
		p = seprint(buf, e, "Twrite tag %u fid %d offset %lld count %u ",
			tag, fid, f->offset, f->count);
		dumpsome(p, e, f->data, f->count);
		break;
	case Rwrite:
		seprint(buf, e, "Rwrite tag %u count %u", tag, f->count);
		break;
	case Tclunk:	/* 120 */
		seprint(buf, e, "Tclunk tag %u fid %u", tag, fid);
		break;
	case Rclunk:
		seprint(buf, e, "Rclunk tag %u", tag);
		break;
	case Tremove:	/* 122 */
		seprint(buf, e, "Tremove tag %u fid %u", tag, fid);
		break;
	case Rremove:
		seprint(buf, e, "Rremove tag %u", tag);
		break;
	case Tstat:	/* 124 */
		seprint(buf, e, "Tstat tag %u fid %u", tag, fid);
		break;
	case Rstat:
		p = seprint(buf, e, "Rstat tag %u ", tag);
		if(f->nstat > sizeof tmp)
			seprint(p, e, " stat(%d bytes)", f->nstat);
		else{
			d = (Dir*)tmp;
			convM2D(f->stat, f->nstat, d, (char*)(d+1));
			seprint(p, e, " stat ");
			fdirconv(p+6, e, d);
		}
		break;
	case Twstat:	/* 126 */
		p = seprint(buf, e, "Twstat tag %u fid %u", tag, fid);
		if(f->nstat > sizeof tmp)
			seprint(p, e, " stat(%d bytes)", f->nstat);
		else{
			d = (Dir*)tmp;
			convM2D(f->stat, f->nstat, d, (char*)(d+1));
			seprint(p, e, " stat ");
			fdirconv(p+6, e, d);
		}
		break;
	case Rwstat:
		seprint(buf, e, "Rwstat tag %u", tag);
		break;
	default:
		seprint(buf, e,  "unknown type %d", type);
	}
	return fmtstrcpy(fmt, buf);
}

static char*
qidtype(char *s, uint8_t t)
{
	char *p;

	p = s;
	if(t & QTDIR)
		*p++ = 'd';
	if(t & QTAPPEND)
		*p++ = 'a';
	if(t & QTEXCL)
		*p++ = 'l';
	if(t & QTAUTH)
		*p++ = 'A';
	*p = '\0';
	return s;
}

int
dirfmt(Fmt *fmt)
{
	char buf[160];

	fdirconv(buf, buf+sizeof buf, va_arg(fmt->args, Dir*));
	return fmtstrcpy(fmt, buf);
}

static void
fdirconv(char *buf, char *e, Dir *d)
{
	char tmp[16];

	seprint(buf, e, "'%s' '%s' '%s' '%s' "
		"q " QIDFMT " m %#luo "
		"at %ld mt %ld l %lld "
		"t %d d %d",
			d->name, d->uid, d->gid, d->muid,
			d->qid.path, d->qid.vers, qidtype(tmp, d->qid.type), d->mode,
			d->atime, d->mtime, d->length,
			d->type, d->dev);
}

/*
 * dump out count (or DUMPL, if count is bigger) bytes from
 * buf to ans, as a string if they are all printable,
 * else as a series of hex bytes
 */
#define DUMPL 64

static uint
dumpsome(char *ans, char *e, char *buf, int32_t count)
{
	int i, printable;
	char *p;

	if(buf == nil){
		seprint(ans, e, "<no data>");
		return strlen(ans);
	}
	printable = 1;
	if(count > DUMPL)
		count = DUMPL;
	for(i=0; i<count && printable; i++)
		if((buf[i]<32 && buf[i] !='\n' && buf[i] !='\t') || (uint8_t)buf[i]>127)
			printable = 0;
	p = ans;
	*p++ = '\'';
	if(printable){
		if(count > e-p-2)
			count = e-p-2;
		memmove(p, buf, count);
		p += count;
	}else{
		if(2*count > e-p-2)
			count = (e-p-2)/2;
		for(i=0; i<count; i++){
			if(i>0 && i%4==0)
				*p++ = ' ';
			sprint(p, "%2.2x", (uint8_t)buf[i]);
			p += 2;
		}
	}
	*p++ = '\'';
	*p = 0;
	return p - ans;
}
