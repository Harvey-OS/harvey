#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>

static char Ebadattach[] = "unknown specifier in attach";
static char Ebadoffset[] = "bad offset";
static char Ebadcount[] = "bad count";
static char Ebotch[] = "9P protocol botch";
static char Ecreatenondir[] = "create in non-directory";
static char Edupfid[] = "duplicate fid";
static char Eduptag[] = "duplicate tag";
static char Eisdir[] = "is a directory";
static char Enocreate[] = "create prohibited";
static char Enomem[] = "out of memory";
static char Enoremove[] = "remove prohibited";
static char Enostat[] = "stat prohibited";
static char Enotfound[] = "file not found";
static char Enowrite[] = "write prohibited";
static char Enowstat[] = "wstat prohibited";
static char Eperm[] = "permission denied";
static char Eunknownfid[] = "unknown fid";
static char Ebaddir[] = "bad directory in wstat";
static char Ewalknodir[] = "walk in non-directory";

static void
setfcallerror(Fcall *f, char *err)
{
	f->ename = err;
	f->type = Rerror;
}

static void
changemsize(Srv *srv, int msize)
{
	if(srv->rbuf && srv->wbuf && srv->msize == msize)
		return;
	qlock(&srv->rlock);
	qlock(&srv->wlock);
	srv->msize = msize;
	free(srv->rbuf);
	free(srv->wbuf);
	srv->rbuf = emalloc9p(msize);
	srv->wbuf = emalloc9p(msize);
	if(srv->rbuf==nil || srv->wbuf==nil)
		sysfatal("out of memory");	/* BUG */
	qunlock(&srv->rlock);
	qunlock(&srv->wlock);
}

static Req*
getreq(Srv *s)
{
	long n;
	uchar *buf;
	Fcall f;
	Req *r;

	qlock(&s->rlock);
	if((n = read9pmsg(s->infd, s->rbuf, s->msize)) <= 0){
		qunlock(&s->rlock);
		return nil;
	}

	buf = emalloc9p(n);
	memmove(buf, s->rbuf, n);
	qunlock(&s->rlock);

	if(convM2S(buf, n, &f) != n){
		free(buf);
		return nil;
	}

	if((r=allocreq(s->rpool, f.tag)) == nil){	/* duplicate tag: cons up a fake Req */
		r = emalloc9p(sizeof *r);
		incref(&r->ref);
		r->tag = f.tag;
		r->ifcall = f;
		r->error = Eduptag;
		r->buf = buf;
		r->responded = 0;
		r->type = 0;
		r->srv = s;
		r->pool = nil;
if(chatty9p)
	fprint(2, "<-%d- %F: dup tag\n", s->infd, &f);
		return r;
	}

	r->srv = s;
	r->responded = 0;
	r->buf = buf;
	r->ifcall = f;
	memset(&r->ofcall, 0, sizeof r->ofcall);
	r->type = r->ifcall.type;

if(chatty9p)
	if(r->error)
		fprint(2, "<-%d- %F: %s\n", s->infd, &r->ifcall, r->error);
	else	
		fprint(2, "<-%d- %F\n", s->infd, &r->ifcall);

	return r;
}

static void
filewalk(Req *r)
{
	int i;
	File *f;

	f = r->fid->file;
	assert(f != nil);

	incref(f);
	for(i=0; i<r->ifcall.nwname; i++)
		if(f = walkfile(f, r->ifcall.wname[i]))
			r->ofcall.wqid[i] = f->qid;
		else
			break;

	r->ofcall.nwqid = i;
	if(f){
		r->newfid->file = f;
		r->newfid->qid = r->newfid->file->qid;
	}
	respond(r, nil);
}

static void
conswalk(Req *r, char *(*clone)(Fid*, Fid*), char *(*walk1)(Fid*, char*, Qid*))
{
	int i;
	char *e;

	if(r->fid == r->newfid && r->ifcall.nwname > 1){
		respond(r, "lib9p: unused documented feature not implemented");
		return;
	}

	if(r->fid != r->newfid){
		r->newfid->qid = r->fid->qid;
		if(clone && (e = clone(r->fid, r->newfid))){
			respond(r, e);
			return;
		}
	}

	e = nil;
	for(i=0; i<r->ifcall.nwname; i++){
		if(e = walk1(r->newfid, r->ifcall.wname[i], &r->ofcall.wqid[i]))
			break;
		r->newfid->qid = r->ofcall.wqid[i];
	}

	r->ofcall.nwqid = i;
	if(e && i==0)
		respond(r, e);
	else
		respond(r, nil);
}

void
srv(Srv *srv)
{
	int o, p;
	Req *r;
	char e[ERRMAX];

	fmtinstall('D', dirfmt);
	fmtinstall('F', fcallfmt);

	if(srv->fpool == nil)
		srv->fpool = allocfidpool(srv->destroyfid);
	if(srv->rpool == nil)
		srv->rpool = allocreqpool(srv->destroyreq);
	if(srv->msize == 0)
		srv->msize = 8192+IOHDRSZ;

	changemsize(srv, srv->msize);

	srv->fpool->srv = srv;
	srv->rpool->srv = srv;

	while(r = getreq(srv)){
		if(r->error){
			respond(r, r->error);
			continue;	
		}

		switch(r->type){
		default:
			respond(r, "unknown message");
			break;

		case Tversion:
			if(strncmp(r->ifcall.version, "9P", 2) != 0){
				r->ofcall.version = "unknown";
				respond(r, nil);
				break;
			}

			r->ofcall.version = "9P2000";
			r->ofcall.msize = r->ifcall.msize;
			respond(r, nil);
			break;

		case Tauth:
			if((r->afid = allocfid(srv->fpool, r->ifcall.afid)) == nil){
				respond(r, Edupfid);
				break;
			}
			if(srv->auth)
				srv->auth(r);
			else{
				snprint(e, sizeof e, "%s: authentication not required", argv0);
				respond(r, e);
			}
			break;

		case Tattach:
			if((r->fid = allocfid(srv->fpool, r->ifcall.fid)) == nil){
				respond(r, Edupfid);
				break;
			}
			r->afid = nil;
			if(r->ifcall.afid != NOFID && (r->afid = allocfid(srv->fpool, r->ifcall.fid)) == nil){
				respond(r, Eunknownfid);
				break;
			}
			r->fid->uid = estrdup9p(r->ifcall.uname);
			if(srv->tree){
				r->fid->file = srv->tree->root;
				/* BUG? incref(r->fid->file) ??? */
				r->ofcall.qid = r->fid->file->qid;
				r->fid->qid = r->ofcall.qid;
			}
			if(srv->attach)
				srv->attach(r);
			else
				respond(r, nil);
			break;

		case Tflush:
			r->oldreq = lookupreq(srv->rpool, r->ifcall.oldtag);
			if(r->oldreq == nil || r->oldreq == r)
				respond(r, nil);
			else if(srv->flush)
				srv->flush(r);
			else
				sysfatal("outstanding message but flush not provided");
			break;

		case Twalk:
			if((r->fid = lookupfid(srv->fpool, r->ifcall.fid)) == nil){
				respond(r, Eunknownfid);
				break;
			}
			if(r->fid->omode != -1){
				respond(r, "cannot clone open fid");
				break;
			}
			if(r->ifcall.nwname && !(r->fid->qid.type&QTDIR)){
				respond(r, Ewalknodir);
				break;
			}
			if(r->ifcall.fid != r->ifcall.newfid){
				if((r->newfid = allocfid(srv->fpool, r->ifcall.newfid)) == nil){
					respond(r, Edupfid);
					break;
				}
				r->newfid->uid = estrdup9p(r->fid->uid);
			}else{
				incref(&r->fid->ref);
				r->newfid = r->fid;
			}

			if(r->fid->file){
				filewalk(r);
			}else if(srv->walk1)
				conswalk(r, srv->clone, srv->walk1);
			else if(srv->walk)
				srv->walk(r);
			else
				sysfatal("no walk function, no file trees");
			break;

		case Topen:
			if((r->fid = lookupfid(srv->fpool, r->ifcall.fid)) == nil){
				respond(r, Eunknownfid);
				break;
			}
			if(r->fid->omode != -1){
				respond(r, Ebotch);
				break;
			}
			if((r->fid->qid.type&QTDIR) && (r->ifcall.mode&~ORCLOSE) != OREAD){
				respond(r, Eisdir);
				break;
			}
			r->ofcall.qid = r->fid->qid;
			switch(r->ifcall.mode&3){
			default:
				assert(0);
			case OREAD:
				p = AREAD;	
				break;
			case OWRITE:
				p = AWRITE;
				break;
			case ORDWR:
				p = AREAD|AWRITE;
				break;
			case OEXEC:
				p = AEXEC;	
				break;
			}
			if(r->ifcall.mode&OTRUNC)
				p |= AWRITE;
			if((r->fid->qid.type&QTDIR) && p!=AREAD){
				respond(r, Eperm);
				break;
			}
			if(r->fid->file){
				if(!hasperm(r->fid->file, r->fid->uid, p)){
					respond(r, Eperm);
					break;
				}
			/* BUG RACE */
				if((r->ifcall.mode&ORCLOSE)
				&& !hasperm(r->fid->file->parent, r->fid->uid, AWRITE)){
					respond(r, Eperm);
					break;
				}
				r->ofcall.qid = r->fid->file->qid;
				if((r->ofcall.qid.type&QTDIR)
				&& (r->fid->rdir = opendirfile(r->fid->file)) == nil){
					respond(r, "opendirfile failed");
					break;
				}
			}
			if(srv->open)
				srv->open(r);
			else
				respond(r, nil);
			break;
			
		case Tcreate:
			if((r->fid = lookupfid(srv->fpool, r->ifcall.fid)) == nil)
				respond(r, Eunknownfid);
			else if(r->fid->omode != -1)
				respond(r, Ebotch);
			else if(!(r->fid->qid.type&QTDIR))
				respond(r, Ecreatenondir);
			else if(r->fid->file && !hasperm(r->fid->file, r->fid->uid, AWRITE))
				respond(r, Eperm);
			else if(srv->create)
				srv->create(r);
			else
				respond(r, Enocreate);
			break;

		case Tread:
			if((r->fid = lookupfid(srv->fpool, r->ifcall.fid)) == nil){
				respond(r, Eunknownfid);
				break;
			}
			if(r->ifcall.count < 0){
				respond(r, Ebotch);
				break;
			}
			if(r->ifcall.offset < 0
			|| ((r->fid->qid.type&QTDIR) && r->ifcall.offset != 0 && r->ifcall.offset != r->fid->diroffset)){
				respond(r, Ebadoffset);
				break;
			}

			if(r->ifcall.count > srv->msize - IOHDRSZ)
				r->ifcall.count = srv->msize - IOHDRSZ;
			if((r->rbuf = emalloc9p(r->ifcall.count)) == nil){
				respond(r, Enomem);
				break;
			}
			r->ofcall.data = r->rbuf;
			o = r->fid->omode & 3;
			if(o != OREAD && o != ORDWR && o != OEXEC){
				respond(r, Ebotch);
				break;
			}
			if((r->fid->qid.type&QTDIR) && r->fid->file){
				r->ofcall.count = readdirfile(r->fid->rdir, r->rbuf, r->ifcall.count);
				respond(r, nil);
				break;
			}
			if(srv->read)
				srv->read(r);
			else
				respond(r, "no srv->read");
			break;

		case Twrite:
			if((r->fid = lookupfid(srv->fpool, r->ifcall.fid)) == nil){
				respond(r, Eunknownfid);
				break;
			}
			if(r->ifcall.count < 0){
				respond(r, Ebotch);
				break;
			}
			if(r->ifcall.offset < 0){
				respond(r, Ebotch);
				break;
			}
			if(r->ifcall.count > srv->msize - IOHDRSZ)
				r->ifcall.count = srv->msize - IOHDRSZ;
			o = r->fid->omode & 3;
			if(o != OWRITE && o != ORDWR){
				snprint(e, sizeof e, "write on fid with open mode 0x%ux", r->fid->omode);
				respond(r, e);
				break;
			}
			if(srv->write)
				srv->write(r);
			else
				respond(r, "no srv->write");
			break;

		case Tclunk:
			if((r->fid = removefid(srv->fpool, r->ifcall.fid)) == nil)
				respond(r, Eunknownfid);
			else
				respond(r, nil);
			break;

		case Tremove:
			if((r->fid = removefid(srv->fpool, r->ifcall.fid)) == nil){
				respond(r, Eunknownfid);
				break;
			}
			/* BUG RACE */
			if(r->fid->file && !hasperm(r->fid->file->parent, r->fid->uid, AWRITE)){
				respond(r, Eperm);
				break;
			}
			if(srv->remove)
				srv->remove(r);
			else
				respond(r, r->fid->file ? nil : Enoremove);
			break;

		case Tstat:
			if((r->fid = lookupfid(srv->fpool, r->ifcall.fid)) == nil){
				respond(r, Eunknownfid);
				break;
			}
			if(r->fid->file){
				r->d = r->fid->file->Dir;
				if(r->d.name)
					r->d.name = estrdup9p(r->d.name);
				if(r->d.uid)
					r->d.uid = estrdup9p(r->d.uid);
				if(r->d.gid)
					r->d.gid = estrdup9p(r->d.gid);
				if(r->d.muid)
					r->d.muid = estrdup9p(r->d.muid);
			}
			if(srv->stat)	
				srv->stat(r);	
			else if(r->fid->file)
				respond(r, nil);
			else
				respond(r, Enostat);
			break;

		case Twstat:
			if((r->fid = lookupfid(srv->fpool, r->ifcall.fid)) == nil)
				respond(r, Eunknownfid);
			else if(srv->wstat){
				if(convM2D(r->ifcall.stat, r->ifcall.nstat, &r->d, (char*)r->ifcall.stat) != r->ifcall.nstat)
					respond(r, Ebaddir);
				else
					srv->wstat(r);
			}else
				respond(r, Enowstat);
			break;
		}
	}
}

void
respond(Req *r, char *error)
{
	int m, n;
	Req *or;
	uchar *statbuf, tmp[BIT16SZ];
	char errbuf[ERRMAX];
	Srv *srv;

	srv = r->srv;
	assert(srv != nil);

	assert(r->responded == 0);
	r->responded = 1;

	switch(r->ifcall.type){
	default:
		assert(0);

	case Tversion:
		assert(error == nil);
		changemsize(srv, r->ofcall.msize);
		break;

	case Tauth:
		if(error){
			if(r->afid)
				closefid(removefid(srv->fpool, r->afid->fid));
			break;
		}
		break;

	case Tattach:
		if(error){
			if(r->fid)
				closefid(removefid(srv->fpool, r->fid->fid));
			break;
		}
		break;

	case Tflush:
		assert(error == nil);
		if(r->oldreq){
			if(or = removereq(r->pool, r->oldreq->tag)){
				assert(or == r->oldreq);
				if(or->ifcall.type == Twalk && or->ifcall.fid != or->ifcall.newfid)
					closefid(removefid(srv->fpool, or->newfid->fid));
				closereq(or);
			}
			closereq(r->oldreq);
		}
		r->oldreq = nil;
		break;
		
	case Twalk:
		if(error || r->ofcall.nwqid < r->ifcall.nwname){
			if(r->ifcall.fid != r->ifcall.newfid && r->newfid)
				closefid(removefid(srv->fpool, r->newfid->fid));
			if (r->ofcall.nwqid==0){
				if(error==nil && r->ifcall.nwname!=0)
					error = Enotfound;
			}else
				error = nil;	// No error on partial walks
			break;
		}else{
			if(r->ofcall.nwqid == 0){
				/* Just a clone */
				r->newfid->qid = r->fid->qid;
			}else{
				/* if file trees are in use, filewalk took care of the rest */
				r->newfid->qid = r->ofcall.wqid[r->ofcall.nwqid-1];
			}
		}
		break;

	case Topen:
		if(error){
			break;
		}
		if(chatty9p){
			snprint(errbuf, sizeof errbuf, "fid mode is 0x%ux\n", r->ifcall.mode);
			write(2, errbuf, strlen(errbuf));
		}
		r->fid->omode = r->ifcall.mode;
		r->fid->qid = r->ofcall.qid;
		if(r->ofcall.qid.type&QTDIR)
			r->fid->diroffset = 0;
		break;

	case Tcreate:
		if(error){
			break;
		}
		r->fid->omode = r->ifcall.mode;
		r->fid->qid = r->ofcall.qid;
		break;

	case Tread:
		if(error==nil && (r->fid->qid.type&QTDIR))
			r->fid->diroffset += r->ofcall.count;
		break;

	case Twrite:
		if(error)
			break;
		if(r->fid->file)
			r->fid->file->qid.vers++;
		break;

	case Tclunk:
		break;

	case Tremove:
		if(error)
			break;
		if(r->fid->file){
			if(removefile(r->fid->file) < 0){
				snprint(errbuf, ERRMAX, "remove %s: %r", 
					r->fid->file->name);
				error = errbuf;
			}
			r->fid->file = nil;
		}
		break;

	case Tstat:
		if(error)
			break;
		if(convD2M(&r->d, tmp, BIT16SZ) != BIT16SZ){
			error = "convD2M(_,_,BIT16SZ) did not return BIT16SZ";
			break;
		}
		n = GBIT16(tmp)+BIT16SZ;
		statbuf = emalloc9p(n);
		if(statbuf == nil){
			error = "out of memory";
			break;
		}
		r->ofcall.nstat = convD2M(&r->d, statbuf, n);
		r->ofcall.stat = statbuf;
		if(r->ofcall.nstat < 0){
			error = "convD2M fails";
			break;
		}
		break;

	case Twstat:
		break;
	}

	r->ofcall.tag = r->ifcall.tag;
	r->ofcall.type = r->ifcall.type+1;
	if(error)
		setfcallerror(&r->ofcall, error);

if(chatty9p)
	fprint(2, "-%d-> %F\n", srv->outfd, &r->ofcall);

	qlock(&srv->wlock);
	n = convS2M(&r->ofcall, srv->wbuf, srv->msize);
	if(n <= 0){
		fprint(2, "n = %d %F\n", n, &r->ofcall);
		abort();
	}
	assert(n > 2);
	m = write(srv->outfd, srv->wbuf, n);
	if(m != n)
		sysfatal("lib9p srv: write %d returned %d on fd %d: %r", n, m, srv->outfd);
	qunlock(&srv->wlock);

	if(r->pool){	/* not a fake */
		closereq(removereq(r->pool, r->ifcall.tag));
		closereq(r);
	}else
		free(r);
}

int
postfd(char *name, int pfd)
{
	int fd;
	char buf[80];

	snprint(buf, sizeof buf, "/srv/%s", name);
	if(chatty9p)
		fprint(2, "postfd %s\n", buf);
	fd = create(buf, OWRITE|ORCLOSE|OCEXEC, 0600);
	if(fd < 0){
		if(chatty9p)
			fprint(2, "create fails: %r\n");
		return -1;
	}
	if(fprint(fd, "%d", pfd) < 0){
		if(chatty9p)
			fprint(2, "write fails: %r\n");
		close(fd);
		return -1;
	}
	if(chatty9p)
		fprint(2, "postfd successful\n");
	return 0;
}

