/*
 * Present venti hash tree as file system for debugging.
 */

#include <u.h>
#include <libc.h>
#include <venti.h>
#include <vac.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>

VtFile *rootf;

void
responderrstr(Req *r)
{
	char e[ERRMAX];

	rerrstr(e, sizeof e);
	respond(r, e);
}

void
mkqide(Qid *q, VtEntry *e)
{
	if(e->flags&VtEntryDir)
		q->type = QTDIR;
	else
		q->type = 0;
	q->path = *(uvlong*)e->score;
	q->vers = *(ulong*)(e->score+8);
}

void
mkqid(Qid *q, VtFile *f)
{
	VtEntry e;

	vtFileLock(f, VtOReadOnly);
	vtFileGetEntry(f, &e);
	vtFileUnlock(f);
	mkqide(q, &e);
}

void
fsdestroyfid(Fid *fid)
{
	if(fid->aux)
		vtFileClose(fid->aux);
}

void
fsattach(Req *r)
{
	r->fid->aux = rootf;
	vtFileIncRef(rootf);
	mkqid(&r->ofcall.qid, rootf);
	r->fid->qid = r->ofcall.qid;
	respond(r, nil);
}

void
fsopen(Req *r)
{
	if(r->ifcall.mode != OREAD){
		respond(r, "read-only file system");
		return;
	}
	r->ofcall.qid = r->fid->qid;
	respond(r, nil);
}

void
fsreaddir(Req *r)
{
	Dir d;
	int dsize, nn;
	char score[2*VtScoreSize+1];
	uchar edata[VtEntrySize], *p, *ep;
	u32int i, n, epb;
	u64int o;
	VtFile *f;
	VtEntry e;

	memset(&d, 0, sizeof d);
	d.name = "0123456789012345678901234567890123456789";
	d.uid = "venti";
	d.gid = "venti";
	d.muid = "";
	d.atime = time(0);
	d.mtime = time(0);
	d.mode = 0444;
	dsize = sizeD2M(&d);

	p = (uchar*)r->ofcall.data;
	ep = p+r->ifcall.count;
	f = r->fid->aux;
	vtFileLock(f, VtOReadOnly);
	vtFileGetEntry(f, &e);
	epb = e.dsize / VtEntrySize;
	n = vtFileGetDirSize(f);
fprint(2, "dirsize %d\n", n);
	i = r->ifcall.offset / dsize;
	for(; i<n; i++){
fprint(2, "%d...", i);
		if(p+dsize > ep)
{
fprint(2, "done %p %d %p\n", p, dsize, ep);
			break;
}
		o = (i/epb)*e.dsize+(i%epb)*VtEntrySize;
		if(vtFileRead(f, edata, VtEntrySize, o) != VtEntrySize){
			vtFileUnlock(f);
			responderrstr(r);
			return;
		}
		if(!vtEntryUnpack(&e, edata, 0)){
			fprint(2, "entryunpack: %R\n");
			continue;
		}
		if(!(e.flags&VtEntryActive)){
			strcpy(score, "________________________________________");
			d.qid.type = 0;
		}else{
			snprint(score, sizeof score, "%V", e.score);
			mkqide(&d.qid, &e);
		}
		d.name = score;
		if(e.flags&VtEntryDir){
			d.mode = DMDIR|0555;
			d.qid.type = QTDIR;
		}else{
			d.mode = 0444;
			d.qid.type = 0;
		}
		d.length = e.size;
		nn = convD2M(&d, p, dsize);
		if(nn != dsize)
			fprint(2, "oops: dsize mismatch\n");
		p += dsize;
	}
	r->ofcall.count = p - (uchar*)r->ofcall.data;
	vtFileUnlock(f);
	respond(r, nil);
}

void
fsread(Req *r)
{
	long n;

	if(r->fid->qid.type&QTDIR){
		fsreaddir(r);
		return;
	}
	vtFileLock(r->fid->aux, VtOReadOnly);
	n = vtFileRead(r->fid->aux, r->ofcall.data, r->ifcall.count, r->ifcall.offset);
	vtFileUnlock(r->fid->aux);
	if(n < 0){
		responderrstr(r);
		return;
	}
	r->ofcall.count = n;
	respond(r, nil);
}

void
fsstat(Req *r)
{
	VtEntry e;

	vtFileLock(r->fid->aux, VtOReadOnly);
	if(!vtFileGetEntry(r->fid->aux, &e)){
		vtFileUnlock(r->fid->aux);
		responderrstr(r);
		return;
	}
	vtFileUnlock(r->fid->aux);
	memset(&r->d, 0, sizeof r->d);
	r->d.name = smprint("%V", e.score);
	r->d.qid = r->fid->qid;
	r->d.mode = 0444;
	if(r->d.qid.type&QTDIR)
		r->d.mode |= DMDIR|0111;
	r->d.uid = estrdup9p("venti");
	r->d.gid = estrdup9p("venti");
	r->d.muid = estrdup9p("");
	r->d.atime = time(0);
	r->d.mtime = time(0);
	r->d.length = e.size;
	respond(r, nil);
}

char*
fswalk1(Fid *fid, char *name, void*)
{
	uchar score[VtScoreSize], edata[VtEntrySize];
	u32int i, n, epb;
	u64int o;
	VtFile *f, *ff;
	VtEntry e;

	if(dec16(score, sizeof score, name, strlen(name)) != VtScoreSize)
		return "file not found";

	f = fid->aux;
	vtFileGetEntry(f, &e);
	epb = e.dsize / VtEntrySize;
	n = vtFileGetDirSize(f);
	o = 0;
	for(i=0; i<n; i++){
		o = (i/epb)*e.dsize+(i%epb)*VtEntrySize;
		if(vtFileRead(f, edata, VtEntrySize, o) != VtEntrySize)
			return "error in vtfileread";
		if(!vtEntryUnpack(&e, edata, 0))
			continue;
		if(!(e.flags&VtEntryActive))
			continue;
		if(memcmp(e.score, score, VtScoreSize) == 0)
			break;
	}
	if(i == n)
		return "entry not found";
	ff = vtFileOpen(f, i, VtOReadOnly);
	if(ff == nil)
		return "error in vtfileopen";
	vtFileClose(f);
	fid->aux = ff;
	mkqid(&fid->qid, ff);
	return nil;
}

char*
fsclone(Fid *fid, Fid *newfid, void*)
{
	newfid->aux = fid->aux;
	vtFileIncRef(fid->aux);
	return nil;
}

void
fswalk(Req *r)
{
	VtFile *f;

	f = r->fid->aux;
	vtFileLock(f, VtOReadOnly);
	walkandclone(r, fswalk1, fsclone, nil);
	vtFileUnlock(f);
}

Srv fs =
{
.destroyfid=	fsdestroyfid,
.attach=		fsattach,
.open=		fsopen,
.read=		fsread,
.stat=		fsstat,
.walk=		fswalk
};

void
usage(void)
{
	fprint(2, "usage: ventifs score\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	VtSession *z;
	VtCache *c;
	VtBlock *b;
	VtRoot root;
	VtEntry e;
	int bsize;
	uchar score[VtScoreSize];

	bsize = 16384;
	ARGBEGIN{
	case 'D':
		chatty9p++;
		break;
	case 'b':
		bsize = atoi(EARGF(usage()));
		break;
	default:
		usage();
	}ARGEND

	if(argc != 1)
		usage();

	fmtinstall('V', vtScoreFmt);
	fmtinstall('R', vtErrFmt);

	vtAttach();
	if((z = vtDial(nil, 0)) == nil)
		vtFatal("vtDial: %r");
	if(!vtConnect(z, nil))
		vtFatal("vtConnect: %r");

	c = vtCacheAlloc(z, bsize, 50, VtOReadWrite);

	if(dec16(score, sizeof score, argv[0], strlen(argv[0])) != VtScoreSize)
		vtFatal("bad score: %R");
	b = vtCacheGlobal(c, score, VtRootType);
	if(b){
		if(!vtRootUnpack(&root, b->data))
			vtFatal("bad root: %R");
		memmove(score, root.score, VtScoreSize);
		vtBlockPut(b);
	}
	b = vtCacheGlobal(c, score, VtDirType);
	if(b == nil)
		sysfatal("vtCacheGlobal %V: %R", score);
	if(!vtEntryUnpack(&e, b->data, 0))
		sysfatal("%V: vtEntryUnpack failed", score);
	fprint(2, "entry: size %llud psize %d dsize %d\n",
		e.size, e.psize, e.dsize);
	vtBlockPut(b);

	if(!(e.flags&VtEntryDir)){
		b = vtCacheAllocBlock(c, VtDirType);
		vtEntryPack(&e, b->data, 0);
		memmove(e.score, b->score, VtScoreSize);
		e.flags |= VtEntryDir;
		e.size = VtEntrySize;
		e.depth = 0;
		vtBlockPut(b);
	}

	rootf = vtFileOpenRoot(c, &e);
	if(rootf == nil)
		vtFatal("vtFileOpenRoot: %R");

	postmountsrv(&fs, nil, "/n/kremvax", MBEFORE);
}
